// Copyright (c) John A. Carlos Jr., all rights reserved.

#define _mm_add3_ps( a, b, c )   ( _mm_add_ps( a, _mm_add_ps( b, c ) ) )
#define _mm_mul3_ps( a, b, c )   ( _mm_mul_ps( a, _mm_mul_ps( b, c ) ) )
#define _mm_and3_ps( a, b, c )   ( _mm_and_ps( a, _mm_and_ps( b, c ) ) )
#define _mm_or3_si128( a, b, c )   ( _mm_or_si128( a, _mm_or_si128( b, c ) ) )
#define _mm_add4_ps( a, b, c, d )   ( _mm_add_ps( _mm_add_ps( a, b ), _mm_add_ps( c, d ) ) )
#define _mm_and4_ps( a, b, c, d )   ( _mm_and_ps( _mm_and_ps( a, b ), _mm_and_ps( c, d ) ) )
#define _mm_clamp_ps( x, lower, upper )   ( _mm_max_ps( lower, _mm_min_ps( x, upper ) ) )
#define _mm_cross_ps( a_x, a_y, b_x, b_y )   ( _mm_sub_ps( _mm_mul_ps( a_x, b_y ), _mm_mul_ps( b_x, a_y ) ) )

#define _mm_add3_epi32( a, b, c )   ( _mm_add_epi32( a, _mm_add_epi32( b, c ) ) )
#define _mm_clamp_epi32( x, lower, upper )   ( _mm_max_epi32( lower, _mm_min_epi32( x, upper ) ) )


Inl vec2<s32>
IntegerPixelFromImagespace( f32 x, f32 y, s32 img_x_m1, s32 img_y_m1 )
{
  return _vec2(
    CLAMP( Round_s32_from_f32( x ), 0, img_x_m1 ),
    CLAMP( Round_s32_from_f32( y ), 0, img_y_m1 )
    );
}

Inl bool
SameImagespacePixelY( f32 y0, f32 y1 )
{
  return ( Round_s32_from_f32( y0 ) == Round_s32_from_f32( y1 ) );
}


struct
img_t
{
  u32 x;
  u32 y;
  u32 stride_x;
  u32 bytes_per_px;
  s32 x_m1;
  s32 y_m1;
  f32 xf;
  f32 yf;
  f32 aspect_ratio;
  string_t mem;
};

Inl void
Init( img_t& img, u32 x, u32 y, u32 stride_x, u32 bytes_per_px )
{
  AssertCrash( stride_x >= x ); // TODO: auto stride!
  img.x = x;
  img.y = y;
  img.stride_x = stride_x;
  img.bytes_per_px = bytes_per_px;
  AssertCrash( x <= MAX_s32 );
  AssertCrash( y <= MAX_s32 );
  img.x_m1 = Cast( s32, x ) - 1;
  img.y_m1 = Cast( s32, y ) - 1;
  img.xf = Cast( f32, img.x );
  img.yf = Cast( f32, img.y );
  img.aspect_ratio = img.xf / img.yf;
  Zero( img.mem );
}

Inl void
Alloc( img_t& img )
{
  Alloc( img.mem, img.stride_x * img.y * img.bytes_per_px );
}

Inl void
Free( img_t& img )
{
  Free( img.mem );
}



Inl void
Zero( img_t& img )
{
  Memzero( img.mem.mem, img.stride_x * img.y * img.bytes_per_px );
}



Inl u8*
Lookup( img_t& img, u32 x, u32 y )
{
  u32 idx = img.bytes_per_px * ( x + img.stride_x * y );
  u8* mem = img.mem.mem + idx;
  return mem;
}

#define LookupAs( type, img, x, y )   ( *Cast( type*, Lookup( img, x, y ) ) )


Templ Inl void
StorePx( img_t& img, T& src )
{
  AssertCrash( sizeof( T ) == img.bytes_per_px );

  Fori( u32, y, 0, img.y ) {
  Fori( u32, x, 0, img.x ) {
    auto& dst = LookupAs( T, img, x, y );
    dst = src;
  }
  }
}

Inl void
StorePx16B( img_t& img, __m128& src )
{
  AssertCrash( img.bytes_per_px == 16 );

  Fori( u32, y, 0, img.y ) {
  Fori( u32, x, 0, img.x ) {
    auto& dst = LookupAs( f32, img, x, y );
    _mm_store_ps( &dst, src );
  }
  }
}

Inl void
Store4Px4B( img_t& img, __m128& src )
{
  AssertCrash( img.bytes_per_px == 4 );
  AssertCrash( img.stride_x % 4 == 0 );

  Fori( u32, y, 0, img.y ) {
  Forinc( u32, x, 0, img.x, 4 ) {
    auto& dst = LookupAs( f32, img, x, y );
    _mm_store_ps( &dst, src );
  }
  }
}
Inl void
Store4Px4Bi( img_t& img, __m128i& src )
{
  AssertCrash( img.bytes_per_px == 4 );
  AssertCrash( img.stride_x % 4 == 0 );

  Fori( u32, y, 0, img.y ) {
  Forinc( u32, x, 0, img.x, 4 ) {
    auto& dst = LookupAs( __m128i, img, x, y );
    _mm_store_si128( &dst, src );
  }
  }
}
//Inl void
//Set( img_t& img, d128_t& d )
//{
//  __m128 src = _mm_setr_ps( d.mf32[0], d.mf32[1], d.mf32[2], d.mf32[3] );
//  Fori( u32, y, 0, img.y ) {
//  Fori( u32, x, 0, img.x ) {
//    _mm_store_ps( Cast( f32*, &img.m[ x + img.stride_x * y ] ), src );
//  }
//  }
//}


//Inl void
//FlipOverHorizontal( img_t& img )
//{
//  u32 half_y = img.y / 2;
//  u32 yb;
//  for( u32 y = 0;  y < half_y;  ++y ) {
//    yb = img.y - y;
//    Fori( u32, x, 0, img.x ) {
//      SWAP( d128_t, img.m[ x + img.stride_x * y ], img.m[ x + img.stride_x * yb ] );
//    }
//  }
//}
//
//Inl void
//FlipOverVertical( img_t& img )
//{
//  u32 half_x = img.x / 2;
//  u32 xb;
//  for( u32 x = 0;  x < half_x;  ++x ) {
//    xb = img.x - x;
//    Fori( u32, y, 0, img.y ) {
//      SWAP( d128_t, img.m[ x + img.stride_x * y ], img.m[ xb + img.stride_x * y ] );
//    }
//  }
//}
//
//Inl void
//Rotate180DegCW( img_t& img )
//{
//  u32 half_y = img.y / 2;
//  u32 xb, yb;
//  for( u32 y = 0;  y < half_y;  ++y ) {
//    yb = img.y - y;
//    Fori( u32, x, 0, img.x ) {
//      xb = img.x - x;
//      SWAP( d128_t, img.m[ x + img.stride_x * y ], img.m[ xb + img.stride_x * yb ] );
//    }
//  }
//}



Inl s32
ClampXToInside( img_t& img, s32 x )
{
  s32 r = MAX( x, 0 );
  r = MIN( x, Cast( s32, img.x ) ); // fill rule is [ x0, x1 ), so this is the right upper bound.
  return r;
}

Inl s32
ClampYToInside( img_t& img, s32 y )
{
  s32 r = MAX( y, 0 );
  r = MIN( y, Cast( s32, img.y ) ); // fill rule is [ y0, y1 ), so this is the right upper bound.
  return r;
}

Inl bool
IsInsideX( img_t& img, s32 x )
{
  bool r = ( 0 <= x )  &&  ( x <= img.x_m1 );
  return r;
}

Inl bool
IsInsideY( img_t& img, s32 y )
{
  bool r = ( 0 <= y )  &&  ( y <= img.y_m1 );
  return r;
}

Inl bool
IsInside( img_t& img, s32 x, s32 y )
{
  bool r = IsInsideX( img, x )  &&  IsInsideY( img, y );
  return r;
}






// Zero-mean, 2D gaussian.
Inl f32
Gaussian2( f32 x, f32 y, f32 sigma )
{
  f32 two_sigma2 = 2 * sigma * sigma;
  f32 coeff = 1 / ( f32_PI * two_sigma2 );
  f32 expon = -( x * x + y * y ) / two_sigma2;
  f32 r = coeff * Exp32( expon );
  return r;
}


void
BlurEdges( img_t& target_img )
{
  ProfFunc();
  static f32 edge_detect_kernel[9] =
  {
    -1, -1, -1,
    -1,  8, -1,
    -1, -1, -1,
  };

  static f32 edge_value_threshold = 0.1f;

  static f32 gaussian_kernel[9] =
  {
    1.0 / 16, 2.0 / 16, 1.0 / 16,
    2.0 / 16, 4.0 / 16, 2.0 / 16,
    1.0 / 16, 2.0 / 16, 1.0 / 16,
  };

  img_t tmp;
  Init( tmp, target_img.x, target_img.y, target_img.stride_x, 16 );
  Alloc( tmp );

  // Zero out the 1px borders.
  Fori( u32, x, 0, tmp.x ) {
    auto& top = LookupAs( vec4<f32>, tmp, x, 0 );
    auto& bot = LookupAs( vec4<f32>, tmp, x, tmp.y_m1 );
    top = {};
    bot = {};
  }
  Fori( u32, y, 0, target_img.y ) {
    auto& left = LookupAs( vec4<f32>, tmp, 0, y );
    auto& rght = LookupAs( vec4<f32>, tmp, tmp.x_m1, y );
    left = {};
    rght = {};
  }

  f32 max_edge_value = 0;
  for( u32 y = 1;  y + 1 < target_img.y;  ++y ) {
  for( u32 x = 1;  x + 1 < target_img.x;  ++x ) {

    f32 convolved = 0;
    Fori( u32, i, 0, 9 ) {
      u32 x_offset = ( i % 3 ) - 1;
      u32 y_offset = ( i / 3 ) - 1;
      vec4<f32>& px_img = LookupAs( vec4<f32>, target_img, x + x_offset, y + y_offset );

      f32 lumin = px_img.w * ( px_img.x + 2 * px_img.y + px_img.z ) / 4;
      f32 weight = edge_detect_kernel[i];
      convolved += weight * lumin;
    }
    f32 edge_value = MAX( 0, convolved );
    vec4<f32>& px_tmp = LookupAs( vec4<f32>, tmp, x, y );
    px_tmp.x = edge_value;

    max_edge_value = MAX( max_edge_value, edge_value );
  }
  }

//  for( u32 y = 1;  y + 1 < target.img.y;  ++y ) {
//  for( u32 x = 1;  x + 1 < target.img.x;  ++x ) {
//    f32 edge_value = tmp[ x + target.img.stride_x * y ].x / max_edge_value;
//    vec4<f32>& dst = LookupAs( vec4<f32>, target.img, x, y ) );
//    dst = { edge_value, edge_value, edge_value, 1 };
//  }
//  }

  for( u32 y = 1;  y + 1 < target_img.y;  ++y ) {
  for( u32 x = 1;  x + 1 < target_img.x;  ++x ) {

    vec4<f32>& dst = LookupAs( vec4<f32>, tmp, x, y );
    f32 edge_value = dst.x / max_edge_value;

    if( edge_value >= edge_value_threshold ) {
      dst = {};
      Fori( u32, i, 0, 9 ) {
        u32 x_offset = ( i % 3 ) - 1;
        u32 y_offset = ( i / 3 ) - 1;
        vec4<f32>& px_img = LookupAs( vec4<f32>, target_img, x + x_offset, y + y_offset );

        f32 weight = 1.0f / 9.0f;
//        f32 weight = gaussian_kernel[i];
        AddMul( &dst, weight, px_img );
      }
    } else {
      dst = LookupAs( vec4<f32>, target_img, x, y );
    }
  }
  }

  for( u32 y = 1;  y + 1 < target_img.y;  ++y ) {
  for( u32 x = 1;  x + 1 < target_img.x;  ++x ) {
    vec4<f32>& src = LookupAs( vec4<f32>, tmp, x, y );
    vec4<f32>& dst = LookupAs( vec4<f32>, target_img, x, y );
    dst = src;
  }
  }

  Free( tmp );
}











//Inl void
//LineHorizontal( img_t& img, s32 x0, s32 x1, s32 y, d128_t& c )
//{
//  if( !IsInsideY( img, y ) ) {
//    return;
//  }
//
//  if( x0 > x1 ) {
//    SWAP( s32, x0, x1 );
//  }
//  ClampXToInside( img, x0, &x0 );
//  ClampXToInside( img, x1, &x1 );
//  for( s32 x = x0;  x < x1;  ++x ) {
//    Set( img, x, y, c );
//  }
//}
//
//
//Inl void
//LineHorizontalGouraud(
//  img_t& img,
//  s32 x0,
//  s32 x1,
//  s32 y,
//  d128_t& c0,
//  d128_t& c1 )
//{
//  if( !IsInsideY( img, y ) ) {
//    return;
//  }
//
//  if( x0 > x1 ) {
//    SWAP( s32, x0, x1 );
//    SWAP( d128_t, c0, c1 );
//  }
//  ClampXToInside( img, x0, &x0 );
//  ClampXToInside( img, x1, &x1 );
//  d128_t c;
//  for( s32 x = x0;  x < x1;  ++x ) {
//    Lerp_f32x4_from_s32( &c, c0, c1, x, x0, x1 );
//    Set( img, x, y, c );
//  }
//}


//Inl void
//LineHorizontalTex(
//  img_t& img,
//  s32 x0,
//  s32 x1,
//  s32 y,
//  f32 s0,
//  f32 s1,
//  f32 t,
//  img_t& tex )
//{
//  if( x0 > x1 ) {
//    SWAP( s32, x0, x1 );
//    SWAP( f32, s0, s1 );
//  }
//  f32 px_s0 = s0 * tex.x;
//  f32 px_s1 = s1 * tex.x;
//  s32 px_s;
//  s32 px_t;
//  Round_s32_from_f32( &px_t, t * tex.y );
//
//  d256_t c;
//  for( s32 x = x0;  x < x1;  ++x ) {
//    Lerp_s32_from_s32( &px_s, px_s0, px_s1, x, x0, x1 );
//
//    if( !IsInsideX( tex, px_s ) ) {
//      Zero( c ); // TODO: texture border colors.
//    } else {
//      c = tex.m[ px_s + tex.x * px_t ]; // this is nearest sampling. TODO: bilinear sampling.
//    }
//    Set( img, x, y, c );
//  }
//}



//Inl void
//LineVertical(
//  img_t& img,
//  s32 y0,
//  s32 y1,
//  s32 x,
//  d128_t& c )
//{
//  if( !IsInsideX( img, x ) )
//    return;
//
//  if( y0 > y1 ) {
//    SWAP( s32, y0, y1 );
//  }
//  ClampYToInside( img, y0, &y0 );
//  ClampYToInside( img, y1, &y1 );
//  for( s32 y = y0;  y < y1;  ++y ) {
//    Set( img, x, y, c );
//  }
//}
//
//
//
//Inl void
//Line(
//  img_t& img,
//  s32 x0,
//  s32 y0,
//  s32 x1,
//  s32 y1,
//  d128_t& c )
//{
//  s32 dx = ABS( x1 - x0 );
//  s32 dy = ABS( y1 - y0 );
//  s32 sx = ( x0 < x1 )  ?  1  :  -1;
//  s32 sy = ( y0 < y1 )  ?  1  :  -1;
//  s32 err = dx - dy;
//  s32 two_err;
//  s32 x = x0;
//  s32 y = y0;
//  while( x != x1  ||  y != y1 ) {
//    if( IsInside( img, x, y ) ) {
//      Set( img, x, y, c );
//    }
//    two_err = 2 * err;
//    if( two_err > -dy ) {
//      err -= dy;
//      x += sx;
//    }
//    if( two_err < dx ) {
//      err += dx;
//      y += sy;
//    }
//  }
//}
//
//
//
//Inl void
//Rect(
//  img_t& img,
//  s32 x0,
//  s32 y0,
//  s32 x1,
//  s32 y1,
//  d128_t& c )
//{
//  ClampXToInside( img, x0, &x0 );
//  ClampXToInside( img, x1, &x1 );
//  ClampYToInside( img, y0, &y0 );
//  ClampYToInside( img, y1, &y1 );
//  for( s32 x = x0;  x < x1;  ++x ) {
//    for( s32 y = y0;  y < y1;  ++y ) {
//      Set( img, x, y, c );
//    }
//  }
//}
//
//
//Inl void
//RectOutline(
//  img_t& img,
//  s32 x0,
//  s32 y0,
//  s32 x1,
//  s32 y1,
//  d128_t& c )
//{
//  LineHorizontal( img, x0, x1, y0, c );
//  LineHorizontal( img, x0, x1, y1, c );
//  LineVertical( img, y0, y1, x0, c );
//  LineVertical( img, y0, y1, x1, c );
//}
//
//
//
//Inl void
//Tri(
//  img_t& img,
//  s32 x0,
//  s32 y0,
//  s32 x1,
//  s32 y1,
//  s32 x2,
//  s32 y2,
//  d128_t& c )
//{
//  // order the vertices v0, v1, v2 from top to bottom, 0 to 2.
//  if( y0 > y1 ) {
//    SWAP( s32, x0, x1 );
//    SWAP( s32, y0, y1 );
//  }
//  if( y0 > y2 ) {
//    SWAP( s32, x0, x2 );
//    SWAP( s32, y0, y2 );
//  }
//  if( y1 > y2 ) {
//    SWAP( s32, x1, x2 );
//    SWAP( s32, y1, y2 );
//  }
//  if( y0 == y1  &&  y1 == y2 ) {
//    LineHorizontal( img, MIN3( x0, x1, x2 ), MAX3( x0, x1, x2 ), y0, c );
//  } else {
//    s32 xL, xR;
//    for( s32 y = y0;  y < y2;  ++y ) {
//      Lerp_s32_from_s32( &xR, x0, x2, y, y0, y2 );
//      if( y >= y1 ) {
//        Lerp_s32_from_s32( &xL, x1, x2, y, y1, y2 );
//      } else {
//        Lerp_s32_from_s32( &xL, x0, x1, y, y0, y1 );
//      }
//      LineHorizontal( img, xL, xR, y, c );
//    }
//  }
//}
//
//
//Inl void
//TriOutline(
//  img_t& img,
//  s32 x0,
//  s32 y0,
//  s32 x1,
//  s32 y1,
//  s32 x2,
//  s32 y2,
//  d128_t& c )
//{
//  Line( img, x0, y0, x1, y1, c );
//  Line( img, x1, y1, x2, y2, c );
//  Line( img, x2, y2, x0, y0, c );
//}
//
//
//Inl void
//TriGouraud(
//  img_t& img,
//  s32 x0,
//  s32 y0,
//  d128_t& c0,
//  s32 x1,
//  s32 y1,
//  d128_t& c1,
//  s32 x2,
//  s32 y2,
//  d128_t& c2 )
//{
//  // order the vertices V0, V1, V2 from top to bottom, 0 to 2.
//  if( y0 > y1 ) {
//    SWAP( s32, x0, x1 );
//    SWAP( s32, y0, y1 );
//    SWAP( d128_t, c0, c1 );
//  }
//  if( y0 > y2 ) {
//    SWAP( s32, x0, x2 );
//    SWAP( s32, y0, y2 );
//    SWAP( d128_t, c0, c2 );
//  }
//  if( y1 > y2 ) {
//    SWAP( s32, x1, x2 );
//    SWAP( s32, y1, y2 );
//    SWAP( d128_t, c1, c2 );
//  }
//  if( y0 == y1  &&  y1 == y2 ) {
//    // TODO: this is not quite right.
//    LineHorizontalGouraud( img, MIN3( x0, x1, x2 ), MAX3( x0, x1, x2 ), y0, c0, c2 );
//  } else {
//    s32 xL, xR;
//    d128_t cL, cR;
//    for( s32 y = y0;  y < y2;  ++y ) {
//      Lerp_s32_from_s32( &xR, x0, x2, y, y0, y2 );
//      Lerp_f32x4_from_s32( &cR, c0, c2, y, y0, y2 );
//      if( y >= y1 ) {
//        Lerp_s32_from_s32( &xL, x1, x2, y, y1, y2 );
//        Lerp_f32x4_from_s32( &cL, c1, c2, y, y1, y2 );
//      } else {
//        Lerp_s32_from_s32( &xL, x0, x1, y, y0, y1 );
//        Lerp_f32x4_from_s32( &cL, c0, c1, y, y0, y1 );
//      }
//      LineHorizontalGouraud( img, xL, xR, y, cL, cR );
//    }
//  }
//}

