// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// fixed capacity, circular queues.
//

template< idx_t Capacity, typename Index, typename Data >
Inl void
Enqueue(
  Data* mem,
  Index* tail,
  Index head,
  Data* src,
  bool* success
  )
{
  auto local_wr = *tail;
  local_wr = ( local_wr + 1 ) % Capacity;
  if( local_wr == head ) {
    *success = 0;
    return;
  }
  mem[local_wr] = *src;
  *tail = local_wr;
  *success = 1;
}
template< idx_t Capacity, typename Index, typename Data >
ForceInl void
EnqueueAssumingRoom(
  Data* mem,
  Index* tail_,
  Index head,
  Data* src,
  idx_t src_len = 1
  )
{
  auto tail = *tail_;
  auto len_used = RingbufferLenRemaining<Capacity>( head, tail );
  AssertCrash( len_used + src_len <= Capacity );

  auto local_wr = ( tail + 1 ) % Capacity;
  auto num_write_to_end = MIN( src_len, Capacity - local_wr );
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
    *tail_ = num_write_start - 1;
  }
  else {
    *tail_ = num_write_to_end;
  }
}
//template< idx_t Capacity, typename Index, typename Data >
//Inl void
//EnqueueAssumingRoom(
//  Data* mem,
//  Index head,
//  Index* tail,
//  Data* src,
//  Index src_len = 1
//  )
//{
//  auto local_wr = *tail;
//  local_wr = ( local_wr + 1 ) % Capacity;
//  AssertCrash( local_wr != head );
//  // mem[local_wr] = *src;
//  Memmove( mem + local_wr, src, src_len * sizeof( Data ) );
//  *tail = local_wr;
//}

template< idx_t Capacity, typename Index, typename Data >
Inl void
Dequeue(
  Data* mem,
  Index tail,
  Index* head,
  Data* dst,
  bool* success
  )
{
  auto local_rd = *head;
  if( local_rd == tail ) {
    *success = 0;
    return;
  }
  local_rd = ( local_rd + 1 ) % Capacity;
  *dst = mem[local_rd];
  *head = local_rd;
  *success = 1;
}
template< idx_t Capacity, typename Index, typename Data >
Inl void
DequeueAssumingRoom(
  Data* mem,
  Index* head,
  Index tail,
  Data* dst
  )
{
  auto local_rd = *head;
  AssertCrash( local_rd != tail );
  local_rd = ( local_rd + 1 ) % Capacity;
  *dst = mem[local_rd];
  *head = local_rd;
}


#define QUEUE   queue_nonresizeable_stack_t<T, N>
TemplTIdxN struct
queue_nonresizeable_stack_t
{
  T mem[N];
  idx_t head;
  idx_t tail;
};
TemplTIdxN Inl void
Init( QUEUE& q )
{
  q.head = 0;
  q.tail = 0;
}
TemplTIdxN Inl void
Kill( QUEUE& q )
{
  q.head = 0;
  q.tail = 0;
}
TemplTIdxN Inl void
Enqueue( QUEUE& q, T* src, bool* success )
{
  Enqueue<N>( q.mem, q.tail, q.head, src, success );
}
TemplTIdxN Inl void
Dequeue( QUEUE& q, T* dst, bool* success )
{
  Dequeue<N>( q.mem, q.tail, &q.head, dst, success );
}

RegisterTest([]()
{

});

#undef QUEUE
