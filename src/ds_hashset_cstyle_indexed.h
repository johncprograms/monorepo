// Copyright (c) John A. Carlos Jr., all rights reserved.


// TODO: add links between hashset elems so we can iterate nicely over all elems/assocs.

#define HASHSET_ELEM_EQUAL( name )   bool ( name )( void* elem0, void* elem1, idx_t elem_size )
typedef HASHSET_ELEM_EQUAL( *pfn_hashset_elem_equal_t );

#define HASHSET_ELEM_HASH( name )   idx_t ( name )( void* elem, idx_t elem_len )
typedef HASHSET_ELEM_HASH( *pfn_hashset_elem_hash_t );

//
// This is an indexed C-style hashset, meaning:
// - When you insert, we hand back a persistent index you can use later to refer to what you inserted.
// - The size of the key and value types are defined at runtime/creation
//
struct
idx_hashset_t
{
  stack_cstyle_t elems;
  stack_resizeable_cont_t<idx_t> int_from_ext; // maps external indices to internal indices of idx_hashset_t.elems.
  stack_resizeable_cont_t<idx_t> ext_from_int; // maps internal indices of idx_hashset_t.elems to external indices.
  idx_t elem_data_len; // each hashed element has to be the same size, stored here.
  idx_t assoc_data_len; // each associated element has to be the same size, stored here.
  idx_t cardinality; // number of elems that have data currently.
  f32 expand_load_factor; // in ( 0, 1 ). will double capacity on Add when load factor goes beyond this threshold.
  pfn_hashset_elem_equal_t ElemEqual;
  pfn_hashset_elem_hash_t ElemHash;
};



// NOTE: only cast to this; you shouldn't ever instantiate one of these.
ALIGNTOIDX struct
idx_hashset_elem_t
{
  idx_t hash_code;
  bool has_data;
};

Inl void*
_GetElemData( idx_hashset_t& set, idx_hashset_elem_t* elem )
{
  void* r = Cast( u8*, elem + 1 );
  return r;
}

Inl void*
_GetAssocData( idx_hashset_t& set, idx_hashset_elem_t* elem )
{
  void* r = Cast( u8*, elem + 1 ) + set.elem_data_len;
  return r;
}



Inl bool
_ValidIdxInt( idx_hashset_t& set, idx_t idx_int )
{
  if( idx_int >= Capacity( set.elems ) )
    return 0;

  auto elem = ByteArrayElem( idx_hashset_elem_t, set.elems, idx_int );
  return elem->has_data;
}

Inl bool
_ValidIdxExt( idx_hashset_t& set, idx_t idx_ext )
{
  if( idx_ext >= Capacity( set.elems ) )
    return 0;

  idx_t idx_int = set.int_from_ext.mem[idx_ext];
  bool r = _ValidIdxInt( set, idx_int );
  return r;
}



void
Init(
  idx_hashset_t& set,
  idx_t capacity,
  idx_t elem_data_len,
  idx_t assoc_data_len,
  f32 expand_load_factor,
  pfn_hashset_elem_equal_t ElemEqual,
  pfn_hashset_elem_hash_t ElemHash
  )
{
  AssertCrash( ( expand_load_factor > 0 )  &  ( expand_load_factor <= 1 ) );

  // set all external indices to map to an invalid internal index.
  Alloc( set.int_from_ext, capacity );
  For( i, 0, capacity ) {
    idx_t idx_invalid = MAX_idx;
    *AddBack( set.int_from_ext ) = idx_invalid;
  }

  Alloc( set.ext_from_int, capacity );
  For( i, 0, capacity ) {
    idx_t idx_invalid = MAX_idx;
    *AddBack( set.ext_from_int ) = idx_invalid;
  }


  idx_t elem_len = sizeof( idx_hashset_elem_t ) + elem_data_len + assoc_data_len;
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
Kill( idx_hashset_t& set )
{
  Free( set.elems );
  Free( set.int_from_ext );
  Free( set.ext_from_int );
  set.elem_data_len = 0;
  set.assoc_data_len = 0;
  set.expand_load_factor = 0;
  set.cardinality = 0;
  set.ElemEqual = 0;
  set.ElemHash = 0;
}



static void
DoubleCapacity( idx_hashset_t& old_set )
{
  idx_hashset_t set;
  Init(
    set,
    2 * Capacity( old_set.elems ),
    old_set.elem_data_len,
    old_set.assoc_data_len,
    old_set.expand_load_factor,
    old_set.ElemEqual,
    old_set.ElemHash
    );

  ForLen( i, old_set.elems ) {
    auto old_elem = ByteArrayElem( idx_hashset_elem_t, old_set.elems, i );
    if( old_elem->has_data ) {
      void* element = _GetElemData( old_set, old_elem );
      void* assoc = _GetAssocData( old_set, old_elem );

      idx_t hash_code = old_elem->hash_code;
      idx_t idx = hash_code % set.elems.len;
      idx_t nprobed = 0;
      while( nprobed < set.elems.len ) {

        auto elem = ByteArrayElem( idx_hashset_elem_t, set.elems, idx );

        if( elem->has_data ) {
          if( elem->hash_code == hash_code ) {
            if( set.ElemEqual( _GetElemData( set, elem ), element, set.elem_data_len ) ) {
              UnreachableCrash();
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

          // Preserve the idx_ext on resize!
          idx_t idx_ext = old_set.ext_from_int.mem[i];
          idx_t idx_int = idx;
          idx_t* assigned_ext = &set.ext_from_int.mem[idx_int];
          idx_t* assigned_int = &set.int_from_ext.mem[idx_ext];
          *assigned_int = idx_int;
          *assigned_ext = idx_ext;

          break;
        }
      }
      AssertCrash( nprobed < set.elems.len );
    }
  }
  Kill( old_set );
  old_set = set;
}



void
Add(
  idx_hashset_t& set,
  void* element,
  void* assoc,
  idx_t* dst_idx_ext,
  bool* already_there,
  void* assoc_already_there
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
    auto elem = ByteArrayElem( idx_hashset_elem_t, set.elems, idx );
    if( elem->has_data ) {
      if( elem->hash_code == hash_code ) {
        if( set.ElemEqual( _GetElemData( set, elem ), element, set.elem_data_len ) ) {
          idx_t* idx_ext = &set.ext_from_int.mem[idx];
          AssertCrash( _ValidIdxExt( set, *idx_ext ) );
          *dst_idx_ext = *idx_ext;
          if( already_there ) {
            *already_there = 1;
          }
          if( assoc_already_there ) {
            Memmove( assoc_already_there, _GetAssocData( set, elem ), set.assoc_data_len );
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

      idx_t probed_idx_int = idx;
      idx_t nprobed_int = 0;
      while( nprobed_int < set.elems.len ) {
        idx_t* candid_ext = &set.ext_from_int.mem[probed_idx_int];
        if( _ValidIdxExt( set, *candid_ext ) ) {
          probed_idx_int = ( probed_idx_int + 1 ) % set.elems.len;
          nprobed_int += 1;
        } else {
          break;
        }
      }
      AssertCrash( nprobed_int < set.elems.len );

      idx_t probed_idx_ext = idx;
      idx_t nprobed_ext = 0;
      while( nprobed_ext < set.elems.len ) {
        idx_t* candid_int = &set.int_from_ext.mem[probed_idx_ext];
        if( _ValidIdxInt( set, *candid_int ) ) {
          probed_idx_ext = ( probed_idx_ext + 1 ) % set.elems.len;
          nprobed_ext += 1;
        } else {
          break;
        }
      }
      AssertCrash( nprobed_ext < set.elems.len );

      idx_t* assigned_int = &set.int_from_ext.mem[probed_idx_ext];
      idx_t* assigned_ext = &set.ext_from_int.mem[probed_idx_int];
      *assigned_int = probed_idx_int;
      *assigned_ext = probed_idx_ext;

      *dst_idx_ext = *assigned_ext;

      return;
    }
  }
  UnreachableCrash();
}



void
Lookup(
  idx_hashset_t& set,
  void* element,
  idx_t* dst_idx_ext,
  bool* found,
  void* found_assoc
  )
{
  *found = 0;

  idx_t hash_code = set.ElemHash( element, set.elem_data_len );
  idx_t idx = hash_code % set.elems.len;

  idx_t nprobed = 0;
  while( nprobed < set.elems.len ) {
    auto elem = ByteArrayElem( idx_hashset_elem_t, set.elems, idx );

    if( elem->has_data ) {
      if( hash_code == elem->hash_code ) {
        if( set.ElemEqual( _GetElemData( set, elem ), element, set.elem_data_len ) ) {
          idx_t* idx_ext = &set.ext_from_int.mem[idx];
          AssertCrash( _ValidIdxExt( set, *idx_ext ) );
          *dst_idx_ext = *idx_ext;
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
Remove( idx_hashset_t& set, idx_t src_idx_ext )
{
  AssertCrash( _ValidIdxExt( set, src_idx_ext ) );

  idx_t* idx_int = &set.int_from_ext.mem[src_idx_ext];
  AssertCrash( _ValidIdxInt( set, *idx_int ) );

  idx_t* idx_ext = &set.ext_from_int.mem[*idx_int];
  AssertCrash( *idx_ext == src_idx_ext );

  auto elem = ByteArrayElem( idx_hashset_elem_t, set.elems, *idx_int );
  AssertCrash( elem->has_data );

  elem->has_data = 0;

  idx_t idx_invalid = MAX_idx;
  *idx_int = idx_invalid;
  *idx_ext = idx_invalid;

  set.cardinality -= 1;
}



void
GetElement(
  idx_hashset_t& set,
  idx_t src_idx_ext,
  bool* valid_idx_ext,
  void** dst_element,
  void** dst_assoc
  )
{
  if( _ValidIdxExt( set, src_idx_ext ) ) {

    *valid_idx_ext = 1;

    idx_t* idx_int = &set.int_from_ext.mem[src_idx_ext];
    AssertCrash( _ValidIdxInt( set, *idx_int ) );

    idx_t* idx_ext = &set.ext_from_int.mem[*idx_int];
    AssertCrash( *idx_ext == src_idx_ext );

    auto elem = ByteArrayElem( idx_hashset_elem_t, set.elems, *idx_int );
    AssertCrash( elem->has_data );

    if( dst_element ) {
      *dst_element = _GetElemData( set, elem );
    }
    if( dst_assoc ) {
      *dst_assoc = _GetAssocData( set, elem );
    }

  } else {
    *valid_idx_ext = 0;
  }
}




static void
TestIdxHashset()
{
  idx_hashset_t set;
  Init(
    set,
    8,
    sizeof( idx_t ),
    sizeof( idx_t ),
    0.75f,
    Equal_FirstIdx,
    Hash_FirstIdx
    );

  static idx_t count = 256;

  stack_resizeable_cont_t<idx_t> idx_ext;
  Alloc( idx_ext, count );
  AddBack( idx_ext, count );
  For( i, 0, count ) {
    idx_ext.mem[i] = MAX_idx;
  }

  idx_t tmp, tmp2;
  idx_t* ptmp = &tmp;
  idx_t* ptmp2 = &tmp2;
  idx_t ext;
  bool found, already_there;

  For( i, 0, count ) {

    AssertCrash( set.cardinality == i );

    For( j, 0, i ) {
      Lookup( set, &j, &ext, &found, &tmp );
      AssertCrash( found );
      AssertCrash( tmp == j + 1 );
      AssertCrash( idx_ext.mem[j] == ext );

      GetElement( set, idx_ext.mem[j], &found, Cast( void**, &ptmp ), Cast( void**, &ptmp2 ) );
      AssertCrash( found );
      AssertCrash( *ptmp == j );
      AssertCrash( *ptmp2 == j + 1 );

      tmp = j + 1;
      Add( set, &j, &tmp, &ext, &already_there, &tmp2 );
      AssertCrash( already_there );
      AssertCrash( tmp2 == j + 1 );
      AssertCrash( idx_ext.mem[j] == ext );
    }

    tmp = i + 1;
    Add( set, &i, &tmp, &ext, &already_there, 0 );
    AssertCrash( !already_there );
    AssertCrash( idx_ext.mem[i] == MAX_idx );
    idx_ext.mem[i] = ext;
    AssertCrash( set.cardinality == i + 1 );

    GetElement( set, idx_ext.mem[i], &found, Cast( void**, &ptmp ), Cast( void**, &ptmp2 ) );
    AssertCrash( found );
    AssertCrash( *ptmp == i );
    AssertCrash( *ptmp2 == i + 1 );
  }

  For( i, 0, count ) {
    if( i & 1 ) {
      AssertCrash( idx_ext.mem[i] != MAX_idx );
      Remove( set, idx_ext.mem[i] );
      idx_ext.mem[i] = MAX_idx;
    }
  }
  AssertCrash( set.cardinality == count / 2 );

  For( i, 0, count ) {
    if( i & 1 ) {
      GetElement( set, idx_ext.mem[i], &found, 0, 0 );
      AssertCrash( !found );

      Lookup( set, &i, &ext, &found, 0 );
      AssertCrash( !found );

      AssertCrash( idx_ext.mem[i] == MAX_idx );

    } else {
      GetElement( set, idx_ext.mem[i], &found, Cast( void**, &ptmp ), Cast( void**, &ptmp2 ) );
      AssertCrash( found );
      AssertCrash( idx_ext.mem[i] != MAX_idx );
      AssertCrash( *ptmp == i );
      AssertCrash( *ptmp2 == i + 1 );
    }
  }

#if 0
  Clear( set );
  AssertCrash( set.cardinality == 0 );

  Lookup( set, &tmp, &found, 0 );
  AssertCrash( !found );
#endif

  Free( idx_ext );

  Kill( set );
}
