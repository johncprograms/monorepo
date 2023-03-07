// Copyright (c) John A. Carlos Jr., all rights reserved.

struct
inc_stats_t
{
  kahansum32_t mean;
  idx_t count;
  // TODO: variance
};

Inl void
Init( inc_stats_t& s )
{
  s.mean = {};
  s.count = 0;
}

Inl void
AddValue( inc_stats_t& s, f32 value )
{
  s.count += 1;
  Add( s.mean, ( value - s.mean.sum ) / Cast( f32, s.count ) );
}

Inl void
AddMean( inc_stats_t& s, f32 mean, idx_t count )
{
  AssertCrash( count );
  auto new_count = s.count + count;
  s.mean.sum *= Cast( f32, s.count ) / Cast( f32, new_count );
  s.mean.err = 0;
  mean *= Cast( f32, count ) / Cast( f32, new_count );
  Add( s.mean, mean );
  s.count = new_count;
}

// TODO: incremental variance.

#if 0
      // calculate incrementally the total time ( using kahan summation )
      auto time_elapsed_cor = time_elapsed - stats.time_total_err; // TODO: convert to kahansum64_t
      auto time_total = stats.time_total + time_elapsed_cor;
      stats.time_total_err = ( time_total - stats.time_total ) - time_elapsed_cor;
      stats.time_total = time_total;

      // calculate incrementally the mean.
      // mu' = mu + ( x + mu ) / ( n + 1 )
      auto prev_n = stats.n_invocs;
      auto rec_n = 1.0 / ( prev_n + 1 );
      auto prev_mu = stats.time_mean;
      auto prev_diff = time_elapsed - prev_mu;
      auto mu = prev_mu + rec_n * prev_diff;
      stats.time_mean = mu;

      // calculate incrementally the variance.
      // var' = ( n * var + ( x - mu ) * ( x - mu' ) ) / ( n + 1 )
      auto prev_sn = prev_n * stats.time_variance;
      stats.time_variance = rec_n * ( prev_sn + prev_diff * ( time_elapsed - mu ) );

      // n' = n + 1
      stats.n_invocs += 1;
#endif
