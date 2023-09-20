// Copyright (c) John A. Carlos Jr., all rights reserved.

struct
kahansum32_t
{
  f32 sum;
  f32 err;
};

struct
kahansum64_t
{
  f64 sum;
  f64 err;
};

Templ struct kahansum_t
{
  T sum;
  T err;
};

Inl void
Add( kahansum32_t& kahan, f32 val )
{
  f32 val_corrected = val - kahan.err;
  f32 new_sum = kahan.sum + val_corrected;
  kahan.err = ( new_sum - kahan.sum ) - val_corrected;
  kahan.sum = new_sum;
}

Inl void
Add( kahansum64_t& kahan, f64 val )
{
  f64 val_corrected = val - kahan.err;
  f64 new_sum = kahan.sum + val_corrected;
  kahan.err = ( new_sum - kahan.sum ) - val_corrected;
  kahan.sum = new_sum;
}

Templ Inl void
Add( kahansum_t<T>& kahan, T val )
{
  T val_corrected = val - kahan.err;
  T new_sum = kahan.sum + val_corrected;
  kahan.err = ( new_sum - kahan.sum ) - val_corrected;
  kahan.sum = new_sum;
}

Inl void
Sub( kahansum32_t& kahan, f32 val )
{
  Add( kahan, -val );
}

Inl void
Sub( kahansum64_t& kahan, f64 val )
{
  Add( kahan, -val );
}

