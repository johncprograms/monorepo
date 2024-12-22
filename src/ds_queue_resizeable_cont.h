// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// fixed capacity, circular queues.
//

#define QUEUERESIZEABLECONT   queue_resizeable_cont_t<T>
Templ struct
queue_resizeable_cont_t
{
  T* mem;
  idx_t head;
  idx_t tail;
  idx_t capacity;
  idx_t len_used;
  alloctype_t allocn;
};
Templ Inl void
Init( QUEUERESIZEABLECONT& q, idx_t initial_capacity )
{
  q.mem = Allocate<T>( q.allocn, initial_capacity );
  q.capacity = initial_capacity;
  q.head = 0;
  q.tail = 0;
  q.len_used = 0;
}
Templ Inl void
Kill( QUEUERESIZEABLECONT& q )
{
  Free( q.mem );
  q.mem = 0;
  q.capacity = 0;
  q.head = 0;
  q.tail = 0;
  q.len_used = 0;
}
Templ ForceInl void
EnqueueAssumingRoom( QUEUERESIZEABLECONT& q, T* src, idx_t src_len )
{
  auto mem = q.mem;
  auto capacity = q.capacity;
  auto tail = q.tail;
  
  auto local_wr = ( tail + 1 ) % capacity;
  auto num_write_to_end = MIN( src_len, capacity - local_wr );
  TMove(
    mem + local_wr,
    src + 0,
    num_write_to_end
    );
  auto num_write_start = src_len - num_write_to_end;
  if( num_write_start ) {
    TMove(
      mem + 0,
      src + num_write_to_end,
      num_write_start
      );
    q.tail = num_write_start - 1;
  }
  else {
    q.tail = num_write_to_end;
  }
  
  q.len_used += src_len;
}
Templ Inl void
ResizeRingbuffer(
  T** mem_,
  idx_t* capacity_,
  alloctype_t& allocn,
  idx_t* head_,
  idx_t* tail_
  )
{
  auto head = *head_;
  auto tail = *tail_;
  auto oldlen = *capacity_;
  auto oldmem = *mem_;
  auto newlen = 2 * oldlen;
  alloctype_t newallocn = {};
  auto newmem = Allocate<T>( newallocn, newlen );
  if( head <= tail ) {
    TMove(
      newmem + 0,
      oldmem + head,
      tail - head + 1
      );
    *head_ = 0;
    *tail_ = tail - head;
  }
  else {
    auto num_write_to_end = oldlen - head;
    TMove(
      newmem + 0,
      oldmem + head,
      num_write_to_end
      );
    auto num_write_start = tail + 1;
    TMove(
      newmem + num_write_to_end,
      oldmem + 0,
      num_write_start
      );
    *head_ = 0;
    *tail_ = oldlen - 1;
  }
  Free( allocn, oldmem );
  *mem_ = newmem;
  *capacity_ = newlen;
}
Templ Inl void
Enqueue( QUEUERESIZEABLECONT& q, T* src, idx_t src_len )
{
  auto len_remaining = RingbufferLenRemaining( q.capacity, q.head, q.tail );
  if( len_remaining < src_len ) {
    // not enough room, we have to resize.
    ResizeRingbuffer(
      &q.mem,
      &q.capacity,
      q.allocn,
      &q.head,
      &q.tail
      );
  }
  EnqueueAssumingRoom( q, src, src_len );
}
/*Templ Inl void
DequeueOne( QUEUERESIZEABLECONT& q, T* dst, bool* success )
{
  auto local_rd = q.head;
  if( local_rd == q.tail ) {
    *success = 0;
    return;
  }
  local_rd = ( local_rd + 1 ) % q.capacity;
  *dst = q.mem[local_rd];
  q.head = local_rd;
  *success = 1;
}*/
Templ Inl void
Dequeue( QUEUERESIZEABLECONT& q, T* dst, idx_t dst_len )
{
  AssertCrash( dst_len <= q.len_used );
  auto mem = q.mem;
  auto capacity = q.capacity;
  auto head = q.head;
  auto tail = q.tail;
  if( head <= tail ) {
    TMove(
      dst + 0,
      mem + head,
      dst_len
      );
    q.head = head + dst_len;
  }
  else {
    auto num_read_to_end = capacity - head;
    if( dst_len <= num_read_to_end ) {
      TMove(
        dst + 0,
        mem + head,
        num_read_to_end
        );
      q.head = head + dst_len;
    }
    else {
      TMove(
        dst + 0,
        mem + head,
        num_read_to_end
        );
      auto num_read_start = dst_len - num_read_to_end;
      TMove(
        dst + num_read_to_end,
        mem + 0,
        num_read_start
        );
      q.head = num_read_start + 1;
    }
  }
}
