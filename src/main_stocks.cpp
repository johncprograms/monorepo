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

// Zeros and writes the histogram to 'counts'.
// The given counts.len is the number of buckets to use; they're uniformly sized.
// data_min/data_max define the bounds of the data. This assumes they're pre-computed,
// since min/max are likely to be useful statistics for other things.
Templ Inl void
Histogram(
  tslice_t<T> data,
  T data_min,
  T data_max,
  tslice_t<T> counts
  )
{
  TZero( ML( counts ) );
  ForLen( i, data ) {
    auto x = data.mem[i];
    auto bucket_fractional = lerp_T_from_T<T>( 0, Cast( T, counts.len ), x, data_min, data_max );
    auto bucket = Cast( idx_t, bucket_fractional );
    bucket = MIN( bucket, counts.len - 1 );
    AssertCrash( bucket <= counts.len );
    counts.mem[bucket] += 1;
  }
}
Templ Inl void
MinMax(
  tslice_t<T> data,
  T* data_min,
  T* data_max
  )
{
  AssertCrash( data.len );
  auto min = data.mem[0];
  auto max = min;
  For( i, 1, data.len ) {
    auto x = data.mem[i];
    min = MIN( min, x );
    max = MAX( max, x );
  }
  *data_min = min;
  *data_max = max;
}
Templ Inl T
Mean(
  tslice_t<T> data
  )
{
  AssertCrash( data.len );
  kahansum_t<T> mean = { data.mem[0] };
  For( i, 1, data.len ) {
    auto x = data.mem[i];
    auto delta_mean = ( x - mean.sum ) / i;
    Add( mean, delta_mean );
  }
  return mean.sum;
}
Templ Inl T
Variance(
  tslice_t<T> data,
  T data_mean
  )
{
  AssertCrash( data.len );
  kahansum_t<T> variance = {};
  auto scale = 1 / Cast( T, data.len );
  For( i, 0, data.len ) {
    auto x = data.mem[i];
    auto deviation = x - data_mean;
    // TODO: is it better to factor out the scale multiply? It keeps floats small though.
    //   Test Variance vs Variance2 below to find out.
    //   Maybe there's NIST gold standard data we can compare against.
    //   Also we might need super-high precision math to test in general.
    auto variance_i = scale * deviation * deviation;
    Add( variance, variance_i );
  }
  return variance.sum;
}
Templ Inl T
Variance2(
  tslice_t<T> data,
  T data_mean
  )
{
  AssertCrash( data.len );
  kahansum_t<T> variance = {};
  auto scale = 1 / Cast( T, data.len );
  For( i, 0, data.len ) {
    auto x = data.mem[i];
    auto deviation = x - data_mean;
    auto variance_i = deviation * deviation;
    Add( variance, variance_i );
  }
  return scale * variance.sum;
}

// For a lag plot, we want to plot f(x[i]) vs. f(x[i - lag])
// i.e. we're lining up two instances of f, at various lag offsets.
// Note we can only plot the overlap of the two instances.
// The overlap length is f.len - lag
// Hence this type stores the starts of the two instances, plus the overlap length.
Templ struct
lag_t
{
  T* y;
  T* x;
  idx_t len;
  T min_y;
  T min_x;
  T max_y;
  T max_x;
};
// PERF: N^2 if we call repeatedly to compute all possible lag_offset values.
Templ lag_t<T>
Lag(
  tslice_t<T> f,
  idx_t lag_offset = 1
  )
{
  AssertCrash( f.len >= lag_offset );
  lag_t<T> lag = {};
  lag.y = f.mem + lag_offset;
  lag.x = f.mem;
  lag.len = f.len - lag_offset;
  MinMax( { lag.y, lag.len }, &lag.min_y, &lag.max_y );
  MinMax( { lag.x, lag.len }, &lag.min_x, &lag.max_x );
  return lag;
}

Templ Inl void
AutoCorrelation(
  tslice_t<T> data,
  T data_mean,
  T data_variance,
  tslice_t<T> autocorrelation // length: data.len
  )
{
  AssertCrash( data.len );
  AssertCrash( autocorrelation.len == data.len );
  auto scale = 1 / Cast( T, data.len );
  autocorrelation.mem[0] = 1; // by definition.
  For( k, 1, data.len ) {
    kahansum_t<T> correlation_k = {};
    For( t, 0, data.len - k ) {
      auto x = data.mem[t];
      auto y = data.mem[t+k];
      auto correlation_k_i = scale * ( x - data_mean ) * ( y - data_mean );
      Add( correlation_k, correlation_k_i );
    }
    autocorrelation.mem[k] = correlation_k.sum / data_variance;
  }
}
// Note that the Pearson correlation coefficient is lagcorrelation[0], no lag.
Templ Inl void
LagCorrelation(
  tslice_t<T> f,
  tslice_t<T> g,
  T f_mean,
  T f_variance,
  T g_mean,
  T g_variance,
  tslice_t<T> lagcorrelation // length: data.len
  )
{
  AssertCrash( f.len );
  AssertCrash( g.len == f.len );
  AssertCrash( lagcorrelation.len == f.len );
  auto scale = 1 / Cast( T, f.len );
  auto scale_lagcorrelation = 1 / ( Sqrt( f_variance )  * Sqrt( g_variance ) );
  For( k, 0, f.len ) {
    kahansum_t<T> correlation_k = {};
    For( t, 0, f.len - k ) {
      auto f_t = f.mem[t];
      auto g_t_lag = g.mem[t+k];
      auto correlation_k_i = scale * ( f_t - f_mean ) * ( g_t_lag - g_mean );
      Add( correlation_k, correlation_k_i );
    }
    lagcorrelation.mem[k] = correlation_k.sum * scale_lagcorrelation;
  }
}
Templ Inl T
Covariance(
  tslice_t<T> f,
  tslice_t<T> g,
  T f_mean,
  T g_mean
  )
{
  AssertCrash( f.len );
  AssertCrash( g.len == f.len );
  kahansum_t<T> covariance = {};
  auto scale = 1 / Cast( T, f.len );
  For( i, 0, f.len ) {
    auto f_i = f.mem[i];
    auto g_i = g.mem[i];
    // TODO: is it better to factor out the scale multiply? It keeps floats small though.
    auto covariance_i = scale * ( f_i - f_mean ) * ( g_i - g_mean );
    Add( covariance, covariance_i );
  }
  return covariance.sum;
}

// In-place Fourier transforms of 'data', which stores complex numbers.
// Assumes NN is a power of 2. Use a 0-padded buffer to round up to that.
// When isign==1, the result is the discrete fourier transform.
// When isign==-1, the result is NN times the inverse discrete fourier transform.
Templ Inl void
FOUR1(
  T* data, // length 2*NN
  idx_t NN,
  T isign
  )
{
  AssertCrash( IsPowerOf2( NN ) );
  AssertCrash( isign == -1  ||  isign == 1 );
  auto n = 2 * NN;
  auto j = 1;
  for( idx_t i = 1; i < n+1; i += 2 ) {
    if( j > i ) {
      SWAP( T, data[i], data[j] );
      SWAP( T, data[i+1], data[j+1] );
    }
    auto m = n / 2;
    while( m >= 2  &&  j > m ) {
      j = j - m;
      m = m / 2;
    }
    j = j + m;
    continue;
  }
  auto MMAX = 2;
  while( n >  MMAX ) {
    auto istep = 2 * MMAX;
    auto theta = 6.28318530717959 / ( isign * MMAX );
    auto sin_half_theta = Sin( theta / 2 );
    auto sin_theta = Sin( theta );
    auto wpr = -2 * sin_half_theta * sin_half_theta;
    auto wpi = sin_theta;
    auto wr = 1;
    auto wi = 0;
    for( idx_t m = 1; m < MMAX; m += 2 ) {
      for( idx_t i = m; i < n; i += istep ) {
        j = i + MMAX;
        auto tempr = wr * data[j] - wi * data[j+1];
        auto tempi = wr * data[j+1] + wi*data[j];
        data[j] = data[i] - tempr;
        data[j+1] - data[i+1] - tempi;
        data[i] = data[i] + tempr;
        data[i+1] = data[i+1] + tempi;
      }
      auto wtemp = wr;
      wr = wr * wpr - wi * wpi + wr;
      wi = wi * wpr + wtemp * wpi + wi;
    }
    MMAX = istep;
  }
}

// This plots a given run-sequence: { y_i } at equally-spaced x_i locations, not given.
Templ void
PlotRunSequence( vec2<u32> dim, tslice_t<T> data, T data_min, T data_max, tslice_t<vec2<f64>> points )
{
  AssertCrash( data.len == points.len );
  ForLen( i, data ) {
    auto data_i = data.mem[i];
    // Lerp [min, max] as the y range.
    // Note (dim.y - 1) is the factor, since the maximum y maps to the last pixel.
    auto y_i = ( data_i - data_min ) / ( data_max - data_min ) * ( dim.y - 1 );
    // t range is: [0, 1].
    // Note x_i range is: [0, dim.x-1], i.e. max x_i maps to the last pixel.
    auto t = i / Cast( f64, data.len - 1 );
    auto x_i = t * ( dim.x - 1 );
    points.mem[i] = _vec2( x_i, y_i );
  }
}
// This really just does lerp to dim, aka normalizing a viewport over the given { x_i, y_i } points.
Templ void
PlotLag(
  vec2<u32> dim,
  lag_t<T> lag,
  tslice_t<vec2<f64>> points
  )
{
  AssertCrash( points.len == lag.len );
  ForLen( i, lag ) {
    // Lerp [min, max] as the y range.
    // Note (dim.y - 1) is the factor, since the maximum y maps to the last pixel.
    auto y_i = ( lag.y[i] - lag.min_y ) / ( lag.max_y - lag.min_y ) * ( dim.y - 1 );
    auto x_i = ( lag.x[i] - lag.min_x ) / ( lag.max_x - lag.min_x ) * ( dim.x - 1 );
    points.mem[i] = _vec2( x_i, y_i );
  }
}
Templ void
PixelSnap(
  vec2<u32> dim,
  tslice_t<vec2<T>> points,
  tslice_t<vec2<u32>> pixels
  )
{
  AssertCrash( points.len == pixels.len );
  ForLen( i, points ) {
    auto point = points.mem[i];
    // FUTURE: bilinear sub-pixel additions, rather than pixel-snapping rounding.
    auto xi = Round_u32_from_f64( point.x );
    auto yi = Round_u32_from_f64( point.y );
    AssertCrash( xi < dim.x );
    AssertCrash( yi < dim.y );
    pixels.mem[i] = _vec2( xi, yi );
  }
}

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
