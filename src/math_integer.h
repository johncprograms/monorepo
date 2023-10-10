// Copyright (c) John A. Carlos Jr., all rights reserved.


// intel intrinsics that aren't really intrinsic; need decls:
u8 _addcarry_u64( u8 c_in, u64 a, u64 b, u64* out );
u8 _addcarry_u32( u8 c_in, u32 a, u32 b, u32* out );
u64 _lzcnt_u64( u64 a );
u32 _lzcnt_u32( u32 a );
u64 _tzcnt_u64( u64 a );
u32 _tzcnt_u32( u32 a );
#if defined(WIN)
  #define BitScanForward_u32( a, b )     _BitScanForward( Cast( unsigned long*, ( a ) ), b )
  #define BitScanForward_u64( a, b )   _BitScanForward64( Cast( unsigned long*, ( a ) ), b )
#elif defined(MAC)
  #define BitScanForward_u32( a, b )   ( ( *( a ) = Cast( u32, std::countr_zero<u32>( b ) ) ), ( ( b ) != 0 ) )
  #define BitScanForward_u64( a, b )   ( ( *( a ) = Cast( u32, std::countr_zero<u64>( b ) ) ), ( ( b ) != 0 ) )
  #define _lzcnt_u32( a )   Cast( u32, std::countl_zero<u32>( a ) )
  #define _lzcnt_u64( a )   Cast( u64, std::countl_zero<u64>( a ) )
  #define _tzcnt_u32( a )   Cast( u32, std::countr_zero<u32>( a ) )
  #define _tzcnt_u64( a )   Cast( u64, std::countr_zero<u64>( a ) )
#else
#error
#endif


#ifdef MAC
  #define _mm_popcnt_u32( x ) Cast( u32, std::popcount<u32>( x ) )
  #define _mm_popcnt_u64( x ) Cast( u32, std::popcount<u64>( x ) )
#endif


#if _SIZEOF_IDX_T == 4

  #define _lzcnt_idx_t \
    _lzcnt_u32

  #define _tzcnt_idx_t \
    _tzcnt_u32

  #define _popcnt_idx_t( x ) \
    Cast( u32, _mm_popcnt_u32( x ) )

  #define BitScanForward_idx_t( a, b ) \
    BitScanForward_u32( ( a ), ( b ) )

#elif _SIZEOF_IDX_T == 8

  #define _lzcnt_idx_t \
    _lzcnt_u64

  #define _tzcnt_idx_t \
    _tzcnt_u64

  #define _popcnt_idx_t( x ) \
    Cast( u32, _mm_popcnt_u64( x ) )

  #define BitScanForward_idx_t( a, b ) \
    BitScanForward_u64( ( a ), ( b ) )

#else
  #error Unexpected _SIZEOF_IDX_T value!
#endif


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
  ( _flags ) |= ( 1 << ( _bit ) )

#define ClearAndSetBitIndex( _flags, _bit ) \
  ( _flags ) &= ~( 1 << ( _bit ) ) \
  ( _flags ) |= ( 1 << ( _bit ) ) \


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


#if defined(TEST)

static void
TestMathInteger()
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

#endif // defined(TEST)
