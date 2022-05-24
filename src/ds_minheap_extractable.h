// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// simple min-heap, that only allows this sequence of ops:
// 1. InitMinHeapInPlace
// 2. any number of MinHeapExtract calls.
//
// if you need DecreaseKey support, use ds_minheap_decreaseable.h
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
// note we're using an auxilliary data array, which we don't ever modify here.
// we only manipulate the index array, which is presumably smaller, and avoids
// all the operator overloading nonsense.
//
// one complication is that since we're not swapping the data array elements around,
// we'd have to maintain another index mapping, to go from external index to offset into idxs.
//
// TODO: minheap that allows arbitrary insertions
//

// TODO: iterative version. tail recursion, so should be easy.
template< typename Index, typename Data >
Inl void
_MinHeapifyDownRecursive( Index* idxs, Data* data, idx_t len, Index idx )
{
  // swap with the smaller of our two children to maintain the heap invariant.
  // then recurse if necessary.
  auto left = 2 * idx + 1;
  auto rght = 2 * idx + 2;
  auto smallest = idx;
  if( left < len && data[ left ] < data[ smallest ] ) {
    smallest = left;
  }
  if( rght < len && data[ rght ] < data[ smallest ] ) {
    smallest = rght;
  }
  // TODO: could eliminate this conditional with inlining above.
  if( smallest != idx ) {
    SWAP( Index, idxs[ idx ], idxs[ smallest ] );
    _MinHeapifyDownRecursive( idxs, data, len, smallest );
  }
}

// since minheap_t is just a slice, this swaps elements around in the slice to form a min-heap.
template< typename Index, typename Data >
Inl void
InitMinHeapInPlace( Index* idxs, Data* data, idx_t len )
{
  auto one_past_last_idx = len / 2;
  ReverseFori( Index, i, 0, one_past_last_idx ) {
    _MinHeapifyDownRecursive( idxs, data, len, i );
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
  _MinHeapifyDownRecursive( idxs, data, len, 0 );
}
