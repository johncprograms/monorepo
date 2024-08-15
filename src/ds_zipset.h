// Copyright (c) John A. Carlos Jr., all rights reserved.

#define ZIPSET   zipset_t<T, Allocator, Allocation>

// TODO: implement the fractional cascading version.

//
// set/multiset-like datastructure that holds items with a total ordering.
// the basic idea is to have power-of-2 arrays for all powers-of-2 up to some max.
// and each of those arrays is either totally full or empty.
//
// invariant:
// ( N & bit_idx ) => rows[ bit_idx ] is totally full of inserted values.
// meaning, as we insert and increment N, we have to shuffle elements according to the carry chain.
//
// total capacity = sum( i=1..num_rows, 2^(i-1) ), which is an integer with all one bits.
//   i.e. for num_rows == 32, that's MAX_u32.
//
// e.g. say N=15
// [ h i j k l m n o ]
// [ d e f g ]
// [ b c ]
// [ a ]
//
TEA struct
zipset_t
{
  constant idx_t c_num_rows = 32;

  Allocation allocns[c_num_rows];
  T* rows[c_num_rows];
  idx_t N;
  Allocator alloc;
};
TEA Inl void
Init( ZIPSET& s, Allocator alloc = {} )
{
  Arrayzero( s.rows );
  s.N = 0;
  s.alloc = alloc;
}
ForceInl idx_t 
NumRows( idx_t N )
{
  return 8u * sizeof( idx_t ) - _lzcnt_idx_t( N );
}
TEA Inl void
Kill(
  ZIPSET& s
  )
{
  auto N = s.N;
  auto rows = s.rows;
  auto num_rows = NumRows( N );
  For( i, 0, num_rows ) {
    auto row = rows[ i ];
    auto row_len = 1u << i;
    if( !( N & row_len ) ) {
      if( row ) {
        Free( s.alloc, s.allocns[i], row );
        rows[ i ] = 0;
      }
    }
  }
  s.N = 0;
  s.alloc = {};
}
// TODO: InsertIfUnique
//   we could use the same algorithm as InsertAllowDuplicates and just throw away the carry_row if we
//   find a duplicate.
//   or, we could do a lookup and only insert if not found.
TEA Inl void
InsertAllowDuplicates(
  ZIPSET& s,
  T value
  )
{
  auto N = s.N;
  auto rows = s.rows;

  AssertCrash( N <= MAX_u32 );
  
  // the number of rows, starting with the len=1 one, that we have to merge upwards.
  auto num_rows_to_merge = _tzcnt_idx_t( ~N );
  
  auto dst_row_len = 1u << num_rows_to_merge; // length of the row we're merging into.
  AssertCrash( !( N & dst_row_len ) ); // the row we're merging into should be unpopulated.

  if( !rows[ num_rows_to_merge ] ) {
    rows[ num_rows_to_merge ] = Allocate<T>( s.alloc, s.allocns[ num_rows_to_merge ], dst_row_len );
  }
  auto dst_row = rows[ num_rows_to_merge ];
  
  if( !num_rows_to_merge ) { // writing to empty row 0.
    *dst_row = value;
  }
  else {
    //
    // write the new value to the start + 1 of the empty dst_row.
    // why the initial offset of +1? see below.
    //
    // then we'll zipper-merge existing lower-level rows with the last merge-result in the dst_row.
    //
    // e.g. say N=15
    // [ h i j k l m n o ]
    // [ d e f g ]
    // [ b c ]
    // [ a ]
    // and we insert p
    //
    // each subsequent carry zipper-merge will have dst_row:
    // zipper 0: [ _ p _ _ _ _ _ _ _ _ _ _ _ _ _ _ ] len = 1
    // zipper 1: [ _ p a p _ _ _ _ _ _ _ _ _ _ _ _ ] len = 1 + 2
    // zipper 2: [ _ p a p a b c p _ _ _ _ _ _ _ _ ] len = 1 + 2 + 4
    // zipper 3: [ _ p a p a b c p a b c d e f g p ] len = 1 + 2 + 4 + 8
    // zipper 4: [ a b c d e f g h i j k l m n o p ] dst_row is now full and valid!
    // where the _ indicates empty space.
    //
    // note that we initially skip the first slot so that zipper 3 writes its results into
    // the second half of dst_row precisely.
    //
    // that's important so that the last carry zipper-merge can write to the start of dst_row,
    // as we read from the second half of dst_row.
    // there's no danger of read overtaking write here, as long as the dst_row is precisely twice
    // the size of the row we're zipper-merging with.
    // this is the core idea to help avoid having to allocate a temporary buffer.
    //
    auto dst_write = dst_row + 1;
    *dst_write++ = value;
    auto dst_read = dst_row + 1;
  
    For( i, 0, num_rows_to_merge ) {
      // note the final zipper-merge writes to the start of dst_row, see above.
      if( i + 1 == num_rows_to_merge ) {
        dst_write = dst_row;
      }
  
      auto row_i = rows[ i ];
      auto row_i_len = 1u << i;
      AssertCrash( N & row_i_len );
  
      //
      // standard zipper-merge: ( dst_read, row_i_read ) -> ( dst_write )
      //
      auto dst_read_before_merge = dst_read;
      auto row_i_read = row_i;
      auto merge_len = 2 * row_i_len;
      For( merge_idx, 0, merge_len ) {
        if( *dst_read < *row_i_read ) {
          *dst_write++ = *dst_read++;
        }
        else {
          *dst_write++ = *row_i_read++;
        }
      }
      AssertCrash( row_i_read == row_i + row_i_len );
      AssertCrash( dst_read == dst_read_before_merge + row_i_len );
    }
  }

  // note that integer arithmetic does the appropriate bit carries trivially, which is nice.
  s.N = N + 1;
}
TEA Inl void
Lookup(
  ZIPSET& s,
  T value,
  bool* found,
  idx_t* row_idx,
  idx_t* idx_in_row
  )
{
  auto N = s.N;
  auto rows = s.rows;

  auto num_rows = NumRows( N );
  For( i, 0, num_rows ) {
    auto row_len = 1u << i;
    if( !( N & row_len ) ) continue;
    auto row = rows[ i ];
    idx_t sorted_insert_idx;
    BinarySearch( row, row_len, value, &sorted_insert_idx );
    if( sorted_insert_idx < row_len  &&  row[ sorted_insert_idx ] == value ) {
      *found = 1;
      *row_idx = i;
      *idx_in_row = sorted_insert_idx;
      return;
    }
  }

  *found = 0;
}
TEA Inl void
Delete(
  ZIPSET& s,
  T value,
  bool* found
  )
{
  auto N = s.N;
  auto rows = s.rows;

  bool found_lookup;
  idx_t row_idx;
  idx_t idx_in_row;
  Lookup( s, value, &found_lookup, &row_idx, &idx_in_row );
  if( !found_lookup ) {
    *found = 0;
    return;
  }

  // TODO: implement. how?
  //   i think it's similar: we have to follow the borrow chain.
  ImplementCrash();

  // TODO: arbitrary element deletion has to carry pretty much every time for base2.
  //   because we're much more likely to find arbitrary elements in the highest rows,
  //   it's very likely we'll have to follow the borrow chain down from the top.
  //   since the top row has N / 2 elements, half the time we'll have to move that half down
  //   to the rest of the smaller rows.
  //   so deletion is very very slow in comparison to insert!
  //   if there's some way we could relax the invariant, and allow half-filled rows, that'd be nice.

  *found = 0;
}

// TODO: zipset_t but for some other base, e.g. base16 instead of base2.
//   that would allow for some leeway to avoid borrowing work during deletion.
//   it would also allow for less-frequent carrying during insertion.
//   in baseK we have to do carrying at least every K sequential inserts.

#if 0

struct
zip10set_t
{
  static constexpr idx_t c_num_rows = 9;

  // rows[i].len is the active size of that row.
  // it's capacity is 10^(i+1) by definition, so we don't store that.
  tslice_t<int> rows[ c_num_rows ];
  idx_t N;
};


#endif


// TODO: fix the zipset bugs.
#if 0
RegisterTest([]()
{
  rng_xorshift32_t rng;
  Init( rng, 1234 );
  zipset_t<u32> s;
  Init( s );

  For( i, 0, 100000 ) {
    auto val = Rand32( rng );
//    auto val = Cast( u32, i );
    InsertAllowDuplicates( s, val );

    bool found;
    idx_t row_idx;
    idx_t idx_in_row;
    Lookup( s, val, &found, &row_idx, &idx_in_row );
    AssertCrash( found );
    AssertCrash( s.rows[row_idx][idx_in_row] == val );
  }

  Kill( s );
});
#endif
