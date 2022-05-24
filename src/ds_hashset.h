// Copyright (c) John A. Carlos Jr., all rights reserved.

#define INSTRUMENT_PROBING 0

// TODO: worth considering static-size template param as well
// then allocation is trivially all in one block when we init one.
// one downside is resizing gets harder, since we couldn't share the same fns after resize.
// so i'll leave this as dynamic size for now.

// TODO: our general hash functions return idx_t, which is much too big for our usual desired
// table size. So when we modulo table size, we're throwing away useful hashing work.
// we should fold higher bits down into the lower bits for better hashing.

#define HASHSET_COMPLEXKEY_EQUAL( TKey, name )   bool ( name )( TKey* a, TKey* b )
#define HASHSET_COMPLEXKEY_HASH( TKey, name )   idx_t ( name )( TKey* a )

// C++ metaprogramming doesn't allow associating standalone fns with structs, so we have to
// do this ugly define nonsense to wrap the fns we want in a struct type, which can be associated.
// This means we rely more on inlining optimizations, which is no good.
#define DEFINE_HASHSET_TRAITS( name, TKey, fn_equalcomplexkey, fn_hashcomplexkey ) \
struct name { \
  static ForceInl \
  HASHSET_COMPLEXKEY_EQUAL( TKey, EqualComplexKey ) \
  { \
    return fn_equalcomplexkey( a, b ); \
  } \
  static ForceInl \
  HASHSET_COMPLEXKEY_HASH( TKey, HashComplexKey ) \
  { \
    return fn_hashcomplexkey( a ); \
  } \
}

#define TKCHE \
  template< \
    typename Key, \
    typename Val, \
    typename TraitsComplexKey \
    >

#define HASHSET hashset_complexkey_t<Key, Val, TraitsComplexKey>

// This is intended for use with complex Key structs, usually larger than a CPU register.
// You must define the following for your complex Key type:
//     ForceInl idx_t HashComplexKey( Key* key );
//     ForceInl bool EqualComplexKey( Key* a, Key* b );
// Please mark it forceinline for better optimization here.
TKCHE struct
hashset_complexkey_t
{
  // we'll use 1 bit of the hash value stored here to mean: slot_filled
  // we need some way of encoding when slots are filled, so one bit here makes sense.
  idx_t* hashes;
  Key* keys;
  Val* vals;
  idx_t len;
  idx_t capacity;
#if INSTRUMENT_PROBING
  idx_t count_add;
  idx_t count_probes_during_add;
  idx_t count_lookup;
  idx_t count_probes_during_lookup;
#endif
};
TKCHE Inl void
Clear( HASHSET* set )
{
  set->len = 0;
  Memzero( set->hashes, sizeof( set->hashes[0] ) * set->capacity );
#if INSTRUMENT_PROBING
  set->count_add = 0;
  set->count_probes_during_add = 0;
  set->count_lookup = 0;
  set->count_probes_during_lookup = 0;
#endif
}
TKCHE Inl void
Init( HASHSET* set, idx_t initial_capacity )
{
  // PERF: try one bulk allocation here.
  set->hashes = MemHeapAlloc( idx_t, initial_capacity );
  set->keys = MemHeapAlloc( Key, initial_capacity );
  set->vals = MemHeapAlloc( Val, initial_capacity );
  set->capacity = initial_capacity;
  Clear( set );
}
TKCHE Inl void
Kill( HASHSET* set )
{
  MemHeapFree( set->hashes );
  MemHeapFree( set->keys );
  MemHeapFree( set->vals );
  set->len = 0;
  set->capacity = 0;
}
TKCHE Inl void
Resize( HASHSET* old_set, idx_t new_capacity )
{
  AssertCrash( old_set->len <= new_capacity );
  HASHSET new_set;
  Init( new_set, new_capacity );
  Fori( idx_t, idx_old, 0, old_set->len ) {
    auto slot_hash_old = old_set->hashes[idx_old];
    auto has_data_old = slot_hash_old & HIGHBIT_idx;
    if( !has_data_old ) {
      continue;
    }
    auto hash = slot_hash_old & ~HIGHBIT_idx;
    auto idx_new = hash % new_set->capacity;
    Forever { // limited to O( new_set->capacity ) since we have empty slots
      auto slot_hash_new = new_set->hashes[idx_new];
      auto has_data_new = slot_hash_new & HIGHBIT_idx;
      if( !has_data_new ) {
        // implicitly sets the high bit, since it's set already on slot_hash_old
        new_set->hashes[idx_new] = slot_hash_old;
        new_set->keys[idx_new] = old_set->keys[idx_old];
        new_set->vals[idx_new] = old_set->vals[idx_old];
        new_set->len += 1;
        break;
      }
      auto hash_at_idx = slot_hash_new & ~HIGHBIT_idx;
      if( hash_at_idx == hash ) {
        auto equal = TraitsComplexKey::EqualComplexKey( new_set->keys + idx_new, old_set->keys + idx_old );
        AssertCrash( !equal );
      }
      idx_new = ( idx_new + 1 ) % new_set->capacity;
    }
  }
  Kill( old_set );
  *old_set = new_set;
}
TKCHE Inl void
Add(
  HASHSET* set,
  Key* key,
  Val* val,
  bool* already_there,
  Val* val_already_there,
  bool overwrite_val_if_already_there
  )
{
  CompileAssert( sizeof( idx_t ) == sizeof( set->hashes[0] ) );
  AssertCrash( set->len < set->capacity );
  auto hash = TraitsComplexKey::HashComplexKey( key ) & ~HIGHBIT_idx;
  auto idx = hash % set->capacity;
#if INSTRUMENT_PROBING
  set->count_add += 1;
#endif
  Forever { // limited to O( set->capacity ) since we have one empty slot
    auto slot_hash = set->hashes[idx];
    auto has_data = slot_hash & HIGHBIT_idx;
    if( !has_data ) {
      // empty slot! put our value in the slot.
      set->hashes[idx] = hash | HIGHBIT_idx;
      set->keys[idx] = *key;
      set->vals[idx] = *val;
      set->len += 1;
      if( already_there ) {
        *already_there = 0;
      }
      return;
    }
    // NOTE: for simple key types, comparing keys directly will be faster than hash-then-key comparisons.
    auto hash_at_idx = slot_hash & ~HIGHBIT_idx;
    if( hash_at_idx == hash ) {
      auto equal = TraitsComplexKey::EqualComplexKey( key, set->keys + idx );
      if( equal ) {
        if( already_there ) {
          *already_there = 1;
        }
        if( val_already_there ) {
          *val_already_there = set->vals[idx];
        }
        if( overwrite_val_if_already_there ) {
          set->vals[idx] = *val;
        }
        return;
      }
    }
    idx = ( idx + 1 ) % set->capacity;
#if INSTRUMENT_PROBING
    set->count_probes_during_add += 1;
#endif
  }
//  UnreachableCrash();
}
TKCHE Inl void
LookupRaw(
  HASHSET* set,
  Key* key,
  bool* found,
  Val** found_pointer, // only valid when remove_when_found == 0
  Val* found_value,    // only valid when remove_when_found == 1
  bool remove_when_found
  )
{
  CompileAssert( sizeof( idx_t ) == sizeof( set->hashes[0] ) );
  *found = 0;
  auto capacity = set->capacity;
  auto hash = TraitsComplexKey::HashComplexKey( key ) & ~HIGHBIT_idx;
  auto idx = hash % capacity;
#if INSTRUMENT_PROBING
  set->count_lookup += 1;
#endif
  Fori( idx_t, nprobed, 0, capacity ) {
    auto slot_hash = set->hashes[idx];
    auto has_data = slot_hash & HIGHBIT_idx;
    if( !has_data ) {
      break;
    }
    // NOTE: for simple key types, comparing keys directly will be faster than hash-then-key comparisons.
    auto hash_at_idx = slot_hash & ~HIGHBIT_idx;
    if( hash_at_idx == hash ) {
      auto equal = TraitsComplexKey::EqualComplexKey( key, set->keys + idx );
      if( equal ) {
        *found = 1;
        if( remove_when_found ) {
          AssertCrash( set->len );
          set->len -= 1;
          //
          // return the value and clear the slot, before we deal with chains.
          //
          set->hashes[idx] = 0;
          *found_value = set->vals[idx];
          //
          // find the contiguous chunk of filled slots that we need to consider for chaining.
          // note the bounds are: [ idx_contiguous_start, idx_contiguous_end ]
          //
          auto idx_contiguous_start = idx;
          Fori( idx_t, nprobed_chain, nprobed + 1, capacity ) {
            auto idx_prev = ( idx_contiguous_start - 1 ) % capacity;
            auto slot_hash_prev = set->hashes[idx_prev];
            auto has_data_prev = slot_hash_prev & HIGHBIT_idx;
            if( !has_data_prev ) {
              break;
            }
            idx_contiguous_start = idx_prev;
          }
          auto idx_contiguous_end = idx;
          Fori( idx_t, nprobed_chain, nprobed + 1, capacity ) {
            auto idx_next = ( idx_contiguous_end + 1 ) % capacity;
            auto slot_hash_next = set->hashes[idx_next];
            auto has_data_next = slot_hash_next & HIGHBIT_idx;
            if( !has_data_next ) {
              break;
            }
            idx_contiguous_end = idx_next;
          }
          //
          // look inside ( idx, idx_contiguous_end ] for filled slots that will fail lookup now that we've
          // empied out the slot at idx.
          // once we find one, move it to the empty slot, then repeat with the newly-emptied slot, since
          // we've got the same iterative problem to solve.
          //
          // idx_empty = idx;
          // loop j from ( idx, idx_contiguous_end ]
          //   if hash_slot(j) is in [ idx_contiguous_start, idx_empty ]
          //     move j to idx_empty
          //     idx_empty = j
          //
          if( idx_contiguous_start != idx_contiguous_end ) {
            auto idx_empty = idx;
            auto idx_probe = ( idx + 1 ) % capacity;
            Forever {
              auto slot_hash_probe = set->hashes[idx_probe];
              auto hash_at_idx_probe = slot_hash_probe & ~HIGHBIT_idx;
              auto idx_original_probe = hash_at_idx_probe % capacity;
              // c inside [a,b]
              //   when a <= b:
              //     return a <= c && c <= b
              //   when a > b:
              //     return a <= c || c <= b
              auto probe_belongs_on_left =
                idx_contiguous_start <= idx_empty ?
                  idx_contiguous_start <= idx_original_probe && idx_original_probe <= idx_empty :
                  idx_contiguous_start <= idx_original_probe || idx_original_probe <= idx_empty;
              if( probe_belongs_on_left ) {
                set->keys[idx_empty] = set->keys[idx_probe];
                set->vals[idx_empty] = set->vals[idx_probe];
                set->hashes[idx_empty] = set->hashes[idx_probe];
                set->hashes[idx_probe] = 0;
                idx_empty = idx_probe;
              }
              if( idx_probe == idx_contiguous_end ) {
                break;
              }
              idx_probe = ( idx_probe + 1 ) % capacity;
            }
          }
        }
        else {
          *found_pointer = set->vals + idx;
        }
        break;
      }
    }
    idx = ( idx + 1 ) % capacity;
#if INSTRUMENT_PROBING
    set->count_probes_during_lookup += 1;
#endif
  }
}
TKCHE Inl void
LookupAndCopyValue(
  HASHSET* set,
  Key* key,
  bool* found,
  Val* found_val
  )
{
  Val* found_pointer = 0;
  LookupRaw( set, key, found, &found_pointer, (Val*)0, 0 );
  if( found_val  &&  *found ) {
    *found_val = *found_pointer;
  }
}
TKCHE Inl void
Lookup(
  HASHSET* set,
  Key* key,
  bool* found,
  Val** found_ptr
  )
{
  Val* found_pointer = 0;
  LookupRaw( set, key, found, &found_pointer, (Val*)0, 0 );
  if( found_ptr ) {
    *found_ptr = found_pointer;
  }
}
TKCHE Inl void
Remove(
  HASHSET* set,
  Key* key,
  bool* found,
  Val* found_val
  )
{
  Val found_value = 0;
  LookupRaw( set, key, found, (Val**)0, &found_value, 1 );
  if( found_val ) {
    *found_val = found_value;
  }
}

#undef HASHSET
#undef TKCHE




#define TKV \
  template< \
    typename Key, \
    typename Val \
    >

// This is intended for pointer-type Key, where null isn't a valid Key.
TKV struct
hashset_nonzeroptrs_t
{
  CompileAssert( std::is_pointer_v<Key> );
  // we'll use keys[i] == 0 to encode when a slot is empty.
  // this means you can't use this hashset to map null to something.
  Key* keys;
  Val* vals;
  idx_t len;
  idx_t capacity;
#if INSTRUMENT_PROBING
  idx_t count_add;
  idx_t count_probes_during_add;
  idx_t count_lookup;
  idx_t count_probes_during_lookup;
#endif
};
TKV Inl void
Clear( hashset_nonzeroptrs_t<Key, Val>* set )
{
  set->len = 0;
  Memzero( set->keys, sizeof( set->keys[0] ) * set->capacity );
#if INSTRUMENT_PROBING
  set->count_add = 0;
  set->count_probes_during_add = 0;
  set->count_lookup = 0;
  set->count_probes_during_lookup = 0;
#endif
}
TKV Inl void
Init( hashset_nonzeroptrs_t<Key, Val>* set, idx_t initial_capacity )
{
  // PERF: try one bulk allocation here.
  set->keys = MemHeapAlloc( Key, initial_capacity );
  set->vals = MemHeapAlloc( Val, initial_capacity );
  set->capacity = initial_capacity;
  Clear( set );
}
TKV Inl void
Kill( hashset_nonzeroptrs_t<Key, Val>* set )
{
  MemHeapFree( set->keys );
  MemHeapFree( set->vals );
  set->len = 0;
  set->capacity = 0;
}
ForceInl idx_t
HashPointer( void* p )
{
#if 1
  constexpr idx_t offset = 14695981039346656037ULL;
  constexpr idx_t prime  = 1099511628211ULL;
  auto r = offset;
  r ^= (idx_t)p;
  r *= prime;
  r ^= (idx_t)p >> ( sizeof( p ) / 2 );
  r *= prime;
  return r;
#else
  // TODO: do better than CRT?
  return std::hash<void*>{}(p);
#endif
}
TKV Inl void
Resize( hashset_nonzeroptrs_t<Key, Val>* old_set, idx_t new_capacity )
{
  AssertCrash( old_set->len <= new_capacity );
  hashset_nonzeroptrs_t<Key, Val> new_set;
  Init( new_set, new_capacity );
  Fori( idx_t, idx_old, 0, old_set->len ) {
    auto slot_old = old_set->keys[idx_old];
    if( !slot_old ) {
      continue;
    }
    // Note we have to re-hash here since we don't store it.
    auto hash = HashPointer( (void*)slot_old );
    auto idx_new = hash % new_set->capacity;
    Forever { // limited to O( new_set->capacity ) since we have empty slots
      auto slot_new = new_set->keys[idx_new];
      if( !slot_new ) {
        new_set->keys[idx_new] = slot_old;
        new_set->vals[idx_new] = old_set->vals[idx_old];
        new_set->len += 1;
        break;
      }
      AssertCrash( new_set->keys[idx_new] != slot_new );
      idx_new = ( idx_new + 1 ) % new_set->capacity;
    }
  }
  Kill( old_set );
  *old_set = new_set;
}
TKV Inl void
Add(
  hashset_nonzeroptrs_t<Key, Val>* set,
  Key key,
  Val* val,
  bool* already_there,
  Val* val_already_there,
  bool overwrite_val_if_already_there
  )
{
  AssertCrash( set->len < set->capacity );
  AssertCrash( key );
  auto hash = HashPointer( (void*)key );
  auto idx = hash % set->capacity;
#if INSTRUMENT_PROBING
  set->count_add += 1;
#endif
  Forever { // limited to O( set->capacity ) since we have one empty slot
    auto slot = set->keys[idx];
    if( !slot ) {
      // empty slot! put our value in the slot.
      set->keys[idx] = key;
      set->vals[idx] = *val;
      set->len += 1;
      if( already_there ) {
        *already_there = 0;
      }
      return;
    }
    if( key == slot ) {
      if( already_there ) {
        *already_there = 1;
      }
      if( val_already_there ) {
        *val_already_there = set->vals[idx];
      }
      if( overwrite_val_if_already_there ) {
        set->vals[idx] = *val;
      }
      return;
    }
    idx = ( idx + 1 ) % set->capacity;
#if INSTRUMENT_PROBING
    set->count_probes_during_add += 1;
#endif
  }
//  UnreachableCrash();
}
TKV Inl void
LookupRaw(
  hashset_nonzeroptrs_t<Key, Val>* set,
  Key key,
  bool* found,
  Val** found_pointer, // only valid when remove_when_found == 0
  Val* found_value,    // only valid when remove_when_found == 1
  bool remove_when_found
  )
{
  *found = 0;
  auto capacity = set->capacity;
  auto hash = HashPointer( (void*)key );
  auto idx = hash % capacity;
#if INSTRUMENT_PROBING
  set->count_lookup += 1;
#endif
  Fori( idx_t, nprobed, 0, capacity ) {
    auto slot = set->keys[idx];
    if( !slot ) {
      break;
    }
    // NOTE: since we're not checking hashes, failure to find will iterate a whole chain.
    // given that our keys are pointers, it's likely faster to just do the simple iteration.
    if( key == slot ) {
      *found = 1;
      if( remove_when_found ) {
        AssertCrash( set->len );
        set->len -= 1;
        //
        // return the value and clear the slot, before we deal with chains.
        //
        set->keys[idx] = 0;
        *found_value = set->vals[idx];
        //
        // find the contiguous chunk of filled slots that we need to consider for chaining.
        // note the bounds are: [ idx_contiguous_start, idx_contiguous_end ]
        //
        auto idx_contiguous_start = idx;
        Fori( idx_t, nprobed_chain, nprobed + 1, capacity ) {
          auto idx_prev = ( idx_contiguous_start - 1 ) % capacity;
          auto slot_prev = set->keys[idx_prev];
          if( !slot_prev ) {
            break;
          }
          idx_contiguous_start = idx_prev;
        }
        auto idx_contiguous_end = idx;
        Fori( idx_t, nprobed_chain, nprobed + 1, capacity ) {
          auto idx_next = ( idx_contiguous_end + 1 ) % capacity;
          auto slot_next = set->keys[idx_next];
          if( !slot_next ) {
            break;
          }
          idx_contiguous_end = idx_next;
        }
        //
        // look inside ( idx, idx_contiguous_end ] for filled slots that will fail lookup now that we've
        // empied out the slot at idx.
        // once we find one, move it to the empty slot, then repeat with the newly-emptied slot, since
        // we've got the same iterative problem to solve.
        //
        // idx_empty = idx;
        // loop j from ( idx, idx_contiguous_end ]
        //   if hash_slot(j) is in [ idx_contiguous_start, idx_empty ]
        //     move j to idx_empty
        //     idx_empty = j
        //
        if( idx_contiguous_start != idx_contiguous_end ) {
          auto idx_empty = idx;
          auto idx_probe = ( idx + 1 ) % capacity;
          Forever {
            auto slot_probe = set->keys[idx_probe];
            auto hash_probe = HashPointer( (void*)slot_probe );
            auto idx_original_probe = hash_probe % capacity;
            // c inside [a,b]
            //   when a <= b:
            //     return a <= c && c <= b
            //   when a > b:
            //     return a <= c || c <= b
            auto probe_belongs_on_left =
              idx_contiguous_start <= idx_empty ?
                idx_contiguous_start <= idx_original_probe && idx_original_probe <= idx_empty :
                idx_contiguous_start <= idx_original_probe || idx_original_probe <= idx_empty;
            if( probe_belongs_on_left ) {
              set->keys[idx_empty] = set->keys[idx_probe];
              set->keys[idx_probe] = 0;
              set->vals[idx_empty] = set->vals[idx_probe];
              idx_empty = idx_probe;
            }
            if( idx_probe == idx_contiguous_end ) {
              break;
            }
            idx_probe = ( idx_probe + 1 ) % capacity;
          }
        }
      }
      else {
        *found_pointer = set->vals + idx;
      }
      break;
    }
    idx = ( idx + 1 ) % capacity;
#if INSTRUMENT_PROBING
    set->count_probes_during_lookup += 1;
#endif
  }
}
TKV Inl void
LookupAndCopyValue(
  hashset_nonzeroptrs_t<Key, Val>* set,
  Key key,
  bool* found,
  Val* found_val
  )
{
  Val* found_pointer = 0;
  LookupRaw( set, key, found, &found_pointer, (Val*)0, 0 );
  if( found_val  &&  *found ) {
    *found_val = *found_pointer;
  }
}
TKV Inl void
Lookup(
  hashset_nonzeroptrs_t<Key, Val>* set,
  Key key,
  bool* found,
  Val** found_ptr
  )
{
  Val* found_pointer = 0;
  LookupRaw( set, key, found, &found_pointer, (Val*)0, 0 );
  if( found_ptr ) {
    *found_ptr = found_pointer;
  }
}
TKV Inl void
Remove(
  hashset_nonzeroptrs_t<Key, Val>* set,
  Key key,
  bool* found,
  Val* found_val
  )
{
  Val found_value;
  LookupRaw( set, key, found, (Val**)0, &found_value, 1 );
  if( found_val ) {
    *found_val = found_value;
  }
}




// ==========================================================================
//
// C-style hashset
//


// TODO: add links between hashset elems so we can iterate nicely over all elems/assocs.

#define HASHSET_ELEM_EQUAL( name )   bool ( name )( void* elem0, void* elem1, idx_t elem_size )
typedef HASHSET_ELEM_EQUAL( *pfn_hashset_elem_equal_t );

#define HASHSET_ELEM_HASH( name )   idx_t ( name )( void* elem, idx_t elem_len )
typedef HASHSET_ELEM_HASH( *pfn_hashset_elem_hash_t );

struct
hashset_t
{
  bytearray_t elems;
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



static void
TestHashset()
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
}

static void
TestHashsetNonzeroptrs()
{
  constant idx_t count = 256;

  hashset_nonzeroptrs_t<u32*, idx_t> set;
  Init( &set, 2 * count );

  idx_t tmp, tmp2;
  bool found, already_there;

  tstring_t<u32*> values;
  Alloc( values, count );
  For( i, 0, count ) {
    values.mem[i] = (u32*)( i + 1 );
  }

  For( i, 0, count ) {

    AssertCrash( set.len == i );

    // verify all lookup mechanisms for all values already added.
    For( j, 0, i ) {
      LookupAndCopyValue( &set, values.mem[j], &found, &tmp );
      AssertCrash( found );
      AssertCrash( tmp == j + 1 );

      idx_t* c = &tmp;
      Lookup( &set, values.mem[j], &found, &c );
      AssertCrash( found );
      AssertCrash( *c == j + 1 );

      Add( &set, values.mem[j], &tmp, &already_there, &tmp2, 0 );

      AssertCrash( already_there );
    }

    // add the actual new value.
    tmp = i + 1;
    Add( &set, values.mem[i], &tmp, &already_there, &tmp2, 0 );
    AssertCrash( !already_there );
    AssertCrash( set.len == i + 1 );

    // validate it's been added.
    LookupAndCopyValue( &set, values.mem[i], &found, &tmp );
    AssertCrash( found );
    AssertCrash( tmp == i + 1 );
  }

  // remove all the odd elements
  For( i, 0, count ) {
    if( i & 1 ) {
      Remove( &set, values.mem[i], &found, &tmp );
      AssertCrash( found );
      AssertCrash( tmp == i + 1 );
    }
  }
  AssertCrash( set.len == count / 2 );

  // verify all the odd elements are gone, and evens are still there.
  For( i, 0, count ) {
    if( i & 1 ) {
      LookupAndCopyValue( &set, values.mem[i], &found, &tmp );
      AssertCrash( !found );
    } else {
      LookupAndCopyValue( &set, values.mem[i], &found, &tmp );
      AssertCrash( found );
      AssertCrash( tmp == i + 1 );
    }
  }

  Clear( &set );
  AssertCrash( set.len == 0 );

  LookupAndCopyValue( &set, values.mem[0], &found, &tmp );
  AssertCrash( !found );

  idx_t* c = &tmp2;
  Lookup( &set, values.mem[0], &found, &c );
  AssertCrash( !found );

  Free( values );
  Kill( &set );
}

static void
TestHashsetComplexkey()
{
}
