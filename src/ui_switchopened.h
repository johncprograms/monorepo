// Copyright (c) John A. Carlos Jr., all rights reserved.

Enumc( solayer_t )
{
  bkgd,
  sel,
  txt,
  scroll_bkgd,
  scroll_btn,

  COUNT
};

// TODO: migrate this over to listview_t.
// TODO: finish moving edit.opened and edit.openedmru over here.
//   it makes more sense for this to expose a 'GetAllOpenedFiles' kind of interface than for the inverse.
// TODO: come up with a better name for this.
// TODO: separate the opened file list from the UI state? we can consider this later. it's likely not worth it, but sounds nice.

struct
edittxtopen_t
{
  txt_t txt;
#if USE_FILEMAPPED_OPEN
  filemapped_t file_contents;
#endif
  bool unsaved;
  bool horz_l; // else r.  // TODO: replace with an enum.
  u64 time_lastwrite;
};
#if USE_FILEMAPPED_OPEN
  Inl void
  Kill( edittxtopen_t& open )
  {
    FileFree( open.file_contents );
    Kill( open.txt );
  }
#endif

Inl void
Save( edittxtopen_t* open )
{
  AssertCrash( open );
#if USE_FILEMAPPED_OPEN
  We have to figure out how to save, given we have a R/R mapped file.
  We could
    save somewhere else, close the mapped file, rename/stomp, open mapped file.
    copy file contents, close the mapped file, save via a regular W/R handle, open mapped file.
  FileFree( open->file_contents
#endif
  file_t file = FileOpen( ML( open->txt.filename ), fileopen_t::only_existing, fileop_t::W, fileop_t::R );
  if( file.loaded ) {
    if( open->time_lastwrite != file.time_lastwrite ) {
      auto tmp = AllocCstr( ML( open->txt.filename ) );
      u64 timediff =
        MAX( open->time_lastwrite, file.time_lastwrite ) -
        MIN( open->time_lastwrite, file.time_lastwrite );
      LogUI( "[EDIT SAVE] Warning: stomping on external changes made %llu seconds ago: \"%s\"", timediff, tmp );
      MemHeapFree( tmp );
    }
    TxtSave( open->txt, file );
    open->unsaved = 0;
    open->time_lastwrite = file.time_lastwrite;
  }
  else {
    file = FileOpen( ML( open->txt.filename ), fileopen_t::only_new, fileop_t::W, fileop_t::R );
    AssertWarn( file.loaded );
    if( file.loaded ) {
      TxtSave( open->txt, file );
      open->unsaved = 0;
      open->time_lastwrite = file.time_lastwrite;
    } else {
      auto tmp = AllocCstr( ML( open->txt.filename ) );
      LogUI( "[EDIT SAVE] Failed to open file for write: \"%s\"", tmp );
      MemHeapFree( tmp );
    }
  }
  FileFree( file );
}

struct
switchopened_t
{
  listwalloc_t<edittxtopen_t, allocator_pagelist_t, allocation_pagelist_t> opened;
  listwalloc_t<edittxtopen_t*, allocator_pagelist_t, allocation_pagelist_t> openedmru;
  stack_resizeable_cont_t<edittxtopen_t*> search_matches;
  txt_t opened_search;
  idx_t opened_cursor;
  idx_t opened_scroll_start;
  idx_t opened_scroll_end;
  idx_t nlines_screen;
};
void
Init( switchopened_t& so )
{
  Alloc( so.search_matches, 128 );
  Init( so.opened_search );
  TxtLoadEmpty( so.opened_search );
  so.opened_cursor = 0;
  so.opened_scroll_start = 0;
  so.opened_scroll_end = 0;
  so.nlines_screen = 0;
}
void
Kill( switchopened_t& so )
{
  Free( so.search_matches );
  Kill( so.opened_search );
  so.opened_cursor = 0;
  so.opened_scroll_start = 0;
  so.opened_scroll_end = 0;
  so.nlines_screen = 0;
}

Inl edittxtopen_t*
EditGetOpenedFile( switchopened_t& so, u8* filename, idx_t filename_len )
{
  ForList( elem, so.opened ) {
    auto txt = &elem->value.txt;
    bool already_open = MemEqual( ML( txt->filename ), filename, filename_len );
    if( already_open ) {
      return &elem->value;
    }
  }
  return 0;
}

#define __SwitchopenedCmd( name )   void ( name )( switchopened_t& so, idx_t misc = 0, idx_t misc2 = 0 )
#define __SwitchopenedCmdDef( name )   void ( name )( switchopened_t& so, idx_t misc, idx_t misc2 )
typedef __SwitchopenedCmdDef( *pfn_switchopenedcmd_t );

Inl void
MakeOpenedCursorVisible( switchopened_t& so )
{
  so.opened_scroll_start = MIN( so.opened_scroll_start, MAX( so.search_matches.len, 1 ) - 1 );
  so.opened_cursor = MIN( so.opened_cursor, MAX( so.search_matches.len, 1 ) - 1 );

  bool offscreen_u = so.opened_scroll_start && ( so.opened_cursor < so.opened_scroll_start );
  bool offscreen_d = so.opened_scroll_end && ( so.opened_scroll_end <= so.opened_cursor );
  if( offscreen_u ) {
    auto dlines = so.opened_scroll_start - so.opened_cursor;
    so.opened_scroll_start -= dlines;
  } elif( offscreen_d ) {
    auto dlines = so.opened_cursor - so.opened_scroll_end + 1;
    so.opened_scroll_start += dlines;
  }
}

__SwitchopenedCmd( CmdSwitchopenedCursorU )
{
  auto nlines = misc ? misc : 1;
  so.opened_cursor -= MIN( nlines, so.opened_cursor );
  MakeOpenedCursorVisible( so );
}
__SwitchopenedCmd( CmdSwitchopenedCursorD )
{
  auto nlines = misc ? misc : 1;
  so.opened_cursor += nlines;
  so.opened_cursor = MIN( so.opened_cursor, MAX( so.search_matches.len, 1 ) - 1 );
  MakeOpenedCursorVisible( so );
}
__SwitchopenedCmd( CmdSwitchopenedScrollU )
{
  auto nlines = misc ? misc : 1;
  so.opened_scroll_start -= MIN( nlines, so.opened_scroll_start );
}
__SwitchopenedCmd( CmdSwitchopenedScrollD )
{
  auto nlines = misc ? misc : 1;
  so.opened_scroll_start += nlines;
  so.opened_scroll_start = MIN( so.opened_scroll_start, MAX( so.search_matches.len, 1 ) - 1 );
}
__SwitchopenedCmd( CmdSwitchopenedMakeCursorPresent )
{
  so.opened_cursor = so.opened_scroll_start;
  CmdSwitchopenedCursorD( so, so.nlines_screen / 2 );
}
__SwitchopenedCmd( CmdUpdateSearchMatches )
{
  so.search_matches.len = 0;
  auto key = AllocContents( &so.opened_search.buf, eoltype_t::crlf );

  // TODO: openedmru access how?
  ForList( elem, so.openedmru ) {
    auto open = elem->value;
    if( !key.len ) {
      *AddBack( so.search_matches ) = open;
    }
    else {
      idx_t pos;
      if( StringIdxScanR( &pos, ML( open->txt.filename ), 0, ML( key ), 0, 0 ) ) {
        *AddBack( so.search_matches ) = open;
      }
    }
  }

  Free( key );
  so.opened_scroll_start = MIN( so.opened_scroll_start, MAX( so.search_matches.len, 1 ) - 1 );
  so.opened_cursor = MIN( so.opened_cursor, MAX( so.search_matches.len, 1 ) - 1 );
  MakeOpenedCursorVisible( so );
}

Inl fsobj_t*
SwitchopenedGetSelection( switchopened_t& so )
{
  AssertCrash( so.opened_cursor < so.search_matches.len );
  auto open = so.search_matches.mem[so.opened_cursor];
  return &open->txt.filename;
}

#define __SwitchopenedOpenFileForChoose( name )   void ( name )( idx_t misc, slice_t filename, bool* loaded )
typedef __SwitchopenedOpenFileForChoose( *FnSOOpenFileForChoose ); // TODO: rename to snake case.

__SwitchopenedCmd( CmdSwitchopenedChoose )
{
  ProfFunc();
  if( !so.search_matches.len ) {
    return;
  }
  auto fnSOOpenFileForChoose = Cast( FnSOOpenFileForChoose, misc );
  auto obj = SwitchopenedGetSelection( so );
  if( obj ) {
    bool loaded;
    auto filename = SliceFromArray( *obj );
    fnSOOpenFileForChoose( misc2, filename, &loaded );
    if( !loaded ) {
      auto cstr = AllocCstr( filename );
      LogUI( "[EDIT] SwithopenedChoose couldn't load file: \"%s\"", cstr );
      MemHeapFree( cstr );
      return;
    }
  }
}

__SwitchopenedCmd( CmdSwitchopenedCloseFile )
{
  ProfFunc();
  if( !so.search_matches.len ) {
    return;
  }
  auto file = SwitchopenedGetSelection( so );
  AssertCrash( file );
  auto open = EditGetOpenedFile( so, file->mem, file->len );
  AssertCrash( open );
  Save( open );
  if( !open->unsaved ) {
    bool mruremoved = 0;
    ForList( elem, so.openedmru ) {
      if( elem->value == open ) {
        Rem( so.openedmru, elem );
        mruremoved = 1;
        break;
      }
    }
    AssertCrash( mruremoved );
    Kill( open->txt );
    bool removed = 0;
    ForList( elem, so.opened ) {
      if( &elem->value == open ) {
        Rem( so.opened, elem );
        removed = 1;
        break;
      }
    }
    AssertCrash( removed );
    For( i, 0, 2 ) {
      if( edit.active[i] == open ) {
        auto elem = so.openedmru.first;
        edit.active[i] = elem  ?  elem->value  :  0;
      }
    }
    if( !so.opened.len ) {
      CmdMode_fileopener_from_switchopened( edit );
    } else {
      CmdUpdateSearchMatches( edit );
    }
  }
  AssertCrash( so.openedmru.len == so.opened.len );
}

void
SwitchopenedRender(
  switchopened_t& so,
  bool& target_valid,
  stack_resizeable_cont_t<f32>& stream,
  font_t& font,
  rectf32_t bounds,
  vec2<f32> zrange,
  f64 timestep_realtime,
  f64 timestep_fixed
  )
{
  ProfFunc();

  auto rgba_text = GetPropFromDb( vec4<f32>, rgba_text );
  auto rgba_cursor_bkgd = GetPropFromDb( vec4<f32>, rgba_cursor_bkgd );
  auto spaces_per_tab = GetPropFromDb( u8, u8_spaces_per_tab );

  auto line_h = FontLineH( font );

  static const auto header = Str( "SWITCH TO FILE ( MRU ):" );
  static const auto header_len = CstrLength( header );
  auto header_w = LayoutString( font, spaces_per_tab, header, header_len );
  DrawString(
    stream,
    font,
    AlignCenter( bounds, header_w ),
    GetZ( zrange, solayer_t::txt ),
    bounds,
    rgba_text,
    spaces_per_tab,
    header, header_len
    );
  bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );

  so.opened_scroll_start = MIN( so.opened_scroll_start, MAX( so.search_matches.len, 1 ) - 1 );
  so.opened_cursor = MIN( so.opened_cursor, MAX( so.search_matches.len, 1 ) - 1 );

  auto nlines_screen_max = Cast( idx_t, ( bounds.p1.y - bounds.p0.y ) / line_h );
  if( nlines_screen_max ) {
    nlines_screen_max -= 1; // for search bar.
  }
  so.nlines_screen = MIN( nlines_screen_max, so.search_matches.len - so.opened_scroll_start );
  so.opened_scroll_end = so.opened_scroll_start + so.nlines_screen;

  // render search bar.
  static const auto search = Str( "Search: " );
  static const auto search_len = CstrLength( search );
  auto search_w = LayoutString( font, spaces_per_tab, search, search_len );
  DrawString(
    stream,
    font,
    bounds.p0,
    GetZ( zrange, solayer_t::txt ),
    bounds,
    rgba_text,
    spaces_per_tab,
    search, search_len
    );

  TxtRenderSingleLineSubset(
    so.opened_search,
    stream,
    font,
    0,
    _rect( bounds.p0 + _vec2( search_w, 0.0f ), bounds.p1 ),
    ZRange( zrange, solayer_t::txt ),
    0,
    1,
    1
    );

  bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );

  // render current cursor.
  if( so.opened_scroll_start <= so.opened_cursor  &&  so.opened_cursor < so.opened_scroll_end ) {
    auto p0 = bounds.p0 + _vec2( 0.0f, line_h * ( so.opened_cursor - so.opened_scroll_start ) );
    auto p1 = _vec2( bounds.p1.x, p0.y + line_h );
    RenderQuad(
      stream,
      rgba_cursor_bkgd,
      p0,
      p1,
      bounds,
      GetZ( zrange, solayer_t::bkgd )
      );
  }
  static const auto unsaved = Str( " unsaved " );
  static const auto unsaved_len = CstrLength( unsaved );
  auto unsaved_w = LayoutString( font, spaces_per_tab, unsaved, unsaved_len );
  For( i, 0, so.nlines_screen ) {
    idx_t rowidx = ( i + so.opened_scroll_start );
    if( rowidx >= so.search_matches.len ) {
      break;
    }

    auto open = so.search_matches.mem[rowidx];
    auto row_origin = bounds.p0 + _vec2( 0.0f, line_h * i );

    if( open->unsaved ) {
      DrawString(
        stream,
        font,
        row_origin,
        GetZ( zrange, solayer_t::txt ),
        bounds,
        rgba_text,
        spaces_per_tab,
        unsaved, unsaved_len
        );
    }

    row_origin.x += unsaved_w;

    DrawString(
      stream,
      font,
      row_origin,
      GetZ( zrange, solayer_t::txt ),
      bounds,
      rgba_text,
      spaces_per_tab,
      ML( open->txt.filename )
      );
  }
}

void
SwitchopenedControlMouse(
  switchopened_t& so,
  bool& target_valid,
  font_t& font,
  rectf32_t bounds,
  glwmouseevent_t type,
  glwmousebtn_t btn,
  vec2<s32> m,
  vec2<s32> raw_delta,
  s32 dwheel,
  bool allow_scrollbar
  )
{
  ProfFunc();
  auto line_h = FontLineH( font );

}

struct
switchopened_cmdmap_t
{
  glwkeybind_t keybind;
  pfn_switchopenedcmd_t fn;
  idx_t misc;
  idx_t misc2;
};

Inl switchopened_cmdmap_t
_switchopenedcmdmap(
  glwkeybind_t keybind,
  pfn_switchopenedcmd_t fn,
  idx_t misc = 0,
  idx_t misc2 = 0
  )
{
  switchopened_cmdmap_t r;
  r.keybind = keybind;
  r.fn = fn;
  r.misc = misc;
  r.misc2 = misc2;
  return r;
}

Inl void
ExecuteCmdMap(
  switchopened_t& so,
  switchopened_cmdmap_t* table,
  idx_t table_len,
  glwkey_t key,
  bool& target_valid,
  bool& ran_cmd
  )
{
  For( i, 0, table_len ) {
    auto entry = table + i;
    if( GlwKeybind( key, entry->keybind ) ) {
      entry->fn( edit, entry->misc, entry->misc2 );
      target_valid = 0;
      ran_cmd = 1;
    }
  }
}

void
SwitchopenedControlKeyboard(
  switchopened_t& so,
  bool kb_command,
  bool& target_valid,
  bool& ran_cmd,
  glwkeyevent_t type,
  glwkey_t key,
  glwkeylocks_t& keylocks
  )
{
  ProfFunc();

  if( kb_command ) {
    switch( type ) {
      case glwkeyevent_t::dn: {
        // edit level commands
        switchopened_cmdmap_t table[] = {
          _switchopenedcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_choose             ), CmdSwitchopenedChoose              ),
        };
        ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
      } __fallthrough;

      case glwkeyevent_t::repeat: {
        // edit level commands
        switchopened_cmdmap_t table[] = {
          _switchopenedcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_closefile           ), CmdSwitchopenedCloseFile         ),
          _switchopenedcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_cursor_u            ), CmdSwitchopenedCursorU           ),
          _switchopenedcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_cursor_d            ), CmdSwitchopenedCursorD           ),
          _switchopenedcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_cursor_page_u       ), CmdSwitchopenedCursorU           , so.nlines_screen ),
          _switchopenedcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_cursor_page_d       ), CmdSwitchopenedCursorD           , so.nlines_screen ),
          _switchopenedcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_scroll_u            ), CmdSwitchopenedScrollU           , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
          _switchopenedcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_scroll_d            ), CmdSwitchopenedScrollD           , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
          _switchopenedcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_scroll_page_u       ), CmdSwitchopenedScrollU           , so.nlines_screen ),
          _switchopenedcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_scroll_page_d       ), CmdSwitchopenedScrollD           , so.nlines_screen ),
          _switchopenedcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_make_cursor_present ), CmdSwitchopenedMakeCursorPresent ),
        };
        ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
      } break;

      case glwkeyevent_t::up: {
      } break;

      default: UnreachableCrash();
    }
  }

  if( !ran_cmd ) {
    bool content_changed = 0;
    TxtControlKeyboardSingleLine(
      so.opened_search,
      kb_command,
      target_valid,
      content_changed,
      ran_cmd,
      type,
      key,
      keylocks
      );
    // auto-update the matches, since it's pretty fast.
    if( content_changed ) {
      CmdUpdateSearchMatches( edit );
    }
  }
}

