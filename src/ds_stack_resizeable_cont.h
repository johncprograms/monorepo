// Copyright (c) John A. Carlos Jr., all rights reserved.

#define STACK   stack_resizeable_cont_t<T, Allocator, Allocation>

// meant to hold elements which are uniform in size.
TEA struct
stack_resizeable_cont_t
{
  T* mem;
  idx_t capacity; // # of elements mem can possibly hold.
  idx_t len; // # of elements in mem.
  Allocator alloc;
  Allocation allocn;
};

TEA Inl tslice_t<T>
SliceFromArray( STACK& stack )
{
  tslice_t<T> r;
  r.mem = stack.mem;
  r.len = stack.len;
  return r;
}

TEA Inl void
Zero( STACK& stack )
{
  stack.mem = 0;
  stack.capacity = 0;
  stack.len = 0;
  stack.alloc = {};
  stack.allocn = {};
}
TEA Inl void
ZeroContents( STACK& stack )
{
  Memzero( stack.mem, stack.len * sizeof( T ) );
}
TEA Inl void
Alloc( STACK& stack, idx_t nelems, Allocator alloc = {} )
{
  Zero( stack );
  stack.mem = Allocate<T>( alloc, stack.allocn, nelems );
  stack.capacity = nelems;
  stack.alloc = alloc;
}
TEA Inl void
Free( STACK& stack )
{
  AssertCrash( stack.len <= stack.capacity );
  if( stack.mem ) {
    Free( stack.alloc, stack.allocn, stack.mem );
  }
  Zero( stack );
}

template< typename T, typename Allocator, typename Allocation, typename Idx >
ForceInl void
Reserve( Allocator& alloc, Allocation& allocn, T*& mem, Idx* new_capacity_, Idx capacity, Idx len, Idx enforce_capacity )
{
  AssertCrash( capacity );
  AssertCrash( len <= capacity );
  if( capacity < enforce_capacity ) {
    auto new_capacity = 2 * capacity;
    while( new_capacity < enforce_capacity ) {
      new_capacity *= 2;
    }
    mem = Reallocate<T>( alloc, allocn, mem, capacity, new_capacity );
    capacity = new_capacity;
  }
  *new_capacity_ = capacity;
}
TEA Inl void
Reserve( STACK& stack, idx_t enforce_capacity )
{
  Reserve( stack.alloc, stack.allocn, stack.mem, &stack.capacity, stack.capacity, stack.len, enforce_capacity );
}

TEA Inl void
Copy( STACK& stack, STACK& src )
{
  AssertCrash( stack.len <= stack.capacity );
  Reserve( stack, src.len );
  Memmove( stack.mem, src.mem, src.len * sizeof( T ) );
  stack.len = src.len;
}

template< typename T, typename Allocator, typename Allocation, typename Idx >
ForceInl T*
AddBack( Allocator& alloc, Allocation& allocn, T*& mem, Idx& capacity, Idx& len, Idx nelems = 1 )
{
  AssertCrash( len <= capacity );
  Reserve( alloc, allocn, mem, &capacity, capacity, len, len + nelems );
  auto r = mem + len;
  len += nelems;
  return r;
}
TEA Inl T*
AddBack( STACK& stack, idx_t nelems = 1 )
{
  return AddBack( stack.alloc, stack.allocn, stack.mem, stack.capacity, stack.len, nelems );
}
TEA Inl T*
AddAt( STACK& stack, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( stack.len <= stack.capacity );
  AssertCrash( idx <= stack.len );
  Reserve( stack, stack.len + nelems );
  if( idx < stack.len ) {
    auto nshift = stack.len - idx;
    Memmove(
      stack.mem + idx + nelems,
      stack.mem + idx,
      sizeof( T ) * nshift
    );
  }
  auto r = stack.mem + idx;
  stack.len += nelems;
  return r;
}
TEA Inl void
RemBack( STACK& stack, idx_t nelems = 1 )
{
  AssertCrash( stack.len <= stack.capacity );
  AssertCrash( nelems <= stack.len );
  stack.len -= nelems;
}
TEA Inl void
RemAt( STACK& stack, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( stack.len <= stack.capacity );
  AssertCrash( idx + nelems <= stack.len );
  if( idx + nelems < stack.len ) {
    Memmove(
      stack.mem + idx,
      stack.mem + idx + nelems,
      sizeof( T ) * ( stack.len - idx - nelems )
      );
  }
  stack.len -= nelems;
}
TEA Inl void
UnorderedRemAt( STACK& stack, idx_t idx )
{
  idx_t nelems = 1; // TODO: rewrite for larger values?
  AssertCrash( stack.len <= stack.capacity );
  AssertCrash( idx + nelems <= stack.len );
  if( idx + nelems < stack.len ) {
    Memmove(
      stack.mem + idx,
      stack.mem + ( stack.len - nelems ),
      sizeof( T ) * nelems
      );
  }
  stack.len -= nelems;
}



#if defined(TEST)

static void
TestArray()
{
  idx_t indices[] = {
    10, 5, 7, 8, 1, 3, 5, 5000, 1221, 200, 0, 20, 2
  };
  idx_t values[] = {
    0, 1, 2, 255, 128, 50, MAX_idx, MAX_idx - 1, MAX_idx - 2, 100, 200, 222, MAX_idx / 2,
  };
  AssertCrash( _countof( indices ) == _countof( values ) );
  idx_t fill_count = _countof( indices );

  stack_resizeable_cont_t<idx_t> stack;
  idx_t sizes[] = {
    1, 2, 3, 4, 5, 8, 10, 16, 24, 1024, 65536, 100000,
  };
  ForEach( size, sizes ) {
    Alloc( stack, size );
    stack.len = size; // fake a full stack.
    For( i, 0, fill_count ) {
      auto idx = indices[i];
      auto value = values[i];
      if( idx < size ) {
        stack.mem[idx] = value;
        AssertCrash( *Cast( idx_t*, stack.mem + idx ) == value );
      }
    }

    auto onesize = stack.capacity;
    auto twosize = 2 * stack.capacity;
    Reserve( stack, 0 );
    AssertCrash( stack.capacity == onesize );
    Reserve( stack, 1 );
    AssertCrash( stack.capacity == onesize );
    Reserve( stack, stack.capacity - 1 );
    AssertCrash( stack.capacity == onesize );
    Reserve( stack, stack.capacity + 0 );
    AssertCrash( stack.capacity == onesize );
    Reserve( stack, stack.capacity + 1 );
    AssertCrash( stack.capacity == twosize );
    Reserve( stack, stack.capacity + 1 );
    AssertCrash( stack.capacity == 4 * onesize );

    Free( stack );

    auto EqualElems = []( stack_resizeable_cont_t<idx_t>& arr, idx_t* elemlist, idx_t size )
    {
      AssertCrash( arr.len == size );
      ForLen( i, arr ) {
        AssertCrash( arr.mem[i] == elemlist[i] );
      }
    };

    Alloc( stack, size );
    stack.len = 0;
    AssertCrash( stack.len == 0 );
    idx_t list[] = { 0, 1, 500, MAX_idx / 2, MAX_idx - 1, MAX_idx };
    ForEach( entry, list ) {
      *AddBack( stack ) = entry;
      AssertCrash( stack.capacity >= stack.len );
    }
    EqualElems( stack, list, _countof( list ) );

    Memmove( AddBack( stack, _countof( list ) ), list, sizeof( idx_t ) * _countof( list ) );
    ForLen( i, stack ) {
      AssertCrash( stack.mem[i] == list[i % _countof( list )] );
    }

    stack.len = 0;
    Memmove( AddBack( stack, _countof( list ) ), list, sizeof( idx_t ) * _countof( list ) );
    idx_t list0[] = { 0, 1, 10, 500, MAX_idx / 2, MAX_idx - 1, MAX_idx };
    *AddAt( stack, 2 ) = list0[2];
    EqualElems( stack, list0, _countof( list0 ) );

    idx_t list1[] = { 0, 1, 10, 500, MAX_idx / 2, MAX_idx - 1, MAX_idx, 0 };
    *AddAt( stack, stack.len ) = list1[ _countof( list1 ) - 1 ];
    EqualElems( stack, list1, _countof( list1 ) );

    idx_t list2[] = { 0, 1, 10, 500, 0, 1, 10, MAX_idx / 2, MAX_idx - 1, MAX_idx, 0 };
    Memmove( AddAt( stack, 4, 3 ), &list1[0], sizeof( idx_t ) * 3 );
    EqualElems( stack, list2, _countof( list2 ) );

    idx_t list3[] = { 0, 1, 10, 500, 0, 1, 10, MAX_idx / 2, MAX_idx - 1, MAX_idx };
    RemBack( stack );
    EqualElems( stack, list3, _countof( list3 ) );

    idx_t list4[] = { 0, 1, 10, 500, 0, 1, 10, MAX_idx / 2, MAX_idx - 1 };
    idx_t elem = stack.mem[stack.len - 1];
    RemBack( stack, 1 );
    EqualElems( stack, list4, _countof( list4 ) );
    AssertCrash( elem == MAX_idx );

    idx_t list5[] = { 0, 1, 10, 500, 0, 1 };
    idx_t elems[3];
    Memmove( elems, stack.mem + stack.len - 3, sizeof( idx_t ) * 3 );
    RemBack( stack, 3 );
    EqualElems( stack, list5, _countof( list5 ) );
    AssertCrash( elems[0] == 10 );
    AssertCrash( elems[1] == MAX_idx / 2 );
    AssertCrash( elems[2] == MAX_idx - 1 );

    idx_t list6[] = { 0, 1, 10, 500, 1 };
    RemAt( stack, 4 );
    EqualElems( stack, list6, _countof( list6 ) );

    idx_t list7[] = { 0, 10, 500, 1 };
    elem = stack.mem[1];
    RemAt( stack, 1 );
    EqualElems( stack, list7, _countof( list7 ) );
    AssertCrash( elem == 1 );

    idx_t list8[] = { 0 };
    Memmove( elems, stack.mem + 1, sizeof( idx_t ) * 3 );
    RemAt( stack, 1, 3 );
    EqualElems( stack, list8, _countof( list8 ) );
    AssertCrash( elems[0] == 10 );
    AssertCrash( elems[1] == 500 );
    AssertCrash( elems[2] == 1 );

    AssertCrash( stack.mem[0] == 0 );

    Free( stack );
  }
}

Inl void
TestBinSearch()
{
  {
    idx_t sorted[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    idx_t tests[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    idx_t sorted_insert_idx;
    For( i, 0, _countof( tests ) ) {
      auto test = tests[i];
      BinarySearch( AL( sorted ), test, &sorted_insert_idx );
      AssertCrash( test == sorted_insert_idx );
    }
  }

  {
    idx_t sorted[]   = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    idx_t tests[]    = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
    idx_t expected[] = { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9 };
    CompileAssert( _countof( tests ) == _countof( expected ) );
    idx_t sorted_insert_idx;
    For( i, 0, _countof( tests ) ) {
      auto test = tests[i];
      auto expect = expected[i];
      BinarySearch( AL( sorted ), test, &sorted_insert_idx );
      AssertCrash( sorted_insert_idx == expect );
    }
  }
}

#endif // defined(TEST)
