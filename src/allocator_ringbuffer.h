// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// idea: a ringbuffer that tries harder than usual to return a contiguous allocation, when
// you're pushing multiple elements at once.
//
// in particular, we have this two-region buffer, where b is initially empty:
// [ b | empty | a | empty ]
//
// pushes expand a to the right, unless there's not room for a contiguous region, in which case
// we'll try expanding b to the right. note that the b region always starts at idx=0.
//
// pops shrink a from the left, until it becomes empty. then a becomes the b region, and we continue.
//
Templ struct
rballoc_t
{
  T* mem;
  idx_t mem_len; // capacity of mem.
  // b_start is always 0, no need to store it
  idx_t b_end;
  idx_t a_start;
  idx_t a_end;
};
Templ Inl void
Init(
  rballoc_t<T>& t,
  idx_t initial_capacity
  )
{
  t.mem = MemHeapAlloc( T, initial_capacity );
  t.mem_len = initial_capacity;
  t.b_end = 0;
  t.a_start = 0;
  t.a_end = 0;
}
Templ Inl void
Kill( rballoc_t<T>& t )
{
  MemHeapFree( t.mem );
  t.mem = 0;
  t.mem_len = 0;
  t.b_end = 0;
  t.a_start = 0;
  t.a_end = 0;
}
Templ Inl T*
Allocate(
  rballoc_t<T>& t,
  idx_t alloc_size
  )
{
  auto mem = t.mem;
  auto mem_len = t.mem_len;
  auto b_end = t.b_end;
  auto a_start = t.a_start;
  auto a_end = t.a_end;

  //
  // note we have to switch to allocating from b once we've started it.
  // this is to preserve push/pop ordering.
  // this is a weakness of this design, particularly if remaining_a is large.
  //
  if( !b_end ) {
    auto remaining_a = mem_len - a_end;
    if( alloc_size <= remaining_a ) {
      auto r = mem + a_end;
      a_end += alloc_size;
      t.a_end = a_end;
      return r;
    }
  }

  auto remaining_b = a_start - b_end;
  if( alloc_size <= remaining_b ) {
    auto r = mem + b_end;
    b_end += alloc_size;
    t.b_end = b_end;
    return r;
  }

  return 0;
}
Templ Inl void
Free(
  rballoc_t<T>& t,
  idx_t alloc_size
  )
{
  auto b_end = t.b_end;
  auto a_start = t.a_start;
  auto a_end = t.a_end;

  auto a_len = a_end - a_start;
  if( alloc_size < a_len ) {
    a_start += alloc_size;
    t.a_start = a_start;
    return;
  }

  alloc_size -= a_len;
  AssertCrash( alloc_size <= b_end ); // over-pop
  t.a_start = alloc_size;
  t.a_end = b_end;
  t.b_end = 0;
}
