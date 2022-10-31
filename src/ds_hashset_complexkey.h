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

static void
TestHashsetComplexkey()
{
}