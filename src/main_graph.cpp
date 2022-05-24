// build:window_x64_debug
// Copyright (c) John A. Carlos Jr., all rights reserved.

#define FINDLEAKS   0
#include "common.h"
#include "math_vec.h"
#include "math_matrix.h"
#include "ds_slice.h"
#include "ds_string.h"
#include "ds_plist.h"
#include "ds_array.h"
#include "ds_embeddedarray.h"
#include "ds_fixedarray.h"
#include "ds_pagearray.h"
#include "ds_list.h"
#include "ds_bytearray.h"
#include "ds_hashset.h"
#include "ds_queue.h"
#include "ds_minheap_extractable.h"
#include "ds_minheap_decreaseable.h"
#include "cstr.h"
#include "filesys.h"
#include "timedate.h"
#include "threading.h"
#define LOGGER_ENABLED   1
#include "logger.h"
#define PROF_ENABLED   0
#define PROF_ENABLED_AT_LAUNCH   0
#include "profile.h"
#include "ds_graph.h"
#include "rand.h"
#include "main.h"

#define OPENGL_INSTEAD_OF_SOFTWARE       0
#define GLW_RAWINPUT_KEYBOARD            0
#define GLW_RAWINPUT_KEYBOARD_NOHOTKEY   1
#include "glw.h"

#define USE_FILEMAPPED_OPEN 0 // TODO: unfinished
#define USE_SIMPLE_SCROLLING 0 // TODO: questionable decision to try to simplify scrolling datastructs, getting rid of smooth scrolling.

#include "ui_propdb.h"
#include "ui_font.h"
#include "ui_render.h"


struct
app_t
{
  glwclient_t client;
  bool fullscreen;
  array_t<f32> stream;

  array_t<font_t> fonts;

  idx_t m = 1;
  f32 factor = 1.0f;
};

static app_t g_app = {};


void
AppInit( app_t* app )
{
  GlwInit();

  Alloc( app->stream, 65536 );
  Alloc( app->fonts, 16 );
  app->fullscreen = 0;

  GlwInitWindow(
    app->client,
    Str( "graph" ), 5,
    0, _vec2<u32>( 800, 800 )
    );
}

void
AppKill( app_t* app )
{
  ForLen( i, app->fonts ) {
    auto& font = app->fonts.mem[i];
    FontKill( font );
  }
  Free( app->fonts );

  Free( app->stream );

  GlwKillWindow( app->client );
  GlwKill();
}



Enumc( fontid_t )
{
  normal,
};


void
LoadFont(
  app_t* app,
  enum_t fontid,
  u8* filename_ttf,
  idx_t filename_ttf_len,
  f32 char_h
  )
{
  AssertWarn( fontid < 10000 ); // sanity check.
  Reserve( app->fonts, fontid + 1 );
  app->fonts.len = MAX( app->fonts.len, fontid + 1 );

  auto& font = app->fonts.mem[fontid];
  FontLoad( font, filename_ttf, filename_ttf_len, char_h );
  FontLoadAscii( font );
}

void
UnloadFont( app_t* app, enum_t fontid )
{
  auto font = app->fonts.mem + fontid;
  FontKill( *font );
}

font_t&
GetFont( app_t* app, enum_t fontid )
{
  return app->fonts.mem[fontid];
}


Enumc( applayer_t )
{
  edit,
  bkgd,
  txt,
  COUNT
};


Inl f32
CalcA(
  f32* A,
  u32 N,
  f32 t
  )
{
  kahan32_t sum = {};
  AssertCrash( N );
  sum.sum = A[0];
  Fori( u32, i, 1, N ) {
    f32 term = A[i] * Pow32( t, Cast( f32, i ) );
    Add( sum, term );
  }
  return sum.sum;
}

Inl f32
CalcTotalErrRenormalization(
  f32* t_tests,
  u32 t_tests_len,
  f32* A,
  u32 N
  )
{
  kahan32_t total_err = {};
  For( i, 0, t_tests_len ) {
    auto t = t_tests[i];
    // we're storing alpha as A[ N - 1 ]
    auto alpha = A[ N - 1 ];
    auto FofXoverAlpha = CalcA( A, N - 1, t / alpha );
    auto F = CalcA( A, N - 1, t );
    auto FofFofXoverAlpha = CalcA( A, N - 1, FofXoverAlpha );
    auto err = FofFofXoverAlpha - F;
    Add( total_err, ABS( err ) );
  }
  return total_err.sum / t_tests_len;
}

Inl vec2<f32>
SpringEqns(
  f32 t,
  f32 a,
  f32 n,
  f32 A,
  f32 B
  )
{
  // x(t) = a exp( -n ) t^n exp( -A t ) exp( +-sqrt( n + t^2 ( A^2 - B ) ) )
  // x(t) = a t^n exp( -n - A t +- sqrt( n + t^2 ( A^2 - B ) ) )
  // x(t) = a exp( b t ) t^n, with b t = -n - A t +- sqrt( n + t^2 ( A^2 - B ) )
  // x'(t) = a exp( b t ) [ n t^( n - 1 ) + b t^n ]
  // x'(t) = a exp( b t ) t^n [ n / t + b ]
  // x'(t) = x(t) [ n / t + b ]
  // x'(t) = x(t) [ n + b t ] / t
  // x''(t) = a exp( b t ) [ n ( n - 1 ) t^( n - 2 ) + 2 b n t^( n - 1 ) + b^2 t^n ]
  // x''(t) = a exp( b t ) t^n [ n ( n - 1 ) / t^2 + 2 b n / t + b^2 ]
  // x''(t) = x(t) [ n ( n - 1 ) / t^2 + 2 b n / t + b^2 ]
  // x''(t) = x(t) [ n ( n - 1 ) / t^2 + 2 b t n / t^2 + ( b t )^2 / t^2 ]
  // x''(t) = x(t) [ n ( n - 1 ) + 2 b t n + ( b t )^2 ] / t^2
  auto disc = n + t * t * ( A * A - B );
  AssertCrash( disc >= 0 );
  auto sqroot = Sqrt32( disc );
  auto leadfac = a * Pow32( t, n );
  auto leadexp = -n - A * t;
  auto bt0 = leadexp + sqroot;
  auto bt1 = leadexp - sqroot;
  auto x0 = leadfac * Exp32( bt0 );
  auto x1 = leadfac * Exp32( bt1 );
  auto xp0 = x0 * ( n + bt0 ) / t;
  auto xp1 = x1 * ( n + bt1 ) / t;
  auto xpp0 = x0 * ( n * ( n - 1 ) + 2 * n * bt0 + bt0 * bt0 ) / ( t * t );
  auto xpp1 = x1 * ( n * ( n - 1 ) + 2 * n * bt1 + bt1 * bt1 ) / ( t * t );

  auto soln0 = xpp0 + 2 * A * xp0 + B * x0;
  auto soln1 = xpp1 + 2 * A * xp1 + B * x1;

  return _vec2( soln0, soln1 );
}

__OnRender( AppOnRender )
{
  ProfFunc();

  auto app = Cast( app_t*, misc );
  auto& font = GetFont( app, Cast( enum_t, fontid_t::normal ) );
  auto line_h = FontLineH( font );
  auto zrange = _vec2<f32>( 0, 1 );

  auto spaces_per_tab = GetPropFromDb( u8, u8_spaces_per_tab );
  auto rgba_notify_text = GetPropFromDb( vec4<f32>, rgba_notify_text );

  auto timestep = MIN( timestep_realtime, timestep_fixed );

  { // display timestep_realtime, as a way of tracking how long rendering takes.
    embeddedarray_t<u8, 64> tmp;
    CsFrom_f64( tmp.mem, Capacity( tmp ), &tmp.len, 1000 * timestep_realtime );
    auto tmp_w = LayoutString( font, spaces_per_tab, ML( tmp ) );
    DrawString(
      app->stream,
      font,
      AlignRight( bounds, tmp_w ),
      GetZ( zrange, applayer_t::txt ),
      bounds,
      rgba_notify_text,
      spaces_per_tab,
      ML( tmp )
      );

    CsFromIntegerU( tmp.mem, Capacity( tmp ), &tmp.len, app->m, 1 );
    tmp_w = LayoutString( font, spaces_per_tab, ML( tmp ) );
    DrawString(
      app->stream,
      font,
      bounds.p0,
      GetZ( zrange, applayer_t::txt ),
      bounds,
      rgba_notify_text,
      spaces_per_tab,
      ML( tmp )
      );

    auto space_w = LayoutString( font, spaces_per_tab, Str( " " ), 1 );
    CsFrom_f32( tmp.mem, Capacity( tmp ), &tmp.len, app->factor );
    DrawString(
      app->stream,
      font,
      bounds.p0 + _vec2<f32>( tmp_w + space_w, 0 ),
      GetZ( zrange, applayer_t::txt ),
      bounds,
      rgba_notify_text,
      spaces_per_tab,
      ML( tmp )
      );

    bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );
  }

  target_valid = 0;

  {
    auto dim = bounds.p1 - bounds.p0;
    auto dim_x = Truncate_idx_from_f32( dim.x );
    auto dim_y = Truncate_idx_from_f32( dim.y );

    rng_xorshift32_t rng_xorsh;
    Init( rng_xorsh, TimeTSC() );

    auto binsize = 10;
    auto nbins = dim_x / binsize;
    auto bins = MemHeapAlloc( f32, nbins );
    auto nsamples = 1000 / app->m;
    auto z = MemHeapAlloc( f32, nsamples );
    auto m = app->m;
    auto mf = Cast( f32, m );

    For( i, 0, nsamples ) {
      z[i] = 0;
    }
    For( i, 0, nsamples ) {
      For( j, 0, m ) {
        z[i] += Zeta32( rng_xorsh ) / m;
      }
    }
    For( i, 0, nbins ) {
      bins[i] = 0;
    }
    For( i, 0, nsamples ) {
      auto bini = Truncate_idx_from_f32( z[i] * nbins );
      AssertCrash( bini <= nbins );
      bini = MIN( bini, MAX( nbins, 1 ) - 1 );
      AssertCrash( bini < nbins );
      bins[bini] += dim.x / ( binsize * nsamples );
    }
    app->factor = 0;
    For( i, 0, nbins ) {
      app->factor = MAX( app->factor, bins[i] );
    }

    auto y = MemHeapAlloc( f32, dim_x );
    For( i, 0, dim_x ) {
      auto t = i / dim.x;
//      y[i] = t;
//      y[i] = Zeta32( rng_xorsh );
//      y[i] = 0.5f * bins[i / binsize] * dim.y / app->factor;
//      y[i] = Exp32( t );
//      y[i] = fastexp32( t );
//      y[i] = fastln32( fastexp32( t ) );
//      y[i] = fastpow32( t, 2 );

//      f32 A = 1.0f;
//      f32 B = -8.0f;
//      f32 a = 1.0f;
//      t = 1.001f + t * 100.0f;
//      auto lnt = Ln32( t );
//      auto disc = ( Sq( A ) - B ) * Sq( t ) + t / lnt + 1 / Sq( lnt );
//      if( disc < 0 ) {
//        y[i] = a * lnt * Exp32( -A * t - 1 / lnt ) * Cos32( +Sqrt32( -disc ) );
//        y[i] = a * lnt * Exp32( -A * t - 1 / lnt ) * Cos32( -Sqrt32( -disc ) );
//      } else {
//        y[i] = a * lnt * Exp32( -A * t - 1 / lnt + Sqrt32( disc ) );
//        y[i] = a * lnt * Exp32( -A * t - 1 / lnt - Sqrt32( disc ) );
//      }
//      y[i] = a * lnt * Cos32( -A * t ) * Exp32( - 1 / lnt ) * Cos32( +Sqrt32( -disc ) );
//      y[i] = a * lnt * Cos32( -A * t ) * Exp32( - 1 / lnt ) * Cos32( +Sqrt32( +disc ) );

#if 0
      f32 t_tests[16] = {  };
      f32 t_tests_bounds = 0.2f;
      auto bounds_loop_len = _countof( t_tests ) / 2;
      For( j, 0, bounds_loop_len ) {
        t_tests[ 2 * j + 0 ] = +Cast( f32, j + 1 ) * t_tests_bounds / bounds_loop_len;
        t_tests[ 2 * j + 1 ] = -Cast( f32, j + 1 ) * t_tests_bounds / bounds_loop_len;
      }
      constant u32 N = 4;
      f32 A[N] = { 1.0f, 0.0f, -1.366f, -2.732f };
//      A[3] = 5 * ( 2 * t - 1 );
      A[3] = -5 * ( t + 0.00001f );
//      A[2] = -10 * ( t + 0.00001f );
//      A[2] = 400 * ( 2 * t - 1 );
      y[i] = CalcTotalErrRenormalization( t_tests, _countof( t_tests ), A, N );
      y[i] = Ln32( y[i] + 0.00001f );
#endif

#if 0
      constant f32 mass = 1.0f;
      constant f32 spring_k = 100.0f;
      static f32 friction_k = 2.2f * Sqrt32( mass * spring_k ); // 2 * Sqrt( m * k ) is critical damping, but we want a little overdamping.

      f32 a = 1.0f;
      f32 n = 20.0f;
      f32 B = -spring_k;
      f32 A = 0.5f * friction_k;

      auto disc = n + t * t * ( A * A - B );
      AssertCrash( disc >= 0 );
      auto sqroot = Sqrt32( disc );
      auto leadfac = a * Pow32( t, n );
      auto leadexp = -n - A * t;
      auto bt0 = leadexp + sqroot;
      auto bt1 = leadexp - sqroot;
      auto x0 = leadfac * Exp32( bt0 );
      auto x1 = leadfac * Exp32( bt1 );
      auto xp0 = x0 * ( n + bt0 ) / t;
      auto xp1 = x1 * ( n + bt1 ) / t;
      auto xpp0 = x0 * ( n * ( n - 1 ) + 2 * n * bt0 + bt0 * bt0 ) / ( t * t );
      auto xpp1 = x1 * ( n * ( n - 1 ) + 2 * n * bt1 + bt1 * bt1 ) / ( t * t );

      auto soln0 = xpp0 + 2 * A * xp0 + B * x0;
      auto soln1 = xpp1 + 2 * A * xp1 + B * x1;

      y[i] = x0;
#endif

#if 1
      n_t N = 3;
      e_t E = 2;
      e_t outoffsets[3] = { 0, 0, 0 };
      e_t outdegrees[3] = { 2, 0, 0 };
      n_t outnodes[2] = { 1, 2 };
      vec2<f32> positions[3] = { _vec2<f32>( 100, 0 ), _vec2<f32>( 0, 0 ), _vec2<f32>( 200, 0 ) };
      vec2<f32> velocities[3] = {};
      vec2<f32> forces[3];
      idx_t num_iterations = 100;
      LayoutSimpleSpring( N, E, outoffsets, outdegrees, outnodes, positions, velocities, forces, bounds, (f32)timestep, num_iterations);
      y[i] = 0.0f;
      ForEach( pos, positions ) {
        auto del = ABS( (s64)pos.x - (s64)i );
        constant f32 spread = 10.0f;
        if( del <= spread ) {
          y[i] += spread - del;
        }
      }
#endif

#if 0
      idx_t tmp = 0;
      auto vol = Cast( volatile idx_t*, &tmp );
//      u64 t0 = TimeTSC();
      u32 aux = 0;
      u64 t0 = __rdtscp( &aux );
//      u64 t0;
//      GetSystemTimePreciseAsFileTime( Cast( FILETIME*, &t0 ) );
//      u64 t0 = TimeClock();
      s32 cpuidtmp[4];
      __cpuid( cpuidtmp, 0 );
      _ReadWriteBarrier();
//      For( j, 0, i ) {
//        idx_t bounds = Cast( idx_t, Exp32( Cast( f32, j ) ) );
//        For( k, 0, j ) {
//          *vol = TimeTSC();
//        }
//      }
      For( j, 0, 1 ) {
        *vol = TimeTSC();
      }
      _ReadWriteBarrier();
      __cpuid( cpuidtmp, 0 );
//      u64 t1 = TimeTSC();
      u64 t1 = __rdtscp( &aux );
//      u64 t1 = TimeClock();
//      u64 t1;
//      GetSystemTimePreciseAsFileTime( Cast( FILETIME*, &t1 ) );
      y[i] = t1 - t0;
#endif

    }

    if( 0 ) {//dim_x ) {
      For( i, 1, dim_x - 1 ) {
        auto y_l = y[i-1];
        auto y_m = y[i+0];
        auto y_r = y[i+1];
//        y[i] = ( y_l + 1.0f * y_m + y_r ) / 3.0f;
        y[i-1] = 0.5f * ( y_m + y_r );
      }
    }

    f32 y_min = 0;
    f32 y_max = 0;
    For( i, 0, dim_x ) {
      y_min = MIN( y_min, y[i] );
      y_max = MAX( y_max, y[i] );
    }
//    y_max = 350 * 2;

    For( i, 0, dim_x ) {
      auto xi = Cast( f32, i );
      auto yi = ( y[i] - y_min ) / ( y_max - y_min ) * dim.y;

#if 0
      f32 yn;
      if( i + 1 == dim_x ) {
        yn = yi;
      } else {
        yn = ( y[i+1] - y_min ) / ( y_max - y_min ) * dim.y;
      }
      auto ydel = yn - yi;
      auto y0del = ydel < 0  ?  ydel  :  0;
      auto y1del = ydel < 0  ?  0  :  ydel;
      RenderQuad(
        app->stream,
        rgba_notify_text,
        origin + _vec2<f32>( xi, dim.y - yi + y0del ),
        origin + _vec2<f32>( xi + 1, dim.y - yi + 1 + y1del ),
        origin,
        origin + dim,
        GetZ( zrange, applayer_t::txt )
        );
#else
      RenderQuad(
        app->stream,
        rgba_notify_text,
        bounds.p0 + _vec2<f32>( xi, dim.y - ( yi + 1 ) ),
        bounds.p0 + _vec2<f32>( xi + 1, dim.y - yi ),
        bounds,
        GetZ( zrange, applayer_t::txt )
        );
#endif
    }
    MemHeapFree( y );
    MemHeapFree( z );
    MemHeapFree( bins );
  }

  if( app->stream.len ) {

    auto pos = app->stream.mem;
    auto end = app->stream.mem + app->stream.len;
    AssertCrash( app->stream.len % 10 == 0 );
    while( pos < end ) {
      Prof( tmp_UnpackStream );
      vec2<u32> p0;
      vec2<u32> p1;
      vec2<u32> tc0;
      vec2<u32> tc1;
      p0.x  = Round_u32_from_f32( *pos++ );
      p0.y  = Round_u32_from_f32( *pos++ );
      p1.x  = Round_u32_from_f32( *pos++ );
      p1.y  = Round_u32_from_f32( *pos++ );
      tc0.x = Round_u32_from_f32( *pos++ );
      tc0.y = Round_u32_from_f32( *pos++ );
      tc1.x = Round_u32_from_f32( *pos++ );
      tc1.y = Round_u32_from_f32( *pos++ );
      *pos++; // this is z; but we don't do z reordering right now.
      auto color = UnpackColorForShader( *pos++ );
      auto color_a = Cast( u8, Round_u32_from_f32( color.w * 255.0f ) & 0xFFu );
      auto color_r = Cast( u8, Round_u32_from_f32( color.x * 255.0f ) & 0xFFu );
      auto color_g = Cast( u8, Round_u32_from_f32( color.y * 255.0f ) & 0xFFu );
      auto color_b = Cast( u8, Round_u32_from_f32( color.z * 255.0f ) & 0xFFu );
      auto coloru = ( color_a << 24 ) | ( color_r << 16 ) | ( color_g <<  8 ) | color_b;
      auto color_argb = _mm_set_ps( color.w, color.x, color.y, color.z );
      auto color255_argb = _mm_mul_ps( color_argb, _mm_set1_ps( 255.0f ) );
      bool just_copy =
        color.x == 1.0f  &&
        color.y == 1.0f  &&
        color.z == 1.0f  &&
        color.w == 1.0f;
      bool no_alpha = color.w == 1.0f;

      AssertCrash( p0.x <= p1.x );
      AssertCrash( p0.y <= p1.y );
      auto dstdim = p1 - p0;
      AssertCrash( tc0.x <= tc1.x );
      AssertCrash( tc0.y <= tc1.y );
      auto srcdim = tc1 - tc0;
      AssertCrash( p0.x + srcdim.x <= app->client.dim.x );
      AssertCrash( p0.y + srcdim.y <= app->client.dim.y );
      AssertCrash( p0.x + dstdim.x <= app->client.dim.x ); // should have clipped by now.
      AssertCrash( p0.y + dstdim.y <= app->client.dim.y );
      AssertCrash( tc0.x + srcdim.x <= font.tex_dim.x );
      AssertCrash( tc0.y + srcdim.y <= font.tex_dim.y );
      ProfClose( tmp_UnpackStream );

      bool subpixel_quad =
        ( p0.x == p1.x )  ||
        ( p0.y == p1.y );

      // below our resolution, so ignore it.
      // we're not going to do linear-filtering rendering here.
      // something to consider for the future.
      if( subpixel_quad ) {
        continue;
      }

      // for now, assume quads with empty tex coords are just plain rects, with the given color.
      bool nontex_quad =
        ( !srcdim.x  &&  !srcdim.y );

      if( nontex_quad ) {
        u32 copy = 0;
        if( just_copy ) {
          copy = MAX_u32;
        } elif( no_alpha ) {
          copy = ( 0xFF << 24 ) | coloru;
        }

        if( just_copy  ||  no_alpha ) {
          Prof( tmp_DrawOpaqueQuads );

          // PERF: this is ~0.7 cycles / pixel
          // suprisingly, rep stosq is about the same as rep stosd!
          // presumably the hardware folks made that happen, so old 32bit code also got faster.
          auto copy2 = Pack( copy, copy );
          Fori( u32, j, 0, dstdim.y ) {
            auto dst = app->client.fullscreen_bitmap_argb + ( p0.y + j ) * app->client.dim.x + ( p0.x + 0 );
            if( dstdim.x & 1 ) {
              *dst++ = copy;
            }
            auto dim2 = dstdim.x / 2;
            auto dst2 = Cast( u64*, dst );
            Fori( u32, k, 0, dim2 ) {
              *dst2++ = copy2;
            }
          }
        } else {
          Prof( tmp_DrawAlphaQuads );
          // more complicated / slow alpha case.
          Fori( u32, j, 0, dstdim.y ) {
            auto dst = app->client.fullscreen_bitmap_argb + ( p0.y + j ) * app->client.dim.x + ( p0.x + 0 );
            Fori( u32, k, 0, dstdim.x ) {
              // PERF: this is ~6 cycles / pixel

              // unpack dst
              auto dstf = _mm_set_ss( *Cast( f32*, dst ) );
              auto dsti = _mm_cvtepu8_epi32( _mm_castps_si128( dstf ) );
              auto dst_argb = _mm_cvtepi32_ps( dsti );

              // src = color
              auto src_argb = color255_argb;

              // out = dst + src_a * ( src - dst )
              auto src_a = _mm_set1_ps( color.w );
              auto diff_argb = _mm_sub_ps( src_argb, dst_argb );
              auto out_argb = _mm_fmadd_ps( src_a, diff_argb, dst_argb );

              // put out in [0, 255]
              auto outi = _mm_cvtps_epi32( out_argb );
              auto out = _mm_packs_epi32( outi, _mm_set1_epi32( 0 ) );
              out = _mm_packus_epi16( out, _mm_set1_epi32( 0 ) );

              _mm_storeu_si32( dst, out ); // equivalent to: *dst = out.m128i_u32[0]
              ++dst;
            }
          }
        }
      } else {
        Prof( tmp_DrawTexQuads );
        // for now, assume quads with nonempty tex coords are exact-size copies onto dst.
        // that's true of all our text glyphs, which makes for faster code here.
        //
        // TODO: handle scaling up, or make a separate path for scaled-up tex, non-tex, etc. quads
        // we'll need separate paths for tris, lines anyways, so maybe do that.
        Fori( u32, j, 0, srcdim.y ) {
          auto dst = app->client.fullscreen_bitmap_argb + ( p0.y + j ) * app->client.dim.x + ( p0.x + 0 );
          auto src = font.tex_mem + ( tc0.y + j ) * font.tex_dim.x + ( tc0.x + 0 );
          Fori( u32, k, 0, srcdim.x ) {
            // PERF: this is ~7 cycles / pixel

            // given color in [0, 1]
            // given src, dst in [0, 255]
            // src *= color
            // fac = src_a / 255
            // out = dst + fac * ( src - dst )

            // unpack src, dst
            auto dstf = _mm_set_ss( *Cast( f32*, dst ) );
            auto dsti = _mm_cvtepu8_epi32( _mm_castps_si128( dstf ) );
            auto dst_argb = _mm_cvtepi32_ps( dsti );

            auto srcf = _mm_set_ss( *Cast( f32*, src ) );
            auto srci = _mm_cvtepu8_epi32( _mm_castps_si128( srcf ) );
            auto src_argb = _mm_cvtepi32_ps( srci );

            // src *= color
            src_argb = _mm_mul_ps( src_argb, color_argb );

            // out = dst + src_a * ( src - dst )
            auto fac = _mm_shuffle_ps( src_argb, src_argb, 255 ); // equivalent to set1( m128_f32[3] )
            fac = _mm_mul_ps( fac, _mm_set1_ps( 1.0f / 255.0f ) );
            auto diff_argb = _mm_sub_ps( src_argb, dst_argb );
            auto out_argb = _mm_fmadd_ps( fac, diff_argb, dst_argb );

            // get out in [0, 255]
            auto outi = _mm_cvtps_epi32( out_argb );
            auto out = _mm_packs_epi32( outi, _mm_set1_epi32( 0 ) );
            out = _mm_packus_epi16( out, _mm_set1_epi32( 0 ) );

            _mm_storeu_si32( dst, out ); // equivalent to: *dst = out.m128i_u32[0]
            ++dst;
            ++src;
          }
        }
      }
    }

    app->stream.len = 0;
  }
}


#define __AppCmd( name )   void ( name )( app_t* app )
typedef __AppCmd( *pfn_appcmd_t );


struct
app_cmdmap_t
{
  glwkeybind_t keybind;
  pfn_appcmd_t fn;
};

Inl app_cmdmap_t
_appcmdmap(
  glwkeybind_t keybind,
  pfn_appcmd_t fn
  )
{
  app_cmdmap_t r;
  r.keybind = keybind;
  r.fn = fn;
  return r;
}


__OnKeyEvent( AppOnKeyEvent )
{
  ProfFunc();

  auto app = Cast( app_t*, misc );

  // Global key event handling:
  bool ran_cmd = 0;
  switch( type ) {
    case glwkeyevent_t::dn:
    case glwkeyevent_t::repeat: {

//      app_cmdmap_t table[] = {
//      };
//      ForEach( entry, table ) {
//        if( GlwKeybind( key, entry.keybind ) ) {
//          entry.fn( app );
//          target_valid = 0;
//          ran_cmd = 1;
//        }
//      }

      if( GlwKeybind( key, GetPropFromDb( glwkeybind_t, keybind_app_quit ) ) ) {
        GlwEarlyKill( app->client );
        target_valid = 0;
        ran_cmd = 1;
      }
    } break;

    case glwkeyevent_t::up: {
      switch( key ) {
#if 0
        case glwkey_t::esc: {
          GlwEarlyKill( app->client );
          ran_cmd = 1;
        } break;
#endif

        case glwkey_t::fn_11: {
          app->fullscreen = !app->fullscreen;
          fullscreen = app->fullscreen;
          ran_cmd = 1;
        } break;

#if PROF_ENABLED
        case glwkey_t::fn_9: {
          LogUI( "Started profiling." );
          ProfEnable();
          ran_cmd = 1;
        } break;

        case glwkey_t::fn_10: {
          ProfDisable();
          LogUI( "Stopped profiling." );
          ran_cmd = 1;
        } break;
#endif

      }
    } break;

    default: UnreachableCrash();
  }
  if( !ran_cmd ) {
  }
}



__OnMouseEvent( AppOnMouseEvent )
{
  ProfFunc();

  auto app = Cast( app_t*, misc );

  auto& font = GetFont( app, Cast( enum_t, fontid_t::normal ) );

  auto scroll_nlines = GetPropFromDb( u8, u8_scroll_nlines );
  auto scroll_sign = GetPropFromDb( s8, s8_scroll_sign );

  cursortype = glwcursortype_t::arrow;


  auto keymods = GlwKeyModifiersDown();
  bool mod_isdn = AnyDown( keymods );

  switch( type ) {

    case glwmouseevent_t::wheelmove: {
      if( mod_isdn ) {
        dwheel *= Cast( s32, app->factor );
      }
      if( dwheel ) {
        if( dwheel < 0 ) {
          auto delta = Cast( idx_t, -dwheel );
          app->m = CLAMP( MAX( app->m, delta ) - delta, 1, 100000 );
        } else {
          app->m = CLAMP( app->m + Cast( idx_t, dwheel ), 1, 100000 );
        }
        target_valid = 0;
      }
    } break;

    case glwmouseevent_t::dn: {

      switch( btn ) {
        case glwmousebtn_t::l: {
          target_valid = 0;
        } break;

        case glwmousebtn_t::r:
        case glwmousebtn_t::m:
        case glwmousebtn_t::b4:
        case glwmousebtn_t::b5: {
        } break;

        default: UnreachableCrash();
      }
    } break;

    case glwmouseevent_t::up: {
    } break;

    case glwmouseevent_t::move: {
      //printf( "move ( %d, %d )\n", m.x, m.y );
    } break;

    default: UnreachableCrash();
  }
}


// TODO: move to config file once it has comment support.
#if 0
  static const f32 fontsize_px = 16.0f; // 0.1666666666 i
  static const f32 fontsize_px = 44.0f / 3.0f; // 0.1527777777777 i

    Str( "c:/windows/fonts/droidsansmono.ttf" ),
    Str( "c:/windows/fonts/lucon.ttf" ),
    Str( "c:/windows/fonts/liberationmono-regular.ttf" ),
    Str( "c:/windows/fonts/consola.ttf" ),
    Str( "c:/windows/fonts/ubuntumono-r.ttf" ),
    Str( "c:/windows/fonts/arial.ttf" ),
    Str( "c:/windows/fonts/times.ttf" ),
#endif


__OnWindowEvent( AppOnWindowEvent )
{
  ProfFunc();

  auto app = Cast( app_t*, misc );

  if( type & glwwindowevent_resize ) {
  }
  if( type & glwwindowevent_focuschange ) {
  }
  if( type & glwwindowevent_dpichange ) {
    auto fontsize_normal = GetPropFromDb( f32, f32_fontsize_normal );
    auto filename_font_normal = GetPropFromDb( slice_t, string_filename_font_normal );

    UnloadFont( app, Cast( idx_t, fontid_t::normal ) );
    LoadFont(
      app,
      Cast( idx_t, fontid_t::normal ),
      ML( filename_font_normal ),
      fontsize_normal * Cast( f32, dpi )
      );
  }
}



int
Main( array_t<slice_t>& args )
{
  PinThreadToOneCore();

  auto app = &g_app;
  AppInit( app );

  if( args.len ) {
    auto arg = args.mem + 0;
  }

  auto fontsize_normal = GetPropFromDb( f32, f32_fontsize_normal );
  auto filename_font_normal = GetPropFromDb( slice_t, string_filename_font_normal );

  LoadFont(
    app,
    Cast( idx_t, fontid_t::normal ),
    ML( filename_font_normal ),
    fontsize_normal * Cast( f32, app->client.dpi )
    );

  glwcallback_t callbacks[] = {
    { glwcallbacktype_t::keyevent           , app, AppOnKeyEvent            },
    { glwcallbacktype_t::mouseevent         , app, AppOnMouseEvent          },
    { glwcallbacktype_t::windowevent        , app, AppOnWindowEvent         },
    { glwcallbacktype_t::render             , app, AppOnRender              },
  };
  ForEach( callback, callbacks ) {
    GlwRegisterCallback( app->client, callback );
  }

  GlwMainLoop( app->client );

  // Do this before destroying any datastructures, so other threads stop trying to access things.
  SignalQuitAndWaitForTaskThreads();

  AppKill( app );

  return 0;
}




Inl void
IgnoreSurroundingSpaces( u8*& a, idx_t& a_len )
{
  while( a_len  &&  IsWhitespace( a[0] ) ) {
    a += 1;
    a_len -= 1;
  }
  // TODO: prog_cmd_line actually has two 0-terms!
  while( a_len  &&  ( !a[a_len - 1]  ||  IsWhitespace( a[a_len - 1] ) ) ) {
    a_len -= 1;
  }
}

int WINAPI
WinMain( HINSTANCE prog_inst, HINSTANCE prog_inst_prev, LPSTR prog_cmd_line, int prog_cmd_show )
{
  MainInit();

  u8* cmdline = Str( prog_cmd_line );
  idx_t cmdline_len = CsLen( Str( prog_cmd_line ) );

  array_t<slice_t> args;
  Alloc( args, 64 );

  while( cmdline_len ) {
    IgnoreSurroundingSpaces( cmdline, cmdline_len );
    if( !cmdline_len ) {
      break;
    }
    auto arg = cmdline;
    idx_t arg_len = 0;
    auto quoted = cmdline[0] == '"';
    if( quoted ) {
      arg = cmdline + 1;
      auto end = CsScanR( arg, cmdline_len - 1, '"' );
      arg_len = !end  ?  0  :  end - arg;
      IgnoreSurroundingSpaces( arg, arg_len );
    } else {
      auto end = CsScanR( arg, cmdline_len - 1, ' ' );
      arg_len = !end  ?  cmdline_len  :  end - arg;
      IgnoreSurroundingSpaces( arg, arg_len );
    }
    if( arg_len ) {
      auto add = AddBack( args );
      add->mem = arg;
      add->len = arg_len;
    }
    cmdline = arg + arg_len;
    cmdline_len -= arg_len;
    if( quoted ) {
      cmdline += 1;
      cmdline_len -= 1;
    }
  }
  auto r = Main( args );
  Free( args );

  Log( "Main returned: %d", r );

  MainKill();
  return r;
}
