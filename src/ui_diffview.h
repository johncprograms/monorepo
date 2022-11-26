// Copyright (c) John A. Carlos Jr., all rights reserved.

struct
diffview_t
{
  pagelist_t mem;
  slice_t file_l;
  slice_t file_r;
};
Inl void
Init( diffview_t& d )
{
  Init( d.mem, 32768 );
  d.file_l = {};
  d.file_r = {};
}
Inl void
Kill( diffview_t& d )
{
  Kill( d.mem );
  d.file_l = {};
  d.file_r = {};
}

#define __DiffCmd( name )      void ( name )( diffview_t& d, idx_t misc = 0 )
#define __DiffCmdDef( name )   void ( name )( diffview_t& d, idx_t misc )
typedef __DiffCmdDef( *pfn_diffcmd_t );

Inl void
SetFiles(
  diffview_t& d,
  slice_t file_l,
  slice_t file_r
  )
{
  d.file_l = file_l;
  d.file_r = file_r;
}



Inl void
DiffWindowEvent(
  diffview_t& d,
  enum_t type,
  vec2<u32> dim,
  u32 dpi,
  bool focused,
  bool& target_valid
  )
{
  if( type & glwwindowevent_resize ) {
  }
  if( type & glwwindowevent_dpichange ) {
  }
  if( type & glwwindowevent_focuschange ) {
    if( focused ) {
      // TODO: do we need to worry about checking for external changes?
      //   we probably do, since we likely want to diff actual files, not just temporaries.
      // CmdCheckForExternalChanges( d );
      target_valid = 0;
    }
  }
}

void
DiffRender(
  diffview_t& d,
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
}

void
DiffControlMouse(
  diffview_t& d,
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
diff_cmdmap_t
{
  glwkeybind_t keybind;
  pfn_diffcmd_t fn;
  idx_t misc;
};
Inl diff_cmdmap_t
_diffcmdmap(
  glwkeybind_t keybind,
  pfn_diffcmd_t fn,
  idx_t misc = 0
  )
{
  diff_cmdmap_t r;
  r.keybind = keybind;
  r.fn = fn;
  r.misc = misc;
  return r;
}
Inl void
ExecuteCmdMap(
  diffview_t& d,
  diff_cmdmap_t* table,
  idx_t table_len,
  glwkey_t key,
  bool& target_valid,
  bool& ran_cmd
  )
{
  For( i, 0, table_len ) {
    auto entry = table + i;
    if( GlwKeybind( key, entry->keybind ) ) {
      entry->fn( d, entry->misc );
      target_valid = 0;
      ran_cmd = 1;
    }
  }
}
void
DiffControlKeyboard(
  diffview_t& d,
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
        // diffview level commands
//        diff_cmdmap_t table[] = {
//          _diffcmdmap( GetPropFromDb( glwkeybind_t, keybind_save    ), CmdSave    ),
//          _diffcmdmap( GetPropFromDb( glwkeybind_t, keybind_saveall ), CmdSaveAll ),
//        };
//        ExecuteCmdMap( d, AL( table ), key, target_valid, ran_cmd );
      } __fallthrough;

      case glwkeyevent_t::repeat: {
        // diffview level commands
//        diff_cmdmap_t table[] = {
//          _diffcmdmap( GetPropFromDb( glwkeybind_t, keybind_ ), CmdFindinfilesFocusU ),
//        };
//        ExecuteCmdMap( d, AL( table ), key, target_valid, ran_cmd );
      } break;

      case glwkeyevent_t::up: {
      } break;

      default: UnreachableCrash();
    }
  }
}
