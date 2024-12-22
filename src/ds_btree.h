// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// the famous B-tree!
//
// conceptually, it's an ordered list of unique keys. no duplicates allowed.
//
// the key concept here is that in each tree node, we're embedding ordering information as:
//   [ child[0], key[0], child[1], key[1], ..., key[M-1], child[M] ]
// that is, everything in child[0] is < key[0].
// and key[0] is < everything in child[1].
// and so on, for every node in the tree.
//
// since we're storing keys and children as separate arrays, this actually looks like:
//   [ child[0], child[1], ..., child[M-1], child[M] ]
//   [  keys[0],  keys[1], ...,  keys[M-1] ]
// note keys has the usual bounds of [0, M), and children has the special-case bounds [0, M].
//
// this ordering invariant is what makes all the relevant tree operations O( log N ) in time, where N = number of keys.
//
// the additional bonus here is that, unlike binary trees, we can effectively use hardware caches,
// by picking appropriate chunk sizes for our nodes. e.g. hard disk sector sizes for disk B-trees,
// CPU L1 cache line sizes for memory B-trees, etc.
//
// since i'm starting with writing a memory B-tree, the L1 cache line size of ~64B is what i'll be targeting initially here.
//

// TODO: ( key, value ) mappings ?
// TODO: complicated keys ?

// TODO: binary trees have the linear array / constant-time index mapping
//   these are N-ary, so presumably we could do the same kind of mapping.
//   this would save space at the cost of more memory moves.
//   could be worth trying.

// TODO: if we use pointer-sized keys, then we could use even/odd to mash the whole ordering in one array.
//   that probably wouldn't be worth it, but who knows.

// TODO: alignment for keys and children ?

// TODO: rename M to num_keys

// TODO: user macro type / template constant for btree_node_t?
//   that would simplify some of the memory nonsense, and make debugging a heck of a lot better.
//   also i'm getting memory corruption bugs that look like potential aliasing / UB problems?

// TODO: B+ tree


#define TC   template< typename T, idx_t C >

#define BTREENODE btree_node_t<T, C>
#define BTREE btree_t<T, C>

TC struct
btree_node_t
{
  // the memory packing looks like:
  //   [ M ] [ ... keys ... ] [ ... children ... ]
  // keys has length M
  // children has length M+1
  u32 M;
  T keys[C];
  BTREENODE* children[C+1];
  alloctype_t allocn;
};
TC Inl T*
Keys( BTREENODE* node )
{
  return node->keys;
}
TC Inl BTREENODE**
Children( BTREENODE* node )
{
  return node->children;
}
TC Inl BTREENODE*
AllocateBtreeNode()
{
  alloctype_t allocn = {};
  auto node = Allocate<BTREENODE>( &allocn, 1 );
  node->M = 0;
  Arrayzero( node->keys );
  Arrayzero( node->children );
  node->allocn = allocn;
  return node;
}

TC struct
btree_t
{
  BTREENODE* root;
  idx_t nbytes_node; // TODO: delete this if possible
};
TC Inl void
Init( BTREE* t )
{
  auto root = AllocateBtreeNode<T, C>();
  t->root = root;
  t->nbytes_node = sizeof( BTREENODE );
}
TC Inl void
Lookup(
  BTREE* t,
  T key,
  bool* found,
  BTREENODE** found_node,
  idx_t* found_idx
  )
{
  auto node = t->root;
  AssertCrash( node );
  // TODO: binary search for large C instead ?
  Forever {
    auto M = node->M;
    auto keys = Keys<T>( node );
    idx_t idx = 0;
    for( ; idx < M; ++idx ) {
      auto key_at_idx = keys[ idx ];
      if( key == key_at_idx ) {
        *found = 1;
        *found_node = node;
        *found_idx = idx;
        return;
      }
      if( key < key_at_idx ) {
        break;
      }
    }
    //
    // now we know key should be down the tree inside children[ idx ].
    // either we terminated the loop above early with idx < M,
    // or we reached the end with idx = M.
    //
    auto children = Children<T>( node );
    node = children[ idx ];
    if( !node ) {
      *found = 0;
      *found_node = 0;
      return;
    }
  }
  UnreachableCrash();
}

TC Inl void
_FlattenValues(
  BTREE* t,
  BTREENODE* node,
  stack_resizeable_cont_t<T>* result
  )
{
  auto M = node->M;
  auto children = Children<T>( node );
  auto keys = Keys<T>( node );
  idx_t idx = 0;
  for( ; idx < M; ++idx ) {
    auto child = children[ idx ];
    if( child ) {
      _FlattenValues( t, child, result );
    }
    *AddBack( *result ) = keys[ idx ];
  }
  {
    auto child = children[ idx ];
    if( child ) {
      _FlattenValues( t, child, result );
    }
  }
}
TC void
FlattenValues(
  BTREE* t,
  stack_resizeable_cont_t<T>* result
  )
{
  _FlattenValues( t, t->root, result );
}

TC Inl void
_Validate(
  BTREE* t,
  BTREENODE* node,
  T* min,
  T* max
  )
{
  auto M = node->M;
  auto children = Children<T>( node );
  auto keys = Keys<T>( node );
  AssertCrash( M <= C );
  idx_t idx = 0;
  for( ; idx < M; ++idx ) {
    auto child = children[ idx ];
    auto key_at_idx = keys[ idx ];
    *min = MIN( *min, key_at_idx );
    *max = MAX( *max, key_at_idx );
    if( child ) {
      T min_subtree;
      T max_subtree;
      _Validate( t, child, min_subtree, max_subtree );
      *min = MIN( *min, min_subtree );
      *max = MAX( *max, max_subtree );
      AssertCrash( min_subtree <= max_subtree );
      AssertCrash( max_subtree < key_at_idx );
      if( idx ) {
        AssertCrash( keys[ idx - 1 ] < min_subtree );
      }
    }
    if( idx + 1 < M ) {
      AssertCrash( key_at_idx < keys[ idx + 1 ] );
    }
  }
  {
    auto child = children[ idx ];
    if( child ) {
      T min_subtree;
      T max_subtree;
      _Validate( t, child, min_subtree, max_subtree );
      *min = MIN( *min, min_subtree );
      *max = MAX( *max, max_subtree );
      AssertCrash( min_subtree <= max_subtree );
      if( idx ) {
        AssertCrash( keys[ idx - 1 ] < min_subtree );
      }
    }
  }
}
TC Inl void
Validate(
  BTREE* t
  )
{
  T min_subtree;
  T max_subtree;
  _Validate( t, t->root, min_subtree, max_subtree );
  AssertCrash( min_subtree <= max_subtree );
}

TC ForceInl void
_InsertAtIdxAssumingRoom(
  BTREENODE* node,
  T* keys,
  BTREENODE** children,
  u32 M,
  u32 idx,
  T key
  )
{
  AssertCrash( idx <= M );
  //
  // there's room in this node to insert a new key!
  //
  // according to the ordering, child[i].keys[j] < keys[i] for all j.
  // so since our new key belongs at key[ idx ], we need to:
  //   make room by moving into keys[ > idx ], which is ( M - idx - 1 ) keys.
  //   maintain order by moving into children[ > idx ], which is ( M - idx ) children.
  //
  if( idx + 1 <= M ) {
    auto dst = keys + idx + 1;
    auto src = keys + idx;
    auto len = M - idx - 1;
    AssertCrash( keys <= dst  &&  dst + len <= keys + M );
    AssertCrash( keys <= src  &&  src + len <= keys + M );
    TMove( dst, src, len );
  }
  {
    auto dst = children + idx + 1;
    auto src = children + idx;
    auto len = M - idx;
    AssertCrash( children <= dst  &&  dst + len <= children + M + 1 );
    AssertCrash( children <= src  &&  src + len <= children + M + 1 );
    TMove( dst, src, len );
  }
  children[ idx ] = 0;
  keys[ idx ] = key;
  node->M += 1;
}


#define PARENTINFO   parent_info_t<T, C>
TC struct
parent_info_t
{
  BTREENODE* node;
  u32 idx;
};

// TODO: parent_chain can be stack memory most likely, since the tree branches exponentially; max height is reasonable.

#if 0
    // assumes
    // - node is the leaf part of parent_chain
    // - the key to insert belongs at idx.
    //
    // for internal nodes as we walk upwards, child_left, child_right are the result of the previous iterations's split.
    // we need to store these in the node's children to maintain the tree structure.
    //
    TC Inl void
    _Insert_WalkUpwardsRecur(
      BTREE* t,
      T key,
      stack_resizeable_cont_t<PARENTINFO>* parent_chain,
      BTREENODE* node,
      u32 idx,
      BTREENODE* child_left,
      BTREENODE* child_right
      )
    {
      auto leaf = !child_left && !child_right;
      auto nonleaf = child_left && child_right;
      AssertCrash( leaf != nonleaf );

      auto nbytes_node = t->nbytes_node;

      auto M = node->M;
      auto keys = Keys<T>( node );
      auto children = Children<T>( node );

      AssertCrash( M == C );

      auto new_right = Cast( BTREENODE*, AddBackBytes( *t->mem, _SIZEOF_IDX_T, nbytes_node ) );
      Memzero( new_right, nbytes_node );
      auto new_right_children = Children<T>( new_right );
      auto new_right_keys = Keys<T>( new_right );

      auto left = node;
      auto left_children = children;
      auto left_keys = keys;

      auto idx_median = M / 2;
      T key_median;
      if( idx == idx_median ) {
        //
        // keys:   1   3   5   7
        // chil: 0   2   4   6   8
        // idx:          ^
        // note idx points to effectively a children slot, between the key slots.
        // key splitting:
        //   left  gets keys: 1 3
        //   right gets keys: 5 7
        //   so in general,
        //   left  gets keys[ < idx_median ]
        //   right gets keys[ >= idx_median ]
        // the median key is just the key we're inserting.
        //
        // if leaf,
        //   no children, so nothing to do.
        // else nonleaf,
        //   left  gets children: 0 2 <left>
        //   right gets children: <new_right> 6 8
        //   so in general,
        //   left  gets children[ < idx_median ], plus left as the last thing.
        //   right gets children[ > idx_median ], plus new_right as the first thing.
        //
        key_median = key;

        auto len_right_keys = M - idx_median;
        new_right->M = len_right_keys;
        auto len_left_keys = idx_median;
        left->M = len_left_keys;
        auto len_right_children = len_right_keys + 1;
        auto len_left_children = len_left_keys + 1;
        {
          auto dst = new_right_keys;
          auto src = left_keys + idx_median;
          auto len = len_right_keys;
          AssertCrash( new_right_keys <= dst  &&  dst + len <= new_right_keys + len_right_keys );
          AssertCrash( left_keys <= src  &&  src + len <= left_keys + M );
          TMove( dst, src, len );
          TZero( src, len );
        }

        if( nonleaf ) {
          new_right_children[ 0 ] = child_right;
          {
            auto dst = new_right_children + 1;
            auto src = left_children + idx_median;
            auto len = M - idx_median;
            AssertCrash( new_right_children <= dst  &&  dst + len <= new_right_children + len_right_children );
            AssertCrash( left_children <= src  &&  src + len <= left_children + M + 1 );
            TMove( dst, src, len );
          }
          AssertCrash( len_left_keys < len_left_children );
          left_children[ len_left_keys ] = child_left;
          {
            auto dst = left_children + len_left_children;
            auto len = M - len_left_children;
            AssertCrash( left_children <= dst  &&  dst + len <= left_children + M + 1 );
            TZero( dst, len );
          }
        }
        //
        // leaves have all-zero children, so no need to maintain the arrays.
        //
      }
      elif( idx < idx_median ) {
        //
        // keys:   1   3   5   7   9    11
        // chil: 0   2   4   6   8   10    12
        // idx:      ^
        // idx_median:         ^
        // so the effective new keys list is: 1 <key> 3 5 7 9 11
        // key splitting:
        //   left  gets keys: 1 <key> 3
        //   right gets keys; 7 9 11
        //   median is 5
        //   so in general,
        //   left  gets keys[ < idx_median - 1 ], plus the key we're inserting.
        //   right gets keys[ >= idx_median ]
        //   the median key is keys[ idx_median - 1 ]
        //
        // if leaf,
        //   no children, so nothing to do.
        // else nonleaf,
        //   left  gets children: 0 <left> <new_right> 4
        //   right gets children: 6 8 10 12
        //   so in general,
        //   left  gets children[ < idx_median ], plus left and new_right in place at idx.
        //   right gets children[ >= idx_median ]
        //
        AssertCrash( idx_median - 1 < M );
        key_median = keys[ idx_median - 1 ];

        auto len_right_keys = M - idx_median;
        new_right->M = len_right_keys;
        auto len_left_keys = idx_median;
        left->M = len_left_keys;
        auto len_right_children = len_right_keys + 1;
        auto len_left_children = len_left_keys + 1;
        {
          auto dst = new_right_keys;
          auto src = left_keys + idx_median;
          auto len = len_right_keys;
          AssertCrash( new_right_keys <= dst  &&  dst + len <= new_right_keys + len_right_keys );
          AssertCrash( left_keys <= src  &&  src + len <= left_keys + M );
          TMove( dst, src, len );
          TZero( src, len );
        }
        //
        // insert key at idx into { left_keys, len_left_keys }.
        //
        {
          auto dst = left_keys + idx + 1;
          auto src = left_keys + idx;
          auto len = len_left_keys - idx - 1;
          AssertCrash( left_keys <= dst  &&  dst + len <= left_keys + len_left_keys );
          AssertCrash( left_keys <= src  &&  src + len <= left_keys + len_left_keys );
          TMove( dst, src, len );
        }
        AssertCrash( idx < len_left_keys );
        left_keys[idx] = key;

        if( nonleaf ) {
          {
            auto dst = new_right_children;
            auto src = left_children + idx_median;
            auto len = M + 1 - idx_median;
            AssertCrash( new_right_children <= dst  &&  dst + len <= new_right_children + len_right_children );
            AssertCrash( left_children <= src  &&  src + len <= left_children + M + 1 );
            TMove( dst, src, len );
            TZero( src, len );
          }
          {
            auto dst = left_children + idx;
            auto src = left_children + idx + 1;
            auto len = len_left_children - idx - 1;
            TMove( dst, src, len );
          }
          left_children[ idx ] = child_left;
          left_children[ idx + 1 ] = child_right;
        }
      }
      else { // idx > idx_median
        //
        // keys:   1   3   5   7   9    11
        // chil: 0   2   4   6   8   10    12
        // idx:                       ^
        // idx_median:         ^
        // so the effective new keys list is: 1 3 5 7 9 <key> 11
        // key splitting:
        //   left  gets keys: 1 3 5
        //   right gets keys: 9 <key> 11
        //   median is 7
        //   so in general,
        //   left  gets keys[ < idx_median ]
        //   right gets keys[ > idx_median ], plus the key we're inserting
        //   the median key is keys[ idx_median ]
        //
        // if leaf,
        //   no children, so nothing to do.
        // else nonleaf,
        //   left  gets children: 0 2 4 6
        //   right gets children: 8 <left> <new_right> 12
        //   so in general,
        //   left  gets children[ <= idx_median ]
        //   right gets children[ > idx_median ], plus left and new_right in place at idx.
        //
        key_median = keys[ idx_median ];

        auto len_right_keys = M - idx_median;
        new_right->M = len_right_keys;
        auto len_left_keys = idx_median;
        left->M = len_left_keys;
        auto len_right_children = len_right_keys + 1;
        auto len_left_children = len_left_keys + 1;
        {
          auto dst = new_right_keys;
          auto src = left_keys + idx_median + 1;
          auto len = idx - idx_median - 1;
          AssertCrash( new_right_keys <= dst  &&  dst + len <= new_right_keys + len_right_keys );
          AssertCrash( left_keys <= src  &&  src + len <= left_keys + M );
          TMove( dst, src, len );
        }
        AssertCrash( idx - idx_median - 1 < len_right_keys );
        new_right_keys[ idx - idx_median - 1 ] = key;
        {
          auto dst = new_right_keys + idx - idx_median;
          auto src = left_keys + idx;
          auto len = M - idx;
          AssertCrash( new_right_keys <= dst  &&  dst + len <= new_right_keys + len_right_keys );
          AssertCrash( left_keys <= src  &&  src + len <= left_keys + M );
          TMove( dst, src, len );
        }
        {
          auto dst = left_keys + len_left_keys;
          auto len = M - len_left_keys;
          AssertCrash( left_keys <= dst  &&  dst + len <= left_keys + M );
          TZero( dst, len );
        }

        if( nonleaf ) {
          {
            auto dst = new_right_children;
            auto src = left_children + idx_median + 1;
            auto len = idx - idx_median - 1;
            AssertCrash( new_right_children <= dst  &&  dst + len <= new_right_children + len_right_children );
            AssertCrash( left_children <= src  &&  src + len <= left_children + M + 1 );
            TMove( dst, src, len );
          }
          AssertCrash( idx - idx_median - 1 < len_right_children );
          AssertCrash( idx - idx_median < len_right_children );
          new_right_children[ idx - idx_median - 1 ] = child_left;
          new_right_children[ idx - idx_median ] = child_right;
          {
            auto dst = new_right_children + idx - idx_median + 1;
            auto src = left_children + idx + 1;
            auto len = M - idx;
            AssertCrash( new_right_children <= dst  &&  dst + len <= new_right_children + len_right_children );
            AssertCrash( left_children <= src  &&  src + len <= left_children + M + 1 );
            TMove( dst, src, len );
          }
          {
            auto dst = left_children + len_left_children;
            auto len = M + 1 - len_left_children;
            AssertCrash( left_children <= dst  &&  dst + len <= left_children + M + 1 );
            TZero( dst, len );
          }
        }
      }

      //
      // now we have the modified left, and new_right nodes.
      // we still have to pull the key_median up into the parent_chain
      //

      if( !parent_chain->len ) {
        //
        // we're at the root, and it's full.
        // this means we need to make a new root, and set it's children to [ left, new_right ].
        //
        auto new_root = Cast( BTREENODE*, AddBackBytes( *t->mem, _SIZEOF_IDX_T, nbytes_node ) );
        Memzero( new_root, nbytes_node );
        new_root->M = 1;
        auto new_root_children = Children<T>( new_root );
        auto new_root_keys = Keys<T>( new_root );
        new_root_keys[0] = key_median;
        new_root_children[0] = left;
        new_root_children[1] = new_right;
        t->root = new_root;

        return;
      }

      auto parent_info = parent_chain->mem + parent_chain->len - 1;
      auto parent = parent_info->node;
      auto idx_into_parent = parent_info->idx;
      auto parent_M = parent->M;
      if( parent_M < C ) {
        //
        // parent has room for the key_median.
        //
        auto parent_keys = Keys<T>( parent );
        auto parent_children = Children<T>( parent );

        //
        // we need to update parent's children array to point to left, new_right.
        //
        //     parent
        // C_left  C_right
        //
        if( idx_into_parent + 1 <= parent_M ) {
          auto dst = parent_keys + idx_into_parent + 1;
          auto src = parent_keys + idx_into_parent;
          auto len = parent_M - idx_into_parent - 1;
          AssertCrash( parent_keys <= dst  &&  dst + len <= parent_keys + parent_M );
          AssertCrash( parent_keys <= src  &&  src + len <= parent_keys + parent_M );
          TMove( dst, src, len );
        }
        {
          auto dst = parent_children + idx_into_parent + 1;
          auto src = parent_children + idx_into_parent;
          auto len = parent_M - idx_into_parent;
          AssertCrash( parent_children <= dst  &&  dst + len <= parent_children + parent_M + 1 );
          AssertCrash( parent_children <= src  &&  src + len <= parent_children + parent_M + 1 );
          TMove( dst, src, len );
        }
        parent_children[ idx_into_parent ] = left;
        parent_children[ idx_into_parent + 1 ] = new_right;
        parent_keys[ idx_into_parent ] = key_median;
        parent->M += 1;

        return;
      }
      //
      // parent is full! we have to recurse to split the parent, and continue up the parent_chain.
      //
      // note we can't drop left, new_right here! we need to push these to the parent somehow.
      // since the parent might be full and require more splitting, we can't do it here; the logic is above for nonleaf nodes.
      //
      // TODO: tail recursion can be unrolled.
      //
      RemBack( *parent_chain );
      _Insert_WalkUpwardsRecur<T,C>( t, key_median, parent_chain, parent, idx_into_parent, left, new_right );
    }

#endif

TC Inl void
_Insert_WalkUpwards(
  BTREE* t,
  T key,
  stack_resizeable_cont_t<PARENTINFO>* parent_chain,
  BTREENODE* node,
  u32 idx
  )
{
  // as we walk upwards, these two children track the subtree roots we need to find parents for.
  // since we start from a leaf, these start as empty.
  BTREENODE* child_left = 0;
  BTREENODE* child_right = 0;
  Forever {
    auto leaf = !child_left && !child_right;
    auto nonleaf = child_left && child_right;
    AssertCrash( leaf != nonleaf );

    auto nbytes_node = t->nbytes_node;

    auto M = node->M;
    auto keys = Keys<T>( node );
    auto children = Children<T>( node );

    AssertCrash( M == C );

    auto new_right = AllocateBtreeNode<T, C>();
    auto new_right_children = Children<T>( new_right );
    auto new_right_keys = Keys<T>( new_right );

    auto left = node;
    auto left_children = children;
    auto left_keys = keys;

    auto idx_median = M / 2;
    T key_median;
    if( idx == idx_median ) {
      //
      // keys:   1   3   5   7
      // chil: 0   2   4   6   8
      // idx:          ^
      // note idx points to effectively a children slot, between the key slots.
      // key splitting:
      //   left  gets keys: 1 3
      //   right gets keys: 5 7
      //   so in general,
      //   left  gets keys[ < idx_median ]
      //   right gets keys[ >= idx_median ]
      // the median key is just the key we're inserting.
      //
      // if leaf,
      //   no children, so nothing to do.
      // else nonleaf,
      //   left  gets children: 0 2 <left>
      //   right gets children: <new_right> 6 8
      //   so in general,
      //   left  gets children[ < idx_median ], plus left as the last thing.
      //   right gets children[ > idx_median ], plus new_right as the first thing.
      //
      key_median = key;

      auto len_right_keys = M - idx_median;
      new_right->M = len_right_keys;
      auto len_left_keys = idx_median;
      left->M = len_left_keys;
      auto len_right_children = len_right_keys + 1;
      auto len_left_children = len_left_keys + 1;
      {
        auto dst = new_right_keys;
        auto src = left_keys + idx_median;
        auto len = len_right_keys;
        AssertCrash( new_right_keys <= dst  &&  dst + len <= new_right_keys + len_right_keys );
        AssertCrash( left_keys <= src  &&  src + len <= left_keys + M );
        TMove( dst, src, len );
        TZero( src, len );
      }

      if( nonleaf ) {
        new_right_children[ 0 ] = child_right;
        {
          auto dst = new_right_children + 1;
          auto src = left_children + idx_median;
          auto len = M - idx_median;
          AssertCrash( new_right_children <= dst  &&  dst + len <= new_right_children + len_right_children );
          AssertCrash( left_children <= src  &&  src + len <= left_children + M + 1 );
          TMove( dst, src, len );
        }
        AssertCrash( len_left_keys < len_left_children );
        left_children[ len_left_keys ] = child_left;
        {
          auto dst = left_children + len_left_children;
          auto len = M - len_left_children;
          AssertCrash( left_children <= dst  &&  dst + len <= left_children + M + 1 );
          TZero( dst, len );
        }
      }
      //
      // leaves have all-zero children, so no need to maintain the arrays.
      //
    }
    elif( idx < idx_median ) {
      //
      // keys:   1   3   5   7   9    11
      // chil: 0   2   4   6   8   10    12
      // idx:      ^
      // idx_median:         ^
      // so the effective new keys list is: 1 <key> 3 5 7 9 11
      // key splitting:
      //   left  gets keys: 1 <key> 3
      //   right gets keys; 7 9 11
      //   median is 5
      //   so in general,
      //   left  gets keys[ < idx_median - 1 ], plus the key we're inserting.
      //   right gets keys[ >= idx_median ]
      //   the median key is keys[ idx_median - 1 ]
      //
      // if leaf,
      //   no children, so nothing to do.
      // else nonleaf,
      //   left  gets children: 0 <left> <new_right> 4
      //   right gets children: 6 8 10 12
      //   so in general,
      //   left  gets children[ < idx_median ], plus left and new_right in place at idx.
      //   right gets children[ >= idx_median ]
      //
      AssertCrash( idx_median - 1 < M );
      key_median = keys[ idx_median - 1 ];

      auto len_right_keys = M - idx_median;
      new_right->M = len_right_keys;
      auto len_left_keys = idx_median;
      left->M = len_left_keys;
      auto len_right_children = len_right_keys + 1;
      auto len_left_children = len_left_keys + 1;
      {
        auto dst = new_right_keys;
        auto src = left_keys + idx_median;
        auto len = len_right_keys;
        AssertCrash( new_right_keys <= dst  &&  dst + len <= new_right_keys + len_right_keys );
        AssertCrash( left_keys <= src  &&  src + len <= left_keys + M );
        TMove( dst, src, len );
        TZero( src, len );
      }
      //
      // insert key at idx into { left_keys, len_left_keys }.
      //
      {
        auto dst = left_keys + idx + 1;
        auto src = left_keys + idx;
        auto len = len_left_keys - idx - 1;
        AssertCrash( left_keys <= dst  &&  dst + len <= left_keys + len_left_keys );
        AssertCrash( left_keys <= src  &&  src + len <= left_keys + len_left_keys );
        TMove( dst, src, len );
      }
      AssertCrash( idx < len_left_keys );
      left_keys[idx] = key;

      if( nonleaf ) {
        {
          auto dst = new_right_children;
          auto src = left_children + idx_median;
          auto len = M + 1 - idx_median;
          AssertCrash( new_right_children <= dst  &&  dst + len <= new_right_children + len_right_children );
          AssertCrash( left_children <= src  &&  src + len <= left_children + M + 1 );
          TMove( dst, src, len );
          TZero( src, len );
        }
        {
          auto dst = left_children + idx;
          auto src = left_children + idx + 1;
          auto len = len_left_children - idx - 1;
          TMove( dst, src, len );
        }
        left_children[ idx ] = child_left;
        left_children[ idx + 1 ] = child_right;
      }
    }
    else { // idx > idx_median
      //
      // keys:   1   3   5   7   9    11
      // chil: 0   2   4   6   8   10    12
      // idx:                       ^
      // idx_median:         ^
      // so the effective new keys list is: 1 3 5 7 9 <key> 11
      // key splitting:
      //   left  gets keys: 1 3 5
      //   right gets keys: 9 <key> 11
      //   median is 7
      //   so in general,
      //   left  gets keys[ < idx_median ]
      //   right gets keys[ > idx_median ], plus the key we're inserting
      //   the median key is keys[ idx_median ]
      //
      // if leaf,
      //   no children, so nothing to do.
      // else nonleaf,
      //   left  gets children: 0 2 4 6
      //   right gets children: 8 <left> <new_right> 12
      //   so in general,
      //   left  gets children[ <= idx_median ]
      //   right gets children[ > idx_median ], plus left and new_right in place at idx.
      //
      key_median = keys[ idx_median ];

      auto len_right_keys = M - idx_median;
      new_right->M = len_right_keys;
      auto len_left_keys = idx_median;
      left->M = len_left_keys;
      auto len_right_children = len_right_keys + 1;
      auto len_left_children = len_left_keys + 1;
      {
        auto dst = new_right_keys;
        auto src = left_keys + idx_median + 1;
        auto len = idx - idx_median - 1;
        AssertCrash( new_right_keys <= dst  &&  dst + len <= new_right_keys + len_right_keys );
        AssertCrash( left_keys <= src  &&  src + len <= left_keys + M );
        TMove( dst, src, len );
      }
      AssertCrash( idx - idx_median - 1 < len_right_keys );
      new_right_keys[ idx - idx_median - 1 ] = key;
      {
        auto dst = new_right_keys + idx - idx_median;
        auto src = left_keys + idx;
        auto len = M - idx;
        AssertCrash( new_right_keys <= dst  &&  dst + len <= new_right_keys + len_right_keys );
        AssertCrash( left_keys <= src  &&  src + len <= left_keys + M );
        TMove( dst, src, len );
      }
      {
        auto dst = left_keys + len_left_keys;
        auto len = M - len_left_keys;
        AssertCrash( left_keys <= dst  &&  dst + len <= left_keys + M );
        TZero( dst, len );
      }

      if( nonleaf ) {
        {
          auto dst = new_right_children;
          auto src = left_children + idx_median + 1;
          auto len = idx - idx_median - 1;
          AssertCrash( new_right_children <= dst  &&  dst + len <= new_right_children + len_right_children );
          AssertCrash( left_children <= src  &&  src + len <= left_children + M + 1 );
          TMove( dst, src, len );
        }
        AssertCrash( idx - idx_median - 1 < len_right_children );
        AssertCrash( idx - idx_median < len_right_children );
        new_right_children[ idx - idx_median - 1 ] = child_left;
        new_right_children[ idx - idx_median ] = child_right;
        {
          auto dst = new_right_children + idx - idx_median + 1;
          auto src = left_children + idx + 1;
          auto len = M - idx;
          AssertCrash( new_right_children <= dst  &&  dst + len <= new_right_children + len_right_children );
          AssertCrash( left_children <= src  &&  src + len <= left_children + M + 1 );
          TMove( dst, src, len );
        }
        {
          auto dst = left_children + len_left_children;
          auto len = M + 1 - len_left_children;
          AssertCrash( left_children <= dst  &&  dst + len <= left_children + M + 1 );
          TZero( dst, len );
        }
      }
    }

    //
    // now we have the modified left, and new_right nodes.
    // we still have to pull the key_median up into the parent_chain
    //

    if( !parent_chain->len ) {
      //
      // we're at the root, and it's full.
      // this means we need to make a new root, and set it's children to [ left, new_right ].
      //
      auto new_root = AllocateBtreeNode<T, C>();
      new_root->M = 1;
      auto new_root_children = Children<T>( new_root );
      auto new_root_keys = Keys<T>( new_root );
      new_root_keys[0] = key_median;
      new_root_children[0] = left;
      new_root_children[1] = new_right;
      t->root = new_root;

      return;
    }

    auto parent_info = parent_chain->mem + parent_chain->len - 1;
    auto parent = parent_info->node;
    auto idx_into_parent = parent_info->idx;
    auto parent_M = parent->M;
    if( parent_M < C ) {
      //
      // parent has room for the key_median.
      //
      auto parent_keys = Keys<T>( parent );
      auto parent_children = Children<T>( parent );

      //
      // we need to update parent's children array to point to left, new_right.
      //
      //     parent
      // C_left  C_right
      //
      if( idx_into_parent + 1 <= parent_M ) {
        auto dst = parent_keys + idx_into_parent + 1;
        auto src = parent_keys + idx_into_parent;
        auto len = parent_M - idx_into_parent - 1;
        AssertCrash( parent_keys <= dst  &&  dst + len <= parent_keys + parent_M );
        AssertCrash( parent_keys <= src  &&  src + len <= parent_keys + parent_M );
        TMove( dst, src, len );
      }
      {
        auto dst = parent_children + idx_into_parent + 1;
        auto src = parent_children + idx_into_parent;
        auto len = parent_M - idx_into_parent;
        AssertCrash( parent_children <= dst  &&  dst + len <= parent_children + parent_M + 1 );
        AssertCrash( parent_children <= src  &&  src + len <= parent_children + parent_M + 1 );
        TMove( dst, src, len );
      }
      parent_children[ idx_into_parent ] = left;
      parent_children[ idx_into_parent + 1 ] = new_right;
      parent_keys[ idx_into_parent ] = key_median;
      parent->M += 1;

      return;
    }
    //
    // parent is full! we have to recurse to split the parent, and continue up the parent_chain.
    //
    // note we can't drop left, new_right here! we need to push these to the parent somehow.
    // since the parent might be full and require more splitting, we can't do it here; the logic is above for nonleaf nodes.
    //
    RemBack( *parent_chain );
    // old recursion looked like:
    // _Insert_WalkUpwardsRecur<T,C>( t, key_median, parent_chain, parent, idx_into_parent, left, new_right );
    key = key_median;
    node = parent;
    idx = idx_into_parent;
    child_left = left;
    child_right = new_right;
    continue;
  }
}

TC Inl void
Insert(
  BTREE* t,
  T key,
  bool* already_there,
  stack_resizeable_cont_t<PARENTINFO>* parent_chain
  )
{
#if DEBUGSLOW
  Validate( t );
#endif

  auto root = t->root;
  AssertCrash( root );

  parent_chain->len = 0;

  auto node = root;
  u32 idx = 0;
  Forever {
    auto M = node->M;
    auto keys = Keys<T>( node );
    auto children = Children<T>( node );
    AssertCrash( M <= C );
    //
    // search for a key match in this node.
    // if we find one, great, we're done.
    // otherwise, we'll have to iterate into the specific, ordered child that should contain it.
    //
    // TODO: binary search for large C instead ?
    //
    for( idx = 0; idx < M; ++idx ) {
      auto key_at_idx = keys[ idx ];
      if( key == key_at_idx ) {
        *already_there = 1;
        return;
      }
      if( key < key_at_idx ) {
        break;
      }
    }

    auto child_node = children[ idx ];
    if( child_node ) {
      auto parent_info = AddBack( *parent_chain );
      parent_info->node = node;
      parent_info->idx = idx;
      node = child_node;
      continue;
    }

    if( M < C ) {
      _InsertAtIdxAssumingRoom( node, keys, children, M, idx, key );
      *already_there = 0;
      return;
    }

#if DBG
    For( i, 0, M + 1 ) {
      AssertCrash( !children[i] );
    }
#endif

    break;
  }

  *already_there = 0;
  _Insert_WalkUpwards( t, key, parent_chain, node, idx );

#if DEBUGSLOW
  Validate( t );
#endif
}



#if 0

  recursive insert:

  build the node-chain from the root to the leaf node that should contain the new key.
    if any nodes contain the identical key, exit early.

  if the leaf node has room for insertion,


TC Inl void
Insert(
  BTREE* t,
  T key,
  bool* already_there
  )
{
#if DEBUGSLOW
  Validate( t );
#endif

  auto root = t->root;
  AssertCrash( root );

  auto node = root;
  u32 idx = 0;
  Forever {
    auto M = node->M;
    auto keys = Keys<T>( node );
    auto children = Children<T>( node );
    AssertCrash( M <= C );
    //
    // search for a key match in this node.
    // if we find one, great, we're done.
    // otherwise, we'll have to iterate into the specific, ordered child that should contain it.
    //
    // TODO: binary search for large C instead ?
    //
    for( idx = 0; idx < M; ++idx ) {
      auto key_at_idx = keys[ idx ];
      if( key == key_at_idx ) {
        *already_there = 1;
        return;
      }
      if( key < key_at_idx ) {
        break;
      }
    }



  }

#if DEBUGSLOW
  Validate( t );
#endif
}


#endif

#define MIN_CAPACITY ( C / 2 )

// assumes
// - node is the leaf part of parent_chain
// - node has fewer than the minimum allowed number of keys: ( M < C / 2 )
//
TC Inl void
_DeleteRebalance_WalkUpwardsRecur(
  BTREE* t,
  stack_resizeable_cont_t<PARENTINFO>* parent_chain,
  BTREENODE* node
  )
{
  Forever {
    auto M = node->M;
    AssertCrash( M < MIN_CAPACITY );

    if( !parent_chain->len ) {
      return;
    }

    //
    // basic rebalancing algorithm:
    //   if left has keys to spare, then rotate the left's last key/child over towards node.
    //   if right has keys to spare, then rotate the right's first key/child over towards node.
    //   else merge ( left, node ) together, or equivalently ( node, right ), stealing the parent
    //     separator key, and then recurse on the parent if we have to.
    //
    // we have two independent algorithmic choices here:
    // choice A:
    //   1. check left for spares before right; OR,
    //   2. check right for spares before left.
    //
    // choice B:
    //   1. merge ( left, node ) together in the else-case; OR,
    //   2. merge ( node, right ) together in the else-case.
    //
    // for choice A.1:
    //   we're popping from the end of left's arrays, which is nice.
    //   we're pushing to the front of node's arrays, which is less nice.
    // note that choice A.2 is basically the same, by symmetry.
    // so i think we'll do an equivalent amount of memory-moving with A.1 as with A.2.
    //
    // however, maybe there's a minor balancing win here?
    // if you insert in sorted order, all insertions happen at the rightmost edge of the tree,
    // leaving basically all nodes half-full. and there's a special bulk-loading algorithm to
    // improve on this, making most nodes full instead of half-full.
    //
    // the same kind of balancing question applies to choice B.1 vs B.2, and i'm not sure.
    //
    // TODO: what if we opportunistically merge? e.g. check left for spares or merging, before even considering right.
    //

    auto parent_info = parent_chain->mem + parent_chain->len - 1;
    auto parent = parent_info->node;
    auto idx_into_parent = parent_info->idx;
    auto parent_M = parent->M;
    auto parent_keys = Keys<T>( parent );
    auto parent_children = Children<T>( parent );
    AssertCrash( parent_children[ idx_into_parent ] == node );

    // keys:   1   3   5   7
    // chil: 0   2   4   6   8
    // node:         ^
    // left:     ^
    // right:            ^

    if( idx_into_parent ) {
      auto idx_left_into_parent = idx_into_parent - 1;
      auto left = parent_children[ idx_left_into_parent ];
      auto left_M = left->M;
      if( left_M > MIN_CAPACITY ) {
        // move the parent separator key into the start of node
        // move the left's last child to the start of node
        // move the left's last key to the parent separator

        // TODO: implement

        // we are now rebalanced!
        return;
      }
    }

    auto idx_right_into_parent = idx_into_parent + 1;
    if( idx_right_into_parent <= parent_M ) {
      auto right = parent_children[ idx_right_into_parent ];
      auto right_M = right->M;
      if( right_M > MIN_CAPACITY ) {
        // move the parent separator key into the end of node
        // move the right's first child into the end of node
        // move the right's first key into the parent separator

        // TODO: implement

        // we are now rebalanced!
        return;
      }
    }

    if( idx_into_parent ) {
      auto idx_left_into_parent = idx_into_parent - 1;
      auto left = parent_children[ idx_left_into_parent ];
      auto left_M = left->M;
      // WARNING: we're tying choice A.1 with choice B.1 here.
      AssertCrash( left_M == MIN_CAPACITY );

      //
      // conceptually we're merging ( left, node ) into one node.
      //
      // move the parent separator key into the end of left
      // move all keys from node into the end of left, after the moved separator key
      // move all children from node into the end of left
      // delete the parent separator key
      // delete node from the parent's children
      // if the parent is root and is now empty, delete it and make left the new root.
      // else, if the parent fell below min capacity, recurse on it.
      //

      // TODO: implement

      return;
    }

    if( idx_right_into_parent <= parent_M ) {
      auto right = parent_children[ idx_right_into_parent ];
      auto right_M = right->M;
      // WARNING: we're tying choice A.1 with choice B.1 here.
      AssertCrash( right_M == MIN_CAPACITY );

      //
      // conceptually we're merging ( node, right ) into one node.
      // note this is precisely the same algorithm as above.
      //
      // move the parent separator key into the end of node
      // move all keys from right into the end of node, after the moved separator key
      // move all children from right into the end of node
      // delete the parent separator key
      // delete right from the parent's children
      // if the parent is root and is now empty, delete it and make node the new root.
      // else, if the parent fell below min capacity, recurse on it.
      //

      // TODO: implement

      return;
    }

    // we should have returned by now, but the logic is complicated enough that this is a safety net.
    UnreachableCrash();
  }
}

TC Inl void
DeleteRecur(
  BTREE* t,
  T key,
  bool* deleted,
  stack_resizeable_cont_t<PARENTINFO>* parent_chain
  )
{
#if DEBUGSLOW
  Validate( t );
#endif

  auto root = t->root;
  AssertCrash( root );

  parent_chain->len = 0;

  auto node = root;
  u32 idx = 0;
  Forever {
    auto M = node->M;
    auto keys = Keys<T>( node );
    auto children = Children<T>( node );
    AssertCrash( M <= C );
    //
    // search for a key match in this node.
    // if we find one, great, we're done.
    // otherwise, we'll have to iterate into the specific, ordered child that should contain it.
    //
    // TODO: binary search for large C instead ?
    //
    bool found = 0;
    for( idx = 0; idx < M; ++idx ) {
      auto key_at_idx = keys[ idx ];
      if( key == key_at_idx ) {
        found = 1;
        break;
      }
      if( key < key_at_idx ) {
        break;
      }
    }

    auto child_node = children[ idx ];
    if( !found && child_node ) {
      auto parent_info = AddBack( *parent_chain );
      parent_info->node = node;
      parent_info->idx = idx;
      node = child_node;
      continue;
    }

    // TODO: store a leaf/nonleaf bit in every node?
    //   that would let us make smaller-memory leaf nodes.
    bool nonleaf = 0;
    For( i, 0, M + 1 ) {
      if( children[i] ) {
        nonleaf = 1;
        break;
      }
    }

    if( !found && !nonleaf ) {
      // not found within the tree!
      *deleted = 0;
      return;
    }

    //
    // now we can assume found=true, and we're either a leaf or nonleaf.
    //
    AssertCrash( found );

    if( !nonleaf ) {
      {
        auto dst = keys + idx;
        auto src = keys + idx + 1;
        auto len = M - idx - 1;
        AssertCrash( keys <= dst  &&  dst + len <= keys + M );
        AssertCrash( keys <= src  &&  src + len <= keys + M );
        TMove( dst, src, len );
      }

      M -= 1;
      node->M = M;
      if( M < MIN_CAPACITY ) {
        // deletion rebalancing that walks upwards.
        _DeleteRebalance_WalkUpwardsRecur<T,C>( t, parent_chain, node );
      }
      return;
    }

    //
    // now we can assume found=true and nonleaf=true
    //

    //
    // we can either:
    //   1. choose the min key of the subtree to the right; OR,
    //   2. choose the max key of the subtree to the left.
    // both are equivalent logically.
    // option 1 will end up moving slightly more memory, since we're popping idx=0 from an array, effectively.
    // so i think option 2 is slightly preferable.
    //
    // so that's what we're doing:
    //   2. choose the max key of the subtree to the left.
    //
    // note we need to set up the parent_chain above the leaf so we can do the upwards walk later.
    //
    auto left_node = children[ idx ];
    AssertCrash( left_node );
    u32 left_M;
    BTREENODE** left_children;
    Forever {
      left_M = left_node->M;
      left_children = Children<T>( left_node );
      auto next_left = left_children[ left_M ];
      if( next_left ) {
        auto parent_info = AddBack( *parent_chain );
        parent_info->node = left_node;
        parent_info->idx = left_M;
        left_node = next_left;
        continue;
      }
      break;
    }
    //
    // now we know left_node is the leaf node.
    // move the max key of the left subtree into this nonleaf node.
    //
    T* left_keys = Keys<T>( left_node );
    keys[ idx ] = left_keys[ left_M ];
    left_M -= 1;
    left_node->M = left_M;
    if( left_M < MIN_CAPACITY ) {
      _DeleteRebalance_WalkUpwardsRecur<T,C>( t, parent_chain, left_node );
    }

    break; // TODO: move this up?
  }

#if DEBUGSLOW
  Validate( t );
#endif
}



#define CC 10
struct btnode {
  std::vector<int> keys;
  std::vector<btnode*> children;

  bool leaf() { return children.empty(); }
  void insertKey(size_t i, int key) {
    AssertCrash(i <= keys.size());
    keys.insert(std::begin(keys)+i, key);
  }
  void insertChild(size_t i, btnode* c) {
    AssertCrash(i <= children.size());
    children.insert(std::begin(children)+i, c);
  }
  void removeKey(size_t i) {
    AssertCrash(i < keys.size());
    keys.erase(std::begin(keys)+i);
  }
  void removeChild(size_t i) {
    AssertCrash(i < children.size());
    children.erase(std::begin(children)+i);
  }

  static btnode* Alloc() {
    auto r = new btnode;
    return r;
  }
  static void Free(btnode* t) {
    for (auto c : t->children) {
      Free(c);
      delete c;
    }
    delete t;
  }

  void Lookup(int key, size_t& index, bool& found) {
    found = false;
    size_t i = 0;
    for (; i < keys.size(); ++i) {
      auto cmp = key <=> keys[i];
      if (cmp == 0) {
        found = 1;
        break;
      }
      else if (cmp < 0) {
        break;
      }
    }
    index = i;
  }
};

struct btree {
  btnode* root;

  btree() {
    root = btnode::Alloc();
  }
  ~btree() {
    btnode::Free(root);
  }

  void Lookup(int key, bool& found) {
    found = 0;
    auto t = root;
    while (t) {
      size_t index;
      t->Lookup(key, index, found);
      if (found) return;

      // key should be down the tree in children[index].
      t = t->children[index];
    }
  }

  struct parentelt {
    btnode* t;
    size_t index;
  };

  void Delete(int key, bool& deleted) {
    deleted = 0;

    std::vector<parentelt> parents;
    auto t = root;
    btnode* rebalance = 0;
    Forever {
      size_t index;
      bool found = 0;
      t->Lookup(key, index, found);

      const auto leaf = t->leaf();
      if (!found) {
        if (leaf) {
          // not found!
          return;
        }

        parents.push_back(parentelt{t, index});
        t = t->children[index];
        continue;
      }

      if (leaf) {
        t->removeKey(index);
        rebalance = t;
        break;
      }

      // choose the max key of the left subtree.
      auto L = t->children[index];
      while (!L->leaf()) {
        parents.push_back(parentelt{L, L->children.size()-1});
        L = L->children.back();
      }
      // move the max key of the left subtree into this nonleaf node.
      t->keys[index] = L->keys.back();
      L->keys.resize(L->keys.size()-1);
      rebalance = L;
      break;
    }
    deleted = 1;
    if (rebalance->keys.size() >= CC/2) return; // no need to rebalance.

    // _DeleteRebalance_WalkUpwardsRecur( parents, rebalance );
    Forever {
      if (parents.empty()) break;

      auto parent = parents.back().t;
      auto index_in_parent = parents.back().index;
      parents.pop_back();

      btnode* L = (index_in_parent > 0) ? parent->children[index_in_parent-1] : 0;
      btnode* R = (index_in_parent+1 < parent->children.size()) ? parent->children[index_in_parent+1] : 0;
      if (L && L->keys.size() > CC/2) {
        rebalance->insertKey(0, parent->keys[index_in_parent]);
        rebalance->insertChild(0, L->children.back());
        L->children.resize(L->children.size()-1);
        parent->keys[index_in_parent] = L->keys.back();
        L->keys.resize(L->keys.size()-1);
        break;
      }
      if (R && R->keys.size() > CC/2) {
        rebalance->keys.push_back(parent->keys[index_in_parent]);
        rebalance->children.push_back(R->children[0]);
        R->removeChild(0);
        parent->keys[index_in_parent] = R->keys[0];
        R->removeKey(0);
        break;
      }
      if (L) {
        AssertCrash(L->keys.size() == CC/2);
        L->keys.push_back(parent->keys[index_in_parent]);
        for (size_t i = 0; i < rebalance->keys.size(); ++i) {
          L->keys.emplace_back(std::move(rebalance->keys[i]));
        }
        for (size_t i = 0; i < rebalance->children.size(); ++i) {
          L->children.emplace_back(std::move(rebalance->children[i]));
        }
        parent->removeKey(index_in_parent);
        parent->removeChild(index_in_parent);
        if (parent->keys.empty() && parents.empty()) {
          btnode::Free(parent);
          root = L;
        }
        elif (parent->keys.size() < CC/2) {
          rebalance = parent;
          continue;
        }
        break;
      }
      // FUTURE: can combine with the above with aliasing.
      if (R) {
        AssertCrash(R->keys.size() == CC/2);
        rebalance->keys.push_back(parent->keys[index_in_parent]);
        for (size_t i = 0; i < R->keys.size(); ++i) {
          rebalance->keys.emplace_back(std::move(R->keys[i]));
        }
        for (size_t i = 0; i < R->children.size(); ++i) {
          rebalance->children.emplace_back(std::move(R->children[i]));
        }
        parent->removeKey(index_in_parent);
        parent->removeChild(index_in_parent);
        if (parent->keys.empty() && parents.empty()) {
          btnode::Free(parent);
          root = rebalance;
        }
        elif (parent->keys.size() < CC/2) {
          rebalance = parent;
          continue;
        }
        break;
      }
    }
  }

  void Insert(int key, bool& already_there) {
    already_there = false;

    std::vector<parentelt> parents;
    auto t = root;
    size_t index;
    while (t) {
      bool found;
      t->Lookup(key, index, found);
      if (found) { already_there = 1; return; }

      if (!t->leaf()) {
        parents.push_back(parentelt{t, index});
        t = t->children[index];
        continue;
      }

      if (t->keys.size() < CC) {
        t->insertKey(index, key);
        return;
      }

      break;
    }

    // _Insert_WalkUpwards( t, key, parent_chain, node, idx );
    btnode* childL = 0;
    btnode* childR = 0;
    Forever {
      const auto leaf = t->leaf();
      auto R = btnode::Alloc();
      auto L = t;

      auto index_median = t->keys.size() / 2;
      int key_median;
      if (index == index_median) {
        key_median = key;

        for (size_t i = index_median; i < L->keys.size(); ++i) {
          R->keys.push_back(L->keys[i]);
        }
        L->keys.resize(index_median);

        if (!leaf) {
          R->children.push_back(childR);
          for (size_t i = index_median+1; i < L->children.size(); ++i) {
            R->children.push_back(L->children[i]);
          }

          L->children.resize(index_median+1);
          L->children[index_median] = childL;
        }
      }
      elif (index < index_median) {
        key_median = L->keys[index_median-1];

        for (size_t i = index_median; i < L->keys.size(); ++i) {
          R->keys.push_back(L->keys[i]);
        }
        L->keys.resize(index_median-1);
        L->insertKey(index, key);

        if (!leaf) {
          for (size_t i = index_median; i < L->children.size(); ++i) {
            R->children.push_back(L->children[i]);
          }
          L->children.resize(index_median);
          L->children[index] = childL;
          L->insertChild(index+1, childR);
        }
      }
      else { // index > index_median
        key_median = L->keys[index_median];

        for (size_t i = index_median+1; i < L->keys.size(); ++i) {
          R->keys.push_back(L->keys[i]);
        }
        R->insertKey(index - (index_median+1), key);
        L->keys.resize(index_median);

        if (!leaf) {
          for (size_t i = index_median+1; i < L->children.size(); ++i) {
            R->children.push_back(L->children[i]);
          }
          R->children.push_back(childL);
          R->children.push_back(childR);
          for (size_t i = index+1; i < L->children.size(); ++i) {
            R->children.push_back(L->children[i]);
          }
          L->children.resize(index_median+1);
        }
      }

      // pull the key_median up into the parent.

      if (parents.empty()) {
        // At the root, and it is full.
        AssertCrash(root->keys.size() == CC);
        auto rootN = btnode::Alloc();
        rootN->keys.push_back(key_median);
        rootN->children.push_back(L);
        rootN->children.push_back(R);
        root = rootN;
        return;
      }

      auto parent = parents.back().t;
      auto index_in_parent = parents.back().index;
      parents.pop_back();
      if (parent->keys.size() < CC) {
        parent->insertKey(index_in_parent, key_median);
        parent->insertChild(index_in_parent+1, R);
        parent->children[index_in_parent] = L;
        return;
      }

      key = key_median;
      t = parent;
      index = index_in_parent;
      childL = L;
      childR = R;
      continue;
    }
  }
};






RegisterTest([]()
{
  constexpr idx_t C = 10;
  btree_t<u32, C> t;
  Init( &t );
  stack_resizeable_cont_t<parent_info_t<u32, C>> infos;
  Alloc( infos, 1024 );
  stack_resizeable_cont_t<u32> flat;
  Alloc( flat, 1024 );
  u32 N = 100;
  Fori( u32, i, 0, N ) {
    auto value = i + 1;
    bool already_there = 0;
    Insert( &t, value, &already_there, &infos );
    AssertCrash( !already_there );
    flat.len = 0;
    FlattenValues( &t, &flat );
    AssertCrash( flat.len == i + 1 );
    ForLen( j, flat ) {
      auto lj = j + 1;
      auto fj = flat.mem[j];
      AssertCrash( lj == fj );
    }
  }
  Free( flat );
  Free( infos );
});












// attempt at algorithmic improvement to avoid the parent_chain.
// it's supposedly possible, but i'm not far enough along with the regular b-tree implementation to be able to finish this.

#if 0

Templ Inl void
Insert(
  BTREE* t,
  T key,
  bool* already_there
  )
{
  auto root = t->root;
  AssertCrash( root );
  auto C = t->C;
  auto parents_child_pointer = &t->root;
  //
  // pre-emptively split the root if it's full
  //
  {
    auto M = root->M;
    if( M == C ) {

      auto new_root = Cast( BTREENODE*, AddBackBytes( *t->mem, _SIZEOF_IDX_T, nbytes_node ) );
      Memzero( new_root, nbytes_node );
      auto new_root_children = Children<T>( new_root );
      auto new_root_keys = Keys<T>( new_root );

      auto new_right = Cast( BTREENODE*, AddBackBytes( *t->mem, _SIZEOF_IDX_T, nbytes_node ) );
      Memzero( new_right, nbytes_node );
      auto new_right_children = Children<T>( new_right );
      auto new_right_keys = Keys<T>( new_right );

      auto left = root;
      auto left_children = Children<T>( left );
      auto left_keys = Keys<T>( left );

      //
      // according to the ordering, child[i].keys[j] < keys[i] for all j.
      // so if i is our idx_median, we need to move child[ <= idx_median ], and move keys[ < idx_median ].
      //
      // well since we're leaving left in place, what are we moving on the right side?
      //   left keeps child[ <= idx_median ], as discussed.
      //   left keeps keys[ < idx_median ], as discussed.
      //   keys[ idx_median ] moves to the parent.
      //   right gets child[ > idx_median ], which is ( M + 1 - idx_median - 1 ) children.
      //   right gets keys[ > idx_median ], which is ( M - idx_median - 1 ) keys.
      //

      auto idx_median = M / 2;

      t->root = new_root;
      new_root->M = 2;
      new_root_children[0] = left;
      new_root_keys[0] = left_keys[ idx_median ];
      new_root_children[1] = new_right;

      auto len_right_children = M - idx_median;
      auto len_right_keys = len_right_children - 1;
      new_right->M = len_right_keys;
      TMove( new_right_children, left_children + idx_median + 1, len_right_children );
      TMove( new_right_keys, left_keys + idx_median + 1, len_right_keys );

      TZero( left_children + idx_median + 1, len_right_children );
      TZero( left_keys + idx_median + 1, len_right_keys );
      left->M = idx_median + 1;
    }
  }

  auto node = root;
  Forever {
    auto M = node->M;
    auto keys = Keys<T>( node );
    auto children = Children<T>( node );
    //
    // since we already split the root if needed above, we'll iteratively assume that this node has room for 1 more key.
    // but, note that we then have to check if the children have room, and then potentially pull the median up into this node.
    // since we're only allowing insert of one key at a time, this is sufficient.
    //
    AssertCrash( M < C );
    //
    // search for a key match in this node.
    // if we find one, great, we're done.
    // otherwise, we'll have to iterate into the specific, ordered child that should contain it.
    //
    // TODO: binary search for large C instead ?
    //
    u32 idx = 0;
    for( ; idx < M; ++idx ) {
      auto key_at_idx = keys[ idx ];
      if( key == key_at_idx ) {
        *already_there = 1;
        return;
      }
      if( key < key_at_idx ) {
        break;
      }
    }
    //
    // either we terminated the loop above early with idx < M,
    // or we reached the end with idx = M.
    //
    auto child_node = children[ idx ];
    if( !child_node  &&  M + 1 < C ) {
      //
      // there's room in this node to insert a new key!
      //
      // according to the ordering, child[i].keys[j] < keys[i] for all j.
      // so since our new key belongs at key[ idx ], we need to:
      //   make room by moving into keys[ > idx ], which is ( M - idx - 1 ) keys.
      //   maintain order by moving into children[ > idx ], which is ( M - idx ) children.
      //
      _InsertAtIdxAssumingRoom( node, keys, children, M, idx, key );
      *already_there = 0;
      return;
    }
    //
    // otherwise we've got a child_node, or there's not room.
    //
    if( !child_node  &&  M + 1 == C ) {
      //
      // haven't yet created a child_node in this slot, and we're out of room.
      //
      child_node = AllocateBtreeNode<T, C>();
      auto child_keys = Keys<T>( child_node );
      auto child_children = Children<T>( child_node );

      //
      // according to the ordering, child[i].keys[j] < keys[i] for all j.
      // so if i is our idx_median, we need to move child[ <= idx_median ], and move keys[ < idx_median ].
      //
      // split this node into two:
      //   leave keys[ < idx_median ] in this node, which is ( idx_median ) keys.
      //   leave children[ < idx_median ] in this node, which is ( idx_median ) children.
      //   move keys[ >= idx_median ] into the child_node, which is ( M - idx_median ) keys.
      //   move children[ >= idx_median ] into the child_node, which is ( M + 1 - idx_median ) children.
      // and then set
      //   children[ idx_median ] = child_node
      //
      // then we know idx belongs in either this node or the new child_node, depending on whether it's key is < or >= keys[ idx_median ].
      // so if key < keys[ idx_median ], insert into this node at keys[ idx ].
      // otherwise, key >= keys[ idx_median ], so insert into child_node at child_keys[ idx_median - idx ].
      //

      // TODO: better rebalancing ?
      // this won't redistribute amongst children, which would be beneficial
      // i think we could theoretically end up with an unbalanced tree here.
      // perhaps we need to count keys present in all child subtrees, and pick that median, instead of the node-local median?
      // well we don't really want to rebalance globally here; that's probably too expensive. this is an incremental insert, after all.
      // probably we can get away with incremental rebalancing, and with some invariant, we could maintain global balance?
      // maybe we only balance by peeking one level down?
      // nah, theoretically i think we need something like a 3-chain -> root-and-2-leaves tree rotation.
      //


      auto idx_median = M / 2;
      auto child_M = M - idx_median;
      TMove( child_children, children + idx_median, child_M + 1 );
      TMove( child_keys, keys + idx_median, child_M );
      child_node->M = child_M;
      children[ idx_median ] = child_node;
      M = idx_median;
      node->M = M;

      if( idx <= idx_median ) {
        _InsertAtIdxAssumingRoom( node, keys, children, M, idx, key );
        *already_there = 0;
        return;
      }

      _InsertAtIdxAssumingRoom( child_node, child_keys, child_children, child_M, idx_median - idx, key );
      *already_there = 0;
      return;
    }
    //
    // otherwise we've got a child_node to recurse into.
    //
    if( child_node ) {
      auto child_keys = Keys<T>( child_node );
      auto child_children = Children<T>( child_node );
      auto child_M = child_node->M;
      if( child_M == C ) {
        //
        // split child_node, the same way we split the root.
        //
        XXXXXX

        //
        // recurse into the child_node.
        //
        node = child_node;
        continue;
      }
      return;
    }

    // TODO: reorder these ifs above?

    UnreachableCrash();
  }
}

#endif





// TODO: B+ tree

struct
bptree_node_t
{

};










