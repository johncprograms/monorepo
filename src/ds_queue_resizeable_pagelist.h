// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// fixed capacity, circular queues.
//

#define QUEUERESIZEABLEPAGE   queue_resizeable_page_t<T>
template< typename T >
struct
queue_resizeable_page_t
{
  alloctype_t allocn;
  QUEUERESIZEABLEPAGE* prev;
  QUEUERESIZEABLEPAGE* next;
  idx_t head;
  idx_t tail;
  idx_t capacity;
  
  constant idx_t c_hdrlen = DEFAULT_ALIGN;
  CompileAssert( sizeof( QUEUERESIZEABLEPAGE ) <= c_hdrlen );
};

#define QUEUERESIZEABLEPAGELIST   queue_resizeable_pagelist_t<T>
Templ struct
queue_resizeable_pagelist_t
{
  // TODO: embed the first page in here?
  QUEUERESIZEABLEPAGE* head;
  QUEUERESIZEABLEPAGE* tail;
  idx_t default_page_capacity;
};

Templ ForceInl T*
PageMem( QUEUERESIZEABLEPAGE* page )
{
  auto pagebytes = Cast( u8*, page );
  auto pagemem = pagebytes + QUEUERESIZEABLEPAGE::c_hdrlen;
  return Cast( T*, pagemem );
}
// Templ ForceInl idx_t
// PageLen( idx_t head, idx_t tail, idx_t capacity )
// {
//   auto len = head <= tail  ?  tail - head + 1  :  tail + 1 + ( capacity - head );
//   return len;
// }
Templ ForceInl void
FreePage( QUEUERESIZEABLEPAGELIST& q, QUEUERESIZEABLEPAGE* page )
{
  auto allocn = page->allocn;
  Free( allocn, page );
}
Templ ForceInl QUEUERESIZEABLEPAGE*
AllocatePage( QUEUERESIZEABLEPAGELIST& q, idx_t capacity )
{
  alloctype_t allocn = {};
  auto pagebytes = Allocate<u8>( allocn, QUEUERESIZEABLEPAGE::c_hdrlen + capacity * sizeof( T ) );
  auto page = Cast( QUEUERESIZEABLEPAGE*, pagebytes );
  auto pagemem = PageMem( page );
  page->allocn = allocn;
  page->prev = 0;
  page->next = 0;
  page->head = 0;
  page->tail = 0;
  page->capacity = capacity;
  return page;
}
Templ Inl void
Init( QUEUERESIZEABLEPAGELIST& q, idx_t initial_page_capacity )
{
  auto page = AllocatePage<T>( initial_page_capacity );
  q.default_page_capacity = initial_page_capacity;
  q.head = page;
  q.tail = page;
}
Templ Inl void
Kill( QUEUERESIZEABLEPAGELIST& q )
{
  auto page = q.head;
  while( page ) {
    auto next = page->next;
    FreePage( page );
    page = next;
  }
  q.head = 0;
  q.tail = 0;
}
Templ Inl void
EnqueueNewTailPage( QUEUERESIZEABLEPAGELIST& q, T* src, idx_t src_len )
{
  auto page = q.tail;
  auto capacity = q.default_page_capacity;
  auto newcap = MAX( 2 * capacity, src_len );
  q.default_page_capacity = newcap; // Update so the next new page is twice as big as the last.
  auto newpage = AllocatePage<T>( newcap );
  newpage->prev = page;
  page->next = newpage;
  q.tail = newpage;
  
  auto newpage_mem = PageMem( newpage );
  EnqueueAssumingRoom( newpage_mem, newcap, newpage->head, &newpage->tail, src, src_len );
}
Templ Inl void
Enqueue( QUEUERESIZEABLEPAGELIST& q, T* src, idx_t src_len )
{
  auto page = q.tail;
  AssertCrash( page );
  auto page_mem = PageMem( page );
  auto capacity = page->capacity;
  auto head = page->head;
  auto tail = page->tail;
  auto len_remaining = RingbufferLenRemaining( capacity, head, tail );
  if( src_len <= len_remaining ) {
    // there's room trivially in the last page.
    EnqueueAssumingRoom( page_mem, capacity, head, &tail, src, src_len );
    page->tail = tail;
  }
  else {
    // there's not enough room in the last page. we need to fill the last page, and then make a new page for the rest of what we're enqueueing.
    EnqueueAssumingRoom( page_mem, capacity, head, &tail, src, len_remaining );
    page->tail = tail;
    auto num_in_newpage = src_len - len_remaining;
    EnqueueNewTailPage( q, src + len_remaining, num_in_newpage );
  }
}
Templ Inl void
Dequeue( QUEUERESIZEABLEPAGELIST& q, T* dst, idx_t dst_len )
{
  auto page_tail = q.tail;
  auto page_head = q.head;
  AssertCrash( page_tail );
  AssertCrash( page_head );
  
  auto page = page_head;
  while( dst_len ) {
    auto page_mem = PageMem( page );
    auto capacity = page->capacity;
    auto head = page->head;
    auto tail = page->tail;
    auto len = RingbufferLen( capacity, head, tail );
    auto num_dequeue = MIN( dst_len, len );
    AssertCrash( num_dequeue );
    DequeueAssumingRoom( page_mem, capacity, &head, tail, dst, num_dequeue );
    page->head = head;
    dst += num_dequeue;
    dst_len -= num_dequeue;
    
    if( len == num_dequeue  &&  page != page_tail ) { // Leave the last page allocated and present for future use.
      auto nextpage = page->next;
      if( nextpage ) {
        nextpage->prev = 0;
      }
      page_head = nextpage;
      
      FreePage( page );
    }
  }
  
  q.head = page_head;
}


#define ITER_QUEUERESIZEABLEPAGELIST   iterator_queue_resizeable_page_t<T>
Templ struct
iterator_queue_resizeable_page_t
{
  QUEUERESIZEABLEPAGE* page;
  idx_t pos_in_page;
};
Templ Inl ITER_QUEUERESIZEABLEPAGELIST
Begin( QUEUERESIZEABLEPAGELIST* q )
{
  ITER_QUEUERESIZEABLEPAGELIST r;
  auto page = q->head;
  r.page = page;
  r.pos_in_page = page->head;
  return r;
}
Templ Inl ITER_QUEUERESIZEABLEPAGELIST
End( QUEUERESIZEABLEPAGELIST* q )
{
  ITER_QUEUERESIZEABLEPAGELIST r;
  auto page = q->tail;
  r.page = page;
  r.pos_in_page = page->tail;
  return r;
}
Templ Inl ITER_QUEUERESIZEABLEPAGELIST
Advance( ITER_QUEUERESIZEABLEPAGELIST iter, idx_t advance = 1 )
{
  ITER_QUEUERESIZEABLEPAGELIST r = iter;
  auto page = iter.page;
  auto pos_in_page = iter.pos_in_page;
  auto head = page->head;
  Forever {
    auto capacity = page->capacity;
    auto len = RingbufferLen( capacity, head, page->tail );
    if( advance < len ) {
      r.page = page;
      r.pos_in_page = ( pos_in_page + advance ) % capacity;
      return r;
    }
    advance -= len;
    page = page->next;
    head = page->head;
    pos_in_page = head;
  }
  UnreachableCrash();
}
