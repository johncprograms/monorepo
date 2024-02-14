// Copyright (c) John A. Carlos Jr., all rights reserved.


// TODO: look at void funcs and see if we can return idx_t for # u8s processed ?

#define AsciiIsSpaceTab( c )     ( ( ( c ) == ' '  ) || ( ( c ) == '\t' ) )
#define AsciiIsNewlineCh( c )    ( ( ( c ) == '\r' ) || ( ( c ) == '\n' ) )
#define AsciiIsWhitespace( c )   ( AsciiIsSpaceTab( c )  ||  AsciiIsNewlineCh( c ) )
#define AsciiIsNumber( c )       ( ( '0' <= ( c ) )  &&  ( ( c ) <= '9' ) )
#define AsciiIsLower( c )        ( ( 'a' <= ( c ) )  &&  ( ( c ) <= 'z' ) )
#define AsciiIsUpper( c )        ( ( 'A' <= ( c ) )  &&  ( ( c ) <= 'Z' ) )
#define AsciiIsAlpha( c )        ( AsciiIsLower( c )  ||  AsciiIsUpper( c ) )
#define AsciiInWord( c )         ( AsciiIsAlpha( c )  ||  AsciiIsNumber( c )  ||  ( c ) == '_' )
#define AsciiAsNumber( c )       ( ( c ) - '0' )


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


Inl idx_t
CstrLength( u8* src )
{
  idx_t len = 0;
  while( *src++ )
    ++len;
  return len;
}

Inl idx_t CstrLength( const u8* src ) { return CstrLength( Cast( u8*, src ) ); }

Inl idx_t
CstrLength( u8* src_start, u8* src_end )
{
  sidx_t src_len = src_end - src_start;
  AssertCrash( src_len >= 0 );
  return Cast( idx_t, src_len );
}

Inl idx_t
CstrLengthOrMax( u8* src, idx_t max )
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




Inl u8
ToLowerAscii( u8 c )
{
  return AsciiIsUpper( c )  ?  ( c + 32 )  :  c;
}

Inl u8
ToUpperAscii( u8 c )
{
  return AsciiIsLower( c )  ?  ( c - 32 )  :  c;
}


Inl idx_t
StringCount( u8* src, idx_t src_len, u8 key )
{
  idx_t count = 0;
  For( i, 0, src_len ) {
    if( src[i] == key )
      ++count;
  }
  return count;
}


Inl bool
StringEquals( u8* a, idx_t a_len, u8* b, idx_t b_len, bool case_sens )
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
      if( ToLowerAscii( a[i] ) != ToLowerAscii( b[i] ) )
        return 0;
    }
  }
  return 1;
}

Inl idx_t
StringCount( u8* src, idx_t src_len, u8* key, idx_t key_len )
{
  idx_t count = 0;
  if( src_len < key_len )
    return count;
  For( i, 0, src_len - key_len ) {
    if( StringEquals( &src[i], key_len, key, key_len, 1 ) )
      ++count;
  }
  return count;
}


Inl idx_t
StringReplaceAscii( u8* dst, idx_t dst_len, u8 key, u8 val )
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
StringToLowerAscii( u8* dst, idx_t dst_len )
{
  For( i, 0, dst_len )
    dst[i] = ToLowerAscii( dst[i] );
}

Inl void
StringToUpperAscii( u8* dst, idx_t dst_len )
{
  For( i, 0, dst_len )
    dst[i] = ToUpperAscii( dst[i] );
}


Inl void
CstrCopy( u8* dst, u8 src )
{
  dst[0] = src;
  dst[1] = 0;
}

// TODO: safety dst_len
Inl void
CstrCopy( u8* dst, u8* src, idx_t src_len )
{
  For( i, 0, src_len )
    *dst++ = src[i];
  *dst = 0;
}

// TODO: safety dst_len
Inl idx_t
CstrCopy( u8* dst, u8* src_start, u8* src_end )
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
_StringIdxScanR( T* dst, u8* src, T src_len, T start, u8 key )
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
StringIdxScanR( u32* dst, u8* src, u32 src_len, u32 start, u8 key )
{
  return _StringIdxScanR( dst, src, src_len, start, key );
}
Inl bool
StringIdxScanR( u64* dst, u8* src, u64 src_len, u64 start, u8 key )
{
  return _StringIdxScanR( dst, src, src_len, start, key );
}

Templ Inl bool
_StringIdxScanR( T* dst, u8* src, T src_len, T start, u8* key, T key_len, bool case_sens, bool word_boundary )
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
      if( StringEquals( src + i, MIN( src_len - i, key_len ), key, key_len, case_sens ) ) {
        //
        // now we have a fullstring match!
        //
        // eliminate the fullstring match, if it fails the word_boundary test.
        // if we're searching "0123456789" for "234", the test is:
        //   keep-match <=> AsciiInWord( "1" ) != AsciiInWord( "2" ) && AsciiInWord( "4" ) != AsciiInWord( "5" )
        // i.e. the first and last chars in the match must have different "wordiness" than the chars outside.
        //
        bool found = 1;
        if( i > 0  &&  ( AsciiInWord( src[i - 1] ) == AsciiInWord( src[i] ) ) ) {
          found = 0;
        }
        if( i + key_len < src_len  &&  ( AsciiInWord( src[i + key_len - 1] ) == AsciiInWord( src[i + key_len] ) ) ) {
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
      if( StringEquals( src + i, MIN( src_len - i, key_len ), key, key_len, case_sens ) ) {
        *dst = i;
        return 1;
      }
    }
  }
  return 0;
}
Inl bool
StringIdxScanR( u32* dst, u8* src, u32 src_len, u32 start, u8* key, u32 key_len, bool case_sens, bool word_boundary )
{
  return _StringIdxScanR( dst, src, src_len, start, key, key_len, case_sens, word_boundary );
}
Inl bool
StringIdxScanR( u64* dst, u8* src, u64 src_len, u64 start, u8* key, u64 key_len, bool case_sens, bool word_boundary )
{
  return _StringIdxScanR( dst, src, src_len, start, key, key_len, case_sens, word_boundary );
}

Templ Inl bool
_StringIdxScanL( T* dst, u8* src, T src_len, T start, u8 key )
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
StringIdxScanL( u32* dst, u8* src, u32 src_len, u32 start, u8 key )
{
  return _StringIdxScanL( dst, src, src_len, start, key );
}
Inl bool
StringIdxScanL( u64* dst, u8* src, u64 src_len, u64 start, u8 key )
{
  return _StringIdxScanL( dst, src, src_len, start, key );
}

Templ Inl bool
_StringIdxScanL( T* dst, u8* src, T src_len, T start, u8* key, T key_len, bool case_sens, bool word_boundary )
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
      if( StringEquals( src + i, MIN( src_len - i, key_len ), key, key_len, case_sens ) ) {
        //
        // now we have a fullstring match!
        //
        // eliminate the fullstring match, if it fails the word_boundary test.
        // if we're searching "0123456789" for "234", the test is:
        //   keep-match <=> AsciiInWord( "1" ) != AsciiInWord( "2" ) && AsciiInWord( "4" ) != AsciiInWord( "5" )
        // i.e. the first and last chars in the match must have different "wordiness" than the chars outside.
        //
        bool found = 1;
        if( i > 0  &&  ( AsciiInWord( src[i - 1] ) == AsciiInWord( src[i] ) ) ) {
          found = 0;
        }
        if( i + key_len < src_len  &&  ( AsciiInWord( src[i + key_len - 1] ) == AsciiInWord( src[i + key_len] ) ) ) {
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
      if( StringEquals( src + i, MIN( src_len - i, key_len ), key, key_len, case_sens ) ) {
        *dst = i;
        return 1;
      }
    }
  }
  return 0;
}
Inl bool
StringIdxScanL( u32* dst, u8* src, u32 src_len, u32 start, u8* key, u32 key_len, bool case_sens, bool word_boundary )
{
  return _StringIdxScanL( dst, src, src_len, start, key, key_len, case_sens, word_boundary );
}
Inl bool
StringIdxScanL( u64* dst, u8* src, u64 src_len, u64 start, u8* key, u64 key_len, bool case_sens, bool word_boundary )
{
  return _StringIdxScanL( dst, src, src_len, start, key, key_len, case_sens, word_boundary );
}

Inl u8*
StringScanR( u8* src, idx_t src_len, u8 key )
{
  For( i, 0, src_len ) {
    if( src[i] == key )
      return src + i;
  }
  return 0;
}

Inl u8*
StringScanR( u8* src, idx_t src_len, u8* key, idx_t key_len, bool case_sens, bool word_boundary )
{
  idx_t i;
  bool f = StringIdxScanR( &i, src, src_len, 0, key, key_len, case_sens, word_boundary );
  return f  ?  src + i  :  0;
}


Inl u8*
StringScanL( u8* src, idx_t src_len, u8 key )
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
StringScanL( u8* src, idx_t src_len, u8* key, idx_t key_len )
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
CstrAddBack( u8* dst, idx_t dst_len, u8 src )
{
  CstrCopy( &dst[dst_len], src );
}

Inl void
CstrAddBack( u8* dst, idx_t dst_len, u8* src, idx_t src_len )
{
  CstrCopy( &dst[dst_len], src, src_len );
}

Inl idx_t
CstrAddBack( u8* dst, idx_t dst_len, u8* src_start, u8* src_end )
{
  return CstrCopy( &dst[dst_len], src_start, src_end );
}



Inl void
CstrAddFront( u8* dst, idx_t dst_len, u8 src )
{
  u8* d1 = &dst[ dst_len + 1 ];
  u8* d2 = &dst[ dst_len + 2 ];
  For( i, 0, dst_len + 1 )
    *--d2 = *--d1;
  *--d2 = src;
}

Inl void
CstrAddFront( u8* dst, idx_t dst_len, u8* src, idx_t src_len )
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
CstrAddFront( u8* dst, idx_t dst_len, u8* src_start, u8* src_end )
{
  idx_t src_len = CstrLength( src_start, src_end );
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
CstrAddAt( u8* dst, idx_t dst_len, idx_t idx, u8 src )
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
CstrAddAt( u8* dst, idx_t dst_len, idx_t idx, u8* src, idx_t src_len )
{
  ImplementCrash();
}


Inl void
CstrRemBack( u8* dst, u8* src, idx_t src_len, idx_t nchars )
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
CstrRemBackTo( u8* dst, u8* src, idx_t src_len, u8 delim )
{
  idx_t idx;
  if( !StringIdxScanL( &idx, src, src_len, src_len - 1, delim ) )
    idx = 0;
  idx_t nrem = src_len - idx;
  CstrRemBack( dst, src, src_len, nrem );
  return nrem;
}

Inl void
CstrRemBackTo( u8* dst, u8* src, idx_t src_len, u8* delim, idx_t delim_len )
{
  idx_t idx;
  if( !StringIdxScanL( &idx, src, src_len, src_len - 1, delim, delim_len, 1, 0 ) )
    idx = 0;
  idx_t nrem = src_len - idx;
  CstrRemBack( dst, src, src_len, nrem );
}


Inl void
CstrRemFront( u8* dst, u8* src, idx_t src_len, idx_t nchars )
{
  AssertCrash( nchars <= src_len );
  if( dst )
    CstrCopy( dst, src, nchars );
  u8* c = &src[nchars];
  For( i, 0, src_len - nchars )
    *src++ = *c++;
  *src = 0;
}


Inl idx_t
CstrRemFrontTo( u8* dst, u8* src, idx_t src_len, u8 delim )
{
  idx_t idx;
  if( !StringIdxScanR( &idx, src, src_len, 0, delim ) )
    idx = src_len;
  CstrRemFront( dst, src, src_len, idx );
  return idx;
}

Inl void
CstrRemFrontTo( u8* dst, u8* src, idx_t src_len, u8* delim, idx_t delim_len )
{
  idx_t idx;
  if( !StringIdxScanR( &idx, src, src_len, 0, delim, delim_len, 1, 0 ) )
    idx = src_len;
  CstrRemFront( dst, src, src_len, idx );
}


Inl void
CstrRemAt( u8* dst, u8* src, idx_t src_len, idx_t idx, idx_t nchars )
{
  AssertCrash( idx < src_len );
  CstrRemFront( dst, &src[idx], src_len - idx, nchars );
}



Inl idx_t
CstrRemWhitespaceL( u8* dst, idx_t dst_len )
{
  idx_t nrem;
  for( nrem = 0;  nrem < dst_len;  ++nrem ) {
    if( !isspace( dst[nrem] ) )
      break;
  }
  if( !nrem )
    return 0;
  CstrRemFront( 0, dst, dst_len, nrem );
  return nrem;
}

Inl idx_t
CstrRemWhitespaceR( u8* dst, idx_t dst_len )
{
  idx_t nrem;
  for( nrem = 0;  nrem < dst_len;  ++nrem ) {
    if( !isspace( dst[ dst_len - nrem - 1 ] ) )
      break;
  }
  if( !nrem )
    return 0;
  CstrRemBack( 0, dst, dst_len, nrem );
  return nrem;
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

