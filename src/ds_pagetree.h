// Copyright (c) John A. Carlos Jr., all rights reserved.

template< idx_t N >
struct
pagetree_page_t
{
  void* entries[N];
  alloctype_t allocn;
};
Templ Inl T* // T should be pagetree_page_t<N> of some kind. Easier to specify this way.
AllocatePage()
{
  alloctype_t allocn = {};
  auto page = Allocate<T>( &allocn, 1 );
  page->allocn = allocn;
  Arrayzero( page->entries );
  return page;
}

using pagetree_page8_t  = pagetree_page_t< 1 <<  8 >;
using pagetree_page10_t = pagetree_page_t< 1 << 10 >;
using pagetree_page11_t = pagetree_page_t< 1 << 11 >;
using pagetree_page12_t = pagetree_page_t< 1 << 12 >;
using pagetree_page16_t = pagetree_page_t< 1 << 16 >;


#define PAGETREE11x2   pagetree_11x2_t

//
// models a 2-level page tree with equal 11bits for each level:
// so the addressing looks like:
//   [ 11b | 11b ]
//
struct
pagetree_11x2_t
{
  pagetree_page11_t* toplevel;
};
Inl void
Init( PAGETREE11x2* pt )
{
  pt->toplevel = AllocatePage<pagetree_page11_t>();
}
Inl void
Zero( PAGETREE11x2* pt )
{
  pt->toplevel = 0;
}
// allocates intermediate level tables as necessary.
// leaves the LL page allocation up to the caller, since it's going to be something other
// than a pagetree_page_t.
Inl void**
AccessLastLevelPage( PAGETREE11x2* pt, u32 address )
{
  AssertCrash( !( address & ~AllOnes( 22 ) ) ); // only low 22 bits set.
  auto idx0 = ( address >> 11 ) & AllOnes( 11 );
  auto idx1 = ( address       ) & AllOnes( 11 );

  auto table0 = pt->toplevel;
  auto entry0 = table0->entries + idx0;
  if( !*entry0 ) {
    *entry0 = AllocatePage<pagetree_page11_t>();
  }
  auto table1 = Cast( pagetree_page11_t*, *entry0 );
  auto entry1 = table1->entries + idx1;
  // note that it's up to the caller to do the final nullcheck / LL page initialization.
  // it's assumed the LL page is something other than a pagetree_page_t, so the caller
  // has to deal with it.
  return entry1;
}
Inl void*
TryAccessLastLevelPage( PAGETREE11x2* pt, u64 address )
{
  AssertCrash( !( address & ~AllOnes( 22 ) ) ); // only low 22 bits set.
  auto idx0 = ( address >> 11 ) & AllOnes( 11 );
  auto idx1 = ( address       ) & AllOnes( 11 );

  auto table0 = pt->toplevel;
  auto entry0 = table0->entries + idx0;
  if( !*entry0 ) {
    return 0;
  }
  auto table1 = Cast( pagetree_page11_t*, *entry0 );
  auto entry1 = table1->entries + idx1;
  return *entry1;
}


#define PAGETREE16x2   pagetree_16x2_t

//
// models a 2-level page tree with equal 16bits for each level:
// so the addressing looks like:
//   [ 16b | 16b ]
//
struct
pagetree_16x2_t
{
  pagetree_page16_t* toplevel;
};
Inl void
Init( PAGETREE16x2* pt )
{
  pt->toplevel = AllocatePage<pagetree_page16_t>();
}
Inl void
Zero( PAGETREE16x2* pt )
{
  pt->toplevel = 0;
}
// allocates intermediate level tables as necessary.
// leaves the LL page allocation up to the caller, since it's going to be something other
// than a pagetree_page_t.
Inl void**
AccessLastLevelPage( PAGETREE16x2* pt, u32 address )
{
  auto idx0 = Cast( u16, ( address >> 16 ) & AllOnes( 16 ) );
  auto idx1 = Cast( u16, ( address       ) & AllOnes( 16 ) );

  auto table0 = pt->toplevel;
  auto entry0 = table0->entries + idx0;
  if( !*entry0 ) {
    *entry0 = AllocatePage<pagetree_page16_t>();
  }
  auto table1 = Cast( pagetree_page16_t*, *entry0 );
  auto entry1 = table1->entries + idx1;
  // note that it's up to the caller to do the final nullcheck / LL page initialization.
  // it's assumed the LL page is something other than a pagetree_page_t, so the caller
  // has to deal with it.
  return entry1;
}
Inl void*
TryAccessLastLevelPage( PAGETREE16x2* pt, u64 address )
{
  auto idx0 = Cast( u16, ( address >> 16 ) & AllOnes( 16 ) );
  auto idx1 = Cast( u16, ( address       ) & AllOnes( 16 ) );

  auto table0 = pt->toplevel;
  auto entry0 = table0->entries + idx0;
  if( !*entry0 ) {
    return 0;
  }
  auto table1 = Cast( pagetree_page16_t*, *entry0 );
  auto entry1 = table1->entries + idx1;
  return *entry1;
}


#define PAGETREE16x4   tpagetree_16x4_t

//
// models a 4-level page tree with equal 16bits for each level:
// so the addressing looks like:
//   [ 16b | 16b | 16b | 16b ]
//
struct
tpagetree_16x4_t
{
  pagetree_page16_t* toplevel;
};

using pagetree_16x4_t = tpagetree_16x4_t;

Inl void
Init( PAGETREE16x4* pt )
{
  pt->toplevel = AllocatePage<pagetree_page16_t>();
}
Inl void
Zero( PAGETREE16x4* pt )
{
  pt->toplevel = 0;
}

// TODO: for dynamic address slicing/dicing, can we rewrite this as a loop?
//   something like: AccessLastLevelPage( pt, tslice_t<u64> idxs )
//   which assumes the address has already been diced.
//   we'd probably have to also pass the size of each level, since we have to allocate here.
//   so maybe passing the dicing array AND the full address is best, and we slice/dice here.

// PERF: try caching idx=0 table pointers, and see if direct lookup when available improves access perf.
// this assumes idx=0 is more common than others, which is probably true, but we should verify that.
// it also assumes that branching will be faster than pointer-chasing, which might not be true here.
// something like the following:
//   if( !idx0  &&  !idx1  &&  !idx2 ) {
//     auto table3 = pt->cached_idx0_table3;
//     ...
//     return entry3;
//   }
//   if( !idx0  &&  !idx1 ) {
//     auto table2 = pt->cached_idx0_table2;
//     ...
//     return entry3;
//   }
//   if( !idx0 ) {
//     auto table1 = pt->cached_idx0_table1;
//     ...
//     return entry3;
//   }
//   // else we do full access.


// allocates intermediate level tables as necessary.
// leaves the LL page allocation up to the caller, since it's going to be something other
// than a pagetree_page_t.
Inl void**
AccessLastLevelPage( PAGETREE16x4* pt, u64 address )
{
  auto idx0 = Cast( u16, ( address >> 48 ) & AllOnes( 16 ) );
  auto idx1 = Cast( u16, ( address >> 32 ) & AllOnes( 16 ) );
  auto idx2 = Cast( u16, ( address >> 16 ) & AllOnes( 16 ) );
  auto idx3 = Cast( u16, ( address       ) & AllOnes( 16 ) );

  auto table0 = pt->toplevel;
  auto entry0 = table0->entries + idx0;
  if( !*entry0 ) {
    *entry0 = AllocatePage<pagetree_page16_t>();
  }
  auto table1 = Cast( pagetree_page16_t*, *entry0 );
  auto entry1 = table1->entries + idx1;
  if( !*entry1 ) {
    *entry1 = AllocatePage<pagetree_page16_t>();
  }
  auto table2 = Cast( pagetree_page16_t*, *entry1 );
  auto entry2 = table2->entries + idx2;
  if( !*entry2 ) {
    *entry2 = AllocatePage<pagetree_page16_t>();
  }
  auto table3 = Cast( pagetree_page16_t*, *entry2 );
  auto entry3 = table3->entries + idx3;
  // note that it's up to the caller to do the final nullcheck / LL page initialization.
  // it's assumed the LL page is something other than a pagetree_page_t, so the caller
  // has to deal with it.
  return entry3;
}

Inl void*
TryAccessLastLevelPage( PAGETREE16x4* pt, u64 address )
{
  auto idx0 = Cast( u16, ( address >> 48 ) & AllOnes( 16 ) );
  auto idx1 = Cast( u16, ( address >> 32 ) & AllOnes( 16 ) );
  auto idx2 = Cast( u16, ( address >> 16 ) & AllOnes( 16 ) );
  auto idx3 = Cast( u16, ( address       ) & AllOnes( 16 ) );

  auto table0 = pt->toplevel;
  auto entry0 = table0->entries + idx0;
  if( !*entry0 ) {
    return 0;
  }
  auto table1 = Cast( pagetree_page16_t*, *entry0 );
  auto entry1 = table1->entries + idx1;
  if( !*entry1 ) {
    return 0;
  }
  auto table2 = Cast( pagetree_page16_t*, *entry1 );
  auto entry2 = table2->entries + idx2;
  if( !*entry2 ) {
    return 0;
  }
  auto table3 = Cast( pagetree_page16_t*, *entry2 );
  auto entry3 = table3->entries + idx3;
  return *entry3;
}
