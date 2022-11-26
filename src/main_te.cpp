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

#define RENDER_UNPACKED   0

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
#include "ui_cmd.h"
#include "ui_edit2.h"

struct
notify_t
{
  slice_t msg;
  kahansum32_t t;
};

struct
notifyui_t
{
  pagelist_t mem;
  listwalloc_t<notify_t, allocator_pagelist_t, allocation_pagelist_t> msgs;
  pagelist_t lmem; // TODO: why do we crash w/ 1 pagelist?
  bool fadedin;
};

Inl void
Init( notifyui_t& ui )
{
  Init( ui.mem, 65536 );
  Init( ui.lmem, 65536 );
  Init( ui.msgs, allocator_pagelist_t{ &ui.lmem } );
  ui.fadedin = 0;
}

Inl void
Kill( notifyui_t& ui )
{
  Kill( ui.mem );
  Kill( ui.lmem );
  Kill( ui.msgs );
  ui.fadedin = 0;
}


Enumc( app_active_t )
{
  edit,
  cmd,
};

#if PROF_ENABLED
  struct
  prof_renderedscope_t
  {
    rectf32_t rect;
    prof_loc_t loc;
    u64 time_duration;
    u32 tid;
  };
#endif // PROF_ENABLED


struct
debugmode_t
{
  // the idea is to have each glw event log a new line containing info about the event.
  // hopefully that's enough to spot issues like mouse positioning in remote desktop situations, etc.
  txt_t display;
};

Inl void
Init( debugmode_t* dm )
{
  Init( dm->display );
  TxtLoadEmpty( dm->display );
}
Inl void
Kill( debugmode_t* dm )
{
  Kill( dm->display );
}


struct
app_t
{
  glwclient_t client;
  edit_t edit;
  cmd_t cmd;
  app_active_t active;
  bool fullscreen;
  bool show_debugmode;
  debugmode_t debugmode;
  bool kb_command; // else edit.
  stack_resizeable_cont_t<f32> stream;

#if OPENGL_INSTEAD_OF_SOFTWARE
  u32 glstream;
  shader_tex2_t shader;
#endif // !OPENGL_INSTEAD_OF_SOFTWARE

  stack_resizeable_cont_t<font_t> fonts;
  notifyui_t notifyui;

#if PROF_ENABLED
  stack_resizeable_cont_t<prof_renderedscope_t> prof_renderedscopes;
#endif // PROF_ENABLED
};

static app_t g_app = {};


void
AppInit( app_t* app )
{
  GlwInit();

  g_client = &app->client;

  Init( app->notifyui );
  Alloc( app->stream, 65536 );
  Alloc( app->fonts, 16 );
  app->fullscreen = 0;
  app->show_debugmode = 0;
  Init( &app->debugmode );
  app->active = app_active_t::edit;
  app->kb_command = 1;
  EditInit( app->edit );
  CmdInit( app->cmd );

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

  GlwInitWindow(
    app->client,
    ML( window_name )
    );

  Free( window_name );

#if OPENGL_INSTEAD_OF_SOFTWARE
  glGenBuffers( 1, &app->glstream );
  ShaderInit( app->shader );
#endif // OPENGL_INSTEAD_OF_SOFTWARE

#if PROF_ENABLED
  Alloc( app->prof_renderedscopes, 1 );
#endif // PROF_ENABLED
}

void
AppKill( app_t* app )
{
  ForLen( i, app->fonts ) {
    auto& font = app->fonts.mem[i];
    FontKill( font );
  }
  Free( app->fonts );

#if OPENGL_INSTEAD_OF_SOFTWARE
  ShaderKill( app->shader );
  glDeleteBuffers( 1, &app->glstream );
#endif // OPENGL_INSTEAD_OF_SOFTWARE

  Free( app->stream );
  EditKill( app->edit );
  CmdKill( app->cmd );
  Kill( &app->debugmode );
  Kill( app->notifyui );

#if PROF_ENABLED
  Free( app->prof_renderedscopes );
#endif // PROF_ENABLED

  GlwKillWindow( app->client );
  GlwKill();
}


#if PROF_ENABLED
  Inl prof_renderedscope_t*
  FindTooltip( app_t* app, vec2<f32> mp )
  {
    prof_renderedscope_t* closest = 0;
    f32 min_distance = MAX_f32;
    constant f32 epsilon = 0.001f;
    FORLEN( renderedscope, i, app->prof_renderedscopes )
      if( PtInBox( mp, renderedscope->rect.p0, renderedscope->rect.p1, epsilon ) ) {
        min_distance = 0.0f;
        closest = renderedscope;
        break;
      }
      auto distance = DistanceToBox( mp, renderedscope->rect.p0, renderedscope->rect.p1 );
      if( distance < min_distance ) {
        min_distance = distance;
        closest = renderedscope;
      }
    }

    // allow some slop distance, for really small rects that are hard to mouse over.
    if( min_distance < 2.0f ) {
      return closest;
    }
    return 0;
  }
#endif // PROF_ENABLED


void
LogUI( const void* cstr ... )
{
  auto app = &g_app;
  static stack_nonresizeable_stack_t<u8, 32768> buffer;

  va_list args;
  va_start( args, cstr );

  buffer.len += vsprintf_s(
    Cast( char* const, buffer.mem + buffer.len ),
    MAX( Capacity( buffer ), buffer.len ) - buffer.len,
    Cast( const char* const, cstr ),
    args
    );

  va_end( args );

  auto notify_elem = AddLast( app->notifyui.msgs );
  auto notify = &notify_elem->value;
  notify->msg.mem = AddPagelist( app->notifyui.mem, u8, 1, buffer.len );
  notify->msg.len = buffer.len;
  Memmove( notify->msg.mem, ML( buffer ) );
  notify->t = {};

  buffer.len = 0;

  app->client.target_valid = 0;
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


#if PROF_ENABLED

  struct
  perthread_t
  {
    u32 max_depth;
    u32 min_depth;
  };

  Inl void
  RenderThreadTimelineElem(
    stack_resizeable_cont_t<prof_renderedscope_t>& prof_renderedscopes,
    stack_resizeable_cont_t<f32>& stream,
    vec2<f32> zrange,
    font_t& font,
    u8 spaces_per_tab,
    rectf32_t bounds_orig,
    rectf32_t bounds_prof,
    f32 line_h,
    u64 time_first,
    u64 time_last,
    u64 time_start,
    u64 time_elapsed,
    u32 id,
    u32 tid,
    u32 scope_depth,
    stack_resizeable_cont_t<u32>& threadids,
    stack_resizeable_cont_t<perthread_t>& per_thread,
    idx_t thread_count
    )
  {
    auto x0_elem = Cast( f32, Lerp_from_f64(
      bounds_prof.p0.x,
      bounds_prof.p1.x,
      Cast( f64, time_start - time_first ),
      0.0,
      Cast( f64, time_last )
      ));
    auto x1_elem = Cast( f32, Lerp_from_f64(
      bounds_prof.p0.x,
      bounds_prof.p1.x,
      Cast( f64, time_start - time_first + time_elapsed ),
      0.0,
      Cast( f64, time_last )
      )); // - 1.0f;

    if( Ceil64( x0_elem ) >= Ceil64( x1_elem ) ) {
      return;
    }

    AssertCrash( LTEandLTE( x0_elem, bounds_orig.p0.x, bounds_orig.p1.x ) );
    AssertCrash( LTEandLTE( x1_elem, bounds_orig.p0.x, bounds_orig.p1.x ) );

    rng_xorshift32_t rng;
    Init( rng, id ^ ( id << 16 ) ); // seed with the same id, so we get the same color per-id.
    auto color = _vec4<f32>(
      ( 30u + Rand32( rng ) % 50u ) / 100.0f,
      ( 30u + Rand32( rng ) % 50u ) / 100.0f,
      ( 30u + Rand32( rng ) % 50u ) / 100.0f,
      1.0f
      );

    idx_t thread_idx;
    auto found = TIdxScanR( &thread_idx, ML( threadids ), tid );
    AssertCrash( found );

    auto perthread = per_thread.mem[ thread_idx ];
    u32 max_depth = perthread.max_depth;
    u32 min_depth = perthread.min_depth;

    // convert to [ 0, max_depth - min_depth ] space.
    scope_depth -= min_depth;
    max_depth -= min_depth;

    auto dy_thread = ( bounds_prof.p1.y - bounds_prof.p0.y ) / thread_count;
    auto dy_depth = dy_thread / ( max_depth + 1 );

    auto bounds_elem = _rect(
      _vec2( x0_elem, bounds_prof.p0.y + thread_idx * dy_thread + ( scope_depth + 0 ) * dy_depth ),
      _vec2( x1_elem, bounds_prof.p0.y + thread_idx * dy_thread + ( scope_depth + 1 ) * dy_depth - 1.0f )
      );

    AssertCrash( LTEandLTE( bounds_elem.p0.y, bounds_orig.p0.y, bounds_orig.p1.y ) );
    AssertCrash( LTEandLTE( bounds_elem.p1.y, bounds_orig.p0.y, bounds_orig.p1.y ) );

    auto loc = *Cast( prof_loc_t*, g_prof_locations + id );

    // store what we rendered, so we can do mouse interactivity, e.g. tooltips.
    auto renderedscope = AddBack( prof_renderedscopes );
    renderedscope->rect = bounds_elem;
    renderedscope->loc = loc;
    renderedscope->time_duration = time_elapsed;
    renderedscope->tid = tid;

    RenderQuad(
      stream,
      color,
      bounds_elem,
      bounds_prof,
      GetZ( zrange, applayer_t::bkgd )
      );

    auto bounds_elem_interior = _rect(
      Min( bounds_prof.p1, bounds_elem.p0 + _vec2<f32>( 1, 1 ) ),
      Max( bounds_prof.p0, bounds_elem.p1 - _vec2<f32>( 1, 1 ) )
      );
    auto color_interior = color * ( 1.0f / 0.8f );

    RenderQuad(
      stream,
      color_interior,
      bounds_elem_interior,
      bounds_prof,
      GetZ( zrange, applayer_t::bkgd )
      );

    if( bounds_elem.p1.x - bounds_elem.p0.x > line_h * 5 ) {
      auto name = SliceFromCStr( loc.name );
      auto name_w = LayoutString( font, spaces_per_tab, ML( name ) );
      auto color_text =
        ( PerceivedBrightness( color ) < 0.6f )  ?
          _vec4<f32>( 1, 1, 1, 1 )  :
          _vec4<f32>( 0, 0, 0, 1 );

      DrawString(
        stream,
        font,
        AlignCenter( bounds_elem, name_w, line_h ),
        GetZ( zrange, editlayer_t::txt ),
        bounds_elem,
        color_text,
        spaces_per_tab,
        ML( name )
        );
    }
  }

  void
  DrawProfiler(
    app_t* app
    )
  {
    idx_t num_prof_elems = 0;
    if( g_prof_enabled  ||  g_saved_prof_pos ) {
      num_prof_elems = MIN( g_prof_pos, g_prof_buffer_len );
      if( !g_prof_enabled ) {
        num_prof_elems = g_saved_prof_pos;
      }
    }

    {
      stack_nonresizeable_stack_t<u8, 32> tmp;
      CsFrom_u64( tmp.mem, Capacity( tmp ), &tmp.len, num_prof_elems, 1 );
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

      bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );
    }

    app->prof_renderedscopes.len = 0;

    if( num_prof_elems ) {

      Reserve( app->prof_renderedscopes, num_prof_elems / 100 );

      stack_resizeable_cont_t<u32> threadids;
      Alloc( threadids, 10 );

      // TODO: we only want to know the max depth of visible elems.
      stack_resizeable_cont_t<perthread_t> per_thread;
      Alloc( per_thread, 10 );

      // TODO: bake in the time_first calculation into actual prof_elem_t data ?
      //   or is that going to slow things down in the optimal future?

      //
      // note that we don't actually record the prof_entry_t until the scope's closed, but we've taken the
      // time_start at the scope-open. so we actually have to search to find the earliest time_start, since
      // we could have arbitrary durations delaying when we log that first, early entry.
      //
      // note we have a different problem on scope-close that also means we have to search.
      // we do the atomic inc to get the prof_elem_t index, and then we take the time.
      // if that execution interleaves with another thread, we might have an earlier entry with a later time.
      // TODO: handle this scope-close problem.
      //   i'm ignoring it for now, since the scaling difference it has isn't likely to cause much concern now.
      //   but for better correctness, it'd be nice to resolve this.
      //
      // even if we split the prof_elem_t entries into scope-start and scope-close, we'd still have this
      // interleaving concern about when we do the atomic inc vs. when we take the time.
      // so i think fundamentally we just have to do a search, unless we use a more complicated model than
      // the simple atomic inc.
      // something like a lock-free queue, i.e. use mtqueue_srmw_t<prof_elem_t> for example.
      // that would give slower instrumentation, since it requires retries.
      // i don't know that we care enough about the strict ordering, but we do care about the cost of retries.
      // so for now i think we'll stick with searches for first/last.
      //
  //    auto first_elem = Cast( prof_elem_t*, g_prof_buffer + 0 );
  //    u64 time_first = first_elem->time_start;
      u64 time_first = MAX_u64;

  //    auto last_elem = Cast( prof_elem_t*, g_prof_buffer + num_prof_elems - 1 );
  //    u64 time_last = last_elem->time_start + last_elem->time_elapsed;
      u64 time_last = 0;

      For( i, 0, num_prof_elems ) {
        auto elem = Cast( prof_elem_t*, g_prof_buffer + i );

  #if PROF_SPLITSCOPES
        auto time = elem->time;
        time_first = MIN( time_first, time );
        time_last = MAX( time_last, time );
  #else
        // early out for tiny events, which we can have millions of.
        auto time_start = elem->time_start;
        auto time_end = elem->time_end;
  //      if( time_elapsed < cycles_per_px ) {
  //        continue;
  //      }

        time_first = MIN( time_first, time_start );
        time_last = MAX( time_last, time_end );
  #endif

  #if PROF_SPLITSCOPES
        if( elem->start  &&  !TContains( ML( threadids ), &elem->tid ) ) {
          *AddBack( threadids ) = elem->tid;
        }
  #else
        idx_t thread_idx;
        auto found = TIdxScanR( &thread_idx, ML( threadids ), elem->tid );
        if( !found ) {
          *AddBack( threadids ) = elem->tid;
          auto perthread = AddBack( per_thread );
          perthread->max_depth = elem->depth;
          perthread->min_depth = elem->depth;
        }
        else {
          auto perthread = per_thread.mem + thread_idx;
          perthread->max_depth = MAX( perthread->max_depth, elem->depth );
          perthread->min_depth = MIN( perthread->min_depth, elem->depth );
        }
  #endif
      }
      auto thread_count = threadids.len;

      // for now, use the top half for prof display
      auto bounds_orig = bounds;
      auto bounds_prof = bounds;
      bounds_prof.p1.y = 0.66f * ( bounds.p0.y + bounds.p1.y );
      bounds.p0.y = bounds_prof.p1.y;

      // reserve space at the left side for thread id headers.
      f32 threadid_w = 0.0f;
      FORLEN( tid, thread_idx, threadids )
        stack_nonresizeable_stack_t<u8, 32> tmp;
        CsFrom_u64( tmp.mem, Capacity( tmp ), &tmp.len, *tid, 0 );
        auto w = LayoutString( font, spaces_per_tab, ML( tmp ) );
        threadid_w = MAX( threadid_w, w );
      }
      threadid_w += 0.5f * px_space_advance;

      auto bounds_thread_hdrs = bounds_prof;
      bounds_thread_hdrs.p1.x = bounds_prof.p0.x + threadid_w;
      bounds_prof.p0.x = bounds_thread_hdrs.p1.x;

      // draw thread id headers
      auto dy_thread = ( bounds_prof.p1.y - bounds_prof.p0.y ) / thread_count;
      FORLEN( tid, thread_idx, threadids )
        stack_nonresizeable_stack_t<u8, 32> tmp;
        CsFrom_u64( tmp.mem, Capacity( tmp ), &tmp.len, *tid, 0 );
        auto w = LayoutString( font, spaces_per_tab, ML( tmp ) );
        auto hdr_p0 = bounds_thread_hdrs.p0 + _vec2( 0.0f, dy_thread * thread_idx );
        auto hdr_p1 = _vec2( bounds_thread_hdrs.p1.x, bounds_thread_hdrs.p0.y + dy_thread * ( thread_idx + 1 ) );
        auto hdr_pos = AlignCenter( _rect( hdr_p0, hdr_p1 ), w, line_h );
        DrawString(
          app->stream,
          font,
          hdr_pos,
          GetZ( zrange, applayer_t::txt ),
          bounds_thread_hdrs,
          rgba_notify_text,
          spaces_per_tab,
          ML( tmp )
          );
        RenderQuad(
          app->stream,
          rgba_notify_text,
          _vec2( hdr_p0.x, hdr_p1.y - 1.0f ),
          _vec2( bounds_prof.p1.x, hdr_p1.y ),
          bounds_orig,
          GetZ( zrange, applayer_t::bkgd )
          );
      }
      RenderQuad(
        app->stream,
        rgba_notify_text,
        _vec2( bounds_thread_hdrs.p1.x - 1.0f, bounds_thread_hdrs.p0.y ),
        _vec2( bounds_thread_hdrs.p1.x, bounds_thread_hdrs.p1.y ),
        bounds_orig,
        GetZ( zrange, applayer_t::bkgd )
        );

      // convert time_last to the space where the origin is time_first.
      time_last -= time_first;
      auto cycles_per_px = Ceil32( Cast( f32, time_last ) / ( bounds_prof.p1.x - bounds_prof.p0.x ) );

      // TODO: parent/child scope discovery or storage, so we can do the flame chart thing.
      //   or at least get the z-order right.

  #if PROF_SPLITSCOPES
      stack_resizeable_cont_t<prof_elem_t> open_scopes;
      Alloc( open_scopes, 32000 );
  #else
  #endif

      For( i, 0, num_prof_elems ) {
        auto elem = *Cast( prof_elem_t*, g_prof_buffer + i );

  #if PROF_SPLITSCOPES
        if( elem.start ) {
          *AddBack( open_scopes ) = elem;
          continue;
        }

        idx_t opened_idx = 0;
        prof_elem_t* popen = 0;
        REVERSEFORLEN( open_scope, j, open_scopes )
          if( elem.id == open_scope->id  &&  elem.tid == open_scope->tid ) {
            opened_idx = j;
            popen = open_scope;
            break;
          }
        }
        if( !popen ) {
          // TODO: should we need this early-out? somehow pushing a scope-end as the first elem?
          continue;
        }
        auto open = *popen;
        UnorderedRemAt( open_scopes, opened_idx );
  #else
  #endif

        // early out for tiny events, which we can have millions of.
  #if PROF_SPLITSCOPES
        auto time_start = open.time;
        auto time_elapsed = elem.time - time_start;
  #else
        auto time_start = elem.time_start;
        auto time_elapsed = elem.time_end - time_start;
  #endif
        if( time_elapsed < cycles_per_px ) {
          continue;
        }

        RenderThreadTimelineElem(
          app->prof_renderedscopes,
          app->stream,
          zrange,
          font,
          spaces_per_tab,
          bounds_orig,
          bounds_prof,
          line_h,
          time_first,
          time_last,
          time_start,
          time_elapsed,
          elem.id,
          elem.tid,
  #if PROF_DEPTHCOUNT
          elem.depth,
  #else
          0,
  #endif
          threadids,
          per_thread,
          thread_count
          );
      }

  #if PROF_SPLITSCOPES
      // now render all the scopes that haven't closed yet, with time_last as a pseudo-close.
      FORLEN( open_scope, j, open_scopes )
        auto time_start = open_scope->time;
        auto time_elapsed = time_last + time_first - time_start;
        RenderThreadTimelineElem(
          app->prof_renderedscopes,
          app->stream,
          zrange,
          font,
          spaces_per_tab,
          bounds_orig,
          bounds_prof,
          line_h,
          time_first,
          time_last,
          time_start,
          time_elapsed,
          open_scope->id,
          open_scope->tid,
  #if PROF_DEPTHCOUNT
          open_scope->depth,
  #else
          0,
  #endif
          threadids,
          per_thread,
          thread_count
          );
      }
  #else
  #endif

  #if PROF_SPLITSCOPES
      Free( open_scopes );
  #else
  #endif

      Free( per_thread );
      Free( threadids );

      // draw a tooltip
      auto mp = _vec2( Cast( f32, m.x ), Cast( f32, m.y ) );
      auto closest = FindTooltip( app, mp );
      if( closest ) {

        auto scope_name = SliceFromCStr( closest->loc.name );
        auto name_w = LayoutString( font, spaces_per_tab, ML( scope_name ) );
        auto scope_file = SliceFromCStr( closest->loc.file );
        auto file_w = LayoutString( font, spaces_per_tab, ML( scope_file ) );
        auto lineno = closest->loc.line;
        stack_nonresizeable_stack_t<u8, 256> scope_lineno;
        CsFrom_u64( scope_lineno.mem, Capacity( scope_lineno ), &scope_lineno.len, lineno, 1 );
        auto lineno_w = LayoutString( font, spaces_per_tab, ML( scope_lineno ) );
        auto time_duration = closest->time_duration;
        stack_nonresizeable_stack_t<u8, 256> scope_duration;
        CsFrom_u64( scope_duration.mem, Capacity( scope_duration ), &scope_duration.len, time_duration, 1 );
        auto duration_w = LayoutString( font, spaces_per_tab, ML( scope_duration ) );
        auto tid = closest->tid;
        stack_nonresizeable_stack_t<u8, 256> scope_tid;
        CsFrom_u64( scope_tid.mem, Capacity( scope_tid ), &scope_tid.len, tid, 0 );
        auto tid_w = LayoutString( font, spaces_per_tab, ML( scope_tid ) );
        auto colon = SliceFromCStr( " : " );
        auto colon_w = LayoutString( font, spaces_per_tab, ML( colon ) );

        // scope_name
        // scope_duration
        // scope_file : scope_lineno
        // scope_tid

        auto tt_h = 4.0f * line_h;
        auto tt_w = MAX4(
          name_w,
          duration_w,
          file_w + colon_w + lineno_w,
          tid_w
          );

  //      // TODO: smart positioning based on mouse pos.
  //      auto bounds_tt = _rect( bounds_prof.p0, bounds_prof.p0 + _vec2( tt_w, tt_h ) );

        auto margin = _vec2( 2.0f * px_space_advance );

        auto tt_dim = _vec2( tt_w, tt_h );
        auto tt_p0 = mp + margin;
        auto tt_p1 = tt_p0 + tt_dim;
        if( tt_p1.x > bounds_prof.p1.x  ||
            tt_p1.y > bounds_prof.p1.y )
        {
          tt_p0 = mp - margin - tt_dim;
          tt_p0.x = MAX( tt_p0.x, bounds_prof.p0.x );
          tt_p0.y = MAX( tt_p0.y, bounds_prof.p0.y );
          tt_p1 = tt_p0 + tt_dim;
        }

        auto bounds_tt = _rect( tt_p0, tt_p1 );

        auto rgba_tooltip_bkgd = _vec4( 0.2f, 0.2f, 0.2f, 0.8f );
        RenderQuad(
          app->stream,
          rgba_tooltip_bkgd,
          bounds_tt,
          bounds_orig,
          GetZ( zrange, applayer_t::bkgd )
          );
        DrawString(
          app->stream,
          font,
          bounds_tt.p0 + _vec2( 0.0f, 0.0f ),
          GetZ( zrange, applayer_t::txt ),
          bounds_tt,
          rgba_notify_text,
          spaces_per_tab,
          ML( scope_name )
          );
        DrawString(
          app->stream,
          font,
          bounds_tt.p0 + _vec2( 0.0f, line_h ),
          GetZ( zrange, applayer_t::txt ),
          bounds_tt,
          rgba_notify_text,
          spaces_per_tab,
          ML( scope_duration )
          );
        DrawString(
          app->stream,
          font,
          bounds_tt.p0 + _vec2( 0.0f, 2.0f * line_h ),
          GetZ( zrange, applayer_t::txt ),
          bounds_tt,
          rgba_notify_text,
          spaces_per_tab,
          ML( scope_file )
          );
        DrawString(
          app->stream,
          font,
          bounds_tt.p0 + _vec2( file_w, 2.0f * line_h ),
          GetZ( zrange, applayer_t::txt ),
          bounds_tt,
          rgba_notify_text,
          spaces_per_tab,
          ML( colon )
          );
        DrawString(
          app->stream,
          font,
          bounds_tt.p0 + _vec2( file_w + colon_w, 2.0f * line_h ),
          GetZ( zrange, applayer_t::txt ),
          bounds_tt,
          rgba_notify_text,
          spaces_per_tab,
          ML( scope_lineno )
          );
        DrawString(
          app->stream,
          font,
          bounds_tt.p0 + _vec2( 0.0f, 3.0f * line_h ),
          GetZ( zrange, applayer_t::txt ),
          bounds_tt,
          rgba_notify_text,
          spaces_per_tab,
          ML( scope_tid )
          );
      }
    }
  }

#endif // PROF_ENABLED

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
  auto rgba_notify_text = GetPropFromDb( vec4<f32>, rgba_notify_text );
  auto rgba_notify_bkgd = GetPropFromDb( vec4<f32>, rgba_notify_bkgd );
  auto rgba_mode_command = GetPropFromDb( vec4<f32>, rgba_mode_command );
  auto rgba_mode_edit = GetPropFromDb( vec4<f32>, rgba_mode_edit );
  auto notification_lifetime = GetPropFromDb( f32, f32_notification_lifetime );
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

  if( app->show_debugmode ) {

    // this causes infinite spamming of this entry, which we don't really want.
    // it's because the debugmode.display has enough scroll_vel as we keep adding stuff to keep rendering.
    // so for now, turn this off.
#if 0
    stack_resizeable_cont_t<u8> line;
    Alloc( line, 1024 );
    AddBackCStr( &line, "bounds{ " );
    AddBackF32( &line, bounds.p0.x, 1 );
    AddBackCStr( &line, ", " );
    AddBackF32( &line, bounds.p0.y, 1 );
    AddBackCStr( &line, ", " );
    AddBackF32( &line, bounds.p1.x, 1 );
    AddBackCStr( &line, ", " );
    AddBackF32( &line, bounds.p1.y, 1 );
    AddBackCStr( &line, " } m{ " );
    AddBackSInt( &line, m.x );
    AddBackCStr( &line, ", " );
    AddBackSInt( &line, m.y );
    AddBackCStr( &line, " } time_real{ " );
    AddBackF64( &line, timestep_realtime );
    AddBackCStr( &line, " } time_fixed{ " );
    AddBackF64( &line, timestep_fixed );
    AddBackCStr( &line, " }\n" );
    CmdAddString( app->debugmode.display, Cast( idx_t, line.mem ), line.len );
    Free( line );
#endif

    auto label_debugmode = SliceFromCStr( "debugmode" );
    DrawString(
      app->stream,
      font,
      bounds.p0,
      GetZ( zrange, applayer_t::txt ),
      bounds,
      rgba_notify_text,
      spaces_per_tab,
      ML( label_debugmode )
      );
    bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );

    TxtRender(
      app->debugmode.display,
      target_valid,
      app->stream,
      font,
      bounds,
      zrange,
      timestep_realtime,
      timestep_fixed,
      0,
      0,
      0,
      1
      );
  }
  else {

  #if PROF_ENABLED
    DrawProfiler(
      );
  #endif // PROF_ENABLED

    switch( app->active ) {

      case app_active_t::edit: {
        EditRender(
          app->edit,
          target_valid,
          app->stream,
          font,
          bounds,
          ZRange( zrange, applayer_t::edit ),
          timestep_realtime,
          timestep_fixed
          );
      } break;

      case app_active_t::cmd: {
        CmdRender(
          app->cmd,
          target_valid,
          app->stream,
          font,
          bounds,
          ZRange( zrange, applayer_t::edit ),
          timestep_realtime,
          timestep_fixed
          );
      } break;

      default: UnreachableCrash();
    }
  }


  // TODO: probably pull this notification stuff out to a function, and stop overlay-drawing.

  auto notifyui = &app->notifyui;
  if( notifyui->msgs.len ) {
    auto elem = notifyui->msgs.first;
    auto notify = &elem->value;

    target_valid = 0;

    if( notify->t.sum >= notification_lifetime ) {
      auto remove = elem;
      elem = elem->next;
      Rem( notifyui->msgs, remove );
      Reclaim( notifyui->msgs, remove );
    }
    if( elem ) {
      notify = &elem->value;

      Add( notify->t, Cast( f32, timestep ) );

      auto msg_p0 = _vec2( bounds.p0.x, bounds.p1.y - line_h );
      auto msg_p1 = bounds.p1;

      auto rampup = 0.05f * notification_lifetime; // TODO: propdb coeffs
      auto middle = 0.9f * notification_lifetime;
      auto rampdn = 0.05f * notification_lifetime;
      auto color_bkgd = rgba_notify_bkgd;
      auto color_text = rgba_notify_text;
      if( notify->t.sum < rampup ) {
        auto t = notify->t.sum / rampup;
        color_text.w = Smoothstep32( t );
        if( !notifyui->fadedin ) {
          color_bkgd.w = Smoothstep32( t );
        }
      }
      elif( notify->t.sum < rampup + middle ) {
        notifyui->fadedin = 1;
      }
      else {
        auto t = ( notify->t.sum - rampup - middle ) / rampdn;
        color_text.w = 1.0f - Smoothstep32( t );
        if( !notifyui->fadedin  ||  notifyui->msgs.len == 1 ) {
          color_bkgd.w = 1.0f - Smoothstep32( t );
        }
      }

      RenderQuad(
        app->stream,
        color_bkgd,
        msg_p0,
        msg_p1,
        bounds,
        GetZ( zrange, applayer_t::bkgd )
        );

      f32 count_w = 0.0f;
      if( notifyui->msgs.len > 1 ) {
        auto count = AllocFormattedString( " %d left ", notifyui->msgs.len - 1 );
        count_w = LayoutString( font, spaces_per_tab, ML( count ) );
        DrawString(
          app->stream,
          font,
          msg_p0,
          GetZ( zrange, applayer_t::txt ),
          bounds,
          color_text,
          spaces_per_tab,
          ML( count )
          );
        Free( count );
      }

      auto w = LayoutString( font, spaces_per_tab, ML( notify->msg ) );
      DrawString(
        app->stream,
        font,
        msg_p0 + _vec2( MAX( count_w, 0.5f * ( ( msg_p1.x - msg_p0.x ) - w ) ), 0.0f ),
        GetZ( zrange, applayer_t::txt ),
        bounds,
        color_text,
        spaces_per_tab,
        ML( notify->msg )
        );
    }
  }
  else {
    notifyui->fadedin = 0;
    Reset( notifyui->mem );
  }

  if( 0 )
  { // display timestep_realtime, as a way of tracking how long rendering takes.
    stack_nonresizeable_stack_t<u8, 64> tmp;
    CsFrom_f64( tmp.mem, Capacity( tmp ), &tmp.len, 1000 * timestep_realtime );
    auto w = LayoutString( font, spaces_per_tab, ML( tmp ) );
    DrawString(
      app->stream,
      font,
      _vec2( bounds.p1.x - w, bounds.p0.y ),
      GetZ( zrange, applayer_t::txt ),
      bounds,
      rgba_notify_text,
      spaces_per_tab,
      ML( tmp )
      );
  }

  if( app->stream.len ) {
#if OPENGL_INSTEAD_OF_SOFTWARE
    glUseProgram( app->shader.core.program );  glVerify();

#if 0
    app->stream.len = 0;
    OutputQuad(
      app->stream,
      _vec2<f32>( 0, Ceil32( offset ) ) + _vec2<f32>( 0, 0 ),
      _vec2<f32>( 0, Ceil32( offset ) ) + _vec2<f32>( 512, 108 ),
      1.0f,
      _vec2<f32>( 0, 0 ),
      _vec2<f32>( 1, 1 ),
      _vec4<f32>( 1, 1, 1, 1 )
      );
#endif

    // we invert y, since we layout from the top-left as the origin.
    // NOTE: it's important we use orig_dim, since we need ortho-size to match viewport-size for 1:1 pixel rendering.
    mat4x4r<f32> ndc_from_client;
    Ortho( &ndc_from_client, 0.0f, orig_dim.x, orig_dim.y, 0.0f, 0.0f, 1.0f );
    glUniformMatrix4fv( app->shader.loc_ndc_from_client, 1, 1, &ndc_from_client.row0.x );  glVerify();

    glUniform1i( app->shader.loc_tex_sampler, 0 );  glVerify();
    GlwBindTexture( 0, font.texid );

    glBindBuffer( GL_ARRAY_BUFFER, app->glstream );
    glBufferData( GL_ARRAY_BUFFER, app->stream.len * sizeof( f32 ), app->stream.mem, GL_STREAM_DRAW );  glVerify();
    glEnableVertexAttribArray( app->shader.attribloc_pos );  glVerify();
    glEnableVertexAttribArray( app->shader.attribloc_tccolor );  glVerify();
    glVertexAttribPointer( app->shader.attribloc_pos, 3, GL_FLOAT, 0, 6 * sizeof( f32 ), 0 );  glVerify();
    glVertexAttribPointer( app->shader.attribloc_tccolor, 3, GL_FLOAT, 0, 6 * sizeof( f32 ), Cast( void*, 3 * sizeof( f32 ) ) );  glVerify();

    auto vert_count = app->stream.len / 6;
    AssertCrash( app->stream.len % 6 == 0 );
    AssertCrash( vert_count <= MAX_s32 );
    glDrawArrays( GL_TRIANGLES, 0, Cast( s32, vert_count ) );  glVerify();

    glDisableVertexAttribArray( app->shader.attribloc_pos );  glVerify();
    glDisableVertexAttribArray( app->shader.attribloc_tccolor );  glVerify();

    //Log( "bytes sent to gpu: %llu", sizeof( f32 ) * app->stream.len );
#else // !OPENGL_INSTEAD_OF_SOFTWARE

#define RAND_NOISE 0

  #if RAND_NOISE
    rng_xorshift32_t rng;
    Init( rng, TimeTSC() );
  #endif


#define LOGFILLRATE 0

  #if LOGFILLRATE
    inc_stats_t stats_texalpha;
    Init( stats_texalpha );

    inc_stats_t stats_solidalpha;
    Init( stats_solidalpha );

    inc_stats_t stats_solidopaque;
    Init( stats_solidopaque );
  #endif


    //
    // PERF: frame stats on 4k TV, with main_test.asm open, 2 pages down:
    //
    // TotalFrame          12.918 ms
    //   ZeroBitmap           1.092 ms
    //   AppOnRender          8.841 ms
    //     tmp_DrawTexQuads     4.769 ms
    //     tmp_DrawPlainRects   3.388 ms
    //   BlitToScreen         2.985 ms
    //
    // TotalFrame          12.374 ms
    //   ZeroBitmap           0.972 ms
    //   AppOnRender          7.896 ms
    //     tmp_DrawTexQuads     4.993 ms
    //     tmp_DrawOpaqueQuads  1.344 ms
    //     tmp_DrawAlphaQuads   0.887 ms
    //   BlitToScreen         3.506 ms
    //
    // MakeTargetValid     10.050 ms
    //   ZeroBitmap           1.067 ms
    //   AppOnRender          5.869 ms
    //     tmp_DrawTexQuads     3.365 ms
    //     tmp_DrawOpaqueQuads  1.485 ms
    //     tmp_DrawAlphaQuads   0.543 ms
    //   BlitToScreen         3.114 ms
    //



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

  #if LOGFILLRATE
          auto t0 = TimeTSC();
  #endif

#if 0
          // PERF: this is ~0.7 cycles / pixel
          // turns out rep stosd is hard to beat.
          // movdqu ( _mm_storeu_si128 ), plus remainder rep stosd, is ~1 cycle / pixel, slower than this.
          // vmovdqu ( _mm256_storeu_si256 ), plus remainder rep stosd, also ~1 cycle / pixel.

          Fori( u32, j, 0, dstdim.y ) {
            auto dst = app->client.fullscreen_bitmap_argb + ( q0.y + j ) * app->client.dim.x + ( q0.x + 0 );
            Fori( u32, k, 0, dstdim.x ) {
              *dst++ = copy;
            }
          }

#elif 1
  #if RENDER_UNPACKED
          Fori( u32, j, 0, dstdim.y ) {
            auto dst = app->client.fullscreen_bitmap_argb_unpacked + ( q0.y + j ) * app->client.dim.x + ( q0.x + 0 );
            Fori( u32, k, 0, dstdim.x ) {
              *dst++ = color255_argb;
            }
          }
  #else
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
  #endif

#elif 0
          // PERF: this is ~0.6 cycles / pixel
          // vmovntdq ( _mm256_stream_si256 ), plus alignment, and remainder, is sometimes faster.
          // on a horizontal 4k monitor, it looks like our rows are long enough to pay for alignment setup.
          // but on a smaller monitor, the rep stos* wins out. so keep that one enabled for now.

          auto copy8 = _mm256_set1_epi32( *Cast( s32*, &copy ) );
          Fori( u32, j, 0, dstdim.y ) {
            auto dst = app->client.fullscreen_bitmap_argb + ( q0.y + j ) * app->client.dim.x + ( q0.x + 0 );
            auto ntoalign = ( RoundUpToMultipleOf32( Cast( idx_t, dst ) ) - Cast( idx_t, dst ) ) / 4;
            if( dstdim.x <= ntoalign ) {
              Fori( u32, k, 0, dstdim.x ) {
                *dst++ = copy;
              }
              continue;
            }
            Fori( u32, k, 0, ntoalign ) {
              *dst++ = copy;
            }
            auto dimx = dstdim.x - ntoalign;
            auto quo = dimx / 8;
            auto rem = dimx % 8;
            auto dst8 = Cast( __m256i*, dst );
            Fori( u32, k, 0, quo ) {
              _mm256_stream_si256( dst8, copy8 );
              ++dst8;
            }
            dst = Cast( u32*, dst8 );
            Fori( u32, k, 0, rem ) {
              *dst++ = copy;
            }
          }
#endif

  #if LOGFILLRATE
          auto dt = TimeTSC() - t0;
          auto count = dstdim.x * dstdim.y;
          AddMean( stats_solidopaque, Cast( f32, dt ) / count, count );
  #endif

        } else {
//          Prof( tmp_DrawAlphaQuads );
          // more complicated / slow alpha case.
  #if LOGFILLRATE
          auto t0 = TimeTSC();
  #endif


  #if RENDER_UNPACKED
          auto src_a = _mm_set1_ps( color.w );
          auto src_argb = color255_argb;
          Fori( u32, j, 0, dstdim.y ) {
            auto dst = app->client.fullscreen_bitmap_argb_unpacked + ( q0.y + j ) * app->client.dim.x + ( q0.x + 0 );
            Fori( u32, k, 0, dstdim.x ) {

              auto dst_argb = _mm_loadu_ps( Cast( f32*, dst ) );

              // src = color

              // out = dst + src_a * ( src - dst )
              auto diff_argb = _mm_sub_ps( src_argb, dst_argb );
              auto out_argb = _mm_fmadd_ps( src_a, diff_argb, dst_argb );

              _mm_storeu_ps( Cast( f32*, dst ), out_argb );
              ++dst;
            }
          }

  #else // !RENDER_UNPACKED

          Fori( u32, j, 0, dstdim.y ) {
            auto dst = app->client.fullscreen_bitmap_argb + ( q0.y + j ) * app->client.dim.x + ( q0.x + 0 );
            Fori( u32, k, 0, dstdim.x ) {

#if 0
              // PERF: this is ~17 cycles / pixel

              u8 dst_a = ( *dst >> 24 ) & 0xFF;
              u8 dst_r = ( *dst >> 16 ) & 0xFF;
              u8 dst_g = ( *dst >>  8 ) & 0xFF;
              u8 dst_b = ( *dst >>  0 ) & 0xFF;
              auto src_factor = color.w;
              auto dst_factor = 1 - src_factor;
  #if RAND_NOISE
              u32 val = Rand32( rng );
              u8 src = Cast( u8, val ) | ( 3 << 6 ); // rand from 192 to 255.
  #else
              u8 src = 255;
  #endif
              auto out_a = Cast( u8, Round_u32_from_f32( ( src           * src_factor + dst_a * dst_factor ) ) );
              auto out_r = Cast( u8, Round_u32_from_f32( ( src * color.x * src_factor + dst_r * dst_factor ) ) );
              auto out_g = Cast( u8, Round_u32_from_f32( ( src * color.y * src_factor + dst_g * dst_factor ) ) );
              auto out_b = Cast( u8, Round_u32_from_f32( ( src * color.z * src_factor + dst_b * dst_factor ) ) );
              *dst++ = ( out_a << 24 ) | ( out_r << 16 ) | ( out_g << 8 ) | out_b;

#elif 1
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
#endif
            }
          }

  #endif // !RENDER_UNPACKED

  #if LOGFILLRATE
          auto dt = TimeTSC() - t0;
          auto count = dstdim.x * dstdim.y;
          AddMean( stats_solidalpha, Cast( f32, dt ) / count, count );
  #endif
        }
      } else {
//        Prof( tmp_DrawTexQuads );
        // for now, assume quads with nonempty tex coords are exact-size copies onto dst.
        // that's true of all our text glyphs, which makes for faster code here.
        //
        // TODO: handle scaling up, or make a separate path for scaled-up tex, non-tex, etc. quads
        // we'll need separate paths for tris, lines anyways, so maybe do that.
  #if LOGFILLRATE
        auto t0 = TimeTSC();
  #endif


  #if RENDER_UNPACKED

        Fori( u32, j, 0, srcdim.y ) {
          auto dst = app->client.fullscreen_bitmap_argb_unpacked + ( q0.y + j ) * app->client.dim.x + ( q0.x + 0 );
          auto src = font.tex_mem + ( tc0.y + j ) * font.tex_dim.x + ( tc0.x + 0 );
          Fori( u32, k, 0, srcdim.x ) {

            // given color in [0, 1]
            // given src, dst in [0, 255]
            // src *= color
            // fac = src_a / 255
            // out = dst + fac * ( src - dst )

            auto dst_argb = _mm_loadu_ps( Cast( f32*, dst ) );

            // unpack src
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

            _mm_storeu_ps( Cast( f32*, dst ), out_argb );
            ++dst;
            ++src;
          }
        }

  #else // !RENDER_UNPACKED

        Fori( u32, j, 0, srcdim.y ) {
          auto dst = app->client.fullscreen_bitmap_argb + ( q0.y + j ) * app->client.dim.x + ( q0.x + 0 );
          auto src = font.tex_mem + ( tc0.y + j ) * font.tex_dim.x + ( tc0.x + 0 );
          Fori( u32, k, 0, srcdim.x ) {
#if 0
            // PERF: this is ~27 cycles / pixel

            u8 dst_a = ( *dst >> 24 ) & 0xFF;
            u8 dst_r = ( *dst >> 16 ) & 0xFF;
            u8 dst_g = ( *dst >>  8 ) & 0xFF;
            u8 dst_b = ( *dst >>  0 ) & 0xFF;
            u8 src_a = ( *src >> 24 ) & 0xFF;
            u8 src_r = ( *src >> 16 ) & 0xFF;
            u8 src_g = ( *src >>  8 ) & 0xFF;
            u8 src_b = ( *src >>  0 ) & 0xFF;
            auto src_factor = color.w * src_a / 255.0f;
            auto dst_factor = 1 - src_factor;
            auto out_a = Cast( u8, Round_u32_from_f32( ( src_a           * src_factor + dst_a * dst_factor ) ) );
            auto out_r = Cast( u8, Round_u32_from_f32( ( src_r * color.x * src_factor + dst_r * dst_factor ) ) );
            auto out_g = Cast( u8, Round_u32_from_f32( ( src_g * color.y * src_factor + dst_g * dst_factor ) ) );
            auto out_b = Cast( u8, Round_u32_from_f32( ( src_b * color.z * src_factor + dst_b * dst_factor ) ) );
            *dst++ = ( out_a << 24 ) | ( out_r << 16 ) | ( out_g << 8 ) | out_b;
            ++src;

#elif 1
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
#endif
          }
        }

  #endif // !RENDER_UNPACKED

  #if LOGFILLRATE
        auto dt = TimeTSC() - t0;
        auto count = srcdim.x * srcdim.y;
        AddMean( stats_texalpha, Cast( f32, dt ) / count, count );
  #endif
      }
    }

  #if LOGFILLRATE
    Log( "quad solid opaque ( cycles / %u px ): %f   total cycles: %f",
      stats_solidopaque.count,
      stats_solidopaque.mean.sum,
      stats_solidopaque.count * stats_solidopaque.mean.sum
      );
    Log( "quad solid alpha ( cycles / %u px ): %f   total cycles: %f",
      stats_solidalpha.count,
      stats_solidalpha.mean.sum,
      stats_solidalpha.count * stats_solidalpha.mean.sum
      );
    Log( "quad textured alpha ( cycles / %u px ): %f   total cycles: %f",
      stats_texalpha.count,
      stats_texalpha.mean.sum,
      stats_texalpha.count * stats_texalpha.mean.sum
      );
  #endif

  #if RENDER_UNPACKED
    Prof( tmp_PackFullscreenBitmap );

    Fori( u32, j, 0, app->client.dim.y ) {
      auto dst = app->client.fullscreen_bitmap_argb          + j * app->client.dim.x + 0;
      auto src = app->client.fullscreen_bitmap_argb_unpacked + j * app->client.dim.x + 0;
      Fori( u32, k, 0, app->client.dim.x ) {

        auto src_argb = _mm_loadu_ps( Cast( f32*, src ) );

        // put out in [0, 255]
        auto outi = _mm_cvtps_epi32( src_argb );
        auto out = _mm_packs_epi32( outi, _mm_set1_epi32( 0 ) );
        out = _mm_packus_epi16( out, _mm_set1_epi32( 0 ) );

        _mm_storeu_si32( dst, out ); // equivalent to: *dst = out.m128i_u32[0]
        ++dst;
        ++src;
      }
    }

    ProfClose( tmp_PackFullscreenBitmap );
  #endif

#endif // !OPENGL_INSTEAD_OF_SOFTWARE

    app->stream.len = 0;
  }
}


#define __AppCmd( name )   void ( name )( app_t* app )
typedef __AppCmd( *pfn_appcmd_t );


__AppCmd( SwitchEditFromCmd ) { app->active = app_active_t::edit; }
__AppCmd( SwitchCmdFromEdit ) { app->active = app_active_t::cmd ; }


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

  if( app->show_debugmode ) {
    auto keylocks = GlwKeylocks();
    stack_resizeable_cont_t<u8> line;
    Alloc( line, 1024 );
    AddBackCStr( &line, "bounds{ " );
    AddBackF32( &line, bounds.p0.x, 1 );
    AddBackCStr( &line, ", " );
    AddBackF32( &line, bounds.p0.y, 1 );
    AddBackCStr( &line, ", " );
    AddBackF32( &line, bounds.p1.x, 1 );
    AddBackCStr( &line, ", " );
    AddBackF32( &line, bounds.p1.y, 1 );
    AddBackCStr( &line, " } " );
    AddBackContents( &line, KeyStringFromGlw( key ) );
    AddBackCStr( &line, " " );
    AddBackContents( &line, SliceFromKeyEventType( type ) );
    AddBackCStr( &line, " locks_cns{ " );
    AddBackCStr( &line, keylocks.caps    ?  "1"  :  "0" );
    AddBackCStr( &line, keylocks.num     ?  "1"  :  "0" );
    AddBackCStr( &line, keylocks.scroll  ?  "1"  :  "0" );
    AddBackCStr( &line, " } alreadydn{ " );
    idx_t count = 0;
    For( i, 0, Cast( idx_t, glwkey_t::count ) ) {
      auto isdn = GlwKeyIsDown( Cast( glwkey_t, i ) );
      if( isdn ) {
        if( count ) {
          AddBackCStr( &line, ", " );
        }
        AddBackContents( &line, KeyStringFromGlw( Cast( glwkey_t, i ) ) );
        count += 1;
      }
    }
    AddBackCStr( &line, " }\n" );
    CmdAddString( app->debugmode.display, Cast( idx_t, line.mem ), line.len );
    Free( line );

    target_valid = 0;
  }

  // Global key event handling:
  bool ran_cmd = 0;
  switch( type ) {
    case glwkeyevent_t::dn:
    case glwkeyevent_t::repeat: {

#if 0
      glwkeybind_t tmp = { glwkey_t::fn_9 };
      if( GlwKeybind( key, tmp ) ) {
        LogUI( "Notification test, %d", glwkey_t::k );
      }
#endif

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

        case glwkey_t::fn_10: {
          app->show_debugmode = !app->show_debugmode;
          target_valid = 0;
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
    switch( app->active ) {

      case app_active_t::edit: {
        if( app->kb_command ) {
          switch( type ) {
            case glwkeyevent_t::dn: {
              app_cmdmap_t table[] = {
                _appcmdmap( GetPropFromDb( glwkeybind_t, keybind_app_switch_cmd_from_edit ), SwitchCmdFromEdit ),
              };
              ForEach( entry, table ) {
                if( GlwKeybind( key, entry.keybind ) ) {
                  entry.fn( app );
                  target_valid = 0;
                  ran_cmd = 1;
                }
              }
            } break;

            case glwkeyevent_t::up:
            case glwkeyevent_t::repeat: {
            } break;

            default: UnreachableCrash();
          }
        }
        if( !ran_cmd ) {
          auto keylocks = GlwKeylocks();
          EditControlKeyboard(
            app->edit,
            app->kb_command,
            target_valid,
            ran_cmd,
            type,
            key,
            keylocks
            );
        }
      } break;

      case app_active_t::cmd: {
        if( app->kb_command ) {
          switch( type ) {
            case glwkeyevent_t::dn: {
              app_cmdmap_t table[] = {
                _appcmdmap( GetPropFromDb( glwkeybind_t, keybind_app_switch_edit_from_cmd ), SwitchEditFromCmd ),
              };
              ForEach( entry, table ) {
                if( GlwKeybind( key, entry.keybind ) ) {
                  entry.fn( app );
                  target_valid = 0;
                  ran_cmd = 1;
                }
              }
            } break;

            case glwkeyevent_t::up:
            case glwkeyevent_t::repeat: {
            } break;

            default: UnreachableCrash();
          }
        }
        if( !ran_cmd ) {
          auto keylocks = GlwKeylocks();
          CmdControlKeyboard(
            app->cmd,
            app->kb_command,
            target_valid,
            ran_cmd,
            type,
            key,
            keylocks
            );
        }
      } break;

      default: UnreachableCrash();
    } // end switch( app->active )
  }
}



__OnMouseEvent( AppOnMouseEvent )
{
  ProfFunc();

#if 0
  printf( "mouse " );
  switch( type ) {
    case glwmouseevent_t::dn: printf( "dn " ); break;
    case glwmouseevent_t::up: printf( "up " ); break;
    case glwmouseevent_t::move: printf( "move " ); break;
    case glwmouseevent_t::wheelmove: printf( "wheelmove " ); break;
  }
  printf( "pos(%d,%d) ", m.x, m.y );
  printf( "delta(%d,%d) ", raw_delta.x, raw_delta.y );
  printf( "dwheel(%d) ", dwheel );
  printf( "\n" );
#endif

  auto app = Cast( app_t*, misc );
  auto mp = _vec2( Cast( f32, m.x ), Cast( f32, m.y ) );

  auto& font = GetFont( app, Cast( enum_t, fontid_t::normal ) );

  if( app->show_debugmode ) {
    stack_resizeable_cont_t<u8> line;
    Alloc( line, 1024 );
    AddBackCStr( &line, "bounds{ " );
    AddBackF32( &line, bounds.p0.x, 1 );
    AddBackCStr( &line, ", " );
    AddBackF32( &line, bounds.p0.y, 1 );
    AddBackCStr( &line, ", " );
    AddBackF32( &line, bounds.p1.x, 1 );
    AddBackCStr( &line, ", " );
    AddBackF32( &line, bounds.p1.y, 1 );
    AddBackCStr( &line, " } " );
    AddBackContents( &line, SliceFromMouseBtn( btn ) );
    AddBackCStr( &line, " " );
    AddBackContents( &line, SliceFromMouseEventType( type ) );
    AddBackCStr( &line, " m{ " );
    AddBackSInt( &line, m.x );
    AddBackCStr( &line, ", " );
    AddBackSInt( &line, m.y );
    AddBackCStr( &line, " } mdel{ " );
    AddBackSInt( &line, raw_delta.x );
    AddBackCStr( &line, ", " );
    AddBackSInt( &line, raw_delta.y );
    AddBackCStr( &line, " } dwheel{ " );
    AddBackSInt( &line, dwheel );
    AddBackCStr( &line, " } btnalreadydn{ " );
    idx_t count = 0;
    For( i, 0, Cast( idx_t, glwmousebtn_t::count ) ) {
      auto isdn = GlwMouseBtnIsDown( Cast( glwmousebtn_t, i ) );
      if( isdn ) {
        if( count ) {
          AddBackCStr( &line, ", " );
        }
        AddBackContents( &line, SliceFromMouseBtn( Cast( glwmousebtn_t, i ) ) );
        count += 1;
      }
    }
    AddBackCStr( &line, " } keyalreadydn{ " );
    count = 0;
    For( i, 0, Cast( idx_t, glwkey_t::count ) ) {
      auto isdn = GlwKeyIsDown( Cast( glwkey_t, i ) );
      if( isdn ) {
        if( count ) {
          AddBackCStr( &line, ", " );
        }
        AddBackContents( &line, KeyStringFromGlw( Cast( glwkey_t, i ) ) );
        count += 1;
      }
    }
    AddBackCStr( &line, " }\n" );
    CmdAddString( app->debugmode.display, Cast( idx_t, line.mem ), line.len );
    Free( line );

    target_valid = 0;
  }

#if PROF_ENABLED
  // TODO: detect the case where we should stop displaying the tooltip.
  //   probably need a bool showing_tooltip or something for that stored in app_t.
  if( app->prof_renderedscopes.len ) {
    auto closest = FindTooltip( app, mp );
    if( closest ) {
      target_valid = 0;
    }
  }
#endif // PROF_ENABLED

  switch( app->active ) {

    case app_active_t::edit: {
      cursortype = glwcursortype_t::arrow;

      EditControlMouse(
        app->edit,
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
    } break;

    case app_active_t::cmd: {
      cursortype = glwcursortype_t::arrow;

      CmdControlMouse(
        app->cmd,
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

  if( app->show_debugmode ) {
    stack_resizeable_cont_t<u8> line;
    Alloc( line, 1024 );
    AddBackCStr( &line, "dim{ " );
    AddBackUInt( &line, dim.x );
    AddBackCStr( &line, ", " );
    AddBackUInt( &line, dim.y );
    AddBackCStr( &line, " } " );
    if( type & glwwindowevent_resize ) {
      AddBackCStr( &line, "resize " );
    }
    if( type & glwwindowevent_dpichange ) {
      AddBackCStr( &line, "dpichange " );
    }
    if( type & glwwindowevent_focuschange ) {
      AddBackCStr( &line, "focuschange " );
    }
    AddBackCStr( &line, "dpi{ " );
    AddBackUInt( &line, dpi );
    AddBackCStr( &line, " } focused{ " );
    AddBackUInt( &line, Cast( u32, focused ) );
    AddBackCStr( &line, " }\n" );
    CmdAddString( app->debugmode.display, Cast( idx_t, line.mem ), line.len );
    Free( line );

    target_valid = 0;
  }

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

  switch( app->active ) {
    case app_active_t::edit: {
      EditWindowEvent(
        app->edit,
        type,
        dim,
        dpi,
        focused,
        target_valid
        );
    } break;

    case app_active_t::cmd: {
    } break;

    default: UnreachableCrash();
  }
}

int
Main( stack_resizeable_cont_t<slice_t>& args )
{
//  PinThreadToOneCore();

  auto has_filename = args.len > 0;
  auto has_lineno = args.len > 1;

  auto app = &g_app;
  AppInit( app );

  if( !has_filename ) {
    // if we launch in directory mode, aka fileopener, refresh the fileopener now.
    // otherwise we're launching in file mode, and we don't want to trigger a huge
    // recursive directory walk.
    CmdFileopenerRefresh( app->edit );
  }
  else {
    auto filename = args.mem[0];

    u32 lineno = 0;
    if( has_lineno ) {
      auto gotoline = args.mem[1];
      bool valid = 1;
      For( i, 0, gotoline.len ) {
        if( !AsciiIsNumber( gotoline.mem[i] ) ) {
          valid = 0;
          break;
        }
      }
      lineno = CsTo_u32( ML( gotoline ) );
      if( lineno ) {
        lineno -= 1;
      }
    }

#if USE_FILEMAPPED_OPEN
    auto file = FileOpenMappedExistingReadShareRead( ML( filename ) );
#else
    auto file = FileOpen( ML( filename ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
#endif
    if( file.loaded ) {
      edittxtopen_t* open = 0;
      bool opened_existing = 0;
      EditOpen( app->edit, file, &open, &opened_existing );
      AssertCrash( open );
      EditSetActiveTxt( app->edit, open );
      CmdMode_editfile_from_fileopener( app->edit );
      if( has_lineno ) {
        CmdCursorGotoline( open->txt, lineno );
      }
    } else {
      u8 tmp[1024];
      CstrCopy( tmp, filename.mem, MIN( filename.len, _countof( tmp ) - 1 ) );
      MessageBoxA(
        0,
        Cast( LPCSTR, tmp ),
        "Couldn't open file!",
        0
        );
      return -1;
    }
#if !USE_FILEMAPPED_OPEN
    FileFree( file );
#endif
  }

  auto fontsize_normal = GetPropFromDb( f32, f32_fontsize_normal );
  auto filename_font_normal = GetPropFromDb( slice_t, string_filename_font_normal );

  LoadFont(
    app,
    Cast( idx_t, fontid_t::normal ),
    ML( filename_font_normal ),
    fontsize_normal * Cast( f32, app->client.dpi )
    );

#if OPENGL_INSTEAD_OF_SOFTWARE
  GlwSetSwapInterval( app->client, 0 );
#else // !OPENGL_INSTEAD_OF_SOFTWARE
  // TODO_SOFTWARE_RENDER
#endif // !OPENGL_INSTEAD_OF_SOFTWARE

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




// OVERVIEW:
#if 0

  text editor
    keyboard type
    mouse cursor/select
    textual undo/redo
    cursor/select undo/redo
    multi-line cursor/select
    textual copy/paste
    smooth scrolling
    find next/prev
    find and replace next/prev

  directory viewer
    see files/dirs in current directory
    delete files/dirs
    create files/dirs
    rename files/dirs

  command prompt
    execute programs from textual commands in current directory.
    change current directory.
    show textual output of program executions.
    copy/paste of textual commands and textual output.

  quick swapping between these modes
    text editor -> directory viewer
    text editor -> command prompt
    directory viewer -> text editor
      open file for editing
    directory viewer -> command prompt
      propogate the current directory
    command prompt -> text editor
      open file for editing
    command prompt -> directory viewer

  hotloaded options
    colors
    keybinds
    scrolling

#endif



// TODO LIST:
#if 0

  bad scroll bug where sometimes the cursor jumps off-screen below, e.g. find next.
    it should auto scroll so it's within the nice range, but we don't.
    we should probably add some scroll debug output to make this easier to figure out.
    test coverage would also be super cool, we don't have any rendering tests yet.

  // TODO: convert txt.linespans/charspans/etc. to u32 sizes?

  update profiler to record absolute time in prof_elem_t
  then make a per-thread timeline view.
    maybe do CSV output as a separate thing, and then a separate viewer program?
    or maybe start with the realtime viewer, in app?

  make the cmd_t Execute call fully async, so we get nice console updates from long-running programs.
    // TODO: CmdExecute stalls the main thread, and we don't redraw during all that.

  unify listview_t with txt_t scrolling/viewing.
    // TODO: unify this scrolling code with listview_t

  try the pagetree in buf2
    #define LB   1

  configurable exe string on file save.
    we already have this on file open, but that's actually the wrong place for my work usage.
    need to mark file as edited on save, esp. when i've reverted the file while it's open, and edited again.

  fileopener scroll is unstable:
    on first te.exe open, scroll_start is 0, but after any makecurvis, we scroll up.
    txt2 scroll seems to start at max negative scroll, so we should do that too.

  add a vertical scrollbar for findinfiles, and maybe switchopened
    copy/share fileopener's scrolling code.

  add a 'currently available commands and keybinds' view, on F1-hold or something.

  consider adding ... prefix/suffix in the foundinfile_t line sample if it's too long, since it doesn't scroll.

  prevent edit_t double-opening of a file.
    repro: "te src/main.cpp", then use fileopener to open the same file.
    maybe normalizing to full filepath
      which can also strip '..'s, '.'s, etc.
    maybe detect double-open by file handle comparisons of some kind, and abort open?

  hotload config file

  when we move a txtopen across horz, find the mru txtopen on the source side, so we don't have a txtopen dupe.
    maybe tag each mru entry with a 'left or right' flag, so we can do this.
    or, just pick the next mru, no matter which side it's on, and stick it on the source side.
    alternately:
  real multi-views of a single file. right now we just draw the same thing twice.
    probably start with forceinbounds of all the other views
    probably want line tracking to make that better

  add a new cursel / view mode that hides all indented and trivial lines.
    the idea being visual compression of the file.
    probably have an option for showing indent=1 too.
    maybe add an is_nontrivial_toplevel flag to buf_t unordered_lines
    then have a custom txt_t mode that ignores all other lines.
    if we want full s/m modes in this custom view, do we need to change selection to fundamentally be a list of contiguous ranges?
    right now our selection model is just one contiguous range; c to s, or the reverse, depending on directionality.
    i'm thinking of a regular s selection in the custom view, but we want to skip trivial lines.
    one way of doing that is to change core selection model to a list of nonoverlapping contiguous ranges.
    this could be a nice way of simplifying the m selection code...
    we might need some extra code for handling auto-space-insertion at eols in m mode AddString, for example.
    so maybe this wouldn't eliminate much m code.
    i also imagine that updating the list of ranges in each command isn't always trivial.
    maybe we just do a view mode, not an editing mode?
    should we allow cursel movements?

  horizontal scrollbars

  if the program detects a fatal error, save any unsaved work for recovery.
    do we want a special recovery mode that asks, or just always stomp the files?
    we do externalmerge stuff, but we do a terrible job of showing the diffs.

  autosave!
    do it on window defocus.
    do it on some idle interval.
    make sure not to stomp on external changes.

  in txt_t, add line wrapping.
    be careful not to split words, unless the word is larger than a line.
    make sure this is a txt_t option.
    make sure the lineno is drawn only once.
    problems:
      mouse input remapping.
      cursor behavior might need to change, ie we might want CursorU to behave like normal even though it's staying inside the same line.
    better yet, have a CmdWrapAllLines, and CmdUnwrapAllLines. then we don't have to worry about changing txt much.
      but, you can't unwrap without significant contextual information!
    best:
      just do line wrapping for rendering, but not for CursorU, ScrollU, etc.
      might make scrolling feel a little weird, but that's fine. most things shouldn't be wrapped.
      maybe revisit scrolling later if it's really not good.
    consider auto-indenting to one higher leading whitespace level.
    probably also render an arrow or something to show we've line wrapped.
    maybe a bkgd rect like the cursorline to show which lines are wrapped?
      e.g. alternating line bkgd colors?



  move clipboard handling to os interface.

  tests for filesys.

  look at the ship asm and fix issues.

  combine keybinds and cmdmap so we don't have to make all these lists on the stack on every keyevent.
    we can't do this easily if we need to pass in params to the cmd :(

  add debug macros that keep track of begin/end call pairs, and check for failure on program exit.



  in fileopener_t, finish adding undo/redo.

  add a progress indicator for dirmode_t::search, since it can take a long time.

  add unsaved indicator to dirmode_t::search view.

  add copyfile + copydir.

  in fileopener_t, add a proper selection set!
    we need a hashset that prevents duplicates.

  in fileopener_t, add Back.
    this requires an UndoStack with units of ( fsobj_t cwd, size_t cursor ).

  in fileopener_t, add sorting and sort states.

  in fileopener_t, add cursor wraparound, given some option is set to 1.

  in fileopener_t, add directory size computation.
    this probably requires a ( fullpathdir, lastwritetime ) cache that we check/update as necessary with a bkgd thread.
    we need a way of queueing a new fullpathdir for the bkgd thread, and a way of getting that result.



  get rid of txt_t cmd*jump*; can just use cmd* with a default argument of 1.

  change txt_t's cs_undo_t strategy to use a history instead of double-stacks.

  in txt_t, move the basic text rendering into buf_t, so we have cleaner separation.
    eliminate the split between single-line and multi-line rendering.

  in txt_t, make CmdFindCursorWordL/R loop around back to start if it doesn't move.

  in txt_t, immediately after a dblclick to open a file from fileopener_t, the click+drag selection kicks in.
    we don't have enough state to distinguish these two cases, so we need to store an extra timer that disables click+drag selection after a dblclick was detected. ugh.

  in txt_t, add a px_between_lines option.

  in txt_t, fix SelectCopy so that it doesn't assume buf has CRLF. it has to add explicitly!

  in txt_t, on SaveToFile, just use the EOL type found on loading. so set a flag on Load, and use that same type on SaveToFile.

  in txt_t, have multicursor CmdSelectHome alternate between x=0 and x=x0 thru spaces.

  in txt_t, fix CmdTabL so that it can handle mixed tabs and spaces and do the right thing.
    first pass: verify we can remove 1 tab / N spaces from each ms line.
    second pass: remove 1 tab / N spaces from each ms line.



  in cmd_t, make the top_frac line adjustable by mouse.

  in cmd_t, draw old command executions in a darker color, so it's easy to spot the last and current command.



  figure out why we have to SetClipboardData with CF_OEMTEXT, and GetClipboardData with CF_TEXT. other configurations don't seem to work!

  robust handling of options file paths.
    right now, paths are hardcoded, which is bad.  we need relative paths.
    also, if relative paths don't exist, create them.
    same for the options files--if they don't exist, fill in defaults.
    this means we should just have a options_default/ directory that we can copy from if needed.



  make buf_t + txt_t undo optional.

  generalize fileopener_t so it's a generic table that we can fill with whatever we want.
    not sure that's worth it; we've got some differences between fileopener, switchopened, findinfiles.

  in txt_t, add advanced skip_l, skip_r options.
    camel-case.
    underscores.
    non-alphabet chars ( eg '.', '->', '//', etc ).

  in txt_t, add auto-indenting.
    do }-matching, so it scans back through for a matching { and indents appropriately. have an enable-bool.
    or have a specific command to run it given a selection.

  in txt_t, add support for non-ascii files ( utf8, etc ).

  in txt_t, add language-specific text coloring.
    define a keyword with a str. all text matching str ( word boundaries ) is set to a specified color.
    define a text range with str start, str end. all text in range is set to a specified color.
    need some way to specify name mappings like typedef, define, struct, etc.
    also need to make options for coloring the background, not just text.
    change the word to keyword matching to use a hashtable.
      we already do string hashing in hotloader.cpp; reuse that.

  in txt_t, add advanced cursel movement logic for non-monospace
    think about cursel up/down; we'd want to choose a vertical line where the cursel is, and minimize the distance from that line, instead of just a c_inline scheme.
    this also applies to lines with tabs.

  in txt_t, add guidelines at user-defined ch spacings.

  in txt_t, try the explicit ODE soln for smooth scrolling.
    this will let us do arbitrary strength springs, which isn't true right now.
    we've maxed out what we can do with the spring eqns in terms of evaluation stability.
    idea is:
      whenever we change scroll_target, store the new ODE soln, reset t to zero.
      every time we render, advance t by the timestep, and let scroll_start = equation(t).
    ODE soln:
      given
      x'' + 2 A x' + B x = 0
      one general soln is:
      x(t) = a exp( -n ) t^n exp( -A t ) exp( +-sqrt( n + t^2 ( A^2 - B ) ) )
      note the +- which causes multiple solns.
      note the sqrt of a potentially negative number, leading to the complex sine wave solutions.
      note "a" and "n" are a arbitrary constants. we usually take n = 0, leaving:
      x(t) = a exp( -A t ) exp( +- sqrt( t^2 ( A^2 - B ) ) )
    since this is for scrolling, we don't want sine wave solutions really.
    so we want to assume the sqrt argument is nonnegative.



  !!!!!!!!!NOTE!!!!!!!!!
      SVN REV 530 is the last revision before the content_ptr_t rewrite!
      rollback to that for diffing, if need be.
  !!!!!!!!!NOTE!!!!!!!!!


  reduce heap allocations per-tick.

    change stack_resizeable_cont_t to delay-alloc until add/reserve
      nuances here around not adding more code to arrays we always add to.
      maybe add a lazyarray_t ?

    change to keep this array alive longer than just TxtRender?

        stack_resizeable_cont_t<wordspan_t> spans;
        Alloc( spans, 64 );

      proj64d.exe!MemHeapAllocBytes(unsigned __int64 nbytes) Line 1135	C++
      proj64d.exe!Alloc<wordspan_t>(stack_resizeable_cont_t<wordspan_t> & array, unsigned __int64 nelems) Line 39	C++
      proj64d.exe!TxtRender(txt_t & txt, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, bool draw_cursor, bool draw_cursorline, bool draw_cursorwordmatch, bool allow_scrollbar) Line 4481	C++
      proj64d.exe!_RenderTxt(txt_t & txt, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, double timestep_realtime, double timestep_fixed, bool draw_cursor, bool draw_cursorline, bool draw_cursorwordmatch, bool allow_scrollbar) Line 2677	C++
      proj64d.exe!_RenderBothSides(edit_t & edit, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, double timestep_realtime, double timestep_fixed) Line 2846	C++
      proj64d.exe!EditRender(edit_t & edit, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, double timestep_realtime, double timestep_fixed) Line 3311	C++
      proj64d.exe!AppOnRender(void * misc, vec2<float> origin, vec2<float> dim, double timestep_realtime, double timestep_fixed, bool & target_valid) Line 293	C++
      proj64d.exe!_Render(glwclient_t & client) Line 1378	C++
      proj64d.exe!GlwMainLoop(glwclient_t & client) Line 2375	C++
      proj64d.exe!Main(stack_resizeable_cont_t<slice_t> & args) Line 1198	C++
      proj64d.exe!WinMain(HINSTANCE__ * prog_inst, HINSTANCE__ * prog_inst_prev, char * prog_cmd_line, int prog_cmd_show) Line 1262	C++


    change this to use a max-size stack array:

        For( i, 0, c_lines ) {
          auto bol = CursorStopAtNewlineL( txt.buf, uniform_pos, 0 );
          idx_t len;
          auto bol_space = CursorSkipSpacetabR( txt.buf, bol, &len );
          if( len ) {
            auto tmp = AllocContents( txt.buf, bol, len );

      proj64d.exe!AllocContents(buf_t & buf, content_ptr_t ptr, unsigned __int64 len) Line 3213	C++
      proj64d.exe!TxtLoad(txt_t & txt, file_t & file) Line 257	C++
      proj64d.exe!EditOpen(edit_t & edit, file_t & file, edittxtopen_t * * opened, bool * opened_existing) Line 635	C++
      proj64d.exe!EditOpenAndSetActiveTxt(edit_t & edit, file_t & file) Line 652	C++
      proj64d.exe!FileopenerOpenRow(edit_t & edit, fileopener_row_t * row) Line 1110	C++
      proj64d.exe!CmdFileopenerChoose(edit_t & edit, unsigned __int64 misc) Line 1160	C++
      proj64d.exe!ExecuteCmdMap(edit_t & edit, edit_cmdmap_t * table, unsigned __int64 table_len, glwkey_t key, bool * alreadydn, bool & target_valid, bool & ran_cmd) Line 4409	C++
      proj64d.exe!EditControlKeyboard(edit_t & edit, bool kb_command, bool & target_valid, bool & ran_cmd, glwkeyevent_t type, glwkey_t key, glwkeylocks_t & keylocks, bool * alreadydn) Line 4647	C++
      proj64d.exe!AppOnKeyEvent(void * misc, vec2<float> origin, vec2<float> dim, bool & fullscreen, bool & target_valid, glwkeyevent_t type, glwkey_t key, glwkeylocks_t keylocks, bool * alreadydn) Line 980	C++
      proj64d.exe!WindowProc(HWND__ * hwnd, unsigned int msg, unsigned __int64 wp, __int64 lp) Line 1607	C++


        Prof( TxtUpdateScrollingHorizontal );

        auto ln_start = CursorStopAtNewlineL( txt.buf, txt.c, 0 );
        auto temp = AllocContents( txt.buf, ln_start, txt.c );
        auto offset = LayoutString( font, spaces_per_tab, ML( temp ) );

      proj64d.exe!AllocContents(buf_t & buf, content_ptr_t start, content_ptr_t end) Line 3204	C++
      proj64d.exe!TxtUpdateScrolling(txt_t & txt, font_t & font, vec2<float> origin, vec2<float> dim, double timestep_realtime, double timestep_fixed) Line 3854	C++
      proj64d.exe!_RenderTxt(txt_t & txt, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, double timestep_realtime, double timestep_fixed, bool draw_cursor, bool draw_cursorline, bool draw_cursorwordmatch, bool allow_scrollbar) Line 2660	C++
      proj64d.exe!_RenderBothSides(edit_t & edit, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, double timestep_realtime, double timestep_fixed) Line 2846	C++
      proj64d.exe!EditRender(edit_t & edit, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, double timestep_realtime, double timestep_fixed) Line 3311	C++
      proj64d.exe!AppOnRender(void * misc, vec2<float> origin, vec2<float> dim, double timestep_realtime, double timestep_fixed, bool & target_valid) Line 293	C++
      proj64d.exe!_Render(glwclient_t & client) Line 1378	C++
      proj64d.exe!GlwMainLoop(glwclient_t & client) Line 2375	C++
      proj64d.exe!Main(stack_resizeable_cont_t<slice_t> & args) Line 1198	C++
      proj64d.exe!WinMain(HINSTANCE__ * prog_inst, HINSTANCE__ * prog_inst_prev, char * prog_cmd_line, int prog_cmd_show) Line 1262	C++


        auto word_l = CursorStopAtNonWordCharL( txt.buf, txt.c, 0 );
        auto word_r = CursorStopAtNonWordCharR( txt.buf, txt.c, 0 );
        auto word = AllocContents( txt.buf, word_l, word_r );

      proj64d.exe!AllocContents(buf_t & buf, content_ptr_t start, content_ptr_t end) Line 3204	C++
      proj64d.exe!TxtRender(txt_t & txt, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, bool draw_cursor, bool draw_cursorline, bool draw_cursorwordmatch, bool allow_scrollbar) Line 4424	C++
      proj64d.exe!_RenderTxt(txt_t & txt, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, double timestep_realtime, double timestep_fixed, bool draw_cursor, bool draw_cursorline, bool draw_cursorwordmatch, bool allow_scrollbar) Line 2677	C++
      proj64d.exe!_RenderBothSides(edit_t & edit, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, double timestep_realtime, double timestep_fixed) Line 2846	C++
      proj64d.exe!EditRender(edit_t & edit, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, double timestep_realtime, double timestep_fixed) Line 3311	C++
      proj64d.exe!AppOnRender(void * misc, vec2<float> origin, vec2<float> dim, double timestep_realtime, double timestep_fixed, bool & target_valid) Line 293	C++
      proj64d.exe!_Render(glwclient_t & client) Line 1378	C++
      proj64d.exe!GlwMainLoop(glwclient_t & client) Line 2375	C++
      proj64d.exe!Main(stack_resizeable_cont_t<slice_t> & args) Line 1198	C++
      proj64d.exe!WinMain(HINSTANCE__ * prog_inst, HINSTANCE__ * prog_inst_prev, char * prog_cmd_line, int prog_cmd_show) Line 1262	C++


        auto span = spans.mem + i;
        auto span_len = CountBytesBetween( txt.buf, span->l, span->r );
        AssertWarn( span_len );
        auto tmp = MemHeapAlloc( u8, span_len );
        Contents( txt.buf, span->l, tmp, span_len );

      proj64d.exe!MemHeapAllocBytes(unsigned __int64 nbytes) Line 1135	C++
      proj64d.exe!TxtRender(txt_t & txt, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, bool draw_cursor, bool draw_cursorline, bool draw_cursorwordmatch, bool allow_scrollbar) Line 4529	C++
      proj64d.exe!_RenderTxt(txt_t & txt, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, double timestep_realtime, double timestep_fixed, bool draw_cursor, bool draw_cursorline, bool draw_cursorwordmatch, bool allow_scrollbar) Line 2677	C++
      proj64d.exe!_RenderBothSides(edit_t & edit, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, double timestep_realtime, double timestep_fixed) Line 2846	C++
      proj64d.exe!EditRender(edit_t & edit, bool & target_valid, stack_resizeable_cont_t<float> & stream, font_t & font, vec2<float> origin, vec2<float> dim, vec2<float> zrange, double timestep_realtime, double timestep_fixed) Line 3311	C++
      proj64d.exe!AppOnRender(void * misc, vec2<float> origin, vec2<float> dim, double timestep_realtime, double timestep_fixed, bool & target_valid) Line 293	C++
      proj64d.exe!_Render(glwclient_t & client) Line 1378	C++
      proj64d.exe!GlwMainLoop(glwclient_t & client) Line 2375	C++
      proj64d.exe!Main(stack_resizeable_cont_t<slice_t> & args) Line 1198	C++
      proj64d.exe!WinMain(HINSTANCE__ * prog_inst, HINSTANCE__ * prog_inst_prev, char * prog_cmd_line, int prog_cmd_show) Line 1262	C++



    hundreds of 192 byte allocations via:
      proj64d.exe!MemHeapAllocBytes(unsigned __int64 nbytes) Line 1140	C++
      proj64d.exe!Alloc<slice_t>(stack_resizeable_cont_t<slice_t> & array, unsigned __int64 nelems) Line 39	C++
      proj64d.exe!Init(statement_t & stm) Line 201	C++
      proj64d.exe!Parse(ast_t & ast, stack_resizeable_cont_t<token_t> & tokens, slice_t & src) Line 271	C++
      proj64d.exe!_LoadFromMem(propdb_t & db, slice_t & mem) Line 340	C++
      proj64d.exe!_Init(propdb_t & db) Line 631	C++
      proj64d.exe!_InitPropdb() Line 672	C++
      proj64d.exe!GetProp(unsigned char * name) Line 680	C++
      proj64d.exe!TxtLoadEmpty(txt_t & txt) Line 199	C++
      proj64d.exe!EditInit(edit_t & edit) Line 1523	C++
      proj64d.exe!AppInit(app_t * app) Line 125	C++
      proj64d.exe!Main(stack_resizeable_cont_t<slice_t> & args) Line 1148	C++
      proj64d.exe!WinMain(HINSTANCE__ * prog_inst, HINSTANCE__ * prog_inst_prev, char * prog_cmd_line, int prog_cmd_show) Line 1262	C++

    hundreds of 164 byte allocations via:
      proj64d.exe!MemHeapAllocBytes(unsigned __int64 nbytes) Line 1136	C++
      proj64d.exe!Alloc<unsigned char>(stack_resizeable_cont_t<unsigned char> & array, unsigned __int64 nelems) Line 39	C++
      proj64d.exe!Init(num_t & num, unsigned __int64 size) Line 1072	C++
      proj64d.exe!CsToFloat32(unsigned char * src, unsigned __int64 src_len, float & dst) Line 1509	C++
      proj64d.exe!_LoadFromMem(propdb_t & db, slice_t & mem) Line 419	C++
      proj64d.exe!_Init(propdb_t & db) Line 631	C++
      proj64d.exe!_InitPropdb() Line 672	C++
      proj64d.exe!GetProp(unsigned char * name) Line 680	C++
      proj64d.exe!TxtLoadEmpty(txt_t & txt) Line 199	C++
      proj64d.exe!EditInit(edit_t & edit) Line 1523	C++
      proj64d.exe!AppInit(app_t * app) Line 125	C++
      proj64d.exe!Main(stack_resizeable_cont_t<slice_t> & args) Line 1148	C++
      proj64d.exe!WinMain(HINSTANCE__ * prog_inst, HINSTANCE__ * prog_inst_prev, char * prog_cmd_line, int prog_cmd_show) Line 1262	C++

    thousands of allocations via stb truetype, e.g.
      proj64d.exe!MemHeapAllocBytes(unsigned __int64 nbytes) Line 1140	C++
      proj64d.exe!stbtt__hheap_alloc(stbtt__hheap * hh, unsigned __int64 size, void * userdata) Line 2281	C++
      proj64d.exe!stbtt__new_active(stbtt__hheap * hh, stbtt__edge * e, int off_x, float start_point, void * userdata) Line 2361	C++
      proj64d.exe!stbtt__rasterize_sorted_edges(stbtt__bitmap * result, stbtt__edge * e, int n, int vsubsample, int off_x, int off_y, void * userdata) Line 2775	C++
      proj64d.exe!stbtt__rasterize(stbtt__bitmap * result, stbtt__point * pts, int * wcount, int windings, float scale_x, float scale_y, float shift_x, float shift_y, int off_x, int off_y, int invert, void * userdata) Line 2971	C++
      proj64d.exe!stbtt_Rasterize(stbtt__bitmap * result, float flatness_in_pixels, stbtt_vertex * vertices, int num_verts, float scale_x, float scale_y, float shift_x, float shift_y, int x_off, int y_off, int invert, void * userdata) Line 3129	C++
      proj64d.exe!stbtt_GetGlyphBitmapSubpixel(const stbtt_fontinfo * info, float scale_x, float scale_y, float shift_x, float shift_y, int glyph, int * width, int * height, int * xoff, int * yoff) Line 3175	C++
      proj64d.exe!stbtt_GetCodepointBitmapSubpixel(const stbtt_fontinfo * info, float scale_x, float scale_y, float shift_x, float shift_y, int codepoint, int * width, int * height, int * xoff, int * yoff) Line 3211	C++
      proj64d.exe!FontLoadGlyphImage(font_t & font, unsigned int codept, vec2<float> & dimf, vec2<float> & offsetf) Line 500	C++
      proj64d.exe!FontLoadAscii(font_t & font) Line 735	C++
      proj64d.exe!LoadFont(app_t * app, unsigned int fontid, unsigned char * filename_ttf, unsigned __int64 filename_ttf_len, float char_h) Line 215	C++
      proj64d.exe!Main(stack_resizeable_cont_t<slice_t> & args) Line 1186	C++
      proj64d.exe!WinMain(HINSTANCE__ * prog_inst, HINSTANCE__ * prog_inst_prev, char * prog_cmd_line, int prog_cmd_show) Line 1262	C++

      proj64d.exe!MemHeapAllocBytes(unsigned __int64 nbytes) Line 1136	C++
      proj64d.exe!stbtt__GetGlyphShapeTT(const stbtt_fontinfo * info, int glyph_index, stbtt_vertex * * pvertices) Line 1523	C++
      proj64d.exe!stbtt_GetGlyphShape(const stbtt_fontinfo * info, int glyph_index, stbtt_vertex * * pvertices) Line 2127	C++
      proj64d.exe!stbtt_GetGlyphBitmapSubpixel(const stbtt_fontinfo * info, float scale_x, float scale_y, float shift_x, float shift_y, int glyph, int * width, int * height, int * xoff, int * yoff) Line 3144	C++
      proj64d.exe!stbtt_GetCodepointBitmapSubpixel(const stbtt_fontinfo * info, float scale_x, float scale_y, float shift_x, float shift_y, int codepoint, int * width, int * height, int * xoff, int * yoff) Line 3211	C++
      proj64d.exe!FontLoadGlyphImage(font_t & font, unsigned int codept, vec2<float> & dimf, vec2<float> & offsetf) Line 526	C++
      proj64d.exe!FontLoadAscii(font_t & font) Line 735	C++
      proj64d.exe!LoadFont(app_t * app, unsigned int fontid, unsigned char * filename_ttf, unsigned __int64 filename_ttf_len, float char_h) Line 215	C++
      proj64d.exe!Main(stack_resizeable_cont_t<slice_t> & args) Line 1186	C++
      proj64d.exe!WinMain(HINSTANCE__ * prog_inst, HINSTANCE__ * prog_inst_prev, char * prog_cmd_line, int prog_cmd_show) Line 1262	C++


#endif
