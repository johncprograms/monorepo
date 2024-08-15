// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// min-max-heap, that allows this sequence of ops:
// 1. InitMinmaxHeapInPlace
// 2. any number of calls to these functions in any order:
//    - MinHeapExtract
//    - MinHeapInsert
//
// the binary tree layout we're using is:
//    L0      0
//    L1    1   2
//    L2   3 4 5 6
//    L3  ...
// which in sequential memory looks like:
//    |0|1 2|3 4 5 6|...
//
//               lz  64-lz
//     0  00000  64    0
//     1  00001  63    1
//     2  00010  62    2
//     3  00011  62    2
//     4  00100  61    3
//     5  00101  61    3
//     6  00110  61    3
//     7  00111  61    3
//     8  01000  60    4
//     9  01001  60    4
//    10  01010  60    4
//    11  01011  60    4
//    12  01100  60    4
//    13  01101  60    4
//    14  01110  60    4
//    15  01111  60    4
// so if nodes and levels were 1-based, we'd have
//    level(i) = 64 - countl_zero(i)
// but since both are 0-based, we have
//    level(i) = 63 - countl_zero(i+1)
//
// this means our mapping functions are:
//    left-child(i) = 2i + 1
//    rght-child(i) = 2i + 2
//    parent(i) = (i - 1) / 2
//    level(i) = 63 - countl_zero(i+1)
//
// even levels are min levels
// odd levels are max levels
// invariants:
//   node i on a min-level is <= all its children
//   node i on a max-level is >= all its children
//
// note this is a value-based data array, meaning we're not using auxilliary indexing.
// so, you should probably only use this for small data types, like integers.
//
// you should only use this for short to medium size heaps, since it's fundamentally an stack_resizeable_cont_t<Data>.
// huge heaps should use a smarter paging approach, like a B-heap, veb-tree, etc.
//

ForceInl idx_t
_Level( idx_t i )
{
  return _SIZEOF_IDX_T - 1 - _lzcnt_idx_t( i + 1 );
}
ForceInl idx_t
_Parent( idx_t i )
{
  return ( i - 1 ) / 2;
}

template< typename Data >
Inl void
_MinmaxHeapifyDown( Data* data, idx_t len, idx_t idx )
{
  idx_t i;
  Forever {
    if( idx > len / 2 ) break; // idx has no children
    i = idx;
    auto left = 2 * i + 1;
    auto rght = 2 * i + 2;
    auto left_left = 2 * left + 1;
    auto left_rght = 2 * left + 2;
    auto rght_left = 2 * rght + 1;
    auto rght_rght = 2 * rght + 2;
    if( _Level( i ) & 1 ) { // odd, so max-level
      auto largest = i;
      if( left < len && data[ left ] > data[ largest ] ) {
        largest = left;
      }
      if( rght < len && data[ rght ] > data[ largest ] ) {
        largest = rght;
      }
      if( left_left < len && data[ left_left ] > data[ largest ] ) {
        largest = left_left;
      }
      if( left_rght < len && data[ left_rght ] > data[ largest ] ) {
        largest = left_rght;
      }
      if( rght_left < len && data[ rght_left ] > data[ largest ] ) {
        largest = rght_left;
      }
      if( rght_rght < len && data[ rght_rght ] > data[ largest ] ) {
        largest = rght_rght;
      }
      if( largest != i ) {
        idx = largest;
        SWAP( Data, data[ idx ], data[ i ] );
        if( idx <= rght ) return; // largest was not a grandchild.
        auto parent_idx = _Parent( idx );
        if( data[ idx ] < data[ parent_idx ] ) {
          SWAP( Data, data[ idx ], data[ parent_idx ] );
        }
        continue;
      }
    }
    else { // even, so min-level
      auto smallest = i;
      if( left < len && data[ left ] < data[ smallest ] ) {
        smallest = left;
      }
      if( rght < len && data[ rght ] < data[ smallest ] ) {
        smallest = rght;
      }
      if( left_left < len && data[ left_left ] < data[ smallest ] ) {
        smallest = left_left;
      }
      if( left_rght < len && data[ left_rght ] < data[ smallest ] ) {
        smallest = left_rght;
      }
      if( rght_left < len && data[ rght_left ] < data[ smallest ] ) {
        smallest = rght_left;
      }
      if( rght_rght < len && data[ rght_rght ] < data[ smallest ] ) {
        smallest = rght_rght;
      }
      if( smallest != i ) {
        idx = smallest;
        SWAP( Data, data[ idx ], data[ i ] );
        if( idx <= rght ) return; // smallest was not a grandchild.
        auto parent_idx = _Parent( idx );
        if( data[ idx ] > data[ parent_idx ] ) {
          SWAP( Data, data[ idx ], data[ parent_idx ] );
        }
        continue;
      }
    }
    return;
  }
}

// since a min-max heap is just a slice, this swaps elements around in the slice to form a min-heap.
template< typename Data >
Inl void
InitMinmaxHeapInPlace( Data* data, idx_t len )
{
  ProfFunc();
  auto one_past_last_idx = len / 2;
  ReverseFori( idx_t, i, 0, one_past_last_idx ) {
    _MinmaxHeapifyDown( data, len, i );
  }
}

template< typename Data >
Inl void
_MinmaxHeapifyUpMin( Data* data, idx_t len, idx_t i )
{
  AssertCrash( i < len );
  AssertCrash( !( _Level( i ) & 1 ) ); // should be given a min-level index
  while( i >= 3 ) { // nodes >= 3 have grandparents, by the structure of the tree. We know a priori the first two levels don't have them.
    auto parent_i = _Parent( i );
    auto grandparent_i = _Parent( parent_i );
    if( data[i] >= data[ grandparent_i ] ) break;
    SWAP( Data, data[i], data[ grandparent_i ] );
    i = grandparent_i;
  }
}

template< typename Data >
Inl void
_MinmaxHeapifyUpMax( Data* data, idx_t len, idx_t i )
{
  AssertCrash( i < len );
  AssertCrash( _Level( i ) & 1 ); // should be given a max-level index
  while( i >= 3 ) { // nodes >= 3 have grandparents, by the structure of the tree. We know a priori the first two levels don't have them.
    auto parent_i = _Parent( i );
    auto grandparent_i = _Parent( parent_i );
    if( data[i] <= data[ grandparent_i ] ) break;
    SWAP( Data, data[i], data[ grandparent_i ] );
    i = grandparent_i;
  }
}

template< typename Data >
Inl void
_MinmaxHeapifyUp( Data* data, idx_t len, idx_t i )
{
  if( !i ) return;
  // i is not the root
  auto parent_i = _Parent( i );
  if( _Level( i ) & 1 ) { // max-level
    if( data[i] < data[parent_i] ) {
      SWAP( Data, data[i], data[parent_i] );
      _MinmaxHeapifyUpMin( data, len, parent_i );
    }
    else {
      _MinmaxHeapifyUpMax( data, len, i );
    }
  }
  else { // min-level
    if( data[i] > data[parent_i] ) {
      SWAP( Data, data[i], data[parent_i] );
      _MinmaxHeapifyUpMax( data, len, parent_i );
    }
    else {
      _MinmaxHeapifyUpMin( data, len, i );
    }
  }
}

template< typename Data >
Inl void
MinmaxHeapExtractMin( stack_resizeable_cont_t<Data>* array, Data* dst )
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
  _MinmaxHeapifyDown( data, len, 0 );
}

template< typename Data >
Inl void
MinmaxHeapExtractMax( stack_resizeable_cont_t<Data>* array, Data* dst )
{
  ProfFunc();
  auto data = array->mem;
  auto len = array->len;
  AssertCrash( len );

  auto largest = 0;
  auto left = 1;
  auto rght = 2;
  if( left < len  &&  data[ left ] > data[ largest ] ) {
    largest = left;
  }
  if( rght < len  &&  data[ rght ] > data[ largest ] ) {
    largest = rght;
  }
  *dst = data[largest];
  len -= 1;
  array->len = len;
  if( !len ) return;
  data[largest] = data[len];
  _MinmaxHeapifyDown( data, len, largest );
}

template< typename Data >
Inl void
MinmaxHeapInsert( stack_resizeable_cont_t<Data>* array, Data* src )
{
  ProfFunc();
  auto idx_insert = array->len;
  *AddBack( *array ) = *src;
  _MinmaxHeapifyUp( array->mem, array->len, idx_insert );
}

template< typename Data >
Inl void
MinmaxHeapVerify( Data* data, idx_t len )
{
  auto one_past_last_idx = len / 2;
  For( idx, 0, one_past_last_idx ) {
    auto left = 2 * idx + 1;
    auto rght = 2 * idx + 2;
    if( _Level( idx ) & 1 ) { // max-level
      if( left < len ) {
        AssertCrash( data[idx] >= data[left] );
      }
      if( rght < len ) {
        AssertCrash( data[idx] >= data[rght] );
      }
    }
    else { // min-level
      if( left < len ) {
        AssertCrash( data[idx] <= data[left] );
      }
      if( rght < len ) {
        AssertCrash( data[idx] <= data[rght] );
      }
    }
  }
}



RegisterTest([]()
{
  rng_lcg_t lcg;
  Init( lcg, 0x123456789 );

  idx_t N = 1000;
  For( len, 1, N ) {
    stack_resizeable_cont_t<u32> heap_min;
    stack_resizeable_cont_t<u32> heap_max;
    Alloc( heap_min, len );
    Alloc( heap_max, len );
    heap_min.len = len;
    heap_max.len = len;
    ForLen( i, heap_min ) {
      heap_min.mem[i] = Rand32( lcg );
      heap_max.mem[i] = Rand32( lcg );
    }

    auto sorted_min = AllocString<u32>( len );
    auto sorted_max = AllocString<u32>( len );

    InitMinmaxHeapInPlace( ML( heap_min ) );
    InitMinmaxHeapInPlace( ML( heap_max ) );
    MinmaxHeapVerify( ML( heap_min ) );
    MinmaxHeapVerify( ML( heap_max ) );

    For( i, 0, len ) {
      MinmaxHeapExtractMin( &heap_min, sorted_min.mem + i );
      MinmaxHeapExtractMax( &heap_max, sorted_max.mem + i );
      MinmaxHeapVerify( ML( heap_min ) );
      MinmaxHeapVerify( ML( heap_max ) );
    }
    AssertCrash( !heap_min.len );
    AssertCrash( !heap_max.len );

    // after doing min-extracts, we should have a sorted_min list result.
    For( i, 0, len - 1 ) {
      auto min_i = sorted_min.mem[i];
      auto min_j = sorted_min.mem[ i + 1 ];
      AssertCrash( min_i <= min_j );

      auto max_i = sorted_max.mem[i];
      auto max_j = sorted_max.mem[ i + 1 ];
      AssertCrash( max_i >= max_j );
    }

    Free( sorted_min );
    Free( sorted_max );
    Free( heap_min );
    Free( heap_max );
  }

  {
    stack_resizeable_cont_t<u32> data;
    Alloc( data, 10000 );
    For( c, 0, 10000 ) {
      auto z = Zeta32( lcg );
      if( data.len  &&  z < 0.2 ) {
        u32 min;
        MinmaxHeapExtractMin( &data, &min );
      }
      elif( data.len  &&  z < 0.4 ) {
        u32 max;
        MinmaxHeapExtractMax( &data, &max );
      }
      else {
        auto val = Rand32( lcg );
        MinmaxHeapInsert( &data, &val );
      }
      MinmaxHeapVerify( ML( data ) );
    }
    Free( data );
  }
});
