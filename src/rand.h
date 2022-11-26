// Copyright (c) John A. Carlos Jr., all rights reserved.

// xorshift RNG.
//
struct rng_xorshift32_t
{
  u32 x;
};
#define RAND_XORSHIFT32_UNIT_NORM32   ( 1.0f / 0xFFFFFFFFU )
#define RAND_XORSHIFT32_UNIT_NORM64   ( 1.0 / 0xFFFFFFFFU )
Inl void
Init( rng_xorshift32_t& rng, u64 seed )
{
  rng.x = Cast( u32, seed );
}
Inl u32
Rand32( rng_xorshift32_t& rng )
{
  rng.x ^= rng.x << 13;
  rng.x ^= rng.x >> 17;
  rng.x ^= rng.x << 5;
  return rng.x;
}
Inl u64
Rand64( rng_xorshift32_t& rng )
{
  u64 r = ( Cast( u64, Rand32( rng ) ) << 32ULL ) | Rand32( rng );
  return r;
}
Inl f32
Zeta32( rng_xorshift32_t& rng )
{
  return Rand32( rng ) * RAND_XORSHIFT32_UNIT_NORM32;
}
Inl f64
Zeta64( rng_xorshift32_t& rng )
{
  return Rand32( rng ) * RAND_XORSHIFT32_UNIT_NORM64;
}


// Linear congruential RNG.
//   Xn+1 = ( a * Xn + b ) mod m.
//
struct rng_lcg_t
{
  u64 m;
  u64 a;
  u64 b;
  u64 x;
  f64 unit_norm64;
  f32 unit_norm32;
};
Inl void
Init( rng_lcg_t& rng, u64 seed )
{
  rng.m = 0x0000000100000000ULL; // modulus, 2^32.
  rng.a = 0x000000005851F42DULL; // in( 0, m )
  rng.b = 0x0000000014057B7DULL; // in [ 0, m )
  rng.x = 0x00000000FFFFFFFFULL & ( rng.m - seed ); // in [ 0, m )
  rng.unit_norm64 = 1.0 / ( rng.m - 1.0 );
  rng.unit_norm32 = 1.0f / ( rng.m - 1.0f );
}
Inl u32
Rand32( rng_lcg_t& rng )
{
  rng.x = 0x00000000FFFFFFFFULL & ( rng.a * rng.x + rng.b );
  return Cast( u32, rng.x );
}
Inl u64
Rand64( rng_lcg_t& rng )
{
  u64 r;
  u32* dst32 = Cast( u32*, &r );
  rng.x = 0x00000000FFFFFFFFULL & ( rng.a * rng.x + rng.b );
  *dst32++ = Cast( u32, rng.x );
  rng.x = 0x00000000FFFFFFFFULL & ( rng.a * rng.x + rng.b );
  *dst32++ = Cast( u32, rng.x );
  return r;
}
Inl f32
Zeta32( rng_lcg_t& rng )
{
  rng.x = 0x00000000FFFFFFFFULL & ( rng.a * rng.x + rng.b );
  return rng.x * rng.unit_norm32;
}
Inl f64
Zeta64( rng_lcg_t& rng )
{
  rng.x = 0x00000000FFFFFFFFULL & ( rng.a * rng.x + rng.b );
  return rng.x * rng.unit_norm64;
}
Inl void
Rand( rng_lcg_t& rng, u32* dst, u32 n )
{
  while( n-- ) {
    *dst++ = Rand32( rng );
  }
}
Inl void
Rand( rng_lcg_t& rng, u64* dst, u32 n )
{
  while( n-- ) {
    *dst++ = Rand64( rng );
  }
}
Inl void
Zeta( rng_lcg_t& rng, f32* dst, u32 n )
{
  while( n-- ) {
    *dst++ = Zeta32( rng );
  }
}
Inl void
Zeta( rng_lcg_t& rng, f64* dst, u32 n )
{
  while( n-- ) {
    *dst++ = Zeta64( rng );
  }
}


// Mersenne Twister RNG.
//
struct rng_mt_t
{
  u32 mt [ 624 ];
  u32 idx;
};
#define RAND_MT_UNIT_NORM32   ( 1.0f / 0xFFFFFFFFU )
#define RAND_MT_UNIT_NORM64   ( 1.0 / 0xFFFFFFFFU )
Inl void
Init( rng_mt_t& rng, u64 seed )
{
  rng.idx = 0;
  rng.mt[0] = Cast( u32, seed );
  Fori( u32, i, 1, 624 ) {
    rng.mt[i] = 0x6C078965U * ( rng.mt[i-1] ^ ( rng.mt[i-1] >> 30 ) ) + i;
  }
}
Inl u32
ComputeRand( rng_mt_t& rng )
{
  if( rng.idx == 0 ) {
    Fori( u32, i, 0, 624 ) {
      u32 y = ( rng.mt[i] & 0x80000000U ) + ( rng.mt[ (i+1) % 624 ] & 0x7FFFFFFFU );
      rng.mt[i] = rng.mt[ (i+397) % 624 ] ^ ( y >> 1 );
      if( y & 1 ) {
        rng.mt[i] ^= 0x9908B0DFU;
      }
    }
  }
  u32 z = rng.mt[rng.idx];
  rng.idx = ( rng.idx + 1 ) % 624;
  z ^= ( z >> 11 );
  z ^= ( ( z << 7 ) & 0x9D2C5680U );
  z ^= ( ( z << 15 ) & 0xEFC60000U );
  z ^= ( z >> 18 );
  return z;
}
Inl u32
Rand32( rng_mt_t& rng )
{
  return ComputeRand( rng );
}
Inl u64
Rand64( rng_mt_t& rng )
{
  u64 r;
  u32* dst32 = Cast( u32*, &r );
  *dst32++ = ComputeRand( rng );
  *dst32++ = ComputeRand( rng );
  return r;
}
Inl f32
Zeta32( rng_mt_t& rng )
{
  return ComputeRand( rng ) * RAND_MT_UNIT_NORM32;
}
Inl f64
Zeta64( rng_mt_t& rng )
{
  return ComputeRand( rng ) * RAND_MT_UNIT_NORM64;
}
Inl void
Rand( rng_mt_t& rng, u32* dst, u32 n )
{
  while( n-- ) {
    *dst++ = ComputeRand( rng );
  }
}
Inl void
Rand( rng_mt_t& rng, u64* dst, u32 n )
{
  u32* dst32 = Cast( u32*, dst );
  while( n-- ) {
    *dst32++ = ComputeRand( rng );
    *dst32++ = ComputeRand( rng );
  }
}
Inl void
Zeta( rng_mt_t& rng, f32* dst, u32 n )
{
  while( n-- ) {
    *dst++ = ComputeRand( rng ) * RAND_MT_UNIT_NORM32;
  }
}
Inl void
Zeta( rng_mt_t& rng, f64* dst, u32 n )
{
  while( n-- ) {
    *dst++ = ComputeRand( rng ) * RAND_MT_UNIT_NORM64;
  }
}



#if defined(TEST)

static void
TestRng()
{
#if 0
  rng_mt_t mt;
  Init( mt, 0x123456789 );

  rng_lcg_t lcg;
  Init( lcg, 0x123456789 );

  u64 n = 10000;

  f64 mu = 0;
  f64 var = 0;
  u64 count = 0;

  For( i, 0, n ) {
    u64 val;

    //while( !_rdrand64_step( &val ) );
    //while( !_rdseed64_step( &val ) );
    //val = Rand64( mt );
    val = Rand64( lcg );

    //count += _mm_popcnt_u64( val );

    For( j, 0, 64 ) {
      u64 digit = ( val >> j ) & 1;

      u64 prev_n = count;
      f64 rec_n = 1.0 / ( prev_n + 1 );
      f64 prev_mu = mu;
      f64 prev_diff = digit - prev_mu;
      mu = prev_mu + rec_n * prev_diff;

      f64 prev_sn = prev_n * var;
      var = rec_n * ( prev_sn + prev_diff * ( digit - mu ) );

      count += 1;
    }

#if 0
    u8 tmp[65];
    CsFromIntegerU(
      tmp,
      _countof( tmp ),
      val,
      0,
      0,
      0,
      2,
      Cast( u8*, "0 " )
      );
    printf( "%064s", tmp );
#endif
  }
  printf( "\n" );

  printf( "%f\n", mu );
  printf( "%f\n", var );
#endif
}

#endif // defined(TEST)
