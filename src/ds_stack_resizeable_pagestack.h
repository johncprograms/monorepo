// Copyright (c) John A. Carlos Jr., all rights reserved.

// TODO: change to stack_nonresizeable_stack_t for the page, to avoid an extra allocation/indirection.
//   the problem there is that the page capacity is dynamic.

#define STACKRESIZEABLEPAGESTACK_PAGE   stack_resizeable_pagestack_page_t<T, Allocator, Allocation>
TEA struct
stack_resizeable_pagestack_page_t
{
  stack_nonresizeable_t<T, Allocator, Allocation> space;
  Allocation allocn;
};
TEA Inl STACKRESIZEABLEPAGESTACK_PAGE*
AllocatePagestackPage( Allocator& alloc, idx_t page_capacity )
{
  Allocation allocn = {};
  auto page = Allocate<STACKRESIZEABLEPAGESTACK_PAGE>( alloc, allocn, 1 );
  Alloc( page->space, page_capacity, alloc );
  page->allocn = allocn;
  return page;
}
TEA Inl void
FreePage( Allocator& alloc, STACKRESIZEABLEPAGESTACK_PAGE* page )
{
  Free( page->space );
  Allocation allocn = page->allocn;
  Free( alloc, allocn, page );
}

#define STACKRESIZEABLEPAGESTACK   stack_resizeable_pagestack_t<T, Allocator, Allocation>
TEA struct
stack_resizeable_pagestack_t
{
  // TODO: how to forward the Allocator instance so we don't have to duplicate it?
  //   or are we fine to duplicate it? e.g. it's a pagelist_t* which is fine.
  //   for simplest generated code we'd dedupe, so probably it's worth doing.
  //   we could share/inline a bunch of the code here, which sounds best i think.
  stack_resizeable_cont_t<STACKRESIZEABLEPAGESTACK_PAGE*> pages;
  idx_t len; // number of elements currently in the stack. Redundant, but useful.

  constant idx_t c_default_num_pages = 4000 / _SIZEOF_IDX_T;
};
TEA Inl void
Zero( STACKRESIZEABLEPAGESTACK& list )
{
  Zero( list.pages );
  list.len = 0;
}
TEA Inl void
Init( STACKRESIZEABLEPAGESTACK& list, idx_t nelems_capacity, Allocator alloc = {} )
{
  AssertCrash( nelems_capacity );
  Alloc( list.pages, STACKRESIZEABLEPAGESTACK::c_default_num_pages, alloc );
  *AddBack( list.pages ) = AllocatePagestackPage<T>( alloc, nelems_capacity );
  list.len = 0;
}
TEA Inl void
Kill( STACKRESIZEABLEPAGESTACK& list )
{
  ForLen( i, list.pages ) {
    auto page = list.pages.mem[i];
    FreePage( list.pages.alloc, page );
  }
  Free( list.pages );
  Zero( list );
}
TEA Inl void
Reset( STACKRESIZEABLEPAGESTACK& list )
{
  For( i, 1, list.pages.len ) {
    auto page = list.pages.mem[i];
    FreePage( list.pages.alloc, page );
  }
  auto page_first = list.pages.mem[0];
  page_first->space.len = 0;
  list.pages.len = 1;
  list.len = 0;
}
// TODO: contiguous version that returns T* instead of copying.
TEA Inl void
AddBack( STACKRESIZEABLEPAGESTACK& list, T* src, idx_t src_len )
{
  list.len += src_len; // doing this first since nothing fails, and we modify src_len below.
  AssertCrash( list.pages.len );
  auto page = list.pages.mem[ list.pages.len - 1 ];
  AssertCrash( page );
  auto space = &page->space;
  {
    auto len_remaining = LenRemaining( *space );
    auto num_add = MIN( src_len, len_remaining );
//    idx_t num_added = 0;
//    AddBack( *space, src, num_add, &num_added );
//    AssertCrash( num_added == num_add );
    TMove( AddBack( *space, num_add ), src, num_add );
    src += num_add;
    src_len -= num_add;
  }
  if( src_len ) {
    AssertCrash( space->len <= MAX_idx / 2 );
    auto new_default = 2 * space->len;
    while( src_len > new_default ) {
      AssertCrash( new_default <= MAX_idx / 2 );
      new_default *= 2;
    }
    auto newpage = AllocatePagestackPage<T>( list.pages.alloc, new_default );
    *AddBack( list.pages ) = newpage;
    auto newspace = &newpage->space;
//    idx_t num_added = 0;
//    AddBack( *newspace, src, src_len, &num_added );
//    AssertCrash( num_added == src_len );
    TMove( AddBack( *newspace, src_len ), src, src_len );
  }
}
TEA Inl void
RemBack( STACKRESIZEABLEPAGESTACK& list, T* dst, idx_t dst_len )
{
  AssertCrash( dst_len <= list.len );
  list.len -= dst_len; // doing this first since nothing fails, and we modify src_len below.
  auto& pages = list.pages;
  auto& pages_mem = list.pages.mem;
  auto& pages_len = list.pages.len;
  AssertCrash( pages_len );
  auto dst_write = dst + dst_len;
  while( dst_len ) {
    auto page = pages_mem[ pages_len - 1 ];
    auto space = &page->space;
    auto num_rem = MIN( dst_len, space->len );
    AssertCrash( num_rem );
    dst_write -= num_rem;
    RemBackReverse( *space, dst_write, num_rem );
    dst_len -= num_rem;
    if( !space->len  &&  pages_len > 1 ) {
      FreePage( pages.alloc, page );
      RemBack( pages );
      continue;
    }
  }
}


#define STACKRESIZEABLEPAGESTACK_PAGERELATIVEPOS   stack_resizeable_pagestack_pagerelativepos_t<T, Allocation>
template< typename T, typename Allocation = allocation_heap_or_virtual_t >
struct
stack_resizeable_pagestack_pagerelativepos_t
{
  STACKRESIZEABLEPAGE* page;
  idx_t idx;
};

TEA Inl STACKRESIZEABLEPAGESTACK_PAGERELATIVEPOS
MakeIteratorAtLinearIndex( STACKRESIZEABLEPAGESTACK& list, idx_t idx )
{
  AssertCrash( idx < list.totallen );
  ForNext( page, list.first_page ) {
    if( idx < page->len ) {
      STACKRESIZEABLEPAGESTACK_PAGERELATIVEPOS pos;
      pos.page = page;
      pos.idx = idx;
      return pos;
    }
    idx -= page->len;
  }
  UnreachableCrash();
  return {};
}

TEA Inl T*
GetElemAtIterator( STACKRESIZEABLEPAGESTACK& list, STACKRESIZEABLEPAGESTACK_PAGERELATIVEPOS pos )
{
  auto elem = pos.page->mem + pos.idx;
  return elem;
}

template< typename T, typename Allocation = allocation_heap_or_virtual_t >
Inl bool
CanIterate( STACKRESIZEABLEPAGESTACK_PAGERELATIVEPOS pos )
{
  auto r = pos.page;
  return r;
}

TEA Inl STACKRESIZEABLEPAGESTACK_PAGERELATIVEPOS
IteratorMoveR( STACKRESIZEABLEPAGESTACK& list, STACKRESIZEABLEPAGESTACK_PAGERELATIVEPOS pos, idx_t nelems = 1 )
{
  auto r = pos;
  r.idx += nelems;
  while( r.idx >= r.page->len ) {
    r.idx -= r.page->len;
    r.page = r.page->next;
    if( !r.page ) {
      // we allow pos to go one past the last element; same as integer indices.
      AssertCrash( !r.idx );
      break;
    }
  }
  return r;
}

TEA Inl T*
LookupElemByLinearIndex( STACKRESIZEABLEPAGESTACK& list, idx_t idx )
{
  AssertCrash( idx < list.totallen );
  ForNext( page, list.first_page ) {
    if( idx < page->len ) {
      auto r = page->mem + idx;
      return r;
    }
    idx -= page->len;
  }
  UnreachableCrash();
  return 0;
}

RegisterTest([]()
{
  stack_resizeable_pagestack_t<idx_t> s;
  Init( s, 10 );
  stack_resizeable_cont_t<idx_t> verif;
  Alloc( verif, 100000 );
  auto FnReset = [&]() {
    Reset( s );
    AssertCrash( s.pages.len == 1 );
    AssertCrash( s.pages.mem[0]->space.len == 0 );
    AssertCrash( s.len == 0 );
    verif.len = 0;
  };
  rng_xorshift32_t rng;
  Init( rng, 0x1234567812345678ULL );
  constant idx_t c_nchunk = 10;
  idx_t counter = 0;
  auto FnAddBack = [&]() {
    stack_nonresizeable_stack_t<idx_t, c_nchunk> tmp;
    Zero( tmp );
    auto nchunk = Rand32( rng ) % c_nchunk;
    For( c, 0, nchunk ) {
      *AddBack( tmp ) = counter;
      *AddBack( verif ) = counter;
      counter += 1;
    }
    auto len_before = s.len;
    AddBack( s, ML( tmp ) );
    AssertCrash( s.len == len_before + tmp.len );
  };
  auto FnRemBack = [&]() {
    stack_nonresizeable_stack_t<idx_t, c_nchunk> tmp;
    tmp.len = Min<idx_t>( s.len, Rand32( rng ) % c_nchunk );
    auto len_before = s.len;
    RemBack( s, ML( tmp ) );
    AssertCrash( s.len == len_before - tmp.len );
    // verify that the pop order is consistent with a stack by comparison with a simpler one.
    ReverseForLen( t, tmp ) {
      auto verif_last = verif.mem[ verif.len - 1 ];
      RemBack( verif );
      AssertCrash( tmp.mem[t] == verif_last );
    }
  };
  std::function<void()> fns[] = {
    FnAddBack,
    FnRemBack,
  };
  For( r, 0, 1000 ) {
    For( i, 0, 10000 ) {
      auto idx = Rand32( rng ) % _countof( fns );
      fns[idx]();
    }
    FnAddBack();
    FnReset();
  }
  Kill( s );
  Free( verif );
});


// TODO: we really need an easier way of iterating pagearrays

#if 0

  Templ struct
  pagearray_iter_t
  {
    STACKRESIZEABLEPAGE* page;
    idx_t idx;
  };

  #define
  for(
    auto pa_iter = PageArrayIteratorMake( list, start ), pa_end = PageArrayIteratorMake( list, end );
    PageArrayIteratorLessThan( list, pa_iter, pa_end );
    pa_iter = PageArrayIteratorR( list, pa_iter, 1 )
    )

  Templ Inl STACKRESIZEABLEPAGESTACK_PAGERELATIVEPOS
  PageArrayIteratorMake

  Templ Inl bool
  IteratorLessThan( STACKRESIZEABLEPAGESTACK& list, STACKRESIZEABLEPAGESTACK_PAGERELATIVEPOS pos, idx_t end )
  {
  }

#endif

