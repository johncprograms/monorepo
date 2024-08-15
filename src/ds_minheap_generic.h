// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// min-heap, that allows this sequence of ops:
// 1. InitMinHeapInPlace
// 2. any number of calls to these functions in any order:
//    - MinHeapExtract
//    - MinHeapInsert
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
// note this is a value-based data array, meaning we're not using auxilliary indexing.
// so, you should probably only use this for small data types, like integers.
//
// you should only use this for short to medium size heaps, since it's fundamentally an stack_resizeable_cont_t<Data>.
// huge heaps should use a smarter paging approach, like a B-heap, veb-tree, etc.
//

template< typename Data >
Inl void
_MinHeapifyDown( Data* data, idx_t len, idx_t idx )
{
  Forever {
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
    if( smallest != idx ) {
      SWAP( Data, data[ idx ], data[ smallest ] );
      idx = smallest;
      continue;
    }
    return;
  }
}

template< typename Data >
Inl void
_MinHeapifyUp(
  Data* data,
  idx_t len,
  idx_t index
  )
{
  Forever {
    if( !index ) return;
    auto index_parent = ( index - 1 ) / 2;
    if( data[ index ] < data[ index_parent ] ) {
      SWAP( Data, data[ index ], data[ index_parent ] );
      index = index_parent;
      continue;
    }
    return;
  }
}

// since minheap_t is just a slice, this swaps elements around in the slice to form a min-heap.
template< typename Data >
Inl void
InitMinHeapInPlace( Data* data, idx_t len )
{
  ProfFunc();
  auto one_past_last_idx = len / 2;
  ReverseFori( idx_t, i, 0, one_past_last_idx ) {
    _MinHeapifyDown( data, len, i );
  }
}

template< typename Data >
Inl void
MinHeapExtract( stack_resizeable_cont_t<Data>* array, Data* dst )
{
  ProfFunc();
  auto data = array->mem;
  auto len = array->len;
  AssertCrash( len );

  *dst = data[0];
  len -= 1;
  array->len = len;
  if( !len ) return;
  data[0] = data[len];
  _MinHeapifyDown( data, len, 0 );
}

template< typename Data >
Inl void
MinHeapInsert( stack_resizeable_cont_t<Data>* array, Data* src )
{
  ProfFunc();
  auto idx_insert = array->len;
  *AddBack( *array ) = *src;
  _MinHeapifyUp( array->mem, array->len, idx_insert );
}

template< typename Data >
Inl void
MinHeapVerify( Data* data, idx_t len )
{
  auto one_past_last_idx = len / 2;
  For( idx, 0, one_past_last_idx ) {
    auto left = 2 * idx + 1;
    auto rght = 2 * idx + 2;
    if( left < len ) {
      AssertCrash( data[idx] < data[left] );
    }
    if( rght < len ) {
      AssertCrash( data[idx] < data[rght] );
    }
  }
}



RegisterTest([]()
{
  rng_lcg_t lcg;
  Init( lcg, 0x123456789 );

  idx_t N = 1000;
  For( len, 1, N ) {
    stack_resizeable_cont_t<u32> data;
    Alloc( data, len );
    data.len = len;
    ForLen( i, data ) {
      data.mem[i] = Rand32( lcg );
    }

    auto sorted = AllocString<u32>( len );

    InitMinHeapInPlace( ML( data ) );
    MinHeapVerify( ML( data ) );

    For( i, 0, len ) {
      MinHeapExtract( &data, sorted.mem + i );
      MinHeapVerify( ML( data ) );
    }
    AssertCrash( !data.len );

    // after doing min-heap extracts, we should have a sorted list result.
    For( i, 0, len - 1 ) {
      auto data_i = sorted.mem[i];
      auto data_n = sorted.mem[ i + 1 ];
      AssertCrash( data_i <= data_n );
    }

    Free( sorted );
    Free( data );
  }

  {
    stack_resizeable_cont_t<u32> data;
    Alloc( data, 10000 );
    For( c, 0, 10000 ) {
      if( data.len  &&  Zeta32( lcg ) < 0.34 ) {
        u32 min;
        MinHeapExtract( &data, &min );
      }
      else {
        auto val = Rand32( lcg );
        MinHeapInsert( &data, &val );
      }
      MinHeapVerify( ML( data ) );
    }
    Free( data );
  }
});
