// build:console_x64_debug
// build:console_x64_optimized
// Copyright (c) John A. Carlos Jr., all rights reserved.

#include "os_mac.h"
#include "os_windows.h"

#define FINDLEAKS   0
#define WEAKINLINING   0
#include "core_cruntime.h"
#include "core_types.h"
#include "core_language_macros.h"
#include "asserts.h"
#include "memory_operations.h"
#include "math_integer.h"
#include "math_float.h"
#include "math_lerp.h"
#include "math_floatvec.h"
#include "math_matrix.h"
#include "math_kahansummation.h"
#include "allocator_heap.h"
#include "allocator_virtual.h"
#include "allocator_heap_or_virtual.h"
#include "cstr.h"
#include "ds_slice.h"
#include "ds_string.h"
#include "allocator_pagelist.h"
#include "ds_stack_resizeable_cont.h"
#include "ds_stack_nonresizeable_stack.h"
#include "ds_stack_nonresizeable.h"
#include "ds_stack_resizeable_pagelist.h"
#include "ds_pagetree.h"
#include "ds_list.h"
#include "ds_stack_cstyle.h"
#include "ds_hashset_cstyle.h"
#include "filesys.h"
#include "cstr_integer.h"
#include "cstr_float.h"
#include "timedate.h"
#include "thread_atomics.h"
#include "ds_mtqueue_srsw_nonresizeable.h"
#include "ds_mtqueue_srmw_nonresizeable.h"
#include "ds_mtqueue_mrsw_nonresizeable.h"
#include "ds_mtqueue_mrmw_nonresizeable.h"
#include "threading.h"
#define LOGGER_ENABLED   0
#include "logger.h"
#define PROF_ENABLED   0
#define PROF_ENABLED_AT_LAUNCH   0
#include "profile.h"
#include "rand.h"
#include "allocator_heap_findleaks.h"
#include "mainthread.h"
#include "text_parsing.h"
#include "ds_stack_resizeable_cont_addbacks.h"
#include "statistics.h"

#if 0

  We want a 'table' concept: load csv table, and then be able to add new derived columns with some syntax.
    SQL is the table manipulation standard, so maybe we should just do that.
    Original csv files should be preserved, so we'll have companion column file(s).
    Design choice:
      One companion file per original csv makes things simple.
      One companion file per derived column would be fastest w.r.t. CRUD'ing derived columns.
    The second option sounds better.
    
  We want an 'analysis' concept: load in multiple tables, and start exploring statistics.
    Different workflows:
      1-column analysis
      2-column analysis, within 1 table
      2-column analysis, across 2 tables
        complexity around index-matching
    User analysis choices should be preserved, so I'm thinking one analysis file to store that.
      It will reference the original csv files, and the companion derived column files.
      
  
#endif


int
Main( stack_resizeable_cont_t<slice_t>& args )
{
  if( args.len != 1 ) {
    printf( "Expected one argument: path to a .csv file\n" );
    return 1;
  }

  auto path = args.mem[ 0 ];
  auto file = FileOpen( ML( path ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
  if( !file.loaded ) {
    auto mem = AllocCstr( path );
    printf( "Failed to load file: %s\n", mem );
    MemHeapFree( mem );
    return 1;
  }

  auto csv = FileAlloc( file );

  // parse the csv into columns.
  u32 nlines = CountNewlines( ML( csv ) ) + 1;
  stack_resizeable_cont_t<slice32_t> lines;
  Alloc( lines, nlines );
  SplitIntoLines( &lines, ML( csv ) );
  AssertCrash( lines.len );
  AssertCrash( nlines == lines.len );
  auto nrows = nlines - 1;
  auto line_headers = lines.mem[0];
  AssertCrash( line_headers.len ); // TODO: More robust whitespace parsing support, handling empty lines.
  auto ncolumns = CountCommas( ML( line_headers ) ) + 1; // TODO: double-quote parsing support.
  AssertCrash( ncolumns );
  // store the csv cells in column-major order in one block allocation.
  auto alloc_cells = AllocString<slice_t>( nrows * ncolumns );
  // we'll leave the headers and column-index as separate allocations for now, for convenience.
  auto headers = AllocString<slice_t>( ncolumns );
  stack_resizeable_cont_t<slice_t> entries;
  Alloc( entries, ncolumns );
  SplitByCommas( &entries, ML( line_headers ) );
  AssertCrash( ncolumns == entries.len );
  For( x, 0, ncolumns ) {
    headers.mem[x] = entries.mem[x];
  }
  auto columns = AllocString<tslice_t<slice_t>>( ncolumns );
  For( x, 0, ncolumns ) {
    // column-major striping of alloc_cells.
    columns.mem[x] = { alloc_cells.mem + x * nrows, nrows };
  }
  Fori( u32, y, 1, lines.len ) {
    auto line = lines.mem[y];
    AssertCrash( line.len ); // TODO: More robust whitespace parsing support, handling empty lines.
    auto nentries = CountCommas( ML( line ) ) + 1;
    if( nentries != ncolumns ) {
      printf(
        "Line number %u has different number of elements: %Iu than expected: %Iu: \n",
        y + 1,
        nentries,
        ncolumns );
      return 1;
    }
    entries.len = 0;
    Reserve( entries, nentries );
    SplitByCommas( &entries, ML( line ) );
    AssertCrash( nentries == entries.len );
    For( x, 0, nentries ) {
      auto column = columns.mem + x;
      column->mem[y - 1] = entries.mem[x];
    }
  }

  auto ColumnByName = [&](slice_t name) -> tslice_t<slice_t>*
  {
    For( x, 0, ncolumns ) {
      auto header = headers.mem[x];
      if( StringEquals( ML( name ), ML( header ), 0 ) ) {
        return columns.mem + x;
      }
    }
    return 0;
  };

  // RESEARCH: consider metric: spread = high - low
  //   This could have interesting relationships, e.g. trading volume.

  auto col_date = ColumnByName( SliceFromCStr( "date" ) );
  auto col_close = ColumnByName( SliceFromCStr( "close/last" ) );
  auto col_high = ColumnByName( SliceFromCStr( "high" ) );

  auto DColDollarF64 = [&](tslice_t<slice_t>* col)
  {
    auto dcol = AllocString<f64>( nrows );
    For( y, 0, nrows ) {
      auto last_sale = col->mem[y];
      AssertCrash( last_sale.len );
      AssertCrash( last_sale.mem[0] == '$' );
      last_sale.mem += 1;
      last_sale.len -= 1;
      AssertCrash( last_sale.len );
      dcol.mem[y] = CsTo_f64( ML( last_sale ) );
    }
    return dcol;
  };

  auto dcol_close = DColDollarF64( col_close );
  auto dcol_high = DColDollarF64( col_high );

  auto mean_close = Mean( SliceFromString( dcol_close ) );
  auto mean_high = Mean( SliceFromString( dcol_high ) );
  f64 min_close;
  f64 max_close;
  MinMax( SliceFromString( dcol_close ), &min_close, &max_close );
  f64 min_high;
  f64 max_high;
  MinMax( SliceFromString( dcol_high ), &min_high, &max_high );
  auto counts_close = AllocString<f64>( nrows );
  Histogram( SliceFromString( dcol_close ), min_close, max_close, SliceFromString( counts_close ) );
  auto counts_high = AllocString<f64>( nrows );
  Histogram( SliceFromString( dcol_high ), min_high, max_high, SliceFromString( counts_high ) );
  ForLen( i, counts_close ) {
    //printf( "%llu\n", counts_close.mem[i] );
  }

  auto variance_close = Variance( SliceFromString( dcol_close ), mean_close );
  auto varianc2_close = Variance2( SliceFromString( dcol_close ), mean_close );
  auto variance_high = Variance( SliceFromString( dcol_high ), mean_high );
  auto autocorr_close = AllocString<f64>( nrows );
  AutoCorrelation( SliceFromString( dcol_close ), mean_close, variance_close, SliceFromString( autocorr_close ) );
  auto autocorr_high = AllocString<f64>( nrows );
  AutoCorrelation( SliceFromString( dcol_high ), mean_high, variance_high, SliceFromString( autocorr_high ) );
  auto lagcorr_close_high = AllocString<f64>( nrows );
  LagCorrelation( SliceFromString( dcol_close ), SliceFromString( dcol_high ), mean_close, variance_close, mean_high, variance_high, SliceFromString( lagcorr_close_high ) );
  auto lag_close = Lag( SliceFromString( dcol_close ) );
  auto lag_high = Lag( SliceFromString( dcol_high ) );
  ForLen( i, lag_close ) {
    //printf( "%F,%F\n", lag_close.y[i], lag_close.x[i] );
  }

  // TODO: layout the graphs

  // FUTURE: render graphs of all the data above.
  // PERF: emit points/rects in pixel space, not an image.
  f64 min_autocorr_close;
  f64 max_autocorr_close;
  MinMax( SliceFromString( autocorr_close ), &min_autocorr_close, &max_autocorr_close );

  f64 min_counts_close;
  f64 max_counts_close;
  MinMax( SliceFromString( counts_close ), &min_counts_close, &max_counts_close );

  auto dim = _vec2<u32>( 80, 40 );
//  auto points = AllocString<vec2<u32>>( autocorr_close.len );
//  PlotRunSequence( dim, SliceFromString( autocorr_close ), min_autocorr_close, max_autocorr_close, SliceFromString( points ) );

//  auto points = AllocString<vec2<f64>>( counts_close.len );
//  PlotRunSequence( dim, SliceFromString( counts_close ), min_counts_close, max_counts_close, SliceFromString( points ) );

  auto points = AllocString<vec2<f64>>( lag_close.len );
  PlotLag( dim, lag_close, SliceFromString( points ) );

  auto pixels = AllocString<vec2<u32>>( points.len );
  PixelSnap( dim, SliceFromString( points ), SliceFromString( pixels ) );

  auto rgba = AllocString<u32>( dim.x * dim.y );
  TZero( ML( rgba ) );
  ForLen( i, pixels ) {
    auto pixel = pixels.mem[i];
    rgba.mem[pixel.x + dim.x * pixel.y] = 0xFFFFFFFFu;
  }
  ReverseFor( y, 0, dim.y ) {
    For( x, 0, dim.x ) {
      auto px = rgba.mem[x + dim.x * y];
      printf(px > 0 ? "+" : " ");
    }
    printf("\n");
  }

  Free( alloc_cells );
  Free( columns );
  Free( headers );
  Free( entries );
  Free( lines );

  Free( csv );
  FileFree( file );

  return 0;
}



#if 0
  auto col_last_sale = ColumnByName( SliceFromCStr( "last sale" ) );
  auto col_volume = ColumnByName( SliceFromCStr( "volume" ) );
  auto col_symbol = ColumnByName( SliceFromCStr( "symbol" ) );

  // TODO: f64 isn't so great for financial data like this...
  //   infinite precision integers is what we want.
  //   and we can use fixed point: x100, aka number of cents.
  auto dcol_last_sale = AllocString<f64>( nrows );
  auto dcol_volume = AllocString<u64>( nrows );
  For( y, 0, nrows ) {
    auto last_sale = col_last_sale->mem[y];
    AssertCrash( last_sale.len );
    AssertCrash( last_sale.mem[0] == '$' );
    last_sale.mem += 1;
    last_sale.len -= 1;
    AssertCrash( last_sale.len );
    dcol_last_sale.mem[y] = CsTo_f64( ML( last_sale ) );
  }
  For( y, 0, nrows ) {
    auto volume = col_volume->mem[y];
    AssertCrash( volume.len );
    dcol_volume.mem[y] = CsToIntegerU<u64>( ML( volume ) );
  }

  auto dcol_price_volume = AllocString<f64>( nrows );
  For( y, 0, nrows ) {
    dcol_price_volume.mem[y] = dcol_last_sale.mem[y] * dcol_volume.mem[y];
//    printf( "%F\n", dcol_price_volume.mem[y] );
  }

  auto icol = AllocString<idx_t>( nrows );
  For( y, 0, nrows ) {
    icol.mem[y] = y;
  }
  std::sort(
    icol.mem,
    icol.mem + icol.len,
    [&](const idx_t& a, const idx_t& b) -> bool
    {
      return dcol_price_volume.mem[a] < dcol_price_volume.mem[b];
    }
    );

  For( y, 0, nrows ) {
    auto yi = icol.mem[y];
    auto mem = AllocCstr( col_symbol->mem[yi] );
    printf( "%s\t\t%F\n", mem, dcol_price_volume.mem[yi] );
    MemHeapFree( mem );
  }

  Free( icol );
  Free( dcol_price_volume );
  Free( dcol_last_sale );
  Free( dcol_volume );
#endif


int
main( int argc, char** argv )
{
  MainInit();

  stack_resizeable_cont_t<slice_t> args;
  Alloc( args, argc - 1 );
  Fori( int, i, 1, argc ) {
    auto arg = AddBack( args );
    arg->mem = Cast( u8*, argv[i] );
    arg->len = CstrLength( arg->mem );
  }
  int r = Main( args );
  Free( args );

  // Do this before destroying any datastructures, so other threads stop trying to access things.
  SignalQuitAndWaitForTaskThreads();

//  system( "pause" );
  MainKill();
  return r;
}
