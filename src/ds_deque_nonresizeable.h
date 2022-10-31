// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// double-ended queue, aka deque.
//

template< typename Index >
Inl idx_t
RingbufferLenRemaining( Index capacity, Index head, Index tail )
{
  auto len_remaining = head <= tail  ?  head + capacity - ( tail + 1 )  :  head - tail + 1;
  return len_remaining;
}
template< typename Index >
Inl idx_t
RingbufferLen( Index capacity, Index head, Index tail )
{
  auto len = head <= tail  ?  tail - head + 1  :  tail + capacity - ( head + 1 );
  return len;
}

template< typename Index, typename Data >
Inl void
AddBackAssumingRoom(
  Data* mem,
  Index capacity,
  Index head,
  Index* tail,
  Data* src,
  Index src_len = 1
  )
{
  auto write = ( *tail + 1 ) % capacity;
  AssertCrash( write != head );
  auto num_write_to_end = MIN( src_len, capacity - write );
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
template< typename Index, typename Data >
Inl void
AddFrontAssumingRoom(
  Data* mem,
  Index capacity,
  Index* head_,
  Index tail,
  Data* src,
  Index src_len = 1
  )
{
  auto head = *head_;
  Fori( Index, i, 0, src_len ) {
    auto place = head  ?  ( head - 1 ) % capacity  :  capacity - 1;
    AssertCrash( place != tail );
    TMove( mem + place, src + i, 1 );
    head = place;
  }
  *head_ = head;
}
template< typename Index, typename Data >
Inl void
RemFrontAssumingRoom(
  Data* mem,
  Index capacity,
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
    auto num_read_to_end = capacity - head;
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
template< typename Index, typename Data >
Inl void
RemBackAssumingRoom(
  Data* mem,
  Index capacity,
  Index head,
  Index* tail_,
  Data* dst,
  Index dst_len = 1
  )
{
  auto tail = *tail_;
  Fori( Index, i, 0, dst_len ) {
    auto place = tail  ?  ( tail - 1 ) % capacity  :  capacity - 1;
    AssertCrash( place != head );
    TMove( mem + place, dst + i, 1 );
    tail = place;
  }
  *tail_ = tail;
}

#define DEQUENONRESIZEABLE   deque_nonresizeable_t<T, Allocator, Allocation>
TEA struct
deque_nonresizeable_t
{
  T* mem;
  idx_t head;
  idx_t tail;
  idx_t capacity;
  Allocation allocn;
  Allocator alloc;
};
TEA Inl void
Init( DEQUENONRESIZEABLE& q, idx_t capacity, Allocator alloc = {} )
{
  Allocation allocn = {};
  q.mem = Allocate<T>( alloc, allocn, capacity );
  q.alloc = alloc;
  q.allocn = allocn;
  q.capacity = capacity;
  q.head = 0;
  q.tail = 0;
}
TEA Inl void
Kill( DEQUENONRESIZEABLE& q )
{
  Free( q.alloc, q.allocn, q.mem );
  q.mem = 0;
  q.capacity = 0;
  q.allocn = {};
  q.head = 0;
  q.tail = 0;
}
TEA Inl void
AddFront( DEQUENONRESIZEABLE& q, T* src, idx_t src_len, idx_t* num_added )
{
  auto len_remaining = RingbufferLenRemaining( q.capacity, q.head, q.tail );
  auto num_add = MIN( src_len, len_remaining );
  AddFrontAssumingRoom( q.mem, q.capacity, &q.head, q.tail, src, num_add );
  *num_added = num_add;
}
TEA Inl void
RemFront( DEQUENONRESIZEABLE& q, T* dst, idx_t dst_len, idx_t* num_removed )
{
  auto len = RingbufferLen( q.capacity, q.head, q.tail );
  auto num_remove = MIN( dst_len, len );
  RemFrontAssumingRoom( q.mem, q.capacity, &q.head, q.tail, dst, num_remove );
  *num_removed = num_remove;
}
TEA Inl void
AddBack( DEQUENONRESIZEABLE& q, T* src, idx_t src_len, idx_t* num_added )
{
  auto len_remaining = RingbufferLenRemaining( q.capacity, q.head, q.tail );
  auto num_add = MIN( src_len, len_remaining );
  auto num_left_to_add = src_len - num_add;
  AddBackAssumingRoom( q.mem, q.capacity, q.head, &q.tail, src, num_add );
  *num_added = num_add;
}
TEA Inl void
RemBack( DEQUENONRESIZEABLE& q, T* dst, idx_t dst_len, idx_t* num_removed )
{
  auto len = RingbufferLen( q.capacity, q.head, q.tail );
  auto num_remove = MIN( dst_len, len );
  RemBackAssumingRoom( q.mem, q.capacity, q.head, &q.tail, dst, num_remove );
  *num_removed = num_remove;
}
