// Copyright (c) John A. Carlos Jr., all rights reserved.

Templ struct
fixedarray_t
{
  idx_t len; // # of elements in mem.
  T* mem;
  idx_t capacity; // # of elements mem can possibly hold.
};



Templ Inl void
Zero( fixedarray_t<T>& array )
{
  array.len = 0;
  array.mem = 0;
  array.capacity = 0;
}



Templ Inl void
Alloc( fixedarray_t<T>& array, idx_t nelems )
{
  AssertCrash( sizeof( T ) * nelems <= c_virtualalloc_threshold ); // You should use other data structures for large allocations!
  Zero( array );
  array.mem = MemHeapAlloc( T, nelems );
  array.capacity = nelems;
}


Templ Inl void
Free( fixedarray_t<T>& array )
{
  AssertCrash( array.len <= array.capacity );
  if( array.mem ) {
    MemHeapFree( array.mem );
  }
  Zero( array );
}



// Intentionally no expansion/contraction here!


Templ Inl idx_t
Capacity( fixedarray_t<T>& array )
{
  return array.capacity;
}


Templ Inl T*
AddBack( fixedarray_t<T>& array, idx_t nelems = 1 )
{
  AssertCrash( array.len + nelems <= array.capacity );
  auto r = array.mem + array.len;
  array.len += nelems;
  return r;
}


Templ Inl T*
AddAt( fixedarray_t<T>& array, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( array.len + nelems <= array.capacity );
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


Templ Inl void
RemBack( fixedarray_t<T>& array, idx_t nelems = 1 )
{
  AssertCrash( array.len <= array.capacity );
  AssertCrash( nelems <= array.len );
  array.len -= nelems;
}


Templ Inl void
RemAt( fixedarray_t<T>& array, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( array.len <= array.capacity );
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
