// Copyright (c) John A. Carlos Jr., all rights reserved.

// simple dynamic stack, which always has a power of 2 capacity.
// this means we can elide the usual capacity field, and instead just round up the length field.
// the theory being, smaller stack size is sometimes worth the computation cost of reconstructing capacity.

#define STACKIMPLICITCAPACITY   stack_implicitcapacity_t<T>

// meant to hold elements which are uniform in size.
Templ struct
stack_implicitcapacity_t
{
  T* mem;
  idx_t len; // # of elements in mem.
  alloctype_t allocn;
};

Inl idx_t
ImplicitCapacity( idx_t len )
{
  auto r = RoundUpToNextPowerOf2( len );
  return r;
}
Templ Inl idx_t
Capacity( STACKIMPLICITCAPACITY& stack )
{
  return ImplicitCapacity( stack.len );
}

Templ Inl tslice_t<T>
SliceFromArray( STACKIMPLICITCAPACITY& stack )
{
  tslice_t<T> r;
  r.mem = stack.mem;
  r.len = stack.len;
  return r;
}
Templ Inl void
Zero( STACKIMPLICITCAPACITY& stack )
{
  stack.mem = 0;
  stack.len = 0;
}
Templ Inl void
Alloc( STACKIMPLICITCAPACITY& stack, idx_t nelems )
{
  auto capacity = ImplicitCapacity( nelems );
  stack.len = nelems;
  stack.mem = Allocate<T>( &stack.allocn, capacity );
}
Templ Inl void
Free( STACKIMPLICITCAPACITY& stack )
{
  if( stack.mem ) {
    Free( stack.allocn, stack.mem );
  }
  Zero( stack );
}
Templ Inl void
Reserve( STACKIMPLICITCAPACITY& stack, idx_t enforce_capacity )
{
  auto new_capacity = ImplicitCapacity( enforce_capacity );
  auto capacity = ImplicitCapacity( stack.len );
  if( capacity < new_capacity ) {
    stack.mem = Reallocate<T>( stack.mem, capacity, new_capacity );
  }
}
Templ Inl void
Copy( STACKIMPLICITCAPACITY& stack, tslice_t<T>& src )
{
  Reserve( stack, src.len );
  Memmove( stack.mem, src.mem, src.len * sizeof( T ) );
  stack.len = src.len;
}
Templ Inl T*
AddBack( STACKIMPLICITCAPACITY& stack, idx_t nelems = 1 )
{
  Reserve( stack, stack.len + nelems );
  auto r = stack.mem + stack.len;
  stack.len += nelems;
  return r;
}
Templ Inl T*
AddAt( STACKIMPLICITCAPACITY& stack, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( idx <= stack.len );
  Reserve( stack, stack.len + nelems );
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
Templ Inl void
RemBack( STACKIMPLICITCAPACITY& stack, idx_t nelems = 1 )
{
  AssertCrash( nelems <= stack.len );
  stack.len -= nelems;
}
Templ Inl void
RemAt( STACKIMPLICITCAPACITY& stack, idx_t idx, idx_t nelems = 1 )
{
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
Templ Inl void
UnorderedRemAt( STACKIMPLICITCAPACITY& stack, idx_t idx )
{
  idx_t nelems = 1; // TODO: rewrite for larger values?
  AssertCrash( idx + nelems <= stack.len );
  if( idx + nelems < stack.len ) {
    Memmove(
      stack.mem + idx,
      stack.mem + ( stack.len - nelems ),
      sizeof( T ) * nelems
      );
  }
  stack.len -= nelems;
}
Templ Inl void
AddBackContents(
  STACKIMPLICITCAPACITY* stack,
  tslice_t<T> contents
  )
{
  Memmove( AddBack( *stack, contents.len ), contents.mem, contents.len * sizeof( T ) );
}
