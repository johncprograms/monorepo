// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// C-style hashset, where the size of the key and value types are defined at runtime/creation.
//

// TODO: add links between hashset elems so we can iterate nicely over all elems/assocs.

#define HASHSET_ELEM_EQUAL( name )   bool ( name )( void* elem0, void* elem1, idx_t elem_size )
typedef HASHSET_ELEM_EQUAL( *pfn_hashset_elem_equal_t );

#define HASHSET_ELEM_HASH( name )   idx_t ( name )( void* elem, idx_t elem_len )
typedef HASHSET_ELEM_HASH( *pfn_hashset_elem_hash_t );

struct
hashset_t
{
  stack_cstyle_t elems;
  idx_t elem_data_len; // each hashed element has to be the same size, stored here.
  idx_t assoc_data_len; // each associated element has to be the same size, stored here.
  idx_t cardinality; // number of elems that have data currently.
  f32 expand_load_factor; // in ( 0, 1 ). will double capacity on Add when load factor goes beyond this threshold.
  pfn_hashset_elem_equal_t ElemEqual;
  pfn_hashset_elem_hash_t ElemHash;
};

// NOTE: only cast to this; you shouldn't ever instantiate one of these.
ALIGNTOIDX struct
hashset_elem_t
{
  idx_t hash_code;
  bool has_data; // NOTE: we rely on Memzero to set this to false in Init.
};
Inl void*
_GetElemData( hashset_t& set, hashset_elem_t* elem )
{
  void* r = Cast( u8*, elem ) + sizeof( hashset_elem_t );
  return r;
}
Inl void*
_GetAssocData( hashset_t& set, hashset_elem_t* elem )
{
  void* r = Cast( u8*, elem ) + sizeof( hashset_elem_t ) + set.elem_data_len;
  return r;
}

void
Init(
  hashset_t& set,
  idx_t capacity,
  idx_t elem_data_len,
  idx_t assoc_data_len,
  f32 expand_load_factor,
  pfn_hashset_elem_equal_t ElemEqual,
  pfn_hashset_elem_hash_t ElemHash
  )
{
  AssertCrash( ( expand_load_factor > 0 )  &  ( expand_load_factor <= 1 ) );

  idx_t elem_len = sizeof( hashset_elem_t ) + elem_data_len + assoc_data_len; // NOTE: change if hashset_elem_t ever changes.
  Alloc( set.elems, capacity, elem_len );

  Memzero( set.elems.mem, capacity * elem_len );
  set.elems.len = capacity;

  set.elem_data_len = elem_data_len;
  set.assoc_data_len = assoc_data_len;
  set.expand_load_factor = expand_load_factor;
  set.cardinality = 0;
  set.ElemEqual = ElemEqual;
  set.ElemHash = ElemHash;
}
void
Kill( hashset_t& set )
{
  Free( set.elems );
  set.elem_data_len = 0;
  set.assoc_data_len = 0;
  set.expand_load_factor = 0;
  set.cardinality = 0;
  set.ElemEqual = 0;
  set.ElemHash = 0;
}

void
Clear( hashset_t& set )
{
  set.cardinality = 0;
  ForLen( i, set.elems ) {
    auto& elem = *ByteArrayElem( hashset_elem_t, set.elems, i );
    elem.has_data = 0;
  }
}

static void
DoubleCapacity( hashset_t& old_set )
{
  hashset_t new_set;
  Init(
    new_set,
    2 * Capacity( old_set.elems ),
    old_set.elem_data_len,
    old_set.assoc_data_len,
    old_set.expand_load_factor,
    old_set.ElemEqual,
    old_set.ElemHash
    );

  ForLen( i, old_set.elems ) {
    auto old_elem = ByteArrayElem( hashset_elem_t, old_set.elems, i );
    if( old_elem->has_data ) {
      void* element = _GetElemData( old_set, old_elem );
      void* assoc = _GetAssocData( old_set, old_elem );

      idx_t hash_code = old_elem->hash_code;
      idx_t idx = hash_code % new_set.elems.len;
      idx_t nprobed = 0;
      while( nprobed < new_set.elems.len ) {

        auto elem = ByteArrayElem( hashset_elem_t, new_set.elems, idx );

        if( elem->has_data ) {
          if( elem->hash_code == hash_code ) {
            if( new_set.ElemEqual( _GetElemData( new_set, elem ), element, new_set.elem_data_len ) ) {
              UnreachableCrash();
            }
          }
          idx = ( idx + 1 ) % new_set.elems.len;
          nprobed += 1;

        } else {
          elem->hash_code = hash_code;
          Memmove( _GetElemData( new_set, elem ), element, new_set.elem_data_len );
          Memmove( _GetAssocData( new_set, elem ), assoc, new_set.assoc_data_len );
          elem->has_data = 1;
          new_set.cardinality += 1;
          break;
        }
      }
      AssertCrash( nprobed < new_set.elems.len );
    }
  }
  Kill( old_set );
  old_set = new_set;
}
void
Add(
  hashset_t& set,
  void* element,
  void* assoc,
  bool* already_there,
  void* assoc_already_there,
  bool overwrite_assoc_if_already_there
  )
{
  f32 load_factor = Cast( f32, set.cardinality + 1 ) / Cast( f32, set.elems.len );
  if( load_factor > set.expand_load_factor ) {
    DoubleCapacity( set );
  }

  idx_t hash_code = set.ElemHash( element, set.elem_data_len );
  idx_t idx = hash_code % set.elems.len;
  idx_t nprobed = 0;
  while( nprobed < set.elems.len ) {
    auto elem = ByteArrayElem( hashset_elem_t, set.elems, idx );
    if( elem->has_data ) {
      if( elem->hash_code == hash_code ) {
        if( set.ElemEqual( _GetElemData( set, elem ), element, set.elem_data_len ) ) {
          if( already_there ) {
            *already_there = 1;
          }
          if( assoc_already_there ) {
            Memmove( assoc_already_there, _GetAssocData( set, elem ), set.assoc_data_len );
          }
          if( overwrite_assoc_if_already_there ) {
            Memmove( _GetAssocData( set, elem ), assoc, set.assoc_data_len );
          }
          return;
        }
      }
      idx = ( idx + 1 ) % set.elems.len;
      nprobed += 1;

    } else {
      elem->hash_code = hash_code;
      elem->has_data = 1;
      set.cardinality += 1;
      Memmove( _GetElemData( set, elem ), element, set.elem_data_len );
      Memmove( _GetAssocData( set, elem ), assoc, set.assoc_data_len );
      if( already_there ) {
        *already_there = 0;
      }
      return;
    }
  }
  UnreachableCrash();
}
void
Lookup(
  hashset_t& set,
  void* element,
  bool* found,
  void* found_assoc
  )
{
  *found = 0;

  idx_t hash_code = set.ElemHash( element, set.elem_data_len );
  idx_t idx = hash_code % set.elems.len;

  idx_t nprobed = 0;
  while( nprobed < set.elems.len ) {
    auto elem = ByteArrayElem( hashset_elem_t, set.elems, idx );

    if( elem->has_data ) {
      if( hash_code == elem->hash_code ) {
        if( set.ElemEqual( _GetElemData( set, elem ), element, set.elem_data_len ) ) {
          *found = 1;
          if( found_assoc ) {
            Memmove( found_assoc, _GetAssocData( set, elem ), set.assoc_data_len );
          }
          break;
        }
      }
      idx = ( idx + 1 ) % set.elems.len;
      nprobed += 1;

    } else {
      break;
    }
  }
}
void
LookupRaw(
  hashset_t& set,
  void* element,
  bool* found,
  void** found_assoc
  )
{
  *found = 0;
  if( found_assoc ) {
    *found_assoc = 0;
  }

  idx_t hash_code = set.ElemHash( element, set.elem_data_len );
  idx_t idx = hash_code % set.elems.len;

  idx_t nprobed = 0;
  while( nprobed < set.elems.len ) {
    auto elem = ByteArrayElem( hashset_elem_t, set.elems, idx );

    if( elem->has_data ) {
      if( hash_code == elem->hash_code ) {
        if( set.ElemEqual( _GetElemData( set, elem ), element, set.elem_data_len ) ) {
          *found = 1;
          if( found_assoc ) {
            *found_assoc = _GetAssocData( set, elem );
          }
          break;
        }
      }
      idx = ( idx + 1 ) % set.elems.len;
      nprobed += 1;

    } else {
      break;
    }
  }
}
void
Remove(
  hashset_t& set,
  void* element,
  bool* found,
  void* found_assoc
  )
{
  *found = 0;

  idx_t hash_code = set.ElemHash( element, set.elem_data_len );
  idx_t idx = hash_code % set.elems.len;

  idx_t nprobed = 0;
  while( nprobed < set.elems.len ) {
    auto elem = ByteArrayElem( hashset_elem_t, set.elems, idx );

    if( elem->has_data ) {
      if( hash_code == elem->hash_code ) {
        if( set.ElemEqual( _GetElemData( set, elem ), element, set.elem_data_len ) ) {
          elem->has_data = 0;
          set.cardinality -= 1;
          *found = 1;
          if( found_assoc ) {
            Memmove( found_assoc, _GetAssocData( set, elem ), set.assoc_data_len );
          }
          break;
        }
      }
      idx = ( idx + 1 ) % set.elems.len;
      nprobed += 1;

    } else {
      break;
    }
  }
}

ForceInl
HASHSET_ELEM_EQUAL( Equal_Memcmp )
{
  bool r = MemEqual( elem0, elem1, elem_size );
  return r;
}
ForceInl
HASHSET_ELEM_EQUAL( Equal_FirstU32 )
{
  AssertCrash( elem_size >= sizeof( u32 ) );
  auto& a = *Cast( u32*, elem0 );
  auto& b = *Cast( u32*, elem1 );
  bool r = ( a == b );
  return r;
}
ForceInl
HASHSET_ELEM_EQUAL( Equal_FirstU64 )
{
  AssertCrash( elem_size >= sizeof( u64 ) );
  auto& a = *Cast( u64*, elem0 );
  auto& b = *Cast( u64*, elem1 );
  bool r = ( a == b );
  return r;
}
ForceInl
HASHSET_ELEM_EQUAL( Equal_FirstIdx )
{
  AssertCrash( elem_size >= sizeof( idx_t ) );
  auto& a = *Cast( idx_t*, elem0 );
  auto& b = *Cast( idx_t*, elem1 );
  bool r = ( a == b );
  return r;
}
ForceInl
HASHSET_ELEM_EQUAL( Equal_SliceContents )
{
  AssertCrash( elem_size == sizeof( slice_t ) );
  auto a = Cast( slice_t*, elem0 );
  auto b = Cast( slice_t*, elem1 );
  bool r = MemEqual( a->mem, a->len, b->mem, b->len );
  return r;
}

ForceInl
HASHSET_ELEM_HASH( Hash_FirstU32 )
{
  AssertCrash( elem_len >= sizeof( u32 ) );
  auto a = *Cast( u32*, elem );
  idx_t r = 3 + 7 * a;
  return r;
}
ForceInl
HASHSET_ELEM_HASH( Hash_FirstU64 )
{
  AssertCrash( elem_len >= sizeof( u64 ) );
  auto a = *Cast( u64*, elem );
  idx_t r = Cast( idx_t, 3 + 7 * a );
  return r;
}
ForceInl
HASHSET_ELEM_HASH( Hash_FirstIdx )
{
  AssertCrash( elem_len >= sizeof( idx_t ) );
  idx_t a = *Cast( idx_t*, elem );
  idx_t r = 3 + 7 * a;
  return r;
}
ForceInl
HASHSET_ELEM_HASH( Hash_SliceContents )
{
  AssertCrash( elem_len == sizeof( slice_t ) );
  auto a = Cast( slice_t*, elem );
  idx_t r = StringHash( a->mem, a->len );
  return r;
}



RegisterTest([]()
{
  // TODO: test overwrite_assoc_if_already_there

  hashset_t set;
  Init(
    set,
    8,
    sizeof( idx_t ),
    sizeof( idx_t ),
    0.75f,
    Equal_FirstIdx,
    Hash_FirstIdx
    );

  idx_t tmp;
  bool found, already_there;

  constant idx_t count = 256;
  For( i, 0, count ) {

    AssertCrash( set.cardinality == i );

    // verify all lookup mechanisms for all values already added.
    For( j, 0, i ) {
      Lookup( set, &j, &found, &tmp );
      AssertCrash( found );
      AssertCrash( tmp == j + 1 );

      idx_t* c = &tmp;
      LookupRaw( set, &j, &found, Cast( void**, &c ) );
      AssertCrash( found );
      AssertCrash( *c == j + 1 );

      Add( set, &j, &tmp, &already_there, 0, 0 );
      AssertCrash( already_there );
    }

    // add the actual new value.
    tmp = i + 1;
    Add( set, &i, &tmp, &already_there, 0, 0 );
    AssertCrash( !already_there );
    AssertCrash( set.cardinality == i + 1 );

    // validate it's been added.
    Lookup( set, &i, &found, &tmp );
    AssertCrash( found );
    AssertCrash( tmp == i + 1 );
  }

  // remove all the odd elements
  For( i, 0, count ) {
    if( i & 1 ) {
      Remove( set, &i, &found, &tmp );
      AssertCrash( found );
      AssertCrash( tmp == i + 1 );
    }
  }
  AssertCrash( set.cardinality == count / 2 );

  // verify all the odd elements are gone, and evens are still there.
  For( i, 0, count ) {
    if( i & 1 ) {
      Lookup( set, &i, &found, 0 );
      AssertCrash( !found );
    } else {
      Lookup( set, &i, &found, &tmp );
      AssertCrash( found );
      AssertCrash( tmp == i + 1 );
    }
  }

  Clear( set );
  AssertCrash( set.cardinality == 0 );

  Lookup( set, &tmp, &found, 0 );
  AssertCrash( !found );

  LookupRaw( set, &tmp, &found, 0 );
  AssertCrash( !found );

  Kill( set );
});
