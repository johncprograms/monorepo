// Copyright (c) John A. Carlos Jr., all rights reserved.

#define INSTRUMENT_PROBING 0

// TODO: worth considering static-size template param as well
// then allocation is trivially all in one block when we init one.
// one downside is resizing gets harder, since we couldn't share the same fns after resize.
// so i'll leave this as dynamic size for now.

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

#define FORELEM_HASHSET_NONZEROPTRS( set, integer_index_name, key_name, val_name ) \
  For( integer_index_name, 0, set.capacity ) { \
    auto key_name = set.keys[ integer_index_name ]; \
    if( !key_name ) continue; \
    auto val_name = set.vals[ integer_index_name ]; \


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

static void
TestHashsetNonzeroptrs()
{
  constant idx_t count = 256;

  hashset_nonzeroptrs_t<u32*, idx_t> set;
  Init( &set, 2 * count );

  idx_t tmp, tmp2;
  bool found, already_there;

  auto values = AllocString<u32*, allocator_heap_t, allocation_heap_t>( count );
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
