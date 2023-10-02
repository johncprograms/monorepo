// build:window_x64_debug
// build:window_x64_optimized
// build:window_x64_releaseversion
// Copyright (c) John A. Carlos Jr., all rights reserved.

#include "os_mac.h"
#include "os_windows.h"

void
LogUI( const void* cstr ... );

#define FINDLEAKS   0
#define WEAKINLINING   1
#define LOGASYNCTASKS   0   // logger is global, and i was seeing lock contention, so disable this.
#include "core_cruntime.h"
#include "core_types.h"
#include "core_language_macros.h"
#include "asserts.h"
#include "memory_operations.h"
#include "math_integer.h"
#include "math_float.h"
#include "math_lerp.h"
#include "math_floatvec.h"
#include "math_matrix.h"
#include "math_kahansummation.h"
#include "allocator_heap.h"
#include "allocator_virtual.h"
#include "allocator_heap_or_virtual.h"
#include "cstr.h"
#include "ds_slice.h"
#include "ds_string.h"
#include "allocator_pagelist.h"
#include "ds_stack_resizeable_cont.h"
#include "ds_stack_nonresizeable_stack.h"
#include "ds_stack_nonresizeable.h"
#include "ds_stack_resizeable_pagelist.h"
#include "ds_pagetree.h"
#include "ds_list.h"
#include "ds_stack_cstyle.h"
#include "ds_hashset_cstyle.h"
#include "ds_bitarray_nonresizeable_stack.h"
#include "filesys.h"
#include "cstr_integer.h"
#include "cstr_float.h"
#include "timedate.h"
#include "thread_atomics.h"
#include "ds_mtqueue_mrmw_nonresizeable.h"
#include "ds_mtqueue_mrsw_nonresizeable.h"
#include "ds_mtqueue_srmw_nonresizeable.h"
#include "ds_mtqueue_srsw_nonresizeable.h"
#include "threading.h"
#define LOGGER_ENABLED   1
#include "logger.h"
#define PROF_ENABLED   0
#define PROF_ENABLED_AT_LAUNCH   0
#include "profile.h"
#include "rand.h"
#include "allocator_heap_findleaks.h"
#include "mainthread.h"
#include "ds_hashset_complexkey.h"
#include "text_parsing.h"
#include "ds_stack_resizeable_cont_addbacks.h"

#define OPENGL_INSTEAD_OF_SOFTWARE       0
#define GLW_RAWINPUT                     0
#define GLW_RAWINPUT_KEYBOARD            0
#define GLW_RAWINPUT_KEYBOARD_NOHOTKEY   1
#include "glw.h"

#define USE_FILEMAPPED_OPEN 0 // TODO: unfinished
#define USE_SIMPLE_SCROLLING 0 // TODO: questionable decision to try to simplify scrolling datastructs, getting rid of smooth scrolling.

#include "ui_propdb.h"
#include "ui_font.h"
#include "ui_render.h"
#include "ui_buf2.h"
#include "ui_txt2.h"
#include "ui_diffview.h"

struct
app_t
{
  glwclient_t client;
  bool fullscreen;
  bool kb_command; // else edit.
  stack_resizeable_cont_t<f32> stream;
  stack_resizeable_cont_t<font_t> fonts;
  diffview_t diff;
};

static app_t g_app = {};

void
AppInit( app_t* app )
{
  GlwInit();

  g_client = &app->client;

  Alloc( app->stream, 65536 );
  Alloc( app->fonts, 16 );
  app->fullscreen = 0;
  app->kb_command = 1;

  stack_nonresizeable_stack_t<u8, c_fspath_len> exe;
  FsGetExe( exe.mem, Capacity( exe ), &exe.len );
  auto exe_name = FileNameOnly( ML( exe ) );

// TODO: do this better when version is not defined? or make it always defined?
// this should be defined to a "yy.mm.dd.hh.mm.ss" datetime string by the build system.
// this is how we know what version to display, to allow matching of .exe to source code.
#if !defined(JCVERSION)
#define JCVERSION "unknown_version"
#endif

  auto version = SliceFromCStr( JCVERSION );

  stack_resizeable_cont_t<u8> window_name;
  Alloc( window_name, exe_name.len + version.len + 1 );
  AddBackContents( &window_name, exe_name );
  AddBackCStr( &window_name, " " );
  AddBackContents( &window_name, version );

  Init( app->diff );

  GlwInitWindow(
    app->client,
    ML( window_name )
    );

  Free( window_name );
}

void
AppKill( app_t* app )
{
  Kill( app->diff );

  ForLen( i, app->fonts ) {
    auto& font = app->fonts.mem[i];
    FontKill( font );
  }
  Free( app->fonts );

  Free( app->stream );

  GlwKillWindow( app->client );
  GlwKill();
}

void
LogUI( const void* cstr ... )
{
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
  diff,
  bkgd,
  COUNT
};

__OnRender( AppOnRender )
{
  ProfFunc();

  auto app = Cast( app_t*, misc );
  auto& font = GetFont( app, Cast( enum_t, fontid_t::normal ) );
  auto line_h = FontLineH( font );
  auto px_space_advance = FontGetAdvance( font, ' ' );
  auto orig_dim = bounds.p1 - bounds.p0;
  auto zrange = _vec2<f32>( 0, 1 );

  auto spaces_per_tab = GetPropFromDb( u8, u8_spaces_per_tab );
  auto rgba_mode_command = GetPropFromDb( vec4<f32>, rgba_mode_command );
  auto rgba_mode_edit = GetPropFromDb( vec4<f32>, rgba_mode_edit );
  auto modeoutline_pct = GetPropFromDb( f32, f32_modeoutline_pct );

  auto timestep = MIN( timestep_realtime, timestep_fixed );

  // draw the global kb_command as a colored line.
  {
    auto px_border = MAX( 3.0f, Round32( modeoutline_pct * MinElem( bounds.p1 - bounds.p0 ) ) );
    RenderQuad(
      app->stream,
      app->kb_command ? rgba_mode_command : rgba_mode_edit,
      bounds.p0,
      _vec2( bounds.p1.x, bounds.p0.y + px_border ),
      bounds,
      GetZ( zrange, applayer_t::bkgd )
      );
    bounds.p0.y = MIN( bounds.p0.y + px_border, bounds.p1.y );
  }

  DiffRender(
    app->diff,
    target_valid,
    app->stream,
    font,
    bounds,
    ZRange( zrange, applayer_t::diff ),
    timestep_realtime,
    timestep_fixed
    );

  if( app->stream.len ) {

#define RAND_NOISE 0
  #if RAND_NOISE
    rng_xorshift32_t rng;
    Init( rng, TimeTSC() );
  #endif

    auto pos = app->stream.mem;
    auto end = app->stream.mem + app->stream.len;
    AssertCrash( app->stream.len % 10 == 0 );
    while( pos < end ) {
//      Prof( tmp_UnpackStream );
      vec2<u32> q0;
      vec2<u32> q1;
      vec2<u32> tc0;
      vec2<u32> tc1;
      q0.x  = Round_u32_from_f32( *pos++ );
      q0.y  = Round_u32_from_f32( *pos++ );
      q1.x  = Round_u32_from_f32( *pos++ );
      q1.y  = Round_u32_from_f32( *pos++ );
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

      AssertCrash( q0.x <= q1.x );
      AssertCrash( q0.y <= q1.y );
      auto dstdim = q1 - q0;
      AssertCrash( tc0.x <= tc1.x );
      AssertCrash( tc0.y <= tc1.y );
      auto srcdim = tc1 - tc0;
      AssertCrash( q0.x + srcdim.x <= app->client.dim.x );
      AssertCrash( q0.y + srcdim.y <= app->client.dim.y );
      AssertCrash( q0.x + dstdim.x <= app->client.dim.x ); // should have clipped by now.
      AssertCrash( q0.y + dstdim.y <= app->client.dim.y );
      AssertCrash( tc0.x + srcdim.x <= font.tex_dim.x );
      AssertCrash( tc0.y + srcdim.y <= font.tex_dim.y );
//      ProfClose( tmp_UnpackStream );

      bool subpixel_quad =
        ( q0.x == q1.x )  ||
        ( q0.y == q1.y );

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
//          Prof( tmp_DrawOpaqueQuads );

          // PERF: this is ~0.7 cycles / pixel
          // suprisingly, rep stosq is about the same as rep stosd!
          // presumably the hardware folks made that happen, so old 32bit code also got faster.
          auto copy2 = Pack( copy, copy );
          Fori( u32, j, 0, dstdim.y ) {
            auto dst = app->client.fullscreen_bitmap_argb + ( q0.y + j ) * app->client.dim.x + ( q0.x + 0 );
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
//          Prof( tmp_DrawAlphaQuads );
          // more complicated / slow alpha case.

          Fori( u32, j, 0, dstdim.y ) {
            auto dst = app->client.fullscreen_bitmap_argb + ( q0.y + j ) * app->client.dim.x + ( q0.x + 0 );
            Fori( u32, k, 0, dstdim.x ) {

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
//        Prof( tmp_DrawTexQuads );
        // for now, assume quads with nonempty tex coords are exact-size copies onto dst.
        // that's true of all our text glyphs, which makes for faster code here.
        //
        // TODO: handle scaling up, or make a separate path for scaled-up tex, non-tex, etc. quads
        // we'll need separate paths for tris, lines anyways, so maybe do that.

        Fori( u32, j, 0, srcdim.y ) {
          auto dst = app->client.fullscreen_bitmap_argb + ( q0.y + j ) * app->client.dim.x + ( q0.x + 0 );
          auto src = font.tex_mem + ( tc0.y + j ) * font.tex_dim.x + ( tc0.x + 0 );
          Fori( u32, k, 0, srcdim.x ) {

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

__AppCmd( CmdToggleKbCommand )
{
  app->kb_command = !app->kb_command;
}

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

      app_cmdmap_t table[] = {
        _appcmdmap( GetPropFromDb( glwkeybind_t, keybind_app_toggle_kbcommand ), CmdToggleKbCommand ),
      };
      ForEach( entry, table ) {
        if( GlwKeybind( key, entry.keybind ) ) {
          entry.fn( app );
          target_valid = 0;
          ran_cmd = 1;
        }
      }

      if( GlwKeybind( key, GetPropFromDb( glwkeybind_t, keybind_app_switch_kbcommand_from_kbedit ) ) ) {
        if( !app->kb_command ) {
          app->kb_command = 1;
          target_valid = 0;
          ran_cmd = 1;
        }
      }

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
        case glwkey_t::fn_6: {
          LogUI( "Started profiling." );
          ProfEnable();
          ran_cmd = 1;
        } break;

        case glwkey_t::fn_7: {
          ProfDisable();
          LogUI( "Stopped profiling." );
          ran_cmd = 1;
        } break;

        case glwkey_t::fn_8: {
          ProfReset();
          LogUI( "Reset profiling buffer." );
          ran_cmd = 1;
        } break;
#endif

      }
    } break;

    default: UnreachableCrash();
  }
  if( !ran_cmd ) {
    auto keylocks = GlwKeylocks();
    DiffControlKeyboard(
      app->diff,
      app->kb_command,
      target_valid,
      ran_cmd,
      type,
      key,
      keylocks
      );
  }
}

__OnMouseEvent( AppOnMouseEvent )
{
  ProfFunc();

  auto app = Cast( app_t*, misc );
  auto mp = _vec2( Cast( f32, m.x ), Cast( f32, m.y ) );

  auto& font = GetFont( app, Cast( enum_t, fontid_t::normal ) );

  cursortype = glwcursortype_t::arrow;

  DiffControlMouse(
    app->diff,
    target_valid,
    font,
    bounds,
    type,
    btn,
    m,
    raw_delta,
    dwheel,
    1
    );
}

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

  DiffWindowEvent(
    app->diff,
    type,
    dim,
    dpi,
    focused,
    target_valid
    );
}

int
Main( stack_resizeable_cont_t<slice_t>& args )
{
//  PinThreadToOneCore();

  auto app = &g_app;
  AppInit( app );

  if( args.len != 2 ) {
    auto message = "expected two params: a left filepath, and a right filepath.";
    MessageBoxA(
      0,
      Cast( LPCSTR, message ),
      "executable given bad params",
      0
      );
    return -1;
  }

  auto filepath_l = args.mem[0];
  auto filepath_r = args.mem[1];

  auto file_l = FileOpen( ML( filepath_l ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
  if( !file_l.loaded ) {
    // TODO: better error diagnostics here.
    u8 tmp[1024];
    CstrCopy( tmp, filepath_l.mem, MIN( filepath_l.len, _countof( tmp ) - 1 ) );
    MessageBoxA(
      0,
      Cast( LPCSTR, tmp ),
      "Couldn't open file!",
      0
      );
    return -1;
  }

  auto file_r = FileOpen( ML( filepath_l ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
  if( !file_r.loaded ) {
    // TODO: better error diagnostics here.
    u8 tmp[1024];
    CstrCopy( tmp, filepath_r.mem, MIN( filepath_r.len, _countof( tmp ) - 1 ) );
    MessageBoxA(
      0,
      Cast( LPCSTR, tmp ),
      "Couldn't open file!",
      0
      );
    return -1;
  }

  auto filecontents_l = FileAlloc( file_l );
  auto filecontents_r = FileAlloc( file_l );
  FileFree( file_r );
  FileFree( file_l );
  SetFiles( app->diff, filecontents_l, filecontents_r );

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

  Free( filecontents_r );
  Free( filecontents_l );

  // Do this before destroying any datastructures, so other threads stop trying to access things.
  SignalQuitAndWaitForTaskThreads();

  AppKill( app );

  return 0;
}




Inl void
IgnoreSurroundingSpaces( u8*& a, idx_t& a_len )
{
  while( a_len  &&  AsciiIsWhitespace( a[0] ) ) {
    a += 1;
    a_len -= 1;
  }
  // TODO: prog_cmd_line actually has two 0-terms!
  while( a_len  &&  ( !a[a_len - 1]  ||  AsciiIsWhitespace( a[a_len - 1] ) ) ) {
    a_len -= 1;
  }
}

int WINAPI
WinMain( HINSTANCE prog_inst, HINSTANCE prog_inst_prev, LPSTR prog_cmd_line, int prog_cmd_show )
{
  MainInit();

  u8* cmdline = Str( prog_cmd_line );
  idx_t cmdline_len = CstrLength( Str( prog_cmd_line ) );

  stack_resizeable_cont_t<slice_t> args;
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
      auto end = StringScanR( arg, cmdline_len - 1, '"' );
      arg_len = !end  ?  0  :  end - arg;
      IgnoreSurroundingSpaces( arg, arg_len );
    } else {
      auto end = StringScanR( arg, cmdline_len - 1, ' ' );
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
