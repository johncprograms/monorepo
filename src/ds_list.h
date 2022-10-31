// Copyright (c) John A. Carlos Jr., all rights reserved.

#define LISTELEM   listelem_t<T, Allocation>
template< typename T, typename Allocation = allocation_heap_or_virtual_t >
struct
listelem_t
{
  Allocation allocn;
  LISTELEM* prev;
  LISTELEM* next;
  T value;
};
TEA Inl LISTELEM*
AllocateListelem( Allocator& alloc )
{
  Allocation allocn = {};
  auto elem = Allocate<LISTELEM>( alloc, allocn, 1 );
  elem->allocn = allocn;
  elem->prev = 0;
  elem->next = 0;
  return elem;
}

#define LIST   list_t<T, Allocator, Allocation>
// note this is zero-initialized
TEA struct
list_t
{
  idx_t len; // number of elements currently in the list.
  LISTELEM* first;
  LISTELEM* last;
  Allocator alloc;
};
TEA Inl void
Zero( LIST& list )
{
  list.len = 0;
  list.first = 0;
  list.last = 0;
}
TEA Inl void
_InitialInsert( LIST& list, LISTELEM* elem )
{
  AssertCrash( !list.len );
  elem->prev = 0;
  elem->next = 0;
  list.first = elem;
  list.last = elem;
  list.len = 1;
}
TEA Inl void
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
TEA Inl void
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
TEA Inl void
InsertFirst( LIST& list, LISTELEM* elem )
{
  InsertBefore( list, elem, list.first );
}
TEA Inl void
InsertLast( LIST& list, LISTELEM* elem )
{
  InsertAfter( list, elem, list.last );
}
TEA Inl void
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
TEA Inl void
RemFirst( LIST& list )
{
  Rem( list, list.first );
}
TEA Inl void
RemLast( LIST& list )
{
  Rem( list, list.last );
}


#define LISTWALLOC   listwalloc_t<T, Allocator, Allocation>

// TODO: make this have a list_t field instead of duplicating the code?
TEA struct
listwalloc_t
{
  idx_t len; // number of elements currently in the list.
  LISTELEM* first;
  LISTELEM* last;
  stack_nonresizeable_stack_t<LISTELEM*, 128> free_elems; // indices into listwalloc_t.elems.
  Allocator alloc;
};
TEA Inl void
Init( LISTWALLOC& list, Allocator alloc = {} )
{
  list.len = 0;
  list.first = 0;
  list.last = 0;
  list.free_elems.len = 0;
  list.alloc = alloc;
}
TEA Inl void
Kill( LISTWALLOC& list )
{
  list.len = 0;
  list.first = 0;
  list.last = 0;
  list.free_elems.len = 0;
  list.alloc = {};
}
TEA void
Clear( LISTWALLOC& list )
{
  list.len = 0;
  list.first = 0;
  list.last = 0;
  list.free_elems.len = 0;
}
TEA Inl LISTELEM*
_GetNewElem( LISTWALLOC& list )
{
  LISTELEM* elem;
  if( list.free_elems.len ) {
    elem = list.free_elems.mem[ list.free_elems.len - 1 ];
    RemBack( list.free_elems );
  }
  else {
    // TODO: should allow customization of the alignment here!
    elem = AllocateListelem<T, Allocator, Allocation>( list.alloc );
  }
  AssertCrash( elem );
  return elem;
}
TEA Inl void
_InitialInsert( LISTWALLOC& list, LISTELEM* elem )
{
  AssertCrash( !list.len );
  elem->prev = 0;
  elem->next = 0;
  list.first = elem;
  list.last = elem;
  list.len = 1;
}
TEA Inl void
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
TEA Inl void
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
TEA Inl void
InsertFirst( LISTWALLOC& list, LISTELEM* elem )
{
  InsertBefore( list, elem, list.first );
}
TEA Inl void
InsertLast( LISTWALLOC& list, LISTELEM* elem )
{
  InsertAfter( list, elem, list.last );
}
TEA Inl LISTELEM*
AddBefore( LISTWALLOC& list, LISTELEM* place )
{
  auto elem = _GetNewElem( list );
  InsertBefore( list, elem, place );
  return elem;
}
TEA Inl LISTELEM*
AddAfter( LISTWALLOC& list, LISTELEM* place )
{
  auto elem = _GetNewElem( list );
  InsertAfter( list, elem, place );
  return elem;
}
TEA Inl LISTELEM*
AddFirst( LISTWALLOC& list )
{
  return AddBefore( list, list.first );
}
TEA Inl LISTELEM*
AddLast( LISTWALLOC& list )
{
  return AddAfter( list, list.last );
}
TEA Inl void
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
TEA Inl void
RemFirst( LISTWALLOC& list )
{
  Rem( list, list.first );
}
TEA Inl void
RemLast( LISTWALLOC& list )
{
  Rem( list, list.last );
}
TEA Inl void
Reclaim( LISTWALLOC& list, LISTELEM* elem )
{
  if( list.free_elems.len < Capacity( list.free_elems ) ) {
    *AddBack( list.free_elems ) = elem;
  }
}
