// Copyright (c) John A. Carlos Jr., all rights reserved.

#if defined(MAC)
  int sprintf_s(
     char *buffer,
     size_t sizeOfBuffer,
     const char *format,
     ...
  )
  {
		va_list args;
		va_start( args, format );
		int r = vsprintf_s( buffer, sizeOfBuffer, format, args );
		va_end( args );
		return r;
  }
#endif

Inl void
CsFrom_f64(
  u8* dst,
  idx_t dst_len,
  idx_t* dst_size,
  f64 src,
  u8 num_decimal_places = 14
  )
{
  *dst_size = sprintf_s( Cast( char*, dst ), dst_len, "%.*f", num_decimal_places, src );
}

Inl void
CsFrom_f32(
  u8* dst,
  idx_t dst_len,
  idx_t* dst_size,
  f32 src,
  u8 num_decimal_places = 7
  )
{
  *dst_size = sprintf_s( Cast( char*, dst ), dst_len, "%.*f", num_decimal_places, src );
}



Inl f64
CsTo_f64( u8* src, idx_t src_len )
{
  stack_nonresizeable_stack_t<u8, 64> tmp;
  Zero( tmp );
  Memmove( AddBack( tmp, src_len ), src, src_len );
  *AddBack( tmp ) = 0;
  return Cast( f64, atof( Cast( char*, tmp.mem ) ) );
}

Inl f32
CsTo_f32( u8* src, idx_t src_len )
{
  stack_nonresizeable_stack_t<u8, 64> tmp;
  Zero( tmp );
  Memmove( AddBack( tmp, src_len ), src, src_len );
  *AddBack( tmp ) = 0;
  return Cast( f32, atof( Cast( char*, tmp.mem ) ) );
}



constant f32 g_f32_from_digit[] = {
  0.0f,
  1.0f,
  2.0f,
  3.0f,
  4.0f,
  5.0f,
  6.0f,
  7.0f,
  8.0f,
  9.0f,
};
#if 0
Inl bool
CsToFloat32(
  u8* src,
  idx_t src_len,
  f32& dst
  )
{
  dst = 0;
  if( !( src_len || AsciiIsNumber( *src ) || *src == '.' || *src == '-' || *src == '+' || *src == 'N' || *src == 'I' ) ) {
    return 0;
  }
  bool negative = ( *src == '-' );
  if( *src == '-' || *src == '+' ) {
    src += 1;
    src_len -= 1;
  }
  if( !( src_len || AsciiIsNumber( *src ) || *src == '.' || *src == 'N' || *src == 'I' ) ) {
    return 0;
  }
  if( StringEquals( src, src_len, Str( "NAN" ), 3, 1 ) ) {
    dst = ( negative  ?  -NAN  :  NAN );
    return 1;
  } elif( StringEquals( src, src_len, Str( "INF" ), 3, 1 ) ) {
    dst = ( negative  ?  -INFINITY  :  INFINITY );
    return 1;
  }
  if( !( AsciiIsNumber( *src ) || *src == '.' ) ) {
    return 0;
  }
  // TODO: only do one iteration.
  auto pos_exponent = src_len;
  For( i, 0, src_len ) {
    if( src[i] == 'e' || src[i] == 'E' ) {
      pos_exponent = i;
      break;
    }
  }
  f32 digit_factor = 1.0f; // TODO: ordered table, so we don't accumulate errors.
  auto pos_decimal = pos_exponent;
  For( i, 0, pos_exponent ) {
    if( src[i] == '.' ) {
      pos_decimal = i;
      break;
    }
    if( i ) {
      digit_factor *= 10.0f;
    }
  }
  kahansum32_t d = {};
  For( i, 0, pos_exponent ) {
    auto half = src[i];
    if( half == '.' ) {
      continue;
    }
    if( !AsciiIsNumber( half ) ) {
      return 0;
    }
    auto digit = g_f32_from_digit[ AsciiAsNumber( half ) ];
    Add( d, digit * digit_factor );
    digit_factor *= 0.1f;
  }
  src += pos_exponent;
  src_len -= pos_exponent;
  if( src_len ) {
    if( !( *src == 'e' || *src == 'E' ) ) {
      return 0;
    }
    src += 1;
    src_len -= 1;
    if( !( src_len || AsciiIsNumber( *src ) || *src == '-' || *src == '+' ) ) {
      return 0;
    }
    bool exp_negative = ( *src == '-' );
    if( *src == '-' || *src == '+' ) {
      src += 1;
      src_len -= 1;
    }
    if( !( src_len || AsciiIsNumber( *src ) ) ) {
      return 0;
    }
    auto exp = g_f32_from_digit[ AsciiAsNumber( *src ) ];
    while( src_len ) {
      src += 1;
      src_len -= 1;
      if( src_len ) {
        if( !AsciiIsNumber( *src ) ) {
          return 0;
        }
        exp *= 10.0f;
        exp += g_f32_from_digit[ AsciiAsNumber( *src ) ];
      }
    }
    exp = ( exp_negative  ?  -exp  :  exp );
    d.sum *= Pow( 10.0f, exp );
  }
  dst = d.sum;
  return 1;
}
#endif
#if 0
-149  1.4012984643248170709237295832899e-45
-148  2.8025969286496341418474591665798e-45
-147  5.6051938572992682836949183331597e-45
-146
-145
-144
-143
-142
 -25
 -24  -8   5.9604644775390625
 -23  -7   1.1920928955078125
 -22  -7   2.384185791015625
 -21  -7   4.76837158203125
 -20  -7   9.5367431640625
 -19  -6   1.9073486328125
 -18  -6   3.814697265625
 -17  -6   7.62939453125
 -16  -5   1.52587890625
 -15  -5   3.0517578125
 -14  -5   6.103515625
 -13  -4   1.220703125
 -12  -4   2.44140625
 -11  -4   4.8828125
 -10  -4   9.765625
  -9  -3   1.953125
  -8  -3   3.90625
  -7  -3   7.8125
  -6  -2   1.5625
  -5  -2   3.125
  -4  -2   6.25
  -3  -1   1.25
  -2  -1   2.5
  -1  -1   5
   0   0   1
   1   0   2
   2   0   4
   3   0   8
   4   1   1.6
   5   1   3.2
   6   1   6.4
   7   2   1.28
   8   2   2.56
   9   2   5.12
  10   3   1.024
  11   3   2.048
  12   3   4.096
  13   3   8.192
  14   4   1.6384
  15   4   3.2768
  16   4   6.5536
  17   5   1.31072
  18   5   2.62144
  19   5   5.24288
  10   6   1.048576
  11   6   2.097152
  12   6   4.194304
  13   6   8.388608
  14   7   1.6777216
  15   7   3.3554432
  16   7   6.7108864
  17   8   1.34217728
  18   8   2.68435456
  19   8   5.36870912
  20   9   1.073741824
  21   9   2.147483648
  22   9   4.294967296
  23   9   8.589934592
  24  10   1.7179869184
  25  10   3.4359738368
  26  10   6.8719476736
  27  11   1.37438953472
  28  11   2.74877906944
  29  11   5.49755813888

64
63        1
61       11
57      111
49     1111
33    11111
1    111111
0   1000000


63
62        1
60       11
56      111
48     1111
32    11111
0    111111


62
61        1
59       11
55      111
47     1111
31    11111
15   101111
7    110111
3    111011
1    111101
0    111110

#endif


struct
num2_t
{
  stack_resizeable_cont_t<u64> bins;
};

Inl void
Init( num2_t& num, idx_t size )
{
  Alloc( num.bins, size );
}

Inl void
Kill( num2_t& num )
{
  Free( num.bins );
}

Inl void
TrimLeadingZeros( num2_t& dst )
{
  idx_t leading_zeros = 0;
  ReverseFor( i, 0, dst.bins.len ) {
    if( dst.bins.mem[i] != 0 ) {
      break;
    }
    leading_zeros += 1;
  }
  dst.bins.len -= leading_zeros;
}

Inl bool
IsZero( num2_t& dst )
{
  return !dst.bins.len;
}

#if 0
Inl void
Add( num2_t& dst, u64 b )
{
  if( !dst.bins.len ) {
    *AddBack( dst.bins ) = b;
  } else {
    u8 carry = 0;
    idx_t bin = 0;
    do {
      if( bin >= dst.bins.len ) {
        *AddBack( dst.bins ) = 0;
      }
      carry = _addcarry_u64( carry, dst.bins.mem[bin], b, dst.bins.mem + bin );
      b = 0;
      bin += 1;
    } while( carry );
  }
}
#endif

// dst += 1 << index
Inl void
AddBit( num2_t& dst, u64 index )
{
  u64 bin = index / 64;
  u64 bit = index % 64;
  if( bin >= dst.bins.len ) {
    auto nbins = bin - dst.bins.len + 1;
    AssertCrash( nbins <= MAX_idx );
    Memzero( AddBack( dst.bins, Cast( idx_t, nbins ) ), Cast( idx_t, nbins ) * sizeof( u64 ) );
  }
  u8 carry = 0;
  u64 b = ( 1ULL << bit );
  do {
    if( bin >= dst.bins.len ) {
      *AddBack( dst.bins ) = 0;
    }
    carry = _addcarry_u64( carry, dst.bins.mem[bin], b, dst.bins.mem + bin );
    b = 0;
    bin += 1;
  } while( carry );
}

// dst[index] = value
Inl void
SetBit( num2_t& dst, u64 index, bool value )
{
  u64 bin = index / 64;
  u64 bit = index % 64;
  if( value ) {
    if( bin >= dst.bins.len ) {
      auto nbins = bin - dst.bins.len + 1;
      AssertCrash( nbins <= MAX_idx );
      Memzero( AddBack( dst.bins, Cast( idx_t, nbins ) ), Cast( idx_t, nbins ) * sizeof( u64 ) );
    }
    dst.bins.mem[bin] |= ( 1ULL << bit );
  } else {
    if( bin < dst.bins.len ) {
      dst.bins.mem[bin] &= ~( 1ULL << bit );
    }
  }
}

Inl bool
GetBit( num2_t& src, u64 index ) // TODO: not optimal
{
  u64 bin = index / 64;
  u64 bit = index % 64;
  if( bin >= src.bins.len ) {
    return 0;
  }
  return ( src.bins.mem[bin] >> bit ) & 1;
}

struct
bignum_t
{
  stack_resizeable_cont_t<u8> digits; // index 0 is 10s place, index 1 is 100s place, etc.
  s64 exp;
  bool positive;
};

Inl void
Init( bignum_t& num, idx_t size )
{
  Alloc( num.digits, size );
  num.exp = 0;
  num.positive = 1;
}

Inl void
Kill( bignum_t& num )
{
  Free( num.digits );
}

Inl void
CopyNum( bignum_t& dst, bignum_t& src )
{
  Copy( dst.digits, src.digits );
  dst.exp = src.exp;
  dst.positive = src.positive;
}

Inl bool
IsZero( bignum_t& num )
{
  return ( !num.digits.len );
}

Inl void
TrimLeadingZeros( bignum_t& dst )
{
  idx_t leading_zeros = 0;
  ReverseFor( i, 0, dst.digits.len ) {
    if( dst.digits.mem[i] != 0 ) {
      break;
    }
    leading_zeros += 1;
  }
  AssertCrash( dst.digits.len >= leading_zeros );
  dst.digits.len -= leading_zeros;
}

Inl void
IfAllZerosSetToZero( bignum_t& dst )
{
  bool nonzero = 0;
  ForLen( i, dst.digits ) {
    if( dst.digits.mem[i] != 0 ) {
      nonzero = 1;
      break;
    }
  }
  if( !nonzero ) {
    dst.digits.len = 0;
    dst.positive = 1;
  }
}

Inl void
Add( bignum_t& a, bignum_t& b, bignum_t& dst )
{
  auto len_a = Cast( s64, a.digits.len );
  auto len_b = Cast( s64, b.digits.len );
  dst.digits.len = 0;
  s64 exp = MIN( a.exp, b.exp );
  s64 nplaces = MAX( len_a + a.exp, len_b + b.exp ) - exp;
  dst.exp = exp;
  s8 carry = 0;
  auto place_a = exp - a.exp;
  auto place_b = exp - b.exp;
  Fori( s64, i, 0, nplaces ) {
    s8 digit_a = ( 0 <= place_a && place_a < len_a )  ?  Cast( s8, a.digits.mem[ place_a ] )  :  0;
    s8 digit_b = ( 0 <= place_b && place_b < len_b )  ?  Cast( s8, b.digits.mem[ place_b ] )  :  0;
    place_a += 1;
    place_b += 1;
    s8 extra_carry = 0;
    if( a.positive  &&  b.positive ) {
    } elif( !a.positive  &&  !b.positive ) {
      digit_a = -digit_a;
      digit_b = -digit_b;
    } elif( !a.positive  &&  b.positive ) {
      digit_a = -digit_a;
      digit_a -= 10;
      extra_carry = 1;
    } else {
      digit_b = -digit_b;
      digit_a += 10;
      extra_carry = -1;
    }
//    digit_a = a.positive  ?  digit_a  :  -digit_a;
//    digit_b = b.positive  ?  digit_b  :  -digit_b;
//    if( ABS( digit_a ) < ABS( digit_b ) ) {
//      digit_a += 10;
//      extra_carry = -1;
//    }
    s8 sum = digit_a + digit_b + carry;
    s8 digit;
    if( sum < 0 ) {
      carry = ( sum <= -10 )  ?  -1  :  0;
      digit = ( sum <= -10 )  ?  sum + 10  :  sum;
    } else {
      carry = ( sum >= 10 )  ?  1  :  0;
      digit = ( sum >= 10 )  ?  sum - 10  :  sum;
    }
    carry += extra_carry;
    *AddBack( dst.digits ) = Cast( u8, ABS( digit ) );
    dst.positive = ( sum >= 0 );
  }
  if( carry ) {
    *AddBack( dst.digits ) = 1;
    dst.positive = ( carry >= 0 );
  } else {
    TrimLeadingZeros( dst );
  }
}

Inl void
Mul( bignum_t& a, u8 b, bool b_positive, bignum_t& dst )
{
  dst.digits.len = 0;
  switch( b ) {
    case 0: {
      dst.exp = 0;
    } break;
    case 1: {
      Copy( dst.digits, a.digits );
      dst.exp = a.exp;
    } break;
    default: {
      Reserve( dst.digits, a.digits.len + 1 );
      dst.exp = a.exp;
      u8 carry = 0;
      For( i, 0, a.digits.len ) {
        u8 prod = b * a.digits.mem[i] + carry;
        carry = prod / 10;
        u8 digit = prod % 10;
        *AddBack( dst.digits ) = digit;
      }
      if( carry ) {
        *AddBack( dst.digits ) = carry;
      } else {
        TrimLeadingZeros( dst );
      }
    } break;
  }
  dst.positive = ( a.positive == b_positive );
}

Inl void
Mul( bignum_t& a, bignum_t& b, bignum_t& tmp, bignum_t& tmp2, bignum_t& dst )
{
  dst.digits.len = 0;
  dst.exp = 0;
  For( i, 0, a.digits.len ) {
    Mul( b, a.digits.mem[i], a.positive, tmp ); // TODO: only 10 possible results in this loop; cache them!
    tmp.exp += i;
    CopyNum( tmp2, dst ); // TODO: add in place
    Add( tmp, tmp2, dst );
  }
  dst.exp += a.exp;
}

Inl bool
CsFromFloat32(
  u8* dst,
  idx_t dst_len,
  idx_t* dst_size,
  f32 src
  )
{
  auto srcu = *Cast( u32*, &src );
  auto sign = srcu >> 31u;
  u8 rawexpu = Cast( u8, srcu >> 23u );
  auto f = ( 1 << 23u ) | ( srcu & AllOnes( 23u ) );

  idx_t size = 0;

  if( sign ) {
    if( 1 > dst_len ) {
      return 0;
    }
    *dst++ = '-';
    dst_len -= 1;
    size += 1;
  }

  if( !rawexpu ) { // zero, denormals
    if( f ) { // denormals
      UnreachableCrash();
      return 0;
    } else { // zero
      if( 1 > dst_len ) {
        return 0;
      }
      *dst++ = '0';
      dst_len -= 1;
      size += 1;
    }
  } elif( rawexpu == AllOnes( 8 ) ) { // inf, nan
    if( 3 > dst_len ) {
      return 0;
    }
    auto str = f  ?  Str( "nan" )  :  Str( "inf" );
    Memmove( dst, str, 3 );
    dst += 3;
    dst_len -= 3;
    size += 3;
  } else {
    s8 rawexp = ( rawexpu >= 127 )  ?  Cast( s8, rawexpu - 127 )  :  Cast( s8, rawexpu ) - 127;

    bignum_t a, b, c, d, e;
    Init( a, 100 );
    Init( b, 100 );
    Init( c, 100 );
    Init( d, 100 );
    Init( e, 100 );

    bignum_t sum;
    Init( sum, 100 );
    Fori( s8, i, 0, 24 ) {
      if( f & ( 1 << i ) ) {
        auto exp = rawexp + i - 23;

        // TODO: cache powers of 2

        // a = 2 ^ exp
        a.exp = 0;
        a.positive = 1;
        a.digits.len = 0;
        *AddBack( a.digits ) = 1;

        if( exp > 0 ) {
          b.exp = 0;
          b.positive = 1;
          b.digits.len = 0;
          *AddBack( b.digits ) = 2;

        } elif( exp < 0 ) {
          b.exp = -1;
          b.positive = 1;
          b.digits.len = 0;
          *AddBack( b.digits ) = 5;
        }
        idx_t expu = ABS( exp );
        For( j, 0, expu ) {
          Mul( a, b, c, d, e ); // TODO: mul in place
          CopyNum( a, e );
        }

        // sum += 2 ^ exp
        CopyNum( c, sum ); // TODO: add in place
        Add( a, c, sum );
      }
    }

    // TODO: limit digits length.

    if( sum.digits.len > dst_len ) {
      Kill( a );
      Kill( b );
      Kill( c );
      Kill( d );
      Kill( e );
      Kill( sum );
      return 0;
    }
    ReverseFor( i, 0, sum.digits.len ) {
      *dst++ = '0' + sum.digits.mem[i];
    }
    dst_len -= sum.digits.len;
    size += sum.digits.len;

    if( 1 > dst_len ) {
      Kill( a );
      Kill( b );
      Kill( c );
      Kill( d );
      Kill( e );
      Kill( sum );
      return 0;
    }
    *dst++ = 'e';
    dst_len -= 1;
    size += 1;

    idx_t exp_size = 0;
    if( !CsFromIntegerS( dst, dst_len, &exp_size, sum.exp ) ) {
      Kill( a );
      Kill( b );
      Kill( c );
      Kill( d );
      Kill( e );
      Kill( sum );
      return 0;
    }
    dst += exp_size;
    dst_len -= exp_size;
    size += exp_size;

    *dst_size = size;

    Kill( a );
    Kill( b );
    Kill( c );
    Kill( d );
    Kill( e );
    Kill( sum );
  }

  if( 1 > dst_len ) {
    return 0;
  }
  *dst++ = 0;
  dst_len -= 1;
  size += 1;

  return 1;
}

Inl bool
CsToFloat32(
  u8* src,
  idx_t src_len,
  f32& dst
  )
{
  // TODO: full input verification.

  dst = 0;
  if( !( src_len || AsciiIsNumber( *src ) || *src == '.' || *src == '-' || *src == '+' || *src == 'n' || *src == 'i' ) ) {
    return 0;
  }
  bool negative = ( *src == '-' );
  if( *src == '-' || *src == '+' ) {
    src += 1;
    src_len -= 1;
  }
  if( !( src_len || AsciiIsNumber( *src ) || *src == '.' || *src == 'n' || *src == 'i' ) ) {
    return 0;
  }
  if( StringEquals( src, src_len, Str( "nan" ), 3, 1 ) ) {
    dst = ( negative  ?  -NAN  :  NAN );
    return 1;
  } elif( StringEquals( src, src_len, Str( "inf" ), 3, 1 ) ) {
    dst = ( negative  ?  -INFINITY  :  INFINITY );
    return 1;
  }
  if( !( AsciiIsNumber( *src ) || *src == '.' ) ) {
    return 0;
  }
  auto pos_decimal = src_len;
  auto pos_exponent = src_len;
  For( i, 0, src_len ) {
    if( src[i] == '.' ) {
      pos_decimal = i;
      continue;
    }
    if( src[i] == 'e' || src[i] == 'E' ) {
      pos_exponent = i;
      continue;
    }
  }
  s16 exp = 0;
  if( pos_exponent != src_len ) {
    exp = CsToIntegerS<s16>( src + pos_exponent + 1, src_len - pos_exponent - 1, 0 );
  }

  // TODO: exp bounds!

  bignum_t intnum;
  Init( intnum, 100 );
  bignum_t fracnum;
  Init( fracnum, 100 );

  s64 pos_abs = Cast( s64, pos_decimal ) + exp;
  if( exp >= 0 ) {
    // 01234567
    // 123.45e4
    // 1234500
    // pos_decimal = 3
    // exp = 4
    // pos_abs = 7

    // 012345678
    // 12.3456e1
    // 123.456
    // pos_decimal = 2
    // exp = 1
    // pos_abs = 3

    ReverseFori( s64, i, 0, pos_abs + 1 ) {
      if( i >= Cast( s64, pos_exponent ) ) {
        *AddBack( intnum.digits ) = 0;
      } elif( i == Cast( s64, pos_decimal ) ) {
        continue;
      } else {
        *AddBack( intnum.digits ) = AsciiAsNumber( src[i] );
      }
    }
    ReverseFori( s64, i, pos_abs + 1, Cast( s64, pos_exponent ) ) {
      *AddBack( fracnum.digits ) = AsciiAsNumber( src[i] );
    }
  } else {
    // 0123456789
    // 1234.56e-2
    // 12.3456
    // pos_decimal = 4
    // exp = -2
    // pos_abs = 2

    // 0123456789
    // 12.345e-4
    // 0.0012345
    // pos_decimal = 2
    // exp = -4
    // pos_abs = -2

    ReverseFori( s64, i, 0, pos_abs + 1 ) {
      *AddBack( intnum.digits ) = AsciiAsNumber( src[i] );
    }
    ReverseFori( s64, i, pos_abs, Cast( s64, pos_exponent ) ) {
      if( i < 0 ) {
        *AddBack( fracnum.digits ) = 0;
      } elif( i == Cast( s64, pos_decimal ) ) {
        continue;
      } else {
        *AddBack( fracnum.digits ) = AsciiAsNumber( src[i] );
      }
    }
  }
  TrimLeadingZeros( intnum );
  AssertCrash( intnum.exp == 0 ); // must be 0 for algorithm to work!
  fracnum.exp = -Cast( s64, fracnum.digits.len );
  TrimLeadingZeros( fracnum );

  bignum_t neg_pow2, half, two, tmp0, tmp1, tmp2, tmp3;
  Init( neg_pow2, 100 );
  Init( half, 100 );
  Init( two, 100 );
  Init( tmp0, 100 );
  Init( tmp1, 100 );
  Init( tmp2, 100 );
  Init( tmp3, 100 );
  *AddBack( neg_pow2.digits ) = 1;
  neg_pow2.positive = 0;
  *AddBack( half.digits ) = 5;
  half.exp = -1;
  *AddBack( two.digits ) = 2;
  num2_t r;
  Init( r, 10 );
  u64 bit = 0;
  bool exactly_pow2 = 0;

  if( !IsZero( intnum ) ) {
    Forever {
      // we'll keep doubling neg_pow2 until we hit or go past our intnum.
      // this lets us determine the highest-set bit.
      // TODO: something smarter / quicker.
      Add( intnum, neg_pow2, tmp0 );
      if( IsZero( tmp0 ) ) {
        // our intnum is exactly a power of 2, so set that bit and quit.
        SetBit( r, bit, 1 );
        exactly_pow2 = 1;
        break;
      }
      if( !tmp0.positive ) {
        // we've gone past our intnum, so rollback one iter.
        SetBit( r, bit - 1, 1 );
        CopyNum( intnum, tmp3 );
        break;
      }
      CopyNum( tmp3, tmp0 ); // save for rollback
      Mul( neg_pow2, two, tmp0, tmp1, tmp2 );
      CopyNum( neg_pow2, tmp2 );
      bit += 1;
    }
    // now we know the high bit, and we've subtracted that value from our intnum.
    // walk down from the high bit, subtracting values from our intnum and setting bits.
    // TODO: don't recompute powers of 2!
    while( !exactly_pow2 ) {
      bit -= 1;
      if( !bit ) {
        // early exit for final bit.
        SetBit( r, bit, 1 );
        exactly_pow2 = 1;
        break;
      }
      Mul( neg_pow2, half, tmp0, tmp1, tmp2 );
      Add( intnum, tmp2, tmp0 );
      CopyNum( neg_pow2, tmp2 );
      if( IsZero( tmp0 ) ) {
        // our intnum is exactly a power of 2, so set that bit and quit.
        SetBit( r, bit, 1 );
        exactly_pow2 = 1;
        break;
      }
      if( !tmp0.positive ) {
        // keep walking down.
        continue;
      }
      CopyNum( intnum, tmp0 );
      SetBit( r, bit, 1 );
    }
    TrimLeadingZeros( r );
  }

  // TODO: we set the 24th bit of 'frac', even though we mask it out when forming the f32.
  u8 expu = 0;
  u32 frac = 0;

  u64 intexp = 0;
  if( r.bins.len ) {
    u64 leadingbitindex = 63 - _lzcnt_u64( r.bins.mem[ r.bins.len - 1 ] );
    intexp = leadingbitindex + 64 * ( r.bins.len - 1 );
    For( i, 0, MIN( 23, intexp ) + 1 ) {
      frac |= ( GetBit( r, intexp - i ) << ( 23 - i ) );
    }
  }

  // digits after the decimal place!
  // TODO: round to nearest ULP, instead of truncating at 23 bits.
  u32 frac2 = 0;
  For( i, intexp + 1, 23 ) {
    Mul( fracnum, two, tmp0, tmp1, tmp2 );
    CopyNum( fracnum, tmp2 );

    // throw away the integral part of the number.
    bool fracbit = 0;
    bool quit = 0;
    if( fracnum.exp >= 0 ) {
      fracnum.digits.len = 0;
      fracnum.positive = 1;
      fracbit = 1;
      quit = 1;
    } else {
      // 12345e-3  ->  12.345
      Fori( s64, j, -fracnum.exp, Cast( s64, fracnum.digits.len ) ) {
        if( fracnum.digits.mem[j] != 0 ) {
          fracbit = 1;
          fracnum.digits.len = Cast( idx_t, -fracnum.exp );
        }
      }
    }

    frac2 |= fracbit << ( 23 - i );
    if( quit ) {
      break;
    }
  }

  // add integral part and fractional part.
  frac += frac2;

  if( frac != 0 ) {
    expu = Cast( u8, intexp + 127 );

    // TODO: this left shift is causing us to lose some precision.
    //
    while( !( frac & ( 1 << 23 ) ) ) {
      frac = frac << 1;
      expu -= 1;
    }
  }


  Kill( r );
  Kill( tmp3 );
  Kill( tmp2 );
  Kill( tmp1 );
  Kill( tmp0 );
  Kill( two );
  Kill( half );
  Kill( neg_pow2 );
  Kill( intnum );
  Kill( fracnum );

  auto& dstu = *Cast( u32*, &dst );
  dstu |= ( expu << 23u );
  dstu |= ( frac & AllOnes( 23u ) );
  dstu |= ( negative << 31u );

  return 1;
}



RegisterTest([]()
{
  {
    f32 d;
    bool r;
//    r = CsToFloat32( Str( "1.0" ), 3, d );
//    r = CsToFloat32( Str( "3.50" ), 4, d );
//    r = CsToFloat32( Str( "-123.456" ), 8, d );
//    r = CsToFloat32( Str( "-123.456e20" ), 11, d );
    r = CsToFloat32( Str( "12.345e-4" ), 9, d );
    r = CsToFloat32( Str( "-123.456e-20" ), 12, d );

//    u8 tmp[256];
//    idx_t len;
//    CsFromFloat32( tmp, _countof( tmp ), &len, 2 );
  }

  {
    bignum_t a;
    Init( a, 4 );
    *AddBack( a.digits ) = 1;
    *AddBack( a.digits ) = 2;
    *AddBack( a.digits ) = 3;
    *AddBack( a.digits ) = 4;

    bignum_t b;
    Init( b, 4 );
    b.exp = -2;
    *AddBack( b.digits ) = 5;
    *AddBack( b.digits ) = 5;

    bignum_t c;
    Init( c, 10 );
    Add( a, b, c );

    bignum_t d, e;
    Init( d, 10 );
    Init( e, 10 );
    Mul( a, b, d, e, c );

    u8 str[4096];
    idx_t size;
    bool r = CsFromFloat32( str, _countof( str ), &size, 1.234e-9f );
    AssertCrash( r );

    Kill( a );
    Kill( b );
    Kill( c );
    Kill( d );
    Kill( e );
  }
});
