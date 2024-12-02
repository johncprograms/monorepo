// Copyright (c) John A. Carlos Jr., all rights reserved.

#define STACKNONRESIZEABLE   stack_nonresizeable_t<T>

Templ struct
stack_nonresizeable_t
{
  idx_t len; // # of elements in mem.
  T* mem;
  idx_t capacity; // # of elements mem can possibly hold.
  alloctype_t allocn;
};
Templ Inl void
Zero( STACKNONRESIZEABLE& stack )
{
  stack.len = 0;
  stack.mem = 0;
  stack.capacity = 0;
}
Templ Inl void
Alloc( STACKNONRESIZEABLE& stack, idx_t nelems )
{
  Zero( stack );
  stack.mem = Allocate<T>( &stack.allocn, nelems );
  stack.capacity = nelems;
}
Templ Inl void
Free( STACKNONRESIZEABLE& stack )
{
  AssertCrash( stack.len <= stack.capacity );
  if( stack.mem ) {
    Free( stack.allocn, stack.mem );
  }
  Zero( stack );
}

// Intentionally no expansion/contraction here!

Templ Inl idx_t
Capacity( STACKNONRESIZEABLE& stack )
{
  return stack.capacity;
}
Templ Inl idx_t
LenRemaining( STACKNONRESIZEABLE& stack )
{
  return stack.capacity - stack.len;
}
Templ Inl T*
AddBack( STACKNONRESIZEABLE& stack, idx_t nelems = 1 )
{
  AssertCrash( stack.len + nelems <= stack.capacity );
  auto r = stack.mem + stack.len;
  stack.len += nelems;
  return r;
}
Templ Inl T*
AddAt( STACKNONRESIZEABLE& stack, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( stack.len + nelems <= stack.capacity );
  AssertCrash( idx <= stack.len );
  if( idx < stack.len ) {
    auto nshift = stack.len - idx;
    TMove(
      stack.mem + idx + nelems,
      stack.mem + idx,
      nshift
      );
  }
  auto r = stack.mem + idx;
  stack.len += nelems;
  return r;
}
Templ Inl void
RemBack( STACKNONRESIZEABLE& stack, idx_t nelems = 1 )
{
  AssertCrash( stack.len <= stack.capacity );
  AssertCrash( nelems <= stack.len );
  stack.len -= nelems;
}
Templ Inl void
RemBack( STACKNONRESIZEABLE& stack, T* dst, idx_t dst_len = 1 )
{
  AssertCrash( stack.len <= stack.capacity );
  AssertCrash( dst_len <= stack.len );
  stack.len -= dst_len;
  TMove(
    dst,
    stack.mem + stack.len,
    dst_len
    );
}
Templ Inl void
RemBackReverse( STACKNONRESIZEABLE& stack, T* dst, idx_t dst_len = 1 )
{
  AssertCrash( stack.len <= stack.capacity );
  AssertCrash( dst_len <= stack.len );
  stack.len -= dst_len;
  TMoveReverse(
    dst,
    stack.mem + stack.len,
    dst_len
    );
}
Templ Inl void
RemAt( STACKNONRESIZEABLE& stack, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( stack.len <= stack.capacity );
  AssertCrash( idx + nelems <= stack.len );
  if( idx + nelems < stack.len ) {
    TMove(
      stack.mem + idx,
      stack.mem + idx + nelems,
      stack.len - idx - nelems
      );
  }
  stack.len -= nelems;
}
