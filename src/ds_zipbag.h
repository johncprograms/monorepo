// Copyright (c) John A. Carlos Jr., all rights reserved.

#define ZIPBAG   zipbag_t<T>

//
// bag datastructure that holds items in unspecified order.
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
Templ struct
zipbag_t
{
  static constexpr idx_t c_num_rows = 32;

  T* rows[c_num_rows];
  idx_t N;
};
Templ Inl void
Init( ZIPBAG& s )
{
  Typezero( &s );
}
Templ Inl void
Insert(
  ZIPBAG& s,
  T* src,
  idx_t src_len
  )
{
  auto N = s.N;
  auto rows = s.rows;

  AssertCrash( N <= MAX_u32 );

  // TODO: work out the carrying that has to happen for arbitrary src_len.
  xxxxx;
}
Templ Inl void
Remove(
  ZIPBAG& s,
  T* dst,
  idx_t dst_len
  )
{
  auto N = s.N;
  auto rows = s.rows;

  // TODO: work out the carrying that has to happen for arbitrary dst_len.
  xxxxx;
}
Templ Inl void
FreeUnusedRows(
  ZIPBAG& s
  )
{
  auto N = s.N;
  auto rows = s.rows;

  auto num_rows = ZIPBAG::c_num_rows - _lzcnt_idx_t( N );
  For( i, 0, num_rows ) {
    auto row = rows[ i ];
    auto row_len = 1u << i;
    if( !( N & row_len ) ) {
      if( row ) {
        Free( row );
        rows[ i ] = 0;
      }
    }
  }
}

// TODO: zipbag_t but for some other base, e.g. base16 instead of base2.
//   that would allow for some leeway to avoid borrowing work during deletion.
//   it would also allow for less-frequent carrying during insertion.
//   in baseK we have to do carrying at least every K sequential inserts.

#if 0

struct
zip10bag_t
{
  static constexpr idx_t c_num_rows = 9;
  
  // rows[i].len is the active size of that row.
  // it's capacity is 10^(i+1) by definition, so we don't store that.
  tslice_t<int> rows[ c_num_rows ];
  idx_t N;
};


#endif
