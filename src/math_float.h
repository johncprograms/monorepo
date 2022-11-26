// Copyright (c) John A. Carlos Jr., all rights reserved.

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


// TODO: make function overloading first-class for all the following functions, for better callsite variance, e.g. templates.

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

#if _SIZEOF_IDX_T == 4

  #define Round_idx_from_f32( x ) \
    Round_u32_from_f32( x )

  #define Round_idx_from_f64( x ) \
    Round_u32_from_f64( x )

  #define Truncate_idx_from_f32( x ) \
    Truncate_u32_from_f32( x )

  #define Truncate_idx_from_f64( x ) \
    Truncate_u32_from_f64( x )

#elif _SIZEOF_IDX_T == 8

  #define Round_idx_from_f32( x ) \
    Round_u64_from_f32( x )

  #define Round_idx_from_f64( x ) \
    Round_u64_from_f64( x )

  #define Truncate_idx_from_f32( x ) \
    Truncate_u64_from_f32( x )

  #define Truncate_idx_from_f64( x ) \
    Truncate_u64_from_f64( x )

#else
  #error Unexpected _SIZEOF_IDX_T value!
#endif


#if defined(MAC) // TODO: do this better.

  f32 Floor32( f32 x );
  f64 Floor64( f64 x );

  f32 Ceil32( f32 x );
  f64 Ceil64( f64 x );

  f32 Truncate32( f32 x );
  f64 Truncate64( f64 x );

  f32 Round32( f32 x );
  f64 Round64( f64 x );

  f32 Sqrt32( f32 x );
  f64 Sqrt64( f64 x );

#else

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

#endif

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

Inl f32 Square( f32 x ) { return x * x; }
Inl f64 Square( f64 x ) { return x * x; }

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



#if defined(TEST)

Inl void
TestMathFloat()
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
    { 9223372036854775808ULL, 9223372036854775800.0 },
    { 15000000000000000000ULL, 15000000000000000000.0 },
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

#endif // defined(TEST)
