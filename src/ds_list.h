// Copyright (c) John A. Carlos Jr., all rights reserved.

#define LISTELEM   listelem_t<T>
Templ struct
listelem_t
{
  alloctype_t allocn;
  LISTELEM* prev;
  LISTELEM* next;
  T value;
};
Templ Inl LISTELEM*
AllocateListelem()
{
  alloctype_t allocn = {};
  auto elem = Allocate<LISTELEM>( &allocn, 1 );
  elem->allocn = allocn;
  elem->prev = 0;
  elem->next = 0;
  return elem;
}

#define LIST   list_t<T>
// note this is zero-initialized
Templ struct
list_t
{
  idx_t len; // number of elements currently in the list.
  LISTELEM* first;
  LISTELEM* last;
};
Templ Inl void
Zero( LIST& list )
{
  list.len = 0;
  list.first = 0;
  list.last = 0;
}
Templ Inl void
_InitialInsert( LIST& list, LISTELEM* elem )
{
  AssertCrash( !list.len );
  elem->prev = 0;
  elem->next = 0;
  list.first = elem;
  list.last = elem;
  list.len = 1;
}
Templ Inl void
InsertBefore( LIST& list, LISTELEM* elem, LISTELEM* place )
{
  AssertCrash( elem );
  if( !list.len ) {
    AssertCrash( !place );
    _InitialInsert( list, elem );
  }
  else {
    AssertCrash( list.len );
    AssertCrash( place );
    auto prev = place->prev;
    auto next = place;
    elem->prev = prev;
    elem->next = next;
    if( prev ) {
      prev->next = elem;
    }
    next->prev = elem;
    if( list.first == place ) {
      list.first = elem;
    }
    list.len += 1;
  }
  AssertCrash( elem->prev != elem  &&  elem->next != elem );
}
Templ Inl void
InsertAfter( LIST& list, LISTELEM* elem, LISTELEM* place )
{
  AssertCrash( elem );
  if( !list.len ) {
    AssertCrash( !place );
    _InitialInsert( list, elem );
  }
  else {
    AssertCrash( list.len );
    AssertCrash( place );
    auto prev = place;
    auto next = place->next;
    elem->prev = prev;
    elem->next = next;
    prev->next = elem;
    if( next ) {
      next->prev = elem;
    }
    if( list.last == place ) {
      list.last = elem;
    }
    list.len += 1;
  }
  AssertCrash( elem->prev != elem  &&  elem->next != elem );
}
Templ Inl void
InsertFirst( LIST& list, LISTELEM* elem )
{
  InsertBefore( list, elem, list.first );
}
Templ Inl void
InsertLast( LIST& list, LISTELEM* elem )
{
  InsertAfter( list, elem, list.last );
}
Templ Inl void
Rem( LIST& list, LISTELEM* elem )
{
  AssertCrash( list.len );
  auto prev = elem->prev;
  auto next = elem->next;
  elem->next = 0;
  elem->prev = 0;
  if( prev ) {
    prev->next = next;
  }
  if( next ) {
    next->prev = prev;
  }
  if( list.first == elem ) {
    list.first = next;
  }
  if( list.last == elem ) {
    list.last = prev;
  }
  list.len -= 1;
}
Templ Inl void
RemFirst( LIST& list )
{
  Rem( list, list.first );
}
Templ Inl void
RemLast( LIST& list )
{
  Rem( list, list.last );
}


#define LISTWALLOC   listwalloc_t<T>

// TODO: make this have a list_t field instead of duplicating the code?
Templ struct
listwalloc_t
{
  idx_t len; // number of elements currently in the list.
  LISTELEM* first;
  LISTELEM* last;
  stack_nonresizeable_stack_t<LISTELEM*, 128> free_elems; // indices into listwalloc_t.elems.
};
Templ Inl void
Init( LISTWALLOC& list )
{
  list.len = 0;
  list.first = 0;
  list.last = 0;
  list.free_elems.len = 0;
}
Templ Inl void
Kill( LISTWALLOC& list )
{
  list.len = 0;
  list.first = 0;
  list.last = 0;
  list.free_elems.len = 0;
}
Templ void
Clear( LISTWALLOC& list )
{
  list.len = 0;
  list.first = 0;
  list.last = 0;
  list.free_elems.len = 0;
}
Templ Inl LISTELEM*
_GetNewElem( LISTWALLOC& list )
{
  LISTELEM* elem;
  if( list.free_elems.len ) {
    elem = list.free_elems.mem[ list.free_elems.len - 1 ];
    RemBack( list.free_elems );
  }
  else {
    // TODO: should allow customization of the alignment here!
    elem = AllocateListelem<T>();
  }
  AssertCrash( elem );
  return elem;
}
Templ Inl void
_InitialInsert( LISTWALLOC& list, LISTELEM* elem )
{
  AssertCrash( !list.len );
  elem->prev = 0;
  elem->next = 0;
  list.first = elem;
  list.last = elem;
  list.len = 1;
}
Templ Inl void
InsertBefore( LISTWALLOC& list, LISTELEM* elem, LISTELEM* place )
{
  AssertCrash( elem );
  if( !list.len ) {
    AssertCrash( !place );
    _InitialInsert( list, elem );
  }
  else {
    AssertCrash( list.len );
    AssertCrash( place );
    auto prev = place->prev;
    auto next = place;
    elem->prev = prev;
    elem->next = next;
    if( prev ) {
      prev->next = elem;
    }
    next->prev = elem;
    if( list.first == place ) {
      list.first = elem;
    }
    list.len += 1;
  }
  AssertCrash( elem->prev != elem  &&  elem->next != elem );
}
Templ Inl void
InsertAfter( LISTWALLOC& list, LISTELEM* elem, LISTELEM* place )
{
  AssertCrash( elem );
  if( !list.len ) {
    AssertCrash( !place );
    _InitialInsert( list, elem );
  }
  else {
    AssertCrash( list.len );
    AssertCrash( place );
    auto prev = place;
    auto next = place->next;
    elem->prev = prev;
    elem->next = next;
    prev->next = elem;
    if( next ) {
      next->prev = elem;
    }
    if( list.last == place ) {
      list.last = elem;
    }
    list.len += 1;
  }
  AssertCrash( elem->prev != elem  &&  elem->next != elem );
}
Templ Inl void
InsertFirst( LISTWALLOC& list, LISTELEM* elem )
{
  InsertBefore( list, elem, list.first );
}
Templ Inl void
InsertLast( LISTWALLOC& list, LISTELEM* elem )
{
  InsertAfter( list, elem, list.last );
}
Templ Inl LISTELEM*
AddBefore( LISTWALLOC& list, LISTELEM* place )
{
  auto elem = _GetNewElem( list );
  InsertBefore( list, elem, place );
  return elem;
}
Templ Inl LISTELEM*
AddAfter( LISTWALLOC& list, LISTELEM* place )
{
  auto elem = _GetNewElem( list );
  InsertAfter( list, elem, place );
  return elem;
}
Templ Inl LISTELEM*
AddFirst( LISTWALLOC& list )
{
  return AddBefore( list, list.first );
}
Templ Inl LISTELEM*
AddLast( LISTWALLOC& list )
{
  return AddAfter( list, list.last );
}
Templ Inl void
Rem( LISTWALLOC& list, LISTELEM* elem )
{
  AssertCrash( list.len );
  auto prev = elem->prev;
  auto next = elem->next;
  elem->next = 0;
  elem->prev = 0;
  if( prev ) {
    prev->next = next;
  }
  if( next ) {
    next->prev = prev;
  }
  if( list.first == elem ) {
    list.first = next;
  }
  if( list.last == elem ) {
    list.last = prev;
  }
  list.len -= 1;
}
Templ Inl void
RemFirst( LISTWALLOC& list )
{
  Rem( list, list.first );
}
Templ Inl void
RemLast( LISTWALLOC& list )
{
  Rem( list, list.last );
}
Templ Inl void
Reclaim( LISTWALLOC& list, LISTELEM* elem )
{
  if( list.free_elems.len < Capacity( list.free_elems ) ) {
    *AddBack( list.free_elems ) = elem;
  }
}
