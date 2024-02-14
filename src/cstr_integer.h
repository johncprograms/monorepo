// Copyright (c) John A. Carlos Jr., all rights reserved.

Templ Inl bool
CsFromIntegerU(
  u8* dst,
  idx_t dst_len,
  idx_t* dst_size,
  T src,
  bool use_separator = 0,
  u8 separator = ',',
  u8 separator_count = 3,
  u8 radix = 10,
  u8* digitmap = Str( "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" )
  )
{
  idx_t out_len = 0;
  u8 sep_count = 0;
  auto write = dst;
  if( !src ) {
    if( out_len + 1 > dst_len ) {
      return 0;
    }
    *write++ = digitmap[0];
    out_len += 1;
  } else {
    while( src ) {
      auto digit = Cast( u8, src % radix );
      src /= radix;
      if( use_separator ) {
        if( sep_count == separator_count ) {
          sep_count = 0;
          if( out_len + 1 > dst_len ) {
            return 0;
          }
          *write++ = separator;
          out_len += 1;
        }
        sep_count += 1;
      }
      if( out_len + 1 > dst_len ) {
        return 0;
      }
      *write++ = digitmap[digit];
      out_len += 1;
    }
    MemReverse( dst, out_len );
  }
  if( out_len + 1 > dst_len ) {
    return 0;
  }
  *write++ = 0;
  *dst_size = out_len;
  return 1;
}

Templ Inl bool
CsFromIntegerS(
  u8* dst,
  idx_t dst_len,
  idx_t* dst_size,
  T src,
  bool use_separator = 0,
  u8 separator = ',',
  u8 separator_count = 3,
  u8 radix = 10,
  u8* digitmap = Str( "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" )
  )
{
  idx_t out_len = 0;
  u8 sep_count = 0;
  auto write = dst;
  if( !src ) {
    if( out_len + 1 > dst_len ) {
      return 0;
    }
    *write++ = digitmap[0];
    out_len += 1;
  } else {
    bool write_negative = ( src < 0 );
    while( src ) {
      auto digit = ABS( Cast( s8, src % radix ) );
      src /= radix;
      if( use_separator ) {
        if( sep_count == separator_count ) {
          sep_count = 0;
          if( out_len + 1 > dst_len ) {
            return 0;
          }
          *write++ = separator;
          out_len += 1;
        }
        sep_count += 1;
      }
      if( out_len + 1 > dst_len ) {
        return 0;
      }
      *write++ = digitmap[digit];
      out_len += 1;
    }
    if( write_negative ) {
      if( out_len + 1 > dst_len ) {
        return 0;
      }
      *write++ = '-';
      out_len += 1;
    }
    MemReverse( dst, out_len );
  }
  if( out_len + 1 > dst_len ) {
    return 0;
  }
  *write++ = 0;
  *dst_size = out_len;
  return 1;
}


Templ Inl T
CsToIntegerU(
  u8* src,
  idx_t src_len,
  u8 ignore = ',',
  u8 radix = 10,
  u8* digitmap = Str( "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" )
  )
{
  T dst = 0;
  T digit_factor = 1;
  ReverseFor( i, 0, src_len ) {
    auto c = src[i];
    if( ignore  &&  c == ignore ) {
      continue;
    }
    // TODO: extended mapping.
    AssertCrash( '0' <= c  &&  c <= '9' );
    T digit = c - '0';
    dst += digit * digit_factor;
    digit_factor *= radix;
  }
  return dst;
}
Templ Inl void
CsToIntegerU(
  T* result,
  bool* success,
  u8* src,
  idx_t src_len,
  u8 ignore = ',',
  u8 radix = 10,
  u8* digitmap = Str( "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" )
  )
{
  *success = 0;
  *result = 0;
  T dst = 0;
  T digit_factor = 1;
  ReverseFor( i, 0, src_len ) {
    auto c = src[i];
    if( ignore  &&  c == ignore ) {
      continue;
    }
    // TODO: extended mapping.
    if( !( '0' <= c  &&  c <= '9' ) ) {
      return;
    }
    T digit = c - '0';
    dst += digit * digit_factor;
    digit_factor *= radix;
  }
  *success = 1;
  *result = dst;
}

Templ Inl T
CsToIntegerS(
  u8* src,
  idx_t src_len,
  u8 ignore = ',',
  u8 radix = 10,
  u8* digitmap = Str( "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" )
  )
{
  T dst = 0;
  T digit_factor = 1;
  ReverseFor( i, 0, src_len ) {
    auto c = src[i];
    if( ignore  &&  c == ignore ) {
      continue;
    }
    if( !i  &&  c == '-' ) {
      dst *= -1;
      continue;
    }
    // TODO: extended mapping.
    AssertCrash( '0' <= c  &&  c <= '9' );
    T digit = c - '0';
    dst += digit * digit_factor;
    digit_factor *= radix;
  }
  return dst;
}


Inl bool
CsFrom_u64( u8* dst, idx_t dst_len, idx_t* dst_size, u64 src, bool thousands_sep = 0 )
{
  return CsFromIntegerU( dst, dst_len, dst_size, src, thousands_sep );
}

Inl bool
CsFrom_u32( u8* dst, idx_t dst_len, idx_t* dst_size, u32 src, bool thousands_sep = 0 )
{
  return CsFromIntegerU( dst, dst_len, dst_size, src, thousands_sep );
}

Inl bool
CsFrom_u16( u8* dst, idx_t dst_len, idx_t* dst_size, u16 src, bool thousands_sep = 0 )
{
  return CsFromIntegerU( dst, dst_len, dst_size, src, thousands_sep );
}

Inl bool
CsFrom_u8( u8* dst, idx_t dst_len, idx_t* dst_size, u8 src, bool thousands_sep = 0 )
{
  return CsFromIntegerU( dst, dst_len, dst_size, src, thousands_sep );
}



Inl bool
CsFrom_s64( u8* dst, idx_t dst_len, idx_t* dst_size, s64 src, bool thousands_sep = 0 )
{
  return CsFromIntegerS( dst, dst_len, dst_size, src, thousands_sep );
}

Inl bool
CsFrom_s32( u8* dst, idx_t dst_len, idx_t* dst_size, s32 src, bool thousands_sep = 0 )
{
  return CsFromIntegerS( dst, dst_len, dst_size, src, thousands_sep );
}

Inl bool
CsFrom_s16( u8* dst, idx_t dst_len, idx_t* dst_size, s16 src, bool thousands_sep = 0 )
{
  return CsFromIntegerS( dst, dst_len, dst_size, src, thousands_sep );
}

Inl bool
CsFrom_s8( u8* dst, idx_t dst_len, idx_t* dst_size, s8 src, bool thousands_sep = 0 )
{
  return CsFromIntegerS( dst, dst_len, dst_size, src, thousands_sep );
}






Inl u64
CsTo_u64( u8* src, idx_t src_len, u8 radix = 10 )
{
  return CsToIntegerU<u64>( src, src_len, ',', radix );
}

Inl u32
CsTo_u32( u8* src, idx_t src_len, u8 radix = 10 )
{
  return CsToIntegerU<u32>( src, src_len, ',', radix );
}

Inl u16
CsTo_u16( u8* src, idx_t src_len, u8 radix = 10 )
{
  return CsToIntegerU<u16>( src, src_len, ',', radix );
}

Inl u8
CsTo_u8( u8* src, idx_t src_len, u8 radix = 10 )
{
  return CsToIntegerU<u8>( src, src_len, ',', radix );
}



Inl s64
CsTo_s64( u8* src, idx_t src_len, u8 radix = 10 )
{
  return CsToIntegerS<s64>( src, src_len, ',', radix );
}

Inl s32
CsTo_s32( u8* src, idx_t src_len, u8 radix = 10 )
{
  return CsToIntegerS<s32>( src, src_len, ',', radix );
}

Inl s16
CsTo_s16( u8* src, idx_t src_len, u8 radix = 10 )
{
  return CsToIntegerS<s16>( src, src_len, ',', radix );
}

Inl s8
CsTo_s8( u8* src, idx_t src_len, u8 radix = 10 )
{
  return CsToIntegerS<s8>( src, src_len, ',', radix );
}






Templ struct
test_cstr_t
{
  T value;
  u8* str;
};

Templ static void
ValidateU( test_cstr_t<T>& test )
{
  static u8 teststr[4096];
  idx_t size;
  CsFromIntegerU( teststr, _countof( teststr ), &size, test.value );
  AssertCrash( StringEquals( teststr, CstrLength( teststr ), test.str, CstrLength( teststr ), 1 ) );
  AssertCrash( size < _countof( teststr ) );
  T value = CsToIntegerU<T>( teststr, CstrLength( teststr ) );
  AssertCrash( value == test.value );
};

Templ static void
ValidateS( test_cstr_t<T>& test )
{
  static u8 teststr[4096];
  idx_t size;
  CsFromIntegerS( teststr, _countof( teststr ), &size, test.value );
  AssertCrash( StringEquals( teststr, CstrLength( teststr ), test.str, CstrLength( teststr ), 1 ) );
  AssertCrash( size < _countof( teststr ) );
  T value = CsToIntegerS<T>( teststr, CstrLength( teststr ) );
  AssertCrash( value == test.value );
};

RegisterTest([]()
{
  test_cstr_t<u8> cases_u8[] = {
    { 0, Str( "0" ) },
    { 128, Str( "128" ) },
    { MAX_u8, Str( "255" ) },
  };
  ForEach( test, cases_u8 ) {
    ValidateU( test );
  }
  test_cstr_t<u16> cases_u16[] = {
    { 0, Str( "0" ) },
    { 256, Str( "256" ) },
    { MAX_u16, Str( "65535" ) },
  };
  ForEach( test, cases_u16 ) {
    ValidateU( test );
  }
  test_cstr_t<u32> cases_u32[] = {
    { 0, Str( "0" ) },
    { 65536, Str( "65536" ) },
    { MAX_u32, Str( "4294967295" ) },
  };
  ForEach( test, cases_u32 ) {
    ValidateU( test );
  }
  test_cstr_t<u64> cases_u64[] = {
    { 0, Str( "0" ) },
    { 4294967296, Str( "4294967296" ) },
    { MAX_u64, Str( "18446744073709551615" ) },
  };
  ForEach( test, cases_u64 ) {
    ValidateU( test );
  }

  test_cstr_t<s8> cases_s8[] = {
    { MIN_s8, Str( "-128" ) },
    { -2, Str( "-2" ) },
    { 0, Str( "0" ) },
    { 2, Str( "2" ) },
    { MAX_s8, Str( "127" ) },
  };
  ForEach( test, cases_s8 ) {
    ValidateS( test );
  }
  test_cstr_t<s16> cases_s16[] = {
    { MIN_s16, Str( "-32768" ) },
    { MIN_s8 - 1, Str( "-129" ) },
    { 0, Str( "0" ) },
    { MAX_s8 + 1, Str( "128" ) },
    { MAX_s16, Str( "32767" ) },
  };
  ForEach( test, cases_s16 ) {
    ValidateS( test );
  }
  test_cstr_t<s32> cases_s32[] = {
    { MIN_s32, Str( "-2147483648" ) },
    { MIN_s16 - 1, Str( "-32769" ) },
    { 0, Str( "0" ) },
    { MAX_s16 + 1, Str( "32768" ) },
    { MAX_s32, Str( "2147483647" ) },
  };
  ForEach( test, cases_s32 ) {
    ValidateS( test );
  }
  test_cstr_t<s64> cases_s64[] = {
    { MIN_s64, Str( "-9223372036854775808" ) },
    { Cast( s64, MIN_s32 ) - 1, Str( "-2147483649" ) },
    { 0, Str( "0" ) },
    { Cast( s64, MAX_s32 ) + 1, Str( "2147483648" ) },
    { MAX_s64, Str( "9223372036854775807" ) },
  };
  ForEach( test, cases_s64 ) {
    ValidateS( test );
  }
});
