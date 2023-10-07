// build:window_x64_debug
// build:window_x64_optimized
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
#include "ds_hashset_complexkey.h"
#include "ds_hashset_nonzeroptrs.h"
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
#include "sparse2d_compressedsparserow.h"

#define OPENGL_INSTEAD_OF_SOFTWARE       0
#define GLW_RAWINPUT_KEYBOARD            0
#define GLW_RAWINPUT_KEYBOARD_NOHOTKEY   1
#include "glw.h"

#define USE_FILEMAPPED_OPEN 0 // TODO: unfinished
#define USE_SIMPLE_SCROLLING 0 // TODO: questionable decision to try to simplify scrolling datastructs, getting rid of smooth scrolling.

#include "ui_propdb.h"
#include "ui_font.h"
#include "ui_render.h"
#include "statistics.h"

// Test args:
//   "C:\Users\admin\Desktop\aapl_5y.csv" "C:\Users\admin\Desktop\msft_5y.csv"

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
    Ideally we'd:
      auto-discover useful data
      auto-join time series data for subsequent analysis ops

#endif

struct
csv_t
{
  string_t csv; // file contents
  idx_t ncolumns;
  u32 nrows;
  tstring_t<slice_t> alloc_cells; // the csv cells in column-major order in one block allocation (excluding headers)
  tstring_t<slice_t> headers; // headers per-column.
  tstring_t<tslice_t<slice_t>> columns; // 2-D indexed column-major csv cells.
};

Inl void
Kill( csv_t* table )
{
  Free( table->alloc_cells );
  Free( table->columns );
  Free( table->headers );
  Free( table->csv );
}

int
LoadCsv( slice_t path, csv_t* table )
{
  if( path.len  &&  path.mem[0] == '\"' ) {
    if( path.mem[ path.len - 1 ] != '\"' ) {
      auto mem = AllocCstr( path );
      printf( "Bad double-quoted file path, missing ending double-quote: %s\n", mem );
      MemHeapFree( mem );
      return 1;
    }
    path.mem += 1;
    path.len -= 1;
  }
  auto file = FileOpen( ML( path ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
  if( !file.loaded ) {
    auto mem = AllocCstr( path );
    printf( "Failed to load file: %s\n", mem );
    MemHeapFree( mem );
    return 1;
  }

  auto csv = FileAlloc( file );
  FileFree( file );

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

  Free( entries );
  Free( lines );

  table->csv = csv;
  table->ncolumns = ncolumns;
  table->nrows = nrows;
  table->alloc_cells = alloc_cells;
  table->headers = headers;
  table->columns = columns;

  return 0;
}

Inl
tslice_t<slice_t>*
ColumnByName( csv_t& table, slice_t name )
{
  For( x, 0, table.ncolumns ) {
    auto header = table.headers.mem[x];
    if( StringEquals( ML( name ), ML( header ), 0 ) ) {
      return table.columns.mem + x;
    }
  }
  return 0;
};

Inl tstring_t<f64>
DColDollarF64( tslice_t<slice_t> col )
{
  auto dcol = AllocString<f64>( col.len );
  For( y, 0, col.len ) {
    auto last_sale = col.mem[y];
    AssertCrash( last_sale.len );
    AssertCrash( last_sale.mem[0] == '$' );
    last_sale.mem += 1;
    last_sale.len -= 1;
    AssertCrash( last_sale.len );
    dcol.mem[y] = CsTo_f64( ML( last_sale ) );
  }
  return dcol;
};

Inl tstring_t<f64>
DColF64( tslice_t<slice_t> col )
{
  auto dcol = AllocString<f64>( col.len );
  For( y, 0, col.len ) {
    auto last_sale = col.mem[y];
    AssertCrash( last_sale.len );
    dcol.mem[y] = CsTo_f64( ML( last_sale ) );
  }
  return dcol;
};

struct
app_t
{
  glwclient_t client;
  bool fullscreen;
  stack_resizeable_cont_t<f32> stream;
  stack_resizeable_cont_t<font_t> fonts;

  tstring_t<csv_t> tables;
};
static app_t g_app = {};
int
AppInit( app_t* app, tslice_t<slice_t> args )
{
  GlwInit();

  Alloc( app->stream, 65536 );
  Alloc( app->fonts, 16 );
  app->fullscreen = 0;

  GlwInitWindow(
    app->client,
    Str( "graph" ), 5,
    0, _vec2<u32>( 800, 800 )
    );

  if( !args.len ) {
    printf( "Expected one or more arguments: path to a .csv file\n" );
    return 1;
  }

  app->tables = AllocString<csv_t>( args.len );
  ForLen( args_idx, args ) {
    auto path = args.mem[ args_idx ];
    auto table = app->tables.mem + args_idx;
    int r = LoadCsv( path, table );
    if( r ) return r;
  }

  return 0;
}
void
AppKill( app_t* app )
{
  ForLen( i, app->fonts ) {
    auto& font = app->fonts.mem[i];
    FontKill( font );
  }
  Free( app->fonts );
  Free( app->stream );
  GlwKillWindow( app->client );
  GlwKill();

  ForLen( t, app->tables ) {
    Kill( app->tables.mem + t );
  }
  Free( app->tables );
}

Enumc( fontid_t )
{
  normal,
};
Inl void
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

Enumc( applayer_t )
{
  edit,
  bkgd,
  txt,
  COUNT
};


__OnRender( AppOnRender )
{
  ProfFunc();

  auto app = Cast( app_t*, misc );
  auto& font = GetFont( app, Cast( enum_t, fontid_t::normal ) );
  auto line_h = FontLineH( font );
  auto zrange = _vec2<f32>( 0, 1 );

  auto spaces_per_tab = GetPropFromDb( u8, u8_spaces_per_tab );
  auto rgba_notify_text = GetPropFromDb( vec4<f32>, rgba_notify_text );

  auto timestep = MIN( timestep_realtime, timestep_fixed );

  { // display timestep_realtime, as a way of tracking how long rendering takes.
    stack_nonresizeable_stack_t<u8, 64> tmp;
    CsFrom_f64( tmp.mem, Capacity( tmp ), &tmp.len, 1000 * timestep_realtime );
    auto tmp_w = LayoutString( font, spaces_per_tab, ML( tmp ) );
    DrawString(
      app->stream,
      font,
      AlignRight( bounds, tmp_w ),
      GetZ( zrange, applayer_t::txt ),
      bounds,
      rgba_notify_text,
      spaces_per_tab,
      ML( tmp )
      );

    bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );
  }

  target_valid = 0;

  {
    rng_xorshift32_t rng_xorsh;
    Init( rng_xorsh, TimeTSC() );

    // RESEARCH: consider metric: spread = high - low
    //   This could have interesting relationships, e.g. trading volume.

    auto& table = app->tables.mem[0];

    auto col_date = ColumnByName( table, SliceFromCStr( "date" ) );
    auto col_close = ColumnByName( table, SliceFromCStr( "close/last" ) );
    auto col_high = ColumnByName( table, SliceFromCStr( "high" ) );
    auto col_low = ColumnByName( table, SliceFromCStr( "low" ) );
    auto col_volume = ColumnByName( table, SliceFromCStr( "volume" ) );

    auto dcol_close = DColDollarF64( *col_close );
    auto dcol_high = DColDollarF64( *col_high );
    auto dcol_low = DColDollarF64( *col_low );
    auto dcol_volume = DColF64( *col_volume );

    // TODO: sort by date rather than just assume reverse.
    //   either:
    //     1. list of columns, and iterate it to swap during the sort alg.
    //     2. generate an index permutation column, and then apply it to all columns.
    //   i'm thinking option 2. It's more memory intensive, but has simpler inner loops.
    TReverse( ML( dcol_close ) );
    TReverse( ML( dcol_high ) );
    TReverse( ML( dcol_low ) );
    TReverse( ML( dcol_volume ) );

    AssertCrash( dcol_high.len == dcol_low.len ); // TODO: domain join
    auto dcol_spread = AllocString<f64>( dcol_high.len );
    ForLen( i, dcol_spread ) {
      auto spread = dcol_high.mem[i] - dcol_low.mem[i];
      AssertCrash( spread >= 0 );
      dcol_spread.mem[i] = spread;
    }

    // TODO: correct fourier transform
    auto dcol_fourier = AllocString<f64>( 2u * RoundUpToNextPowerOf2( dcol_close.len ) );
    TZero( ML( dcol_fourier ) );
    ForLen( i, dcol_close ) {
      dcol_fourier.mem[2*i+0] = dcol_close.mem[i];
    }
    FOUR1<f64>( dcol_fourier.mem, dcol_fourier.len / 2, -1 );
    dcol_fourier.len = 2u * dcol_close.len;

    // SCRATCH FORMULAIC COLUMN:
//    auto dcol_spread_over_volume = AllocString<f64>( dcol_fourier.len );
//    ForLen( i, dcol_spread_over_volume ) {
//      dcol_spread_over_volume.mem[i] = dcol_fourier.mem[i];
//    }
    auto dcol_spread_over_volume = AllocString<f64>( dcol_spread.len );
    ForLen( i, dcol_spread_over_volume ) {
//      dcol_spread_over_volume.mem[i] = dcol_spread.mem[i] * ( dcol_volume.mem[i] * 1e-9f );
//      dcol_spread_over_volume.mem[i] = dcol_volume.mem[i] / ( 1.0f + dcol_spread.mem[i] );
//      dcol_spread_over_volume.mem[i] = Ln64( dcol_volume.mem[i] / ( 1.0f + dcol_spread.mem[i] ) );
//      dcol_spread_over_volume.mem[i] = Ln64( dcol_spread.mem[i] / dcol_volume.mem[i] );
//      dcol_spread_over_volume.mem[i] = Ln64( dcol_spread.mem[i] / dcol_high.mem[i] );
//      dcol_spread_over_volume.mem[i] = Ln64( dcol_high.mem[i] );
//      dcol_spread_over_volume.mem[i] = Ln64( dcol_high.mem[i] / dcol_volume.mem[i] );
      dcol_spread_over_volume.mem[i] = Ln64( dcol_high.mem[i] * ( dcol_volume.mem[i] * 1e-9f ) );
    }

    auto mean_close = Mean<f64>( dcol_close );
    f64 min_close;
    f64 max_close;
    MinMax<f64>( dcol_close, &min_close, &max_close );
    auto mean_high = Mean<f64>( dcol_high );
    f64 min_high;
    f64 max_high;
    MinMax<f64>( dcol_high, &min_high, &max_high );
    auto mean_low = Mean<f64>( dcol_low );
    f64 min_low;
    f64 max_low;
    MinMax<f64>( dcol_low, &min_low, &max_low );
    auto mean_spread = Mean<f64>( dcol_spread );
    f64 min_spread;
    f64 max_spread;
    MinMax<f64>( dcol_spread, &min_spread, &max_spread );
    auto mean_volume = Mean<f64>( dcol_volume );
    f64 min_volume;
    f64 max_volume;
    MinMax<f64>( dcol_volume, &min_volume, &max_volume );
    auto mean_spread_over_volume = Mean<f64>( dcol_spread_over_volume );
    f64 min_spread_over_volume;
    f64 max_spread_over_volume;
    MinMax<f64>( dcol_spread_over_volume, &min_spread_over_volume, &max_spread_over_volume );

    auto variance_close = Variance<f64>( dcol_close, mean_close );
    auto varianc2_close = Variance2<f64>( dcol_close, mean_close );
    auto variance_high = Variance<f64>( dcol_high, mean_high );
    auto variance_low = Variance<f64>( dcol_low, mean_low );
    auto variance_spread = Variance<f64>( dcol_spread, mean_spread );
    auto variance_volume = Variance<f64>( dcol_volume, mean_volume );
    auto variance_spread_over_volume = Variance<f64>( dcol_spread_over_volume, mean_spread_over_volume );

    #define DRAWQUAD( _color, _p0, _p1 ) \
      RenderQuad( \
        app->stream, \
        _color, \
        _p0, \
        _p1, \
        bounds, \
        GetZ( zrange, applayer_t::txt ) \
        );

    auto dim = bounds.p1 - bounds.p0;
    constexpr auto pct_gap = 1 / 256.0f;
    auto border_radius = MAX( 3.0f, Truncate32( pct_gap * MIN( dim.x, dim.y ) ) );

    // TODO: auto-layout the graphs
    auto dim_00 = _vec2( Truncate32( 0.5f * dim.x ), Truncate32( 0.5f * dim.y ) );

    auto p0_00 = bounds.p0;
    auto p0_10 = bounds.p0 + _vec2( dim_00.x, 0.0f );
    auto p0_01 = bounds.p0 + _vec2( 0.0f, dim_00.y );
    auto p0_11 = bounds.p0 + dim_00;

    auto rect_00 = _rect( p0_00, p0_00 + dim_00 );
    auto rect_10 = _rect( p0_10, p0_10 + dim_00 );
    auto rect_01 = _rect( p0_01, p0_01 + dim_00 );
    auto rect_11 = _rect( p0_11, p0_11 + dim_00 );
    rectf32_t* rects[] = { &rect_00, &rect_10, &rect_01, &rect_11 };
    For( r, 0, _countof( rects ) ) {
      auto rect = *rects[r];
      AssertCrash( border_radius >= 1.0f );
      ShrinkRect( rect, border_radius - 1.0f, &rect, 0 );
      rectf32_t within;
      rectf32_t borders[4];
      ShrinkRect( rect, 1.0f, &within, borders );
      auto bordercolor = _vec4( 0.4f, 0.4f, 0.4f, 1.0f );
      For( i, 0, _countof( borders ) ) {
        auto border = borders + i;
        DRAWQUAD( bordercolor, border->p0, border->p1 );
      }
      *rects[r] = within;
    }
    dim_00 = rect_00.p1 - rect_00.p0;

    // SEQUENCE SETUP:
    auto sequence_data = dcol_spread_over_volume;
    auto sequence_data_min = min_spread_over_volume;
    auto sequence_data_max = max_spread_over_volume;
    // ======
    auto points_sequence = AllocString<vec2<f32>>( sequence_data.len );
    PlotRunSequence<f64>( dim_00, sequence_data, sequence_data_min, sequence_data_max, points_sequence );

    // LAG SETUP:
    auto lag_data = dcol_spread_over_volume;
    // ======
    auto lag = Lag<f64>( lag_data );
    auto points_lag = AllocString<vec2<f32>>( lag.len );
    PlotXY<f64>( dim_00, { lag.y, lag.len }, lag.min_y, lag.max_y, { lag.x, lag.len }, lag.min_x, lag.max_x, points_lag );

    // AUTO-CORRELATION SETUP:
    auto autocorr_data = dcol_spread_over_volume;
    auto autocorr_data_mean = mean_spread_over_volume;
    auto autocorr_data_variance = variance_spread_over_volume;
    // ======
    auto autocorr = AllocString<f64>( autocorr_data.len );
    AutoCorrelation<f64>( autocorr_data, autocorr_data_mean, autocorr_data_variance, autocorr );
    auto points_autocorr = AllocString<vec2<f32>>( autocorr.len );
    // We use [-1, 1] as the range, since it's guaranteed to be within [-1,1].
    PlotRunSequence<f64>( dim_00, autocorr, -1 /*min_autocorr*/, 1 /*max_autocorr*/, points_autocorr );

    // HISTOGRAM SETUP:
    auto histogram_data = dcol_spread_over_volume;
    auto histogram_data_variance = variance_spread_over_volume;
    auto histogram_data_min = min_spread_over_volume;
    auto histogram_data_max = max_spread_over_volume;
    // ======
    // Scott's rule for choosing histogram bin width is:
    //   B = 3.49 * stddev * N^(-1/3)
    auto histogram_nbins = 3.49 * Sqrt( histogram_data_variance ) * Pow64( Cast( f64, histogram_data.len ), -1.0 / 3.0 );
    if( histogram_nbins < 3 ) {
      histogram_nbins = Sqrt64( Cast( f64, histogram_data.len ) );
    }
    if( histogram_nbins < 3 ) {
      histogram_nbins = 3;
    }
    if( histogram_nbins > 0.499f * dim_00.x ) {
      histogram_nbins = 0.499f * dim_00.x;
    }
    auto histogram_counts = AllocString<f64>( Cast( idx_t, histogram_nbins ) + 1 );
    auto bucket_from_close_idx = AllocString<idx_t>( histogram_data.len );
    auto counts_when_inserted = AllocString<f64>( histogram_data.len );
    Histogram<f64>( histogram_data, histogram_data_min, histogram_data_max, histogram_counts, bucket_from_close_idx, counts_when_inserted );
    f64 histogram_counts_min;
    f64 histogram_counts_max;
    MinMax<f64>( histogram_counts, &histogram_counts_min, &histogram_counts_max );
    auto histogram_rects = AllocString<rectf32_t>( histogram_data.len );
    PlotHistogram<f64>( dim_00, histogram_counts, histogram_counts_max, bucket_from_close_idx, counts_when_inserted, histogram_rects );


    // 2-COLUMN ANALYSIS:

    // LAG-CORRELATION SETUP:
    auto lagcorr_a = dcol_spread;
    auto lagcorr_a_mean = mean_spread;
    auto lagcorr_a_variance = variance_spread;
    auto lagcorr_b = dcol_volume;
    auto lagcorr_b_mean = mean_volume;
    auto lagcorr_b_variance = variance_volume;
    // ======
    AssertCrash( lagcorr_a.len == lagcorr_b.len ); // FUTURE: domain-join?
    auto lagcorr = AllocString<f64>( lagcorr_a.len );
    LagCorrelation<f64>( lagcorr_a, lagcorr_b, lagcorr_a_mean, lagcorr_a_variance, lagcorr_b_mean, lagcorr_b_variance, lagcorr );
    auto points_lagcorr = AllocString<vec2<f32>>( lagcorr.len );
    // We use [-1, 1] as the range, since it's guaranteed to be within [-1,1].
    PlotRunSequence<f64>( dim_00, lagcorr, -1 /*min_lagcorr*/, 1 /*max_lagcorr*/, points_lagcorr );

    // CORRELATION PLOT SETUP:
    auto corr_a = dcol_spread;
    auto corr_a_min = min_spread;
    auto corr_a_max = max_spread;
    auto corr_b = dcol_volume;
    auto corr_b_min = min_volume;
    auto corr_b_max = max_volume;
    // ======
    AssertCrash( corr_a.len == corr_b.len ); // FUTURE: domain-join?
    auto corr = AllocString<f64>( corr_a.len );
    auto points_corr = AllocString<vec2<f32>>( corr.len );
    PlotXY<f64>(
      dim_00,
      corr_a,
      corr_a_min,
      corr_a_max,
      corr_b,
      corr_b_min,
      corr_b_max,
      points_corr
      );


    auto DrawPoints = [](
      app_t* app,
      vec2<f32> zrange,
      rectf32_t bounds,
      vec2<f32> p0,
      vec2<f32> dim,
      tslice_t<vec2<f32>> points,
      bool gradient = 1,
      f32 radius = 0.0f
      )
    {
      auto pointcolor_base = _vec4( 1.0f );
      auto pointcolor_start = _vec4( 1.0f, 0.2f, 0.2f, 0.7f );
      auto pointcolor_end = _vec4( 0.2f, 1.0f, 0.2f, 0.7f );
      ForLen( i, points ) {
        auto pointcolor = gradient ?
          Lerp_from_idx( pointcolor_start, pointcolor_end, i, 0, points.len - 1 )
          :
          _vec4( 1.0f );
        auto point = points.mem[i];
        auto xi = point.x;
        auto yi = point.y;
        RenderQuad(
          app->stream,
          pointcolor,
          p0 + _vec2<f32>( xi, dim.y - ( yi + 1 ) ) - _vec2( radius ),
          p0 + _vec2<f32>( xi + 1, dim.y - yi ) + _vec2( radius ),
          bounds,
          GetZ( zrange, applayer_t::txt )
          );
      }
    };

    auto DrawColumns = [](
      app_t* app,
      vec2<f32> zrange,
      rectf32_t bounds,
      vec2<f32> p0,
      vec2<f32> dim,
      tslice_t<vec2<f32>> points
      )
    {
      auto pointcolor_base = _vec4( 1.0f );
      auto pointcolor_start = _vec4( 1.0f, 0.2f, 0.2f, 0.7f );
      auto pointcolor_end = _vec4( 0.2f, 1.0f, 0.2f, 0.7f );
      auto subdivision_w = Truncate32( dim.x / points.len );
      auto col_w = subdivision_w - 1;
      if( col_w < 1.0f ) return;
      ForLen( i, points ) {
        auto pointcolor = Lerp_from_idx( pointcolor_start, pointcolor_end, i, 0, points.len - 1 );
        auto point = points.mem[i];
        auto xi = i * subdivision_w;
        auto yi = point.y;
        RenderQuad(
          app->stream,
          pointcolor,
          p0 + _vec2<f32>( xi, dim.y - 1 - yi ),
          p0 + _vec2<f32>( xi + col_w, dim.y - 1 ),
          bounds,
          GetZ( zrange, applayer_t::txt )
          );
      }
    };

    auto DrawRects = [](
      app_t* app,
      vec2<f32> zrange,
      rectf32_t bounds,
      vec2<f32> p0,
      tslice_t<rectf32_t> rects
      )
    {
      auto pointcolor_base = _vec4( 1.0f );
      auto pointcolor_start = _vec4( 1.0f, 0.2f, 0.2f, 1.0f );
      auto pointcolor_end = _vec4( 0.2f, 1.0f, 0.2f, 1.0f );
      ForLen( i, rects ) {
        auto pointcolor = Lerp_from_idx( pointcolor_start, pointcolor_end, i, 0, rects.len - 1 );
        auto rect = rects.mem[i];
        RenderQuad(
          app->stream,
          pointcolor,
          p0 + rect.p0,
          p0 + rect.p1,
          bounds,
          GetZ( zrange, applayer_t::txt )
          );
      }
    };

    DrawPoints(
      app,
      zrange,
      bounds,
      rect_00.p0,
      dim_00,
      points_sequence,
      0 /*gradient*/ );

    DrawPoints(
      app,
      zrange,
      bounds,
      rect_10.p0,
      dim_00,
      points_lag,
      1, /*gradient*/
      1.0f /*radius*/ );

    DrawRects(
      app,
      zrange,
      bounds,
      rect_01.p0,
      histogram_rects );

    {
      // TODO: one px line width if total dim is odd, otherwise two px when even.
      //   this accurately represents the 0 line.
      //   or maybe just always one px.
      //   I need to do more detailed round-off error analysis of our drawing strategies.
      auto linecolor = _vec4( 0.5f );
      RenderQuad(
        app->stream,
        linecolor,
        rect_11.p0 + _vec2<f32>( 0, 0.5f * dim_00.y - 1 ),
        rect_11.p0 + _vec2<f32>( dim_00.x - 1, 0.5f * dim_00.y + 1 ),
        bounds,
        GetZ( zrange, applayer_t::txt )
        );

      auto points = points_autocorr;
      // TODO: stay with autocorr here, and move lagcorr + corr to 2-column analysis.
//      auto points = points_lagcorr;
//      auto points = points_corr;
      DrawPoints(
        app,
        zrange,
        bounds,
        rect_11.p0,
        dim_00,
        points,
        0 /*gradient*/ );
    }

    Free( corr );
    Free( points_corr );
    Free( dcol_fourier );
    Free( dcol_spread );
    Free( dcol_spread_over_volume );
    Free( dcol_low );
    Free( dcol_volume );
    Free( dcol_close );
    Free( dcol_high );
    Free( histogram_counts );
    Free( bucket_from_close_idx );
    Free( counts_when_inserted );
    Free( autocorr );
    Free( points_lagcorr );
    Free( lagcorr );
    Free( points_sequence );
    Free( histogram_rects );
    Free( points_autocorr );
    Free( points_lag );
  }

  if( app->stream.len ) {

    auto pos = app->stream.mem;
    auto end = app->stream.mem + app->stream.len;
    AssertCrash( app->stream.len % 10 == 0 );
    while( pos < end ) {
      Prof( tmp_UnpackStream );
      vec2<u32> p0;
      vec2<u32> p1;
      vec2<u32> tc0;
      vec2<u32> tc1;
      p0.x  = Round_u32_from_f32( *pos++ );
      p0.y  = Round_u32_from_f32( *pos++ );
      p1.x  = Round_u32_from_f32( *pos++ );
      p1.y  = Round_u32_from_f32( *pos++ );
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

      AssertCrash( p0.x <= p1.x );
      AssertCrash( p0.y <= p1.y );
      auto dstdim = p1 - p0;
      AssertCrash( tc0.x <= tc1.x );
      AssertCrash( tc0.y <= tc1.y );
      auto srcdim = tc1 - tc0;
      AssertCrash( p0.x + srcdim.x <= app->client.dim.x );
      AssertCrash( p0.y + srcdim.y <= app->client.dim.y );
      AssertCrash( p0.x + dstdim.x <= app->client.dim.x ); // should have clipped by now.
      AssertCrash( p0.y + dstdim.y <= app->client.dim.y );
      AssertCrash( tc0.x + srcdim.x <= font.tex_dim.x );
      AssertCrash( tc0.y + srcdim.y <= font.tex_dim.y );
      ProfClose( tmp_UnpackStream );

      bool subpixel_quad =
        ( p0.x == p1.x )  ||
        ( p0.y == p1.y );

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
            auto dst = app->client.fullscreen_bitmap_argb + ( p0.y + j ) * app->client.dim.x + ( p0.x + 0 );
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
            auto dst = app->client.fullscreen_bitmap_argb + ( p0.y + j ) * app->client.dim.x + ( p0.x + 0 );
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
          auto dst = app->client.fullscreen_bitmap_argb + ( p0.y + j ) * app->client.dim.x + ( p0.x + 0 );
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

#define __AppCmd( name )   void ( name )( app_t* app )
typedef __AppCmd( *pfn_appcmd_t );


struct
app_cmdmap_t
{
  glwkeybind_t keybind;
  pfn_appcmd_t fn;
};

Inl app_cmdmap_t
_appcmdmap(
  glwkeybind_t keybind,
  pfn_appcmd_t fn
  )
{
  app_cmdmap_t r;
  r.keybind = keybind;
  r.fn = fn;
  return r;
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

//      app_cmdmap_t table[] = {
//      };
//      ForEach( entry, table ) {
//        if( GlwKeybind( key, entry.keybind ) ) {
//          entry.fn( app );
//          target_valid = 0;
//          ran_cmd = 1;
//        }
//      }

      if( GlwKeybind( key, GetPropFromDb( glwkeybind_t, keybind_app_quit ) ) ) {
        GlwEarlyKill( app->client );
        target_valid = 0;
        ran_cmd = 1;
      }
    } break;

    case glwkeyevent_t::up: {
      switch( key ) {
#if 0
        case glwkey_t::esc: {
          GlwEarlyKill( app->client );
          ran_cmd = 1;
        } break;
#endif

        case glwkey_t::fn_11: {
          app->fullscreen = !app->fullscreen;
          fullscreen = app->fullscreen;
          ran_cmd = 1;
        } break;

#if PROF_ENABLED
        case glwkey_t::fn_9: {
          LogUI( "Started profiling." );
          ProfEnable();
          ran_cmd = 1;
        } break;

        case glwkey_t::fn_10: {
          ProfDisable();
          LogUI( "Stopped profiling." );
          ran_cmd = 1;
        } break;
#endif

      }
    } break;

    default: UnreachableCrash();
  }
  if( !ran_cmd ) {
  }
}



__OnMouseEvent( AppOnMouseEvent )
{
  ProfFunc();

  auto app = Cast( app_t*, misc );

  auto& font = GetFont( app, Cast( enum_t, fontid_t::normal ) );

  auto scroll_nlines = GetPropFromDb( u8, u8_scroll_nlines );
  auto scroll_sign = GetPropFromDb( s8, s8_scroll_sign );

  cursortype = glwcursortype_t::arrow;


  auto keymods = GlwKeyModifiersDown();
  bool mod_isdn = AnyDown( keymods );

  switch( type ) {

    case glwmouseevent_t::wheelmove: {
//      if( mod_isdn ) {
////        dwheel *= Cast( s32, app->factor );
//      }
//      if( dwheel ) {
//        if( dwheel < 0 ) {
//          auto delta = Cast( idx_t, -dwheel );
//          app->m = CLAMP( MAX( app->m, delta ) - delta, 1, 100000 );
//        } else {
//          app->m = CLAMP( app->m + Cast( idx_t, dwheel ), 1, 100000 );
//        }
//        target_valid = 0;
//      }
    } break;

    case glwmouseevent_t::dn: {

      switch( btn ) {
        case glwmousebtn_t::l: {
          target_valid = 0;
        } break;

        case glwmousebtn_t::r:
        case glwmousebtn_t::m:
        case glwmousebtn_t::b4:
        case glwmousebtn_t::b5: {
        } break;

        default: UnreachableCrash();
      }
    } break;

    case glwmouseevent_t::up: {
    } break;

    case glwmouseevent_t::move: {
      //printf( "move ( %d, %d )\n", m.x, m.y );
    } break;

    default: UnreachableCrash();
  }
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
Main( tslice_t<slice_t> args )
{
  PinThreadToOneCore();

  auto app = &g_app;
  AppInit( app, args );

  if( args.len ) {
    auto arg = args.mem + 0;
  }

  auto fontsize_normal = GetPropFromDb( f32, f32_fontsize_normal );
  auto filename_font_normal = GetPropFromDb( slice_t, string_filename_font_normal );

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
  while( a_len  &&  AsciiIsWhitespace( a[0] ) ) {
    a += 1;
    a_len -= 1;
  }
  // TODO: prog_cmd_line actually has two 0-terms!
  while( a_len  &&  ( !a[a_len - 1]  ||  AsciiIsWhitespace( a[a_len - 1] ) ) ) {
    a_len -= 1;
  }
}

int WINAPI
WinMain( HINSTANCE prog_inst, HINSTANCE prog_inst_prev, LPSTR prog_cmd_line, int prog_cmd_show )
{
  MainInit();

  u8* cmdline = Str( prog_cmd_line );
  idx_t cmdline_len = CstrLength( Str( prog_cmd_line ) );

  stack_resizeable_cont_t<slice_t> args;
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
      auto end = StringScanR( arg, cmdline_len - 1, '"' );
      arg_len = !end  ?  0  :  end - arg;
      IgnoreSurroundingSpaces( arg, arg_len );
    } else {
      auto end = StringScanR( arg, cmdline_len - 1, ' ' );
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
  auto r = Main( SliceFromArray( args ) );
  Free( args );

  Log( "Main returned: %d", r );

  MainKill();
  return r;
}
