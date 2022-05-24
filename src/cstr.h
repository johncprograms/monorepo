// Copyright (c) John A. Carlos Jr., all rights reserved.


// TODO: look at void funcs and see if we can return a idx_t for # u8s processed ?


Inl u8
CsToLower( u8 c )
{
  return IsUpper( c )  ?  ( c + 32 )  :  c;
}

Inl u8
CsToUpper( u8 c )
{
  return IsLower( c )  ?  ( c - 32 )  :  c;
}

Inl idx_t
CsLen( u8* src )
{
  idx_t len = 0;
  while( *src++ )
    ++len;
  return len;
}

Inl idx_t CsLen( const u8* src ) { return CsLen( Cast( u8*, src ) ); }

Inl idx_t
CsLen( u8* src_start, u8* src_end )
{
  sidx_t src_len = src_end - src_start;
  AssertCrash( src_len >= 0 );
  return Cast( idx_t, src_len );
}

Inl idx_t
CsLenOrMax( u8* src, idx_t max )
{
  For( i, 0, max ) {
    if( !src[i] ) {
      return i;
    }
  }
  return max;
}


Inl u8*
AllocCstr( u8* mem, idx_t len )
{
  auto r = MemHeapAlloc( u8, len + 1 );
  Memmove( r, mem, len );
  r[len] = 0;
  return r;
}

Inl u8*
AllocCstr( slice_t& str )
{
  return AllocCstr( ML( str ) );
}

Inl u8*
AllocCstr( string_t& str )
{
  return AllocCstr( ML( str ) );
}



Inl slice_t
SliceFromCStr( void* str )
{
  slice_t r;
  r.mem = Cast( u8*, str );
  r.len = CsLen( r.mem );
  return r;
}

Inl slice32_t
Slice32FromCStr( void* str )
{
  slice32_t r;
  r.mem = Cast( u8*, str );
  auto len = CsLen( r.mem );
  AssertCrash( len <= MAX_u32 );
  r.len = Cast( u32, len );
  return r;
}

Inl void
AddBackCStr(
  array_t<u8>* array,
  void* cstr
  )
{
  auto text = SliceFromCStr( cstr );
  AddBackContents( array, text );
}

TemplTIdxN Inl void
AddBackCStr(
  embeddedarray_t<T, N>* array,
  void* cstr
  )
{
  CompileAssert(sizeof(T) == 1);
  auto text = SliceFromCStr( cstr );
  AddBackContents( array, text );
}



Inl idx_t
CsCount( u8* src, idx_t src_len, u8 key )
{
  idx_t count = 0;
  For( i, 0, src_len ) {
    if( src[i] == key )
      ++count;
  }
  return count;
}


Inl bool
CsEquals( u8* a, idx_t a_len, u8* b, idx_t b_len, bool case_sens )
{
  if( b_len != a_len )
    return 0;
  if( case_sens ) {
    For( i, 0, a_len ) {
      if( a[i] != b[i] )
        return 0;
    }
  } else {
    For( i, 0, a_len ) {
      if( CsToLower( a[i] ) != CsToLower( b[i] ) )
        return 0;
    }
  }
  return 1;
}

Inl idx_t
CsCount( u8* src, idx_t src_len, u8* key, idx_t key_len )
{
  idx_t count = 0;
  if( src_len < key_len )
    return count;
  For( i, 0, src_len - key_len ) {
    if( CsEquals( &src[i], key_len, key, key_len, 1 ) )
      ++count;
  }
  return count;
}


Inl idx_t
CsReplace( u8* dst, idx_t dst_len, u8 key, u8 val )
{
  idx_t nreplaced = 0;
  For( i, 0, dst_len ) {
    if( dst[i] == key ) {
      dst[i] = val;
      ++nreplaced;
    }
  }
  return nreplaced;
}



Inl void
CsToLower( u8* dst, idx_t dst_len )
{
  For( i, 0, dst_len )
    dst[i] = CsToLower( dst[i] );
}

Inl void
CsToUpper( u8* dst, idx_t dst_len )
{
  For( i, 0, dst_len )
    dst[i] = CsToUpper( dst[i] );
}


Inl void
CsCopy( u8* dst, u8 src )
{
  dst[0] = src;
  dst[1] = 0;
}

Inl void
CsCopy( u8* dst, u8* src, idx_t src_len )
{
  For( i, 0, src_len )
    *dst++ = src[i];
  *dst = 0;
}

Inl idx_t
CsCopy( u8* dst, u8* src_start, u8* src_end )
{
  idx_t ncpy = 0;
  u8* c = src_start;
  while( c != src_end ) {
    *dst++ = *c++;
    ++ncpy;
  }
  *dst = 0;
  return ncpy;
}



// C++ type inference for argument passing sucks ( esp. constants ), so we have to do this nonsense.

Templ Inl bool
_CsIdxScanR( T* dst, u8* src, T src_len, T start, u8 key )
{
  Fori( T, i, start, src_len ) {
    if( src[i] == key ) {
      *dst = i;
      return 1;
    }
  }
  return 0;
}
Inl bool
CsIdxScanR( u32* dst, u8* src, u32 src_len, u32 start, u8 key )
{
  return _CsIdxScanR( dst, src, src_len, start, key );
}
Inl bool
CsIdxScanR( u64* dst, u8* src, u64 src_len, u64 start, u8 key )
{
  return _CsIdxScanR( dst, src, src_len, start, key );
}

Templ Inl bool
_CsIdxScanR( T* dst, u8* src, T src_len, T start, u8* key, T key_len, bool case_sens, bool word_boundary )
{
  AssertCrash( start <= src_len );
  if( !key_len ) {
    return 1;
  }
  if( !src_len ) {
    return 0;
  }
  if( word_boundary ) {
    Fori( T, i, start, src_len ) {
      if( CsEquals( src + i, MIN( src_len - i, key_len ), key, key_len, case_sens ) ) {
        //
        // now we have a fullstring match!
        //
        // eliminate the fullstring match, if it fails the word_boundary test.
        // if we're searching "0123456789" for "234", the test is:
        //   keep-match <=> InWord( "1" ) != InWord( "2" ) && InWord( "4" ) != InWord( "5" )
        // i.e. the first and last chars in the match must have different "wordiness" than the chars outside.
        //
        bool found = 1;
        if( i > 0  &&  ( InWord( src[i - 1] ) == InWord( src[i] ) ) ) {
          found = 0;
        }
        if( i + key_len < src_len  &&  ( InWord( src[i + key_len - 1] ) == InWord( src[i + key_len] ) ) ) {
          found = 0;
        }
        if( found ) {
          *dst = i;
          return 1;
        }
      }
    }
  }
  else {
    // simpler code for non word_boundary.
    Fori( T, i, start, src_len ) {
      if( CsEquals( src + i, MIN( src_len - i, key_len ), key, key_len, case_sens ) ) {
        *dst = i;
        return 1;
      }
    }
  }
  return 0;
}
Inl bool
CsIdxScanR( u32* dst, u8* src, u32 src_len, u32 start, u8* key, u32 key_len, bool case_sens, bool word_boundary )
{
  return _CsIdxScanR( dst, src, src_len, start, key, key_len, case_sens, word_boundary );
}
Inl bool
CsIdxScanR( u64* dst, u8* src, u64 src_len, u64 start, u8* key, u64 key_len, bool case_sens, bool word_boundary )
{
  return _CsIdxScanR( dst, src, src_len, start, key, key_len, case_sens, word_boundary );
}

Templ Inl bool
_CsIdxScanL( T* dst, u8* src, T src_len, T start, u8 key )
{
  auto i = start + 1;
  while( i ) {
    --i;
    if( src[i] == key ) {
      *dst = i;
      return 1;
    }
  }
  return 0;
}
Inl bool
CsIdxScanL( u32* dst, u8* src, u32 src_len, u32 start, u8 key )
{
  return _CsIdxScanL( dst, src, src_len, start, key );
}
Inl bool
CsIdxScanL( u64* dst, u8* src, u64 src_len, u64 start, u8 key )
{
  return _CsIdxScanL( dst, src, src_len, start, key );
}

Templ Inl bool
_CsIdxScanL( T* dst, u8* src, T src_len, T start, u8* key, T key_len, bool case_sens, bool word_boundary )
{
  AssertCrash( start <= src_len );
  if( !key_len ) {
    return 1;
  }
  if( !src_len ) {
    return 0;
  }
  if( word_boundary ) {
    ReverseFori( T, i, 0, start + 1 ) {
      if( CsEquals( src + i, MIN( src_len - i, key_len ), key, key_len, case_sens ) ) {
        //
        // now we have a fullstring match!
        //
        // eliminate the fullstring match, if it fails the word_boundary test.
        // if we're searching "0123456789" for "234", the test is:
        //   keep-match <=> InWord( "1" ) != InWord( "2" ) && InWord( "4" ) != InWord( "5" )
        // i.e. the first and last chars in the match must have different "wordiness" than the chars outside.
        //
        bool found = 1;
        if( i > 0  &&  ( InWord( src[i - 1] ) == InWord( src[i] ) ) ) {
          found = 0;
        }
        if( i + key_len < src_len  &&  ( InWord( src[i + key_len - 1] ) == InWord( src[i + key_len] ) ) ) {
          found = 0;
        }
        if( found ) {
          *dst = i;
          return 1;
        }
      }
    }
  }
  else {
    // simpler code for non word_boundary.
    ReverseFori( T, i, 0, start + 1 ) {
      if( CsEquals( src + i, MIN( src_len - i, key_len ), key, key_len, case_sens ) ) {
        *dst = i;
        return 1;
      }
    }
  }
  return 0;
}
Inl bool
CsIdxScanL( u32* dst, u8* src, u32 src_len, u32 start, u8* key, u32 key_len, bool case_sens, bool word_boundary )
{
  return _CsIdxScanL( dst, src, src_len, start, key, key_len, case_sens, word_boundary );
}
Inl bool
CsIdxScanL( u64* dst, u8* src, u64 src_len, u64 start, u8* key, u64 key_len, bool case_sens, bool word_boundary )
{
  return _CsIdxScanL( dst, src, src_len, start, key, key_len, case_sens, word_boundary );
}

Inl u8*
CsScanR( u8* src, idx_t src_len, u8 key )
{
  For( i, 0, src_len ) {
    if( src[i] == key )
      return src + i;
  }
  return 0;
}

Inl u8*
CsScanR( u8* src, idx_t src_len, u8* key, idx_t key_len, bool case_sens, bool word_boundary )
{
  idx_t i;
  bool f = CsIdxScanR( &i, src, src_len, 0, key, key_len, case_sens, word_boundary );
  return f  ?  src + i  :  0;
}


Inl u8*
CsScanL( u8* src, idx_t src_len, u8 key )
{
  idx_t i = src_len;
  while( i ) {
    --i;
    if( src[i] == key )
      return &src[i];
  }
  return 0;
}

Inl u8*
CsScanL( u8* src, idx_t src_len, u8* key, idx_t key_len )
{
  ImplementCrash();
  return 0;
  //u8* forw = Scan( src, src_len, key, key_len );
  //u8* last = 0;
  //while( forw ) {
  //  last = forw;
  //  forw = Scan( forw + 1, key );
  //}
  //return last;
}



Inl void
CsAddBack( u8* dst, idx_t dst_len, u8 src )
{
  CsCopy( &dst[dst_len], src );
}

Inl void
CsAddBack( u8* dst, idx_t dst_len, u8* src, idx_t src_len )
{
  CsCopy( &dst[dst_len], src, src_len );
}

Inl idx_t
CsAddBack( u8* dst, idx_t dst_len, u8* src_start, u8* src_end )
{
  return CsCopy( &dst[dst_len], src_start, src_end );
}



Inl void
CsAddFront( u8* dst, idx_t dst_len, u8 src )
{
  u8* d1 = &dst[ dst_len + 1 ];
  u8* d2 = &dst[ dst_len + 2 ];
  For( i, 0, dst_len + 1 )
    *--d2 = *--d1;
  *--d2 = src;
}

Inl void
CsAddFront( u8* dst, idx_t dst_len, u8* src, idx_t src_len )
{
  if( !src_len )
    return;
  u8* d1 = &dst[ dst_len + 1 ];
  u8* d2 = &dst[ dst_len + 1 + src_len ];
  For( i, 0, dst_len + 1 )
    *--d2 = *--d1;
  u8* c = &src[ src_len ];
  For( i, 0, src_len )
    *--d2 = *--c;
}

Inl idx_t
CsAddFront( u8* dst, idx_t dst_len, u8* src_start, u8* src_end )
{
  idx_t src_len = CsLen( src_start, src_end );
  if( !src_len )
    return 0;
  u8* d1 = &dst[ dst_len + 1 ];
  u8* d2 = &dst[ dst_len + 1 + src_len ];
  For( i, 0, dst_len + 1 )
    *--d2 = *--d1;
  u8* c = &src_start[ src_len ];
  For( i, 0, src_len )
    *--d2 = *--c;
  return src_len;
}


Inl void
CsAddAt( u8* dst, idx_t dst_len, idx_t idx, u8 src )
{
  AssertCrash( idx <= dst_len );
  u8* d1 = &dst[ dst_len + 1 ];
  u8* d2 = &dst[ dst_len + 2 ];
  idx_t nshift = dst_len - idx;
  For( i, 0, nshift + 1 )
    *--d2 = *--d1;
  *--d2 = src;
}

Inl void
CsAddAt( u8* dst, idx_t dst_len, idx_t idx, u8* src, idx_t src_len )
{
  ImplementCrash();
}


Inl void
CsRemBack( u8* dst, u8* src, idx_t src_len, idx_t nchars )
{
  AssertCrash( nchars <= src_len );
  if( dst ) {
    For( i, 0, nchars )
      dst[i] = src[ src_len - nchars + i ];
    dst[nchars] = 0;
  }
  src[ src_len - nchars ] = 0;
}


Inl idx_t
CsRemBackTo( u8* dst, u8* src, idx_t src_len, u8 delim )
{
  idx_t idx;
  if( !CsIdxScanL( &idx, src, src_len, src_len - 1, delim ) )
    idx = 0;
  idx_t nrem = src_len - idx;
  CsRemBack( dst, src, src_len, nrem );
  return nrem;
}

Inl void
CsRemBackTo( u8* dst, u8* src, idx_t src_len, u8* delim, idx_t delim_len )
{
  idx_t idx;
  if( !CsIdxScanL( &idx, src, src_len, src_len - 1, delim, delim_len, 1, 0 ) )
    idx = 0;
  idx_t nrem = src_len - idx;
  CsRemBack( dst, src, src_len, nrem );
}


Inl void
CsRemFront( u8* dst, u8* src, idx_t src_len, idx_t nchars )
{
  AssertCrash( nchars <= src_len );
  if( dst )
    CsCopy( dst, src, nchars );
  u8* c = &src[nchars];
  For( i, 0, src_len - nchars )
    *src++ = *c++;
  *src = 0;
}


Inl idx_t
CsRemFrontTo( u8* dst, u8* src, idx_t src_len, u8 delim )
{
  idx_t idx;
  if( !CsIdxScanR( &idx, src, src_len, 0, delim ) )
    idx = src_len;
  CsRemFront( dst, src, src_len, idx );
  return idx;
}

Inl void
CsRemFrontTo( u8* dst, u8* src, idx_t src_len, u8* delim, idx_t delim_len )
{
  idx_t idx;
  if( !CsIdxScanR( &idx, src, src_len, 0, delim, delim_len, 1, 0 ) )
    idx = src_len;
  CsRemFront( dst, src, src_len, idx );
}


Inl void
CsRemAt( u8* dst, u8* src, idx_t src_len, idx_t idx, idx_t nchars )
{
  AssertCrash( idx < src_len );
  CsRemFront( dst, &src[idx], src_len - idx, nchars );
}



Inl idx_t
CsRemWhitespaceL( u8* dst, idx_t dst_len )
{
  idx_t nrem;
  for( nrem = 0;  nrem < dst_len;  ++nrem ) {
    if( !isspace( dst[nrem] ) )
      break;
  }
  if( !nrem )
    return 0;
  CsRemFront( 0, dst, dst_len, nrem );
  return nrem;
}

Inl idx_t
CsRemWhitespaceR( u8* dst, idx_t dst_len )
{
  idx_t nrem;
  for( nrem = 0;  nrem < dst_len;  ++nrem ) {
    if( !isspace( dst[ dst_len - nrem - 1 ] ) )
      break;
  }
  if( !nrem )
    return 0;
  CsRemBack( 0, dst, dst_len, nrem );
  return nrem;
}


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

Inl void
AddBackF64(
  array_t<u8>* array,
  f64 src,
  u8 num_decimal_places = 7
  )
{
  embeddedarray_t<u8, 32> tmp;
  CsFrom_f64( tmp.mem, Capacity( tmp ), &tmp.len, src, num_decimal_places );
  AddBackContents( array, SliceFromArray( tmp ) );
}
Inl void
AddBackF32(
  array_t<u8>* array,
  f32 src,
  u8 num_decimal_places = 7
  )
{
  embeddedarray_t<u8, 32> tmp;
  CsFrom_f32( tmp.mem, Capacity( tmp ), &tmp.len, src, num_decimal_places );
  AddBackContents( array, SliceFromArray( tmp ) );
}



Inl f64
CsTo_f64( u8* src, idx_t src_len )
{
  embeddedarray_t<u8, 64> tmp;
  Zero( tmp );
  Memmove( AddBack( tmp, src_len ), src, src_len );
  *AddBack( tmp ) = 0;
  return Cast( f64, atof( Cast( char*, tmp.mem ) ) );
}

Inl f32
CsTo_f32( u8* src, idx_t src_len )
{
  embeddedarray_t<u8, 64> tmp;
  Zero( tmp );
  Memmove( AddBack( tmp, src_len ), src, src_len );
  *AddBack( tmp ) = 0;
  return Cast( f32, atof( Cast( char*, tmp.mem ) ) );
}







#if 0 // TODO: use tls temp.
Inl void
Format( u8* dst, idx_t dstlen, u8* format, idx_t formatlen, ... )
{
  ImplementCrash();

  va_list args;
  va_start( args, formatlen );

  idx_t dstpos = 0;
  idx_t pos = 0;

  while( pos < formatlen ) {
    if( pos + 2 < formatlen ) {
      if( format[pos + 0] == '|'  &&  format[pos + 1] == '|' ) {
        if( dstpos < dstlen ) {
          dst[dstpos++] = '|';
          pos += 2;
        } else {
          break;
        }
      }
      if( format[pos + 0] == '|' ) {

      }
    }
  }

  va_end( args );
}
#endif

string_t
AllocString( void* cstr ... )
{
  string_t str;
  Alloc( str, MAX( 32768, 2 * CsLen( Str( cstr ) ) ) );

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

Templ Inl void
AddBackUInt(
  array_t<u8>* array,
  T src,
  bool use_separator = 0
  )
{
  embeddedarray_t<u8, 32> tmp;
  CsFromIntegerU( tmp.mem, Capacity( tmp ), &tmp.len, src, use_separator );
  AddBackContents( array, SliceFromArray( tmp ) );
}
Templ Inl void
AddBackSInt(
  array_t<u8>* array,
  T src,
  bool use_separator = 0
  )
{
  embeddedarray_t<u8, 32> tmp;
  CsFromIntegerS( tmp.mem, Capacity( tmp ), &tmp.len, src, use_separator );
  AddBackContents( array, SliceFromArray( tmp ) );
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
  if( !( src_len || IsNumber( *src ) || *src == '.' || *src == '-' || *src == '+' || *src == 'N' || *src == 'I' ) ) {
    return 0;
  }
  bool negative = ( *src == '-' );
  if( *src == '-' || *src == '+' ) {
    src += 1;
    src_len -= 1;
  }
  if( !( src_len || IsNumber( *src ) || *src == '.' || *src == 'N' || *src == 'I' ) ) {
    return 0;
  }
  if( CsEquals( src, src_len, Str( "NAN" ), 3, 1 ) ) {
    dst = ( negative  ?  -NAN  :  NAN );
    return 1;
  } elif( CsEquals( src, src_len, Str( "INF" ), 3, 1 ) ) {
    dst = ( negative  ?  -INFINITY  :  INFINITY );
    return 1;
  }
  if( !( IsNumber( *src ) || *src == '.' ) ) {
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
  kahan32_t d = {};
  For( i, 0, pos_exponent ) {
    auto half = src[i];
    if( half == '.' ) {
      continue;
    }
    if( !IsNumber( half ) ) {
      return 0;
    }
    auto digit = g_f32_from_digit[ AsNumber( half ) ];
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
    if( !( src_len || IsNumber( *src ) || *src == '-' || *src == '+' ) ) {
      return 0;
    }
    bool exp_negative = ( *src == '-' );
    if( *src == '-' || *src == '+' ) {
      src += 1;
      src_len -= 1;
    }
    if( !( src_len || IsNumber( *src ) ) ) {
      return 0;
    }
    auto exp = g_f32_from_digit[ AsNumber( *src ) ];
    while( src_len ) {
      src += 1;
      src_len -= 1;
      if( src_len ) {
        if( !IsNumber( *src ) ) {
          return 0;
        }
        exp *= 10.0f;
        exp += g_f32_from_digit[ AsNumber( *src ) ];
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
  array_t<u64> bins;
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
  array_t<u8> digits; // index 0 is 10s place, index 1 is 100s place, etc.
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
  if( !( src_len || IsNumber( *src ) || *src == '.' || *src == '-' || *src == '+' || *src == 'n' || *src == 'i' ) ) {
    return 0;
  }
  bool negative = ( *src == '-' );
  if( *src == '-' || *src == '+' ) {
    src += 1;
    src_len -= 1;
  }
  if( !( src_len || IsNumber( *src ) || *src == '.' || *src == 'n' || *src == 'i' ) ) {
    return 0;
  }
  if( CsEquals( src, src_len, Str( "nan" ), 3, 1 ) ) {
    dst = ( negative  ?  -NAN  :  NAN );
    return 1;
  } elif( CsEquals( src, src_len, Str( "inf" ), 3, 1 ) ) {
    dst = ( negative  ?  -INFINITY  :  INFINITY );
    return 1;
  }
  if( !( IsNumber( *src ) || *src == '.' ) ) {
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
        *AddBack( intnum.digits ) = AsNumber( src[i] );
      }
    }
    ReverseFori( s64, i, pos_abs + 1, Cast( s64, pos_exponent ) ) {
      *AddBack( fracnum.digits ) = AsNumber( src[i] );
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
      *AddBack( intnum.digits ) = AsNumber( src[i] );
    }
    ReverseFori( s64, i, pos_abs, Cast( s64, pos_exponent ) ) {
      if( i < 0 ) {
        *AddBack( fracnum.digits ) = 0;
      } elif( i == Cast( s64, pos_decimal ) ) {
        continue;
      } else {
        *AddBack( fracnum.digits ) = AsNumber( src[i] );
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





#define CURSORMOVE( _idx_type, _name, _cond ) \
  Inl _idx_type \
  NAMEJOIN( _name, L )( u8* src, _idx_type src_len, _idx_type pos ) \
  { \
    while( pos ) { \
      pos -= 1; \
      if( _cond( src[pos] ) ) { \
        pos += 1; \
        break; \
      } \
    } \
    return pos; \
  } \
  Inl _idx_type \
  NAMEJOIN( _name, R )( u8* src, _idx_type src_len, _idx_type pos ) \
  { \
    while( pos != src_len ) { \
      if( _cond( src[pos] ) ) { \
        break; \
      } \
      pos += 1; \
    } \
    return pos; \
  } \

#define COND( c )   ( c == '\r' )  ||  ( c == '\n' )
CURSORMOVE( u32, CursorStopAtNewline, COND );
CURSORMOVE( u64, CursorStopAtNewline, COND );
#undef COND

#define COND( c )   ( c == ' '  )  ||  ( c == '\t' )
CURSORMOVE( u32, CursorStopAtSpacetab, COND );
CURSORMOVE( u64, CursorStopAtSpacetab, COND );
#undef COND

#define COND( c )   ( c == ','  )
CURSORMOVE( u32, CursorStopAtComma, COND );
CURSORMOVE( u64, CursorStopAtComma, COND );
#undef COND

#define COND( c )   ( c != ' '  )  &&  ( c != '\t' )
CURSORMOVE( u32, CursorSkipSpacetab, COND );
CURSORMOVE( u64, CursorSkipSpacetab, COND );
#undef COND

#define COND( c )   ( !InWord( c ) )
CURSORMOVE( u32, CursorStopAtNonWordChar, COND );
CURSORMOVE( u64, CursorStopAtNonWordChar, COND );
#undef COND

#define COND( c )   ( InWord( c ) )
CURSORMOVE( u32, CursorStopAtWordChar, COND );
CURSORMOVE( u64, CursorStopAtWordChar, COND );
#undef COND

#undef CURSORMOVE

Templ Inl T
CursorInlineEnd(
  u8* line,
  T line_len,
  T pos
  )
{
  // note the first 'end' should take us to actual eol.
  // this is different than 'home' behavior, but it's what i tend to expect.
  auto eol = line_len;
  auto eol_whitespace = CursorSkipSpacetabL( line, line_len, eol );
  if( pos == eol ) {
    return eol_whitespace;
  }
  else {
    return eol;
  }
}

Templ Inl T
CursorInlineHome(
  u8* line,
  T line_len,
  T pos
  )
{
  // note the first 'home' should take us to the bol_whitespace.
  // this is different from 'end' behavior, but it's what i tend to expect.
  auto bol_whitespace = CursorSkipSpacetabR( line, line_len, 0 );
  if( pos == bol_whitespace ) {
    return 0;
  }
  else {
    return bol_whitespace;
  }
}


Inl idx_t
CursorCharL( u8* src, idx_t src_len, idx_t pos )
{
  if( !pos ) {
    return 0;
  } else {
    return pos - 1;
  }
}

Inl idx_t
CursorCharR( u8* src, idx_t src_len, idx_t pos )
{
  if( pos == src_len ) {
    return pos;
  } else {
    return pos + 1;
  }
}

Inl u32
CountNewlines(
  u8* src,
  idx_t src_len
  )
{
#if 0
  u32 count = 0;
  idx_t idx = 0;
  while( idx < src_len ) {
    while( idx < src_len  &&  ( src[idx] != '\r'  &&  src[idx] != '\n' ) ) {
      idx += 1;
    }
    // skip a single CR, CRLF, or LF
    if( idx + 1 < src_len  &&  MemEqual( src + idx, "\r\n", 2 ) ) {
      idx += 2;
      count += 1;
    }
    elif( idx < src_len  &&  src[idx] == '\r' ) {
      idx += 1;
      count += 1;
    }
    elif( idx < src_len  &&  src[idx] == '\n' ) {
      idx += 1;
      count += 1;
    }
  }
  return count;
#else
  u32 count = 0;
  auto last = src + src_len;
  Forever {
    if( src == last ) {
      break;
    }

    auto c = *src++;
    if( c == '\r' ) {
      count += 1;

      if( src != last  &&  *src == '\n' ) {
        src += 1;
      }
    }
    elif( c == '\n' ) {
      count += 1;
    }
  }
  return count;
#endif
}

Inl void
SplitIntoLines(
  array32_t<slice32_t>* lines,
  u8* src,
  idx_t src_len
  )
{
  idx_t idx = 0;
  idx_t num_cr = 0;
  idx_t num_lf = 0;
  idx_t num_crlf = 0;
  idx_t line_start = 0;
  Forever {
    if( idx == src_len ) {
      break;
    }

    auto c = src[idx];
    if( c == '\r' || c == '\n' ) {
      // found an eol, so emit a line.
      auto line_end = idx;
      auto line_len = line_end - line_start;
      auto line = AddBack( *lines );
      line->mem = src + line_start;
      AssertCrash( line_len <= MAX_u32 );
      line->len = Cast( u32, line_len );

      // now advance line_start past the eol for the next line.
      if( idx + 1 < src_len  &&  c == '\r'  &&  src[idx + 1] == '\n' ) {
        idx += 2;
        num_crlf += 1;
      }
      elif( c == '\r' ) {
        idx += 1;
        num_cr += 1;
      }
      else {
        idx += 1;
        num_lf += 1;
      }
      line_start = idx;
    }
    else {
      idx += 1;
    }
  }

  // emit the final line.
  {
    auto line_end = idx;
    auto line_len = line_end - line_start;
    auto line = AddBack( *lines );
    line->mem = src + line_start;
    AssertCrash( line_len <= MAX_u32 );
    line->len = Cast( u32, line_len );
  }
}

Inl void
SplitIntoLines(
  pagearray_t<slice32_t>* lines,
  u8* src,
  idx_t src_len,
  idx_t* num_cr_,
  idx_t* num_lf_,
  idx_t* num_crlf_
  )
{
  idx_t idx = 0;
  idx_t num_cr = 0;
  idx_t num_lf = 0;
  idx_t num_crlf = 0;
  idx_t line_start = 0;
  Forever {
    if( idx == src_len ) {
      break;
    }

    auto c = src[idx];
    if( c == '\r' || c == '\n' ) {
      // found an eol, so emit a line.
      auto line_end = idx;
      auto line_len = line_end - line_start;
      auto line = AddBack( *lines );
      line->mem = src + line_start;
      AssertCrash( line_len <= MAX_u32 );
      line->len = Cast( u32, line_len );

      // now advance line_start past the eol for the next line.
      if( idx + 1 < src_len  &&  c == '\r'  &&  src[idx + 1] == '\n' ) {
        idx += 2;
        num_crlf += 1;
      }
      elif( c == '\r' ) {
        idx += 1;
        num_cr += 1;
      }
      else {
        idx += 1;
        num_lf += 1;
      }
      line_start = idx;
    }
    else {
      idx += 1;
    }
  }

  // emit the final line.
  {
    auto line_end = idx;
    auto line_len = line_end - line_start;
    auto line = AddBack( *lines );
    line->mem = src + line_start;
    AssertCrash( line_len <= MAX_u32 );
    line->len = Cast( u32, line_len );
  }

  *num_cr_ = num_cr;
  *num_lf_ = num_lf;
  *num_crlf_ = num_crlf;
}

Inl idx_t
CountCommas(
  u8* src,
  idx_t src_len
  )
{
  idx_t count = 0;
  For( i, 0, src_len ) {
    if( src[i] == ',' ) {
      count += 1;
    }
  }
  return count;
}

Inl void
SplitByCommas(
  array_t<slice_t>* entries,
  u8* src,
  idx_t src_len
  )
{
  idx_t idx = 0;
  while( idx < src_len ) {
    auto elem_start = idx;
    auto elem_end = CursorStopAtCommaR( src, src_len, idx );
    auto elem_len = elem_end - elem_start;
    auto elem = AddBack( *entries );
    elem->mem = src + elem_start;
    elem->len = elem_len;
    idx = elem_end + 1;
    // final trailing comma.
    if( idx == src_len ) {
      elem = AddBack( *entries );
      elem->mem = src + idx;
      elem->len = 0;
      break;
    }
  }
}

struct
wordspan_t
{
  idx_t l;
  idx_t r;
  bool inword;
};
Inl void
SplitIntoWords(
  array_t<wordspan_t>* words,
  u8* src,
  idx_t src_len
  )
{
  idx_t idx = 0;
  while( idx < src_len ) {
    auto span = AddBack( *words );
    span->l = idx;
    auto word_end = CursorStopAtNonWordCharR( src, src_len, idx );
    auto nonword_end = CursorStopAtWordCharR( src, src_len, idx );
    AssertCrash( word_end != nonword_end );
    if( idx == word_end ) {
      // nonword.
      span->r = nonword_end;
      span->inword = 0;
      idx = nonword_end;
    }
    else {
      // word.
      span->r = word_end;
      span->inword = 1;
      idx = word_end;
    }
  }
}

Inl void
SplitBySpacesAndCopyContents(
  plist_t* dst_mem,
  array_t<slice_t>* dst,
  u8* src,
  idx_t src_len
  )
{
  Reserve( *dst, src_len / 2 );
  idx_t idx = 0;
  while( idx < src_len ) {
    auto elem_start = CursorSkipSpacetabR( src, src_len, idx );
    auto elem_end = CursorStopAtSpacetabR( src, src_len, elem_start );
    auto elem_len = elem_end - elem_start;
    if( !elem_len ) {
      break;
    }
    auto elem = AddBack( *dst );
    elem->mem = AddPlist( *dst_mem, u8, 1, elem_len );
    elem->len = elem_len;
    Memmove( elem->mem, src + elem_start, elem_len );
    idx = elem_end;
  }
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
  AssertCrash( CsEquals( teststr, CsLen( teststr ), test.str, CsLen( teststr ), 1 ) );
  AssertCrash( size < _countof( teststr ) );
  T value = CsToIntegerU<T>( teststr, CsLen( teststr ) );
  AssertCrash( value == test.value );
};

Templ static void
ValidateS( test_cstr_t<T>& test )
{
  static u8 teststr[4096];
  idx_t size;
  CsFromIntegerS( teststr, _countof( teststr ), &size, test.value );
  AssertCrash( CsEquals( teststr, CsLen( teststr ), test.str, CsLen( teststr ), 1 ) );
  AssertCrash( size < _countof( teststr ) );
  T value = CsToIntegerS<T>( teststr, CsLen( teststr ) );
  AssertCrash( value == test.value );
};

static void
TestCstr()
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

  {
    array32_t<slice32_t> lines;
    Alloc( lines, 1024 );
    SplitIntoLines( &lines, ML( SliceFromCStr( "abc\ndef\nghi\n\njkl\n" ) ) );
    AssertCrash( lines.len == 6 );
    AssertCrash( EqualContents( lines.mem[0], Slice32FromCStr( "abc" ) ) );
    AssertCrash( EqualContents( lines.mem[1], Slice32FromCStr( "def" ) ) );
    AssertCrash( EqualContents( lines.mem[2], Slice32FromCStr( "ghi" ) ) );
    AssertCrash( EqualContents( lines.mem[3], Slice32FromCStr( "" ) ) );
    AssertCrash( EqualContents( lines.mem[4], Slice32FromCStr( "jkl" ) ) );
    AssertCrash( EqualContents( lines.mem[5], Slice32FromCStr( "" ) ) );
    Free( lines );
  }

  {
    array_t<slice_t> entries;
    Alloc( entries, 1024 );
    SplitByCommas( &entries, ML( SliceFromCStr( "abc,def,ghi,,jkl," ) ) );
    AssertCrash( entries.len == 6 );
    AssertCrash( EqualContents( entries.mem[0], SliceFromCStr( "abc" ) ) );
    AssertCrash( EqualContents( entries.mem[1], SliceFromCStr( "def" ) ) );
    AssertCrash( EqualContents( entries.mem[2], SliceFromCStr( "ghi" ) ) );
    AssertCrash( EqualContents( entries.mem[3], SliceFromCStr( "" ) ) );
    AssertCrash( EqualContents( entries.mem[4], SliceFromCStr( "jkl" ) ) );
    AssertCrash( EqualContents( entries.mem[5], SliceFromCStr( "" ) ) );
    Free( entries );
  }

  {
    test_cstr_t<u32> cstrs[] = {
      { 0, Str( "" ) },
      { 0, Str( " " ) },
      { 1, Str( "\n" ) },
      { 1, Str( "\r" ) },
      { 1, Str( "\r\n" ) },
      { 2, Str( "\n\n" ) },
      { 2, Str( "\n\r" ) },
      { 2, Str( "\n\r\n" ) },
      { 2, Str( "\r\r" ) },
      { 2, Str( "\r\r\n" ) },
      { 2, Str( "\r\n\r" ) },
      { 2, Str( "\r\n\n" ) },
      { 2, Str( "\r\n\r\n" ) },
      };
    ForEach( cstr, cstrs ) {
      u32 count = CountNewlines( cstr.str, CsLen( cstr.str ) );
      AssertCrash( count == cstr.value );
    }
  }
}
