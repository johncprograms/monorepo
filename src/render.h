// Copyright (c) John A. Carlos Jr., all rights reserved.

Enumc( rendermode_t )
{
  ref,
  opt,
  simd,
  ray,
};




Inl void
SampleNearest( vec4<f32>* dst, img_t& texture, vec2<f32>& texel )
{
  auto texel_int = IntegerPixelFromImagespace( texel.x, texel.y, texture.x_m1, texture.y_m1 );
  *dst = LookupAs( vec4<f32>, texture, texel_int.x, texel_int.y );
}
Inl vec4<u8>
SampleNearest_u8x4( img_t& texture, vec2<f32>& texel )
{
  auto texel_int = IntegerPixelFromImagespace( texel.x, texel.y, texture.x_m1, texture.y_m1 );
  return LookupAs( vec4<u8>, texture, texel_int.x, texel_int.y );
}

Inl void
SampleNearest4_u8x4(
  __m128i& dst,
  img_t& texture,
  __m128& texel_x,
  __m128& texel_y,
  __m128i& texture_x_m1,
  __m128i& texture_y_m1
  )
{
  //ProfFunc();
  static __m128i zero = _mm_setzero_si128();

  __m128i texel_int_x = _mm_cvtps_epi32( texel_x );
  __m128i texel_int_y = _mm_cvtps_epi32( texel_y );

  texel_int_x = _mm_clamp_epi32( texel_int_x, zero, texture_x_m1 );
  texel_int_y = _mm_clamp_epi32( texel_int_y, zero, texture_y_m1 );

  auto& px0 = LookupAs( u32, texture, texel_int_x.m128i_i32[0], texel_int_y.m128i_i32[0] );
  auto& px1 = LookupAs( u32, texture, texel_int_x.m128i_i32[1], texel_int_y.m128i_i32[1] );
  auto& px2 = LookupAs( u32, texture, texel_int_x.m128i_i32[2], texel_int_y.m128i_i32[2] );
  auto& px3 = LookupAs( u32, texture, texel_int_x.m128i_i32[3], texel_int_y.m128i_i32[3] );

  dst = _mm_setr_epi32( px0, px1, px2, px3 );
}

Inl vec4<f32>
SampleBilinear( img_t& texture, vec2<f32> texel )
{
  //ProfFunc();
  auto texel_floor = Floor( texel );
  auto texel_ceil = Ceil( texel );

  auto texel00 = IntegerPixelFromImagespace( texel_floor.x, texel_floor.y, texture.x_m1, texture.y_m1 );
  auto texel11 = IntegerPixelFromImagespace( texel_ceil.x, texel_ceil.y, texture.x_m1, texture.y_m1 );

  auto bilinear_u = texel - texel_floor;
  auto bilinear_v = _vec2<f32>( 1 ) - bilinear_u;

  auto sample00 = LookupAs( vec4<f32>, texture, texel00.x, texel00.y );
  auto sample10 = LookupAs( vec4<f32>, texture, texel11.x, texel00.y );
  auto sample11 = LookupAs( vec4<f32>, texture, texel11.x, texel11.y );
  auto sample01 = LookupAs( vec4<f32>, texture, texel00.x, texel11.y );

  return
    sample00 * bilinear_v.x * bilinear_v.y +
    sample10 * bilinear_u.x * bilinear_v.y +
    sample11 * bilinear_u.x * bilinear_u.y +
    sample01 * bilinear_v.x * bilinear_u.y;
}

Inl vec4<f32>
Convert_f32x4_from_u8x4( vec4<u8> src )
{
  static const f32 rec_255 = 1 / 255.0f;
  vec4<f32> r;
  r.x = src.x * rec_255;
  r.y = src.y * rec_255;
  r.z = src.z * rec_255;
  r.w = src.w * rec_255;
  return r;
}

Inl vec4<u8>
Convert_u8x4_from_f32x4( vec4<f32> src )
{
  vec4<u8> r;
  r.x = Cast( u8, 0.5f + src.x * 255.0f );
  r.y = Cast( u8, 0.5f + src.y * 255.0f );
  r.z = Cast( u8, 0.5f + src.z * 255.0f );
  r.w = Cast( u8, 0.5f + src.w * 255.0f );
  return r;
}

Inl vec4<u8>
SampleBilinear_u8x4( img_t& texture, vec2<f32>& texel )
{
  //ProfFunc();
  auto texel_floor = Floor( texel );
  auto texel_ceil = Ceil( texel );

  auto texel00 = IntegerPixelFromImagespace( texel_floor.x, texel_floor.y, texture.x_m1, texture.y_m1 );
  auto texel11 = IntegerPixelFromImagespace( texel_ceil.x, texel_ceil.y, texture.x_m1, texture.y_m1 );

  auto bilinear_u = texel - texel_floor;
  auto bilinear_v = _vec2<f32>( 1 ) - bilinear_u;

  auto sample00 = Convert_f32x4_from_u8x4( LookupAs( vec4<u8>, texture, texel00.x, texel00.y ) );
  auto sample10 = Convert_f32x4_from_u8x4( LookupAs( vec4<u8>, texture, texel11.x, texel00.y ) );
  auto sample11 = Convert_f32x4_from_u8x4( LookupAs( vec4<u8>, texture, texel11.x, texel11.y ) );
  auto sample01 = Convert_f32x4_from_u8x4( LookupAs( vec4<u8>, texture, texel00.x, texel11.y ) );

  auto fdst =
    sample00 * bilinear_v.x * bilinear_v.y +
    sample10 * bilinear_u.x * bilinear_v.y +
    sample11 * bilinear_u.x * bilinear_u.y +
    sample01 * bilinear_v.x * bilinear_u.y;

  return Convert_u8x4_from_f32x4( fdst );
}

Inl void
SampleBilinear4(
  __m128& dst_x,
  __m128& dst_y,
  __m128& dst_z,
  __m128& dst_w,
  img_t& texture,
  __m128& texel_x,
  __m128& texel_y
  )
{
  //ProfFunc();
  static __m128i zero = _mm_setzero_si128();
  static __m128 one = _mm_set1_ps( 1 );

  __m128 texel_x_f = _mm_floor_ps( texel_x );
  __m128 texel_y_f = _mm_floor_ps( texel_y );
  __m128 texel_x_c = _mm_ceil_ps( texel_x );
  __m128 texel_y_c = _mm_ceil_ps( texel_y );

  __m128i max_texel_int_x = _mm_set1_epi32( texture.x_m1 );
  __m128i max_texel_int_y = _mm_set1_epi32( texture.y_m1 );

  __m128i texel00_x = _mm_cvtps_epi32( texel_x_f );
  __m128i texel00_y = _mm_cvtps_epi32( texel_y_f );
  __m128i texel11_x = _mm_cvtps_epi32( texel_x_c );
  __m128i texel11_y = _mm_cvtps_epi32( texel_y_c );

  texel00_x = _mm_clamp_epi32( texel00_x, zero, max_texel_int_x );
  texel00_y = _mm_clamp_epi32( texel00_y, zero, max_texel_int_y );
  texel11_x = _mm_clamp_epi32( texel11_x, zero, max_texel_int_x );
  texel11_y = _mm_clamp_epi32( texel11_y, zero, max_texel_int_y );

  __m128 u_x = _mm_sub_ps( texel_x, texel_x_f );
  __m128 u_y = _mm_sub_ps( texel_y, texel_y_f );
  __m128 v_x = _mm_sub_ps( one, u_x );
  __m128 v_y = _mm_sub_ps( one, u_y );

  __m128 sample00_x = _mm_load_ps( &LookupAs( f32, texture, texel00_x.m128i_i32[0], texel00_y.m128i_i32[0] ) );
  __m128 sample00_y = _mm_load_ps( &LookupAs( f32, texture, texel00_x.m128i_i32[1], texel00_y.m128i_i32[1] ) );
  __m128 sample00_z = _mm_load_ps( &LookupAs( f32, texture, texel00_x.m128i_i32[2], texel00_y.m128i_i32[2] ) );
  __m128 sample00_w = _mm_load_ps( &LookupAs( f32, texture, texel00_x.m128i_i32[3], texel00_y.m128i_i32[3] ) );
  _MM_TRANSPOSE4_PS( sample00_x, sample00_y, sample00_z, sample00_w );

  __m128 sample10_x = _mm_load_ps( &LookupAs( f32, texture, texel11_x.m128i_i32[0], texel00_y.m128i_i32[0] ) );
  __m128 sample10_y = _mm_load_ps( &LookupAs( f32, texture, texel11_x.m128i_i32[1], texel00_y.m128i_i32[1] ) );
  __m128 sample10_z = _mm_load_ps( &LookupAs( f32, texture, texel11_x.m128i_i32[2], texel00_y.m128i_i32[2] ) );
  __m128 sample10_w = _mm_load_ps( &LookupAs( f32, texture, texel11_x.m128i_i32[3], texel00_y.m128i_i32[3] ) );
  _MM_TRANSPOSE4_PS( sample10_x, sample10_y, sample10_z, sample10_w );

  __m128 sample11_x = _mm_load_ps( &LookupAs( f32, texture, texel11_x.m128i_i32[0], texel11_y.m128i_i32[0] ) );
  __m128 sample11_y = _mm_load_ps( &LookupAs( f32, texture, texel11_x.m128i_i32[1], texel11_y.m128i_i32[1] ) );
  __m128 sample11_z = _mm_load_ps( &LookupAs( f32, texture, texel11_x.m128i_i32[2], texel11_y.m128i_i32[2] ) );
  __m128 sample11_w = _mm_load_ps( &LookupAs( f32, texture, texel11_x.m128i_i32[3], texel11_y.m128i_i32[3] ) );
  _MM_TRANSPOSE4_PS( sample11_x, sample11_y, sample11_z, sample11_w );

  __m128 sample01_x = _mm_load_ps( &LookupAs( f32, texture, texel00_x.m128i_i32[0], texel11_y.m128i_i32[0] ) );
  __m128 sample01_y = _mm_load_ps( &LookupAs( f32, texture, texel00_x.m128i_i32[1], texel11_y.m128i_i32[1] ) );
  __m128 sample01_z = _mm_load_ps( &LookupAs( f32, texture, texel00_x.m128i_i32[2], texel11_y.m128i_i32[2] ) );
  __m128 sample01_w = _mm_load_ps( &LookupAs( f32, texture, texel00_x.m128i_i32[3], texel11_y.m128i_i32[3] ) );
  _MM_TRANSPOSE4_PS( sample01_x, sample01_y, sample01_z, sample01_w );

  __m128 v_x_v_y = _mm_mul_ps( v_x, v_y );
  __m128 u_x_v_y = _mm_mul_ps( u_x, v_y );
  __m128 u_x_u_y = _mm_mul_ps( u_x, u_y );
  __m128 v_x_u_y = _mm_mul_ps( v_x, u_y );

  dst_x = _mm_add4_ps( _mm_mul_ps( sample00_x, v_x_v_y ), _mm_mul_ps( sample10_x, u_x_v_y ), _mm_mul_ps( sample11_x, u_x_u_y ), _mm_mul_ps( sample01_x, v_x_u_y ) );
  dst_y = _mm_add4_ps( _mm_mul_ps( sample00_y, v_x_v_y ), _mm_mul_ps( sample10_y, u_x_v_y ), _mm_mul_ps( sample11_y, u_x_u_y ), _mm_mul_ps( sample01_y, v_x_u_y ) );
  dst_z = _mm_add4_ps( _mm_mul_ps( sample00_z, v_x_v_y ), _mm_mul_ps( sample10_z, u_x_v_y ), _mm_mul_ps( sample11_z, u_x_u_y ), _mm_mul_ps( sample01_z, v_x_u_y ) );
  dst_w = _mm_add4_ps( _mm_mul_ps( sample00_w, v_x_v_y ), _mm_mul_ps( sample10_w, u_x_v_y ), _mm_mul_ps( sample11_w, u_x_u_y ), _mm_mul_ps( sample01_w, v_x_u_y ) );
}
Inl void
SampleBilinear4_u8x4(
  __m128& dst,
  img_t& texture,
  __m128& texel_x,
  __m128& texel_y
  )
{
  //ProfFunc();
  static __m128i zero = _mm_setzero_si128();
  static __m128 one = _mm_set1_ps( 1 );


  __m128 texel_x_f = _mm_floor_ps( texel_x );
  __m128 texel_y_f = _mm_floor_ps( texel_y );
  __m128 texel_x_c = _mm_ceil_ps( texel_x );
  __m128 texel_y_c = _mm_ceil_ps( texel_y );

  __m128i max_texel_int_x = _mm_set1_epi32( texture.x_m1 );
  __m128i max_texel_int_y = _mm_set1_epi32( texture.y_m1 );

  __m128i texel00_x = _mm_cvtps_epi32( texel_x_f );
  __m128i texel00_y = _mm_cvtps_epi32( texel_y_f );
  __m128i texel11_x = _mm_cvtps_epi32( texel_x_c );
  __m128i texel11_y = _mm_cvtps_epi32( texel_y_c );

  texel00_x = _mm_clamp_epi32( texel00_x, zero, max_texel_int_x );
  texel00_y = _mm_clamp_epi32( texel00_y, zero, max_texel_int_y );
  texel11_x = _mm_clamp_epi32( texel11_x, zero, max_texel_int_x );
  texel11_y = _mm_clamp_epi32( texel11_y, zero, max_texel_int_y );

  __m128 u_x = _mm_sub_ps( texel_x, texel_x_f );
  __m128 u_y = _mm_sub_ps( texel_y, texel_y_f );
  __m128 v_x = _mm_sub_ps( one, u_x );
  __m128 v_y = _mm_sub_ps( one, u_y );

  __m128 v_x_v_y = _mm_mul_ps( v_x, v_y );
  __m128 u_x_v_y = _mm_mul_ps( u_x, v_y );
  __m128 u_x_u_y = _mm_mul_ps( u_x, u_y );
  __m128 v_x_u_y = _mm_mul_ps( v_x, u_y );

  __m128i c00 = _mm_cvtps_epi32( v_x_v_y );
  __m128i c10 = _mm_cvtps_epi32( u_x_v_y );
  __m128i c11 = _mm_cvtps_epi32( u_x_u_y );
  __m128i c01 = _mm_cvtps_epi32( v_x_u_y );

  u32 px0, px1, px2, px3;

  px0 = LookupAs( u32, texture, texel00_x.m128i_i32[0], texel00_y.m128i_i32[0] );
  px1 = LookupAs( u32, texture, texel00_x.m128i_i32[1], texel00_y.m128i_i32[1] );
  px2 = LookupAs( u32, texture, texel00_x.m128i_i32[2], texel00_y.m128i_i32[2] );
  px3 = LookupAs( u32, texture, texel00_x.m128i_i32[3], texel00_y.m128i_i32[3] );
  __m128i sample00 = _mm_setr_epi32( px0, px1, px2, px3 );

  px0 = LookupAs( u32, texture, texel11_x.m128i_i32[0], texel00_y.m128i_i32[0] );
  px1 = LookupAs( u32, texture, texel11_x.m128i_i32[1], texel00_y.m128i_i32[1] );
  px2 = LookupAs( u32, texture, texel11_x.m128i_i32[2], texel00_y.m128i_i32[2] );
  px3 = LookupAs( u32, texture, texel11_x.m128i_i32[3], texel00_y.m128i_i32[3] );
  __m128i sample10 = _mm_setr_epi32( px0, px1, px2, px3 );

  px0 = LookupAs( u32, texture, texel11_x.m128i_i32[0], texel11_y.m128i_i32[0] );
  px1 = LookupAs( u32, texture, texel11_x.m128i_i32[1], texel11_y.m128i_i32[1] );
  px2 = LookupAs( u32, texture, texel11_x.m128i_i32[2], texel11_y.m128i_i32[2] );
  px3 = LookupAs( u32, texture, texel11_x.m128i_i32[3], texel11_y.m128i_i32[3] );
  __m128i sample11 = _mm_setr_epi32( px0, px1, px2, px3 );

  px0 = LookupAs( u32, texture, texel00_x.m128i_i32[0], texel11_y.m128i_i32[0] );
  px1 = LookupAs( u32, texture, texel00_x.m128i_i32[1], texel11_y.m128i_i32[1] );
  px2 = LookupAs( u32, texture, texel00_x.m128i_i32[2], texel11_y.m128i_i32[2] );
  px3 = LookupAs( u32, texture, texel00_x.m128i_i32[3], texel11_y.m128i_i32[3] );
  __m128i sample01 = _mm_setr_epi32( px0, px1, px2, px3 );

  // TODO: need to multiply bilinear coeffs with the sample pixels, which requires unpacking.
  ImplementCrash();
  //_mm_mullo_epi16

  //dst = _mm_add4_ps( _mm_mul_ps( sample00, v_x_v_y ), _mm_mul_ps( sample10, u_x_v_y ), _mm_mul_ps( sample11, u_x_u_y ), _mm_mul_ps( sample01, v_x_u_y ) );
}




struct
raster_tiles_t
{
  stack_resizeable_cont_t<stack_resizeable_cont_t<idx_t>> tilelist;
  u32 tile_x;
  u32 tile_y;
  vec2<s32> tile_size; // { tile_x, tile_y }
  u32 last_tile_x;
  u32 last_tile_y;
  u32 num_tiles_x;
  u32 num_tiles_y;
  vec2<s32> num_tiles; // { num_tiles_x, num_tiles_y }
};

Inl void
Init( raster_tiles_t& tiles, u32 target_x, u32 target_y, u32 target_stride_x, u32 tile_x, u32 tile_y, idx_t tris_per_tile )
{
  tiles.tile_x = tile_x;
  tiles.tile_y = tile_y;
  tiles.tile_size = { Cast( s32, tile_x ), Cast( s32, tile_y ) };

  tiles.last_tile_x = ( target_stride_x % tile_x );
  tiles.last_tile_y = ( target_y % tile_y );

  AssertCrash( tiles.tile_x % 4 == 0 );
  AssertCrash( tiles.last_tile_x % 4 == 0 ); // allocated image is padded so we can just assume junk.

  tiles.num_tiles_x = ( target_stride_x / tile_x ) + ( ( tiles.last_tile_x > 0 )  ?  1  :  0 );
  tiles.num_tiles_y = ( target_y / tile_y ) + ( ( tiles.last_tile_y > 0 )  ?  1  :  0 );

  AssertCrash( tiles.num_tiles_x <= MAX_s32 );
  AssertCrash( tiles.num_tiles_y <= MAX_s32 );
  tiles.num_tiles = { Cast( s32, tiles.num_tiles_x ), Cast( s32, tiles.num_tiles_y ) };

  idx_t num_tiles_total = tiles.num_tiles_x * tiles.num_tiles_y;
  Alloc( tiles.tilelist, num_tiles_total );
  tiles.tilelist.len = num_tiles_total;

  For( i, 0, num_tiles_total ) {
    Alloc( tiles.tilelist.mem[i], tris_per_tile );
  }
}

Inl void
Kill( raster_tiles_t& tiles )
{
  ForLen( i, tiles.tilelist ) {
    Free( tiles.tilelist.mem[i] );
  }
  Free( tiles.tilelist );

  tiles = {};
}

Inl void
ClearTiles( raster_tiles_t& tiles )
{
  ForLen( i, tiles.tilelist ) {
    tiles.tilelist.mem[i].len = 0;
  }
}

Inl stack_resizeable_cont_t<idx_t>&
Lookup( raster_tiles_t& tiles, s32 tile_pos_x, s32 tile_pos_y )
{
  s32 tile_idx = tile_pos_x + tile_pos_y * tiles.num_tiles.x;
  return tiles.tilelist.mem[tile_idx];
}

Inl u32
TileW( raster_tiles_t& tiles, s32 tile_pos_x )
{
  u32 tile_w = ( ( tile_pos_x + 1 == tiles.num_tiles.x )  &&  ( tiles.last_tile_x > 0 ) )  ?  tiles.last_tile_x  :  tiles.tile_x;
  return tile_w;
}

Inl u32
TileH( raster_tiles_t& tiles, s32 tile_pos_y )
{
  u32 tile_h = ( ( tile_pos_y + 1 == tiles.num_tiles.y )  &&  ( tiles.last_tile_y > 0 ) )  ?  tiles.last_tile_y  :  tiles.tile_y;
  return tile_h;
}


void
BinTriangles(
  raster_tiles_t& raster_tiles,
  img_t& target_img,
  stack_resizeable_cont_t<vec4<f32>> pos_scratch,
  stack_resizeable_cont_t<u32> idxs_scratch
  )
{
  // TODO: do we need to do this axis scaling?
  //   it's intended to make precision more uniform across the screen.
  f32 axis_scale = 1.0f / MAX( target_img.xf, target_img.yf );

  Forinc( idx_t, i, 0, idxs_scratch.len, 3 ) {
    u32 idx0 = idxs_scratch.mem[i+0];
    u32 idx1 = idxs_scratch.mem[i+1];
    u32 idx2 = idxs_scratch.mem[i+2];
    auto& pos0 = pos_scratch.mem[idx0];
    auto& pos1 = pos_scratch.mem[idx1];
    auto& pos2 = pos_scratch.mem[idx2];

    vec2<f32> p0 = { pos0.x, pos0.y };
    vec2<f32> p1 = { pos1.x, pos1.y };
    vec2<f32> p2 = { pos2.x, pos2.y };
    p0 *= axis_scale;
    p1 *= axis_scale;
    p2 *= axis_scale;

    auto e0 = p1 - p0;
    auto e1 = p2 - p1;
    auto e2 = p0 - p2;

    auto e0perp = Normalize( Perp( e0 ) );
    auto e1perp = Normalize( Perp( e1 ) );
    auto e2perp = Normalize( Perp( e2 ) );

    auto tri_t0 = -Dot( e0perp, e2 );
    auto tri_t1 = -Dot( e1perp, e0 );
    auto tri_t2 = -Dot( e2perp, e1 );

    auto tri_min_t0 = MIN( 0, tri_t0 );
    auto tri_max_t0 = MAX( 0, tri_t0 );

    auto tri_min_t1 = MIN( 0, tri_t1 );
    auto tri_max_t1 = MAX( 0, tri_t1 );

    auto tri_min_t2 = MIN( 0, tri_t2 );
    auto tri_max_t2 = MAX( 0, tri_t2 );

    vec2<f32> pos_min, pos_max;
    pos_min.x = MIN3( pos0.x, pos1.x, pos2.x );
    pos_min.y = MIN3( pos0.y, pos1.y, pos2.y );
    pos_max.x = MAX3( pos0.x, pos1.x, pos2.x );
    pos_max.y = MAX3( pos0.y, pos1.y, pos2.y );

    auto px_min = IntegerPixelFromImagespace( pos_min.x, pos_min.y, target_img.x_m1, target_img.y_m1 );
    auto px_max = IntegerPixelFromImagespace( pos_max.x, pos_max.y, target_img.x_m1, target_img.y_m1 );

    auto tile_min = px_min / raster_tiles.tile_size;
    auto tile_max = px_max / raster_tiles.tile_size;

    pos_min *= axis_scale;
    pos_max *= axis_scale;

    // Don't waste time doing the sat overlap test if the triangle doesn't span >1 tile in both dims!
    s32 tile_span_x = ( tile_max.x - tile_min.x + 1 );
    s32 tile_span_y = ( tile_max.y - tile_min.y + 1 );
    bool do_sat_overlap = ( tile_span_x > 1 )  &&  ( tile_span_y > 1 );

    Fori( s32, tile_pos_y, tile_min.y, tile_max.y + 1 ) {
    Fori( s32, tile_pos_x, tile_min.x, tile_max.x + 1 ) {

      if( do_sat_overlap ) {
        // Check if the tile and triangle overlap. If they do, then make sure the tile has a reference to the triangle.
        vec2<f32> b00 = { Cast( f32, tile_pos_x * raster_tiles.tile_size.x ), Cast( f32, tile_pos_y * raster_tiles.tile_size.y ) };

        s32 tile_w = TileW( raster_tiles, tile_pos_x );
        s32 tile_h = TileH( raster_tiles, tile_pos_y );
        vec2<f32> b11 = { b00.x + tile_w, b00.y + tile_h };

        b00 *= axis_scale;
        b11 *= axis_scale;

        vec2<f32> b10 = { b11.x, b00.y };
        vec2<f32> b01 = { b00.x, b11.y };

        static f32 epsilon = 1e-5f;

        bool axis_box_x = IntervalsOverlap( pos_min.x, pos_max.x, b00.x, b11.x, epsilon );
        bool axis_box_y = IntervalsOverlap( pos_min.y, pos_max.y, b00.y, b11.y, epsilon );

        if( !axis_box_x  ||  !axis_box_y ) {
          continue;
        }

        auto c000 = b00 - p0;
        auto c010 = b10 - p0;
        auto c011 = b11 - p0;
        auto c001 = b01 - p0;

        auto c100 = b00 - p1;
        auto c110 = b10 - p1;
        auto c111 = b11 - p1;
        auto c101 = b01 - p1;

        auto c200 = b00 - p2;
        auto c210 = b10 - p2;
        auto c211 = b11 - p2;
        auto c201 = b01 - p2;

        auto box_t000 = Dot( e0perp, c000 );
        auto box_t010 = Dot( e0perp, c010 );
        auto box_t011 = Dot( e0perp, c011 );
        auto box_t001 = Dot( e0perp, c001 );

        auto box_t100 = Dot( e1perp, c100 );
        auto box_t110 = Dot( e1perp, c110 );
        auto box_t111 = Dot( e1perp, c111 );
        auto box_t101 = Dot( e1perp, c101 );

        auto box_t200 = Dot( e2perp, c200 );
        auto box_t210 = Dot( e2perp, c210 );
        auto box_t211 = Dot( e2perp, c211 );
        auto box_t201 = Dot( e2perp, c201 );

        auto box_min_t0 = MIN4( box_t000, box_t010, box_t011, box_t001 );
        auto box_max_t0 = MAX4( box_t000, box_t010, box_t011, box_t001 );

        auto box_min_t1 = MIN4( box_t100, box_t110, box_t111, box_t101 );
        auto box_max_t1 = MAX4( box_t100, box_t110, box_t111, box_t101 );

        auto box_min_t2 = MIN4( box_t200, box_t210, box_t211, box_t201 );
        auto box_max_t2 = MAX4( box_t200, box_t210, box_t211, box_t201 );

        bool axis_tri_0 = IntervalsOverlap( tri_min_t0, tri_max_t0, box_min_t0, box_max_t0, epsilon );
        bool axis_tri_1 = IntervalsOverlap( tri_min_t1, tri_max_t1, box_min_t1, box_max_t1, epsilon );
        bool axis_tri_2 = IntervalsOverlap( tri_min_t2, tri_max_t2, box_min_t2, box_max_t2, epsilon );

        if( !axis_tri_0  ||  !axis_tri_1  ||  !axis_tri_2 ) {
          continue;
        }
      }

      auto& tile = Lookup( raster_tiles, tile_pos_x, tile_pos_y );
      *AddBack( tile ) = i;
    }
    }
  }
}



Inl bool
OnClipPlaneGoodSide(
  idx_t plane_idx,
  vec4<f32>& pos
  )
{
  switch( plane_idx ) {
    case 0: {  return (  pos.x + pos.w >= 0 );  } break;
    case 1: {  return ( -pos.x + pos.w >= 0 );  } break;
    case 2: {  return (  pos.y + pos.w >= 0 );  } break;
    case 3: {  return ( -pos.y + pos.w >= 0 );  } break;
    case 4: {  return ( -pos.z         >= 0 );  } break;
    case 5: {  return (  pos.z + pos.w >= 0 );  } break;

    default: UnreachableCrash();
  }
  return {};
}

Inl f32
IntersectLineWithClipPlane(
  idx_t plane_idx,
  vec4<f32>& v0,
  vec4<f32>& ei
  )
{
  switch( plane_idx ) {
    case 0: {  return -(  v0.x + v0.w ) / (  ei.x + ei.w );  } break;
    case 1: {  return -( -v0.x + v0.w ) / ( -ei.x + ei.w );  } break;
    case 2: {  return -(  v0.y + v0.w ) / (  ei.y + ei.w );  } break;
    case 3: {  return -( -v0.y + v0.w ) / ( -ei.y + ei.w );  } break;
    case 4: {  return -(  v0.z        ) / (  ei.z        );  } break;
    case 5: {  return -(  v0.z + v0.w ) / (  ei.z + ei.w );  } break;

    default: UnreachableCrash();
  }
  return {};
}



Inl void
ClipMeshAgainstClipPlane(
  idx_t plane_idx,
  stack_resizeable_cont_t<vec4<f32>>& poss,
  stack_resizeable_cont_t<vec2<f32>>& tcs,
  stack_resizeable_cont_t<u32>& idxs
  )
{
  idx_t i = 0;
  while( i < idxs.len ) {

    idx_t i0 = i + 0;
    idx_t i1 = i + 1;
    idx_t i2 = i + 2;
    u32 idx0 = idxs.mem[i0];
    u32 idx1 = idxs.mem[i1];
    u32 idx2 = idxs.mem[i2];

    bool on_good_side[3];
    on_good_side[0] = OnClipPlaneGoodSide( plane_idx, poss.mem[idx0] );
    on_good_side[1] = OnClipPlaneGoodSide( plane_idx, poss.mem[idx1] );
    on_good_side[2] = OnClipPlaneGoodSide( plane_idx, poss.mem[idx2] );

    idx_t num_good = ( on_good_side[0] + on_good_side[1] + on_good_side[2] );

    if( !num_good ) {
      // Throw away tri.

      // Overwrite the current triangle with the last triangle, and decrement the triangle count.

      AssertCrash( idxs.len >= 3 );

      idx_t i_last = idxs.len - 3;

      idxs.mem[i0] = idxs.mem[i_last+0];
      idxs.mem[i1] = idxs.mem[i_last+1];
      idxs.mem[i2] = idxs.mem[i_last+2];

      idxs.len -= 3;

      // Don't increment i, so we can clip the just-moved triangle.

    } elif( num_good == 1 ) {

      if( on_good_side[0] ) {

      } elif( on_good_side[1] ) {
        // Permute triangle ( 0, 1, 2 ) so we get ( 1, 2, 0 ). This maintains winding.
        PERMUTELEFT3( idx_t, i0, i1, i2 );
        PERMUTELEFT3( u32, idx0, idx1, idx2 );

      } else {
        AssertCrash( on_good_side[2] );

        // Permute triangle ( 0, 1, 2 ) so we get ( 2, 0, 1 ). This maintains winding.
        PERMUTERIGHT3( idx_t, i0, i1, i2 );
        PERMUTERIGHT3( u32, idx0, idx1, idx2 );
      }

      auto& pos0 = poss.mem[idx0];
      auto& pos1 = poss.mem[idx1];
      auto& pos2 = poss.mem[idx2];
      auto& tc0 = tcs.mem[idx0];
      auto& tc1 = tcs.mem[idx1];
      auto& tc2 = tcs.mem[idx2];

      // We need to introduce two new vertices, pos3/tc3 and pos4/tc4.
      // We need to modify the triangle to use these vertices instead of the bad ones.

      auto e0 = pos1 - pos0;
      auto e1 = pos2 - pos0;

      f32 t0 = IntersectLineWithClipPlane( plane_idx, pos0, e0 );
      f32 t1 = IntersectLineWithClipPlane( plane_idx, pos0, e1 );
      //AssertWarn( 0 <= t0  &&  t0 <= 1 );
      //AssertWarn( 0 <= t1  &&  t1 <= 1 );
      t0 = CLAMP( t0, 0, 1 );
      t1 = CLAMP( t1, 0, 1 );

      auto pos3 = pos0 + t0 * e0;
      auto pos4 = pos0 + t1 * e1;

      auto tc3 = Lerp_from_f32( tc0, tc1, t0, 0, 1 );
      auto tc4 = Lerp_from_f32( tc0, tc2, t1, 0, 1 );

      AssertCrash( poss.len <= MAX_u32 - 2 );
      u32 wr_idx3 = Cast( u32, poss.len );
      u32 wr_idx4 = wr_idx3 + 1;

      *AddBack( poss ) = pos3;
      *AddBack( poss ) = pos4;

      *AddBack( tcs ) = tc3;
      *AddBack( tcs ) = tc4;

      // Modify the two bad vertices in the tri.
      idxs.mem[i1] = wr_idx3;
      idxs.mem[i2] = wr_idx4;

      i += 3;

    } elif( num_good == 2 ) {

      if( !on_good_side[0] ) {

      } elif( !on_good_side[1] ) {
        // Permute triangle ( 0, 1, 2 ) so we get ( 1, 2, 0 ). This maintains winding.
        PERMUTELEFT3( idx_t, i0, i1, i2 );
        PERMUTELEFT3( u32, idx0, idx1, idx2 );

      } else {
        AssertCrash( !on_good_side[2] );

        // Permute triangle ( 0, 1, 2 ) so we get ( 2, 0, 1 ). This maintains winding.
        PERMUTERIGHT3( idx_t, i0, i1, i2 );
        PERMUTERIGHT3( u32, idx0, idx1, idx2 );
      }

      auto& pos0 = poss.mem[idx0];
      auto& pos1 = poss.mem[idx1];
      auto& pos2 = poss.mem[idx2];
      auto& tc0 = tcs.mem[idx0];
      auto& tc1 = tcs.mem[idx1];
      auto& tc2 = tcs.mem[idx2];

      // We need to introduce two new vertices, pos3/tc3 and pos4/tc4.
      // We need to modify one triangle, and introduce another triangle using these vertices.
      // The two triangles are given as: ( 4, 1, 2 ), ( 3, 1, 4 )

      auto e0 = pos1 - pos0;
      auto e1 = pos2 - pos0;

      f32 t0 = IntersectLineWithClipPlane( plane_idx, pos0, e0 );
      f32 t1 = IntersectLineWithClipPlane( plane_idx, pos0, e1 );
      //AssertWarn( 0 <= t0  &&  t0 <= 1 );
      //AssertWarn( 0 <= t1  &&  t1 <= 1 );
      t0 = CLAMP( t0, 0, 1 );
      t1 = CLAMP( t1, 0, 1 );

      auto pos3 = pos0 + t0 * e0;
      auto pos4 = pos0 + t1 * e1;

      auto tc3 = Lerp_from_f32( tc0, tc1, t0, 0, 1 );
      auto tc4 = Lerp_from_f32( tc0, tc2, t1, 0, 1 );

      AssertCrash( poss.len <= MAX_u32 - 2 );
      u32 wr_idx3 = Cast( u32, poss.len );
      u32 wr_idx4 = wr_idx3 + 1;

      *AddBack( poss ) = pos3;
      *AddBack( poss ) = pos4;

      *AddBack( tcs ) = tc3;
      *AddBack( tcs ) = tc4;

      // Replace ( 0, 1, 2 ) triangle with ( 4, 1, 2 ) triangle.
      idxs.mem[i0] = wr_idx4;

      if( i + 6 <= idxs.len ) {
        // If there's a next triangle, then:

        // Move the next triangle to the back.
        u32 next_idx0 = idxs.mem[i+3];
        u32 next_idx1 = idxs.mem[i+4];
        u32 next_idx2 = idxs.mem[i+5];
        *AddBack( idxs ) = next_idx0;
        *AddBack( idxs ) = next_idx1;
        *AddBack( idxs ) = next_idx2;

        // Overwrite the next triangle with ( 3, 1, 4 ) triangle.
        idxs.mem[i+3] = wr_idx3;
        idxs.mem[i+4] = idx1;
        idxs.mem[i+5] = wr_idx4;

      } else {
        *AddBack( idxs ) = wr_idx3;
        *AddBack( idxs ) = idx1;
        *AddBack( idxs ) = wr_idx4;
      }

      // Skip over the triangle we added, since it's already guaranteed to be clipped.
      // It may also cause problems if we tried to clip it again, because of float precision.
      i += 6;

    } else {
      // Keep tri.
      i += 3;
    }
  }
}




struct
mesh_t
{
  img_t* texture;
  vec4<f32>* positions;
  vec2<f32>* texcoords;
  idx_t vert_len;
  u32* idxs;
  idx_t idxs_len;
  mat3x3r<f32> rotation_world_from_model;
  mat3x3r<f32> rotation_model_from_world;
  f32 scale_world_from_model;
  f32 scale_model_from_world;
  vec3<f32> translation;
};



void
RasterReference(
  raster_tiles_t& raster_tiles,
  img_t& target_img,
  img_t& target_dep,
  img_t& texture,
  stack_resizeable_cont_t<vec4<f32>> pos_scratch,
  stack_resizeable_cont_t<vec2<f32>> tc_scratch,
  stack_resizeable_cont_t<u32> idxs_scratch
  )
{
  ProfFunc();

  vec2<f32> texture_dim_m1 = { texture.xf - 1.0f, texture.yf - 1.0f };
  vec2<f32> texture_dim = { texture.xf, texture.yf };

  Fori( s32, tile_pos_y, 0, raster_tiles.num_tiles.y ) {
  Fori( s32, tile_pos_x, 0, raster_tiles.num_tiles.x ) {

    auto& tile = Lookup( raster_tiles, tile_pos_x, tile_pos_y );

    s32 tile_x0 = tile_pos_x * raster_tiles.tile_x;
    s32 tile_y0 = tile_pos_y * raster_tiles.tile_y;
    s32 tile_x1 = tile_x0 + TileW( raster_tiles, tile_pos_x );
    s32 tile_y1 = tile_y0 + TileH( raster_tiles, tile_pos_y );

    ForLen( i, tile ) {

      idx_t i0 = tile.mem[i];
      idx_t i1 = i0 + 1;
      idx_t i2 = i0 + 2;

      u32 idx0 = idxs_scratch.mem[i0];
      u32 idx1 = idxs_scratch.mem[i1];
      u32 idx2 = idxs_scratch.mem[i2];
      auto& pos0 = pos_scratch.mem[idx0];
      auto& pos1 = pos_scratch.mem[idx1];
      auto& pos2 = pos_scratch.mem[idx2];
      auto& tc0 = tc_scratch.mem[idx0];
      auto& tc1 = tc_scratch.mem[idx1];
      auto& tc2 = tc_scratch.mem[idx2];

      auto v0 = IntegerPixelFromImagespace( pos0.x, pos0.y, target_img.x_m1, target_img.y_m1 );
      auto v1 = IntegerPixelFromImagespace( pos1.x, pos1.y, target_img.x_m1, target_img.y_m1 );
      auto v2 = IntegerPixelFromImagespace( pos2.x, pos2.y, target_img.x_m1, target_img.y_m1 );

      auto e0 = v1 - v0;
      auto e1 = v2 - v1;
      auto e2 = v0 - v2;

      auto twice_tri_area = CrossZ( e0, e1 );
      f32 rec_twice_tri_area = 1 / Cast( f32, twice_tri_area );

//      bool topleft0 = ( e0.y < 0 )  |  ( ( e0.y == 0 )  &  ( e0.x < 0 ) );
//      bool topleft1 = ( e1.y < 0 )  |  ( ( e1.y == 0 )  &  ( e1.x < 0 ) );
//      bool topleft2 = ( e2.y < 0 )  |  ( ( e2.y == 0 )  &  ( e2.x < 0 ) );
      s32 fill_bias0 = 0;//topleft0  ?  0  :  -1;
      s32 fill_bias1 = 0;//topleft1  ?  0  :  -1;
      s32 fill_bias2 = 0;//topleft2  ?  0  :  -1;

      auto C0 = CrossZ( v0, v1 );
      auto C1 = CrossZ( v1, v2 );
      auto C2 = CrossZ( v2, v0 );

      auto A0 = -e0.y;
      auto A1 = -e1.y;
      auto A2 = -e2.y;

      auto B0 = e0.x;
      auto B1 = e1.x;
      auto B2 = e2.x;

      auto delta_pos1 = pos1 - pos0;
      auto delta_pos2 = pos2 - pos0;

      auto delta_tc1 = tc1 - tc0;
      auto delta_tc2 = tc2 - tc0;

      Fori( s32, y, tile_y0, tile_y1 ) {
      Fori( s32, x, tile_x0, tile_x1 ) {

        s32 E0 = A0 * x + B0 * y + C0 + fill_bias0;
        s32 E1 = A1 * x + B1 * y + C1 + fill_bias1;
        s32 E2 = A2 * x + B2 * y + C2 + fill_bias2;

        bool inside_tri = ( ( E0 | E1 | E2 ) >= 0 );
        if( inside_tri ) {
          //f32 coeff0 = E1 * rec_twice_tri_area;
          f32 coeff1 = E2 * rec_twice_tri_area;
          f32 coeff2 = E0 * rec_twice_tri_area;

          auto pos = pos0 + coeff1 * delta_pos1 + coeff2 * delta_pos2;

          auto& depth = LookupAs( f32, target_dep, x, y );
          if( pos.w >= depth ) {
            auto tc = tc0 + coeff1 * delta_tc1 + coeff2 * delta_tc2;
            tc /= pos.w;
            tc = CLAMP( tc, 0.0f, 1.0f );

            // ====================================================================
            // Shading calculations:

            //auto texel = tc * texture_dim_m1;
            auto texel = tc * texture_dim - _vec2<f32>( 0.5f );

            //SampleBilinear( &src, texture, texel );
            //SampleNearest( &src, texture, texel );
            auto src = SampleNearest_u8x4( texture, texel );

            //u8 src_fac = src.w;
            //u8 dst_fac = 255 - src.w;

            auto& dst = LookupAs( vec4<u8>, target_img, x, y );

            //Mul( &dst, dst_fac );
            //AddMul( &dst, src_fac, src );
            dst = src;

            depth = pos.w;
          }
        }
      }
      }
    }
  }
  }
}


void
RasterOptimized(
  raster_tiles_t& raster_tiles,
  img_t& target_img,
  img_t& target_dep,
  img_t& texture,
  stack_resizeable_cont_t<vec4<f32>> pos_scratch,
  stack_resizeable_cont_t<vec2<f32>> tc_scratch,
  stack_resizeable_cont_t<u32> idxs_scratch
  )
{
  ProfFunc();

  vec2<f32> texture_dim = { texture.xf, texture.yf };

  Fori( s32, tile_pos_y, 0, raster_tiles.num_tiles.y ) {
  Fori( s32, tile_pos_x, 0, raster_tiles.num_tiles.x ) {

    auto& tile = Lookup( raster_tiles, tile_pos_x, tile_pos_y );

    s32 tile_x0 = tile_pos_x * raster_tiles.tile_x;
    s32 tile_y0 = tile_pos_y * raster_tiles.tile_y;
    s32 tile_x1 = tile_x0 + TileW( raster_tiles, tile_pos_x );
    s32 tile_y1 = tile_y0 + TileH( raster_tiles, tile_pos_y );

    ForLen( i, tile ) {

      idx_t i0 = tile.mem[i];
      idx_t i1 = i0 + 1;
      idx_t i2 = i0 + 2;

      u32 idx0 = idxs_scratch.mem[i0];
      u32 idx1 = idxs_scratch.mem[i1];
      u32 idx2 = idxs_scratch.mem[i2];
      auto& pos0 = pos_scratch.mem[idx0];
      auto& pos1 = pos_scratch.mem[idx1];
      auto& pos2 = pos_scratch.mem[idx2];
      auto& tc0 = tc_scratch.mem[idx0];
      auto& tc1 = tc_scratch.mem[idx1];
      auto& tc2 = tc_scratch.mem[idx2];

      auto v0 = IntegerPixelFromImagespace( pos0.x, pos0.y, target_img.x_m1, target_img.y_m1 );
      auto v1 = IntegerPixelFromImagespace( pos1.x, pos1.y, target_img.x_m1, target_img.y_m1 );
      auto v2 = IntegerPixelFromImagespace( pos2.x, pos2.y, target_img.x_m1, target_img.y_m1 );

      auto e0 = v1 - v0;
      auto e1 = v2 - v1;
      auto e2 = v0 - v2;

      auto twice_tri_area = CrossZ( e0, e1 );
      f32 rec_twice_tri_area = 1 / Cast( f32, twice_tri_area );

      bool topleft0 = ( e0.y < 0 )  |  ( ( e0.y == 0 )  &  ( e0.x < 0 ) );
      bool topleft1 = ( e1.y < 0 )  |  ( ( e1.y == 0 )  &  ( e1.x < 0 ) );
      bool topleft2 = ( e2.y < 0 )  |  ( ( e2.y == 0 )  &  ( e2.x < 0 ) );
      s32 fill_bias0 = topleft0  ?  0  :  -1;
      s32 fill_bias1 = topleft1  ?  0  :  -1;
      s32 fill_bias2 = topleft2  ?  0  :  -1;

      auto C0 = CrossZ( v0, v1 );
      auto C1 = CrossZ( v1, v2 );
      auto C2 = CrossZ( v2, v0 );

      auto A0 = -e0.y;
      auto A1 = -e1.y;
      auto A2 = -e2.y;

      auto B0 = e0.x;
      auto B1 = e1.x;
      auto B2 = e2.x;

      auto delta_pos1 = pos1 - pos0;
      auto delta_pos2 = pos2 - pos0;

      auto delta_tc1 = tc1 - tc0;
      auto delta_tc2 = tc2 - tc0;

      // starting values of the edge functions E_i
      s32 row_E0 = A0 * tile_x0 + B0 * tile_y0 + C0 + fill_bias0;
      s32 row_E1 = A1 * tile_x0 + B1 * tile_y0 + C1 + fill_bias1;
      s32 row_E2 = A2 * tile_x0 + B2 * tile_y0 + C2 + fill_bias2;

      Fori( s32, y, tile_y0, tile_y1 ) {

        s32 E0 = row_E0;
        s32 E1 = row_E1;
        s32 E2 = row_E2;

        Fori( s32, x, tile_x0, tile_x1 ) {

          //Prof( raster_perpixel );

          bool inside_tri = ( ( E0 | E1 | E2 ) >= 0 );
          if( inside_tri ) {
            //f32 coeff0 = E1 * rec_twice_tri_area;
            f32 coeff1 = E2 * rec_twice_tri_area;
            f32 coeff2 = E0 * rec_twice_tri_area;

            // pos = sum{ coeff_i * pos_i }
            auto pos = pos0 + coeff1 * delta_pos1 + coeff2 * delta_pos2;

            // depth test.
            auto& depth = LookupAs( f32, target_dep, x, y );
            if( pos.w >= depth ) {

              // tc = sum{ coeff_i * tc_i }
              auto tc = tc0 + coeff1 * delta_tc1 + coeff2 * delta_tc2;

              // Convert the interpolated s/w, t/w back into s, t.
              tc /= pos.w;

              // CLAMP to valid range.
              tc = CLAMP( tc, 0.0f, 1.0f );

              // ====================================================================
              // Shading calculations:

              auto texel = tc * texture_dim - _vec2<f32>( 0.5f );

              //SampleBilinear( &src, texture, texel );
              //SampleNearest( &src, texture, texel );
              auto src = SampleNearest_u8x4( texture, texel );

              //u8 src_fac = src.w;
              //u8 dst_fac = 255 - src.w;

              auto& dst = LookupAs( vec4<u8>, target_img, x, y );

              //Mul( &dst, dst_fac );
              //AddMul( &dst, src_fac, src );
              dst = src;

              depth = pos.w;
            }
          }

          E0 += A0;
          E1 += A1;
          E2 += A2;
        }

        row_E0 += B0;
        row_E1 += B1;
        row_E2 += B2;
      }
    }
  }
  }
}



void
RasterOptimizedSimd(
  raster_tiles_t& raster_tiles,
  img_t& target_img,
  img_t& target_dep,
  img_t& texture,
  stack_resizeable_cont_t<vec4<f32>> pos_scratch,
  stack_resizeable_cont_t<vec2<f32>> tc_scratch,
  stack_resizeable_cont_t<u32> idxs_scratch
  )
{
  ProfFunc();

  __m128 texture_x_m1 = _mm_set1_ps( texture.xf );
  __m128 texture_y_m1 = _mm_set1_ps( texture.yf );
  __m128i texture_xi_m1 = _mm_set1_epi32( texture.x_m1 );
  __m128i texture_yi_m1 = _mm_set1_epi32( texture.y_m1 );

  static __m128 zero = _mm_setzero_ps();
  static __m128 one = _mm_set1_ps( 1 );
  static __m128i zeroi = _mm_setzero_si128();
  static __m128i onei = _mm_set1_epi32( 1 );
  static __m128i sign_biti = _mm_set1_epi32( 1 << 31 );

  Fori( s32, tile_pos_y, 0, raster_tiles.num_tiles.y ) {
  Fori( s32, tile_pos_x, 0, raster_tiles.num_tiles.x ) {

    auto& tile = Lookup( raster_tiles, tile_pos_x, tile_pos_y );

    s32 tile_x0 = tile_pos_x * raster_tiles.tile_x;
    s32 tile_y0 = tile_pos_y * raster_tiles.tile_y;
    s32 tile_x1 = tile_x0 + TileW( raster_tiles, tile_pos_x );
    s32 tile_y1 = tile_y0 + TileH( raster_tiles, tile_pos_y );

    ForLen( i, tile ) {

      idx_t i0 = tile.mem[i];
      idx_t i1 = i0 + 1;
      idx_t i2 = i0 + 2;

      u32 idx0 = idxs_scratch.mem[i0];
      u32 idx1 = idxs_scratch.mem[i1];
      u32 idx2 = idxs_scratch.mem[i2];
      auto& pos0 = pos_scratch.mem[idx0];
      auto& pos1 = pos_scratch.mem[idx1];
      auto& pos2 = pos_scratch.mem[idx2];
      auto& tc0 = tc_scratch.mem[idx0];
      auto& tc1 = tc_scratch.mem[idx1];
      auto& tc2 = tc_scratch.mem[idx2];

      Prof( raster_simd_trisetup );

      auto v0 = IntegerPixelFromImagespace( pos0.x, pos0.y, target_img.x_m1, target_img.y_m1 );
      auto v1 = IntegerPixelFromImagespace( pos1.x, pos1.y, target_img.x_m1, target_img.y_m1 );
      auto v2 = IntegerPixelFromImagespace( pos2.x, pos2.y, target_img.x_m1, target_img.y_m1 );

      auto e0 = v1 - v0;
      auto e1 = v2 - v1;
      auto e2 = v0 - v2;

      auto twice_tri_area = CrossZ( e0, e1 );
      f32 rec_twice_tri_area = 1 / Cast( f32, twice_tri_area );
      __m128 rec_twice_tri_area4 = _mm_set1_ps( rec_twice_tri_area );

      bool topleft0 = ( e0.y < 0 )  |  ( ( e0.y == 0 )  &  ( e0.x < 0 ) );
      bool topleft1 = ( e1.y < 0 )  |  ( ( e1.y == 0 )  &  ( e1.x < 0 ) );
      bool topleft2 = ( e2.y < 0 )  |  ( ( e2.y == 0 )  &  ( e2.x < 0 ) );
      s32 fill_bias0 = topleft0  ?  0  :  -1;
      s32 fill_bias1 = topleft1  ?  0  :  -1;
      s32 fill_bias2 = topleft2  ?  0  :  -1;

      auto C0 = CrossZ( v0, v1 );
      auto C1 = CrossZ( v1, v2 );
      auto C2 = CrossZ( v2, v0 );

      auto A0 = -e0.y;
      auto A1 = -e1.y;
      auto A2 = -e2.y;

      auto B0 = e0.x;
      auto B1 = e1.x;
      auto B2 = e2.x;

      __m128 delta_pos1_x = _mm_set1_ps( pos1.x - pos0.x );
      __m128 delta_pos1_y = _mm_set1_ps( pos1.y - pos0.y );
      __m128 delta_pos1_z = _mm_set1_ps( pos1.z - pos0.z );
      __m128 delta_pos1_w = _mm_set1_ps( pos1.w - pos0.w );

      __m128 delta_pos2_x = _mm_set1_ps( pos2.x - pos0.x );
      __m128 delta_pos2_y = _mm_set1_ps( pos2.y - pos0.y );
      __m128 delta_pos2_z = _mm_set1_ps( pos2.z - pos0.z );
      __m128 delta_pos2_w = _mm_set1_ps( pos2.w - pos0.w );

      __m128 tc0_x = _mm_set1_ps( tc0.x );
      __m128 tc0_y = _mm_set1_ps( tc0.y );

      __m128 delta_tc1_x = _mm_set1_ps( tc1.x - tc0.x );
      __m128 delta_tc1_y = _mm_set1_ps( tc1.y - tc0.y );

      __m128 delta_tc2_x = _mm_set1_ps( tc2.x - tc0.x );
      __m128 delta_tc2_y = _mm_set1_ps( tc2.y - tc0.y );

      // starting values of the edge functions E_i
      s32 row_E0 = A0 * tile_x0 + B0 * tile_y0 + C0 + fill_bias0;
      s32 row_E1 = A1 * tile_x0 + B1 * tile_y0 + C1 + fill_bias1;
      s32 row_E2 = A2 * tile_x0 + B2 * tile_y0 + C2 + fill_bias2;

      __m128i fourA0 = _mm_set1_epi32( 4 * A0 );
      __m128i fourA1 = _mm_set1_epi32( 4 * A1 );
      __m128i fourA2 = _mm_set1_epi32( 4 * A2 );

      __m128i oneB0 = _mm_set1_epi32( B0 );
      __m128i oneB1 = _mm_set1_epi32( B1 );
      __m128i oneB2 = _mm_set1_epi32( B2 );

      __m128i spreadA0 = _mm_setr_epi32( 0, A0, 2 * A0, 3 * A0 );
      __m128i spreadA1 = _mm_setr_epi32( 0, A1, 2 * A1, 3 * A1 );
      __m128i spreadA2 = _mm_setr_epi32( 0, A2, 2 * A2, 3 * A2 );
      __m128i row__E0 = _mm_add_epi32( _mm_set1_epi32( row_E0 ), spreadA0 );
      __m128i row__E1 = _mm_add_epi32( _mm_set1_epi32( row_E1 ), spreadA1 );
      __m128i row__E2 = _mm_add_epi32( _mm_set1_epi32( row_E2 ), spreadA2 );

      ProfClose( raster_simd_trisetup );

      Fori( s32, y, tile_y0, tile_y1 ) {

        __m128i E0 = row__E0;
        __m128i E1 = row__E1;
        __m128i E2 = row__E2;

        Forinc( s32, x, tile_x0, tile_x1, 4 ) {

          auto& dst = LookupAs( __m128i, target_img, x, y );
          auto dst_depth = &LookupAs( f32, target_dep, x, y );
          auto depth = *Cast( __m128*, dst_depth );

          __m128i Ecombined = _mm_or3_si128( E0, E1, E2 );
          __m128i mask = _mm_cmplt_epi32( zeroi, Ecombined );

          if( !_mm_test_all_zeros( mask, mask ) ) {
            //__m128 coeff0 = _mm_mul_ps( _mm_cvtepi32_ps( E1 ), rec_twice_tri_area4 );
            __m128 coeff1 = _mm_mul_ps( _mm_cvtepi32_ps( E2 ), rec_twice_tri_area4 );
            __m128 coeff2 = _mm_mul_ps( _mm_cvtepi32_ps( E0 ), rec_twice_tri_area4 );

            // pos = sum{ coeff_i * pos_i }
            __m128 pos_x = _mm_set1_ps( pos0.x );
            __m128 pos_y = _mm_set1_ps( pos0.y );
            __m128 pos_z = _mm_set1_ps( pos0.z );
            __m128 pos_w = _mm_set1_ps( pos0.w );

            pos_x = _mm_add_ps( pos_x, _mm_mul_ps( delta_pos1_x, coeff1 ) );
            pos_y = _mm_add_ps( pos_y, _mm_mul_ps( delta_pos1_y, coeff1 ) );
            pos_z = _mm_add_ps( pos_z, _mm_mul_ps( delta_pos1_z, coeff1 ) );
            pos_w = _mm_add_ps( pos_w, _mm_mul_ps( delta_pos1_w, coeff1 ) );

            pos_x = _mm_add_ps( pos_x, _mm_mul_ps( delta_pos2_x, coeff2 ) );
            pos_y = _mm_add_ps( pos_y, _mm_mul_ps( delta_pos2_y, coeff2 ) );
            pos_z = _mm_add_ps( pos_z, _mm_mul_ps( delta_pos2_z, coeff2 ) );
            pos_w = _mm_add_ps( pos_w, _mm_mul_ps( delta_pos2_w, coeff2 ) );

            // depth test.
            __m128 maskf = _mm_cmpge_ps( pos_w, depth );
            mask = _mm_and_si128( mask, *Cast( __m128i*, &maskf ) );
            if( !_mm_test_all_zeros( mask, mask ) ) { // TODO: removing this early out may improve perf.

              // tc = sum{ coeff_i * tc_i }
              __m128 tc_x = _mm_add3_ps( tc0_x, _mm_mul_ps( delta_tc1_x, coeff1 ), _mm_mul_ps( delta_tc2_x, coeff2 ) );
              __m128 tc_y = _mm_add3_ps( tc0_y, _mm_mul_ps( delta_tc1_y, coeff1 ), _mm_mul_ps( delta_tc2_y, coeff2 ) );

              // Convert the interpolated s/w, t/w back into s, t.
              tc_x = _mm_div_ps( tc_x, pos_w );
              tc_y = _mm_div_ps( tc_y, pos_w );

              // CLAMP to valid range.
              tc_x = _mm_clamp_ps( tc_x, zero, one );
              tc_y = _mm_clamp_ps( tc_y, zero, one );

              __m128 texel_x = _mm_sub_ps( _mm_mul_ps( tc_x, texture_x_m1 ), _mm_set1_ps( 0.5f ) );
              __m128 texel_y = _mm_sub_ps( _mm_mul_ps( tc_y, texture_y_m1 ), _mm_set1_ps( 0.5f ) );

              __m128i src;
              SampleNearest4_u8x4( src, texture, texel_x, texel_y, texture_xi_m1, texture_yi_m1 );
              //SampleBilinear4_u8x4( src, texture, texel_x, texel_y );

              src = _mm_and_si128( src, mask );
              src = _mm_or_si128( src, _mm_andnot_si128( mask, dst ) );

              // TODO: blending.

              _mm_store_si128( &dst, src );

              maskf = *Cast( __m128*, &mask );
              depth = _mm_or_ps( _mm_and_ps( maskf, pos_w ), _mm_andnot_ps( maskf, depth ) );

              _mm_store_ps( dst_depth, depth );
            }
          }

          E0 = _mm_add_epi32( E0, fourA0 );
          E1 = _mm_add_epi32( E1, fourA1 );
          E2 = _mm_add_epi32( E2, fourA2 );
        }

        row__E0 = _mm_add_epi32( row__E0, oneB0 );
        row__E1 = _mm_add_epi32( row__E1, oneB1 );
        row__E2 = _mm_add_epi32( row__E2, oneB2 );
      }
    }
  }
  }
}



void
RenderMesh(
  img_t& target_img,
  img_t& target_dep,
  raster_tiles_t& raster_tiles,
  mat4x4r<f32>& clip_from_world,
  mesh_t& mesh,
  rendermode_t rendermode
  )
{
  ProfFunc();

  AssertCrash( mesh.idxs_len % 3 == 0 );

  stack_resizeable_cont_t<vec4<f32>> pos_scratch;
  stack_resizeable_cont_t<vec2<f32>> tc_scratch;
  stack_resizeable_cont_t<u32> idxs_scratch;

  Prof( rendermesh_allocscratch );
  Alloc( pos_scratch, 2 * mesh.vert_len );
  Alloc( tc_scratch, 2 * mesh.vert_len );
  Alloc( idxs_scratch, 2 * mesh.idxs_len );
  ProfClose( rendermesh_allocscratch );

  Prof( rendermesh_transform_clipfrommodel );
  For( i, 0, mesh.vert_len ) {
    auto& pos = mesh.positions[i];
    auto& pos_scr = pos_scratch.mem[i];
    auto pos_world = _vec3<f32>( 0 );
    auto pos3 = _vec3( pos.x, pos.y, pos.z );
    Mul( &pos_world, mesh.rotation_world_from_model, pos3 );
    Mul( &pos_world, mesh.scale_world_from_model );
    Add( &pos_world, mesh.translation );
    auto pos4_world = _vec4<f32>( pos_world.x, pos_world.y, pos_world.z, 1 );
    Mul( &pos_scr, clip_from_world, pos4_world );
  }
  pos_scratch.len = mesh.vert_len;
  ProfClose( rendermesh_transform_clipfrommodel );

  Prof( rendermesh_copy_tcs_idxs );
  Memmove( tc_scratch.mem, mesh.texcoords, mesh.vert_len * sizeof( mesh.texcoords[0] ) );
  tc_scratch.len = mesh.vert_len;

  Memmove( idxs_scratch.mem, mesh.idxs, mesh.idxs_len * sizeof( mesh.idxs[0] ) );
  idxs_scratch.len = mesh.idxs_len;
  ProfClose( rendermesh_copy_tcs_idxs );

  Prof( rendermesh_clipping );
  // idea: iterate over clipping planes.
  // for each plane:
  //   for each tri:
  //     if 3 verts on wrong side: throw away tri.
  //     if 0 verts on wrong side: keep tri.
  //     if 1 vert on wrong side: find the intersections of the edges out of the wrong vert with the plane. This gives 2 new verts. Divide the 4 verts into two tris and keep them.
  //     if 2 verts on wrong side: find the intersections of the edges out of the good vert with the plane. This gives 2 new verts. Change the tri to be the good vert and two new verts.

  // TODO: make sure we obey some of the cardinal clipping rules, so seamless geom stays seamless.

  For( plane_idx, 0, 6 ) {
    ClipMeshAgainstClipPlane( plane_idx, pos_scratch, tc_scratch, idxs_scratch );
  }
  AssertCrash( idxs_scratch.len % 3 == 0 );
  AssertCrash( pos_scratch.len == tc_scratch.len );
//  Log( "Tris after clipping: %u", idxs_scratch.len / 3 );
  ProfClose( rendermesh_clipping );

  // TODO: introduce an intermediate rasterizer space. Thus, we would go:
  //   ndc -> rasterizer -> screen
  //   in terms of spaces. This is necessary if we want to do rasterization at higher resolution than the target image.

  Prof( rendermesh_transform_rasterizerfromndc );
  // Go from clip space to ndc space. Then go from ndc space to rasterizer space.

  mat4x4r<f32> rasterizer_from_ndc;
  {
    Prof( matrix_screenfromndc );

    mat4x4r<f32> offset;
    Translate( &offset, 1.0f, 1.0f, 0.0f );

    mat4x4r<f32> upscale;
    Scale( &upscale, 0.5f * target_img.xf, 0.5f * target_img.yf, 1.0f );

    mat4x4r<f32> px_bias;
    //Translate( &px_bias, 0.5f, 0.5f, 0.0f );
    Translate( &px_bias, 0.0f, 0.0f, 0.0f );

    mat4x4r<f32> tmp;
    Mul( &tmp, upscale, offset );
    Mul( &rasterizer_from_ndc, px_bias, tmp );
  }

  ForLen( i, pos_scratch ) {
    auto& pos_scr = pos_scratch.mem[i];
    auto& tc_scr = tc_scratch.mem[i];

    f32 rec_w = 1 / pos_scr.w;

    pos_scr.x *= rec_w;
    pos_scr.y *= rec_w;
    pos_scr.z *= rec_w;
    pos_scr.w = 1;

    // We need to interpolate s/w, t/w, and then divide by 1/w at each pixel to recover s, t. The 1/w will be stored in pos_scr.w to allow reconstruction.
    tc_scr *= rec_w;

    vec4<f32> tmp = pos_scr;
    Mul( &pos_scr, rasterizer_from_ndc, tmp );

    // Store the 1/w in the w component, to allow for reconstruction of s,t.
    pos_scr.w = rec_w;
  }

  ProfClose( rendermesh_transform_rasterizerfromndc );



  Prof( rendermesh_orientfaces );
  // TODO: add a backface culling pass, which culls CW/CCW tris, and ensures all output tris are CCW.
  //   The rasterizer requires CCW to determine the top-left fill rule.

  bool desired_winding = 1;

  Forinc( idx_t, i, 0, idxs_scratch.len, 3 ) {
    u32 idx0 = idxs_scratch.mem[i+0];
    u32 idx1 = idxs_scratch.mem[i+1];
    u32 idx2 = idxs_scratch.mem[i+2];
    auto& pos0 = pos_scratch.mem[idx0];
    auto& pos1 = pos_scratch.mem[idx1];
    auto& pos2 = pos_scratch.mem[idx2];

    vec2<f32> p0 = { pos0.x, pos0.y };
    vec2<f32> p1 = { pos1.x, pos1.y };
    vec2<f32> p2 = { pos2.x, pos2.y };

    auto e0 = p1 - p0;
    auto e1 = p2 - p1;
    //auto e2 = p0 - p2;

    bool winding = ( CrossZ( e0, e1 ) >= 0 );
    bool mismatch = winding ^ desired_winding;
    if( mismatch ) {
      // change tri ( 0 1 2 ) to be ( 1 0 2 ), which only swaps the winding.
      u32 tmp = idxs_scratch.mem[i+0];
      idxs_scratch.mem[i+0] = idxs_scratch.mem[i+1];
      idxs_scratch.mem[i+1] = tmp;
    }
  }
  ProfClose( rendermesh_orientfaces );

  // TODO: do we want to accumulate tris from all meshes into the tiles before rasterization?
  // TODO: clip tris against tile boundaries, so we don't have duplicates in bins. then we can really multithread things.
  // TODO: determine a fast tile_x, tile_y.
  // TODO: don't use an array of arrays; just use two arrays.

  // tile the target image, and bin tris into those tiles. then we don't have to iterate over all pixels for all tris.

  Prof( rendermesh_cleartiles );
  ClearTiles( raster_tiles );
  ProfClose( rendermesh_cleartiles );

  Prof( rendermesh_binning );
  BinTriangles( raster_tiles, target_img, pos_scratch, idxs_scratch );
  ProfClose( rendermesh_binning );

  Prof( rendermesh_raster );
  switch( rendermode ) {
    case rendermode_t::ref: {
      RasterReference( raster_tiles, target_img, target_dep, *mesh.texture, pos_scratch, tc_scratch, idxs_scratch );
    } break;
    case rendermode_t::opt: {
      RasterOptimized( raster_tiles, target_img, target_dep, *mesh.texture, pos_scratch, tc_scratch, idxs_scratch );
    } break;
    case rendermode_t::simd: {
      RasterOptimizedSimd( raster_tiles, target_img, target_dep, *mesh.texture, pos_scratch, tc_scratch, idxs_scratch );
    } break;
    default: UnreachableCrash();
  }
  ProfClose( rendermesh_raster );

  Prof( rendermesh_freescratch );
  Free( idxs_scratch );
  Free( tc_scratch );
  Free( pos_scratch );
  ProfClose( rendermesh_freescratch );
}


void
Render(
  img_t& target_img,
  img_t& target_dep,
  raster_tiles_t& raster_tiles,
  mat4x4r<f32>& clip_from_world,
  mesh_t* meshes,
  idx_t nmeshes,
  rendermode_t rendermode
  )
{
  ProfFunc();

  For( m, 0, nmeshes ) {
    auto& mesh = meshes[m];

    RenderMesh(
      target_img,
      target_dep,
      raster_tiles,
      clip_from_world,
      mesh,
      rendermode
      );
  }
}



// TODO: put c_ or g_ prefix on these.
#define cos_epsilon ( 1e-5f )
#define uv_epsilon ( 1e-5f )
#define sphere_epsilon ( 1e-5f )
#define bkgd_radiance_emit ( _vec3( 0.0f ) )




// Given in model space.
struct
rttri_tex_t
{
  vec3<f32> p;
  vec3<f32> e0;
  vec3<f32> e1;
  vec3<f32> n;
  f32 twice_surface_area_model;
  vec2<f32> tc0;
  vec2<f32> delta_tc1;
  vec2<f32> delta_tc2;
};

struct
rttri_t
{
  vec3<f32> p;
  vec3<f32> e0;
  vec3<f32> e1;
  vec3<f32> n;
  f32 twice_surface_area_model;
};

struct
rtparallelo_tex_t
{
  vec3<f32> p;
  vec3<f32> e0;
  vec3<f32> e1;
  vec3<f32> n;
  f32 surface_area_model;
  vec2<f32> tc0;
  vec2<f32> delta_tc1;
  vec2<f32> delta_tc2;
};

struct
rtparallelo_t
{
  vec3<f32> p;
  vec3<f32> e0;
  vec3<f32> e1;
  vec3<f32> n;
  f32 surface_area_model;
};

struct
rtsphere_t
{
  vec3<f32> p;
  f32 radius;
  f32 sq_radius;
};

struct
rtdisc_t
{
  vec3<f32> p;
  vec3<f32> n;
  f32 radius;
  f32 sq_radius;
};

struct
rtcylinder_t
{
  vec3<f32> p;
  vec3<f32> n;
  f32 height;
  f32 radius;
  f32 sq_radius;
};

struct
rtcone_t
{
  vec3<f32> p;
  vec3<f32> n;
  f32 r;
  f32 r2;
  f32 extent_pos;
  f32 extent_neg;
};

struct
rtmesh_t
{
  vec3<f32> reflectance;
  img_t* reflectance_tex;
  stack_resizeable_cont_t<rttri_tex_t> tris_tex;
  stack_resizeable_cont_t<rttri_t> tris;
  stack_resizeable_cont_t<rtparallelo_tex_t> parallelos_tex;
  stack_resizeable_cont_t<rtparallelo_t> parallelos;
  stack_resizeable_cont_t<rtsphere_t> spheres;
  stack_resizeable_cont_t<rtdisc_t> discs;
  stack_resizeable_cont_t<rtcylinder_t> cylinders;
  stack_resizeable_cont_t<rtcone_t> cones;
  mat3x3r<f32> rotation_world_from_model;
  mat3x3r<f32> rotation_model_from_world;
  f32 scale_world_from_model;
  f32 scale_model_from_world;
  vec3<f32> translation_world;
  f32 surface_area_model;
  vec3<f32> radiance_emit;
};



Inl vec3<f32>
WorldFromModelP( rtmesh_t& mesh, vec3<f32> p_model )
{
  vec3<f32> p_world;
  Mul( &p_world, mesh.rotation_world_from_model, p_model );
  p_world *= mesh.scale_world_from_model;
  p_world += mesh.translation_world;
  return p_world;
}

Inl vec3<f32>
ModelFromWorldP( rtmesh_t& mesh, vec3<f32> p_world )
{
  p_world -= mesh.translation_world;
  p_world *= mesh.scale_model_from_world;
  vec3<f32> p_model;
  Mul( &p_model, mesh.rotation_model_from_world, p_world );
  return p_model;
}

Inl vec3<f32>
WorldFromModelPosDelta( rtmesh_t& mesh, vec3<f32> pdelta_model )
{
  vec3<f32> pdelta_world;
  Mul( &pdelta_world, mesh.rotation_world_from_model, pdelta_model );
  pdelta_world *= mesh.scale_world_from_model;
  return pdelta_world;
}

Inl vec3<f32>
WorldFromModelN( rtmesh_t& mesh, vec3<f32> n_model )
{
  vec3<f32> n_world;
  Mul( &n_world, mesh.rotation_world_from_model, n_model );
  return n_world;
}

Inl vec3<f32>
ModelFromWorldN( rtmesh_t& mesh, vec3<f32> n_world )
{
  vec3<f32> n_model;
  Mul( &n_model, mesh.rotation_model_from_world, n_world );
  return n_model;
}

Inl f32
WorldFromModelDist( rtmesh_t& mesh, f32 dist_model )
{
  auto dist_world = dist_model * mesh.scale_world_from_model;
  return dist_world;
}

Inl f32
WorldFromModelArea( rtmesh_t& mesh, f32 area_model )
{
  auto area_world = area_model * Square( mesh.scale_world_from_model );
  return area_world;
}



// TODO: change to enumc.
enum
primitive_type_t
{
  primitive_none = 0,
  primitive_tritex,
  primitive_tri,
  primitive_sphere,
  primitive_parallelotex,
  primitive_parallelo,
  primitive_disc,
  primitive_cylinder,
  primitive_cone,
};


struct
basis_t
{
  vec3<f32> t;
  vec3<f32> b;
  vec3<f32> n;
};


struct
hit_t
{
  f32 t_world;
  vec3<f32> x_world;
  vec3<f32> x_model;
  vec2<f32> uv;
  basis_t basis;
  rtmesh_t* mesh;
  void* primitive;
  primitive_type_t primitive_type;
};

Inl void
Init( hit_t& hit )
{
  hit.t_world = MAX_f32;
  hit.x_world = {};
  hit.x_model = {};
  hit.uv = {};
  hit.basis = {};
  hit.mesh = 0;
  hit.primitive = 0;
  hit.primitive_type = primitive_none;
}

Inl bool
Invalid( hit_t& hit )
{
  bool res = ( hit.t_world == MAX_f32 );
  return res;
}

Inl bool
Valid( hit_t& hit )
{
  bool res = ( hit.t_world != MAX_f32 );
  return res;
}





Inl void
ReflectAndRefract(
  vec3<f32> w_i,
  vec3<f32> n,
  f32 eta_i,
  f32 eta_t,
  vec3<f32>& w_r,
  vec3<f32>& w_t
  )
{
  auto r = eta_i / eta_t;
  auto cos_theta_i = Dot( n, w_i );
  auto cos_theta_t = Sqrt32( 1 - r * r * ( 1 - cos_theta_i * cos_theta_i ) );

  auto w_in = cos_theta_i * n;
  auto w_ip = w_i - w_in;
  w_r = w_in - w_ip;

  auto w_tp = -r * w_ip;
  auto w_tn = -cos_theta_t * n;
  w_t = w_tn + w_tp;
}

Inl void
Reflect(
  vec3<f32>& w_r,
  vec3<f32> w_i,
  vec3<f32> n
  )
{
  auto cos_theta_i = Dot( n, w_i );

  auto w_in = cos_theta_i * n;
  auto w_ip = w_i - w_in;
  w_r = w_in - w_ip;
}






Templ Inl void
TraceTri(
  hit_t& closest,
  rtmesh_t& mesh,
  T& tri,
  primitive_type_t primitive_type,
  vec3<f32>& ray_p_model,
  vec3<f32>& ray_d_model,
  f32 t_min_world
  )
{
  auto cos_theta = Dot( tri.n, ray_d_model );
  bool intersects_plane = ( ABS( cos_theta ) > cos_epsilon );
  if( intersects_plane ) {
    auto rec_cos_theta = 1 / cos_theta;
    auto c = ray_p_model - tri.p;
    auto t_model = -Dot( tri.n, c ) * rec_cos_theta;
    auto t_world = WorldFromModelDist( mesh, t_model );
    if( ( t_world > t_min_world )  &  ( t_world < closest.t_world ) ) {
      auto cross_c_d = Cross( c, ray_d_model );
      auto rec_denom = rec_cos_theta / tri.twice_surface_area_model;
      auto u = -Dot( cross_c_d, tri.e1 ) * rec_denom;
      auto v = Dot( cross_c_d, tri.e0 ) * rec_denom;
      bool valid_b1 = ( u >= -uv_epsilon );
      bool valid_b2 = ( v >= -uv_epsilon );
      bool valid_sum = ( u + v <= 1 + uv_epsilon );
      bool inside_tri = valid_b1  &  valid_b2  &  valid_sum;
      if( inside_tri ) {
        closest.t_world = t_world;
        closest.x_model = ray_p_model + t_model * ray_d_model;
        //closest.x_model = tri.p + u * tri.e0 + v * tri.e1;
        closest.uv = _vec2( u, v );
        closest.basis.n = tri.n;
        if( Dot( closest.basis.n, ray_d_model ) > 0 ) {
          closest.basis.n *= -1.0f;
        }
        closest.mesh = &mesh;
        closest.primitive = &tri;
        closest.primitive_type = primitive_type;
      }
    }
  }
}


Inl void
TraceTri(
  hit_t& closest,
  rtmesh_t& mesh,
  rttri_tex_t& tri,
  vec3<f32>& ray_p_model,
  vec3<f32>& ray_d_model,
  f32 t_min_world
  )
{
  TraceTri( closest, mesh, tri, primitive_tritex, ray_p_model, ray_d_model, t_min_world );
}

Inl void
TraceTri(
  hit_t& closest,
  rtmesh_t& mesh,
  rttri_t& tri,
  vec3<f32>& ray_p_model,
  vec3<f32>& ray_d_model,
  f32 t_min_world
  )
{
  TraceTri( closest, mesh, tri, primitive_tri, ray_p_model, ray_d_model, t_min_world );
}




Templ Inl void
TraceParallelo(
  hit_t& closest,
  rtmesh_t& mesh,
  T& parallelo,
  primitive_type_t primitive_type,
  vec3<f32>& ray_p_model,
  vec3<f32>& ray_d_model,
  f32 t_min_world
  )
{
  auto cos_theta = Dot( parallelo.n, ray_d_model );
  bool intersects_plane = ( ABS( cos_theta ) > cos_epsilon );
  if( intersects_plane ) {
    auto rec_cos_theta = 1 / cos_theta;
    auto c = ray_p_model - parallelo.p;
    auto t_model = -Dot( parallelo.n, c ) * rec_cos_theta;
    auto t_world = WorldFromModelDist( mesh, t_model );
    if( ( t_world > t_min_world )  &  ( t_world < closest.t_world ) ) {
      auto cross_c_d = Cross( c, ray_d_model );
      auto rec_denom = rec_cos_theta / parallelo.surface_area_model;
      auto u = -Dot( cross_c_d, parallelo.e1 ) * rec_denom;
      auto v = Dot( cross_c_d, parallelo.e0 ) * rec_denom;
      bool valid_b1 = ( -uv_epsilon <= u )  &  ( u <= 1 + uv_epsilon );
      bool valid_b2 = ( -uv_epsilon <= v )  &  ( v <= 1 + uv_epsilon );
      bool inside_parallelo = valid_b1  &  valid_b2;
      if( inside_parallelo ) {
        closest.t_world = t_world;
        closest.x_model = ray_p_model + t_model * ray_d_model;
        //closest.x_model = parallelo.p + u * parallelo.e0 + v * parallelo.e1;
        closest.uv = _vec2( u, v );
        closest.basis.n = parallelo.n;
        if( Dot( closest.basis.n, ray_d_model ) > 0 ) {
          closest.basis.n *= -1.0f;
        }
        closest.mesh = &mesh;
        closest.primitive = &parallelo;
        closest.primitive_type = primitive_type;
      }
    }
  }
}


Inl void
TraceParallelo(
  hit_t& closest,
  rtmesh_t& mesh,
  rtparallelo_tex_t& parallelo,
  vec3<f32>& ray_p_model,
  vec3<f32>& ray_d_model,
  f32 t_min_world
  )
{
  TraceParallelo( closest, mesh, parallelo, primitive_parallelotex, ray_p_model, ray_d_model, t_min_world );
}

Inl void
TraceParallelo(
  hit_t& closest,
  rtmesh_t& mesh,
  rtparallelo_t& parallelo,
  vec3<f32>& ray_p_model,
  vec3<f32>& ray_d_model,
  f32 t_min_world
  )
{
  TraceParallelo( closest, mesh, parallelo, primitive_parallelo, ray_p_model, ray_d_model, t_min_world );
}



Inl void
TraceDisc(
  hit_t& closest,
  rtmesh_t& mesh,
  rtdisc_t& disc,
  vec3<f32>& ray_p_model,
  vec3<f32>& ray_d_model,
  f32 t_min_world
  )
{
  auto cos_theta = Dot( disc.n, ray_d_model );
  bool intersects_plane = ( ABS( cos_theta ) > cos_epsilon );
  if( intersects_plane ) {
    auto rec_cos_theta = 1 / cos_theta;
    auto c = ray_p_model - disc.p;
    auto t_model = -Dot( disc.n, c ) * rec_cos_theta;
    auto t_world = WorldFromModelDist( mesh, t_model );
    if( ( t_world > t_min_world )  &  ( t_world < closest.t_world ) ) {
      auto x_model = ray_p_model + t_model * ray_d_model;
      auto d = x_model - disc.p;
      auto d2 = Squared( d );
      if( d2 < disc.sq_radius ) {
        closest.t_world = t_world;
        closest.x_model = x_model;
        closest.uv = {}; // TODO: parameterize disc.
        closest.basis.n = disc.n;
        if( Dot( closest.basis.n, ray_d_model ) > 0 ) {
          closest.basis.n *= -1.0f;
        }
        closest.mesh = &mesh;
        closest.primitive = &disc;
        closest.primitive_type = primitive_disc;
      }
    }
  }
}




Inl void
WriteSphereHit(
  hit_t& closest,
  f32 t_model,
  f32 t_world,
  vec3<f32>& ray_p_model,
  vec3<f32>& ray_d_model,
  rtsphere_t& sphere,
  rtmesh_t& mesh
  )
{
  closest.t_world = t_world;
  closest.x_model = ray_p_model + t_model * ray_d_model;
  closest.uv = {}; // TODO: sphere surface parameterization.
  auto n_model = Normalize( closest.x_model - sphere.p );
  // Project x_model back onto sphere surface, for best precision.
  closest.x_model = sphere.p + sphere.radius * n_model;
  closest.basis.n = n_model;
  if( Dot( closest.basis.n, ray_d_model ) > 0 ) {
    closest.basis.n *= -1.0f;
  }
  closest.mesh = &mesh;
  closest.primitive = &sphere;
  closest.primitive_type = primitive_sphere;
}

Inl void
TraceSphere(
  hit_t& closest,
  rtmesh_t& mesh,
  rtsphere_t& sphere,
  vec3<f32>& ray_p_model,
  vec3<f32>& ray_d_model,
  f32 t_min_world
  )
{
  // Setup equation:
  //   t^2 - tau*t + del = 0
  // which has solns:
  //   t = 0.5f * ( tau +- Sqrt( Square( tau ) - 4 * del )

  auto rel = sphere.p - ray_p_model;
  auto tau = 2 * Dot( ray_d_model, rel );
  auto del = Squared( rel ) - sphere.sq_radius;

  auto disc = Square( tau ) - 4 * del;
  if( disc < 0 ) {

  } elif( disc < sphere_epsilon ) {
    auto t_model = 0.5f * tau;
    auto t_world = WorldFromModelDist( mesh, t_model );
    if( ( t_world > t_min_world )  &  ( t_world < closest.t_world ) ) {
      WriteSphereHit( closest, t_model, t_world, ray_p_model, ray_d_model, sphere, mesh );
    }

  } else {
    auto rt_disc = Sqrt32( disc );
    auto t0_model = 0.5f * ( tau - rt_disc );
    auto t0_world = WorldFromModelDist( mesh, t0_model );
    if( ( t0_world > t_min_world )  &  ( t0_world < closest.t_world ) ) {
      WriteSphereHit( closest, t0_model, t0_world, ray_p_model, ray_d_model, sphere, mesh );
    } else {
      auto t1_model = 0.5f * ( tau + rt_disc );
      auto t1_world = WorldFromModelDist( mesh, t1_model );
      if( ( t1_world > t_min_world )  &  ( t1_world < closest.t_world ) ) {
        WriteSphereHit( closest, t1_model, t1_world, ray_p_model, ray_d_model, sphere, mesh );
      }
    }
  }
}




Inl void
WriteCylinderHit(
  hit_t& closest,
  f32 t_model,
  f32 t_world,
  vec3<f32>& ray_p_model,
  vec3<f32>& ray_d_model,
  rtcylinder_t& cylinder,
  rtmesh_t& mesh
  )
{
  auto x_model = ray_p_model + t_model * ray_d_model;
  auto rel = x_model - cylinder.p;
  auto height = Dot( rel, cylinder.n );
  if( ( height > -cos_epsilon )  &  ( height < cylinder.height + cos_epsilon ) ) {
    auto height_vec = height * cylinder.n;
    auto n_model = Normalize( rel - height_vec );
    // Project x_model back onto cylinder surface, for best precision.
    x_model = cylinder.p + height_vec + cylinder.radius * n_model;

    closest.t_world = t_world;
    closest.x_model = x_model;
    closest.uv = {}; // TODO: cylinder surface parameterization.
    closest.basis.n = n_model;
    if( Dot( closest.basis.n, ray_d_model ) > 0 ) {
      closest.basis.n *= -1.0f;
    }
    closest.mesh = &mesh;
    closest.primitive = &cylinder;
    closest.primitive_type = primitive_cylinder;
  }
}

Inl void
TraceCylinder(
  hit_t& closest,
  rtmesh_t& mesh,
  rtcylinder_t& cylinder,
  vec3<f32>& ray_p_model,
  vec3<f32>& ray_d_model,
  f32 t_min_world
  )
{
  // Setup equation:
  //   a*t^2 - b*t + c = 0
  auto dn = Dot( ray_d_model, cylinder.n );
  auto a = 1 - Square( dn );
  auto s = cylinder.p - ray_p_model;
  auto sn = Dot( s, cylinder.n );
  auto sd = Dot( s, ray_d_model );
  auto b = 2 * ( sd - dn * sn );
  auto c = Squared( s ) - cylinder.sq_radius - Square( sn );

  if( a < sphere_epsilon ) {
    //if( c < sphere_epsilon ) {
    //
    //} else {
    //  auto t_model = b / c;
    //  auto t_world = WorldFromModelDist( mesh, t_model );
    //  if( ( t_world > t_min_world )  &  ( t_world < closest.t_world ) ) {
    //    WriteCylinderHit( closest, t_model, t_world, ray_p_model, ray_d_model, cylinder, mesh );
    //  }
    //}

  } else {
    // Setup equation:
    //   t^2 - tau*t + del = 0
    //   t = 0.5f * ( tau +- Sqrt( Square( tau ) - 4 * del )

    auto rec_a = 1 / a;
    auto tau = b * rec_a;
    auto del = c * rec_a;

    auto disc = Square( tau ) - 4 * del;
    if( disc < 0 ) {

    } elif( disc < sphere_epsilon ) {
      auto t_model = 0.5f * tau;
      auto t_world = WorldFromModelDist( mesh, t_model );
      if( ( t_world > t_min_world )  &  ( t_world < closest.t_world ) ) {
        WriteCylinderHit( closest, t_model, t_world, ray_p_model, ray_d_model, cylinder, mesh );
      }

    } else {
      auto rt_disc = Sqrt32( disc );

      // We have to try both t0, t1 because t0 may fail due to the height constraint.
      // Only the minimum of the two will be written.

      // If we had a closed cylinder with disc caps, then we wouldn't need to check both.

      auto t0_model = 0.5f * ( tau - rt_disc );
      auto t0_world = WorldFromModelDist( mesh, t0_model );
      if( ( t0_world > t_min_world )  &  ( t0_world < closest.t_world ) ) {
        WriteCylinderHit( closest, t0_model, t0_world, ray_p_model, ray_d_model, cylinder, mesh );
      }
      auto t1_model = 0.5f * ( tau + rt_disc );
      auto t1_world = WorldFromModelDist( mesh, t1_model );
      if( ( t1_world > t_min_world )  &  ( t1_world < closest.t_world ) ) {
        WriteCylinderHit( closest, t1_model, t1_world, ray_p_model, ray_d_model, cylinder, mesh );
      }
    }
  }
}






Inl void
TraceCone(
  hit_t& closest,
  rtmesh_t& mesh,
  rtcone_t& cone,
  vec3<f32>& ray_p_model,
  vec3<f32>& ray_d_model,
  f32 t_min_world
  )
{
  // Setup equation:
  //   a*t^2 - b*t + c = 0
  auto s = cone.p - ray_p_model;
  auto rfactor = 1 + cone.r2;
  auto dn = Dot( ray_d_model, cone.n );
  auto a = 1 - rfactor * Square( dn );
  auto sn = Dot( s, cone.n );
  auto sd = Dot( s, ray_d_model );
  auto b = 2 * ( sd - rfactor * dn * sn );
  auto c = Squared( s ) - rfactor * Square( sn );

  if( ABS( a ) < sphere_epsilon ) {

  } else {
    auto rec_a = 1 / a;
    auto tau = b * rec_a;
    auto del = c * rec_a;
    auto disc = Square( tau ) - 4 * del;
    if( disc < 0 ) {

    } else {
      auto rt_disc = Sqrt32( disc );
      // We have to try both t0, t1 because t0 may fail due to the height constraint.
      // Only the minimum of the two will be written.
      // If we had a closed cone with disc caps, then we wouldn't need to check both.
      {
        auto t0_model = 0.5f * ( tau - rt_disc );
        auto x_model = ray_p_model + t0_model * ray_d_model;
        s = cone.p - x_model;
        auto height = Dot( s, cone.n );
        bool valid_height = ( height > cone.extent_neg - cos_epsilon )  &  ( height < cone.extent_pos + cos_epsilon );
        if( valid_height ) {
          auto s_para = height * cone.n;
          auto s_perp = s - s_para;
          auto n_model = Normalize( s_perp - cone.r2 * s_para );
          // Project x_model back onto surface, for best precision.
          auto err = Dot( s, n_model );
          s -= err * n_model;
          x_model = cone.p - s;
          auto t0_world = WorldFromModelDist( mesh, t0_model );
          if( ( t0_world > t_min_world )  &  ( t0_world < closest.t_world ) ) {
            closest.t_world = t0_world;
            closest.x_model = x_model;
            closest.uv = {}; // TODO: surface parameterization.
            closest.basis.n = n_model;
            if( Dot( closest.basis.n, ray_d_model ) > 0 ) {
              closest.basis.n *= -1.0f;
            }
            closest.mesh = &mesh;
            closest.primitive = &cone;
            closest.primitive_type = primitive_cone;
          }
        }
      }

      {
        auto t1_model = 0.5f * ( tau + rt_disc );
        auto x_model = ray_p_model + t1_model * ray_d_model;
        s = cone.p - x_model;
        auto height = Dot( s, cone.n );
        bool valid_height = ( height > cone.extent_neg - cos_epsilon )  &  ( height < cone.extent_pos + cos_epsilon );
        if( valid_height ) {
          auto s_para = height * cone.n;
          auto s_perp = s - s_para;
          auto n_model = Normalize( s_perp - cone.r2 * s_para );
          // Project x_model back onto surface, for best precision.
          auto err = Dot( s, n_model );
          s -= err * n_model;
          x_model = cone.p - s;
          auto t1_world = WorldFromModelDist( mesh, t1_model );
          if( ( t1_world > t_min_world )  &  ( t1_world < closest.t_world ) ) {
            closest.t_world = t1_world;
            closest.x_model = x_model;
            closest.uv = {}; // TODO: surface parameterization.
            closest.basis.n = n_model;
            if( Dot( closest.basis.n, ray_d_model ) > 0 ) {
              closest.basis.n *= -1.0f;
            }
            closest.mesh = &mesh;
            closest.primitive = &cone;
            closest.primitive_type = primitive_cone;
          }
        }
      }
    }
  }
}







Inl hit_t
Trace(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  vec3<f32>& ray_p_world,
  vec3<f32>& ray_d_world,
  f32 t_min_world
  )
{
  ProfFunc();

  hit_t closest;
  Init( closest );

  For( m, 0, meshes_len ) {
    auto& mesh = *meshes[m];

    auto ray_p_model = ModelFromWorldP( mesh, ray_p_world );
    auto ray_d_model = ModelFromWorldN( mesh, ray_d_world );

    ForLen( i, mesh.tris_tex ) {
      auto& tri = mesh.tris_tex.mem[i];
      TraceTri( closest, mesh, tri, ray_p_model, ray_d_model, t_min_world );
    }

    ForLen( i, mesh.tris ) {
      auto& tri = mesh.tris.mem[i];
      TraceTri( closest, mesh, tri, ray_p_model, ray_d_model, t_min_world );
    }

    ForLen( i, mesh.parallelos_tex ) {
      auto& parallelo = mesh.parallelos_tex.mem[i];
      TraceParallelo( closest, mesh, parallelo, ray_p_model, ray_d_model, t_min_world );
    }

    ForLen( i, mesh.parallelos ) {
      auto& parallelo = mesh.parallelos.mem[i];
      TraceParallelo( closest, mesh, parallelo, ray_p_model, ray_d_model, t_min_world );
    }

    ForLen( i, mesh.spheres ) {
      auto& sphere = mesh.spheres.mem[i];
      TraceSphere( closest, mesh, sphere, ray_p_model, ray_d_model, t_min_world );
    }

    ForLen( i, mesh.discs ) {
      auto& disc = mesh.discs.mem[i];
      TraceDisc( closest, mesh, disc, ray_p_model, ray_d_model, t_min_world );
    }

    ForLen( i, mesh.cylinders ) {
      auto& cylinder = mesh.cylinders.mem[i];
      TraceCylinder( closest, mesh, cylinder, ray_p_model, ray_d_model, t_min_world );
    }

    ForLen( i, mesh.cones ) {
      auto& cone = mesh.cones.mem[i];
      TraceCone( closest, mesh, cone, ray_p_model, ray_d_model, t_min_world );
    }

  }

  if( Valid( closest ) ) {
    auto& mesh = *closest.mesh;

    closest.basis.n = WorldFromModelN( mesh, closest.basis.n );
    OrthonormalBasisGivenNorm( closest.basis.t, closest.basis.b, closest.basis.n, cos_epsilon );

    closest.x_world = WorldFromModelP( mesh, closest.x_model );
    //closest.x_world = ray_p_world + closest.t_world * ray_d_world;
    //auto err = Length( x_world - closest.x_world );
    //Log( "transform err: %f", err );
  }

  return closest;
}



//typedef rng_mt_t   rng_t;
//typedef rng_lcg_t   rng_t;
typedef rng_xorshift32_t   rng_t;






Inl rttri_tex_t&
UniformTriangle(
  rtmesh_t& mesh,
  u64 urand
  )
{
  auto tri_idx = urand % mesh.tris_tex.len;
  return mesh.tris_tex.mem[tri_idx];
}

Inl vec2<f32>
UniformTriBaryCoords(
  f32 zeta0,
  f32 zeta1
  )
{
  auto s = Sqrt32( zeta0 );
  auto t = zeta1;
  return _vec2<f32>( s * ( 1 - t ), s * t );
}



Inl vec3<f32>
UniformParallelo(
  vec3<f32> p,
  vec3<f32> e0,
  vec3<f32> e1,
  f32 zeta0,
  f32 zeta1
  )
{
  return p + zeta0 * e0 + zeta1 * e1;
}




Inl vec3<f32>
UniformHemisphere(
  basis_t& basis,
  f32 zeta0,
  f32 zeta1
  )
{
  f32 angle = f32_2PI * zeta0;
  f32 z = zeta1;
  f32 scale = Sqrt32( 1 - z * z );
  f32 x = scale * Cos32( angle );
  f32 y = scale * Sin32( angle );
  return x * basis.t + y * basis.b + z * basis.n;
}

Inl vec3<f32>
CosineHemisphere(
  basis_t& basis,
  f32 zeta0,
  f32 zeta1
  )
{
  f32 angle = f32_2PI * zeta0;
  f32 z = Sqrt32( zeta1 );
  f32 scale = Sqrt32( 1 - zeta1 );
  f32 x = scale * Cos32( angle );
  f32 y = scale * Sin32( angle );
  return x * basis.t + y * basis.b + z * basis.n;
}

Inl vec3<f32>
CosinePowerHemisphere(
  basis_t& basis,
  f32 cos_exponent,
  f32 zeta0,
  f32 zeta1
  )
{
  f32 angle = f32_2PI * zeta0;
  f32 z = Pow32( zeta1, 1 / ( cos_exponent + 1 ) );
  f32 scale = Sqrt32( 1 - z * z );
  f32 x = scale * Cos32( angle );
  f32 y = scale * Sin32( angle );
  return x * basis.t + y * basis.b + z * basis.n;
}

Inl vec3<f32>
UniformUnitDisc(
  basis_t& basis,
  f32 zeta0,
  f32 zeta1
  )
{
  f32 angle = f32_2PI * zeta0;
  f32 scale = Sqrt32( zeta1 );
  f32 x = scale * Cos32( angle );
  f32 y = scale * Sin32( angle );
  return x * basis.t + y * basis.b;
}





Inl vec3<f32>
Reflectance( hit_t& hit )
{
  auto& mesh = *hit.mesh;

  switch( hit.primitive_type ) {
    case primitive_tritex: {
      auto& tri = *Cast( rttri_tex_t*, hit.primitive );
      auto& texture = *mesh.reflectance_tex;

      auto tc = tri.tc0 + hit.uv.x * tri.delta_tc1 + hit.uv.y * tri.delta_tc2;
      auto texel = CLAMP( tc, 0.0f, 1.0f ) * _vec2( texture.xf, texture.yf );
      auto src = SampleNearest_u8x4( texture, texel );
      auto srcf = Convert_f32x4_from_u8x4( src );
      return mesh.reflectance * _vec3( srcf.x, srcf.y, srcf.z );
    } break;

    case primitive_tri: {
      //auto& tri = *Cast( rttri_t*, hit.primitive );
      return mesh.reflectance;
    } break;

    case primitive_parallelotex: {
      auto& parallelo = *Cast( rtparallelo_tex_t*, hit.primitive );
      auto& texture = *mesh.reflectance_tex;

      auto tc = parallelo.tc0 + hit.uv.x * parallelo.delta_tc1 + hit.uv.y * parallelo.delta_tc2;
      auto texel = CLAMP( tc, 0.0f, 1.0f ) * _vec2( texture.xf, texture.yf );
      auto src = SampleNearest_u8x4( texture, texel );
      auto srcf = Convert_f32x4_from_u8x4( src );
      return mesh.reflectance * _vec3( srcf.x, srcf.y, srcf.z );
    } break;

    case primitive_parallelo: {
      //auto& parallelo = *Cast( rtparallelo_t*, hit.primitive );
      return mesh.reflectance;
    } break;

    case primitive_sphere: {
      //auto& sphere = *Cast( rtsphere_t*, hit.primitive );
      return mesh.reflectance;
    } break;

    case primitive_disc: {
      //auto& disc = *Cast( rtdisc_t*, hit.primitive );
      return mesh.reflectance;
    } break;

    case primitive_cylinder: {
      //auto& cylinder = *Cast( rtcylinder_t*, hit.primitive );
      return mesh.reflectance;
    } break;

    case primitive_cone: {
      //auto& cone = *Cast( rtcone_t*, hit.primitive );
      return mesh.reflectance;
    } break;

    default: UnreachableCrash();
  }
  return {};
}

Inl vec3<f32>
RadianceEmit( hit_t& hit )
{
  auto& mesh = *hit.mesh;
  return mesh.radiance_emit;
}





// VERSION 0.75
//
Inl vec3<f32>
ReflectedRadiance_0_75(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  );

Inl vec3<f32>
Radiance_0_75(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  vec3<f32>& x,
  vec3<f32> w,
  rng_t& rng,
  f32 t_min_world
  )
{
  hit_t hit = Trace( meshes, meshes_len, x, w, t_min_world );
  if( Invalid( hit ) ) {
    return bkgd_radiance_emit; // background radiance_emit
  }

  auto radiance_emit = RadianceEmit( hit );
  auto radiance_refl = ReflectedRadiance_0_75(
    meshes,
    meshes_len,
    hit,
    -w,
    rng,
    t_min_world
    );

  auto res = radiance_emit + radiance_refl;
  return res;
}

Inl vec3<f32>
ReflectedRadiance_0_75(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  )
{
  auto& x = hit.x_world;

  static const f32 prob_survival = 0.9f;
  auto rec_prob_survival = 1 / prob_survival;
  if( Zeta32( rng ) < prob_survival ) {
    //auto wi = UniformHemisphere( hit.basis, Zeta32( rng ), Zeta32( rng ) );
    //auto wi_pdf = f32_2PI_REC;
    auto wi = CosineHemisphere( hit.basis, Zeta32( rng ), Zeta32( rng ) );
    //auto wi_pdf = Dot( wi, hit.basis.n ) * f32_PI_REC;
    auto rec_wi_pdf = f32_PI / Dot( wi, hit.basis.n );

    auto brdf = Reflectance( hit ) * f32_PI_REC * Dot( wr, hit.basis.n );

    auto radiance_in = Radiance_0_75(
      meshes,
      meshes_len,
      x,
      wi,
      rng,
      t_min_world
      );
    auto res = ( brdf * radiance_in * rec_wi_pdf * rec_prob_survival );
    return res;
  } else {
    return _vec3( 0.0f );
  }
}





// VERSION 1.0
//
Inl vec3<f32>
ReflectedRadiance_1_0(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  );

Inl vec3<f32>
Radiance_1_0(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len,
  vec3<f32>& x,
  vec3<f32> w,
  rng_t& rng,
  f32 t_min_world
  )
{
  hit_t hit = Trace( meshes, meshes_len, x, w, t_min_world );
  if( Invalid( hit ) ) {
    return bkgd_radiance_emit;
  }

  auto radiance_emit = RadianceEmit( hit );
  if( MaxElem( radiance_emit ) > 0 ) {
    return radiance_emit;
  }

  auto radiance_refl = ReflectedRadiance_1_0(
    meshes,
    meshes_len,
    lights,
    lights_len,
    hit,
    -w,
    rng,
    t_min_world
    );
  return radiance_refl;
}

Inl vec3<f32>
DirectRadianceFromSphere_1_0(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t& light,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  )
{
  AssertCrash( light.spheres.len == 1 );
  auto& sphere = light.spheres.mem[0];

  auto sphere_p_world = WorldFromModelP( light, sphere.p );
  auto sphere_r_world = WorldFromModelDist( light, sphere.radius );
  auto sphere_r2_world = Square( sphere_r_world );

  auto d = sphere_p_world - hit.x_world;
  auto d2 = Squared( d );

  vec3<f32> wi;
  //f32 wi_pdf;
  f32 rec_wi_pdf;
  f32 cos_theta_i;
  if( d2 < sphere_r2_world ) { // inside sphere.
    wi = UniformHemisphere( hit.basis, Zeta32( rng ), Zeta32( rng ) );
    //wi_pdf = f32_2PI_REC;
    rec_wi_pdf = f32_2PI;
    cos_theta_i = Dot( wi, hit.basis.n );
    if( cos_theta_i < cos_epsilon ) {
      return _vec3( 0.0f );
    }

  } else {
    auto cos_alpha = Sqrt32( 1 - sphere_r2_world / d2 );
    auto disc_radius = sphere_r_world * cos_alpha;

    basis_t basis;
    basis.n = d / Sqrt32( d2 );
    OrthonormalBasisGivenNorm( basis.t, basis.b, basis.n, cos_epsilon );

    auto y = sphere_p_world + disc_radius * UniformUnitDisc( basis, Zeta32( rng ), Zeta32( rng ) );
    wi = Normalize( y - hit.x_world );
    //wi_pdf = f32_2PI_REC / ( 1 - cos_alpha );
    rec_wi_pdf = f32_2PI * ( 1 - cos_alpha );
    cos_theta_i = Dot( wi, hit.basis.n );
    if( cos_theta_i < cos_epsilon ) {
      return _vec3( 0.0f );
    }
  }

  t_min_world /= MAX( t_min_world, cos_theta_i );
  hit_t lighthit = Trace( meshes, meshes_len, hit.x_world, wi, t_min_world );
  if( Invalid( lighthit ) ) {
    return _vec3( 0.0f );
  }

  auto brdf = Reflectance( hit ) * f32_PI_REC * Dot( wr, hit.basis.n );

  auto res = ( brdf * RadianceEmit( lighthit ) * rec_wi_pdf );
  return res;
}

Inl vec3<f32>
DirectRadianceFromParallelo_1_0(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t& light,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  )
{
  AssertCrash( light.parallelos.len == 1 );
  auto& parallelo = light.parallelos.mem[0];

  auto p_world = WorldFromModelP( light, parallelo.p );
  auto n_world = WorldFromModelN( light, parallelo.n );
  auto surface_area = WorldFromModelArea( light, parallelo.surface_area_model );

  auto e0_world = WorldFromModelPosDelta( light, parallelo.e0 );
  auto e1_world = WorldFromModelPosDelta( light, parallelo.e1 );

  auto y = UniformParallelo( p_world, e0_world, e1_world, Zeta32( rng ), Zeta32( rng ) );

  auto d = y - hit.x_world;

  auto d2 = Squared( d );
  auto rec_d2 = 1 / d2;
  auto wi = d / Sqrt32( d2 );
  //auto wi_pdf = 1 / surface_area;
  auto rec_wi_pdf = surface_area;
  auto cos_theta_i = Dot( wi, hit.basis.n );

  if( cos_theta_i < cos_epsilon ) {
    return _vec3( 0.0f );
  }

  t_min_world /= MAX( t_min_world, cos_theta_i );
  hit_t lighthit = Trace( meshes, meshes_len, hit.x_world, wi, t_min_world );
  if( Invalid( lighthit ) ) {
    return _vec3( 0.0f );
  }

  auto brdf = Reflectance( hit ) * f32_PI_REC * Dot( wr, hit.basis.n );
  auto cos_thetaprime = ABS( Dot( n_world, wi ) );

  auto res = ( brdf * RadianceEmit( lighthit ) * cos_thetaprime * rec_wi_pdf * rec_d2 );
  return res;
}

Inl vec3<f32>
DirectRadianceFromDisc_1_0(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t& light,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  )
{
  // TODO: lookup which disc
  AssertCrash( light.discs.len == 1 );
  auto& disc = light.discs.mem[0];

  auto p_world = WorldFromModelP( light, disc.p );
  auto n_world = WorldFromModelN( light, disc.n );
  auto radius_world = WorldFromModelDist( light, disc.radius );
  auto surface_area = WorldFromModelArea( light, f32_PI * disc.sq_radius );

  basis_t disc_basis;
  disc_basis.n = n_world;
  OrthonormalBasisGivenNorm( disc_basis.t, disc_basis.b, disc_basis.n, cos_epsilon );
  auto y = p_world + radius_world * UniformUnitDisc( disc_basis, Zeta32( rng ), Zeta32( rng ) );

  auto d = y - hit.x_world;
  auto d2 = Squared( d );
  auto rec_d2 = 1 / d2;
  auto wi = d / Sqrt32( d2 );
  //auto wi_pdf = 1 / surface_area;
  auto rec_wi_pdf = surface_area;
  auto cos_theta_i = Dot( wi, hit.basis.n );

  if( cos_theta_i < cos_epsilon ) {
    return _vec3( 0.0f );
  }

  t_min_world /= MAX( t_min_world, cos_theta_i );
  hit_t lighthit = Trace( meshes, meshes_len, hit.x_world, wi, t_min_world );
  if( Invalid( lighthit ) ) {
    return _vec3( 0.0f );
  }

  auto brdf = Reflectance( hit ) * f32_PI_REC * Dot( wr, hit.basis.n );
  auto cos_thetaprime = ABS( Dot( n_world, wi ) );

  auto res = ( brdf * RadianceEmit( lighthit ) * cos_thetaprime * rec_wi_pdf * rec_d2 );
  return res;
}


Inl vec3<f32>
DirectRadiance_1_0(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  )
{
  // TODO: smarter choice of light.
  auto light_idx = Rand64( rng ) % lights_len;
  auto& light = *lights[light_idx];

  vec3<f32> direct_radiance = {};

  // TODO: enum all light types.

  if( light.spheres.len > 0 ) {
    direct_radiance = DirectRadianceFromSphere_1_0( meshes, meshes_len, light, hit, wr, rng, t_min_world );
  } elif( light.parallelos.len > 0 ) {
    direct_radiance = DirectRadianceFromParallelo_1_0( meshes, meshes_len, light, hit, wr, rng, t_min_world );
  } elif( light.discs.len > 0 ) {
    direct_radiance = DirectRadianceFromDisc_1_0( meshes, meshes_len, light, hit, wr, rng, t_min_world );
  }

  return direct_radiance;
}

Inl vec3<f32>
IndirectRadiance_1_0(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  )
{
  auto reflectance = Reflectance( hit );
  auto prob_survival = CLAMP( MaxElem( reflectance ), 0.1f, 0.9f );
  auto rec_prob_survival = 1 / prob_survival;
  if( Zeta32( rng ) >= prob_survival ) {
    return _vec3( 0.0f );
  }

  //auto wi = UniformHemisphere( hit.basis, Zeta32( rng ), Zeta32( rng ) );
  //auto wi_pdf = f32_2PI_REC;
  //auto rec_wi_pdf = f32_2PI;
  auto wi = CosineHemisphere( hit.basis, Zeta32( rng ), Zeta32( rng ) );
  //auto wi_pdf = Dot( wi, hit.basis.n ) * f32_PI_REC;
  auto rec_wi_pdf = f32_PI / Dot( wi, hit.basis.n );
  AssertCrash( isfinite( rec_wi_pdf ) );

  auto brdf = reflectance * f32_PI_REC * Dot( wr, hit.basis.n );

  auto radiance_in = Radiance_1_0(
    meshes,
    meshes_len,
    lights,
    lights_len,
    hit.x_world,
    wi,
    rng,
    t_min_world
    );
  auto res = ( brdf * radiance_in * rec_wi_pdf * rec_prob_survival );
  return res;
}

Inl vec3<f32>
ReflectedRadiance_1_0(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  )
{
#if 0
  auto prob_direct = 0.5f;
  auto rec_prob_direct = 1 / prob_direct;
  auto rec_prob_indirect = 1 / ( 1 - prob_direct );
  if( Zeta32( rng ) < prob_direct ) {
    auto radiance_direct = DirectRadiance_1_0(
      meshes,
      meshes_len,
      lights,
      lights_len,
      hit,
      wr,
      rng,
      t_min_world
      );
    return radiance_direct * rec_prob_direct;
  } else {
    auto radiance_indirect = IndirectRadiance_1_0(
      meshes,
      meshes_len,
      lights,
      lights_len,
      hit,
      wr,
      rng,
      t_min_world
      );
    return radiance_indirect * rec_prob_indirect;
  }
#else

  auto radiance_direct = DirectRadiance_1_0(
    meshes,
    meshes_len,
    lights,
    lights_len,
    hit,
    wr,
    rng,
    t_min_world
    );
//  auto radiance_direct = _vec3( 0.0f );
  auto radiance_indirect = IndirectRadiance_1_0(
    meshes,
    meshes_len,
    lights,
    lights_len,
    hit,
    wr,
    rng,
    t_min_world
    );
//  auto radiance_indirect = _vec3( 0.0f );

  auto res = radiance_direct + radiance_indirect;
  return res;
#endif
}








// VERSION 1.0m
//
Inl vec3<f32>
ReflectedRadiance_1_0m(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  );

Inl vec3<f32>
Radiance_1_0m(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len,
  vec3<f32>& x,
  vec3<f32> w,
  rng_t& rng,
  f32 t_min_world
  )
{
  hit_t hit = Trace( meshes, meshes_len, x, w, t_min_world );
  if( Invalid( hit ) ) {
    return bkgd_radiance_emit;
  }

  auto radiance_emit = RadianceEmit( hit );
  auto radiance_refl = ReflectedRadiance_1_0m( meshes, meshes_len, lights, lights_len, hit, -w, rng, t_min_world );

  auto res = radiance_emit + radiance_refl;
  return res;
}

Inl vec3<f32>
DirectRadianceFromDisc_1_0m(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t& light,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  )
{
  ImplementCrash();
  return _vec3( 0.0f );
}


Inl vec3<f32>
DirectRadiance_1_0m(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  )
{
  // TODO: smarter choice of light.
  auto light_idx = Rand64( rng ) % lights_len;
  auto& light = *lights[light_idx];

  auto reflectance = Reflectance( hit );
  auto cos_theta_o = Dot( wr, hit.basis.n );


  auto direct_radiance_light = _vec3( 0.0f );

  // TODO: enum all light types.
  AssertCrash( light.discs.len == 1 );
  auto& disc = light.discs.mem[0];

  auto p_world = WorldFromModelP( light, disc.p );
  auto n_world = WorldFromModelN( light, disc.n );
  auto radius_world = WorldFromModelDist( light, disc.radius );
  auto surface_area = WorldFromModelArea( light, f32_PI * disc.sq_radius );

  basis_t disc_basis;
  disc_basis.n = n_world;
  OrthonormalBasisGivenNorm( disc_basis.t, disc_basis.b, disc_basis.n, cos_epsilon );
  auto y = p_world + radius_world * UniformUnitDisc( disc_basis, Zeta32( rng ), Zeta32( rng ) );

  auto d = y - hit.x_world;
  auto d2 = Squared( d );
  auto rec_d2 = 1 / d2;
  auto wi_light = d / Sqrt32( d2 );
  auto wi_light_pdf = 1 / surface_area;
  //auto rec_wi_light_pdf = surface_area;
  auto cos_thetaprime = ABS( Dot( n_world, wi_light ) );
  auto rec_cos_thetaprime = 1 / cos_thetaprime;
  auto cos_theta_i_light = Dot( wi_light, hit.basis.n );
  if( cos_theta_i_light > cos_epsilon ) {
    hit_t lighthit = Trace( meshes, meshes_len, hit.x_world, wi_light, t_min_world / MAX( t_min_world, cos_theta_i_light ) );
    if( Valid( lighthit ) ) {
      auto brdf = reflectance * f32_PI_REC * cos_theta_o;
      direct_radiance_light = ( brdf * RadianceEmit( lighthit ) );
    }
  }


  auto direct_radiance_brdf = _vec3( 0.0f );
  auto wi_brdf = CosineHemisphere( hit.basis, Zeta32( rng ), Zeta32( rng ) );
  auto wi_brdf_pdf = Dot( wi_brdf, hit.basis.n ) * f32_PI_REC;
  auto rec_wi_brdf_pdf = f32_PI / Dot( wi_brdf, hit.basis.n );
  AssertCrash( isfinite( rec_wi_brdf_pdf ) );
  auto cos_theta_i_brdf = Dot( wi_brdf, hit.basis.n );
  hit_t brdfhit = Trace( meshes, meshes_len, hit.x_world, wi_brdf, t_min_world / MAX( t_min_world, cos_theta_i_brdf ) );
  if( Valid( brdfhit ) ) {
    auto brdf = reflectance * f32_PI_REC * cos_theta_o;
    direct_radiance_brdf = ( brdf * RadianceEmit( brdfhit ) );
  }


  auto wi_light_pdf_in_solidangle = wi_light_pdf * cos_thetaprime * rec_d2;
  auto wi_light_pdf_in_area = wi_light_pdf;
  auto wi_brdf_pdf_in_solidangle = wi_brdf_pdf;
  auto wi_brdf_pdf_in_area = wi_brdf_pdf * rec_cos_thetaprime * d2;

  auto direct_radiance =
    direct_radiance_light / ( wi_light_pdf_in_area + wi_brdf_pdf_in_area ) +
    direct_radiance_brdf / ( wi_light_pdf_in_solidangle + wi_brdf_pdf_in_solidangle );
  //auto direct_radiance = 0.5f * ( direct_radiance_light * cos_thetaprime * rec_d2 * rec_wi_light_pdf + direct_radiance_brdf * rec_wi_brdf_pdf );
  //auto direct_radiance = direct_radiance_light * cos_thetaprime * rec_d2 * rec_wi_light_pdf;
  //auto direct_radiance = direct_radiance_brdf * rec_wi_brdf_pdf;

  return direct_radiance;
}

Inl vec3<f32>
IndirectRadiance_1_0m(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  )
{
  auto reflectance = Reflectance( hit );
  auto prob_survival = CLAMP( MaxElem( reflectance ), 0.1f, 0.9f );
  auto rec_prob_survival = 1 / prob_survival;
  if( Zeta32( rng ) < prob_survival ) {
    //auto wi = UniformHemisphere( hit.basis, Zeta32( rng ), Zeta32( rng ) );
    //auto wi_pdf = f32_2PI_REC;
    //auto rec_wi_pdf = f32_2PI;
    auto wi = CosineHemisphere( hit.basis, Zeta32( rng ), Zeta32( rng ) );
    //auto wi_pdf = Dot( wi, hit.basis.n ) * f32_PI_REC;
    auto rec_wi_pdf = f32_PI / Dot( wi, hit.basis.n );
    AssertCrash( isfinite( rec_wi_pdf ) );

    auto brdf = reflectance * f32_PI_REC * Dot( wr, hit.basis.n );

    auto radiance_in = Radiance_1_0m(
      meshes,
      meshes_len,
      lights,
      lights_len,
      hit.x_world,
      wi,
      rng,
      t_min_world
      );
    auto res = ( brdf * radiance_in * rec_wi_pdf * rec_prob_survival );
    return res;
  } else {
    return _vec3( 0.0f );
  }
}

Inl vec3<f32>
ReflectedRadiance_1_0m(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  )
{
  auto radiance_direct = DirectRadiance_1_0m( meshes, meshes_len, lights, lights_len, hit, wr, rng, t_min_world );
  //auto radiance_direct = _vec3( 0.0f );
  auto radiance_indirect = IndirectRadiance_1_0m( meshes, meshes_len, lights, lights_len, hit, wr, rng, t_min_world );
  //auto radiance_indirect = _vec3( 0.0f );

  auto res = radiance_direct + radiance_indirect;
  return res;
}















Inl vec3<f32>
ReflectedRadiance_1_1(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len,
  hit_t& hit,
  vec3<f32> wr,
  rng_t& rng,
  f32 t_min_world
  )
{
  // TODO: smarter choice of light.
  auto light_idx = Rand64( rng ) % lights_len;
  auto& light = *lights[light_idx];

  auto reflectance = Reflectance( hit );
  auto cos_theta_o = Dot( wr, hit.basis.n );

  auto brdf = reflectance * f32_PI_REC * cos_theta_o;


  auto radiance_direct_light = _vec3( 0.0f );

  // TODO: enum all light types.
  AssertCrash( light.discs.len == 1 );
  auto& disc = light.discs.mem[0];

  auto p_world = WorldFromModelP( light, disc.p );
  auto n_world = WorldFromModelN( light, disc.n );
  auto radius_world = WorldFromModelDist( light, disc.radius );
  auto surface_area = WorldFromModelArea( light, f32_PI * disc.sq_radius );

  basis_t disc_basis;
  disc_basis.n = n_world;
  OrthonormalBasisGivenNorm( disc_basis.t, disc_basis.b, disc_basis.n, cos_epsilon );
  auto y = p_world + radius_world * UniformUnitDisc( disc_basis, Zeta32( rng ), Zeta32( rng ) );

  auto d = y - hit.x_world;
  auto d2 = Squared( d );
  auto rec_d2 = 1 / d2;
  auto wi_light = d / Sqrt32( d2 );
  auto wi_light_pdf = 1 / surface_area;
  //auto rec_wi_light_pdf = surface_area;
  auto cos_thetaprime = ABS( Dot( n_world, wi_light ) );
  auto rec_cos_thetaprime = 1 / cos_thetaprime;
  auto cos_theta_i_light = Dot( wi_light, hit.basis.n );
  if( cos_theta_i_light > cos_epsilon ) {
    hit_t lighthit = Trace( meshes, meshes_len, hit.x_world, wi_light, t_min_world / MAX( t_min_world, cos_theta_i_light ) );
    if( Valid( lighthit ) ) {
      radiance_direct_light = ( brdf * RadianceEmit( lighthit ) );
    }
  }


  auto radiance_direct_brdf = _vec3( 0.0f );
  auto wi_brdf = CosineHemisphere( hit.basis, Zeta32( rng ), Zeta32( rng ) );
  auto wi_brdf_pdf = Dot( wi_brdf, hit.basis.n ) * f32_PI_REC;
  auto rec_wi_brdf_pdf = f32_PI / Dot( wi_brdf, hit.basis.n );
  AssertCrash( isfinite( rec_wi_brdf_pdf ) );
  auto cos_theta_i_brdf = Dot( wi_brdf, hit.basis.n );
  hit_t brdfhit = Trace(
    meshes,
    meshes_len,
    hit.x_world,
    wi_brdf,
    t_min_world / MAX( t_min_world, cos_theta_i_brdf )
    );
  if( Valid( brdfhit ) ) {
    radiance_direct_brdf = ( brdf * RadianceEmit( brdfhit ) );
  }


  auto wi_light_pdf_in_solidangle = wi_light_pdf * cos_thetaprime * rec_d2;
  auto wi_light_pdf_in_area = wi_light_pdf;
  auto wi_brdf_pdf_in_solidangle = wi_brdf_pdf;
  auto wi_brdf_pdf_in_area = wi_brdf_pdf * rec_cos_thetaprime * d2;

  auto weight_light = 1 / ( wi_light_pdf_in_area + wi_brdf_pdf_in_area );
  auto weight_brdf = 1 / ( wi_light_pdf_in_solidangle + wi_brdf_pdf_in_solidangle );
#if 1
  auto radiance_direct =
    ( radiance_direct_light * Square( weight_light ) +
      radiance_direct_brdf * Square( weight_brdf ) ) /
        ( Square( weight_light ) + Square( weight_brdf ) );
#else
  auto radiance_direct =
    radiance_direct_light * weight_light +
    radiance_direct_brdf * weight_brdf;
#endif
  //auto direct_radiance = 0.5f * ( direct_radiance_light * cos_thetaprime * rec_d2 * rec_wi_light_pdf + direct_radiance_brdf * rec_wi_brdf_pdf );
  //auto direct_radiance = direct_radiance_light * cos_thetaprime * rec_d2 * rec_wi_light_pdf;
  //auto direct_radiance = direct_radiance_brdf * rec_wi_brdf_pdf;

  // terminate recursion early, aka Russian Roulette
  auto prob_survival = CLAMP( MaxElem( reflectance ), 0.1f, 0.9f );
  auto rec_prob_survival = 1 / prob_survival;
  auto radiance_indirect = _vec3( 0.0f );
  if( Zeta32( rng ) < prob_survival ) {
    vec3<f32> radiance_refl;
    if( Valid( brdfhit ) ) {
      radiance_refl = ReflectedRadiance_1_1(
        meshes,
        meshes_len,
        lights,
        lights_len,
        brdfhit,
        -wi_brdf,
        rng,
        t_min_world
        );
    } else {
      radiance_refl = bkgd_radiance_emit;
    }
    radiance_indirect = ( brdf * radiance_refl * rec_wi_brdf_pdf * rec_prob_survival );
  }

  auto res = radiance_direct + radiance_indirect;
  return res;
}

Inl vec3<f32>
Radiance_1_1(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len,
  vec3<f32>& x,
  vec3<f32> w,
  rng_t& rng,
  f32 t_min_world
  )
{
  hit_t hit = Trace( meshes, meshes_len, x, w, t_min_world );
  if( Invalid( hit ) ) {
    return bkgd_radiance_emit;
  }

  auto radiance_emit = RadianceEmit( hit );
  if( MaxElem( radiance_emit ) > 0 ) {
    return radiance_emit;
  }

  auto radiance_refl = ReflectedRadiance_1_1(
    meshes,
    meshes_len,
    lights,
    lights_len,
    hit,
    -w,
    rng,
    t_min_world
    );
  return radiance_refl;
}





Inl vec3<f32>
PathTrace(
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len,
  vec3<f32>& x,
  vec3<f32> v,
  rng_t& rng,
  f32 t_min_world
  )
{
  hit_t hit = Trace( meshes, meshes_len, x, v, t_min_world );
  if( Invalid( hit ) ) {
    return bkgd_radiance_emit;
  }

  auto radiance_emit = RadianceEmit( hit );
  if( MaxElem( radiance_emit ) > 0 ) {
    return radiance_emit;
  }

  auto reflectance = Reflectance( hit );
  auto w_o = -v;
  auto cos_theta_o = Dot( w_o, hit.basis.n );
  auto brdf = reflectance * f32_PI_REC * cos_theta_o;

  // pick a new bounce direction.
  auto w_i = CosineHemisphere( hit.basis, Zeta32( rng ), Zeta32( rng ) );
  auto cos_theta_i = Dot( w_i, hit.basis.n );
  if( cos_theta_i < cos_epsilon ) {
    return _vec3<f32>();
  }
  auto w_i_pdf = cos_theta_i * f32_PI_REC;
  auto rec_w_i_pdf = 1 / w_i_pdf;

  // terminate recursion early, aka Russian Roulette
  auto prob_survival = 0.9f;
  auto rec_prob_survival = 1 / prob_survival;
  if( Zeta32( rng ) >= prob_survival ) {
    return _vec3<f32>();
  }

  auto radiance_i = PathTrace(
    meshes,
    meshes_len,
    lights,
    lights_len,
    hit.x_world,
    w_i,
    rng,
    MAX( t_min_world, cos_theta_i )
    );
  auto radiance_o = ( brdf * radiance_i * rec_w_i_pdf * rec_prob_survival );
  return radiance_o;
}







void
Raytrace(
  img_t& target_img,
  rng_t& rng,
  u32 samples_per_pixel,
  vec3<f32>& translate_world_from_camera,
  mat3x3r<f32>& rotation_world_from_camera,
  f32 camera_half_x,
  f32 camera_half_y,
  f32 camera_near_z,
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len
  )
{
  ProfFunc();

  AssertCrash( camera_half_x > 0 );
  AssertCrash( camera_half_y > 0 );
  AssertCrash( camera_near_z > 0 );

  u32 supersample_x = Round_u32_from_f32( qsqrt32( Cast( f32, samples_per_pixel ) ) );
  u32 supersample_y = supersample_x;

  idx_t brightness_count = 0;
  f32 mean_brightness = 0;

  Fori( u32, y, 0, target_img.y ) {
  Fori( u32, x, 0, target_img.x ) {
    auto& dst = LookupAs( vec4<f32>, target_img, x, y );

    Prof( raytrace_perpixel );

    vec3<f32> supersample_dst = {};
    u32 supersample_count = 0;

    Fori( u32, j, 0, supersample_y ) {
    Fori( u32, i, 0, supersample_x ) {

      Prof( raytrace_persubpixel );

      f32 ox = ( i + 0.5f ) / supersample_x - 0.5f;
      f32 oy = ( j + 0.5f ) / supersample_y - 0.5f;

      f32 cx = ( x + ox ) / target_img.x;
      f32 cy = ( y + oy ) / target_img.y;

      auto ray_p_cam = _vec3<f32>( 0 );
      auto ray_d_cam = _vec3<f32>(
        ( 2 * cx - 1 ) * camera_half_x,
        ( 2 * cy - 1 ) * camera_half_y,
        -camera_near_z
        );

      // Move ray_p to the image plane in world space.
      vec3<f32> ray_d;
      Mul( &ray_d, rotation_world_from_camera, ray_d_cam );
      auto ray_p = translate_world_from_camera + ray_p_cam + ray_d;
      ray_d = Normalize( ray_d );


      constant f32 t_min_world = 1e-3f;

//      auto radiance = Radiance_0_75( meshes, meshes_len, ray_p, ray_d, rng, t_min_world );
      auto radiance = Radiance_1_0( meshes, meshes_len, lights, lights_len, ray_p, ray_d, rng, t_min_world );
//      auto radiance = Radiance_1_0m( meshes, meshes_len, lights, lights_len, ray_p, ray_d, rng, t_min_world );
//      auto radiance = Radiance_1_1( meshes, meshes_len, lights, lights_len, ray_p, ray_d, rng, t_min_world );
//      auto radiance = PathTrace( meshes, meshes_len, lights, lights_len, ray_p, ray_d, rng, t_min_world );

      supersample_dst += radiance;
      supersample_count += 1;

      ProfClose( raytrace_persubpixel );
    }
    }

    if( supersample_count ) {
      supersample_dst *= ( 1.0f / supersample_count );
    }

    dst.x = supersample_dst.x;
    dst.y = supersample_dst.y;
    dst.z = supersample_dst.z;
    dst.w = 1;

    auto lumin = PerceivedBrightness( _vec3( dst.x, dst.y, dst.z ) );
    brightness_count += 1;
    auto new_mean = mean_brightness + ( lumin - mean_brightness ) / brightness_count;
    mean_brightness = new_mean;

    ProfClose( raytrace_perpixel );
  }
  }

  Log( "mean_brightness: %f", mean_brightness );

  if( mean_brightness > 0 ) {
    Fori( u32, y, 0, target_img.y ) {
    Fori( u32, x, 0, target_img.x ) {
      auto& dst = LookupAs( vec4<f32>, target_img, x, y );

#if 0
      auto lumin = PerceivedBrightness( _vec3( dst.x, dst.y, dst.z ) );
      auto lumin_fac = 2.0f / ( 1.0f + lumin );
      dst.x = CLAMP( dst.x * lumin_fac, 0, 1 );
      dst.y = CLAMP( dst.y * lumin_fac, 0, 1 );
      dst.z = CLAMP( dst.z * lumin_fac, 0, 1 );
#elif 0
      dst.x = CLAMP( 2.0f * dst.x / mean_brightness, 0, 1 );
      dst.y = CLAMP( 2.0f * dst.y / mean_brightness, 0, 1 );
      dst.z = CLAMP( 2.0f * dst.z / mean_brightness, 0, 1 );
#else
      dst.x = CLAMP( dst.x, 0, 1 );
      dst.y = CLAMP( dst.y, 0, 1 );
      dst.z = CLAMP( dst.z, 0, 1 );
#endif

      static const f32 expon = 1 / 2.2f;
      dst.x = Pow32( dst.x, expon );
      dst.y = Pow32( dst.y, expon );
      dst.z = Pow32( dst.z, expon );

      dst.w = 1;
    }
    }
  }
}



struct
vertex_t
{
  hit_t hit;

  // probability of picking this position, with respect to surface area.
  // e.g. pick position uniformly random on an area light, and pdf_area = 1 / area_of_light.
  f32 pdf_area;
};

struct
bounce_t
{

};


struct
path_t
{
  vec2<f32> eye_uv;
  vec2<u32> eye_px;
  vec3<f32> eye_pos;
  vec3<f32> eye_n;
  vec3<f32> eye_ray_p;
  vec3<f32> eye_ray_d;
  stack_resizeable_cont_t<bounce_t> bounces;
  vec2<f32> light_uv;
  vec3<f32> light_pos;
  vec3<f32> light_n;
  vec3<f32> radiance;
};

Inl void
Init( path_t& path )
{
  path.light_uv = { };
  path.light_pos = { };
  path.light_n = { };
  path.eye_uv = { };
  path.eye_pos = { };
  path.eye_n = { };
  Alloc( path.bounces, 32 );
}

Inl void
Kill( path_t& path )
{
  Free( path.bounces );
}



void
Metropolis(
  img_t& target_img,
  rng_t& rng,
  vec3<f32>& translate_world_from_camera,
  mat3x3r<f32>& rotation_world_from_camera,
  f32 camera_half_x,
  f32 camera_half_y,
  f32 camera_near_z,
  rtmesh_t* meshes[],
  idx_t meshes_len,
  rtmesh_t* lights[],
  idx_t lights_len
  )
{
  ProfFunc();

  AssertCrash( camera_half_x > 0 );
  AssertCrash( camera_half_y > 0 );
  AssertCrash( camera_near_z > 0 );

  static const auto t_min_world = 1e-3f;

  // Initial path
  static const idx_t npaths = 1024;
  path_t paths[npaths];
  For( i, 0, npaths ) {
    path_t& path = paths[i];

    path.eye_uv = _vec2( Zeta32( rng ), Zeta32( rng ) );
    path.eye_px = _vec2(
      Cast( u32, 0.5f + path.eye_uv.x * target_img.x_m1 ),
      Cast( u32, 0.5f + path.eye_uv.y * target_img.y_m1 )
      );

    {
      auto ray_p_cam = _vec3<f32>( 0 );
      auto ray_d_cam = _vec3<f32>(
        ( 2 * path.eye_uv.x - 1 ) * camera_half_x,
        ( 2 * path.eye_uv.y - 1 ) * camera_half_y,
        -camera_near_z
        );
      vec3<f32> ray_d;
      Mul( &ray_d, rotation_world_from_camera, ray_d_cam );
      auto ray_p = translate_world_from_camera + ray_p_cam + ray_d;
      ray_d = Normalize( ray_d );

      path.eye_ray_p = ray_p;
      path.eye_ray_d = ray_d;
    }

    auto hit = Trace( meshes, meshes_len, path.eye_ray_p, path.eye_ray_d, t_min_world );
    if( Invalid( hit ) ) {
      path.radiance = bkgd_radiance_emit;
      continue;
    }


  }


//  Forever {
//    auto& dst = LookupAs( vec4<f32>, target_img, path.eye_px.x, path.eye_px.y );
//    ImplementCrash();
//  }


  Fori( u32, y, 0, target_img.y ) {
  Fori( u32, x, 0, target_img.x ) {
    auto& dst = LookupAs( vec4<f32>, target_img, x, y );
    auto& dst3 = *Cast( vec3<f32>*, &dst );

    auto lumin = PerceivedBrightness( dst3 );
    dst3 *= 1.0f / ( 1.0f + lumin );

    dst3 = CLAMP( dst3, 0.0f, 1.0f );

    static const f32 expon = 1 / 2.2f;
    dst3.x = Pow32( dst3.x, expon );
    dst3.y = Pow32( dst3.y, expon );
    dst3.z = Pow32( dst3.z, expon );

    dst.x = dst3.x;
    dst.y = dst3.y;
    dst.z = dst3.z;
    dst.w = 1;
  }
  }
}


static img_t checker_tex;
static img_t white_tex;
static img_t box_tex;

static rtmesh_t rtmesh_box;
static rtmesh_t rtmesh_b;
static rtmesh_t rtmesh_tri;
static rtmesh_t rtmesh_s;
static rtmesh_t rtmesh_light;
static rtmesh_t rtmesh_light2;
static rtmesh_t rtmesh_c;
static rtmesh_t rtmesh_cone;


static rtmesh_t* rtmeshes[] = {
  &rtmesh_box,
//  &rtmesh_b,
//  &rtmesh_tri,
//  &rtmesh_s,
  &rtmesh_light,
  &rtmesh_light2,
//  &rtmesh_c,
//  &rtmesh_cone,
  };
static idx_t rtmeshes_len = _countof( rtmeshes );


void
InitScene()
{
  ProfFunc();

  Prof( checker_tex_init );

//  u8* filename = Str( "c:/doc/dev/cpp/master/rt/output.png" );

  Init( checker_tex, 512, 512, 512, 4 );
  Alloc( checker_tex );

  Fori( u32, y, 0, checker_tex.y ) {
  Fori( u32, x, 0, checker_tex.x ) {
    auto& tx = LookupAs( vec4<u8>, checker_tex, x, y );
//    if( ( x / Cast( u32, 0.1 * checker_tex.x ) + y / Cast( u32, 0.1 * checker_tex.y ) ) % 2 ) {
//    if( ( x + y ) % 2 ) {
    if( ( ( x / 64 ) + ( y / 64 ) ) % 2 ) {
      tx = _vec4<u8>( 255, 255, 255, 255 );
    } else {
      tx = _vec4<u8>( 128, 128, 255, 255 );
    }
  }
  }
  ProfClose( checker_tex_init );


  Init( white_tex, 1, 1, 4, 4 );
  Alloc( white_tex );
  LookupAs( vec4<u8>, white_tex, 0, 0 ) = _vec4<u8>( 255, 255, 255, 255 );


  Init( box_tex, 12, 6, 12, 4 );
  Alloc( box_tex );
  Fori( u8, j, 0, box_tex.y ) {
    u8 step = 255 / Cast( u8, box_tex.y );
    LookupAs( vec4<u8>, box_tex,  0, j ) = _vec4<u8>( 255, 255, step * ( j + 0 ), 255 ); // yellow
    LookupAs( vec4<u8>, box_tex,  1, j ) = _vec4<u8>( 255, 255, step * ( j + 1 ), 255 );
    LookupAs( vec4<u8>, box_tex,  2, j ) = _vec4<u8>( 255, step * ( j + 0 ), 255, 255 ); // magenta
    LookupAs( vec4<u8>, box_tex,  3, j ) = _vec4<u8>( 255, step * ( j + 1 ), 255, 255 );
    LookupAs( vec4<u8>, box_tex,  4, j ) = _vec4<u8>( step * ( j + 0 ), 255, 255, 255 ); // cyan
    LookupAs( vec4<u8>, box_tex,  5, j ) = _vec4<u8>( step * ( j + 1 ), 255, 255, 255 );
    LookupAs( vec4<u8>, box_tex,  6, j ) = _vec4<u8>(   0, 255, step * ( j + 0 ), 255 ); // green
    LookupAs( vec4<u8>, box_tex,  7, j ) = _vec4<u8>(   0, 255, step * ( j + 1 ), 255 );
    LookupAs( vec4<u8>, box_tex,  8, j ) = _vec4<u8>( 255, step * ( j + 0 ),   0, 255 ); // red
    LookupAs( vec4<u8>, box_tex,  9, j ) = _vec4<u8>( 255, step * ( j + 1 ),   0, 255 );
    LookupAs( vec4<u8>, box_tex, 10, j ) = _vec4<u8>( step * ( j + 0 ),   0, 255, 255 ); // blue
    LookupAs( vec4<u8>, box_tex, 11, j ) = _vec4<u8>( step * ( j + 1 ),   0, 255, 255 );
  }


  vec3<f32> box_pos_and_deltas[] =
  {
    { -1, -1, -1 },
    {  0,  2,  0 },
    {  0,  0,  2 },

    {  1, -1, -1 },
    {  0,  2,  0 },
    {  0,  0,  2 },

    { -1, -1, -1 },
    {  2,  0,  0 },
    {  0,  0,  2 },

    { -1,  1, -1 },
    {  2,  0,  0 },
    {  0,  0,  2 },

    { -1, -1, -1 },
    {  2,  0,  0 },
    {  0,  2,  0 },

    { -1, -1,  1 },
    {  2,  0,  0 },
    {  0,  2,  0 },
  };

  f32 ze = 0.0f;
  f32 ot = 1.0f / 3.0f;
  f32 ha = 0.5f;
  f32 tt = 2.0f / 3.0f;

  vec2<f32> box_tcs_and_deltas[] =
  {
    { ze, ze },
    {  0, ot },
    { ha,  0 },

    { ha, ze },
    {  0, ot },
    { ha,  0 },

    { ze, ot },
    {  0, ot },
    { ha,  0 },

    { ha, ot },
    {  0, ot },
    { ha,  0 },

    { ze, tt },
    {  0, ot },
    { ha,  0 },

    { ha, tt },
    {  0, ot },
    { ha,  0 },
  };

  //u32 box_num_sides = 6;
  idx_t box_num_sides = 5;

  f32 box_surface_area = 4.0f * box_num_sides;

  rtmesh_box.reflectance = _vec3( 1.0f );
  rtmesh_box.reflectance_tex = &box_tex;
  rtmesh_box.tris_tex = {};
  rtmesh_box.tris = {};
#if 0 // TODO: texture lookup during rt seems broken...
  Alloc( rtmesh_box.parallelos_tex, box_num_sides );
  For( i, 0, box_num_sides ) {
    rtparallelo_tex_t parallelo;
    parallelo.p = box_pos_and_deltas[3*i+0];
    parallelo.e0 = box_pos_and_deltas[3*i+1];
    parallelo.e1 = box_pos_and_deltas[3*i+2];
    parallelo.n = Cross( parallelo.e0, parallelo.e1 );
    parallelo.surface_area_model = Length( parallelo.n );
    parallelo.n /= parallelo.surface_area_model;
    parallelo.tc0 = box_tcs_and_deltas[3*i+0];
    parallelo.delta_tc1 = box_tcs_and_deltas[3*i+1];
    parallelo.delta_tc2 = box_tcs_and_deltas[3*i+2];
    *AddBack( rtmesh_box.parallelos_tex ) = parallelo;
  }
  rtmesh_box.parallelos = {};
#else
  rtmesh_box.parallelos_tex = {};
  Alloc( rtmesh_box.parallelos, box_num_sides );
  For( i, 0, box_num_sides ) {
    rtparallelo_t parallelo;
    parallelo.p = box_pos_and_deltas[3*i+0];
    parallelo.e0 = box_pos_and_deltas[3*i+1];
    parallelo.e1 = box_pos_and_deltas[3*i+2];
    parallelo.n = Cross( parallelo.e0, parallelo.e1 );
    parallelo.surface_area_model = Length( parallelo.n );
    parallelo.n /= parallelo.surface_area_model;
    *AddBack( rtmesh_box.parallelos ) = parallelo;
  }
#endif
  rtmesh_box.spheres = {};
  rtmesh_box.discs = {};
  rtmesh_box.cylinders = {};
  rtmesh_box.cones = {};
  Identity( &rtmesh_box.rotation_world_from_model );
  Transpose( &rtmesh_box.rotation_model_from_world, rtmesh_box.rotation_world_from_model );
  rtmesh_box.scale_world_from_model = 16.0f;
  rtmesh_box.scale_model_from_world = 1 / rtmesh_box.scale_world_from_model;
  rtmesh_box.translation_world = {};
  rtmesh_box.surface_area_model = box_surface_area;
  rtmesh_box.radiance_emit = _vec3( 0.0f );



  rtmesh_b.reflectance = _vec3( 1.0f );
  rtmesh_b.reflectance_tex = 0;
  rtmesh_b.tris_tex = {};
  rtmesh_b.tris = {};
  rtmesh_b.parallelos_tex = {};
  Alloc( rtmesh_b.parallelos, box_num_sides );
  For( i, 0, box_num_sides ) {
    rtparallelo_t parallelo;
    parallelo.p = box_pos_and_deltas[3*i+0];
    parallelo.e0 = box_pos_and_deltas[3*i+1];
    parallelo.e1 = box_pos_and_deltas[3*i+2];
    parallelo.n = Cross( parallelo.e0, parallelo.e1 );
    parallelo.surface_area_model = Length( parallelo.n );
    parallelo.n /= parallelo.surface_area_model;
    *AddBack( rtmesh_b.parallelos ) = parallelo;
  }
  rtmesh_b.spheres = {};
  {
    mat3x3r<f32> tmp, tmp2;
    RotateY( &tmp, 0.2f * f32_PI );
    RotateX( &tmp2, 0.5f * f32_PI );
    Mul( &rtmesh_b.rotation_world_from_model, tmp, tmp2 );
  }
  rtmesh_b.discs = {};
  rtmesh_b.cylinders = {};
  rtmesh_b.cones = {};
  Transpose( &rtmesh_b.rotation_model_from_world, rtmesh_b.rotation_world_from_model );
  rtmesh_b.scale_world_from_model = 6.0f;
  rtmesh_b.scale_model_from_world = 1 / rtmesh_b.scale_world_from_model;
  rtmesh_b.translation_world = _vec3<f32>( -8, -10, 5 );
  rtmesh_b.surface_area_model = box_surface_area;
  rtmesh_b.radiance_emit = _vec3( 0.0f );



  rttri_tex_t tri;
  tri.p = _vec3<f32>( -1, 0, 0 );
  tri.e0 = _vec3<f32>( 2, 0, 0 );
  tri.e1 = _vec3<f32>( 1, 1, 0 );
  tri.n = _vec3<f32>( 0, 0, 1 );
  tri.twice_surface_area_model = 2;
  tri.tc0 = _vec2( 0.0f, 0.0f );
  tri.delta_tc1 = _vec2( 1.0f, 0.0f );
  tri.delta_tc2 = _vec2( 0.5f, 1.0f );

  rtmesh_tri.reflectance = _vec3( 1.0f );
  rtmesh_tri.reflectance_tex = &checker_tex;
  Alloc( rtmesh_tri.tris_tex, 1 );
  *AddBack( rtmesh_tri.tris_tex ) = tri;
  rtmesh_tri.tris = {};
  rtmesh_tri.parallelos_tex = {};
  rtmesh_tri.parallelos = {};
  rtmesh_tri.spheres = {};
  rtmesh_tri.discs = {};
  rtmesh_tri.cylinders = {};
  rtmesh_tri.cones = {};
  //Identity( &rtmesh_tri.rotation_world_from_model );
  RotateY( &rtmesh_tri.rotation_world_from_model, 0.3f * f32_PI );
  Transpose( &rtmesh_tri.rotation_model_from_world, rtmesh_tri.rotation_world_from_model );
  rtmesh_tri.scale_world_from_model = 8.0f;
  rtmesh_tri.scale_model_from_world = 1 / rtmesh_tri.scale_world_from_model;
  rtmesh_tri.translation_world = _vec3<f32>( -10, -2, 0 );
  rtmesh_tri.surface_area_model = 0.5f * tri.twice_surface_area_model;
  rtmesh_tri.radiance_emit = _vec3( 0.0f );



  rtmesh_s.reflectance = _vec3( 1.0f );
  rtmesh_s.reflectance_tex = 0;
  rtmesh_s.tris_tex = {};
  rtmesh_s.tris = {};
  rtmesh_s.parallelos_tex = {};
  rtmesh_s.parallelos = {};
  Alloc( rtmesh_s.spheres, 1 );
  {
    rtsphere_t sphere;
    sphere.p = _vec3( 0.0f );
    sphere.radius = 1000;
    sphere.sq_radius = Square( sphere.radius );
    *AddBack( rtmesh_s.spheres ) = sphere;
  }
  rtmesh_s.discs = {};
  rtmesh_s.cylinders = {};
  rtmesh_s.cones = {};
  Identity( &rtmesh_s.rotation_world_from_model );
  Transpose( &rtmesh_s.rotation_model_from_world, rtmesh_s.rotation_world_from_model );
  rtmesh_s.scale_world_from_model = 8.0f;
  rtmesh_s.scale_model_from_world = 1 / rtmesh_s.scale_world_from_model;
  rtmesh_s.translation_world = _vec3<f32>( 8, -8, -4 );
  rtmesh_s.surface_area_model = 4 * f32_PI * rtmesh_s.spheres.mem[0].sq_radius;
  rtmesh_s.radiance_emit = _vec3( 0.0f );




  rtmesh_light.reflectance = _vec3( 1.0f );
  rtmesh_light.reflectance_tex = 0;
  rtmesh_light.tris_tex = {};
  rtmesh_light.tris = {};
  rtmesh_light.parallelos_tex = {};
#if 0
  Alloc( rtmesh_light.parallelos, 1 );
  {
    rtparallelo_t parallelo;
    parallelo.p = _vec3<f32>( -1, 0, -1 );
    parallelo.e0 = _vec3<f32>( 2, 0, 0 );
    parallelo.e1 = _vec3<f32>( 0, 0, 2 );
    parallelo.n = Cross( parallelo.e0, parallelo.e1 );
    parallelo.surface_area_model = Length( parallelo.n );
    parallelo.n /= parallelo.surface_area_model;
    CstrAddBack( rtmesh_light.parallelos, &parallelo );
  }
  rtmesh_light.discs = {};
  rtmesh_light.spheres = {};
#elif 0
  rtmesh_light.parallelos = {};
  Alloc( rtmesh_light.discs, 1 );
  {
    rtdisc_t disc;
    disc.p = _vec3<f32>( 0 );
    disc.n = Normalize( _vec3<f32>( -4, -5, -10 ) );
    disc.radius = 0.5f;
    disc.sq_radius = Square( disc.radius );
    *AddBack( rtmesh_light.discs ) = disc;
  }
  rtmesh_light.spheres = {};
#else
  rtmesh_light.parallelos = {};
  rtmesh_light.discs = {};
  Alloc( rtmesh_light.spheres, 1 );
  {
    rtsphere_t sphere;
    sphere.p = _vec3( 0.0f );
    sphere.radius = 0.5f;
    sphere.sq_radius = Square( sphere.radius );
    *AddBack( rtmesh_light.spheres ) = sphere;
  }
#endif
  rtmesh_light.cylinders = {};
  rtmesh_light.cones = {};
  Identity( &rtmesh_light.rotation_world_from_model );
  Transpose( &rtmesh_light.rotation_model_from_world, rtmesh_light.rotation_world_from_model );
  rtmesh_light.scale_world_from_model = 8.0f;
  rtmesh_light.scale_model_from_world = 1 / rtmesh_light.scale_world_from_model;
  rtmesh_light.translation_world = _vec3<f32>( -10, 13, 0 );
  rtmesh_light.surface_area_model = box_surface_area;
  rtmesh_light.radiance_emit = _vec3( 1.0f, 0.3f, 0.3f );



  rtmesh_light2.reflectance = _vec3( 1.0f );
  rtmesh_light2.reflectance_tex = 0;
  rtmesh_light2.tris_tex = {};
  rtmesh_light2.tris = {};
  rtmesh_light2.parallelos_tex = {};
#if 0
  Alloc( rtmesh_light2.parallelos, 1 );
  {
    rtparallelo_t parallelo;
    parallelo.p = _vec3<f32>( -1, 0, -1 );
    parallelo.e0 = _vec3<f32>( 2, 0, 0 );
    parallelo.e1 = _vec3<f32>( 0, 0, 2 );
    parallelo.n = Cross( parallelo.e0, parallelo.e1 );
    parallelo.surface_area_model = Length( parallelo.n );
    parallelo.n /= parallelo.surface_area_model;
    CstrAddBack( rtmesh_light2.parallelos, &parallelo );
  }
  rtmesh_light2.discs = {};
  rtmesh_light2.spheres = {};
#elif 0
  rtmesh_light2.parallelos = {};
  Alloc( rtmesh_light2.discs, 1 );
  {
    rtdisc_t disc;
    disc.p = _vec3<f32>( 0 );
    disc.n = Normalize( _vec3<f32>( -4, -5, -10 ) );
    disc.radius = 0.5f;
    disc.sq_radius = Square( disc.radius );
    *AddBack( rtmesh_light2.discs ) = disc;
  }
  rtmesh_light2.spheres = {};
#else
  rtmesh_light2.parallelos = {};
  rtmesh_light2.discs = {};
  Alloc( rtmesh_light2.spheres, 1 );
  {
    rtsphere_t sphere;
    sphere.p = _vec3( 0.0f );
    sphere.radius = 0.5f;
    sphere.sq_radius = Square( sphere.radius );
    *AddBack( rtmesh_light2.spheres ) = sphere;
  }
#endif
  rtmesh_light2.cylinders = {};
  rtmesh_light2.cones = {};
  Identity( &rtmesh_light2.rotation_world_from_model );
  Transpose( &rtmesh_light2.rotation_model_from_world, rtmesh_light2.rotation_world_from_model );
  rtmesh_light2.scale_world_from_model = 8.0f;
  rtmesh_light2.scale_model_from_world = 1 / rtmesh_light2.scale_world_from_model;
  rtmesh_light2.translation_world = _vec3<f32>( 10, -13, -10 );
  rtmesh_light2.surface_area_model = box_surface_area;
  rtmesh_light2.radiance_emit = _vec3( 0.3f, 1.0f, 0.3f );



  rtmesh_c.reflectance = _vec3( 1.0f );
  rtmesh_c.reflectance_tex = 0;
  rtmesh_c.tris_tex = {};
  rtmesh_c.tris = {};
  rtmesh_c.parallelos_tex = {};
  rtmesh_c.parallelos = {};
  rtmesh_c.spheres = {};
  rtmesh_c.discs = {};
  Alloc( rtmesh_c.cylinders, 1 );
  {
    rtcylinder_t cylinder;
    cylinder.p = _vec3( 0.0f );
    cylinder.n = _vec3<f32>( 0, -1, 0 );
    cylinder.height = 1 / 3.0f;
    cylinder.radius = 1;
    cylinder.sq_radius = Square( cylinder.radius );
    *AddBack( rtmesh_c.cylinders ) = cylinder;
  }
  rtmesh_c.cones = {};
  Identity( &rtmesh_c.rotation_world_from_model );
  Transpose( &rtmesh_c.rotation_model_from_world, rtmesh_c.rotation_world_from_model );
  rtmesh_c.scale_world_from_model = 3.0f;
  rtmesh_c.scale_model_from_world = 1 / rtmesh_c.scale_world_from_model;
  rtmesh_c.translation_world = _vec3<f32>( 0, 16, 0 );
  rtmesh_c.surface_area_model = f32_2PI * rtmesh_c.cylinders.mem[0].radius * rtmesh_c.cylinders.mem[0].height;
  rtmesh_c.radiance_emit = _vec3( 0.0f );



  rtmesh_cone.reflectance = _vec3( 1.0f );
  rtmesh_cone.reflectance_tex = 0;
  rtmesh_cone.tris_tex = {};
  rtmesh_cone.tris = {};
  rtmesh_cone.parallelos_tex = {};
  rtmesh_cone.parallelos = {};
  rtmesh_cone.spheres = {};
  rtmesh_cone.discs = {};
  rtmesh_cone.cylinders = {};
  Alloc( rtmesh_cone.cones, 1 );
  {
    rtcone_t cone;
    cone.p = _vec3( 0.0f );
    cone.n = _vec3<f32>( 1, 0, 0 );
    cone.r = 0.5f;
    cone.r2 = Square( cone.r );
    cone.extent_pos = 1;
    cone.extent_neg = -1;
    *AddBack( rtmesh_cone.cones ) = cone;
  }
  Identity( &rtmesh_cone.rotation_world_from_model );
  Transpose( &rtmesh_cone.rotation_model_from_world, rtmesh_cone.rotation_world_from_model );
  rtmesh_cone.scale_world_from_model = 15.0f;
  rtmesh_cone.scale_model_from_world = 1 / rtmesh_cone.scale_world_from_model;
  rtmesh_cone.translation_world = _vec3<f32>( 0, 0, 0 );
  rtmesh_cone.surface_area_model = f32_2PI * rtmesh_cone.cones.mem[0].r2 * ( Square( rtmesh_cone.cones.mem[0].extent_neg ) + Square( rtmesh_cone.cones.mem[0].extent_neg ) );
  rtmesh_cone.radiance_emit = _vec3( 0.0f );

}

