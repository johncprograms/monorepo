// Copyright (c) John A. Carlos Jr., all rights reserved.

TemplTIdxN struct
embeddedarray_t
{
  idx_t len; // # of elements in mem.
  T mem[N];
};

template<idx_t N> Inl slice_t
SliceFromArray( embeddedarray_t<u8, N>& array )
{
  slice_t r;
  r.mem = array.mem;
  r.len = array.len;
  return r;
}


TemplTIdxN Inl void
Zero( embeddedarray_t<T, N>& array )
{
  array.len = 0;
}


TemplTIdxN Inl idx_t
Capacity( embeddedarray_t<T, N>& array )
{
  return N;
}


TemplTIdxN Inl void
Copy( embeddedarray_t<T, N>& dst, embeddedarray_t<T, N>& src )
{
  AssertCrash( dst.len <= N );
  Memmove( dst.mem, src.mem, src.len * sizeof( T ) );
  dst.len = src.len;
}

TemplTIdxN Inl void
Clear( embeddedarray_t<T, N>& array )
{
  array.len = 0;
}


TemplTIdxN Inl T*
AddBack( embeddedarray_t<T, N>& array, idx_t nelems = 1 )
{
  AssertCrash( array.len + nelems <= N );
  auto r = array.mem + array.len;
  array.len += nelems;
  return r;
}


TemplTIdxN Inl T*
AddAt( embeddedarray_t<T, N>& array, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( array.len + nelems <= N );
  AssertCrash( idx <= array.len );
  if( idx < array.len ) {
    auto nshift = array.len - idx;
    Memmove(
      array.mem + idx + nelems,
      array.mem + idx,
      sizeof( T ) * nshift
    );
  }
  auto r = array.mem + idx;
  array.len += nelems;
  return r;
}


TemplTIdxN Inl void
RemBack( embeddedarray_t<T, N>& array, idx_t nelems = 1 )
{
  AssertCrash( array.len <= N );
  AssertCrash( nelems <= array.len );
  array.len -= nelems;
}


TemplTIdxN Inl void
RemAt( embeddedarray_t<T, N>& array, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( array.len <= N );
  AssertCrash( idx + nelems <= array.len );
  if( idx + nelems < array.len ) {
    Memmove(
      array.mem + idx,
      array.mem + idx + nelems,
      sizeof( T ) * ( array.len - idx - nelems )
      );
  }
  array.len -= nelems;
}

TemplTIdxN Inl void
AddBackContents(
  embeddedarray_t<T, N>* array,
  tslice_t<T> contents
  )
{
  Memmove( AddBack( *array, contents.len ), contents.mem, contents.len * sizeof( T ) );
}
