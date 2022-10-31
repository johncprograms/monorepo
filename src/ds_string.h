// Copyright (c) John A. Carlos Jr., all rights reserved.

// TODO: we can make tstring_t an alias of stack_resizeable_cont_t<u8> i believe.
//   the only consideration is: should we even have an alias, or just use <u8> everywhere?

#define TSTRING   tstring_t<T, Allocator, Allocation>

// arbitrary-length contiguous string of memory.
TEA struct
tstring_t
{
  T* mem;
  idx_t len; // # of bytes mem can possibly hold.
  Allocator alloc;
  Allocation allocn;
};

//template< typename Allocator = allocator_heap_or_virtual_t >
using string_t = tstring_t<u8>;

TEA Inl void
Zero( TSTRING& dst )
{
  dst.mem = 0;
  dst.len = 0;
  dst.alloc = {};
  dst.allocn = {};
}
TEA Inl void
ZeroContents( TSTRING& dst )
{
  Memzero( dst.mem, dst.len * sizeof( T ) );
}
TEA Inl tslice_t<T>
SliceFromString( TSTRING& str )
{
  tslice_t<T> r;
  r.mem = str.mem;
  r.len = str.len;
  return r;
}
TEA Inl tslice32_t<T>
Slice32FromString( TSTRING& str )
{
  tslice32_t<T> r;
  r.mem = str.mem;
  AssertCrash( str.len <= MAX_u32 );
  r.len = Cast( u32, str.len );
  return r;
}
template< typename T = u8, typename Allocator = allocator_heap_or_virtual_t, typename Allocation = allocation_heap_or_virtual_t >
Inl TSTRING
AllocString( idx_t len, Allocator alloc = {} )
{
  TSTRING dst;
  dst.alloc = alloc;
  dst.len = len;
  dst.mem = Allocate<T>( dst.alloc, dst.allocn, len );
  return dst;
}
// TA Inl tstring_t<u8, Allocator, Allocation>
// AllocString( idx_t len, Allocator alloc = {} )
// {
//   return AllocString<u8, Allocator, Allocation>( len, alloc );
// }

TEA Inl void
Free( TSTRING& dst )
{
  Free( dst.alloc, dst.allocn, dst.mem );
  Zero( dst );
}
TEA Inl void
ExpandTo( TSTRING& dst, idx_t len_new )
{
  AssertCrash( len_new > dst.len );
  dst.mem = Reallocate<T>( dst.alloc, dst.allocn, dst.mem, dst.len, len_new );
  dst.len = len_new;
}
TEA Inl void
ShrinkTo( TSTRING& dst, idx_t len_new )
{
  AssertCrash( len_new < dst.len );
  AssertCrash( len_new > 0 );
  dst.mem = Reallocate<T>( dst.alloc, dst.allocn, dst.mem, dst.len, len_new );
  dst.len = len_new;
}
TEA Inl void
Reserve( TSTRING& dst, idx_t enforce_capacity )
{
  AssertCrash( dst.len );
  if( dst.len < enforce_capacity ) {
    auto len_new = 2 * dst.len;
    while( len_new < enforce_capacity ) {
      len_new *= 2;
    }
    ExpandTo( dst, len_new );
  }
}
TEA Inl bool
PtrInsideMem( TSTRING& str, void* ptr )
{
  AssertCrash( str.mem );
  AssertCrash( str.len );

  void* start = str.mem;
  void* end = str.mem + str.len;
  bool inside = ( start <= ptr )  &&  ( ptr < end );
  return inside;
}
TEA Inl bool
Equal( TSTRING& a, TSTRING& b )
{
  return MemEqual( ML( a ), ML( b ) );
}

TEA Inl u8*
AllocCstr( TSTRING& str )
{
  return AllocCstr( ML( str ) );
}

#ifdef MAC
  int vsprintf_s(
     char *buffer,
     size_t numberOfElements,
     const char *format,
     va_list argptr
  );
#endif
tstring_t<u8, allocator_heap_t, allocation_heap_t>
AllocFormattedString( const void* cstr ... )
{
  auto str = AllocString<u8, allocator_heap_t, allocation_heap_t>( MAX( 32768, 2 * CstrLength( Str( cstr ) ) ) );

  va_list args;
  va_start( args, cstr );
  str.len = vsprintf_s( // TODO: stop using CRT version.
    Cast( char* const, str.mem ),
    str.len,
    Cast( const char* const, cstr ),
    args
    );
  va_end( args );

  return str;
}
