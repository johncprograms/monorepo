// Copyright (c) John A. Carlos Jr., all rights reserved.

Templ struct
listelem_t
{
  listelem_t<T>* prev;
  listelem_t<T>* next;
  T value;
};


// note this is zero-initialized
Templ struct
list_t
{
  idx_t len; // number of elements currently in the list.
  listelem_t<T>* first;
  listelem_t<T>* last;
};

Templ Inl void
Zero( list_t<T>& list )
{
  list.len = 0;
  list.first = 0;
  list.last = 0;
}

Templ Inl void
_InitialInsert( list_t<T>& list, listelem_t<T>* elem )
{
  AssertCrash( !list.len );
  elem->prev = 0;
  elem->next = 0;
  list.first = elem;
  list.last = elem;
  list.len = 1;
}

Templ Inl void
InsertBefore( list_t<T>& list, listelem_t<T>* elem, listelem_t<T>* place )
{
  AssertCrash( elem );
  if( !list.len ) {
    AssertCrash( !place );
    _InitialInsert( list, elem );
  } else {
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
InsertAfter( list_t<T>& list, listelem_t<T>* elem, listelem_t<T>* place )
{
  AssertCrash( elem );
  if( !list.len ) {
    AssertCrash( !place );
    _InitialInsert( list, elem );
  } else {
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
InsertFirst( list_t<T>& list, listelem_t<T>* elem )
{
  InsertBefore( list, elem, list.first );
}

Templ Inl void
InsertLast( list_t<T>& list, listelem_t<T>* elem )
{
  InsertAfter( list, elem, list.last );
}

Templ Inl void
Rem( list_t<T>& list, listelem_t<T>* elem )
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
RemFirst( list_t<T>& list )
{
  Rem( list, list.first );
}

Templ Inl void
RemLast( list_t<T>& list )
{
  Rem( list, list.last );
}



















Templ struct
listwalloc_t
{
  idx_t len; // number of elements currently in the list.
  listelem_t<T>* first;
  listelem_t<T>* last;
  plist_t* elems;
  embeddedarray_t<listelem_t<T>*, 128> free_elems; // indices into listwalloc_t.elems.
};


Templ Inl void
Init( listwalloc_t<T>& list, plist_t* elems )
{
  list.len = 0;
  list.first = 0;
  list.last = 0;
  list.elems = elems;
  list.free_elems.len = 0;
}

Templ Inl void
Kill( listwalloc_t<T>& list )
{
  list.len = 0;
  list.first = 0;
  list.last = 0;
  list.free_elems.len = 0;
}

Templ void
Clear( listwalloc_t<T>& list )
{
  list.len = 0;
  list.first = 0;
  list.last = 0;
  list.free_elems.len = 0;
}

Templ Inl listelem_t<T>*
_GetNewElem( listwalloc_t<T>& list )
{
  listelem_t<T>* elem;
  if( list.free_elems.len ) {
    elem = list.free_elems.mem[ list.free_elems.len - 1 ];
    RemBack( list.free_elems );
  } else {
    // TODO: should allow customization of the alignment here!
    elem = AddPlist( *list.elems, listelem_t<T>, _SIZEOF_IDX_T, 1 );
  }
  AssertCrash( elem );
  return elem;
}

Templ Inl void
_InitialInsert( listwalloc_t<T>& list, listelem_t<T>* elem )
{
  AssertCrash( !list.len );
  elem->prev = 0;
  elem->next = 0;
  list.first = elem;
  list.last = elem;
  list.len = 1;
}

Templ Inl void
InsertBefore( listwalloc_t<T>& list, listelem_t<T>* elem, listelem_t<T>* place )
{
  AssertCrash( elem );
  if( !list.len ) {
    AssertCrash( !place );
    _InitialInsert( list, elem );
  } else {
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
InsertAfter( listwalloc_t<T>& list, listelem_t<T>* elem, listelem_t<T>* place )
{
  AssertCrash( elem );
  if( !list.len ) {
    AssertCrash( !place );
    _InitialInsert( list, elem );
  } else {
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
InsertFirst( listwalloc_t<T>& list, listelem_t<T>* elem )
{
  InsertBefore( list, elem, list.first );
}

Templ Inl void
InsertLast( listwalloc_t<T>& list, listelem_t<T>* elem )
{
  InsertAfter( list, elem, list.last );
}

Templ Inl listelem_t<T>*
AddBefore( listwalloc_t<T>& list, listelem_t<T>* place )
{
  auto elem = _GetNewElem( list );
  InsertBefore( list, elem, place );
  return elem;
}

Templ Inl listelem_t<T>*
AddAfter( listwalloc_t<T>& list, listelem_t<T>* place )
{
  auto elem = _GetNewElem( list );
  InsertAfter( list, elem, place );
  return elem;
}

Templ Inl listelem_t<T>*
AddFirst( listwalloc_t<T>& list )
{
  return AddBefore( list, list.first );
}

Templ Inl listelem_t<T>*
AddLast( listwalloc_t<T>& list )
{
  return AddAfter( list, list.last );
}

Templ Inl void
Rem( listwalloc_t<T>& list, listelem_t<T>* elem )
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
RemFirst( listwalloc_t<T>& list )
{
  Rem( list, list.first );
}

Templ Inl void
RemLast( listwalloc_t<T>& list )
{
  Rem( list, list.last );
}

Templ Inl void
Reclaim( listwalloc_t<T>& list, listelem_t<T>* elem )
{
  if( list.free_elems.len < Capacity( list.free_elems ) ) {
    *AddBack( list.free_elems ) = elem;
  }
}
