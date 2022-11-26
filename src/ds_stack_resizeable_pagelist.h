// Copyright (c) John A. Carlos Jr., all rights reserved.

#define STACKRESIZEABLEPAGE   stack_resizeable_page_t<T, Allocation>
// NOTE: only cast to this; you shouldn't ever instantiate one of these.
template< typename T, typename Allocation = allocation_heap_or_virtual_t >
struct
stack_resizeable_page_t
{
  STACKRESIZEABLEPAGE* prev;
  STACKRESIZEABLEPAGE* next;

  // TODO: we need to store Allocator here, since it's a unique allocation.
  
  // TODO: use a fixed offset instead of storing an offset in effect here.
  T* mem; // points to memory immediately following this struct. important for proper alignment.

  // these are in units of T
  // so, multiply by sizeof( T ) to get them in terms of bytes.
  idx_t capacity;
  idx_t len;
  
  Allocation allocn;
};

#define STACKRESIZEABLEPAGELIST   stack_resizeable_pagelist_t<T, Allocator, Allocation>
TEA struct
stack_resizeable_pagelist_t
{
  STACKRESIZEABLEPAGE* first_page;
  STACKRESIZEABLEPAGE* current_page;
  idx_t hdrbytes;
  idx_t totallen;
  Allocator alloc;
};
TEA Inl void
Zero( STACKRESIZEABLEPAGELIST& list )
{
  list.first_page = 0;
  list.current_page = 0;
  list.hdrbytes = 0;
  list.totallen = 0;
  list.alloc = {};
}
TEA Inl void
Init( STACKRESIZEABLEPAGELIST& list, idx_t nelems_capacity, Allocator alloc = {} )
{
  AssertCrash( nelems_capacity );

  Zero( list );
  list.hdrbytes = RoundUpToMultipleOfN( sizeof( STACKRESIZEABLEPAGE ), sizeof( T ) );

  auto str = AllocString<u8>( list.hdrbytes + nelems_capacity * sizeof( T ), alloc );
  auto page = Cast( STACKRESIZEABLEPAGE*, str.mem );
  page->prev = 0;
  page->next = 0;
  page->mem = Cast( T*, Cast( u8*, page ) + list.hdrbytes );
  page->capacity = nelems_capacity;
  page->len = 0;
  page->allocn = str.allocn;

  list.alloc = str.alloc;
  list.current_page = page;
  list.first_page = list.current_page;
}
// each page is a string_t allocation, but we store the fields in a different way.
// this lets us reconstruct the string_t we allocated, so we can free it.
TEA Inl tstring_t<u8, Allocator>
_StringFromPage( STACKRESIZEABLEPAGELIST& list, STACKRESIZEABLEPAGE* page )
{
  tstring_t<u8, Allocator> r;
  r.mem = Cast( u8*, page );
  auto hdrbytes = RoundUpToMultipleOfN( sizeof( STACKRESIZEABLEPAGE ), sizeof( T ) );
  r.len = hdrbytes + page->capacity * sizeof( T );
  r.alloc = list.alloc;
  r.allocn = page->allocn;
  return r;
}
TEA Inl void
FreePage( STACKRESIZEABLEPAGELIST& list, STACKRESIZEABLEPAGE* page )
{
  auto str = _StringFromPage( list, page );
  Free( str );
}

TEA Inl void
Kill( STACKRESIZEABLEPAGELIST& list )
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

TEA Inl void
Reset( STACKRESIZEABLEPAGELIST& list )
{
  auto page = list.current_page;
  AssertCrash( !page->next );
  while( page->prev ) {
    auto prev = page->prev;
    FreePage( list, page );
    page = prev;
  }
  page->next = 0;
  page->len = 0;
  list.current_page = page;
  list.totallen = 0;
}

TEA Inl T*
CurrentPage_AddBack( STACKRESIZEABLEPAGELIST& list, idx_t nelems )
{
  auto page = list.current_page;
  AssertCrash( page->len + nelems <= page->capacity );
  auto r = page->mem + page->len;
  page->len += nelems;
  list.totallen += nelems;
  return r;
}

TEA Inl bool
CurrentPage_HasRoomFor( STACKRESIZEABLEPAGELIST& list, idx_t nelems )
{
  auto page = list.current_page;
  auto r = page->len + nelems <= page->capacity;
  return r;
}

TEA Inl idx_t
CurrentPage_Capacity( STACKRESIZEABLEPAGELIST& list )
{
  auto page = list.current_page;
  return page->capacity;
}

TEA Inl void
AddNewPage( STACKRESIZEABLEPAGELIST& list, idx_t nelems_capacity )
{
  AssertCrash( nelems_capacity );

  auto str = AllocString( list.hdrbytes + nelems_capacity * sizeof( T ), list.alloc );
  auto page = Cast( STACKRESIZEABLEPAGE*, str.mem );
  page->next = 0;
  page->mem = Cast( T*, Cast( u8*, page ) + list.hdrbytes );
  page->capacity = nelems_capacity;
  page->len = 0;
  page->allocn = str.allocn;

  page->prev = list.current_page;
  list.current_page->next = page;

  list.current_page = page;
}

TEA Inl T*
AddBack( STACKRESIZEABLEPAGELIST& list, idx_t nelems = 1 )
{
  if( !CurrentPage_HasRoomFor( list, nelems ) ) {
    auto new_capacity = 2 * CurrentPage_Capacity( list );
    while( new_capacity < nelems ) {
      new_capacity *= 2;
    }
    AddNewPage( list, new_capacity );
  }
  auto elem = CurrentPage_AddBack( list, nelems );
  return elem;
}






#define PAGERELATIVEPOS   pagerelativepos_t<T, Allocation>
template< typename T, typename Allocation = allocation_heap_or_virtual_t >
struct
pagerelativepos_t
{
  STACKRESIZEABLEPAGE* page;
  idx_t idx;
};

TEA Inl PAGERELATIVEPOS
MakeIteratorAtLinearIndex( STACKRESIZEABLEPAGELIST& list, idx_t idx )
{
  AssertCrash( idx < list.totallen );
  ForNext( page, list.first_page ) {
    if( idx < page->len ) {
      PAGERELATIVEPOS pos;
      pos.page = page;
      pos.idx = idx;
      return pos;
    }
    idx -= page->len;
  }
  UnreachableCrash();
  return {};
}

TEA Inl T*
GetElemAtIterator( STACKRESIZEABLEPAGELIST& list, PAGERELATIVEPOS pos )
{
  auto elem = pos.page->mem + pos.idx;
  return elem;
}

template< typename T, typename Allocation = allocation_heap_or_virtual_t >
Inl bool
CanIterate( PAGERELATIVEPOS pos )
{
  auto r = pos.page;
  return r;
}

TEA Inl PAGERELATIVEPOS
IteratorMoveR( STACKRESIZEABLEPAGELIST& list, PAGERELATIVEPOS pos, idx_t nelems = 1 )
{
  auto r = pos;
  r.idx += nelems;
  while( r.idx >= r.page->len ) {
    r.idx -= r.page->len;
    r.page = r.page->next;
    if( !r.page ) {
      // we allow pos to go one past the last element; same as integer indices.
      AssertCrash( !r.idx );
      break;
    }
  }
  return r;
}

TEA Inl T*
LookupElemByLinearIndex( STACKRESIZEABLEPAGELIST& list, idx_t idx )
{
  AssertCrash( idx < list.totallen );
  ForNext( page, list.first_page ) {
    if( idx < page->len ) {
      auto r = page->mem + idx;
      return r;
    }
    idx -= page->len;
  }
  UnreachableCrash();
  return 0;
}


// TODO: we really need an easier way of iterating pagearrays

#if 0

  Templ struct
  pagearray_iter_t
  {
    STACKRESIZEABLEPAGE* page;
    idx_t idx;
  };

  #define
  for(
    auto pa_iter = PageArrayIteratorMake( list, start ), pa_end = PageArrayIteratorMake( list, end );
    PageArrayIteratorLessThan( list, pa_iter, pa_end );
    pa_iter = PageArrayIteratorR( list, pa_iter, 1 )
    )

  Templ Inl PAGERELATIVEPOS
  PageArrayIteratorMake

  Templ Inl bool
  IteratorLessThan( STACKRESIZEABLEPAGELIST& list, PAGERELATIVEPOS pos, idx_t end )
  {
  }

#endif

