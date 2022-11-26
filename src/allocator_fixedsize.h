// Copyright (c) John A. Carlos Jr., all rights reserved.

// TODO: implement more unordered queues. e.g. nonresizeable with embedded freelist.

//
// fixed-size allocators
//
// the basic idea is to use a bitarray to represent whether a fixed-size chunk is allocated or not.
// if the bitarray fills up, we can just have a linked-list of them.
//
// which fixed sizes should we support?
//   powers of 2 are probably natural. i'm thinking 64B up to 1GB. which means we want [ 2^6, 2^30 ].
//   which is 25 different size buckets.
//
// allocation wants to trivially skip full pages. this is especially important for the huge-size
// pages, since they only store a couple allocations, and we end up with lots of elements.
// whereas, free wants to traverse in most-recently-allocated order.
// these are two different orderings; how do we reconcile that with our storage here?
// well allocation doesn't particularly care about order; just full or not.
// if we maintain a separate list for not-full pages, then maintenance is probably O( # pages ).
// unless we start using hashtables, but those have a high constant cost, plus amortization.
//
// to let allocation skip full pages, we split the page_list into two:
// - elements < top_notfull are full pages, no more room for allocations.
// - elements >= top_notfull have room for at least one more allocation.
// on allocation,
//   if we have any not-full pages:
//     we pick the last not-full page, which is guaranteed to have room.
//     mark an allocation in the page; queue it for returning.
//     if the page is now full, move it to before top_notfull.
//   else we have no pages, or no not-empty pages.
//   insert a new page at the end, and update top_notfull to point to it.
// on free,
//   iterate the pages in reverse order:
//     if the allocation doesn't overlap the page, continue iteration.
//     clear the allocation in the page.
//     if !top_notfull, meaning all pages are full, this is the first not-full page.
//       move this page to the end.
//       set top_notfull to this page.
//     else we have other not-full pages:
//       TODO: is the end better than moving to top_notfull?
//         i.e. if we have multiple free pages, are we more likely to free again in allocation order, or free order?
//       move this page to the end.
//

constant idx_t c_min_numbits_region = 6;
constant idx_t c_min_region_size = 1 << 6;
constant idx_t c_max_numbits_region = 30;
constant idx_t c_max_region_size = 1 << 30;
constant idx_t c_num_regions = c_max_numbits_region - c_min_numbits_region + 1;
// use 2GB as a reasonable sizing for each fsapage_t.
constant idx_t c_target_page_size = 2 * c_max_region_size;

struct
fsapage_t
{
  u8* vmem;
  u8* mem_allocation_space; // TODO: eliminate a load here; redundant with vmem.
  u64 num_regions; // number of allocations we can support in this page
  u64 num_regions_in_use;
  u64 bitarray_len;
};
struct
fsalloc_t
{
  u64 nbits;
  list_t<fsapage_t> page_list;
  listelem_t<fsapage_t>* top_notfull;
};
// nbits is the bit count of the allocation unit size.
// e.g. if you want 1024byte units, pass nbits=10.
Inl void
Init( fsalloc_t* f, u64 nbits )
{
  Typezero( f );
  f->nbits = nbits;
}
Inl void
Kill( fsalloc_t* f )
{
  // can't use ForList, since the listelem_t are stored in the vmem allocation.
  auto elem = f->page_list.first;
  while( elem ) {
    auto page = &elem->value;
    auto next = elem->next;
    MemVirtualFree( page->vmem );
    elem = next;
  }
  Typezero( f );
}
ForceInl u8*
_AllocBytesFromPage(
  fsapage_t* page,
  u64 region_size
  )
{
  // TODO: this is effectively N^2 for repeated allocations.
  //
  auto bitarray = Cast( u64*, page->vmem + sizeof( listelem_t<fsapage_t> ) );
  Fori( u64, bitarray_idx, 0, page->bitarray_len ) {
    auto bitarray_elem = bitarray[ bitarray_idx ];
    // do a bit-scan-forward on the bitwise-negated bitarray_elem,
    // which will find the index of first un-set bit of bitarray_elem.
    u32 bit_idx;
    auto found = BitScanForward_idx_t( &bit_idx, ~bitarray_elem );
    if( !found ) {
      // bitarray_elem is all 1s, no space here.
      continue;
    }
    auto r = page->mem_allocation_space + ( bitarray_idx * 64 + bit_idx ) * region_size;
    bitarray[ bitarray_idx ] |= ( 1ull << bit_idx );
    page->num_regions_in_use += 1;
    return r;
  }
  AssertCrash( !"why isn't our top_notfull mechanism guarding full pages?" );
  return 0;
}
Inl u8*
Alloc( fsalloc_t* f )
{
  auto nbits = f->nbits;
  auto page_list = &f->page_list;
  auto region_size = 1ull << nbits;
  if( f->top_notfull ) {
    auto pageelem = f->top_notfull;
    auto page = &pageelem->value;
    AssertCrash( page->num_regions_in_use < page->num_regions );
    auto r = _AllocBytesFromPage( page, region_size );
    AssertCrash( r );
    if( page->num_regions_in_use == page->num_regions ) {
      auto next_notfull = pageelem->next;
      Rem( *page_list, pageelem );
      if( next_notfull ) {
        InsertBefore( *page_list, pageelem, next_notfull );
      }
      else {
        InsertLast( *page_list, pageelem );
      }
      f->top_notfull = next_notfull;
    }
    return r;
  }
  // nothing available in the page_list, so we need to add a new page.
  auto nbytes_header = RoundUpToMultipleOfPowerOf2( sizeof( listelem_t<fsapage_t> ), 64 );
  auto num_regions = c_target_page_size >> nbits;
  auto bitarray_len = Bitbuffer64Len( num_regions );
  auto nbytes_bitbuffer = RoundUpToMultipleOfPowerOf2( bitarray_len * sizeof( u64 ), 64 );
  auto nbytes_page = nbytes_header + nbytes_bitbuffer + c_target_page_size;
  auto vmem = Cast( u8*, MemVirtualAllocBytes( nbytes_page ) );
  AssertCrash( vmem );
  auto new_page_elem = Cast( listelem_t<fsapage_t>*, vmem );
  InsertLast( *page_list, new_page_elem );
  f->top_notfull = new_page_elem;
  auto new_page = &new_page_elem->value;
  new_page->vmem = vmem;
  new_page->mem_allocation_space = vmem + nbytes_header + nbytes_bitbuffer;
  new_page->num_regions = num_regions;
  new_page->num_regions_in_use = 0;
  new_page->bitarray_len = bitarray_len;
  auto r = _AllocBytesFromPage( new_page, region_size );
  AssertCrash( r );
  return r;
}
Inl bool
TryFree( fsalloc_t* f, void* pv )
{
  auto p = Cast( u8*, pv );
  auto nbits = f->nbits;
  auto page_list = &f->page_list;
  // auto region_size = 1ull << nbits;
  REVERSEFORLIST( page, elem, *page_list )
    auto page_mem_start = page->mem_allocation_space;
    auto page_mem_end = page->mem_allocation_space + c_target_page_size;
    if( !LTEandLT( p, page_mem_start, page_mem_end ) ) {
      continue;
    }
    auto nbytes_offset = Cast( u64, p - page_mem_start );
    auto region_offset = nbytes_offset >> nbits;
    auto bitarray_idx = region_offset / 64;
    auto bit_idx = region_offset % 64;
    auto bitarray = Cast( u64*, page->vmem + sizeof( listelem_t<fsapage_t> ) );
    bitarray[ bitarray_idx ] &= ~( 1ull << bit_idx );
    AssertCrash( page->num_regions_in_use );
    page->num_regions_in_use -= 1;
    if( !f->top_notfull ) {
      // all pages are full, this is the first not-full page.
      f->top_notfull = elem;
    }
    // TODO: when f->top_notfull, meaning we have other not-full pages, should we move
    //   to before top_notfull, or to the end like we're doing now?
    //   it's a question of whether we want to maintain the most-recent-allocation ordering,
    //   or if we want to switch to most-recently-freed ordering.
    Rem( *page_list, elem );
    InsertLast( *page_list, elem );
    return 1;
  }
  return 0;
}
Inl void
Free( fsalloc_t* f, void* pv )
{
  auto freed = TryFree( f, pv );
  AssertCrash( freed ); // freeing a pointer not tracked by this heap!
}






//
// group of fsalloc_t, with one for each power of 2 range.
//
struct
generalfsalloc_t
{
  fsalloc_t fsallocs[ c_num_regions ];
};
Inl void
Init( generalfsalloc_t* g )
{
  For( region_idx, 0, c_num_regions ) {
    auto nbits = region_idx + c_min_numbits_region;
    Init( &g->fsallocs[region_idx], nbits );
  }
}
Inl void
Kill( generalfsalloc_t* g )
{
  ForEach( fsalloc, g->fsallocs ) {
    Kill( &fsalloc );
  }
}
Inl u8*
AllocBytes( generalfsalloc_t* g, idx_t nbytes )
{
  AssertCrash( nbytes <= c_max_region_size ); // probably use alternate strategies to break up huge allocs.
  if( !nbytes ) {
    // NOTE: we don't return any sentinel nonzero value, like 0x1 here.
    // we'll crash here if we fail to allocate, so the callers shouldn't be checking !=0.
    return 0;
  }
  //
  // we subtract one for the lzcnt, because we want the exact powers of two to go in the precise-fit pages.
  // e.g.
  // 1024 = 00000000 00000000 00000100 00000000
  // 1023 = 00000000 00000000 00000011 11111111
  // lzcnt(1024) = 32 - 11 = 21
  // lzcnt(1023) = 32 - 10 = 22
  // nbits_after_leading_zeros(1024) = 32 - lzcnt(1024) = 32 - 21 = 11
  // nbits_after_leading_zeros(1023) = 32 - lzcnt(1023) = 32 - 22 = 10
  // since we can store precisely 1024 bytes in the region_size = 1 << 10, that's what we want.
  // this forces us to handle nbytes=0 above, separately, to avoid wraparound.
  //
  auto nbits_leading_zeros = _lzcnt_idx_t( nbytes - 1 );
  auto nbits_after_leading_zeros = NUMBITS_idx - nbits_leading_zeros;
  auto nbits = MAX( nbits_after_leading_zeros, c_min_numbits_region );
  AssertCrash( LTEandLTE( nbits, c_min_numbits_region, c_max_numbits_region ) );
  auto region_idx = nbits - c_min_numbits_region;
  auto r = Alloc( &g->fsallocs[region_idx] );
  return r;
}
Inl void
Free( generalfsalloc_t* g, void* pv )
{
  // TODO: MRU cache of which region_idx ?
  //   we could also do some set membership test, based on the given pointer value.
  Fori( idx_t, region_idx, 0, c_num_regions ) {
    auto freed = TryFree( &g->fsallocs[region_idx], pv );
    if( freed ) return;
  }
  AssertCrash( !"freeing a pointer not tracked by this heap!" );
}




#if defined(TEST)

Inl void
TestFsalloc()
{
  rng_xorshift32_t rng;
  Init( rng, 0x1234567812345678ULL );

  stack_resizeable_cont_t<slice_t> allocs;
  Alloc( allocs, 1000 );

  For( region_idx, 0, c_num_regions ) {
    auto nbits = region_idx + c_min_numbits_region;
    fsalloc_t f;
    Init( &f, nbits );
    auto nbytes = 1ull << nbits;
    auto N = ( c_target_page_size / nbytes );
    N = MAX( 10, N / 1000 );
    For( i, 0, N ) {
      if( Zeta32( rng ) > 0.2f ) {
        u8* bytes = Alloc( &f );
        memset( bytes, 0xAA, nbytes );
        *AddBack( allocs ) = { bytes, nbytes };
      }
      else if( allocs.len ) {
        auto bytes = allocs.mem[ allocs.len - 1 ];
        RemBack( allocs );
        memset( bytes.mem, 0xFF, bytes.len );
        Free( &f, bytes.mem );
      }
    }
    ForLen( i, allocs ) {
      Free( &f, allocs.mem[i].mem );
    }
    allocs.len = 0;
    Kill( &f );
  }

  {
    //
    // weight the probability of picking a region according to 1/region_size.
    // this is so we can test with lots of allocations, without ending up with
    // a gazillion 1GB allocations, which will cause OOM very quickly.
    //
    f32 probability_thresholds[c_num_regions];
    kahansum32_t sum = {};
    For( region_idx, 0, c_num_regions ) {
      auto nbits = region_idx + c_min_numbits_region;
      auto region_size = 1ull << nbits;
      auto pdf = Cast( f32, c_target_page_size / region_size );
      Add( sum, pdf );
      probability_thresholds[region_idx] = sum.sum;
    }
    For( region_idx, 0, c_num_regions ) {
      probability_thresholds[region_idx] /= sum.sum;
    }

    generalfsalloc_t g;
    Init( &g );
    constant idx_t N = 10000;
    For( i, 0, N ) {
      if( Zeta32( rng ) > 0.2f ) {
        auto zeta = Zeta32( rng );
        idx_t region_idx = 0;
        for( ; region_idx < c_num_regions; ++region_idx ) {
          if( zeta <= probability_thresholds[region_idx] ) {
            break;
          }
        }
        AssertCrash( region_idx < c_num_regions );
        auto nbits = region_idx + c_min_numbits_region;
        auto nbytes_min = 1ull << nbits;
        auto nbytes_max = 1ull << ( nbits + 1 );
        auto nbytes = Lerp_from_f32( nbytes_min, nbytes_max - 1, Zeta32( rng ), 0.0f, 1.0f );
        // account for the hard limit on allocation size.
        nbytes = MIN( nbytes, c_max_region_size );
        u8* bytes = AllocBytes( &g, nbytes );
        memset( bytes, 0xAA, nbytes );
        *AddBack( allocs ) = { bytes, nbytes };
      }
      else if( allocs.len ) {
        auto bytes = allocs.mem[ allocs.len - 1 ];
        RemBack( allocs );
        memset( bytes.mem, 0xFF, bytes.len );
        Free( &g, bytes.mem );
      }
    }
    ForLen( i, allocs ) {
      Free( &g, allocs.mem[i].mem );
    }
    allocs.len = 0;
    Kill( &g );
  }

  Free( allocs );
}

#endif // defined(TEST)
