// Copyright (c) John A. Carlos Jr., all rights reserved.


struct
fontglyph_t
{
  vec2<f32> offset;
  vec2<f32> tc0;
  vec2<f32> tc1;
  vec2<f32> dim;
  f32 advance;
};




struct
fontadvance_t
{
  u32 codept;
  fontglyph_t* glyph;
  // cumulative sum of x in the line.
  // this means we can lookup start/end only, and do constant-time measuring of sub-line strings.
  f32 xl_within_entire_line;
  f32 xr_within_entire_line;
};

struct
fontlinespan_t
{
  idx_t pos;
  idx_t len;
  f32 width_entire_line;
};

struct
fontlayout_t
{
  array_t<fontadvance_t> raw_advances;
  array_t<fontlinespan_t> advances_per_ln;
};

Inl void
FontInit( fontlayout_t& text )
{
  Alloc( text.raw_advances, 16 );
  Alloc( text.advances_per_ln, 16 );
}

Inl void
FontKill( fontlayout_t& text )
{
  Free( text.raw_advances );
  Free( text.advances_per_ln );
}

#define FontClear( text ) \
  do { \
    text.raw_advances.len = 0; \
    text.advances_per_ln.len = 0; \
  } while( 0 )

#define FontEmpty( text ) \
  ( text.raw_advances.len == 0 )


struct
font_t
{
  string_t ttf;
  stbtt_fontinfo info;
  array_t<idx_t> glyph_from_codept;
  array_t<fontglyph_t> glyphs;
  u32 texid;
  f32 scale;
  f32 ascent;
  f32 descent;
  f32 linegap;
#if OPENGL_INSTEAD_OF_SOFTWARE
#else // !OPENGL_INSTEAD_OF_SOFTWARE
  vec2<u32> tex_dim;
  u32* tex_mem;
#endif // !OPENGL_INSTEAD_OF_SOFTWARE
};


#define FontGetGlyph( font, codept ) \
  ( font.glyphs.mem + font.glyph_from_codept.mem[codept] )


void
FontLoad(
  font_t& font,
  u8* filename_ttf,
  idx_t filename_ttf_len,
  f32 px_char_h
  )
{
  ProfFunc();

  file_t file = FileOpen( filename_ttf, filename_ttf_len, fileopen_t::only_existing, fileop_t::R, fileop_t::RW );
  AssertCrash( file.loaded );
  font.ttf = FileAlloc( file );
  FileFree( file );
  AssertCrash( font.ttf.mem != 0 );

  stbtt_InitFont( &font.info, font.ttf.mem, stbtt_GetFontOffsetForIndex( font.ttf.mem, 0 ) );

  font.scale = stbtt_ScaleForPixelHeight( &font.info, px_char_h );

  s32 ascent, descent, linegap;
  stbtt_GetFontVMetrics( &font.info, &ascent, &descent, &linegap );
  font.ascent = font.scale * ascent;
  font.descent = -font.scale * descent;
  font.linegap = CLAMP( font.scale * linegap, 1.0f, 2.0f );

  Alloc( font.glyph_from_codept, 1024 );
  font.glyph_from_codept.len = 1024;

  Alloc( font.glyphs, 1024 );
}

void
FontKill( font_t& font )
{
#if OPENGL_INSTEAD_OF_SOFTWARE
#else // !OPENGL_INSTEAD_OF_SOFTWARE
  font.tex_dim = {};
  MemHeapFree( font.tex_mem );
#endif // !OPENGL_INSTEAD_OF_SOFTWARE
  Free( font.glyph_from_codept );
  Free( font.glyphs );
  Free( font.ttf );
}


#define FontLineH( font ) \
  ( font.ascent + font.descent + font.linegap )


#define GLYGPHGEN 5
// 0 -- straight from stbtt
// 1 -- subpixel bgr blending
// 2 -- simple sharpen filter, border px passthru.
// 3 -- simple sharpen filter
// 4 -- sharpen filter
// 5 -- sharpen filter with subpixel bgr blending.

u32*
FontLoadGlyphImage(
  font_t& font,
  u32 codept,
  vec2<f32>& dimf,
  vec2<f32>& offsetf
  )
{
  ProfFunc();

#if GLYGPHGEN == 0
  vec2<s32> dim, offset;
  auto img_alpha = stbtt_GetCodepointBitmap(
    &font.info,
    font.scale,
    font.scale,
    codept,
    &dim.x,
    &dim.y,
    &offset.x,
    &offset.y
    );
  dimf = _vec2( Cast( f32, dim.x ), Cast( f32, dim.y ) );
  offsetf = _vec2( Cast( f32, offset.x ), Cast( f32, offset.y ) );
  auto img_alpha_len = dim.x * dim.y;
  auto img = MemHeapAlloc( u32, img_alpha_len );
  For( i, 0, img_alpha_len ) {
    auto dst = img + i;
    auto& src = img_alpha[i];
    *dst =
      ( src << 24 ) |
      ( src << 16 ) |
      ( src <<  8 ) |
      ( src <<  0 );
  }
  stbtt_FreeBitmap( img_alpha, 0 );
  return img;
#elif GLYGPHGEN == 1
  vec2<s32> dim_b, offset_b;
  auto img_b = stbtt_GetCodepointBitmapSubpixel(
    &font.info,
    font.scale,
    font.scale,
    0.0f / 3.0f,
    0.0f,
    codept,
    &dim_b.x,
    &dim_b.y,
    &offset_b.x,
    &offset_b.y
    );
  vec2<s32> dim_g, offset_g;
  auto img_g = stbtt_GetCodepointBitmapSubpixel(
    &font.info,
    font.scale,
    font.scale,
    1.0f / 3.0f,
    0.0f,
    codept,
    &dim_g.x,
    &dim_g.y,
    &offset_g.x,
    &offset_g.y
    );
  vec2<s32> dim_r, offset_r;
  auto img_r = stbtt_GetCodepointBitmapSubpixel(
    &font.info,
    font.scale,
    font.scale,
    2.0f / 3.0f,
    0.0f,
    codept,
    &dim_r.x,
    &dim_r.y,
    &offset_r.x,
    &offset_r.y
    );

  AssertWarn( dim_r.y == dim_g.y  &&  dim_g.y == dim_b.y );

  auto dim = MAX( MAX( dim_r, dim_g ), dim_b );
  dim.x += 4;

  auto offset = offset_b;

  auto align_r = offset_r.x - offset_b.x;
  auto align_g = offset_g.x - offset_b.x;
  auto align_b = 0;

  auto dim_blend = _vec2( dim.x, dim.y );
  auto img_blend_len = dim_blend.x * dim_blend.y;
  auto img_blend = MemHeapAlloc( vec4<f32>, img_blend_len );
  Memzero( img_blend, img_blend_len * sizeof( vec4<f32> ) );

  Fori( s32, j, 0, dim.y ) {
    Fori( s32, i, 0, dim_r.x ) {
      auto idx = i + align_r;
      if( idx < dim_blend.x ) {
        auto& dst = img_blend[ j * dim_blend.x + idx ];
        auto val = Cast( f32, img_r[ j * dim_r.x + i ] ) / 255.0f;
        dst.x += val;
        dst.w += val / 3;
      }
    }
    Fori( s32, i, 0, dim_g.x ) {
      auto idx = i + align_g;
      if( idx < dim_blend.x ) {
        auto& dst = img_blend[ j * dim_blend.x + idx ];
        auto val = Cast( f32, img_g[ j * dim_g.x + i ] ) / 255.0f;
        dst.y += val;
        dst.w += val / 3;
      }
    }
    Fori( s32, i, 0, dim_b.x ) {
      auto idx = i + align_b;
      if( idx < dim_blend.x ) {
        auto& dst = img_blend[ j * dim_blend.x + idx ];
        auto val = Cast( f32, img_b[ j * dim_b.x + i ] ) / 255.0f;
        dst.z += val;
        dst.w += val / 3;
      }
    }
  }

  auto img_output_len = dim.x * dim.y;
  auto img_output = MemHeapAlloc( u32, img_output_len );
  Fori( s32, i, 0, img_output_len ) {
    auto& src = img_blend[i];
    auto& dst = img_output[i];
    dst =
      ( Cast( u8, src.x * 255.0f + 0.0001f ) <<  0 ) |
      ( Cast( u8, src.y * 255.0f + 0.0001f ) <<  8 ) |
      ( Cast( u8, src.z * 255.0f + 0.0001f ) << 16 ) |
      ( Cast( u8, src.w * 255.0f + 0.0001f ) << 24 );
  }
  dimf = _vec2( Cast( f32, dim.x ), Cast( f32, dim.y ) );
  offsetf = _vec2( Cast( f32, offset.x ), Cast( f32, offset.y ) );
  MemHeapFree( img_blend );
  stbtt_FreeBitmap( img_r, 0 );
  stbtt_FreeBitmap( img_g, 0 );
  stbtt_FreeBitmap( img_b, 0 );
  return img_output;
#elif GLYGPHGEN == 2
  vec2<s32> dim, offset;
  auto img_alpha = stbtt_GetCodepointBitmap(
    &font.info,
    font.scale,
    font.scale,
    codept,
    &dim.x,
    &dim.y,
    &offset.x,
    &offset.y
    );

  auto dim_blend = _vec2( dim.x, dim.y );
  auto img_blend_len = dim_blend.x * dim_blend.y;
  auto img_blend = MemHeapAlloc( vec4<f32>, img_blend_len );
  Memzero( img_blend, img_blend_len * sizeof( vec4<f32> ) );

  Fori( s32, j, 0, dim.y ) {
    {
      auto& dst = img_blend[ j * dim_blend.x + 0 ];
      auto val = Cast( f32, img_alpha[ j * dim.x + 0 ] ) / 255.0f;
      dst = _vec4( val );
    }
    {
      auto& dst = img_blend[ j * dim_blend.x + dim.x - 1 ];
      auto val = Cast( f32, img_alpha[ j * dim.x + dim.x - 1 ] ) / 255.0f;
      dst = _vec4( val );
    }
  }
  Fori( s32, i, 0, dim.x ) {
    {
      auto& dst = img_blend[ 0 * dim_blend.x + i ];
      auto val = Cast( f32, img_alpha[ 0 * dim.x + i ] ) / 255.0f;
      dst = _vec4( val );
    }
    {
      auto& dst = img_blend[ ( dim.y - 1 ) * dim_blend.x + i ];
      auto val = Cast( f32, img_alpha[ ( dim.y - 1 ) * dim.x + i ] ) / 255.0f;
      dst = _vec4( val );
    }
  }
  Fori( s32, j, 1, dim.y - 1 ) {
    Fori( s32, i, 1, dim.x - 1 ) {
      auto& dst = img_blend[ j * dim_blend.x + i ];
      static const f32 coeffs[] = {
         0, -1,  0,
        -1,  5, -1,
         0, -1,  0,
      };
      f32 vals[] = {
        Cast( f32, img_alpha[ ( j - 1 ) * dim.x + i - 1 ] ) / 255.0f,
        Cast( f32, img_alpha[ ( j - 1 ) * dim.x + i + 0 ] ) / 255.0f,
        Cast( f32, img_alpha[ ( j - 1 ) * dim.x + i + 1 ] ) / 255.0f,
        Cast( f32, img_alpha[ ( j + 0 ) * dim.x + i - 1 ] ) / 255.0f,
        Cast( f32, img_alpha[ ( j + 0 ) * dim.x + i + 0 ] ) / 255.0f,
        Cast( f32, img_alpha[ ( j + 0 ) * dim.x + i + 1 ] ) / 255.0f,
        Cast( f32, img_alpha[ ( j + 1 ) * dim.x + i - 1 ] ) / 255.0f,
        Cast( f32, img_alpha[ ( j + 1 ) * dim.x + i + 0 ] ) / 255.0f,
        Cast( f32, img_alpha[ ( j + 1 ) * dim.x + i + 1 ] ) / 255.0f,
      };
      f32 val = 0;
      For( k, 0, _countof( coeffs ) ) {
        val += coeffs[k] * vals[k];
      }
      dst = _vec4( CLAMP( val, 0, 1 ) );
    }
  }

  auto img_output_len = dim.x * dim.y;
  auto img_output = MemHeapAlloc( u32, img_output_len );
  Fori( s32, i, 0, img_output_len ) {
    auto& src = img_blend[i];
    auto& dst = img_output[i];
    dst =
      ( Cast( u8, src.x * 255.0f + 0.0001f ) <<  0 ) |
      ( Cast( u8, src.y * 255.0f + 0.0001f ) <<  8 ) |
      ( Cast( u8, src.z * 255.0f + 0.0001f ) << 16 ) |
      ( Cast( u8, src.w * 255.0f + 0.0001f ) << 24 );
  }
  dimf = _vec2( Cast( f32, dim.x ), Cast( f32, dim.y ) );
  offsetf = _vec2( Cast( f32, offset.x ), Cast( f32, offset.y ) );
  MemHeapFree( img_blend );
  stbtt_FreeBitmap( img_alpha, 0 );
  return img_output;
#elif GLYGPHGEN == 3
  vec2<s32> dim, offset;
  auto img_alpha = stbtt_GetCodepointBitmap(
    &font.info,
    font.scale,
    font.scale,
    codept,
    &dim.x,
    &dim.y,
    &offset.x,
    &offset.y
    );

  auto dim_blend = _vec2( dim.x, dim.y );
  auto img_blend_len = dim_blend.x * dim_blend.y;
  auto img_blend = MemHeapAlloc( vec4<f32>, img_blend_len );
  Memzero( img_blend, img_blend_len * sizeof( vec4<f32> ) );

  Fori( s32, j, 0, dim.y ) {
    Fori( s32, i, 0, dim.x ) {
      auto& dst = img_blend[ j * dim_blend.x + i ];
      static const f32 coeffs[] = {
         0, -1,  0,
        -1,  5, -1,
         0, -1,  0,
      };
      f32 vals[] = {
        ( j == 0  ||  i == 0 )                  ?  0.0f  :  Cast( f32, img_alpha[ ( j - 1 ) * dim.x + i - 1 ] ) / 255.0f,
        ( j == 0 )                              ?  0.0f  :  Cast( f32, img_alpha[ ( j - 1 ) * dim.x + i + 0 ] ) / 255.0f,
        ( j == 0  ||  i == dim.x - 1 )          ?  0.0f  :  Cast( f32, img_alpha[ ( j - 1 ) * dim.x + i + 1 ] ) / 255.0f,
        ( i == 0 )                              ?  0.0f  :  Cast( f32, img_alpha[ ( j + 0 ) * dim.x + i - 1 ] ) / 255.0f,
                                                            Cast( f32, img_alpha[ ( j + 0 ) * dim.x + i + 0 ] ) / 255.0f,
        ( i == dim.x - 1 )                      ?  0.0f  :  Cast( f32, img_alpha[ ( j + 0 ) * dim.x + i + 1 ] ) / 255.0f,
        ( j == dim.y - 1  ||  i == 0 )          ?  0.0f  :  Cast( f32, img_alpha[ ( j + 1 ) * dim.x + i - 1 ] ) / 255.0f,
        ( j == dim.y - 1 )                      ?  0.0f  :  Cast( f32, img_alpha[ ( j + 1 ) * dim.x + i + 0 ] ) / 255.0f,
        ( j == dim.y - 1  ||  i == dim.x - 1 )  ?  0.0f  :  Cast( f32, img_alpha[ ( j + 1 ) * dim.x + i + 1 ] ) / 255.0f,
      };
      f32 val = 0;
      For( k, 0, _countof( coeffs ) ) {
        val += coeffs[k] * vals[k];
      }
      dst = _vec4( CLAMP( val, 0, 1 ) );
    }
  }

  auto img_output_len = dim.x * dim.y;
  auto img_output = MemHeapAlloc( u32, img_output_len );
  Fori( s32, i, 0, img_output_len ) {
    auto& src = img_blend[i];
    auto& dst = img_output[i];
    dst =
      ( Cast( u8, src.x * 255.0f + 0.0001f ) <<  0 ) |
      ( Cast( u8, src.y * 255.0f + 0.0001f ) <<  8 ) |
      ( Cast( u8, src.z * 255.0f + 0.0001f ) << 16 ) |
      ( Cast( u8, src.w * 255.0f + 0.0001f ) << 24 );
  }
  dimf = _vec2( Cast( f32, dim.x ), Cast( f32, dim.y ) );
  offsetf = _vec2( Cast( f32, offset.x ), Cast( f32, offset.y ) );
  MemHeapFree( img_blend );
  stbtt_FreeBitmap( img_alpha, 0 );
  return img_output;
#elif GLYGPHGEN == 4
  vec2<s32> dim, offset;
  auto img_alpha = stbtt_GetCodepointBitmap(
    &font.info,
    font.scale,
    font.scale,
    codept,
    &dim.x,
    &dim.y,
    &offset.x,
    &offset.y
    );

  auto dim_blend = _vec2( dim.x, dim.y );
  auto img_blend_len = dim_blend.x * dim_blend.y;
  auto img_blend = MemHeapAlloc( vec4<f32>, img_blend_len );
  Memzero( img_blend, img_blend_len * sizeof( vec4<f32> ) );

  Fori( s32, j, 0, dim.y ) {
    Fori( s32, i, 0, dim.x ) {
      auto& dst = img_blend[ j * dim_blend.x + i ];
      static const f32 coeffs[] = {
        1 / 16.0f, 2 / 16.0f, 1 / 16.0f,
        2 / 16.0f, 4 / 16.0f, 2 / 16.0f,
        1 / 16.0f, 2 / 16.0f, 1 / 16.0f,
      };
      f32 vals[] = {
        ( j == 0  ||  i == 0 )                  ?  0.0f  :  Cast( f32, img_alpha[ ( j - 1 ) * dim.x + i - 1 ] ) / 255.0f,
        ( j == 0 )                              ?  0.0f  :  Cast( f32, img_alpha[ ( j - 1 ) * dim.x + i + 0 ] ) / 255.0f,
        ( j == 0  ||  i == dim.x - 1 )          ?  0.0f  :  Cast( f32, img_alpha[ ( j - 1 ) * dim.x + i + 1 ] ) / 255.0f,
        ( i == 0 )                              ?  0.0f  :  Cast( f32, img_alpha[ ( j + 0 ) * dim.x + i - 1 ] ) / 255.0f,
                                                            Cast( f32, img_alpha[ ( j + 0 ) * dim.x + i + 0 ] ) / 255.0f,
        ( i == dim.x - 1 )                      ?  0.0f  :  Cast( f32, img_alpha[ ( j + 0 ) * dim.x + i + 1 ] ) / 255.0f,
        ( j == dim.y - 1  ||  i == 0 )          ?  0.0f  :  Cast( f32, img_alpha[ ( j + 1 ) * dim.x + i - 1 ] ) / 255.0f,
        ( j == dim.y - 1 )                      ?  0.0f  :  Cast( f32, img_alpha[ ( j + 1 ) * dim.x + i + 0 ] ) / 255.0f,
        ( j == dim.y - 1  ||  i == dim.x - 1 )  ?  0.0f  :  Cast( f32, img_alpha[ ( j + 1 ) * dim.x + i + 1 ] ) / 255.0f,
      };
      f32 val = 0;
      For( k, 0, _countof( coeffs ) ) {
        val += coeffs[k] * vals[k];
      }
      static const f32 strength = 1.0f;
      val = vals[4] + strength * ( vals[4] - val );
      val = CLAMP( val, 0, 1 );
      dst = _vec4( val );
    }
  }

  auto img_output_len = dim.x * dim.y;
  auto img_output = MemHeapAlloc( u32, img_output_len );
  Fori( s32, i, 0, img_output_len ) {
    auto& src = img_blend[i];
    auto& dst = img_output[i];
    dst =
      ( Cast( u8, src.x * 255.0f + 0.0001f ) <<  0 ) |
      ( Cast( u8, src.y * 255.0f + 0.0001f ) <<  8 ) |
      ( Cast( u8, src.z * 255.0f + 0.0001f ) << 16 ) |
      ( Cast( u8, src.w * 255.0f + 0.0001f ) << 24 );
  }
  dimf = _vec2( Cast( f32, dim.x ), Cast( f32, dim.y ) );
  offsetf = _vec2( Cast( f32, offset.x ), Cast( f32, offset.y ) );
  MemHeapFree( img_blend );
  stbtt_FreeBitmap( img_alpha, 0 );
  return img_output;
#elif GLYGPHGEN == 5
  vec2<s32> dim_b, offset_b;
  auto img_b = stbtt_GetCodepointBitmapSubpixel(
    &font.info,
    font.scale,
    font.scale,
    0.0f / 3.0f,
    0.0f,
    codept,
    &dim_b.x,
    &dim_b.y,
    &offset_b.x,
    &offset_b.y
    );
  vec2<s32> dim_g, offset_g;
  auto img_g = stbtt_GetCodepointBitmapSubpixel(
    &font.info,
    font.scale,
    font.scale,
    1.0f / 6.0f,
    0.0f,
    codept,
    &dim_g.x,
    &dim_g.y,
    &offset_g.x,
    &offset_g.y
    );
  vec2<s32> dim_r, offset_r;
  auto img_r = stbtt_GetCodepointBitmapSubpixel(
    &font.info,
    font.scale,
    font.scale,
    1.0f / 3.0f,
    0.0f,
    codept,
    &dim_r.x,
    &dim_r.y,
    &offset_r.x,
    &offset_r.y
    );

  AssertWarn( dim_r.y == dim_g.y  &&  dim_g.y == dim_b.y );

  auto dim = Max( Max( dim_r, dim_g ), dim_b );

  // add space on left and right so we don't lose any info when blending.
  static const s32 spacing = 4;
  static const s32 half_spacing = 2;
  dim.x += spacing;

  // align everything to b, and move everything to the right 2px.
  auto offset = offset_b - _vec2<s32>( half_spacing, 0 );
  auto align_r = half_spacing + offset_r.x - offset_b.x;
  auto align_g = half_spacing + offset_g.x - offset_b.x;
  auto align_b = half_spacing + 0;

  auto img_blend_len = dim.x * dim.y;
  auto img_blend = MemHeapAlloc( vec4<f32>, img_blend_len );
  Memzero( img_blend, img_blend_len * sizeof( vec4<f32> ) );

  Fori( s32, j, 0, dim.y ) {
    Fori( s32, i, 0, dim_r.x ) {
      auto idx = i + align_r;
      if( 0 <= idx  &&  idx < dim.x ) {
        auto& dst = img_blend[ j * dim.x + idx ];
        auto val = Cast( f32, img_r[ j * dim_r.x + i ] ) / 255.0f;
        dst.x += val;
        dst.w += val / 3;
      }
    }
    Fori( s32, i, 0, dim_g.x ) {
      auto idx = i + align_g;
      if( 0 <= idx  &&  idx < dim.x ) {
        auto& dst = img_blend[ j * dim.x + idx ];
        auto val = Cast( f32, img_g[ j * dim_g.x + i ] ) / 255.0f;
        dst.y += val;
        dst.w += val / 3;
      }
    }
    Fori( s32, i, 0, dim_b.x ) {
      auto idx = i + align_b;
      if( 0 <= idx  &&  idx < dim.x ) {
        auto& dst = img_blend[ j * dim.x + idx ];
        auto val = Cast( f32, img_b[ j * dim_b.x + i ] ) / 255.0f;
        dst.z += val;
        dst.w += val / 3;
      }
    }
  }

#if 1
  auto img_tmp = MemHeapAlloc( vec4<f32>, dim.x * dim.y );
  Fori( s32, j, 0, dim.y ) {
    Fori( s32, i, 0, dim.x ) {
      static const f32 coeff = 1 / 28.0f;
      static const f32 coeffs[] = {
        -1, -2, -1,
        -2, 40, -2,
        -1, -2, -1,
      };
      vec4<f32> vals[] = {
        ( j == 0  ||  i == 0 )                  ?  _vec4( 0.0f )  :  img_blend[ ( j - 1 ) * dim.x + i - 1 ],
        ( j == 0 )                              ?  _vec4( 0.0f )  :  img_blend[ ( j - 1 ) * dim.x + i + 0 ],
        ( j == 0  ||  i == dim.x - 1 )          ?  _vec4( 0.0f )  :  img_blend[ ( j - 1 ) * dim.x + i + 1 ],
        ( i == 0 )                              ?  _vec4( 0.0f )  :  img_blend[ ( j + 0 ) * dim.x + i - 1 ],
                                                                     img_blend[ ( j + 0 ) * dim.x + i + 0 ],
        ( i == dim.x - 1 )                      ?  _vec4( 0.0f )  :  img_blend[ ( j + 0 ) * dim.x + i + 1 ],
        ( j == dim.y - 1  ||  i == 0 )          ?  _vec4( 0.0f )  :  img_blend[ ( j + 1 ) * dim.x + i - 1 ],
        ( j == dim.y - 1 )                      ?  _vec4( 0.0f )  :  img_blend[ ( j + 1 ) * dim.x + i + 0 ],
        ( j == dim.y - 1  ||  i == dim.x - 1 )  ?  _vec4( 0.0f )  :  img_blend[ ( j + 1 ) * dim.x + i + 1 ],
      };
      vec4<f32> val = {};
      For( k, 0, _countof( coeffs ) ) {
        val += _vec4( coeff * coeffs[k] ) * vals[k];
      }
      auto& dst = img_tmp[ j * dim.x + i ];
      dst = val;
    }
  }

  Fori( s32, j, 0, dim.y ) {
    Fori( s32, i, 0, dim.x ) {
      auto& src = img_tmp[ j * dim.x + i ];
      auto& dst = img_blend[ j * dim.x + i ];
      dst = src;
    }
  }
  MemHeapFree( img_tmp );
#endif

#if 0
  ReverseFori( s32, j, 0, dim.y - 1 ) {
    ReverseFori( s32, i, 0, dim.x - 1 ) {
      auto& src00 = img_blend[ ( j + 0 ) * dim.x + i + 0 ];
      auto& src10 = img_blend[ ( j + 0 ) * dim.x + i + 1 ];
      auto& src01 = img_blend[ ( j + 1 ) * dim.x + i + 0 ];
      auto& src11 = img_blend[ ( j + 1 ) * dim.x + i + 1 ];

      bool close =
        ( ( src00.w - src10.w ) < 0.1f )  &
        ( ( src00.w - src01.w ) < 0.1f )  &
        ( ( src00.w - src11.w ) < 0.2f );

      if( close  &&  0.5f < src00.w  &&  src00.w < 0.75f ) {
        src00 /= src00.w;
        //src10 *= 1.0f;
        //src01 *= 1.0f;
        //src11 *= 1.0f;
      }
    }
  }
#endif

  auto img_output_len = dim.x * dim.y;
  auto img_output = MemHeapAlloc( u32, img_output_len );
  Fori( s32, j, 0, dim.y ) {
    Fori( s32, i, 0, dim.x ) {
      static const f32 coeffs[] = {
        1 / 16.0f, 2 / 16.0f, 1 / 16.0f,
        2 / 16.0f, 4 / 16.0f, 2 / 16.0f,
        1 / 16.0f, 2 / 16.0f, 1 / 16.0f,
      };
      vec4<f32> vals[] = {
        ( j == 0  ||  i == 0 )                  ?  _vec4( 0.0f )  :  img_blend[ ( j - 1 ) * dim.x + i - 1 ],
        ( j == 0 )                              ?  _vec4( 0.0f )  :  img_blend[ ( j - 1 ) * dim.x + i + 0 ],
        ( j == 0  ||  i == dim.x - 1 )          ?  _vec4( 0.0f )  :  img_blend[ ( j - 1 ) * dim.x + i + 1 ],
        ( i == 0 )                              ?  _vec4( 0.0f )  :  img_blend[ ( j + 0 ) * dim.x + i - 1 ],
                                                                     img_blend[ ( j + 0 ) * dim.x + i + 0 ],
        ( i == dim.x - 1 )                      ?  _vec4( 0.0f )  :  img_blend[ ( j + 0 ) * dim.x + i + 1 ],
        ( j == dim.y - 1  ||  i == 0 )          ?  _vec4( 0.0f )  :  img_blend[ ( j + 1 ) * dim.x + i - 1 ],
        ( j == dim.y - 1 )                      ?  _vec4( 0.0f )  :  img_blend[ ( j + 1 ) * dim.x + i + 0 ],
        ( j == dim.y - 1  ||  i == dim.x - 1 )  ?  _vec4( 0.0f )  :  img_blend[ ( j + 1 ) * dim.x + i + 1 ],
      };
//      if( vals[4][3] > 0.5f ) {
//        vals[4] *= vals[4];
//      }

      vec4<f32> val = {};
      For( k, 0, _countof( coeffs ) ) {
        val += _vec4( coeffs[k] ) * vals[k];
      }

      static const f32 sharpen_strength = 1.0f;
      auto& src = img_blend[ j * dim.x + i ];
      val = src + sharpen_strength * ( src - val );

      static const f32 max_value = 1.0f;
      static const f32 rec_max_value = 1 / max_value;
      val = Clamp( val * rec_max_value, _vec4<f32>( 0 ), _vec4<f32>( 1 ) );

      auto& dst = img_output[ j * dim.x + i ];
      dst =
        ( Cast( u8, val.x * 255.0f + 0.0001f ) <<  0 ) |
        ( Cast( u8, val.y * 255.0f + 0.0001f ) <<  8 ) |
        ( Cast( u8, val.z * 255.0f + 0.0001f ) << 16 ) |
        ( Cast( u8, val.w * 255.0f + 0.0001f ) << 24 );
    }
  }

  dimf = _vec2( Cast( f32, dim.x ), Cast( f32, dim.y ) );
  offsetf = _vec2( Cast( f32, offset.x ), Cast( f32, offset.y ) );
  MemHeapFree( img_blend );
  stbtt_FreeBitmap( img_r, 0 );
  stbtt_FreeBitmap( img_g, 0 );
  stbtt_FreeBitmap( img_b, 0 );
  return img_output;
#else
#error GLYPHGEN
#endif

}


struct
fontglyphimg_t
{
  u32* img;
  vec2<s32> dim;
  vec2<f32> dimf;
};

Inl void
FontLoadAscii( font_t& font )
{
  ProfFunc();

  array_t<fontglyphimg_t> glyphimgs;
  Alloc( glyphimgs, 128 );

  // TODO: load extended chars that we want.

  //Fori( u32, codept, ' ', '~' + 1 ) {
  Fori( u32, codept, 0, 256 ) {
    auto glyph_idx = font.glyphs.len;

    fontglyph_t glyph = {};
    fontglyphimg_t glyphimg = {};
    glyphimg.img = FontLoadGlyphImage(
      font,
      codept,
      glyph.dim,
      glyph.offset
      );

    glyphimg.dimf = glyph.dim + _vec2<f32>( 1, 1 );
    glyphimg.dim = _vec2( Round_s32_from_f32( glyphimg.dimf.x ), Round_s32_from_f32( glyphimg.dimf.y ) );

    s32 advance;
    stbtt_GetCodepointHMetrics( &font.info, Cast( s32, codept ), &advance, 0 );
    glyph.advance = font.scale * advance;

    *AddBack( font.glyphs ) = glyph;
    *AddBack( glyphimgs ) = glyphimg;

    Reserve( font.glyph_from_codept, codept + 1 );
    font.glyph_from_codept.len = MAX( font.glyph_from_codept.len, codept + 1 );
    font.glyph_from_codept.mem[codept] = glyph_idx;
  }

  kahan32_t x = {};
  kahan32_t y = {};
  f32 delta_y = 0;

  static const auto maxdim = _vec2<f32>( 512, 4096 );

  // reserve space for 1px white.
  Add( x, 1.0f );
  delta_y = 1.0f;

  AssertCrash( font.glyphs.len == glyphimgs.len );
  ForLen( i, font.glyphs ) {
    auto glyph = font.glyphs.mem + i;
    auto glyphimg = glyphimgs.mem + i;

    AssertCrash( y.sum + glyphimg->dimf.y <= maxdim.y ); // out of room!

    if( x.sum + glyphimg->dimf.x > maxdim.x ) {
      Add( y, delta_y );
      y.sum = Ceil32( y.sum );
      y.err = 0;
      delta_y = 0;
      x = {};
      --i; // retry this glyph.
    } else {
      delta_y = MAX( delta_y, glyphimg->dimf.y );
      glyph->tc0 = _vec2( x.sum, y.sum );
      glyph->tc1 = glyph->tc0 + glyph->dim;
      Add( x, glyphimg->dimf.x );
      x.sum = Ceil32( x.sum );
      x.err = 0;
    }
  }
  Add( y, delta_y );
  y.sum = Ceil32( y.sum );
  y.err = 0;

  auto dimf = _vec2( maxdim.x, y.sum );
  auto dim = _vec2( Round_s32_from_f32( dimf.x ), Round_s32_from_f32( dimf.y ) );
  auto img = MemHeapAlloc( u32, dim.x * dim.y );

  // write first px white.
  *img = MAX_u32;

  AssertCrash( font.glyphs.len == glyphimgs.len );
  ForLen( i, font.glyphs ) {
    auto glyph = font.glyphs.mem + i;
    auto glyphimg = glyphimgs.mem + i;

    auto p0 = _vec2( Round_s32_from_f32( glyph->tc0.x ), Round_s32_from_f32( glyph->tc0.y ) );
    auto glyphdim = _vec2( Round_s32_from_f32( glyph->dim.x ), Round_s32_from_f32( glyph->dim.y ) );
    Fori( s32, j, 0, glyphdim.y ) {
      Fori( s32, k, 0, glyphdim.x ) {
        auto dst = img + ( p0.y + j ) * dim.x + ( p0.x + k );
        auto src = glyphimg->img + j * glyphdim.x + k;
        *dst = *src;
      }
    }

#if OPENGL_INSTEAD_OF_SOFTWARE
    glyph->tc0 /= dimf;
    glyph->tc1 /= dimf;
#else // !OPENGL_INSTEAD_OF_SOFTWARE
    // leave tc's as absolute integers, to save some computation later.
#endif // !OPENGL_INSTEAD_OF_SOFTWARE
  }

  ForLen( i, glyphimgs ) {
    auto glyphimg = glyphimgs.mem + i;
    MemHeapFree( glyphimg->img );
  }
  Free( glyphimgs );

#if OPENGL_INSTEAD_OF_SOFTWARE
  // TODO: consider switching the opengl here to use the GlwUploadTexture path.

  glDeleteTextures( 1, &font.texid );
  glGenTextures( 1, &font.texid );
  glBindTexture( GL_TEXTURE_2D, font.texid );  glVerify();
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA,
    dim.x,
    dim.y,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    img
    );
  // NOTE: we use nearest filtering, since it looks a tiny bit better if we get the 1:1 pixel drawing wrong.
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );  glVerify();
  glGenerateMipmap( GL_TEXTURE_2D );  glVerify();

  glBindTexture( GL_TEXTURE_2D, 0 );  glVerify();

  MemHeapFree( img );
#else // !OPENGL_INSTEAD_OF_SOFTWARE

  font.tex_dim = _vec2( Cast( u32, dim.x ), Cast( u32, dim.y ) );
  font.tex_mem = img;
#endif // !OPENGL_INSTEAD_OF_SOFTWARE
}


#define FontGetAdvance( font, codept ) \
  ( FontGetGlyph( font, codept )->advance )


Inl f32
FontGetKerning( font_t& font, u32 codept, u32 codept2 )
{
  // PERF: fixed-size cache, some replacement policy
  auto advance = stbtt_GetCodepointKernAdvance( &font.info, Cast( s32, codept ), Cast( s32, codept2 ) );
  return font.scale * advance;
}



void
FontAddLayoutLine(
  font_t& font,
  fontlayout_t& layout,
  u8* text,
  idx_t text_len,
  u32 spaces_per_tab
  )
{
  ProfFunc();

  auto linespan = AddBack( layout.advances_per_ln );
  linespan->pos = layout.raw_advances.len;
  linespan->len = 0;
  kahan32_t x = {};
  For( i, 0, text_len ) {
    u32 codept = text[i];
    if( codept == '\t' ) {
      auto adv = AddBack( layout.raw_advances );
      adv->codept = ' ';
      adv->glyph = FontGetGlyph( font, adv->codept );
      adv->xl_within_entire_line = x.sum;
      Add( x, spaces_per_tab * adv->glyph->advance );
      adv->xr_within_entire_line = x.sum;
      linespan->len += 1;
    } else {
      auto adv = AddBack( layout.raw_advances );
      adv->codept = codept;
      adv->glyph = FontGetGlyph( font, adv->codept );
      adv->xl_within_entire_line = x.sum;
      if( i + 1 < text_len ) {
        u32 codept2 = text[i + 1];
        Add( x, adv->glyph->advance + FontGetKerning( font, codept, codept2 ) );
      } else {
        Add( x, adv->glyph->advance );
      }
      adv->xr_within_entire_line = x.sum;
      linespan->len += 1;
    }
  }
  linespan->width_entire_line = x.sum;
}

Inl f32
FontSumAdvances(
  fontlayout_t& layout,
  idx_t line,
  idx_t char_offset,
  idx_t char_len
  )
{
//  ProfFunc();

  if( !char_len ) {
    return 0.0f;
  }

  AssertCrash( line < layout.advances_per_ln.len );
  auto linespan = layout.advances_per_ln.mem + line;
  auto width = linespan->width_entire_line;
  AssertCrash( char_offset + char_len <= linespan->len ); // addressing out of bounds from what you laid out previously!
  AssertCrash( linespan->pos + char_offset < layout.raw_advances.len );
  auto first = layout.raw_advances.mem + linespan->pos + char_offset;
  width -= first->xl_within_entire_line;
  if( char_offset + char_len < linespan->len ) {
    auto last = first + char_len;
    width -= ( linespan->width_entire_line - last->xl_within_entire_line );
  }
  return width;
}

struct
charspan_t
{
  u32 char_idx_in_line;
  f32 xl;
  f32 xr;
};

Inl void
FontGetCharSpans(
  fontlayout_t& layout,
  idx_t line, // TODO: convert txt.linespans/charspans/etc. to array32_t.
  idx_t char_offset, // TODO: convert txt.linespans/charspans/etc. to array32_t.
  idx_t char_len, // TODO: convert txt.linespans/charspans/etc. to array32_t.
  f32 x0,
  array_t<charspan_t>* charspans
  )
{
//  ProfFunc();

  if( !char_len ) {
    return;
  }

  AssertCrash( line < layout.advances_per_ln.len );
  auto linespan = layout.advances_per_ln.mem + line;
  AssertCrash( char_offset + char_len <= linespan->len ); // addressing out of bounds from what you laid out previously!
  AssertCrash( linespan->pos + char_offset < layout.raw_advances.len );
  auto first = layout.raw_advances.mem + linespan->pos + char_offset;
  auto dx_from_first = -first->xl_within_entire_line;
  For( i, 0, char_len ) { // TODO: convert txt.linespans/charspans/etc. to array32_t.
    auto raw_idx = linespan->pos + i + char_offset;
    auto adv = layout.raw_advances.mem + raw_idx;
    auto span = AddBack( *charspans );
    span->xl = x0 + adv->xl_within_entire_line + dx_from_first;
    span->xr = x0 + adv->xr_within_entire_line + dx_from_first;
    span->char_idx_in_line = Cast( u32, i + char_offset ); // TODO: convert txt.linespans/charspans/etc. to array32_t.
  }
}
