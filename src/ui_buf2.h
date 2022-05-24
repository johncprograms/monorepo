// Copyright (c) John A. Carlos Jr., all rights reserved.


// TODO: this is questionable.
//   do we really need another text storage model?

// different coordinate spaces to consider:
//   linear space, [0, file_len]
//   line space, [0, num_lines]
//   slice space, if we use the slices model. [0, num_slices]
//   char_inline space [0, line_len] given a line position.
//
// txt_t's natural spaces are probably line space + char_inline space.
// it also uses linear space moderately.
//
// our previous model was linear space favored, which was relatively fast. we just didn't have a good
// line space solution, so line ops like scrolling were slow.
//
// our current model is linear/slice space favored, which removed a lot of the line space conversions we used to do.
// but line space ops still have to discover line boundaries. and iterating in slice space is relatively slow.
//
// what if our next model is line space favored, since that's what txt_t really wants?
// something like:
//   array_t<content_ptr_t> bols;
// or
//   array_t<slice_t> lines;
// to provide constant-time line space operations?
// the bols approach would make ALL ins/dels be O( num_lines ) since we'd have to update bols.
// the lines approach would make ONLY LINE ins/dels be O( num_lines ). that's slightly better.
// num_lines can get up to ~100K reasonably, so that's still quite high. we're talking ~1MB modified memory,
// and probably 10s - 100s of milliseconds execution at that size.
//
// pow2 pagetable addressing is probably the best way to bring those numbers down.
// i.e. lowest 16bit indexes the LLP ( last level page )
//      higher 16bit indexes the L1D ( level 1 directory )
// and we probably don't need any more for line space. nobody needs >4 billion lines, right?
// 16bit means 65536 entries.
// for LLP entries that means 16byte * 65536 = 1MB
// for L1D entries that means  8byte * 65536 = 512KB
// so in total, 1.5MB for line metadata. not too bad on modern machines.
// with this model,
//   char ins/del are still O( 1 ) with our leaky plist and line replacement.
//   line ins/del are still O( num_lines ), so it doesn't really help there.
//
// what about line hashing?
// hashset_t<u32,slice_t> to map line positions to line storage.
// that keeps the O( 1 ) line space lookup, but it doesn't really solve the ordering problem.
// i guess we can do something like idx_hashset_t to avoid actually moving slices.
// that would let us just write over the line positions.
// note we could do the same with { array_t<u32>, array_t<slice_t> }, no hashset really req'd.
// in either case, that gets us down to ~400KB modified memory, which is ~3x smaller.
//
// what about pagearrays?
// that gives up the constant line space lookup, which isn't ideal.
// for pagearrays, line space lookups are O( num_pages ).
// we could make the pages sufficiently large to mitigate that.
// but then line ins/dels are O( page_size ), so we want to make the pages sufficiently small.
// conflicting concerns, so there's some mediocre page_size in the middle we'd want.
// this effectively improves line ins/dels at the cost of line space lookups.
// so not a great tradeoff.
//
// what else can we do?
// what about tree structures?
// those maintain order, and allow easy ins/dels.
// we could likely get:
//   line space lookups: O( log( num_lines ) )
//   line ins/dels: O( 1 )  or O( log( num_lines ) ) if we need to do tree maintenance for that tree kind.
// that's probably better than pagearrays for exceedingly large files.
// but is it better for medium files? hard to say.
// if we say num_lines = 1M, then log( 1M ) = 20. so if we used a binary tree, that's 20 levels deep. 
//
// measuring array_t<slice_t> addat(0) shows ~100K entries is fine, but ~1M is not.
// at that many, we've blown the last level cache size, and are presumably stalling for memory.
// last level cache for my intel 6600K is something like 5MB, so that makes sense.
// intel server chips have something like 15MB, which would make more of the ~100K class safe.
// we could shrink the array by factor of 3/4 by splitting into two, and letting len be u32.
// lines >4GB aren't really in scope for editing.
// we could also shrink further by switching the pointer to a u32 offset.
// that limits us to <4GB filesize entirely, which might not be great. it's probably fine though.
// for files that large, you probably want to do disk<->memory windowing/streaming anyways.
// so doing both of these gives us a 2x increase in line capacity.
// we could even go more hardcore, and make line len be u8 or so, since we never render wider than that.
// doing line streaming like this might be a pain to implement though.
// in all, we could go from 16bytes -> 5bytes. that's ~3x increase in line capacity.
// not bad for the simple array approach...
// actually, much simpler, we just do the array_t<u32> indirection layer suggested above.
// that's 4bytes, even better. 4x increase in capacity.
// can't really do any better than this, and keep the constant time line space lookup.
// maybe some encoding to reduce sizes of smaller line numbers, but it couldn't be too aggressive.
// e.g. half of 4byte ints fit inside 2bytes, and half of those fit in 1byte.
// so have 256 1bytes, then 65536-256 2bytes, then 4B-65536-256 4bytes.
// that scheme would save 256*3 + 65280*2 = 131,328bytes.
// for our ~100K lines ceiling, that's probably actually a win worth pursuing.
//
// now should we try to do this line structure alongside the slice structure? or is the line structure alone sufficient?
// i.e. are we really okay to give up fast linear space ops?
// i guess the hope is most of them don't really need linear space.
// presumably eliminating the line boundary-finding will make up the difference.
// the primary points of struggle were: scrolling, line rendering, and find.
// the line structure should make the first two trivially fast.
// unfortunately it won't help much with the find i don't think.
// at worst we could autosave, then do the regular linear space find.
// but find wasn't all that bad; just in the complicated slice-space it was terrible.
// hopefully line space fares better, requiring fewer iterator adjustments.
//

// TODO: convert y to always be [0, len)
//   always make at least one line, and don't allow deleting it.

// TODO: consider moving most of the Cursor fns out to be in cstr.h, since they're inline.
//   we should probably also fixup the callers to use all the line context they have, to avoid redundant line lookups.

//
// unordered remove will move the last unordered line to the 'internal' slot.
// we also need to change the permutation entry for that last unordered line to contain 'internal' now.
// we could do that via:
// - storing the inverse permutation array; i.e. line_from_internal_idx. then do a lookup chain:
//     internal_idx_from_line[ line_from_internal_idx[ internal ] ]
//     to get the right entry, and then assign 'internal' to it.
//     this is how idx_hashset_t worked.
//     this req's twice the memory and twice the book-keeping.
// - keep track of the internal idx of just the last unordered line.
//     this is relatively simple, since the line ops all have this information handy.
//     well except for when we delete that last line...
//     that's a pretty good reason to do the bidirectional permutation storage.
//
// or maybe we can just leave empty entries in unordered_lines.
// make a separate array_t<u32> of unused entries, so we don't have to compact unordered_lines at all?
// then we can avoid all fixups of internal pointers, which is nice.
//
// i've gone with this design for now.
//
//
//
// it also might be nice to change unordered_lines into a pagetree.
// we only have a u32 address by design, to maximize the number of lines we can handle.
// keeping a giant array of slices isn't ideal; the indirection is supposed to eliminate that.
// otherwise we should just use an ordered array of slices by itself.
// switching to a pagetree should theoretically make our max line count 2x-4x bigger.
//
// but it's worth noting the line-based system still has the cliff of death, when the big
// internal_idx_from_line array grows beyond the cache size.
// since file contents also eat up cache space, we might hit this limit sooner than we want.
//
// pagetree structure could look something like:
//
// struct
// lineblock_t
// {
//   // TODO: could also save some packing memory by changing to slice32_t, and then splitting it into two,
//   // so the u32 len's can be packed tightly.
//   // or, maybe we want a u32 worth of flags per line!
//   // already had the idea to store 'is_nontrivial_toplevel' per-line, for easier custom toplevel editing view.
//   // also had the idea of a fundamental 'edited' flag to indicate the line is an sarray_t into the plist, for better memory usage.
//   // i can also imagine 'unsaved_changes', which we render somehow to show what's locally changed.
//   // maybe we render 'edited' too; VS does this with a yellow for 'unsaved_changes', green for 'edited', blank else.
//   // something like: struct line_t { u8* mem;  u32 len;  u32 flags; };
//   slice_t lines[c_lineblock_size];
// };
//
// Inl slice_t*
// TryAccessLine( buf_t* buf, u32 internal_idx )
// {
//   // TODO: with this division, we're not using all 32 bits of blockaddr, assuming c_lineblock_size is >1.
//   // maybe we should pick a fixed size, and use a smaller number of bits in the pagetree addressing.
//   // e.g. make c_lineblock_size = 2^10=1024, then use [ 11b | 11b ] addressing for our pagetree.
//   // that would utilize all 10+11+11=32bits we have.
//   // just as an additional note, we probably want c_lineblock_size to be >= 2 * nlines_visible, so rendering happens in one block.
//   auto blockaddr = internal_idx / c_lineblock_size;
//   auto lineaddr  = internal_idx % c_lineblock_size;
//   auto entry = TryAccessLastLevelPage( &buf->internal_lineblocks, blockaddr );
//   if( !entry ) {
//     return 0;
//   }
//   auto block = Cast( lineblock_t*, entry );
//   auto line = block->lines + lineaddr;
//   return line;
// }
//
// Inl slice_t*
// AccessLine( buf_t* buf, u32 internal_idx )
// {
//   auto blockaddr = internal_idx / c_lineblock_size;
//   auto lineaddr  = internal_idx % c_lineblock_size;
//   auto entry = AccessLastLevelPage( &buf->internal_lineblocks, blockaddr );
//   if( !*entry ) {
//     *entry = AddPlist( buf->plist, lineblock_t, _SIZEOF_IDX_T, 1 );
//     Memzero( *entry, sizeof( lineblock_t ) );
//   }
//   auto block = Cast( lineblock_t*, *entry );
//   auto line = block->lines + lineaddr;
//   return line;
// }
//
// struct
// buf_t
// {
//   array32_t<u32> internal_idx_from_line;
//   pagetree_16x2_t internal_lineblocks;
//   array32_t<u32> unused_internal_idxs;
// };
//
// OPEN QUESTION:
// should we routinely use AccessLine to lookup lines, or TryAccessLine ?
// since we're likely to inline the AddPlist code to initialize pages, probably best to use TryAccessLine.
// we just need to do an AccessLine on first use of any new line.
//


// TODO: to save memory usage, consider a line caching system that dedupes line contents.
// in the current model everything is line-based, so we need to cache on that granularity.
// since our goal is instant everything, a constant cost cache lookup is best.
// maybe a simple cache like:
//
// struct
// linecache_line_t
// {
//   idx_t top;
//   u8* lines[15];
// };
//
// constant idx_t c_max_line_cache_len = 256;
//
// struct
// linecache_t
// {
//   // cache line index is line.len, and then we check for duplicates in the cacheline.
//   linecache_line_t cache_lines[c_max_line_cache_len];
// };
//
// Inl void
// Zero( linecache_t* cache )
// {
//   Typezero( cache );
// }
//
// // adds the line to the cache if it wasn't there.
// // ringbuffer/fifo cache eviction policy.
// // TODO: consider when the copying happens, etc. right now we make some bad assumptions here.
// Inl slice_t*
// LineCacheLookup(
//   linecache_t* cache,
//   slice_t* line
//   )
// {
//   // opt out of large lines, so our cache doesn't grow w/o bounds.
//   // large lines have it worst anyways, so maybe we should figure out how to cache them effectively.
//   if( line->len > _countof( cache->cache_lines ) ) {
//     return line;
//   }
//   auto cacheline = cache->cache_lines + line->len;
//   ForEach( line_cached, cacheline->lines ) {
//     if( line_cached == line->mem  ||  MemEqual( line_cached, ML( *line ) ) ) {
//       // TODO: throw away the input line contents?
//       return line_cached;
//     }
//   }
//   auto line_cached = cacheline->lines + top;
//   top = ( top + 1 ) % _countof( cacheline->lines );
//   // TODO: copy the line contents?
//   *line_cached = line->mem;
//   return *line_cached;
// }
//
// this kind of cache is O( c_max_line_cache_len * line length ) for lookup.
// assuming c_max_line_cache_len=15, and a max line length of 256, that means roughly 4K bytes tops.
// assuming 1 cycle per byte, that's roughly 1 microsecond. not unreasonable.
//
// alternately, we could just reload the file at idle time or something if we see too much wasted memory.
// keeping track of a content_len and a plist userbytes would let us check a ratio.
// not sure if it's worth writing memory->memory functionality for this reload, or if existing save->load is fine.
// actually doing this would throw away all ability to undo, which isn't ideal.
// we could walk the history chains and save/copy everything to a new plist, but i want to avoid history traversals.
// i'd prefer to do better core datastructure design and caching to help with memory usage.
//
// with our current model i'm seeing on the order of ~10MB usage increase while just typing into files over an hour or so.
// that's a bit high, but our current model is "copy the whole line on any edit", so it's expected.
//
// another caching idea:
//
// since most typing takes place on a single line, make sure continuous typing into one line reuses the same region.
// maybe we do something like over-allocate the current line contents, and then do the usual array geometric growth.
// this could be an ideal candidate for sarray_t actually, which does the capacity = power-of-2-round-up-length thing.
// we have two options:
// - do the sarray_t thing on all lines, recently-edited or not. we'll waste at most file.size bytes, on average file.size / 2 bytes.
// - store an 'edited' flag on each line, and only do the sarray_t thing on those lines. from-file lines stay exact-bounded.
// note we'd have to be careful about when we set/reset line flags.
//
// we'd also need to change how we store line modify undos, since right now it's an entire-line swap.
//

//
// yet another conception of text storage:
//
// the key idea being, the number of diff slices back when we had slice-space favoring was in the thousands.
// that's small enough that maybe we didn't really need constant-time line lookups.
//
// if we need to speed this up further, we could use skiplists. although maintaining those are no fun.
//
// struct
// diff_t
// {
//   slice_t slice;
//   array32_t<u32> line_starts;
// };
//
// struct
// buf_t
// {
//   array32_t<diff_t> diffs;
//   plist_t mem;
// };
//

//
// fixed-size view of things:
//
// struct
// chunk_t
// {
//   u8 data[chunk_size];
//   array32_t<u16> line_starts;
// };
// struct
// buf_t
// {
//   list_t<chunk_t> chunks;
// };
//


//
// even more fixed-size view:
//
// this keeps a fixed-size, editable view of a subset of the file.
// that way we only do line-logic on what's visible / used 99% of the time; the viewable lines.
//
// this doesn't allow for arbitrary line lookups; we're really constrained to the view space.
//
// idx_t max_lines_in_view = 1000; // resizeable based on render dimensions?
// struct
// line_t
// {
//   slice_t slice;
// };
// struct
// buf_t
// {
//   line_t lines[max_lines_in_view];
//   slice_t view_bounds;
//   string_t whole_file;
// };
//






// TODO: perf analysis and tuning of the pagetree approach in LB.
//   main_test's big fuzz test on a really small file seems to show LB=1 is slightly slower.
//   something like 1.40sec -> 1.44sec in ship build, so it's not a huge difference.
//   but definitely slower.
//   on a larger ~40MB file, i'm seeing roughly the same differences; ~1.11x slower with LB=1.
//   it's still acceptably fast; cpu rasterization is dominant, not buf_t issues.
//   the theoretical benefit is really for increasing our linecount cliff.
//   but we have other issues capping us at lower linecounts, so i haven't quite tested massive files yet.
//   e.g. a ~5M line file will cause us to try to alloc space for ~7M lines, which blows past array_t limits.
//   that puts the max filesize on LB=0 to roughly 200MB, assuming our test file has usual line lens.
//   that's lower than i'd want, so maybe that alone is enough to justify getting rid of LB=0.
#define LB   1



// already had the idea to store 'is_nontrivial_toplevel' per-line, for easier custom toplevel editing view.
// also had the idea of a fundamental 'edited' flag to indicate the line is an sarray_t into the plist, for better memory usage.
// i can also imagine 'unsaved_changes', which we render somehow to show what's locally changed.
// maybe we render 'edited' too; VS does this with a yellow for 'unsaved_changes', green for 'edited', blank else.
constant u32 lineflag_bit_arrayline = 0;

struct
line_t
{
  u8* mem;
  u32 len;
  u32 flags;
};

Enumc( undoableopertype_t )
{
  checkpt,
  add, // new slice stored in slice_new
  mod, // change stored in slice_old -> slice_new
  rem, // old slice stored in slice_old
  permuteu,
  permuted,
};

#define PAGEHISTORY   0
// TODO: for massive copy/paste, we generate separate opers for each line.
// this leads to massive expansion of the buf's history.
// undoableoper_t is ~40bytes right now, so we can have ~2M of them before we blow up the array_t len threshold.
// we should consider ways to cut down on the history size in that case.
// we may also have to convert history to be a pagearray or something else.
// linkedlist structures are totally fine, since we only iterate element at a time. no random accessing.
//
// i'd prefer we add some bulk-line opers, since we push many line edits at once; see ForwardLineOper called in loops.
// even with that mitigation though, we probably still want pagearray / list of some kind here.
// using list_t adds 16bytes for forward and back listelem_t pointers, per oper.
// that's kind of a lot, relative to ~40bytes. so i think i'd prefer pagearray.
// pagearray has more complicated iteration, but it's likely worth it here.
//
// one more question is: should we use the buf_t plist, or make another plist on the buf_t for these opers?
// i think we're already copying lines to the existing plist, so probably that's fine.
// indeed, line edits always create new contents in the existing plist, preserving the old one in place.
// if we ever want to allow dynamic history deletion, then a separate plist might be nice to have.
//
// another size optimization we could do is one-line diffing, for oper type mod.
// when we're replacing a subset of the line, we don't need to preserve the old line in it's entirety.
// this goes hand in hand with the line_t flags for marking is_pointing_to_original_file, and leaving
// extra allocation space when that's not the case.
struct
undoableoper_t
{
  undoableopertype_t type;
  union {
    struct { // for add, mod, rem
      u32 y;
      line_t line_old;
      line_t line_new;
    };
    struct { // for permuteu, permuted
      u32 y_start;
      u32 y_end;
    };
  };
};

// NOTE: this value is tied to our use of pagetree_11x2_t in buf_t below.
// we use the lowest 10 bits for within-lineblock addressing, and the highest 22 bits for pagetree addressing.
// just as an additional note, we probably want c_lineblock_size to be >= 2 * nlines_visible, so rendering happens in one block.
constant u32 c_lineblock_size = 1 << 10;

struct
lineblock_t
{
  line_t lines[c_lineblock_size];
};

// to access a line, this is the model:
//   line = AccessLine( &buf->internal_lineblocks, buf->internal_idx_from_line.mem[ line ] )
struct
buf_t
{
  plist_t plist;
  // TODO: we hit array limits on internal_idx_from_line.
  //   our current limit is 100MB for regular arrays, which is 25M lines.
  // this is what's encoding line ordering, so we can't use an unordered hash map for example.
  // one interesting idea is maybe to use a linked list, so we don't have to slide array elements over.
  // 
  // 
  array32_t<u32> internal_idx_from_line; // len = number of actual lines.
  array32_t<u32> unused_internal_idxs; // len = O( max len of internal_idx_from_line )
#if LB
  pagetree_11x2_t internal_lineblocks;
#else
  array32_t<line_t> unordered_lines; // TODO: do something different. see above comment blocks.
#endif
#if PAGEHISTORY
#else
  array_t<undoableoper_t> history; // TODO: do something different. this can blow past array_t limits.
  idx_t history_idx;
#endif
  string_t orig_file_contents;
};

Inl u32
NLines( buf_t* buf )
{
  return buf->internal_idx_from_line.len;
}

Inl u32
LastLine( buf_t* buf )
{
  auto nlines = NLines( buf );
  AssertCrash( nlines );
  return nlines - 1;
}

#if LB
Inl line_t*
TryAccessLine( buf_t* buf, u32 internal_idx )
{
//  ProfFunc();
  auto blockaddr = internal_idx / c_lineblock_size;
  auto lineaddr  = internal_idx % c_lineblock_size;
  auto entry = TryAccessLastLevelPage( &buf->internal_lineblocks, blockaddr );
  if( !entry ) {
    return 0;
  }
  auto block = Cast( lineblock_t*, entry );
  auto line = block->lines + lineaddr;
  return line;
}

Inl line_t*
AccessLine( buf_t* buf, u32 internal_idx )
{
//  ProfFunc();
  auto blockaddr = internal_idx / c_lineblock_size;
  auto lineaddr  = internal_idx % c_lineblock_size;
  auto entry = AccessLastLevelPage( &buf->internal_lineblocks, blockaddr );
  if( !*entry ) {
    *entry = AddPlist( buf->plist, lineblock_t, _SIZEOF_IDX_T, 1 );
    Memzero( *entry, sizeof( lineblock_t ) );
  }
  auto block = Cast( lineblock_t*, *entry );
  auto line = block->lines + lineaddr;
  return line;
}
#else
#endif


#if LB

// note we use TryAccessLine since the lines should already exist if we're iterating over them.

#define FORALLLINES( _buf, _linemem, _lineidx ) \
  Fori( u32, _lineidx, 0, NLines( _buf ) ) { \
    auto _linemem = TryAccessLine( ( _buf ), ( _buf )->internal_idx_from_line.mem[ _lineidx ] ); \
    AssertCrash( _linemem ); \

#define FORLINES( _buf, _linemem, _lineidx, _line_start, _line_end ) \
  AssertCrash( _line_start <= _line_end ); \
  AssertCrash( _line_end <= NLines( _buf ) ); \
  Fori( u32, _lineidx, _line_start, _line_end ) { \
    auto _linemem = TryAccessLine( ( _buf ), ( _buf )->internal_idx_from_line.mem[ _lineidx ] ); \
    AssertCrash( _linemem ); \

#define REVERSEFORLINES( _buf, _linemem, _lineidx, _line_start, _line_end ) \
  AssertCrash( _line_start <= _line_end ); \
  AssertCrash( _line_end <= NLines( _buf ) ); \
  ReverseFori( u32, _lineidx, _line_start, _line_end ) { \
    auto _linemem = TryAccessLine( ( _buf ), ( _buf )->internal_idx_from_line.mem[ _lineidx ] ); \
    AssertCrash( _linemem ); \

#else

#define FORALLLINES( _buf, _linemem, _lineidx ) \
  Fori( u32, _lineidx, 0, NLines( _buf ) ) { \
    auto _linemem = ( _buf )->unordered_lines.mem + ( _buf )->internal_idx_from_line.mem[ _lineidx ]; \

#define FORLINES( _buf, _linemem, _lineidx, _line_start, _line_end ) \
  AssertCrash( _line_start <= _line_end ); \
  AssertCrash( _line_end <= NLines( _buf ) ); \
  Fori( u32, _lineidx, _line_start, _line_end ) { \
    auto _linemem = ( _buf )->unordered_lines.mem + ( _buf )->internal_idx_from_line.mem[ _lineidx ]; \

#define REVERSEFORLINES( _buf, _linemem, _lineidx, _line_start, _line_end ) \
  AssertCrash( _line_start <= _line_end ); \
  AssertCrash( _line_end <= NLines( _buf ) ); \
  ReverseFori( u32, _lineidx, _line_start, _line_end ) { \
    auto _linemem = ( _buf )->unordered_lines.mem + ( _buf )->internal_idx_from_line.mem[ _lineidx ]; \
    ( void )_linemem; \

#endif

#if LB
Inl line_t*
LineFromY( buf_t* buf, u32 y )
{
//  ProfFunc();
  AssertCrash( y < NLines( buf ) );
  auto linemem = TryAccessLine( buf, buf->internal_idx_from_line.mem[ y ] );
  AssertCrash( linemem );
  return linemem;
}
#else
Inl line_t*
LineFromY( buf_t* buf, u32 y )
{
  ProfFunc();
  AssertCrash( y < NLines( buf ) );
  auto linemem = buf->unordered_lines.mem + buf->internal_idx_from_line.mem[ y ];
  AssertCrash( linemem );
  return linemem;
}
#endif

Inl bool
IsEmpty( buf_t* buf )
{
  auto nlines = NLines( buf );
  AssertCrash( nlines );
  if( nlines > 1 ) {
    return 0;
  }
  auto line = LineFromY( buf, 0 );
  return !line->len;
}

Inl void
_CheckNoDupes(
  buf_t* buf
  )
{
#if DEBUGSLOW
  auto nlines = NLines( buf );
  AssertCrash( nlines );
  array_t<u8> counts;
#if LB
  auto nlines_conservative = nlines + buf->unused_internal_idxs.len;
  Alloc( counts, nlines_conservative );
  Memzero( AddBack( counts, nlines_conservative ), nlines_conservative );
#else
  Alloc( counts, buf->unordered_lines.len );
  Memzero( AddBack( counts, buf->unordered_lines.len ), buf->unordered_lines.len );
#endif
  Fori( u32, i, 0, nlines ) {
    auto internal = buf->internal_idx_from_line.mem[i];
    AssertCrash( !counts.mem[internal] );
    counts.mem[internal] = 1;
  }
  Fori( u32, i, 0, buf->unused_internal_idxs.len ) {
    auto internal = buf->unused_internal_idxs.mem[i];
    AssertCrash( !counts.mem[internal] );
    counts.mem[internal] = 1;
  }
  Free( counts );
#endif
}

Inl void
LineInsert(
  buf_t* buf,
  u32 y,
  line_t* line_new
  )
{
  ProfFunc();
  AssertCrash( y <= NLines( buf ) );
  auto nlines = NLines( buf );
  u32 internal;
#if LB
  if( buf->unused_internal_idxs.len ) {
    internal = buf->unused_internal_idxs.mem[ buf->unused_internal_idxs.len - 1 ];
    RemBack( buf->unused_internal_idxs );
  }
  else {
    internal = nlines;
  }
  auto line = AccessLine( buf, internal );
#else
  line_t* line;
  if( buf->unused_internal_idxs.len ) {
    internal = buf->unused_internal_idxs.mem[ buf->unused_internal_idxs.len - 1 ];
    line = buf->unordered_lines.mem + internal;
    RemBack( buf->unused_internal_idxs );
  }
  else {
    internal = nlines;
    line = AddBack( buf->unordered_lines );
  }
#endif
  *AddAt( buf->internal_idx_from_line, y ) = internal;
  *line = *line_new;
  _CheckNoDupes( buf );
}

Inl void
LineRemove(
  buf_t* buf,
  u32 y,
  line_t* verify_removed
  )
{
  ProfFunc();
  AssertCrash( y < NLines( buf ) );
  auto internal = buf->internal_idx_from_line.mem[ y ];
#if LB
  auto removed = TryAccessLine( buf, internal );
#else
  auto removed = buf->unordered_lines.mem + internal;
#endif
  if( verify_removed ) {
    AssertCrash( removed->mem == verify_removed->mem );
    AssertCrash( removed->len == verify_removed->len );
  }
  // note that trying to compact unordered_lines leads to all kinds of complications.
  // we'd need to know the inverse permutation as well as the forward permutation, e.g. what idx_hashset_t does.
  // this is so we could fixup the internal_idx_from_line entry for the line compaction.
  // that's a bit more expensive than we want here, so we do a reclamation strategy instead.
  *AddBack( buf->unused_internal_idxs ) = internal;
  RemAt( buf->internal_idx_from_line, y );
  _CheckNoDupes( buf );
}

Inl void
LineReplace(
  buf_t* buf,
  u32 y,
  line_t* dst_verify,
  line_t* src
  )
{
  ProfFunc();
  auto line = LineFromY( buf, y );
  if( dst_verify ) {
    AssertCrash( line->mem == dst_verify->mem );
    AssertCrash( line->len == dst_verify->len );
  }
  *line = *src;
  _CheckNoDupes( buf );
}

Inl void
LinePermuteU(
  buf_t* buf,
  u32 y_start,
  u32 y_end,
  bool* moved
  )
{
  ProfFunc();
  AssertCrash( y_start <= y_end );
  AssertCrash( y_end < NLines( buf ) );
  if( !y_start ) {
    *moved = 0;
    return;
  }
  auto first_internal = buf->internal_idx_from_line.mem[ y_start - 1 ];
  Fori( u32, y, y_start - 1, y_end ) {
    buf->internal_idx_from_line.mem[ y ] = buf->internal_idx_from_line.mem[ y + 1 ];
  }
  buf->internal_idx_from_line.mem[ y_end ] = first_internal;
  *moved = 1;
  _CheckNoDupes( buf );
}

Inl void
LinePermuteD(
  buf_t* buf,
  u32 y_start,
  u32 y_end,
  bool* moved
  )
{
  ProfFunc();
  AssertCrash( y_start <= y_end );
  AssertCrash( y_end < NLines( buf ) );
  auto last_line = LastLine( buf );
  if( y_end == last_line ) {
    *moved = 0;
    return;
  }
  auto last_internal = buf->internal_idx_from_line.mem[ y_end + 1 ];
  ReverseFori( u32, y, y_start + 1, y_end + 2 ) {
    buf->internal_idx_from_line.mem[ y ] = buf->internal_idx_from_line.mem[ y - 1 ];
  }
  buf->internal_idx_from_line.mem[ y_start ] = last_internal;
  *moved = 1;
  _CheckNoDupes( buf );
}


// =================================================================================
// FIRST / LAST CALLS
//

Inl void
Zero( buf_t* buf )
{
  Zero( buf->internal_idx_from_line );
  Zero( buf->unused_internal_idxs );
#if LB
  buf->internal_lineblocks = {};
#else
  Zero( buf->unordered_lines );
#endif
  Zero( buf->plist );
#if PAGEHISTORY
#else
  Zero( buf->history );
#endif
  Zero( buf->orig_file_contents );
  buf->history_idx = 0;
}

Inl void
Init( buf_t* buf )
{
  Zero( buf );
}

Inl void
Kill( buf_t* buf )
{
  Free( buf->internal_idx_from_line );
  Free( buf->unused_internal_idxs );
#if LB
  Kill( &buf->internal_lineblocks );
#else
  Free( buf->unordered_lines );
#endif
  Kill( buf->plist );
#if PAGEHISTORY
#else
  Free( buf->history );
#endif
  Free( buf->orig_file_contents );
  Zero( buf );
}



// =================================================================================
// FILESYS INTERFACE CALLS
//

constantold slice_t c_crlf = SliceFromCStr( "\r\n" );
constantold slice_t c_cr = SliceFromCStr( "\r" );
constantold slice_t c_lf = SliceFromCStr( "\n" );

Enumc( eoltype_t )
{
  crlf,
  cr,
  lf,
};

Inl slice_t
EolString( eoltype_t type )
{
  switch( type ) {
    case eoltype_t::crlf: return c_crlf;
    case eoltype_t::cr: return c_cr;
    case eoltype_t::lf: return c_lf;
    default: UnreachableCrash();  return {};
  }
}

void
BufLoadEmpty( buf_t* buf )
{
  Init( buf->plist, 4000 );
  Alloc( buf->internal_idx_from_line, 8 );
  *AddBack( buf->internal_idx_from_line ) = 0;
  Alloc( buf->unused_internal_idxs, 8 );
#if LB
  Init( &buf->internal_lineblocks, &buf->plist );
  auto first_line = AccessLine( buf, 0 );
  *first_line = {};
#else
  Alloc( buf->unordered_lines, 8 );
  *AddBack( buf->unordered_lines ) = {};
#endif
#if PAGEHISTORY
#else
  Alloc( buf->history, 8 );
#endif
  Zero( buf->orig_file_contents );
  buf->history_idx = 0;
}

void
BufLoad( buf_t* buf, file_t* file, eoltype_t* eoltype_detected )
{
  ProfFunc();
  AssertCrash( file->size < MAX_idx );

  constant u64 c_chunk_size = 200*1000*1000;
  Init( buf->plist, CLAMP( Cast( idx_t, file->size ), 4000, c_chunk_size ) );

  Prof( BufLoad_FileAlloc );
  buf->orig_file_contents = FileAlloc( *file );
  auto filemem = SliceFromString( buf->orig_file_contents );
  ProfClose( BufLoad_FileAlloc );

#if LB
  pagearray_t<slice32_t> lines;
  Init( lines, 65000 );
  Init( &buf->internal_lineblocks, &buf->plist );
  idx_t num_cr = 0;
  idx_t num_lf = 0;
  idx_t num_crlf = 0;
  Prof( BufLoad_SplitIntoLines );
  SplitIntoLines( &lines, ML( filemem ), &num_cr, &num_lf, &num_crlf );
  AssertCrash( lines.totallen <= MAX_u32 );
  auto nlines = Cast( u32, lines.totallen );
  auto nlines_alloc = Cast( u32, MIN( MAX_u32, ( 4 * Cast( u64, nlines ) ) / 3 ) );
  ProfClose( BufLoad_SplitIntoLines );

  // pick eoltype based on the eols we saw.
  // we just pick the most frequently one seen.
  // ties are broken in the order: crlf, lf, cr. that seems like the most
  // reasonable order to me.
  {
    auto max_eof = MAX3( num_cr, num_lf, num_crlf );
    if( max_eof == num_crlf ) {
      *eoltype_detected = eoltype_t::crlf;
    }
    elif( max_eof == num_lf ) {
      *eoltype_detected = eoltype_t::lf;
    }
    else {
      *eoltype_detected = eoltype_t::cr;
    }
  }

  Prof( BufLoad_SetupPageTreeContents );
  auto pa_iter = MakeIteratorAtLinearIndex( lines, 0 );
  Fori( u32, i, 0, nlines ) {
    auto line = GetElemAtIterator( lines, pa_iter );
    pa_iter = IteratorMoveR( lines, pa_iter );

    // PERF: not ideal to copy array like this. could split into place directly.
    auto dst = AccessLine( buf, i ); // note internal_idx is the trivial [0, nlines-1] here on load.
    dst->mem = line->mem;
    dst->len = line->len;
    dst->flags = 0;
  }
  ProfClose( BufLoad_SetupPageTreeContents );
  Kill( lines );

#else
  Prof( BufLoad_CountNewlines );
  auto nlines = CountNewlines( ML( filemem ) ) + 1;
  auto nlines_alloc = ( 4 * nlines ) / 3;
  ProfClose( BufLoad_CountNewlines );

  array32_t<slice32_t> lines;
  Alloc( lines, nlines );
  SplitIntoLines( &lines, ML( filemem ) );
  // PERF: not ideal to copy array like this. unordered_lines is going away soon anyways, so leave this here for now.
  Alloc( buf->unordered_lines, nlines_alloc );
  buf->unordered_lines.len = nlines;
  FORLEN( line, i, lines )
    auto dst = buf->unordered_lines.mem + i;
    dst->mem = line->mem;
    dst->len = line->len;
    dst->flags = 0;
  }
  Free( lines );
#endif

  Prof( BufLoad_SetupPermutationArray );
  Alloc( buf->internal_idx_from_line, nlines_alloc );
  Fori( u32, i, 0, nlines ) {
    *AddBack( buf->internal_idx_from_line ) = i;
  }
  ProfClose( BufLoad_SetupPermutationArray );

  Alloc( buf->unused_internal_idxs, 1000 );

#if PAGEHISTORY
#else
  Alloc( buf->history, 256 );
#endif
  buf->history_idx = 0;
}

// TODO: check filesys retvals?
void
BufSave( buf_t* buf, file_t* file, eoltype_t eoltype )
{
  ProfFunc();

  slice_t eol = EolString( eoltype );
  constant idx_t c_chunk_size = 200*1024*1024;
  string_t chunk;
  Alloc( chunk, c_chunk_size );
  idx_t chunkpos = 0;
  idx_t bytepos = 0;
  auto last_line = LastLine( buf );
  FORALLLINES( buf, line, i )
    if( chunkpos + line->len <= chunk.len ) {
      Memmove( chunk.mem + chunkpos, ML( *line ) );
      chunkpos += line->len;
    }
    else {
      if( chunkpos ) {
        FileWrite( *file, bytepos, chunk.mem, chunkpos );
        bytepos += chunkpos;
        chunkpos = 0;
      }
      if( chunkpos + line->len <= chunk.len ) {
        Memmove( chunk.mem + chunkpos, ML( *line ) );
        chunkpos += line->len;
      }
      else {
        FileWrite( *file, bytepos, ML( *line ) );
        bytepos += line->len;
      }
    }
    if( i != last_line ) {
      // same exact treatment for eol.
      if( chunkpos + eol.len <= chunk.len ) {
        Memmove( chunk.mem + chunkpos, ML( eol ) );
        chunkpos += eol.len;
      }
      else {
        if( chunkpos ) {
          FileWrite( *file, bytepos, chunk.mem, chunkpos );
          bytepos += chunkpos;
          chunkpos = 0;
        }
        // eol is fixed-size and smaller than the chunk, so we don't need more logic here.
        AssertCrash( chunkpos + eol.len <= chunk.len );
        Memmove( chunk.mem + chunkpos, ML( eol ) );
        chunkpos += eol.len;
      }
    }
  }
  if( chunkpos ) {
    FileWrite( *file, bytepos, chunk.mem, chunkpos );
    bytepos += chunkpos;
    chunkpos = 0;
  }
  FileSetEOF( *file, bytepos );
  Free( chunk );
}

// easier version of chunking write?
#if 0
        // PERF: we shouldn't bother copying chunks that are already appropriately sized.
        // i.e. if a diff is already a reasonable medium size, we shouldn't copy it for chunking.

        idx_t contentlen = buf.content_len;
        idx_t nchunks = contentlen / c_chunk_size;
        idx_t nrem    = contentlen % c_chunk_size;

        idx_t bytepos = 0;
        content_ptr_t bufpos = GetBOF( buf );
        For( i, 0, nchunks + 1 ) {
          auto chunk_size = ( i == nchunks )  ?  nrem  :  c_chunk_size;
          if( chunk_size ) {
            bufpos = Contents( buf, bufpos, chunk.mem, chunk_size );
            FileWrite( file, bytepos, chunk.mem, chunk_size );
            bytepos += chunk_size;
          }
        }
        FileSetEOF( file, contentlen );
#endif



// =================================================================================
// CONTENT-INDEPENDENT NAVIGATION CALLS
//   these only depend on file size.
//

Inl void
CursorCharL(
  buf_t* buf,
  u32* x,
  u32* y
  )
{
  AssertCrash( *y < NLines( buf ) );
  if( *x ) {
    *x -= 1;
  }
  else {
    if( *y ) {
      *y -= 1;
      auto line = LineFromY( buf, *y );
      *x = line->len;
    }
  }
}
Inl void
CursorCharL(
  buf_t* buf,
  u32* x,
  u32* y,
  u32 len_x
  )
{
  while( len_x-- ) {
    CursorCharL( buf, x, y );
  }
}

Inl void
CursorCharR(
  buf_t* buf,
  u32* x,
  u32* y
  )
{
  AssertCrash( *y < NLines( buf ) );
  auto last_line = LastLine( buf );
  auto line = LineFromY( buf, *y );
  if( *x < line->len ) {
    *x += 1;
  }
  elif( *y != last_line ) {
    *y += 1;
    *x = 0;
  }
}
Inl void
CursorCharR(
  buf_t* buf,
  u32* x,
  u32* y,
  u32 len_x
  )
{
  while( len_x-- ) {
    CursorCharR( buf, x, y );
  }
}



// =================================================================================
// CONTENT SEARCH CALLS
//

void
FindFirstInlineR(
  buf_t* buf,
  u32 x_start,
  u32 y_start,
  u8* str,
  u32 str_len,
  u32* x_result,
  u32* y_result,
  bool* found,
  bool case_sensitive,
  bool word_boundary
  )
{
  ProfFunc();

  *found = 0;
  if( !str_len ) {
    return;
  }
  AssertCrash( str );

  auto nlines = NLines( buf );
  auto x = x_start;
  FORLINES( buf, line, y, y_start, nlines )
    AssertCrash( x <= line->len );
    auto f = CsIdxScanR( x_result, line->mem, line->len, x, str, str_len, case_sensitive, word_boundary );
    x = 0;
    if( f ) {
      *found = 1;
      *y_result = y;
      return;
    }
  }
}

void
FindFirstInlineL(
  buf_t* buf,
  u32 x_start,
  u32 y_start,
  u8* str,
  u32 str_len,
  u32* x_result,
  u32* y_result,
  bool* found,
  bool case_sensitive,
  bool word_boundary
  )
{
  ProfFunc();

  *found = 0;
  if( !str_len ) {
    return;
  }
  AssertCrash( str );

  auto x = x_start;
  REVERSEFORLINES( buf, line, y, 0, y_start + 1 )
    if( y == y_start ) {
      AssertCrash( x <= line->len );
    }
    else {
      x = line->len;
    }
    auto f = CsIdxScanL( x_result, line->mem, line->len, x, str, str_len, case_sensitive, word_boundary );
    if( f ) {
      *found = 1;
      *y_result = y;
      return;
    }
  }
}



// =================================================================================
// CONTENT MODIFY CALLS
//

Inl undoableoper_t*
AddHistorical( buf_t* buf )
{
  // invalidate previous futures.
#if PAGEHISTORY
#else
  AssertCrash( buf->history_idx <= buf->history.len );
  buf->history.len = buf->history_idx;
  auto oper = AddBack( buf->history );
  buf->history_idx += 1;
  return oper;
#endif
}

Inl void
WipeHistories( buf_t* buf )
{
#if PAGEHISTORY
#else
  AssertCrash( buf->history_idx <= buf->history.len );
  buf->history.len = 0;
  buf->history_idx = 0;
#endif
}

Inl void
ForwardLineOper(
  buf_t* buf,
  u32 y,
  line_t* line_old,
  line_t* line_new,
  undoableopertype_t type
  )
{
  auto oper = AddHistorical( buf );
  oper->type = type;
  if( line_old ) {
    oper->line_old = *line_old;
  } else {
    oper->line_old = {};
  }
  if( line_new ) {
    oper->line_new = *line_new;
  } else {
    oper->line_new = {};
  }
  oper->y = y;

  // actually perform the operation:

  switch( type ) {
    case undoableopertype_t::add: {
      LineInsert( buf, y, line_new );
    } break;
    case undoableopertype_t::mod: {
      LineReplace( buf, y, line_old, line_new );
    } break;
    case undoableopertype_t::rem: {
      LineRemove( buf, y, line_old );
    } break;
    case undoableopertype_t::checkpt: UnreachableCrash();
    default: UnreachableCrash();
  }
}
Inl void
ForwardPermuteOper(
  buf_t* buf,
  u32 y_start,
  u32 y_end,
  undoableopertype_t type,
  bool* moved
  )
{
  switch( type ) {
    case undoableopertype_t::permuteu: {
      LinePermuteU( buf, y_start, y_end, moved );
    } break;
    case undoableopertype_t::permuted: {
      LinePermuteD( buf, y_start, y_end, moved );
    } break;
    default: UnreachableCrash();
  }
  // we skip adding undo opers for permutes that don't actually permute.
  // e.g. trying to PermuteD the very last line is a no-op.
  // this is so that undo of such a no-op doesn't actually do anything.
  // e.g. to undo the PermuteD, trying to PermuteU the very last line _will_ do something.
  // note we can do this as a post-op check, since the oper doesn't contain anything from the pre-op.
  if( *moved ) {
    auto oper = AddHistorical( buf );
    oper->type = type;
    oper->y_start = y_start;
    oper->y_end = y_end;
  }
}


Inl void
Replace(
  buf_t* buf,
  u32 x_start,
  u32 y_start,
  u32 x_end,
  u32 y_end,
  u8* str,
  idx_t str_len,
  u32* x_finish = 0,
  u32* y_finish = 0
  )
{
  AssertCrash( y_start <= y_end );
  AssertCrash( Implies( y_start == y_end, x_start <= x_end ) );
  // TODO: see if we can merge some of the 4 cases here.
  AssertCrash( y_end < NLines( buf ) );
  auto nlines = CountNewlines( str, str_len ) + 1;
  array32_t<slice32_t> lines;
  Alloc( lines, nlines );
  SplitIntoLines( &lines, str, str_len );
  if( y_start == y_end  &&  lines.len == 1 ) {
    auto add = lines.mem[0];
    auto line = LineFromY( buf, y_start );
    AssertCrash( x_start <= x_end );
    AssertCrash( x_end <= line->len );
    auto len_x = x_end - x_start;
    if( x_finish ) {
      *x_finish = x_start + add.len;
    }
    if( y_finish ) {
      *y_finish = y_start;
    }
    // TODO: this is likely to grow a bit too fast.
    //   we used to grow by O( num chars typed ), this is O( num chars typed * line length ).
    //   we probably want to optimize for the last-line-modified, since that's the most likely edit.
    //   at minimum we could likely avoid allocing the line contents up to x.
    line_t line_new;
    line_new.len = line->len - len_x + add.len;
    line_new.mem = AddPlist( buf->plist, u8, 1, line_new.len );
    line_new.flags = 0;
    Memmove( line_new.mem, line->mem, x_start );
    Memmove( line_new.mem + x_start, ML( add ) );
    Memmove( line_new.mem + x_start + add.len, line->mem + x_end, line->len - x_end );
    ForwardLineOper(
      buf,
      y_start,
      line,
      &line_new,
      undoableopertype_t::mod
      );
  }
  elif( y_start != y_end  &&  lines.len == 1 ) {
    // merge the last line into the first.
    auto add = lines.mem[0];
    auto line_start = LineFromY( buf, y_start );
    auto line_end = LineFromY( buf, y_end );
    AssertCrash( x_start <= line_start->len );
    AssertCrash( x_end <= line_end->len );
    if( x_finish ) {
      *x_finish = x_start + add.len;
    }
    if( y_finish ) {
      *y_finish = y_start;
    }
    line_t line_new;
    line_new.len = x_start + add.len + line_end->len - x_end;
    line_new.mem = AddPlist( buf->plist, u8, 1, line_new.len );
    line_new.flags = 0;
    Memmove( line_new.mem, line_start->mem, x_start );
    Memmove( line_new.mem + x_start, ML( add ) );
    Memmove( line_new.mem + x_start + add.len, line_end->mem + x_end, line_end->len - x_end );
    ForwardLineOper(
      buf,
      y_start,
      line_start,
      &line_new,
      undoableopertype_t::mod
      );
    // delete intermediate lines in their entirety, as well as the final line we've already merged into the first.
    REVERSEFORLINES( buf, line, y, y_start + 1, y_end + 1 )
      ForwardLineOper(
        buf,
        y,
        line,
        0,
        undoableopertype_t::rem
        );
    }
  }
  elif( y_start == y_end  &&  lines.len > 1 ) {
    auto line = LineFromY( buf, y_start );
    auto line_orig = *line;
    AssertCrash( x_start <= x_end );
    AssertCrash( x_end <= line->len );
    auto add_first = lines.mem[0];
    auto add_last = lines.mem[ lines.len - 1 ];
    if( x_finish ) {
      *x_finish = add_last.len;
    }
    if( y_finish ) {
      *y_finish = y_start + lines.len - 1;
    }
    line_t line_new;
    line_new.len = x_start + add_first.len;
    line_new.mem = AddPlist( buf->plist, u8, 1, line_new.len );
    line_new.flags = 0;
    Memmove( line_new.mem, line->mem, x_start );
    Memmove( line_new.mem + x_start, ML( add_first ) );
    ForwardLineOper(
      buf,
      y_start,
      line,
      &line_new,
      undoableopertype_t::mod
      );
    Fori( u32, i, 1, lines.len - 1 ) {
      auto add = lines.mem[i];
      line_new.len = add.len;
      line_new.mem = AddPlist( buf->plist, u8, 1, line_new.len );
      line_new.flags = 0;
      Memmove( line_new.mem, ML( add ) );
      ForwardLineOper(
        buf,
        y_start + i,
        0,
        &line_new,
        undoableopertype_t::add
        );
    }
    line_new.len = add_last.len + line_orig.len - x_end;
    line_new.mem = AddPlist( buf->plist, u8, 1, line_new.len );
    line_new.flags = 0;
    Memmove( line_new.mem, ML( add_last ) );
    Memmove( line_new.mem + add_last.len, line_orig.mem + x_end, line_orig.len - x_end );
    ForwardLineOper(
      buf,
      y_start + lines.len - 1,
      0,
      &line_new,
      undoableopertype_t::add
      );
  }
  else {
    auto line_start = LineFromY( buf, y_start );
    auto line_end = LineFromY( buf, y_end );
    AssertCrash( x_start <= line_start->len );
    AssertCrash( x_end <= line_end->len );
    auto add_first = lines.mem[0];
    auto add_last = lines.mem[ lines.len - 1 ];
    if( x_finish ) {
      *x_finish = add_last.len;
    }
    if( y_finish ) {
      *y_finish = y_start + lines.len - 1;
    }
    line_t line_new;
    line_new.len = x_start + add_first.len;
    line_new.mem = AddPlist( buf->plist, u8, 1, line_new.len );
    line_new.flags = 0;
    Memmove( line_new.mem, line_start->mem, x_start );
    Memmove( line_new.mem + x_start, ML( add_first ) );
    ForwardLineOper(
      buf,
      y_start,
      line_start,
      &line_new,
      undoableopertype_t::mod
      );
    // delete intermediate lines in their entirety, as well as the final line we've already merged into the first.
    REVERSEFORLINES( buf, line, y, y_start + 1, y_end + 1 )
      ForwardLineOper(
        buf,
        y,
        line,
        0,
        undoableopertype_t::rem
        );
    }
    // add intermediate lines in their entirety.
    // TODO: consolidate these with the rems above to make mods, making fewer ops overall?
    Fori( u32, i, 1, lines.len - 1 ) {
      auto add = lines.mem[i];
      line_new.len = add.len;
      line_new.mem = AddPlist( buf->plist, u8, 1, line_new.len );
      line_new.flags = 0;
      Memmove( line_new.mem, ML( add ) );
      ForwardLineOper(
        buf,
        y_start + i,
        0,
        &line_new,
        undoableopertype_t::add
        );
    }
    line_new.len = add_last.len + line_end->len - x_end;
    line_new.mem = AddPlist( buf->plist, u8, 1, line_new.len );
    line_new.flags = 0;
    Memmove( line_new.mem, ML( add_last ) );
    Memmove( line_new.mem + add_last.len, line_end->mem + x_end, line_end->len - x_end );
    ForwardLineOper(
      buf,
      y_start + lines.len - 1,
      0,
      &line_new,
      undoableopertype_t::add
      );
  }
  Free( lines );
}

//
// insert contents at the given position.
// e.g. Insert( "0123456789", 4, "asdf" )' -> "0123asdf456789"
//
Inl void
Insert(
  buf_t* buf,
  u32 x,
  u32 y,
  u8* str,
  idx_t str_len,
  u32* x_finish = 0,
  u32* y_finish = 0
  )
{
  // insert = replace nothing with the given string
  Replace(
    buf,
    x,
    y,
    x,
    y,
    str,
    str_len,
    x_finish,
    y_finish
    );
}

//
// delete contents in the range: [a, b)
// e.g. Delete( "0123456789", 4, 8 ) -> "012389"
//
Inl void
Delete(
  buf_t* buf,
  u32 x_start,
  u32 y_start,
  u32 x_end,
  u32 y_end,
  u32* x_finish = 0,
  u32* y_finish = 0
  )
{
  // delete = replace with nothing
  Replace(
    buf,
    x_start,
    y_start,
    x_end,
    y_end,
    0,
    0,
    x_finish,
    y_finish
    );
}

Inl void
DeleteLine(
  buf_t* buf,
  line_t* line,
  u32 y
  )
{
  if( !y  &&  NLines( buf ) == 1 ) {
    line_t line_new;
    line_new.len = 0;
    line_new.mem = 0;
    line_new.flags = 0;
    ForwardLineOper( buf, y, line, &line_new, undoableopertype_t::mod );
  }
  else {
    ForwardLineOper( buf, y, line, 0, undoableopertype_t::rem );
  }
}


// =================================================================================
// CONTENT MODIFY UNDO CALLS
//

void
UndoCheckpt( buf_t* buf )
{
  auto checkpt = AddHistorical( buf );
  checkpt->type = undoableopertype_t::checkpt;
}


void
Undo( buf_t* buf )
{
#if PAGEHISTORY
#else
  AssertCrash( buf->history_idx <= buf->history.len );
  AssertCrash( buf->history_idx == buf->history.len  ||  buf->history.mem[buf->history_idx].type == undoableopertype_t::checkpt );
#endif
  if( !buf->history_idx ) {
    return;
  }

  bool loop = 1;
  while( loop ) {
    buf->history_idx -= 1;
#if PAGEHISTORY
#else
    auto oper = buf->history.mem + buf->history_idx;
#endif

    // undo this operation:
    switch( oper->type ) {
      case undoableopertype_t::checkpt: {
        loop = 0;
      } break;
      case undoableopertype_t::add: {
        LineRemove( buf, oper->y, &oper->line_new );
      } break;
      case undoableopertype_t::mod: {
        LineReplace( buf, oper->y, &oper->line_new, &oper->line_old );
      } break;
      case undoableopertype_t::rem: {
        LineInsert( buf, oper->y, &oper->line_old );
      } break;
      case undoableopertype_t::permuteu: {
        bool moved;
        // TODO: consider changing what we store in the oper to make this simpler.
        AssertCrash( oper->y_start );
        AssertCrash( oper->y_end );
        LinePermuteD( buf, oper->y_start - 1, oper->y_end - 1, &moved );
        AssertCrash( moved );
      } break;
      case undoableopertype_t::permuted: {
        bool moved;
        LinePermuteU( buf, oper->y_start + 1, oper->y_end + 1, &moved );
        AssertCrash( moved );
      } break;
      default: UnreachableCrash();
    }
  }

#if PAGEHISTORY
#else
  AssertCrash( buf->history_idx == buf->history.len  ||  buf->history.mem[buf->history_idx].type == undoableopertype_t::checkpt );
#endif
}



void
Redo( buf_t* buf )
{
#if PAGEHISTORY
#else
  AssertCrash( buf->history_idx <= buf->history.len );
  AssertCrash( buf->history_idx == buf->history.len  ||  buf->history.mem[buf->history_idx].type == undoableopertype_t::checkpt );
  if( buf->history_idx == buf->history.len ) {
    return;
  }
#endif

  buf->history_idx += 1;

  bool loop = 1;
  while( loop ) {
#if PAGEHISTORY
#else
    if( buf->history_idx == buf->history.len ) {
      break;
    }
    auto oper = buf->history.mem + buf->history_idx;
#endif

    // redo this operation:
    switch( oper->type ) {
      case undoableopertype_t::checkpt: {
        loop = 0;
      } break;
      case undoableopertype_t::add: {
        LineInsert( buf, oper->y, &oper->line_new );
      } break;
      case undoableopertype_t::mod: {
        LineReplace( buf, oper->y, &oper->line_old, &oper->line_new );
      } break;
      case undoableopertype_t::rem: {
        LineRemove( buf, oper->y, &oper->line_old );
      } break;
      case undoableopertype_t::permuteu: {
        bool moved;
        LinePermuteU( buf, oper->y_start, oper->y_end, &moved );
      } break;
      case undoableopertype_t::permuted: {
        bool moved;
        LinePermuteD( buf, oper->y_start, oper->y_end, &moved );
      } break;
      default: UnreachableCrash();
    }

    if( loop ) {
      buf->history_idx += 1;
    }
  }

#if PAGEHISTORY
#else
  AssertCrash( buf->history_idx == buf->history.len  ||  buf->history.mem[buf->history_idx].type == undoableopertype_t::checkpt );
#endif
}



// =================================================================================
// CONTENT-DEPENDENT NAVIGATION CALLS
//

Inl u32
CursorInlineEOL(
  buf_t* buf,
  u32 y
  )
{
  auto line = LineFromY( buf, y );
  return line->len;
}



Inl u32
CursorInlineCharL(
  buf_t* buf,
  u32 x,
  u32 y,
  u32 len_x
  )
{
  auto line = LineFromY( buf, y );
  AssertCrash( x <= line->len );
  x = MAX( x, len_x ) - len_x;
  return x;
}
Inl u32
CursorInlineCharR(
  buf_t* buf,
  u32 x,
  u32 y,
  u32 len_x
  )
{
  auto line = LineFromY( buf, y );
  AssertCrash( x <= line->len );
  x = MIN( x + len_x, line->len );
  return x;
}





// TODO: rename!
Inl void
CursorSkipWordSpacetabNewlineL(
  buf_t* buf,
  u32* x,
  u32* y
  )
{
  auto line = LineFromY( buf, *y );
  auto x0 = CursorStopAtNonWordCharL( ML( *line ), *x );
  auto x1 = CursorSkipSpacetabL( ML( *line ), *x );
  auto x2 = *x;
  auto y2 = *y;
  CursorCharL( buf, &x2, &y2 );
  if( *y != y2 ) {
    *x = x2;
    *y = y2;
  }
  else {
    *x = MIN3( x0, x1, x2 );
  }
}
// TODO: rename!
Inl void
CursorSkipWordSpacetabNewlineR(
  buf_t* buf,
  u32* x,
  u32* y
  )
{
  auto line = LineFromY( buf, *y );
  auto x0 = CursorStopAtNonWordCharR( ML( *line ), *x );
  auto x1 = CursorSkipSpacetabR( ML( *line ), *x );
  auto x2 = *x;
  auto y2 = *y;
  CursorCharR( buf, &x2, &y2 );
  if( *y != y2 ) {
    *x = x2;
    *y = y2;
  }
  else {
    *x = MAX3( x0, x1, x2 );
  }
}






Inl void
CursorLineU(
  buf_t* buf,
  u32* x,
  u32* y,
  u32 x_virtual,
  u32 dy
  )
{
  AssertCrash( *y < NLines( buf ) );
  *y = MAX( *y, dy ) - dy;
  *x = MIN( x_virtual, CursorInlineEOL( buf, *y ) );
}
Inl void
CursorLineD(
  buf_t* buf,
  u32* x,
  u32* y,
  u32 x_virtual,
  u32 dy
  )
{
  AssertCrash( *y < NLines( buf ) );
  auto last_line = LastLine( buf );
  *y = MIN( last_line, *y + dy );
  *x = MIN( x_virtual, CursorInlineEOL( buf, *y ) );
}


Inl string_t
AllocContents( buf_t* buf, eoltype_t eoltype )
{
  slice_t eol = EolString( eoltype );
  auto last_line = LastLine( buf );

  idx_t bytepos = 0;
  FORALLLINES( buf, line, i )
    bytepos += line->len;
  }
  bytepos += last_line * eol.len;

  string_t r;
  Alloc( r, bytepos );

  bytepos = 0;
  FORALLLINES( buf, line, i )
    Memmove( r.mem + bytepos, ML( *line ) );
    bytepos += line->len;
    if( i != last_line ) {
      Memmove( r.mem + bytepos, ML( eol ) );
      bytepos += eol.len;
    }
  }

  return r;
}

Inl string_t
AllocContents(
  buf_t* buf,
  u32 x_start,
  u32 y_start,
  u32 x_end,
  u32 y_end,
  eoltype_t eoltype
  )
{
  AssertCrash( y_start <= y_end );
  AssertCrash( y_end < NLines( buf ) );
  slice_t eol = EolString( eoltype );

  if( y_start == y_end ) {
    auto line = LineFromY( buf, y_start );
    AssertCrash( x_start <= x_end );
    AssertCrash( x_end <= line->len );
    auto len_x = x_end - x_start;
    string_t r;
    Alloc( r, len_x );
    Memmove( r.mem, line->mem + x_start, len_x );
    return r;
  }

  auto line_start = LineFromY( buf, y_start );
  auto line_end = LineFromY( buf, y_end );
  AssertCrash( x_start <= line_start->len );
  AssertCrash( x_end <= line_end->len );

  idx_t bytecount = 0;
  bytecount += line_start->len - x_start;
  bytecount += eol.len;
  FORLINES( buf, line, y, y_start + 1, y_end )
    bytecount += line->len;
    bytecount += eol.len;
  }
  bytecount += x_end;

  string_t r;
  Alloc( r, bytecount );

  idx_t bytepos = 0;
  Memmove( r.mem + bytepos, line_start->mem + x_start, line_start->len - x_start );
  bytepos += line_start->len - x_start;
  Memmove( r.mem + bytepos, ML( eol ) );
  bytepos += eol.len;
  FORLINES( buf, line, y, y_start + 1, y_end )
    Memmove( r.mem + bytepos, ML( *line ) );
    bytepos += line->len;
    Memmove( r.mem + bytepos, ML( eol ) );
    bytepos += eol.len;
  }
  Memmove( r.mem + bytepos, line_end->mem, x_end );
  bytepos += x_end;

  return r;
}




struct
test_bufchange_t
{
  slice_t* str_old;
  slice_t* str_new;
};

Inl void
AssertCrashBuf( buf_t& buf, u8* str, eoltype_t eoltype )
{
  auto state = AllocContents( &buf, eoltype );
  AssertCrash( MemEqual( ML( state ), str, CsLen( str ) ) );
  Free( state );
}
Inl void
AssertCrashBuf( buf_t& buf, u8* str )
{
  AssertCrashBuf( buf, str, eoltype_t::crlf );
}

static void
TestBuf()
{
  static u8 dump[4096];
  static u8 dump2[4096];

  buf_t buf;
  Init( &buf );
  BufLoadEmpty( &buf );

  {
    // test history -N
    idx_t testcounts[] = {
      1, 2, 5, 10, 20, 17, 7, 35,
    };
    ForEach( historycount, testcounts ) {
      Fori( u32, i, 0, historycount ) {
        dump2[i] = Cast( u8, 'A' + i );
        UndoCheckpt( &buf );
        auto chr = Cast( u8, 'A' + i );
        Insert( &buf, i, 0, &chr, 1 );
      }
      For( i, 0, historycount ) {
        string_t str = AllocContents( &buf, eoltype_t::crlf );
        AssertCrash( MemEqual( dump2, historycount - i, ML( str ) ) );
        Undo( &buf );
        Free( str );
      }
      For( i, 0, historycount ) {
        Redo( &buf );
        string_t str = AllocContents( &buf, eoltype_t::crlf );
        AssertCrash( MemEqual( dump2, 1 + i, ML( str ) ) );
        Free( str );
      }

      Kill( &buf );
      BufLoadEmpty( &buf );
    }
  }

  // test history -1 with complex ops ( >1 elements between checkpoint elements ).
  {
    auto VerifyChange = [&]( slice_t& str_old, slice_t& str_new )
    {
      string_t str = AllocContents( &buf, eoltype_t::crlf );
      AssertCrash( MemEqual( ML( str ), ML( str_new ) ) );
      Free( str );
      Undo( &buf );
      str = AllocContents( &buf, eoltype_t::crlf );
      AssertCrash( MemEqual( ML( str ), ML( str_old ) ) );
      Free( str );
      Redo( &buf );
      str = AllocContents( &buf, eoltype_t::crlf );
      AssertCrash( MemEqual( ML( str ), ML( str_new ) ) );
      Free( str );
    };

    auto str0 = SliceFromCStr( "" );

    auto str1 = SliceFromCStr( "abcdefABCDEF012345" );
    UndoCheckpt( &buf );
    Insert( &buf, 0, 0, ML( str1 ) );
    VerifyChange( str0, str1 );

    auto str2 = SliceFromCStr( "abaaaacdefABCDEF012345" );
    UndoCheckpt( &buf );
    Insert( &buf, 2, 0, str2.mem + 2, 4 );
    VerifyChange( str1, str2 );

    auto str3 = SliceFromCStr( "abaaaacdefDEF012345" );
    UndoCheckpt( &buf );
    auto del_start = 10;
    auto del_end = 13;
    Delete( &buf, del_start, 0, del_end, 0 );
    VerifyChange( str2, str3 );

    auto str4 = SliceFromCStr( "abDEF012345" );
    UndoCheckpt( &buf );
    del_start = 2;
    del_end = 10;
    Delete( &buf, del_start, 0, del_end, 0 );
    VerifyChange( str3, str4 );

    auto str5 = SliceFromCStr( "abDEFab2345" );
    UndoCheckpt( &buf );
    auto repl_start = 5;
    auto repl_end = 7;
    Replace( &buf, repl_start, 0, repl_end, 0, str5.mem, 2 );
    VerifyChange( str4, str5 );

    // TODO:
    //   Replace( buf_t& buf, idx_t pos, u8* str, idx_t len )
    //   Copy( buf_t& buf, idx_t src, idx_t src_len, idx_t dst )
    //   Move( buf_t& buf, idx_t src, idx_t src_len, idx_t dst )

    Kill( &buf );
  }

  // test finding at word boundaries.
  {
    BufLoadEmpty( &buf );

    //                                    0         1         2
    //                                    0123456789012345678901234
    auto findcontents = Slice32FromCStr( "ab aab baaab bbaaa ab bbb" );
    auto find = Slice32FromCStr( "ab" );
    idx_t ab_instances[] = { 0, 19 };

    auto TestFind = [&]()
    {
      u32 start = 0;
      For( i, 0, _countof( ab_instances ) ) {
        u32 match_start;
        u32 match_y;
        bool found;
        FindFirstInlineR( &buf, start, 0, ML( find ), &match_start, &match_y, &found, 1, 1 );
        AssertCrash( found );
        AssertCrash( match_start == ab_instances[i] );
        auto match_end = match_start + find.len;
        auto match = AllocContents( &buf, match_start, 0, match_end, 0, eoltype_t::crlf );
        AssertCrash( MemEqual( ML( match ), ML( find ) ) );
        Free( match );
        start = match_start + 1;
      }

      start = findcontents.len;
      ReverseFor( i, 0, _countof( ab_instances ) ) {
        u32 match_start;
        u32 match_y;
        bool found;
        FindFirstInlineL( &buf, start, 0, ML( find ), &match_start, &match_y, &found, 1, 1 );
        AssertCrash( found );
        AssertCrash( match_start == ab_instances[i] );
        auto match_end = match_start + find.len;
        auto match = AllocContents( &buf, match_start, 0, match_end, 0, eoltype_t::crlf );
        AssertCrash( MemEqual( ML( match ), ML( find ) ) );
        Free( match );
        start = match_start - 1;
      }
    };

    UndoCheckpt( &buf );

    // try with one big starting diff.
    {
      Insert( &buf, 0, 0, ML( findcontents ) );
      TestFind();
    }

    Undo( &buf );

    // now with lots of little starting diffs.
    {
      FORLEN32( chr, i, findcontents )
        Insert( &buf, i, 0, chr, 1 );
      }
      TestFind();
    }

    Kill( &buf );
  }

  {
    BufLoadEmpty( &buf );

    //                                    01234567890123456789
    auto findcontents = Slice32FromCStr( "abaabbaaabbbaaaabbbb" );
    auto find = Slice32FromCStr( "ab" );
    idx_t ab_instances[] = { 0, 3, 8, 15 };

    auto TestFind = [&]()
    {
      u32 start = 0;
      For( i, 0, _countof( ab_instances ) ) {
        u32 match_start;
        u32 match_y;
        bool found;
        FindFirstInlineR( &buf, start, 0, ML( find ), &match_start, &match_y, &found, 1, 0 );
        AssertCrash( found );
        AssertCrash( match_start == ab_instances[i] );
        auto match_end = match_start + find.len;
        auto match = AllocContents( &buf, match_start, 0, match_end, 0, eoltype_t::crlf );
        AssertCrash( MemEqual( ML( match ), ML( find ) ) );
        Free( match );
        start = match_start + 1;
      }

      start = findcontents.len;
      ReverseFor( i, 0, _countof( ab_instances ) ) {
        u32 match_start;
        u32 match_y;
        bool found;
        FindFirstInlineL( &buf, start, 0, ML( find ), &match_start, &match_y, &found, 1, 0 );
        AssertCrash( found );
        AssertCrash( match_start == ab_instances[i] );
        auto match_end = match_start + find.len;
        auto match = AllocContents( &buf, match_start, 0, match_end, 0, eoltype_t::crlf );
        AssertCrash( MemEqual( ML( match ), ML( find ) ) );
        Free( match );
        start = match_start - 1;
      }
    };

    UndoCheckpt( &buf );

    // try with one big starting diff.
    {
      Insert( &buf, 0, 0, ML( findcontents ) );
      TestFind();
    }

    Undo( &buf );

    // now with lots of little starting diffs.
    {
      FORLEN32( chr, i, findcontents )
        Insert( &buf, i, 0, chr, 1 );
      }
      TestFind();
    }

    Kill( &buf );
  }

  {
    BufLoadEmpty( &buf );

    //                                    0123456789
    auto findcontents = Slice32FromCStr( "aaaaaaaaaa" );
    auto find = Slice32FromCStr( "aaa" );
    idx_t find_instances[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

    Insert( &buf, 0, 0, ML( findcontents ) );

    u32 start = 0;
    For( i, 0, _countof( find_instances ) ) {
      u32 match_start;
      u32 match_y;
      bool found;
      FindFirstInlineR( &buf, start, 0, ML( find ), &match_start, &match_y, &found, 1, 0 );
      AssertCrash( found );
      AssertCrash( match_start == find_instances[i] );
      auto match_end = match_start + find.len;
      auto match = AllocContents( &buf, match_start, 0, match_end, 0, eoltype_t::crlf );
      AssertCrash( MemEqual( ML( match ), ML( find ) ) );
      Free( match );
      start = match_start + 1;
    }

    start = findcontents.len;
    ReverseFor( i, 0, _countof( find_instances ) ) {
      u32 match_start;
      u32 match_y;
      bool found;
      FindFirstInlineL( &buf, start, 0, ML( find ), &match_start, &match_y, &found, 1, 0 );
      AssertCrash( found );
      AssertCrash( match_start == find_instances[i] );
      auto match_end = match_start + find.len;
      auto match = AllocContents( &buf, match_start, 0, match_end, 0, eoltype_t::crlf );
      AssertCrash( MemEqual( ML( match ), ML( find ) ) );
      Free( match );
      start = match_start - 1;
    }

    Kill( &buf );
  }

  // try deleting a bunch of different substrings that cross diff_t boundaries.
  // operating on diff_t storage gets pretty complex, so cover some edge cases here.
  {
    BufLoadEmpty( &buf );

    auto tmp = SliceFromCStr( "0123456789\r\n" );
    u32 c = 6;
    Fori( u32, i, 0, c ) {
      Insert( &buf, i * 10, 0, ML( tmp ) );
    }
    Fori( u32, k, 1, c ) {
      u32 del_start = 0;
      For( i, 0, 10 ) {
        UndoCheckpt( &buf );
        auto del_end = del_start + k * 10;
        Delete( &buf, del_start, 0, del_end, 0 );
        Undo( &buf );
        del_start += 1;
      }
    }

    Kill( &buf );
  }

#if 0
  // copy
  {
    BufLoadEmpty( &buf );

    auto tmp = SliceFromCStr( "0123456789" );
    u8* expected[] = {
      Str( "34560123456789" ),
      Str( "03456123456789" ),
      Str( "01345623456789" ),
      Str( "01234563456789" ),
      Str( "01233456456789" ),
      Str( "01234345656789" ),
      Str( "01234534566789" ),
      Str( "01234563456789" ),
      Str( "01234567345689" ),
      Str( "01234567834569" ),
      Str( "01234567893456" ),
    };
    idx_t src_idx = 3;
    idx_t src_len = 4;

    constant idx_t tmp_len = 10;
    AssertCrash( tmp.len == tmp_len );
    idx_t expected_ptrs[ _countof( expected ) ][ tmp_len ] = {
      4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
      0, 5, 6, 7, 8, 9, 10, 11, 12, 13,
      0, 1, 6, 7, 8, 9, 10, 11, 12, 13,
      0, 1, 2, 7, 8, 9, 10, 11, 12, 13,
      0, 1, 2, 3, 8, 9, 10, 11, 12, 13,
      0, 1, 2, 3, 4, 9, 10, 11, 12, 13,
      0, 1, 2, 3, 4, 5, 10, 11, 12, 13,
      0, 1, 2, 3, 4, 5,  6, 11, 12, 13,
      0, 1, 2, 3, 4, 5,  6,  7, 12, 13,
      0, 1, 2, 3, 4, 5,  6,  7,  8, 13,
      0, 1, 2, 3, 4, 5,  6,  7,  8,  9,
    };

    UndoCheckpt( &buf );

    auto TestCopy = [&]()
    {
      auto src = CursorCharR( buf, bof, src_idx, 0 );
      auto dst = bof;
      For( i, 0, tmp.len + 1 ) {
        UndoCheckpt( buf );
        auto copy_start = src;
        auto copy_end = CursorCharR( buf, src, src_len, 0 );

        content_ptr_t test_ptrs[ tmp_len ];
        For( j, 0, _countof( test_ptrs ) ) {
          test_ptrs[j] = CursorCharR( buf, bof, j, 0 );
        }
        content_ptr_t* concurrents[ _countof( test_ptrs ) ];
        For( j, 0, _countof( concurrents ) ) {
          concurrents[j] = test_ptrs + j;
        }

        Copy( buf, copy_start, copy_end, dst, AL( concurrents ) );

        AssertCrashBuf( buf, expected[i] );

        For( j, 0, _countof( test_ptrs ) ) {
          auto test_ptr = CountBytesBetween( buf, bof, test_ptrs[j] );
          AssertCrash( test_ptr == expected_ptrs[i][j] );
        }

        Undo( buf );
        dst = CursorCharR( buf, dst, 1, 0 );
      }
    };

    // try with one big starting diff.
    {
      Insert( buf, bof, ML( tmp ), 0, 0 );
      TestCopy();
    }

    Undo( buf );

    // now with lots of little starting diffs.
    {
      Insert( buf, bof, ML( tmp ), 0, 0 );
      For( i, 1, tmp.len ) {
        auto split_pos = CursorCharR( buf, bof, i, 0 );
        _SplitDiffAt( buf, split_pos, 0, 0 );
      }
      TestCopy();
    }

    Kill( buf );
  }

  // move
  {
    BufLoadEmpty( buf );

    auto tmp =  SliceFromCStr( "012345" );
    u8* expected = Str( "301245" );
    idx_t dst_idx = 0;
    idx_t src_idx = 3;
    idx_t src_len = 1;

    Insert( buf, bof, ML( tmp ), 0, 0 );
    For( i, 1, tmp.len ) {
      auto split_pos = CursorCharR( buf, bof, i, 0 );
      _SplitDiffAt( buf, split_pos, 0, 0 );
    }
    auto src_start = CursorCharR( buf, bof, src_idx, 0 );
    auto dst = CursorCharR( buf, bof, dst_idx, 0 );
    auto src_end = CursorCharR( buf, src_start, src_len, 0 );
    Move( buf, src_start, src_end, dst, 0, 0 );
    AssertCrashBuf( buf, expected );

    Kill( buf );
  }

  // move
  {
    BufLoadEmpty( buf );

    auto tmp = SliceFromCStr( "0123456789" );
    constant idx_t tmp_len = 10;
    AssertCrash( tmp.len == tmp_len );

    u8* expected[] = {
      Str( "3456012789" ),
      Str( "0345612789" ),
      Str( "0134562789" ),
      Str( "0123456789" ),
      Str( "0123456789" ),
      Str( "0123456789" ),
      Str( "0123456789" ),
      Str( "0123456789" ),
      Str( "0127345689" ),
      Str( "0127834569" ),
      Str( "0127893456" ),
    };
    idx_t src_idx = 3;
    idx_t src_len = 4;

    UndoCheckpt( buf );

    auto TestMove = [&]()
    {
      auto src_start = CursorCharR( buf, bof, src_idx, 0 );
      auto dst = bof;
      For( i, 0, tmp.len + 1 ) {
        UndoCheckpt( buf );

        content_ptr_t test_ptrs[ tmp_len ];
        For( j, 0, _countof( test_ptrs ) ) {
          test_ptrs[j] = CursorCharR( buf, bof, j, 0 );
        }
        content_ptr_t* concurrents[ _countof( test_ptrs ) ];
        For( j, 0, _countof( concurrents ) ) {
          concurrents[j] = test_ptrs + j;
        }

        auto src_end = CursorCharR( buf, src_start, src_len, 0 );
        Move( buf, src_start, src_end, dst, AL( concurrents ) );

        AssertCrashBuf( buf, expected[i] );

        For( j, 0, _countof( test_ptrs ) ) {
          auto test_ptr = CountBytesBetween( buf, bof, test_ptrs[j] );

          // since we're just permuting elements, and our starting string is sequential indices;
          // we can use the expected string's index-of-char to find the expected dst location.
          idx_t expected_ptr;
          AssertCrash( j < 10 );
          bool found = CsIdxScanR( &expected_ptr, expected[i], tmp_len, 0, '0' + Cast( u8, j ) );
          AssertCrash( found );
          AssertCrash( test_ptr == expected_ptr );
        }

        Undo( buf );
        dst = CursorCharR( buf, dst, 1, 0 );
      }
    };

    // try with one big starting diff.
    {
      Insert( buf, bof, ML( tmp ), 0, 0 );
      TestMove();
    }

    Undo( buf );

    // now with lots of little starting diffs.
    {
      Insert( buf, bof, ML( tmp ), 0, 0 );
      For( i, 1, tmp.len ) {
        auto split_pos = CursorCharR( buf, bof, i, 0 );
        _SplitDiffAt( buf, split_pos, 0, 0 );
      }
      TestMove();
    }

    Kill( buf );
  }
#endif


  // SPECIFIC BUGS

#if 0
  // FindFirstL, with a partial match that fails to match at a diff_t boundary.
  {
    BufLoadEmpty( &buf );

    //                           0123456789
    test_str_t findcontents = { "ytest" };
    test_str_t find = { "xtest" };

    Insert( &buf, 0, 0, ML( findcontents ) );

    // split at a half-matching boundary, to stress test straddling that border.
    auto split_pos = CursorCharR( buf, bof, 1, 0 );
    _SplitDiffAt( buf, split_pos, 0, 0 );

    content_ptr_t start = GetEOF( buf );
    content_ptr_t match_end;
    bool found;
    FindFirstL( buf, start, ML( find ), &match_end, &found, 0, 0 );
    AssertCrash( !found );

    Kill( buf );
  }
#endif

  // FindFirstInlineR, with a partial match that fails to match at EOF
  {
    BufLoadEmpty( &buf );

    //                           0123456789
    auto findcontents = SliceFromCStr( "for" );
    auto find = Slice32FromCStr( "rdy" );

    Insert( &buf, 0, 0, ML( findcontents ) );

    u32 start = 0;
    u32 match_start;
    u32 match_y;
    bool found;
    FindFirstInlineR( &buf, start, 0, ML( find ), &match_start, &match_y, &found, 0, 0 );
    AssertCrash( !found );

    Kill( &buf );
  }
}
