// Copyright (c) John A. Carlos Jr., all rights reserved.

constexpr Inl idx_t 
Bitbuffer64Len( idx_t N )
{
  return N / 64 + ( ( N % 64 ) != 0 );
}

TemplIdxN struct
bitarray_nonresizeable_stack_t
{
  u64 mem[ Bitbuffer64Len( N ) ];
};

TemplIdxN ForceInl void
Zero( bitarray_nonresizeable_stack_t<N>& array )
{
  Memzero( array.mem, _countof( array.mem ) * sizeof( array.mem[0] ) );
}

TemplIdxN ForceInl void
SetBit( bitarray_nonresizeable_stack_t<N>& array, idx_t index, bool value )
{
  auto bin = index >> 6;
  auto bit = index & 63;
  AssertCrash( index < N );
  AssertCrash( bin < _countof( array.mem ) );
  if( value ) {
    array.mem[bin] |= ( 1ULL << bit );
  }
  else {
    array.mem[bin] &= ~( 1ULL << bit );
  }
}
TemplIdxN ForceInl void
SetBit1( bitarray_nonresizeable_stack_t<N>& array, idx_t index )
{
  auto bin = index >> 6;
  auto bit = index & 63;
  AssertCrash( index < N );
  AssertCrash( bin < _countof( array.mem ) );
  array.mem[bin] |= ( 1ULL << bit );
}
TemplIdxN ForceInl void
SetBit0( bitarray_nonresizeable_stack_t<N>& array, idx_t index )
{
  auto bin = index >> 6;
  auto bit = index & 63;
  AssertCrash( index < N );
  AssertCrash( bin < _countof( array.mem ) );
  array.mem[bin] &= ~( 1ULL << bit );
}
TemplIdxN ForceInl bool
GetBit( bitarray_nonresizeable_stack_t<N>& array, idx_t index )
{
  auto bin = index >> 6;
  auto bit = index & 63;
  AssertCrash( index < N );
  AssertCrash( bin < _countof( array.mem ) );
  bool r = array.mem[bin] & ( 1ULL << bit );
  return r;
}


ForceInl void
SetBit( u64* mem, idx_t bitcapacity, idx_t bitindex, bool value )
{
  auto bin = bitindex >> 6;
  auto bit = bitindex & 63;
  AssertCrash( bitindex < bitcapacity );
  if( value ) {
    mem[bin] |= ( 1ULL << bit );
  }
  else {
    mem[bin] &= ~( 1ULL << bit );
  }
}
ForceInl void
SetBit1( u64* mem, idx_t bitcapacity, idx_t bitindex )
{
  auto bin = bitindex >> 6;
  auto bit = bitindex & 63;
  AssertCrash( bitindex < bitcapacity );
  mem[bin] |= ( 1ULL << bit );
}
ForceInl void
SetBit0( u64* mem, idx_t bitcapacity, idx_t bitindex )
{
  auto bin = bitindex >> 6;
  auto bit = bitindex & 63;
  AssertCrash( bitindex < bitcapacity );
  mem[bin] &= ~( 1ULL << bit );
}
ForceInl bool
GetBit( u64* mem, idx_t bitlen, idx_t index )
{
  auto bin = index >> 6;
  auto bit = index & 63;
  AssertCrash( index < bitlen );
  bool r = mem[bin] & ( 1ULL << bit );
  return r;
}


ForceInl void
SetBit( u32* mem, idx_t bitcapacity, idx_t bitindex, bool value )
{
  auto bin = bitindex >> 5;
  auto bit = bitindex & 31;
  AssertCrash( bitindex < bitcapacity );
  if( value ) {
    mem[bin] |= ( 1u << bit );
  }
  else {
    mem[bin] &= ~( 1u << bit );
  }
}
ForceInl void
SetBit1( u32* mem, idx_t bitcapacity, idx_t bitindex )
{
  auto bin = bitindex >> 5;
  auto bit = bitindex & 31;
  AssertCrash( bitindex < bitcapacity );
  mem[bin] |= ( 1u << bit );
}
ForceInl void
SetBit0( u32* mem, idx_t bitcapacity, idx_t bitindex )
{
  auto bin = bitindex >> 5;
  auto bit = bitindex & 31;
  AssertCrash( bitindex < bitcapacity );
  mem[bin] &= ~( 1u << bit );
}
ForceInl bool
GetBit( u32* mem, idx_t bitlen, idx_t index )
{
  auto bin = index >> 5;
  auto bit = index & 31;
  AssertCrash( index < bitlen );
  bool r = mem[bin] & ( 1u << bit );
  return r;
}
