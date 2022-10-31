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

  auto num_carries = _tzcnt_idx_t( ~N );
  auto carry_row_len = 1u << num_carries;
  AssertCrash( !( N & carry_row_len ) );

  if( !rows[ num_carries ] ) {
    rows[ num_carries ] = Allocate<int>( s.allocs[ num_carries ], carry_row_len );
  }
  auto carry_row = rows[ num_carries ];

  //
  // write the new value to the start + 1 of the new carry_row.
  // why the initial offset of +1? see below.
  //
  // then we'll zipper-merge existing lower-level rows with the last merge-result in the carry_row.
  //
  // e.g. say N=15
  // [ h i j k l m n o ]
  // [ d e f g ]
  // [ b c ]
  // [ a ]
  // and we insert p
  //
  // each subsequent carry zipper-merge will have carry_row:
  // zipper 0: [ _ p _ _ _ _ _ _ _ _ _ _ _ _ _ _ ] len = 1
  // zipper 1: [ _ p a p _ _ _ _ _ _ _ _ _ _ _ _ ] len = 1 + 2
  // zipper 2: [ _ p a p a b c p _ _ _ _ _ _ _ _ ] len = 1 + 2 + 4
  // zipper 3: [ _ p a p a b c p a b c d e f g p ] len = 1 + 2 + 4 + 8
  // zipper 4: [ a b c d e f g h i j k l m n o p ] carry_row is now full and valid!
  // where the _ indicates empty space.
  //
  // note that we initially skip the first slot so that zipper 3 writes its results into
  // the second half of carry_row precisely.
  //
  // that's important so that the last carry zipper-merge can write to the start of carry_row,
  // as we read from the second half of carry_row.
  // there's no danger of read overtaking write here, as long as the carry_row is precisely twice
  // the size of the row we're zipper-merging with.
  // this is the core idea to help avoid having to allocate a temporary buffer.
  //
  auto carry_row_write = carry_row + 1;
  *carry_row_write++ = value;
  auto carry_row_read = carry_row;

  For( i, 0, num_carries ) {
    // note the final zipper-merge writes to the start of carry_row, see above.
    if( i + 1 == num_carries ) {
      carry_row_write = carry_row;
    }

    auto row = rows[ i ];
    auto row_len = 1u << i;
    AssertCrash( N & row_len );

    //
    // standard zipper-merge: ( carry_row_read, row_read ) -> ( carry_row_write )
    //
    auto carry_row_read_before_merge = carry_row_read;
    auto row_read = row;
    auto merge_len = 2 * row_len;
    For( merge_idx, 0, merge_len ) {
      if( *carry_row_read < *row_read ) {
        *carry_row_write++ = *carry_row_read++;
      }
      else {
        *carry_row_write++ = *row_read++;
      }
    }
    AssertCrash( row_read == row + row_len );
    AssertCrash( carry_row_read == carry_row_read_before_merge + row_len );
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

  auto num_rows = ZIPSET::c_num_rows - _lzcnt_idx_t( N );
  For( i, 0, num_rows ) {
    auto row = rows[ i ];
    auto row_len = 1u << i;
    idx_t sorted_insert_idx;
    BinarySearch( row, row_len, value, &sorted_insert_idx );
    if( row[ sorted_insert_idx ] == value ) {
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
TEA Inl void
FreeUnusedRows(
  ZIPSET& s
  )
{
  auto N = s.N;
  auto rows = s.rows;

  auto num_rows = ZIPSET::c_num_rows - _lzcnt_idx_t( N );
  For( i, 0, num_rows ) {
    auto row = rows[ i ];
    auto row_len = 1u << i;
    if( !( N & row_len ) ) {
      if( row ) {
        Free( s.alloc, row );
        rows[ i ] = 0;
      }
    }
  }
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
