// Copyright (c) John A. Carlos Jr., all rights reserved.

// HEAP OR VIRTUAL ALLOCATOR
// This uses the CRT heap for smaller allocations, and the OS Virtual heap for large allocations.

Enumc( alloctype_t )
{
  no_alloc,
  crt_heap,
  virtualalloc,
};

Templ Inl T* Allocate( alloctype_t* allocn, idx_t num_elements, idx_t alignment_pow2 = DEFAULT_ALIGN )
{
  auto nbytes = num_elements * sizeof( T );
  T* r = 0;
  if( nbytes >= c_virtualalloc_threshold ) {
    *allocn = alloctype_t::virtualalloc;
    r = MemVirtualAlloc( T, num_elements );
  }
  else {
    *allocn = alloctype_t::crt_heap;
    r = MemHeapAlloc( T, num_elements );
  }
  AssertCrash( Cast( idx_t, r ) % alignment_pow2 == 0 );
  return r;
}
Templ Inl T* Reallocate( alloctype_t allocn, T* oldmem, idx_t oldlen, idx_t newlen, idx_t alignment_pow2 = DEFAULT_ALIGN )
{
  T* r = 0;
  switch( allocn ) {
    case alloctype_t::crt_heap:
      r = MemHeapRealloc( T, oldmem, oldlen, newlen );
      break;

    case alloctype_t::virtualalloc:
      r = MemVirtualRealloc( T, oldmem, oldlen, newlen );
      break;

    default: UnreachableCrash();
  }
  AssertCrash( Cast( idx_t, r ) % alignment_pow2 == 0 );
  return r;
}
Inl void Free( alloctype_t allocn, void* mem )
{
  if( mem ) {
    switch( allocn ) {
      case alloctype_t::crt_heap:
        MemHeapFree( mem );
        break;

      case alloctype_t::virtualalloc:
        MemVirtualFree( mem );
        break;

      case alloctype_t::no_alloc:
        break;

      default: UnreachableCrash();
    }
  }
}
