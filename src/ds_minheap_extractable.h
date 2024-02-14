// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// simple min-heap, that only allows this sequence of ops:
// 1. InitMinHeapInPlace
// 2. any number of MinHeapExtract calls.
//
// this is externally indexed, meaning the original data array is untouched.
// Index space is the heap order space. This is where we do all the 'element swaps' to maintain the heap
// invariant. Although notice we're not actually swapping data array elements, it's virtual swaps.
// This has the benefit of leaving the original data alone, which is useful for serializeability.
// Instead of copying the original data so we can heap order the copy, we have one index array.
// So this is a net memory size win when: sizeof(Index) < sizeof(Data) and data must be read-only.
//
// if you need DecreaseKey support, use ds_minheap_decreaseable.h
// it has more complex index array requirements in order to support DecreaseKey.
//
// if you need to add things to the min-heap, don't use this, go use some other structure!
// we'd have to be smarter about reserving space than this simple slice can support.
//
// the binary tree layout we're using is:
//       0
//     1   2
//    3 4 5 6
//   ...
// which in sequential memory looks like:
//    0 1 2 3 4 5 6 ...
// this means our mapping functions are:
//    left-child(i) = 2i + 1
//    rght-child(i) = 2i + 2
//    parent(i) = (i - 1) / 2
//

template< typename Index, typename Data >
ForceInl void
_MinHeapifyDown( Index* idxs, Data* data, idx_t len, Index int_idx )
{
  Forever {
    // swap with the smaller of our two children to maintain the heap invariant.
    // then recurse if necessary.
    auto int_left = 2 * int_idx + 1;
    auto int_rght = 2 * int_idx + 2;
    auto int_smallest = int_idx;
    if( int_left < len ) {
      auto ext_left = idxs[int_left];
      auto ext_smallest = idxs[int_smallest];
      if( data[ ext_left ] < data[ ext_smallest ] ) {
        int_smallest = int_left;
      }
    }
    if( int_rght < len ) {
      auto ext_rght = idxs[int_rght];
      auto ext_smallest = idxs[int_smallest];
      if( data[ ext_rght ] < data[ ext_smallest ] ) {
        int_smallest = int_rght;
      }
    }
    if( int_smallest != int_idx ) {
      SWAP( Index, idxs[ int_idx ], idxs[ int_smallest ] );
      int_idx = int_smallest;
      continue;
    }
    return;
  }
}

// since minheap_t is just a slice, this swaps elements around in the slice to form a min-heap.
template< typename Index, typename Data >
Inl void
InitMinHeapInPlace( Index* idxs, Data* data, idx_t len )
{
  auto one_past_last_idx = Cast( Index, len / 2 );
  ReverseFori( Index, i, 0, one_past_last_idx ) {
    _MinHeapifyDown( idxs, data, len, i );
  }
}

// WARNING: modifies the 'len' field, so consider if you should copy yourself!
template< typename Index, typename Data >
Inl void
MinHeapExtract( Index* idxs, Data* data, idx_t* len_, Index* dst )
{
  auto len = *len_;
  AssertCrash( len );

  *dst = idxs[0];
  len -= 1;
  *len_ = len;
  if( !len ) return;

  idxs[0] = idxs[ len ];
  _MinHeapifyDown( idxs, data, len, Cast( Index, 0 ) );
}

RegisterTest([]()
{
  rng_lcg_t lcg;
  Init( lcg, 0x123456789 );

  idx_t N = 1000;
  For( len, 1, N ) {
    auto data = AllocString<u32>( len );
    auto idxs = AllocString<u32>( len );
    auto sort = AllocString<u32>( len );

    ForLen( i, data ) {
      data.mem[i] = Rand32( lcg );
      idxs.mem[i] = Cast( u32, i );
    }

    InitMinHeapInPlace( idxs.mem, ML( data ) );

    auto len_while_removing = data.len;
    For( i, 0, len ) {
      MinHeapExtract( idxs.mem, data.mem, &len_while_removing, sort.mem + i );
    }
    AssertCrash( !len_while_removing );

    // after doing min-heap extracts, we should have a sorted list result.
    For( i, 0, len - 1 ) {
      auto data_i = data.mem[ sort.mem[i] ];
      auto data_n = data.mem[ sort.mem[ i + 1 ] ];
      AssertCrash( data_i <= data_n );
    }

    Free( data );
    Free( idxs );
    Free( sort );
  }
});