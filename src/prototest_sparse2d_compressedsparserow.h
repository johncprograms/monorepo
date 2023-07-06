// Copyright (c) John A. Carlos Jr., all rights reserved.

// compressed sparse row format
//
// row_starts[y] stores the number of nonzero elements on all rows < y.
// the # of nonzero elements in a row y is: row_starts[y+1] - row_starts[y]
// so first you index by y to get the row of interest, and how many nonzero elements are in it.
// then the row elements are assumed to be sorted in column order, and direct-mapped. that is,
//   col_idxs[row_start[y] + i] stores the column number for the i-th nonzero element in row y.
//   nonzero_data[row_start[y] + i] stores the data for the i-th nonzero element in row y.
//
// this makes for maximum-speed row-wise iteration of the whole grid, since the data is packed as tightly
// as possible.
//
Templ struct
sparse2d_csr_t
{
  idx_t dim_x;
  idx_t dim_y;
  idx_t nonzero_len;
  T* nonzero_data; // array of length nonzero_len
  idx_t* col_idxs; // array of length nonzero_len
  idx_t* row_starts; // array of length ( dim_x + 1 )
};
Templ Inl void
ElementValue( sparse2d_csr_t<T>* a, idx_t x, idx_t y, T* dst, bool* found )
{
  auto dim_x = a->dim_x;
  auto dim_y = a->dim_y;
  auto row_starts = a->row_starts;
  auto col_idxs = a->col_idxs;
  auto nonzero_data = a->nonzero_data;
  AssertCrash( x < dim_x );
  AssertCrash( y < dim_y );
  auto row_start = row_starts[ y ];
  auto row_end = row_starts[ y + 1 ];
  auto col_start = col_idxs + row_start;
  auto col_len = row_end - row_start;
  // assume the column indices are sorted, within each row.
  idx_t sorted_insert_idx;
  BinarySearch( col_start, col_len, x, &sorted_insert_idx );
  if( sorted_insert_idx == col_len || col_start[sorted_insert_idx] != x ) {
    // x not found in the sorted list of column indices.
    *found = false;
    return;
  }
  *found = true;
  *dst = nonzero_data[ row_start + sorted_insert_idx ];
}



// packed upper triangular, row-wise
//   0 1 2 3   4
//     4 5 6   3
//       7 8   2
//         9   1
//
//       i: 0 1 2 3 | 4 5 6 | 7 8 | 9
//       y: 0 0 0 0 | 1 1 1 | 2 2 | 3
//       x: 0 1 2 3 | 1 2 3 | 2 3 | 3
// (i-0)/4: 0 0 0 0 | 1 1 1 | 1 2 | 2
// (i-4)/3:         | 0 0 0 | 1 1 | 1
// (i-7)/2:                 | 0 0 | 1
// (i-9)/1:                       | 0
// diag: 0 4 7 9
// in general, diag: 0 (n) (n+n-1) (n+n-1+n-2) ... (n+...+n-n)
// diag(k,n) = sum( n-i, i=0..k )
//           = (2n - k)(k + 1)/2
// diag(n,n) = n(n+1)/2
// so we can go from x,y to i by:
// i(x,y) = (2n-y)(y+1)/2 + x
// note this requires using n in the computation, precisely an extra integer FMA over packed lower tri.
// so packed lower tri is going to be fewer integer ops than packed upper tri.
//
// transposed packed upper triangular
// aka
// packed lower triangular, col-wise
//   0 4 7 9
//   1 5 8
//   2 6
//   3
// i(x,y) = (2n-x)(x+1)/2 + y
//

Templ ForceInl idx_t
PackedUpperTriRowWise_IndexFromXY(
  idx_t x,
  idx_t y,
  idx_t n
  )
{
  auto i = ( 2 * n - y ) * ( y + 1 ) / 2 + x;
  return i;
}

// packed lower triangular, row-wise
//   0         1
//   1 2       2
//   3 4 5     3
//   6 7 8 9   4
//
// i: 0 | 1 2 | 3 4 5 | 6 7 8 9
// y: 0 | 1 1 | 2 2 2 | 3 3 3 3
// x: 0 | 0 1 | 0 1 2 | 0 1 2 3
// diag: 0 2 5 9
// diag: 0 (0+2) (0+2+3) (0+2+3+4)
// diag(n) = n(n+1)/2 - 1, for n>0
// first: 0 1 3 6
// first: 0 (0+1) (0+1+2) (0+1+2+3)
// first(k) = sum( i, i=0..k )
// first(n) = n(n+1)/2
// so we can go from x,y to i by:
// i(x,y) = y(y+1)/2 + x
// it looks like finding an inverse mapping from i to x,y is a bit harder.
// loop y and compare i <= diag(y) to find the row y containing i. then x is easy.
// that's O(log n) loop since we can binary search y.
// can we do better? constant time would be cool.
// it's a shame the rows aren't powers of two in length, since we can do log2 in constant time with bitops.
//
// transposed packed lower triangular
// aka
// packed upper triangular, col-wise
//   0 1 3 6
//     2 4 7
//       5 8
//         9
// i(x,y) = x(x+1)/2 + y
//

Templ ForceInl idx_t
PackedLowerTriRowWise_IndexFromXY(
  idx_t x,
  idx_t y
  )
{
  auto i = y * ( y + 1 ) / 2 + x;
  return i;
}

// symmetric matrix
// this is a matrix s.t. m[i][j] = m[j][i]
// so we only need to store the upper or lower tri (including diagonal).
// the basic idea: given [i][j], always take [u][v] where:
//   u = min(i,j)
//   v = max(i,j)
// that way u,v are always sorted s.t. u <= v.
// note it'll also work for i==j.
//
// these two options use fewer integer ops than the alternatives:
//   packed lower triangular, row-wise
//   packed upper triangular, col-wise
//

Templ ForceInl idx_t
SymmetricRowWise_IndexFromXY(
  idx_t x,
  idx_t y
  )
{
  if( x > y ) { // PERF: ensure branchless
    SWAP( idx_t, x, y );
  }
  auto i = y * ( y + 1 ) / 2 + x;
  return i;
}

Templ ForceInl idx_t
SymmetricColWise_IndexFromXY(
  idx_t x,
  idx_t y
  )
{
  if( x > y ) { // PERF: ensure branchless
    SWAP( idx_t, x, y );
  }
  auto i = x * ( x + 1 ) / 2 + y;
  return i;
}

// banded matrix
// example with B=1, the band radius (# elements either side of diagonal, per-row)
//   0 1
//   2 3 4
//     5 6 7
//       8 9
// i: 0 1 | 2 3 4 | 5 6 7 | 8 9
// x: 0 1 | 0 1 2 | 1 2 3 | 2 3
// y: 0 0 | 1 1 1 | 2 2 2 | 3 3
// # elements per-row = 1 + 2B
//                    = 3
// y(i) = 0, when i<=B,
//        1 + (i-(B+1))/(1+2B), otherwise.
// x(i) = i, when i<=B,
//        1+B + TODO: what is this?
//
// it might be easier to store diagonal traces i think, rather than row-wise like above. CASE1
//   6
//   3 7
//   1 4 8
//   0 2 5 9
// i(x=0,y) = (n-1-y)((n-1-y)+1)/2
//          = (n-y-1)(n-y)/2
// i(x,y) = 1 + i(x-1,y-1)
//        = x + i(x=0,y-x)
//        = x + (n-(y-x)-1)(n-(y-x))/2
//
// now the opposite diagonal trace layout, to try and eliminate the n term. CASE2
//   0
//   4 1
//   7 5 2
//   9 8 6 3
// i(x=0,y) = (2n-y)(y+1)/2
// i(x,y) = x + i(x=0,y-x)
//        = x + (2n-(y-x))((y-x)+1)/2
//
// well that wasn't it. let's try another. CASE3
//   9
//   5 8
//   2 4 7
//   0 1 3 6
// i(x,y=n-1) = x(x+1)/2
// i(x,y) = 1 + i(x+1,y+1)
//        = 1 + 1 + i(x+2,y+2)
//        = (n-1-y) + ((x+n-1)-y)((x+n-1-y)+1)/2
//        = (n-y-1) + (n-y-1+x)(n-y+x)/2
//
// that's also not it. another. CASE4
//   3
//   6 2
//   8 5 1
//   9 7 4 0
// i(x,y=n-1) = (2n-x)(x+1)/2
// i(x,y) = 1 + i(x+1,y+1)
//        = 1 + 1 + i(x+2,y+2)
//        = (n-1-y) + i(x+n-1-y,y=n-1)
//        = (n-1-y) + (2n-(x+n-1-y))(x+n-1-y)/2
//        = (n-1-y) + (2n-x-n+1+y)(x+n-1-y)/2
//        = (n-y-1) + (n-x+1+y)(n-y-1+x)/2
//
// still no success. is it possible? i think a mirror is required. CASE5
//   0 1 3 6
//   2 4 7
//   5 8
//   9
// i(x,y=0) = x(x+1)/2
// i(x,y) = 1 + i(x+1,y-1)
//        = y + i(x+y,y=0)
//        = y + (x+y)(x+y+1)/2
// mirroring y gives the CASE3.
//
// mirroring x gives CASE6
//   6 3 1 0
//     7 4 2
//       8 5
//         9
// i(x,y) = y + (n-1-x+y)(n-x+y)/2
//
// yeah so i think we're required to use n when computing left-leaning traces.
// that's pretty interesting that it's not required for right-leaning traces...


// Hankel matrix
// This is only made up of right leaning traces, where each value along a trace is identical.
//   a b c d
//   b c d e
//   c d e f
//   d e f g
// The number of traces is: 2(n-1)+1 = 2n-1
// So optimally we'd store f32[2n-1]. Natural indexing:
//   0 1 2 3
//   1 2 3 4
//   2 3 4 5
//   3 4 5 6
// i(x,y) = x+y
// Simple triangle formula, so that's nice.
// Note it's not uniquely invertible, since e.g. i=5 maps to (3,2) and (2,3).

// Toeplitz matrix
// This is the X-mirror of the Hankel matrix.
//   3 2 1 0
//   4 3 2 1
//   5 4 3 2
//   6 5 4 3
// i(x,y) = (n-1-x)+y


// What about packed lower triangular, row-wise, without the primary diagonal?
//   -
//   0 -
//   1 2 -
//   3 4 5 -
//
// Well this is identical to the version with the primary diagonal, just shifted down one row.
// The version with the primary diagonal has:
//   i_withdiag(x,y) = y(y+1)/2 + x
// So we just need to decrement y by one.
//   i(x,y) = i_withdiag(x,y-1) = (y-1)y/2 + x
//
Templ ForceInl idx_t
PackedLowerTriRowWiseNoPrimaryDiagonal_IndexFromXY(
  idx_t x,
  idx_t y
  )
{
  AssertCrash( x < y ); // implicitly checks that y > 0.
  auto i = ( y - 1 ) * y / 2 + x;
  return i;
}
