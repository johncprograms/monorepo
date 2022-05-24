// Copyright (c) John A. Carlos Jr., all rights reserved.

// NOTE: only cast to this; you shouldn't ever instantiate one of these.
Templ struct
pagearray_elem_t
{
  pagearray_elem_t<T>* prev;
  pagearray_elem_t<T>* next;

  T* mem; // points to memory immediately following this struct. important for proper alignment.

  // these are in units of T
  // so, multiply by sizeof( T ) to get them in terms of bytes.
  idx_t capacity;
  idx_t len;
  alloctype_t alloctype;
};

Templ struct
pagearray_t
{
  pagearray_elem_t<T>* first_page;
  pagearray_elem_t<T>* current_page;
  idx_t hdrbytes;
  idx_t totallen;
};

Templ Inl void
Zero( pagearray_t<T>& list )
{
  list.first_page = 0;
  list.current_page = 0;
  list.hdrbytes = 0;
  list.totallen = 0;
}

Templ Inl void
Init( pagearray_t<T>& list, idx_t nelems_capacity )
{
  AssertCrash( nelems_capacity );

  Zero( list );
  list.hdrbytes = RoundUpToMultipleOfN( sizeof( pagearray_elem_t<T> ), sizeof( T ) );

  string_t str;
  Alloc( str, list.hdrbytes + nelems_capacity * sizeof( T ) );
  auto page = Cast( pagearray_elem_t<T>*, str.mem );
  page->prev = 0;
  page->next = 0;
  page->mem = Cast( T*, Cast( u8*, page ) + list.hdrbytes );
  page->capacity = nelems_capacity;
  page->len = 0;
  page->alloctype = str.alloctype;

  list.current_page = page;
  list.first_page = list.current_page;
}

// each page is a string_t allocation, but we store the fields in a different way.
// this lets us reconstruct the string_t we allocated, so we can free it.
Templ Inl string_t
_StringFromPage( pagearray_elem_t<T>* page )
{
  string_t r;
  r.mem = Cast( u8*, page );
  auto hdrbytes = RoundUpToMultipleOfN( sizeof( pagearray_elem_t<T> ), sizeof( T ) );
  r.len = hdrbytes + page->capacity * sizeof( T );
  r.alloctype = page->alloctype;
  return r;
}
Templ Inl void
FreePage( pagearray_elem_t<T>* page )
{
  auto str = _StringFromPage( page );
  Free( str );
}

Templ Inl void
Kill( pagearray_t<T>& list )
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

Templ Inl void
Reset( pagearray_t<T>& list )
{
  auto page = list.current_page;
  AssertCrash( !page->next );
  while( page->prev ) {
    auto prev = page->prev;
    FreePage( page );
    page = prev;
  }
  page->next = 0;
  page->len = 0;
  list.current_page = page;
  list.totallen = 0;
}

Templ Inl T*
CurrentPage_AddBack( pagearray_t<T>& list, idx_t nelems )
{
  auto page = list.current_page;
  AssertCrash( page->len + nelems <= page->capacity );
  auto r = page->mem + page->len;
  page->len += nelems;
  list.totallen += nelems;
  return r;
}

Templ Inl bool
CurrentPage_HasRoomFor( pagearray_t<T>& list, idx_t nelems )
{
  auto page = list.current_page;
  auto r = page->len + nelems <= page->capacity;
  return r;
}

Templ Inl idx_t
CurrentPage_Capacity( pagearray_t<T>& list )
{
  auto page = list.current_page;
  return page->capacity;
}

Templ Inl void
AddNewPage( pagearray_t<T>& list, idx_t nelems_capacity )
{
  AssertCrash( nelems_capacity );

  string_t str;
  Alloc( str, list.hdrbytes + nelems_capacity * sizeof( T ) );
  auto page = Cast( pagearray_elem_t<T>*, str.mem );
  page->next = 0;
  page->mem = Cast( T*, Cast( u8*, page ) + list.hdrbytes );
  page->capacity = nelems_capacity;
  page->len = 0;
  page->alloctype = str.alloctype;

  page->prev = list.current_page;
  list.current_page->next = page;

  list.current_page = page;
}

Templ Inl T*
AddBack( pagearray_t<T>& list, idx_t nelems = 1 )
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







Templ struct
pagerelativepos_t
{
  pagearray_elem_t<T>* page;
  idx_t idx;
};

Templ Inl pagerelativepos_t<T>
MakeIteratorAtLinearIndex( pagearray_t<T>& list, idx_t idx )
{
  AssertCrash( idx < list.totallen );
  ForNext( page, list.first_page ) {
    if( idx < page->len ) {
      pagerelativepos_t<T> pos;
      pos.page = page;
      pos.idx = idx;
      return pos;
    }
    idx -= page->len;
  }
  UnreachableCrash();
  return {};
}

Templ Inl T*
GetElemAtIterator( pagearray_t<T>& list, pagerelativepos_t<T> pos )
{
  auto elem = pos.page->mem + pos.idx;
  return elem;
}

Templ Inl bool
CanIterate( pagerelativepos_t<T> pos )
{
  auto r = pos.page;
  return r;
}

Templ Inl pagerelativepos_t<T>
IteratorMoveR( pagearray_t<T>& list, pagerelativepos_t<T> pos, idx_t nelems = 1 )
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

Templ Inl T*
LookupElemByLinearIndex( pagearray_t<T>& list, idx_t idx )
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
    pagearray_elem_t<T>* page;
    idx_t idx;
  };

  #define
  for(
    auto pa_iter = PageArrayIteratorMake( list, start ), pa_end = PageArrayIteratorMake( list, end );
    PageArrayIteratorLessThan( list, pa_iter, pa_end );
    pa_iter = PageArrayIteratorR( list, pa_iter, 1 )
    )

  Templ Inl pagerelativepos_t<T>
  PageArrayIteratorMake

  Templ Inl bool
  IteratorLessThan( pagearray_t<T>& list, pagerelativepos_t<T> pos, idx_t end )
  {
  }

#endif

