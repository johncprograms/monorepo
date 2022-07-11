// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// comment text that i'll use for searchability later:
//     TODO: something left to do here.
//     PERF: pointing out a perf problem.
// we should resolve all the TODOs before considering any kind of general release.
// i'm okay with PERFs sticking around; we probably shouldn't resolve them until they become a measurable problem.
//







// ============================================================================
// WINDOWS JUNK


// for memory leak checking, put CrtShowMemleaks() first in every entry point.
#ifdef _DEBUG
  #define _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>
  #define CrtShowMemleaks()   _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF )
#else
  #define CrtShowMemleaks()    // nothing
#endif

#pragma comment( lib, "kernel32" )
#pragma comment( lib, "user32"   )
#pragma comment( lib, "gdi32"    )
//#pragma comment( lib, "winspool" )
//#pragma comment( lib, "comdlg32" )
//#pragma comment( lib, "advapi32" )
#pragma comment( lib, "shell32"  )
#pragma comment( lib, "shcore"   )
//#pragma comment( lib, "ole32"    )
//#pragma comment( lib, "oleaut32" )
//#pragma comment( lib, "uuid"     )
//#pragma comment( lib, "odbc32"   )
//#pragma comment( lib, "odbccp32" )
#pragma comment( lib, "winmm"    )


//#define _CRT_SECURE_NO_WARNINGS


// TODO: move these includes to crt_platform.h
// TODO: move platform specific headers out of this file!

//#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <stddef.h> // for idx_t, etc.
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h> // for isspace, etc.
#include <windows.h>
#include <ShellScalingApi.h>
#include <time.h>
#include <intrin.h>
#include <immintrin.h>
#include <process.h>
#include <dbghelp.h>
#include <type_traits>

// TODO: more undefs

#undef GetProp
#undef min
#undef Min
#undef MIN
#undef max
#undef Max
#undef MAX
#undef clamp
#undef Clamp
#undef CLAMP
#undef abs
#undef Abs
#undef ABS
#undef swap
#undef Swap
#undef SWAP





// ============================================================================
// CORE TYPE DEFINITIONS

#define CompileAssert( error_if_false )    static_assert( error_if_false, # error_if_false )

typedef int8_t     s8;
typedef int16_t    s16;
typedef int32_t    s32;
typedef int64_t    s64;

#define MIN_s8    Cast( s8, -128 )
#define MAX_s8    Cast( s8, 0x7F )

#define MIN_s16   Cast( s16, -32768 )
#define MAX_s16   Cast( s16, 0x7FFF )

#define MIN_s32   Cast( s32, INT_MIN ) // -2147483648, VC compiler doesn't parse this min value for whatever reason; see C4146 docs.
#define MAX_s32   Cast( s32, 0x7FFFFFFF )

#define MIN_s64   Cast( s64, LLONG_MIN ) // -9223372036854775808
#define MAX_s64   Cast( s64, 0x7FFFFFFFFFFFFFFFLL )


typedef uint8_t    u8;
typedef uint16_t   u16;
typedef uint32_t   u32;
typedef uint64_t   u64;

#define MAX_u8    Cast( u8, 0xFFU )
#define MAX_u16   Cast( u16, 0xFFFFU )
#define MAX_u32   Cast( u32, 0xFFFFFFFFU )
#define MAX_u64   Cast( u64, 0xFFFFFFFFFFFFFFFFULL )

#define HIGHBIT_u8    Cast( u8, 0x80U )
#define HIGHBIT_u16   Cast( u16, 0x8000U )
#define HIGHBIT_u32   Cast( u32, 0x80000000U )
#define HIGHBIT_u64   Cast( u64, 0x8000000000000000ULL )


typedef uintptr_t   idx_t;
typedef ptrdiff_t   sidx_t;

typedef float    f32;
typedef double   f64;

#define MIN_f32   ( -3.402823466e+38F )
#define MAX_f32   (  3.402823466e+38F )

#define MAX_INT_REPRESENTABLE_IN_f32 ( 1u << 24 )
#define MAX_INT_REPRESENTABLE_IN_f64 ( 1ull << 53 )

#define MIN_f64   ( -1.7976931348623158e+308 )
#define MAX_f64   (  1.7976931348623158e+308 )

#ifdef _M_AMD64

  CompileAssert( sizeof( long ) == 4 );

  #define _SIZEOF_IDX_T   8
  #define NUMBITS_idx     64
  #define MAX_idx         MAX_u64
  #define HIGHBIT_idx     HIGHBIT_u64
  #define MAX_sidx        MAX_s64
  #define MIN_sidx        MIN_s64

  #define Round_idx_from_f32( x ) \
    Round_u64_from_f32( x )

  #define Round_idx_from_f64( x ) \
    Round_u64_from_f64( x )

  #define Truncate_idx_from_f32( x ) \
    Truncate_u64_from_f32( x )

  #define Truncate_idx_from_f64( x ) \
    Truncate_u64_from_f64( x )

  #define GetThreadIdFast() \
    ( *Cast( u32*, Cast( u8*, __readgsqword( 0x30 ) ) + 0x48 ) )

  #define _lzcnt_idx_t \
    _lzcnt_u64

  #define _popcnt_idx_t( x ) \
    Cast( u32, _popcnt64( x ) )

  #define _BitScanForward_idx_t( a, b ) \
    _BitScanForward64( Cast( unsigned long*, ( a ) ), b )

#elif _M_IX86

  CompileAssert( sizeof( long ) == 4 );

  #define _SIZEOF_IDX_T   4
  #define NUMBITS_idx     32
  #define MAX_idx         MAX_u32
  #define HIGHBIT_idx     HIGHBIT_u32
  #define MAX_sidx        MAX_s32
  #define MIN_sidx        MIN_s32

  #define Round_idx_from_f32( x ) \
    Round_u32_from_f32( x )

  #define Round_idx_from_f64( x ) \
    Round_u32_from_f64( x )

  #define Truncate_idx_from_f32( x ) \
    Truncate_u32_from_f32( x )

  #define Truncate_idx_from_f64( x ) \
    Truncate_u32_from_f64( x )

  #define GetThreadIdFast() \
    ( *Cast( u32*, Cast( u8*, __readfsdword( 0x18 ) ) + 0x24 ) )

  #define _lzcnt_idx_t \
    _lzcnt_u32

  #define _popcnt_idx_t( x ) \
    Cast( u32, _popcnt32( x ) )

  #define _BitScanForward_idx_t( a, b ) \
    _BitScanForward( Cast( unsigned long*, ( a ) ), b )

#else
  #error Define _SIZEOF_IDX_T based on a predefined macro for your new platform.
#endif







// ============================================================================
// CORE LANGUAGE MACROS

#define elif \
  else if

// TODO: too many existing uses of 'ret' in enums, vars, etc.
//#define ret \
//  return


#define Cast( type, var )   ( ( type )( var ) )

// for string literals, we need to cast to u8* because there's no compiler option to make literals unsigned!
#define Str( x )   Cast( u8*, x )


#define ML( a ) \
  ( a ).mem, ( a ).len

#define AL( a ) \
  ( a + 0 ), Cast( idx_t, _countof( a ) )


// dear god, please save us from the infinitesimal wisdom of c++ macros.
// these are needed due to the order of evaluation of macros.
// so, never use ## directly unless you know what you're doing, use this NAMEJOIN instead!
// the _NAMEJOIN is intended to be private and shouldn't be used.
#define _NAMEJOIN( a, b ) a ## b
#define NAMEJOIN( a, b ) _NAMEJOIN( a, b )


#define ForceInl   __forceinline
#define ForceNoInl   __declspec( noinline )

#define NoInl   __declspec( noinline ) static

#if WEAKINLINING
  #define Inl   inline static
#else
  #define Inl   __forceinline static
#endif

#define constant      static constexpr
#define constantold   static const


// NOTE: for aligning some set of structs of arbitrary size that you're packing contiguously into an array.
//   we need each field to have aligned loads/stores, which is done by forcing _SIZEOF_IDX_T alignment.
#define ALIGNTOIDX   __declspec( align( _SIZEOF_IDX_T ) )


#define Templ        template< typename T >
#define TemplTIdxN   template< typename T, idx_t N >
#define TemplIdxN    template< idx_t N >


#define Implies( p, q ) \
  ( !( p )  ||  ( q ) )

#define LTEandLTE( x, x0, x1 )   ( ( x0 <= x )  &&  ( x <= x1 ) )
#define LTandLTE(  x, x0, x1 )   ( ( x0 <  x )  &&  ( x <= x1 ) )
#define LTEandLT(  x, x0, x1 )   ( ( x0 <= x )  &&  ( x <  x1 ) )

#define MIN( a, b )   ( ( ( a ) <= ( b ) )  ?  ( a )  :  ( b ) )
#define MAX( a, b )   ( ( ( a ) >  ( b ) )  ?  ( a )  :  ( b ) )

#define MIN3( a, b, c )   ( MIN( MIN( ( a ), ( b ) ), ( c ) ) )
#define MAX3( a, b, c )   ( MAX( MAX( ( a ), ( b ) ), ( c ) ) )

#define MIN4( a, b, c, d )   ( MIN( MIN( ( a ), ( b ) ), MIN( ( c ), ( d ) ) ) )
#define MAX4( a, b, c, d )   ( MAX( MAX( ( a ), ( b ) ), MAX( ( c ), ( d ) ) ) )

#define MIN5( a, b, c, d, e )   MIN( ( MIN( MIN( ( a ), ( b ) ), MIN( ( c ), ( d ) ) ) ), ( e ) )
#define MAX5( a, b, c, d, e )   MAX( ( MAX( MAX( ( a ), ( b ) ), MAX( ( c ), ( d ) ) ) ), ( e ) )

#define CLAMP( x, r0, r1 )   MAX( r0, MIN( r1, x ) )

#define ABS( a )   ( ( ( a ) < 0 )  ?  -( a )  :  ( a ) )

#define SWAP( type, a, b )   do { type tmp = a;  a = b;  b = tmp; } while( 0 )

#define PERMUTELEFT3( type, a, b, c )    do { type tmp = a;  a = b;  b = c;  c = tmp; } while( 0 )
#define PERMUTERIGHT3( type, a, b, c )   do { type tmp = a;  a = c;  c = b;  b = tmp; } while( 0 )

Inl u64
AModB( s64 a, u64 b )
{
  if( a < 0 ) {
    // TODO: terrible, mod twice.
    // try to avoid calling this function.
    auto r = b - Cast( u64, -a ) % b;
    return r % b;
  }
  else {
    auto r = Cast( u64, a ) % b;
    return r;
  }
}


// intel intrinsics that aren't really intrinsic; need decls:
u8 _addcarry_u64( u8 c_in, u64 a, u64 b, u64* out );
u8 _addcarry_u32( u8 c_in, u32 a, u32 b, u32* out );
u64 _lzcnt_u64( u64 a );
u32 _lzcnt_u32( u32 a );
s32 _popcnt64( s64 a );
s32 _popcnt32( s32 a );


// this should be defined to a "yy.mm.dd.hh.mm.ss" datetime string by the build system.
// this is how we know what version to display, to allow matching of .exe to source code.
#ifndef JCVERSION
#define JCVERSION "unknown_version"
#endif





// ============================================================================
// ENUM MACROS

typedef u32 enum_t;

#define Enumc( name ) \
  enum class name : enum_t

#if 0
  // ugh, c++ doesn't let you do bitmask enums very easily.
  // instead of reusing enumc for this, we'll make our own thing.

  define ENUM_IS_BITMASK( type ) \
    Inl type operator|( type a, type b ) \
    { \
      return Cast( type, Cast( enum_t, a ) | Cast( enum_t, b ) ); \
    } \
    Inl type operator&( type a, type b ) \
    { \
      return Cast( type, Cast( enum_t, a ) & Cast( enum_t, b ) ); \
    } \
    Inl bool operator bool( type a ) \
    { \
      return Cast( enum_t, a ); \
    } \

#endif







// ============================================================================
// FOR LOOP MACROS

#define Fori( idxtype, idx, start, end ) \
  for( idxtype idx = start;  idx < end;  ++idx )

// we can declare things outside the for(), then we can use different types.
// for now, it's fine to use idxtype for the bool too.
#define ReverseFori( idxtype, idx, start, end ) \
  for( \
    idxtype __loop = start < end, idx = MAX( 1, end ) - 1; \
    __loop  &&  idx >= start; \
    idx == start  ?  __loop = 0  :  --idx )

#define Forinc( idxtype, idx, start, end, inc ) \
  for( idxtype idx = start;  idx < end;  idx += inc )

#define ReverseForPrev( elem, lastelem ) \
  for( auto elem = lastelem;  elem;  elem = elem->prev )

#define ForLen( idx, arr ) \
  Fori( idx_t, idx, 0, ( arr ).len )

#define ForLen32( idx, arr ) \
  Fori( u32, idx, 0, ( arr ).len )

#define ReverseForLen( idx, arr ) \
  ReverseFori( idx_t, idx, 0, ( arr ).len )

#define ReverseForLen32( idx, arr ) \
  ReverseFori( u32, idx, 0, ( arr ).len )

// TODO: outlaw this!
//   i always get the end vs. count part wrong, and waste time debugging to figure that out.
// TODO: rename to ForRange ?
#define For( idx, start, end ) \
  Fori( idx_t, idx, start, end )

#define ReverseFor( idx, start, end ) \
  ReverseFori( idx_t, idx, start, end )

#define ForEach( elem, list ) \
  for( auto& elem : list )

#define Forever \
  for( ;; )


// WARNING: causes mismatched { }
#define FORLEN( elem, idx, list ) \
  ForLen( idx, list ) { \
    auto elem = ( list ).mem + ( idx ); \

// WARNING: causes mismatched { }
#define FORLEN32( elem, idx, list ) \
  ForLen32( idx, list ) { \
    auto elem = ( list ).mem + ( idx ); \

// WARNING: causes mismatched { }
#define REVERSEFORLEN( elem, idx, list ) \
  ReverseForLen( idx, list ) { \
    auto elem = ( list ).mem + ( idx ); \

// WARNING: causes mismatched { }
#define REVERSEFORLEN32( elem, idx, list ) \
  ReverseForLen32( idx, list ) { \
    auto elem = ( list ).mem + ( idx ); \


// ideally, we would add a T* val param, so you don't need to do: auto val = &elem->value;
// but, you can't declare two things of different types in a for-decl.
// so we're stuck with this for now.
#define ForList( elem, list ) \
  for( auto elem = ( list ).first;  elem;  elem = ( elem  ?  elem->next  :  0 ) )

// WARNING: causes mismatched { }
#define FORLIST( val, elem, list ) \
  ForList( elem, list ) { \
    auto val = &elem->value; \

#define ReverseForList( elem, list ) \
  for( auto elem = ( list ).last;  elem;  elem = ( elem  ?  elem->prev  :  0 ) )

// WARNING: causes mismatched { }
#define REVERSEFORLIST( val, elem, list ) \
  ReverseForList( elem, list ) { \
    auto val = &elem->value; \


#define ForNext( elem, firstelem ) \
  for( auto elem = firstelem;  elem;  elem = elem->next )

// WARNING: causes mismatched { }
#define FORNEXT( val, elem, elemfirst ) \
  ForNext( elem, elemfirst ) { \
    auto val = &elem->value; \



#define BEGIN_FORLISTALLPAIRS( elem0, elem1, list ) \
  FORLIST( elem0, __forlistallpairs_elem0, list ) \
    auto __forlistallpairs_elem1 = __forlistallpairs_elem0->next; \
    while( __forlistallpairs_elem1 ) { \
      auto elem1 = &__forlistallpairs_elem1->value; \

#define END_FORLISTALLPAIRS \
      __forlistallpairs_elem1 = __forlistallpairs_elem1->next; \
    } \
  } \


// assumes list0.len == list1.len
#define BEGIN_FORTWOLISTS( idx, elem0, list0, elem1, list1 ) \
  auto __fortwolists_listelem0 = ( list0 ).first; \
  auto __fortwolists_listelem1 = ( list1 ).first; \
  ForLen( idx, list0 ) { \
    auto elem0 = &__fortwolists_listelem0->value; \
    auto elem1 = &__fortwolists_listelem1->value; \

#define END_FORTWOLISTS \
    __fortwolists_listelem0 = __fortwolists_listelem0->next; \
    __fortwolists_listelem1 = __fortwolists_listelem1->next; \
  } \

#define BEGIN_FORTWOLISTS2( elem0, list0, elem1, list1 ) \
  auto __fortwolists2_listelem0 = ( list0 ).first; \
  auto __fortwolists2_listelem1 = ( list1 ).first; \
  ForLen( __fortwolists2_idx, list0 ) { \
    auto elem0 = &__fortwolists2_listelem0->value; \
    auto elem1 = &__fortwolists2_listelem1->value; \

#define END_FORTWOLISTS2 \
    __fortwolists2_listelem0 = __fortwolists2_listelem0->next; \
    __fortwolists2_listelem1 = __fortwolists2_listelem1->next; \
  } \


// ============================================================================
// MEMORY OPERATIONS

#if 1

  // not a macro, due to having an overload that takes lengths for src0 and src1.
  Inl bool
  MemEqual( void* src0, void* src1, idx_t nbytes )
  {
    return ( memcmp( src0, src1, nbytes ) == 0 );
  }

  // we can't have Memmove take args, since it can take macro args, which don't evaluate in proper order.
  #define Memmove                         memmove
  #define Memzero( dst, nbytes )          memset( dst, 0, nbytes )
  #define Typezero( ptr )                 memset( ptr, 0, sizeof( *ptr ) )
  #define Arrayzero( carray )             memset( carray, 0, sizeof( carray ) )


#else

  // PERF: try inlining some loops via chunking, to avoid the loop decr cost.
  Inl bool
  MemEqual( void* src0, void* src1, idx_t nbytes )
  {
    if( src0 == src1 ) {
      return 1;
    } else {
      idx_t* aReg = Cast( idx_t*, src0 );
      idx_t* bReg = Cast( idx_t*, src1 );
      idx_t nReg = nbytes / sizeof( idx_t );
      while( nReg-- ) {
        if( *aReg++ != *bReg++ ) {
          return 0;
        }
      }
      u8* a8 = Cast( u8*, aReg );
      u8* b8 = Cast( u8*, bReg );
      idx_t n8 = nbytes % sizeof( idx_t );
      while( n8-- ) {
        if( *a8++ != *b8++ ) {
          return 0;
        }
      }
      return 1;
    }
  }

  // PERF: try inlining some loops via chunking, to avoid the loop decr cost.
  Inl void
  _MemcpyL( void* dst, void* src, idx_t nbytes )
  {
    if( nbytes ) {
      idx_t* dstReg = Cast( idx_t*, dst );
      idx_t* srcReg = Cast( idx_t*, src );
      idx_t nReg = nbytes / sizeof( idx_t );
      while( nReg-- ) {
        *dstReg++ = *srcReg++;
      }
      u8* dst8 = Cast( u8*, dstReg );
      u8* src8 = Cast( u8*, srcReg );
      idx_t n8 = nbytes % sizeof( idx_t );
      while( n8-- ) {
        *dst8++ = *src8++;
      }
    }
  }

  // PERF: try inlining some loops via chunking, to avoid the loop decr cost.
  Inl void
  _MemcpyR( void* dst, void* src, idx_t nbytes )
  {
    if( nbytes ) {
      dst = Cast( u8*, dst ) + nbytes;
      src = Cast( u8*, src ) + nbytes;
      u8* dst8 = Cast( u8*, dst );
      u8* src8 = Cast( u8*, src );
      idx_t n8 = nbytes % sizeof( idx_t );
      while( n8-- ) {
        *--dst8 = *--src8;
      }
      idx_t* dstReg = Cast( idx_t*, dst8 );
      idx_t* srcReg = Cast( idx_t*, src8 );
      idx_t nReg = nbytes / sizeof( idx_t );
      while( nReg-- ) {
        *--dstReg = *--srcReg;
      }
    }
  }


  Inl void
  Memmove( void* dst, void* src, idx_t nbytes )
  {
    if( dst == src ) {
    } elif( ( src < dst )  &  ( Cast( u8*, src ) + nbytes > dst ) ) {
      _MemcpyR( dst, src, nbytes );
    } else {
      _MemcpyL( dst, src, nbytes );
    }
  }


  // PERF: try inlining some loops via chunking, to avoid the loop decr cost.
  Inl void
  Memzero( void* dst, idx_t nbytes )
  {
    if( nbytes ) {
      idx_t* dstReg = Cast( idx_t*, dst );
      idx_t zeroReg = 0;
      idx_t nReg = nbytes / sizeof( idx_t );
      while( nReg-- ) {
        *dstReg++ = zeroReg;
      }
      u8* dst8 = Cast( u8*, dstReg );
      u8 zero8 = 0;
      idx_t n8 = nbytes % sizeof( idx_t );
      while( n8-- ) {
        *dst8++ = zero8;
      }
    }
  }

#endif

Templ Inl void
TMove( T* dst, T* src, idx_t num_elements )
{
  Memmove( dst, src, num_elements * sizeof( T ) );
}

Templ Inl void
TZero( T* dst, idx_t num_elements )
{
  Memzero( dst, num_elements * sizeof( T ) );
}


Inl bool
MemEqual( void* src0, idx_t src0_len, void* src1, idx_t src1_len )
{
  if( src0_len == src1_len ) {
    return MemEqual( src0, src1, src1_len );
  } else {
    return 0;
  }
}


Inl bool
MemIsZero( void* src, idx_t nbytes )
{
  auto srcR = Cast( idx_t*, src );
  auto numR = nbytes / sizeof( idx_t );
  while( numR-- ) {
    if( *srcR++ ) {
      return 0;
    }
  }
  auto src8 = Cast( u8*, srcR );
  auto num8 = nbytes % sizeof( idx_t );
  while( num8-- ) {
    if( *src8++ ) {
      return 0;
    }
  }
  return 1;
}


Inl void
MemReverse( u8* src, idx_t src_len )
{
  For( i, 0, src_len / 2 ) {
    SWAP( u8, src[i], src[src_len - i - 1] );
  }
}
Inl void
MemReverse( u32* src, idx_t src_len )
{
  For( i, 0, src_len / 2 ) {
    SWAP( u32, src[i], src[src_len - i - 1] );
  }
}
Inl void
MemReverse( u64* src, idx_t src_len )
{
  For( i, 0, src_len / 2 ) {
    SWAP( u64, src[i], src[src_len - i - 1] );
  }
}
Templ Inl void
TReverse( T* src, idx_t src_len )
{
  For( i, 0, src_len / 2 ) {
    SWAP( T, src[i], src[src_len - i - 1] );
  }
}

Inl bool
MemScanIdx( idx_t* dst, void* src, idx_t src_len, void* key, idx_t elemsize )
{
  For( i, 0, src_len ) {
    bool equal = MemEqual( src, key, elemsize );
    if( equal ) {
      *dst = i;
      return 1;
    }
    src = Cast( u8*, src ) + elemsize;
  }
  return 0;
}

Inl bool
MemScanIdxRev( idx_t* dst, void* src, idx_t src_len, void* key, idx_t elemsize )
{
  void* iter;
  idx_t i = src_len;
  while( i ) {
    i -= 1;
    iter = Cast( u8*, src ) + i * elemsize;
    bool equal = MemEqual( iter, key, elemsize );
    if( equal ) {
      *dst = i;
      return 1;
    }
  }
  return 0;
}

Inl void*
MemScan( void* src, idx_t src_len, void* key, idx_t elemsize )
{
  if( src_len ) {
    idx_t idx;
    bool found = MemScanIdx( &idx, src, src_len, key, elemsize );
    if( found ) {
      void* res = Cast( u8*, src ) + idx * elemsize;
      return res;
    }
  }
  return 0;
}

Inl void*
MemScanRev( void* src, idx_t src_len, void* key, idx_t elemsize )
{
  if( src_len ) {
    idx_t idx;
    bool found = MemScanIdxRev( &idx, src, src_len, key, elemsize );
    if( found ) {
      void* res = Cast( u8*, src ) + idx * elemsize;
      return res;
    }
  }
  return 0;
}








// ============================================================================
// ASSERT MACROS

//        printf( "AssertWarn fired!\n" ); \
//        printf( "AssertCrash fired! Crashing the app.\n" ); \

#ifdef _DEBUG
  #define AssertWarn( break_if_false )   \
    do{ \
      if( !( break_if_false ) ) {  \
        __debugbreak(); \
      } \
    } while( 0 ) \

  #define AssertCrash( break_if_false )   \
    do{ \
      if( !( break_if_false ) ) {  \
        __debugbreak(); \
      } \
    } while( 0 ) \

#else

  NoInl void
  _WarningTriggered( char* break_if_false, char* file, u32 line, char* function );

  NoInl void
  _CrashTriggered( char* break_if_false, char* file, u32 line, char* function );

  #define AssertWarn( break_if_false )   \
    { \
      if( !( break_if_false ) ) {  \
        _WarningTriggered( #break_if_false, __FILE__, __LINE__, __FUNCTION__ ); \
      } \
    } \

  #define AssertCrash( break_if_false )   \
    { \
      if( !( break_if_false ) ) {  \
        _CrashTriggered( #break_if_false, __FILE__, __LINE__, __FUNCTION__ ); \
      } \
    } \

#endif

#define ImplementWarn()   AssertWarn( !"Implement!" );
#define ImplementCrash()     AssertCrash( !"Implement!" );

#define UnreachableWarn()     AssertWarn( !"Unreachable" );
#define UnreachableCrash()       AssertCrash( !"Unreachable" );












// ============================================================================
// INTEGER MATH

Inl s32
Clamp_s32_from_s64( s64 x )
{
  if( x > Cast( s64, MAX_s32 ) ) {
    x = MAX_s32;
  } elif( x < Cast( s64, MIN_s32 ) ) {
    x = MIN_s32;
  }
  return Cast( s32, x );
}


Inl u32
RoundDownToMultipleOf4( u32 x )
{
  u32 r = ( x & ~3 );
  return r;
}

Inl u32
RoundUpToMultipleOf4( u32 x )
{
  u32 r = ( ( x + 3 ) & ~3 );
  return r;
}


Inl idx_t
RoundUpToMultipleOf16( idx_t x )
{
  idx_t r = ( ( x + 15 ) & ~15 );
  return r;
}

Inl idx_t
RoundUpToMultipleOf32( idx_t x )
{
  idx_t r = ( ( x + 31 ) & ~31 );
  return r;
}

Inl idx_t
RoundUpToMultipleOfN( idx_t x, idx_t n )
{
  auto rem = x % n;
  if( rem ) {
    x += ( n - rem );
  }
  return x;
}


Inl idx_t
RoundDownToMultipleOfPowerOf2( idx_t x, idx_t pow2 )
{
  idx_t mask = pow2 - 1;
  AssertCrash( ( pow2 & mask ) == 0 );
  idx_t r = ( x & ~mask );
  return r;
}
Inl idx_t
RoundUpToMultipleOfPowerOf2( idx_t x, idx_t pow2 )
{
  idx_t mask = pow2 - 1;
  AssertCrash( ( pow2 & mask ) == 0 );
  idx_t r = ( ( x + mask ) & ~mask );
  return r;
}

Inl u32
RoundUpToNextPowerOf2( u32 x )
{
  auto r = Cast( u32, 1 ) << ( Cast( u32, 32 ) - _lzcnt_u32( x - 1 ) );
  return r;
}
Inl u64
RoundUpToNextPowerOf2( u64 x )
{
  auto r = Cast( u64, 1 ) << ( Cast( u64, 64 ) - _lzcnt_u64( x - 1 ) );
  return r;
}


Inl bool
IsPowerOf2( idx_t x )
{
  bool r = ( x & ( x - 1 ) ) == 0;
  return r;
}

#define AllOnes( num_ones ) \
  ( ( 1 << num_ones ) - 1 )

#define CheckBitIndex( _flags, _bit ) \
  ( ( _flags ) & ( 1 << ( _bit ) ) )

#define SetBitIndex( _flags, _bit ) \
  ( _flags ) |= ( _bit )


//
// equivalent of:
//     Cast( u8, Round_u32_from_f32( Cast( f32, a ) * Cast( f32, b ) / 255.0f ) )
// for a,b in [0, 256)
//
#if 1

  #define Mul255ByPercentage255( a, b ) \
    Cast( u8, ( ( 2 * Cast( u16, ( a ) ) * Cast( u16, ( b ) ) ) / 255 + 1 ) >> 1 )

#else

  #define Mul255ByPercentage255( a, b ) \
    ( g_mulpcttable[a][b] )

#endif



Inl u64
Pack( u32 hi, u32 lo )
{
  return ( Cast( u64, hi ) << 32ULL ) | Cast( u64, lo );
}




// ============================================================================
// FLOAT MATH

#define f32_PI            ( 3.1415926535897932385f )
#define f32_PI_REC        ( 0.3183098861837906715f )
#define f32_2PI           ( 6.2831853071795864769f )
#define f32_2PI_REC       ( 0.1591549430918953358f )
#define f32_4PI           ( 12.566370614359172954f )
#define f32_4PI_REC       ( 0.0795774715459476679f )
#define f32_E             ( 2.7182818284590452354f )
#define f32_E_REC         ( 0.3678794411714423216f )
#define f32_RT2_REC       ( 0.7071067811865475244f )
#define f32_RAD_PER_DEG   ( f32_PI / 180.0f )
#define f32_DEG_PER_RAD   ( 180.0f / f32_PI )


#define f64_PI            ( 3.1415926535897932385 )
#define f64_PI_REC        ( 0.3183098861837906715 )
#define f64_2PI           ( 6.2831853071795864769 )
#define f64_2PI_REC       ( 0.1591549430918953358 )
#define f64_4PI           ( 12.566370614359172954 )
#define f64_4PI_REC       ( 0.0795774715459476679 )
#define f64_E             ( 2.7182818284590452354 )
#define f64_E_REC         ( 0.3678794411714423216 )
#define f64_RT2_REC       ( 0.7071067811865475244 )
#define f64_RAD_PER_DEG   ( f64_PI / 180.0 )
#define f64_DEG_PER_RAD   ( 180.0 / f64_PI )


Inl f32
rsqrt( f32 r )
{
  f32 half = 0.5f * r;
  f32 result = r;
  s32 bits = *Cast( s32*, &result );
  bits = 0x5F375A86 - ( bits >> 1 );
  result = *Cast( f32*, &bits );
  result = result * ( 1.5f - half * result * result );
  result = result * ( 1.5f - half * result * result );
  return result;
}

Inl f64
rsqrt( f64 r )
{
  f64 half = 0.5 * r;
  f64 result = r;
  s64 bits = *Cast( s64*, &result );
  bits = 0x5FE6EB50C7B537A9L - ( bits >> 1 );
  result = *Cast( f64*, &bits );
  result = result * ( 1.5 - half * result * result );
  result = result * ( 1.5 - half * result * result );
  return result;
}


Inl f32 qsqrt32( f32 r ) { return 1.0f / rsqrt( r ); }
Inl f64 qsqrt64( f64 r ) { return 1.0  / rsqrt( r ); }


Inl f32 fastexp32( f32 r )
{
  auto bits = Cast( s32, 184 * r + 16249 ) << 16;
  auto result = *Cast( f32*, &bits );
  return result;
}

Inl f32 fastln32( f32 r )
{
  auto bits = *Cast( s32*, &r ) >> 16;
  auto result = Cast( f32, ( bits - 16249 ) ) / 184.0f;
  return result;
}

Inl f32 fastpow32( f32 x, f32 e )
{
  auto result = fastexp32( e * fastln32( x ) );
  return result;
}


// Note that we can't handcode cvttss2si for these! It converts to a signed int; we need extra logic to handle unsigned.
// Let the compiler generate these for us; the C++ spec requires what we need.
#define Truncate_s32_from_f32( x )   Cast( s32, x )
#define Truncate_s64_from_f64( x )   Cast( s64, x )
#define Truncate_s64_from_f32( x )   Cast( s64, x )
#define Truncate_u32_from_f32( x )   Cast( u32, x )
#define Truncate_u32_from_f64( x )   Cast( u32, x )
#define Truncate_u64_from_f32( x )   Cast( u64, x )
#define Truncate_u64_from_f64( x )   Cast( u64, x )
#define Truncate_u64_from_f32( x )   Cast( u64, x )

#if 0
  // these are still usable if we set the FP rounding mode correctly:
  //   check if these are faster than compiler-generated versions.
  #define Truncate_s32_from_f32( x )   _mm_cvttss_si32( _mm_set_ss( x ) )
  #define Truncate_s64_from_f64( x )   _mm_cvttsd_si64( _mm_set_sd( x ) )
  #define Truncate_s64_from_f32( x )   _mm_cvttss_si64( _mm_set_ss( x ) )

#endif

#if 0
  #define Round_s32_from_f32( x )   Truncate_s32_from_f32( x >= 0  ?  x + 0.5f  :  x - 0.5f )
  #define Round_s64_from_f32( x )   Truncate_s64_from_f32( x >= 0  ?  x + 0.5f  :  x - 0.5f )
  #define Round_s64_from_f64( x )   Truncate_s64_from_f64( x >= 0  ?  x + 0.5   :  x - 0.5  )
  #define Round_u32_from_f32( x )   Truncate_u32_from_f32( x >= 0  ?  x + 0.5f  :  x - 0.5f )
  #define Round_u32_from_f64( x )   Truncate_u32_from_f64( x >= 0  ?  x + 0.5   :  x - 0.5  )
  #define Round_u64_from_f32( x )   Truncate_u64_from_f32( x >= 0  ?  x + 0.5f  :  x - 0.5f )
  #define Round_u64_from_f64( x )   Truncate_u64_from_f64( x >= 0  ?  x + 0.5   :  x - 0.5  )
#endif

#define Round_s32_from_f32( x )   Truncate_s32_from_f32( Round32( x ) )
#define Round_s64_from_f32( x )   Truncate_s64_from_f32( Round32( x ) )
#define Round_s64_from_f64( x )   Truncate_s64_from_f64( Round64( x ) )
#define Round_u32_from_f32( x )   Truncate_u32_from_f32( Round32( x ) )
#define Round_u32_from_f64( x )   Truncate_u32_from_f64( Round64( x ) )
#define Round_u64_from_f32( x )   Truncate_u64_from_f32( Round32( x ) )
#define Round_u64_from_f64( x )   Truncate_u64_from_f64( Round64( x ) )


#define Sq( x )   ( ( x ) * ( x ) )
#define Sign( x )   ( ( ( x ) > 0 ) ? 1 : ( ( ( x ) < 0 ) ? -1 : 0 ) )

#define Floor32( x )   _mm_cvtss_f32( _mm_floor_ps( _mm_set_ss( x ) ) )
#define Floor64( x )   _mm_cvtsd_f64( _mm_floor_pd( _mm_set_sd( x ) ) )

#define Ceil32( x )   _mm_cvtss_f32( _mm_ceil_ps( _mm_set_ss( x ) ) )
#define Ceil64( x )   _mm_cvtsd_f64( _mm_ceil_pd( _mm_set_sd( x ) ) )

#define Truncate32( x )   _mm_cvtss_f32( _mm_round_ps( _mm_set_ss( x ), _MM_FROUND_TO_ZERO ) )
#define Truncate64( x )   _mm_cvtsd_f64( _mm_round_pd( _mm_set_sd( x ), _MM_FROUND_TO_ZERO ) )

#define Round32( x )   _mm_cvtss_f32( _mm_round_ps( _mm_set_ss( x ), _MM_FROUND_TO_NEAREST_INT ) )
#define Round64( x )   _mm_cvtsd_f64( _mm_round_pd( _mm_set_sd( x ), _MM_FROUND_TO_NEAREST_INT ) )

#define Sqrt32( x )   _mm_cvtss_f32( _mm_sqrt_ss( _mm_set_ss( x ) ) )
#define Sqrt64( x )   _mm_cvtsd_f64( _mm_sqrt_sd( _mm_set_sd( x ), _mm_set_sd( x ) ) )

#define Frac32( x )   ( x - Truncate32( x ) )
#define Frac64( x )   ( x - Truncate64( x ) )

#define Remainder32( x, d )   ( x - Truncate32( x / d ) * d )
#define Remainder64( x, d )   ( x - Truncate64( x / d ) * d )

#define Smoothstep32( x )   ( x * x * ( 3.0f - 2.0f * x ) )
#define Smoothstep64( x )   ( x * x * ( 3.0  - 2.0  * x ) )

#define SmoothstepDerivative64( x )   ( ( 6.0 * x ) * ( 1.0 - x ) )


// PERF: quicker versions of these math functions.

// i was tracing the assembly of the crt impls, to find how they compute this.
// the crt functions have function calls in them, which are terrifyingly slow.
//
// xmm0 = x
// xmm1 = 2.4801587301587298e-05
// xmm2 = 1 - x^2/2
// xmm3 = x^2

#define Cos32( x )   cosf( x )
#define Cos64( x )   cos( x )
Inl f32 Cos( f32 x ) { return Cos32( x ); }
Inl f64 Cos( f64 x ) { return Cos64( x ); }

#define Sin32( x )   sinf( x )
#define Sin64( x )   sin( x )
Inl f32 Sin( f32 x ) { return Sin32( x ); }
Inl f64 Sin( f64 x ) { return Sin64( x ); }

Inl f32 Tan32( f32 x ) { return tanf( x ); }
Inl f64 Tan64( f64 x ) { return tan( x ); }

Inl f32 Mod32( f32 x, f32 d ) { return fmodf( x, d ); }
Inl f64 Mod64( f64 x, f64 d ) { return fmod( x, d ); }

Inl f32 Pow32( f32 x, f32 e ) { return powf( x, e ); }
Inl f64 Pow64( f64 x, f64 e ) { return pow( x, e ); }

Inl f32 Exp32( f32 x ) { return expf( x ); }
Inl f64 Exp64( f64 x ) { return exp( x ); }

Inl f32 Ln32( f32 x ) { return logf( x ); }
Inl f64 Ln64( f64 x ) { return log( x ); }


Inl f32 lerp( f32 x0, f32 x1, f32 t ) { return t * x1 + ( 1.0f - t ) * x0; }
Inl f64 lerp( f64 x0, f64 x1, f64 t ) { return t * x1 + ( 1.0  - t ) * x0; }


Inl f64
Lerp_from_f64(
  f64 y0,
  f64 y1,
  f64 x,
  f64 x0,
  f64 x1 )
{
  return y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 );
}

Inl f32
Lerp_from_f32(
  f32 y0,
  f32 y1,
  f32 x,
  f32 x0,
  f32 x1 )
{
  return y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 );
}

Inl f32
Lerp_from_s32(
  f32 y0,
  f32 y1,
  s32 x,
  s32 x0,
  s32 x1 )
{
  return y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 );
}

Inl f64
Lerp_from_s32(
  f64 y0,
  f64 y1,
  s32 x,
  s32 x0,
  s32 x1 )
{
  return y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 );
}

Inl s32
Lerp_from_s32(
  s32 y0,
  s32 y1,
  s32 x,
  s32 x0,
  s32 x1 )
{
  return Round_s32_from_f32( y0 + ( ( y1 - y0 ) / Cast( f32, x1 - x0 ) ) * ( x - x0 ) );
}

Inl s32
Lerp_from_f32(
  s32 y0,
  s32 y1,
  f32 x,
  f32 x0,
  f32 x1 )
{
  return Round_s32_from_f32( y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 ) );
}

Inl u32
Lerp_from_f32(
  u32 y0,
  u32 y1,
  f32 x,
  f32 x0,
  f32 x1 )
{
  return Round_u32_from_f32( y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 ) );
}
Inl u64
Lerp_from_f32(
  u64 y0,
  u64 y1,
  f32 x,
  f32 x0,
  f32 x1 )
{
  return Round_u64_from_f32( y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 ) );
}


Inl bool
PtInInterval( f32 x, f32 x0, f32 x1, f32 epsilon )
{
  bool r = ( x0 - epsilon <= x )  &&  ( x <= x1 + epsilon );
  return r;
}

Inl bool
IntervalsOverlap( f32 x0, f32 x1, f32 y0, f32 y1, f32 epsilon )
{
  bool r =
    PtInInterval( x0, y0, y1, epsilon )  ||
    PtInInterval( x1, y0, y1, epsilon )  ||
    PtInInterval( y0, x0, x1, epsilon )  ||
    PtInInterval( y1, x0, x1, epsilon );
  return r;
}


struct
kahan32_t
{
  f32 sum;
  f32 err;
};

struct
kahan64_t
{
  f64 sum;
  f64 err;
};

Inl void
Add( kahan32_t& kahan, f32 val )
{
  f32 val_corrected = val - kahan.err;
  f32 new_sum = kahan.sum + val_corrected;
  kahan.err = ( new_sum - kahan.sum ) - val_corrected;
  kahan.sum = new_sum;
}

Inl void
Add( kahan64_t& kahan, f64 val )
{
  f64 val_corrected = val - kahan.err;
  f64 new_sum = kahan.sum + val_corrected;
  kahan.err = ( new_sum - kahan.sum ) - val_corrected;
  kahan.sum = new_sum;
}

Inl void
Sub( kahan32_t& kahan, f32 val )
{
  Add( kahan, -val );
}

Inl void
Sub( kahan64_t& kahan, f64 val )
{
  Add( kahan, -val );
}


struct
inc_stats_t
{
  kahan32_t mean;
  idx_t count;
};

Inl void
Init( inc_stats_t& s )
{
  s.mean = {};
  s.count = 0;
}

Inl void
AddValue( inc_stats_t& s, f32 value )
{
  s.count += 1;
  Add( s.mean, ( value - s.mean.sum ) / Cast( f32, s.count ) );
}

Inl void
AddMean( inc_stats_t& s, f32 mean, idx_t count )
{
  AssertCrash( count );
  auto new_count = s.count + count;
  s.mean.sum *= Cast( f32, s.count ) / Cast( f32, new_count );
  s.mean.err = 0;
  mean *= Cast( f32, count ) / Cast( f32, new_count );
  Add( s.mean, mean );
  s.count = new_count;
}





// ============================================================================
// BASIC STRINGS

#define IsSpaceTab( c )     ( ( ( c ) == ' '  ) || ( ( c ) == '\t' ) )
#define IsNewlineCh( c )    ( ( ( c ) == '\r' ) || ( ( c ) == '\n' ) )
#define IsWhitespace( c )   ( IsSpaceTab( c )  ||  IsNewlineCh( c ) )
#define IsNumber( c )       ( ( '0' <= ( c ) )  &&  ( ( c ) <= '9' ) )
#define IsLower( c )        ( ( 'a' <= ( c ) )  &&  ( ( c ) <= 'z' ) )
#define IsUpper( c )        ( ( 'A' <= ( c ) )  &&  ( ( c ) <= 'Z' ) )
#define IsAlpha( c )        ( IsLower( c )  ||  IsUpper( c ) )
#define InWord( c )         ( IsAlpha( c )  ||  IsNumber( c )  ||  ( c ) == '_' )
#define AsNumber( c )       ( ( c ) - '0' )


ForceInl idx_t
StringHash( void* key, idx_t key_len )
{
  sidx_t h = 5381;
  s8* s = Cast( s8*, key );
  sidx_t c;
  For( i, 0, key_len ) {
    c = *s++;
    h = c + h * ( h << 5 );
  }
  idx_t r = Cast( idx_t, h );
  return r;
}





// ============================================================================
// ALLOCATION ENTRY POINTS

// must be power of 2.
// 64 bytes is one cache line on most chips these days, so it's convenient to have that alignment.
// that's kind of large, but you shouldn't be alloc'ing tiny things anyways.
#define DEFAULT_ALIGN   64
#define DEFAULT_ALIGNMASK   ( DEFAULT_ALIGN - 1 )

#ifndef FINDLEAKS
#define FINDLEAKS 0
#endif

// moved these to main.h so we have access to hashset_t code and so forth.
// decl order is annoying.
Inl void*
MemHeapAllocBytes( idx_t nbytes );
Inl void*
MemHeapReallocBytes( void* mem, idx_t oldlen, idx_t newlen );
Inl void
MemHeapFree( void* mem );


#define MemHeapAlloc( type, num ) \
  Cast( type*, MemHeapAllocBytes( num * sizeof( type ) ) )

#define MemHeapRealloc( type, mem, oldnum, newnum ) \
  Cast( type*, MemHeapReallocBytes( mem, oldnum * sizeof( type ), newnum * sizeof( type ) ) )





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

Inl void*
MemVirtualReallocBytes( void* mem, idx_t oldlen, idx_t newlen )
{
  void* memnew = MemVirtualAllocBytes( newlen );
  Memmove( memnew, mem, MIN( oldlen, newlen ) );
  MemVirtualFree( mem );
  return memnew;
}


#define MemVirtualAlloc( type, num ) \
  Cast( type*, MemVirtualAllocBytes( num * sizeof( type ) ) )

#define MemVirtualRealloc( type, mem, oldnum, newnum ) \
  Cast( type*, MemVirtualReallocBytes( mem, oldnum * sizeof( type ), newnum * sizeof( type ) ) )


Inl void
MemzeroAligned16( void* mem, idx_t len )
{
  AssertCrash( !( Cast( idx_t, mem ) & 15 ) );

  auto mem256 = Cast( __m256i*, mem );
  auto copy256 = _mm256_set1_epi32( 0 );
  auto len256 = len / 32;
  For( i, 0, len256 ) {
    _mm256_stream_si256( mem256, copy256 );
    ++mem256;
  }
}












#if 0

  #define Defer( code ) \
    auto NAMEJOIN( deferred_, __LINE__ ) = _MakeDefer( [&]() { code } )

  Templ struct
  _defer_t
  {
    T lambda;

    ForceInl _defer_t( T t ) :
      lambda( t )
    {
    }

    ForceInl ~_defer_t()
    {
      lambda();
    }
  };

  Templ ForceInl _defer_t<T>
  _MakeDefer( T t )
  {
    return _defer_t<T>( t );
  }

#endif



#if 0

  Templ struct
  ExitScope
  {
    T lambda;
    ExitScope(T t) :
      lambda( t )
    {
    }

    ~ExitScope()
    {
      lambda();
    }

    ExitScope( const ExitScope& );

  private:
    ExitScope& operator=( const ExitScope& );
  };

  struct
  ExitScopeHelp
  {
    Templ ForceInl
    ExitScope<T> operator+(T t)
    {
      return t;
    }
  };

  #define Defer \
    const auto& NAMEJOIN( defer__, __LINE__ ) = ExitScopeHelp() + [&]()

#endif
