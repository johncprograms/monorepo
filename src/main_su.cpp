// build:window_x64_debug
// Copyright (c) John A. Carlos Jr., all rights reserved.

#if defined(WIN)

#define FINDLEAKS 0
#include "core_cruntime.h"
#include "core_types.h"
#include "core_language_macros.h"
#include "os_mac.h"
#include "os_windows.h"
#include "memory_operations.h"
#include "asserts.h"
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
#include "ds_list.h"
#include "ds_stack_cstyle.h"
#include "ds_hashset_cstyle.h"
#include "filesys.h"
#include "timedate.h"
#include "thread_atomics.h"
#include "threading.h"
#define LOGGER_ENABLED   0
#include "logger.h"
#define PROF_ENABLED   0
#define PROF_ENABLED_AT_LAUNCH   0
#include "profile.h"
#include "rand.h"
#include "allocator_heap_findleaks.h"
#include "mainthread.h"

#define OPENGL_INSTEAD_OF_SOFTWARE       0
#define GLW_RAWINPUT_KEYBOARD            0
#define GLW_RAWINPUT_KEYBOARD_NOHOTKEY   1
#include "glw.h"

#include "sudoku.h"


struct
app_opts_t
{
};

void
Defaults( app_opts_t& opts )
{
}


struct
app_t
{
  bool fullscreen;
  app_opts_t opts;
  board_t board;
  u8 entry_towrite;
};

void
Init( app_t& app )
{
  app.fullscreen = 0;
  Defaults( app.opts );

  Zero( app.board );
  //SetCanonicalSolution( app.board );
  SetEmptySolution( app.board );

  app.entry_towrite = 0;
}

void
Kill( app_t& app )
{
  Zero( app.board );
}


Enumc( fontid_t )
{
  number,
  smallnum,
};


  // TODO: auto layout.
  //   this may be overkill.
  //   really, we just need auto centering. manually doing left-justify isn't all that bad.


__OnRender( AppOnRender )
{
  auto& app = *Cast( app_t*, misc );

  ClearRenderCache( client );

  auto font = Glw::GetFont( client, fontid_t::number );
  auto font_size = _vec2( font.char_w, font.char_h );
  auto border = 0.67f * MAX( font.char_w, font.char_h );
  vec2<f32> board_pos = { border, border };
  auto square_size = font_size * _vec2( 3.0f, 1.7f );
  auto square_text_offset = 0.5f * ( square_size - font_size );
  auto board_size = 9.0f * square_size;
  auto smallfont = Glw::GetFont( client, fontid_t::smallnum );
  auto smallfont_size = _vec2( smallfont.char_w, smallfont.char_h );
  auto smallsquare_size = square_size / 3.0f;
  auto smallsquare_text_offset = 0.5f * ( smallsquare_size - smallfont_size );
  auto bigsquare_size = 3.0f * square_size;

  RenderBoard(
    client,
    app.board,
    board_pos,
    font,
    smallfont,
    dim,
    square_size,
    square_text_offset,
    smallsquare_size,
    smallsquare_text_offset,
    bigsquare_size,
    board_size
    );

  auto entrytowrite_pos = board_pos + _vec2<f32>( 4 * square_size.x + square_text_offset.x, board_size.y + border + square_text_offset.y );
  {
    auto rgba_entrytowrite = _vec4<f32>( 1, 1, 1, 1 );

    static const char* entrytowrite_texts[] = {
      "Erase", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    };

    ptext_t text;
    text.font_id = font.id;
    text.spaces_per_tab = 2;
    text.z = 0;
    text.cliprect[0] = entrytowrite_pos;
    text.cliprect[1] = dim;
    text.color = rgba_entrytowrite;
    text.text = Str( entrytowrite_texts[app.entry_towrite] );
    text.text_len = strlen( entrytowrite_texts[app.entry_towrite] );
    text.pos = entrytowrite_pos;
    Glw::RenderText( client, text );
  }

  auto complete_pos = board_pos + _vec2<f32>( board_size.x + border, square_text_offset.y );
  {
    bool complete = ValidAndFilled( app.board );

    auto rgba_complete = _vec4<f32>( 0.5f, 1, 0.5f, 1 );
    auto rgba_incomplete = _vec4<f32>( 0.5f, 0.5f, 0.5f, 1 );

    ptext_t text;
    text.font_id = font.id;
    text.spaces_per_tab = 2;
    text.z = 0;
    text.cliprect[0] = complete_pos;
    text.cliprect[1] = dim;
    if( complete ) {
      text.color = rgba_complete;
      text.text = Str( "Complete!" );
      text.text_len = 9;
    } else {
      text.color = rgba_incomplete;
      text.text = Str( "Incomplete" );
      text.text_len = 10;
    }
    text.pos = complete_pos;
    Glw::RenderText( client, text );
  }

  auto valid_pos = complete_pos + _vec2<f32>( 0, square_size.y );
  {
    bool valid = Valid( app.board );

    auto rgba_valid = _vec4<f32>( 0.5f, 1, 0.5f, 1 );
    auto rgba_invalid = _vec4<f32>( 1, 0.5f, 0.5f, 1 );

    ptext_t text;
    text.font_id = font.id;
    text.spaces_per_tab = 2;
    text.z = 0;
    text.cliprect[0] = complete_pos;
    text.cliprect[1] = dim;
    if( valid ) {
      text.color = rgba_valid;
      text.text = Str( "Valid" );
      text.text_len = 5;
    } else {
      text.color = rgba_invalid;
      text.text = Str( "Invalid!" );
      text.text_len = 8;
    }
    text.pos = valid_pos;
    Glw::RenderText( client, text );
  }
}



__OnKeyEvent( AppOnKeyEvent )
{
  auto& app = *Cast( app_t*, misc );

  // Global key event handling:
  switch( type ) {
    case glwkeyevent_t::dn:
    case glwkeyevent_t::repeat: {
    } break;

    case glwkeyevent_t::up: {
      switch( key ) {
        case glwkey_t::esc: {
          GlwEarlyKill( client );
        } break;

        case glwkey_t::fn_11: {
          app.fullscreen = !app.fullscreen;
          Glw::_SetFullscreen( client, app.fullscreen );
        } break;
      }
    } break;

    default: UnreachableCrash();
  }

  // TODO: pull out to EditControlKeyboard in sudoku.h
#if 0
  if( KeyUp( client, glwkey_t::num_0 )  |  KeyUp( client, glwkey_t::numpad_0 ) ) { entry_towrite = 0; }
  if( KeyUp( client, glwkey_t::num_1 )  |  KeyUp( client, glwkey_t::numpad_1 ) ) { entry_towrite = 1; }
  if( KeyUp( client, glwkey_t::num_2 )  |  KeyUp( client, glwkey_t::numpad_2 ) ) { entry_towrite = 2; }
  if( KeyUp( client, glwkey_t::num_3 )  |  KeyUp( client, glwkey_t::numpad_3 ) ) { entry_towrite = 3; }
  if( KeyUp( client, glwkey_t::num_4 )  |  KeyUp( client, glwkey_t::numpad_4 ) ) { entry_towrite = 4; }
  if( KeyUp( client, glwkey_t::num_5 )  |  KeyUp( client, glwkey_t::numpad_5 ) ) { entry_towrite = 5; }
  if( KeyUp( client, glwkey_t::num_6 )  |  KeyUp( client, glwkey_t::numpad_6 ) ) { entry_towrite = 6; }
  if( KeyUp( client, glwkey_t::num_7 )  |  KeyUp( client, glwkey_t::numpad_7 ) ) { entry_towrite = 7; }
  if( KeyUp( client, glwkey_t::num_8 )  |  KeyUp( client, glwkey_t::numpad_8 ) ) { entry_towrite = 8; }
  if( KeyUp( client, glwkey_t::num_9 )  |  KeyUp( client, glwkey_t::numpad_9 ) ) { entry_towrite = 9; }
#endif
}



__OnMouseEvent( AppOnMouseEvent )
{
  auto& app = *Cast( app_t*, misc );

  GlwSetCursorType( client, glwcursortype_t::arrow );

  // TODO: pull out into EditControlMouse inside sudoku.h

  // TODO: don't dupe from OnRender
  auto font = Glw::GetFont( client, fontid_t::number );
  auto font_size = _vec2( font.char_w, font.char_h );
  auto border = 0.67f * MAX( font.char_w, font.char_h );
  vec2<f32> board_pos = { border, border };
  auto square_size = font_size * _vec2( 3.0f, 1.7f );
  auto square_text_offset = 0.5f * ( square_size - font_size );
  auto board_size = 9.0f * square_size;
  auto smallfont = Glw::GetFont( client, fontid_t::smallnum );
  auto smallfont_size = _vec2( smallfont.char_w, smallfont.char_h );
  auto smallsquare_size = square_size / 3.0f;
  auto smallsquare_text_offset = 0.5f * ( smallsquare_size - smallfont_size );
  auto bigsquare_size = 3.0f * square_size;

  u8 msqx, msqy;
  bool over_board;
  MapMouseToBoard( m.x, m.y, board_pos, square_size, over_board, msqx, msqy );
  if( over_board ) {
    switch( type ) {
      case glwmouseevent_t::up: {
        switch( btn ) {
          case glwmousebtn_t::l: {
            //bool success = Write( board, mx, my, entry_towrite );
            bool success = WriteAndEraseScratch( app.board, msqx, msqy, app.entry_towrite );
            if( !success ) {
              printf( "failed to write" );
            }
          } break;

          case glwmousebtn_t::r:
          case glwmousebtn_t::m:
          case glwmousebtn_t::b4:
          case glwmousebtn_t::b5: {
          } break;

          default: UnreachableCrash();
        }
      } break;

      case glwmouseevent_t::wheelmove:
      case glwmouseevent_t::move:
      case glwmouseevent_t::dn: {
      } break;

      default: UnreachableCrash();
    }
  }
}




__OnWindowEvent( AppOnWindowEvent )
{
  //auto& app = *Cast( app_t*, misc );
  switch( type ) {
    case glwwindowevent_t::resize: {
    } break;

    case glwwindowevent_t::focuschange: {
      if( focused ) {
        //printf( "focus ( 1 )\n" );
      } else {
        //printf( "focus ( 0 )\n" );
      }
    } break;

    default: UnreachableCrash();
  }
}



int
Main( u8* cmdline, idx_t cmdline_len )
{
  Prof( glw_init );
  client_t client;
  glw_create_t create;
  SetDefaults( create );
  CstrCopy( create.title, Str( "sudoku" ), 6 );
  create.title_len = 6;
  create.fullscreen = 0;
  create.dim_windowed.x = 700;
  create.dim_windowed.y = 500;
  Init( client, create );
  ProfClose( glw_init );


  // c:/windows/fonts/droidsansmono.ttf
  // c:/windows/fonts/lucon.ttf
  // c:/windows/fonts/liberationmono-regular.ttf
  // c:/windows/fonts/consola.ttf
  auto font_filename = Str( "c:/windows/fonts/consola.ttf" );
  auto font_filename_len = CstrLength( font_filename );

  glw_fontopts_t font_number;
  font_number.font_id = Cast( idx_t, fontid_t::number );
  font_number.char_h = 22.0f;
  font_number.filename_ttf_len = font_filename_len;
  Memmove( font_number.filename_ttf, font_filename, font_filename_len );

  glw_fontopts_t font_smallnum;
  font_smallnum.font_id = Cast( idx_t, fontid_t::smallnum );
  font_smallnum.char_h = 10.0f;
  font_smallnum.filename_ttf_len = font_filename_len;
  Memmove( font_smallnum.filename_ttf, font_filename, font_filename_len );

  LoadFont( client, font_number );
  LoadFont( client, font_smallnum );

  SetSwapInterval( client, 0 );

  app_t app;
  Init( app );

  callback_t callbacks[] = {
    { glwcallbacktype_t::keyevent    , &app, AppOnKeyEvent    },
    { glwcallbacktype_t::mouseevent  , &app, AppOnMouseEvent  },
    { glwcallbacktype_t::windowevent , &app, AppOnWindowEvent },
    { glwcallbacktype_t::render      , &app, AppOnRender      },
  };
  ForEach( callback, callbacks ) {
    GlwRegisterCallback( client, callback );
  }

  HANDLE wait_timers[] = {
    client.timer_queue,
  };

  while( client.alive ) {

    bool do_queue = 0;

    static const s32 wait_timeout_millisec = 500;
    DWORD waitres = WaitForMultipleObjects( _countof( wait_timers ), wait_timers, 0, wait_timeout_millisec );
    if( waitres == WAIT_FAILED ) {
      AssertWarn( 0 );
    } elif( waitres == WAIT_TIMEOUT ) {
      AssertWarn( 0 );

    } elif( waitres == WAIT_OBJECT_0 + 0 ) { // Regular timer 0 wakeup.
      do_queue = 1;
      //printf( "w0\n" );

    } else {
      AssertWarn( 0 );
    }

    if( do_queue ) {
      GlwProcessClientQueues( client );
    }
  } // end while( alive )

  Kill( client );

  Kill( app );

  return 0;
}




#if defined(_DEBUG)

int
main( int argc, char** argv )
{
  MainInit();

  stack_resizeable_cont_t<u8> cmdline;
  Alloc( cmdline, 512 );
  Fori( int, i, 1, argc ) {
    u8* arg = Cast( u8*, argv[i] );
    idx_t arg_len = CstrLength( arg );
    AddBack( cmdline, arg, arg_len );
    AddBack( cmdline, Str( " " ) );
  }
  int r = Main( ML( cmdline ) );
  Free( cmdline );

  printf( "Main returned: %d\n", r );
  system( "pause" );

  MainKill();
  return r;
}

#else

int WINAPI
WinMain( HINSTANCE prog_inst, HINSTANCE prog_inst_prev, LPSTR prog_cmd_line, int prog_cmd_show )
{
  MainInit();

  u8* cmdline = Str( prog_cmd_line );
  idx_t cmdline_len = CstrLength( Str( prog_cmd_line ) );

  auto r = Main( cmdline, cmdline_len );

  MainKill();
  return r;
}

#endif

#endif // WIN
