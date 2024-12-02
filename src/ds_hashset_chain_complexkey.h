// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

the common hashtable is a stack of linked lists:
  0 e0
  1
  2 e1 -> e2 -> e3
where:
  list 0 contains e0 because it hashed to 0
  list 1 contains nothing because no elements hashed to 1
  list 2 contains e1,e2,e3 because they hashed to 2

the chaining idea here is different. intead of using linked lists, we'll use indirection lists (very similar, but with better packing properties).
we maintain two tables. the first is a key index table:
  0 0
  1 invalid
  2 1
is where we store chain starts. This maps hash slots to the keys that each start their own chain.

the second table is the chain table:
  0 invalid
  1 2
  2 3
  3 invalid
this is a unidirectional linked list table, telling you which elements come next. invalid means nothing is chained after.

QUESTION: make the lists cyclical? i.e. 0 points to 0? 3 points to 1? is that beneficial?
  if we did, then we could adjust the start indices on lookup (trying to optimize lookup for common elements).
  this optimization is somewhat questionable; more beneficial would be to reorder the list, not just adjust the head pointer.

QUESTION: bidirectional linked list here should make arbitrary insert/remove better, in cases of large chains. Right?
  this would also allow for the list reordering optimization

QUESTION: store the hashes?
  strictly an improvement for Resize. Not needed nearly as much as the linear probing hashsets which do frequent reorders.

#endif

#define TKCHE \
  template< \
    typename Key, \
    typename Val, \
    typename TraitsComplexKey \
    >

#define HASHSET hashset_chain_complexkey_t<Key, Val, TraitsComplexKey>

//using Index = idx_t;
//#define InvalidIndex MAX_idx
using Index = u32;
#define InvalidIndex MAX_u32

#define _Invalid( i ) ( ( i ) == InvalidIndex )

TKCHE struct
hashset_chain_complexkey_t
{
  Index* chain_starts; // length num_chains
  idx_t num_chains;
//  idx_t* hashes;
//  Index* chain_lengths;

  Index* chain_nexts; // length capacity
  Key* keys;          // length capacity
  Val* vals;          // length capacity
  Index capacity;

  Index* free_keys; // length capacity.

  Index len;
};

TKCHE Inl
void
Lookup(
  HASHSET* s,
  Key* key,
  bool* found,
  Val** found_pval
  )
{
  ProfFunc();
  auto num_chains = s->num_chains;
  auto chain_starts = s->chain_starts;
  auto chain_nexts = s->chain_nexts;
  auto keys = s->keys;
  auto vals = s->vals;

  auto hash = TraitsComplexKey::HashComplexKey( key );
  auto chain = hash % num_chains;
  auto chain_element = chain_starts[chain];
  Forever { // TODO: safety to prevent infinite loops? e.g. store the length of each chain and iterate up to that, and then crash.
    if( _Invalid( chain_element ) ) {
      *found = 0;
      return;
    }
    auto equal = TraitsComplexKey::EqualComplexKey( keys + chain_element, key );
    if( !equal ) {
      auto next = chain_nexts[chain_element];
      chain_element = next;
      continue;
    }
    *found = 1;
    *found_pval = vals + chain_element;
    return;
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
  Lookup( set, key, found, &found_pointer );
  if( found_val  &&  *found ) {
    *found_val = *found_pointer;
  }
}

TKCHE Inl
void
Remove(
  HASHSET* s,
  Key* key,
  bool* found,
  Val* found_val
  )
{
  ProfFunc();
  auto len = s->len;
  AssertCrash( len );
  auto capacity = s->capacity;
  auto num_chains = s->num_chains;
  auto chain_starts = s->chain_starts;
  auto chain_nexts = s->chain_nexts;
  auto free_keys = s->free_keys;
  auto keys = s->keys;
  auto vals = s->vals;

  auto hash = TraitsComplexKey::HashComplexKey( key );
  auto chain = hash % num_chains;
  auto pchain_element = chain_starts + chain;
  Forever { // TODO: safety to prevent infinite loops? e.g. store the length of each chain and iterate up to that, and then crash.
    auto chain_element = *pchain_element;
    if( _Invalid( chain_element ) ) {
      *found = 0;
      return;
    }
    auto equal = TraitsComplexKey::EqualComplexKey( keys + chain_element, key );
    if( !equal ) {
      auto next = chain_nexts + chain_element;
      pchain_element = next;
      continue;
    }
    *found = 1;
    *found_val = vals[chain_element];

#if 1
    len -= 1;
    free_keys[len] = chain_element;
    s->len = len;
#else
    free_keys[capacity - len] = chain_element; // TODO: try reversing the stack-direction of free_keys.
    s->len = len - 1;
#endif

    // repoint the chain_nexts/chain_start value to the element after this one, since we're removing it from the list.
    // if that happens to be invalid, great; we've correctly shortened/eliminated the list.
    auto next = chain_nexts[chain_element];
    *pchain_element = next;
    return;
  }
}

TKCHE Inl
void
Add(
  HASHSET* s,
  Key* key,
  Val* val,
  bool* already_there,
  Val* val_already_there,
  bool overwrite_val_if_already_there
  )
{
  ProfFunc();
  if( s->len >= s->capacity ) { // TODO: smarter resize conditions, e.g. chain statistics
    // TODO: smarter resize factor. E.g. adjust num_chains according to chain statistics
    Resize( s, 2 * s->num_chains, 2 * s->capacity );
  }
  auto len = s->len;
  auto capacity = s->capacity;
  AssertCrash( len < capacity );
  auto num_chains = s->num_chains;
  auto chain_starts = s->chain_starts;
  auto chain_nexts = s->chain_nexts;
  auto keys = s->keys;
  auto vals = s->vals;
  auto free_keys = s->free_keys;

  auto hash = TraitsComplexKey::HashComplexKey( key );
  auto chain = hash % num_chains;
  auto pchain_element = chain_starts + chain;
  Forever { // TODO: safety to prevent infinite loops? e.g. store the length of each chain and iterate up to that, and then crash.
    auto chain_element = *pchain_element;
    if( _Invalid( chain_element ) ) {
#if 1
      auto element = free_keys[ len ];
      s->len = len + 1;
#else
      len += 1;
      auto element = free_keys[capacity - len];
      s->len = len;
#endif

      *pchain_element = element;
      keys[element] = *key;
      vals[element] = *val;
      if( already_there ) {
        *already_there = 0;
      }
      return;
    }
    auto equal = TraitsComplexKey::EqualComplexKey( keys + chain_element, key );
    if( !equal ) {
      auto next = chain_nexts + chain_element;
      pchain_element = next;
      continue;
    }
    if( already_there ) {
      *already_there = 1;
    }
    if( val_already_there ) {
      *val_already_there = vals[chain_element];
    }
    if( overwrite_val_if_already_there ) {
      vals[chain_element] = *val;
    }
    return;
  }
}

TKCHE Inl
void
Init(
  HASHSET* s,
  idx_t num_chains,
  Index capacity
)
{
  ProfFunc();
  s->num_chains = num_chains;
  s->capacity = capacity;
  s->len = 0;

  // PERF: coalesce allocations, at least the idx_t ones.
  auto chain_starts = MemHeapAlloc( Index, num_chains );
  TSet( chain_starts, num_chains, InvalidIndex );
  s->chain_starts = chain_starts;
  auto chain_nexts = MemHeapAlloc( Index, capacity );
  TSet( chain_nexts, capacity, InvalidIndex );
  s->chain_nexts = chain_nexts;
  auto free_keys = MemHeapAlloc( Index, capacity );
  Fori( Index, i, 0, capacity ) {
    free_keys[i] = i;
  }
  s->free_keys = free_keys;
  s->keys = MemHeapAlloc( Key, capacity );
  s->vals = MemHeapAlloc( Val, capacity );
}

TKCHE Inl
void
Kill(
  HASHSET* s
)
{
  ProfFunc();
  MemHeapFree( s->chain_starts );
  MemHeapFree( s->chain_nexts );
  MemHeapFree( s->free_keys );
  MemHeapFree( s->keys );
  MemHeapFree( s->vals );
  Typezero( s );
}

TKCHE Inl
void
Resize(
  HASHSET* s,
  idx_t new_num_chains,
  Index new_capacity
  )
{
  ProfFunc();
  HASHSET new_s;
  Init( &new_s, new_num_chains, new_capacity );

  auto num_chains = s->num_chains;
  auto chain_starts = s->chain_starts;
  auto chain_nexts = s->chain_nexts;
  auto keys = s->keys;
  auto vals = s->vals;

  For( chain, 0, num_chains ) {
    auto chain_element = chain_starts[chain];
    Forever { // TODO: safety to prevent infinite loops? e.g. store the length of each chain and iterate up to that, and then crash.
      if( _Invalid( chain_element ) ) break;

      // PERF: if we stored hashes per element, we could do a slightly optimized, custom Add.
      //   avoiding rehashing everything might be a reasonable win, if we resize often enough.
      //   it comes at a sizeof(idx_t)*len memory cost though.
      bool already_there = 0;
      Add( &new_s, keys + chain_element, vals + chain_element, &already_there, (Val*)0, 0 );
      AssertCrash( !already_there );

      auto next = chain_nexts[chain_element];
      chain_element = next;
      continue;
    }
  }

  Kill( s );
  *s = new_s;
}

//TKCHE Inl void
//Flatten(
//  HASHSET* set,
//  idx_t* dst_hashes, // length set.len
//  Key* dst_keys, // length set.len
//  Val* dst_vals // length set.len
//  )
//{
//  ProfFunc();
//  idx_t ndst = 0;
//  auto capacity = set->capacity;
//  auto hashes = set->hashes;
//  auto keys = set->keys;
//  For( idx, 0, capacity ) {
//    auto slot_hash = hashes[idx];
//    auto has_data = slot_hash & HIGHBIT_idx;
//    if( !has_data ) {
//      continue;
//    }
//    auto hash_at_idx = slot_hash & ~HIGHBIT_idx;
//    if( dst_hashes ) {
//      dst_hashes[ndst] = hash_at_idx;
//    }
//    if( dst_keys ) {
//      dst_keys[ndst] = keys[idx];
//    }
//    if constexpr( !std::is_same_v<Val, void> ) {
//      if( dst_vals ) {
//        dst_vals[ndst] = set->vals[idx];
//      }
//    }
//    ndst += 1;
//  }
//  AssertCrash( ndst == set->len );
//}

#undef HASHSET
#undef TKCHE


RegisterTest([]()
{
  constant idx_t count = 256;

  hashset_chain_complexkey_t<u32*, idx_t, HashsetTraits_U32Pointer> set;
  Init( &set, count, 2 * count );

  idx_t tmp, tmp2;
  bool found, already_there;

  auto values = AllocString<u32*>( count );
  For( i, 0, count ) {
    values.mem[i] = (u32*)( i + 1 );
  }

  For( i, 0, count ) {

    AssertCrash( set.len == i );

    // verify all lookup mechanisms for all values already added.
    For( j, 0, i ) {
      LookupAndCopyValue( &set, &values.mem[j], &found, &tmp );
      AssertCrash( found );
      AssertCrash( tmp == j + 1 );

      idx_t* c = &tmp;
      Lookup( &set, &values.mem[j], &found, &c );
      AssertCrash( found );
      AssertCrash( *c == j + 1 );

      Add( &set, &values.mem[j], &tmp, &already_there, &tmp2, 0 );

      AssertCrash( already_there );
    }

    // add the actual new value.
    tmp = i + 1;
    Add( &set, &values.mem[i], &tmp, &already_there, &tmp2, 0 );
    AssertCrash( !already_there );
    AssertCrash( set.len == i + 1 );

    // validate it's been added.
    LookupAndCopyValue( &set, &values.mem[i], &found, &tmp );
    AssertCrash( found );
    AssertCrash( tmp == i + 1 );
  }

  // remove all the odd elements
  For( i, 0, count ) {
    if( i & 1 ) {
      Remove( &set, &values.mem[i], &found, &tmp );
      AssertCrash( found );
      AssertCrash( tmp == i + 1 );
    }
  }
  AssertCrash( set.len == count / 2 );

  // verify all the odd elements are gone, and evens are still there.
  For( i, 0, count ) {
    if( i & 1 ) {
      LookupAndCopyValue( &set, &values.mem[i], &found, &tmp );
      AssertCrash( !found );
    } else {
      LookupAndCopyValue( &set, &values.mem[i], &found, &tmp );
      AssertCrash( found );
      AssertCrash( tmp == i + 1 );
    }
  }

//  Clear( &set );
//  AssertCrash( set.len == 0 );
//
//  LookupAndCopyValue( &set, &values.mem[0], &found, &tmp );
//  AssertCrash( !found );
//
//  idx_t* c = &tmp2;
//  Lookup( &set, &values.mem[0], &found, &c );
//  AssertCrash( !found );

  Free( values );
  Kill( &set );
});
