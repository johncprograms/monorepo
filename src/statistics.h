// Copyright (c) John A. Carlos Jr., all rights reserved.

// Zeros and writes the histogram to 'counts'.
// The given counts.len is the number of buckets to use; they're uniformly sized.
// data_min/data_max define the bounds of the data. This assumes they're pre-computed,
// since min/max are likely to be useful statistics for other things.
Templ Inl void
Histogram(
  tslice_t<T> data,
  T data_min,
  T data_max,
  tslice_t<T> counts,
  tslice_t<idx_t> bucket_from_data_idx, // maps data to buckets.
  tslice_t<T> counts_when_inserted // per-datum, the count of its bucket when the datum is inserted.
  )
{
  TZero( ML( counts ) );
  ForLen( i, data ) {
    auto x = data.mem[i];
    auto bucket_fractional = lerp_T_from_T<T>( 0, Cast( T, counts.len ), x, data_min, data_max );
    auto bucket = Cast( idx_t, bucket_fractional );
    bucket = MIN( bucket, counts.len - 1 );
    AssertCrash( bucket <= counts.len );
    counts_when_inserted.mem[i] = counts.mem[bucket];
    counts.mem[bucket] += 1;
    bucket_from_data_idx.mem[i] = bucket;
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
  while( n > MMAX ) {
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
PlotRunSequence(
  vec2<f32> dim,
  tslice_t<T> data,
  T data_min,
  T data_max,
  tslice_t<vec2<f32>> points
  )
{
  AssertCrash( data.len == points.len );
  ForLen( i, data ) {
    auto data_i = data.mem[i];
    // Lerp [min, max] as the y range.
    // Note (dim.y - 1) is the factor, since the maximum y maps to the last pixel.
    auto y_i = Cast( f32, ( data_i - data_min ) / ( data_max - data_min ) ) * ( dim.y - 1 );
    // t range is: [0, 1].
    // Note x_i range is: [0, dim.x-1], i.e. max x_i maps to the last pixel.
    auto t = i / Cast( f32, data.len - 1 );
    auto x_i = t * ( dim.x - 1 );
    points.mem[i] = _vec2( x_i, y_i );
  }
}
// This plots a given histogram, with each datapoint represented as a small rect.
// TODO: area-normalized histogram option. This works better for overlaying more-precise pdfs.
Templ void
PlotHistogram(
  vec2<f32> dim,
  tslice_t<T> counts,
  T counts_max,
  tslice_t<idx_t> bucket_from_data_idx,
  tslice_t<T> counts_when_inserted,
  tslice_t<rectf32_t> rects
  )
{
  AssertCrash( counts_when_inserted.len == bucket_from_data_idx.len );
  auto subdivision_w = Truncate32( dim.x / counts.len );
  auto col_w = subdivision_w - 1;
  if( col_w < 1.0f ) return; // TODO: not signalling failure upwards.
  ForLen( i, bucket_from_data_idx ) {
    auto bucket_i = bucket_from_data_idx.mem[i];
    auto count_when_inserted = counts_when_inserted.mem[i];
    AssertCrash( bucket_i < counts.len );
    auto count = counts.mem[bucket_i];

    // Lerp [0, counts_max] as the y range.
    // Note (dim.y - 1) is the factor, since the maximum y maps to the last pixel.
    auto y_i  = Cast( f32, ( ( count_when_inserted + 0 ) / counts_max ) ) * ( dim.y - 1 );
    auto y_ip = Cast( f32, ( ( count_when_inserted + 1 ) / counts_max ) ) * ( dim.y - 1 );
    auto x_i = bucket_i * subdivision_w;

    auto rect = rects.mem + i;
    rect->p0 = _vec2( x_i, dim.y - 1 - y_ip );
    rect->p1 = _vec2( x_i + col_w, dim.y - 1 - y_i );
  }
}
// This really just does lerp to dim, aka normalizing a viewport over the given { x_i, y_i } points.
Templ void
PlotLag(
  vec2<f32> dim,
  lag_t<T> lag,
  tslice_t<vec2<f32>> points
  )
{
  AssertCrash( points.len == lag.len );
  ForLen( i, lag ) {
    // Lerp [min, max] as the y range.
    // Note (dim.y - 1) is the factor, since the maximum y maps to the last pixel.
    auto y_i = Cast( f32, ( lag.y[i] - lag.min_y ) / ( lag.max_y - lag.min_y ) ) * ( dim.y - 1 );
    auto x_i = Cast( f32, ( lag.x[i] - lag.min_x ) / ( lag.max_x - lag.min_x ) ) * ( dim.x - 1 );
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
