// Copyright (c) John A. Carlos Jr., all rights reserved.

#if defined(WIN)
  Inl void
  MemVirtualFree( void* mem )
  {
    BOOL r = VirtualFree( mem, 0, MEM_RELEASE );
    AssertWarn( r );
  }

  Inl void*
  MemVirtualAllocBytes( idx_t nbytes )
  {
    //Log( "VA: %llu", nbytes );
    void* mem = VirtualAlloc( 0, nbytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
    return mem;
  }
#elif defined(MAC)

  #define stack_resizeable_cont_t signalstack_t
  #include <sys/types.h>
  #include <sys/mman.h>

  Inl void
  MemVirtualFree( void* mem )
  {
		auto umem = Cast( idx_t*, mem );
		auto nbytes_alloc = umem[-1];
    int result = munmap( umem - 1, nbytes_alloc );
    AssertCrash( !result );
  }
  Inl void*
  MemVirtualAllocBytes( idx_t nbytes )
  {
		auto nbytes_alloc = nbytes + _SIZEOF_IDX_T;
    void* memnew = mmap( 0 /*start_addr*/, nbytes_alloc, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1 /*file_descriptor*/, 0 /*offset_into_file*/);
    if( !memnew || memnew == Cast( void*, -1 ) ) {
      return 0;
    }
    auto umemnew = Cast( idx_t*, memnew );
    umemnew[0] = nbytes_alloc;
    return umemnew + 1;
  }
#endif

Inl void*
MemVirtualReallocBytes( void* oldmem, idx_t oldlen, idx_t newlen )
{
  void* memnew = MemVirtualAllocBytes( newlen );
  Memmove( memnew, oldmem, MIN( oldlen, newlen ) );
  MemVirtualFree( oldmem );
  return memnew;
}


#define MemVirtualAlloc( type, num ) \
  Cast( type*, MemVirtualAllocBytes( num * sizeof( type ) ) )

#define MemVirtualRealloc( type, mem, oldnum, newnum ) \
  Cast( type*, MemVirtualReallocBytes( mem, oldnum * sizeof( type ), newnum * sizeof( type ) ) )


// ============================================================================
// VIRTUAL ALLOCATOR

struct allocator_virtual_t {};
struct allocation_virtual_t {};

Templ Inl T* Allocate( allocator_virtual_t& alloc, allocation_virtual_t& allocn, idx_t num_elements )
{
  return MemVirtualAlloc( T, num_elements );
}
Templ Inl T* Reallocate( allocator_virtual_t& alloc, allocation_virtual_t& allocn, T* oldmem, idx_t oldlen, idx_t newlen )
{
  return MemVirtualRealloc( T, oldmem, oldlen, newlen );
}
Inl void Free( allocator_virtual_t& alloc, allocation_virtual_t& allocn, void* mem )
{
  return MemVirtualFree( mem );
}
