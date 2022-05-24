// build:console_x64_debug
// build:console_x64_optimized
// Copyright (c) John A. Carlos Jr., all rights reserved.

void
LogUI( void* cstr ... )
{
}

#define FINDLEAKS 1 // make it a test failure to leak memory.
#define DEBUGSLOW 0 // enable some slower debug checks.
#define WEAKINLINING 1
#include "common.h"
#include "math_vec.h"
#include "math_matrix.h"
#include "ds_slice.h"
#include "ds_string.h"
#include "ds_plist.h"
#include "ds_array.h"
#include "ds_embeddedarray.h"
#include "ds_fixedarray.h"
#include "ds_pagearray.h"
#include "ds_pagetree.h"
#include "ds_list.h"
#include "ds_bytearray.h"
#include "ds_hashset.h"
#include "ds_embeddedbitbuffer.h"
#include "rand.h"
#include "ds_minheap_extractable.h"
#include "ds_minheap_decreaseable.h"
#include "ds_queue.h"
#include "ds_fsalloc.h"
#include "cstr.h"
#include "filesys.h"
#include "timedate.h"
#include "threading.h"
#define LOGGER_ENABLED   1
#include "logger.h"
#define PROF_ENABLED   1
#define PROF_ENABLED_AT_LAUNCH   1
#include "profile.h"
#include "ds_graph.h"
#include "ds_btree.h"
#include "main.h"
#include "simplex.h"

#include "ds_idx_hashset.h"

#define OPENGL_INSTEAD_OF_SOFTWARE       1
#define GLW_RAWINPUT                     1
#define GLW_RAWINPUT_KEYBOARD            0
#define GLW_RAWINPUT_KEYBOARD_NOHOTKEY   1
#include "glw.h"

#include "ui_propdb.h"
#include "ui_font.h"
#include "ui_render.h"
#include "ui_buf2.h"
#include "ui_txt2.h"
#include "ui_cmd.h"
#include "ui_edit2.h"

#include <ehdata.h>

static void
TestCore()
{
  struct
  modtest_t
  {
    s64 a;
    u64 b;
    u64 r;
  };

  idx_t c = 0;
  c = 0;  Fori( u32, i, 0, 0 ) { ++c; }  AssertCrash( c == 0 );
  c = 0;  Fori( u32, i, 0, 10 ) { ++c; }  AssertCrash( c == 10 );
  c = 0;  ReverseFori( u32, i, 0, 0 ) { ++c; }  AssertCrash( c == 0 );
  c = 0;  ReverseFori( u32, i, 0, 10 ) { ++c; }  AssertCrash( c == 10 );
  c = 0;  Forinc( u32, i, 0, 0, 2 ) { ++c; }  AssertCrash( c == 0 );
  c = 0;  Forinc( u32, i, 0, 1, 2 ) { ++c; }  AssertCrash( c == 1 );
  c = 0;  Forinc( u32, i, 0, 2, 2 ) { ++c; }  AssertCrash( c == 1 );
  c = 0;  Forinc( u32, i, 0, 3, 2 ) { ++c; }  AssertCrash( c == 2 );
  c = 0;  Forinc( u32, i, 0, 10, 2 ) { ++c; }  AssertCrash( c == 5 );

  modtest_t tests[] = {
    { -22, 10, 8 },
    { -21, 10, 9 },
    { -20, 10, 0 },
    { -19, 10, 1 },
    { -18, 10, 2 },
    { -12, 10, 8 },
    { -11, 10, 9 },
    { -10, 10, 0 },
    { -9, 10, 1 },
    { -8, 10, 2 },
    { -2, 10, 8 },
    { -1, 10, 9 },
    { 0, 10, 0 },
    { 1, 10, 1 },
    { 2, 10, 2 },
    { 8, 10, 8 },
    { 9, 10, 9 },
    { 10, 10, 0 },
    { 11, 10, 1 },
    { 12, 10, 2 },
    { 18, 10, 8 },
    { 19, 10, 9 },
    { 20, 10, 0 },
    { 21, 10, 1 },
    { 22, 10, 2 },
    };
  ForEach( test, tests ) {
    auto r = AModB( test.a, test.b );
    AssertCrash( r == test.r );
  }
}

struct
test_bool_void_t
{
  bool b;
  void* p;
};

struct
test_hirschberg_t
{
  void* x;
  void* y;
  void* z;
  void* w;
  // TODO: cost params?
};

Inl s32
CostInsDel( u8 c )
{
  return -2;
}
Inl s32
CostRep( u8 a, u8 b )
{
  return a == b  ?  2  :  -1;
}

static void
TestString()
{
  idx_t indices[] = {
    10, 5, 7, 8, 1, 3, 5, 5000, 1221, 200, 0, 20,
  };
  u8 values[] = {
    0, 1, 2, 255, 128, 50, 254, 253, 3, 100, 200, 222,
  };
  AssertCrash( _countof( indices ) == _countof( values ) );
  idx_t fill_count = _countof( indices );

  string_t str;
  idx_t sizes[] = {
    1, 2, 3, 4, 5, 8, 10, 16, 24, 1024, 65536, 100000,
  };
  ForEach( size, sizes ) {
    Alloc( str, size );
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

  if( 0 ) { // TODO: crashing bug
    test_hirschberg_t tests[] = { { "AGTACGCA", "TATGC", "--TATGC-", "AGTACGCA" } };
    ForEach( test, tests ) {
      auto x = SliceFromCStr( test.x );
      auto y = SliceFromCStr( test.y );
      auto z_expected = SliceFromCStr( test.z );
      auto w_expected = SliceFromCStr( test.w );
      AssertCrash( MAX( x.len, y.len ) == w_expected.len );
      AssertCrash( z_expected.len == w_expected.len );
      string_t z;
      string_t w;
      tstring_t<int> score;
      tstring_t<int> buffer;
      tstring_t<int> buffer2;
      Alloc( z, z_expected.len );
      Alloc( w, z_expected.len );
      ZeroContents( z );
      ZeroContents( w );
      Alloc( score, 2 * ( y.len + 1 ) );
      Alloc( buffer, y.len + 1 );
      Alloc( buffer2, y.len + 1 );
      Hirschberg( x, y, Cast( u8, '-' ), SliceFromString( z ), SliceFromString( w ), score.mem, CostInsDel, CostInsDel, CostRep, buffer.mem, buffer2.mem );
      AssertCrash( EqualContents( SliceFromString( z ), z_expected ) );
      AssertCrash( EqualContents( SliceFromString( w ), w_expected ) );
      Free( z );
      Free( w );
      Free( score );
      Free( buffer );
      Free( buffer2 );
    }
  }
}

Inl void
TestMath()
{
  struct
  {
    s32 s;
    f32 f;
  }
  cases0[] = {
    { 0, 0.0f },
    { 0, 0.004f },
    { 0, 0.2f },
    { 0, 0.9f },
    { 0, 0.9999f },
    { 1, 1.0f },
    { 1, 1.4f },
    { 1, 1.9f },
    { -2, -2.0f },
    { -2, -2.054f },
    { -2, -2.54f },
    { -2, -2.94f },
    { 100, 100.9f },
    { -100, -100.9f },
    { -128, -128.0f },
    { 127, 127.0f },
    { 255, 255.0f },
    { -32768, -32768.0f },
    { 32767, 32767.0f },
    { 65536, 65536.0f },
    { -2147483520, -2147483520.0f },
    { 2147483520, 2147483520.0f },
    };
  ForEach( test, cases0 ) {
    auto s = Truncate_s32_from_f32( test.f );
    AssertCrash( s == test.s );
  }

  struct
  {
    u32 u;
    f32 f;
  }
  cases1[] = {
    { 0, 0.0f },
    { 0, 0.004f },
    { 0, 0.2f },
    { 0, 0.9f },
    { 0, 0.9999f },
    { 1, 1.0f },
    { 1, 1.4f },
    { 1, 1.9f },
    { 2, 2.0f },
    { 2, 2.054f },
    { 2, 2.54f },
    { 2, 2.94f },
    { 100, 100.9f },
    { 128, 128.0f },
    { 127, 127.0f },
    { 255, 255.0f },
    { 32768, 32768.0f },
    { 32767, 32767.0f },
    { 65536, 65536.0f },
    { 2147483520, 2147483520.0f },
    { 3000000000, 3000000000.0f },
    { 4294967040, 4294967040.0f },
    };
  ForEach( test, cases1 ) {
    auto u = Truncate_u32_from_f32( test.f );
    AssertCrash( u == test.u );
  }

  struct
  {
    u64 u;
    f64 f;
  }
  cases4[] = {
    { 0, 0.0 },
    { 0, 0.004 },
    { 0, 0.2 },
    { 0, 0.9 },
    { 0, 0.9999 },
    { 1, 1.0 },
    { 1, 1.4 },
    { 1, 1.9 },
    { 2, 2.0 },
    { 2, 2.054 },
    { 2, 2.54 },
    { 2, 2.94 },
    { 100, 100.9 },
    { 128, 128.0 },
    { 127, 127.0 },
    { 255, 255.0 },
    { 32768, 32768.0 },
    { 32767, 32767.0 },
    { 65536, 65536.0 },
    { 2147483520, 2147483520.0 },
    { 3000000000, 3000000000.0 },
    { 4294967040, 4294967040.0 },
    { 9223372036854775808, 9223372036854775800.0 },
    { 15000000000000000000, 15000000000000000000.0 },
    };
  ForEach( test, cases4 ) {
    auto u = Truncate_u64_from_f64( test.f );
    AssertCrash( u == test.u );
  }


//  Prof( costime );
//  auto d = Cos32( 0.234f );
//    { MAX_u64, Str( "18446744073709551615" ) },
//    { MIN_s64, Str( "-9223372036854775808" ) },
//    { MAX_s64, Str( "9223372036854775807" ) },
}



#if 0

Inl void
VerifyFoundDFS( tslice_t<n_t> found_nodes, n_t* expected, idx_t expected_len )
{
//  AssertCrash( found_nodes.len == expected_len );
//  AssertCrash( MemEqual( ML( found_nodes ), expected, expected_len
}

void A(u32 a, u32 i)
{
  printf( "A(%u, %u)\n", a, i );
}
void B(u32 a, u32 i)
{
  printf( "B(%u, %u)\n", a, i );
}
void Recurse(u32 a, u32 i)
{
  A(a, i);

  u32 count = 0;
  if( a == 0 ) count = 3;
  elif( a == 1 ) count = 1;

  u32 b = a + 1;
  Fori( u32, j, 0, count ) {
    Recurse(b, j);
  }

  B(a, i);
}


// TODO: the i and k are all wrong.
// at least the a is correct!
// and our termination condition is also correct!
// k is correct for A, but not for B.

struct pair_t { u32 a; u32 i; u32 k; u32 c; };
void A(pair_t* p)
{
  printf( "A(%u, %u, %u)\n", p->a, p->i, p->k );
}
void B(pair_t* p)
{
  printf( "B(%u, %u, %u)\n", p->a, p->i, p->k );
}
void Iterative()
{
#define SETC(x) \
  if( x.a == 0 ) x.c = 3; \
  elif( x.a == 1 ) x.c = 1; \
  else x.c = 0; \

  pair_t buffer[100];
  u32 idx = 0;
  pair_t a = { 0, 0, 0 };
  SETC( a );
  pair_t b;

Entry:
  A( &a );

  if( a.c ) {
    // Construct the first child:
    b = { a.a + 1, 0, 0 };
    SETC( b );

Loop:
    buffer[idx++] = a; // we'll restore to this on 'exit'
    a = b;
    goto Entry;

Exit:
    if( a.k + 1 < a.c ) {
      // Construct the next sibling:
      b = { a.a + 1, a.i + 1, a.k + 1 };
      SETC( b );
      a.k += 1;
      goto Loop;
    }

    goto LoopDone;
  }

LoopDone:
  B( &a );

  if( idx ) {
    idx -= 1;
    a = buffer[idx];
    goto Exit;
  }
}
static void TestXXXXXX()
{
  printf( "Recursive:\n" );
  Recurse(0, 0);

  printf( "Iterative:\n" );
  Iterative();
}
#endif


Inl f32
Eval(
  f32* coeffs,
  f32* vars,
  idx_t N
  )
{
  auto r = coeffs[N];
  For( i, 0, N ) {
    r += vars[i] * coeffs[i];
  }
  return r;
}


Inl f32
CalcAdot(
  f32* A,
  u32 N,
  f32 t
  )
{
  AssertCrash( N );
  kahan32_t sum = {};
  Fori( u32, i, 0, N - 1 ) {
    f32 term = ( i + 1 ) * A[ i + 1 ] * Pow32( t, Cast( f32, i ) );
    Add( sum, term );
  }
  return sum.sum;
}

Inl void
CalcAdot(
  f32* A,
  u32 N,
  f32* R
  )
{
  AssertCrash( N );
  Fori( u32, i, 0, N - 1 ) {
    R[i] = ( i + 1 ) * A[ i + 1 ];
  }
}

Inl f32
CalcA(
  f32* A,
  u32 N,
  f32 t
  )
{
  AssertCrash( N );
  kahan32_t sum = {};
  sum.sum = A[0];
  Fori( u32, i, 1, N ) {
    f32 term = A[i] * Pow32( t, Cast( f32, i ) );
    Add( sum, term );
  }
  return sum.sum;
}

Inl void
CalcAsquared(
  f32* A,
  u32 N,
  f32* R
  )
{
  AssertCrash( A != R );
  AssertCrash( N );
  Fori( u32, i, 0, N ) {
    R[i] = 0;
  }
  Fori( u32, i, 0, N ) {
    Fori( u32, j, 0, N ) {
      if( i + j >= N ) {
        break; // drop higher order terms
      }
      R[i+j] += A[i] * A[j];
    }
  }
}

Inl void
CalcAtimesB(
  f32* A,
  u32 N,
  f32* B,
  f32* R
  )
{
  AssertCrash( A != R );
  AssertCrash( B != R );
  AssertCrash( N );
  Fori( u32, i, 0, N ) {
    R[i] = 0;
  }
  Fori( u32, i, 0, N ) {
    Fori( u32, j, 0, N ) {
      if( i + j >= N ) {
        break; // drop higher order terms
      }
      R[i+j] += A[i] * B[j];
    }
  }
}

Inl void
CalcAofA(
  f32* A,
  u32 N,
  f32* R,
  f32* T0,
  f32* T1
  )
{
  AssertCrash( A != R );
  AssertCrash( N );

  // T0 is A^i, starting at i=1
  Fori( u32, i, 0, N ) {
    T0[i] = A[i];
  }

  R[0] = A[0]; // no multiplies necessary for this, see below loop starting at i=1
  Fori( u32, i, 1, N ) {
    R[i] = 0;
  }

  Fori( u32, i, 1, N ) {
    // compute A[i] * A^i, and add to R

    Fori( u32, j, 0, N ) {
      T1[j] = A[i] * T0[j];
    }

    // add A[i] * A^i to R
    Fori( u32, j, 0, N ) {
      R[j] += T1[j];
    }

    // update T0 to be A^i for the next i.
    CalcAtimesB( T0, N, A, T1 );
    Fori( u32, j, 0, N ) { // PERF: unroll outer loop once and swap T0/T1 for second "iter" unrolled ?
      T0[j] = T1[j];
    }
  }
}

// calculates the truncated polynomial c * A( A( t / c ) )
Inl void
CalcCtimesAofAofToverC(
  f32* A,
  u32 N,
  f32* R,
  f32* T0,
  f32* T1,
  f32 C
  )
{
  CalcAofA( A, N, R, T0, T1 );

  // note that we can do a change of variables for A( A( t ) ) -> A( A( t / c ) ), so we can easily do this after figuring out A( A( t ) )
  f32 recC = 1 / C;
  f32 recCtoI = 1 / C;
  Fori( u32, i, 1, N ) {
    R[i] *= recCtoI;
    recCtoI *= recC;
  }

  // now multiply by C
  Fori( u32, i, 0, N ) {
    R[i] *= C;
  }
}

Inl f32
CalcAofA(
  f32* A,
  u32 N,
  f32 t
  )
{
  return CalcA( A, N, CalcA( A, N, t ) );
}

Inl f32
CalcTotalErr(
  f32* t_tests,
  u32 t_tests_len,
  f32* A,
  u32 N
  )
{
  kahan32_t total_err = {};
  For( i, 0, t_tests_len ) {
    auto t = t_tests[i];
    auto AofA = CalcAofA( A, N, t );
    auto Adot = CalcAdot( A, N, t );
    auto err = Adot - AofA;
    Add( total_err, ABS( err ) );
  }
  return total_err.sum / t_tests_len;
}

Inl f32
CalcTotalErrRenormalization(
  f32* t_tests,
  u32 t_tests_len,
  f32* A,
  u32 N
  )
{
  kahan32_t total_err = {};
  For( i, 0, t_tests_len ) {
    auto t = t_tests[i];
    // we're storing alpha as A[ N - 1 ]
    auto alpha = A[ N - 1 ];
    auto FofXoverAlpha = CalcA( A, N - 1, t / alpha );
    auto F = CalcA( A, N - 1, t );
    auto FofFofXoverAlpha = CalcA( A, N - 1, FofXoverAlpha );
    auto err = FofFofXoverAlpha - F;
    Add( total_err, ABS( err ) );
  }
  return total_err.sum / t_tests_len;
}

Inl f32
CalcTotalErrRenormalization(
  f32* A,
  u32 N,
  f32* T0,
  f32* T1,
  f32* T2
  )
{
  AssertCrash( N >= 2 );

  auto alpha = A[N-1];
  CalcCtimesAofAofToverC( A, N-1, T0, T1, T2, alpha ); // T0 = alpha * A( A( t / alpha ) )

#if 0
  kahan32_t total_err = {};
  For( i, 0, N-1 ) {
    auto err = T0[i] - A[i];
    Add( total_err, ABS( err ) );
  }
  return total_err.sum;

#else
  CalcAdot( A, N-1, T1 ); // T1 = Adot( t ), which is N-2 in length
  kahan32_t total_err = {};
  For( i, 0, N-2 ) {
    auto err = T0[i] - T1[i];
    Add( total_err, ABS( err ) );
  }
  return total_err.sum;

#endif

}

Inl void
PrintArray(
  f32* A,
  u32 N
  )
{
  printf( "array: " );
  Fori( u32, i, 0, N ) {
    printf( "%f ", A[i] );
  }
  printf( "\n" );
}


Inl vec2<f32>
SpringEqns(
  f32 t,
  f32 a,
  f32 n,
  f32 A,
  f32 B
  )
{
  // x(t) = a exp( -n ) t^n exp( -A t ) exp( +-sqrt( n + t^2 ( A^2 - B ) ) )
  // x(t) = a t^n exp( -n - A t +- sqrt( n + t^2 ( A^2 - B ) ) )
  // x(t) = a exp( b t ) t^n, with b t = -n - A t +- sqrt( n + t^2 ( A^2 - B ) )
  // x'(t) = a exp( b t ) [ n t^( n - 1 ) + b t^n ]
  // x'(t) = a exp( b t ) t^n [ n / t + b ]
  // x'(t) = x(t) [ n / t + b ]
  // x'(t) = x(t) [ n + b t ] / t
  // x''(t) = a exp( b t ) [ n ( n - 1 ) t^( n - 2 ) + 2 b n t^( n - 1 ) + b^2 t^n ]
  // x''(t) = a exp( b t ) t^n [ n ( n - 1 ) / t^2 + 2 b n / t + b^2 ]
  // x''(t) = x(t) [ n ( n - 1 ) / t^2 + 2 b n / t + b^2 ]
  // x''(t) = x(t) [ n ( n - 1 ) / t^2 + 2 b t n / t^2 + ( b t )^2 / t^2 ]
  // x''(t) = x(t) [ n ( n - 1 ) + 2 b t n + ( b t )^2 ] / t^2
  auto disc = n + t * t * ( A * A - B );
  AssertCrash( disc >= 0 );
  auto sqroot = Sqrt32( disc );
  auto leadfac = a * Pow32( t, n );
  auto leadexp = -n - A * t;
  auto bt0 = leadexp + sqroot;
  auto bt1 = leadexp - sqroot;
  auto x0 = leadfac * Exp32( bt0 );
  auto x1 = leadfac * Exp32( bt1 );
  auto xp0 = x0 * ( n + bt0 ) / t;
  auto xp1 = x1 * ( n + bt1 ) / t;
  auto xpp0 = x0 * ( n * ( n - 1 ) + 2 * n * bt0 + bt0 * bt0 ) / ( t * t );
  auto xpp1 = x1 * ( n * ( n - 1 ) + 2 * n * bt1 + bt1 * bt1 ) / ( t * t );

  auto soln0 = xpp0 + 2 * A * xp0 + B * x0;
  auto soln1 = xpp1 + 2 * A * xp1 + B * x1;
//  printf( "%f, %f\n", soln0, soln1 );

  return _vec2( soln0, soln1 );
}

Inl void
CalcJunk()
{
  For( i, 1, 10000 ) {
    constant f32 mass = 1.0f;
    constant f32 spring_k = 1000.0f;
    static f32 friction_k = 2.2f * Sqrt32( mass * spring_k ); // 2 * Sqrt( m * k ) is critical damping, but we want a little overdamping.

    auto t = Cast( f32, i );
    f32 a = 1.0f;
    f32 n = 0.0f;
    f32 B = -spring_k;
    f32 A = 0.5f * friction_k;
    auto r = SpringEqns( t, a, n, A, B );
  }

  {
    constant u32 n = 1000000;
    array_t<slice_t> lines;
    Alloc( lines, n + 1 );
    kahan32_t x = {};
    constant idx_t m = 100;
    For( iter, 0, m ) {
      lines.len = 0;
      auto lines_mem = AddBack( lines, n );
      FORLEN( line, i, lines )
        line->mem = Cast( u8*, lines_mem + i );
        line->len = i + 1;
      }
      auto t0 = TimeTSC();
//      *AddAt( lines, 0 ) = { 0, 0 };
      *AddBack( lines ) = { 0, 0 };
      auto t1 = TimeTSC();
      auto dt = TimeSecFromTSC32( t1 - t0 );
      Add( x, dt );
    }
    x.sum /= m;
    Free( lines );
  }

  // for 100K slice_t's:
  // addback and write of a slice_t is ~70 nanoseconds.
  // what about addat(0) ?
  // that's ~50 microseconds.
  // still not too bad.

  // for 1M slice_t's:
  // addat(0) is ~1.3 milliseconds. pretty terrible.
  // addback is ~20 microseconds.




  {
    constant u32 n = 513;
    u32 x[n];
    u32 y[n];
    Fori( u32, i, 0, n ) {
      x[i] = i;
      y[i] = x[i] & ( x[i] - 1 );
      y[i] = __popcnt( i );
      y[i] = __lzcnt( i );
      y[i] = 32 - __lzcnt( i );
      x[i] = 1u << ( 32u - __lzcnt( i ) );
      x[i] = 1u << ( 32u - __lzcnt( i - 1 ) );
      y[i] = Cast( u32, -1 ) >> __lzcnt( i );
      y[i] = 1u + ( Cast( u32, -1 ) >> __lzcnt( i - 1 ) );
      auto t = i - 1;
      y[i] = ( t | (t >> 1) | (t >> 2) | (t >> 4) | (t >> 8) | (t >> 16) ) + 1;
      y[i] = 1u << ( 32u - __lzcnt( i >> 1 ) );
    }
    x[0] = 0;
  }

  // well we know the renormalization functional equation does have a convergent alpha,
  // f( t ) = alpha * f( f( t / alpha ) )
  // so maybe we should try that as a test of our solver

  // well it looks like our solver for renormalization converges to f=0, which isn't all that helpful.
  // we need to add constraints to avoid the null soln.

  // unfortunately even adding constraints isn't quite enough; we just pick the closest thing to the edge of the constraints.
  // maybe we have multiple local minima, but our solver is always finding the global minimum.
  // finding the global min is kind of the point of the metropolis algorithm; perhaps we need a different solver?
  // we could cluster/sort successful transitions or top results, to get an idea of good values in the space.
  //
  // we could do a gradient descent algorithm by looking a tiny step forward in each A[i]+tinystep, computing
  // the difference in error for each i, and then committing to a direction proportional to the difference in error.
  // or, pick a random i, and do the tiny step forward/backwards in the direction of the difference in error.
  //
  // is there some way we can visualize the error as a function of A[i]s?
  // if we pick one i to look at, then yeah it's trivial.

  // visualizing total_err vs. A[2] and A[3] for starting A = { 1, 0, -1.366, -2.732 }, it looks like:
  // A[2] has one min at 0
  // A[3] has one min at -0.1875
  // so our metric doesn't optimize for what we want.
  // it finds the null soln, but that's not helpful.

  // maybe if we change our metric to only sum the coefficient errors, effectively ignoring t?
  // that would avoid "t close to 0" having such an influence; maybe enough to expose other solns?
  // doing this effectively requires a polynomial evaluator system, so we can compute coefficients of a(a(t)) and f(f(t/alpha))
  //
  // maybe we can just divide terms in our functional eqn by t, to achieve the same effect?
  // nope that didn't work. looks like we'll need that polynomial system to match coefficients...

  // matching coefficients appears to converge to multiple solutions, which is good news.
  // if we keep track of the best 10% or something, we should be able to cluster/sort.

  // it appears that people solve for feigenbaum's alpha via picking some t values, constructing f = g - alpha g( g( t / alpha ) ) = 0
  // for each t, which forms a system of equations in coefficients, computing gradients of f at each t value,
  // constructing a jacobian, and then using newton's method to find the root.
  // subsequent iteration of that whole process apparently converges on the right answer.

  {
    f32 F[4] = { 1, 1, 1, 1 };
    f32 G[4] = { 2, 3, 4, 5 };
    f32 R[4];
    f32 T0[4];
    f32 T1[4];
    CalcAsquared( F, _countof( F ), R );
    CalcAtimesB( F, _countof( F ), G, R );

    CalcAofA( F, _countof( F ), R, T0, T1 );

    f32 alpha = -2.5f;
    CalcCtimesAofAofToverC( F, _countof( F ), R, T0, T1, alpha );
    CalcAdot( F, _countof( F ), T0 );

    // f( t ) = alpha * f( f( t / alpha ) )
    PrintArray( R, _countof( R ) );
  }



  if( 0 )
  {
  //  f32 t_tests[] = { -0.5f, -0.4f, -0.3f, -0.2f, -0.1f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };
    f32 t_tests[16] = {  };
    f32 t_tests_bounds = 0.2f;
    auto bounds_loop_len = _countof( t_tests ) / 2;
    For( i, 0, bounds_loop_len ) {
      t_tests[ 2 * i + 0 ] = +Cast( f32, i + 1 ) * t_tests_bounds / bounds_loop_len;
      t_tests[ 2 * i + 1 ] = -Cast( f32, i + 1 ) * t_tests_bounds / bounds_loop_len;
    }

    constant u32 N = 12;
  //  f32 A[N] = { 1, 1.05412, -0.671615, -0.328385 };
  //  f32 A[N] = { 1, 1, 1, 1 };
    f32 A[N];
    Fori( u32, i, 0, N ) {
      A[i] = 1;
    }
//    A[2] = -1.366f;
//    A[3] = -2.732f;

//    A[1] = 0;
//    A[3] = 0;
//    A[5] = 0;

    f32 T0[N];
    f32 T1[N];
    f32 T2[N];

  //  f32 total_err = CalcTotalErr( t_tests, _countof( t_tests ), A, N );
//    f32 total_err = CalcTotalErrRenormalization( t_tests, _countof( t_tests ), A, N );
    f32 total_err = CalcTotalErrRenormalization( A, N, T0, T1, T2 );

    f32 Abest[N] = {};

    rng_xorshift32_t rng;
  //  Init( rng, 0x1234567812345678ULL );
    Init( rng, TimeTSC() );

    u32 num_transitions = 0;

    f32 best_total_err = total_err;
//    For( u, 0, 100000000 ) {
    u64 loops = 0;
    while( best_total_err > 1.0f || loops < 100000000 ) {
      loops += 1;

      if( loops % 10000000 == 0 ) {
        printf( "loops: %llu M\n", loops / 1000000 );
      }

      // leave A[0]=1 unchanged, we're not fitting that param.
      auto idx = 1 + Rand32( rng ) % ( N - 1 );
//      auto idx = 2 + Rand32( rng ) % ( N - 2 );
//      while( ( idx != N - 1 ) && ( idx % 2 == 1 ) ) {
//        idx = 2 + Rand32( rng ) % ( N - 2 );
//      }
      AssertCrash( idx < N );
      auto old = A[idx];
      A[idx] = 10 * ( 2 * Zeta32( rng ) - 1 );

  //    f32 potential_total_err = CalcTotalErr( t_tests, _countof( t_tests ), A, N );
//      f32 potential_total_err = CalcTotalErrRenormalization( t_tests, _countof( t_tests ), A, N );
      f32 potential_total_err = CalcTotalErrRenormalization( A, N, T0, T1, T2 );

      f32 prob_switch = CLAMP( total_err / potential_total_err, 0, 1 );
      if( Zeta32( rng ) < prob_switch ) {
        num_transitions += 1;
        total_err = potential_total_err;
        if( total_err < best_total_err ) {
          Fori( u32, i, 0, N ) {
            Abest[i] = A[i];
          }
          best_total_err = total_err;
          printf( "err: %f   ", total_err );
          PrintArray( Abest, N );
        }
      } else {
        A[idx] = old;
      }
    }

    printf( "best_total_err: %f\n", best_total_err );
    printf( "num_transitions: %u\n", num_transitions );
    PrintArray( Abest, N-1 );
    printf( "alpha: %f\n", Abest[N-1] );
  }

// random lattice walks in a [ -10, 10 ] ^ N cube.

// 10M iterations, N=4
// best_total_err: 0.450952
// num_transitions: 5550948
// A: 1.000000 1.132835 -0.561097 -0.436260

// 10M iterations, N=5
// best_total_err: 0.721790
// num_transitions: 5401105
// A: 1.000000 1.563395 -0.412851 -0.881557 0.300946

// 10M iterations, N=10
// best_total_err: 1.548208
// num_transitions: 6117062
// A: 1.000000 0.097001 -0.154874 0.636585 -0.187020 -2.952207 3.018682 1.502590 -5.194418 2.303742



//best_total_err: 0.015664
//num_transitions: 62495683
//array: 1.000000 0.875630 -0.363438 -0.324348
//alpha: 0.745621

//err: 0.010398   array: 1.000000 0.998591 -1.897126 -0.335768 -4.259650
//err: 0.016358   array: 1.000000 -1.012293 0.257753 -0.006233 -4.222347
//err: 0.268438   array: 1.000000 1.240661 -2.380484 -0.416772 0.232913 -4.032364
//err: 0.147309   array: 1.000000 1.208125 -2.255611 -0.282461 0.118561 -5.492319
//err: 0.167881   array: 1.000000 -1.119583 0.286368 0.011928 -0.010871 -6.784761
//err: 0.074995   array: 1.000000 -0.987400 -3.028404 0.348996 3.029171 -2.681639
}


Inl void
ExeJunk()
{
  auto name = Str( "C:/doc/dev/cpp/proj/main/exe/nocrt.exe" );
  auto name_len = CsLen( name );
  auto file = FileOpen( name, name_len, fileopen_t::only_existing, fileop_t::R, fileop_t::R );
  auto contents = FileAlloc( file );
  auto hdr = Cast( _IMAGE_DOS_HEADER*, contents.mem );

  array_t<u8> exe;
  Alloc( exe, 1024*1024 );

  IMAGE_DOS_HEADER h = {};
  h.e_magic = 0x5a4d;
  h.e_cblp = 0x0090;
  h.e_cp = 0x0003;
  h.e_cparhdr = 0x0004;
  h.e_maxalloc = 0xffff;
  h.e_sp = 0x00b8;
  h.e_lfarlc = 0x0040;
  h.e_lfanew = 0x000000b0;
  Memmove( AddBack( exe, sizeof( h ) ), &h, sizeof( h ) );

  u8 dos_program[112] = {
//    0x0e,0x1f,0xba,0x0e,0x00,0xb4,0x09,0xcd,0x21,0xb8,0x01,0x4c,0xcd,0x21,0x54,0x68,0x69,0x73,0x20,0x70,
//    0x72,0x6f,0x67,0x72,0x61,0x6d,0x20,0x63,0x61,0x6e,0x6e,0x6f,0x74,0x20,0x62,0x65,0x20,0x72,0x75,0x6e,
//    0x20,0x69,0x6e,0x20,0x44,0x4f,0x53,0x20,0x6d,0x6f,0x64,0x65,0x2e,0x0d,0x0d,0x0a,0x24,0x00,0x00,0x00,
//    0x00,0x00,0x00,0x00,0xcc,0xab,0xf0,0xd5,0x88,0xca,0x9e,0x86,0x88,0xca,0x9e,0x86,0x88,0xca,0x9e,0x86,
//    0xd3,0xa2,0x9a,0x87,0x83,0xca,0x9e,0x86,0xd3,0xa2,0x9d,0x87,0x81,0xca,0x9e,0x86,0xd3,0xa2,0x9b,0x87,
//    0x21,0xca,0x9e,0x86,0x5d,0xa7,0x9b,0x87,0xa7,0xca,0x9e,0x86,0x5d,0xa7,0x9a,0x87,0x99,0xca,0x9e,0x86,
//    0x5d,0xa7,0x9d,0x87,0x80,0xca,0x9e,0x86,0x81,0xb2,0x0d,0x86,0x8a,0xca,0x9e,0x86,0xd3,0xa2,0x9f,0x87,
//    0x85,0xca,0x9e,0x86,0x88,0xca,0x9f,0x86,0x5d,0xca,0x9e,0x86,0x13,0xa4,0x9b,0x87,0x89,0xca,0x9e,0x86,
//    0x13,0xa4,0x9c,0x87,0x89,0xca,0x9e,0x86,0x52,0x69,0x63,0x68,0x88,0xca,0x9e,0x86,0x00,0x00,0x00,0x00,
//    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    0x0e,0x1f,0xba,0x0e,0x00,0xb4,0x09,0xcd,0x21,0xb8,0x01,0x4c,0xcd,0x21,0x54,0x68,0x69,0x73,0x20,0x70,
    0x72,0x6f,0x67,0x72,0x61,0x6d,0x20,0x63,0x61,0x6e,0x6e,0x6f,0x74,0x20,0x62,0x65,0x20,0x72,0x75,0x6e,
    0x20,0x69,0x6e,0x20,0x44,0x4f,0x53,0x20,0x6d,0x6f,0x64,0x65,0x2e,0x0d,0x0d,0x0a,0x24,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0xc9,0x8f,0x0a,0xdf,0x8d,0xee,0x64,0x8c,0x8d,0xee,0x64,0x8c,0x8d,0xee,0x64,0x8c,
    0x16,0x80,0x6c,0x8d,0x8c,0xee,0x64,0x8c,0x16,0x80,0x66,0x8d,0x8c,0xee,0x64,0x8c,0x52,0x69,0x63,0x68,
    0x8d,0xee,0x64,0x8c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };
  Memmove( AddBack( exe, _countof( dos_program ) ), AL( dos_program ) );

  u8 nt_signature[4] = {
    0x50,0x45,0x00,0x00 // PE00
  };
  Memmove( AddBack( exe, _countof( nt_signature ) ), AL( nt_signature ) );

  IMAGE_FILE_HEADER fh = {};
  fh.Machine = 0x8664;
  fh.NumberOfSections = 0x0004;
  fh.TimeDateStamp = 0x5ecf6a0e;
  fh.PointerToSymbolTable = 0x00000000;
  fh.NumberOfSymbols = 0x00000000;
  fh.SizeOfOptionalHeader = 0x00f0;
  fh.Characteristics = 0x0022;
  Memmove( AddBack( exe, sizeof( fh ) ), &fh, sizeof( fh ) );

  IMAGE_OPTIONAL_HEADER oh = {};
  oh.Magic = 0x020b;
  oh.MajorLinkerVersion = 0x0e;
  oh.MinorLinkerVersion = 0x18;
  oh.SizeOfCode = 0x00000200;
  oh.SizeOfInitializedData = 0x00000600;
  oh.SizeOfUninitializedData = 0x00000000;
  oh.AddressOfEntryPoint = 0x00001000;
  oh.BaseOfCode = 0x00001000;
  oh.ImageBase = 0x0000000140000000;
  oh.SectionAlignment = 0x00001000;
  oh.FileAlignment = 0x00000200;
  oh.MajorOperatingSystemVersion = 0x0005;
  oh.MinorOperatingSystemVersion = 0x0002;
  oh.MajorImageVersion = 0x0000;
  oh.MinorImageVersion = 0x0000;
  oh.MajorSubsystemVersion = 0x0005;
  oh.MinorSubsystemVersion = 0x0002;
  oh.Win32VersionValue = 0x00000000;
  oh.SizeOfImage = 0x00005000;
  oh.SizeOfHeaders = 0x00000400;
  oh.CheckSum = 0x00000000;
  oh.Subsystem = 0x0003;
  oh.DllCharacteristics = 0x8160;
  oh.SizeOfStackReserve = 0x0000000000100000;
  oh.SizeOfStackCommit = 0x0000000000001000;
  oh.SizeOfHeapReserve = 0x0000000000100000;
  oh.SizeOfHeapCommit = 0x0000000000001000;
  oh.LoaderFlags = 0x00000000;
  oh.NumberOfRvaAndSizes = 0x00000010;
  oh.DataDirectory[0x00000000] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // export
  oh.DataDirectory[0x00000001] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // import
  oh.DataDirectory[0x00000002] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // resource
  oh.DataDirectory[0x00000003] = { /*VirtualAddress=*/0x00004000, /*Size=*/0x0000000c }; // exception
  oh.DataDirectory[0x00000004] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // security
  oh.DataDirectory[0x00000005] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // base relocation table
  oh.DataDirectory[0x00000006] = { /*VirtualAddress=*/0x00002010, /*Size=*/0x00000054 }; // debug
  oh.DataDirectory[0x00000007] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // description string
  oh.DataDirectory[0x00000008] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // RVA of global pointer
  oh.DataDirectory[0x00000009] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // TLS
  oh.DataDirectory[0x0000000a] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // load configuration
  oh.DataDirectory[0x0000000b] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // bound import
  oh.DataDirectory[0x0000000c] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // import address table
  oh.DataDirectory[0x0000000d] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // delay load import
  oh.DataDirectory[0x0000000e] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // com runtime
  oh.DataDirectory[0x0000000f] = { /*VirtualAddress=*/0x00000000, /*Size=*/0x00000000 }; // reserved
  Memmove( AddBack( exe, sizeof( oh ) ), &oh, sizeof( oh ) );

  // read/execute
  IMAGE_SECTION_HEADER sh_text = {};
  Memmove( sh_text.Name, ".text", 5 );
  sh_text.Misc.VirtualSize = 0x00000090;
  sh_text.VirtualAddress = 0x00001000;
  sh_text.SizeOfRawData = 0x00000200;
  sh_text.PointerToRawData = 0x00000400;
  sh_text.PointerToRelocations = 0x00000000;
  sh_text.PointerToLinenumbers = 0x00000000;
  sh_text.NumberOfRelocations = 0x0000;
  sh_text.NumberOfLinenumbers = 0x0000;
  sh_text.Characteristics = 0x60000020;
  Memmove( AddBack( exe, sizeof( sh_text ) ), &sh_text, sizeof( sh_text ) );

  // read-only initialized data
  IMAGE_SECTION_HEADER sh_rdata = {};
  Memmove( sh_rdata.Name, ".rdata", 6 );
  sh_rdata.Misc.VirtualSize = 0x00000144;
  sh_rdata.VirtualAddress = 0x00002000;
  sh_rdata.SizeOfRawData = 0x00000200;
  sh_rdata.PointerToRawData = 0x00000600;
  sh_rdata.PointerToRelocations = 0x00000000;
  sh_rdata.PointerToLinenumbers = 0x00000000;
  sh_rdata.NumberOfRelocations = 0x0000;
  sh_rdata.NumberOfLinenumbers = 0x0000;
  sh_rdata.Characteristics = 0x40000040;
  Memmove( AddBack( exe, sizeof( sh_rdata ) ), &sh_rdata, sizeof( sh_rdata ) );

  // read/write initialized data
  IMAGE_SECTION_HEADER sh_data = {};
  Memmove( sh_data.Name, ".data", 5 );
  sh_data.Misc.VirtualSize = 0x00000012;
  sh_data.VirtualAddress = 0x00003000;
  sh_data.SizeOfRawData = 0x00000200;
  sh_data.PointerToRawData = 0x00000800;
  sh_data.PointerToRelocations = 0x00000000;
  sh_data.PointerToLinenumbers = 0x00000000;
  sh_data.NumberOfRelocations = 0x0000;
  sh_data.NumberOfLinenumbers = 0x0000;
  sh_data.Characteristics = 0xc0000040;
  Memmove( AddBack( exe, sizeof( sh_data ) ), &sh_data, sizeof( sh_data ) );

  IMAGE_SECTION_HEADER sh_pdata = {};
  Memmove( sh_pdata.Name, ".pdata", 6 );
  sh_pdata.Misc.VirtualSize = 0x0000000c;
  sh_pdata.VirtualAddress = 0x00004000;
  sh_pdata.SizeOfRawData = 0x00000200;
  sh_pdata.PointerToRawData = 0x00000a00;
  sh_pdata.PointerToRelocations = 0x00000000;
  sh_pdata.PointerToLinenumbers = 0x00000000;
  sh_pdata.NumberOfRelocations = 0x0000;
  sh_pdata.NumberOfLinenumbers = 0x0000;
  sh_pdata.Characteristics = 0x40000040;
  Memmove( AddBack( exe, sizeof( sh_pdata ) ), &sh_pdata, sizeof( sh_pdata ) );

  auto pdata = contents.mem + sh_pdata.PointerToRawData;
  auto pdata_len = sh_pdata.Misc.VirtualSize / sizeof( RUNTIME_FUNCTION );

  For( i, 0, pdata_len ) {
    auto rf = Cast( RUNTIME_FUNCTION*, pdata ) + i;
    AssertCrash(
      sh_rdata.VirtualAddress <= rf->UnwindInfoAddress  &&
      rf->UnwindInfoAddress < sh_rdata.VirtualAddress + sh_rdata.Misc.VirtualSize
      );
    auto file_ptr_unwindinfo = sh_rdata.PointerToRawData + rf->UnwindInfoAddress - sh_rdata.VirtualAddress;
    auto ui = Cast( UNWIND_INFO*, contents.mem + file_ptr_unwindinfo );
    rf = rf;
  }

  For( i, 0, exe.len ) {
    AssertCrash( exe.mem[i] == contents.mem[i] );
  }

  Free( exe );
  Free( contents );
  FileFree( file );
}



// IDEA: pagetree, where each node represents a character.
// if we store a unique id per page, then we can uniquely identify any string.
// pagetrees are sparse, so we're not paying huge memory costs.
// i suspect this will be slower than string hashtables, but may as well see.

struct
stringid_page_t
{
  // [0] stores this page's id.
  // we don't support 0s within strings.
  // 0 in [x] where x!=0 means the page isn't initialized.
  stringid_page_t* child_pages[256];
};
struct
stringid_system_t
{
  stringid_page_t root;
  plist_t* mem;
  idx_t id_generator;
  idx_t num_pages;
};
Inl void
Init( stringid_system_t* sys, plist_t* mem )
{
  // Implicitly sets root's id to 0
  Typezero( &sys->root );
  sys->mem = mem;
  sys->id_generator = 1;
  sys->num_pages = 1;
}
Inl idx_t
Uniquify( stringid_system_t* sys, slice_t str )
{
  stringid_page_t* page = &sys->root;
  ForLen( i, str ) {
    auto c = str.mem[i];
    AssertCrash( c );
    auto next_page = page->child_pages[c];
    if( !next_page ) {
      next_page = AddPlist( *sys->mem, stringid_page_t, _SIZEOF_IDX_T, 1 );
      page->child_pages[c] = next_page;
      Typezero( next_page );
      next_page->child_pages[0] = Cast( stringid_page_t*, sys->id_generator );
      sys->id_generator += 1;
      sys->num_pages += 1;
    }
    page = next_page;
  }
  return Cast( idx_t, page->child_pages[0] );
}

void
TestStringid()
{
  stringid_system_t sys;
  plist_t mem;
  Init( mem, 64000 );
  Init( &sys, &mem );

  slice_t tests[] = {
    SliceFromCStr( "" ),
    SliceFromCStr( "a" ),
    SliceFromCStr( "b" ),
    SliceFromCStr( "ab" ),
    SliceFromCStr( "abc" ),
    SliceFromCStr( "foobar" ),
    SliceFromCStr( "----------" ),
    };

  ForEach( test, tests ) {
    auto id = Uniquify( &sys, test );
    (void)id;
  }

  Kill( mem );
}




#if 0

struct
radixtree_edge_t
{
  slice_t text;
};
struct
radixtree_t
{
  plist_t* mem;
  n_t root;
  n_t N;
  n_t capacity_N;
  e_t E;
  e_t capacity_E; // TODO: array isn't ideal for per-edge data, since E can be huge
  e_t* outoffsets; // length N
  e_t* outdegrees; // length N
  e_t* outdegcaps; // length N
  n_t* outnodes; // length E
  radixtree_edge_t* outedgeprops; // length E
};
Inl void
LookupRaw(
  radixtree_t* t,
  slice_t s,
  idx_t* num_chars_found_,
  n_t* last_node_found_
  )
{
  auto node = t->root;
  idx_t num_chars_found = 0;
  auto last_node_found = node;
  Forever {
LOOP_NODE:
    if( num_chars_found >= s.len ) {
      break;
    }
    auto outdegree = t->outdegrees[node];
    if( !outdegree ) {
      break;
    }
    auto outoffset = t->outoffsets[node];
    Fori( e_t, e, 0, outdegree ) {
      auto outnode = t->outnodes[ outoffset + e ];
      auto outedgeprop = t->outedgeprops[ outoffset + e ];
      auto edge_text = outedgeprop->text;
      // compare suffix( s, num_chars_found ) against edge_text
      auto suffix_len = MIN( s.len, num_chars_found );
      auto suffix_offset = s.len - suffix_len;
      auto max_prefix_len = MIN( edge_text.len, suffix_len );
      auto suffix_mem = s.mem + suffix_offset;
      auto edge_text_mem = edge_text.mem;
      auto prefix_match = MemEqual( suffix_mem, edge_text_mem, max_prefix_len );
      if( prefix_match ) {
        last_node_found = node;
        node = outnode;
        num_chars_found += max_prefix_len;
        goto LOOP_NODE;
      }
    }
    // no matching prefix found!
    break;
  }
  *num_chars_found_ = num_chars_found;
  *last_node_found_ = last_node_found;
}
Inl void
Insert(
  radixtree_t* t,
  slice_t s,
  bool* already_there,
  n_t* node_found
  )
{
  //
  // traverse the tree to find the existing prefix-matching chain.
  // we may have a suffix of s to deal with afterwards.
  //
  idx_t num_chars_found;
  n_t last_node_found;
  LookupRaw( t, s, &num_chars_found, &last_node_found );
  if( num_chars_found == s.len ) {
    *already_there = true;
    *node_found = last_node_found;
    return;
  }
  //
  // check for a partial-prefix match against our remaining suffix of s.
  // we may have to split an edge, introducing a new node, to deal with this.
  //
  auto outdegree = t->outdegrees[node];
  auto outoffset = t->outoffsets[node];
  Fori( e_t, e, 0, outdegree ) {
    auto outnode = t->outnodes[ outoffset + e ];
    auto outedgeprop = t->outedgeprops[ outoffset + e ];
    auto edge_text = outedgeprop->text;
    // compare suffix( s, num_chars_found ) against edge_text
    auto suffix_len = MIN( s.len, num_chars_found );
    auto suffix_offset = s.len - suffix_len;
    auto max_prefix_len = MIN( edge_text.len, suffix_len );
    auto suffix_mem = s.mem + suffix_offset;
    auto edge_text_mem = edge_text.mem;
    idx_t prefix_len = 0;
    while( prefix_len < max_prefix_len ) {
      if( *suffix_mem++ != *edge_text_mem++ ) {
        break;
      }
      prefix_len += 1;
    }
    if( prefix_len ) {
      //
      // we're guaranteed that only one edge will match here, because we'll
      // split all other edges to maintain that unique-prefix property.
      //
      TODO
      return;
    }
  }
  //
  // no existing children had a matching prefix, so we can simply add a new child.
  //
  auto capacity_N = t->capacity_N;
  Reserve( t->outoffsets, &capacity_N, capacity_N, N, N + 1 );
  Reserve( t->outdegrees, &capacity_N, capacity_N, N, N + 1 );
  Reserve( t->outdegcaps, &capacity_N, capacity_N, N, N + 1 );
  t->capacity_N = capacity_N;
  auto node_add = t->N;
  t->N += 1;
  t->outoffsets[node_add] = 0;
  t->outdegrees[node_add] = 0;
  t->outdegcaps[node_add] = 0;
  auto outdegcap = t->outdegcaps[node];
  if( outdegcap < outdegree + 1 ) {
    t->out
  }

  auto capacity_E = t->capacity_E;
  Reserve( t->outnodes    , &capacity_E, capacity_E, E, E + 1 );
  Reserve( t->outedgeprops, &capacity_E, capacity_E, E, E + 1 );
  t->capacity_E = capacity_E;
  auto edge_add = t->E;
  t->E += 1;

  n_t* outnodes; // length E
  radixtree_edge_t* outedgeprops; // length E


      // compare suffix( s, num_chars_found ) against edge_text
      auto suffix_len = MIN( s.len, num_chars_found );
      auto suffix_offset = s.len - suffix_len;
      auto max_prefix_len = MIN( edge_text.len, suffix_len );
      idx_t prefix_len = 0;
      auto suffix_mem = s.mem + suffix_offset;
      auto edge_text_mem = edge_text.mem;
      while( prefix_len < max_prefix_len ) {
        if( *suffix_mem++ != *edge_text_mem++ ) {
          break;
        }
        prefix_len += 1;
      }
      if( prefix_len ) {
        last_node_found = node;
        node = outnode;
        num_chars_found += prefix_len;
        goto LOOP_NODE;
      }
    }
}

#endif



int
Main( u8* cmdline, idx_t cmdline_len )
{
  GlwInit();

  // we test copy/paste here, so we need to set up the g_client.hwnd
  glwclient_t client = {};
  client.hwnd = GetActiveWindow();
  g_client = &client;

  TestSimplex();
  TestCore();
  TestString();
  TestArray();
  TestBinSearch();
  TestPlist();
  TestCstr();
  TestBuf();
  TestTxt();
  TestHashset();
  TestHashsetNonzeroptrs();
  TestHashsetComplexkey();
  TestIdxHashset();
  TestStringid();
  TestRng();
  TestFilesys();
  TestMath();
  TestExecute();
  TestGraph();
  TestMinHeap();
  TestBtree();
  TestFsalloc();


  CalcJunk();
  ExeJunk();


  // Do this before destroying any datastructures, so other threads stop trying to access things.
  SignalQuitAndWaitForTaskThreads();

  GlwKill();

  return 0;
}




int
main( int argc, char** argv )
{
  MainInit();
  array_t<u8> cmdline;
  Alloc( cmdline, 512 );
  Fori( int, i, 1, argc ) {
    u8* arg = Cast( u8*, argv[i] );
    idx_t arg_len = CsLen( arg );
    Memmove( AddBack( cmdline, arg_len ), arg, arg_len );
    Memmove( AddBack( cmdline, 2 ), Str( " " ), 2 );
  }
  int r = Main( ML( cmdline ) );
  Free( cmdline );

  MainKill();

  system( "pause" );
  return r;
}


#if 0








island problem: given a 2d grid of 0s and 1s, find the number of islands.

vector<bool> grid;
struct v2 { size_t x; size_t y; };

size_t LinearIdx(v2 dim, v2 pos)
{
  return dim.x * pos.y + pos.x;
}

void DFS(vector<bool>& grid, v2 dim, vector<bool>& visited, v2 pos_start)
{
  vector<v2> stack;
  {
    stack.push_back(pos_start);
    visited[LinearIdx(dim, pos_start)] = 1;
  }
  while (stack.size()) {
    auto pos = stack.back();
    stack.pop_back();
    auto idx = LinearIdx(dim, pos);
    // no need to continue DFS on cells with value=0, they don't form part of the island.
    auto value = grid[idx];
    if (!value) continue;

    // TODO: assert that we haven't visited these before.
    if (pos.x) {
      auto next_pos = pos;
      next_pos.x -= 1;
      stack.push_back(next_pos);
      visited[LinearIdx(dim, next_pos)] = 1;
    }
    if (pos.x + 1 < dim.x) {
      auto next_pos = pos;
      next_pos.x += 1;
      stack.push_back(next_pos);
      visited[LinearIdx(dim, next_pos)] = 1;
    }
    if (pos.y) {
      auto next_pos = pos;
      next_pos.y -= 1;
      stack.push_back(next_pos);
      visited[LinearIdx(dim, next_pos)] = 1;
    }
    if (pos.y + 1 < dim.y) {
      auto next_pos = pos;
      next_pos.y += 1;
      stack.push_back(next_pos);
      visited[LinearIdx(dim, next_pos)] = 1;
    }
  }
}

size_t NumberOfIslands(vector<bool>& grid, v2 dim)
{
  // Equivalent of the DFS-Comp algorithm, which runs a DFS on every unvisited node.
  // If one DFS visits some nodes, we won't revisit those.

  size_t num_islands = 0;
  vector<bool> visited(false, dim.x * dim.y);
  for (size_t y = 0; y < dim.y; ++y) {
    for (size_t x = 0; x < dim.x; ++x) {
      auto idx = LinearIdx(dim.x, x, y);
      auto value = grid[idx];
      // no need to trigger a DFS on cells with value=0
      if (!value) continue;

      auto visited_by_dfs_already = visited[idx];
      if (visited_by_dfs_already) continue;

      // Every time we trigger another DFS, we know we're starting with a new unique graph component.
      // So bump the island count. We could also make the DFS return the component list somehow if
      // we ever wanted to.
      DFS(grid, dim, visited);
      num_islands += 1;
    }
  }
  return num_islands;
}


#endif


#if 0

C:\doc\dev\cpp\proj\main>"exe/proj64s.exe"
1.381625
cwd: C:/doc/dev/cpp/proj/main
array: -10.000000 6.000000 -4.000000 2.400000

C:\doc\dev\cpp\proj\main>"exe/proj64s.exe"
1.445404
cwd: C:/doc/dev/cpp/proj/main
array: -10.000000 6.000000 -4.000000 2.400000

C:\doc\dev\cpp\proj\main>"exe/proj64s.exe"
1.361502
cwd: C:/doc/dev/cpp/proj/main
array: -10.000000 6.000000 -4.000000 2.400000

C:\doc\dev\cpp\proj\main>"exe/proj64s.exe"
1.401971
cwd: C:/doc/dev/cpp/proj/main
array: -10.000000 6.000000 -4.000000 2.400000

C:\doc\dev\cpp\proj\main>"exe/proj64s.exe"
1.431926
cwd: C:/doc/dev/cpp/proj/main
array: -10.000000 6.000000 -4.000000 2.400000

C:\doc\dev\cpp\proj\main>"exe/proj64s.exe"
1.199476
cwd: C:/doc/dev/cpp/proj/main
array: -10.000000 6.000000 -4.000000 2.400000

C:\doc\dev\cpp\proj\main>"exe/proj64s.exe"
1.392530
cwd: C:/doc/dev/cpp/proj/main
array: -10.000000 6.000000 -4.000000 2.400000

C:\doc\dev\cpp\proj\main>"exe/proj64s.exe"
1.400318
cwd: C:/doc/dev/cpp/proj/main
array: -10.000000 6.000000 -4.000000 2.400000

C:\doc\dev\cpp\proj\main>"exe/proj64s.exe"
1.419041
cwd: C:/doc/dev/cpp/proj/main
array: -10.000000 6.000000 -4.000000 2.400000

C:\doc\dev\cpp\proj\main>"exe/proj64s.exe"
1.374627
cwd: C:/doc/dev/cpp/proj/main
array: -10.000000 6.000000 -4.000000 2.400000

C:\doc\dev\cpp\proj\main>"exe/proj64s.exe"
1.390695
cwd: C:/doc/dev/cpp/proj/main
array: -10.000000 6.000000 -4.000000 2.400000

C:\doc\dev\cpp\proj\main>"exe/proj64s.exe"
1.392700
cwd: C:/doc/dev/cpp/proj/main
array: -10.000000 6.000000 -4.000000 2.400000

C:\doc\dev\cpp\proj\main>"exe/proj64s.exe"
1.441574
cwd: C:/doc/dev/cpp/proj/main
array: -10.000000 6.000000 -4.000000 2.400000

#endif