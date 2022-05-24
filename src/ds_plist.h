// Copyright (c) John A. Carlos Jr., all rights reserved.

// allocated with the rest of the page, this is just the header at the start.
// the rest of the page memory follows immediately afterwards.
// note that size and top don't account for the plpage_t header itself.
struct
plpage_t
{
  plpage_t* prev;
  plpage_t* next;
  idx_t top;
  idx_t size;
  alloctype_t alloctype;
};

struct
plist_t
{
  plpage_t* current_page;
  idx_t userbytes;
};


Inl void
Zero( plist_t& list )
{
  list.current_page = 0;
  list.userbytes = 0;
}

Inl void
Init( plist_t& list, idx_t initial_size )
{
  Zero( list );
  string_t str;
  Alloc( str, initial_size + sizeof( plpage_t ) );
  auto newpage = Cast( plpage_t*, str.mem );
  newpage->prev = 0;
  newpage->next = 0;
  newpage->top = 0;
  newpage->size = initial_size;
  newpage->alloctype = str.alloctype;

  list.current_page = newpage;
}

// each plpage_t is a string_t allocation, but we store the fields in a different way.
// this lets us reconstruct the string_t we allocated, so we can free it.
Inl string_t
_StringFromPage( plpage_t* page )
{
  string_t r;
  r.mem = Cast( u8*, page );
  r.len = page->size + sizeof( plpage_t );
  r.alloctype = page->alloctype;
  return r;
}

Inl void
FreePage( plpage_t* page )
{
  auto str = _StringFromPage( page );
  Free( str );
}

Inl void
Kill( plist_t& list )
{
  auto page = list.current_page;
  AssertCrash( !page->next );
  auto prev = page->prev;
  FreePage( page );
  while( prev ) {
    page = prev;
    prev = page->prev;
    FreePage( page );
  }
  Zero( list );
}

Inl void
Reset( plist_t& list )
{
  auto page = list.current_page;
  AssertCrash( !page->next );
  while( page->prev ) {
    auto prev = page->prev;
    FreePage( page );
    page = prev;
  }
  page->next = 0;
  page->top = 0;
  // leave page->size alone.
  list.current_page = page;
  list.userbytes = 0;
}

#define AddPlist( list, type, align_pow2, count ) \
  Cast( type*, AddBackBytes( list, align_pow2, count * sizeof( type ) ) )

Templ Inl tslice_t<T>
_AddPlistSlice(
  plist_t& list,
  idx_t align_pow2,
  idx_t count
  )
{
  tslice_t<T> r;
  r.len = count;
  r.mem = AddPlist( list, T, align_pow2, count );
  return r;
}

#define AddPlistSlice( list, type, align_pow2, count ) \
  _AddPlistSlice<type>( list, align_pow2, count )

Templ Inl tslice32_t<T>
_AddPlistSlice32(
  plist_t& list,
  u32 align_pow2,
  u32 count
  )
{
  tslice32_t<T> r;
  r.len = count;
  r.mem = AddPlist( list, T, align_pow2, count );
  return r;
}

#define AddPlistSlice32( list, type, align_pow2, count ) \
  _AddPlistSlice32<type>( list, align_pow2, count )

Inl u8*
AddBackBytes( plist_t& list, idx_t align_pow2, idx_t len )
{
  list.userbytes += len;

  auto page = list.current_page;
  auto top = page->top;
  auto size = page->size;

  // align newtop
  auto newtop = RoundUpToMultipleOfPowerOf2( top, align_pow2 );

  // return newtop if possible.
  if( ( newtop <= size )  &&  ( len <= size - newtop ) ) {
    auto r = Cast( u8*, page ) + sizeof( plpage_t ) + newtop;
    page->top = newtop + len;
    return r;
  }

  // alloc a new page.
  // double page size each time at least, since we usually start with a small size.
  // if you want large fixed-size pages, pagearray is probably better suited.
  auto newsize = 2 * size;
  AssertCrash( newsize );
  while( newsize < len + align_pow2 + sizeof( plpage_t ) ) {
    newsize *= 2;
  }
  string_t str;
  Alloc( str, newsize + sizeof( plpage_t ) );
  auto newpage = Cast( plpage_t*, str.mem );
  auto oldpage = page;
  oldpage->next = newpage;
  newpage->prev = oldpage;
  newpage->next = 0;
  newpage->top = 0;
  newpage->size = newsize;
  newpage->alloctype = str.alloctype;
  list.current_page = newpage;

  // align pagetop
  newtop = RoundUpToMultipleOfPowerOf2( newpage->top, align_pow2 );

  // return pagetop.
  auto r = Cast( u8*, newpage ) + sizeof( plpage_t ) + newtop;
  newpage->top = newtop + len;

  AssertCrash( newpage->top <= newpage->size );

  return r;
}



Inl void
TestPlist()
{
  struct
  bytes16408_t
  {
    u8 mem[16408];
  };

  {
    plist_t list;
    Init( list, 4 );

    auto r0 = AddPlist( list, u8, 1, 4 );
    Memmove( r0, "1234", 4 );
    AssertCrash( MemEqual( r0, "1234", 4 ) );

    auto r1 = AddPlist( list, u8, 1, 2 );
    Memmove( r1, "56", 2 );
    AssertCrash( MemEqual( r1, "56", 2 ) );

    Kill( list );
  }

  {
    plist_t list;
    Init( list, 8064 );

    auto r0 = AddPlist( list, u8, 1, 3 );
    Memmove( r0, "123", 3 );
    AssertCrash( MemEqual( r0, "123", 3 ) );

    auto r1 = AddPlist( list, bytes16408_t, 16384, 1 );
    Memzero( r1->mem, sizeof( bytes16408_t ) );
    Memmove( r1, "56", 2 );
    AssertCrash( MemEqual( r1, "56", 2 ) );

    Kill( list );
  }
}



#if 0 // This doesn't work as-is, since each page doesn't maintain a 'top' count.

  // test code to exhibit the problem:
  {
    plist_t list;
    Init( list, 4 );

    auto r0 = AddPlist( list, u8, 1, 6 );
    Memmove( r0, "123456", 6 );
    AssertCrash( MemEqual( r0, "123456", 6 ) );

    auto str = StringFromPlist( list );
    AssertCrash( MemEqual( ML( str ), "123456", 6 ) );
    Free( str );

    Kill( list );
  }

  Inl string_t
  StringFromPlist( plist_t& list )
  {
    // we probably don't want to make a contiguous buffer from a plist, unless we've only ever allocated u8s.
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
      auto userbytes_in_page = current_pagetop - sizeof( plistheader_t );
      Memmove( r.mem + count, header + 1, userbytes_in_page );
      count += userbytes_in_page;

      page = header->next;
      header = _GetHeader( page );
    }
    AssertCrash( count == list.userbytes );
    return r;
  }

#endif
