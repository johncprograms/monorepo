// Copyright (c) John A. Carlos Jr., all rights reserved.
//
// min-heap, that only allows this sequence of ops:
// 1. InitMinHeapInPlace
// 2. any number of MinHeapExtract or MinHeapDecreasedKey calls
//
// if you DON'T need DecreaseKey support, use ds_minheap_extractable.h
// it's a simpler tslice_t without as much index mapping nonsense, which is nice.
//
// if you need to add things to the min-heap, don't use this, go use some other structure!
// we'd have to be smarter about reserving space than this simple slice can support.
//

// TODO: iterative version. tail recursion, so should be easy.
template< typename ExtIndex, typename IntIndex, typename Data >
Inl void
_MinHeapifyDownRecursive(
  ExtIndex* external_from_internal,
  IntIndex* internal_from_external,
  Data* data,
  idx_t len,
  IntIndex int_index
  )
{
  // swap with the smaller of our two children to maintain the heap invariant.
  // then recurse if necessary.
  auto int_left = 2 * int_index + 1;
  auto int_rght = 2 * int_index + 2;
  auto int_smallest = int_index;
  if( int_left < len ) {
    auto ext_left = external_from_internal[ int_left ];
    auto ext_smallest = external_from_internal[ int_smallest ];
    if( data[ ext_left ] < data[ ext_smallest ] ) {
      int_smallest = int_left;
    }
  }
  if( int_rght < len ) {
    auto ext_rght = external_from_internal[ int_rght ];
    auto ext_smallest = external_from_internal[ int_smallest ];
    if( data[ ext_rght ] < data[ ext_smallest ] ) {
      int_smallest = int_rght;
    }
  }
  // TODO: could eliminate this conditional with inlining above.
  if( int_smallest != int_index ) {
    auto ext_index = external_from_internal[ int_index ];
    auto ext_smallest = external_from_internal[ int_smallest ];
    SWAP( IntIndex, internal_from_external[ ext_index ], internal_from_external[ ext_smallest ] );
    SWAP( ExtIndex, external_from_internal[ int_index ], external_from_internal[ int_smallest ] );
    _MinHeapifyDownRecursive( external_from_internal, internal_from_external, data, len, int_smallest );
  }
}

// TODO: iterative version. tail recursion, so should be easy.
template< typename ExtIndex, typename IntIndex, typename Data >
Inl void
_MinHeapifyUpRecursive(
  ExtIndex* external_from_internal,
  IntIndex* internal_from_external,
  Data* data,
  idx_t len,
  IntIndex int_index
  )
{
  if( !int_index ) return;
  auto ext_index = external_from_internal[ int_index ];
  auto int_parent = ( int_index - 1 ) / 2;
  auto ext_parent = external_from_internal[ int_parent ];
  if( data[ ext_index ] < data[ ext_parent ] ) {
    SWAP( IntIndex, internal_from_external[ ext_index ], internal_from_external[ ext_parent ] );
    SWAP( ExtIndex, external_from_internal[ int_index ], external_from_internal[ int_parent ] );
    _MinHeapifyUpRecursive( external_from_internal, internal_from_external, data, len, int_parent );
  }
}

// NOTE: assumes you've already decreased the value of data[idx].
// you're expected to call this after doing so to maintain the heap invariant.
template< typename ExtIndex, typename IntIndex, typename Data >
Inl void
MinHeapDecreasedKey(
  ExtIndex* external_from_internal,
  IntIndex* internal_from_external,
  Data* data,
  idx_t len,
  ExtIndex ext_index
  )
{
#if 0
  auto int_index = internal_from_external[ ext_index ];
  auto int_top = 0;
  auto ext_top = external_from_internal[ int_top ];
  if( !int_index ) return;
  SWAP( ExtIndex, external_from_internal[ int_top ], external_from_internal[ int_index ] );
  SWAP( IntIndex, internal_from_external[ ext_top ], internal_from_external[ ext_index ] );
  _MinHeapifyDownRecursive( external_from_internal, internal_from_external, data, len, Cast( IntIndex, 0 ) );
#else
  // this is the bane of our existence, here: mapping from ext_index to int_index.
  // this is the only real spot where we need this, to support decrease-key.
  auto int_index = internal_from_external[ ext_index ];
  _MinHeapifyUpRecursive( external_from_internal, internal_from_external, data, len, int_index );
#endif
}

// since minheap_t is just a slice, this swaps elements around in the slice to form a min-heap.
template< typename ExtIndex, typename IntIndex, typename Data >
Inl void
InitMinHeapInPlace(
  ExtIndex* external_from_internal,
  IntIndex* internal_from_external,
  Data* data,
  idx_t len
  )
{
  auto one_past_last_idx = Cast( IntIndex, len / 2 );
  ReverseFori( IntIndex, i, 0, one_past_last_idx ) {
    _MinHeapifyDownRecursive( external_from_internal, internal_from_external, data, len, i );
  }
}

// WARNING: modifies the 'len' field, so consider if you should copy yourself!
template< typename ExtIndex, typename IntIndex, typename Data >
Inl void
MinHeapExtract(
  ExtIndex* external_from_internal,
  IntIndex* internal_from_external,
  Data* data,
  idx_t* len_,
  ExtIndex* ext_index_
  )
{
  auto len = *len_;
  AssertCrash( len );

  auto ext_index = external_from_internal[0];
  *ext_index_ = ext_index;
  len -= 1;
  *len_ = len;
  if( !len ) return;

  auto ext_last = external_from_internal[ len ];
  external_from_internal[0] = ext_last;
  internal_from_external[ext_last] = 0;
  _MinHeapifyDownRecursive( external_from_internal, internal_from_external, data, len, Cast( IntIndex, 0 ) );
}

Inl void
TestMinHeap()
{
  rng_lcg_t lcg;
  Init( lcg, 0x123456789 );

  idx_t N = 128;
  For( len, 1, N ) {
    tslice_t<u32> data;
    data.mem = MemHeapAlloc( u32, len );
    data.len = len;
    tslice_t<u32> ext_from_int;
    ext_from_int.mem = MemHeapAlloc( u32, len );
    ext_from_int.len = len;
    tslice_t<u32> int_from_ext;
    int_from_ext.mem = MemHeapAlloc( u32, len );
    int_from_ext.len = len;
    tslice_t<u32> sorted;
    sorted.mem = MemHeapAlloc( u32, len );
    sorted.len = len;

    ForLen( i, data ) {
      data.mem[i] = Rand32( lcg );
      int_from_ext.mem[i] = Cast( u32, i );
      ext_from_int.mem[i] = Cast( u32, i );
    }

    InitMinHeapInPlace( ext_from_int.mem, int_from_ext.mem, ML( data ) );

    For( i, 0, len ) {
      MinHeapExtract( ext_from_int.mem, int_from_ext.mem, data.mem, &data.len, sorted.mem + i );
    }
    AssertCrash( !data.len );

    // after doing min-heap extracts, we should have a sorted list result.
    For( i, 0, len - 1 ) {
      auto data_i = data.mem[ sorted.mem[i] ];
      auto data_n = data.mem[ sorted.mem[ i + 1 ] ];
      AssertCrash( data_i <= data_n );
    }

    MemHeapFree( data.mem );
    MemHeapFree( sorted.mem );
    MemHeapFree( int_from_ext.mem );
    MemHeapFree( ext_from_int.mem );
  }
}