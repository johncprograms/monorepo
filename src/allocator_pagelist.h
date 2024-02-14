// Copyright (c) John A. Carlos Jr., all rights reserved.

#define PAGELIST_PAGE   pagelist_page_t<Allocation>
#define PAGELIST        tpagelist_t<Allocator, Allocation>

// allocated with the rest of the page, this is just the header at the start.
// the rest of the page memory follows immediately afterwards.
// note that size and top don't account for the pagelist_page_t header itself.
template< typename Allocation = allocation_heap_or_virtual_t >
struct
pagelist_page_t
{
  PAGELIST_PAGE* prev;
  PAGELIST_PAGE* next;
  idx_t top;
  idx_t size;
  Allocation allocn;
};

template< typename Allocator = allocator_heap_or_virtual_t, typename Allocation = allocation_heap_or_virtual_t >
struct
tpagelist_t
{
  PAGELIST_PAGE* current_page;
  idx_t userbytes;
  Allocator alloc;
};

using pagelist_t = tpagelist_t<>;

TA Inl void
Zero( PAGELIST& list )
{
  list.current_page = 0;
  list.userbytes = 0;
}

TA Inl void
Init( PAGELIST& list, idx_t initial_size, Allocator alloc = {} )
{
  Zero( list );
  auto str = AllocString<u8, Allocator, Allocation>( initial_size + sizeof( PAGELIST_PAGE ), list.alloc );
  auto newpage = Cast( PAGELIST_PAGE*, str.mem );
  newpage->prev = 0;
  newpage->next = 0;
  newpage->top = 0;
  newpage->size = initial_size;
  newpage->allocn = str.allocn;

  list.current_page = newpage;
  list.alloc = str.alloc;
}

// each pagelist_page_t is a string_t allocation, but we store the fields in a different way.
// this lets us reconstruct the string_t we allocated, so we can free it.
TA Inl tstring_t<u8, Allocator, Allocation>
_StringFromPage( PAGELIST& list, PAGELIST_PAGE* page )
{
  tstring_t<u8, Allocator, Allocation> r;
  r.mem = Cast( u8*, page );
  r.len = page->size + sizeof( PAGELIST_PAGE );
  r.alloc = list.alloc;
  r.allocn = page->allocn;
  return r;
}

TA Inl void
FreePage( PAGELIST& list, PAGELIST_PAGE* page )
{
  auto str = _StringFromPage( list, page );
  Free( str );
}

TA Inl void
Kill( PAGELIST& list )
{
  auto page = list.current_page;
  AssertCrash( !page->next );
  auto prev = page->prev;
  FreePage( list, page );
  while( prev ) {
    page = prev;
    prev = page->prev;
    FreePage( list, page );
  }
  Zero( list );
}

TA Inl void
Reset( PAGELIST& list )
{
  auto page = list.current_page;
  AssertCrash( !page->next );
  while( page->prev ) {
    auto prev = page->prev;
    FreePage( list, page );
    page = prev;
  }
  page->next = 0;
  page->top = 0;
  // leave page->size alone.
  list.current_page = page;
  list.userbytes = 0;
}

TA Inl u8*
AddBackBytes( PAGELIST& list, idx_t align_pow2, idx_t len )
{
  list.userbytes += len;

  auto page = list.current_page;
  auto top = page->top;
  auto size = page->size;

  // align newtop
  auto newtop = RoundUpToMultipleOfPowerOf2( top, align_pow2 );

  // return newtop if possible.
  if( ( newtop <= size )  &&  ( len <= size - newtop ) ) {
    auto r = Cast( u8*, page ) + sizeof( PAGELIST_PAGE ) + newtop;
    page->top = newtop + len;
    return r;
  }

  // alloc a new page.
  // double page size each time at least, since we usually start with a small size.
  // if you want large fixed-size pages, pagearray is probably better suited.
  auto newsize = 2 * size;
  AssertCrash( newsize );
  while( newsize < len + align_pow2 + sizeof( PAGELIST_PAGE ) ) {
    newsize *= 2;
  }
  auto str = AllocString<u8, Allocator, Allocation>( newsize + sizeof( PAGELIST_PAGE ), list.alloc );
  auto newpage = Cast( PAGELIST_PAGE*, str.mem );
  auto oldpage = page;
  oldpage->next = newpage;
  newpage->prev = oldpage;
  newpage->next = 0;
  newpage->top = 0;
  newpage->size = newsize;
  newpage->allocn = str.allocn;
  list.current_page = newpage;

  // align pagetop
  newtop = RoundUpToMultipleOfPowerOf2( newpage->top, align_pow2 );

  // return pagetop.
  auto r = Cast( u8*, newpage ) + sizeof( PAGELIST_PAGE ) + newtop;
  newpage->top = newtop + len;

  AssertCrash( newpage->top <= newpage->size );

  return r;
}

#define AddPagelist( list, type, align_pow2, count ) \
  Cast( type*, AddBackBytes( list, align_pow2, count * sizeof( type ) ) )

TEA Inl tslice_t<T>
_AddPagelistSlice(
  PAGELIST& list,
  idx_t align_pow2,
  idx_t count
  )
{
  tslice_t<T> r;
  r.len = count;
  r.mem = AddPagelist( list, T, align_pow2, count );
  return r;
}

#define AddPagelistSlice( list, type, align_pow2, count ) \
  _AddPagelistSlice<type>( list, align_pow2, count )

TEA Inl tslice32_t<T>
_AddPagelistSlice32(
  PAGELIST& list,
  u32 align_pow2,
  u32 count
  )
{
  tslice32_t<T> r;
  r.len = count;
  r.mem = AddPagelist( list, T, align_pow2, count );
  return r;
}

#define AddPagelistSlice32( list, type, align_pow2, count ) \
  _AddPagelistSlice32<type>( list, align_pow2, count )



// ============================================================================
// PAGELIST ALLOCATOR

struct allocator_pagelist_t
{
  pagelist_t* mem;

  ForceInl allocator_pagelist_t()
  {
    mem = 0;
  }
  ForceInl allocator_pagelist_t( pagelist_t* mem_ )
  {
    mem = mem_;
  }
};
struct allocation_pagelist_t
{
  // Nothing to store per-allocation.
};
Templ Inl T* Allocate( allocator_pagelist_t& alloc, allocation_pagelist_t& allocn, idx_t num_elements, idx_t alignment_pow2 = DEFAULT_ALIGN )
{
  return AddPagelist( *alloc.mem, T, alignment_pow2, num_elements );
}
Templ Inl T* Reallocate( allocator_pagelist_t& alloc, allocation_pagelist_t& allocn, T* oldmem, idx_t oldlen, idx_t newlen, idx_t alignment_pow2 = DEFAULT_ALIGN )
{
  auto newmem = Allocate<T>( alloc, allocn, newlen, alignment_pow2 );
  TMove( newmem, oldmem, MIN( oldlen, newlen ) );
  return newmem;
}
Templ Inl tslice_t<T> AllocateSlice( allocator_pagelist_t& alloc, allocation_pagelist_t& allocn, idx_t num_elements, idx_t alignment_pow2 = DEFAULT_ALIGN )
{
  return AddPagelistSlice( *alloc.mem, T, alignment_pow2, num_elements );
}
Templ Inl tslice_t<T> AllocateSlice32( allocator_pagelist_t& alloc, allocation_pagelist_t& allocn, idx_t num_elements, idx_t alignment_pow2 = DEFAULT_ALIGN )
{
  return AddPagelistSlice32( *alloc.mem, T, alignment_pow2, num_elements );
}
Inl void Reset( allocator_pagelist_t& alloc )
{
  return Reset( *alloc.mem );
}









RegisterTest([]()
{
  struct
  bytes16408_t
  {
    u8 mem[16408];
  };

  {
    pagelist_t list;
    Init( list, 4 );

    auto r0 = AddPagelist( list, u8, 1, 4 );
    Memmove( r0, "1234", 4 );
    AssertCrash( MemEqual( r0, "1234", 4 ) );

    auto r1 = AddPagelist( list, u8, 1, 2 );
    Memmove( r1, "56", 2 );
    AssertCrash( MemEqual( r1, "56", 2 ) );

    Kill( list );
  }

  {
    pagelist_t list;
    Init( list, 8064 );

    auto r0 = AddPagelist( list, u8, 1, 3 );
    Memmove( r0, "123", 3 );
    AssertCrash( MemEqual( r0, "123", 3 ) );

    auto r1 = AddPagelist( list, bytes16408_t, 16384, 1 );
    Memzero( r1->mem, sizeof( bytes16408_t ) );
    Memmove( r1, "56", 2 );
    AssertCrash( MemEqual( r1, "56", 2 ) );

    Kill( list );
  }
});



#if 0 // This doesn't work as-is, since each page doesn't maintain a 'top' count.

  // test code to exhibit the problem:
  {
    pagelist_t list;
    Init( list, 4 );

    auto r0 = AddPagelist( list, u8, 1, 6 );
    Memmove( r0, "123456", 6 );
    AssertCrash( MemEqual( r0, "123456", 6 ) );

    auto str = StringFromPagelist( list );
    AssertCrash( MemEqual( ML( str ), "123456", 6 ) );
    Free( str );

    Kill( list );
  }

  Inl string_t
  StringFromPagelist( pagelist_t& list )
  {
    // we probably don't want to make a contiguous buffer from a pagelist, unless we've only ever allocated u8s.
    AssertCrash( list.max_aligned == 1 );

    string_t r;
    Alloc( r, list.userbytes );

    auto page = list.current_page;
    auto header = _GetHeader( page );
    AssertCrash( !header->next );

    // walk back to the first page.
    while( header->prev ) {
      page = header->prev;
      header = _GetHeader( page );
    }

    // walk forw to the last page, copying mem along the way.
    idx_t count = 0;
    while( page ) {
      auto current_pagetop = header->next  ?  header->pagesize  :  list.pagetop;
      auto userbytes_in_page = current_pagetop - sizeof( pagelistheader_t );
      Memmove( r.mem + count, header + 1, userbytes_in_page );
      count += userbytes_in_page;

      page = header->next;
      header = _GetHeader( page );
    }
    AssertCrash( count == list.userbytes );
    return r;
  }

#endif
