// Copyright (c) John A. Carlos Jr., all rights reserved.


// meant to hold elements which are uniform in size.
Templ struct
array_t
{
  T* mem;
  idx_t capacity; // # of elements mem can possibly hold.
  idx_t len; // # of elements in mem.
};
Templ struct
array32_t
{
  T* mem;
  u32 capacity;
  u32 len;
};




Inl slice_t
SliceFromArray( array_t<u8>& array )
{
  slice_t r;
  r.mem = array.mem;
  r.len = array.len;
  return r;
}
Templ Inl tslice_t<T>
SliceFromArray( array_t<T>& array )
{
  tslice_t<T> r;
  r.mem = array.mem;
  r.len = array.len;
  return r;
}
Templ Inl tslice32_t<T>
SliceFromArray( array32_t<T>& array )
{
  tslice32_t<T> r;
  r.mem = array.mem;
  r.len = array.len;
  return r;
}


Templ Inl void
Zero( array_t<T>& array )
{
  array.mem = 0;
  array.capacity = 0;
  array.len = 0;
}
Templ Inl void
Zero( array32_t<T>& array )
{
  array.mem = 0;
  array.capacity = 0;
  array.len = 0;
}
Templ Inl void
ZeroContents( array_t<T>& array )
{
  Memzero( array.mem, array.len * sizeof( T ) );
}
Templ Inl void
ZeroContents( array32_t<T>& array )
{
  Memzero( array.mem, array.len * sizeof( T ) );
}



Templ Inl void
Alloc( array_t<T>& array, idx_t nelems )
{
  AssertCrash( sizeof( T ) * nelems <= c_virtualalloc_threshold ); // You should use other data structures for large allocations!
  Zero( array );
  array.mem = MemHeapAlloc( T, nelems );
  array.capacity = nelems;
}
Templ Inl void
Alloc( array32_t<T>& array, u32 nelems )
{
  AssertCrash( sizeof( T ) * nelems <= c_virtualalloc_threshold ); // You should use other data structures for large allocations!
  Zero( array );
  array.mem = MemHeapAlloc( T, nelems );
  array.capacity = nelems;
}


Templ Inl void
Free( array_t<T>& array )
{
  AssertCrash( array.len <= array.capacity );
  if( array.mem ) {
    MemHeapFree( array.mem );
  }
  Zero( array );
}
Templ Inl void
Free( array32_t<T>& array )
{
  AssertCrash( array.len <= array.capacity );
  if( array.mem ) {
    MemHeapFree( array.mem );
  }
  Zero( array );
}


template< typename T, typename Idx> ForceInl void
Reserve( T*& mem, Idx* new_capacity_, Idx capacity, Idx len, Idx enforce_capacity )
{
  AssertCrash( capacity );
  AssertCrash( len <= capacity );
  AssertCrash( sizeof( T ) * enforce_capacity <= c_virtualalloc_threshold ); // You should use other data structures for large allocations!
  if( capacity < enforce_capacity ) {
    auto new_capacity = 2 * capacity;
    while( new_capacity < enforce_capacity ) {
      new_capacity *= 2;
    }
    mem = MemHeapRealloc( T, mem, capacity, new_capacity );
    capacity = new_capacity;
  }
  *new_capacity_ = capacity;
}
Templ Inl void
Reserve( array_t<T>& array, idx_t enforce_capacity )
{
  Reserve( array.mem, &array.capacity, array.capacity, array.len, enforce_capacity );
}
Templ Inl void
Reserve( array32_t<T>& array, u32 enforce_capacity )
{
  Reserve( array.mem, &array.capacity, array.capacity, array.len, enforce_capacity );
}



Templ Inl void
Copy( array_t<T>& array, array_t<T>& src )
{
  AssertCrash( array.len <= array.capacity );
  Reserve( array, src.len );
  Memmove( array.mem, src.mem, src.len * sizeof( T ) );
  array.len = src.len;
}
Templ Inl void
Copy( array32_t<T>& array, array32_t<T>& src )
{
  AssertCrash( array.len <= array.capacity );
  Reserve( array, src.len );
  Memmove( array.mem, src.mem, src.len * sizeof( T ) );
  array.len = src.len;
}


template< typename T, typename Idx> ForceInl T*
AddBack( T*& mem, Idx& capacity, Idx& len, Idx nelems = 1 )
{
  AssertCrash( len <= capacity );
  Reserve( mem, &capacity, capacity, len, len + nelems );
  auto r = mem + len;
  len += nelems;
  return r;
}
Templ Inl T*
AddBack( array_t<T>& array, idx_t nelems = 1 )
{
  return AddBack( array.mem, array.capacity, array.len, nelems );
}
Templ Inl T*
AddBack( array32_t<T>& array, u32 nelems = 1 )
{
  return AddBack( array.mem, array.capacity, array.len, nelems );
}


Templ Inl T*
AddAt( array_t<T>& array, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( array.len <= array.capacity );
  AssertCrash( idx <= array.len );
  Reserve( array, array.len + nelems );
  if( idx < array.len ) {
    auto nshift = array.len - idx;
    Memmove(
      array.mem + idx + nelems,
      array.mem + idx,
      sizeof( T ) * nshift
    );
  }
  auto r = array.mem + idx;
  array.len += nelems;
  return r;
}
Templ Inl T*
AddAt( array32_t<T>& array, u32 idx, u32 nelems = 1 )
{
  AssertCrash( array.len <= array.capacity );
  AssertCrash( idx <= array.len );
  Reserve( array, array.len + nelems );
  if( idx < array.len ) {
    auto nshift = array.len - idx;
    Memmove(
      array.mem + idx + nelems,
      array.mem + idx,
      sizeof( T ) * nshift
    );
  }
  auto r = array.mem + idx;
  array.len += nelems;
  return r;
}


Templ Inl void
RemBack( array_t<T>& array, idx_t nelems = 1 )
{
  AssertCrash( array.len <= array.capacity );
  AssertCrash( nelems <= array.len );
  array.len -= nelems;
}
Templ Inl void
RemBack( array32_t<T>& array, u32 nelems = 1 )
{
  AssertCrash( array.len <= array.capacity );
  AssertCrash( nelems <= array.len );
  array.len -= nelems;
}


Templ Inl void
RemAt( array_t<T>& array, idx_t idx, idx_t nelems = 1 )
{
  AssertCrash( array.len <= array.capacity );
  AssertCrash( idx + nelems <= array.len );
  if( idx + nelems < array.len ) {
    Memmove(
      array.mem + idx,
      array.mem + idx + nelems,
      sizeof( T ) * ( array.len - idx - nelems )
      );
  }
  array.len -= nelems;
}
Templ Inl void
RemAt( array32_t<T>& array, u32 idx, u32 nelems = 1 )
{
  AssertCrash( array.len <= array.capacity );
  AssertCrash( idx + nelems <= array.len );
  if( idx + nelems < array.len ) {
    Memmove(
      array.mem + idx,
      array.mem + idx + nelems,
      sizeof( T ) * ( array.len - idx - nelems )
      );
  }
  array.len -= nelems;
}


Templ Inl void
UnorderedRemAt( array_t<T>& array, idx_t idx )
{
  idx_t nelems = 1; // TODO: rewrite for larger values?
  AssertCrash( array.len <= array.capacity );
  AssertCrash( idx + nelems <= array.len );
  if( idx + nelems < array.len ) {
    Memmove(
      array.mem + idx,
      array.mem + ( array.len - nelems ),
      sizeof( T ) * nelems
      );
  }
  array.len -= nelems;
}
Templ Inl void
UnorderedRemAt( array32_t<T>& array, u32 idx )
{
  u32 nelems = 1; // TODO: rewrite for larger values?
  AssertCrash( array.len <= array.capacity );
  AssertCrash( idx + nelems <= array.len );
  if( idx + nelems < array.len ) {
    Memmove(
      array.mem + idx,
      array.mem + ( array.len - nelems ),
      sizeof( T ) * nelems
      );
  }
  array.len -= nelems;
}

// search { mem, len } for val.
// returns 'idx', the place where val would belong in sorted order. this is in the interval [ 0, len ].
//   '0' meaning val belongs before the element currently at index 0.
//   'len' meaning val belongs at the very end.
Templ Inl void
BinarySearch( T* mem, idx_t len, T val, idx_t* sorted_insert_idx )
{
  idx_t left = 0;
  auto middle = len / 2;
  auto right = len;
  Forever {
    if( left == right ) {
      *sorted_insert_idx = left;
      return;
    }
    auto mid = mem[middle];
    if( val < mid ) {
      // left stays put.
      right = middle;
      middle = left + ( right - left ) / 2;
    } elif( val > mid ) {
      left = middle;
      middle = left + ( right - left ) / 2;
      // right stays put.

      // out of bounds on right side.
      if( left == middle ) {
        *sorted_insert_idx = right;
        return;
      }
    } else {
      *sorted_insert_idx = middle;
      return;
    }
  }
}

Templ Inl bool
IdxScanR( idx_t* dst, T* mem, idx_t len, T val )
{
  For( i, 0, len ) {
    if( val == mem[i] ) {
      *dst = i;
      return 1;
    }
  }
  return 0;
}
Templ Inl bool
IdxScanL( idx_t* dst, T* mem, idx_t len, T val )
{
  ReverseFor( i, 0, len ) {
    if( val == mem[i] ) {
      *dst = i;
      return 1;
    }
  }
  return 0;
}

Templ Inl bool
ArrayContains( T* mem, idx_t len, T* val )
{
  For( i, 0, len ) {
    if( *Cast( T*, val ) == *Cast( T*, mem + i ) ) {
      return 1;
    }
  }
  return 0;
}

Inl void
AddBackContents(
  array_t<u8>* array,
  slice_t contents
  )
{
  Memmove( AddBack( *array, contents.len ), ML( contents ) );
}
Templ Inl void
AddBackContents(
  array_t<T>* array,
  tslice_t<T> contents
  )
{
  Memmove( AddBack( *array, contents.len ), contents.mem, contents.len * sizeof( T ) );
}
Templ Inl void
AddBackContents(
  array32_t<T>* array,
  tslice32_t<T> contents
  )
{
  Memmove( AddBack( *array, contents.len ), contents.mem, contents.len * sizeof( T ) );
}





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

  array_t<idx_t> array;
  idx_t sizes[] = {
    1, 2, 3, 4, 5, 8, 10, 16, 24, 1024, 65536, 100000,
  };
  ForEach( size, sizes ) {
    Alloc( array, size );
    array.len = size; // fake a full array.
    For( i, 0, fill_count ) {
      auto idx = indices[i];
      auto value = values[i];
      if( idx < size ) {
        array.mem[idx] = value;
        AssertCrash( *Cast( idx_t*, array.mem + idx ) == value );
      }
    }

    auto onesize = array.capacity;
    auto twosize = 2 * array.capacity;
    Reserve( array, 0 );
    AssertCrash( array.capacity == onesize );
    Reserve( array, 1 );
    AssertCrash( array.capacity == onesize );
    Reserve( array, array.capacity - 1 );
    AssertCrash( array.capacity == onesize );
    Reserve( array, array.capacity + 0 );
    AssertCrash( array.capacity == onesize );
    Reserve( array, array.capacity + 1 );
    AssertCrash( array.capacity == twosize );
    Reserve( array, array.capacity + 1 );
    AssertCrash( array.capacity == 4 * onesize );

    Free( array );

    auto EqualElems = []( array_t<idx_t>& arr, idx_t* elemlist, idx_t size )
    {
      AssertCrash( arr.len == size );
      ForLen( i, arr ) {
        AssertCrash( arr.mem[i] == elemlist[i] );
      }
    };

    Alloc( array, size );
    array.len = 0;
    AssertCrash( array.len == 0 );
    idx_t list[] = { 0, 1, 500, MAX_idx / 2, MAX_idx - 1, MAX_idx };
    ForEach( entry, list ) {
      *AddBack( array ) = entry;
      AssertCrash( array.capacity >= array.len );
    }
    EqualElems( array, list, _countof( list ) );

    Memmove( AddBack( array, _countof( list ) ), list, sizeof( idx_t ) * _countof( list ) );
    ForLen( i, array ) {
      AssertCrash( array.mem[i] == list[i % _countof( list )] );
    }

    array.len = 0;
    Memmove( AddBack( array, _countof( list ) ), list, sizeof( idx_t ) * _countof( list ) );
    idx_t list0[] = { 0, 1, 10, 500, MAX_idx / 2, MAX_idx - 1, MAX_idx };
    *AddAt( array, 2 ) = list0[2];
    EqualElems( array, list0, _countof( list0 ) );

    idx_t list1[] = { 0, 1, 10, 500, MAX_idx / 2, MAX_idx - 1, MAX_idx, 0 };
    *AddAt( array, array.len ) = list1[ _countof( list1 ) - 1 ];
    EqualElems( array, list1, _countof( list1 ) );

    idx_t list2[] = { 0, 1, 10, 500, 0, 1, 10, MAX_idx / 2, MAX_idx - 1, MAX_idx, 0 };
    Memmove( AddAt( array, 4, 3 ), &list1[0], sizeof( idx_t ) * 3 );
    EqualElems( array, list2, _countof( list2 ) );

    idx_t list3[] = { 0, 1, 10, 500, 0, 1, 10, MAX_idx / 2, MAX_idx - 1, MAX_idx };
    RemBack( array );
    EqualElems( array, list3, _countof( list3 ) );

    idx_t list4[] = { 0, 1, 10, 500, 0, 1, 10, MAX_idx / 2, MAX_idx - 1 };
    idx_t elem = array.mem[array.len - 1];
    RemBack( array, 1 );
    EqualElems( array, list4, _countof( list4 ) );
    AssertCrash( elem == MAX_idx );

    idx_t list5[] = { 0, 1, 10, 500, 0, 1 };
    idx_t elems[3];
    Memmove( elems, array.mem + array.len - 3, sizeof( idx_t ) * 3 );
    RemBack( array, 3 );
    EqualElems( array, list5, _countof( list5 ) );
    AssertCrash( elems[0] == 10 );
    AssertCrash( elems[1] == MAX_idx / 2 );
    AssertCrash( elems[2] == MAX_idx - 1 );

    idx_t list6[] = { 0, 1, 10, 500, 1 };
    RemAt( array, 4 );
    EqualElems( array, list6, _countof( list6 ) );

    idx_t list7[] = { 0, 10, 500, 1 };
    elem = array.mem[1];
    RemAt( array, 1 );
    EqualElems( array, list7, _countof( list7 ) );
    AssertCrash( elem == 1 );

    idx_t list8[] = { 0 };
    Memmove( elems, array.mem + 1, sizeof( idx_t ) * 3 );
    RemAt( array, 1, 3 );
    EqualElems( array, list8, _countof( list8 ) );
    AssertCrash( elems[0] == 10 );
    AssertCrash( elems[1] == 500 );
    AssertCrash( elems[2] == 1 );

    AssertCrash( array.mem[0] == 0 );

    Free( array );
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
