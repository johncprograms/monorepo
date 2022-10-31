// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// double-ended queue, aka deque.
//

#define DEQUERESIZEABLEPAGE   deque_resizeable_page_t<T, Allocation>
template< typename T, typename Allocation = allocation_heap_or_virtual_t >
struct
deque_resizeable_page_t
{
  DEQUERESIZEABLEPAGE* prev;
  DEQUERESIZEABLEPAGE* next;
  T* mem;
  Allocation allocn;
  idx_t head;
  idx_t tail;
  idx_t capacity;
  
  constant idx_t c_hdrlen = DEFAULT_ALIGN;
  CompileAssert( sizeof( QUEUERESIZEABLEPAGE ) <= c_hdrlen );
};

#define DEQUERESIZEABLEPAGELIST   deque_resizeable_pagelist_t<T, Allocator, Allocation>
TEA struct
deque_resizeable_pagelist_t
{
  // TODO: embed the first page in here?
  DEQUERESIZEABLEPAGE* head;
  DEQUERESIZEABLEPAGE* tail;
  idx_t default_page_capacity;
  Allocator alloc;
};

TEA ForceInl T*
PageMem( DEQUERESIZEABLEPAGE* page )
{
  auto pagebytes = Cast( u8*, page );
  auto pagemem = pagebytes + DEQUERESIZEABLEPAGE::c_hdrlen;
  return Cast( T*, pagemem );
}
TEA ForceInl void
FreePage( DEQUERESIZEABLEPAGELIST& q, DEQUERESIZEABLEPAGE* page )
{
  auto allocn = page->allocn;
  Free( q.alloc, allocn, page );
}
TEA ForceInl DEQUERESIZEABLEPAGE*
AllocatePage( DEQUERESIZEABLEPAGELIST& q, idx_t capacity )
{
  Allocation allocn = {};
  auto pagebytes = Allocate<u8>( q.alloc, allocn, DEQUERESIZEABLEPAGE::c_hdrlen + capacity * sizeof( T ) );
  auto page = Cast( DEQUERESIZEABLEPAGE*, pagebytes );
  auto pagemem = PageMem( page );
  page->allocn = allocn;
  page->prev = 0;
  page->next = 0;
  page->head = 0;
  page->tail = 0;
  page->capacity = capacity;
  return page;
}
TEA Inl void
Init( DEQUERESIZEABLEPAGELIST& q, idx_t capacity, Allocator alloc = {} )
{
  q.alloc = alloc;
  auto page = AllocatePage<T, Allocator, Allocation>( q, capacity );
  q.head = page;
  q.tail = page;
  q.default_page_capacity = capacity;
}
TEA Inl void
Kill( DEQUERESIZEABLEPAGELIST& q )
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
TEA Inl void
AddNewTailPage( DEQUERESIZEABLEPAGELIST& q, T* src, idx_t src_len )
{
  auto page = q.tail;
  auto capacity = q.default_page_capacity;
  auto newcap = MAX( 2 * capacity, src_len );
  q.default_page_capacity = newcap; // Update so the next new page is twice as big as the last.
  auto newpage = AllocatePage<T, Allocator, Allocation>( newcap );
  newpage->prev = page;
  page->next = newpage;
  q.tail = newpage;
  
  auto newpage_mem = PageMem( newpage );
  AddBackAssumingRoom( newpage_mem, newcap, newpage->head, &newpage->tail, src, src_len );
}
TEA Inl void
AddNewHeadPage( DEQUERESIZEABLEPAGELIST& q, T* src, idx_t src_len )
{
  auto page = q.head;
  auto capacity = q.default_page_capacity;
  auto newcap = MAX( 2 * capacity, src_len );
  q.default_page_capacity = newcap; // Update so the next new page is twice as big as the last.
  auto newpage = AllocatePage<T, Allocator, Allocation>( newcap );
  newpage->next = page;
  page->prev = newpage;
  q.head = newpage;
  
  auto newpage_mem = PageMem( newpage );
  AddFrontAssumingRoom( newpage_mem, newcap, newpage->head, &newpage->tail, src, src_len );
}
TEA Inl void
AddFront( DEQUERESIZEABLEPAGELIST& q, T* src, idx_t src_len )
{
  auto page = q.head;
  AssertCrash( page );
  auto page_mem = PageMem( page );
  auto capacity = page->capacity;
  auto head = page->head;
  auto tail = page->tail;
  auto len_remaining = RingbufferLenRemaining( capacity, head, tail );
  if( src_len <= len_remaining ) {
    // there's room trivially in the last page.
    AddFrontAssumingRoom( page_mem, capacity, &head, tail, src, src_len );
    page->head = head;
  }
  else {
    // there's not enough room in the last page. we need to fill the last page, and then make a new page for the rest of what we're enqueueing.
    AddFrontAssumingRoom( page_mem, capacity, &head, tail, src, len_remaining );
    page->head = head;
    auto num_in_newpage = src_len - len_remaining;
    AddNewHeadPage( q, src + len_remaining, num_in_newpage );
  }
}
TEA Inl void
AddBack( DEQUERESIZEABLEPAGELIST& q, T* src, idx_t src_len )
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
    AddBackAssumingRoom( page_mem, capacity, head, &tail, src, src_len );
    page->tail = tail;
  }
  else {
    // there's not enough room in the last page. we need to fill the last page, and then make a new page for the rest of what we're enqueueing.
    AddBackAssumingRoom( page_mem, capacity, head, &tail, src, len_remaining );
    page->tail = tail;
    auto num_in_newpage = src_len - len_remaining;
    AddNewTailPage( q, src + len_remaining, num_in_newpage );
  }
}
// TODO: shrinking option?
TEA Inl void
RemFront( DEQUERESIZEABLEPAGELIST& q, T* dst, idx_t dst_len )
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
    RemFrontAssumingRoom( page_mem, capacity, &head, tail, dst, num_dequeue );
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
TEA Inl void
RemBack( DEQUERESIZEABLEPAGELIST& q, T* dst, idx_t dst_len )
{
  auto page_tail = q.tail;
  auto page_head = q.head;
  AssertCrash( page_tail );
  AssertCrash( page_head );
  
  auto page = page_tail;
  while( dst_len ) {
    auto page_mem = PageMem( page );
    auto capacity = page->capacity;
    auto head = page->head;
    auto tail = page->tail;
    auto len = RingbufferLen( capacity, head, tail );
    auto num_dequeue = MIN( dst_len, len );
    AssertCrash( num_dequeue );
    RemBackAssumingRoom( page_mem, capacity, head, &tail, dst, num_dequeue );
    page->tail = tail;
    dst += num_dequeue;
    dst_len -= num_dequeue;
    if( len == num_dequeue  &&  page != page_head ) { // Leave the last page allocated and present for future use.
      auto prevpage = page->prev;
      if( prevpage ) {
        prevpage->next = 0;
      }
      page_tail = prevpage;
      
      FreePage( page );
    }
  }
  
  q.tail = page_tail;
}
