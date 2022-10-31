// Copyright (c) John A. Carlos Jr., all rights reserved.

void _ReadWriteBarrier();

// multi-reader, multi-writer circular queue, fixed elemsize.
Templ struct
mtqueue_mrmw_t
{
  T* mem;
  volatile idx_t head;
  volatile idx_t tail;
  volatile idx_t tail_advance;
  idx_t capacity;
};

Templ Inl void
Zero( mtqueue_mrmw_t<T>& queue )
{
  queue.mem = 0;
  queue.head = 0;
  queue.tail = 0;
  queue.tail_advance = 0;
  queue.capacity = 0;
}

Templ Inl void
Alloc( mtqueue_mrmw_t<T>& queue, idx_t size )
{
  Zero( queue );
  queue.mem = MemHeapAlloc( T, size );
  queue.capacity = size;
}

Templ Inl void
Free( mtqueue_mrmw_t<T>& queue )
{
  MemHeapFree( queue.mem  );
  Zero( queue );
}

Templ Inl void
EnqueueM( mtqueue_mrmw_t<T>& queue, T* src, bool* success )
{
  Forever {
    idx_t local_wr = queue.tail;
    idx_t local_wr_advance = queue.tail_advance;
    if( local_wr_advance != local_wr ) {
      continue;
    }
    idx_t new_wr = ( local_wr + 1 ) % queue.capacity;
    idx_t new_wr_advance = ( local_wr_advance + 1 ) % queue.capacity;
    if( new_wr == queue.head ) {
      *success = 0;
      return;
    }
    if( CAS( &queue.tail_advance, local_wr_advance, new_wr_advance ) ) {
      queue.mem[new_wr] = *src;
      _ReadWriteBarrier();
      AssertCrash( queue.tail == local_wr );
      queue.tail = new_wr;
      break;
    }
  }
  *success = 1;
}

Templ Inl void
DequeueM( mtqueue_mrmw_t<T>& queue, T* dst, bool* success )
{
  Forever {
    idx_t local_rd = queue.head;
    if( local_rd == queue.tail ) {
      *success = 0;
      return;
    }
    idx_t new_rd = ( local_rd + 1 ) % queue.capacity;
    *dst = queue.mem[new_rd];
    if( CAS( &queue.head, local_rd, new_rd ) ) {
      break;
    }
  }
  *success = 1;
}
