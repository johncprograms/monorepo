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
  alloctype_t allocn;
};
Inl CHARTREEPAGE*
AllocateChartreePage( idx_t page_id )
{
  alloctype_t allocn = {};
  auto page = Allocate<CHARTREEPAGE>( &allocn, 1 );
  Arrayzero( page->child_pages );
  page->page_id = page_id;
  page->allocn = allocn;
  return page;
}

#define CHARTREE   chartree_t
struct
chartree_t
{
  CHARTREEPAGE* root;
  idx_t id_generator;
  idx_t num_pages;
};
Inl void
Init( CHARTREE* tree )
{
  // Give the root page_id=0.
  tree->root = AllocateChartreePage( 0 );
  tree->id_generator = 1;
  tree->num_pages = 1;
}
Inl idx_t
Uniquify( CHARTREE* tree, slice_t str )
{
  auto page = tree->root;
  ForLen( i, str ) {
    auto c = str.mem[i];
    auto next_page = page->child_pages[c];
    if( !next_page ) {
      next_page = AllocateChartreePage( tree->id_generator );
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
  pagelist_t mem;
  Init( mem, 64000 );
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

  Kill( mem );
});
