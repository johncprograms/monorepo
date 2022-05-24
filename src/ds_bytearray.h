// Copyright (c) John A. Carlos Jr., all rights reserved.


// meant to hold elements which are uniform in size, but the element size isn't known at compile-time.
struct
bytearray_t
{
  u8* mem;
  idx_t bytecapacity; // # of bytes mem can possibly hold.
  idx_t len; // # of elements in mem.
  idx_t byteelem; // # of bytes in one element.
};


Inl void
Zero( bytearray_t& bytearray )
{
  bytearray.mem = 0;
  bytearray.bytecapacity = 0;
  bytearray.len = 0;
  bytearray.byteelem = 0;
}


Inl idx_t
Capacity( bytearray_t& bytearray )
{
  idx_t capacity = bytearray.bytecapacity / bytearray.byteelem;
  return capacity;
}



Inl void
Alloc( bytearray_t& bytearray, idx_t nelems, idx_t elemsize )
{
  AssertCrash( nelems * elemsize <= 1ULL*1024*1024*1024 ); // You should use other data structures for large allocations!
  Zero( bytearray );
  bytearray.byteelem = elemsize;
  bytearray.bytecapacity = nelems * elemsize;
  bytearray.mem = MemHeapAlloc( u8, bytearray.bytecapacity );
  AssertCrash( ( bytearray.bytecapacity % bytearray.byteelem ) == 0 );
}

Inl void
Free( bytearray_t& bytearray )
{
  if( bytearray.mem ) {
    MemHeapFree( bytearray.mem );
  }
  Zero( bytearray );
}


#define ByteArrayElem( type, bytearray, idx ) \
  Cast( type*, bytearray.mem + bytearray.byteelem * idx )


Inl void
Clear( bytearray_t& bytearray )
{
  bytearray.len = 0;
}


Inl void
Reserve( bytearray_t& bytearray, idx_t enforce_capacity )
{
  AssertCrash( enforce_capacity * bytearray.byteelem <= 1ULL*1024*1024*1024 ); // You should use other data structures for large allocations!
  if( bytearray.bytecapacity < enforce_capacity ) {
    auto new_capacity = 2 * bytearray.bytecapacity;
    while( new_capacity < enforce_capacity ) {
      new_capacity *= 2;
    }
    bytearray.mem = MemHeapRealloc( u8, bytearray.mem, bytearray.bytecapacity, new_capacity );
    bytearray.bytecapacity = new_capacity;
  }
}


Inl void*
AddBack( bytearray_t& bytearray, idx_t nelems = 1 )
{
  Reserve( bytearray, bytearray.len + nelems );
  auto r = bytearray.mem + bytearray.byteelem * bytearray.len;
  bytearray.len += nelems;
  return r;
}



Inl void*
AddAt( bytearray_t& bytearray, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( idx <= bytearray.len );
  Reserve( bytearray, bytearray.len + nelems );
  if( idx < bytearray.len ) {
    auto nshift = bytearray.len - idx;
    Memmove(
      bytearray.mem + bytearray.byteelem * ( idx + nelems ),
      bytearray.mem + bytearray.byteelem * idx,
      bytearray.byteelem * nshift
      );
  }
  auto r = bytearray.mem + bytearray.byteelem * idx;
  bytearray.len += nelems;
  return r;
}


Inl void
RemBack( bytearray_t& bytearray, idx_t nelems = 1 )
{
  AssertCrash( nelems <= bytearray.len );
  bytearray.len -= nelems;
}


Inl void
RemAt( bytearray_t& bytearray, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( idx + nelems <= bytearray.len );
  if( idx + nelems < bytearray.len ) {
    Memmove(
      bytearray.mem + bytearray.byteelem * idx,
      bytearray.mem + bytearray.byteelem * ( idx + nelems ),
      bytearray.byteelem * ( bytearray.len - idx - nelems )
      );
  }
  bytearray.len -= nelems;
}
