// Copyright (c) John A. Carlos Jr., all rights reserved.

#ifdef MAC
  #define _mm_pause() /*nothing*/
#endif

void _ReadWriteBarrier();

#ifdef MAC
  u32 InterlockedIncrement( volatile u32* x )
  {
    return __c11_atomic_fetch_add( Cast( _Atomic u32*, x ), 1u, Cast( int, std::memory_order_seq_cst ) ) + 1u;
  }
  u64 InterlockedIncrement( volatile u64* x )
  {
    return __c11_atomic_fetch_add( Cast( _Atomic u32*, x ), 1u, Cast( int, std::memory_order_seq_cst ) ) + 1u;
  }
  
  bool CAS( volatile u32* dst, u32 compare, u32 exchange )
  {
    return __c11_atomic_compare_exchange_strong( Cast( _Atomic u32*, dst ), &compare, exchange, Cast( int, std::memory_order_seq_cst ), Cast( int, std::memory_order_seq_cst ) );
  }
  bool CAS( volatile u64* dst, u64 compare, u64 exchange )
  {
    return __c11_atomic_compare_exchange_strong( Cast( _Atomic u64*, dst ), &compare, exchange, Cast( int, std::memory_order_seq_cst ), Cast( int, std::memory_order_seq_cst ) );
  }
#else

  #define CAS( dst, compare, exchange ) \
    ( compare == InterlockedCompareExchange( dst, exchange, compare ) )

#endif

#define GetValueBeforeAtomicInc( dst ) \
  ( InterlockedIncrement( dst ) - 1 )

#define GetValueAfterAtomicInc( dst ) \
  ( InterlockedIncrement( dst ) )


typedef volatile idx_t lock_t;

Inl void
Lock( lock_t* lock )
{
  while( !CAS( lock, 0, 1 ) ) {
    _mm_pause();
  }
}

Inl void
Unlock( lock_t* lock )
{
  *lock = 0;
  _ReadWriteBarrier();
}
