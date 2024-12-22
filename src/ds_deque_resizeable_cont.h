// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// double-ended queue, aka deque.
//

#define DEQUERESIZEABLECONT   deque_resizeable_cont_t<T>
Templ struct
deque_resizeable_cont_t
{
  T* mem;
  idx_t head;
  idx_t tail;
  idx_t capacity;
  alloctype_t allocn;
};
Templ Inl void
Init( DEQUERESIZEABLECONT& q, idx_t capacity )
{
  alloctype_t allocn = {};
  q.mem = Allocate<T>( &allocn, capacity );
  q.allocn = allocn;
  q.capacity = capacity;
  q.head = 0;
  q.tail = 0;
}
Templ Inl void
Kill( DEQUERESIZEABLECONT& q )
{
  Free( q.allocn, q.mem );
  q.mem = 0;
  q.capacity = 0;
  q.allocn = {};
  q.head = 0;
  q.tail = 0;
}
Templ Inl void
AddFront( DEQUERESIZEABLECONT& q, T* src, idx_t src_len )
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
  AddFrontAssumingRoom( q.mem, q.capacity, &q.head, q.tail, src, src_len );
}
Templ Inl void
AddBack( DEQUERESIZEABLECONT& q, T* src, idx_t src_len, idx_t* num_added )
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
  AddBackAssumingRoom( q.mem, q.capacity, q.head, &q.tail, src, src_len );
}
// TODO: shrinking option?
Templ Inl void
RemFront( DEQUERESIZEABLECONT& q, T* dst, idx_t dst_len, idx_t* num_removed )
{
  auto len = RingbufferLen( q.capacity, q.head, q.tail );
  auto num_remove = MIN( dst_len, len );
  RemFrontAssumingRoom( q.mem, q.capacity, &q.head, q.tail, dst, num_remove );
  *num_removed = num_remove;
}
Templ Inl void
RemBack( DEQUERESIZEABLECONT& q, T* dst, idx_t dst_len, idx_t* num_removed )
{
  auto len = RingbufferLen( q.capacity, q.head, q.tail );
  auto num_remove = MIN( dst_len, len );
  RemBackAssumingRoom( q.mem, q.capacity, q.head, &q.tail, dst, num_remove );
  *num_removed = num_remove;
}
