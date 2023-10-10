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
// Fills F, given f.
Templ Inl void
DiscreteFourierTransformReference(
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
// The inverse DFT is defined as:
//   f[k] = 1/N * sum( n=0..N-1, F[n] * exp( i * 2pi * n * k / N ) )
// Very similar to F's defn, just with an extra 1/N factor, and the exponent is negated.
// Expanding with Euler's formula:
//   f[k] = 1/N * sum( n=0..N-1, ( Re(F[n]) + i Im(F[n]) ) * ( cos( i * 2pi * n * k / N ) + i sin( i * 2pi * n * k / N ) ) )
// Separating real/imaginary,
//   Re(f[k]) = 1/N * sum( n=0..N-1, Re(F[n]) * cos - Im(F[n]) * sin )
//   Im(f[k]) = 1/N * sum( n=0..N-1, Re(F[n]) * sin + Im(F[n]) * cos )
//
// Fills f, given F.
Templ Inl void
InverseDiscreteFourierTransformReference(
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

Templ Inl void
DiscreteFourierTransformRecursive(
  tslice_t<T> Re_f,
  tslice_t<T> Im_f,
  tslice_t<T> Re_F,
  tslice_t<T> Im_F
  )
{
  auto N = Re_f.len;
  if( N == 1 ) {
    Re_F.mem[0] = Re_f.mem[0];
    Im_F.mem[0] = Im_f.mem[0];
    return;
  }
  auto buffer = AllocString<T>( 4*N );
  auto tmp = buffer.mem;
  auto Re_evn = tslice_t<T>{ tmp, N/2 };  tmp += N/2;
  auto Im_evn = tslice_t<T>{ tmp, N/2 };  tmp += N/2;
  auto Re_odd = tslice_t<T>{ tmp, N/2 };  tmp += N/2;
  auto Im_odd = tslice_t<T>{ tmp, N/2 };  tmp += N/2;
  auto Re_Evn = tslice_t<T>{ tmp, N/2 };  tmp += N/2;
  auto Im_Evn = tslice_t<T>{ tmp, N/2 };  tmp += N/2;
  auto Re_Odd = tslice_t<T>{ tmp, N/2 };  tmp += N/2;
  auto Im_Odd = tslice_t<T>{ tmp, N/2 };  tmp += N/2;
  For( i, 0, N/2 ) {
    Re_evn.mem[i] = Re_f.mem[2*i+0];
    Im_evn.mem[i] = Im_f.mem[2*i+0];
    Re_odd.mem[i] = Re_f.mem[2*i+1];
    Im_odd.mem[i] = Im_f.mem[2*i+1];
  }
  DiscreteFourierTransformRecursive(
    Re_evn,
    Im_evn,
    Re_Evn,
    Im_Evn
    );
  DiscreteFourierTransformRecursive(
    Re_odd,
    Im_odd,
    Re_Odd,
    Im_Odd
    );
  auto negative_twopi_over_N = Cast( T, -f64_2PI ) / N;
  auto rec_N = 1 / Cast( T, N );
  For( k, 0, N/2 ) {
    // (a+bi)(c+di) = (ac-bd)+(ad+bc)i
    // (Ar+Aii)(Br+Bii) = (Ar Br - Ai Bi)+(Ar Bi + Ai Br)i
    auto t = negative_twopi_over_N * k;
    auto Re_W = Cos( t );
    auto Im_W = Sin( t );
    auto Re_A = Re_Evn.mem[k];
    auto Im_A = Im_Evn.mem[k];
    auto Re_B = Re_Odd.mem[k];
    auto Im_B = Im_Odd.mem[k];
    auto Re_W_B = Re_W * Re_B - Im_W * Im_B;
    auto Im_W_B = Re_W * Im_B + Im_W * Re_B;
    Re_F.mem[k]       = Re_A + Re_W_B;
    Im_F.mem[k]       = Im_A + Im_W_B;
    Re_F.mem[k + N/2] = Re_A - Re_W_B;
    Im_F.mem[k + N/2] = Im_A - Im_W_B;
  }
  Free( buffer );
}

ForceInl u32
ReverseBits(u32 x)
{
  constant u32 mask1 = 0b01010101010101010101010101010101;
  x = ( ( x & mask1 ) << 1 ) | ( ( x >> 1 ) & mask1 );
  constant u32 mask2 = 0b00110011001100110011001100110011;
  x = ( ( x & mask2 ) << 2 ) | ( ( x >> 2 ) & mask2 );
  constant u32 mask4 = 0b00001111000011110000111100001111;
  x = ( ( x & mask4 ) << 4 ) | ( ( x >> 4 ) & mask4 );
  constant u32 mask8 = 0b00000000111111110000000011111111;
  x = ( ( x & mask8 ) << 8 ) | ( ( x >> 8 ) & mask8 );
  //                   0b00000000000000001111111111111111;
  x = ( x << 16 ) | ( x >> 16 );
  return x;
}
Templ Inl void
DiscreteFourierTransform(
  tslice_t<T> Re_f,
  tslice_t<T> Im_f,
  tslice_t<T> Re_F,
  tslice_t<T> Im_F
  )
{
  auto n = Re_f.len;
  AssertCrash( n == Im_f.len );
  AssertCrash( n == Re_F.len );
  AssertCrash( n == Im_F.len );
  AssertCrash( IsPowerOf2( n ) );

  // log_base_2(n), the number of bits needed to represent { 0, 1, ..., n-1 }.
  auto num_bits_n = 8u * _SIZEOF_IDX_T - _lzcnt_idx_t( n ) - 1;
  AssertCrash( n <= MAX_u32 );
  For( i, 0, n ) {
    auto rev_i = ReverseBits( Cast( u32, i ) ) >> ( 32 - num_bits_n );
    Re_F.mem[rev_i] = Re_f.mem[i];
    Im_F.mem[rev_i] = Im_f.mem[i];
  }

  idx_t step = 2;
  For( pass, 0, num_bits_n ) {
    auto negative_twopi_over_step = Cast( T, -f64_2PI ) / step;
    for( idx_t offset = 0;  offset < n;  offset += step ) {
      auto half_step = step/2;
      For( k, 0, half_step ) {
        auto offset_k = offset + k;
        auto offset_k_half_step = offset_k + half_step;
        auto t = negative_twopi_over_step * k;
        auto Re_w = Cos( t );
        auto Im_w = Sin( t );
        auto Re_u = Re_F.mem[offset_k];
        auto Im_u = Im_F.mem[offset_k];
        auto Re_v = Re_F.mem[offset_k_half_step];
        auto Im_v = Im_F.mem[offset_k_half_step];
        auto Re_w_v = Re_w * Re_v - Im_w * Im_v;
        auto Im_w_v = Re_w * Im_v + Im_w * Re_v;
        Re_F.mem[offset_k]           = Re_u + Re_w_v;
        Im_F.mem[offset_k]           = Im_u + Im_w_v;
        Re_F.mem[offset_k_half_step] = Re_u - Re_w_v;
        Im_F.mem[offset_k_half_step] = Im_u - Im_w_v;
      }
    }
    step *= 2;
  }
}
Templ Inl void
InverseDiscreteFourierTransform(
  tslice_t<T> Re_f,
  tslice_t<T> Im_f,
  tslice_t<T> Re_F,
  tslice_t<T> Im_F
  )
{
  auto n = Re_f.len;
  AssertCrash( n == Im_f.len );
  AssertCrash( n == Re_F.len );
  AssertCrash( n == Im_F.len );
  AssertCrash( IsPowerOf2( n ) );

  // log_base_2(n), the number of bits needed to represent { 0, 1, ..., n-1 }.
  auto num_bits_n = 8u * _SIZEOF_IDX_T - _lzcnt_idx_t( n ) - 1;
  AssertCrash( n <= MAX_u32 );
  For( i, 0, n ) {
    auto rev_i = ReverseBits( Cast( u32, i ) ) >> ( 32 - num_bits_n );
    Re_f.mem[rev_i] = Re_F.mem[i];
    Im_f.mem[rev_i] = Im_F.mem[i];
  }

  idx_t step = 2;
  For( pass, 0, num_bits_n ) {
    auto twopi_over_step = Cast( T, f64_2PI ) / step;
    for( idx_t offset = 0;  offset < n;  offset += step ) {
      auto half_step = step/2;
      For( k, 0, half_step ) {
        auto offset_k = offset + k;
        auto offset_k_half_step = offset_k + half_step;
        auto t = twopi_over_step * k;
        auto Re_w = Cos( t );
        auto Im_w = Sin( t );
        auto Re_u = Re_f.mem[offset_k];
        auto Im_u = Im_f.mem[offset_k];
        auto Re_v = Re_f.mem[offset_k_half_step];
        auto Im_v = Im_f.mem[offset_k_half_step];
        auto Re_w_v = Re_w * Re_v - Im_w * Im_v;
        auto Im_w_v = Re_w * Im_v + Im_w * Re_v;
        Re_f.mem[offset_k]           = Re_u + Re_w_v;
        Im_f.mem[offset_k]           = Im_u + Im_w_v;
        Re_f.mem[offset_k_half_step] = Re_u - Re_w_v;
        Im_f.mem[offset_k_half_step] = Im_u - Im_w_v;
      }
    }
    step *= 2;
  }

  auto rec_N = 1 / Cast( T, n );
  For( i, 0, n ) {
    Re_f.mem[i] *= rec_N;
    Im_f.mem[i] *= rec_N;
  }
}

Templ Inl void
PowerSpectrum(
  tslice_t<T> data,
  tslice_t<T> power,
  tslice_t<T> buffer // length 2 * data.len
  )
{
  auto n = data.len;
  AssertCrash( n == power.len );
  tslice_t<T> Im_data = { buffer.mem, n };
  TZero( ML( Im_data ) );
  tslice_t<T> Im_power = { buffer.mem + n, n };
  DiscreteFourierTransform(
    data,
    Im_data,
    power,
    Im_power
    );
  For( i, 0, n ) {
    auto a = power.mem[i];
    auto b = Im_power.mem[i];
    power.mem[i] = Sqrt( a * a + b * b );
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
    DiscreteFourierTransformReference(
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
    f64 Re_f[4] = { 1, 2, 0, -1 };
    f64 Im_f[4] = { 0, -1, -1, 2 };
    f64 Re_F[4];
    f64 Im_F[4];
    DiscreteFourierTransformRecursive(
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
    f64 Re_f[2] = { 1.3, 0.7 };
    f64 Im_f[2] = { 1.2, -0.7 };
    f64 Re_F[2];
    f64 Im_F[2];
    DiscreteFourierTransformReference(
      SliceFromCArray( f64, Re_f ),
      SliceFromCArray( f64, Im_f ),
      SliceFromCArray( f64, Re_F ),
      SliceFromCArray( f64, Im_F )
      );
    // Prediction is:
    //   F[n,2] = f[0] + f[1] exp( n A / 2 )
    //   F[0] = f[0] + f[1] exp(0)
    //        = 1.3+1.2i + 0.7-0.7i
    //        = 2+0.5i
    //   F[1] = f[0] + f[1] exp(-2pi i / 2)
    //        = 1.3+1.2i + 0.7-0.7i (cos(-pi)+isin(-pi))
    //        = 1.3+1.2i + 0.7-0.7i (-1)
    //        = 1.3+1.2i + -0.7+0.7i
    //        = 0.6+1.9i
    f64 Re_expected[] = { 2, 0.6 };
    f64 Im_expected[] = { 0.5, 1.9 };
    AssertCrash( RoughlyEqual( AL( Re_F ), AL( Re_expected ), epsilon ) );
    AssertCrash( RoughlyEqual( AL( Im_F ), AL( Im_expected ), epsilon ) );
  }

  if (0)
  {
    //              0   1  2   3  4   5  6   7
    f64 Re_f[8] = { 1,  2, 3,  4, 5,  6, 7,  8 };
    f64 Im_f[8] = { 1, -2, 3, -4, 5, -6, 7, -8 };
    f64 Re_F[8];
    f64 Im_F[8];
    DiscreteFourierTransformReference(
      SliceFromCArray( f64, Re_f ),
      SliceFromCArray( f64, Im_f ),
      SliceFromCArray( f64, Re_F ),
      SliceFromCArray( f64, Im_F )
      );
    // Prediction is:
    //   F[n,8] = ( f[0] + f[4] exp( n A / 2 ) )
    //          + ( f[2] + f[6] exp( n A / 2 ) ) exp( n A / 4 )
    //          + ( ( f[1] + f[5] exp( n A / 2 ) )
    //          +   ( f[3] + f[7] exp( n A / 2 ) ) exp( n A / 4 )
    //            ) exp( n A / 8 )
    //   F[0] = f[0] + f[4] + f[2] + f[6] + f[1] + f[5] + f[3] + f[7]
    //   F[1] = ( f[0] + f[4] exp( A / 2 ) )
    //        + ( f[2] + f[6] exp( A / 2 ) ) exp( A / 4 )
    //        + ( ( f[1] + f[5] exp( A / 2 ) )
    //        +   ( f[3] + f[7] exp( A / 2 ) ) exp( A / 4 )
    //          ) exp( A / 8 )
    //
    //        = ( 1+1i + 5+5i exp(-pi i) )
    //        + ( 3+3i + 7+7i exp(-pi i) ) exp(-pi/2 i)
    //        + ( ( 2-2i + 6-6i exp(-pi i) )
    //        +   ( 4-4i + 8-8i exp(-pi i) ) exp(-pi/2 i)
    //          ) exp(-pi/4 i)
    //
    //        = ( 1+1i + -5-5i )
    //        + ( 3+3i + -7-7i ) ( cos(-pi/2) + isin(-pi/2) )
    //        + ( ( 2-2i + -6+6i )
    //        +   ( 4-4i + -8+8i ) ( cos(-pi/2) + isin(-pi/2) )
    //          ) ( cos(-pi/4) + isin(-pi/4) )
    //
    //        = ( 1+1i + -5-5i )
    //        + ( 3+3i + -7-7i ) (-i)
    //        + ( ( 2-2i + -6+6i )
    //        +   ( 4-4i + -8+8i ) (-i)
    //          ) ( 2^-0.5 + 2^-0.5i )
    //
    //        = ( 1+1i + -5-5i )
    //        + ( 3-3i + -7+7i )
    //        + ( ( 2-2i + -6+6i )
    //        +   ( -4-4i + 8+8i )
    //          ) ( 2^-0.5 + 2^-0.5i )
    //
    //        = ( 4-2i + -12+2i )
    //        + ( -2-6i + 2+14i )( 2^-0.5 + 2^-0.5i )
    //
    //        = ( 4-2i + -12+2i )
    //        + ( -2-6i + 2+14i )( 2^-0.5 + 2^-0.5i )
    //
    //        = -8 + 8i( 2^-0.5 + 2^-0.5i )
    //        = -8(1-2^-0.5) + 8*2^-0.5 i
    //        = -2.34315... + 5.65685... i
    //
    f64 Re_expected[] = { 2, 0.6 };
    f64 Im_expected[] = { 0.5, 1.9 };
    AssertCrash( RoughlyEqual( AL( Re_F ), AL( Re_expected ), epsilon ) );
    AssertCrash( RoughlyEqual( AL( Im_F ), AL( Im_expected ), epsilon ) );
  }

  {
    f64 Re_f[4];
    f64 Im_f[4];
    f64 Re_F[4] = { 2, -2, 0, 4 };
    f64 Im_F[4] = { 0, -2, -2, 4 };
    InverseDiscreteFourierTransformReference(
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
