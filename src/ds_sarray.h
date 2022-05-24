// Copyright (c) John A. Carlos Jr., all rights reserved.


// meant to hold elements which are uniform in size.
Templ struct
sarray_t
{
  T* mem;
  idx_t len; // # of elements in mem.
};

Inl idx_t
SArrayCapacity( idx_t len )
{
  auto r = RoundUpToNextPowerOf2( array.len );
  return r;
}
Templ Inl idx_t
Capacity( sarray_t<T>& array )
{
  return SArrayCapacity( array.len );
}

Inl slice_t
SliceFromArray( sarray_t<u8>& array )
{
  slice_t r;
  r.mem = array.mem;
  r.len = array.len;
  return r;
}

Templ Inl tslice_t<T>
SliceFromArray( sarray_t<T>& array )
{
  tslice_t<T> r;
  r.mem = array.mem;
  r.len = array.len;
  return r;
}


Templ Inl void
Zero( sarray_t<T>& array )
{
  array.mem = 0;
  array.len = 0;
}



Templ Inl void
Alloc( sarray_t<T>& array, idx_t nelems )
{
  auto capacity = SArrayCapacity( nelems );
  AssertCrash( sizeof( T ) * capacity <= c_virtualalloc_threshold ); // You should use other data structures for large allocations!
  array.len = nelems;
  array.mem = MemHeapAlloc( T, capacity );
}

Templ Inl void
Free( sarray_t<T>& array )
{
  if( array.mem ) {
    MemHeapFree( array.mem );
  }
  Zero( array );
}



Templ Inl void
Reserve( sarray_t<T>& array, idx_t enforce_capacity )
{
  auto new_capacity = SArrayCapacity( enforce_capacity );
  auto capacity = SArrayCapacity( array.len );
  AssertCrash( sizeof( T ) * new_capacity <= c_virtualalloc_threshold ); // You should use other data structures for large allocations!
  if( capacity < new_capacity ) {
    array.mem = MemHeapRealloc( T, array.mem, capacity, new_capacity );
  }
}



Templ Inl void
Copy( sarray_t<T>& array, tslice_t<T>& src )
{
  Reserve( array, src.len );
  Memmove( array.mem, src.mem, src.len * sizeof( T ) );
  array.len = src.len;
}



Templ Inl T*
AddBack( sarray_t<T>& array, idx_t nelems = 1 )
{
  AssertCrash( array.len <= array.capacity );
  Reserve( array, array.len + nelems );
  auto r = array.mem + array.len;
  array.len += nelems;
  return r;
}

Templ Inl T*
AddAt( sarray_t<T>& array, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( idx <= array.len );
  Reserve( array, array.len + nelems );
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

Templ Inl void
RemBack( sarray_t<T>& array, idx_t nelems = 1 )
{
  AssertCrash( nelems <= array.len );
  array.len -= nelems;
}

Templ Inl void
RemAt( sarray_t<T>& array, idx_t idx, idx_t nelems = 1 )
{
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

Templ Inl void
UnorderedRemAt( sarray_t<T>& array, idx_t idx )
{
  idx_t nelems = 1; // TODO: rewrite for larger values?
  AssertCrash( idx + nelems <= array.len );
  if( idx + nelems < array.len ) {
    Memmove(
      array.mem + idx,
      array.mem + ( array.len - nelems ),
      sizeof( T ) * nelems
      );
  }
  array.len -= nelems;
}

Inl void
AddBackContents(
  sarray_t<u8>* array,
  slice_t contents
  )
{
  Memmove( AddBack( *array, contents.len ), ML( contents ) );
}
Templ Inl void
AddBackContents(
  sarray_t<T>* array,
  tslice_t<T> contents
  )
{
  Memmove( AddBack( *array, contents.len ), contents.mem, contents.len * sizeof( T ) );
}
