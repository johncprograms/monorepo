// Copyright (c) John A. Carlos Jr., all rights reserved.

#define STACKNONRESIZEABLE   stack_nonresizeable_t<T, Allocator, Allocation>

TEA struct
stack_nonresizeable_t
{
  idx_t len; // # of elements in mem.
  T* mem;
  idx_t capacity; // # of elements mem can possibly hold.
  Allocator alloc;
  Allocation allocn;
};
TEA Inl void
Zero( STACKNONRESIZEABLE& stack )
{
  stack.len = 0;
  stack.mem = 0;
  stack.capacity = 0;
  stack.alloc = {};
}
TEA Inl void
Alloc( STACKNONRESIZEABLE& stack, idx_t nelems, Allocator alloc = {} )
{
  Zero( stack );
  stack.mem = Allocate<T>( alloc, stack.allocn, nelems );
  stack.capacity = nelems;
  stack.alloc = alloc;
}
TEA Inl void
Free( STACKNONRESIZEABLE& stack )
{
  AssertCrash( stack.len <= stack.capacity );
  if( stack.mem ) {
    Free( stack.alloc, stack.allocn, stack.mem );
  }
  Zero( stack );
}

// Intentionally no expansion/contraction here!

TEA Inl idx_t
Capacity( STACKNONRESIZEABLE& stack )
{
  return stack.capacity;
}
TEA Inl T*
AddBack( STACKNONRESIZEABLE& stack, idx_t nelems = 1 )
{
  AssertCrash( stack.len + nelems <= stack.capacity );
  auto r = stack.mem + stack.len;
  stack.len += nelems;
  return r;
}
TEA Inl T*
AddAt( STACKNONRESIZEABLE& stack, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( stack.len + nelems <= stack.capacity );
  AssertCrash( idx <= stack.len );
  if( idx < stack.len ) {
    auto nshift = stack.len - idx;
    Memmove(
      stack.mem + idx + nelems,
      stack.mem + idx,
      sizeof( T ) * nshift
    );
  }
  auto r = stack.mem + idx;
  stack.len += nelems;
  return r;
}
TEA Inl void
RemBack( STACKNONRESIZEABLE& stack, idx_t nelems = 1 )
{
  AssertCrash( stack.len <= stack.capacity );
  AssertCrash( nelems <= stack.len );
  stack.len -= nelems;
}
TEA Inl void
RemBack( STACKNONRESIZEABLE& stack, T* dst, idx_t dst_len = 1 )
{
  AssertCrash( stack.len <= stack.capacity );
  AssertCrash( dst_len <= stack.len );
  stack.len -= dst_len;
  TMove( dst, stack.mem + stack.len, dst_len );
}
TEA Inl void
RemAt( STACKNONRESIZEABLE& stack, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( stack.len <= stack.capacity );
  AssertCrash( idx + nelems <= stack.len );
  if( idx + nelems < stack.len ) {
    Memmove(
      stack.mem + idx,
      stack.mem + idx + nelems,
      sizeof( T ) * ( stack.len - idx - nelems )
      );
  }
  stack.len -= nelems;
}
