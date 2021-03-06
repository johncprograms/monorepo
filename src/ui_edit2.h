// Copyright (c) John A. Carlos Jr., all rights reserved.

Enumc( editmode_t )
{
  editfile, // defer to txt operations on the active txt_t
  editfile_findrepl,
  editfile_gotoline,

  switchopened, // choose between opened files

  fileopener, // choose a new file to open and make active
  fileopener_renaming,

  externalmerge,

  findinfiles,
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



//
// should we bother using buf for filecontentsearch / findinfiles ?
// presumably it will only ever be slower than raw string search.
// we never need any content-editing capabilities, even for replacement, since we open in the editor for that.
// maybe if we implement largefile-streaming at some point, then it might be worth it.
// we can probably delete this macro.
//
// should we use buf_t for filecontentsearch / findinfiles when any given file is open?
// it's strictly slower as discussed, but we could let you work on unsaved files.
// i'm not sure i really care about that scenario; i've been working without that feature forever now.
// i'll leave the code in place because why not, but this should be set to 0 i think.
// one immediate killer of using an already-open buf_t is that the findinfiles search is async.
// so we can't really do that without adding lockouts, and that feels bad.
//
// one problem is: how do i convert 1d ranges to 2d ranges when you've got a buf_t open already?
// i'm okay to skip unsaved files entirely, emitting a warning for that is sufficient.
// i guess we can just use the stored eoltype to count, which is valid since the last save.
//
#define USE_BUF_FOR_FILECONTENTSEARCH 0



struct
foundinfile_t
{
  slice_t name;
  slice_t sample; // line sample text to display a context preview.
#if USE_BUF_FOR_FILECONTENTSEARCH
  u32 l_x;
  u32 r_x;
  u32 l_y;
  u32 r_y;
#else
  idx_t pos_match_l;
  idx_t pos_match_r;
#endif
  idx_t match_len; // equivalent to ( pos_match_r - pos_match_l )
  idx_t sample_match_offset; // index into sample noting the start of the highlighted match.
  idx_t sample_match_len; // separate len because we may have to truncate the highlighted match in the sample.
};

Inl void
Zero( foundinfile_t& f )
{
  f.name = {};
  f.sample = {};
#if USE_BUF_FOR_FILECONTENTSEARCH
  f.l_x = 0;
  f.r_x = 0;
  f.l_y = 0;
  f.r_y = 0;
#else
  f.pos_match_l = {};
  f.pos_match_r = {};
#endif
  f.match_len = 0;
  f.sample_match_offset = 0;
  f.sample_match_len = 0;
}

Inl void
Init( foundinfile_t& f )
{
  Zero( f );
}

Inl void
Kill( foundinfile_t& f )
{
  Zero( f );
}




// per-thread context to execute the filecontentsearch task.
struct
asynccontext_filecontentsearch_t
{
  // input / readonly:
  array_t<slice_t>* filenames;
  idx_t filenames_start;
  idx_t filenames_count;
  array_t<slice_t>* ignored_filetypes_list;
  array_t<slice_t>* included_filetypes_list;
  slice32_t key;
  bool case_sens;
  bool word_boundary;

  // output / modifiable:
  plist_t mem;
  pagearray_t<foundinfile_t> matches;


  u8 cache_line_padding_to_avoid_thrashing[64]; // last thing, since this type is packed into an array_t
};




Enumc( findinfilesfocus_t )
{
  dir,
  ignored_filetypes,
  included_filetypes,
  query,
  replacement,
  COUNT
};
Inl findinfilesfocus_t
PreviousValue( findinfilesfocus_t value )
{
  switch( value ) {
    case findinfilesfocus_t::dir:                return findinfilesfocus_t::replacement;
    case findinfilesfocus_t::ignored_filetypes:  return findinfilesfocus_t::dir;
    case findinfilesfocus_t::included_filetypes: return findinfilesfocus_t::ignored_filetypes;
    case findinfilesfocus_t::query:              return findinfilesfocus_t::included_filetypes;
    case findinfilesfocus_t::replacement:        return findinfilesfocus_t::query;
    default: UnreachableCrash(); return {};
  }
}
Inl findinfilesfocus_t
NextValue( findinfilesfocus_t value )
{
  switch( value ) {
    case findinfilesfocus_t::dir:                return findinfilesfocus_t::ignored_filetypes;
    case findinfilesfocus_t::ignored_filetypes:  return findinfilesfocus_t::included_filetypes;
    case findinfilesfocus_t::included_filetypes: return findinfilesfocus_t::query;
    case findinfilesfocus_t::query:              return findinfilesfocus_t::replacement;
    case findinfilesfocus_t::replacement:        return findinfilesfocus_t::dir;
    default: UnreachableCrash(); return {};
  }
}


struct
listview_dblclick_t
{
  bool first_made;
  idx_t first_cursor;
  u64 first_clock;
};
struct
listview_rect_t
{
  rectf32_t rect;
  idx_t row_idx;
};
struct
listview_t
{
  // publicly-accessed
  idx_t cursor; // index into matches. commonly read by the owner/caller.

  // not publicly-accessed
  idx_t* len; // number of items in the view, indirect so we can share it with different backing stores.
  idx_t scroll_target; // desired row index at the CENTER of view
  f64 scroll_start; // current row index at the TOP of view
  kahan64_t scroll_vel;
  idx_t scroll_end; // current row index at the BOTTOM of view
  idx_t window_n_lines;
  idx_t pageupdn_distance;
  bool scroll_grabbed;
  bool has_scrollbar;
  listview_dblclick_t dblclick;
  array_t<listview_rect_t> rowrects;
  rectf32_t scroll_track;
  rectf32_t scroll_btn_up;
  rectf32_t scroll_btn_dn;
  rectf32_t scroll_btn_pos;
  rectf32_t scroll_bounds_view_and_bar;
  rectf32_t scroll_bounds_view;
};

Inl void
Init( listview_t* lv, idx_t* len )
{
  lv->len = len;
  lv->cursor = 0;
  lv->scroll_start = 0;
  lv->scroll_vel = {};
  lv->scroll_target = 0;
  lv->scroll_end = 0;
  lv->window_n_lines = 0;
  lv->pageupdn_distance = 0;
  lv->scroll_grabbed = 0;
  lv->dblclick.first_made = 0;
  lv->dblclick.first_cursor = 0;
  lv->dblclick.first_clock = 0;
  Alloc( lv->rowrects, 128 );
  lv->scroll_track   = {};
  lv->scroll_btn_up  = {};
  lv->scroll_btn_dn  = {};
  lv->scroll_btn_pos = {};
  lv->scroll_bounds_view_and_bar = {};
  lv->scroll_bounds_view = {};
}
Inl void
Kill( listview_t* lv )
{
  lv->len = 0;
  lv->cursor = 0;
  lv->scroll_start = 0;
  lv->scroll_vel = {};
  lv->scroll_target = 0;
  lv->scroll_end = 0;
  lv->window_n_lines = 0;
  lv->pageupdn_distance = 0;
  lv->scroll_grabbed = 0;
  lv->dblclick.first_made = 0;
  lv->dblclick.first_cursor = 0;
  lv->dblclick.first_clock = 0;
  Free( lv->rowrects );
  lv->scroll_track   = {};
  lv->scroll_btn_up  = {};
  lv->scroll_btn_dn  = {};
  lv->scroll_btn_pos = {};
  lv->scroll_bounds_view_and_bar = {};
  lv->scroll_bounds_view = {};
}






struct
findinfiles_t
{
  pagearray_t<foundinfile_t> matches;
  plist_t mem;
  findinfilesfocus_t focus;
  txt_t dir;
  txt_t query;
  bool case_sens;
  bool word_boundary;
  txt_t replacement;
  txt_t ignored_filetypes;
  txt_t included_filetypes;
  string_t matches_dir;
  listview_t listview;

  u8 cache_line_padding_to_avoid_thrashing[64];
  array_t<asynccontext_filecontentsearch_t> asynccontexts;
  idx_t progress_num_files;
  idx_t progress_max;

  // used by all other threads as readonly:
  array_t<slice_t> filenames;
  array_t<slice_t> ignored_filetypes_list;
  array_t<slice_t> included_filetypes_list;
  string_t key;
};

Inl void
Init( findinfiles_t& fif )
{
  Init( fif.mem, 128*1024 );
  Init( fif.matches, 256 );
  Init( fif.dir );
  TxtLoadEmpty( fif.dir );
  fsobj_t cwd;
  FsGetCwd( cwd );
  CmdAddString( fif.dir, Cast( idx_t, cwd.mem ), cwd.len );
  Init( fif.query );
  TxtLoadEmpty( fif.query );
  fif.case_sens = 0;
  fif.word_boundary = 0;
  Init( fif.replacement );
  TxtLoadEmpty( fif.replacement );
  Init( fif.ignored_filetypes );
  TxtLoadEmpty( fif.ignored_filetypes );
  auto default_filetypes = GetPropFromDb( slice_t, string_findinfiles_ignored_file_extensions );
  CmdAddString( fif.ignored_filetypes, Cast( idx_t, default_filetypes.mem ), default_filetypes.len );
  Init( fif.included_filetypes );
  TxtLoadEmpty( fif.included_filetypes );
  auto default_included = GetPropFromDb( slice_t, string_findinfiles_included_file_extensions );
  CmdAddString( fif.included_filetypes, Cast( idx_t, default_included.mem ), default_included.len );
  fif.focus = findinfilesfocus_t::query;
  fif.matches_dir = {};
  Init( &fif.listview, &fif.matches.totallen );
  fif.progress_num_files = 0;
  fif.progress_max = 0;
  Alloc( fif.filenames, 4096 );
  Alloc( fif.asynccontexts, 16 );
  Alloc( fif.ignored_filetypes_list, 16 );
  Alloc( fif.included_filetypes_list, 16 );
  Zero( fif.key );
}

Inl void
Kill( findinfiles_t& fif )
{
  Kill( fif.matches );
  Kill( fif.mem );
  Kill( fif.dir );
  Kill( fif.query );
  fif.case_sens = 0;
  fif.word_boundary = 0;
  Kill( fif.replacement );
  Kill( fif.ignored_filetypes );
  Kill( fif.included_filetypes );
  fif.focus = findinfilesfocus_t::query;
  Free( fif.matches_dir );
  fif.matches_dir = {};
  Kill( &fif.listview );
  fif.progress_num_files = 0;
  fif.progress_max = 0;
  Free( fif.filenames );

  ForLen( i, fif.asynccontexts ) {
    auto ac = fif.asynccontexts.mem + i;
    Kill( ac->mem );
    Kill( ac->matches );
  }
  Free( fif.asynccontexts );

  Free( fif.ignored_filetypes_list );
  Free( fif.included_filetypes_list );
  Free( fif.key );
}








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
fileopener_row_t
{
  slice_t name;
  slice_t sizetxt;
  u64 size;
  bool is_file;
  bool readonly;
};

Inl void
Zero( fileopener_row_t& row )
{
  row.name.len = 0;
  row.sizetxt.len = 0;
  row.size = 0;
  row.is_file = 0;
  row.readonly = 0;
}



Enumc( fileopenerfocus_t )
{
  dir,
  ignored_filetypes,
  included_filetypes,
  ignored_substrings,
  query,
};
Inl fileopenerfocus_t
PreviousValue( fileopenerfocus_t value )
{
  switch( value ) {
    case fileopenerfocus_t::dir:                return fileopenerfocus_t::query;
    case fileopenerfocus_t::ignored_filetypes:  return fileopenerfocus_t::dir;
    case fileopenerfocus_t::included_filetypes: return fileopenerfocus_t::ignored_filetypes;
    case fileopenerfocus_t::ignored_substrings: return fileopenerfocus_t::included_filetypes;
    case fileopenerfocus_t::query:              return fileopenerfocus_t::ignored_substrings;
    default: UnreachableCrash(); return {};
  }
}
Inl fileopenerfocus_t
NextValue( fileopenerfocus_t value )
{
  switch( value ) {
    case fileopenerfocus_t::dir:                return fileopenerfocus_t::ignored_filetypes;
    case fileopenerfocus_t::ignored_filetypes:  return fileopenerfocus_t::included_filetypes;
    case fileopenerfocus_t::included_filetypes: return fileopenerfocus_t::ignored_substrings;
    case fileopenerfocus_t::ignored_substrings: return fileopenerfocus_t::query;
    case fileopenerfocus_t::query:              return fileopenerfocus_t::dir;
    default: UnreachableCrash(); return {};
  }
}

Enumc( fileopener_opertype_t )
{
  checkpt,
  rename,
  changecwd,
  delfile,
  deldir,
  newfile,
  newdir,
  COUNT
};

struct
fileopener_oper_t
{
  fileopener_opertype_t type;
  union
  {
    struct
    {
      fsobj_t src;
      fsobj_t dst;
    }
    rename;

    struct
    {
      fsobj_t src;
      fsobj_t dst;
    }
    changecwd;

    fsobj_t delfile;
    fsobj_t deldir;
    fsobj_t newfile;
    fsobj_t newdir;
  };
};

// per-thread context to execute the FileopenerFillPool task.
// note that we only have 1 background thread at a time doing this work.
// that's because it's likely limited by disk performance, which threading won't help with.
struct
asynccontext_fileopenerfillpool_t
{
  // input / readonly:
  fsobj_t cwd;

  // output / modifiable:
  pagearray_t<fileopener_row_t> pool;
  plist_t pool_mem;
};

struct
fileopener_t
{
  array_t<fileopener_oper_t> history;
  idx_t history_idx;

  // pool is the backing store of actual files/dirs given our cwd and query.
  // based on the query, we'll filter down to some subset, stored in matches.
  // ui just shows matches.
  fileopenerfocus_t focus;
  txt_t cwd;
  fsobj_t last_cwd_for_changecwd_rollback;
  txt_t query;
  txt_t ignored_filetypes;
  txt_t included_filetypes;
  txt_t ignored_substrings;
  pagearray_t<fileopener_row_t> pool;
  plist_t pool_mem; // reset everytime we fillpool.
  pagearray_t<fileopener_row_t*> matches; // points into pool.
  listview_t listview;

  u8 cache_line_padding_to_avoid_thrashing[64];
  asynccontext_fileopenerfillpool_t asynccontext_fillpool;
  u8 cache_line_padding_to_avoid_thrashing2[64];
  idx_t ncontexts_active;

  plist_t matches_mem; // reset everytime we regenerate matches.
  array_t<slice_t> ignored_filetypes_list; // uses matches_mem as backing memory.
  array_t<slice_t> included_filetypes_list;
  array_t<slice_t> ignored_substrings_list;

  idx_t renaming_row; // which row we're renaming, if in renaming mode.
  txt_t renaming_txt;
};

void
Init( fileopener_t& fo )
{
  Alloc( fo.history, 512 );
  fo.history_idx = 0;

  fo.focus = fileopenerfocus_t::query;

  Init( fo.cwd );
  TxtLoadEmpty( fo.cwd );
  fsobj_t cwd;
  FsGetCwd( cwd );
  CmdAddString( fo.cwd, Cast( idx_t, cwd.mem ), cwd.len );
  fo.last_cwd_for_changecwd_rollback = {};

  Init( fo.query );
  TxtLoadEmpty( fo.query );

  Init( fo.ignored_filetypes );
  TxtLoadEmpty( fo.ignored_filetypes );
  auto default_filetypes = GetPropFromDb( slice_t, string_fileopener_ignored_file_extensions );
  CmdAddString( fo.ignored_filetypes, Cast( idx_t, default_filetypes.mem ), default_filetypes.len );

  Init( fo.included_filetypes );
  TxtLoadEmpty( fo.included_filetypes );
  auto default_included = GetPropFromDb( slice_t, string_fileopener_included_file_extensions );
  CmdAddString( fo.included_filetypes, Cast( idx_t, default_included.mem ), default_included.len );

  Init( fo.ignored_substrings );
  TxtLoadEmpty( fo.ignored_substrings );
  auto default_substrings = GetPropFromDb( slice_t, string_fileopener_ignored_substrings );
  CmdAddString( fo.ignored_substrings, Cast( idx_t, default_substrings.mem ), default_substrings.len );

  Init( fo.pool, 128 );
  Init( fo.pool_mem, 128*1024 );

  Init( fo.matches, 128 );

  Init( &fo.listview, &fo.matches.totallen );

  fo.asynccontext_fillpool.cwd.len = 0;
  Init( fo.asynccontext_fillpool.pool, 128 );
  Init( fo.asynccontext_fillpool.pool_mem, 128*1024 );
  fo.ncontexts_active = 0;

  Init( fo.matches_mem, 128*1024 );
  Alloc( fo.ignored_filetypes_list, 16 );
  Alloc( fo.included_filetypes_list, 16 );
  Alloc( fo.ignored_substrings_list, 16 );

  fo.renaming_row = 0;
  Init( fo.renaming_txt );
  TxtLoadEmpty( fo.renaming_txt );
}

void
Kill( fileopener_t& fo )
{
  Free( fo.history );
  fo.history_idx = 0;

  fo.focus = fileopenerfocus_t::query;

  Kill( fo.cwd );
  fo.last_cwd_for_changecwd_rollback = {};
  Kill( fo.query );
  Kill( fo.ignored_filetypes );
  Kill( fo.included_filetypes );
  Kill( fo.ignored_substrings );

  Kill( fo.pool );
  Kill( fo.pool_mem );

  Kill( fo.matches );

  Kill( &fo.listview );

  fo.asynccontext_fillpool.cwd.len = 0;
  Kill( fo.asynccontext_fillpool.pool );
  Kill( fo.asynccontext_fillpool.pool_mem );
  fo.ncontexts_active = 0;

  Kill( fo.matches_mem );
  Free( fo.ignored_filetypes_list );
  Free( fo.included_filetypes_list );
  Free( fo.ignored_substrings_list );

  fo.renaming_row = 0;
  Kill( fo.renaming_txt );
}






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


struct
edit_t
{
  editmode_t mode;
  listwalloc_t<edittxtopen_t> opened;
  plist_t mem;

  edittxtopen_t* active[2];
  bool horzview;
  bool horzfocus_l; // else r.
  rectf32_t rect_statusbar[2]; // for mouseclicks on the status bar.

  // openedmru
  listwalloc_t<edittxtopen_t*> openedmru;

  // opened
  array_t<edittxtopen_t*> search_matches;
  txt_t opened_search;
  idx_t opened_cursor;
  idx_t opened_scroll_start;
  idx_t opened_scroll_end;

  idx_t nlines_screen;

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


Inl edittxtopen_t*
EditGetOpenedFile( edit_t& edit, u8* filename, idx_t filename_len )
{
  ForList( elem, edit.opened ) {
    auto txt = &elem->value.txt;
    bool already_open = MemEqual( ML( txt->filename ), filename, filename_len );
    if( already_open ) {
      return &elem->value;
    }
  }
  return 0;
}

Inl void
MoveOpenedToFrontOfMru( edit_t& edit, edittxtopen_t* open )
{
  ForList( elem, edit.openedmru ) {
    if( elem->value == open ) {
      Rem( edit.openedmru, elem );
      InsertFirst( edit.openedmru, elem );
      AssertCrash( edit.openedmru.len == edit.opened.len );
      return;
    }
  }
  AssertCrash( edit.openedmru.len == edit.opened.len );
  UnreachableCrash();
}

Inl void
OnFileOpen( edit_t& edit, edittxtopen_t* open )
{
  auto allowed = GetPropFromDb( bool, bool_run_on_open_allowed );
  if( !allowed ) {
    return;
  }

  auto show_window = GetPropFromDb( bool, bool_run_on_open_show_window );
  auto run_on_open = GetPropFromDb( slice_t, string_run_on_open );
  string_t cmd;
  Alloc( cmd, run_on_open.len + 1 + open->txt.filename.len );
  Memmove( cmd.mem, ML( run_on_open ) );
  Memmove( cmd.mem + run_on_open.len, " ", 1 );
  Memmove( cmd.mem + run_on_open.len + 1, ML( open->txt.filename ) );
  pagearray_t<u8> output;
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
    auto output_str = StringFromPlist( output );
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
  auto open = EditGetOpenedFile( edit, ML( file.obj ) );
  if( open ) {
    *opened_existing = 1;

  } else {
    *opened_existing = 0;

    auto elem = AddLast( edit.opened );
    open = &elem->value;
    open->unsaved = 0;
    open->horz_l = edit.horzfocus_l;
    Init( open->txt );
    TxtLoad( open->txt, file );
    open->time_lastwrite = file.time_lastwrite;

    auto mruelem = AddLast( edit.openedmru );
    mruelem->value = open;
    AssertCrash( edit.openedmru.len == edit.opened.len );

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
  MoveOpenedToFrontOfMru( edit, open );
}




// =================================================================================
// LIST VIEW

Inl void
ListviewResetCS( listview_t* lv )
{
  lv->cursor = 0;
  lv->scroll_target = 0;
}

Inl idx_t
ListviewForceInbounds(
  listview_t* lv,
  idx_t pos
  )
{
  auto r = MIN( pos, MAX( *lv->len, 1 ) - 1 );
  return r;
}

Inl void
ListviewFixupCS( listview_t* lv )
{
  lv->cursor = ListviewForceInbounds( lv, lv->cursor );
  lv->scroll_target = ListviewForceInbounds( lv, lv->scroll_target );
}

Inl idx_t
IdxFromFloatPos(
  listview_t* lv,
  f64 pos
  )
{
  auto v = MAX( pos, 0.0 );
  auto r = ListviewForceInbounds( lv, Cast( idx_t, v ) );
  return r;
}

Inl void
ListviewMakeCursorVisible( listview_t* lv )
{
  auto make_cursor_visible_radius = GetPropFromDb( f32, f32_make_cursor_visible_radius );
  auto nlines_radius = Cast( idx_t, make_cursor_visible_radius * lv->window_n_lines );

  // account for theoretical deletions by forcing inbounds here.
  ListviewFixupCS( lv );

  auto scroll_half = IdxFromFloatPos( lv, lv->scroll_start + 0.5 * lv->window_n_lines );
  auto yl = MAX( scroll_half, nlines_radius ) - nlines_radius;
  auto yr = ListviewForceInbounds( lv, scroll_half + nlines_radius );
  if( lv->cursor < yl ) {
    auto dy = yl - lv->cursor;
    scroll_half = MAX( scroll_half, dy ) - dy;
  }
  elif( lv->cursor > yr ) {
    auto dy = lv->cursor - yr;
    scroll_half = ListviewForceInbounds( lv, scroll_half + dy );
  }
  lv->scroll_target = scroll_half;

  ListviewFixupCS( lv );
}

Inl void
ListviewCursorU( listview_t* lv, idx_t misc )
{
  auto nlines = misc ? misc : 1;
  lv->cursor -= MIN( nlines, lv->cursor );
  ListviewMakeCursorVisible( lv );
}
Inl void
ListviewCursorD( listview_t* lv, idx_t misc )
{
  auto nlines = misc ? misc : 1;
  lv->cursor += nlines;
  ListviewFixupCS( lv );
  ListviewMakeCursorVisible( lv );
}
Inl void
ListviewScrollU( listview_t* lv, idx_t misc )
{
  auto nlines = misc ? misc : 1;
  lv->scroll_target -= MIN( nlines, lv->scroll_target );
}
Inl void
ListviewScrollD( listview_t* lv, idx_t misc )
{
  auto nlines = misc ? misc : 1;
  lv->scroll_target += nlines;
  ListviewFixupCS( lv );
}

Inl f32
MapIdxToT(
  listview_t* lv,
  idx_t idx
  )
{
  auto r = CLAMP( Cast( f32, idx ) / Cast( f32, *lv->len ), 0, 1 );
  return r;
}

Inl void
SetScrollPosFraction(
  listview_t* lv,
  f32 t
  )
{
  auto last_idx = MAX( 1, *lv->len ) - 1;
  auto pos = Round_u32_from_f32( t * last_idx );
  AssertCrash( pos <= last_idx );
  lv->scroll_target = pos;
}

Inl vec2<f32>
GetScrollPos( listview_t* lv )
{
  auto scroll_start_idx = IdxFromFloatPos( lv, lv->scroll_start );
  return _vec2<f32>(
    MapIdxToT( lv, scroll_start_idx ),
    MapIdxToT( lv, lv->scroll_end )
    );
}

// outputs the list entry span that the caller should render.
Inl void
ListviewUpdateScrollingAndRenderScrollbar(
  listview_t* lv,
  bool& target_valid,
  array_t<f32>& stream,
  font_t& font,
  rectf32_t& bounds,
  vec2<f32> zrange,
  f64 timestep,
  tslice_t<listview_rect_t>* lines
  )
{
  *lines = {};

  auto rgba_cursor_bkgd = GetPropFromDb( vec4<f32>, rgba_cursor_bkgd );
  auto rgba_scroll_btn = GetPropFromDb( vec4<f32>, rgba_scroll_btn );
  auto rgba_scroll_bkgd = GetPropFromDb( vec4<f32>, rgba_scroll_bkgd );

  auto line_h = FontLineH( font );
  auto px_space_advance = FontGetAdvance( font, ' ' );

  auto scroll_pct = GetPropFromDb( f32, f32_scroll_pct );
  auto px_scroll = scroll_pct * line_h;

  // leave a small gap on the left/right, so things aren't exactly at the screen edge.
  // esp. important for scrollbars, since window-resize covers some of the active area.
  bounds.p0.x = MIN( bounds.p0.x + 0.25f * px_space_advance, bounds.p1.x );
  bounds.p1.x = MAX( bounds.p1.x - 0.25f * px_space_advance, bounds.p0.x );

  auto nlines_screen_floored = Cast( idx_t, ( bounds.p1.y - bounds.p0.y ) / line_h );
  lv->pageupdn_distance = MAX( 1, nlines_screen_floored / 2 );
  lv->window_n_lines = nlines_screen_floored;

  // vertical scrolling
  auto half_nlines = 0.5 * lv->window_n_lines;
  auto scroll_half = lv->scroll_start + half_nlines; // intentionally not clamped, so distance forcing works.
  auto distance = Cast( f64, lv->scroll_target ) - scroll_half;
#if 0
  // instant scrolling.
  lv->scroll_start = CLAMP(
    lv->scroll_start + distance,
    -half_nlines,
    *lv->len + half_nlines
    );
#elif 0
  // failed attempt at timed interpolation. felt really terrible to use.
  if( lv->scroll_time < 1.0 ) {
    target_valid = 0;
  }
  auto scroll_start_target_anim = Cast( f64, lv->scroll_target ) - half_nlines;
  constant f64 anim_len = 0.2;
  lv->scroll_time = MIN( lv->scroll_time + timestep / anim_len, 1.0 );
  lv->scroll_start = CLAMP(
    lerp( lv->scroll_start_anim, scroll_start_target_anim, lv->scroll_time ),
    -half_nlines,
    *lv->len + half_nlines
    );
  printf( "start_anim=%F, target=%llu, time=%F, start=%F\n", lv->scroll_start_anim, lv->scroll_target, lv->scroll_time, lv->scroll_start );
#else
  // same force calculations as txt has.
  // note: i tried a trivial multistep Euler method here, but it didn't really improve stability.
  // TODO: try other forms of interpolation: midpoint method, RK4, or just explicit soln, which kinda sucks
  //   because of the extra state req'd.
  constant f64 mass = 1.0;
  constant f64 spring_k = 1000.0;
  static f64 friction_k = 2.2 * Sqrt64( mass * spring_k ); // 2 * Sqrt( m * k ) is critical damping, but we want a little overdamping.
  auto force_spring = spring_k * distance;
  auto force_fric = -friction_k * lv->scroll_vel.sum;
  auto force = force_spring + force_fric;
  auto accel = force / mass;
  auto delta_vel = timestep * accel;
  Add( lv->scroll_vel, delta_vel );
  if( ABS( lv->scroll_vel.sum ) <= 0.2 ) {
    // snap to 0 for small velocities to minimize pixel jitter.
    lv->scroll_vel = {};
  }
  if( ABS( lv->scroll_vel.sum ) > 0 ) {
    // invalidate cached target, since we know animation will require a re-render.
    target_valid = 0;
  }
  auto delta_pos = timestep * lv->scroll_vel.sum;
  lv->scroll_start = CLAMP(
    lv->scroll_start + delta_pos,
    -half_nlines,
    *lv->len + half_nlines
    );
#endif
  AssertCrash( lv->scroll_target <= *lv->len );
  auto scroll_start = IdxFromFloatPos( lv, lv->scroll_start );
  lv->scroll_end = MIN( *lv->len, scroll_start + lv->window_n_lines );
  auto nlines_render = MIN( 1 + lv->window_n_lines, *lv->len - scroll_start );

  // render the scrollbar.

  lv->has_scrollbar = ScrollbarVisible( bounds, px_scroll );
  lv->scroll_bounds_view_and_bar = bounds;
  if( lv->has_scrollbar ) {
    auto scroll_pos = GetScrollPos( lv );
    ScrollbarRender(
      stream,
      bounds,
      scroll_pos.x,
      scroll_pos.y,
      GetZ( zrange, editlayer_t::scroll_bkgd ),
      GetZ( zrange, editlayer_t::scroll_btn ),
      px_scroll,
      rgba_scroll_bkgd,
      rgba_scroll_btn,
      &lv->scroll_track,
      &lv->scroll_btn_up,
      &lv->scroll_btn_dn,
      &lv->scroll_btn_pos
      );

    bounds.p1.x = MAX( bounds.p1.x - px_scroll, bounds.p0.x );
  }

  lv->scroll_bounds_view = bounds;

  { // draw bkgd
    RenderQuad(
      stream,
      GetPropFromDb( vec4<f32>, rgba_text_bkgd ),
      bounds,
      bounds,
      GetZ( zrange, editlayer_t::bkgd )
      );
  }

  // shift line rendering down if we're scrolled up past the first one.
  if( nlines_render  &&  lv->scroll_start < 0 ) {
    auto nlines_prefix = Cast( idx_t, -lv->scroll_start );
    bounds.p0.y = MIN( bounds.p0.y + nlines_prefix * line_h, bounds.p1.y );
  }

  // cursor
  {
    if( scroll_start <= lv->cursor  &&  lv->cursor < lv->scroll_end ) {
      auto p0 = bounds.p0 + _vec2( 0.0f, line_h * ( lv->cursor - scroll_start ) );
      auto p1 = _vec2( bounds.p1.x, p0.y + line_h );
      RenderQuad(
        stream,
        rgba_cursor_bkgd,
        p0,
        p1,
        bounds,
        GetZ( zrange, editlayer_t::sel )
        );
    }
  }

  // rowrects for mouse hit testing.
  {
    Reserve( lv->rowrects, nlines_render );
    lv->rowrects.len = 0;
    For( i, 0, nlines_render ) {
      auto elem_p0 = bounds.p0 + _vec2<f32>( 0, line_h * i );
      auto elem_p1 = _vec2( bounds.p1.x, MIN( bounds.p1.y, elem_p0.y + line_h ) );
      auto rowrect = AddBack( lv->rowrects );
      rowrect->rect = _rect( elem_p0, elem_p1 );
      rowrect->row_idx = scroll_start + i;
      AssertCrash( rowrect->row_idx <= *lv->len );
    }

    *lines = SliceFromArray( lv->rowrects );
  }
}

Inl idx_t
ListviewMapMouseToCursor(
  listview_t* lv,
  vec2<s32> m
  )
{
  auto c_y = IdxFromFloatPos( lv, lv->scroll_start );
  auto mp = _vec2( Cast( f32, m.x ), Cast( f32, m.y ) );
  f32 min_distance = MAX_f32;
  constant f32 epsilon = 0.001f;
  FORLEN( rowrect, i, lv->rowrects )
    if( PtInBox( mp, rowrect->rect.p0, rowrect->rect.p1, epsilon ) ) {
      c_y = rowrect->row_idx;
      break;
    }
    auto distance = DistanceToBox( mp, rowrect->rect.p0, rowrect->rect.p1 );
    if( distance < min_distance ) {
      min_distance = distance;
      c_y = rowrect->row_idx;
    }
  }
  return c_y;
}

void
ListviewControlMouse(
  listview_t* lv,
  bool& target_valid,
  font_t& font,
  rectf32_t bounds,
  glwmouseevent_t type,
  glwmousebtn_t btn,
  vec2<s32> m,
  vec2<s32> raw_delta,
  s32 dwheel,
  bool* double_clicked_on_line
  )
{
  ProfFunc();

  *double_clicked_on_line = 0;

//  auto px_click_correct = _vec2<s8>(); // TODO: mouse control.
  auto scroll_nlines = GetPropFromDb( u8, u8_scroll_nlines );
  auto scroll_sign = GetPropFromDb( s8, s8_scroll_sign );
  auto dblclick_period_sec = GetPropFromDb( f64, f64_dblclick_period_sec );

  auto keymods = GlwKeyModifiersDown();
  bool mod_isdn = AnyDown( keymods );

  auto has_scrollbar = lv->has_scrollbar;
  auto btn_up = lv->scroll_btn_up;
  auto btn_dn = lv->scroll_btn_dn;
  auto btn_pos = lv->scroll_btn_pos;
  auto scroll_track = lv->scroll_track;
  auto scroll_t_mouse = GetScrollMouseT( m, scroll_track );
  auto bounds_view_and_bar = lv->scroll_bounds_view_and_bar;
  auto bounds_view = lv->scroll_bounds_view;

  if( has_scrollbar ) {

    if( GlwMouseInsideRect( m, btn_up ) ) {
      switch( type ) {
        case glwmouseevent_t::dn: break;
        case glwmouseevent_t::up: {
          auto t = MapIdxToT( lv, lv->scroll_target );
          SetScrollPosFraction( lv, CLAMP( t - 0.1f, 0, 1 ) );
          target_valid = 0;
        } break;
        case glwmouseevent_t::move: break;
        case glwmouseevent_t::wheelmove: break;
        default: UnreachableCrash();
      }
    }
    elif( GlwMouseInsideRect( m, btn_dn ) ) {
      switch( type ) {
        case glwmouseevent_t::dn: break;
        case glwmouseevent_t::up: {
          auto t = MapIdxToT( lv, lv->scroll_target );
          SetScrollPosFraction( lv, CLAMP( t + 0.1f, 0, 1 ) );
          target_valid = 0;
        } break;
        case glwmouseevent_t::move: break;
        case glwmouseevent_t::wheelmove: break;
        default: UnreachableCrash();
      }
    }
    elif( GlwMouseInsideRect( m, scroll_track ) ) {
      // note that btn_pos isn't really a button; it's just a rendering of where we are in the track.
      // it's more complicated + stateful to track a button that changes size as we scroll.
      // much easier to just do the linear mapping of the track, and it also feels nice to not have to
      // click exactly on a tiny button; the whole track works the same way.
      switch( type ) {
        case glwmouseevent_t::dn: {
          lv->scroll_grabbed = 1;
          SetScrollPosFraction( lv, CLAMP( scroll_t_mouse, 0, 1 ) );
          target_valid = 0;
        } break;
        case glwmouseevent_t::up: break;
        case glwmouseevent_t::move: break;
        case glwmouseevent_t::wheelmove: break;
        default: UnreachableCrash();
      }
    }
  }

  if( GlwMouseInsideRect( m, bounds_view ) ) {
    if( type == glwmouseevent_t::dn  &&  btn == glwmousebtn_t::l ) {
      // TODO: do a text_grabbed thing, where we allow multi-selections.
      lv->cursor = ListviewMapMouseToCursor( lv, m );
      bool same_cursor = ( lv->cursor == lv->dblclick.first_cursor );
      bool double_click = ( lv->dblclick.first_made & same_cursor );
      if( double_click ) {
        if( TimeSecFromClocks64( TimeClock() - lv->dblclick.first_clock ) <= dblclick_period_sec ) {
          lv->dblclick.first_made = 0;
          *double_clicked_on_line = 1;
        } else {
          lv->dblclick.first_clock = TimeClock();
        }
      } else {
        lv->dblclick.first_made = 1;
        lv->dblclick.first_clock = TimeClock();
        lv->dblclick.first_cursor = lv->cursor;
      }
      target_valid = 0;
    }
  }

  switch( type ) {
    case glwmouseevent_t::dn: break;

    case glwmouseevent_t::up: {
      if( lv->scroll_grabbed ) {
        lv->scroll_grabbed = 0;
      }
    } break;

    case glwmouseevent_t::move: {
      if( lv->scroll_grabbed ) {
        SetScrollPosFraction( lv, CLAMP( scroll_t_mouse, 0, 1 ) );
        target_valid = 0;
      }
      // TODO: text_grabbed thing.
    } break;

    case glwmouseevent_t::wheelmove: {
      if( GlwMouseInsideRect( m, bounds_view_and_bar )  &&  dwheel  &&  !mod_isdn ) {
        dwheel *= scroll_sign;
        dwheel *= scroll_nlines;
        if( dwheel < 0 ) {
          ListviewScrollU( lv, Cast( idx_t, -dwheel ) );
        } else {
          ListviewScrollD( lv, Cast( idx_t, dwheel ) );
        }
        target_valid = 0;
      }
    } break;

    default: UnreachableCrash();
  }
}





// =================================================================================
// FILE OPENER

Inl fileopener_oper_t*
FileopenerAddHistorical( fileopener_t& fo, fileopener_opertype_t type )
{
  // invalidate previous futures.
  AssertCrash( fo.history_idx <= fo.history.len );
  fo.history.len = fo.history_idx;

  fo.history_idx += 1;
  auto oper = AddBack( fo.history );
  oper->type = type;
  return oper;
}




Inl void
FileopenerSizestrFromSize( u8* dst, idx_t dst_len, idx_t* dst_size, u64 size )
{
#if 0
  static u64 gb = ( 1 << 30 );
  static u64 mb = ( 1 << 20 );
  static u64 kb = ( 1 << 10 );
  if( size > gb ) {
    size = Cast( u64, 0.5 + size / Cast( f64, gb ) );
    CsFrom_u64( dst, dst_len, size );
    CsAddBack( dst, Str( "g" ), 1 );
    *dst_size = CsLen( dst );
  } elif( size > mb ) {
    size = Cast( u64, 0.5 + size / Cast( f64, mb ) );
    CsFrom_u64( dst, dst_len, size );
    CsAddBack( dst, Str( "m" ), 1 );
    *dst_size = CsLen( dst );
  } elif( size > kb ) {
    size = Cast( u64, 0.5 + size / Cast( f64, kb ) );
    CsFrom_u64( dst, dst_len, size );
    CsAddBack( dst, Str( "k" ), 1 );
    *dst_size = CsLen( dst );
  } else {
    CsFrom_u64( dst, dst_len, size );
    CsAddBack( dst, Str( "b" ), 1 );
    *dst_size = CsLen( dst );
  }
#else
  CsFrom_u64( dst, dst_len, dst_size, size, 1 );
#endif
}


Inl void
FileopenerUpdateMatches( fileopener_t& fo )
{
  Reset( fo.matches_mem );
  Reset( fo.matches );
  fo.ignored_filetypes_list.len = 0;
  fo.included_filetypes_list.len = 0;
  fo.ignored_substrings_list.len = 0;

  AssertCrash( NLines( &fo.ignored_filetypes.buf ) == 1 );
  auto line = LineFromY( &fo.ignored_filetypes.buf, 0 );
  SplitBySpacesAndCopyContents(
    &fo.matches_mem,
    &fo.ignored_filetypes_list,
    ML( *line )
    );

  AssertCrash( NLines( &fo.included_filetypes.buf ) == 1 );
  line = LineFromY( &fo.included_filetypes.buf, 0 );
  SplitBySpacesAndCopyContents(
    &fo.matches_mem,
    &fo.included_filetypes_list,
    ML( *line )
    );

  AssertCrash( NLines( &fo.ignored_substrings.buf ) == 1 );
  line = LineFromY( &fo.ignored_substrings.buf, 0 );
  SplitBySpacesAndCopyContents(
    &fo.matches_mem,
    &fo.ignored_substrings_list,
    ML( *line )
    );

  auto key = AllocContents( &fo.query.buf, eoltype_t::crlf );
  bool must_match_key = key.len;

  if( fo.pool.totallen ) {
    auto pa_iter = MakeIteratorAtLinearIndex( fo.pool, 0 );
    For( i, 0, fo.pool.totallen ) {
      auto elem = GetElemAtIterator( fo.pool, pa_iter );
      pa_iter = IteratorMoveR( fo.pool, pa_iter );

      auto ext = FileExtension( ML( elem->name ) );

      bool include = 1;

      if( ext.len ) {
        // ignore files with extensions in the 'ignore' list.
        ForLen( j, fo.ignored_filetypes_list ) {
          auto filter = fo.ignored_filetypes_list.mem + j;
          if( CsEquals( ML( ext ), ML( *filter ), 0 ) ) {
            include = 0;
            break;
          }
        }
        if( !include ) {
          continue;
        }

        // ignore files with extensions that aren't in the 'include' list.
        // empty 'include' list means include everything.
        bool found = 0;
        ForLen( j, fo.included_filetypes_list ) {
          auto filter = fo.included_filetypes_list.mem + j;
          if( CsEquals( ML( ext ), ML( *filter ), 0 ) ) {
            found = 1;
            break;
          }
        }
        if( fo.included_filetypes_list.len  &&  !found ) {
          continue;
        }
      }

      // ignore things which match the 'ignored_substrings' list.
      ForLen( j, fo.ignored_substrings_list ) {
        auto filter = fo.ignored_substrings_list.mem + j;
        idx_t pos;
        if( CsIdxScanR( &pos, ML( elem->name ), 0, ML( *filter ), 0, 0 ) ) {
          include = 0;
          break;
        }
      }
      if( !include ) {
        continue;
      }

      // ignore things which don't match the 'query' key.
      if( must_match_key ) {
        idx_t pos;
        if( !CsIdxScanR( &pos, ML( elem->name ), 0, ML( key ), 0, 0 ) ) {
          continue;
        }
      }

      AssertCrash( include );
      auto instance = AddBack( fo.matches, 1 );
      *instance = elem;
    }
  }
  Free( key );

  ListviewFixupCS( &fo.listview );
  ListviewMakeCursorVisible( &fo.listview );
}

__MainTaskCompleted( MainTaskCompleted_FileopenerFillPool )
{
  ProfFunc();

  auto ac = Cast( asynccontext_fileopenerfillpool_t*, misc0 );
  auto fo = Cast( fileopener_t*, misc1 );

  AssertCrash( fo->ncontexts_active );
  fo->ncontexts_active -= 1;

  if( ac->pool.totallen ) {
    auto pa_iter = MakeIteratorAtLinearIndex( ac->pool, 0 );
    For( i, 0, ac->pool.totallen ) {
      auto elem = GetElemAtIterator( ac->pool, pa_iter );
      pa_iter = IteratorMoveR( ac->pool, pa_iter );

      auto instance = AddBack( fo->pool, 1 );
      *instance = *elem;
    }
  }

  FileopenerUpdateMatches( *fo );

  *target_valid = 0;
}



Inl
FS_ITERATOR( _IterFindDirsAndFiles )
{
  if( g_mainthread.signal_quit ) {
    return fsiter_result_t::stop;
  }

  auto ac = Cast( asynccontext_fileopenerfillpool_t*, misc );
  auto row = AddBack( ac->pool );

  // take the name only.
  auto cwd_len = ac->cwd.len + 1;
  AssertCrash( len >= cwd_len );
  len -= cwd_len;
  name += cwd_len;
  row->name.mem = AddPlist( ac->pool_mem, u8, 1, len );
  row->name.len = len;
  Memmove( row->name.mem, name, len );

  row->is_file = is_file;
  if( is_file ) {
    row->size = filesize;
    row->readonly = readonly;
    row->sizetxt.mem = AddPlist( ac->pool_mem, u8, 1, 32 );
    FileopenerSizestrFromSize( row->sizetxt.mem, 32, &row->sizetxt.len, filesize );
  } else {
    row->size = 0;
    row->readonly = 0;
    row->sizetxt.len = 0;
  }

  return fsiter_result_t::continue_;
}

// note we only push 1 of these at a time, since it probably doesn't make sense to parallelize FsIterate.
// we're primarily limited by OS filesys calls, so more than 1 thread probably doesn't make sense.
// but, this unblocks the main thread ui, which is good since this can be slow.
__AsyncTask( AsyncTask_FileopenerFillPool )
{
  ProfFunc();

  auto ac = Cast( asynccontext_fileopenerfillpool_t*, misc0 );
  auto fo = Cast( fileopener_t*, misc1 );

  Reset( ac->pool_mem );

  // TODO: add PushMainTaskCompleted incremental results into FsFindDirsAndFiles.
  Prof( foFill_FsIterate );
  FsIterate( ML( ac->cwd ), 1, _IterFindDirsAndFiles, ac );
  ProfClose( foFill_FsIterate );

  // note that we don't send signals back to the main thread in a more fine-grained fashion.
  // we just send the final signal here, after everything's been done.
  // this is because the ac's results datastructure is shared across all its results.

  // TODO: pass fine-grained results back to the main thread ?

  maincompletedqueue_entry_t entry;
  entry.FnMainTaskCompleted = MainTaskCompleted_FileopenerFillPool;
  entry.misc0 = misc0;
  entry.misc1 = misc1;
  entry.misc2 = 0;
  entry.time_generated = TimeTSC();
  PushMainTaskCompleted( taskthread, &entry );
}

Inl void
FileopenerFillPool( fileopener_t& fo )
{
  // prevent reentrancy while async stuff is executing
  if( fo.ncontexts_active ) {
    LogUI( "[EDIT] FileopenerFillPool already in progress!" );
    return;
  }

  Reset( fo.asynccontext_fillpool.pool );
  Reset( fo.asynccontext_fillpool.pool_mem );
  fo.asynccontext_fillpool.cwd.len = 0;

  Reset( fo.pool );
  Reset( fo.pool_mem );

  // add the pseudo up directory.
  {
    auto elem = AddBack( fo.pool );
    elem->name.len = 2;
    elem->name.mem = AddPlist( fo.pool_mem, u8, 1, elem->name.len );
    Memmove( elem->name.mem, Str( ".." ), 2 );
    elem->is_file = 0;
    elem->size = 0;
    elem->readonly = 0;
    elem->sizetxt.len = 0;
  }

  auto cwd = AllocContents( &fo.cwd.buf, eoltype_t::crlf );
  // TODO: user error if cwd is longer than fixed-size fsobj_t can handle.
  Memmove( AddBack( fo.asynccontext_fillpool.cwd, cwd.len ), ML( cwd ) );
  Free( cwd );

  fo.ncontexts_active += 1;

  asyncqueue_entry_t entry;
  entry.FnAsyncTask = AsyncTask_FileopenerFillPool;
  entry.misc0 = &fo.asynccontext_fillpool;
  entry.misc1 = &fo;
  entry.time_generated = TimeTSC();
  PushAsyncTask( 0, &entry );
}

//#if 0
//  auto cwd = AllocContents( &fo.cwd.buf, eoltype_t::crlf );
//
//  plist_t mem;
//  Init( mem, 32768 );
//  array_t<dir_or_file_t> objs;
//  Alloc( objs, fo.pool.capacity + 64 );
//  FsFindDirsAndFiles( objs, mem, ML( cwd ), 1 );
//  ForLen( i, objs ) {
//    auto obj = objs.mem + i;
//    auto elem = AddBack( fo.pool );
//    Init( *elem );
//    AssertCrash( obj->name.len >= cwd.len + 1 );
//    auto nameonly = obj->name.mem + cwd.len + 1;
//    auto nameonly_len = obj->name.len - cwd.len - 1;
//    elem->name.len = 0;
//    Memmove( AddBack( elem->name, nameonly_len ), nameonly, nameonly_len );
//    elem->is_file = obj->is_file;
//    if( obj->is_file ) {
//      auto file = FileOpen( obj->name.mem, obj->name.len, fileopen_t::only_existing, fileop_t::R, fileop_t::RW );
//      if( file.loaded ) {
//        elem->size = file.size;
//        elem->readonly = file.readonly;
//        FileopenerSizestrFromSize( elem->sizetxt.mem, Capacity( elem->sizetxt ), &elem->sizetxt.len, elem->size );
//      }
//      FileFree( file );
//    } else {
//      elem->size = 0;
//      elem->readonly = 0;
//      elem->sizetxt.len = 0;
//    }
//  }
//  Free( cwd );
//  Free( objs );
//  Kill( mem );
//#endif

__EditCmd( CmdMode_editfile_from_fileopener )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  if( !edit.opened.len ) {
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

Inl void
FileopenerChangeCwd( fileopener_t& fo, slice_t dst )
{
  CmdSelectAll( fo.cwd );
  CmdRemChL( fo.cwd );
  CmdAddString( fo.cwd, Cast( idx_t, dst.mem ), dst.len );
}

Inl void
FileopenerCwdUp( fileopener_t& fo )
{
  auto cwd = AllocContents( &fo.cwd.buf, eoltype_t::crlf );
  if( !cwd.len ) {
    return;
  }
  idx_t new_cwd_len;
  bool res = MemScanIdxRev( &new_cwd_len, ML( cwd ), "/", 1 );
  if( res ) {
    fsobj_t obj;
    obj.len = 0;
    Memmove( AddBack( obj, cwd.len ), ML( cwd ) );

    auto oper = FileopenerAddHistorical( fo, fileopener_opertype_t::changecwd );
    oper->changecwd.src = obj;

    idx_t nrem = cwd.len - new_cwd_len;
    RemBack( obj, nrem );

    oper->changecwd.dst = obj;

    FileopenerChangeCwd( fo, SliceFromArray( obj ) );
  }
  Free( cwd );
}

Inl void
FileopenerOpenRow( edit_t& edit, fileopener_row_t* row )
{
  fileopener_t& fo = edit.fileopener;
  fsobj_t name;
  name.len = 0;
  auto cwd = AllocContents( &fo.cwd.buf, eoltype_t::crlf );
  Memmove( AddBack( name, cwd.len ), ML( cwd ) );
  Memmove( AddBack( name ), "/", 1 );
  Memmove( AddBack( name, row->name.len ), ML( row->name ) );

  if( row->is_file ) {
#if USE_FILEMAPPED_OPEN
    auto file = FileOpenMappedExistingReadShareRead( ML( name ) );
#else
    auto file = FileOpen( ML( name ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
#endif
    if( file.loaded ) {
      edittxtopen_t* open = 0;
      bool opened_existing = 0;
      EditOpen( edit, file, &open, &opened_existing );
      AssertCrash( open );
      EditSetActiveTxt( edit, open );
      CmdMode_editfile_from_fileopener( edit );
    }
#if !USE_FILEMAPPED_OPEN
    FileFree( file );
#endif

  } else {
    bool up_dir = MemEqual( ML( row->name ), "..", 2 );
    if( up_dir ) {
      FileopenerCwdUp( fo );

    } else {
      fsobj_t fsobj;
      fsobj.len = 0;
      Memmove( AddBack( fsobj, cwd.len ), ML( cwd ) );

      auto oper = FileopenerAddHistorical( fo, fileopener_opertype_t::changecwd );
      oper->changecwd.src = fsobj;

      oper->changecwd.dst = name;

      FileopenerChangeCwd( fo, SliceFromArray( name ) );
    }

    FileopenerFillPool( fo );
    FileopenerUpdateMatches( fo );
    ListviewResetCS( &fo.listview );
  }

  Free( cwd );
}


__EditCmd( CmdFileopenerUpdateMatches )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  FileopenerUpdateMatches( fo );
}

__EditCmd( CmdFileopenerRefresh )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  FileopenerFillPool( fo );
  FileopenerUpdateMatches( fo );
}

__EditCmd( CmdFileopenerChoose )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  if( fo.matches.totallen ) {
    AssertCrash( fo.listview.cursor < fo.matches.totallen );
    auto row = *LookupElemByLinearIndex( fo.matches, fo.listview.cursor );
    FileopenerOpenRow( edit, row );
  }
}

__EditCmd( CmdFileopenerFocusD )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  fo.focus = NextValue( fo.focus );

  // save previous cwd, in case we fail to apply a changecwd.
  if( fo.focus == fileopenerfocus_t::dir ) {
    auto cwd = AllocContents( &fo.cwd.buf, eoltype_t::crlf );
    fo.last_cwd_for_changecwd_rollback.len = 0;
    Memmove( AddBack( fo.last_cwd_for_changecwd_rollback, cwd.len ), ML( cwd ) );
    Free( cwd );
  }
}

__EditCmd( CmdFileopenerFocusU )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  fo.focus = PreviousValue( fo.focus );

  // save previous cwd, in case we fail to apply a changecwd.
  if( fo.focus == fileopenerfocus_t::dir ) {
    auto cwd = AllocContents( &fo.cwd.buf, eoltype_t::crlf );
    fo.last_cwd_for_changecwd_rollback.len = 0;
    Memmove( AddBack( fo.last_cwd_for_changecwd_rollback, cwd.len ), ML( cwd ) );
    Free( cwd );
  }
}


__EditCmd( CmdFileopenerCursorU )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  ListviewCursorU( &fo.listview, misc );
}

__EditCmd( CmdFileopenerCursorD )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  ListviewCursorD( &fo.listview, misc );
}

__EditCmd( CmdFileopenerScrollU )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  ListviewScrollU( &fo.listview, misc );
}

__EditCmd( CmdFileopenerScrollD )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  ListviewScrollD( &fo.listview, misc );
}



__EditCmd( CmdFileopenerRecycle )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;

  if( !fo.matches.totallen ) {
    return;
  }

  AssertCrash( fo.listview.cursor < fo.matches.totallen );
  auto row = *LookupElemByLinearIndex( fo.matches, fo.listview.cursor );

  fsobj_t name;
  name.len = 0;
  auto cwd = AllocContents( &fo.cwd.buf, eoltype_t::crlf );
  Memmove( AddBack( name, cwd.len ), ML( cwd ) );
  Free( cwd );
  Memmove( AddBack( name ), "/", 1 );
  Memmove( AddBack( name, row->name.len ), ML( row->name ) );

  if( row->is_file ) {
    auto oper = FileopenerAddHistorical( fo, fileopener_opertype_t::delfile );
    oper->delfile = name;

    if( FileRecycle( ML( name ) ) ) {
      FileopenerFillPool( fo );
      FileopenerUpdateMatches( fo );
    } else {
      auto tmp = AllocCstr( ML( name ) );
      LogUI( "[DIR] Failed to delete file: \"%s\"!", tmp );
      MemHeapFree( tmp );
    }
  } else {
    auto oper = FileopenerAddHistorical( fo, fileopener_opertype_t::deldir );
    oper->deldir = name;

    if( DirRecycle( ML( name ) ) ) {
      FileopenerFillPool( fo );
      FileopenerUpdateMatches( fo );
    } else {
      auto tmp = AllocCstr( ML( name ) );
      LogUI( "[DIR] Failed to delete fo: \"%s\"!", tmp );
      MemHeapFree( tmp );
    }
  }
  ListviewFixupCS( &fo.listview );
}


__EditCmd( CmdFileopenerChangeCwdUp )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  FileopenerCwdUp( fo );
  FileopenerFillPool( fo );
  FileopenerUpdateMatches( fo );
  ListviewResetCS( &fo.listview );
}

__EditCmd( CmdFileopenerNewFile )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  u8* default_name = Str( "new_file" );
  idx_t default_name_len = CsLen( default_name );

  fsobj_t name;
  name.len = 0;
  auto cwd = AllocContents( &fo.cwd.buf, eoltype_t::crlf );
  Memmove( AddBack( name, cwd.len ), ML( cwd ) );
  Free( cwd );
  Memmove( AddBack( name ), "/", 1 );
  Memmove( AddBack( name, default_name_len ), default_name, default_name_len );
  Memmove( AddBack( name ), "0", 1 );
  Memmove( AddBack( name, 4 ), ".txt", 4 );

  u32 suffix_num = 0;
  idx_t last_suffix_len = 1;
  embeddedarray_t<u8, 64> suffix;
  Forever {
    bool exists = FileExists( ML( name ) );
    if( !exists ) {
      break;
    }
    RemBack( name, 4 + last_suffix_len );

    suffix_num += 1;
    CsFromIntegerU( suffix.mem, Capacity( suffix ), &suffix.len, suffix_num );
    last_suffix_len = suffix.len;

    Memmove( AddBack( name, suffix.len ), ML( suffix ) );
    Memmove( AddBack( name, 4 ), ".txt", 4 );
  }

  auto oper = FileopenerAddHistorical( fo, fileopener_opertype_t::newfile );
  oper->newfile = name;

  file_t file = FileOpen( ML( name ), fileopen_t::only_new, fileop_t::RW, fileop_t::R );
  AssertWarn( file.loaded );
  FileFree( file );
  FileopenerFillPool( fo );
  FileopenerUpdateMatches( fo );
}

__EditCmd( CmdFileopenerNewDir )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;
  u8* default_name = Str( "new_dir" );
  idx_t default_name_len = CsLen( default_name );

  fsobj_t name;
  name.len = 0;
  auto cwd = AllocContents( &fo.cwd.buf, eoltype_t::crlf );
  Memmove( AddBack( name, cwd.len ), ML( cwd ) );
  Free( cwd );
  Memmove( AddBack( name ), "/", 1 );
  Memmove( AddBack( name, default_name_len ), default_name, default_name_len );
  Memmove( AddBack( name ), "0", 1 );

  u32 suffix_num = 0;
  idx_t last_suffix_len = 1;
  embeddedarray_t<u8, 64> suffix;
  Forever {
    bool exists = DirExists( ML( name ) );
    if( !exists ) {
      break;
    }
    RemBack( name, last_suffix_len );

    suffix_num += 1;
    CsFromIntegerU( suffix.mem, Capacity( suffix ), &suffix.len, suffix_num );
    last_suffix_len = suffix.len;

    Memmove( AddBack( name, suffix.len ), ML( suffix ) );
  }

  auto oper = FileopenerAddHistorical( fo, fileopener_opertype_t::newdir );
  oper->newfile = name;

  bool created = DirCreate( ML( name ) );
  AssertWarn( created );
  FileopenerFillPool( fo );
  FileopenerUpdateMatches( fo );
}


__EditCmd( CmdMode_fileopener_renaming_from_fileopener )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  fileopener_t& fo = edit.fileopener;

  if( !fo.matches.totallen ) {
    return;
  }

  edit.mode = editmode_t::fileopener_renaming;

  AssertCrash( fo.listview.cursor < fo.matches.totallen );
  auto row = *LookupElemByLinearIndex( fo.matches, fo.listview.cursor );
  fo.renaming_row = fo.listview.cursor;

  CmdSelectAll( fo.renaming_txt );
  CmdAddString( fo.renaming_txt, Cast( idx_t, row->name.mem ), row->name.len );
  CmdSelectAll( fo.renaming_txt );
}

Inl void
FileopenerRename( edit_t& edit, slice_t& src, slice_t& dst )
{
  fileopener_t& fo = edit.fileopener;
  if( FileMove( ML( dst ), ML( src ) ) ) {
    FileopenerFillPool( fo );
    FileopenerUpdateMatches( fo );
  } else {
    auto tmp0 = AllocCstr( src );
    auto tmp1 = AllocCstr( dst );
    LogUI( "[DIR] Failed to rename: \"%s\" -> \"%s\"!", tmp0, tmp1 );
    MemHeapFree( tmp0 );
    MemHeapFree( tmp1 );
  }
}

__EditCmd( CmdFileopenerRenamingApply )
{
  AssertCrash( edit.mode == editmode_t::fileopener_renaming );
  fileopener_t& fo = edit.fileopener;

  AssertCrash( fo.renaming_row < fo.matches.totallen );
  auto row = *LookupElemByLinearIndex( fo.matches, fo.renaming_row );

  // TODO: prevent entry into mode fileopener_renaming, if the cursor is on '..'
  // Renaming the dummy parent fo does bad things!
  if( !MemEqual( "..", 2, ML( row->name ) ) ) {
    auto cwd = AllocContents( &fo.cwd.buf, eoltype_t::crlf );

    fsobj_t newname;
    newname.len = 0;
    Memmove( AddBack( newname, cwd.len ), ML( cwd ) );
    Memmove( AddBack( newname ), "/", 1 );
    auto newnameonly = AllocContents( &fo.renaming_txt.buf, eoltype_t::crlf );
    Memmove( AddBack( newname, newnameonly.len ), ML( newnameonly ) );
    Free( newnameonly );

    fsobj_t name;
    name.len = 0;
    Memmove( AddBack( name, cwd.len ), ML( cwd ) );
    Memmove( AddBack( name ), "/", 1 );
    Memmove( AddBack( name, row->name.len ), ML( row->name ) );

    auto oper = FileopenerAddHistorical( fo, fileopener_opertype_t::rename );
    oper->rename.src = name;
    oper->rename.dst = newname;

    auto src = SliceFromArray( name );
    auto dst = SliceFromArray( newname );
    FileopenerRename( edit, src, dst );

    Free( cwd );
  }

  fo.renaming_row = 0;
  CmdSelectAll( fo.renaming_txt );
  CmdRemChL( fo.renaming_txt );

  ListviewFixupCS( &fo.listview );

  edit.mode = editmode_t::fileopener;
}

__EditCmd( CmdFileopenerUndo ) // TODO: finish writing this.
{
  fileopener_t& fo = edit.fileopener;
//  DEBUG_PrintUndoRedo( dir, "PRE-UNDO\n" );
  AssertCrash( fo.history_idx <= fo.history.len );
  AssertCrash( fo.history_idx == fo.history.len  ||  fo.history.mem[fo.history_idx].type == fileopener_opertype_t::checkpt );
  if( !fo.history_idx ) {
    return;
  }

  bool loop = 1;
  while( loop ) {
    fo.history_idx -= 1;
    auto oper = fo.history.mem + fo.history_idx;

    switch( oper->type ) {
      case fileopener_opertype_t::checkpt: {
        loop = 0;
      } break;

      // undo this operation:

      case fileopener_opertype_t::rename: {
        slice_t src;
        slice_t dst;
        dst.mem = oper->rename.src.mem;
        dst.len = oper->rename.src.len;
        src.mem = oper->rename.dst.mem;
        src.len = oper->rename.dst.len;
        FileopenerRename( edit, src, dst );
      } break;
      case fileopener_opertype_t::changecwd: {
        slice_t dst;
        dst.mem = oper->changecwd.src.mem;
        dst.len = oper->changecwd.src.len;
        FileopenerChangeCwd( fo, dst );
      } break;
      case fileopener_opertype_t::delfile: {
      } break;
      case fileopener_opertype_t::deldir: {
      } break;
      case fileopener_opertype_t::newfile: {
      } break;
      case fileopener_opertype_t::newdir: {
      } break;
      default: UnreachableCrash();
    }
    CompileAssert( Cast( enum_t, fileopener_opertype_t::COUNT ) == 7 );
  }

  AssertCrash( fo.history_idx == fo.history.len  ||  fo.history.mem[fo.history_idx].type == fileopener_opertype_t::checkpt );
//  DEBUG_PrintUndoRedo( fo, "POST-UNDO\n" );
}

__EditCmd( CmdFileopenerRedo )
{
}






void
EditInit( edit_t& edit )
{
  Init( edit.mem, 32768 );

  // default to fileopener; the caller will change this if we open to file.
  edit.mode = editmode_t::fileopener;
  Init( edit.opened, &edit.mem );

  Init( edit.openedmru, &edit.mem );

  edit.active[0] = 0;
  edit.active[1] = 0;
  edit.horzview = 0;
  edit.horzfocus_l = 0;
  edit.rect_statusbar[0] = {};
  edit.rect_statusbar[1] = {};

  Alloc( edit.search_matches, 128 );
  Init( edit.opened_search );
  TxtLoadEmpty( edit.opened_search );
  edit.opened_cursor = 0;
  edit.opened_scroll_start = 0;
  edit.opened_scroll_end = 0;
  edit.nlines_screen = 0;

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
  ForList( elem, edit.opened ) {
    Kill( elem->value.txt );
  }
  Kill( edit.opened );

  Kill( edit.openedmru );

  Free( edit.search_matches );
  Kill( edit.opened_search );
  edit.opened_cursor = 0;
  edit.opened_scroll_start = 0;
  edit.opened_scroll_end = 0;
  edit.nlines_screen = 0;

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
__EditCmd( CmdMode_fileopener_from_fileopener_renaming ) { edit.mode = editmode_t::fileopener; }
__EditCmd( CmdMode_editfile_from_editfile_findrepl )     { edit.mode = editmode_t::editfile; }
__EditCmd( CmdMode_editfile_from_editfile_gotoline )     { edit.mode = editmode_t::editfile; }








// =================================================================================
// FINDIN FILES

Inl void
AsyncFileContentSearch( asynccontext_filecontentsearch_t* ac )
{
  ProfFunc();

  For( i, ac->filenames_start, ac->filenames_start + ac->filenames_count ) {
    auto obj = ac->filenames->mem + i;

    if( g_mainthread.signal_quit ) {
      return;
    }

    Prof( tmp_ContentSearchSingleFile );

    Prof( tmp_ApplyFilterFiletype );
    auto ext = FileExtension( ML( *obj ) );
    if( ext.len ) {
      // ignore files with extensions in the 'ignore' list.
      bool found = 0;
      FORLEN( filter, j, *ac->ignored_filetypes_list )
        if( CsEquals( ML( ext ), ML( *filter ), 0 ) ) {
          found = 1;
          break;
        }
      }
      if( found ) {
        continue;
      }

      // ignore files with extensions that aren't in the 'include' list.
      // empty 'include' list means include everything.
      found = 0;
      FORLEN( filter, j, *ac->included_filetypes_list )
        if( CsEquals( ML( ext ), ML( *filter ), 0 ) ) {
          found = 1;
          break;
        }
      }
      if( ac->included_filetypes_list->len  &&  !found ) {
        continue;
      }
    }
    ProfClose( tmp_ApplyFilterFiletype );

    Prof( tmp_FileOpen );
    file_t file = FileOpen( ML( *obj ), fileopen_t::only_existing, fileop_t::R, fileop_t::RW );
    ProfClose( tmp_FileOpen );
    // TODO: log when !file.loaded
    if( file.loaded ) {
      // collect all instances of the key.

#if USE_BUF_FOR_FILECONTENTSEARCH
      buf_t buf;
      Init( &buf );
      eoltype_t eoltype;
      BufLoad( &buf, &file, &eoltype );
      // TODO: close file after loading? we don't need to writeback or anything.
      bool found = 1;
      u32 x = 0;
      u32 y = 0;
      while( found ) {
        u32 x_found;
        u32 y_found;
        FindFirstInlineR(
          &buf,
          x,
          y,
          ML( ac->key ),
          &x_found,
          &y_found,
          &found,
          ac->case_sens,
          ac->word_boundary
          );
        if( found ) {
          auto line = LineFromY( &buf, y_found );
          Prof( tmp_AddMatch );
          auto instance = AddBack( ac->matches, 1 );
          instance->name = *obj;
          instance->l_x = x_found;
          instance->l_y = y_found;
          instance->r_x = x_found + ac->key.len;
          instance->r_y = y_found;
          instance->match_len = ac->key.len;
          auto bol_whitespace = CursorSkipSpacetabR( ML( *line ), 0 );
          auto eol_whitespace = CursorSkipSpacetabL( ML( *line ), line->len );
          // min in case search query contains whitespace.
          auto sample_start = MIN( instance->l_x, bol_whitespace );
          AssertCrash( sample_start <= instance->l_x );
          // samples don't scroll, so c_max_line_len should be wide enough for anyone.
          auto sample_end = MIN( sample_start + c_max_line_len, eol_whitespace );
          AssertCrash( sample_start <= sample_end );
          auto sample_len = sample_end - sample_start;
          // min in case pos_match_l spans beyond sample_end
          instance->sample_match_offset = MIN( instance->l_x - sample_start, sample_len );
          // min in case pos_match_l spans beyond sample_end
          instance->sample_match_len = MIN( instance->match_len, sample_len - instance->sample_match_offset );
          instance->sample.len = sample_len;
          instance->sample.mem = AddPlist( ac->mem, u8, 1, instance->sample.len );
          AssertCrash( sample_start <= instance->l_x );
          Memmove( instance->sample.mem, line->mem + sample_start, instance->sample.len );
          x = x_found;
          y = y_found;
          CursorCharR( &buf, &x, &y );
          ProfClose( tmp_AddMatch );
        }
      }

      Kill( &buf );
#else
      Prof( tmp_FileAlloc );
      string_t mem = FileAlloc( file );
      ProfClose( tmp_FileAlloc );
      Prof( tmp_ContentSearch );
      bool found = 1;
      idx_t pos = 0;
      while( found ) {
        idx_t res = 0;
        found = CsIdxScanR( &res, ML( mem ), pos, ML( ac->key ), ac->case_sens, ac->word_boundary );
        if( found ) {
          Prof( tmp_AddMatch );
          pos = res;

          auto instance = AddBack( ac->matches, 1 );
          instance->name = *obj;
          instance->pos_match_l = pos;
          instance->pos_match_r = pos + ac->key.len;
          instance->match_len = ac->key.len;
          auto bol = CursorStopAtNewlineL( ML( mem ), pos );
          auto eol = CursorStopAtNewlineR( ML( mem ), pos );
          auto bol_skip_whitespace = CursorSkipSpacetabR( ML( mem ), bol );
          auto eol_skip_whitespace = CursorSkipSpacetabL( ML( mem ), eol );
          // min in case search query contains whitespace.
          auto sample_start = MIN( instance->pos_match_l, bol_skip_whitespace );
          AssertCrash( sample_start <= instance->pos_match_l );
          // samples don't scroll, so c_max_line_len should be wide enough for anyone.
          auto sample_end = MIN( sample_start + c_max_line_len, eol_skip_whitespace );
          auto sample_len = sample_end - sample_start;
          AssertCrash( sample_start <= sample_end );
          // min in case pos_match_l spans beyond sample_end
          instance->sample_match_offset = MIN( instance->pos_match_l - sample_start, sample_len );
          // min in case pos_match_l spans beyond sample_end
          instance->sample_match_len = MIN( instance->match_len, sample_len - instance->sample_match_offset );
          instance->sample.len = sample_len;
          instance->sample.mem = AddPlist( ac->mem, u8, 1, instance->sample.len );
          AssertCrash( sample_start <= instance->pos_match_l );
          Memmove( instance->sample.mem, mem.mem + sample_start, instance->sample.len );

          pos = CursorCharR( ML( mem ), pos );
          ProfClose( tmp_AddMatch );
        }
      }
      ProfClose( tmp_ContentSearch );
      Free( mem );
#endif
    }
    FileFree( file );
  }
}

__MainTaskCompleted( MainTaskCompleted_FileContentSearch )
{
  ProfFunc();

  auto ac = Cast( asynccontext_filecontentsearch_t*, misc0 );
  auto fif = Cast( findinfiles_t*, misc1 );

  fif->progress_num_files += ac->filenames_count;

  if( ac->matches.totallen ) {
    auto pa_iter = MakeIteratorAtLinearIndex( ac->matches, 0 );
    For( i, 0, ac->matches.totallen ) {
      auto elem = GetElemAtIterator( ac->matches, pa_iter );
      pa_iter = IteratorMoveR( ac->matches, pa_iter );

      auto instance = AddBack( fif->matches, 1 );
      *instance = *elem;
    }
  }

  *target_valid = 0;
}

__AsyncTask( AsyncTask_FileContentSearch )
{
  ProfFunc();

  auto ac = Cast( asynccontext_filecontentsearch_t*, misc0 );
  auto fif = Cast( findinfiles_t*, misc1 );

  AsyncFileContentSearch( ac );

  // note that we don't send signals back to the main thread in a more fine-grained fashion.
  // we just send the final signal here, after everything's been done.
  // this is because the ac's results datastructure is shared across all its results.
  // our strategy of splitting into small, separate ac's at the start seems to work well enough.
  maincompletedqueue_entry_t entry;
  entry.FnMainTaskCompleted = MainTaskCompleted_FileContentSearch;
  entry.misc0 = misc0;
  entry.misc1 = misc1;
  entry.misc2 = 0;
  entry.time_generated = TimeTSC();
  PushMainTaskCompleted( taskthread, &entry );
}


__EditCmd( CmdFindinfilesRefresh )
{
  Prof( tmp_CmdFindinfilesRefresh );
  AssertCrash( edit.mode == editmode_t::findinfiles );

  auto& fif = edit.findinfiles;

  // prevent reentrancy while async stuff is executing
  if( fif.progress_num_files < fif.progress_max ) {
    LogUI( "[EDIT] FindinFilesRefresh already in progress!" );
    return;
  }

  ForLen( i, fif.asynccontexts ) {
    auto ac = fif.asynccontexts.mem + i;
    Kill( ac->mem );
    Kill( ac->matches );
  }
  fif.asynccontexts.len = 0;

  fif.filenames.len = 0;
  fif.ignored_filetypes_list.len = 0;
  fif.included_filetypes_list.len = 0;
  Free( fif.key );
  Free( fif.matches_dir );

  Reset( fif.matches );
  Reset( fif.mem );
  ListviewResetCS( &fif.listview );

  fif.key = AllocContents( &fif.query.buf, eoltype_t::crlf );

  if( fif.key.len ) {
    fif.matches_dir = AllocContents( &fif.dir.buf, eoltype_t::crlf );

    Prof( tmp_MakeFilterFiletypes );

    AssertCrash( NLines( &fif.ignored_filetypes.buf ) == 1 );
    auto line = LineFromY( &fif.ignored_filetypes.buf, 0 );
    SplitBySpacesAndCopyContents(
      &fif.mem,
      &fif.ignored_filetypes_list,
      ML( *line )
      );

    AssertCrash( NLines( &fif.included_filetypes.buf ) == 1 );
    line = LineFromY( &fif.included_filetypes.buf, 0 );
    SplitBySpacesAndCopyContents(
      &fif.mem,
      &fif.included_filetypes_list,
      ML( *line )
      );

    ProfClose( tmp_MakeFilterFiletypes );

    // this call is ~3% of the cost of this function, after the OS / filesys caches filesys metadata.
    // so, we'll do this part on the main thread, then fan out.
    //
    // if we really need to, we could multithread this:
    // - have some input queues of directories to process.
    // - have some result queues of files.
    // - each thread will:
    //   - pop off a directory.
    //   - enumerate all child files and directories.
    //   - push files onto result queues.
    //   - push child directories onto input queues.
    // the lopsided nature of directories would make scheduling pretty hard though.
    Prof( tmp_FsFindFiles );
    fif.filenames.len = 0;
    FsFindFiles( fif.filenames, fif.mem, ML( fif.matches_dir ), 1 );
    ProfClose( tmp_FsFindFiles );

    fif.progress_num_files = 0;
    fif.progress_max = fif.filenames.len;

    // PERF: optimize chunksize and per-thread-queue sizes
    // PERF: filesize-level chunking, for a more even distribution.
    constant idx_t chunksize = 1;
    auto quo = fif.filenames.len / chunksize;
    auto rem = fif.filenames.len % chunksize;
    Reserve( fif.asynccontexts, quo + 1 );
    For( i, 0, quo + 1 ) {
      auto count = ( i == quo )  ?  rem  :  chunksize;
      if( count ) {
        auto ac = AddBack( fif.asynccontexts );
        ac->filenames = &fif.filenames;
        ac->filenames_start = i * chunksize;
        ac->filenames_count = count;
        ac->ignored_filetypes_list = &fif.ignored_filetypes_list;
        ac->included_filetypes_list = &fif.included_filetypes_list;
        ac->key = Slice32FromString( fif.key );
        ac->case_sens = fif.case_sens;
        ac->word_boundary = fif.word_boundary;
        // keep these small, since we're using chunksize = 1 above.
        Init( ac->mem, 256 );
        Init( ac->matches, 16 );

        asyncqueue_entry_t entry;
        entry.FnAsyncTask = AsyncTask_FileContentSearch;
        entry.misc0 = ac;
        entry.misc1 = &fif;
        entry.time_generated = TimeTSC();
        PushAsyncTask( i, &entry );
      }
    }
  }
}


Inl void
SelectAllQueryText( findinfiles_t& fif )
{
  if( !IsEmpty( &fif.query.buf ) ) {
    CmdSelectAll( fif.query );
  }
  if( !IsEmpty( &fif.replacement.buf ) ) {
    CmdSelectAll( fif.replacement );
  }
}

__EditCmd( CmdMode_findinfiles_from_editfile )
{
  AssertCrash( edit.mode == editmode_t::editfile );
  edit.mode = editmode_t::findinfiles;
  auto& fif = edit.findinfiles;
  SelectAllQueryText( fif );
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
  if( !edit.opened.len ) {
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
  SelectAllQueryText( fif );
//  CmdFindinfilesRefresh( edit );
}

__EditCmd( CmdFindinfilesChoose )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  if( !fif.matches.totallen ) {
    return;
  }
  auto foundinfile = LookupElemByLinearIndex( fif.matches, fif.listview.cursor );
#if USE_FILEMAPPED_OPEN
  auto file = FileOpenMappedExistingReadShareRead( ML( foundinfile->name ) );
#else
  auto file = FileOpen( ML( foundinfile->name ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
#endif
  if( file.loaded ) {
    edittxtopen_t* open = 0;
    bool opened_existing = 0;
    EditOpen( edit, file, &open, &opened_existing );
    AssertCrash( open );
    EditSetActiveTxt( edit, open );
    CmdMode_editfile_from_findinfiles( edit );
#if USE_BUF_FOR_FILECONTENTSEARCH
    txt_setsel_t setsel;
    setsel.x_start = foundinfile->l_x;
    setsel.y_start = foundinfile->l_y;
    setsel.x_end = foundinfile->r_x;
    setsel.y_end = foundinfile->r_y;
    // TODO: update the foundinfile positions when buf contents change.
    // for now, just clamp them to inbounds so it puts you near.
    ForceInbounds( &open->txt.buf, &setsel.x_start, &setsel.y_start );
    ForceInbounds( &open->txt.buf, &setsel.x_end, &setsel.y_end );
    CmdSetSelection( open->txt, Cast( idx_t, &setsel ), 0 );
#else
    // note that it's useful to allow Choose to work for unsaved files.
    // this is so that if you do a Replace of some kind, you can still use the findinfiles to
    // navigate around. it won't be entirely accurate, but it's usually close enough for now.

    // at some point i'll think through how we want to solve line/char drift.
    // maybe unique line ids, and we tie the search result to the line id?
    // that would solve the line drift problem, but not the character drift.
    // i guess since we have a unique id line system, we could add a listening system, and
    // update the search result when that unique line id changes.
    // i like this unique line id idea much better than the persistent ranges i had previously.
    // now the only questions are:
    // - what operations create a new unique line id?
    // - what operations destroy the unique line id?
    // - what operations preserve the unique line id?
    // probably with enough effort, we could make buf_t do reasonable things here, since its line-based.

    if( !opened_existing ) {
      // WARNING!
      // we're using the open->txt.buf.orig_file_contents, meaning we rely on
      // that not changing after we open this file above! this is so that we can make
      // a faster findinfiles that doesn't require opening a buf_t for everything.
      // here is where we'll convert our linear file map into a 2d range.
      //
      auto file_contents = open->txt.buf.orig_file_contents;
      auto match_l = foundinfile->pos_match_l;
      auto match_len = foundinfile->match_len;
      auto match_r = match_l + match_len;
      AssertCrash( match_r <= file_contents.len );
      auto lineno_before_match = CountNewlines( file_contents.mem, match_l );
      auto lineno_in_match = CountNewlines( file_contents.mem + match_l, match_len );
      auto bol_l = CursorStopAtNewlineL( ML( file_contents ), match_l );
      auto bol_r = CursorStopAtNewlineL( ML( file_contents ), match_r );
      txt_setsel_t setsel;
      setsel.x_start = Cast( u32, match_l - bol_l );
      setsel.y_start = lineno_before_match;
      setsel.x_end = Cast( u32, match_r - bol_r );
      setsel.y_end = lineno_before_match + lineno_in_match;
      // TODO: update the foundinfile positions when buf contents change.
      // for now, just clamp them to inbounds so it puts you near.
      ForceInbounds( &open->txt.buf, &setsel.x_start, &setsel.y_start );
      ForceInbounds( &open->txt.buf, &setsel.x_end, &setsel.y_end );
      CmdSetSelection( open->txt, Cast( idx_t, &setsel ), 0 );
    }
    else {
      // we've got an open buf_t before this, so we'll use the eoltype to estimate
      // our 1d -> 2d conversion.
      // this can be inaccurate if the file has mixed eols on disk, which is rare enough
      // for me to ignore this for now. we'll see how common this actually is in practice.
      // once i've got ui to set eoltype, eliminating the mixing, this should be fine.
      //
      auto match_l = foundinfile->pos_match_l;
      auto match_len = foundinfile->match_len;
      auto match_r = match_l + match_len;
      auto cmatch_l = match_l;
      auto cmatch_r = match_r;
      u32 match_l_x = 0;
      u32 match_l_y = 0;
      u32 match_r_x = 0;
      u32 match_r_y = 0;
      auto eol_len = EolString( open->txt.eoltype ).len;
      bool converted_l = 0;
      bool converted_r = 0;
      FORALLLINES( &open->txt.buf, line, y )
        if( converted_r ) break;
        auto line_and_eol_len = line->len + eol_len;
        if( !converted_l ) {
          if( cmatch_l >= line_and_eol_len ) {
            cmatch_l -= line_and_eol_len;
          }
          else { // cmatch_l < line_and_eol_len
            converted_l = 1;
            AssertCrash( cmatch_l <= MAX_u32 );
            match_l_x = Cast( u32, cmatch_l );
            match_l_y = y;
          }
        }
        if( !converted_r ) {
          if( cmatch_r >= line_and_eol_len ) {
            cmatch_r -= line_and_eol_len;
          }
          else { // cmatch_r < line_and_eol_len
            converted_r = 1;
            AssertCrash( cmatch_r <= MAX_u32 );
            match_r_x = Cast( u32, cmatch_r );
            match_r_y = y;
          }
        }
      }
      if( !converted_l  ||  !converted_r ) {
        auto slice = SliceFromArray( open->txt.filename );
        auto cstr = AllocCstr( slice );
        LogUI( "[EDIT] FifChoose couldn't find within: \"%s\"", cstr );
        MemHeapFree( cstr );
      }
      else {
        txt_setsel_t setsel;
        setsel.x_start = match_l_x;
        setsel.y_start = match_l_y;
        setsel.x_end = match_r_x;
        setsel.y_end = match_r_y;
        // TODO: update the foundinfile positions when buf contents change.
        // for now, just clamp them to inbounds so it puts you near.
        ForceInbounds( &open->txt.buf, &setsel.x_start, &setsel.y_start );
        ForceInbounds( &open->txt.buf, &setsel.x_end, &setsel.y_end );
        CmdSetSelection( open->txt, Cast( idx_t, &setsel ), 0 );
      }
    }
#endif
  }
#if !USE_FILEMAPPED_OPEN
  FileFree( file );
#endif
}

__EditCmd( CmdFindinfilesFocusD )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  fif.focus = NextValue( fif.focus );
}

__EditCmd( CmdFindinfilesFocusU )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  fif.focus = PreviousValue( fif.focus );
}

__EditCmd( CmdFindinfilesCursorU )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  ListviewCursorU( &fif.listview, misc );
}

__EditCmd( CmdFindinfilesCursorD )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  ListviewCursorD( &fif.listview, misc );
}

__EditCmd( CmdFindinfilesScrollU )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  ListviewScrollU( &fif.listview, misc );
}

__EditCmd( CmdFindinfilesScrollD )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  ListviewScrollD( &fif.listview, misc );
}

// TODO: group foundinfile_t's by filename, so we don't emit the same message a bunch of times for the same file.
Inl void
ReplaceInFile( edit_t& edit, foundinfile_t* match, slice_t query, slice_t replacement )
{
  auto open = EditGetOpenedFile( edit, ML( match->name ) );
  bool opened_existing = 0;
  if( open ) {
    opened_existing = 1;
  }
  else {
#if USE_FILEMAPPED_OPEN
    auto file = FileOpenMappedExistingReadShareRead( ML( match->name ) );
#else
    auto file = FileOpen( ML( match->name ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
#endif
    if( file.loaded ) {
      EditOpen( edit, file, &open, &opened_existing );
      AssertCrash( open );
      AssertCrash( !opened_existing );
    } else {
      auto cstr = AllocCstr( match->name );
      LogUI( "[EDIT] ReplaceInFile failed to load file: \"%s\"", cstr );
      MemHeapFree( cstr );
    }
#if !USE_FILEMAPPED_OPEN
    FileFree( file );
#endif
  }
  if( open ) {
#if USE_BUF_FOR_FILECONTENTSEARCH
    txt_setsel_t setsel;
    setsel.x_start = match->l_x;
    setsel.y_start = match->l_y;
    setsel.x_end = match->r_x;
    setsel.y_end = match->r_y;
    CmdSetSelection( open->txt, Cast( idx_t, &setsel ), 0 );

    // WARNING! Duplicated below.
    auto contents = AllocSelection( open->txt, eoltype_t::crlf );
    if( CsEquals( ML( contents ), ML( query ), 1 ) ) {
      CmdAddString( open->txt, Cast( idx_t, replacement.mem ), replacement.len );
      open->unsaved = 1;
    } else {
      auto cstr0 = AllocCstr( query );
      auto cstr1 = AllocCstr( contents );
      LogUI( "[EDIT] ReplaceInFile failed, query \"%s\" didn't match contents \"%s\"!", cstr0, cstr1 );
      MemHeapFree( cstr0 );
      MemHeapFree( cstr1 );
    }
    Free( contents );
#else
    // note that it's useful to allow Replace to work for unsaved files.
    // this is so that you can do replaces one by one, and still use the findinfiles to
    // navigate around. it won't be entirely accurate, but it's usually close enough for now.

    if( !opened_existing ) {
      // WARNING!
      // we're using the open->txt.buf.orig_file_contents, meaning we rely on
      // that not changing after we open this file above! this is so that we can make
      // a faster findinfiles that doesn't require opening a buf_t for everything.
      // here is where we'll convert our linear file map into a 2d range.
      //
      auto file_contents = open->txt.buf.orig_file_contents;
      auto match_l = match->pos_match_l;
      auto match_len = match->match_len;
      auto match_r = match_l + match_len;
      AssertCrash( match_r <= file_contents.len );
      auto lineno_before_match = CountNewlines( file_contents.mem, match_l );
      auto lineno_in_match = CountNewlines( file_contents.mem + match_l, match_len );
      auto bol_l = CursorStopAtNewlineL( ML( file_contents ), match_l );
      auto bol_r = CursorStopAtNewlineL( ML( file_contents ), match_r );
      txt_setsel_t setsel;
      setsel.x_start = Cast( u32, match_l - bol_l );
      setsel.y_start = lineno_before_match;
      setsel.x_end = Cast( u32, match_r - bol_r );
      setsel.y_end = lineno_before_match + lineno_in_match;
      // TODO: update the foundinfile positions when buf contents change.
      // for now, just clamp them to inbounds so it puts you near.
      ForceInbounds( &open->txt.buf, &setsel.x_start, &setsel.y_start );
      ForceInbounds( &open->txt.buf, &setsel.x_end, &setsel.y_end );
      CmdSetSelection( open->txt, Cast( idx_t, &setsel ), 0 );

      // WARNING! Duplicated above and below.
      auto contents = AllocSelection( open->txt, eoltype_t::crlf );
      if( CsEquals( ML( contents ), ML( query ), 1 ) ) {
        CmdAddString( open->txt, Cast( idx_t, replacement.mem ), replacement.len );
        open->unsaved = 1;
      } else {
        auto cstr0 = AllocCstr( query );
        auto cstr1 = AllocCstr( contents );
        LogUI( "[EDIT] ReplaceInFile failed, query \"%s\" didn't match contents \"%s\"!", cstr0, cstr1 );
        MemHeapFree( cstr0 );
        MemHeapFree( cstr1 );
      }
      Free( contents );
    }
    else {
      // we've got an open buf_t before this, so we'll use the eoltype to estimate
      // our 1d -> 2d conversion.
      // this can be inaccurate if the file has mixed eols on disk, which is rare enough
      // for me to ignore this for now. we'll see how common this actually is in practice.
      // once i've got ui to set eoltype, eliminating the mixing, this should be fine.
      //
      auto match_l = match->pos_match_l;
      auto match_len = match->match_len;
      auto match_r = match_l + match_len;
      auto cmatch_l = match_l;
      auto cmatch_r = match_r;
      u32 match_l_x = 0;
      u32 match_l_y = 0;
      u32 match_r_x = 0;
      u32 match_r_y = 0;
      auto eol_len = EolString( open->txt.eoltype ).len;
      bool converted_l = 0;
      bool converted_r = 0;
      FORALLLINES( &open->txt.buf, line, y )
        if( converted_r ) break;
        auto line_and_eol_len = line->len + eol_len;
        if( !converted_l ) {
          if( cmatch_l >= line_and_eol_len ) {
            cmatch_l -= line_and_eol_len;
          }
          else { // cmatch_l < line_and_eol_len
            converted_l = 1;
            AssertCrash( cmatch_l <= MAX_u32 );
            match_l_x = Cast( u32, cmatch_l );
            match_l_y = y;
          }
        }
        if( !converted_r ) {
          if( cmatch_r >= line_and_eol_len ) {
            cmatch_r -= line_and_eol_len;
          }
          else { // cmatch_r < line_and_eol_len
            converted_r = 1;
            AssertCrash( cmatch_r <= MAX_u32 );
            match_r_x = Cast( u32, cmatch_r );
            match_r_y = y;
          }
        }
      }
      if( !converted_l  ||  !converted_r ) {
        auto slice = SliceFromArray( open->txt.filename );
        auto cstr = AllocCstr( slice );
        LogUI( "[EDIT] ReplaceInFile couldn't find within: \"%s\"", cstr );
        MemHeapFree( cstr );
      }
      else {
        txt_setsel_t setsel;
        setsel.x_start = match_l_x;
        setsel.y_start = match_l_y;
        setsel.x_end = match_r_x;
        setsel.y_end = match_r_y;
        // TODO: update the foundinfile positions when buf contents change.
        // for now, just clamp them to inbounds so it puts you near.
        ForceInbounds( &open->txt.buf, &setsel.x_start, &setsel.y_start );
        ForceInbounds( &open->txt.buf, &setsel.x_end, &setsel.y_end );
        CmdSetSelection( open->txt, Cast( idx_t, &setsel ), 0 );

        // WARNING! Duplicated above.
        auto contents = AllocSelection( open->txt, eoltype_t::crlf );
        if( CsEquals( ML( contents ), ML( query ), 1 ) ) {
          CmdAddString( open->txt, Cast( idx_t, replacement.mem ), replacement.len );
          open->unsaved = 1;
        } else {
          auto cstr0 = AllocCstr( query );
          auto cstr1 = AllocCstr( contents );
          LogUI( "[EDIT] ReplaceInFile failed, query \"%s\" didn't match contents \"%s\"!", cstr0, cstr1 );
          MemHeapFree( cstr0 );
          MemHeapFree( cstr1 );
        }
        Free( contents );
      }
    }
#endif
  }
}

__EditCmd( CmdFindinfilesReplaceAtCursor )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  if( !fif.matches.totallen ) {
    return;
  }
  auto replacement = AllocContents( &fif.replacement.buf, eoltype_t::crlf );
  auto query = AllocContents( &fif.query.buf, eoltype_t::crlf );
  auto match = LookupElemByLinearIndex( fif.matches, fif.listview.cursor );
  ReplaceInFile( edit, match, SliceFromString( query ), SliceFromString( replacement ) );
  Free( replacement );
  Free( query );
}

__EditCmd( CmdFindinfilesReplaceAll )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  auto replacement = AllocContents( &fif.replacement.buf, eoltype_t::crlf );
  auto query = AllocContents( &fif.query.buf, eoltype_t::crlf );
  ReverseForPrev( page, fif.matches.current_page ) {
    ReverseForLen( i, *page ) {
      auto match = Cast( foundinfile_t*, page->mem ) + i;
      ReplaceInFile( edit, match, SliceFromString( query ), SliceFromString( replacement ) );
    }
  }
  Free( replacement );
  Free( query );
}

__EditCmd( CmdFindinfilesToggleCaseSens )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  fif.case_sens = !fif.case_sens;
}

__EditCmd( CmdFindinfilesToggleWordBoundary )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  fif.word_boundary = !fif.word_boundary;
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




Inl void
Save( edit_t& edit, edittxtopen_t* open )
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
  } else {
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

__EditCmd( CmdSave )
{
  ProfFunc();
  auto open = GetActiveOpen( edit );
  if( !open ) {
    return;
  }
  Save( edit, open );
}

__EditCmd( CmdSaveAll )
{
  ProfFunc();
  ForList( elem, edit.opened ) {
    auto open = &elem->value;
    Save( edit, open );
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
    if( !IsNumber( gotoline.mem[i] ) ) {
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
    MoveOpenedToFrontOfMru( edit, open );
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

Inl void
MakeOpenedCursorVisible( edit_t& edit )
{
  edit.opened_scroll_start = MIN( edit.opened_scroll_start, MAX( edit.search_matches.len, 1 ) - 1 );
  edit.opened_cursor = MIN( edit.opened_cursor, MAX( edit.search_matches.len, 1 ) - 1 );

  bool offscreen_u = edit.opened_scroll_start && ( edit.opened_cursor < edit.opened_scroll_start );
  bool offscreen_d = edit.opened_scroll_end && ( edit.opened_scroll_end <= edit.opened_cursor );
  if( offscreen_u ) {
    auto dlines = edit.opened_scroll_start - edit.opened_cursor;
    edit.opened_scroll_start -= dlines;
  } elif( offscreen_d ) {
    auto dlines = edit.opened_cursor - edit.opened_scroll_end + 1;
    edit.opened_scroll_start += dlines;
  }
}

__EditCmd( CmdSwitchopenedCursorU )
{
  AssertCrash( edit.mode == editmode_t::switchopened );
  auto nlines = misc ? misc : 1;
  edit.opened_cursor -= MIN( nlines, edit.opened_cursor );
  MakeOpenedCursorVisible( edit );
}

__EditCmd( CmdSwitchopenedCursorD )
{
  AssertCrash( edit.mode == editmode_t::switchopened );
  auto nlines = misc ? misc : 1;
  edit.opened_cursor += nlines;
  edit.opened_cursor = MIN( edit.opened_cursor, MAX( edit.search_matches.len, 1 ) - 1 );
  MakeOpenedCursorVisible( edit );
}

__EditCmd( CmdSwitchopenedScrollU )
{
  AssertCrash( edit.mode == editmode_t::switchopened );
  auto nlines = misc ? misc : 1;
  edit.opened_scroll_start -= MIN( nlines, edit.opened_scroll_start );
}

__EditCmd( CmdSwitchopenedScrollD )
{
  AssertCrash( edit.mode == editmode_t::switchopened );
  auto nlines = misc ? misc : 1;
  edit.opened_scroll_start += nlines;
  edit.opened_scroll_start = MIN( edit.opened_scroll_start, MAX( edit.search_matches.len, 1 ) - 1 );
}

__EditCmd( CmdSwitchopenedMakeCursorPresent )
{
  AssertCrash( edit.mode == editmode_t::switchopened );
  edit.opened_cursor = edit.opened_scroll_start;
  CmdSwitchopenedCursorD( edit, edit.nlines_screen / 2 );
}

__EditCmd( CmdUpdateSearchMatches )
{
  AssertCrash( edit.mode == editmode_t::switchopened );
  edit.search_matches.len = 0;
  auto key = AllocContents( &edit.opened_search.buf, eoltype_t::crlf );
  if( !key.len ) {
    ForList( elem, edit.openedmru ) {
      auto open = elem->value;
      *AddBack( edit.search_matches ) = open;
    }
  } else {
    ForList( elem, edit.openedmru ) {
      auto open = elem->value;
      idx_t pos;
      if( CsIdxScanR( &pos, ML( open->txt.filename ), 0, ML( key ), 0, 0 ) ) {
        *AddBack( edit.search_matches ) = open;
      }
    }
  }
  Free( key );
  edit.opened_scroll_start = MIN( edit.opened_scroll_start, MAX( edit.search_matches.len, 1 ) - 1 );
  edit.opened_cursor = MIN( edit.opened_cursor, MAX( edit.search_matches.len, 1 ) - 1 );
  MakeOpenedCursorVisible( edit );
}

__EditCmd( CmdMode_switchopened_from_editfile )
{
  AssertCrash( edit.mode == editmode_t::editfile );
  edit.mode = editmode_t::switchopened;
  CmdUpdateSearchMatches( edit );
  edit.opened_scroll_start = 0;
  edit.opened_cursor = 0;

  // since the search_matches list is populated from openedmru, it's in mru order.
  // to allow quick change to the second mru entry ( the first is already the active txt ),
  // put the cursor over the second mru entry.
  if( edit.search_matches.len > 1 ) {
    edit.opened_cursor += 1;
  }
}

__EditCmd( CmdMode_editfile_from_switchopened )
{
  AssertCrash( edit.mode == editmode_t::switchopened );
  edit.mode = editmode_t::editfile;
  edit.search_matches.len = 0;
  CmdSelectAll( edit.opened_search );
  CmdRemChL( edit.opened_search );
}

Inl fsobj_t*
EditGetOpenedSelection( edit_t& edit )
{
  AssertCrash( edit.opened_cursor < edit.search_matches.len );
  auto open = edit.search_matches.mem[edit.opened_cursor];
  return &open->txt.filename;
}

__EditCmd( CmdSwitchopenedChoose )
{
  ProfFunc();
  AssertCrash( edit.mode == editmode_t::switchopened );
  if( !edit.search_matches.len ) {
    return;
  }
  auto obj = EditGetOpenedSelection( edit );
  if( obj ) {
#if USE_FILEMAPPED_OPEN
    auto file = FileOpenMappedExistingReadShareRead( ML( *obj ) );
#else
    auto file = FileOpen( ML( *obj ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
#endif
    if( file.loaded ) {
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
  }
}

__EditCmd( CmdSwitchopenedCloseFile )
{
  ProfFunc();
  AssertCrash( edit.mode == editmode_t::switchopened );
  AssertCrash( edit.openedmru.len == edit.opened.len );
  if( !edit.search_matches.len ) {
    return;
  }
  auto file = EditGetOpenedSelection( edit );
  AssertCrash( file );
  auto open = EditGetOpenedFile( edit, file->mem, file->len );
  AssertCrash( open );
  Save( edit, open );
  if( !open->unsaved ) {
    bool mruremoved = 0;
    ForList( elem, edit.openedmru ) {
      if( elem->value == open ) {
        Rem( edit.openedmru, elem );
        mruremoved = 1;
        break;
      }
    }
    AssertCrash( mruremoved );
    Kill( open->txt );
    bool removed = 0;
    ForList( elem, edit.opened ) {
      if( &elem->value == open ) {
        Rem( edit.opened, elem );
        removed = 1;
        break;
      }
    }
    AssertCrash( removed );
    For( i, 0, 2 ) {
      if( edit.active[i] == open ) {
        auto elem = edit.openedmru.first;
        edit.active[i] = elem  ?  elem->value  :  0;
      }
    }
    if( !edit.opened.len ) {
      CmdMode_fileopener_from_switchopened( edit );
    } else {
      CmdUpdateSearchMatches( edit );
    }
  }
  AssertCrash( edit.openedmru.len == edit.opened.len );
}






// =================================================================================
// EXTERNAL CHANGES

Inl void
CmdCheckForExternalChanges( edit_t& edit )
{
  ProfFunc();
  ForList( elem, edit.opened ) {
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
  Save( edit, open );
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
  array_t<f32>& stream,
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
  array_t<f32>& stream,
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
  array_t<f32>& stream,
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
  array_t<f32>& stream,
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

Enumc( dirlayer_t )
{
  bkgd,
  sel,
  txt,

  COUNT
};

void
FileopenerRender(
  edit_t& edit,
  bool& target_valid,
  array_t<f32>& stream,
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
  ProfFunc();

  fileopener_t& fo = edit.fileopener;

  AssertCrash( edit.mode == editmode_t::fileopener  ||  edit.mode == editmode_t::fileopener_renaming );

  auto timestep = MIN( timestep_realtime, timestep_fixed );

  auto rgba_text = GetPropFromDb( vec4<f32>, rgba_text );
  auto rgba_cursor_size_text = GetPropFromDb( vec4<f32>, rgba_cursor_size_text );
  auto rgba_size_text = GetPropFromDb( vec4<f32>, rgba_size_text );
  auto rgba_cursor_text = GetPropFromDb( vec4<f32>, rgba_cursor_text );
  auto rgba_scroll_btn = GetPropFromDb( vec4<f32>, rgba_scroll_btn );
  auto px_column_spacing = GetPropFromDb( f32, f32_px_column_spacing );
  auto spaces_per_tab = GetPropFromDb( u8, u8_spaces_per_tab );

  auto line_h = FontLineH( font );
  auto px_space_advance = FontGetAdvance( font, ' ' );

  static const auto label_dir = SliceFromCStr( "Dir: " );
  auto label_dir_w = LayoutString( font, spaces_per_tab, ML( label_dir ) );

  static const auto label_ignore_ext = SliceFromCStr( "Ext Ignore: " );
  auto label_ignore_ext_w = LayoutString( font, spaces_per_tab, ML( label_ignore_ext ) );

  static const auto label_include_ext = SliceFromCStr( "Ext Include: " );
  auto label_include_ext_w = LayoutString( font, spaces_per_tab, ML( label_include_ext ) );

  static const auto label_ignore_substring = SliceFromCStr( "Substring Ignore: " );
  auto label_ignore_substring_w = LayoutString( font, spaces_per_tab, ML( label_ignore_substring ) );

  static const auto label_find = SliceFromCStr( "Find: " );
  auto label_find_w = LayoutString( font, spaces_per_tab, ML( label_find ) );

  auto maxlabelw = MAX5(
    label_dir_w,
    label_ignore_ext_w,
    label_include_ext_w,
    label_ignore_substring_w,
    label_find_w
    );

  #define DRAW_TEXTBOXLINE( _txt, _label, _labelw, _maxlabelw, _infocus ) \
    DrawString( \
      stream, \
      font, \
      bounds.p0 + _vec2( _maxlabelw - _labelw, 0.0f ), \
      GetZ( zrange, editlayer_t::txt ), \
      bounds, \
      rgba_text, \
      spaces_per_tab, \
      ML( _label ) \
      ); \
    TxtRenderSingleLineSubset( \
      _txt, \
      stream, \
      font, \
      0, \
      _rect( bounds.p0 + _vec2( _maxlabelw, 0.0f ), bounds.p1 ), \
      ZRange( zrange, editlayer_t::txt ), \
      0, \
      _infocus, \
      _infocus \
      ); \
    bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y ); \



  // draw cwd
  DRAW_TEXTBOXLINE(
    fo.cwd,
    label_dir,
    label_dir_w,
    maxlabelw,
    ( fo.focus == fileopenerfocus_t::dir )
    );

  // ignored_filetypes bar
  DRAW_TEXTBOXLINE(
    fo.ignored_filetypes,
    label_ignore_ext,
    label_ignore_ext_w,
    maxlabelw,
    ( fo.focus == fileopenerfocus_t::ignored_filetypes )
    );

  // included_filetypes bar
  DRAW_TEXTBOXLINE(
    fo.included_filetypes,
    label_include_ext,
    label_include_ext_w,
    maxlabelw,
    ( fo.focus == fileopenerfocus_t::included_filetypes )
    );

  // ignored_substrings bar
  DRAW_TEXTBOXLINE(
    fo.ignored_substrings,
    label_ignore_substring,
    label_ignore_substring_w,
    maxlabelw,
    ( fo.focus == fileopenerfocus_t::ignored_substrings )
    );

  // search bar
  DRAW_TEXTBOXLINE(
    fo.query,
    label_find,
    label_find_w,
    maxlabelw,
    ( fo.focus == fileopenerfocus_t::query )
    );

  // result count
  if( fo.ncontexts_active ) {
    static const auto label = SliceFromCStr( "scanning directory..." );
    auto label_w = LayoutString( font, spaces_per_tab, ML( label ) );

    DrawString(
      stream,
      font,
      AlignCenter( bounds, label_w ),
      GetZ( zrange, editlayer_t::txt ),
      bounds,
      rgba_text,
      spaces_per_tab,
      ML( label )
      );

    bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );
  }
  else {
    static const auto label = SliceFromCStr( " results" );
    auto label_w = LayoutString( font, spaces_per_tab, ML( label ) );

    u8 count[128];
    idx_t count_len = 0;
    CsFromIntegerU( count, _countof( count ), &count_len, fo.matches.totallen, 1 );
    auto count_w = LayoutString( font, spaces_per_tab, count, count_len );

    DrawString(
      stream,
      font,
      AlignCenter( bounds, label_w + count_w ),
      GetZ( zrange, editlayer_t::txt ),
      bounds,
      rgba_text,
      spaces_per_tab,
      count, count_len
      );

    DrawString(
      stream,
      font,
      AlignCenter( bounds, label_w + count_w ) + _vec2( count_w, 0.0f ),
      GetZ( zrange, editlayer_t::txt ),
      bounds,
      rgba_text,
      spaces_per_tab,
      ML( label )
      );

    bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );
  }

  tslice_t<listview_rect_t> lines;
  ListviewUpdateScrollingAndRenderScrollbar(
    &fo.listview,
    target_valid,
    stream,
    font,
    bounds,
    zrange, // TODO: GetZ this?
    timestep,
    &lines
    );

  if( !lines.len ) {
    static const auto label_nothing_found = SliceFromCStr( "--- nothing found ---" );
    _RenderEmptyView( stream, font, bounds, zrange, label_nothing_found );
    return;
  }

  auto first_line = lines.mem[0].row_idx;
  auto nlines_render = lines.len;

  // elem sizes
  f32 max_sizetxt_w = 0;
  {
    {
      auto pa_iter = MakeIteratorAtLinearIndex( fo.matches, first_line );
      For( i, 0, nlines_render ) {
        auto elem = *GetElemAtIterator( fo.matches, pa_iter );
        pa_iter = IteratorMoveR( fo.matches, pa_iter );

        auto sizetxt_w = LayoutString( font, spaces_per_tab, ML( elem->sizetxt ) );
        max_sizetxt_w = MAX( max_sizetxt_w, sizetxt_w );
      }
    }
    {
      auto pa_iter = MakeIteratorAtLinearIndex( fo.matches, first_line );
      For( i, 0, nlines_render ) {
        auto elem = *GetElemAtIterator( fo.matches, pa_iter );
        pa_iter = IteratorMoveR( fo.matches, pa_iter );

        auto sizetxt_w = LayoutString( font, spaces_per_tab, ML( elem->sizetxt ) ); // PERF: cache this?
        auto elem_origin = _vec2(
          bounds.p1.x - sizetxt_w - 0.25f * px_space_advance,
          bounds.p0.y + line_h * i
          );
        DrawString(
          stream,
          font,
          elem_origin,
          GetZ( zrange, editlayer_t::txt ),
          bounds,
          ( fo.listview.cursor == first_line + i )  ?  rgba_cursor_size_text  :  rgba_size_text,
          spaces_per_tab,
          ML( elem->sizetxt )
          );
      }
    }
    bounds.p1.x = MAX( bounds.p0.x, bounds.p1.x - max_sizetxt_w - 0.25f * px_space_advance - px_column_spacing );
  }

  // elem readonlys
  {
    static const auto readonly_label = Str( "readonly" );
    static const idx_t readonly_label_len = CsLen( readonly_label );
    auto readonly_w = LayoutString( font, spaces_per_tab, readonly_label, readonly_label_len );
    bool readonly_visible = 0;
    {
      auto pa_iter = MakeIteratorAtLinearIndex( fo.matches, first_line );
      For( i, 0, nlines_render ) {
        auto elem = *GetElemAtIterator( fo.matches, pa_iter );
        pa_iter = IteratorMoveR( fo.matches, pa_iter );

        auto elem_origin = _vec2( bounds.p1.x - readonly_w, bounds.p0.y + line_h * i );
        if( elem->readonly ) {
          readonly_visible = 1;
          DrawString(
            stream,
            font,
            elem_origin,
            GetZ( zrange, editlayer_t::txt ),
            bounds,
            ( fo.listview.cursor == first_line + i )  ?  rgba_cursor_size_text  :  rgba_size_text,
            spaces_per_tab,
            readonly_label, readonly_label_len
            );
        }
      }
    }
    if( readonly_visible ) {
      bounds.p1.x = MAX( bounds.p0.x, bounds.p1.x - readonly_w - px_column_spacing );
    }
  }

  // elem names
  {
    {
      auto pa_iter = MakeIteratorAtLinearIndex( fo.matches, first_line );
      For( i, 0, nlines_render ) {
        auto elem = *GetElemAtIterator( fo.matches, pa_iter );
        pa_iter = IteratorMoveR( fo.matches, pa_iter );

        auto elem_p0 = bounds.p0 + _vec2<f32>( 0, line_h * i );
        auto elem_p1 = _vec2( bounds.p1.x, MIN( bounds.p1.y, elem_p0.y + line_h ) );
        if( edit.mode == editmode_t::fileopener_renaming  &&
            fo.renaming_row == first_line + i )
        {
          TxtRenderSingleLineSubset(
            fo.renaming_txt,
            stream,
            font,
            0,
            _rect(elem_p0, elem_p1 ),
            ZRange( zrange, editlayer_t::txt ),
            1,
            1,
            1
            );
        }
        else {
          idx_t last_slash = 0;
          auto found = CsIdxScanL( &last_slash, ML( elem->name ), elem->name.len, '/' );
          if( found ) {
            slice_t path;
            path.mem = elem->name.mem;
            path.len = last_slash + 1;
            slice_t name;
            name.mem = elem->name.mem + last_slash + 1;
            name.len = elem->name.len - last_slash - 1;
            auto path_w = LayoutString( font, spaces_per_tab, ML( path ) );
            DrawString(
              stream,
              font,
              elem_p0,
              GetZ( zrange, editlayer_t::txt ),
              bounds,
              rgba_size_text,
              spaces_per_tab,
              ML( path )
              );
            DrawString(
              stream,
              font,
              elem_p0 + _vec2( path_w, 0.0f ),
              GetZ( zrange, editlayer_t::txt ),
              bounds,
              ( fo.listview.cursor == first_line + i )  ?  rgba_cursor_text  :  rgba_text,
              spaces_per_tab,
              ML( name )
              );
          }
          else {
            DrawString(
              stream,
              font,
              elem_p0,
              GetZ( zrange, editlayer_t::txt ),
              bounds,
              ( fo.listview.cursor == first_line + i )  ?  rgba_cursor_text  :  rgba_text,
              spaces_per_tab,
              ML( elem->name )
              );
          }
        }
      }
    }
  }
}

void
FindinfilesRender(
  edit_t& edit,
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

  AssertCrash( edit.mode == editmode_t::findinfiles );

  findinfiles_t& fif = edit.findinfiles;

  auto timestep = MIN( timestep_realtime, timestep_fixed );

  auto spaces_per_tab = GetPropFromDb( u8, u8_spaces_per_tab );
  auto rgba_text = GetPropFromDb( vec4<f32>, rgba_text );
  auto rgba_size_text = GetPropFromDb( vec4<f32>, rgba_size_text );
  auto rgba_cursor_text = GetPropFromDb( vec4<f32>, rgba_cursor_text );
  auto rgba_wordmatch_bkgd = GetPropFromDb( vec4<f32>, rgba_wordmatch_bkgd );

  auto line_h = FontLineH( font );

  { // case sens
    auto bind0 = GetPropFromDb( glwkeybind_t, keybind_findinfiles_toggle_case_sensitive );
    auto key0 = KeyStringFromGlw( bind0.key );
    AssertCrash( !key0.mem[key0.len] ); // cstr generated by compiler.
    auto label = AllocString( "Case sensitive: %u -- Press [ %s ] to toggle.", fif.case_sens, key0.mem );
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
    auto bind0 = GetPropFromDb( glwkeybind_t, keybind_findinfiles_toggle_word_boundary );
    auto key0 = KeyStringFromGlw( bind0.key );
    AssertCrash( !key0.mem[key0.len] ); // cstr generated by compiler.
    auto label = AllocString( "Whole word: %u -- Press [ %s ] to toggle.", fif.word_boundary, key0.mem );
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

  static const auto label_dir = SliceFromCStr( "Dir: " );
  auto label_dir_w = LayoutString( font, spaces_per_tab, ML( label_dir ) );

  static const auto label_ignore = SliceFromCStr( "Ext Ignore: " );
  auto label_ignore_w = LayoutString( font, spaces_per_tab, ML( label_ignore ) );

  static const auto label_include_ext = SliceFromCStr( "Ext Include: " );
  auto label_include_ext_w = LayoutString( font, spaces_per_tab, ML( label_include_ext ) );

  static const auto label_find = SliceFromCStr( "Find: " );
  auto label_find_w = LayoutString( font, spaces_per_tab, ML( label_find ) );

  static const auto label_repl = SliceFromCStr( "Repl: " );
  auto label_repl_w = LayoutString( font, spaces_per_tab, ML( label_repl ) );

  auto maxlabelw = MAX5(
    label_dir_w,
    label_ignore_w,
    label_include_ext_w,
    label_find_w,
    label_repl_w
    );

  // dir bar
  DRAW_TEXTBOXLINE(
    fif.dir,
    label_dir,
    label_dir_w,
    maxlabelw,
    ( fif.focus == findinfilesfocus_t::dir )
    );

  // ignored_filetypes bar
  DRAW_TEXTBOXLINE(
    fif.ignored_filetypes,
    label_ignore,
    label_ignore_w,
    maxlabelw,
    ( fif.focus == findinfilesfocus_t::ignored_filetypes )
    );

  // included_filetypes bar
  DRAW_TEXTBOXLINE(
    fif.included_filetypes,
    label_include_ext,
    label_include_ext_w,
    maxlabelw,
    ( fif.focus == findinfilesfocus_t::included_filetypes )
    );

  // search bar
  DRAW_TEXTBOXLINE(
    fif.query,
    label_find,
    label_find_w,
    maxlabelw,
    ( fif.focus == findinfilesfocus_t::query )
    );

  // replace bar
  DRAW_TEXTBOXLINE(
    fif.replacement,
    label_repl,
    label_repl_w,
    maxlabelw,
    ( fif.focus == findinfilesfocus_t::replacement )
    );

  { // result count
    if( fif.progress_num_files < fif.progress_max ) {
      static const auto label = SliceFromCStr( " % files scanned..." );
      auto label_w = LayoutString( font, spaces_per_tab, ML( label ) );

      u8 count[128];
      idx_t count_len = 0;
      auto pct = ( fif.progress_num_files * 100.0f ) / fif.progress_max;
      count_len = sprintf_s( Cast( char*, count ), _countof( count ), "%.2f", pct );
      auto count_w = LayoutString( font, spaces_per_tab, count, count_len );

      DrawString(
        stream,
        font,
        AlignCenter( bounds, count_w + label_w ),
        GetZ( zrange, editlayer_t::txt ),
        bounds,
        rgba_text,
        spaces_per_tab,
        count, count_len
        );

      DrawString(
        stream,
        font,
        AlignCenter( bounds, count_w + label_w ) + _vec2( count_w, 0.0f ),
        GetZ( zrange, editlayer_t::txt ),
        bounds,
        rgba_text,
        spaces_per_tab,
        ML( label )
        );
    }
    else {
      static const auto label = SliceFromCStr( " results" );
      auto label_w = LayoutString( font, spaces_per_tab, ML( label ) );

      u8 count[128];
      idx_t count_len = 0;
      CsFromIntegerU( AL( count ), &count_len, fif.matches.totallen, 1 );
      auto count_w = LayoutString( font, spaces_per_tab, count, count_len );

      DrawString(
        stream,
        font,
        AlignCenter( bounds, count_w + label_w ),
        GetZ( zrange, editlayer_t::txt ),
        bounds,
        rgba_text,
        spaces_per_tab,
        count, count_len
        );

      DrawString(
        stream,
        font,
        AlignCenter( bounds, count_w + label_w ) + _vec2( count_w, 0.0f ),
        GetZ( zrange, editlayer_t::txt ),
        bounds,
        rgba_text,
        spaces_per_tab,
        ML( label )
        );
    }

    bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y );
  }

  tslice_t<listview_rect_t> lines;
  ListviewUpdateScrollingAndRenderScrollbar(
    &fif.listview,
    target_valid,
    stream,
    font,
    bounds,
    zrange, // TODO: GetZ this?
    timestep,
    &lines
    );

  if( !lines.len ) {
    static const auto label_no_results = SliceFromCStr( "--- no results ---" );
    _RenderEmptyView( stream, font, bounds, zrange, label_no_results );
    return;
  }

  auto first_line = lines.mem[0].row_idx;
  auto nlines_render = lines.len;

  fontlayout_t layout;
  FontInit( layout );

  auto pa_iter = MakeIteratorAtLinearIndex( fif.matches, first_line );
  For( i, 0, nlines_render ) {
    auto elem = GetElemAtIterator( fif.matches, pa_iter );
    pa_iter = IteratorMoveR( fif.matches, pa_iter );

    auto is_cursor = first_line + i == fif.listview.cursor;

    // elem names
    {
      auto elem_p0 = bounds.p0 + _vec2( 0.0f, line_h * i );
      auto elem_p1 = _vec2( bounds.p1.x, elem_p0.y + line_h );
      slice_t name = elem->name;
      auto prefix_len = fif.matches_dir.len + 1; // include forwslash
      AssertCrash( name.len >= prefix_len );
      name.mem += prefix_len;
      name.len -= prefix_len;

      idx_t last_slash = 0;
      auto found = CsIdxScanL( &last_slash, ML( name ), name.len, '/' );
      if( found ) {
        slice_t path;
        path.mem = name.mem;
        path.len = last_slash + 1;
        slice_t nameonly;
        nameonly.mem = name.mem + last_slash + 1;
        nameonly.len = name.len - last_slash - 1;
        auto path_w = LayoutString( font, spaces_per_tab, ML( path ) );
        DrawString(
          stream,
          font,
          elem_p0,
          GetZ( zrange, editlayer_t::txt ),
          bounds,
          rgba_size_text,
          spaces_per_tab,
          ML( path )
          );
        DrawString(
          stream,
          font,
          elem_p0 + _vec2( path_w, 0.0f ),
          GetZ( zrange, editlayer_t::txt ),
          bounds,
          is_cursor  ?  rgba_cursor_text  :  rgba_text,
          spaces_per_tab,
          ML( nameonly )
          );
      }
      else {
        DrawString(
          stream,
          font,
          elem_p0,
          GetZ( zrange, editlayer_t::txt ),
          bounds,
          is_cursor  ?  rgba_cursor_text  :  rgba_text,
          spaces_per_tab,
          ML( name )
          );
      }

      auto name_w = LayoutString( font, spaces_per_tab, ML( name ) );
      auto space_w = LayoutString( font, spaces_per_tab, Str( "   " ), 3 );

      elem_p0.x += name_w + space_w;

      FontAddLayoutLine( font, layout, ML( elem->sample ), spaces_per_tab );
      auto line = i;
      auto match_start = FontSumAdvances( layout, line, 0, elem->sample_match_offset );
      auto match_w = FontSumAdvances( layout, line, elem->sample_match_offset, elem->sample_match_len );
      auto match_end = match_start + match_w;

      RenderQuad(
        stream,
        rgba_wordmatch_bkgd,
        elem_p0 + _vec2( match_start, 0.0f ),
        _vec2( elem_p0.x + match_end, elem_p1.y ),
        bounds,
        GetZ( zrange, editlayer_t::sel )
        );

      RenderText(
        stream,
        font,
        layout,
        elem_p0,
        GetZ( zrange, editlayer_t::txt ),
        bounds,
        is_cursor  ?  rgba_cursor_text  :  rgba_text,
        line,
        0, elem->sample.len
        );
    }
  }

  FontKill( layout );
}


void
EditRender(
  edit_t& edit,
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

  auto rgba_text = GetPropFromDb( vec4<f32>, rgba_text );
  auto rgba_cursor_bkgd = GetPropFromDb( vec4<f32>, rgba_cursor_bkgd );
  auto rgba_cursor_size_text = GetPropFromDb( vec4<f32>, rgba_cursor_size_text );
  auto rgba_size_text = GetPropFromDb( vec4<f32>, rgba_size_text );
  auto rgba_cursor_text = GetPropFromDb( vec4<f32>, rgba_cursor_text );
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
      static const auto header_len = CsLen( header );
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

      edit.opened_scroll_start = MIN( edit.opened_scroll_start, MAX( edit.search_matches.len, 1 ) - 1 );
      edit.opened_cursor = MIN( edit.opened_cursor, MAX( edit.search_matches.len, 1 ) - 1 );

      auto nlines_screen_max = Cast( idx_t, ( bounds.p1.y - bounds.p0.y ) / line_h );
      if( nlines_screen_max ) {
        nlines_screen_max -= 1; // for search bar.
      }
      edit.nlines_screen = MIN( nlines_screen_max, edit.search_matches.len - edit.opened_scroll_start );
      edit.opened_scroll_end = edit.opened_scroll_start + edit.nlines_screen;

      // render search bar.
      static const auto search = Str( "Search: " );
      static const auto search_len = CsLen( search );
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
        edit.opened_search,
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
      if( edit.opened_scroll_start <= edit.opened_cursor  &&  edit.opened_cursor < edit.opened_scroll_end ) {
        auto p0 = bounds.p0 + _vec2( 0.0f, line_h * ( edit.opened_cursor - edit.opened_scroll_start ) );
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
      static const auto unsaved_len = CsLen( unsaved );
      auto unsaved_w = LayoutString( font, spaces_per_tab, unsaved, unsaved_len );
      For( i, 0, edit.nlines_screen ) {
        idx_t rowidx = ( i + edit.opened_scroll_start );
        if( rowidx >= edit.search_matches.len ) {
          break;
        }

        auto open = edit.search_matches.mem[rowidx];
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

    case editmode_t::fileopener_renaming: __fallthrough;
    case editmode_t::fileopener: {
      FileopenerRender(
        edit,
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
        auto label = AllocString( "Case sensitive: %u -- Press [ %s ] to toggle.", edit.findrepl.case_sens, key0.mem );
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
        auto label = AllocString( "Whole word: %u -- Press [ %s ] to toggle.", edit.findrepl.word_boundary, key0.mem );
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
        static const idx_t find_label_len = CsLen( find_label );
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
        static const idx_t replace_label_len = CsLen( replace_label );
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
      static const idx_t gotoline_label_len = CsLen( gotoline_label );
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
      auto label1 = AllocString( "Press [ %s ] to keep your changes.", key0.mem );
      auto label2 = AllocString( "Press [ %s ] to discard your changes.", key1.mem );
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
//      auto& fif = edit.findinfiles;
      FindinfilesRender(
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

    default: UnreachableCrash();
  }
}









// =================================================================================
// MOUSE

void
FileopenerControlMouse(
  edit_t& edit,
  bool& target_valid,
  font_t& font,
  rectf32_t bounds,
  glwmouseevent_t type,
  glwmousebtn_t btn,
  vec2<s32> m,
  vec2<s32> raw_delta,
  s32 dwheel
  )
{
  auto& fo = edit.fileopener;

  bool double_clicked_on_line;
  ListviewControlMouse(
    &fo.listview,
    target_valid,
    font,
    bounds,
    type,
    btn,
    m,
    raw_delta,
    dwheel,
    &double_clicked_on_line
    );

  if( double_clicked_on_line ) {
    CmdFileopenerChoose( edit );
  }
}

void
FindinfilesControlMouse(
  edit_t& edit,
  bool& target_valid,
  font_t& font,
  rectf32_t bounds,
  glwmouseevent_t type,
  glwmousebtn_t btn,
  vec2<s32> m,
  vec2<s32> raw_delta,
  s32 dwheel
  )
{
  auto& fif = edit.findinfiles;

  bool double_clicked_on_line;
  ListviewControlMouse(
    &fif.listview,
    target_valid,
    font,
    bounds,
    type,
    btn,
    m,
    raw_delta,
    dwheel,
    &double_clicked_on_line
    );

  if( double_clicked_on_line ) {
    CmdFindinfilesChoose( edit );
  }
}


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
        SendToClipboardText( *g_client, ML( open->txt.filename ) );
        LogUI( "Copied filename to clipboard." );
      }
      if( click_on_statusbar_l ) {
        auto open = edit.active[1];
        SendToClipboardText( *g_client, ML( open->txt.filename ) );
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

    case editmode_t::fileopener: {
      FileopenerControlMouse(
        edit,
        target_valid,
        font,
        bounds,
        type,
        btn,
        m,
        raw_delta,
        dwheel
        );
    } break;

    case editmode_t::findinfiles: {
      FindinfilesControlMouse(
        edit,
        target_valid,
        font,
        bounds,
        type,
        btn,
        m,
        raw_delta,
        dwheel
        );
    } break;

//    default: UnreachableCrash();
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
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_save    ), CmdSave    ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_saveall ), CmdSaveAll ),
            };
            ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
          } __fallthrough;

          case glwkeyevent_t::repeat: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_closefile           ), CmdSwitchopenedCloseFile         ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_cursor_u            ), CmdSwitchopenedCursorU           ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_cursor_d            ), CmdSwitchopenedCursorD           ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_cursor_page_u       ), CmdSwitchopenedCursorU           , edit.nlines_screen ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_cursor_page_d       ), CmdSwitchopenedCursorD           , edit.nlines_screen ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_scroll_u            ), CmdSwitchopenedScrollU           , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_scroll_d            ), CmdSwitchopenedScrollD           , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_scroll_page_u       ), CmdSwitchopenedScrollU           , edit.nlines_screen ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_scroll_page_d       ), CmdSwitchopenedScrollD           , edit.nlines_screen ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_switchopened_make_cursor_present ), CmdSwitchopenedMakeCursorPresent ),
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
          edit.opened_search,
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
    } break;

    case editmode_t::fileopener: {
      auto& fo = edit.fileopener;
      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_findinfiles_from_fileopener         ), CmdMode_findinfiles_from_fileopener         ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_editfile_from_fileopener            ), CmdMode_editfile_from_fileopener            ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_fileopener_renaming_from_fileopener ), CmdMode_fileopener_renaming_from_fileopener ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_recycle_file_or_dir           ), CmdFileopenerRecycle                        ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_change_cwd_up                 ), CmdFileopenerChangeCwdUp                    ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_refresh                       ), CmdFileopenerRefresh                        ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_newfile                       ), CmdFileopenerNewFile                        ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_newdir                        ), CmdFileopenerNewDir                         ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_choose                        ), CmdFileopenerChoose                         ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_save    ), CmdSave    ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_saveall ), CmdSaveAll ),
            };
            ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
          } __fallthrough;

          case glwkeyevent_t::repeat: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_focus_u       ), CmdFileopenerFocusU  ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_focus_d       ), CmdFileopenerFocusD  ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_cursor_u      ), CmdFileopenerCursorU ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_cursor_d      ), CmdFileopenerCursorD ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_cursor_page_u ), CmdFileopenerCursorU , fo.listview.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_cursor_page_d ), CmdFileopenerCursorD , fo.listview.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_scroll_u      ), CmdFileopenerScrollU , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_scroll_d      ), CmdFileopenerScrollD , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_scroll_page_u ), CmdFileopenerScrollU , fo.listview.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_scroll_page_d ), CmdFileopenerScrollD , fo.listview.pageupdn_distance ),
            };
            ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
          } break;

          case glwkeyevent_t::up: {
          } break;

          default: UnreachableCrash();
        }
      }

      if( !ran_cmd ) {
        switch( fo.focus ) {
          case fileopenerfocus_t::dir: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              fo.cwd,
              kb_command,
              target_valid,
              content_changed,
              ran_cmd,
              type,
              key,
              keylocks
              );
            if( content_changed ) {
//              CmdFileopenerRefresh( edit );
            }
          } break;
          case fileopenerfocus_t::query: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              fo.query,
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
              CmdFileopenerUpdateMatches( edit );
            }
          } break;
          case fileopenerfocus_t::ignored_filetypes: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              fo.ignored_filetypes,
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
              CmdFileopenerUpdateMatches( edit );
            }
          } break;
          case fileopenerfocus_t::included_filetypes: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              fo.included_filetypes,
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
              CmdFileopenerUpdateMatches( edit );
            }
          } break;
          case fileopenerfocus_t::ignored_substrings: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              fo.ignored_substrings,
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
              CmdFileopenerUpdateMatches( edit );
            }
          } break;
          default: UnreachableCrash();
        }
      }

      if( !ran_cmd ) {
        if( kb_command ) {
          switch( type ) {
            case glwkeyevent_t::dn: {
              edit_cmdmap_t table[] = {
                _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_editfile_from_fileopener ), CmdMode_editfile_from_fileopener ),
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
      }
    } break;

    case editmode_t::fileopener_renaming: {
      auto& fo = edit.fileopener;
      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // dir level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_fileopener_from_fileopener_renaming ), CmdMode_fileopener_from_fileopener_renaming ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_renaming_apply                ), CmdFileopenerRenamingApply                  ),
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
          fo.renaming_txt,
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
      auto& fif = edit.findinfiles;
      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_editfile_from_findinfiles ), CmdMode_editfile_or_fileopener_from_findinfiles ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_replace_at_cursor  ), CmdFindinfilesReplaceAtCursor     ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_replace_all        ), CmdFindinfilesReplaceAll          ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_refresh            ), CmdFindinfilesRefresh             ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_choose             ), CmdFindinfilesChoose              ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_save    ), CmdSave    ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_saveall ), CmdSaveAll ),
            };
            ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
          } __fallthrough;

          case glwkeyevent_t::repeat: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_focus_u               ), CmdFindinfilesFocusU             ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_focus_d               ), CmdFindinfilesFocusD             ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_cursor_u              ), CmdFindinfilesCursorU            ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_cursor_d              ), CmdFindinfilesCursorD            ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_cursor_page_u         ), CmdFindinfilesCursorU            , fif.listview.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_cursor_page_d         ), CmdFindinfilesCursorD            , fif.listview.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_scroll_u              ), CmdFindinfilesScrollU            , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_scroll_d              ), CmdFindinfilesScrollD            , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_scroll_page_u         ), CmdFindinfilesScrollU            , fif.listview.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_scroll_page_d         ), CmdFindinfilesScrollD            , fif.listview.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_toggle_case_sensitive ), CmdFindinfilesToggleCaseSens     ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_toggle_word_boundary  ), CmdFindinfilesToggleWordBoundary ),
            };
            ExecuteCmdMap( edit, AL( table ), key, target_valid, ran_cmd );
          } break;

          case glwkeyevent_t::up: {
          } break;

          default: UnreachableCrash();
        }
      }

      if( !ran_cmd ) {
        switch( fif.focus ) {
          case findinfilesfocus_t::dir: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              fif.dir,
              kb_command,
              target_valid,
              content_changed,
              ran_cmd,
              type,
              key,
              keylocks
              );
            if( content_changed ) {
//              CmdFindinfilesRefresh( edit );
            }
          } break;
          case findinfilesfocus_t::ignored_filetypes: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              fif.ignored_filetypes,
              kb_command,
              target_valid,
              content_changed,
              ran_cmd,
              type,
              key,
              keylocks
              );
            if( content_changed ) {
//              CmdFindinfilesUpdateMatches( edit );
            }
          } break;
          case findinfilesfocus_t::included_filetypes: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              fif.included_filetypes,
              kb_command,
              target_valid,
              content_changed,
              ran_cmd,
              type,
              key,
              keylocks
              );
            if( content_changed ) {
//              CmdFindinfilesUpdateMatches( edit );
            }
          } break;
          case findinfilesfocus_t::query: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              fif.query,
              kb_command,
              target_valid,
              content_changed,
              ran_cmd,
              type,
              key,
              keylocks
              );
            if( content_changed ) {
//              CmdFindinfilesUpdateMatches( edit );
            }
          } break;
          case findinfilesfocus_t::replacement: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              fif.replacement,
              kb_command,
              target_valid,
              content_changed,
              ran_cmd,
              type,
              key,
              keylocks
              );
          } break;
        }
      }
    } break;

    default: UnreachableCrash();
  }
}

