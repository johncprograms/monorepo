// Copyright (c) John A. Carlos Jr., all rights reserved.

constexpr Inl idx_t 
Bitbuffer64Len( idx_t N )
{
  return N / 64 + ( ( N % 64 ) != 0 );
}

TemplIdxN struct
embeddedbitbuffer_t
{
  u64 mem[ Bitbuffer64Len( N ) ];
};

TemplIdxN Inl void
Zero( embeddedbitbuffer_t<N>& array )
{
  Memzero( array.mem, _countof( array.mem ) * sizeof( array.mem[0] ) );
}

TemplIdxN Inl void
SetBit( embeddedbitbuffer_t<N>& array, idx_t index, bool value )
{
#if 0
  auto bin = index / 64;
  auto bit = index % 64;
#else
  auto bin = index >> 6;
  auto bit = index & 63;
#endif
  AssertCrash( index < N );
  AssertCrash( bin < _countof( array.mem ) );
  if( value ) {
    array.mem[bin] |= ( 1ULL << bit );
  }
  else {
    array.mem[bin] &= ~( 1ULL << bit );
  }
}

TemplIdxN Inl bool
GetBit( embeddedbitbuffer_t<N>& array, idx_t index )
{
#if 0
  auto bin = index / 64;
  auto bit = index % 64;
#else
  auto bin = index >> 6;
  auto bit = index & 63;
#endif
  AssertCrash( index < N );
  AssertCrash( bin < _countof( array.mem ) );
  bool r = array.mem[bin] & ( 1ULL << bit );
  return r;
}
