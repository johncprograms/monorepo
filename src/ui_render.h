// Copyright (c) John A. Carlos Jr., all rights reserved.

#if OPENGL_INSTEAD_OF_SOFTWARE

  struct
  shader_tex2_t
  {
    glwshader_t core;
    s32 loc_ndc_from_client;
    s32 loc_tex_sampler;
    s32 attribloc_pos;
    s32 attribloc_tccolor;
  };

  void
  ShaderInit( shader_tex2_t& shader )
  {
    shader.core = GlwLoadShader(
      R"STRING(
        #version 330 core
        layout( location = 0 ) in vec3 vertex_pos;
        layout( location = 1 ) in vec3 vertex_tccolor;
        out vec3 tccolor;
        uniform mat4 ndc_from_client;
        void main() {
          gl_Position = ndc_from_client * vec4( vertex_pos, 1 );
          tccolor = vertex_tccolor;
        }
      )STRING",

  #if 1
      R"STRING(
        #version 330 core
        in vec3 tccolor;
        layout( location = 0 ) out vec4 pixel;
        uniform sampler2D tex_sampler;
        void main() {
          vec2 tc = tccolor.xy;
          uint colorbits = uint( tccolor.z );
          vec4 color;
          color.x = ( ( colorbits >>  0u ) & 63u ) / 63.0f;
          color.y = ( ( colorbits >>  6u ) & 63u ) / 63.0f;
          color.z = ( ( colorbits >> 12u ) & 63u ) / 63.0f;
          color.w = ( ( colorbits >> 18u ) & 31u ) / 31.0f;
          vec4 texel = texture( tex_sampler, tc );
          pixel = color * texel;
        }
      )STRING"
  #else
      R"STRING(
        #version 330 core
        in vec3 tccolor;
        layout( location = 0 ) out vec4 pixel;
        uniform sampler2D tex_sampler;
        void main() {
          vec2 tc = tccolor.xy;
          uint colorbits = floatBitsToUint( tccolor.z );
          vec4 color;
          color.x = ( ( colorbits >>  0u ) & 0xFFu ) / 255.0f;
          color.y = ( ( colorbits >>  8u ) & 0xFFu ) / 255.0f;
          color.z = ( ( colorbits >> 16u ) & 0xFFu ) / 255.0f;
          color.w = ( ( colorbits >> 24u ) & 0xFFu ) / 255.0f;
          vec4 texel = texture( tex_sampler, tc );
          pixel = color * texel;
        }
      )STRING"
  #endif
      );


    // uniforms
    shader.loc_ndc_from_client = glGetUniformLocation( shader.core.program, "ndc_from_client" );  glVerify();
    shader.loc_tex_sampler = glGetUniformLocation( shader.core.program, "tex_sampler" );  glVerify();

    // attribs
    shader.attribloc_pos = glGetAttribLocation( shader.core.program, "vertex_pos" );  glVerify();
    shader.attribloc_tccolor = glGetAttribLocation( shader.core.program, "vertex_tccolor" );  glVerify();
  }

  void
  ShaderKill( shader_tex2_t& shader )
  {
    GlwUnloadShader( shader.core );
    shader.loc_ndc_from_client = -1;
    shader.loc_tex_sampler = -1;
    shader.attribloc_pos = -1;
    shader.attribloc_tccolor = -1;
  }

#endif // OPENGL_INSTEAD_OF_SOFTWARE


Inl f32
PackColorForShader( vec4<f32> color )
{
#if 1
  u32 res =
    ( ( Round_u32_from_f32( color.x * 63.0f ) & 63u ) <<  0u ) |
    ( ( Round_u32_from_f32( color.y * 63.0f ) & 63u ) <<  6u ) |
    ( ( Round_u32_from_f32( color.z * 63.0f ) & 63u ) << 12u ) |
    ( ( Round_u32_from_f32( color.w * 31.0f ) & 31u ) << 18u );
  return Cast( f32, res );
#else
  u32 res =
    ( ( Round_u32_from_f32( color.x * 255.0f ) & 0xFFu ) <<  0 ) |
    ( ( Round_u32_from_f32( color.y * 255.0f ) & 0xFFu ) <<  8 ) |
    ( ( Round_u32_from_f32( color.z * 255.0f ) & 0xFFu ) << 16 ) |
    ( ( Round_u32_from_f32( color.w * 255.0f ) & 0xFFu ) << 24 );
  return Reinterpret( f32, res );
#endif
}

Inl vec4<f32>
UnpackColorForShader( f32 colorf )
{
#if 1
  auto colorbits = Cast( u32, colorf );
  auto res = _vec4(
    ( ( colorbits >>  0u ) & 63u ) / 63.0f,
    ( ( colorbits >>  6u ) & 63u ) / 63.0f,
    ( ( colorbits >> 12u ) & 63u ) / 63.0f,
    ( ( colorbits >> 18u ) & 31u ) / 31.0f
    );
  return res;
#else
  auto colorbits = Reinterpret( u32, colorf );
  auto res = _vec4(
    ( ( colorbits >>  0u ) & 0xFFu ) / 255.0f,
    ( ( colorbits >>  8u ) & 0xFFu ) / 255.0f,
    ( ( colorbits >> 16u ) & 0xFFu ) / 255.0f,
    ( ( colorbits >> 24u ) & 0xFFu ) / 255.0f
    );
  return res;
#endif
}



Inl void
OutputQuad(
  stack_resizeable_cont_t<f32>& stream,
  vec2<f32> p0,
  vec2<f32> p1,
  f32 z,
  vec2<f32> tc0,
  vec2<f32> tc1,
  vec4<f32> color
  )
{
#if OPENGL_INSTEAD_OF_SOFTWARE
  auto pos = AddBack( stream, 6*3 + 6*3 );
  auto colorf = PackColorForShader( color );
  //auto test = UnpackColorForShader( colorf );
  *Cast( vec3<f32>*, pos ) = _vec3( p0.x, p0.y, z );         pos += 3;
  *Cast( vec3<f32>*, pos ) = _vec3( tc0.x, tc0.y, colorf );  pos += 3;
  *Cast( vec3<f32>*, pos ) = _vec3( p1.x, p0.y, z );         pos += 3;
  *Cast( vec3<f32>*, pos ) = _vec3( tc1.x, tc0.y, colorf );  pos += 3;
  *Cast( vec3<f32>*, pos ) = _vec3( p0.x, p1.y, z );         pos += 3;
  *Cast( vec3<f32>*, pos ) = _vec3( tc0.x, tc1.y, colorf );  pos += 3;
  *Cast( vec3<f32>*, pos ) = _vec3( p1.x, p1.y, z );         pos += 3;
  *Cast( vec3<f32>*, pos ) = _vec3( tc1.x, tc1.y, colorf );  pos += 3;
  *Cast( vec3<f32>*, pos ) = _vec3( p0.x, p1.y, z );         pos += 3;
  *Cast( vec3<f32>*, pos ) = _vec3( tc0.x, tc1.y, colorf );  pos += 3;
  *Cast( vec3<f32>*, pos ) = _vec3( p1.x, p0.y, z );         pos += 3;
  *Cast( vec3<f32>*, pos ) = _vec3( tc1.x, tc0.y, colorf );  pos += 3;
#else // !OPENGL_INSTEAD_OF_SOFTWARE
  auto pos = AddBack( stream, 4*2 + 2 );
  auto colorf = PackColorForShader( color );
  //auto test = UnpackColorForShader( colorf );
  *Cast( vec2<f32>*, pos ) = p0;         pos += 2;
  *Cast( vec2<f32>*, pos ) = p1;         pos += 2;
  *Cast( vec2<f32>*, pos ) = tc0;        pos += 2;
  *Cast( vec2<f32>*, pos ) = tc1;        pos += 2;
  *pos++ = z;
  *pos++ = colorf;
#endif // !OPENGL_INSTEAD_OF_SOFTWARE
}

Inl bool
ClipQuadTex(
  vec2<f32>& p0,
  vec2<f32>& p1,
  vec2<f32>& tc0,
  vec2<f32>& tc1,
  vec2<f32> clip_p0,
  vec2<f32> clip_p1
  )
{
  auto s_from_pix = ( tc1.x - tc0.x ) / ( p1.x - p0.x );
  auto t_from_pix = ( tc1.y - tc0.y ) / ( p1.y - p0.y );
  auto dx0 = ( p0.x - clip_p0.x );
  auto dx1 = ( p1.x - clip_p1.x );
  auto dy0 = ( p0.y - clip_p0.y );
  auto dy1 = ( p1.y - clip_p1.y );
  if( dx0 < 0 ) {
    tc0.x -= dx0 * s_from_pix;
    p0.x = clip_p0.x;
  }
  if( dx1 > 0 ) {
    tc1.x -= dx1 * s_from_pix;
    p1.x = clip_p1.x;
  }
  if( dy0 < 0 ) {
    tc0.y -= dy0 * t_from_pix;
    p0.y = clip_p0.y;
  }
  if( dy1 > 0 ) {
    tc1.y -= dy1 * t_from_pix;
    p1.y = clip_p1.y;
  }
  bool draw =
    ( p0.x < p1.x )  &
    ( p0.y < p1.y );
  return draw;
}

Inl bool
ClipQuad(
  vec2<f32>& p0,
  vec2<f32>& p1,
  vec2<f32> clip_p0,
  vec2<f32> clip_p1
  )
{
  auto dx0 = ( p0.x - clip_p0.x );
  auto dx1 = ( p1.x - clip_p1.x );
  auto dy0 = ( p0.y - clip_p0.y );
  auto dy1 = ( p1.y - clip_p1.y );
  if( dx0 < 0 ) {
    p0.x = clip_p0.x;
  }
  if( dx1 > 0 ) {
    p1.x = clip_p1.x;
  }
  if( dy0 < 0 ) {
    p0.y = clip_p0.y;
  }
  if( dy1 > 0 ) {
    p1.y = clip_p1.y;
  }

  bool draw =
    ( p0.x < p1.x )  &
    ( p0.y < p1.y );
  return draw;
}

Inl void
DrawLine(
  )
{
#if 0
  // clip first, since clipping an oriented quad is lots of code.
  // we'll accept a little clipping error, up to half of linewidth.

  vec2<f32> delta = line.p1 - line.p0;
  vec2<f32> half_edge = 0.5f * line.width * Normalize( Perp( delta ) );

  vec2<f32> p0, p1;

  auto t000 =  ( line.clip_pos.x                   - line.p0.x ) / delta.x;
  auto t010 =  ( line.clip_pos.x + line.clip_dim.x - line.p0.x ) / delta.x;
  auto t100 = -( line.clip_pos.x                   - line.p1.x ) / delta.x;
  auto t110 = -( line.clip_pos.x + line.clip_dim.x - line.p1.x ) / delta.x;

  auto t001 =  ( line.clip_pos.y                   - line.p0.y ) / delta.y;
  auto t011 =  ( line.clip_pos.y + line.clip_dim.y - line.p0.y ) / delta.y;
  auto t101 = -( line.clip_pos.y                   - line.p1.y ) / delta.y;
  auto t111 = -( line.clip_pos.y + line.clip_dim.y - line.p1.y ) / delta.y;

  auto p00 = line.p0 - half_edge;
  auto p10 = line.p1 - half_edge;
  auto p01 = line.p0 + half_edge;
  auto p11 = line.p1 + half_edge;

  bool draw =
    ( line.p0.x != line.p1.x )  |
    ( line.p0.y != line.p1.y );
  if( draw ) {
    // TODO: implement
    //ImplementCrash();
    //glBindTexture( GL_TEXTURE_2D, 0 );
    //glColor4fv( &line.color.x );
    //glLineWidth( line.width );
    //glBegin( GL_LINES );
    //glVertex3f( line.p0.x, line.p0.y, line.z );
    //glVertex3f( line.p1.x, line.p1.y, line.z );
    //glEnd();
    //glLineWidth( 1 );
  }
#endif
}

Inl void
RenderLine(
  stack_resizeable_cont_t<f32>& stream,
  vec4<f32> color,
  vec2<f32> p0,
  vec2<f32> p1,
  vec2<f32>& clip_p0,
  vec2<f32>& clip_p1,
  f32 z,
  f32 width
  )
{
  // TODO: implement
  ImplementCrash();
}

Inl void
RenderTri(
  stack_resizeable_cont_t<f32>& stream,
  vec4<f32>& color,
  vec2<f32> p0,
  vec2<f32> p1,
  vec2<f32> p2,
  vec2<f32>& clip_pos,
  vec2<f32>& clip_dim,
  f32 z
  )
{
  AssertWarn( 0.0f <= z  &&  z <= 1.0f );

#if OPENGL_INSTEAD_OF_SOFTWARE
  // TODO: clipping.
  bool draw = 1;
  if( draw ) {
    auto pos = AddBack( stream, 3*3 + 3*3 );
    auto colorf = PackColorForShader( color );
    //auto test = UnpackColorForShader( colorf );
    *Cast( vec3<f32>*, pos ) = _vec3( p0.x, p0.y, z );      pos += 3;
    *Cast( vec3<f32>*, pos ) = _vec3<f32>( 0, 0, colorf );  pos += 3;
    *Cast( vec3<f32>*, pos ) = _vec3( p1.x, p1.y, z );      pos += 3;
    *Cast( vec3<f32>*, pos ) = _vec3<f32>( 0, 0, colorf );  pos += 3;
    *Cast( vec3<f32>*, pos ) = _vec3( p2.x, p2.y, z );      pos += 3;
    *Cast( vec3<f32>*, pos ) = _vec3<f32>( 0, 0, colorf );  pos += 3;
  }
#else // !OPENGL_INSTEAD_OF_SOFTWARE
  // TODO_SOFTWARE_RENDER
#endif // !OPENGL_INSTEAD_OF_SOFTWARE
}

Inl void
RenderQuad(
  stack_resizeable_cont_t<f32>& stream,
  vec4<f32>& color,
  vec2<f32> p0,
  vec2<f32> p1,
  vec2<f32> clip_p0,
  vec2<f32> clip_p1,
  f32 z
  )
{
  AssertWarn( 0.0f <= z  &&  z <= 1.0f );

  bool draw = ClipQuad(
    p0,
    p1,
    clip_p0,
    clip_p1
    );
  if( draw ) {
    OutputQuad(
      stream,
      p0,
      p1,
      z,
      _vec2<f32>( 0, 0 ),
      _vec2<f32>( 0, 0 ),
      color
      );
  }
}
Inl void
RenderQuad(
  stack_resizeable_cont_t<f32>& stream,
  vec4<f32>& color,
  vec2<f32> p0,
  vec2<f32> p1,
  rectf32_t clip,
  f32 z
  )
{
  RenderQuad(
    stream,
    color,
    p0,
    p1,
    clip.p0,
    clip.p1,
    z
    );
}
Inl void
RenderQuad(
  stack_resizeable_cont_t<f32>& stream,
  vec4<f32>& color,
  rectf32_t bounds,
  rectf32_t clip,
  f32 z
  )
{
  RenderQuad(
    stream,
    color,
    bounds.p0,
    bounds.p1,
    clip.p0,
    clip.p1,
    z
    );
}

// for when you're sure p0,p1 are already properly clipped.
Inl void
RenderQuad(
  stack_resizeable_cont_t<f32>& stream,
  vec4<f32>& color,
  vec2<f32> p0,
  vec2<f32> p1,
  f32 z
  )
{
  OutputQuad(
    stream,
    p0,
    p1,
    z,
    _vec2<f32>( 0, 0 ),
    _vec2<f32>( 0, 0 ),
    color
    );
}
Inl void
RenderQuad(
  stack_resizeable_cont_t<f32>& stream,
  vec4<f32>& color,
  rectf32_t bounds,
  f32 z
  )
{
  RenderQuad(
    stream,
    color,
    bounds.p0,
    bounds.p1,
    z
    );
}

Inl void
RenderTQuad(
  stack_resizeable_cont_t<f32>& stream,
  vec4<f32>& color,
  vec2<f32> p0,
  vec2<f32> p1,
  vec2<f32> tc0,
  vec2<f32> tc1,
  vec2<f32> clip_p0,
  vec2<f32> clip_p1,
  f32 z
  )
{
  AssertWarn( 0.0f <= z  &&  z <= 1.0f );

  bool draw = ClipQuadTex(
    p0,
    p1,
    tc0,
    tc1,
    clip_p0,
    clip_p1
    );

  if( draw ) {
    OutputQuad(
      stream,
      p0,
      p1,
      z,
      tc0,
      tc1,
      color
      );
  }
}

Inl void
RenderCodept(
  stack_resizeable_cont_t<f32>& stream,
  font_t& font,
  vec2<f32> pos,
  f32 z,
  vec2<f32> clip_p0,
  vec2<f32> clip_p1,
  vec4<f32> color,
  fontglyph_t* glyph
  )
{
  AssertWarn( 0.0f <= z  &&  z <= 1.0f );

  // add ascent so we're positioning the top-right of the textbox.
  // add linegap so the text is aligned to the bottom of the textbox.
  auto p0 = pos + glyph->offset + _vec2( 0.0f, font.ascent + font.linegap );

  // snap to pixels, so it doesn't look too bad when moving.
  p0 = Ceil( p0 );

  auto p1 = p0 + glyph->dim;

  RenderTQuad(
    stream,
    color,
    p0,
    p1,
    glyph->tc0,
    glyph->tc1,
    clip_p0,
    clip_p1,
    z
    );
}


f32
LayoutString(
  font_t& font,
  idx_t spaces_per_tab,
  u8* text,
  idx_t text_len
  )
{
  ProfFunc();
  f32 width = 0;
  For( i, 0, text_len ) {
    u8 c = *text++;
    if ( c == '\t' ) {
      width += spaces_per_tab * FontGetAdvance( font, c );
    } else {
      // TODO: draw unsupported chars as the box char, like browsers do.
      // for now, draw non-ASCII as ~
      if( ( c < ' ' ) | ( c > '~' ) ) {
        c = '~';
      }
      width += FontGetAdvance( font, c );
    }
  }
  return width;
}

void
DrawString(
  stack_resizeable_cont_t<f32>& stream,
  font_t& font,
  vec2<f32> pos,
  f32 z,
  vec2<f32> clip_p0,
  vec2<f32> clip_p1,
  vec4<f32> color,
  idx_t spaces_per_tab,
  u8* text,
  idx_t text_len
  )
{
  ProfFunc();
  AssertWarn( 0.0f <= z  &&  z <= 1.0f );

  auto glyph_space = FontGetGlyph( font, ' ' );

  For( i, 0, text_len ) {
    u8 c = *text++;
    // draw spaces for tabs.
    if ( c == '\t' ) {
      For( j, 0, spaces_per_tab ) {
        RenderCodept(
          stream,
          font,
          pos,
          z,
          clip_p0,
          clip_p1,
          color,
          glyph_space
          );
        pos.x += glyph_space->advance;
      }
    } else {
      // TODO: draw unsupported chars as the box char, like browsers do.
      // for now, draw non-ASCII as ~
      if( ( c < ' ' ) | ( c > '~' ) ) {
        c = '~';
      }
      auto glyph = FontGetGlyph( font, c );
      RenderCodept(
        stream,
        font,
        pos,
        z,
        clip_p0,
        clip_p1,
        color,
        glyph
        );
      pos.x += glyph->advance;
    }
  }
}

void
DrawString(
  stack_resizeable_cont_t<f32>& stream,
  font_t& font,
  vec2<f32> pos,
  f32 z,
  rectf32_t clip,
  vec4<f32> color,
  idx_t spaces_per_tab,
  u8* text,
  idx_t text_len
  )
{
  DrawString(
    stream,
    font,
    pos,
    z,
    clip.p0,
    clip.p1,
    color,
    spaces_per_tab,
    text,
    text_len
    );
}

void
RenderText(
  stack_resizeable_cont_t<f32>& stream,
  font_t& font,
  fontlayout_t& layout,
  vec2<f32> pos,
  f32 z,
  vec2<f32> clip_p0,
  vec2<f32> clip_p1,
  vec4<f32> color,
  idx_t line,
  idx_t char_offset,
  idx_t char_len
  )
{
  ProfFunc();
  AssertWarn( 0.0f <= z  &&  z <= 1.0f );

  auto ox = pos.x;
  AssertCrash( line < layout.advances_per_ln.len );
  auto linespan = layout.advances_per_ln.mem + line;
  AssertCrash( char_offset + char_len <= linespan->len );
  if( !char_len ) {
    return;
  }
  AssertCrash( linespan->pos + char_offset + 0 < layout.raw_advances.len );
  auto first_advance = layout.raw_advances.mem + linespan->pos + char_offset + 0;
  auto dx_from_first = -first_advance->xl_within_entire_line;
  For( i, 0, char_len ) {
    AssertCrash( linespan->pos + char_offset + i < layout.raw_advances.len );
    auto advance = layout.raw_advances.mem + linespan->pos + char_offset + i;
    pos.x = ox + advance->xl_within_entire_line + dx_from_first;
    RenderCodept(
      stream,
      font,
      pos,
      z,
      clip_p0,
      clip_p1,
      color,
      advance->glyph
      );
  }
}

void
RenderText(
  stack_resizeable_cont_t<f32>& stream,
  font_t& font,
  fontlayout_t& layout,
  vec2<f32> pos,
  f32 z,
  rectf32_t clip,
  vec4<f32> color,
  idx_t line,
  idx_t char_offset,
  idx_t char_len
  )
{
  RenderText(
    stream,
    font,
    layout,
    pos,
    z,
    clip.p0,
    clip.p1,
    color,
    line,
    char_offset,
    char_len
    );
}

Templ Inl vec2<f32>
ZRange(
  vec2<f32> zrange,
  T layer
  )
{
  auto v = _vec2(
    ( Cast( f32, layer ) + 0 ) / Cast( f32, T::COUNT ),
    ( Cast( f32, layer ) + 1 ) / Cast( f32, T::COUNT )
    );
  return v * zrange;
}

Templ Inl f32
GetZ(
  vec2<f32> zrange,
  T layer
  )
{
  auto s = Cast( f32, layer ) / Cast( f32, T::COUNT );
  return lerp( zrange.x, zrange.y, s );
}


Inl f32
PerceivedBrightness( vec3<f32> rgb )
{
  //static const auto photoshop_weights = _vec3( 0.299f, 0.587f, 0.114f );
  //auto rgb2 = rgb * rgb;
  //return Sqrt( Dot( rgb2, photoshop_weights ) );

  static const auto relative_lumin_weights = _vec3( 0.2126f, 0.7152f, 0.0722f );
  return Dot( rgb, relative_lumin_weights );

  //return 0.25f * ( rgb.x + rgb.y + rgb.y + rgb.z );
}
Inl f32
PerceivedBrightness( vec4<f32> rgba )
{
  // not sure what to do with the alpha channel, so just ignore it.
  // maybe we should multiply the result by the alpha ?
  // open question.
  static const auto relative_lumin_weights = _vec4( 0.2126f, 0.7152f, 0.0722f, 0.0f );
  return Dot( rgba, relative_lumin_weights );
}
