// Copyright (c) John A. Carlos Jr., all rights reserved.

// IDEA: pagetree, where each node represents a character.
// if we store a unique id per page, then we can uniquely identify any string.
// pagetrees are sparse, so we're not paying huge memory costs.
// i suspect this will be slower than string hashtables, but may as well see.
#define CHARTREEPAGE   chartree_page_t
struct
chartree_page_t
{
  // 0 in [x] where x!=0 means the page isn't initialized.
  CHARTREEPAGE* child_pages[256];
  idx_t page_id;
};
Inl CHARTREEPAGE*
AllocateChartreePage( pagelist_t* mem, idx_t page_id )
{
  auto page = AddPagelist( *mem, CHARTREEPAGE, alignof(CHARTREEPAGE), 1 );
  Arrayzero( page->child_pages );
  page->page_id = page_id;
  return page;
}

// NOTE: We use a pagelist_t allocator, to avoid having to iterate and free entries[i].
//   It sort of makes sense for this pagetree to own it, since it knows the exact fixed-size it wants.
//   We could use an even simpler pagelist_t since it's compile-time constant size chunks we want, but
//   it's just as easy to use the fully generic pagelist_t, so I haven't bothered.


#define CHARTREE   chartree_t
struct
chartree_t
{
  pagelist_t mem;
  CHARTREEPAGE* root;
  idx_t id_generator;
  idx_t num_pages;
};
Inl void
Init( CHARTREE* tree )
{
  Init( tree->mem, sizeof(CHARTREEPAGE) );
  // Give the root page_id=0.
  tree->root = AllocateChartreePage( &tree->mem, 0 );
  tree->id_generator = 1;
  tree->num_pages = 1;
}
Inl void
Kill( CHARTREE* tree )
{
  Kill( tree->mem );
  tree->root = 0;
  tree->id_generator = 0;
  tree->num_pages = 0;
}
Inl idx_t
Uniquify( CHARTREE* tree, slice_t str )
{
  auto page = tree->root;
  ForLen( i, str ) {
    auto c = str.mem[i];
    auto next_page = page->child_pages[c];
    if( !next_page ) {
      next_page = AllocateChartreePage( &tree->mem, tree->id_generator );
      page->child_pages[c] = next_page;
      tree->id_generator += 1;
      tree->num_pages += 1;
    }
    page = next_page;
  }
  return Cast( idx_t, page->child_pages[0] );
}



RegisterTest([]()
{
  chartree_t tree;
  Init( &tree );

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

  Kill( &tree );
});
