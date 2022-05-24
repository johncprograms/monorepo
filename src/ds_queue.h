// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// fixed capacity, circular queues.
//

template< typename Index, typename Data >
Inl void
Enqueue(
  Data* mem,
  Index capacity,
  Index* pos_wr,
  Index pos_rd,
  Data* src,
  bool* success
  )
{
  auto local_wr = *pos_wr;
  local_wr = ( local_wr + 1 ) % capacity;
  if( local_wr == pos_rd ) {
    *success = 0;
    return;
  }
  mem[local_wr] = *src;
  *pos_wr = local_wr;
  *success = 1;
}
template< typename Index, typename Data >
Inl void
EnqueueAssumingRoom(
  Data* mem,
  Index capacity,
  Index* pos_wr,
  Index pos_rd,
  Data* src,
  Index src_len = 1
  )
{
  auto local_wr = *pos_wr;
  local_wr = ( local_wr + 1 ) % capacity;
  AssertCrash( local_wr != pos_rd );
  // mem[local_wr] = *src;
  Memmove( mem + local_wr, src, src_len * sizeof( Data ) );
  *pos_wr = local_wr;
}

template< typename Index, typename Data >
Inl void
Dequeue(
  Data* mem,
  Index capacity,
  Index pos_wr,
  Index* pos_rd,
  Data* dst,
  bool* success
  )
{
  auto local_rd = *pos_rd;
  if( local_rd == pos_wr ) {
    *success = 0;
    return;
  }
  local_rd = ( local_rd + 1 ) % capacity;
  *dst = mem[local_rd];
  *pos_rd = local_rd;
  *success = 1;
}
template< typename Index, typename Data >
Inl void
DequeueAssumingRoom(
  Data* mem,
  Index capacity,
  Index pos_wr,
  Index* pos_rd,
  Data* dst
  )
{
  auto local_rd = *pos_rd;
  AssertCrash( local_rd != pos_wr );
  local_rd = ( local_rd + 1 ) % capacity;
  *dst = mem[local_rd];
  *pos_rd = local_rd;
}



Templ struct
queue_t
{
  T* mem;
  idx_t pos_rd;
  idx_t pos_wr;
  idx_t capacity;
};
Templ Inl void
Enqueue( queue_t<T>& q, T* src, bool* success )
{
  Enqueue( q.mem, q.pos_wr, q.pos_rd, q.capacity, src, success );
}
Templ Inl void
Dequeue( queue_t<T>& q, T* dst, bool* success )
{
  Dequeue( q.mem, q.pos_wr, &q.pos_rd, q.capacity, dst, success );
}
