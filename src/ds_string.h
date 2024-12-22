// Copyright (c) John A. Carlos Jr., all rights reserved.

// TODO: we can make tstring_t an alias of stack_resizeable_cont_t<u8> i believe.
//   the only consideration is: should we even have an alias, or just use <u8> everywhere?

#define TSTRING   tstring_t<T>

// arbitrary-length contiguous string of memory.
Templ struct
tstring_t
{
  T* mem;
  idx_t len; // # of bytes mem can possibly hold.
  alloctype_t allocn;
  
  ForceInl operator tslice_t<T>()
  {
    return SliceFromString( *this );
  }
};

using string_t = tstring_t<u8>;

Templ Inl void
Zero( TSTRING& dst )
{
  dst.mem = 0;
  dst.len = 0;
  dst.allocn = {};
}
Templ Inl void
ZeroContents( TSTRING& dst )
{
  Memzero( dst.mem, dst.len * sizeof( T ) );
}
Templ Inl tslice_t<T>
SliceFromString( TSTRING& str )
{
  tslice_t<T> r;
  r.mem = str.mem;
  r.len = str.len;
  return r;
}
Templ Inl tslice32_t<T>
Slice32FromString( TSTRING& str )
{
  tslice32_t<T> r;
  r.mem = str.mem;
  AssertCrash( str.len <= MAX_u32 );
  r.len = Cast( u32, str.len );
  return r;
}
template< typename T = u8 >
Inl TSTRING
AllocString( idx_t len )
{
  TSTRING dst;
  dst.len = len;
  dst.mem = Allocate<T>( &dst.allocn, len );
  return dst;
}

Templ Inl void
Free( TSTRING& dst )
{
  Free( dst.allocn, dst.mem );
  Zero( dst );
}
Templ Inl void
ExpandTo( TSTRING& dst, idx_t len_new )
{
  AssertCrash( len_new > dst.len );
  dst.mem = Reallocate<T>( dst.allocn, dst.mem, dst.len, len_new );
  dst.len = len_new;
}
Templ Inl void
ShrinkTo( TSTRING& dst, idx_t len_new )
{
  AssertCrash( len_new < dst.len );
  AssertCrash( len_new > 0 );
  dst.mem = Reallocate<T>( dst.allocn, dst.mem, dst.len, len_new );
  dst.len = len_new;
}
Templ Inl void
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
Templ Inl bool
PtrInsideMem( TSTRING& str, void* ptr )
{
  AssertCrash( str.mem );
  AssertCrash( str.len );

  void* start = str.mem;
  void* end = str.mem + str.len;
  bool inside = ( start <= ptr )  &&  ( ptr < end );
  return inside;
}
Templ Inl bool
Equal( TSTRING& a, TSTRING& b )
{
  return MemEqual( ML( a ), ML( b ) );
}

Templ Inl u8*
AllocCstr( TSTRING& str )
{
  return AllocCstr( ML( str ) );
}

#if defined(MAC)
  int vsprintf_s(
     char *buffer,
     size_t numberOfElements,
     const char *format,
     va_list argptr
  )
  {
    char* temp_buffer;
    int len = vasprintf( &temp_buffer, format, argptr );
    if( len <= 0 ) {
      return len;
    }
    auto ulen = MIN( Cast( idx_t, len ), numberOfElements );
    Memmove( buffer, temp_buffer, ulen );
    free( temp_buffer );
    return Cast( s32, ulen );
  }
#endif
tstring_t<u8>
AllocFormattedString( const void* cstr ... )
{
  auto str = AllocString<u8>( MAX( 32768, 2 * CstrLength( Str( cstr ) ) ) );

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



RegisterTest([]()
{
  struct
  test_bool_void_t
  {
    bool b;
    void* p;
  };
  
  idx_t indices[] = {
    10, 5, 7, 8, 1, 3, 5, 5000, 1221, 200, 0, 20,
  };
  u8 values[] = {
    0, 1, 2, 255, 128, 50, 254, 253, 3, 100, 200, 222,
  };
  AssertCrash( _countof( indices ) == _countof( values ) );
  idx_t fill_count = _countof( indices );

  idx_t sizes[] = {
    1, 2, 3, 4, 5, 8, 10, 16, 24, 1024, 65536, 100000,
  };
  ForEach( size, sizes ) {
    auto str = AllocString<u8>( size );
    For( i, 0, fill_count ) {
      auto idx = indices[i];
      auto value = values[i];
      if( idx < size ) {
        str.mem[idx] = value;
        AssertCrash( str.mem[idx] == value );
      }
    }

    test_bool_void_t testcases[] = {
      { 0, str.mem - 5000 },
      { 0, str.mem - 50 },
      { 0, str.mem - 2 },
      { 0, str.mem - 1 },
      { 1, str.mem + 0 },
      { 1, str.mem + str.len / 2 },
      { 1, str.mem + str.len - 1 },
      { 0, str.mem + str.len + 0 },
      { 0, str.mem + str.len + 1 },
      { 0, str.mem + str.len + 2 },
      { 0, str.mem + str.len + 200 },
    };
    ForEach( testcase, testcases ) {
      AssertCrash( testcase.b == PtrInsideMem( str, testcase.p ) );
    }

    idx_t onesize = str.len;
    idx_t twosize = 2 * str.len;
    ExpandTo( str, twosize );
    AssertCrash( str.len == twosize );
    ShrinkTo( str, onesize );
    AssertCrash( str.len == onesize );

    Reserve( str, 0 );
    AssertCrash( str.len == onesize );
    Reserve( str, 1 );
    AssertCrash( str.len == onesize );
    Reserve( str, str.len - 1 );
    AssertCrash( str.len == onesize );
    Reserve( str, str.len + 0 );
    AssertCrash( str.len == onesize );
    Reserve( str, str.len + 1 );
    AssertCrash( str.len == twosize );
    Reserve( str, str.len + 1 );
    AssertCrash( str.len == 4 * onesize );

    Free( str );
  }
});

