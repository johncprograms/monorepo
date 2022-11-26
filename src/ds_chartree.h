// Copyright (c) John A. Carlos Jr., all rights reserved.

// IDEA: pagetree, where each node represents a character.
// if we store a unique id per page, then we can uniquely identify any string.
// pagetrees are sparse, so we're not paying huge memory costs.
// i suspect this will be slower than string hashtables, but may as well see.
#define CHARTREEPAGE   chartree_page_t<Allocation>
template< typename Allocation = allocation_heap_or_virtual_t >
struct
chartree_page_t
{
  // 0 in [x] where x!=0 means the page isn't initialized.
  CHARTREEPAGE* child_pages[256];
  idx_t page_id;
  Allocation allocn;
};
TA Inl CHARTREEPAGE*
AllocateChartreePage( Allocator& alloc, idx_t page_id )
{
  Allocation allocn = {};
  auto page = Allocate<CHARTREEPAGE>( alloc, allocn, 1 );
  Arrayzero( page->child_pages );
  page->page_id = page_id;
  page->allocn = allocn;
  return page;
}

#define CHARTREE   chartree_t<Allocator, Allocation>
TA struct
chartree_t
{
  CHARTREEPAGE* root;
  Allocator alloc;
  idx_t id_generator;
  idx_t num_pages;
};
TA Inl void
Init( CHARTREE* tree, Allocator alloc = {} )
{
  // Give the root page_id=0.
  tree->root = AllocateChartreePage<Allocator, Allocation>( alloc, 0 );
  tree->alloc = alloc;
  tree->id_generator = 1;
  tree->num_pages = 1;
}
TA Inl idx_t
Uniquify( CHARTREE* tree, slice_t str )
{
  auto page = tree->root;
  ForLen( i, str ) {
    auto c = str.mem[i];
    auto next_page = page->child_pages[c];
    if( !next_page ) {
      next_page = AllocateChartreePage<Allocator, Allocation>( tree->alloc, tree->id_generator );
      page->child_pages[c] = next_page;
      tree->id_generator += 1;
      tree->num_pages += 1;
    }
    page = next_page;
  }
  return Cast( idx_t, page->child_pages[0] );
}



#if defined(TEST)

void
TestChartree()
{
  chartree_t<allocator_pagelist_t, allocation_pagelist_t> tree;
  pagelist_t mem;
  Init( mem, 64000 );
  Init( &tree, allocator_pagelist_t{ &mem } );

  slice_t tests[] = {
    SliceFromCStr( "" ),
    SliceFromCStr( "a" ),
    SliceFromCStr( "b" ),
    SliceFromCStr( "ab" ),
    SliceFromCStr( "abc" ),
    SliceFromCStr( "foobar" ),
    SliceFromCStr( "----------" ),
    };

  ForEach( test, tests ) {
    auto id = Uniquify( &tree, test );
    (void)id;
  }

  Kill( mem );
}

#endif // defined(TEST)
