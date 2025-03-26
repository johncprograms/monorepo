// Copyright (c) John A. Carlos Jr., all rights reserved.

Enumc( editmode_t )
{
  editfile, // defer to txt operations on the active txt_t
  editfile_findrepl,
  editfile_gotoline,
  switchopened, // choose between opened files
  fileopener, // choose a new file to open and make active
  externalmerge, // detected an external change to an open file; you must decide on a resolution
  findinfiles, // recursive directory file content search
};

Enumc( editlayer_t )
{
  bkgd,
  sel,
  txt,
  scroll_bkgd,
  scroll_btn,

  COUNT
};



struct
findreplace_t
{
  bool focus_find; // else replace.
  bool case_sens;
  bool word_boundary;
  txt_t find;
  txt_t replace;
};
Inl void
Init( findreplace_t& fr )
{
  Init( fr.find );
  TxtLoadEmpty( fr.find );
  Init( fr.replace );
  TxtLoadEmpty( fr.replace );
  fr.focus_find = 1;
  fr.case_sens = 0;
  fr.word_boundary = 0;
}
Inl void
Kill( findreplace_t& fr )
{
  Kill( fr.find );
  Kill( fr.replace );
  fr.focus_find = 1;
  fr.case_sens = 0;
  fr.word_boundary = 0;
}


struct
edit_t
{
  editmode_t mode;
  pagelist_t mem;

  edittxtopen_t* active[2];
  bool horzview;
  bool horzfocus_l; // else r.
  rectf32_t rect_statusbar[2]; // for mouseclicks on the status bar.

  idx_t nlines_screen;

  switchopened_t switchopened;
  findinfiles_t findinfiles;
  fileopener_t fileopener;

  txt_t gotoline;
  findreplace_t findrepl;

  editmode_t mode_before_externalmerge;
  bool horzview_before_externalmerge;
  edittxtopen_t* active_externalmerge;
  file_t file_externalmerge;
};

#define __EditCmd( name )   void ( name )( edit_t& edit, idx_t misc = 0 )
#define __EditCmdDef( name )   void ( name )( edit_t& edit, idx_t misc )
typedef __EditCmdDef( *pfn_editcmd_t );


    #if 0
    // NOTE: opens a txt not backed by a real file!
    void
    EditOpenEmptyTxt( edit_t& edit )
    {
      txt_t txt;
      Init( txt );
      LoadEmpty( txt.buf );
      // set txt.filename so we can uniquely identify this empty txt.
      TimeDate( txt.filename.mem, Capacity( txt.filename ), &txt.filename.len );
      AddBack( edit.opened, &txt );

      if( TxtLen( edit.opened_list ) ) {
        CmdAddLn( edit.opened_list );
      }
      CmdAddString( edit.opened_list, Cast( idx_t, txt.filename.mem ), txt.filename.len );
    }
    #endif


Inl void
OnFileOpen( edit_t& edit, edittxtopen_t* open )
{
  auto allowed = GetPropFromDb( bool, bool_run_on_open_allowed );
  if( !allowed ) {
    return;
  }

  auto show_window = GetPropFromDb( bool, bool_run_on_open_show_window );
  auto run_on_open = GetPropFromDb( slice_t, string_run_on_open );
  auto cmd = AllocString( run_on_open.len + 1 + open->txt.filename.len );
  Memmove( cmd.mem, ML( run_on_open ) );
  Memmove( cmd.mem + run_on_open.len, " ", 1 );
  Memmove( cmd.mem + run_on_open.len + 1, ML( open->txt.filename ) );
  stack_resizeable_pagelist_t<u8> output;
  Init( output, 1024 );
  Log( "RUN_ON_OPEN" );
  LogAddIndent( +1 );
  {
    auto cstr = AllocCstr( cmd );
    Log( "executing command: %s", cstr );
    MemHeapFree( cstr );
  }

  s32 r = Execute( SliceFromString( cmd ), show_window, EmptyOutputForExecute, 0, 0 );

  Log( "retcode: %d", r );

#if 0
  {
    // TODO: LogInline each of the elems of output pagearray.
    auto output_str = StringFromPagelist( output );
    auto cstr = AllocCstr( output_str );
    Log( "output: %s", cstr );
    MemHeapFree( cstr );
    Free( output_str );
  }
#endif

  Kill( output );
  LogAddIndent( -1 );
}

void
EditOpen( edit_t& edit, file_t& file, edittxtopen_t** opened, bool* opened_existing )
{
  // detect files we already have open.
  auto open = EditGetOpenedFile( edit.switchopened, ML( file.obj ) );
  if( open ) {
    *opened_existing = 1;

  } else {
    *opened_existing = 0;

    auto elem = AddLast( edit.switchopened.opened );
    open = &elem->value;
    open->unsaved = 0;
    open->horz_l = edit.horzfocus_l;
    Init( open->txt );
    TxtLoad( open->txt, file );
    open->time_lastwrite = file.time_lastwrite;

    auto mruelem = AddLast( edit.switchopened.openedmru );
    mruelem->value = open;
    AssertCrash( edit.switchopened.openedmru.len == edit.switchopened.opened.len );

    OnFileOpen( edit, open );
  }
  *opened = open;
}

Inl void
EditSetActiveTxt( edit_t& edit, edittxtopen_t* open )
{
  AssertCrash( open );
  edit.horzfocus_l = open->horz_l;
  edit.active[edit.horzfocus_l] = open;
  MoveOpenedToFrontOfMru( edit.switchopened, open );
}





// =================================================================================
// FILE OPENER

__EditCmd( CmdMode_editfile_from_fileopener )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  if( !edit.switchopened.opened.len ) {
    return;
  }
  edit.mode = editmode_t::editfile;

  auto clear_search_on_switch = GetPropFromDb( bool, bool_fileopener_clear_search_on_switch );
  if( clear_search_on_switch ) {
    CmdSelectAll( fo.query );
    CmdRemChL( fo.query );
  }
}

Inl void
_SwitchToFileopener( edit_t& edit )
{
  edit.mode = editmode_t::fileopener;
  fileopener_t& fo = edit.fileopener;

  auto clear_search_on_switch = GetPropFromDb( bool, bool_fileopener_clear_search_on_switch );
  if( clear_search_on_switch ) {
    FileopenerUpdateMatches( fo );
    ListviewResetCS( &fo.listview );
  }

  auto select_search_on_switch = GetPropFromDb( bool, bool_fileopener_select_search_on_switch );
  if( select_search_on_switch ) {
    CmdSelectAll( fo.query );
  }
}

__EditCmd( CmdMode_fileopener_from_editfile )
{
  AssertCrash( edit.mode == editmode_t::editfile );
  _SwitchToFileopener( edit );
}

__EditCmd( CmdMode_fileopener_from_switchopened )
{
  AssertCrash( edit.mode == editmode_t::switchopened );
  _SwitchToFileopener( edit );
}

__FileopenerOpenFileFromRow( EditOpenFileFromRow )
{
  auto edit = Cast( edit_t*, misc );

#if USE_FILEMAPPED_OPEN
  auto file = FileOpenMappedExistingReadShareRead( ML( filename ) );
#else
  auto file = FileOpen( ML( filename ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
#endif
  *loaded = file.loaded;
  if( file.loaded ) {
    edittxtopen_t* open = 0;
    bool opened_existing = 0;
    EditOpen( *edit, file, &open, &opened_existing );
    AssertCrash( open );
    EditSetActiveTxt( *edit, open );
    CmdMode_editfile_from_fileopener( *edit );
  }
#if !USE_FILEMAPPED_OPEN
  FileFree( file );
#endif
}



void
EditInit( edit_t& edit )
{
  Init( edit.mem, 32768 );

  // default to fileopener; the caller will change this if we open to file.
  edit.mode = editmode_t::fileopener;
  Init( edit.switchopened );

  edit.active[0] = 0;
  edit.active[1] = 0;
  edit.horzview = 0;
  edit.horzfocus_l = 0;
  edit.rect_statusbar[0] = {};
  edit.rect_statusbar[1] = {};

  Init( edit.findinfiles );

  Init( edit.fileopener );

  Init( edit.gotoline );
  TxtLoadEmpty( edit.gotoline );

  Init( edit.findrepl );

  edit.mode_before_externalmerge = edit.mode;
  edit.horzview_before_externalmerge = edit.horzview;
  edit.active_externalmerge = 0;
  edit.file_externalmerge = {};

  // CmdFileopenerRefresh is hugely expensive, even async, so leave it
  // to the caller to decide whether to do this after init or not.
  //   CmdFileopenerRefresh( edit );
}

void
EditKill( edit_t& edit )
{
  edit.mode = editmode_t::fileopener;
  Kill( edit.switchopened );

  Kill( edit.findinfiles );

  edit.active[0] = 0;
  edit.active[1] = 0;
  edit.rect_statusbar[0] = {};
  edit.rect_statusbar[1] = {};

  Kill( edit.fileopener );
  Kill( edit.gotoline );

  Kill( edit.findrepl );

  edit.mode_before_externalmerge = edit.mode;
  edit.horzview_before_externalmerge = edit.horzview;
  edit.active_externalmerge = 0;
  edit.file_externalmerge = {};

  Kill( edit.mem );
}




Inl edittxtopen_t*
GetActiveOpen( edit_t& edit )
{
  return edit.active[edit.horzfocus_l];
}

Inl txt_t&
GetActiveFindReplaceTxt( findreplace_t& fr )
{
  if( fr.focus_find ) {
    return fr.find;
  } else {
    return fr.replace;
  }
}




__EditCmd( CmdMode_editfile_gotoline_from_editfile )     { edit.mode = editmode_t::editfile_gotoline; }
__EditCmd( CmdMode_editfile_from_editfile_findrepl )     { edit.mode = editmode_t::editfile; }
__EditCmd( CmdMode_editfile_from_editfile_gotoline )     { edit.mode = editmode_t::editfile; }

__EditCmd( CmdMode_findinfiles_from_editfile )
{
  AssertCrash( edit.mode == editmode_t::editfile );
  edit.mode = editmode_t::findinfiles;
  auto& fif = edit.findinfiles;
  CmdSelectAllQueryText( fif );
//  CmdFindinfilesRefresh( edit );
}

__EditCmd( CmdMode_editfile_from_findinfiles )
{
//  auto& fif = edit.findinfiles;
  AssertCrash( edit.mode == editmode_t::findinfiles );
  edit.mode = editmode_t::editfile;
//  fif.matches.len = 0;
//  CmdSelectAll( fif.query );
//  CmdRemChL( fif.query );
//  ListviewResetCS( &fif.listview );
}

__EditCmd( CmdMode_editfile_or_fileopener_from_findinfiles )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  if( !edit.switchopened.opened.len ) {
    _SwitchToFileopener( edit );
  }
  else {
    edit.mode = editmode_t::editfile;
  }
}

__EditCmd( CmdMode_findinfiles_from_fileopener )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  edit.mode = editmode_t::findinfiles;
  auto& fif = edit.findinfiles;
  CmdSelectAllQueryText( fif );
//  CmdFindinfilesRefresh( edit );
}

__FindinfilesOpenFileForChoose( EditOpenFileForChoose )
{
  auto edit = Cast( edit_t*, misc );

#if USE_FILEMAPPED_OPEN
  auto file = FileOpenMappedExistingReadShareRead( ML( filename ) );
#else
  auto file = FileOpen( ML( filename ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
#endif

  *loaded = file.loaded;
  if( file.loaded ) {
    edittxtopen_t* open = 0;
    *opened_existing = 0;
    EditOpen( *edit, file, &open, opened_existing );
    AssertCrash( open );
    EditSetActiveTxt( *edit, open );
    CmdMode_editfile_from_findinfiles( *edit );

    *txt = &open->txt;
  }

#if !USE_FILEMAPPED_OPEN
  FileFree( file );
#endif
}

__FindinfilesOpenFileForReplace( EditOpenFileForReplace )
{
  auto edit = Cast( edit_t*, misc );
  auto open = EditGetOpenedFile( edit->switchopened, ML( filename ) );
  *opened_existing = 0;
  if( open ) {
    *opened_existing = 1;
    *loaded = 1;
  }
  else {
#if USE_FILEMAPPED_OPEN
    auto file = FileOpenMappedExistingReadShareRead( ML( filename ) );
#else
    auto file = FileOpen( ML( filename ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
#endif
    *loaded = file.loaded;
    if( file.loaded ) {
      EditOpen( *edit, file, &open, opened_existing );
      AssertCrash( open );
      AssertCrash( !*opened_existing );
    }
#if !USE_FILEMAPPED_OPEN
    FileFree( file );
#endif
  }
}


// =================================================================================
// EDIT FILE


__EditCmd( CmdMode_editfile_findrepl_from_editfile )
{
  edit.mode = editmode_t::editfile_findrepl;
  edit.findrepl.focus_find = 1;
}

__EditCmd( CmdEditfileFindreplToggleFocus )
{
  AssertCrash( edit.mode == editmode_t::editfile_findrepl );
  edit.findrepl.focus_find = !edit.findrepl.focus_find;
}

__EditCmd( CmdEditfileFindreplToggleCaseSens )
{
  AssertCrash( edit.mode == editmode_t::editfile_findrepl );
  edit.findrepl.case_sens = !edit.findrepl.case_sens;
}

__EditCmd( CmdEditfileFindreplToggleWordBoundary )
{
  AssertCrash( edit.mode == editmode_t::editfile_findrepl );
  edit.findrepl.word_boundary = !edit.findrepl.word_boundary;
}




__EditCmd( CmdSave )
{
  ProfFunc();
  auto open = GetActiveOpen( edit );
  if( !open ) {
    return;
  }
  Save( open );
}

__EditCmd( CmdSaveAll )
{
  ProfFunc();
  ForList( elem, edit.switchopened.opened ) {
    auto open = &elem->value;
    Save( open );
  }
}

__EditCmd( CmdEditfileFindreplFindR )
{
  ProfFunc();
  AssertCrash( edit.mode == editmode_t::editfile_findrepl );
  auto open = GetActiveOpen( edit );
  if( !open ) {
    return;
  }
  auto find = AllocContents( &edit.findrepl.find.buf, eoltype_t::crlf );
  txtfind_t tf;
  tf.str = find.mem;
  AssertCrash( find.len <= MAX_u32 );
  tf.str_len = Cast( u32, find.len );
  tf.case_sens = edit.findrepl.case_sens;
  tf.word_boundary = edit.findrepl.word_boundary;
  CmdFindStringR( open->txt, Cast( idx_t, &tf ) );
  Free( find );
}

__EditCmd( CmdEditfileFindreplFindL )
{
  ProfFunc();
  AssertCrash( edit.mode == editmode_t::editfile_findrepl );
  auto open = GetActiveOpen( edit );
  if( !open ) {
    return;
  }
  auto find = AllocContents( &edit.findrepl.find.buf, eoltype_t::crlf );
  txtfind_t tf;
  tf.str = find.mem;
  AssertCrash( find.len <= MAX_u32 );
  tf.str_len = Cast( u32, find.len );
  tf.case_sens = edit.findrepl.case_sens;
  tf.word_boundary = edit.findrepl.word_boundary;
  CmdFindStringL( open->txt, Cast( idx_t, &tf ) );
  Free( find );
}

Inl void
ReplaceSelection( txt_t& txt, slice_t find, slice_t replace )
{
  if( txt.seltype != seltype_t::s ) {
    return;
  }
  auto sel = AllocSelection( txt, eoltype_t::crlf );
  if( MemEqual( ML( find ), ML( sel ) ) ) {
    CmdAddString( txt, Cast( idx_t, replace.mem ), replace.len );
  }
  Free( sel );
}

__EditCmd( CmdEditfileFindreplReplaceR )
{
  ProfFunc();
  AssertCrash( edit.mode == editmode_t::editfile_findrepl );
  auto open = GetActiveOpen( edit );
  if( !open ) {
    return;
  }
  open->unsaved = 1;
  auto find = AllocContents( &edit.findrepl.find.buf, eoltype_t::crlf );
  auto repl = AllocContents( &edit.findrepl.replace.buf, eoltype_t::crlf );
  ReplaceSelection( open->txt, SliceFromString( find ), SliceFromString( repl ) );
  txtfind_t tf;
  tf.str = find.mem;
  AssertCrash( find.len <= MAX_u32 );
  tf.str_len = Cast( u32, find.len );
  tf.case_sens = edit.findrepl.case_sens;
  tf.word_boundary = edit.findrepl.word_boundary;
  CmdFindStringR( open->txt, Cast( idx_t, &tf ) );
  Free( find );
  Free( repl );
}

__EditCmd( CmdEditfileFindreplReplaceL )
{
  ProfFunc();
  AssertCrash( edit.mode == editmode_t::editfile_findrepl );
  auto open = GetActiveOpen( edit );
  if( !open ) {
    return;
  }
  open->unsaved = 1;
  auto find = AllocContents( &edit.findrepl.find.buf, eoltype_t::crlf );
  auto repl = AllocContents( &edit.findrepl.replace.buf, eoltype_t::crlf );
  ReplaceSelection( open->txt, SliceFromString( find ), SliceFromString( repl ) );
  txtfind_t tf;
  tf.str = find.mem;
  AssertCrash( find.len <= MAX_u32 );
  tf.str_len = Cast( u32, find.len );
  tf.case_sens = edit.findrepl.case_sens;
  tf.word_boundary = edit.findrepl.word_boundary;
  CmdFindStringL( open->txt, Cast( idx_t, &tf ) );
  Free( find );
  Free( repl );
}

__EditCmd( CmdEditfileGotolineChoose )
{
  ProfFunc();
  AssertCrash( edit.mode == editmode_t::editfile_gotoline );
  auto open = GetActiveOpen( edit );
  if( !open ) {
    return;
  }
  auto gotoline = AllocContents( &edit.gotoline.buf, eoltype_t::crlf );
  bool valid = 1;
  For( i, 0, gotoline.len ) {
    if( !AsciiIsNumber( gotoline.mem[i] ) ) {
      valid = 0;
      break;
    }
  }
  if( valid ) {
    auto lineno = CsTo_u64( ML( gotoline ) );
    AssertCrash( lineno <= MAX_idx );
    if( lineno ) {
      lineno -= 1;
    }
    CmdCursorGotoline( open->txt, Cast( idx_t, lineno ) );
    CmdSelectAll( edit.gotoline );
    CmdRemChL( edit.gotoline );
    CmdMode_editfile_from_editfile_gotoline( edit );
  }
  Free( gotoline );
}

__EditCmd( CmdEditfileSwapHorz )
{
  AssertCrash( edit.mode == editmode_t::editfile );
  auto open_l = edit.active[1];
  auto open_r = edit.active[0];
  if( open_l  &&  open_r ) {
    open_l->horz_l = !open_l->horz_l;
    open_r->horz_l = !open_r->horz_l;
  }
  edit.active[1] = open_r;
  edit.active[0] = open_l;
  edit.horzfocus_l = !edit.horzfocus_l;
}

__EditCmd( CmdEditfileSwapHorzFocus )
{
  AssertCrash( edit.mode == editmode_t::editfile );
  edit.horzfocus_l = !edit.horzfocus_l;
  auto open = edit.active[edit.horzfocus_l];
  if( open ) {
    MoveOpenedToFrontOfMru( edit.switchopened, open );
  }
}

__EditCmd( CmdEditfileMoveHorzL )
{
  AssertCrash( edit.mode == editmode_t::editfile );
  if( !edit.horzfocus_l ) {
    auto open_focus = edit.active[edit.horzfocus_l];
    if( open_focus ) {
      open_focus->horz_l = !open_focus->horz_l;
    }
    edit.horzfocus_l = !edit.horzfocus_l;
    edit.active[edit.horzfocus_l] = open_focus;
  }
}

__EditCmd( CmdEditfileMoveHorzR )
{
  AssertCrash( edit.mode == editmode_t::editfile );
  if( edit.horzfocus_l ) {
    auto open_focus = edit.active[edit.horzfocus_l];
    if( open_focus ) {
      open_focus->horz_l = !open_focus->horz_l;
    }
    edit.horzfocus_l = !edit.horzfocus_l;
    edit.active[edit.horzfocus_l] = open_focus;
  }
}

__EditCmd( CmdEditfileToggleHorzview )
{
  AssertCrash( edit.mode == editmode_t::editfile );
  edit.horzview = !edit.horzview;
}







// =================================================================================
// SWITCH OPENED

__EditCmd( CmdMode_switchopened_from_editfile )
{
  AssertCrash( edit.mode == editmode_t::editfile );
  edit.mode = editmode_t::switchopened;
  CmdUpdateSearchMatches( edit.switchopened );
  edit.switchopened.opened_scroll_start = 0;
  edit.switchopened.opened_cursor = 0;

  // since the search_matches list is populated from openedmru, it's in mru order.
  // to allow quick change to the second mru entry ( the first is already the active txt ),
  // put the cursor over the second mru entry.
  if( edit.switchopened.search_matches.len > 1 ) {
    edit.switchopened.opened_cursor += 1;
  }
}

__EditCmd( CmdMode_editfile_from_switchopened )
{
  AssertCrash( edit.mode == editmode_t::switchopened );
  edit.mode = editmode_t::editfile;
  edit.switchopened.search_matches.len = 0;
  CmdSelectAll( edit.switchopened.opened_search );
  CmdRemChL( edit.switchopened.opened_search );
}

__EditCmd( CmdSwitchopenedChoose )
{
  ProfFunc();
  auto& so = edit.switchopened;
  if( !so.search_matches.len ) {
    return;
  }
  auto obj = SwitchopenedGetSelection( so );
  if( obj ) {
    auto filename = SliceFromArray( *obj );

#if USE_FILEMAPPED_OPEN
    auto file = FileOpenMappedExistingReadShareRead( ML( filename ) );
#else
    auto file = FileOpen( ML( filename ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
#endif
    const bool loaded = file.loaded;
    if( loaded ) {
      edittxtopen_t* open = 0;
      bool opened_existing = 0;
      EditOpen( edit, file, &open, &opened_existing );
      AssertCrash( open );
      EditSetActiveTxt( edit, open );
      CmdMode_editfile_from_switchopened( edit );
    }
#if !USE_FILEMAPPED_OPEN
    FileFree( file );
#endif

    if( !loaded ) {
      auto cstr = AllocCstr( filename );
      LogUI( "[EDIT] SwitchopenedChoose couldn't load file: \"%s\"", cstr );
      MemHeapFree( cstr );
      return;
    }
  }
}

__EditCmd( CmdSwitchopenedCloseFile )
{
  ProfFunc();
  auto& so = edit.switchopened;
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
      CmdUpdateSearchMatches( edit.switchopened );
    }
  }
  AssertCrash( so.openedmru.len == so.opened.len );
}


// =================================================================================
// EXTERNAL CHANGES

Inl void
CmdCheckForExternalChanges( edit_t& edit )
{
  ProfFunc();
  ForList( elem, edit.switchopened.opened ) {
    auto open = &elem->value;
    edit.file_externalmerge = FileOpen( ML( open->txt.filename ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
    if( edit.file_externalmerge.loaded  &&  open->time_lastwrite != edit.file_externalmerge.time_lastwrite ) {
      // some other program updated the file.
      if( open->unsaved ) {
        // we have unsaved changes; need to decide if we want to stomp the external changes, or discard ours.
        if( edit.mode != editmode_t::externalmerge ) {
          edit.mode_before_externalmerge = edit.mode;
          edit.horzview_before_externalmerge = edit.horzview;
          edit.mode = editmode_t::externalmerge;
          edit.horzview = 0;
        }
        edit.active_externalmerge = open;
        return;

      } else {
        // no unsaved changes, so just silently reload the file.
        cs_undo_t cs;
        CsUndoFromTxt( open->txt, &cs );
        Kill( open->txt );
        Init( open->txt );
        TxtLoad( open->txt, edit.file_externalmerge );
        ApplyCsUndo( open->txt, cs );
        open->unsaved = 0;
        open->time_lastwrite = edit.file_externalmerge.time_lastwrite;
      }
    }
    FileFree( edit.file_externalmerge );
  }

  // all external changes handled.
  if( edit.mode == editmode_t::externalmerge ) {
    edit.mode = edit.mode_before_externalmerge;
    edit.horzview = edit.horzview_before_externalmerge;
  }
  return;
}

__EditCmd( CmdExternalmergeDiscardLocalChanges )
{
  ProfFunc();
  AssertCrash( edit.mode == editmode_t::externalmerge );
  auto open = edit.active_externalmerge;
  AssertCrash( open->unsaved );
  AssertCrash( edit.file_externalmerge.loaded );

  // discard our local changes.
  cs_undo_t cs;
  CsUndoFromTxt( open->txt, &cs );
  Kill( open->txt );
  Init( open->txt );
  TxtLoad( open->txt, edit.file_externalmerge );
  ApplyCsUndo( open->txt, cs );
  open->unsaved = 0;
  open->time_lastwrite = edit.file_externalmerge.time_lastwrite;

  FileFree( edit.file_externalmerge );

  // continue checking other opened files.
  CmdCheckForExternalChanges( edit );
}

__EditCmd( CmdExternalmergeKeepLocalChanges )
{
  ProfFunc();
  AssertCrash( edit.mode == editmode_t::externalmerge );
  auto open = edit.active_externalmerge;
  AssertCrash( open->unsaved );
  AssertCrash( edit.file_externalmerge.loaded );

  // discard the external changes.
  FileFree( edit.file_externalmerge );
  Save( open );
  AssertCrash( !open->unsaved );

  // continue checking other opened files.
  CmdCheckForExternalChanges( edit );
}


Inl void
EditWindowEvent(
  edit_t& edit,
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
      CmdCheckForExternalChanges( edit );
      target_valid = 0;
    }
  }
}




// =================================================================================
// RENDERING

void
_RenderTxt(
  txt_t& txt,
  bool& target_valid,
  stack_resizeable_cont_t<f32>& stream,
  font_t& font,
  rectf32_t bounds,
  vec2<f32> zrange,
  f64 timestep_realtime,
  f64 timestep_fixed,
  bool draw_cursor,
  bool draw_cursorline,
  bool draw_cursorwordmatch,
  bool allow_scrollbar
  )
{
  TxtRender(
    txt,
    target_valid,
    stream,
    font,
    bounds,
    zrange,
    timestep_realtime,
    timestep_fixed,
    draw_cursor,
    draw_cursorline,
    draw_cursorwordmatch,
    allow_scrollbar
    );
}


void
_RenderStatusBar(
  edit_t& edit,
  edittxtopen_t& open,
  stack_resizeable_cont_t<f32>& stream,
  font_t& font,
  rectf32_t& bounds,
  vec2<f32> zrange
  )
{
  auto rgba_text = GetPropFromDb( vec4<f32>, rgba_text );
  auto spaces_per_tab = GetPropFromDb( u8, u8_spaces_per_tab );
  auto line_h = FontLineH( font );

  // status bar, so we know what file is open and if it's unsaved.

//  u8 diff_count[128];
//  idx_t diff_count_len = 0;
//  CsFromIntegerU( AL( diff_count ), &diff_count_len, open.txt.buf.diffs.len, 1 );
//  DrawString(
//    stream,
//    font,
//    bounds.p0,
//    GetZ( zrange, editlayer_t::txt ),
//    bounds,
//    rgba_text,
//    spaces_per_tab,
//    diff_count, diff_count_len
//    );

//  CsFromIntegerU( AL( diff_count ), &diff_count_len, open.txt.window_n_lines, 1 );
//  DrawString(
//    stream,
//    font,
//    origin + _vec2<f32>( 100, 0 ),
//    GetZ( zrange, editlayer_t::txt ),
//    origin,
//    origin + dim,
//    rgba_text,
//    spaces_per_tab,
//    diff_count, diff_count_len
//    );

  auto filename_w = LayoutString( font, spaces_per_tab, ML( open.txt.filename ) );
  DrawString(
    stream,
    font,
    AlignCenter( bounds, filename_w ),
    GetZ( zrange, editlayer_t::txt ),
    bounds,
    rgba_text,
    spaces_per_tab,
    ML( open.txt.filename )
    );

  if( open.unsaved ) {
    static const auto unsaved_label = SliceFromCStr( "-- UNSAVED --" );
    auto label_w = LayoutString( font, spaces_per_tab, ML( unsaved_label ) );
    DrawString(
      stream,
      font,
      AlignRight( bounds, label_w ),
      GetZ( zrange, editlayer_t::txt ),
      bounds,
      rgba_text,
      spaces_per_tab,
      ML( unsaved_label )
      );
  }

  auto mouserect = edit.rect_statusbar + open.horz_l;
  mouserect->p0 = bounds.p0;
  mouserect->p1 = _vec2( bounds.p1.x, bounds.p0.y + line_h );

  bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );
}

Inl void
_RenderEmptyView(
  stack_resizeable_cont_t<f32>& stream,
  font_t& font,
  rectf32_t bounds,
  vec2<f32> zrange,
  slice_t label
  )
{
  auto rgba_text = GetPropFromDb( vec4<f32>, rgba_text );
  auto spaces_per_tab = GetPropFromDb( u8, u8_spaces_per_tab );
  auto line_h = FontLineH( font );
  auto label_w = LayoutString( font, spaces_per_tab, ML( label ) );
  DrawString(
    stream,
    font,
    AlignCenter( bounds, label_w, line_h ),
    GetZ( zrange, editlayer_t::txt ),
    bounds,
    rgba_text,
    spaces_per_tab,
    ML( label )
    );
}

void
_RenderBothSides(
  edit_t& edit,
  bool& target_valid,
  stack_resizeable_cont_t<f32>& stream,
  font_t& font,
  rectf32_t bounds,
  vec2<f32> zrange,
  f64 timestep_realtime,
  f64 timestep_fixed
  )
{
  static const auto label_nothing_open = SliceFromCStr( "--- nothing open ---" );

  if( edit.horzview ) {
    auto open_r = edit.active[0];
    auto open_l = edit.active[1];

    auto bounds_l = _rect( bounds.p0, _vec2( bounds.p0.x + 0.5f * ( bounds.p1.x - bounds.p0.x ), bounds.p1.y ) );
    auto bounds_r = _rect( _vec2( bounds.p0.x + 0.5f * ( bounds.p1.x - bounds.p0.x ), bounds.p0.y ), bounds.p1 );
    if( open_l ) {
      _RenderStatusBar(
        edit,
        *open_l,
        stream,
        font,
        bounds_l,
        zrange
        );
      _RenderTxt(
        open_l->txt,
        target_valid,
        stream,
        font,
        bounds_l,
        zrange,
        timestep_realtime,
        timestep_fixed,
        edit.horzfocus_l,
        edit.horzfocus_l,
        edit.horzfocus_l,
        1
        );
    }
    else {
      _RenderEmptyView( stream, font, bounds_l, zrange, label_nothing_open );
    }
    if( open_r ) {

      _RenderStatusBar(
        edit,
        *open_r,
        stream,
        font,
        bounds_r,
        zrange
        );
      _RenderTxt(
        open_r->txt,
        target_valid,
        stream,
        font,
        bounds_r,
        zrange,
        timestep_realtime,
        timestep_fixed,
        !edit.horzfocus_l,
        !edit.horzfocus_l,
        !edit.horzfocus_l,
        1
        );
    }
    else {
      _RenderEmptyView( stream, font, bounds_r, zrange, label_nothing_open );
    }
  }
  else {
    auto open = edit.active[edit.horzfocus_l];
    if( open ) {
      _RenderStatusBar(
        edit,
        *open,
        stream,
        font,
        bounds,
        zrange
        );
      _RenderTxt(
        open->txt,
        target_valid,
        stream,
        font,
        bounds,
        zrange,
        timestep_realtime,
        timestep_fixed,
        1,
        1,
        1,
        1
        );
    }
    else {
      _RenderEmptyView( stream, font, bounds, zrange, label_nothing_open );
    }
  }
}

void
EditRender(
  edit_t& edit,
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

  switch( edit.mode ) {

    case editmode_t::editfile: {
      _RenderBothSides(
        edit,
        target_valid,
        stream,
        font,
        bounds,
        zrange,
        timestep_realtime,
        timestep_fixed
        );
    } break;

    case editmode_t::switchopened: {
      static const auto header = Str( "SWITCH TO FILE ( MRU ):" );
      static const auto header_len = CstrLength( header );
      auto& so = edit.switchopened;
      auto header_w = LayoutString( font, spaces_per_tab, header, header_len );
      DrawString(
        stream,
        font,
        AlignCenter( bounds, header_w ),
        GetZ( zrange, editlayer_t::txt ),
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
        GetZ( zrange, editlayer_t::txt ),
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
        ZRange( zrange, editlayer_t::txt ),
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
          GetZ( zrange, editlayer_t::bkgd )
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
            GetZ( zrange, editlayer_t::txt ),
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
          GetZ( zrange, editlayer_t::txt ),
          bounds,
          rgba_text,
          spaces_per_tab,
          ML( open->txt.filename )
          );
      }
    } break;

    case editmode_t::fileopener: {
      FileopenerRender(
        edit.fileopener,
        target_valid,
        stream,
        font,
        bounds,
        zrange,
        timestep_realtime,
        timestep_fixed,
        1,
        1,
        1,
        1
      );
    } break;

    case editmode_t::editfile_findrepl: {

      { // case sens
        auto bind0 = GetPropFromDb( glwkeybind_t, keybind_editfile_findrepl_toggle_case_sensitive );
        auto key0 = KeyStringFromGlw( bind0.key );
        AssertCrash( !key0.mem[key0.len] ); // cstr generated by compiler.
        auto label = AllocFormattedString( "Case sensitive: %u -- Press [ %s ] to toggle.", edit.findrepl.case_sens, key0.mem );
        auto label_w = LayoutString( font, spaces_per_tab, ML( label ) );
        DrawString(
          stream,
          font,
          AlignRight( bounds, label_w ),
          GetZ( zrange, editlayer_t::txt ),
          bounds,
          rgba_text,
          spaces_per_tab,
          ML( label )
          );
        Free( label );

        bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );
      }

      { // word boundary
        auto bind0 = GetPropFromDb( glwkeybind_t, keybind_editfile_findrepl_toggle_word_boundary );
        auto key0 = KeyStringFromGlw( bind0.key );
        AssertCrash( !key0.mem[key0.len] ); // cstr generated by compiler.
        auto label = AllocFormattedString( "Whole word: %u -- Press [ %s ] to toggle.", edit.findrepl.word_boundary, key0.mem );
        auto label_w = LayoutString( font, spaces_per_tab, ML( label ) );
        DrawString(
          stream,
          font,
          AlignRight( bounds, label_w ),
          GetZ( zrange, editlayer_t::txt ),
          bounds,
          rgba_text,
          spaces_per_tab,
          ML( label )
          );
        Free( label );

        bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );
      }

      { // find
        static const auto find_label = Str( "Find: " );
        static const idx_t find_label_len = CstrLength( find_label );
        auto findlabel_w = LayoutString( font, spaces_per_tab, find_label, find_label_len );
        DrawString(
          stream,
          font,
          bounds.p0,
          GetZ( zrange, editlayer_t::txt ),
          bounds,
          rgba_text,
          spaces_per_tab,
          find_label, find_label_len
          );

        auto show_cursor = edit.findrepl.focus_find;

        TxtRenderSingleLineSubset(
          edit.findrepl.find,
          stream,
          font,
          0,
          _rect( bounds.p0 + _vec2( findlabel_w, 0.0f ), bounds.p1 ),
          zrange,
          0,
          show_cursor,
          1
          );

        bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );
      }

      { // replace
        static const auto replace_label = Str( "Repl: " );
        static const idx_t replace_label_len = CstrLength( replace_label );
        auto replacelabel_w = LayoutString( font, spaces_per_tab, replace_label, replace_label_len );
        DrawString(
          stream,
          font,
          bounds.p0,
          GetZ( zrange, editlayer_t::txt ),
          bounds,
          rgba_text,
          spaces_per_tab,
          replace_label,
          replace_label_len
          );

        auto show_cursor = !edit.findrepl.focus_find;

        TxtRenderSingleLineSubset(
          edit.findrepl.replace,
          stream,
          font,
          0,
          _rect( bounds.p0 + _vec2( replacelabel_w, 0.0f ), bounds.p1 ),
          zrange,
          0,
          show_cursor,
          1
          );

        bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );
      }

      _RenderBothSides(
        edit,
        target_valid,
        stream,
        font,
        bounds,
        zrange,
        timestep_realtime,
        timestep_fixed
        );
    } break;

    case editmode_t::editfile_gotoline: {

      static const auto gotoline_label = Str( "Go to line: " );
      static const idx_t gotoline_label_len = CstrLength( gotoline_label );
      auto gotolinelabel_w = LayoutString( font, spaces_per_tab, gotoline_label, gotoline_label_len );
      DrawString(
        stream,
        font,
        bounds.p0,
        GetZ( zrange, editlayer_t::txt ),
        bounds,
        rgba_text,
        spaces_per_tab,
        gotoline_label,
        gotoline_label_len
        );

      TxtRenderSingleLineSubset(
        edit.gotoline,
        stream,
        font,
        0,
        _rect( bounds.p0 + _vec2( gotolinelabel_w, 0.0f ), bounds.p1 ),
        zrange,
        1,
        1,
        1
        );

      bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );

      _RenderBothSides(
        edit,
        target_valid,
        stream,
        font,
        bounds,
        zrange,
        timestep_realtime,
        timestep_fixed
        );
    } break;

    case editmode_t::externalmerge: {
      auto open = edit.active_externalmerge;

      _RenderStatusBar(
        edit,
        *open,
        stream,
        font,
        bounds,
        zrange
        );

      static auto label0 = SliceFromCStr( "--- External change detected! ---" );
      auto bind0 = GetPropFromDb( glwkeybind_t, keybind_externalmerge_keep_local_changes );
      auto bind1 = GetPropFromDb( glwkeybind_t, keybind_externalmerge_discard_local_changes );
      auto key0 = KeyStringFromGlw( bind0.key );
      auto key1 = KeyStringFromGlw( bind1.key );
      AssertCrash( !key0.mem[key0.len] ); // cstr generated by compiler.
      AssertCrash( !key1.mem[key1.len] ); // cstr generated by compiler.
      auto label1 = AllocFormattedString( "Press [ %s ] to keep your changes.", key0.mem );
      auto label2 = AllocFormattedString( "Press [ %s ] to discard your changes.", key1.mem );
      auto label0_w = LayoutString( font, spaces_per_tab, ML( label0 ) );
      DrawString(
        stream,
        font,
        AlignCenter( bounds, label0_w ),
        GetZ( zrange, editlayer_t::txt ),
        bounds,
        rgba_text,
        spaces_per_tab,
        ML( label0 )
        );

      bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );

      DrawString(
        stream,
        font,
        bounds.p0,
        GetZ( zrange, editlayer_t::txt ),
        bounds,
        rgba_text,
        spaces_per_tab,
        ML( label1 )
        );
      Free( label1 );

      bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );

      DrawString(
        stream,
        font,
        bounds.p0,
        GetZ( zrange, editlayer_t::txt ),
        bounds,
        rgba_text,
        spaces_per_tab,
        ML( label2 )
        );
      Free( label2 );

      bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );

      _RenderTxt(
        open->txt,
        target_valid,
        stream,
        font,
        bounds,
        zrange,
        timestep_realtime,
        timestep_fixed,
        1,
        1,
        1,
        1
        );
    } break;

    case editmode_t::findinfiles: {
      FindinfilesRender(
        edit.findinfiles,
        target_valid,
        stream,
        font,
        bounds,
        zrange,
        timestep_realtime,
        timestep_fixed
        );
    } break;

    default: UnreachableCrash();
  }
}









// =================================================================================
// MOUSE

void
EditControlMouse(
  edit_t& edit,
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

  switch( edit.mode ) {
    case editmode_t::editfile: {

      auto click_on_statusbar_r =
        GlwMouseInsideRect( m, edit.rect_statusbar[0] )  &&
        type == glwmouseevent_t::dn  &&
        btn == glwmousebtn_t::l;

      auto click_on_statusbar_l =
        GlwMouseInsideRect( m, edit.rect_statusbar[1] )  &&
        type == glwmouseevent_t::dn  &&
        btn == glwmousebtn_t::l;

      if( click_on_statusbar_r ) {
        auto open = edit.active[0];
        auto filename = _OSFormatFilename( ML( open->txt.filename ) );
        SendToClipboardText( *g_client, ML( filename ) );
        LogUI( "Copied filename to clipboard." );
      }
      if( click_on_statusbar_l ) {
        auto open = edit.active[1];
        auto filename = _OSFormatFilename( ML( open->txt.filename ) );
        SendToClipboardText( *g_client, ML( filename ) );
        LogUI( "Copied filename to clipboard." );
      }

      // status bar
      // TODO: more geometry caching to avoid this.
      bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );

      edittxtopen_t* open = 0;
      rectf32_t active_bounds = {};

      // TODO: let mouse click change the horzfocus_l ?

      if( edit.horzview ) {
        auto open_r = edit.active[0];
        auto open_l = edit.active[1];

        auto half_x = 0.5f * ( bounds.p0.x + bounds.p1.x );
        auto bounds_l = _rect( bounds.p0, _vec2( half_x, bounds.p1.y ) );
        auto bounds_r = _rect( _vec2( half_x, bounds.p0.y ), bounds.p1 );

        if( edit.horzfocus_l ) {
          open = open_l;
          active_bounds = bounds_l;
        }
        else {
          open = open_r;
          active_bounds = bounds_r;
        }
      }
      else {
        open = edit.active[edit.horzfocus_l];
        active_bounds = bounds;
      }
      if( open ) {
        TxtControlMouse(
          open->txt,
          target_valid,
          font,
          active_bounds,
          type,
          btn,
          m,
          raw_delta,
          dwheel,
          allow_scrollbar
          );
      }
    } break;

    case editmode_t::editfile_findrepl: {
    } break;
    case editmode_t::editfile_gotoline: {
    } break;
    case editmode_t::switchopened: {
    } break;

    case editmode_t::fileopener: {
      FileopenerControlMouse(
        edit.fileopener,
        target_valid,
        font,
        bounds,
        type,
        btn,
        m,
        raw_delta,
        dwheel,
        EditOpenFileFromRow,
        Cast( idx_t, &edit )
        );
    } break;

    case editmode_t::externalmerge: {
    } break;

    case editmode_t::findinfiles: {
      FindinfilesControlMouse(
        edit.findinfiles,
        target_valid,
        font,
        bounds,
        type,
        btn,
        m,
        raw_delta,
        dwheel,
        EditOpenFileForChoose,
        Cast( idx_t, &edit )
        );
    } break;

    default: UnreachableCrash();
  }
}








// =================================================================================
// KEYBOARD

struct
edit_cmdmap_t
{
  glwkeybind_t keybind;
  pfn_editcmd_t fn;
  idx_t misc;
};

Inl edit_cmdmap_t
_editcmdmap(
  glwkeybind_t keybind,
  pfn_editcmd_t fn,
  idx_t misc = 0
  )
{
  edit_cmdmap_t r;
  r.keybind = keybind;
  r.fn = fn;
  r.misc = misc;
  return r;
}

Inl void
ExecuteCmdMap(
  edit_t& edit,
  edit_cmdmap_t* table,
  idx_t table_len,
  glwkey_t key,
  bool& target_valid,
  bool& ran_cmd
  )
{
  For( i, 0, table_len ) {
    auto entry = table + i;
    if( GlwKeybind( key, entry->keybind ) ) {
      entry->fn( edit, entry->misc );
      target_valid = 0;
      ran_cmd = 1;
    }
  }
}



void
EditControlKeyboard(
  edit_t& edit,
  bool kb_command,
  bool& target_valid,
  bool& ran_cmd,
  glwkeyevent_t type,
  glwkey_t key,
  glwkeylocks_t& keylocks
  )
{
  ProfFunc();

  switch( edit.mode ) {
    case editmode_t::editfile: {
      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_switchopened_from_editfile      ), CmdMode_switchopened_from_editfile      ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_fileopener_from_editfile        ), CmdMode_fileopener_from_editfile        ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_editfile_findrepl_from_editfile ), CmdMode_editfile_findrepl_from_editfile ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_findinfiles_from_editfile       ), CmdMode_findinfiles_from_editfile       ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_editfile_gotoline_from_editfile ), CmdMode_editfile_gotoline_from_editfile ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_editfile_swap_horz                   ), CmdEditfileSwapHorz                     ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_editfile_swap_horzfocus              ), CmdEditfileSwapHorzFocus                ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_editfile_move_horz_l                 ), CmdEditfileMoveHorzL                    ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_editfile_move_horz_r                 ), CmdEditfileMoveHorzR                    ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_editfile_toggle_horzview             ), CmdEditfileToggleHorzview               ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_save    ), CmdSave    ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_saveall ), CmdSaveAll ),
            };
            ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
          } break;

          case glwkeyevent_t::repeat:
          case glwkeyevent_t::up: {
          } break;

          default: UnreachableCrash();
        }
      }

      if( !ran_cmd ) {
        auto open = GetActiveOpen( edit );
        if( open ) {
          TxtControlKeyboard(
            open->txt,
            kb_command,
            target_valid,
            open->unsaved,
            ran_cmd,
            type,
            key,
            keylocks
            );
        }
      }
    } break;

    case editmode_t::editfile_findrepl: {
      auto& active_findreplace = GetActiveFindReplaceTxt( edit.findrepl );

      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_editfile_from_editfile_findrepl ), CmdMode_editfile_from_editfile_findrepl ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_save    ), CmdSave    ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_saveall ), CmdSaveAll ),
            };
            ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
          } __fallthrough;

          case glwkeyevent_t::repeat: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_editfile_findrepl_toggle_focus          ), CmdEditfileFindreplToggleFocus        ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_editfile_findrepl_toggle_case_sensitive ), CmdEditfileFindreplToggleCaseSens     ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_editfile_findrepl_toggle_word_boundary  ), CmdEditfileFindreplToggleWordBoundary ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_editfile_findrepl_find_l                ), CmdEditfileFindreplFindL              ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_editfile_findrepl_find_r                ), CmdEditfileFindreplFindR              ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_editfile_findrepl_repl_l                ), CmdEditfileFindreplReplaceL           ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_editfile_findrepl_repl_r                ), CmdEditfileFindreplReplaceR           ),
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
          active_findreplace,
          kb_command,
          target_valid,
          content_changed,
          ran_cmd,
          type,
          key,
          keylocks
          );
      }
    } break;

    case editmode_t::editfile_gotoline: {

      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_editfile_from_editfile_gotoline ), CmdMode_editfile_from_editfile_gotoline ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_editfile_gotoline_choose             ), CmdEditfileGotolineChoose               ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_save    ), CmdSave    ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_saveall ), CmdSaveAll ),
            };
            ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
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
          edit.gotoline,
          kb_command,
          target_valid,
          content_changed,
          ran_cmd,
          type,
          key,
          keylocks
          );
      }
    } break;

    case editmode_t::switchopened: {
      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_editfile_from_switchopened ), CmdMode_editfile_from_switchopened ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_choose             ), CmdSwitchopenedChoose              ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_closefile          ), CmdSwitchopenedCloseFile         ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_save    ), CmdSave    ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_saveall ), CmdSaveAll ),
            };
            ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
          } __fallthrough;

          case glwkeyevent_t::repeat: {
          } break;

          case glwkeyevent_t::up: {
          } break;

          default: UnreachableCrash();
        }
      }

      if( !ran_cmd ) {
        SwitchopenedControlKeyboard(
          edit.switchopened,
          kb_command,
          target_valid,
          ran_cmd,
          type,
          key,
          keylocks
          );
      }
    } break;

    case editmode_t::fileopener: {
      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_findinfiles_from_fileopener         ), CmdMode_findinfiles_from_fileopener         ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_editfile_from_fileopener            ), CmdMode_editfile_from_fileopener            ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_save    ), CmdSave    ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_saveall ), CmdSaveAll ),
            };
            ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
          } __fallthrough;

          case glwkeyevent_t::repeat: {
          } break;

          case glwkeyevent_t::up: {
          } break;

          default: UnreachableCrash();
        }
      }

      if( !ran_cmd ) {
        FileopenerControlKeyboard(
          edit.fileopener,
          kb_command,
          target_valid,
          ran_cmd,
          type,
          key,
          keylocks,
          EditOpenFileFromRow,
          Cast( idx_t, &edit )
          );
      }
    } break;

    case editmode_t::externalmerge: {
      auto open = edit.active_externalmerge;

      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_externalmerge_discard_local_changes ), CmdExternalmergeDiscardLocalChanges ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_externalmerge_keep_local_changes    ), CmdExternalmergeKeepLocalChanges    ),
            };
            ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
          } break;

          case glwkeyevent_t::repeat:
          case glwkeyevent_t::up: {
          } break;

          default: UnreachableCrash();
        }
      }

      if( !ran_cmd ) {
        TxtControlKeyboardNoContentChange(
          open->txt,
          kb_command,
          target_valid,
          ran_cmd,
          type,
          key,
          keylocks
          );
      }
    } break;

    case editmode_t::findinfiles: {
      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_editfile_from_findinfiles ), CmdMode_editfile_or_fileopener_from_findinfiles ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_save    ), CmdSave    ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_saveall ), CmdSaveAll ),
            };
            ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
          } __fallthrough;

          case glwkeyevent_t::repeat: {
          } break;

          case glwkeyevent_t::up: {
          } break;

          default: UnreachableCrash();
        }
      }

      if( !ran_cmd ) {
        FindinfilesControlKeyboard(
          edit.findinfiles,
          kb_command,
          target_valid,
          ran_cmd,
          type,
          key,
          keylocks,
          EditOpenFileForReplace,
          Cast( idx_t, &edit ),
          EditOpenFileForChoose,
          Cast( idx_t, &edit )
          );
      }
    } break;

    default: UnreachableCrash();
  }
}

