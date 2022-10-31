// Copyright (c) John A. Carlos Jr., all rights reserved.

// TODO: deprecated, we should delete this. it's always worse than c++ style arrays.
//   variable-length elements are a different story, this type doesn't handle those.

// meant to hold elements which are uniform in size, but the element size isn't known at compile-time.
struct
stack_cstyle_t
{
  u8* mem;
  idx_t bytecapacity; // # of bytes mem can possibly hold.
  idx_t len; // # of elements in mem.
  idx_t byteelem; // # of bytes in one element.
};

Inl void
Zero( stack_cstyle_t& stack )
{
  stack.mem = 0;
  stack.bytecapacity = 0;
  stack.len = 0;
  stack.byteelem = 0;
}

Inl idx_t
Capacity( stack_cstyle_t& stack )
{
  idx_t capacity = stack.bytecapacity / stack.byteelem;
  return capacity;
}

Inl void
Alloc( stack_cstyle_t& stack, idx_t nelems, idx_t elemsize )
{
  AssertCrash( nelems * elemsize <= 1ULL*1024*1024*1024 ); // You should use other data structures for large allocations!
  Zero( stack );
  stack.byteelem = elemsize;
  stack.bytecapacity = nelems * elemsize;
  stack.mem = MemHeapAlloc( u8, stack.bytecapacity );
  AssertCrash( ( stack.bytecapacity % stack.byteelem ) == 0 );
}

Inl void
Free( stack_cstyle_t& stack )
{
  if( stack.mem ) {
    MemHeapFree( stack.mem );
  }
  Zero( stack );
}


#define ByteArrayElem( type, stack, idx ) \
  Cast( type*, stack.mem + stack.byteelem * idx )


Inl void
Clear( stack_cstyle_t& stack )
{
  stack.len = 0;
}

Inl void
Reserve( stack_cstyle_t& stack, idx_t enforce_capacity )
{
  AssertCrash( enforce_capacity * stack.byteelem <= 1ULL*1024*1024*1024 ); // You should use other data structures for large allocations!
  if( stack.bytecapacity < enforce_capacity ) {
    auto new_capacity = 2 * stack.bytecapacity;
    while( new_capacity < enforce_capacity ) {
      new_capacity *= 2;
    }
    stack.mem = MemHeapRealloc( u8, stack.mem, stack.bytecapacity, new_capacity );
    stack.bytecapacity = new_capacity;
  }
}

Inl void*
AddBack( stack_cstyle_t& stack, idx_t nelems = 1 )
{
  Reserve( stack, stack.len + nelems );
  auto r = stack.mem + stack.byteelem * stack.len;
  stack.len += nelems;
  return r;
}

Inl void*
AddAt( stack_cstyle_t& stack, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( idx <= stack.len );
  Reserve( stack, stack.len + nelems );
  if( idx < stack.len ) {
    auto nshift = stack.len - idx;
    Memmove(
      stack.mem + stack.byteelem * ( idx + nelems ),
      stack.mem + stack.byteelem * idx,
      stack.byteelem * nshift
      );
  }
  auto r = stack.mem + stack.byteelem * idx;
  stack.len += nelems;
  return r;
}

Inl void
RemBack( stack_cstyle_t& stack, idx_t nelems = 1 )
{
  AssertCrash( nelems <= stack.len );
  stack.len -= nelems;
}

Inl void
RemAt( stack_cstyle_t& stack, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( idx + nelems <= stack.len );
  if( idx + nelems < stack.len ) {
    Memmove(
      stack.mem + stack.byteelem * idx,
      stack.mem + stack.byteelem * ( idx + nelems ),
      stack.byteelem * ( stack.len - idx - nelems )
      );
  }
  stack.len -= nelems;
}
