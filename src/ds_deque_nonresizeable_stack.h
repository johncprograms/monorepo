// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// double-ended queue, aka deque.
//
// we use the convention that:
// - head points to the valid element that's at the head of the queue.
// - tail points to the valid element that's at the tail of the queue.
// that is, they're both inclusive bounds.
// and since it's a ringbuffer, we might have tail < head sometimes.
//

template< idx_t Capacity, typename Index >
Inl idx_t
RingbufferLenRemaining( Index head, Index tail )
{
  auto len_remaining = head <= tail  ?  head + Capacity - ( tail + 1 )  :  head - tail + 1;
  return len_remaining;
}
template< idx_t Capacity, typename Index >
Inl idx_t
RingbufferLen( Index head, Index tail )
{
  auto len = head <= tail  ?  tail - head + 1  :  tail + Capacity - ( head + 1 );
  return len;
}

template< idx_t Capacity, typename Index, typename Data >
Inl void
AddBackAssumingRoom(
  Data* mem,
  Index head,
  Index* tail,
  Data* src,
  Index src_len = 1
  )
{
  auto write = ( *tail + 1 ) % Capacity;
  AssertCrash( write != head );
  auto num_write_to_end = MIN( src_len, Capacity - write );
  TMove(
    mem + write,
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
    *tail = num_write_start - 1;
  }
  else {
    *tail = num_write_to_end;
  }
}
// We add [src, src_len] as if we're doing it one at a time, so the last src element gets pushed to the very front after this.
// TODO: configurable src ordering flag.
template< idx_t Capacity, typename Index, typename Data >
Inl void
AddFrontAssumingRoom(
  Data* mem,
  Index* head_,
  Index tail,
  Data* src,
  Index src_len = 1
  )
{
  auto head = *head_;
  Fori( Index, i, 0, src_len ) {
    auto place = head  ?  ( head - 1 ) % Capacity  :  Capacity - 1;
    AssertCrash( place != tail );
    TMove( mem + place, src + i, 1 );
    head = place;
  }
  *head_ = head;
}
template< idx_t Capacity, typename Index, typename Data >
Inl void
RemFrontAssumingRoom(
  Data* mem,
  Index* head_,
  Index tail,
  Data* dst,
  Index dst_len = 1
  )
{
  auto head = *head_;
  if( head <= tail ) {
    TMove(
      dst + 0,
      mem + head,
      dst_len
      );
    *head_ = head + dst_len;
  }
  else {
    auto num_read_to_end = Capacity - head;
    if( dst_len <= num_read_to_end ) {
      TMove(
        dst + 0,
        mem + head,
        num_read_to_end
        );
      *head_ = head + dst_len;
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
      *head_ = num_read_start + 1;
    }
  }
}
template< idx_t Capacity, typename Index, typename Data >
Inl void
RemBackAssumingRoom(
  Data* mem,
  Index head,
  Index* tail_,
  Data* dst,
  Index dst_len = 1
  )
{
  auto tail = *tail_;
  Fori( Index, i, 0, dst_len ) {
    auto place = tail  ?  ( tail - 1 ) % Capacity  :  Capacity - 1;
    AssertCrash( place != head );
    TMove( mem + place, dst + i, 1 );
    tail = place;
  }
  *tail_ = tail;
}

#define DEQUENONRESIZEABLESTACK   deque_nonresizeable_stack_t<T, N>
TemplTIdxN struct
deque_nonresizeable_stack_t
{
  T mem[N];
  idx_t head;
  idx_t tail;
};
TemplTIdxN Inl void
Init( DEQUENONRESIZEABLESTACK& q )
{
  q.head = 0;
  q.tail = 0;
}
TemplTIdxN Inl void
Kill( DEQUENONRESIZEABLESTACK& q )
{
  q.head = 0;
  q.tail = 0;
}
TemplTIdxN Inl void
AddFront( DEQUENONRESIZEABLESTACK& q, T* src, idx_t src_len, idx_t* num_added )
{
  auto len_remaining = RingbufferLenRemaining<N>( q.head, q.tail );
  auto num_add = MIN( src_len, len_remaining );
  AddFrontAssumingRoom<N>( q.mem, &q.head, q.tail, src, num_add );
  *num_added = num_add;
}
TemplTIdxN Inl void
RemFront( DEQUENONRESIZEABLESTACK& q, T* dst, idx_t dst_len, idx_t* num_removed )
{
  auto len = RingbufferLen<N>( q.head, q.tail );
  auto num_remove = MIN( dst_len, len );
  RemFrontAssumingRoom<N>( q.mem, &q.head, q.tail, dst, num_remove );
  *num_removed = num_remove;
}
TemplTIdxN Inl void
AddBack( DEQUENONRESIZEABLESTACK& q, T* src, idx_t src_len, idx_t* num_added )
{
  auto len_remaining = RingbufferLenRemaining<N>( q.head, q.tail );
  auto num_add = MIN( src_len, len_remaining );
  auto num_left_to_add = src_len - num_add;
  AddBackAssumingRoom<N>( q.mem, q.head, &q.tail, src, num_add );
  *num_added = num_add;
}
TemplTIdxN Inl void
RemBack( DEQUENONRESIZEABLESTACK& q, T* dst, idx_t dst_len, idx_t* num_removed )
{
  auto len = RingbufferLen<N>( q.head, q.tail );
  auto num_remove = MIN( dst_len, len );
  RemBackAssumingRoom<N>( q.mem, q.head, &q.tail, dst, num_remove );
  *num_removed = num_remove;
}
