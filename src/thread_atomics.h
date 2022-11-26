// Copyright (c) John A. Carlos Jr., all rights reserved.

u32 InterlockedCompareExchange( volatile u32* dst, u32 exchange, u32 compare );
u64 InterlockedCompareExchange( volatile u64* dst, u64 exchange, u64 compare );
void _mm_pause();
void _ReadWriteBarrier();

#define CAS( dst, compare, exchange ) \
  ( compare == InterlockedCompareExchange( dst, exchange, compare ) )


u32 InterlockedIncrement( volatile u32* x );
u64 InterlockedIncrement( volatile u64* x );

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
