// Copyright (c) John A. Carlos Jr., all rights reserved.

Inl f32 lerp( f32 x0, f32 x1, f32 t ) { return t * x1 + ( 1.0f - t ) * x0; }
Inl f64 lerp( f64 x0, f64 x1, f64 t ) { return t * x1 + ( 1.0  - t ) * x0; }


Inl f64
Lerp_from_f64(
  f64 y0,
  f64 y1,
  f64 x,
  f64 x0,
  f64 x1 )
{
  return y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 );
}

Inl f32
Lerp_from_f32(
  f32 y0,
  f32 y1,
  f32 x,
  f32 x0,
  f32 x1 )
{
  return y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 );
}

Inl f32
Lerp_from_s32(
  f32 y0,
  f32 y1,
  s32 x,
  s32 x0,
  s32 x1 )
{
  return y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 );
}

Inl f64
Lerp_from_s32(
  f64 y0,
  f64 y1,
  s32 x,
  s32 x0,
  s32 x1 )
{
  return y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 );
}

Inl s32
Lerp_from_s32(
  s32 y0,
  s32 y1,
  s32 x,
  s32 x0,
  s32 x1 )
{
  return Round_s32_from_f32( y0 + ( ( y1 - y0 ) / Cast( f32, x1 - x0 ) ) * ( x - x0 ) );
}

Inl s32
Lerp_from_f32(
  s32 y0,
  s32 y1,
  f32 x,
  f32 x0,
  f32 x1 )
{
  return Round_s32_from_f32( y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 ) );
}

Inl u32
Lerp_from_f32(
  u32 y0,
  u32 y1,
  f32 x,
  f32 x0,
  f32 x1 )
{
  return Round_u32_from_f32( y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 ) );
}
Inl u64
Lerp_from_f32(
  u64 y0,
  u64 y1,
  f32 x,
  f32 x0,
  f32 x1 )
{
  return Round_u64_from_f32( y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 ) );
}
