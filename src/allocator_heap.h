// Copyright (c) John A. Carlos Jr., all rights reserved.

#ifndef FINDLEAKS
  #error You have to decide if you want leak detection hooks or not. Usually you want them in debug.
#endif

// ============================================================================
// ALIGNED ALLOCATION ENTRY POINTS

#ifdef WIN
  #define AlignedAlloc( num_bytes, alignment )   _aligned_malloc( num_bytes, alignment );
  #define AlignedRealloc( old_mem, new_num_bytes, new_alignment )   _aligned_realloc( old_mem, new_num_bytes, new_alignment )
  #define AlignedFree( mem )   _aligned_free( mem )
#elifdef MAC
  Inl void* AlignedAlloc( idx_t num_bytes, idx_t alignment )
  {
    AssertCrash( IsPowerOf2( alignment ) );
    AssertCrash( alignment % sizeof( idx_t ) == 0u );
    void* mem;
    posix_memalign( &mem, alignment, num_bytes );
    return mem;
  }
  Inl void AlignedFree( void* mem )
  {
    free( mem );
  }
  Inl void* AlignedRealloc( void* old_mem, idx_t new_num_bytes, idx_t new_alignment )
  {
    void* new_mem = AlignedAlloc( new_num_bytes, new_alignment );
    ImplementCrash();
    idx_t old_num_bytes = 0; // TODO: how do we compute this? I guess we need to store the size in the allocated block?
    Memmove( new_mem, old_mem, old_num_bytes );
    AlignedFree( old_mem );
    return new_mem;
  }
#else
#error Unsupported platform
#endif

// ============================================================================
// GENERAL HEAP ALLOCATION ENTRY POINTS

#if FINDLEAKS
  Inl void
  FindLeaks_MemHeapAllocBytes( void* mem, idx_t nbytes );
  Inl void
  FindLeaks_MemHeapReallocBytes( void* oldmem, idx_t oldlen, void* newmem, idx_t newlen );
  Inl void
  FindLeaks_MemHeapFree( void* mem );
#endif

// must be power of 2.
// 64 bytes is one cache line on most chips these days, so it's convenient to have that alignment.
// that's kind of large, but you shouldn't be alloc'ing tiny things anyways.
constexpr idx_t DEFAULT_ALIGN = 64u;
constexpr idx_t DEFAULT_ALIGNMASK = ( DEFAULT_ALIGN - 1 );

Inl void*
MemHeapAllocBytes( idx_t nbytes )
{
  // NOTE: ~1GB heap allocations tended to cause a hang on my old laptop.
  // Anything remotely close to that size should be made by VirtualAlloc, not by the CRT heap!
  AssertCrash( nbytes <= 1ULL*1024*1024*1024 );

  void* mem = AlignedAlloc( nbytes, DEFAULT_ALIGN );
  AssertCrash( !( Cast( idx_t, mem ) & DEFAULT_ALIGNMASK ) );
  
#if FINDLEAKS
  FindLeaks_MemHeapAllocBytes( mem, nbytes );
#endif
  
  return mem;
}


Inl void*
MemHeapReallocBytes( void* oldmem, idx_t oldlen, idx_t newlen )
{
  // NOTE: ~1 Gb heap allocations tended to cause a hang on my old laptop.
  // Anything remotely close to that size should be made by VirtualAlloc, not by the CRT heap!
  AssertCrash( newlen <= 1ULL*1024*1024*1024 );
  AssertCrash( !( Cast( idx_t, oldmem ) & DEFAULT_ALIGNMASK ) );
  void* newmem = AlignedRealloc( oldmem, newlen, DEFAULT_ALIGN );
  AssertCrash( !( Cast( idx_t, newmem ) & DEFAULT_ALIGNMASK ) );
  
#if FINDLEAKS
  FindLeaks_MemHeapReallocBytes( oldmem, oldlen, newmem, newlen );
#endif
  
  return newmem;
}

// TODO: zero mem after free for everyone!
//   this requires knowing the length. we'd have to decide whether to introduce a header, or fixup callsites.

Inl void
MemHeapFree( void* mem )
{
  AssertCrash( !( Cast( idx_t, mem ) & DEFAULT_ALIGNMASK ) );
  AlignedFree( mem );
  
#if FINDLEAKS
  FindLeaks_MemHeapFree( mem );
#endif
  
}

#define MemHeapAlloc( type, num ) \
  Cast( type*, MemHeapAllocBytes( num * sizeof( type ) ) )

#define MemHeapRealloc( type, mem, oldnum, newnum ) \
  Cast( type*, MemHeapReallocBytes( mem, oldnum * sizeof( type ), newnum * sizeof( type ) ) )



constant idx_t c_virtualalloc_threshold = 100u * 1024*1024;

// ============================================================================
// HEAP ALLOCATOR

struct allocator_heap_t {};
struct allocation_heap_t {};

Templ Inl T* Allocate( allocator_heap_t& alloc, allocation_heap_t& allocn, idx_t num_elements, idx_t alignment_pow2 = DEFAULT_ALIGN )
{
  AssertCrash( sizeof( T ) * num_elements <= c_virtualalloc_threshold ); // You should use other data structures for large allocations!
  return MemHeapAlloc( T, num_elements );
}
Templ Inl T* Reallocate( allocator_heap_t& alloc, allocation_heap_t& allocn, T* oldmem, idx_t oldlen, idx_t newlen, idx_t alignment_pow2 = DEFAULT_ALIGN )
{
  AssertCrash( sizeof( T ) * newlen <= c_virtualalloc_threshold ); // You should use other data structures for large allocations!
  return MemHeapRealloc( T, oldmem, oldlen, newlen );
}
Inl void Free( allocator_heap_t& alloc, allocation_heap_t& allocn, void* mem )
{
  return MemHeapFree( mem );
}
