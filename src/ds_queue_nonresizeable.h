// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// fixed capacity, circular queues.
//

template< typename Index, typename Data >
Inl void
Enqueue(
  Data* mem,
  Index capacity,
  Index* tail,
  Index head,
  Data* src,
  bool* success
  )
{
  auto local_wr = *tail;
  local_wr = ( local_wr + 1 ) % capacity;
  if( local_wr == head ) {
    *success = 0;
    return;
  }
  mem[local_wr] = *src;
  *tail = local_wr;
  *success = 1;
}
template< typename Index, typename Data >
Inl void
EnqueueAssumingRoom(
  Data* mem,
  Index capacity,
  Index head,
  Index* tail,
  Data* src,
  Index src_len = 1
  )
{
  auto local_wr = *tail;
  local_wr = ( local_wr + 1 ) % capacity;
  AssertCrash( local_wr != head );
  // mem[local_wr] = *src;
  TMove( mem + local_wr, src, src_len );
  *tail = local_wr;
}

template< typename Index, typename Data >
Inl void
Dequeue(
  Data* mem,
  Index capacity,
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
  local_rd = ( local_rd + 1 ) % capacity;
  *dst = mem[local_rd];
  *head = local_rd;
  *success = 1;
}
template< typename Index, typename Data >
Inl void
DequeueAssumingRoom(
  Data* mem,
  Index capacity,
  Index* head,
  Index tail,
  Data* dst,
  Index dst_len = 1
  )
{
  auto local_rd = *head;
  AssertCrash( local_rd != tail );
  local_rd = ( local_rd + 1 ) % capacity;
  // *dst = mem[local_rd];
  TMove( dst, mem + local_rd, dst_len );
  *head = local_rd;
}


#define QUEUE   queue_nonresizeable_t<T>
// requires:
// - Allocate<T>( Allocator&, idx_t )
// - Free( Allocator&, void* )
Templ struct
queue_nonresizeable_t
{
  T* mem;
  idx_t head;
  idx_t tail;
  idx_t capacity;
  alloctype_t allocn;
};
Templ Inl void
Init( QUEUE& q, idx_t initial_capacity )
{
  q.mem = Allocate<T>( &q.allocn, initial_capacity );
  q.capacity = initial_capacity;
  q.head = 0;
  q.tail = 0;
}
Templ Inl void
Kill( QUEUE& q )
{
  Free( q.allocn, q.mem );
  q.mem = 0;
  q.capacity = 0;
  q.head = 0;
  q.tail = 0;
}
Templ Inl void
Enqueue( QUEUE& q, T* src, bool* success )
{
  Enqueue( q.mem, q.tail, q.head, q.capacity, src, success );
}
Templ Inl void
Dequeue( QUEUE& q, T* dst, bool* success )
{
  Dequeue( q.mem, q.tail, &q.head, q.capacity, dst, success );
}

#undef QUEUE
