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
