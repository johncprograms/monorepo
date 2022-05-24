// Copyright (c) John A. Carlos Jr., all rights reserved.


struct
cmd_t
{
  txt_t txt_display;
  txt_t txt_cmd;
  bool already_executing;
  array_t<u8> async_execute_input; // we copy txt_cmd into here, so the background thread has a copy.
  // note the background thread allocates message space out of this, which the main thread then reads.
  // so we're relying on plist_t never moving any of its pages.
  // also note the main thread is the one that resets this before it kicks off the background thread.
  // TODO: there's probably a better way to manage this memory.
  plist_t async_mem;
};


void
CmdInit( cmd_t& cmd )
{
  Init( cmd.txt_display );
  Init( cmd.txt_cmd );

  TxtLoadEmpty( cmd.txt_display );
  TxtLoadEmpty( cmd.txt_cmd );

  fsobj_t cwd;
  FsGetCwd( cwd );

  CmdAddString( cmd.txt_display, Cast( idx_t, cwd.mem ), cwd.len );
  CmdAddString( cmd.txt_display, Cast( idx_t, ">" ), 1 );

  cmd.already_executing = 0;
  Alloc( cmd.async_execute_input, 200 );
  Init( cmd.async_mem, 4000 );
}

void
CmdKill( cmd_t& cmd )
{
  Kill( cmd.txt_display );
  Kill( cmd.txt_cmd );
  cmd.already_executing = 0;
  Free( cmd.async_execute_input );
  Kill( cmd.async_mem );
}




#define __CmdCmd( name )   void ( name )( cmd_t& cmd )
typedef __CmdCmd( *pfn_cmdcmd_t );



// assume dir has only '/' chars, no '\\' chars.
Inl void
ChangeCwd( cmd_t& cmd, u8* dir, idx_t dir_len )
{
  fsobj_t cwd;
  FsGetCwd( cwd );

  bool do_set = 0;

  // parse loop for processing 'dir'
  Forever {

    if( dir_len == 0 ) {
      do_set = 1; // success !
      break;
    }

    if( dir_len >= 1  &&  dir[0] == '/' ) {
      dir_len -= 1;
      dir += 1;
    }

    if( dir_len >= 2  &&  CsEquals( dir, 2, Str( ".." ), 2, 1 ) ) {
      u8* cwd_lastslash = CsScanL( ML( cwd ), '/' );
      if( !cwd_lastslash ) {
        static const u8* msg = Str( "cannot cd up; at root." );
        CmdAddString( cmd.txt_display, Cast( idx_t, msg ), CsLen( msg ) );
        CmdAddLn( cmd.txt_display );
        break;
      } else {
        cwd.len = CsLen( cwd.mem, cwd_lastslash );
        dir += 2;
        dir_len -= 2;
      }
      continue;
    }

    // must check for '.' after check for '..' so it's unambiguous.
    if( dir_len >= 1  &&  dir[0] == '.' ) {
      dir_len -= 1;
      dir += 1;
      continue;
    }

    u8* subdir_end = CsScanR( dir, dir_len, '/' );
    if( !subdir_end ) {
      subdir_end = dir + dir_len;
    }
    u8* subdir = dir;
    idx_t subdir_len = CsLen( subdir, subdir_end );

    fsobj_t tmp = cwd;
    Memmove( AddBack( tmp ), "/", 1 );
    Memmove( AddBack( tmp, subdir_len ), subdir, subdir_len );

    bool subdir_exists = DirExists( ML( tmp ) );
    if( !subdir_exists ) {
      static const u8* msg = Str( "cannot cd; subdir doesn't exist: " );
      CmdAddString( cmd.txt_display, Cast( idx_t, msg ), CsLen( msg ) );
      CmdAddString( cmd.txt_display, Cast( idx_t, subdir ), subdir_len );
      CmdAddLn( cmd.txt_display );
      break;
    } else {
      Memmove( AddBack( cwd ), "/", 1 );
      Memmove( AddBack( cwd, subdir_len ), subdir, subdir_len );

      dir_len -= subdir_len;
      continue;
    }
  }
  if( do_set ) {
    FsSetCwd( cwd );
  }
}

Inl void
PrintCwdAndBracket( cmd_t* cmd )
{
  fsobj_t cwd;
  FsGetCwd( cwd );
  CmdAddString( cmd->txt_display, Cast( idx_t, cwd.mem ), cwd.len );
  CmdAddString( cmd->txt_display, Cast( idx_t, ">" ), 1 );
}

//
// so after reworking the task message passing a bit, the model here is:
//   CmdExecute sets a bool to prevent reentrant command execution.
//     pushes an async task AsyncTask_Execute
//   AsyncTask_Execute calls Execute with the given args
//     passes an __ExecuteOutput function, Async_OutputForCmdExecute
//     when Execute is done, pushes a main task MainTaskCompleted_ExecuteOutputFinished, copying the exit code.
//   Async_OutputForCmdExecute pushes a main task MainTaskCompleted_ExecuteOutputIncremental, copying the message.
//   MainTaskCompleted_ExecuteOutputIncremental adds the strings to cmd->txt_display, and sets *target_valid = 0
//

__MainTaskCompleted( MainTaskCompleted_ExecuteOutputIncremental )
{
  ProfFunc();

  auto cmd = Cast( cmd_t*, misc0 );
  auto message_mem = Cast( u8*, misc1 );
  auto message_len = Cast( idx_t, misc2 );

  CmdAddString( cmd->txt_display, Cast( idx_t, message_mem ), message_len );
  *target_valid = 0;
}

__MainTaskCompleted( MainTaskCompleted_ExecuteOutputFinished )
{
  ProfFunc();

  auto cmd = Cast( cmd_t*, misc0 );
  auto exit_code = Cast( s32, Cast( sidx_t, misc1 ) );

  auto str = SliceFromCStr( "Exit code: " );
  CmdAddString( cmd->txt_display, Cast( idx_t, str.mem ), str.len );
  embeddedarray_t<u8, 64> tmp;
  CsFrom_s32( tmp.mem, Capacity( tmp ), &tmp.len, exit_code );
  CmdAddString( cmd->txt_display, Cast( idx_t, tmp.mem ), tmp.len );
  CmdAddLn( cmd->txt_display );

  PrintCwdAndBracket( cmd );

  *target_valid = 0;

  // reset this, to start allowing a new CmdExecute to work.
  cmd->already_executing = 0;
}

// copy message contents into cmd->async_mem, and then push a maintask for incremental display.
__ExecuteOutput( Async_OutputForCmdExecute )
{
  ProfFunc();

  auto cmd = Cast( cmd_t*, misc0 );
  auto taskthread = Cast( taskthread_t*, misc1 );

  slice_t internal_error_string = {};
  if( internal_failure ) {
    internal_error_string = SliceFromCStr( "Internal error: " );
  }
  auto len = internal_error_string.len + message->len;
  auto slice = AddPlistSlice( cmd->async_mem, u8, 1, len );
  Memmove( slice.mem, ML( internal_error_string ) );
  Memmove( slice.mem + internal_error_string.len, ML( *message ) );

  maincompletedqueue_entry_t entry;
  entry.FnMainTaskCompleted = MainTaskCompleted_ExecuteOutputIncremental;
  entry.misc0 = cmd;
  entry.misc1 = slice.mem;
  entry.misc2 = Cast( void*, slice.len );
  entry.time_generated = TimeTSC();
  PushMainTaskCompleted( taskthread, &entry );
}

__AsyncTask( AsyncTask_Execute )
{
  ProfFunc();

  auto async_execute_input = Cast( array_t<u8>*, misc0 );
  auto cmd = Cast( cmd_t*, misc1 );

  s32 exit_code = Execute( SliceFromArray( *async_execute_input ), 0, Async_OutputForCmdExecute, cmd, taskthread );

  maincompletedqueue_entry_t entry;
  entry.FnMainTaskCompleted = MainTaskCompleted_ExecuteOutputFinished;
  entry.misc0 = cmd;
  entry.misc1 = Cast( void*, Cast( sidx_t, exit_code ) );
  entry.misc2 = 0;
  entry.time_generated = TimeTSC();
  PushMainTaskCompleted( taskthread, &entry );
}

#if 0 // old version for main thread Execute call.
  __ExecuteOutput( OutputForCmdExecute )
  {
    auto cmd = Cast( cmd_t*, misc );
    if( internal_failure ) {
      auto str = SliceFromCStr( "Internal error: " );
      CmdAddString( cmd->txt_display, Cast( idx_t, str.mem ), str.len );
    }
    CmdAddString( cmd->txt_display, Cast( idx_t, message->mem ), message->len );
  }
#endif

__CmdCmd( CmdExecute )
{
  // prevent reentrancy while async stuff is executing
  if( cmd.already_executing ) {
    LogUI( "[CMD] CmdExecute already in progress!" );
    return;
  }

  // force cursor to end of txt_display
  CmdCursorFileR( cmd.txt_display );

  // TODO: what do we do with newlines in the txt_cmd ?
  string_t input = AllocContents( &cmd.txt_cmd.buf, eoltype_t::crlf );
  if( input.len ) {
    CmdSelectAll( cmd.txt_cmd );
    CmdRemChL( cmd.txt_cmd );
    CmdAddString( cmd.txt_display, Cast( idx_t, input.mem ), input.len );
    CmdAddLn( cmd.txt_display );

    // built-in commands:
    if( CsEquals( input.mem, 3, Str( "cd " ), 3, 0 ) ) {
      auto dir = input.mem + 3;
      auto dir_len = input.len - 3;
      CsReplace( dir, dir_len, '\\', '/' );
      ChangeCwd( cmd, dir, dir_len );
      PrintCwdAndBracket( &cmd );
    }
    elif( CsEquals( input.mem, 2, Str( "ls" ), 2, 0 )  ||
          CsEquals( input.mem, 3, Str( "dir" ), 3, 0 ) ) {
      fsobj_t cwd;
      FsGetCwd( cwd );
      plist_t mem;
      Init( mem, 32768 );
      array_t<slice_t> objs;
      Alloc( objs, 256 );
      FsFindDirs( objs, mem, ML( cwd ), 0 );
      ForLen( i, objs ) {
        auto& obj = objs.mem[i];
        CmdAddString( cmd.txt_display, Cast( idx_t, obj.mem ), obj.len );
        CmdAddLn( cmd.txt_display );
      }
      objs.len = 0;
      FsFindFiles( objs, mem, ML( cwd ), 0 );
      ForLen( i, objs ) {
        auto& obj = objs.mem[i];
        CmdAddString( cmd.txt_display, Cast( idx_t, obj.mem ), obj.len );
        CmdAddLn( cmd.txt_display );
      }
      Free( objs );
      Kill( mem );

      PrintCwdAndBracket( &cmd );
    }
    else {
      // calling other .exe/.bat

      // TODO: we probably need to add a 'cmd.exe /c ' prefix for .bat files.
      // TODO: should we be searching for .exe/.bat in specific dirs?
      //   maybe a config option for that? or just use the Path ENV vars?

      cmd.already_executing = 1;

      Reset( cmd.async_mem );

      auto input_slice = SliceFromString( input );
      Reserve( cmd.async_execute_input, input_slice.len );
      cmd.async_execute_input.len = 0;
      AddBackContents( &cmd.async_execute_input, input_slice );
      asyncqueue_entry_t entry;
      entry.FnAsyncTask = AsyncTask_Execute;
      entry.misc0 = &cmd.async_execute_input;
      entry.misc1 = &cmd;
      entry.time_generated = TimeTSC();
      PushAsyncTask( 0, &entry );

      CmdAddLn( cmd.txt_display );

#if 0 // old code that did execute on the main thread.
      s32 exit_code = Execute( input_slice, 0, OutputForCmdExecute, &cmd );
      auto str = SliceFromCStr( "Exit code: " );
      CmdAddString( cmd.txt_display, Cast( idx_t, str.mem ), str.len );
      embeddedarray_t<u8, 64> tmp;
      CsFrom_s32( tmp.mem, Capacity( tmp ), &tmp.len, exit_code );
      CmdAddString( cmd.txt_display, Cast( idx_t, tmp.mem ), tmp.len );
      CmdAddLn( cmd.txt_display );
#endif

    }
  }
  else {
    CmdAddLn( cmd.txt_display );
    PrintCwdAndBracket( &cmd );
  }
  Free( input );
}



void
CmdRender(
  cmd_t& cmd,
  bool& target_valid,
  array_t<f32>& stream,
  font_t& font,
  rectf32_t bounds,
  vec2<f32> zrange,
  f64 timestep_realtime,
  f64 timestep_fixed
  )
{
  ProfFunc();

  f32 y_frac = 0.8f; // TODO: make this adjustable by mouse.

  auto bounds_u = _rect( bounds.p0, bounds.p0 + ( bounds.p1 - bounds.p0 ) * _vec2( 1.0f, y_frac ) );
  auto bounds_d = _rect( _vec2( bounds_u.p0.x, bounds_u.p1.y ), bounds.p1 );

  // add y padding between the two rects
  constant f32 px_padding = 2.0f;
  bounds_u.p1.y -= 0.5f * px_padding;
  bounds_d.p0.y += 0.5f * px_padding;

  TxtRender(
    cmd.txt_display,
    target_valid,
    stream,
    font,
    bounds_u,
    zrange,
    timestep_realtime,
    timestep_fixed,
    1,
    1,
    0,
    1
    );

  TxtRender(
    cmd.txt_cmd,
    target_valid,
    stream,
    font,
    bounds_d,
    zrange,
    timestep_realtime,
    timestep_fixed,
    1,
    1,
    1,
    1
    );

  auto color = _vec4<f32>( 1, 1, 1, 1 );
  RenderQuad(
    stream,
    color,
    bounds_d.p0,
    bounds_u.p1,
    bounds,
    1 // TODO: proper z
    );
}


void
CmdControlMouse(
  cmd_t& cmd,
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

  TxtControlMouse(
    cmd.txt_display,
    target_valid,
    font,
    bounds,
    type,
    btn,
    m,
    raw_delta,
    dwheel,
    allow_scrollbar
    );
}




struct
cmd_cmdmap_t
{
  glwkeybind_t keybind;
  pfn_cmdcmd_t fn;
};

Inl cmd_cmdmap_t
_cmdcmdmap(
  glwkeybind_t keybind,
  pfn_cmdcmd_t fn
  )
{
  cmd_cmdmap_t r;
  r.keybind = keybind;
  r.fn = fn;
  return r;
}


void
CmdControlKeyboard(
  cmd_t& cmd,
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
        // cmd level commands
        cmd_cmdmap_t table[] = {
          _cmdcmdmap( GetPropFromDb( glwkeybind_t, keybind_cmd_execute ), CmdExecute ),
        };
        ForEach( entry, table ) {
          if( GlwKeybind( key, entry.keybind ) ) {
            entry.fn( cmd );
            target_valid = 0;
            ran_cmd = 1;
          }
        }
      } __fallthrough;

      case glwkeyevent_t::repeat:
      case glwkeyevent_t::up: {
      } break;

      default: UnreachableCrash();
    }
  }

  if( !ran_cmd ) {
    bool content_changed = 0;
    TxtControlKeyboardSingleLine(
      cmd.txt_cmd,
      kb_command,
      target_valid,
      content_changed,
      ran_cmd,
      type,
      key,
      keylocks
      );
  }
}

