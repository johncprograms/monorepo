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
Templ Inl T
DistanceCovariance2(
  tslice_t<T> x,
  tslice_t<T> y,
  tslice_t<T> buffer // length n(n-1) + 6n
  )
{
  AssertCrash( x.len == y.len );
  auto n = x.len;
  auto a_len = ( n * ( n - 1 ) ) / 2 + n;
  auto b_len = a_len;
  AssertCrash( buffer.len >= n * ( n - 1 ) + 6 * n );

  tslice_t<T> a = { buffer.mem + 0, a_len };
  buffer.mem += a_len;
  buffer.len -= a_len;
  tslice_t<T> b = { buffer.mem + 0, b_len };
  buffer.mem += b_len;
  buffer.len -= b_len;
  tslice_t<T> a_row_means = { buffer.mem + 0, n };
  buffer.mem += n;
  buffer.len -= n;
  tslice_t<T> b_row_means = { buffer.mem + 0, n };
  buffer.mem += n;
  buffer.len -= n;
  tslice_t<T> a_col_means = { buffer.mem + 0, n };
  buffer.mem += n;
  buffer.len -= n;
  tslice_t<T> b_col_means = { buffer.mem + 0, n };
  buffer.mem += n;
  buffer.len -= n;

  kahansum_t<T> a_total_sum = {};
  kahansum_t<T> b_total_sum = {};
  auto rec_n2 = 1 / ( Cast( T, n ) * n );
  For( i, 0, n ) {
    For( j, i + 1, n ) {
      auto idx = SymmetricColWise_IndexFromXY( i, j );
      auto a_value = ABS( x.mem[i] - x.mem[j] );
      auto b_value = ABS( y.mem[i] - y.mem[j] );
      Add( a_total_sum, a_value * rec_n2 );
      Add( b_total_sum, b_value * rec_n2 );
      a.mem[idx] = a_value;
      b.mem[idx] = b_value;
    }
  }
  For( j, 0, n ) {
    kahansum_t<T> a_sum = {};
    kahansum_t<T> b_sum = {};
    For( i, 0, n ) {
      auto idx = SymmetricColWise_IndexFromXY( i, j );
      Add( a_sum, a.mem[idx] );
      Add( b_sum, b.mem[idx] );
    }
    a_row_means.mem[j] = a_sum.sum;
    b_row_means.mem[j] = b_sum.sum;
  }
  For( i, 0, n ) {
    kahansum_t<T> a_sum = {};
    kahansum_t<T> b_sum = {};
    For( j, 0, n ) {
      auto idx = SymmetricColWise_IndexFromXY( i, j );
      Add( a_sum, a.mem[idx] );
      Add( b_sum, b.mem[idx] );
    }
    a_col_means.mem[i] = a_sum.sum;
    b_col_means.mem[i] = b_sum.sum;
  }
  For( i, 0, n ) {
    For( j, 0, n ) {
      auto idx = SymmetricColWise_IndexFromXY( i, j );
      a.mem[idx] += a_total_sum.sum - a_row_means.mem[j] - a_col_means.mem[i];
      b.mem[idx] += b_total_sum.sum - b_row_means.mem[j] - b_col_means.mem[i];
    }
  }
  kahansum_t<T> dist_cov2 = {};
  For( i, 0, n ) {
    For( j, 0, n ) {
      auto idx = SymmetricColWise_IndexFromXY( i, j );
      auto term = a.mem[idx] * b.mem[idx] * rec_n2;
      Add( dist_cov2, term );
    }
  }
  return dist_cov2.sum;
}
// DistVar(x) = DistCov(x,x)
// DistCorr^2(x,y) = DistCov^2(x,y) / ( DistVar(x) * DistVar(y) )
Templ Inl void
DistanceCorrelation(
  tslice_t<T> x,
  tslice_t<T> y,
  tslice_t<T> buffer, // length n(n-1) + 6n
  T* x_distvar,
  T* y_distvar,
  T* distcov2,
  T* distcorr
  )
{
  AssertCrash( x.len == y.len );
  auto n = x.len;
  auto a_len = ( n * ( n - 1 ) ) / 2 + n;
  auto b_len = a_len;
  AssertCrash( buffer.len >= n * ( n - 1 ) + 6 * n );

  tslice_t<T> a = { buffer.mem + 0, a_len };
  buffer.mem += a_len;
  buffer.len -= a_len;
  tslice_t<T> b = { buffer.mem + 0, b_len };
  buffer.mem += b_len;
  buffer.len -= b_len;
  tslice_t<T> a_row_means = { buffer.mem + 0, n };
  buffer.mem += n;
  buffer.len -= n;
  tslice_t<T> b_row_means = { buffer.mem + 0, n };
  buffer.mem += n;
  buffer.len -= n;
  tslice_t<T> a_col_means = { buffer.mem + 0, n };
  buffer.mem += n;
  buffer.len -= n;
  tslice_t<T> b_col_means = { buffer.mem + 0, n };
  buffer.mem += n;
  buffer.len -= n;

  kahansum_t<T> a_total_sum = {};
  kahansum_t<T> b_total_sum = {};
  auto rec_n2 = 1 / ( Cast( T, n ) * n );
  For( i, 0, n ) {
    For( j, i + 1, n ) {
      auto idx = SymmetricColWise_IndexFromXY( i, j );
      auto a_value = ABS( x.mem[i] - x.mem[j] );
      auto b_value = ABS( y.mem[i] - y.mem[j] );
      Add( a_total_sum, a_value * rec_n2 );
      Add( b_total_sum, b_value * rec_n2 );
      a.mem[idx] = a_value;
      b.mem[idx] = b_value;
    }
  }
  For( j, 0, n ) {
    kahansum_t<T> a_sum = {};
    kahansum_t<T> b_sum = {};
    For( i, 0, n ) {
      auto idx = SymmetricColWise_IndexFromXY( i, j );
      Add( a_sum, a.mem[idx] );
      Add( b_sum, b.mem[idx] );
    }
    a_row_means.mem[j] = a_sum.sum;
    b_row_means.mem[j] = b_sum.sum;
  }
  For( i, 0, n ) {
    kahansum_t<T> a_sum = {};
    kahansum_t<T> b_sum = {};
    For( j, 0, n ) {
      auto idx = SymmetricColWise_IndexFromXY( i, j );
      Add( a_sum, a.mem[idx] );
      Add( b_sum, b.mem[idx] );
    }
    a_col_means.mem[i] = a_sum.sum;
    b_col_means.mem[i] = b_sum.sum;
  }
  kahansum_t<T> distcov2_a_b = {};
  kahansum_t<T> distvar2_a = {};
  kahansum_t<T> distvar2_b = {};
  For( i, 0, n ) {
    For( j, 0, n ) {
      auto idx = SymmetricColWise_IndexFromXY( i, j );
      auto A = a.mem[idx] + a_total_sum.sum - a_row_means.mem[j] - a_col_means.mem[i];
      auto B = b.mem[idx] + b_total_sum.sum - b_row_means.mem[j] - b_col_means.mem[i];
      Add( distcov2_a_b, A * B * rec_n2 );
      Add( distvar2_a, A * A * rec_n2 );
      Add( distvar2_b, B * B * rec_n2 );
    }
  }
  auto distvar_a = Sqrt( distvar2_a.sum );
  auto distvar_b = Sqrt( distvar2_b.sum );
  *x_distvar = distvar_a;
  *y_distvar = distvar_b;
  *distcov2 = distcov2_a_b.sum;
  *distcorr = Sqrt( distcov2 / ( distvar_a * distvar_b ) );
}

// Discrete Fourier transform (DFT):
//   F[n] = sum( k=0..N-1, f[k] * exp( i * -2pi * n * k / N ) )
// Note that exp( i * a * x ) = cos( a * x ) + i sin( a * x ), Euler's formula.
// So in this instance, where x=k,
//   F[n] = sum( k=0..N-1, f[k] * ( cos( -2pi * n * k / N ) + i sin( -2pi * n * k / N ) ) )
//   F[n] = sum( k=0..N-1, ( Re(f[k]) + i Im(f[k]) ) * ( cos( -2pi * n * k / N ) + i sin( -2pi * n * k / N ) ) )
// Complex multiplication:
//   (a+bi)(c+di) = (ac-bd)+(ad+bc)i
// We have:
//   a = Re(f[k])
//   b = Im(f[k])
//   c = cos( -2pi * n * k / N )
//   d = sin( -2pi * n * k / N )
// F[n] = sum( k=0..N-1, ( Re(f[k]) cos - Im(f[k]) sin ) + i ( Re(f[k]) sin + Im(f[k]) cos ) )
// Separating real/imaginary,
//   Re(F[n]) = sum( k=0..N-1, Re(f[k]) cos - Im(f[k]) sin )
//   Im(F[n]) = sum( k=0..N-1, Re(f[k]) sin + Im(f[k]) cos )
//
//
// The inverse is defined as:
//   f[k] = 1/N * sum( n=0..N-1, F[n] * exp( i * 2pi * n * k / N ) )
// Very similar to F's defn, just with an extra 1/N factor, and the exponent is negated.
// Expanding with Euler's formula:
//   f[k] = 1/N * sum( n=0..N-1, ( Re(F[n]) + i Im(F[n]) ) * ( cos( i * 2pi * n * k / N ) + i sin( i * 2pi * n * k / N ) ) )
// Separating real/imaginary,
//   Re(f[k]) = 1/N * sum( n=0..N-1, Re(F[n]) * cos - Im(F[n]) * sin )
//   Im(f[k]) = 1/N * sum( n=0..N-1, Re(F[n]) * sin + Im(F[n]) * cos )
//

// Fills F, given f.
Templ Inl void
DiscreteFourierTransform(
  tslice_t<T> Re_f,
  tslice_t<T> Im_f,
  tslice_t<T> Re_F,
  tslice_t<T> Im_F
  )
{
  auto N = Re_f.len;
  AssertCrash( Im_f.len == N );
  AssertCrash( Re_F.len == N );
  AssertCrash( Im_F.len == N );
  auto twopi_over_N = Cast( T, f64_2PI ) / N;
  For( n, 0, N ) {
    auto negative_n_twopi_over_N = -Cast( T, n ) * twopi_over_N;
    kahansum_t<T> Re_F_n = {};
    kahansum_t<T> Im_F_n = {};
    For( k, 0, N ) {
      auto t = negative_n_twopi_over_N * k;
      auto cos = Cos( t );
      auto sin = Sin( t );
      auto Re_f_k = Re_f.mem[k];
      auto Im_f_k = Im_f.mem[k];
      auto Re_term = Re_f_k * cos - Im_f_k * sin;
      auto Im_term = Re_f_k * sin + Im_f_k * cos;
      Add( Re_F_n, Re_term );
      Add( Im_F_n, Im_term );
    }
    Re_F.mem[n] = Re_F_n.sum;
    Im_F.mem[n] = Im_F_n.sum;
  }
}
// Fills f, given F.
Templ Inl void
InverseDiscreteFourierTransform(
  tslice_t<T> Re_f,
  tslice_t<T> Im_f,
  tslice_t<T> Re_F,
  tslice_t<T> Im_F
  )
{
  // Re(f[k]) = 1/N * sum( n=0..N-1, Re(F[n]) * cos - Im(F[n]) * sin )
  // Im(f[k]) = 1/N * sum( n=0..N-1, Re(F[n]) * sin + Im(F[n]) * cos )
  auto N = Re_f.len;
  AssertCrash( Im_f.len == N );
  AssertCrash( Re_F.len == N );
  AssertCrash( Im_F.len == N );
  auto twopi_over_N = Cast( T, f64_2PI ) / N;
  auto rec_N = 1 / Cast( T, N );
  For( k, 0, N ) {
    auto k_twopi_over_N = k * twopi_over_N;
    kahansum_t<T> Re_f_k = {};
    kahansum_t<T> Im_f_k = {};
    For( n, 0, N ) {
      auto t = k_twopi_over_N * n;
      auto cos = Cos( t );
      auto sin = Sin( t );
      auto Re_F_n = Re_F.mem[n];
      auto Im_F_n = Im_F.mem[n];
      auto Re_term = ( Re_F_n * cos - Im_F_n * sin ) * rec_N;
      auto Im_term = ( Re_F_n * sin + Im_F_n * cos ) * rec_N;
      Add( Re_f_k, Re_term );
      Add( Im_f_k, Im_term );
    }
    Re_f.mem[k] = Re_f_k.sum;
    Im_f.mem[k] = Im_f_k.sum;
  }
}

// In-place Fourier transforms of 'data', which stores complex numbers.
// Assumes 'data.len' is a power of 2. Use a 0-padded buffer to round up to that.
// When isign==1, the result is the discrete fourier transform.
// When isign==-1, the result is NN times the inverse discrete fourier transform.
Templ Inl void
FOUR1(
  tslice_t<T> data,
  T isign
  )
{
  AssertCrash( !( data.len & 1 ) ); // must be even.
  AssertCrash( IsPowerOf2( data.len / 2 ) );
  AssertCrash( isign == -1  ||  isign == 1 );
  auto n = data.len;
  auto mem = data.mem;
  idx_t j = 1;
  for( idx_t i = 0; i < n; i += 2 ) {
    if( j > i ) {
      SWAP( T, mem[i], mem[j] );
      SWAP( T, mem[i+1], mem[j+1] );
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
    T wr = 1;
    T wi = 0;
    for( idx_t m = 0; m < MMAX; m += 2 ) {
      for( idx_t i = m; i < n; i += istep ) {
        j = i + MMAX;
        auto tempr = wr * mem[j] - wi * mem[j+1];
        auto tempi = wr * mem[j+1] + wi * mem[j];
        mem[j] = mem[i] - tempr;
        mem[j+1] = mem[i+1] - tempi;
        mem[i] = mem[i] + tempr;
        mem[i+1] = mem[i+1] + tempi;
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
// This plots the given { x_i, y_i } points.
Templ void
PlotXY(
  vec2<f32> dim,
  tslice_t<T> data_y,
  T data_y_min,
  T data_y_max,
  tslice_t<T> data_x,
  T data_x_min,
  T data_x_max,
  tslice_t<vec2<f32>> points
  )
{
  AssertCrash( data_x.len == data_y.len );
  AssertCrash( points.len == data_y.len );
  ForLen( i, data_y ) {
    // Lerp [min, max] as the y range.
    // Note (dim.y - 1) is the factor, since the maximum y maps to the last pixel.
    auto y_i = Cast( f32, ( data_y.mem[i] - data_y_min ) / ( data_y_max - data_y_min ) ) * ( dim.y - 1 );
    auto x_i = Cast( f32, ( data_x.mem[i] - data_x_min ) / ( data_x_max - data_x_min ) ) * ( dim.x - 1 );
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


#if defined(TEST)

Inl bool
RoughlyEqual(
  f64* a,
  idx_t a_len,
  f64* b,
  idx_t b_len,
  f64 epsilon
  )
{
  AssertCrash( a_len == b_len );
  For( i, 0, a_len ) {
    if( ABS( a[i] - b[i] ) >= epsilon ) {
      return 0;
    }
  }
  return 1;
}

Inl void
TestStatistics()
{
  auto epsilon = 1e-6;

  {
    f64 Re_f[4] = { 1, 2, 0, -1 };
    f64 Im_f[4] = { 0, -1, -1, 2 };
    f64 Re_F[4];
    f64 Im_F[4];
    DiscreteFourierTransform(
      SliceFromCArray( f64, Re_f ),
      SliceFromCArray( f64, Im_f ),
      SliceFromCArray( f64, Re_F ),
      SliceFromCArray( f64, Im_F )
      );
    f64 Re_expected[] = { 2, -2, 0, 4 };
    f64 Im_expected[] = { 0, -2, -2, 4 };
    AssertCrash( RoughlyEqual( AL( Re_F ), AL( Re_expected ), epsilon ) );
    AssertCrash( RoughlyEqual( AL( Im_F ), AL( Im_expected ), epsilon ) );
  }

  {
    f64 f[8] = { 1, 0, 2, -1, 0, -1, -1, 2 };
    FOUR1<f64>( SliceFromCArray( f64, f ), 1 );
    f64 expected[8] = { 2, 0, -2, -2, 0, -2, 4, 4 };
    AssertCrash( RoughlyEqual( AL( f ), AL( expected ), epsilon ) );
  }

  {
    f64 Re_f[4];
    f64 Im_f[4];
    f64 Re_F[4] = { 2, -2, 0, 4 };
    f64 Im_F[4] = { 0, -2, -2, 4 };
    InverseDiscreteFourierTransform(
      SliceFromCArray( f64, Re_f ),
      SliceFromCArray( f64, Im_f ),
      SliceFromCArray( f64, Re_F ),
      SliceFromCArray( f64, Im_F )
      );
    f64 Re_expected[4] = { 1, 2, 0, -1 };
    f64 Im_expected[4] = { 0, -1, -1, 2 };
    AssertCrash( RoughlyEqual( AL( Re_f ), AL( Re_expected ), epsilon ) );
    AssertCrash( RoughlyEqual( AL( Im_f ), AL( Im_expected ), epsilon ) );
  }
}

#endif // defined(TEST)
