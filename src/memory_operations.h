// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 1

  // not a macro, due to having an overload that takes lengths for src0 and src1.
  Inl bool
  MemEqual( const void* src0, const void* src1, idx_t nbytes )
  {
    return ( memcmp( src0, src1, nbytes ) == 0 );
  }

  // we can't have Memmove take args, since it can take macro args, which don't evaluate in proper order.
  #define Memmove                         memmove
  #define Memzero( dst, nbytes )          memset( dst, 0, nbytes )
  #define Typezero( ptr )                 memset( ptr, 0, sizeof( *ptr ) )
  #define Arrayzero( carray )             memset( carray, 0, sizeof( carray ) )


#else

  // PERF: try inlining some loops via chunking, to avoid the loop decr cost.
  Inl bool
  MemEqual( const void* src0, const void* src1, idx_t nbytes )
  {
    if( src0 == src1 ) {
      return 1;
    } else {
      idx_t* aReg = Cast( idx_t*, src0 );
      idx_t* bReg = Cast( idx_t*, src1 );
      idx_t nReg = nbytes / sizeof( idx_t );
      while( nReg-- ) {
        if( *aReg++ != *bReg++ ) {
          return 0;
        }
      }
      u8* a8 = Cast( u8*, aReg );
      u8* b8 = Cast( u8*, bReg );
      idx_t n8 = nbytes % sizeof( idx_t );
      while( n8-- ) {
        if( *a8++ != *b8++ ) {
          return 0;
        }
      }
      return 1;
    }
  }

  // PERF: try inlining some loops via chunking, to avoid the loop decr cost.
  Inl void
  _MemcpyL( void* dst, const void* src, idx_t nbytes )
  {
    if( nbytes ) {
      idx_t* dstReg = Cast( idx_t*, dst );
      idx_t* srcReg = Cast( idx_t*, src );
      idx_t nReg = nbytes / sizeof( idx_t );
      while( nReg-- ) {
        *dstReg++ = *srcReg++;
      }
      u8* dst8 = Cast( u8*, dstReg );
      u8* src8 = Cast( u8*, srcReg );
      idx_t n8 = nbytes % sizeof( idx_t );
      while( n8-- ) {
        *dst8++ = *src8++;
      }
    }
  }

  // PERF: try inlining some loops via chunking, to avoid the loop decr cost.
  Inl void
  _MemcpyR( void* dst, const void* src, idx_t nbytes )
  {
    if( nbytes ) {
      dst = Cast( u8*, dst ) + nbytes;
      src = Cast( u8*, src ) + nbytes;
      u8* dst8 = Cast( u8*, dst );
      u8* src8 = Cast( u8*, src );
      idx_t n8 = nbytes % sizeof( idx_t );
      while( n8-- ) {
        *--dst8 = *--src8;
      }
      idx_t* dstReg = Cast( idx_t*, dst8 );
      idx_t* srcReg = Cast( idx_t*, src8 );
      idx_t nReg = nbytes / sizeof( idx_t );
      while( nReg-- ) {
        *--dstReg = *--srcReg;
      }
    }
  }


  Inl void
  Memmove( void* dst, const void* src, idx_t nbytes )
  {
    if( dst == src ) {
    } elif( ( src < dst )  &  ( Cast( u8*, src ) + nbytes > dst ) ) {
      _MemcpyR( dst, src, nbytes );
    } else {
      _MemcpyL( dst, src, nbytes );
    }
  }


  // PERF: try inlining some loops via chunking, to avoid the loop decr cost.
  Inl void
  Memzero( void* dst, idx_t nbytes )
  {
    if( nbytes ) {
      idx_t* dstReg = Cast( idx_t*, dst );
      idx_t zeroReg = 0;
      idx_t nReg = nbytes / sizeof( idx_t );
      while( nReg-- ) {
        *dstReg++ = zeroReg;
      }
      u8* dst8 = Cast( u8*, dstReg );
      u8 zero8 = 0;
      idx_t n8 = nbytes % sizeof( idx_t );
      while( n8-- ) {
        *dst8++ = zero8;
      }
    }
  }

#endif

Templ Inl void
TSet( T* src, idx_t src_len, T value )
{
  For( i, 0, src_len ) {
    src[i] = value;
  }
}

Templ Inl void
TMove( T* dst, T* src, idx_t num_elements )
{
  Memmove( dst, src, num_elements * sizeof( T ) );
}

Templ Inl void
TZero( T* dst, idx_t num_elements )
{
  Memzero( dst, num_elements * sizeof( T ) );
}


Inl bool
MemEqual( const void* src0, idx_t src0_len, const void* src1, idx_t src1_len )
{
  if( src0_len == src1_len ) {
    return MemEqual( src0, src1, src1_len );
  } else {
    return 0;
  }
}

Templ Inl bool
TContains( T* mem, idx_t len, T* val )
{
  For( i, 0, len ) {
    if( *Cast( T*, val ) == *Cast( T*, mem + i ) ) {
      return 1;
    }
  }
  return 0;
}


Inl bool
MemIsZero( const void* src, idx_t nbytes )
{
  auto srcR = Cast( idx_t*, src );
  auto numR = nbytes / sizeof( idx_t );
  while( numR-- ) {
    if( *srcR++ ) {
      return 0;
    }
  }
  auto src8 = Cast( u8*, srcR );
  auto num8 = nbytes % sizeof( idx_t );
  while( num8-- ) {
    if( *src8++ ) {
      return 0;
    }
  }
  return 1;
}


Inl void
MemReverse( u8* src, idx_t src_len )
{
  For( i, 0, src_len / 2 ) {
    SWAP( u8, src[i], src[src_len - i - 1] );
  }
}
Inl void
MemReverse( u32* src, idx_t src_len )
{
  For( i, 0, src_len / 2 ) {
    SWAP( u32, src[i], src[src_len - i - 1] );
  }
}
Inl void
MemReverse( u64* src, idx_t src_len )
{
  For( i, 0, src_len / 2 ) {
    SWAP( u64, src[i], src[src_len - i - 1] );
  }
}
Templ Inl void
TReverse( T* src, idx_t src_len )
{
  For( i, 0, src_len / 2 ) {
    SWAP( T, src[i], src[src_len - i - 1] );
  }
}

Inl bool
MemIdxScanR( idx_t* dst, const void* src, idx_t src_len, const void* key, idx_t elemsize )
{
  For( i, 0, src_len ) {
    bool equal = MemEqual( src, key, elemsize );
    if( equal ) {
      *dst = i;
      return 1;
    }
    src = Cast( u8*, src ) + elemsize;
  }
  return 0;
}

Inl bool
MemIdxScanL( idx_t* dst, const void* src, idx_t src_len, const void* key, idx_t elemsize )
{
  void* iter;
  idx_t i = src_len;
  while( i ) {
    i -= 1;
    iter = Cast( u8*, src ) + i * elemsize;
    bool equal = MemEqual( iter, key, elemsize );
    if( equal ) {
      *dst = i;
      return 1;
    }
  }
  return 0;
}

Templ Inl bool
TIdxScanR( idx_t* dst, T* mem, idx_t len, T val )
{
  For( i, 0, len ) {
    if( val == mem[i] ) {
      *dst = i;
      return 1;
    }
  }
  return 0;
}
Templ Inl bool
TIdxScanL( idx_t* dst, T* mem, idx_t len, T val )
{
  ReverseFor( i, 0, len ) {
    if( val == mem[i] ) {
      *dst = i;
      return 1;
    }
  }
  return 0;
}

Inl void*
MemScanR( const void* src, idx_t src_len, const void* key, idx_t elemsize )
{
  if( src_len ) {
    idx_t idx;
    bool found = MemIdxScanR( &idx, src, src_len, key, elemsize );
    if( found ) {
      void* res = Cast( u8*, src ) + idx * elemsize;
      return res;
    }
  }
  return 0;
}

Inl void*
MemScanL( const void* src, idx_t src_len, const void* key, idx_t elemsize )
{
  if( src_len ) {
    idx_t idx;
    bool found = MemIdxScanL( &idx, src, src_len, key, elemsize );
    if( found ) {
      void* res = Cast( u8*, src ) + idx * elemsize;
      return res;
    }
  }
  return 0;
}

#if defined(WIN)
  Inl void
  MemzeroAligned16( void* mem, idx_t len )
  {
    AssertCrash( !( Cast( idx_t, mem ) & 15 ) );

    auto mem256 = Cast( __m256i*, mem );
    auto copy256 = _mm256_set1_epi32( 0 );
    auto len256 = len / 32;
    For( i, 0, len256 ) {
      _mm256_stream_si256( mem256, copy256 );
      ++mem256;
    }
  }
#endif


// search { mem, len } for val.
// returns 'idx', the place where val would belong in sorted order. this is in the interval [ 0, len ].
//   '0' meaning val belongs before the element currently at index 0.
//   'len' meaning val belongs at the very end.
Templ Inl void
BinarySearch( T* mem, idx_t len, T val, idx_t* sorted_insert_idx )
{
  idx_t left = 0;
  auto middle = len / 2;
  auto right = len;
  Forever {
    if( left == right ) {
      *sorted_insert_idx = left;
      return;
    }
    auto mid = mem[middle];
    if( val < mid ) {
      // left stays put.
      right = middle;
      middle = left + ( right - left ) / 2;
    } elif( val > mid ) {
      left = middle;
      middle = left + ( right - left ) / 2;
      // right stays put.

      // out of bounds on right side.
      if( left == middle ) {
        *sorted_insert_idx = right;
        return;
      }
    } else {
      *sorted_insert_idx = middle;
      return;
    }
  }
}
