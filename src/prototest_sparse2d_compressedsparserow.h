// Copyright (c) John A. Carlos Jr., all rights reserved.

// compressed sparse row format
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
Templ Inl T
ElementValue( sparse2d_csr_t<T>* a, idx_t x, idx_t y )
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
    return T{};
  }
  return nonzero_data[ row_start + sorted_insert_idx ];
}
