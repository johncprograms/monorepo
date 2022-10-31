// Copyright (c) John A. Carlos Jr., all rights reserved.

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
  Init( page->space, page_capacity, alloc );
  page->allocn = allocn;
}
TEA Inl void
FreePage( Allocator& alloc, STACKRESIZEABLEPAGESTACK_PAGE* page )
{
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
  idx_t default_page_capacity;
  
  constant idx_t c_default_num_pages = 4000 / _SIZEOF_IDX_T;
};
TEA Inl void
Zero( STACKRESIZEABLEPAGESTACK& list )
{
  Zero( list.pages );
  list.default_page_capacity = 0;
}
TEA Inl void
Init( STACKRESIZEABLEPAGESTACK& list, idx_t nelems_capacity, Allocator alloc = {} )
{
  AssertCrash( nelems_capacity );
  Init( list.pages, STACKRESIZEABLEPAGESTACK::c_default_num_pages, alloc );
  *AddBack( list.pages ) = AllocatePagestackPage( alloc, nelems_capacity );
  list.default_page_capacity = nelems_capacity;
}
TEA Inl void
Kill( STACKRESIZEABLEPAGESTACK& list )
{
  ForLen( i, list.pages ) {
    auto page = list.pages.mem[i];
    FreePage( list.pages.alloc, page );
  }
}
TEA Inl void
Reset( STACKRESIZEABLEPAGESTACK& list )
{
  For( i, 1, list.pages.len ) {
    auto page = list.pages.mem[i];
    FreePage( list.pages.alloc, page );
  }
  auto page_first = list.pages.mem[0];
  page_first->len = 0;
}
// TODO: contiguous version that returns T* instead of copying.
TEA Inl void
AddBack( STACKRESIZEABLEPAGESTACK& list, T* src, idx_t src_len )
{
  AssertCrash( list.pages.len );
  auto page = list.pages.mem[ list.pages.len - 1 ];
  AssertCrash( page );
  auto space = &page->space;
  auto len_remaining = space->capacity - space->len;
  auto num_add = MIN( src_len, len_remaining );
  TMove( AddBack( space, num_add ), src, num_add );
  src += num_add;
  src_len -= num_add;
  if( src_len ) {
    list.default_page_capacity = MAX( 2 * list.default_page_capacity, src_len );
    auto newpage = AllocatePagestackPage( list.pages.alloc, list.default_page_capacity );
    *AddBack( list.pages ) = newpage;
    auto newspace = &newpage->space;
    TMove( AddBack( newspace, src_len ), src, src_len );
  }
}
TEA Inl void
RemBack( STACKRESIZEABLEPAGESTACK& list, T* dst, idx_t dst_len )
{
  auto& pages = list.pages;
  auto& pages_mem = list.pages.mem;
  auto& pages_len = list.pages.len;
  AssertCrash( pages_len );
  while( dst_len ) {
    auto page = pages_mem[ pages_len - 1 ];
    auto space = &page->space;
    auto num_rem = MIN( dst_len, space->len );
    AssertCrash( num_rem );
    RemBack( space, dst, num_rem );
    dst += num_rem;
    dst_len -= num_rem;
    if( !space->len  &&  pages_len > 1 ) {
      FreePage( pages.alloc, page );
      RemBack( pages );
      continue;
    }
  }
}


#define PAGERELATIVEPOS   pagerelativepos_t<T, Allocation>
template< typename T, typename Allocation = allocation_heap_or_virtual_t >
struct
pagerelativepos_t
{
  STACKRESIZEABLEPAGE* page;
  idx_t idx;
};

TEA Inl PAGERELATIVEPOS
MakeIteratorAtLinearIndex( STACKRESIZEABLEPAGESTACK& list, idx_t idx )
{
  AssertCrash( idx < list.totallen );
  ForNext( page, list.first_page ) {
    if( idx < page->len ) {
      PAGERELATIVEPOS pos;
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
GetElemAtIterator( STACKRESIZEABLEPAGESTACK& list, PAGERELATIVEPOS pos )
{
  auto elem = pos.page->mem + pos.idx;
  return elem;
}

template< typename T, typename Allocation = allocation_heap_or_virtual_t >
Inl bool
CanIterate( PAGERELATIVEPOS pos )
{
  auto r = pos.page;
  return r;
}

TEA Inl PAGERELATIVEPOS
IteratorMoveR( STACKRESIZEABLEPAGESTACK& list, PAGERELATIVEPOS pos, idx_t nelems = 1 )
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

  Templ Inl PAGERELATIVEPOS
  PageArrayIteratorMake

  Templ Inl bool
  IteratorLessThan( STACKRESIZEABLEPAGESTACK& list, PAGERELATIVEPOS pos, idx_t end )
  {
  }

#endif

