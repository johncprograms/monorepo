// Copyright (c) John A. Carlos Jr., all rights reserved.

// single-reader, single-writer circular queue, fixed elemsize.
Templ struct
mtqueue_srsw_t
{
  T* mem;
  volatile idx_t head;
  volatile idx_t tail;
  idx_t capacity;
};

Templ Inl void
Zero( mtqueue_srsw_t<T>& queue )
{
  queue.mem = 0;
  queue.head = 0;
  queue.tail = 0;
  queue.capacity = 0;
}

Templ Inl void
Alloc( mtqueue_srsw_t<T>& queue, idx_t size )
{
  Zero( queue );
  queue.mem = MemHeapAlloc( T, size );
  queue.capacity = size;
}

Templ Inl void
Free( mtqueue_srsw_t<T>& queue )
{
  MemHeapFree( queue.mem );
  Zero( queue );
}

Templ Inl void
EnqueueS( mtqueue_srsw_t<T>& queue, T* src, bool* success )
{
  idx_t local_wr = queue.tail;
  local_wr = ( local_wr + 1 ) % queue.capacity;
  if( local_wr == queue.head ) {
    *success = 0;
    return;
  }
  queue.mem[local_wr] = *src;
  _ReadWriteBarrier();
  queue.tail = local_wr;
  *success = 1;
}

Templ Inl void
DequeueS( mtqueue_srsw_t<T>& queue, T* dst, bool* success )
{
  idx_t local_rd = queue.head;
  if( local_rd == queue.tail ) {
    *success = 0;
    return;
  }
  local_rd = ( local_rd + 1 ) % queue.capacity;
  *dst = queue.mem[local_rd];
  _ReadWriteBarrier();
  queue.head = local_rd;
  *success = 1;
}
