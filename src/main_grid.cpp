// build:window_x64_debug
// Copyright (c) John A. Carlos Jr., all rights reserved.

#define FINDLEAKS   0
#include "common.h"
#include "math_vec.h"
#include "math_matrix.h"
#include "ds_slice.h"
#include "ds_string.h"
#include "ds_plist.h"
#include "ds_array.h"
#include "ds_embeddedarray.h"
#include "ds_fixedarray.h"
#include "ds_pagearray.h"
#include "ds_list.h"
#include "ds_bytearray.h"
#include "ds_hashset.h"
#include "ds_pagetree.h"
#include "cstr.h"
#include "filesys.h"
#include "timedate.h"
#include "threading.h"
#define LOGGER_ENABLED   1
#include "logger.h"
#define PROF_ENABLED   0
#define PROF_ENABLED_AT_LAUNCH   0
#include "profile.h"
#include "rand.h"
#include "main.h"

#define OPENGL_INSTEAD_OF_SOFTWARE       0
#define GLW_RAWINPUT_KEYBOARD            0
#define GLW_RAWINPUT_KEYBOARD_NOHOTKEY   1
#include "glw.h"

#include "ui_propdb.h"
#include "ui_font.h"
#include "ui_render.h"
#include "ui_buf.h"
#include "ui_txt.h"

Enumc( applayer_t )
{
  bkgd,
  txt,
  COUNT
};



struct
cell_t;
struct
cellvalue_t;

using
cellvaluearray_t = tslice32_t<cellvalue_t>;

struct
cellerror_t
{
  slice32_t text;
  u32 start; // into cell's input
  u32 end;
};

Enumc( cellvaluetype_t )
{
  _f64,
  _string,
  _array,
  _graph, // maps to _array storage, with each elem storing another array of numbers. so basically 2d data.
};
struct
cellvalue_t
{
  union {
    f64 _f64;
    slice32_t _string;
    cellvaluearray_t _array;
  };
  cellvaluetype_t type;
};

//
// PERF: ugghh, C++ packs this terribly. each nested struct gets aligned as if it was an array,
// even if it's not. I've no idea why they do that; it should effectively inline and reorder for
// optimal packing, if it were up to me.
// i guess they do that so codegen doesn't have to treat nested structs differently?
// yeah, external fns that take a struct* wouldn't know the specific relayout, if you did a
// 'external_fn( &cell->value )' for example.
// if we had a fancy language it could respecialize the fn for that specific layout,
// knowing that the value came from a cell_t.
// or just inline all structs everywhere, even fn params. then no respecialization needed.
// you'd have to rely on codegen optimization to eliminate some of the unnecessary inline expansion/collapse.
// but that's kind of what i want codegen to do for me, pick the fastest way to encode a fncall, pack my data, etc.
// e.g. passing a cell_t* would blow out to passing a ptr for each basic-type field of cell_t.
// what about passing cell_t**? presumably just an address, but what about writes to a cell_t*?
// to do reads/writes thru cell_t* generally, we'd want a fixed layout.
// it sounds hard to do layout analysis, where we tag individual layouts, and codegen relayouts
// when deemed more optimal to use a different one.
// presumably that would reduce lots of unnecessary-moves, same kinds of effects i see with __forceinline.
// just even more aggressive, since C++ doesn't relayout structs at all, from what i can tell.
// we might get most of the bang-for-buck if we just strip trailing padding in nested structs.
// as long as everyone manually optimizes their structs individually, then at least we wouldn't
// have the trailing padding in nested structs problem that C++ has.
// i guess there's concerns around writing a whole struct at a time, so you'd have to know to avoid
// writing over the trailing padding in all cases.
// e.g. sizeof( struct foo_t{ u8* a; u32 b; } ) would be 16 bytes on x64 for array alignment, but any individual
// writes, e.g. *foo = {}; would have to write only 12 bytes. since in some contexts, when it's a nested struct,
// the size would really be 12 bytes.
// i'm not sure why C++ doesn't do this.
//
// note this is zero-initialized
//
struct
cell_t
{
#define PACKCELL 0

#if !PACKCELL
  // this is the final calculated value, not the formula that produces that result.
  cellvalue_t value;

  // the grid's eval_generation is incremented each time we start an eval loop,
  // and this is stored on the cell so we can do eval cycle detection.
  // a generation number means we don't have to do a clear-flag pre-pass.
  u32 eval_generation;

  // note it might be worth sharing this stuff across cells, for mem/speed reasons.
  slice32_t input; // source string that's parsed into an expression, and calculated for a final value.
  cellerror_t error;
#else

  u8* error_text_mem;
  u8* input_mem;

  u8* value_mem;
  u32 value_len;

  u32 input_len;

  u32 eval_generation;
  u16 value_type;

  u16 error_text_len;
  u16 error_start; // into cell's input
  u16 error_end;

#endif
};

struct
cell2_t
{
  u8* error_text_mem;
  u8* input_mem;

  u8* value_mem;
  u32 value_len;

  u32 input_len;

  u32 eval_generation;
  u16 value_type;

  u16 error_text_len;
  u16 error_start; // into cell's input
  u16 error_end;
};

Inl bool
HasError( cellerror_t* error )
{
  bool r = error->text.len;
  return r;
}



// TODO: this is a pretty good candidate for a page table.
//   the only thing is, that would likely limit us to some fixed grid size.
//   well if we did a block-streaming approach, we could probably adapt that to be dynamic.
//   sounds worth trying at some point.
// e.g.
//   we have 64bit address, in the form of [ blockpos.x, blockpos.y ] or maybe reversed. whatever.
//   each last-level-page is just a chunk of memory representing [c_cellblock_size] cell_t.
//   depending how we slice and dice the address, we can make some number of table/dir levels.
//   our current cellblock size is 12288 to 18432 bytes.
//   for equivalently sized table/dir pages, we'd store 1536 to 2304 pointers.
//   nearest powers of two are 2^10=1024 and 2^11=2048.
//   i don't think we need any extra per-entry bits, other than "is it 0 or not" to tell if we've
//   alloc'ed the page entry yet.
//
//   for address slicing/dicing, it might make sense to do a full bit interleave of x and y.
//   this kind of gives us the "sort by distance to 0,0" ordering, which seems like it'd be desirable.
//   although it's probably better to favor y more, since i'd bet row count is generally greater
//   than col count.
//
//   for an example addressing assuming our 16x16 cellblock,
//   [ blockpos.x, blockpos.y ] or some smarter interleaving
//   [ 10b  | 18b | 18b | 18b ]
//   so we'd have a 4-level page tree, with a smaller top level.
//   2^18=262,144 entries is 2MB assuming 8byte pointer entries.
//   2^10=1024 entries is 8KB assuming 8byte pointer entries.
//   for a minimal grid with just one cellblock, our overhead is:
//   8192 + 2097152 + 2097152 + 2097152 = 6299648, or roughly 6MB.
//   the cellblock data itself is 12KB to 18KB, so that's a lot of overhead.
//   but on modern machines, 6MB is a drop in the bucket.
//   it's only 4 large pages, if we alloc them right with the OS.
//   we could make that a little smaller by equalizing the page level bits.
//   the theoretical maximum memory usage is:
//   for cellblocks alone, 2^64 * ( 12KB to 18KB ) = way bigger than any RAM available.
//   for our overhead, 8 * ( 2^10 * 2^18 * 2^18 * 2^18 ) = 8 * 2^64 = also way bigger than anything.
//   ratio of overhead to cellblocks: 8 / ( 12KB to 18KB ) = 0.000651 to 0.000434
//   that's the theoretically best ratio; we'll usually do way worse than that due to sparseness and
//   the big sizes of table/dir pages.
//
//   what about an equal split?
//   [ 16b | 16b | 16b | 16b ]
//   2^16=65536 entries is 512K assuming 8byte pointer entries.
//   for a mimimal grid with one cellblock, the overhead is:
//   512K + 512K + 512K + 512K = 2MB.
//   that's better than 6MB.
//   but we might lose out to the 18bit approach, if it's really better to align to the OS large page size.
//
//   note with 32bit x,y; the maximum row/col counts are 16*4B=64B. that's way bigger than we could ever
//   fit into memory.
//
//   what might be cool is a dynamic number of levels in the heirarchy.
//   presumably small grids don't need or want more than 1 level of indirection.
//
//   i can imagine a fast path for the first entries of each level.
//   i.e. we cache a direct pointer to the page for entry 0 at each level, and during
//   address resolution, we can use the direct pointer if the index is 0.
//   that trades a pointer lookup for an if-0 branch; not sure if that's worth it.
//
//   i can also imagine redefining the addressing format on the fly.
//   i think that would require rewriting the entire heirarchy, excepting the LL pages.
//   that seems like a reasonable way of doing things, if we can pick the format smartly enough.
//   ideally we could actually optimize for the current session, trying different formats until
//   we find the best one.
//   it might be better to just define fixed cost formulas, with cpu cache sizes etc. as given parameters.
//   we'd be limited in the amount of optimization, since this doesn't really consider different LL
//   page sizes.
//   the only thing stopping that freedom is the slowness of rewriting every cellblock.
//

constant u32 c_cellblock_dim_x = 16;
constant u32 c_cellblock_dim_y = 16;
constant u32 c_cellblock_size = c_cellblock_dim_x * c_cellblock_dim_y;
constantold vec2<u32> c_cellblock_dim = _vec2<u32>( c_cellblock_dim_x, c_cellblock_dim_y );
constantold vec2<f32> c_cellblock_dimf = CastVec2( f32, c_cellblock_dim );

#define PT   1


struct
cellblock_t
{
#if PT
  cell_t cells[c_cellblock_size];
#else
  // core data
  vec2<u32> blockpos; // global cellblock index
  cell_t cells[c_cellblock_size];
#endif
};

#if PT
#else
Inl void
Init( cellblock_t* block, vec2<u32> blockpos )
{
  block->blockpos = blockpos;
  Memzero( block->cells, sizeof( block->cells ) );
}
#endif

struct
custom_dim_x_t
{
  u32 absolutepos_x;
  f32 dim_x;
};

struct
custom_dim_y_t
{
  u32 absolutepos_y;
  f32 dim_y;
};

using absolutepos_t = vec2<u32>;

Inl absolutepos_t
MoveXL( absolutepos_t p, u32 n )
{
  p.x = p.x - MIN( p.x, n );
  return p;
}

Inl absolutepos_t
MoveYL( absolutepos_t p, u32 n )
{
  p.y = p.y - MIN( p.y, n );
  return p;
}

Inl absolutepos_t
MoveXR( absolutepos_t p, u32 n )
{
  p.x = p.x + n;
  return p;
}

Inl absolutepos_t
MoveYR( absolutepos_t p, u32 n )
{
  p.y = p.y + n;
  return p;
}

struct
absoluterect_t
{
  absolutepos_t p0; // included
  absolutepos_t p1; // excluded, i.e. this is "one past the end"
};

using
absoluterectlist_t = array_t<absoluterect_t>;

Inl bool
PtOverlapsInterval( u32 x, u32 x0, u32 x1 )
{
  bool r = ( x0 <= x )  &&  ( x <= x1 );
  return r;
}

Inl bool
AEntirelyContainedWithinB( absoluterect_t a, absoluterect_t b )
{
  bool r =
    LTEandLTE( a.p0.x, b.p0.x, b.p1.x )  &&
    LTEandLTE( a.p1.x, b.p0.x, b.p1.x )  &&
    LTEandLTE( a.p0.y, b.p0.y, b.p1.y )  &&
    LTEandLTE( a.p1.y, b.p0.y, b.p1.y );
  return r;
}

Inl bool
RectsOverlap( absoluterect_t a, absoluterect_t b )
{
  bool overlap_x =
    LTEandLT( a.p0.x, b.p0.x, b.p1.x )  ||
    LTandLTE( a.p1.x, b.p0.x, b.p1.x )  ||
    LTEandLT( b.p0.x, a.p0.x, a.p1.x )  ||
    LTandLTE( b.p1.x, a.p0.x, a.p1.x );

  bool overlap_y =
    LTEandLT( a.p0.y, b.p0.y, b.p1.y )  ||
    LTandLTE( a.p1.y, b.p0.y, b.p1.y )  ||
    LTEandLT( b.p0.y, a.p0.y, a.p1.y )  ||
    LTandLTE( b.p1.y, a.p0.y, a.p1.y );

  bool r = overlap_x  &&  overlap_y;
  return r;
}

Inl bool
RectsEqual( absoluterect_t a, absoluterect_t b )
{
  bool r =
    a.p0 == b.p0  &&
    a.p1 == b.p1;
  return r;
}

Inl idx_t
NumCellsCovered( absoluterectlist_t* rectlist )
{
  idx_t count = 0;
  FORLEN( rectp, i, *rectlist )
    auto rect = *rectp;
    AssertCrash( rect.p0.x <= rect.p1.x );
    AssertCrash( rect.p0.y <= rect.p1.y );
    auto rect_count = ( rect.p1.x - rect.p0.x ) * ( rect.p1.y - rect.p0.y );
    count += rect_count;
  }
  return count;
}

Inl bool
RectlistContainsAbspos(
  absoluterectlist_t* rectlist,
  absolutepos_t abspos
  )
{
  FORLEN( rect, i, *rectlist )
    auto inside =
      LTEandLT( abspos.x, rect->p0.x, rect->p1.x )  &&
      LTEandLT( abspos.y, rect->p0.y, rect->p1.y );
    if( inside ) {
      return 1;
    }
  }
  return 0;
}

// slices up the rects in rectlist_modify s.t. none of them overlap with rectlist_readonly.
Inl void
RemoveIntersection(
  absoluterectlist_t* rectlist_readonly,
  absoluterectlist_t* rectlist_modify
  )
{
  idx_t i = 0;
LABEL_LOOPAGAIN:
  while( i < rectlist_modify->len ) {
    auto a = rectlist_modify->mem + i;

    FORLEN( b, j, *rectlist_readonly )

      if( !RectsOverlap( *a, *b ) ) {
        continue;
      }

      if( AEntirelyContainedWithinB( *a, *b ) ) {
        UnorderedRemAt( *rectlist_modify, i );
        goto LABEL_LOOPAGAIN;
      }

      bool a0_inside_b_x = b->p0.x <= a->p0.x  &&  a->p0.x <  b->p1.x;
      bool a1_inside_b_x = b->p0.x <  a->p1.x  &&  a->p1.x <= b->p1.x;
      bool a0_inside_b_y = b->p0.y <= a->p0.y  &&  a->p0.y <  b->p1.y;
      bool a1_inside_b_y = b->p0.y <  a->p1.y  &&  a->p1.y <= b->p1.y;
      AssertCrash( !( a0_inside_b_x  &&  a1_inside_b_x  &&  a0_inside_b_y  &&  a1_inside_b_y ) );
      if( a0_inside_b_x  &&  a1_inside_b_x  &&  a0_inside_b_y ) {
        // ----------
        // |B ----  |
        // |  |A |  |
        // |  |  |  |
        // ---|  |---
        //    |  |
        //    ----
        a->p0.y = b->p1.y;
      }
      elif( a0_inside_b_x  &&  a1_inside_b_x  &&  a1_inside_b_y ) {
        //    ----
        //    |A |
        // ---|  |---
        // |B |  |  |
        // |  ----  |
        // ----------
        a->p1.y = b->p0.y;
      }
      elif( a0_inside_b_y  &&  a1_inside_b_y  &&  a0_inside_b_x ) {
        // --------
        // |B     |
        // |  --------
        // |  |A     |
        // |  --------
        // |      |
        // --------
        a->p0.x = b->p1.x;
      }
      elif( a0_inside_b_y  &&  a1_inside_b_y  &&  a1_inside_b_x ) {
        //    --------
        //    |B     |
        // --------  |
        // |A     |  |
        // --------  |
        //    |      |
        //    --------
        a->p1.x = b->p0.x;
      }
      elif( a0_inside_b_x  &&  a0_inside_b_y ) {
        // ----------         ----------
        // |B       |         |B       |
        // |  ----------      |        |---
        // |  |A       |      |        |A |
        // ---|        |      ---------|  |
        //    |        |         |C    |  |
        //    ----------         ----------
        auto c = AddBack( *rectlist_modify );
        c->p0.x = a->p0.x;
        c->p0.y = b->p1.y;
        c->p1.x = b->p1.x;
        c->p1.y = a->p1.y;

        a->p0.x = b->p1.x;
      }
      elif( a1_inside_b_x  &&  a1_inside_b_y ) {
        // ----------         ----------
        // |A       |         |A |C    |
        // |  ----------      |  |---------
        // |  |B       |      |  |B       |
        // ---|        |      ---|        |
        //    |        |         |        |
        //    ----------         ----------
        auto c = AddBack( *rectlist_modify );
        c->p0.x = b->p0.x;
        c->p0.y = a->p0.y;
        c->p1.x = a->p1.x;
        c->p1.y = b->p0.y;

        a->p1.x = b->p0.x;
      }
      elif( a0_inside_b_x  &&  a1_inside_b_y ) {
        //    ----------         ----------
        //    |A       |         |C    |A |
        // ---|        |      ---------|  |
        // |B |        |      |B       |  |
        // |  ----------      |        |---
        // |        |         |        |
        // ----------         ----------
        auto c = AddBack( *rectlist_modify );
        c->p0.x = a->p0.x;
        c->p0.y = a->p0.y;
        c->p1.x = b->p1.x;
        c->p1.y = b->p0.y;

        a->p0.x = b->p1.x;
      }
      elif( a1_inside_b_x  &&  a0_inside_b_y ) {
        //    ----------         ----------
        //    |B       |         |B       |
        // ---|        |      ---|        |
        // |A |        |      |A |        |
        // |  ----------      |  |---------
        // |        |         |  |C    |
        // ----------         ----------
        auto c = AddBack( *rectlist_modify );
        c->p0.x = b->p0.x;
        c->p0.y = b->p1.y;
        c->p1.x = a->p1.x;
        c->p1.y = a->p1.y;

        a->p1.x = b->p0.x;
      }
      elif( a0_inside_b_x  &&  a1_inside_b_x ) {
        //    ----            ----
        //    |A |            |A |
        // ---|  |---      ----------
        // |B |  |  |      |B       |
        // ---|  |---      ----------
        //    |  |            |C |
        //    ----            ----
        auto c = AddBack( *rectlist_modify );
        c->p0.x = a->p0.x;
        c->p0.y = b->p1.y;
        c->p1.x = a->p1.x;
        c->p1.y = a->p1.y;

        a->p1.y = b->p0.y;
      }
      elif( a0_inside_b_y  &&  a1_inside_b_y ) {
        //    ----           ----
        //    |B |           |B |
        // ----------     ---|  |---
        // |A       |     |A |  |C |
        // ----------     ---|  |---
        //    |  |           |  |
        //    ----           ----
        auto c = AddBack( *rectlist_modify );
        c->p0.x = b->p1.x;
        c->p0.y = a->p0.y;
        c->p1.x = a->p1.x;
        c->p1.y = a->p1.y;

        a->p1.x = b->p0.x;
      }
      elif( a0_inside_b_x ) {
        //    --------        --------
        //    |A     |        |A     |
        // --------  |     ----------|
        // |B     |  |     |B     |D |
        // --------  |     ----------|
        //    |      |        |C     |
        //    --------        --------
        auto c = AddBack( *rectlist_modify );
        c->p0.x = a->p0.x;
        c->p0.y = b->p1.y;
        c->p1.x = a->p1.x;
        c->p1.y = a->p1.y;
        auto d = AddBack( *rectlist_modify );
        d->p0.x = b->p1.x;
        d->p0.y = b->p0.y;
        d->p1.x = a->p1.x;
        d->p1.y = b->p1.y;
      }
      elif( a1_inside_b_x ) {
        // --------        --------
        // |A     |        |A     |
        // |  --------     |----------
        // |  |B     |     |D |B     |
        // |  --------     |----------
        // |      |        |C     |
        // --------        --------
        auto c = AddBack( *rectlist_modify );
        c->p0.x = a->p0.x;
        c->p0.y = b->p1.y;
        c->p1.x = a->p1.x;
        c->p1.y = a->p1.y;
        auto d = AddBack( *rectlist_modify );
        d->p0.x = a->p0.x;
        d->p0.y = b->p0.y;
        d->p1.x = b->p0.x;
        d->p1.y = b->p1.y;
      }
      elif( a0_inside_b_y ) {
        //    ----            ----
        //    |B |            |B |
        // ---|  |---      ---|  |---
        // |A |  |  |      |A |  |C |
        // |  ----  |      |  ----  |
        // |        |      |  |D |  |
        // ----------      ----------
        auto c = AddBack( *rectlist_modify );
        c->p0.x = b->p1.x;
        c->p0.y = a->p0.y;
        c->p1.x = a->p1.x;
        c->p1.y = a->p1.y;
        auto d = AddBack( *rectlist_modify );
        d->p0.x = b->p0.x;
        d->p0.y = b->p1.y;
        d->p1.x = b->p1.x;
        d->p1.y = a->p1.y;
      }
      elif( a1_inside_b_y ) {
        // ----------      ----------
        // |A       |      |A |D |C |
        // |  ----  |      |  ----  |
        // |  |B |  |      |  |B |  |
        // ---|  |---      ---|  |---
        //    |  |            |  |
        //    ----            ----
        auto c = AddBack( *rectlist_modify );
        c->p0.x = b->p1.x;
        c->p0.y = a->p0.y;
        c->p1.x = a->p1.x;
        c->p1.y = a->p1.y;
        auto d = AddBack( *rectlist_modify );
        d->p0.x = b->p0.x;
        d->p0.y = a->p0.y;
        d->p1.x = b->p1.x;
        d->p1.y = b->p0.y;
      }
    }

    i += 1;
  }

  // TODO: do we need to deal with empty rects after intersection?
  // check that we don't for now.
  FORLEN( rect, k, *rectlist_modify )
    AssertCrash( rect->p0.x < rect->p1.x );
    AssertCrash( rect->p0.y < rect->p1.y );
  }
}

//
// assumes there's no double+covering going on in rectlist.
//
// WARNING: this is worst-case O( n^3 ) i think, which is terrifying.
// luckily, most rectlists aren't that complicated, so it'll usually be around n^2 i think.
// still slow, but most rectlists are also small, which helps.
//
Inl void
OptimizeRectlist(
  absoluterectlist_t* rectlist
  )
{
  if( !rectlist->len ) {
    return;
  }

LABEL_LOOPAGAIN:
  idx_t i = 0;
  while( i < rectlist->len - 1 ) {
    auto a = rectlist->mem + i;

    Fori( idx_t, j, i + 1, rectlist->len ) {
      auto b = rectlist->mem + j;

      if( a->p0.x == b->p0.x  &&  a->p1.x == b->p1.x ) {
        if( a->p0.y == b->p1.y ) {
          a->p0.y = b->p0.y;
          UnorderedRemAt( *rectlist, j );
          goto LABEL_LOOPAGAIN;
        }
        elif( a->p1.y == b->p0.y ) {
          a->p1.y = b->p1.y;
          UnorderedRemAt( *rectlist, j );
          goto LABEL_LOOPAGAIN;
        }
      }
      elif( a->p0.y == b->p0.y  &&  a->p1.y == b->p1.y ) {
        if( a->p0.x == b->p1.x ) {
          a->p0.x = b->p0.x;
          UnorderedRemAt( *rectlist, j );
          goto LABEL_LOOPAGAIN;
        }
        elif( a->p1.x == b->p0.x ) {
          a->p1.x = b->p1.x;
          UnorderedRemAt( *rectlist, j );
          goto LABEL_LOOPAGAIN;
        }
      }
    }

    i += 1;
  }
}

//
// after thinking a bit, it seems like a good algorithm here would be:
//
// assume the rects in the rectlist don't double+ count any cells.
// that is, every cell is covered by exactly one rect, or not at all.
//
// intersect the new rect against every rect, and throw away the intersection part of the new rect.
// that basically means we either:
//   - abort, because the new rect is totally covered by an existing one; OR,
//   - shrink the size of the new rect, if it's clipped by a large edge; OR,
//   - split the new rect into two new rects, if it's clipped by a corner.
// so every existing rect has the potential to split the new rect into two.
// once an existing rect has caused a split, it can't cause any more.
// so there's at most N new rects generated by splitting.
// obviously, we have to continue iteration for every generated rect.
//
// going thru that intersection/splitting process will generate a rectlist.
// then, we need to merge them in some fashion.
// simple list append would work, we just wouldn't have the shortest rectlist possible.
//
// what's the fastest algorithm for merging two rectlists into the shortest possible rectlist?
// i think it would take exp time, since it seems like you'd have to recheck everything after performing a merge.
// i guess you could shrink the N to only include rect clusters which share edges.
// but even generating that it seems like you'd need N^2 comparisons to find shared edges.
//
// actually, assuming the original rectlist was optimal, i think we could notate rects which share an edge
// with the new rects, since we had to compare already for intersection testing.
// since we can only make a new merge using coverage from new rects by assumption, that's all we'd need.
//
// with that original optimality assumption, i think we also get the result that we only have one
// cluster to test. since we're adding a single new rect that got split with intersections, we'd have to
// check all rects that caused the splitting, leaving us with a contiguous region to check. hence one cluster.
// that leaves us with N rects to consider for merging.
//
// now can we avoid exp rechecking?
// the only actual merging operation we can do is extend one edge out to precisely cover an adjacent rect.
// then throw away the rect we've covered by making the other one larger.
// is there a case where you have to loop trying that, switch to a different rect to loop, and then come
// back to the first one?
// i don't think so, as long as we're appending the adjacency-lists on merge, and then walk the adjacency-lists
// again after merge. that's because the merge is symmetric, so rechecking from one side is enough.
// i'm pretty sure this is exponential still, unfortunately.
//
// web research shows there's some poly time algorithms for this, but they aren't simple.
// generating interior cuts, doing some bipartite graph algorithms, and then more cuts.
// https://www.cise.ufl.edu/~sahni/papers/part.pdf
// it's probably better to just live with suboptimal rectlists, and maybe do a linear pass
// to avoid trivial list growth.
//
// double+ covering seems like a bigger problem to me than rectlist optimality, so.
//
Inl void
AddUnion(
  absoluterectlist_t* rectlist,
  absoluterect_t a,
  bool* changed_rectlist
  )
{
  *changed_rectlist = 0;

  absoluterectlist_t remnants;
  Alloc( remnants, 256 ); // PERF: pull this out, and just reserve(1) here.
  *AddBack( remnants ) = a;

  RemoveIntersection( rectlist, &remnants );
  if( remnants.len ) {
    Memmove( AddBack( *rectlist, remnants.len ), remnants.mem, remnants.len * sizeof( absoluterect_t ) );
    OptimizeRectlist( rectlist );
    *changed_rectlist = 1;
  }

  Free( remnants );
}

// used for mouse click testing
struct
rendered_cell_t
{
  rectf32_t bounds;
  absolutepos_t absolutepos;
};

Enumc( gridfocus_t )
{
  cursel,
  input,
};

struct
cellandpos_t
{
  cell_t* cell;
  absolutepos_t abspos;
};

// we construct these for tracking evaluation dependencies.
// e.g. if cell(1,1) contains "10", and cell(1,2) contains "5+cell(1,1)",
// then we'll make an edge ( cell(1,2), cell(1,1) )
// the directionality is ( superexpr, subexpr )
struct
graphedge_t
{
  cellandpos_t start;
  cell_t* end;
};

struct
grid_t
{
  // core data
  plist_t cellmem;
#if PT
  pagetree_16x4_t cellblock_tree;
#else
  listwalloc_t<cellblock_t> cellblocks;
#endif
  array_t<graphedge_t> graph_edges;
  array_t<custom_dim_x_t> custom_dim_xs; // TODO: implement
  array_t<custom_dim_y_t> custom_dim_ys;
  gridfocus_t focus;
  u32 eval_generation;

  // cursor/selection data
  absoluterectlist_t cursel;
  absolutepos_t c_head;
  absolutepos_t s_head;
  bool has_identical_input;
  bool has_identical_error;
  slice32_t identical_input;
  cellerror_t identical_error;

  txt_t txt_input;
  // TODO: add a txt_t txt_error that's readonly but selectable, so you can copy the error.
  // maybe l-click puts it on clipboard trivially for convenience too?
//  txt_t txt_error;

  // render data
  array_t<rendered_cell_t> rendered_cells;
  absolutepos_t scroll_absolutepos;
};

Inl void
SetCurselToAbspos( grid_t* grid, absolutepos_t abspos );

Inl void
Init( grid_t* grid )
{
  Init( grid->cellmem, 32000 );
#if PT
  Init( &grid->cellblock_tree, &grid->cellmem );
#else
  Init( grid->cellblocks, &grid->cellmem );
#endif
  Alloc( grid->graph_edges, 256 );
  Alloc( grid->custom_dim_xs, 4 );
  Alloc( grid->custom_dim_ys, 4 );
  grid->focus = gridfocus_t::cursel;
  grid->eval_generation = 0;
  Alloc( grid->cursel, 4 );
  grid->c_head = {};
  grid->s_head = {};
  Init( grid->txt_input );
  TxtLoadEmpty( grid->txt_input );
  Alloc( grid->rendered_cells, 1024 );
  grid->scroll_absolutepos = {};

  SetCurselToAbspos( grid, grid->c_head );
}

Inl void
Kill( grid_t* grid )
{
  grid->scroll_absolutepos = {};
  Free( grid->rendered_cells );
  Kill( grid->txt_input );
  grid->c_head = {};
  grid->s_head = {};
  Free( grid->cursel );
  grid->eval_generation = 0;
  grid->focus = gridfocus_t::cursel;
  Free( grid->custom_dim_ys );
  Free( grid->custom_dim_xs );
  Free( grid->graph_edges );
#if PT
  Kill( &grid->cellblock_tree );
#else
  Kill( grid->cellblocks );
#endif
  Kill( grid->cellmem );
}

Inl void
InsertOrSetCellContents(
  grid_t* grid,
  absoluterectlist_t* rectlist,
  slice32_t input
  );

Inl void
LoadCSV(
  grid_t* grid,
  slice_t csv
  )
{
  u32 nlines = CountNewlines( ML( csv ) ) + 1;
  array32_t<slice32_t> lines;
  Alloc( lines, nlines );
  array_t<slice_t> entries;
  Alloc( entries, 32 );
  SplitIntoLines( &lines, ML( csv ) );
  absoluterectlist_t rectlist;
  Alloc( rectlist, 1 );
  FORLEN32( line, y, lines )
    auto nentries = CountCommas( ML( *line ) ) + 1;
    entries.len = 0;
    Reserve( entries, nentries );
    SplitByCommas( &entries, ML( *line ) );
    FORLEN32( entry, x, entries )
      absolutepos_t abspos = { x, y };
      rectlist.len = 0;
      auto rect = AddBack( rectlist );
      rect->p0 = abspos;
      rect->p1 = abspos + _vec2<u32>( 1, 1 );
      AssertCrash( entry->len <= MAX_u32 );
      auto copied = AddPlistSlice32( grid->cellmem, u8, 1, Cast( u32, entry->len ) );
      Memmove( copied.mem, ML( *entry ) );
      InsertOrSetCellContents( grid, &rectlist, copied );
    }
  }
  Free( rectlist );
  Free( entries );
  Free( lines );
}

Inl u32
PosinblockFromAbsolutepos( vec2<u32> blockpos, absolutepos_t abspos )
{
  auto abspos_block = blockpos * c_cellblock_dim;
  AssertCrash( abspos.x >= abspos_block.x );
  AssertCrash( abspos.y >= abspos_block.y );
  auto abspos_relative = abspos - abspos_block;
  return abspos_relative.x + c_cellblock_dim_x * abspos_relative.y;
}

Inl vec2<u32>
BlockposFromAbsolutepos( absolutepos_t abspos )
{
  auto r = _vec2<u32>(
    abspos.x / c_cellblock_dim_x,
    abspos.y / c_cellblock_dim_y
    );
  return r;
}

Inl u64
PagetreeAddressFromBlockpos( vec2<u32> blockpos )
{
  // TODO: better interleaving of bits.
  u64 r = Cast( u64, blockpos.x ) << 32  |  Cast( u64, blockpos.y );
  return r;
}

Inl cellblock_t*
AccessCellBlock(
  grid_t* grid,
  vec2<u32> blockpos
  )
{
  auto blockaddr = PagetreeAddressFromBlockpos( blockpos );
  auto entry = AccessLastLevelPage( &grid->cellblock_tree, blockaddr );
  if( !*entry ) {
    *entry = AddPlist( grid->cellmem, cellblock_t, _SIZEOF_IDX_T, 1 );
    Memzero( *entry, sizeof( cellblock_t ) );
  }
  auto block = Cast( cellblock_t*, *entry );
  return block;
}

Inl cellblock_t*
TryAccessCellBlock( grid_t* grid, vec2<u32> blockpos )
{
  auto blockaddr = PagetreeAddressFromBlockpos( blockpos );
  auto entry = TryAccessLastLevelPage( &grid->cellblock_tree, blockaddr );
  auto block = Cast( cellblock_t*, entry );
  return block;
}

Inl cell_t*
TryAccessCell( grid_t* grid, absolutepos_t abspos )
{
  auto blockpos = BlockposFromAbsolutepos( abspos );
  auto block = TryAccessCellBlock( grid, blockpos );
  if( !block ) {
    return 0;
  }
  auto posinblock = PosinblockFromAbsolutepos( blockpos, abspos );
  auto cell = block->cells + posinblock;
  return cell;
}
Inl cell_t*
AccessCell( grid_t* grid, absolutepos_t abspos )
{
  auto blockpos = BlockposFromAbsolutepos( abspos );
  auto block = AccessCellBlock( grid, blockpos );
  auto posinblock = PosinblockFromAbsolutepos( blockpos, abspos );
  auto cell = block->cells + posinblock;
  return cell;
}

// PERF: these are terrible, especially how we call them.
Inl bool
HasCustomDimX(
  grid_t* grid,
  u32 abspos_x,
  f32* dim_x
  )
{
  FORLEN( custom, i, grid->custom_dim_xs )
    if( custom->absolutepos_x == abspos_x ) {
      *dim_x = custom->dim_x;
      return 1;
    }
  }
  return 0;
}
Inl bool
HasCustomDimY(
  grid_t* grid,
  u32 abspos_y,
  f32* dim_y
  )
{
  FORLEN( custom, i, grid->custom_dim_ys )
    if( custom->absolutepos_y == abspos_y ) {
      *dim_y = custom->dim_y;
      return 1;
    }
  }
  return 0;
}

#if 0

EXAMPLES:

  = cell(1,2)  ->  foo
  = cell(cell(10, 10), cell(11, 10))  ->  bar
  = row(1, 1, 10)  ->  { foo, bar, ..., baz }
  = col(2, 1, 10)  ->  { bla, dah, ..., fad }
  = rect(1, 1, 10, 10)  ->  { { foo, ..., baz }, { a, b, ..., c }, ..., { 0, 1, ..., 9 } }
  = cell(1,2) + cell(2,1)  ->  17
  = linearCorrelationCoefficient(cell(1, 2), cell(2, 2))  ->  0.8765
  = { cell(1,2), cell(1,3), cell(1,4), foo }  ->  { a, b, c, foo }
  = graph(col(3,1,10),col(4,1,10)) -> little bar graph drawn in the cell, plotting the given data.

GRAMMAR:

  note that { x } means 0 or more of x repeated.
  note that [ x ] means 0 or 1 of x.

  unop =
    "-" | "!" | "~"

  binop =
    "^"
    "*" | "/" | "%"
    "+" | "-"

  num =
    [ "-" ]  digit  { digit }  [ "."  digit  { digit } ]  [ "e"  [ "-" ]  digit  { digit } ]
    0x  digit  { digit }
    0b  "0" | "1"  { "0" | "1" }

  ident =
    alpha  { "_" | alpha | digit }

  e0 =
    ident  "("  { e5 ";" }  [ e5  [ ";" ] ]  ")"
    "{"  { e5 ";" }  [ e5  [ ";" ] ]  "}"
    unop  e3
    num

  expr =
    e0  [ binop  e0 ]

#endif

#define TOKENTYPES( _x ) \
  _x( number ) \
  _x( ident ) \
  _x( exclamation ) \
  _x( tilde ) \
  _x( caret ) \
  _x( star ) \
  _x( plus ) \
  _x( minus ) \
  _x( slash ) \
  _x( percent ) \
  _x( bracket_curly_l ) \
  _x( bracket_curly_r ) \
  _x( bracket_square_l ) \
  _x( bracket_square_r ) \
  _x( paren_l ) \
  _x( paren_r ) \
  _x( comma ) \
  _x( eq ) \
  _x( semicolon ) \
  \
  _x( cell ) \
  _x( row ) \
  _x( col ) \
  _x( rowrect ) \
  _x( colrect ) \
  _x( relcell ) \
  _x( sum ) \
  _x( max ) \
  _x( min ) \
  _x( mean ) \
  _x( median ) \
  _x( graph ) \

Enumc( tokentype_t )
{
  #define CASE( x )   x,
  TOKENTYPES( CASE )
  #undef CASE
};

Inl slice32_t
StringOfTokenType( tokentype_t type )
{
  switch( type ) {
    #define CASE( name )   case tokentype_t::name: return Slice32FromCStr( # name );
    TOKENTYPES( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}

struct
token_t
{
  tokentype_t type;
  u32 offset_into_input;
  slice32_t slice;
};

struct
compileerror_t
{
  slice32_t text;
  u32 start; // into cell's input
  u32 end;
};

Inl void
Error(
  compileerror_t* error,
  token_t* tkn,
  slice32_t errortext
  )
{
  error->text = errortext;
  error->start = tkn->offset_into_input;
  error->end = error->start + tkn->slice.len;
}

Inl void
Error(
  compileerror_t* error,
  u32 start,
  u32 end,
  slice32_t errortext
  )
{
  error->text = errortext;
  error->start = start;
  error->end = end;
}

Inl bool
IsIdentChar( u8 c )
{
  bool r =
    ( c == '_' )  |
    IsAlpha( c )  |
    IsNumber( c );
  return r;
}



static slice32_t Sym_cell    = Slice32FromCStr( "cell" );
static slice32_t Sym_row     = Slice32FromCStr( "row" );
static slice32_t Sym_col     = Slice32FromCStr( "col" );
static slice32_t Sym_rowrect = Slice32FromCStr( "rowrect" );
static slice32_t Sym_colrect = Slice32FromCStr( "colrect" );
static slice32_t Sym_relcell = Slice32FromCStr( "relcell" );
static slice32_t Sym_sum     = Slice32FromCStr( "sum" );
static slice32_t Sym_max     = Slice32FromCStr( "max" );
static slice32_t Sym_min     = Slice32FromCStr( "min" );
static slice32_t Sym_mean    = Slice32FromCStr( "mean" );
static slice32_t Sym_median  = Slice32FromCStr( "median" );
static slice32_t Sym_graph   = Slice32FromCStr( "graph" );

static slice32_t Sym_exclamation      = Slice32FromCStr( "!" );
static slice32_t Sym_tilde            = Slice32FromCStr( "~" );
static slice32_t Sym_caret            = Slice32FromCStr( "^" );
static slice32_t Sym_star             = Slice32FromCStr( "*" );
static slice32_t Sym_plus             = Slice32FromCStr( "+" );
static slice32_t Sym_minus            = Slice32FromCStr( "-" );
static slice32_t Sym_slash            = Slice32FromCStr( "/" );
static slice32_t Sym_percent          = Slice32FromCStr( "%" );
static slice32_t Sym_bracket_curly_l  = Slice32FromCStr( "{" );
static slice32_t Sym_bracket_curly_r  = Slice32FromCStr( "}" );
static slice32_t Sym_bracket_square_l = Slice32FromCStr( "[" );
static slice32_t Sym_bracket_square_r = Slice32FromCStr( "]" );
static slice32_t Sym_paren_l          = Slice32FromCStr( "(" );
static slice32_t Sym_paren_r          = Slice32FromCStr( ")" );
static slice32_t Sym_comma            = Slice32FromCStr( "," );
static slice32_t Sym_eq               = Slice32FromCStr( "=" );
static slice32_t Sym_semicolon        = Slice32FromCStr( ";" );

Inl void
Tokenize(
  slice32_t src,
  array_t<token_t>* tokens,
  compileerror_t* error
  )
{
  u32 pos = 0;
  while( pos < src.len ) {
    auto curr = src.mem + pos;
    auto curr_len = src.len - pos;

    // skip over whitespace.
    if( ( curr[0] == ' ' )  ||  ( curr[0] == '\t' )  ) {
      pos += 1;
      continue;
    }

    // tokenize numbers.
    if( IsNumber( curr[0] ) ) {
      // TODO: allow scientific notation, ie 1.2e9 syntax.
      u32 offset = 1;
      u32 ndots = 0;
      while( pos + offset < src.len ) {
        if( IsNumber( curr[offset] ) ) {
          offset += 1;
          continue;
        }
        if( curr[offset] == '.' ) {
          if( ndots >= 1 ) {
            Error( error, pos + offset, pos + offset + 1, Slice32FromCStr( "numbers can have only one decimal point!" ) );
            return;
          }
          ndots += 1;
          offset += 1;
          continue;
        }
        break;
      }

      auto tkn = AddBack( *tokens );
      tkn->type = tokentype_t::number;
      tkn->slice.mem = curr;
      tkn->slice.len = offset;
      tkn->offset_into_input = pos;

      pos += offset;
      continue;
    }

    // toeknize idents.
    if( IsAlpha( curr[0] ) ) {
      u32 offset = 1;
      while( pos + offset < src.len ) {
        if( IsIdentChar( curr[offset] ) ) {
          offset += 1;
          continue;
        }
        break;
      }

      // tokenize keywords.
      // note these take precedence over idents
      #define TOKENIZE_KEYWORD( tokenname ) \
        if( MemEqual( curr, offset, ML( NAMEJOIN( Sym_, tokenname ) ) ) ) { \
          auto tkn = AddBack( *tokens ); \
          tkn->type = NAMEJOIN( tokentype_t::, tokenname ); \
          tkn->slice.mem = curr; \
          tkn->slice.len = NAMEJOIN( Sym_, tokenname ).len; \
          tkn->offset_into_input = pos; \
          pos += NAMEJOIN( Sym_, tokenname ).len; \
          continue; \
        }

      TOKENIZE_KEYWORD( cell    );
      TOKENIZE_KEYWORD( row     );
      TOKENIZE_KEYWORD( col     );
      TOKENIZE_KEYWORD( rowrect );
      TOKENIZE_KEYWORD( colrect );
      TOKENIZE_KEYWORD( relcell );
      TOKENIZE_KEYWORD( sum     );
      TOKENIZE_KEYWORD( max     );
      TOKENIZE_KEYWORD( min     );
      TOKENIZE_KEYWORD( mean    );
      TOKENIZE_KEYWORD( median  );
      TOKENIZE_KEYWORD( graph   );

      #undef TOKENIZE_KEYWORD

      // tokenize idents that aren't keywords.
      auto tkn = AddBack( *tokens );
      tkn->type = tokentype_t::ident;
      tkn->slice.mem = curr;
      tkn->slice.len = offset;
      tkn->offset_into_input = pos;
      pos += offset;
      continue;
    }

    #define TOKENIZE_SYMBOL( tokenname ) \
      if( MemEqual( curr, MIN( curr_len, NAMEJOIN( Sym_, tokenname ).len ), ML( NAMEJOIN( Sym_, tokenname ) ) ) ) { \
        auto tkn = AddBack( *tokens ); \
        tkn->type = NAMEJOIN( tokentype_t::, tokenname ); \
        tkn->slice.mem = curr; \
        tkn->slice.len = NAMEJOIN( Sym_, tokenname ).len; \
        tkn->offset_into_input = pos; \
        pos += NAMEJOIN( Sym_, tokenname ).len; \
        continue; \
      } \

    TOKENIZE_SYMBOL( exclamation );
    TOKENIZE_SYMBOL( tilde );
    TOKENIZE_SYMBOL( caret );
    TOKENIZE_SYMBOL( star );
    TOKENIZE_SYMBOL( plus );
    TOKENIZE_SYMBOL( minus );
    TOKENIZE_SYMBOL( slash );
    TOKENIZE_SYMBOL( percent );
    TOKENIZE_SYMBOL( bracket_curly_l );
    TOKENIZE_SYMBOL( bracket_curly_r );
    TOKENIZE_SYMBOL( bracket_square_l );
    TOKENIZE_SYMBOL( bracket_square_r );
    TOKENIZE_SYMBOL( paren_l );
    TOKENIZE_SYMBOL( paren_r );
    TOKENIZE_SYMBOL( comma );
    TOKENIZE_SYMBOL( eq );
    TOKENIZE_SYMBOL( semicolon );

    #undef TOKENIZE_SYMBOL

    // unrecognized characters!
    Error( error, pos, pos + 1, Slice32FromCStr( "unrecognized character!" ) );
    return;
  }
}

#define NODETYPES( x ) \
  x( expr ) \

Enumc( nodetype_t )
{
  #define CASE( x )   x,
  NODETYPES( CASE )
  #undef CASE
};
Inl slice_t
StringFromNodeType( nodetype_t type )
{
  switch( type ) {
    #define CASE( x )   case nodetype_t::x: return SliceFromCStr( # x );
    NODETYPES( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}

Templ Inl T*
_AddNode( plist_t* nodemem, nodetype_t nodetype )
{
  auto r = AddPlist( *nodemem, T, _SIZEOF_IDX_T, 1 );
  r->nodetype = nodetype;
  return r;
}

#define ADD_NODE( nodemem, nodetypet ) \
  _AddNode<NAMEJOIN( NAMEJOIN( node_, nodetypet ), _t )>( nodemem, NAMEJOIN( nodetype_t::, nodetypet ) )

struct
node_expr_t;

#define BINOPTYPES( x ) \
  x( none ) \
  x( add ) \
  x( sub ) \
  x( mul ) \
  x( div ) \
  x( rem ) \
  x( pow ) \

Enumc( binoptype_t )
{
  #define CASE( x )   x,
  BINOPTYPES( CASE )
  #undef CASE
};
struct
expr_binop_t
{
  node_expr_t* expr_l;
  node_expr_t* expr_r;
  binoptype_t type;
};
Inl slice_t
StringFromBinoptype( binoptype_t type )
{
  switch( type ) {
    #define CASE( x )   case binoptype_t::x: return SliceFromCStr( # x );
    BINOPTYPES( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}
Inl u32
PrecedenceValue( binoptype_t type )
{
  switch( type ) {
    case binoptype_t::add: __fallthrough;
    case binoptype_t::sub: { return 0; } break;
    case binoptype_t::mul: __fallthrough;
    case binoptype_t::div: __fallthrough;
    case binoptype_t::rem: { return 1; } break;
    case binoptype_t::pow: { return 2; } break;
    default: UnreachableCrash();
  }
  return 0;
}

using
expr_array_t = tslice32_t<node_expr_t*>;

#define FNCALLTYPES( x ) \
  x( cell    ) \
  x( row     ) \
  x( col     ) \
  x( rowrect ) \
  x( colrect ) \
  x( relcell ) \
  x( sum     ) \
  x( max     ) \
  x( min     ) \
  x( mean    ) \
  x( median  ) \
  x( graph   ) \

Enumc( fncalltype_t )
{
  #define CASE( x )   x,
  FNCALLTYPES( CASE )
  #undef CASE
};
struct
expr_fncall_t
{
  fncalltype_t type;
  expr_array_t args;
};
Inl fncalltype_t
FncalltypeFromTokentype( tokentype_t type )
{
  switch( type ) {
    #define CASE( x )   case NAMEJOIN( tokentype_t::, x ): { return NAMEJOIN( fncalltype_t::, x ); } break;
    FNCALLTYPES( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}
Inl slice_t
StringFromFncalltype( fncalltype_t type )
{
  switch( type ) {
    #define CASE( x )   case fncalltype_t::x: return SliceFromCStr( # x );
    FNCALLTYPES( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}

#define UNOPTYPES( x ) \
  x( negate ) \
  x( not_   ) \

Enumc( unoptype_t )
{
  #define CASE( x )   x,
  UNOPTYPES( CASE )
  #undef CASE
};
struct
expr_unop_t
{
  unoptype_t type;
  node_expr_t* expr;
};
Inl slice_t
StringFromUnoptype( unoptype_t type )
{
  switch( type ) {
    #define CASE( x )   case unoptype_t::x: return SliceFromCStr( # x );
    UNOPTYPES( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}

struct
expr_num_t
{
  slice32_t literal;
  f64 value_f64;
};

Enumc( exprtype_t )
{
  fncall,
  array,
  unop,
  num,
  binop,
};
struct
node_expr_t
{
  nodetype_t nodetype;
  token_t* tkn;
  exprtype_t type;
  union {
    expr_fncall_t fncall;
    expr_array_t array;
    expr_unop_t unop;
    expr_num_t num;
    expr_binop_t binop;
  };
};

Inl void
PrintNode(
  void* node,
  array_t<u8>* dst
  )
{
  auto type = *Cast( nodetype_t*, node );
  auto typestr = StringFromNodeType( type );
  switch( type ) {
    case nodetype_t::expr: {
      auto expr = Cast( node_expr_t*, node );
      switch( expr->type ) {
        case exprtype_t::fncall: {
          auto fncalltypestr = StringFromFncalltype( expr->fncall.type );
          Memmove( AddBack( *dst, fncalltypestr.len ), ML( fncalltypestr ) );
          if( expr->fncall.args.len ) {
            Memmove( AddBack( *dst, 2 ), "( ", 2 );
            auto last_but_one = expr->fncall.args.len - 1;
            For( i, 0, last_but_one ) {
              PrintNode( expr->fncall.args.mem[i], dst );
              Memmove( AddBack( *dst, 2 ), ", ", 2 );
            }
            PrintNode( expr->fncall.args.mem[last_but_one], dst );
            Memmove( AddBack( *dst, 2 ), " )", 2 );
          }
          else {
            Memmove( AddBack( *dst, 2 ), "()", 2 );
          }
        } break;
        case exprtype_t::array: {
          if( expr->array.len ) {
            Memmove( AddBack( *dst, 2 ), "array( ", 2 );
            auto last_but_one = expr->array.len - 1;
            For( i, 0, last_but_one ) {
              PrintNode( expr->array.mem[i], dst );
              Memmove( AddBack( *dst, 2 ), ", ", 2 );
            }
            PrintNode( expr->array.mem[last_but_one], dst );
            Memmove( AddBack( *dst, 2 ), " )", 2 );
          }
          else {
            Memmove( AddBack( *dst, 2 ), "{}", 2 );
          }

        } break;
        case exprtype_t::unop: {
          auto unoptypestr = StringFromUnoptype( expr->unop.type );
          Memmove( AddBack( *dst, unoptypestr.len ), ML( unoptypestr ) );
          Memmove( AddBack( *dst, 2 ), "( ", 2 );
          PrintNode( expr->unop.expr, dst );
          Memmove( AddBack( *dst, 2 ), " )", 2 );
        } break;
        case exprtype_t::num: {
          Memmove( AddBack( *dst, expr->num.literal.len ), ML( expr->num.literal ) );
        } break;
        case exprtype_t::binop: {
          auto binoptypestr = StringFromBinoptype( expr->binop.type );
          Memmove( AddBack( *dst, binoptypestr.len ), ML( binoptypestr ) );
          Memmove( AddBack( *dst, 2 ), "( ", 2 );
          PrintNode( expr->binop.expr_l, dst );
          Memmove( AddBack( *dst, 2 ), ", ", 2 );
          PrintNode( expr->binop.expr_r, dst );
          Memmove( AddBack( *dst, 2 ), " )", 2 );
        } break;
      }
    } break;
  }
}

Inl token_t*
ExpectToken(
  array_t<token_t>* tokens,
  idx_t pos,
  compileerror_t* error
  )
{
  if( !tokens->len ) {
    Error( error, 0, 0, Slice32FromCStr( "list of tokens is empty!" ) );
    return 0;
  }
  if( pos >= tokens->len ) {
    auto last = tokens->len - 1;
    Error( error, tokens->mem + last, Slice32FromCStr( "expected token after this, but hit end of text!" ) );
    return 0;
  }
  auto tkn = tokens->mem + pos;
  return tkn;
}

Inl token_t*
ExpectTokenOfType(
  array_t<token_t>* tokens,
  tokentype_t type,
  plist_t* errortextmem,
  idx_t* pos,
  compileerror_t* error
  )
{
  if( !tokens->len ) {
    Error( error, 0, 0, Slice32FromCStr( "list of tokens is empty!" ) );
    return 0;
  }
  if( *pos >= tokens->len ) {
    auto s0 = Slice32FromCStr( "Expected '" );
    auto s1 = StringOfTokenType( type );
    auto s2 = Slice32FromCStr( "', but hit EOF first." );
    slice32_t errortext;
    errortext.len = s0.len + s1.len + s2.len;
    errortext.mem = AddPlist( *errortextmem, u8, 1, errortext.len );
    auto mem = errortext.mem;
    Memmove( mem, ML( s0 ) ); mem += s0.len;
    Memmove( mem, ML( s1 ) ); mem += s1.len;
    Memmove( mem, ML( s2 ) ); mem += s2.len;
    idx_t last = tokens->len - 1;
    Error( error, tokens->mem + last, errortext );
    return 0;
  }
  auto tkn = tokens->mem + *pos;
  if( tkn->type != type ) {
    auto s0 = Slice32FromCStr( "Expected '" );
    auto s1 = StringOfTokenType( type );
    auto s2 = Slice32FromCStr( "', but found '" );
    auto s3 = StringOfTokenType( tkn->type );
    auto s4 = Slice32FromCStr( "' instead." );
    slice32_t errortext;
    errortext.len = s0.len + s1.len + s2.len + s3.len + s4.len;
    errortext.mem = AddPlist( *errortextmem, u8, 1, errortext.len );
    auto mem = errortext.mem;
    Memmove( mem, ML( s0 ) ); mem += s0.len;
    Memmove( mem, ML( s1 ) ); mem += s1.len;
    Memmove( mem, ML( s2 ) ); mem += s2.len;
    Memmove( mem, ML( s3 ) ); mem += s3.len;
    Memmove( mem, ML( s4 ) ); mem += s4.len;
    Error( error, tkn, errortext );
    return 0;
  }
  *pos += 1;
  return tkn;
}

struct
parse_input_t
{
  array_t<token_t>* tokens;
  plist_t* nodemem;
  plist_t* errortextmem;
  array_t<node_expr_t*>* tmpargmem;
};

Inl node_expr_t*
ParseExprExcludingBinop(
  parse_input_t* input,
  idx_t* pos,
  compileerror_t* error
  );

NoInl node_expr_t*
_ParseExpr(
  parse_input_t* input,
  binoptype_t previous_binop_in_chain,
  bool* reorder_parent,
  idx_t* pos,
  compileerror_t* error
  )
{
  auto tkn = ExpectToken( input->tokens, *pos, error );
  if( error->text.len ) {
    return 0;
  }

  auto expr_l = ParseExprExcludingBinop( input, pos, error );
  if( error->text.len ) {
    return 0;
  }

  bool reorder_child = 0;
  *reorder_parent = 0;
  auto binoptype = binoptype_t::none;
  node_expr_t* expr_r = 0;
  if( *pos < input->tokens->len ) {
    auto tkn_binop = input->tokens->mem + *pos;
    switch( tkn_binop->type ) {
      case tokentype_t::plus   : { binoptype = binoptype_t::add; } break;
      case tokentype_t::minus  : { binoptype = binoptype_t::sub; } break;
      case tokentype_t::star   : { binoptype = binoptype_t::mul; } break;
      case tokentype_t::slash  : { binoptype = binoptype_t::div; } break;
      case tokentype_t::percent: { binoptype = binoptype_t::rem; } break;
      case tokentype_t::caret  : { binoptype = binoptype_t::pow; } break;
    }
    if( binoptype != binoptype_t::none ) {
      *pos += 1;

      if( previous_binop_in_chain != binoptype_t::none ) {
        // we're the second or more binop in a chain.
        // see if we should reorder this binop over the previous one.
        if( PrecedenceValue( binoptype ) < PrecedenceValue( previous_binop_in_chain ) ) {
          *reorder_parent = 1;
        }
      }

      expr_r = _ParseExpr( input, binoptype, &reorder_child, pos, error );
      if( error->text.len ) {
        return 0;
      }

      auto expr = ADD_NODE( input->nodemem, expr );
      expr->tkn = tkn;
      expr->type = exprtype_t::binop;
      expr->binop.type = binoptype;
      expr->binop.expr_l = expr_l;
      expr->binop.expr_r = expr_r;

      if( reorder_child ) {
        // for clarity, consider the explicit example of 2*3+4, which we've just parsed into
      	//     mul( 2, add( 3, 4 ) )
      	// we parsed into that form, since we have to avoid left-recursion in our grammar.
      	// now we have to reorder the tree, according to operator precedence, to make the math work out.
      	// we want to reorder to:
      	//     add( mul( 2, 3 ), 4 )
      	// note that we've already constructed the add( 3, 4 ) in the expr_r parse.
      	// now we're finishing the mul parse, and we want to switch things around.
      	// i.e. 'expr' is the mul
      	//      'expr_r' is the add
      	auto mul = expr;
      	auto add = expr_r;

      	AssertCrash( mul->type == exprtype_t::binop );
      	AssertCrash( add->type == exprtype_t::binop );

        // current state:
      	// mul( 2, add( 3, 4 ) )

      	// mul.rhs <- add.lhs
      	mul->binop.expr_r = add->binop.expr_l;

        // current state:
      	// mul( 2, 3 )   add( 3, 4 )

      	// add.lhs <- mul
      	add->binop.expr_l = mul;

        // current state:
      	// add( mul( 2, 3 ), 4 )

      	// now we're done! make sure we return the new parent.
      	expr = add;
      }

      return expr;
    }
  }

  return expr_l;
}

Inl node_expr_t*
ParseExpr(
  parse_input_t* input,
  idx_t* pos,
  compileerror_t* error
  )
{
  bool reorder = 0;
  auto r = _ParseExpr( input, binoptype_t::none, &reorder, pos, error );
  return r;
}

Inl node_expr_t*
ParseExprExcludingBinop(
  parse_input_t* input,
  idx_t* pos,
  compileerror_t* error
  )
{
  auto tkn = ExpectToken( input->tokens, *pos, error );
  if( error->text.len ) {
    return 0;
  }

  node_expr_t* expr = 0;
  switch( tkn->type ) {
    case tokentype_t::number: {
      expr = ADD_NODE( input->nodemem, expr );
      expr->tkn = tkn;
      expr->type = exprtype_t::num;
      expr->num.literal = tkn->slice;
      expr->num.value_f64 = CsTo_f64( ML( tkn->slice ) ); // TODO: customize, if needed for custom number syntax.
      *pos += 1;
    } break;

    case tokentype_t::bracket_curly_l: {
      expr = ADD_NODE( input->nodemem, expr );
      expr->tkn = tkn;
      expr->type = exprtype_t::array;
      auto tmpargmem_start = input->tmpargmem->len;
      bool loop = 1;
      while( loop ) {
        tkn = ExpectToken( input->tokens, *pos, error );
        if( error->text.len ) {
          return 0;
        }
        if( tkn->type == tokentype_t::bracket_curly_r ) {
          *pos += 1;
          break;
        }
        auto expr_elem = ParseExpr( input, pos, error );
        if( error->text.len ) {
          return 0;
        }
        *AddBack( *input->tmpargmem ) = expr_elem;
        tkn = ExpectToken( input->tokens, *pos, error );
        if( error->text.len ) {
          return 0;
        }
        switch( tkn->type ) {
          case tokentype_t::semicolon:
          case tokentype_t::comma: {
            *pos += 1;
          } break;

          case tokentype_t::bracket_curly_r: {
            *pos += 1;
            loop = 0;
          } break;

          default: {
            Error( error, tkn, Slice32FromCStr( "unexpected token in literal array!" ) );
            return 0;
          } break;
        }
      }
      auto args_len = input->tmpargmem->len - tmpargmem_start;
      auto args_mem = input->tmpargmem->mem + tmpargmem_start;
      AssertCrash( args_len <= MAX_u32 );
      expr->array.len = Cast( u32, args_len );
      expr->array.mem = AddPlist( *input->nodemem, node_expr_t*, sizeof( node_expr_t* ), args_len );
      Memmove( expr->array.mem, args_mem, args_len * sizeof( node_expr_t* ) );
      input->tmpargmem->len = tmpargmem_start;
    } break;

    case tokentype_t::paren_l: {
      *pos += 1;
      expr = ParseExpr( input, pos, error );
      if( error->text.len ) {
        return 0;
      }
      ExpectTokenOfType( input->tokens, tokentype_t::paren_r, input->errortextmem, pos, error );
    } break;

    case tokentype_t::plus: {
      // ignore leading unop plus.
      *pos += 1;
    } break;

    case tokentype_t::exclamation: __fallthrough;
    case tokentype_t::tilde: __fallthrough;
    case tokentype_t::minus: {
      *pos += 1;
      auto unoptype = unoptype_t::not_;
      switch( tkn->type ) {
        case tokentype_t::exclamation: __fallthrough;
        case tokentype_t::tilde: {
          unoptype = unoptype_t::not_;
        } break;
        case tokentype_t::minus: {
          unoptype = unoptype_t::negate;
        } break;
        default: UnreachableCrash();
      }
      expr = ADD_NODE( input->nodemem, expr );
      expr->tkn = tkn;
      expr->type = exprtype_t::unop;
      expr->unop.type = unoptype;
      expr->unop.expr = ParseExpr( input, pos, error );
      if( error->text.len ) {
        return 0;
      }
    } break;

    case tokentype_t::cell: __fallthrough;
    case tokentype_t::row: __fallthrough;
    case tokentype_t::col: __fallthrough;
    case tokentype_t::rowrect: __fallthrough;
    case tokentype_t::colrect: __fallthrough;
    case tokentype_t::relcell: __fallthrough;
    case tokentype_t::sum: __fallthrough;
    case tokentype_t::max: __fallthrough;
    case tokentype_t::min: __fallthrough;
    case tokentype_t::mean: __fallthrough;
    case tokentype_t::median: __fallthrough;
    case tokentype_t::graph: {
      *pos += 1;
      expr = ADD_NODE( input->nodemem, expr );
      expr->tkn = tkn;
      expr->type = exprtype_t::fncall;
      expr->fncall.type = FncalltypeFromTokentype( tkn->type );
      ExpectTokenOfType( input->tokens, tokentype_t::paren_l, input->errortextmem, pos, error );
      if( error->text.len ) {
        return 0;
      }
      auto tmpargmem_start = input->tmpargmem->len;
      bool loop = 1;
      while( loop ) {
        tkn = ExpectToken( input->tokens, *pos, error );
        if( error->text.len ) {
          return 0;
        }
        if( tkn->type == tokentype_t::paren_r ) {
          *pos += 1;
          break;
        }
        auto expr_elem = ParseExpr( input, pos, error );
        if( error->text.len ) {
          return 0;
        }
        *AddBack( *input->tmpargmem ) = expr_elem;
        tkn = ExpectToken( input->tokens, *pos, error );
        if( error->text.len ) {
          return 0;
        }
        switch( tkn->type ) {
          case tokentype_t::semicolon:
          case tokentype_t::comma: {
            *pos += 1;
          } break;

          case tokentype_t::paren_r: {
            *pos += 1;
            loop = 0;
          } break;

          default: {
            Error( error, tkn, Slice32FromCStr( "unexpected token in function call!" ) );
            return 0;
          } break;
        }
      }
      auto args_len = input->tmpargmem->len - tmpargmem_start;
      auto args_mem = input->tmpargmem->mem + tmpargmem_start;
      AssertCrash( args_len <= MAX_u32 );
      expr->fncall.args.len = Cast( u32, args_len );
      expr->fncall.args.mem = AddPlist( *input->nodemem, node_expr_t*, sizeof( node_expr_t* ), args_len );
      Memmove( expr->fncall.args.mem, args_mem, args_len * sizeof( node_expr_t* ) );
      input->tmpargmem->len = tmpargmem_start;

      switch( expr->fncall.type ) {
        case fncalltype_t::cell   : __fallthrough;
        case fncalltype_t::relcell: {
          if( expr->fncall.args.len != 2 ) {
            Error( error, tkn, Slice32FromCStr( "'cell' requires exactly 2 parameters!" ) );
            return 0;
          }
        } break;
        case fncalltype_t::row    : __fallthrough;
        case fncalltype_t::col    : {
          if( expr->fncall.args.len != 3 ) {
            Error( error, tkn, Slice32FromCStr( "'row'/'col' require exactly 3 parameters!" ) );
            return 0;
          }
        } break;

        case fncalltype_t::rowrect : __fallthrough;
        case fncalltype_t::colrect : {
          if( expr->fncall.args.len != 4 ) {
            Error( error, tkn, Slice32FromCStr( "'rowrect'/'colrect' require exactly 4 parameters!" ) );
            return 0;
          }
        } break;

        case fncalltype_t::sum    : __fallthrough;
        case fncalltype_t::graph  : __fallthrough;
        case fncalltype_t::mean   : {
        } break;

        case fncalltype_t::max    : __fallthrough;
        case fncalltype_t::min    : __fallthrough;
        case fncalltype_t::median : {
          if( !expr->fncall.args.len ) {
            Error( error, tkn, Slice32FromCStr( "expected at least 1 parameter!" ) );
            return 0;
          }
        } break;

        default: UnreachableCrash(); break;
      }
    } break;

    default: {
      Error( error, tkn, Slice32FromCStr( "unexpected token!" ) );
      return 0;
    } break;
  }

  AssertCrash( expr ); // all successful cases should set expr; failures will be via early-rets.
  return expr;
}

Inl node_expr_t*
Parse(
  parse_input_t* input,
  compileerror_t* error
  )
{
  idx_t pos = 0;
  auto tkn = ExpectToken( input->tokens, pos, error );
  if( error->text.len ) {
    return 0;
  }

  // skip leading eq, it doesn't actually mean anything in our syntax.
  if( tkn->type == tokentype_t::eq ) {
    pos += 1;
    tkn = ExpectToken( input->tokens, pos, error );
    if( error->text.len ) {
      return 0;
    }
  }

  auto expr = ParseExpr( input, &pos, error );
  if( error->text.len ) {
    return 0;
  }

  if( pos < input->tokens->len ) {
    Error( error, input->tokens->mem + pos, Slice32FromCStr( "unexpected trailing text!" ) );
    return 0;
  }

  return expr;
}

NoInl cellvalue_t
EvaluateExpr(
  grid_t* grid,
  plist_t* evalmem,
  absolutepos_t abspos, // which cell we're evaluating into.  used for relcell and other relative stuff.
  node_expr_t* expr,
  array_t<cell_t*>* cells_subexpr,
  compileerror_t* error
  );

Inl cellvalue_t
EvaluateExprArray(
  grid_t* grid,
  plist_t* evalmem,
  absolutepos_t abspos, // which cell we're evaluating into.  used for relcell and other relative stuff.
  expr_array_t* array,
  array_t<cell_t*>* cells_subexpr,
  compileerror_t* error
  )
{
  auto len = array->len;
  cellvalue_t value;
  value.type = cellvaluetype_t::_array;
  value._array.len = len;
  value._array.mem = AddPlist( *evalmem, cellvalue_t, _SIZEOF_IDX_T, len );
  For( i, 0, len ) {
    auto expr_elem = array->mem[i];
    value._array.mem[i] = EvaluateExpr( grid, evalmem, abspos, expr_elem, cells_subexpr, error );
    if( error->text.len ) {
      return {};
    }
  }
  return value;
}

Inl u32
ExpectGridAbsposCoord(
  f64 value,
  node_expr_t* expr,
  compileerror_t* error
  )
{
  if( value <= 0.5 ) {
    Error( error, expr->tkn, Slice32FromCStr( "expected an integer 1 or greater!" ) );
    return {};
  }
  if( value >= Cast( f64, MAX_u32 ) + 0.5 ) {
    Error( error, expr->tkn, Slice32FromCStr( "number is larger than the grid size!" ) );
    return {};
  }
  auto abspos_coord = Round_u32_from_f64( value );
  // we have 1-based row/col indexing.
  // TODO: make this an option?
  AssertCrash( abspos_coord >= 1 );
  return abspos_coord - 1;
}

Inl s64
ExpectGridAbsposRelCoord(
  f64 value,
  node_expr_t* expr,
  compileerror_t* error
  )
{
  if( value <= -Cast( f64, MAX_u32 ) - 0.5 ) {
    Error( error, expr->tkn, Slice32FromCStr( "number is larger than the grid size!" ) );
    return {};
  }
  if( value >= Cast( f64, MAX_u32 ) + 0.5 ) {
    Error( error, expr->tkn, Slice32FromCStr( "number is larger than the grid size!" ) );
    return {};
  }
  auto abspos_relcoord = Round_s64_from_f64( value );
  return abspos_relcoord;
}

Inl cellvalue_t
ReadCellValueDuringExprEval(
  grid_t* grid,
  absolutepos_t abspos,
  array_t<cell_t*>* cells_subexpr
  )
{
  auto cell = AccessCell( grid, abspos );
  *AddBack( *cells_subexpr ) = cell;

#if PACKCELL
  cellvalue_t r;
  r.type = Cast( cellvaluetype_t, cell->value_type );
  switch( r.type ) {
    case cellvaluetype_t::_f64: {
      r._f64 = *Cast( f64*, &cell->value_mem );
    } break;
    case cellvaluetype_t::_string: {
      r._string.len = cell->value_len;
      r._string.mem = cell->value_mem;
    } break;
    case cellvaluetype_t::_array:
    case cellvaluetype_t::_graph: {
      r._array.len = cell->value_len;
      r._array.mem = Cast( cellvalue_t*, cell->value_mem );
    } break;
    default: UnreachableCrash();
  }
#else
  return cell->value;
#endif
}

// TODO: PERF: not ideal. can probably avoid doing this in the future.
Inl void
CopyCellValue(
  cellvalue_t* dst,
  plist_t* dstmem,
  cellvalue_t* src
  )
{
  dst->type = src->type;
  switch( src->type ) {
    case cellvaluetype_t::_f64: {
      dst->_f64 = src->_f64;
    } break;
    case cellvaluetype_t::_string: {
      dst->_string = AddPlistSlice32( *dstmem, u8, 1, src->_string.len );
      Memmove( dst->_string.mem, ML( src->_string ) );
    } break;
    case cellvaluetype_t::_array:
    case cellvaluetype_t::_graph: {
      dst->_array = AddPlistSlice32( *dstmem, cellvalue_t, _SIZEOF_IDX_T, src->_array.len );
      Memmove( dst->_array.mem, ML( src->_array ) );
      FORLEN( dst_elem, i, dst->_array )
        auto src_elem = src->_array.mem + i;
        CopyCellValue( dst_elem, dstmem, src_elem );
      }
    } break;
    default: UnreachableCrash();
  }
}

// flattens all cells read during evalutation into cells_subexpr.
NoInl cellvalue_t
EvaluateExpr(
  grid_t* grid,
  plist_t* evalmem,
  absolutepos_t abspos, // which cell we're evaluating into.  used for relcell and other relative stuff.
  node_expr_t* expr,
  array_t<cell_t*>* cells_subexpr,
  compileerror_t* error
  )
{
  switch( expr->type ) {
    case exprtype_t::fncall: {
      auto args = EvaluateExprArray( grid, evalmem, abspos, &expr->fncall.args, cells_subexpr, error );
      if( error->text.len ) {
        return {};
      }

      // right now all functions take all numbers as args.
      // TODO: this will change for newer fns, and we may even allow auto array inlining, so "cell(cell(1,1))" would work if cell(1,1) evals to a 2-elem array.
      #define VERIFY_ARGS_ARE_NUM_ARRAY \
        AssertCrash( args.type == cellvaluetype_t::_array ); \
        FORLEN( arg, i, args._array ) \
          if( arg->type != cellvaluetype_t::_f64 ) { \
            Error( error, expr->fncall.args.mem[i]->tkn, Slice32FromCStr( "expected this to be a number argument!" ) ); \
            return {}; \
          } \
        } \

      switch( expr->fncall.type ) {
        case fncalltype_t::cell   : {
          VERIFY_ARGS_ARE_NUM_ARRAY;
          AssertCrash( args._array.len == 2 );
          auto abspos_x = ExpectGridAbsposCoord( args._array.mem[0]._f64, expr->fncall.args.mem[0], error );
          if( error->text.len ) {
            return {};
          }
          auto abspos_y = ExpectGridAbsposCoord( args._array.mem[1]._f64, expr->fncall.args.mem[1], error );
          if( error->text.len ) {
            return {};
          }
          absolutepos_t abspos_arg = { abspos_x, abspos_y };
          auto value = ReadCellValueDuringExprEval( grid, abspos_arg, cells_subexpr );
          if( error->text.len ) {
            return {};
          }
          return value;
        } break;

        case fncalltype_t::relcell: {
          VERIFY_ARGS_ARE_NUM_ARRAY;
          AssertCrash( args._array.len == 2 );
          auto relpos_x = ExpectGridAbsposRelCoord( args._array.mem[0]._f64, expr->fncall.args.mem[0], error );
          if( error->text.len ) {
            return {};
          }
          auto relpos_y = ExpectGridAbsposRelCoord( args._array.mem[1]._f64, expr->fncall.args.mem[1], error );
          if( error->text.len ) {
            return {};
          }
          absolutepos_t abspos_arg = _vec2<u32>(
            Cast( u32, CLAMP( abspos.x + relpos_x, 0, MAX_u32 ) ),
            Cast( u32, CLAMP( abspos.y + relpos_y, 0, MAX_u32 ) )
            );
          auto value = ReadCellValueDuringExprEval( grid, abspos_arg, cells_subexpr );
          if( error->text.len ) {
            return {};
          }
          return value;
        } break;

        case fncalltype_t::row    : {
          VERIFY_ARGS_ARE_NUM_ARRAY;
          AssertCrash( args._array.len == 3 );
          auto abspos_y = ExpectGridAbsposCoord( args._array.mem[0]._f64, expr->fncall.args.mem[0], error );
          if( error->text.len ) {
            return {};
          }
          auto abspos_x0 = ExpectGridAbsposCoord( args._array.mem[1]._f64, expr->fncall.args.mem[1], error );
          if( error->text.len ) {
            return {};
          }
          auto abspos_x1 = ExpectGridAbsposCoord( args._array.mem[2]._f64, expr->fncall.args.mem[2], error );
          if( error->text.len ) {
            return {};
          }
          // reorder x0,x1 s.t. x0 <= x1.
          auto tmp0 = abspos_x0;
          auto tmp1 = abspos_x1;
          abspos_x0 = MIN( tmp0, tmp1 );
          abspos_x1 = MAX( tmp0, tmp1 );

          cellvalue_t value;
          value.type = cellvaluetype_t::_array;
          value._array.len = abspos_x1 - abspos_x0;
          value._array.mem = AddPlist( *evalmem, cellvalue_t, _SIZEOF_IDX_T, value._array.len );
          Fori( u32, abspos_x, abspos_x0, abspos_x1 ) {
            absolutepos_t abspos_elem = { abspos_x, abspos_y };
            value._array.mem[abspos_x - abspos_x0] = ReadCellValueDuringExprEval( grid, abspos_elem, cells_subexpr );
          }
          return value;
        } break;

        case fncalltype_t::col    : {
          VERIFY_ARGS_ARE_NUM_ARRAY;
          AssertCrash( args._array.len == 3 );
          auto abspos_x = ExpectGridAbsposCoord( args._array.mem[0]._f64, expr->fncall.args.mem[0], error );
          if( error->text.len ) {
            return {};
          }
          auto abspos_y0 = ExpectGridAbsposCoord( args._array.mem[1]._f64, expr->fncall.args.mem[1], error );
          if( error->text.len ) {
            return {};
          }
          auto abspos_y1 = ExpectGridAbsposCoord( args._array.mem[2]._f64, expr->fncall.args.mem[2], error );
          if( error->text.len ) {
            return {};
          }
          // reorder y0,y1 s.t. y0 <= y1.
          auto tmp0 = abspos_y0;
          auto tmp1 = abspos_y1;
          abspos_y0 = MIN( tmp0, tmp1 );
          abspos_y1 = MAX( tmp0, tmp1 );

          cellvalue_t value;
          value.type = cellvaluetype_t::_array;
          value._array.len = abspos_y1 - abspos_y0;
          value._array.mem = AddPlist( *evalmem, cellvalue_t, _SIZEOF_IDX_T, value._array.len );
          Fori( u32, abspos_y, abspos_y0, abspos_y1 ) {
            absolutepos_t abspos_elem = { abspos_x, abspos_y };
            value._array.mem[abspos_y - abspos_y0] = ReadCellValueDuringExprEval( grid, abspos_elem, cells_subexpr );
          }
          return value;
        } break;

        case fncalltype_t::rowrect: {
          VERIFY_ARGS_ARE_NUM_ARRAY;
          AssertCrash( args._array.len == 4 );
          auto abspos_x0 = ExpectGridAbsposCoord( args._array.mem[0]._f64, expr->fncall.args.mem[0], error );
          if( error->text.len ) {
            return {};
          }
          auto abspos_y0 = ExpectGridAbsposCoord( args._array.mem[1]._f64, expr->fncall.args.mem[1], error );
          if( error->text.len ) {
            return {};
          }
          auto abspos_x1 = ExpectGridAbsposCoord( args._array.mem[2]._f64, expr->fncall.args.mem[2], error );
          if( error->text.len ) {
            return {};
          }
          auto abspos_y1 = ExpectGridAbsposCoord( args._array.mem[3]._f64, expr->fncall.args.mem[3], error );
          if( error->text.len ) {
            return {};
          }
          // reorder x0,x1 s.t. x0 <= x1.
          auto tmp0 = abspos_x0;
          auto tmp1 = abspos_x1;
          abspos_x0 = MIN( tmp0, tmp1 );
          abspos_x1 = MAX( tmp0, tmp1 );
          // reorder y0,y1 s.t. y0 <= y1.
          tmp0 = abspos_y0;
          tmp1 = abspos_y1;
          abspos_y0 = MIN( tmp0, tmp1 );
          abspos_y1 = MAX( tmp0, tmp1 );

          auto dim_x = abspos_x1 - abspos_x0;
          auto dim_y = abspos_y1 - abspos_y0;
          auto memblock = AddPlist( *evalmem, cellvalue_t, _SIZEOF_IDX_T, ( dim_x + 1 ) * dim_y );
          cellvalue_t value;
          value.type = cellvaluetype_t::_array;
          value._array.len = dim_y;
          value._array.mem = memblock;
          memblock += dim_y;
          Fori( u32, y, 0, dim_y ) {
            auto value_row = value._array.mem + y;
            value_row->type = cellvaluetype_t::_array;
            value_row->_array.len = dim_x;
            value_row->_array.mem = memblock;
            memblock += dim_x;
            Fori( u32, x, 0, dim_x ) {
              absolutepos_t abspos_elem = { abspos_x0 + x, abspos_y0 + y };
              value_row->_array.mem[x] = ReadCellValueDuringExprEval( grid, abspos_elem, cells_subexpr );
            }
          }
          return value;
        } break;

        case fncalltype_t::colrect: {
          VERIFY_ARGS_ARE_NUM_ARRAY;
          AssertCrash( args._array.len == 4 );
          auto abspos_x0 = ExpectGridAbsposCoord( args._array.mem[0]._f64, expr->fncall.args.mem[0], error );
          if( error->text.len ) {
            return {};
          }
          auto abspos_y0 = ExpectGridAbsposCoord( args._array.mem[1]._f64, expr->fncall.args.mem[1], error );
          if( error->text.len ) {
            return {};
          }
          auto abspos_x1 = ExpectGridAbsposCoord( args._array.mem[2]._f64, expr->fncall.args.mem[2], error );
          if( error->text.len ) {
            return {};
          }
          auto abspos_y1 = ExpectGridAbsposCoord( args._array.mem[3]._f64, expr->fncall.args.mem[3], error );
          if( error->text.len ) {
            return {};
          }
          // reorder x0,x1 s.t. x0 <= x1.
          auto tmp0 = abspos_x0;
          auto tmp1 = abspos_x1;
          abspos_x0 = MIN( tmp0, tmp1 );
          abspos_x1 = MAX( tmp0, tmp1 );
          // reorder y0,y1 s.t. y0 <= y1.
          tmp0 = abspos_y0;
          tmp1 = abspos_y1;
          abspos_y0 = MIN( tmp0, tmp1 );
          abspos_y1 = MAX( tmp0, tmp1 );

          auto dim_x = abspos_x1 - abspos_x0;
          auto dim_y = abspos_y1 - abspos_y0;
          auto memblock = AddPlist( *evalmem, cellvalue_t, _SIZEOF_IDX_T, dim_x * ( dim_y + 1 ) );
          cellvalue_t value;
          value.type = cellvaluetype_t::_array;
          value._array.len = dim_x;
          value._array.mem = memblock;
          memblock += dim_x;
          Fori( u32, x, 0, dim_x ) {
            auto value_col = value._array.mem + x;
            value_col->type = cellvaluetype_t::_array;
            value_col->_array.len = dim_y;
            value_col->_array.mem = memblock;
            memblock += dim_y;
            Fori( u32, y, 0, dim_y ) {
              absolutepos_t abspos_elem = { abspos_x0 + x, abspos_y0 + y };
              value_col->_array.mem[y] = ReadCellValueDuringExprEval( grid, abspos_elem, cells_subexpr );
            }
          }
          return value;
        } break;

        case fncalltype_t::sum    : {
          VERIFY_ARGS_ARE_NUM_ARRAY;
          kahan64_t sum = {};
          FORLEN( arg, i, args._array )
            Add( sum, arg->_f64 );
          }
          cellvalue_t value;
          value.type = cellvaluetype_t::_f64;
          value._f64 = sum.sum;
          return value;
        } break;

        case fncalltype_t::max    : {
          VERIFY_ARGS_ARE_NUM_ARRAY;
          AssertCrash( args._array.len );
          auto max = args._array.mem[0]._f64;
          For( i, 1, args._array.len ) {
            auto arg = args._array.mem + i;
            max = MAX( max, arg->_f64 );
          }
          cellvalue_t value;
          value.type = cellvaluetype_t::_f64;
          value._f64 = max;
          return value;
        } break;

        case fncalltype_t::min    : {
          VERIFY_ARGS_ARE_NUM_ARRAY;
          AssertCrash( args._array.len );
          auto min = args._array.mem[0]._f64;
          For( i, 1, args._array.len ) {
            auto arg = args._array.mem + i;
            min = MIN( min, arg->_f64 );
          }
          cellvalue_t value;
          value.type = cellvaluetype_t::_f64;
          value._f64 = min;
          return value;
        } break;

        case fncalltype_t::mean   : {
          VERIFY_ARGS_ARE_NUM_ARRAY;
          kahan64_t sum = {};
          FORLEN( arg, i, args._array )
            Add( sum, arg->_f64 );
          }
          cellvalue_t value;
          value.type = cellvaluetype_t::_f64;
          value._f64 = sum.sum / Cast( f64, args._array.len );
          return value;
        } break;

        case fncalltype_t::median : {
          VERIFY_ARGS_ARE_NUM_ARRAY;
          AssertCrash( args._array.len );
          ImplementCrash();
        } break;

        case fncalltype_t::graph  : {
          AssertCrash( args.type == cellvaluetype_t::_array );
          FORLEN( arg, i, args._array )
            if( arg->type != cellvaluetype_t::_array ) {
              Error( error, expr->fncall.args.mem[i]->tkn, Slice32FromCStr( "expected this to be an array argument!" ) );
              return {};
            }
            // TODO: string columns.
            // TODO: allow a minority of strings, and just turn them to 0 ?
            // TODO: do we need to accept array-of-array-of-numbers too ?
            FORLEN( arg_elem, j, arg->_array )
              if( arg_elem->type != cellvaluetype_t::_f64 ) {
                Error( error, expr->fncall.args.mem[i]->tkn, Slice32FromCStr( "expected this to be a number argument!" ) );
                return {};
              }
            }
          }
          // graph(..) evaluates to the 2d array of value data.
          // TODO: don't assume the args are already in that format, once we allow other arg formats.
          auto value = args;
          value.type = cellvaluetype_t::_graph;
          return value;
        } break;

        default: UnreachableCrash();
      }
    } break;

    case exprtype_t::array: {
      return EvaluateExprArray( grid, evalmem, abspos, &expr->array, cells_subexpr, error );
    } break;

    case exprtype_t::unop: {
      auto value = EvaluateExpr( grid, evalmem, abspos, expr->unop.expr, cells_subexpr, error );
      if( error->text.len ) {
        return {};
      }
      if( value.type != cellvaluetype_t::_f64 ) {
        Error( error, expr->tkn, Slice32FromCStr( "unary operators expect a number!" ) );
        return {};
      }
      switch( expr->unop.type ) {
        case unoptype_t::not_: { value._f64 = value._f64 != 0; } break;
        case unoptype_t::negate: { value._f64 = -value._f64; } break;
        default: UnreachableCrash();
      }
      return value;
    } break;

    case exprtype_t::num: {
      cellvalue_t value;
      value.type = cellvaluetype_t::_f64;
      value._f64 = expr->num.value_f64;
      return value;
    } break;

    case exprtype_t::binop: {
      auto value_l = EvaluateExpr( grid, evalmem, abspos, expr->binop.expr_l, cells_subexpr, error );
      if( error->text.len ) {
        return {};
      }
      auto value_r = EvaluateExpr( grid, evalmem, abspos, expr->binop.expr_r, cells_subexpr, error );
      if( error->text.len ) {
        return {};
      }
      if( value_l.type != cellvaluetype_t::_f64 ) {
        Error( error, expr->binop.expr_l->tkn, Slice32FromCStr( "binary operator left-side should be a number!" ) );
        return {};
      }
      if( value_r.type != cellvaluetype_t::_f64 ) {
        Error( error, expr->binop.expr_r->tkn, Slice32FromCStr( "binary operator right-side should be a number!" ) );
        return {};
      }
      cellvalue_t value;
      value.type = cellvaluetype_t::_f64;
      switch( expr->binop.type ) {
        case binoptype_t::add: { value._f64 = value_l._f64 + value_r._f64; } break;
        case binoptype_t::sub: { value._f64 = value_l._f64 - value_r._f64; } break;
        case binoptype_t::mul: { value._f64 = value_l._f64 * value_r._f64; } break;
        case binoptype_t::div: { value._f64 = value_l._f64 / value_r._f64; } break;
        case binoptype_t::rem: { value._f64 = fmod( value_l._f64, value_r._f64 ); } break;
        case binoptype_t::pow: { value._f64 = Pow64( value_l._f64, value_r._f64 ); } break;
        default: UnreachableCrash();
      }
      return value;
    } break;

    default: UnreachableCrash();
  }

  UnreachableCrash();
  return {};
}

Inl bool
EqualErrors(
  cellerror_t* a,
  cellerror_t* b
  )
{
  // PERF: dedupe error strings, so we don't have to strcmp here.
  bool r =
    EqualContents( a->text, b->text )  &&
    a->start == b->start  &&
    a->end == b->end;
  return r;
}

Inl void
IdenticalErrorsInRectlistCells(
  grid_t* grid,
  absoluterectlist_t* rectlist,
  bool* identical_errors,
  cellerror_t* error
  )
{
  *identical_errors = 1;
  *error = {};
  bool first = 1;
  FORLEN( rect, i, *rectlist )
    Fori( u32, y, rect->p0.y, rect->p1.y ) {
    Fori( u32, x, rect->p0.x, rect->p1.x ) {
      absolutepos_t abspos = { x, y };
      auto blockpos = BlockposFromAbsolutepos( abspos );
      auto block = TryAccessCellBlock( grid, blockpos );
      if( !block ) {
        // TODO: do this differently.
        // we don't want cursels to cause cellblock creation.
        // so, let an imaginary cellblock exist, with imaginary zero-initialized cells.
        if( first ) {
          first = 0;
          // 'error' is already zero-initialized.
        } else {
          cellerror_t zeroerror = {};
          if( !EqualErrors( error, &zeroerror ) ) {
            *identical_errors = 0;
            *error = {};
            return;
          }
        }
        continue;
      }
      auto posinblock = PosinblockFromAbsolutepos( blockpos, abspos );
      auto cell = block->cells + posinblock;
      cellerror_t cellerr;
#if PACKCELL
      cellerr.text.mem = cell->error_text_mem;
      cellerr.text.len = cell->error_text_len;
      cellerr.start = cell->error_start;
      cellerr.end = cell->error_end;
#else
      cellerr = cell->error;
#endif
      if( first ) {
        first = 0;
        *error = cellerr;
      } else {
        if( !EqualErrors( error, &cellerr ) ) {
          *identical_errors = 0;
          *error = {};
          return;
        }
      }
    }}
  }
}

Inl void
IdenticalInputsInRectlistCells(
  grid_t* grid,
  absoluterectlist_t* rectlist,
  bool* identical_inputs,
  slice32_t* input
  )
{
  *identical_inputs = 1;
  *input = {};
  bool first = 1;
  FORLEN( rect, i, *rectlist )
    Fori( u32, y, rect->p0.y, rect->p1.y ) {
    Fori( u32, x, rect->p0.x, rect->p1.x ) {
      absolutepos_t abspos = { x, y };
      auto blockpos = BlockposFromAbsolutepos( abspos );
      auto block = TryAccessCellBlock( grid, blockpos );
      if( !block ) {
        // TODO: do this differently.
        if( first ) {
          first = 0;
          // 'input' is already zero-initialized.
        } else {
          if( input->len ) {
            *identical_inputs = 0;
            *input = {};
            return;
          }
        }
        continue;
      }
      auto posinblock = PosinblockFromAbsolutepos( blockpos, abspos );
      auto cell = block->cells + posinblock;
      slice32_t cellinput;
#if PACKCELL
      cellinput.mem = cell->input_mem;
      cellinput.len = cell->input_len;
#else
      cellinput = cell->input;
#endif
      if( first ) {
        first = 0;
        *input = cellinput;
      } else {
        if( !EqualContents( *input, cellinput ) ) {
          *identical_inputs = 0;
          *input = {};
          return;
        }
      }
    }}
  }
}

Inl void
RefreshCurselIdenticals(
  grid_t* grid
  )
{
  grid->has_identical_input = 0;
  grid->identical_input = {};
  IdenticalInputsInRectlistCells( grid, &grid->cursel, &grid->has_identical_input, &grid->identical_input );

  CmdSelectAll( grid->txt_input );
  CmdRemChL( grid->txt_input );
  CmdAddString( grid->txt_input, Cast( idx_t, grid->identical_input.mem ), grid->identical_input.len );
  CmdSelectAll( grid->txt_input );
  CmdWipeHistory( grid->txt_input );

  grid->has_identical_error = 0;
  grid->identical_error = {};
  IdenticalErrorsInRectlistCells( grid, &grid->cursel, &grid->has_identical_error, &grid->identical_error );
}

// track 'changed' since we do this during mousemove, and that happens a _lot_ when dragging sel.
// we don't want to render on every pixel mousemove, only when the sel actually changes.
Inl void
SetCurselToRect( grid_t* grid, absoluterect_t rect, bool* changed )
{
  if( grid->cursel.len == 1  &&  RectsEqual( rect, grid->cursel.mem[0] ) ) {
    *changed = 0;
    return;
  }
  Reserve( grid->cursel, 1 );
  grid->cursel.len = 0;
  *AddBack( grid->cursel ) = rect;
  RefreshCurselIdenticals( grid );
  *changed = 1;
}

Inl void
SetCurselToAbspos( grid_t* grid, absolutepos_t abspos )
{
  Reserve( grid->cursel, 1 );
  grid->cursel.len = 0;
  auto rect = AddBack( grid->cursel );
  rect->p0 = abspos;
  rect->p1 = abspos + _vec2<u32>( 1, 1 );
  RefreshCurselIdenticals( grid );
}

Inl void
AddToCursel( grid_t* grid, absoluterect_t rect )
{
  bool changed_rectlist = 0;
  AddUnion( &grid->cursel, rect, &changed_rectlist );
  if( changed_rectlist ) {
    RefreshCurselIdenticals( grid );
  }
}

Inl void
AddToCursel( grid_t* grid, absolutepos_t abspos )
{
  absoluterect_t rect;
  rect.p0 = abspos;
  rect.p1 = abspos + _vec2<u32>( 1, 1 );
  AddToCursel( grid, rect );
}

//
// assumes input is already allocated on grid->cellmem
//
// TODO: should we try to remove the leading eq requirement?
// seems like it'd be nice, since you're usually putting data in cells
// the one concern is that we'd fail to compile regular string data, and we wouldn't know
// whether to show the compile error or not. when you're putting in string data, you don't
// want to see compile errors constantly. actually, presence of parens is probably all we
// need to decide. we could do more advanced filters if we want.
// let's proceed with no leading eq required, and see how that is.
//
Inl void
InsertOrSetCellContents(
  grid_t* grid,
  absoluterectlist_t* rectlist,
  slice32_t input
  )
{
  FORLEN( rect, i, *rectlist )
    Fori( u32, y, rect->p0.y, rect->p1.y ) {
    Fori( u32, x, rect->p0.x, rect->p1.x ) {
      absolutepos_t abspos = { x, y };
      auto cell = AccessCell( grid, abspos );
#if PACKCELL
      cell->input_mem = input.mem;
      cell->input_len = input.len;
#else
      cell->input = input;
#endif

      // PERF: could tokenize/parse the initial input outside the 3 loops above.
      // it's unlikely we could do eval outside the loops.

      // bump generation, so we can detect cycles anew.
      grid->eval_generation += 1;

      array_t<token_t> tokens;
      Alloc( tokens, 1024 );

      plist_t nodemem;
      Init( nodemem, 16000 );
      array_t<node_expr_t*> tmpargmem;
      Alloc( tmpargmem, 512 );
      array_t<cell_t*> cells_subexpr;
      Alloc( cells_subexpr, 512 );
      array_t<cellandpos_t> cells_superexpr;
      Alloc( cells_superexpr, 512 );

      parse_input_t parse_input;
      parse_input.tokens = &tokens;
      parse_input.nodemem = &nodemem;
      parse_input.errortextmem = &grid->cellmem;
      parse_input.tmpargmem = &tmpargmem;

      auto cellandpos = AddBack( cells_superexpr );
      cellandpos->cell = cell;
      cellandpos->abspos = abspos;

      bool cycle_detected = 0;

      ForLen( j, cells_superexpr ) {
        cellandpos = cells_superexpr.mem + j;
        cell = cellandpos->cell;

        compileerror_t error = {};

        if( cycle_detected ) {
          Error( &error, 0, 0, Slice32FromCStr( "part of a reference cycle!" ) );

          // set cell value
          cell->eval_generation = grid->eval_generation;
#if PACKCELL
          cell->error_text_mem = error.text.mem;
          AssertCrash( error.text.len <= MAX_u16 );
          cell->error_text_len = Cast( u16, error.text.len );
          AssertCrash( error.start <= MAX_u16 );
          cell->error_start = Cast( u16, error.start );
          AssertCrash( error.end <= MAX_u16 );
          cell->error_end = Cast( u16, error.end );
          cell->value_type = Cast( u16, cellvaluetype_t::_string );
          auto value = Slice32FromCStr( "ERROR_CYCLE" );
          cell->value_mem = value.mem;
          cell->value_len = value.len;
#else
          cell->error.text = error.text;
          cell->error.start = error.start;
          cell->error.end = error.end;
          cell->value.type = cellvaluetype_t::_string;
          cell->value._string = Slice32FromCStr( "ERROR_CYCLE" );
#endif

          // since this cell changed, loop over all superexprs which depend on this cell.
          FORLEN( edge, m, grid->graph_edges )
            if( edge->end == cell ) {
              auto cell_superexpr = edge->start.cell;
              auto abspos_superexpr = edge->start.abspos;
              AssertCrash( cell_superexpr->eval_generation <= grid->eval_generation );
              // walk the cycle in the superexpr direction, but avoid visiting cells we already covered.
              // we don't want to infinite-walk the cycle.
              if( cell_superexpr->eval_generation < grid->eval_generation ) {
                auto cellandpos2 = AddBack( cells_superexpr );
                cellandpos2->cell = cell_superexpr;
                cellandpos2->abspos = abspos_superexpr;
              }
            }
          }
          continue;
        }

        if( cell->eval_generation == grid->eval_generation ) {
          Error( &error, 0, 0, Slice32FromCStr( "head of a reference cycle!" ) );
          cycle_detected = 1;

          // bump generation again, since we may have evaluated some cells in the cycle before detecting it!
          // we'll need to cycle through again, so start anew.
          grid->eval_generation += 1;

          // set cell value
          cell->eval_generation = grid->eval_generation;
#if PACKCELL
          cell->error_text_mem = error.text.mem;
          AssertCrash( error.text.len <= MAX_u16 );
          cell->error_text_len = Cast( u16, error.text.len );
          AssertCrash( error.start <= MAX_u16 );
          cell->error_start = Cast( u16, error.start );
          AssertCrash( error.end <= MAX_u16 );
          cell->error_end = Cast( u16, error.end );
          cell->value_type = Cast( u16, cellvaluetype_t::_string );
          auto value = Slice32FromCStr( "ERROR_CYCLEHEAD" );
          cell->value_mem = value.mem;
          cell->value_len = value.len;
#else
          cell->error.text = error.text;
          cell->error.start = error.start;
          cell->error.end = error.end;
          cell->value.type = cellvaluetype_t::_string;
          cell->value._string = Slice32FromCStr( "ERROR_CYCLEHEAD" );
#endif

          // since this cell changed, loop over all superexprs which depend on this cell.
          FORLEN( edge, m, grid->graph_edges )
            if( edge->end == cell ) {
              *AddBack( cells_superexpr ) = edge->start;
            }
          }
          continue;
        }

        cells_subexpr.len = 0;
        tokens.len = 0;

        if( !error.text.len ) {
          slice32_t cellinput;
#if PACKCELL
          cellinput.mem = cell->input_mem;
          cellinput.len = cell->input_len;
#else
          cellinput = cell->input;
#endif
          Tokenize( cellinput, &tokens, &error );
        }

        node_expr_t* expr = 0;
        if( !error.text.len ) {
          expr = Parse( &parse_input, &error );
        }
        cellvalue_t value = {};
        if( !error.text.len ) {
          AssertCrash( expr );
          value = EvaluateExpr( grid, &nodemem, cellandpos->abspos, expr, &cells_subexpr, &error );
        }

        // note we have a graph edge (a,b) iff b is a cell referenced by a's expression.
        // e.g. A=cell(B)

        // keep error cells' dependency info, since it's possible they could eval back to non-error
        // if the user changes other cells.
        if( !error.text.len ) {
#if 1
          // rewrite as many existing (cell,*) edges as we have, and remove excess ones if needed.
          // note we effectively delete the previous subexprs, since they could have changed.
          idx_t m = 0;
          idx_t k = 0;
          while( k < grid->graph_edges.len ) {
            auto edge = grid->graph_edges.mem + k;
            if( edge->start.cell == cell ) {
              if( m < cells_subexpr.len ) {
                auto cell_subexpr = cells_subexpr.mem[m];
                edge->end = cell_subexpr;
                m += 1;
              }
              else {
                UnorderedRemAt( grid->graph_edges, k );
                continue;
              }
            }
            k += 1;
          }
          while( m < cells_subexpr.len ) {
            auto cell_subexpr = cells_subexpr.mem[m];
            auto graph_edge = AddBack( grid->graph_edges );
            graph_edge->start.cell = cell;
            graph_edge->start.abspos = cellandpos->abspos;
            graph_edge->end = cell_subexpr;
            m += 1;
          }
#else
          // delete previous subexprs, since they could have changed.
          idx_t k = 0;
          while( k < grid->graph_edges.len ) {
            auto edge = grid->graph_edges.mem + k;
            if( edge->start == cell ) {
              UnorderedRemAt( grid->graph_edges, k );
              continue;
            }
            k += 1;
          }

          // add subexprs so we have that connectivity info.
          // TODO/PERF: could rewrite existing (cell,*) edges instead of removing, then adding.
          FORLEN( cell_subexpr, m, cells_subexpr )
            auto graph_edge = AddBack( grid->graph_edges );
            graph_edge->start = cell;
            graph_edge->end = cell_subexpr;
          }
#endif
        }

        // set cell value
        cell->eval_generation = grid->eval_generation;
#if PACKCELL
        cell->error_text_mem = error.text.mem;
        AssertCrash( error.text.len <= MAX_u16 );
        cell->error_text_len = Cast( u16, error.text.len );
        AssertCrash( error.start <= MAX_u16 );
        cell->error_start = Cast( u16, error.start );
        AssertCrash( error.end <= MAX_u16 );
        cell->error_end = Cast( u16, error.end );
        if( error.text.len ) {
          cell->value_type = Cast( u16, cellvaluetype_t::_string );
          cell->value_mem = cell->input_mem;
          cell->value_len = cell->input_len;
        }
        else {
          switch( value.type ) {
            case cellvaluetype_t::_f64: {
              *Cast( f64*, &cell->value_mem ) = value._f64;
            } break;
            case cellvaluetype_t::_string: {
              cell->value_len = value._string.len;
              cell->value_mem = value._string.mem;
            } break;
            case cellvaluetype_t::_array:
            case cellvaluetype_t::_graph: {
              // TODO: EvaluateExpr will allocate in the nodemem, which is freed at the end of this fn.
              //   we'll need to copy the array contents into grid->cellmem, or
              //   change EvaluateExpr to allocate there directly.
              //   knowing we can allocate there directly seems a bit challenging, esp. since we'll add/change
              //   a bunch of grid functions.
              //   so probably best to just copy here for now, and try to eliminate it later, once the
              //   grid function set is more stabilized.
              //   note it's kind of annoying to copy, since we have to recurse for nested arrays.
              cellvalue_t copied_value;
              CopyCellValue( &copied_value, &grid->cellmem, &value );
              cell->value_len = copied_value._array.len;
              cell->value_mem = Cast( u8*, copied_value._array.mem );
            } break;
            default: UnreachableCrash();
          }
        }
#else
        cell->error.text = error.text;
        cell->error.start = error.start;
        cell->error.end = error.end;
        if( error.text.len ) {
          cell->value.type = cellvaluetype_t::_string;
          cell->value._string = cell->input;
        }
        else {
          switch( value.type ) {
            case cellvaluetype_t::_f64:
            case cellvaluetype_t::_string: {
              cell->value = value;
            } break;

            case cellvaluetype_t::_array:
            case cellvaluetype_t::_graph: {
              // TODO: EvaluateExpr will allocate in the nodemem, which is freed at the end of this fn.
              //   we'll need to copy the array contents into grid->cellmem, or
              //   change EvaluateExpr to allocate there directly.
              //   knowing we can allocate there directly seems a bit challenging, esp. since we'll add/change
              //   a bunch of grid functions.
              //   so probably best to just copy here for now, and try to eliminate it later, once the
              //   grid function set is more stabilized.
              //   note it's kind of annoying to copy, since we have to recurse for nested arrays.
              CopyCellValue( &cell->value, &grid->cellmem, &value );
            } break;
            default: UnreachableCrash();
          }
        }
#endif

        // since this cell changed, loop over all superexprs which depend on this cell.
        FORLEN( edge, m, grid->graph_edges )
          if( edge->end == cell ) {
            *AddBack( cells_superexpr ) = edge->start;
          }
        }
      }

      Free( cells_superexpr );
      Free( cells_subexpr );
      Free( tmpargmem );
      Kill( nodemem );
      Free( tokens );
    }}
  }
}

Inl void
DeleteCellContents(
  grid_t* grid,
  absoluterectlist_t* rectlist
  )
{
  FORLEN( rect, i, *rectlist )
    Fori( u32, y, rect->p0.y, rect->p1.y ) {
    Fori( u32, x, rect->p0.x, rect->p1.x ) {
      absolutepos_t abspos = { x, y };
      auto cell = TryAccessCell( grid, abspos );
      if( !cell ) {
        // nothing to do.
        continue;
      }
      // zero cell to delete.
      *cell = {};
    }}
  }

  // TODO: some cleanup of all-zero cellblocks?
}

Inl void
DrawGraph(
  array_t<f32>& stream,
  rectf32_t bounds,
  vec2<f32> zrange,
  cellvaluearray_t* array
  )
{
  f64 value_min = 0.0;
  f64 value_max = 0.0;
  idx_t colsize_max = 0;
  FORLEN( col, i, *array )
    AssertCrash( col->type == cellvaluetype_t::_array );
    colsize_max = MAX( colsize_max, col->_array.len );
    FORLEN( value, j, col->_array )
      AssertCrash( value->type == cellvaluetype_t::_f64 );
      value_min = MIN( value_min, value->_f64 );
      value_max = MAX( value_max, value->_f64 );
    }
  }
  if( !colsize_max ) {
    // TODO: draw the string "empty data" or something.
    return;
  }
  auto value_span = value_max - value_min;
  auto padding = 0.05 * value_span;
  value_max += padding;
  value_min -= padding;
  auto rgba_col = _vec4( 1.0f, 0.2f, 0.3f, 1.0f );
  FORLEN( col, i, *array )
    AssertCrash( col->type == cellvaluetype_t::_array );
    FORLEN( value, j, col->_array )
      AssertCrash( value->type == cellvaluetype_t::_f64 );
      // note reversed since bars go up, but we render with positive direction going down.
      auto y0 = Cast( f32, Lerp_from_f64( bounds.p1.y, bounds.p0.y, value->_f64, value_min, value_max ) );
      auto y1 = bounds.p1.y;
      auto tmp0 = Lerp_from_f32( bounds.p0.x, bounds.p1.x, Cast( f32, j + 0 ), 0.0f, Cast( f32, colsize_max ) );
      auto tmp1 = Lerp_from_f32( bounds.p0.x, bounds.p1.x, Cast( f32, j + 1 ), 0.0f, Cast( f32, colsize_max ) );
      auto x0 = Lerp_from_f32( tmp0, tmp1, Cast( f32, i + 0 ), 0.0f, Cast( f32, array->len ) );
      auto x1 = Lerp_from_f32( tmp0, tmp1, Cast( f32, i + 1 ), 0.0f, Cast( f32, array->len ) );
      RenderQuad(
        stream,
        rgba_col,
        _vec2( x0, y0 ),
        _vec2( x1, y1 ),
        bounds,
        GetZ( zrange, applayer_t::txt )
        );
    }
    // pick a new, different color to use.
    rgba_col.x = MIN( Mod32( rgba_col.x + 0.30f, 1.01f ), 1.0f );
    rgba_col.y = MIN( Mod32( rgba_col.x + 0.23f, 1.01f ), 1.0f );
    rgba_col.z = MIN( Mod32( rgba_col.x + 0.19f, 1.01f ), 1.0f );
  }
}

Inl void
RenderGrid(
  grid_t* grid,
  array_t<f32>& stream,
  font_t& font,
  rectf32_t bounds,
  vec2<f32> zrange,
  vec4<f32> rgba_text,
  vec4<f32> rgba_grid_bkgd,
  vec4<f32> rgba_gridlines,
  vec4<f32> rgba_selectedcells_outline,
  vec4<f32> rgba_selectedcells_bkgd,
  vec4<f32> rgba_selectedcells_head_bkgd
  )
{
  AssertCrash( !grid->custom_dim_xs.len ); // TODO: render custom row/col sizes.
  AssertCrash( !grid->custom_dim_ys.len );

  auto line_h = FontLineH( font );
  auto spaces_per_tab = GetPropFromDb( u8, u8_spaces_per_tab );
  auto rgba_selection_bkgd = GetPropFromDb( vec4<f32>, rgba_selection_bkgd );

  auto cell_contents_offset_y = 0.1f * line_h; // cell height is 1.2 * line_h, so offset to center.
  auto cell_contents_offset_x = 0.2f * line_h;
  auto cell_contents_offset = _vec2( cell_contents_offset_x, cell_contents_offset_y );

  constantold auto label_cellinput = SliceFromCStr( "cell input: " );
  auto label_cellinput_w = LayoutString( font, spaces_per_tab, ML( label_cellinput ) );

  constantold auto label_cellerror = SliceFromCStr( "cell error: " );
  auto label_cellerror_w = LayoutString( font, spaces_per_tab, ML( label_cellerror ) );

  // these 2 are equal width, so no need for fancy layout just yet.

  #define DRAW_TEXT( _slice, _origin ) \
    DrawString( \
      stream, \
      font, \
      _origin, \
      GetZ( zrange, applayer_t::txt ), \
      bounds.p0, \
      bounds.p1, \
      rgba_text, \
      spaces_per_tab, \
      ML( _slice ) \
      ) \

  #define DRAW_TEXT2( _slice, _text_pos, _clip_p0, _clip_p1 ) \
    DrawString( \
      stream, \
      font, \
      _text_pos, \
      GetZ( zrange, applayer_t::txt ), \
      _clip_p0, \
      _clip_p1, \
      rgba_text, \
      spaces_per_tab, \
      ML( _slice ) \
      ) \

  // draw txt_input
  {
    if( !grid->cursel.len ) {
      constantold auto text = SliceFromCStr( "make a selection to see cell contents." );
      DRAW_TEXT( text, bounds.p0 );
    }
    else {
      if( !grid->has_identical_input ) {
        constantold auto text = SliceFromCStr( "shrink your selection to see individual cell contents." );
        DRAW_TEXT( text, bounds.p0 );
      }
      else {
        DRAW_TEXT( label_cellinput, bounds.p0 );

        TxtLayoutSingleLineSubset(
          grid->txt_input,
          GetBOF( grid->txt_input.buf ),
          TxtLen( grid->txt_input ),
          font
          );
        TxtRenderSingleLineSubset(
          grid->txt_input,
          stream,
          font,
          _rect(
            bounds.p0 + _vec2( label_cellinput_w, 0.0f ),
            _vec2( bounds.p1.x, MIN( bounds.p1.y, bounds.p0.y + line_h ) )
            ),
          ZRange( zrange, applayer_t::txt ),
          1,
          ( grid->focus == gridfocus_t::input ),
          ( grid->focus == gridfocus_t::input )
          );
      }
    }

    bounds.p0.y += line_h;
  }

  // draw cellerror
  {
    if( !grid->cursel.len ) {
      // code above will draw a message to make a selection.
    }
    else {
      if( !grid->has_identical_error ) {
        constantold auto text = SliceFromCStr( "shrink your selection to see individual errors." );
        DRAW_TEXT( text, bounds.p0 );
      }
      else {
        if( !HasError( &grid->identical_error ) ) {
          // all cells selected have no errors.
        }
        else {
          // all cells selected have identical, _non-empty_ errors.
          DRAW_TEXT( label_cellerror, bounds.p0 );
          DRAW_TEXT( grid->identical_error.text, bounds.p0 + _vec2( label_cellerror_w, 0.0f ) );
        }
      }
    }

    bounds.p0.y += line_h;
  }

  auto default_cell_dimf = _vec2<f32>( 4.0f, 1.2f ) * line_h;
  auto colhdr_h = default_cell_dimf.y;

  // calculate the conservative ncells_x/y values.
  // we have to account for custom dims that are smaller than the default, hence this looping.
  // this has to be >= the actual ncells_x/y that will be finally visible, since we're doing a
  // searching algorithm to find the actual ncells_x/y, and we need a strong upper bound.
  u32 ncells_conservative_y;
  {
    u32 abspos_y = grid->scroll_absolutepos.y;
    kahan32_t y1 = { colhdr_h }; // start one line down, to account for the col headers.
    bool loop_y = 1;
    while( loop_y ) {
      f32 custom_dim_y;
      auto found = HasCustomDimY( grid, abspos_y, &custom_dim_y );
      Add( y1, found  ?  custom_dim_y  :  default_cell_dimf.y );
      if( y1.sum > ( bounds.p1.y - bounds.p0.y ) ) {
        loop_y = 0;
        y1.sum = ( bounds.p1.y - bounds.p0.y );
        // continue for one last iter, so we draw the partially-visible last one.
      }
      abspos_y += 1;
    }
    ncells_conservative_y = abspos_y - grid->scroll_absolutepos.y;
  }
  u32 ncells_conservative_x;
  {
    u32 abspos_x = grid->scroll_absolutepos.x;
    kahan32_t x1 = { 0.0f };
    bool loop_x = 1;
    while( loop_x ) {
      f32 custom_dim_x;
      auto found = HasCustomDimX( grid, abspos_x, &custom_dim_x );
      Add( x1, found  ?  custom_dim_x  :  default_cell_dimf.x );
      if( x1.sum > ( bounds.p1.x - bounds.p0.x ) ) {
        loop_x = 0;
        x1.sum = ( bounds.p1.x - bounds.p0.x );
        // continue for one last iter, so we draw the partially-visible last one.
      }
      abspos_x += 1;
    }
    ncells_conservative_x = abspos_x - grid->scroll_absolutepos.x;
  }

  // this stores the sizing info of an individual cell, ignoring same-row/col constraints.
  // note it takes manual sizing into consideration.
  array32_t<vec2<f32>> cell_autodims;
  auto ncells_conservative = ncells_conservative_x * ncells_conservative_y;
  Alloc( cell_autodims, ncells_conservative ); // PERF: make this a Reserve
  cell_autodims.len = ncells_conservative;
  // size each cell, and store the max for every row+col.
  Fori( u32, j, 0, ncells_conservative_y ) {
  Fori( u32, i, 0, ncells_conservative_x ) {
    absolutepos_t abspos = grid->scroll_absolutepos + _vec2( i, j );
    auto cell_dimf = default_cell_dimf;
    if( grid->focus == gridfocus_t::input  &&  grid->c_head == abspos ) {
      auto input = AllocContents( grid->txt_input.buf );
      auto w = LayoutString( font, spaces_per_tab, ML( input ) );
      cell_dimf.x = 2.0f * cell_contents_offset.x + w;
      Free( input );
    }
    else {
      auto cell = TryAccessCell( grid, abspos );
      if( cell  &&  cell->input.len ) {
        switch( cell->value.type ) {
          case cellvaluetype_t::_f64: {
            // PERF: we do CsFrom_f64 again below to actually output the text; cache instead.
            embeddedarray_t<u8, 64> tmp;
            CsFrom_f64( tmp.mem, Capacity( tmp ), &tmp.len, cell->value._f64, 2 );
            auto w = LayoutString( font, spaces_per_tab, ML( tmp ) );
            cell_dimf.x = 2.0f * cell_contents_offset.x + w;
          } break;
          case cellvaluetype_t::_string: {
            auto w = LayoutString( font, spaces_per_tab, ML( cell->value._string ) );
            cell_dimf.x = 2.0f * cell_contents_offset.x + w;
          } break;
          case cellvaluetype_t::_array: {
            ImplementCrash();
          } break;
          case cellvaluetype_t::_graph: {
            cell_dimf.x = 0.25f * MinElem( bounds.p1 - bounds.p0 );
            cell_dimf.y = cell_dimf.x;
          } break;
          default: UnreachableCrash();
        }
      }
    }
    // limit auto-sized cell_dimf, since we don't have partial-cell scrolling yet.
    cell_dimf = Min( cell_dimf, 0.5f * ( bounds.p1 - bounds.p0 ) );

    auto cell_autodim = cell_autodims.mem + j * ncells_conservative_x + i;
    *cell_autodim = cell_dimf;
  }
  }
  // override auto-sizing with the custom dim.
  Fori( u32, j, 0, ncells_conservative_y ) {
    f32 custom_dim_y;
    if( HasCustomDimY( grid, grid->scroll_absolutepos.y + j, &custom_dim_y ) ) {
      Fori( u32, i, 0, ncells_conservative_x ) {
        auto cell_autodim = cell_autodims.mem + j * ncells_conservative_x + i;
        cell_autodim->y = custom_dim_y;
      }
    }
  }
  Fori( u32, i, 0, ncells_conservative_x ) {
    f32 custom_dim_x;
    if( HasCustomDimX( grid, grid->scroll_absolutepos.x + i, &custom_dim_x ) ) {
      Fori( u32, j, 0, ncells_conservative_y ) {
        auto cell_autodim = cell_autodims.mem + j * ncells_conservative_x + i;
        cell_autodim->x = custom_dim_x;
      }
    }
  }

  // note we reuse these dim_xs/dim_ys for every candidate choice.
  // but the len stays the same, so we use smaller iteration loops, not FORLEN32 loops.
  array32_t<f32> dim_xs;
  array32_t<f32> dim_ys;
  Alloc( dim_xs, ncells_conservative_x ); // PERF: make this a Reserve
  Alloc( dim_ys, ncells_conservative_y ); // PERF: make this a Reserve
  dim_xs.len = ncells_conservative_x;
  dim_ys.len = ncells_conservative_y;
  u32 best_choice_x = 0;
  u32 best_choice_y = 0;
  u32 best_c = 0;
  f32 best_max_hdr_w = 0.0f;
  f32 best_rowhdr_w = 0.0f;
  Fori( u32, ncells_choice_y, 0, ncells_conservative_y + 1 ) {
  Fori( u32, ncells_choice_x, 0, ncells_conservative_x + 1 ) {
    // see if this is a good choice.

    // take the max across rows/cols to figure out row/col sizes.
    Fori( u32, i, 0, ncells_choice_x ) {
      dim_xs.mem[i] = default_cell_dimf.x;
    }
    Fori( u32, j, 0, ncells_choice_y ) {
      dim_ys.mem[j] = default_cell_dimf.y;
    }
    Fori( u32, j, 0, ncells_choice_y ) {
    Fori( u32, i, 0, ncells_choice_x ) {
      auto cell_autodim = cell_autodims.mem + j * ncells_conservative_x + i;
      auto dim_x = dim_xs.mem + i;
      auto dim_y = dim_ys.mem + j;
      *dim_x = MAX( *dim_x, cell_autodim->x );
      *dim_y = MAX( *dim_y, cell_autodim->y );
    }
    }

    // check that we're not overdrawing or underdrawing with this choice of y.
    bool valid_y = 0;
    kahan32_t y1 = { colhdr_h }; // start one line down, to account for the col headers.
    auto last_y = ( bounds.p1.y - bounds.p0.y );
    Fori( u32, j, 0, ncells_choice_y ) { // note dim_ys has a different size than this.
      auto y0 = y1.sum;
      Add( y1, dim_ys.mem[j] );
      if( y0 <= last_y  &&  last_y < y1.sum ) {
        if( j + 1 == ncells_choice_y ) {
          // continue for one last iter, so we draw the partially-visible last one.
          valid_y = 1;
          break;
        }
        else {
          // under or overdraw, so this is a bad choice.
          break;
        }
      }
    }
    if( !valid_y ) {
      continue;
    }

    // using 1-based hdrs, which ncells_choice_y accounts for already.
    auto last_hdr_idx = grid->scroll_absolutepos.y + ncells_choice_y;
    embeddedarray_t<u8, 64> hdr;
    auto success = CsFromIntegerU( hdr.mem, Capacity( hdr ), &hdr.len, last_hdr_idx, 1 );
    AssertCrash( success );
    auto max_hdr_w = LayoutString( font, spaces_per_tab, ML( hdr ) );
    auto rowhdr_w = 2.0f * cell_contents_offset.x + max_hdr_w;

    // check that we're not overdrawing or underdrawing with this choice of x.
    bool valid_x = 0;
    kahan32_t x1 = { rowhdr_w };
    auto last_x = ( bounds.p1.x - bounds.p0.x );
    Fori( u32, i, 0, ncells_choice_x ) { // note dim_xs has a different size than this.
      auto x0 = x1.sum;
      Add( x1, dim_xs.mem[i] );
      if( x0 <= last_x  &&  last_x < x1.sum ) {
        if( i + 1 == ncells_choice_x ) {
          // continue for one last iter, so we draw the partially-visible last one.
          valid_x = 1;
          break;
        }
        else {
          // under or overdraw, so this is a bad choice.
          break;
        }
      }
    }
    if( !valid_x ) {
      continue;
    }

    // keep track of this choice, if it's better than our previous candidates.
    // note if the total ncells 'c' metric is the same across choices, we prefer the one with maximum y span.
    // this is so we can make a choice between e.g. 3,4 vs. 4,3; we rule in favor of more rows visible.
    auto c = ncells_choice_x * ncells_choice_y;
    if( c > best_c  ||  ( c == best_c  &&  ncells_choice_y > best_choice_y ) ) {
      best_c = c;
      best_choice_x = ncells_choice_x;
      best_choice_y = ncells_choice_y;
      best_max_hdr_w = max_hdr_w;
      best_rowhdr_w = rowhdr_w;
    }
  }
  }

  // now we have a choice!
  auto ncells_y = best_choice_y;
  auto ncells_x = best_choice_x;
  auto max_hdr_w = best_max_hdr_w;
  auto rowhdr_w = best_rowhdr_w;
  auto grid_offset = _vec2( rowhdr_w, colhdr_h );
//  printf( "%u, %u\n", ncells_x, ncells_y );

  // set up dim_xs/dim_ys to reflect that choice.
  {
    // take the max across rows/cols to figure out row/col sizes.
    dim_xs.len = ncells_x;
    dim_ys.len = ncells_y;
    FORLEN32( dim_x, i, dim_xs )
      *dim_x = default_cell_dimf.x;
    }
    FORLEN32( dim_y, i, dim_ys )
      *dim_y = default_cell_dimf.y;
    }
    Fori( u32, j, 0, ncells_y ) {
    Fori( u32, i, 0, ncells_x ) {
      auto cell_autodim = cell_autodims.mem + j * ncells_conservative_x + i;
      auto dim_x = dim_xs.mem + i;
      auto dim_y = dim_ys.mem + j;
      *dim_x = MAX( *dim_x, cell_autodim->x );
      *dim_y = MAX( *dim_y, cell_autodim->y );
    }
    }
  }

  // draw row headers
  {
    kahan32_t y1 = { grid_offset.y };
    FORLEN32( dim_y, j, dim_ys )
      auto abspos_y = grid->scroll_absolutepos.y + j;
      auto y0 = y1.sum;
      Add( y1, *dim_y );
      if( j + 1 == ncells_y ) {
        // continue for one last iter, so we draw the partially-visible last one.
        y1.sum = ( bounds.p1.y - bounds.p0.y );
      }
      auto hdr_idx = abspos_y + 1; // using 1-based hdrs
      embeddedarray_t<u8, 64> hdr;
      auto success = CsFromIntegerU( hdr.mem, Capacity( hdr ), &hdr.len, hdr_idx, 1 );
      AssertCrash( success );
      auto w = LayoutString( font, spaces_per_tab, ML( hdr ) );
      DRAW_TEXT( hdr, bounds.p0 + cell_contents_offset + _vec2( max_hdr_w - w, y0 ) );
    }
  }

  // draw bkgd
  {
    RenderQuad(
      stream,
      rgba_grid_bkgd,
      bounds.p0 + grid_offset,
      bounds.p1,
      bounds.p0,
      bounds.p1,
      GetZ( zrange, applayer_t::bkgd )
      );
  }

  Reserve( grid->rendered_cells, ncells_y * ncells_x );
  grid->rendered_cells.len = 0;

  auto multicell_cursel = NumCellsCovered( &grid->cursel ) > 1;

  // draw per-column stuff
  {
    kahan32_t x1 = { grid_offset.x };
    Fori( u32, i, 0, ncells_x ) {
      auto cell_dim_x = dim_xs.mem[i];
      auto x0 = x1.sum;
      Add( x1, cell_dim_x );
      if( i + 1 == ncells_x ) {
        // continue for one last iter, so we draw the partially-visible last one.
        x1.sum = ( bounds.p1.x - bounds.p0.x );
      }

      // draw gridline x
      {
        RenderQuad(
          stream,
          rgba_gridlines,
          _vec2( bounds.p0.x + x0 + 0.0f, bounds.p0.y ),
          _vec2( bounds.p0.x + x0 + 1.0f, bounds.p1.y ),
          bounds.p0,
          bounds.p1,
          GetZ( zrange, applayer_t::bkgd )
          );
      }

      // draw col header
      {
        auto hdr_idx = grid->scroll_absolutepos.x + i + 1; // using 1-based hdrs
        embeddedarray_t<u8, 64> tmp;
        bool success = CsFromIntegerU( tmp.mem, Capacity( tmp ), &tmp.len, hdr_idx, 1 );
        AssertCrash( success );
        auto w = LayoutString( font, spaces_per_tab, ML( tmp ) );
        auto text_origin = bounds.p0 + _vec2( x0 + 0.5f * ( x1.sum - x0 - w ), 0.0f );
        auto clip_p0 = bounds.p0 + _vec2( x0, 0.0f );
        auto clip_p1 = Min( bounds.p1, clip_p0 + _vec2( x1.sum - x0, default_cell_dimf.y ) );
        DRAW_TEXT2(
          tmp,
          text_origin,
          clip_p0,
          clip_p1
          );
      }
    }
  }

  // draw per-row stuff
  {
    kahan32_t y1 = { grid_offset.y };
    Fori( u32, j, 0, ncells_y ) {
      auto cell_dim_y = dim_ys.mem[j];
      auto y0 = y1.sum;
      Add( y1, cell_dim_y );
      if( j + 1 == ncells_y ) {
        // continue for one last iter, so we draw the partially-visible last one.
        y1.sum = ( bounds.p1.y - bounds.p0.y );
      }

      // draw gridline y
      {
        RenderQuad(
          stream,
          rgba_gridlines,
          _vec2( bounds.p0.x, bounds.p0.y + y0 + 0.0f ),
          _vec2( bounds.p1.x, bounds.p0.y + y0 + 1.0f ),
          bounds.p0,
          bounds.p1,
          GetZ( zrange, applayer_t::bkgd )
          );
      }
    }
  }

  // draw per-cell stuff
  {
    kahan32_t y1 = { grid_offset.y };
    Fori( u32, j, 0, ncells_y ) {
      auto cell_dim_y = dim_ys.mem[j];
      auto y0 = y1.sum;
      Add( y1, cell_dim_y );
      if( j + 1 == ncells_y ) {
        // continue for one last iter, so we draw the partially-visible last one.
        y1.sum = ( bounds.p1.y - bounds.p0.y );
      }

      kahan32_t x1 = { grid_offset.x };
      Fori( u32, i, 0, ncells_x ) {
        auto cell_dim_x = dim_xs.mem[i];
        auto x0 = x1.sum;
        Add( x1, cell_dim_x );
        if( i + 1 == ncells_x ) {
          // continue for one last iter, so we draw the partially-visible last one.
          x1.sum = ( bounds.p1.x - bounds.p0.x );
        }
        auto p0 = _vec2( x0, y0 );
        auto p1 = _vec2( x1.sum, y1.sum );
        auto abspos = grid->scroll_absolutepos + _vec2( i, j );
        auto cell_dimf = _vec2( cell_dim_x, cell_dim_y );

        // emit cell rects for later mouse code.
        {
          auto cellrect = AddBack( grid->rendered_cells );
          cellrect->bounds.p0 = bounds.p0 + p0;
          cellrect->bounds.p1 = bounds.p0 + p1;
          cellrect->absolutepos = abspos;
        }

        // draw cursel
        if( multicell_cursel  &&  grid->c_head == abspos ) {
          RenderQuad(
            stream,
            rgba_selectedcells_head_bkgd,
            bounds.p0 + p0 + _vec2( 1.0f, 1.0f ),
            bounds.p0 + p1 + _vec2( 0.0f, 0.0f ),
            bounds.p0,
            bounds.p1,
            GetZ( zrange, applayer_t::bkgd )
            );
        }
        elif( RectlistContainsAbspos( &grid->cursel, abspos ) ) {
          RenderQuad(
            stream,
            rgba_selectedcells_bkgd,
            bounds.p0 + p0 + _vec2( 1.0f, 1.0f ),
            bounds.p0 + p1 + _vec2( 0.0f, 0.0f ),
            bounds.p0,
            bounds.p1,
            GetZ( zrange, applayer_t::bkgd )
            );
        }

        // TODO: cursel rect outlines?
//        RenderQuad(
//          stream,
//          rgba_selectedcells_outline,
//          origin + _vec2( p0.x, p0.y ) * cell_dimf + _vec2( 0.0f, 0.0f ),
//          origin + _vec2( p0.x, p1.y ) * cell_dimf + _vec2( 1.0f, 0.0f ),
//          origin, dim,
//          GetZ( zrange, applayer_t::bkgd )
//          );
//        RenderQuad(
//          stream,
//          rgba_selectedcells_outline,
//          origin + _vec2( p0.x, p0.y ) * cell_dimf + _vec2( 0.0f, 0.0f ),
//          origin + _vec2( p1.x, p0.y ) * cell_dimf + _vec2( 0.0f, 1.0f ),
//          origin, dim,
//          GetZ( zrange, applayer_t::bkgd )
//          );
//        RenderQuad(
//          stream,
//          rgba_selectedcells_outline,
//          origin + _vec2( p1.x, p0.y ) * cell_dimf + _vec2( 0.0f, 0.0f ),
//          origin + _vec2( p1.x, p1.y ) * cell_dimf + _vec2( 1.0f, 0.0f ),
//          origin, dim,
//          GetZ( zrange, applayer_t::bkgd )
//          );
//        RenderQuad(
//          stream,
//          rgba_selectedcells_outline,
//          origin + _vec2( p0.x, p1.y ) * cell_dimf + _vec2( 0.0f, 0.0f ),
//          origin + _vec2( p1.x, p1.y ) * cell_dimf + _vec2( 0.0f, 1.0f ),
//          origin, dim,
//          GetZ( zrange, applayer_t::bkgd )
//          );

        // draw cell contents
        if( grid->focus == gridfocus_t::input  &&  grid->c_head == abspos ) {
          TxtLayoutSingleLineSubset(
            grid->txt_input,
            GetBOF( grid->txt_input.buf ),
            TxtLen( grid->txt_input ),
            font
            );
          TxtRenderSingleLineSubset(
            grid->txt_input,
            stream,
            font,
            _rect(
              bounds.p0 + p0 + cell_contents_offset,
              bounds.p1 // TODO: clip against next col? i think we need better horz-scroll in txt_t for that.
              ),
            ZRange( zrange, applayer_t::txt ),
            0,
            1,
            1
            );
        }
        else {
          auto cell = TryAccessCell( grid, abspos );
          if( cell  &&  cell->input.len ) {
            switch( cell->value.type ) {
              case cellvaluetype_t::_f64: {
                embeddedarray_t<u8, 64> tmp;
                CsFrom_f64( tmp.mem, Capacity( tmp ), &tmp.len, cell->value._f64, 2 );
                auto w = LayoutString( font, spaces_per_tab, ML( tmp ) );
                DrawString(
                  stream,
                  font,
                  bounds.p0 + p0 + cell_contents_offset + _vec2( MAX( cell_dimf.x - w - 2.0f * cell_contents_offset.x, 0.0f ), 0.0f ),
                  GetZ( zrange, applayer_t::txt ),
                  bounds.p0 + p0,
                  bounds.p0 + p1,
                  rgba_text,
                  spaces_per_tab,
                  ML( tmp )
                  );
              } break;
              case cellvaluetype_t::_string: {
                DrawString(
                  stream,
                  font,
                  bounds.p0 + p0 + cell_contents_offset,
                  GetZ( zrange, applayer_t::txt ),
                  bounds.p0 + p0,
                  bounds.p0 + p1,
                  rgba_text,
                  spaces_per_tab,
                  ML( cell->value._string )
                  );
              } break;
              case cellvaluetype_t::_array: {
                ImplementCrash();
              } break;
              case cellvaluetype_t::_graph: {
                DrawGraph(
                  stream,
                  _rect(
                    bounds.p0 + p0,
                    bounds.p0 + p1
                    ),
                  zrange,
                  &cell->value._array
                  );
              } break;
              default: UnreachableCrash();
            }
          }
        }
      } // end Fori( u32, i, 0, ncells_x )
    } // end Fori( u32, j, 0, ncells_y )
  }

  Free( cell_autodims );
  Free( dim_xs );
  Free( dim_ys );
}

Inl absolutepos_t
MapMouseToAbsolutepos(
  grid_t* grid,
  vec2<s32> m
  )
{
  auto c = grid->scroll_absolutepos;

  // PERF: could save some cycles by comparing in origin-relative space.
  f32 min_distance = MAX_f32;
  FORLEN( renderedcell, i, grid->rendered_cells )
    auto mp = _vec2( Cast( f32, m.x ), Cast( f32, m.y ) );
    if( PtInBox( mp, renderedcell->bounds.p0, renderedcell->bounds.p1, 0.001f ) ) {
      c = renderedcell->absolutepos;
      break;
    }
    auto distance = DistanceToBox( mp, renderedcell->bounds.p0, renderedcell->bounds.p1 );
    if( distance < min_distance ) {
      min_distance = distance;
      c = renderedcell->absolutepos;
    }
  }

  return c;
}

Inl void
ControlMouse(
  grid_t* grid,
  bool& target_valid,
  font_t& font,
  rectf32_t bounds,
  glwmouseevent_t type,
  glwmousebtn_t btn,
  vec2<s32> m,
  vec2<s32> raw_delta,
  s32 dwheel
  )
{
  ProfFunc();

  auto scroll_nlines = GetPropFromDb( u8, u8_scroll_nlines );
  auto scroll_sign = GetPropFromDb( s8, s8_scroll_sign );
//  auto dblclick_period_sec = GetPropFromDb( f64, f64_dblclick_period_sec );
//  auto scroll_continuous = GetPropFromDb( bool, bool_scroll_continuous );
//  auto scroll_continuous_sensitivity = GetPropFromDb( f64, f64_scroll_continuous_sensitivity );

//  auto scroll_pct = GetPropFromDb( f32, f32_scroll_pct );
//  auto px_scroll = MAX( 16.0f, Round32( scroll_pct * MIN( dim.x, dim.y ) ) );

  auto keymods = GlwKeyModifiersDown();
  bool mod_isdn = AnyDown( keymods );

  bool only_ctrl_isdn = ( keymods.ctrl  &&  !keymods.shift  && !keymods.alt );

#if 0 // TODO: grid scrollbar
  if( ScrollbarVisible( bounds, px_scroll ) ) {

    auto btn_up = GetScrollBtnUp( bounds, px_scroll );
    auto btn_dn = GetScrollBtnDn( bounds, px_scroll );
    auto scroll_pos = GetScrollPos( txt );
    auto btn_pos = GetScrollBtnPos( scroll_pos.x, scroll_pos.z, bounds, px_scroll );

#if USE_SIMPLE_SCROLLING
    f32 t = CLAMP( EstimateLinearPos( txt, txt.scroll_start.y ), 0, 1 );
#else
    f32 t = CLAMP( EstimateLinearPos( txt, txt.scroll_target.y ), 0, 1 );
#endif

    if( GlwMouseInsideRect( m, btn_up[0], btn_up[1] ) ) {
      switch( type ) {
        case glwmouseevent_t::dn: {
          SetScrollPosFraction( txt, CLAMP( t - 0.1f, 0, 1 ) );
          target_valid = 0;
        } break;
        case glwmouseevent_t::wheelmove:
        case glwmouseevent_t::up:
        case glwmouseevent_t::move: {
        } break;
        default: UnreachableCrash();
      }

    } elif( GlwMouseInsideRect( m, btn_dn[0], btn_dn[1] ) ) {
      switch( type ) {
        case glwmouseevent_t::dn: {
          SetScrollPosFraction( txt, CLAMP( t + 0.1f, 0, 1 ) );
          target_valid = 0;
        } break;
        case glwmouseevent_t::wheelmove:
        case glwmouseevent_t::up:
        case glwmouseevent_t::move: {
        } break;
        default: UnreachableCrash();
      }

    } elif( GlwMouseInsideRect( m, btn_pos[0], btn_pos[1] ) ) {
      // TODO: scrollbar rect interactivity.
    }

    dim.x -= px_scroll;
  }
#endif

  if( !GlwMouseInsideRect( m, bounds ) ) {
    // clear all interactivity state.
//    txt.dblclick.first_made = 0;
    return;
  }

  switch( type ) {

    case glwmouseevent_t::wheelmove: {
      if( dwheel  &&  ( !mod_isdn ) ) {
        dwheel *= scroll_sign;
        dwheel *= scroll_nlines;
        if( dwheel >= 0 ) {
          grid->scroll_absolutepos = MoveYR( grid->scroll_absolutepos, Cast( u32, dwheel ) );
        } else {
          grid->scroll_absolutepos = MoveYL( grid->scroll_absolutepos, Cast( u32, -dwheel ) );
        }
        target_valid = 0;

      } elif( dwheel  &&  only_ctrl_isdn ) {
        dwheel *= scroll_sign;
//        dwheel *= scroll_nlines;
        if( dwheel >= 0 ) {
          grid->scroll_absolutepos = MoveXR( grid->scroll_absolutepos, Cast( u32, dwheel ) );
        } else {
          grid->scroll_absolutepos = MoveXL( grid->scroll_absolutepos, Cast( u32, -dwheel ) );
        }
        target_valid = 0;
      }
    } break;

    case glwmouseevent_t::dn: {

      switch( btn ) {

        case glwmousebtn_t::l: {
          auto absolutepos = MapMouseToAbsolutepos( grid, m );
          grid->c_head = absolutepos;
          grid->s_head = absolutepos;
          if( !keymods.ctrl ) {
            SetCurselToAbspos( grid, grid->c_head );
          }
          else {
            AddToCursel( grid, grid->c_head );
          }
          target_valid = 0;
        } break;

        case glwmousebtn_t::r:
        case glwmousebtn_t::m: {
        } break;

        case glwmousebtn_t::b4: {
          grid->scroll_absolutepos = MoveXL( grid->scroll_absolutepos, scroll_nlines );
          target_valid = 0;
        } break;

        case glwmousebtn_t::b5: {
          grid->scroll_absolutepos = MoveXR( grid->scroll_absolutepos, scroll_nlines );
          target_valid = 0;
        } break;

        default: UnreachableCrash();
      }
    } break;

    case glwmouseevent_t::up: {
    } break;

    case glwmouseevent_t::move: {
      if( GlwMouseBtnIsDown( glwmousebtn_t::l ) ) {
        auto absolutepos = MapMouseToAbsolutepos( grid, m );
        grid->c_head = absolutepos;
        absoluterect_t rect;
        rect.p0 = Min( grid->c_head, grid->s_head );
        rect.p1 = Max( grid->c_head, grid->s_head ) + _vec2<u32>( 1, 1 );
        bool changed;
        SetCurselToRect( grid, rect, &changed );
        if( changed ) {
          target_valid = 0;
        }
      }
    } break;

    default: UnreachableCrash();
  }

//  printf(
//    "c_head  %u,%u    s_head  %u,%u\n",
//    grid->c_head.x,
//    grid->c_head.y,
//    grid->s_head.x,
//    grid->s_head.y
//    );
}





struct
app_t
{
  glwclient_t client;
  bool fullscreen;
  array_t<f32> stream;
  array_t<font_t> fonts;
  grid_t grid;
};

static app_t g_app = {};


void
AppInit( app_t* app )
{
  GlwInit();

  Alloc( app->stream, 65536 );
  Alloc( app->fonts, 16 );
  app->fullscreen = 0;

  GlwInitWindow(
    app->client,
    Str( "grid" ), 4
    );

  Init( &app->grid );
}

void
AppKill( app_t* app )
{
  Kill( &app->grid );
  FORLEN( font, i, app->fonts )
    FontKill( *font );
  }
  Free( app->fonts );
  Free( app->stream );
  GlwKillWindow( app->client );
  GlwKill();
}


Enumc( fontid_t )
{
  normal,
};


void
LoadFont(
  app_t* app,
  enum_t fontid,
  u8* filename_ttf,
  idx_t filename_ttf_len,
  f32 char_h
  )
{
  AssertWarn( fontid < 10000 ); // sanity check.
  Reserve( app->fonts, fontid + 1 );
  app->fonts.len = MAX( app->fonts.len, fontid + 1 );

  auto& font = app->fonts.mem[fontid];
  FontLoad( font, filename_ttf, filename_ttf_len, char_h );
  FontLoadAscii( font );
}

void
UnloadFont( app_t* app, enum_t fontid )
{
  auto font = app->fonts.mem + fontid;
  FontKill( *font );
}

font_t&
GetFont( app_t* app, enum_t fontid )
{
  return app->fonts.mem[fontid];
}


__OnRender( AppOnRender )
{
  ProfFunc();

  auto app = Cast( app_t*, misc );
  auto& font = GetFont( app, Cast( enum_t, fontid_t::normal ) );
  auto zrange = _vec2<f32>( 0, 1 );

#ifdef _DEBUG
  cell2_t c_foo = {};
  c_foo.eval_generation = 2;
#endif

  auto spaces_per_tab = GetPropFromDb( u8, u8_spaces_per_tab );
  auto rgba_notify_text = GetPropFromDb( vec4<f32>, rgba_notify_text );
  auto rgba_notify_bkgd = GetPropFromDb( vec4<f32>, rgba_notify_bkgd );
  auto rgba_mode_command = GetPropFromDb( vec4<f32>, rgba_mode_command );
  auto rgba_mode_edit = GetPropFromDb( vec4<f32>, rgba_mode_edit );
  auto rgba_text = GetPropFromDb( vec4<f32>, rgba_text );
  auto rgba_text_bkgd = GetPropFromDb( vec4<f32>, rgba_text_bkgd );
  auto rgba_selection_bkgd = GetPropFromDb( vec4<f32>, rgba_selection_bkgd );
  auto rgba_selection_outline = GetPropFromDb( vec4<f32>, rgba_selection_outline );
  auto rgba_selection_head = GetPropFromDb( vec4<f32>, rgba_selection_head );

//  auto modeoutline_pct = GetPropFromDb( f32, f32_modeoutline_pct );
//  auto timestep = MIN( timestep_realtime, timestep_fixed );

  RenderGrid(
    &app->grid,
    app->stream,
    font,
    bounds,
    zrange,
    rgba_text,
    rgba_text_bkgd /*rgba_grid_bkgd*/,
    rgba_text /*rgba_gridlines*/,
    rgba_selection_outline /*rgba_selectedcells_outline*/,
    rgba_selection_bkgd /*rgba_selectedcells_bkgd*/,
    rgba_selection_head /*rgba_selectedcells_head_bkgd*/
    );

  if( 0 )
  { // display timestep_realtime, as a way of tracking how long rendering takes.
    embeddedarray_t<u8, 64> tmp;
    CsFrom_f64( tmp.mem, Capacity( tmp ), &tmp.len, 1000 * timestep_realtime );
    auto w = LayoutString( font, spaces_per_tab, ML( tmp ) );
    DrawString(
      app->stream,
      font,
      bounds.p1 + _vec2( -w, 0.0f ),
      GetZ( zrange, applayer_t::txt ),
      bounds,
      rgba_notify_text,
      spaces_per_tab,
      ML( tmp )
      );
  }

  if( app->stream.len ) {

    auto pos = app->stream.mem;
    auto end = app->stream.mem + app->stream.len;
    AssertCrash( app->stream.len % 10 == 0 );
    while( pos < end ) {
      Prof( tmp_UnpackStream );
      auto idx = pos - app->stream.mem;
      vec2<u32> q0;
      vec2<u32> q1;
      vec2<u32> tc0;
      vec2<u32> tc1;
      q0.x  = Round_u32_from_f32( *pos++ );
      q0.y  = Round_u32_from_f32( *pos++ );
      q1.x  = Round_u32_from_f32( *pos++ );
      q1.y  = Round_u32_from_f32( *pos++ );
      tc0.x = Round_u32_from_f32( *pos++ );
      tc0.y = Round_u32_from_f32( *pos++ );
      tc1.x = Round_u32_from_f32( *pos++ );
      tc1.y = Round_u32_from_f32( *pos++ );
      *pos++; // this is z; but we don't do z reordering right now.
      auto color = UnpackColorForShader( *pos++ );
      auto color_a = Cast( u8, Round_u32_from_f32( color.w * 255.0f ) & 0xFFu );
      auto color_r = Cast( u8, Round_u32_from_f32( color.x * 255.0f ) & 0xFFu );
      auto color_g = Cast( u8, Round_u32_from_f32( color.y * 255.0f ) & 0xFFu );
      auto color_b = Cast( u8, Round_u32_from_f32( color.z * 255.0f ) & 0xFFu );
      auto coloru = ( color_a << 24 ) | ( color_r << 16 ) | ( color_g <<  8 ) | color_b;
      auto color_argb = _mm_set_ps( color.w, color.x, color.y, color.z );
      auto color255_argb = _mm_mul_ps( color_argb, _mm_set1_ps( 255.0f ) );
      bool just_copy =
        color.x == 1.0f  &&
        color.y == 1.0f  &&
        color.z == 1.0f  &&
        color.w == 1.0f;
      bool no_alpha = color.w == 1.0f;

      AssertCrash( q0.x <= q1.x );
      AssertCrash( q0.y <= q1.y );
      auto dstdim = q1 - q0;
      AssertCrash( tc0.x <= tc1.x );
      AssertCrash( tc0.y <= tc1.y );
      auto srcdim = tc1 - tc0;
      AssertCrash( q0.x + srcdim.x <= app->client.dim.x );
      AssertCrash( q0.y + srcdim.y <= app->client.dim.y );
      AssertCrash( q0.x + dstdim.x <= app->client.dim.x ); // should have clipped by now.
      AssertCrash( q0.y + dstdim.y <= app->client.dim.y );
      AssertCrash( tc0.x + srcdim.x <= font.tex_dim.x );
      AssertCrash( tc0.y + srcdim.y <= font.tex_dim.y );
      ProfClose( tmp_UnpackStream );

      bool subpixel_quad =
        ( q0.x == q1.x )  ||
        ( q0.y == q1.y );

      // below our resolution, so ignore it.
      // we're not going to do linear-filtering rendering here.
      // something to consider for the future.
      if( subpixel_quad ) {
        continue;
      }

      // for now, assume quads with empty tex coords are just plain rects, with the given color.
      bool nontex_quad =
        ( !srcdim.x  &&  !srcdim.y );

      if( nontex_quad ) {
        u32 copy = 0;
        if( just_copy ) {
          copy = MAX_u32;
        } elif( no_alpha ) {
          copy = ( 0xFF << 24 ) | coloru;
        }

        if( just_copy  ||  no_alpha ) {
          Prof( tmp_DrawOpaqueQuads );

          // PERF: this is ~0.7 cycles / pixel
          // suprisingly, rep stosq is about the same as rep stosd!
          // presumably the hardware folks made that happen, so old 32bit code also got faster.
          auto copy2 = Pack( copy, copy );
          Fori( u32, j, 0, dstdim.y ) {
            auto dst = app->client.fullscreen_bitmap_argb + ( q0.y + j ) * app->client.dim.x + ( q0.x + 0 );
            if( dstdim.x & 1 ) {
              *dst++ = copy;
            }
            auto dim2 = dstdim.x / 2;
            auto dst2 = Cast( u64*, dst );
            Fori( u32, k, 0, dim2 ) {
              *dst2++ = copy2;
            }
          }

        } else {
          Prof( tmp_DrawAlphaQuads );
          // more complicated / slow alpha case.
          Fori( u32, j, 0, dstdim.y ) {
            auto dst = app->client.fullscreen_bitmap_argb + ( q0.y + j ) * app->client.dim.x + ( q0.x + 0 );
            Fori( u32, k, 0, dstdim.x ) {

              // PERF: this is ~6 cycles / pixel

              // unpack dst
              auto dstf = _mm_set_ss( *Cast( f32*, dst ) );
              auto dsti = _mm_cvtepu8_epi32( _mm_castps_si128( dstf ) );
              auto dst_argb = _mm_cvtepi32_ps( dsti );

              // src = color
              auto src_argb = color255_argb;

              // out = dst + src_a * ( src - dst )
              auto src_a = _mm_set1_ps( color.w );
              auto diff_argb = _mm_sub_ps( src_argb, dst_argb );
              auto out_argb = _mm_fmadd_ps( src_a, diff_argb, dst_argb );

              // put out in [0, 255]
              auto outi = _mm_cvtps_epi32( out_argb );
              auto out = _mm_packs_epi32( outi, _mm_set1_epi32( 0 ) );
              out = _mm_packus_epi16( out, _mm_set1_epi32( 0 ) );

              _mm_storeu_si32( dst, out ); // equivalent to: *dst = out.m128i_u32[0]
              ++dst;
            }
          }
        }
      } else {
        Prof( tmp_DrawTexQuads );
        // for now, assume quads with nonempty tex coords are exact-size copies onto dst.
        // that's true of all our text glyphs, which makes for faster code here.
        //
        // TODO: handle scaling up, or make a separate path for scaled-up tex, non-tex, etc. quads
        // we'll need separate paths for tris, lines anyways, so maybe do that.
        Fori( u32, j, 0, srcdim.y ) {
          auto dst = app->client.fullscreen_bitmap_argb + ( q0.y + j ) * app->client.dim.x + ( q0.x + 0 );
          auto src = font.tex_mem + ( tc0.y + j ) * font.tex_dim.x + ( tc0.x + 0 );
          Fori( u32, k, 0, srcdim.x ) {
            // PERF: this is ~7 cycles / pixel

            // given color in [0, 1]
            // given src, dst in [0, 255]
            // src *= color
            // fac = src_a / 255
            // out = dst + fac * ( src - dst )

            // unpack src, dst
            auto dstf = _mm_set_ss( *Cast( f32*, dst ) );
            auto dsti = _mm_cvtepu8_epi32( _mm_castps_si128( dstf ) );
            auto dst_argb = _mm_cvtepi32_ps( dsti );

            auto srcf = _mm_set_ss( *Cast( f32*, src ) );
            auto srci = _mm_cvtepu8_epi32( _mm_castps_si128( srcf ) );
            auto src_argb = _mm_cvtepi32_ps( srci );

            // src *= color
            src_argb = _mm_mul_ps( src_argb, color_argb );

            // out = dst + src_a * ( src - dst )
            auto fac = _mm_shuffle_ps( src_argb, src_argb, 255 ); // equivalent to set1( m128_f32[3] )
            fac = _mm_mul_ps( fac, _mm_set1_ps( 1.0f / 255.0f ) );
            auto diff_argb = _mm_sub_ps( src_argb, dst_argb );
            auto out_argb = _mm_fmadd_ps( fac, diff_argb, dst_argb );

            // get out in [0, 255]
            auto outi = _mm_cvtps_epi32( out_argb );
            auto out = _mm_packs_epi32( outi, _mm_set1_epi32( 0 ) );
            out = _mm_packus_epi16( out, _mm_set1_epi32( 0 ) );

            _mm_storeu_si32( dst, out ); // equivalent to: *dst = out.m128i_u32[0]
            ++dst;
            ++src;
          }
        }
      }
    }

    app->stream.len = 0;
  }
}

Inl void
GridCmd_DeleteCellContents( grid_t* grid )
{
  DeleteCellContents( grid, &grid->cursel );
  RefreshCurselIdenticals( grid );
}

Inl void
GridCmd_EnterCellContents( grid_t* grid )
{
  AssertCrash( grid->focus == gridfocus_t::input );

  auto input_start = GetBOF( grid->txt_input.buf );
  slice32_t input;
  auto len = TxtLen( grid->txt_input );
  AssertCrash( len <= MAX_u32 );
  input.len = Cast( u32, len );
  if( input.len ) {
    input.mem = AddPlist( grid->cellmem, u8, 1, input.len );
    Contents( grid->txt_input.buf, input_start, ML( input ) );
    InsertOrSetCellContents( grid, &grid->cursel, input );
    RefreshCurselIdenticals( grid );
    if( grid->has_identical_error  &&  HasError( &grid->identical_error ) ) {
      auto bof = GetBOF( grid->txt_input.buf );
      auto eof = GetEOF( grid->txt_input.buf );
      auto sel_l = CursorCharR( grid->txt_input.buf, bof, grid->identical_error.start, 0 );
      auto sel_len = grid->identical_error.end - grid->identical_error.start;
      auto sel_r = CursorCharR( grid->txt_input.buf, sel_l, sel_len, 0 );
      if( !sel_len ) {
        sel_l = bof;
        sel_r = eof;
      }
      CmdSetSelection( grid->txt_input, Cast( idx_t, &sel_l ), Cast( idx_t, &sel_r ) );
    }
  }
  else {
    DeleteCellContents( grid, &grid->cursel );
    RefreshCurselIdenticals( grid );
  }

  grid->focus = gridfocus_t::cursel;
}

Inl void
GridCmd_CursorL( grid_t* grid, u32 nmove = 1 )
{
  AssertCrash( grid->focus == gridfocus_t::cursel );
  grid->c_head = MoveXL( grid->c_head, nmove );
  grid->s_head = grid->c_head;
  SetCurselToAbspos( grid, grid->c_head );
}

Inl void
GridCmd_SelectL( grid_t* grid, u32 nmove = 1 )
{
  AssertCrash( grid->focus == gridfocus_t::cursel );
  grid->c_head = MoveXL( grid->c_head, nmove );
  absoluterect_t rect;
  rect.p0 = Min( grid->c_head, grid->s_head );
  rect.p1 = Max( grid->c_head, grid->s_head ) + _vec2<u32>( 1, 1 );
  AddToCursel( grid, rect );
}

Inl void
GridCmd_CursorR( grid_t* grid, u32 nmove = 1 )
{
  AssertCrash( grid->focus == gridfocus_t::cursel );
  grid->c_head = MoveXR( grid->c_head, nmove );
  grid->s_head = grid->c_head;
  SetCurselToAbspos( grid, grid->c_head );
}

Inl void
GridCmd_SelectR( grid_t* grid, u32 nmove = 1 )
{
  AssertCrash( grid->focus == gridfocus_t::cursel );
  grid->c_head = MoveXR( grid->c_head, nmove );
  absoluterect_t rect;
  rect.p0 = Min( grid->c_head, grid->s_head );
  rect.p1 = Max( grid->c_head, grid->s_head ) + _vec2<u32>( 1, 1 );
  AddToCursel( grid, rect );
}

Inl void
GridCmd_CursorU( grid_t* grid, u32 nmove = 1 )
{
  AssertCrash( grid->focus == gridfocus_t::cursel );
  grid->c_head = MoveYL( grid->c_head, nmove );
  grid->s_head = grid->c_head;
  SetCurselToAbspos( grid, grid->c_head );
}

Inl void
GridCmd_SelectU( grid_t* grid, u32 nmove = 1 )
{
  AssertCrash( grid->focus == gridfocus_t::cursel );
  grid->c_head = MoveYL( grid->c_head, nmove );
  absoluterect_t rect;
  rect.p0 = Min( grid->c_head, grid->s_head );
  rect.p1 = Max( grid->c_head, grid->s_head ) + _vec2<u32>( 1, 1 );
  AddToCursel( grid, rect );
}

Inl void
GridCmd_CursorD( grid_t* grid, u32 nmove = 1 )
{
  AssertCrash( grid->focus == gridfocus_t::cursel );
  grid->c_head = MoveYR( grid->c_head, nmove );
  grid->s_head = grid->c_head;
  SetCurselToAbspos( grid, grid->c_head );
}

Inl void
GridCmd_SelectD( grid_t* grid, u32 nmove = 1 )
{
  AssertCrash( grid->focus == gridfocus_t::cursel );
  grid->c_head = MoveYR( grid->c_head, nmove );
  absoluterect_t rect;
  rect.p0 = Min( grid->c_head, grid->s_head );
  rect.p1 = Max( grid->c_head, grid->s_head ) + _vec2<u32>( 1, 1 );
  AddToCursel( grid, rect );
}

Inl void
GridCmd_SwitchModeCurselFromInput( grid_t* grid )
{
  AssertCrash( grid->focus == gridfocus_t::input );
  grid->focus = gridfocus_t::cursel;
}

Inl void
ControlKeyboard(
  grid_t* grid,
  bool& ran_cmd,
  bool& target_valid,
  glwkeyevent_t type,
  glwkey_t key,
  glwkeylocks_t keylocks
  )
{
  #define GRIDCMDMAP0( _keybind, _cmd ) \
    if( !ran_cmd  &&  GlwKeybind( key, GetPropFromDb( glwkeybind_t, _keybind ) ) ) { \
      _cmd( grid ); \
      target_valid = 0; \
      ran_cmd = 1; \
    } \

  #define GRIDCMDMAP1( _keybind, _cmd, _misc ) \
    if( !ran_cmd  &&  GlwKeybind( key, GetPropFromDb( glwkeybind_t, _keybind ) ) ) { \
      _cmd( grid, _misc ); \
      target_valid = 0; \
      ran_cmd = 1; \
    } \

  switch( grid->focus ) {

    case gridfocus_t::cursel: {

      switch( type ) {
        case glwkeyevent_t::dn:
        case glwkeyevent_t::repeat: {
          GRIDCMDMAP0( keybind_txt_cursor_l, GridCmd_CursorL );
          GRIDCMDMAP0( keybind_txt_cursor_r, GridCmd_CursorR );
          GRIDCMDMAP0( keybind_txt_cursor_u, GridCmd_CursorU );
          GRIDCMDMAP0( keybind_txt_cursor_d, GridCmd_CursorD );
          GRIDCMDMAP1( keybind_txt_cursor_page_u, GridCmd_CursorU, 10 ); // TODO: store page height and pass here.
          GRIDCMDMAP1( keybind_txt_cursor_page_d, GridCmd_CursorD, 10 );
          GRIDCMDMAP0( keybind_txt_select_l, GridCmd_SelectL );
          GRIDCMDMAP0( keybind_txt_select_r, GridCmd_SelectR );
          GRIDCMDMAP0( keybind_txt_select_u, GridCmd_SelectU );
          GRIDCMDMAP0( keybind_txt_select_d, GridCmd_SelectD );
          GRIDCMDMAP1( keybind_txt_select_page_u, GridCmd_SelectU, 10 ); // TODO: store page height and pass here.
          GRIDCMDMAP1( keybind_txt_select_page_d, GridCmd_SelectD, 10 );
        } break;

        case glwkeyevent_t::up: {
          GRIDCMDMAP0( keybind_txt_rem_ch_l, GridCmd_DeleteCellContents );
          GRIDCMDMAP0( keybind_txt_rem_ch_r, GridCmd_DeleteCellContents );
        } break;

        default: UnreachableCrash();
      }
      if( !ran_cmd ) {
        bool content_changed = 0;
        TxtControlKeyboardTypeNotModal(
          grid->txt_input,
          target_valid,
          content_changed,
          ran_cmd,
          type,
          key,
          keylocks,
          0
          );
        if( content_changed ) {
          grid->focus = gridfocus_t::input;
        }
      }
    } break;

    case gridfocus_t::input: {

      switch( type ) {
        case glwkeyevent_t::dn:
        case glwkeyevent_t::repeat: {
        } break;

        case glwkeyevent_t::up: {
          GRIDCMDMAP0( keybind_txt_ln_add, GridCmd_EnterCellContents );
          GRIDCMDMAP0( keybind_txt_rem_ch_r, GridCmd_DeleteCellContents );
          GRIDCMDMAP0( keybind_esc, GridCmd_SwitchModeCurselFromInput );
        } break;

        default: UnreachableCrash();
      }
      if( !ran_cmd ) {
        bool content_changed = 0;
        TxtControlKeyboardSingleLineNotModal(
          grid->txt_input,
          target_valid,
          content_changed,
          ran_cmd,
          type,
          key,
          keylocks
          );
      }
    } break;
    default: UnreachableCrash();
  }
}

__OnKeyEvent( AppOnKeyEvent )
{
  ProfFunc();

  auto app = Cast( app_t*, misc );

  // Global key event handling:
  bool ran_cmd = 0;
  switch( type ) {
    case glwkeyevent_t::dn:
    case glwkeyevent_t::repeat: {
      if( GlwKeybind( key, GetPropFromDb( glwkeybind_t, keybind_app_quit ) ) ) {
        GlwEarlyKill( app->client );
        target_valid = 0;
        ran_cmd = 1;
      }
    } break;

    case glwkeyevent_t::up: {
      switch( key ) {
        case glwkey_t::fn_11: {
          app->fullscreen = !app->fullscreen;
          fullscreen = app->fullscreen;
          ran_cmd = 1;
        } break;

#if PROF_ENABLED
        case glwkey_t::fn_9: {
          ProfEnable();
          ran_cmd = 1;
        } break;

        case glwkey_t::fn_10: {
          ProfDisable();
          ran_cmd = 1;
        } break;
#endif

      }
    } break;

    default: UnreachableCrash();
  }

  if( !ran_cmd ) {
    auto keylocks = GlwKeylocks();
    ControlKeyboard(
      &app->grid,
      ran_cmd,
      target_valid,
      type,
      key,
      keylocks
      );
  }
}



__OnMouseEvent( AppOnMouseEvent )
{
  ProfFunc();

  auto app = Cast( app_t*, misc );

  auto& font = GetFont( app, Cast( enum_t, fontid_t::normal ) );

  cursortype = glwcursortype_t::arrow;

  ControlMouse(
    &app->grid,
    target_valid,
    font,
    bounds,
    type,
    btn,
    m,
    raw_delta,
    dwheel
    );
}

__OnWindowEvent( AppOnWindowEvent )
{
  ProfFunc();

  auto app = Cast( app_t*, misc );

  if( type & glwwindowevent_resize ) {
  }
  if( type & glwwindowevent_focuschange ) {
  }
  if( type & glwwindowevent_dpichange ) {
    auto fontsize_normal = GetPropFromDb( f32, f32_fontsize_normal );
    auto filename_font_normal = GetPropFromDb( slice_t, string_filename_font_normal );

    UnloadFont( app, Cast( idx_t, fontid_t::normal ) );
    LoadFont(
      app,
      Cast( idx_t, fontid_t::normal ),
      ML( filename_font_normal ),
      fontsize_normal * Cast( f32, dpi )
      );
  }
}



#ifdef _DEBUG
  struct
  testcase_t
  {
    slice32_t input;
    bool success;
    slice_t printed_string;
  };
#endif

int
Main( array_t<slice_t>& args )
{
  PinThreadToOneCore();

  auto app = &g_app;
  AppInit( app );

  if( args.len == 1 ) {
    auto arg = args.mem + 0;
    filemapped_t file = FileOpenMappedExistingReadShareRead( ML( *arg ) );
    if( !file.loaded ) {
      // TODO: print why we can't load!
      MessageBoxA(
        0,
        Cast( LPCSTR, AllocCstr( *arg ) ),
        "Couldn't open file!",
        0
        );
      return -1;
    }
    auto ext = FileExtension( ML( *arg ) );
    if( ext.len ) {
      if( EqualContents( ext, SliceFromCStr( "csv" ) ) ) {
        slice_t filemem;
        filemem.mem = file.mapped_mem;
        filemem.len = file.size;
        LoadCSV( &app->grid, filemem );
      }
      else {
        MessageBoxA(
          0,
          Cast( LPCSTR, AllocCstr( *arg ) ),
          "File extension not supported!",
          0
          );
        return -1;
      }
    }
    FileFree( file );
  }

  auto fontsize_normal = GetPropFromDb( f32, f32_fontsize_normal );
  auto filename_font_normal = GetPropFromDb( slice_t, string_filename_font_normal );

#ifdef _DEBUG // tokenize/parse testing
    compileerror_t error = {};
    array_t<token_t> tokens;
    Alloc( tokens, 1024 );
    plist_t scratchmem;
    Init( scratchmem, 16000 );
    array_t<u8> tmp;
    Alloc( tmp, 4096 );
    array_t<node_expr_t*> tmpargmem;
    Alloc( tmpargmem, 256 );

    testcase_t cases[] = {
      { Slice32FromCStr( "3+4+5" ), 1, SliceFromCStr( "add( 3, add( 4, 5 ) )" ) },
      { Slice32FromCStr( "2+3*4" ), 1, SliceFromCStr( "add( 2, mul( 3, 4 ) )" ) },
      { Slice32FromCStr( "2*3+4" ), 1, SliceFromCStr( "add( mul( 2, 3 ), 4 )" ) },
      { Slice32FromCStr( "2^3*4" ), 1, SliceFromCStr( "mul( pow( 2, 3 ), 4 )" ) },
      { Slice32FromCStr( "1*4+3*8/4^17" ), 1, SliceFromCStr( "add( mul( 1, 4 ), mul( 3, div( 8, pow( 4, 17 ) ) ) )" ) },
      { Slice32FromCStr( "3*(4+5)" ), 1, SliceFromCStr( "mul( 3, add( 4, 5 ) )" ) },
      { Slice32FromCStr( "(3*4)+5" ), 1, SliceFromCStr( "add( mul( 3, 4 ), 5 )" ) },
      { Slice32FromCStr( "(3*4)^2+5" ), 1, SliceFromCStr( "add( pow( mul( 3, 4 ), 2 ), 5 )" ) },
      { Slice32FromCStr( "3*(4^2)+5" ), 1, SliceFromCStr( "add( mul( 3, pow( 4, 2 ) ), 5 )" ) },
      { Slice32FromCStr( "3*4^(2+5)" ), 1, SliceFromCStr( "mul( 3, pow( 4, add( 2, 5 ) ) )" ) },
      { Slice32FromCStr( "cell(1, 2)" ), 1, SliceFromCStr( "cell( 1, 2 )" ) },
      { Slice32FromCStr( "row(1, 1, 10)" ), 1, SliceFromCStr( "row( 1, 1, 10 )" ) },
      { Slice32FromCStr( "col(2, 2, 9)" ), 1, SliceFromCStr( "col( 2, 2, 9 )" ) },
      { Slice32FromCStr( "cell(1,2)+cell(1,3)" ), 1, SliceFromCStr( "add( cell( 1, 2 ), cell( 1, 3 ) )" ) },
      { Slice32FromCStr( "cell(1,2)+cell(1,3)^2.123" ), 1, SliceFromCStr( "add( cell( 1, 2 ), pow( cell( 1, 3 ), 2.123 ) )" ) },
      { Slice32FromCStr( "cell(cell(1,2),3+4)" ), 1, SliceFromCStr( "cell( cell( 1, 2 ), add( 3, 4 ) )" ) },
      { Slice32FromCStr( "cell(cell(1,2),cell(1,2)+4)" ), 1, SliceFromCStr( "cell( cell( 1, 2 ), add( cell( 1, 2 ), 4 ) )" ) },

//      { Slice32FromCStr( "" ), 1, SliceFromCStr( "" ) },
      { Slice32FromCStr( "foo" ), 0, {} },
      { Slice32FromCStr( "cell" ), 0, {} },
      { Slice32FromCStr( "cell()" ), 0, {} },
      { Slice32FromCStr( "cell(" ), 0, {} },
      { Slice32FromCStr( "cell(1" ), 0, {} },
      { Slice32FromCStr( "cell(1," ), 0, {} },
      { Slice32FromCStr( "cell(1,2" ), 0, {} },
      { Slice32FromCStr( "cell(1,2,3)" ), 0, {} },
      { Slice32FromCStr( "cell(1,2))" ), 0, {} },
//      { Slice32FromCStr( "" ), 0, {} },
    };

    ForEach( test, cases ) {
      error = {};
      tokens.len = 0;
      Tokenize( test.input, &tokens, &error );
      node_expr_t* expr = 0;
      if( !error.text.len ) {
        parse_input_t parse_input;
        parse_input.tokens = &tokens;
        parse_input.nodemem = &scratchmem;
        parse_input.errortextmem = &scratchmem;
        parse_input.tmpargmem = &tmpargmem;
        expr = Parse( &parse_input, &error );
      }

      if( error.text.len ) {
        AssertCrash( !test.success ); // why did we fail to parse a good input?
      } else {
        AssertCrash( expr );
        tmp.len = 0;
        PrintNode( expr, &tmp );
        auto eq = MemEqual( ML( tmp ), ML( test.printed_string ) );
        AssertCrash( test.success ); // why did we successfully parse a malformed input?
        AssertCrash( eq ); // why did we parse to something other than expected?
      }
    }

    Free( tmpargmem );
    Free( tmp );
    Kill( scratchmem );
    Free( tokens );
#endif

  LoadFont(
    app,
    Cast( idx_t, fontid_t::normal ),
    ML( filename_font_normal ),
    fontsize_normal * Cast( f32, app->client.dpi )
    );

  glwcallback_t callbacks[] = {
    { glwcallbacktype_t::keyevent           , app, AppOnKeyEvent            },
    { glwcallbacktype_t::mouseevent         , app, AppOnMouseEvent          },
    { glwcallbacktype_t::windowevent        , app, AppOnWindowEvent         },
    { glwcallbacktype_t::render             , app, AppOnRender              },
  };
  ForEach( callback, callbacks ) {
    GlwRegisterCallback( app->client, callback );
  }

  GlwMainLoop( app->client );

  // Do this before destroying any datastructures, so other threads stop trying to access things.
  SignalQuitAndWaitForTaskThreads();

  AppKill( app );

  return 0;
}




Inl void
IgnoreSurroundingSpaces( u8*& a, idx_t& a_len )
{
  while( a_len  &&  IsWhitespace( a[0] ) ) {
    a += 1;
    a_len -= 1;
  }
  // TODO: prog_cmd_line actually has two 0-terms!
  while( a_len  &&  ( !a[a_len - 1]  ||  IsWhitespace( a[a_len - 1] ) ) ) {
    a_len -= 1;
  }
}

int WINAPI
WinMain( HINSTANCE prog_inst, HINSTANCE prog_inst_prev, LPSTR prog_cmd_line, int prog_cmd_show )
{
  MainInit();

  u8* cmdline = Str( prog_cmd_line );
  idx_t cmdline_len = CsLen( Str( prog_cmd_line ) );

  array_t<slice_t> args;
  Alloc( args, 64 );

  while( cmdline_len ) {
    IgnoreSurroundingSpaces( cmdline, cmdline_len );
    if( !cmdline_len ) {
      break;
    }
    auto arg = cmdline;
    idx_t arg_len = 0;
    auto quoted = cmdline[0] == '"';
    if( quoted ) {
      arg = cmdline + 1;
      auto end = CsScanR( arg, cmdline_len - 1, '"' );
      arg_len = !end  ?  0  :  end - arg;
      IgnoreSurroundingSpaces( arg, arg_len );
    } else {
      auto end = CsScanR( arg, cmdline_len - 1, ' ' );
      arg_len = !end  ?  cmdline_len  :  end - arg;
      IgnoreSurroundingSpaces( arg, arg_len );
    }
    if( arg_len ) {
      auto add = AddBack( args );
      add->mem = arg;
      add->len = arg_len;
    }
    cmdline = arg + arg_len;
    cmdline_len -= arg_len;
    if( quoted ) {
      cmdline += 1;
      cmdline_len -= 1;
    }
  }
  auto r = Main( args );
  Free( args );

  Log( "Main returned: %d", r );

  MainKill();
  return r;
}



// OLD JUNK WE MIGHT WANT TO KEEP AROUND:


  // TODO: this algorithm doesn't quite work.
  // if we have a huge cell in the conservative set that we don't include in the actual ncells set,
  // we'll still have stored the max size.
  // e.g. a huge graph in col 6, when only cols 1-5 are visible.
  // we'll still draw the row containing the graph with a huge height, which we don't want.
  // i guess we need some kind of looping approach...
  // we know for sure we have to draw the cell at 1,1.
  // to find cell(1,1)'s dim_x, we have to look down the column and size each cell.
  //   something like an array_t<f32>, storing the col_w to use if we draw down to that row idx ?
  // to find cell(1,1)'s dim_y, we have to look down the row and size each cell.
  //   similarly, array_t<f32> storing the row_h to use if we draw down to to that col idx ?
  // to figure out how many rows we'll actually draw from 1,1;
  //   we have to scan each row to find it's row_h
  //   but to do that, we need to know how many cols to scan.
  // by symmetry, we have the same situation with the transpose, meaning we've got a circular dependency.
  // how do we break that cycle?
  // maybe by flipping the starting point.
  // if we start at the bottom right cell, the rendering dimensions are fully determined.
  // considering the bottom right cell to be at m,n;
  // col_w[m] = max of cell_w[i] for i=1 to m
  // row_h[m] = max of cell_h[j] for j=1 to n
  // given m,n; we can compute the full dimensions required for all m*n cells.
  // so we need some kind of search procedure for finding the right m,n that will fit our given bounds.
  // we already have a way to make the conservative guess, so that's an upper bound.
  // note that because of the cycle behavior, changing one of m or n can result in the other dimension changing.
  // so, we almost certainly have to test singular changes to m,n.
  // probable algorithm:
  //   m,n = conservative values.
  //   Forever {
  //     if( m <= 1 ) break;
  //     ComputeDims( m - 1, n, &dimf );
  //     if( dimf.x < bounds.x ) {
  //       // making m any smaller will cause us to show blank stuff.
  //       break;
  //     }
  //     m -= 1;
  //   }
  //   Forever {
  //     if( n <= 1 ) break;
  //     ComputeDims( m, n - 1, &dimf );
  //     if( dimf.y < bounds.y ) {
  //       // making n any smaller will cause us to show blank stuff.
  //       break;
  //     }
  //     n -= 1;
  //   }
  // how should we choose which m,n possibilities to test, and in what order?
  // do we need to do the above algorithm, but infinite-loop both until m,n stabilizes?
  // i can think of situations where decrementing m will cause n to increase, and vise-versa.
  // if we do a m,n stabilization loop, what's to stop infinite-looping from happening?
  // put another way, are we sure there's only one m,n that works?
  // we also have yet to consider the row headers' variable width, which complicates things further.
  // maybe we just do the m decrements as much as possible once, and then do the same for n.
  // i.e. don't do any extra stabilization looping.
  // to ask this question another way, we have the whole space of potential bottom right cells up to m,n;
  // some subset of those will be invalid, with a computed dim less than the bounds we're given.
  // if you imagine a 2d grid with invalid m,n choices marked as white, and valid choices black;
  // i can imagine situations where if m,n is valid, 1,n+1 is also valid. The 1,n+1 cell has a huge width.
  // same for m+1,1 being valid, if the m+1,1 cell has a huge height.
  // since our search space is bigger than we want, we need to introduce another metric to choose.
  // one good idea i had is to maximize the total number of visible cells.
  // that would make those m+1,1 and 1,n+1 cases less favorable, which seems like what we want.
  // thus the metric to maximize is just: C = m*n
  // note there's still ambiguity, e.g. with 3,4 vs. 4,3.
  // so maybe we need a more detailed metric, like a percentage of visible cells. this still has ambiguity.
  // maybe we just prefer the largest n across choices with an equal C. this doesn't have ambiguity, which is good.
  // so something like:
  //   m,n = conservative values.
  //   best_m, best_n, best_C
  //   For i,j from 1,1 to m,n:
  //     Cij = ComputeMetric( grid, i, j, bounds )
  //     if( Cij > best_C  ||  ( Cij == best_C  &&  j > best_n ) ) {
  //       best_m = i;
  //       best_n = j;
  //       best_C = Cij;
  //     }
  // with some extra handling for the empty-loop case, and for invalid cases where the dim is too small.
  // this C = m*n metric doesn't quite work, since we'll just always pick the largest diagonal m,n.
  // and then overdraw, which we're trying to avoid.
  // so we need some factor to reduce overdraw, or just do this C metric choice among _only_ the i,j
  // which don't overdraw at all.
  // i guess we can build that into the validity of the choice; if there's overdraw or underdraw, then
  // treat that i,j as invalid, and don't consider it.
  // something like:
  //   m,n = conservative values.
  //   best_m, best_n, best_C
  //   For i,j from 1,1 to m,n:
  //     Cij
  //     valid
  //     ComputeMetric( grid, i, j, bounds, &Cij, &valid )
  //     if( valid  &&  ( Cij > best_C  ||  ( Cij == best_C  &&  j > best_n ) ) ) {
  //       best_m = i;
  //       best_n = j;
  //       best_C = Cij;
  //     }
  // and have ComputeMetric only set validity when there's no overdraw/underdraw.
  // that seems like a reasonable algorithm to me!
  // so how do we now handle the row header variable-width?
  // well each choice of m,n has a given row maximum, which we use as the row header width.
  // so we can just bake that calculation into ComputeMetric, and the dim validity checking.
  // so the dim validity check is basically: does the m,n cell span the given bounds.p1 ?
  
  
#if 0 // BUGS:

  ctrl+click+drag for one rect. release.
  ctrl+click+drag again to add a rect.
    we throw away the first rect.

  resize the window
    we change the cursel, which we shouldn't.

  hold down shift, then arrow_d, arrow_u.
    we keep the second cell selected when we shouldn't.

  we inc grid->eval_generation for every cell in a rectlist in InsertOrSetCellContents.
    shouldn't we do it once at the start, set the cell inputs, and then evaluate all at once?

  we have callers of InsertOrSetCellContents that call it in a loop, which is bad.

  arrow_d or page_d off the screen, both horz+vert should cause scrolling.

  alt+mousedrag after a press over the selection should move the selected cells.

  mousedrag of row/col boundaries should set manual dimensions.
    we should be able to handle rendering them, but we don't have a way to make any yet.

  scrollbars.
    how do we handle infinite scrolling? make the current scrollpos halfway down the bar on mouse-up?

  cut copy paste

  undo redo

  save to CSV
    how do we handle commas in strings? turn them into semicolons?

  custom file format + save/load code for our cell inputs.
    simple CSV with escaped commas allowed?
      doesn't handle sparse data well.
    binary dump of cell input strings, and structured 2d list of slices into the dump?
      could handle sparse data well with some 2d list smarts.
    binary dump of cellblocks, with the aux. string dump?

  disallow arrays within cells?
    they don't really make sense to display.
    probably just allow arrays as an evaluation-time thing.
    this means splitting out cellvalue_t into cellvalue_t and evalvalue_t
    i added graphs within cells, which are an array of arrays.

#endif

