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



// should we bother using buf for filecontentsearch / findinfiles ?
// presumably it will only ever be slower than raw string search.
// we never need any content-editing capabilities, even for replacement, since we open in the editor for that.
// maybe if we implement largefile-streaming at some point, then it might be worth it.
// we can probably delete this macro.
#define USE_BUF_FOR_FILECONTENTSEARCH 0

struct
foundinfile_t
{
  slice_t name;
  slice_t sample; // line sample text to display a context preview.
#if USE_BUF_FOR_FILECONTENTSEARCH
  content_ptr_t l;
  content_ptr_t r;
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
  f.pos_match_l = {};
  f.pos_match_r = {};
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
  stack_resizeable_cont_t<slice_t>* filenames;
  idx_t filenames_start;
  idx_t filenames_count;
  stack_resizeable_cont_t<slice_t>* ignored_filetypes_list;
  slice_t key;
  bool case_sens;
  bool word_boundary;

  // output / modifiable:
  pagelist_t mem;
  stack_resizeable_pagelist_t<foundinfile_t> matches;


  u8 cache_line_padding_to_avoid_thrashing[64]; // last thing, since this type is packed into an stack_resizeable_cont_t
};




Enumc( findinfilesfocus_t )
{
  dir,
  ignored_filetypes,
  query,
  replacement,
  COUNT
};

struct
findinfiles_t
{
  stack_resizeable_pagelist_t<foundinfile_t> matches;
  pagelist_t mem;
  findinfilesfocus_t focus;
  txt_t dir;
  txt_t query;
  bool case_sens;
  bool word_boundary;
  txt_t replacement;
  txt_t ignored_filetypes;
  idx_t cursor;
  idx_t scroll_start;
  idx_t scroll_end;
  idx_t pageupdn_distance;
  slice_t matches_dir;

  u8 cache_line_padding_to_avoid_thrashing[64];
  stack_resizeable_cont_t<asynccontext_filecontentsearch_t> asynccontexts;
  idx_t ncontexts_active;
  idx_t max_ncontexts_active;

  // used by all other threads as readonly:
  stack_resizeable_cont_t<slice_t> filenames;
  stack_resizeable_cont_t<slice_t> ignored_filetypes_list;
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
  fif.focus = findinfilesfocus_t::query;
  fif.cursor = 0;
  fif.scroll_start = 0;
  fif.scroll_end = 0;
  fif.pageupdn_distance = 0;
  fif.matches_dir = {};
  fif.ncontexts_active = 0;
  fif.max_ncontexts_active = 0;
  Alloc( fif.filenames, 4096 );
  Alloc( fif.asynccontexts, 16 );
  Alloc( fif.ignored_filetypes_list, 16 );
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
  fif.focus = findinfilesfocus_t::query;
  fif.cursor = 0;
  fif.scroll_start = 0;
  fif.scroll_end = 0;
  fif.pageupdn_distance = 0;
  fif.matches_dir = {};
  fif.ncontexts_active = 0;
  fif.max_ncontexts_active = 0;
  Free( fif.filenames );

  ForLen( i, fif.asynccontexts ) {
    auto ac = fif.asynccontexts.mem + i;
    Kill( ac->mem );
    Kill( ac->matches );
  }
  Free( fif.asynccontexts );

  Free( fif.ignored_filetypes_list );
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
fileopener_dblclick_t
{
  bool first_made;
  idx_t first_cursor;
  u64 first_clock;
};

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
  ignored_substrings,
  query,
  COUNT
};
Inl fileopenerfocus_t
PreviousValue( fileopenerfocus_t value )
{
  switch( value ) {
    case fileopenerfocus_t::dir:                return fileopenerfocus_t::query;
    case fileopenerfocus_t::ignored_filetypes:  return fileopenerfocus_t::dir;
    case fileopenerfocus_t::ignored_substrings: return fileopenerfocus_t::ignored_filetypes;
    case fileopenerfocus_t::query:              return fileopenerfocus_t::ignored_substrings;
    default: UnreachableCrash(); return {};
  }
}
Inl fileopenerfocus_t
NextValue( fileopenerfocus_t value )
{
  switch( value ) {
    case fileopenerfocus_t::dir:                return fileopenerfocus_t::ignored_filetypes;
    case fileopenerfocus_t::ignored_filetypes:  return fileopenerfocus_t::ignored_substrings;
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
  stack_resizeable_pagelist_t<fileopener_row_t> pool;
  pagelist_t pool_mem;
};

struct
fileopener_t
{
  stack_resizeable_cont_t<fileopener_oper_t> history;
  idx_t history_idx;

  // pool is the backing store of actual files/dirs given our cwd and query.
  // based on the query, we'll filter down to some subset, stored in matches.
  // ui just shows matches.
  fileopenerfocus_t focus;
  txt_t cwd;
  fsobj_t last_cwd_for_changecwd_rollback;
  txt_t query;
  txt_t ignored_filetypes;
  txt_t ignored_substrings;
  stack_resizeable_pagelist_t<fileopener_row_t> pool;
  pagelist_t pool_mem; // reset everytime we fillpool.
  stack_resizeable_pagelist_t<fileopener_row_t*> matches; // points into pool.
  idx_t cursor; // index into matches.
  idx_t scroll_start;
  idx_t scroll_end;
  idx_t pageupdn_distance;

  u8 cache_line_padding_to_avoid_thrashing[64];
  asynccontext_fileopenerfillpool_t asynccontext_fillpool;
  u8 cache_line_padding_to_avoid_thrashing2[64];
  idx_t ncontexts_active;

  pagelist_t matches_mem; // reset everytime we regenerate matches.
  stack_resizeable_cont_t<slice_t> ignored_filetypes_list; // uses matches_mem as backing memory.
  stack_resizeable_cont_t<slice_t> ignored_substrings_list;

  fileopener_dblclick_t dblclick;

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

  Init( fo.ignored_substrings );
  TxtLoadEmpty( fo.ignored_substrings );
  auto default_substrings = GetPropFromDb( slice_t, string_fileopener_ignored_substrings );
  CmdAddString( fo.ignored_substrings, Cast( idx_t, default_substrings.mem ), default_substrings.len );

  Init( fo.pool, 128 );
  Init( fo.pool_mem, 128*1024 );

  Init( fo.matches, 128 );

  fo.cursor = 0;
  fo.scroll_start = 0;
  fo.scroll_end = 0;
  fo.pageupdn_distance = 0;

  fo.asynccontext_fillpool.cwd.len = 0;
  Init( fo.asynccontext_fillpool.pool, 128 );
  Init( fo.asynccontext_fillpool.pool_mem, 128*1024 );
  fo.ncontexts_active = 0;

  Init( fo.matches_mem, 128*1024 );
  Alloc( fo.ignored_filetypes_list, 16 );
  Alloc( fo.ignored_substrings_list, 16 );

  fo.dblclick.first_made = 0;
  fo.dblclick.first_cursor = 0;
  fo.dblclick.first_clock = 0;

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
  Kill( fo.ignored_substrings );

  Kill( fo.pool );
  Kill( fo.pool_mem );

  Kill( fo.matches );

  fo.cursor = 0;
  fo.scroll_start = 0;
  fo.scroll_end = 0;
  fo.pageupdn_distance = 0;

  fo.asynccontext_fillpool.cwd.len = 0;
  Kill( fo.asynccontext_fillpool.pool );
  Kill( fo.asynccontext_fillpool.pool_mem );
  fo.ncontexts_active = 0;

  Kill( fo.matches_mem );
  Free( fo.ignored_filetypes_list );
  Free( fo.ignored_substrings_list );

  fo.dblclick.first_made = 0;
  fo.dblclick.first_cursor = 0;
  fo.dblclick.first_clock = 0;

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
  pagelist_t mem;

  edittxtopen_t* active[2];
  bool horzview;
  bool horzfocus_l; // else r.

  // openedmru
  listwalloc_t<edittxtopen_t*> openedmru;

  // opened
  stack_resizeable_cont_t<edittxtopen_t*> search_matches;
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
  stack_resizeable_pagelist_t<u8> output;
  Init( output, 1024 );
  Log( "RUN_ON_OPEN" );
  LogAddIndent( +1 );
  {
    auto cstr = AllocCstr( cmd );
    Log( "executing command: %s", cstr );
    MemHeapFree( cstr );
  }

  s32 r = Execute( SliceFromString( cmd ), output, show_window );

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


#if USE_FILEMAPPED_OPEN

  void
  EditOpenAndSetActiveTxt( edit_t& edit, filemapped_t& file )
  {
    edittxtopen_t* open = 0;
    bool opened_existing = 0;
    EditOpen( edit, file, &open, &opened_existing );
    AssertCrash( open );

    edit.horzfocus_l = open->horz_l;
    edit.active[edit.horzfocus_l] = open;
    MoveOpenedToFrontOfMru( edit, open );
  }

#else

  void
  EditOpenAndSetActiveTxt( edit_t& edit, file_t& file )
  {
    edittxtopen_t* open = 0;
    bool opened_existing = 0;
    EditOpen( edit, file, &open, &opened_existing );
    AssertCrash( open );

    edit.horzfocus_l = open->horz_l;
    edit.active[edit.horzfocus_l] = open;
    MoveOpenedToFrontOfMru( edit, open );
  }

#endif







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
    CstrAddBack( dst, Str( "g" ), 1 );
    *dst_size = CstrLength( dst );
  } elif( size > mb ) {
    size = Cast( u64, 0.5 + size / Cast( f64, mb ) );
    CsFrom_u64( dst, dst_len, size );
    CstrAddBack( dst, Str( "m" ), 1 );
    *dst_size = CstrLength( dst );
  } elif( size > kb ) {
    size = Cast( u64, 0.5 + size / Cast( f64, kb ) );
    CsFrom_u64( dst, dst_len, size );
    CstrAddBack( dst, Str( "k" ), 1 );
    *dst_size = CstrLength( dst );
  } else {
    CsFrom_u64( dst, dst_len, size );
    CstrAddBack( dst, Str( "b" ), 1 );
    *dst_size = CstrLength( dst );
  }
#else
  CsFrom_u64( dst, dst_len, dst_size, size, 1 );
#endif
}


Inl void
FileopenerResetCS( fileopener_t& fo )
{
  fo.cursor = 0;
  fo.scroll_start = 0;
}

Inl void
FileopenerFixupCS( fileopener_t& fo )
{
  fo.cursor = MIN( fo.cursor, MAX( fo.matches.totallen, 1 ) - 1 );
  fo.scroll_start = MIN( fo.scroll_start, MAX( fo.matches.totallen, 1 ) - 1 );
}

Inl void
FileopenerMakeCursorVisible( fileopener_t& fo )
{
  FileopenerFixupCS( fo );

  bool offscreen_u = fo.scroll_start && ( fo.cursor < fo.scroll_start );
  bool offscreen_d = fo.scroll_end && ( fo.scroll_end <= fo.cursor );
  if( offscreen_u ) {
    auto dlines = fo.scroll_start - fo.cursor;
    fo.scroll_start -= dlines;
  } elif( offscreen_d ) {
    auto dlines = fo.cursor - fo.scroll_end + 1;
    fo.scroll_start += dlines;
  }

  FileopenerFixupCS( fo );
}

Inl void
ParseSpaceSeparatedList(
  pagelist_t& dst_mem,
  stack_resizeable_cont_t<slice_t>& dst,
  buf_t& src
  )
{
  Reserve( dst, src.content_len / 2 );

  auto pos = GetBOF( src );
  Forever {
    auto elem_start = CursorSkipSpacetabR( src, pos, 0 );
    idx_t elem_len;
    auto elem_end = CursorStopAtSpacetabR( src, elem_start, &elem_len );
    if( !elem_len ) {
      auto eof = GetEOF( src );
      elem_end = CursorSkipSpacetabL( src, eof, 0 );
      if( Less( elem_end, elem_start ) ) {
        break;
      }
      elem_len = CountCharsBetween( src, elem_start, elem_end );
      if( !elem_len ) {
        break;
      }
    }

    auto elem = AddBack( dst );
    elem->mem = AddPagelist( dst_mem, u8, 1, elem_len );
    elem->len = elem_len;
    Contents( src, elem_start, ML( *elem ) );

    pos = elem_end;
  }
}

Inl void
FileopenerUpdateMatches( fileopener_t& fo )
{
  Reset( fo.matches_mem );
  Reset( fo.matches );
  fo.ignored_filetypes_list.len = 0;
  fo.ignored_substrings_list.len = 0;

  ParseSpaceSeparatedList(
    fo.matches_mem,
    fo.ignored_filetypes_list,
    fo.ignored_filetypes.buf
    );

  ParseSpaceSeparatedList(
    fo.matches_mem,
    fo.ignored_substrings_list,
    fo.ignored_substrings.buf
    );

  bool must_match_key = TxtLen( fo.query );
  auto key = AllocContents( fo.query.buf );

  if( fo.pool.totallen ) {
    auto pa_iter = MakeIteratorAtLinearIndex( fo.pool, 0 );
    For( i, 0, fo.pool.totallen ) {
      auto elem = GetElemAtIterator( fo.pool, pa_iter );
      pa_iter = IteratorMoveR( fo.pool, pa_iter );

      auto last_dot = StringScanL( ML( elem->name ), '.' );
      slice_t ext = {};
      if( last_dot ) {
        auto fileext = last_dot + 1;
        ext.mem = fileext;
        ext.len = elem->name.mem + elem->name.len - fileext;
      }

      bool include = 1;

      // ignore files with extensions in the 'ignore' list.
      if( ext.len ) {
        ForLen( j, fo.ignored_filetypes_list ) {
          auto filter = fo.ignored_filetypes_list.mem + j;
          if( StringEquals( ML( ext ), ML( *filter ), 0 ) ) {
            include = 0;
            break;
          }
        }
      }

      // ignore things which match the 'ignored_substrings' list.
      ForLen( j, fo.ignored_substrings_list ) {
        auto filter = fo.ignored_substrings_list.mem + j;
        idx_t pos;
        if( StringIdxScanR( &pos, ML( elem->name ), 0, ML( *filter ), 0, 0 ) ) {
          include = 0;
        }
      }

      // ignore things which don't match the 'query' key.
      if( must_match_key ) {
        idx_t pos;
        if( !StringIdxScanR( &pos, ML( elem->name ), 0, ML( key ), 0, 0 ) ) {
          include = 0;
        }
      }

      if( include ) {
        auto instance = AddBack( fo.matches, 1 );
        *instance = elem;
      }
    }
  }
  Free( key );

  FileopenerFixupCS( fo );
  FileopenerMakeCursorVisible( fo );
}

// note we only push 1 of these at a time, since it probably doesn't make sense to parallelize FsFindDirsAndFiles.
// we're primarily limited by disk speed, so more than 1 thread probably doesn't make sense.
// but, this unblocks the main thread ui, which is good since this can be slow.
__AsyncTask( AsyncTask_FileopenerFillPool )
{
  ProfFunc();

  auto ac = Cast( asynccontext_fileopenerfillpool_t*, asyncqueue_entry->asynccontext );

  Reset( ac->pool_mem );

  // TODO: add signal_quit checks into FsFindDirsAndFiles.
  // TODO: add PushMainTaskCompleted incremental results into FsFindDirsAndFiles.
  Prof( foFill_FsFindDirsAndFiles );
  stack_resizeable_cont_t<dir_or_file_t> objs; // PERF: make these pointers into pool_mem ? or just make this stack_resizeable_pagelist_t ?
  Alloc( objs, ac->pool.totallen + 64 );
  FsFindDirsAndFiles( objs, ac->pool_mem, ML( ac->cwd ), 1 );
  ProfClose( foFill_FsFindDirsAndFiles );

  Prof( foFill_TotalFillEntries );
  ForLen( i, objs ) {
    if( g_mainthread.signal_quit ) {
      return;
    }
    auto obj = objs.mem + i;

    auto elem = AddBack( ac->pool, 1 );
    AssertCrash( obj->name.len >= ac->cwd.len + 1 );
    elem->name.mem = obj->name.mem + ac->cwd.len + 1; // take the name only.
    elem->name.len = obj->name.len - ac->cwd.len - 1;
    elem->is_file = obj->is_file;
    if( obj->is_file ) {
      elem->size = obj->filesize;
      elem->readonly = obj->readonly;
      elem->sizetxt.mem = AddPagelist( ac->pool_mem, u8, 1, 64 );
      FileopenerSizestrFromSize( elem->sizetxt.mem, 64, &elem->sizetxt.len, obj->filesize );
    } else {
      elem->size = 0;
      elem->readonly = 0;
      elem->sizetxt.len = 0;
    }
  }
  ProfClose( foFill_TotalFillEntries );
  Free( objs );

  // note that we don't send signals back to the main thread in a more fine-grained fashion.
  // we just send the final signal here, after everything's been done.
  // this is because the ac's results datastructure is shared across all its results.

  // TODO: pass fine-grained results back to the main thread ?
  PushMainTaskCompleted( asyncqueue_entry );
}

__MainTaskCompleted( MainTaskCompleted_FileopenerFillPool )
{
  ProfFunc();

  auto fo = Cast( fileopener_t*, maincontext );
  auto ac = Cast( asynccontext_fileopenerfillpool_t*, asynccontext );

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
    auto elem = AddBack<fileopener_row_t>( fo.pool );
    elem->name.len = 2;
    elem->name.mem = AddPagelist( fo.pool_mem, u8, 1, elem->name.len );
    Memmove( elem->name.mem, Str( ".." ), 2 );
    elem->is_file = 0;
    elem->size = 0;
    elem->readonly = 0;
    elem->sizetxt.len = 0;
  }

  auto cwd = AllocContents( fo.cwd.buf );
  // TODO: user error if cwd is longer than fixed-size fsobj_t can handle.
  Memmove( AddBack( fo.asynccontext_fillpool.cwd, cwd.len ), ML( cwd ) );
  Free( cwd );

  fo.ncontexts_active += 1;

  asyncqueue_entry_t entry;
  entry.FnAsyncTask = AsyncTask_FileopenerFillPool;
  entry.FnMainTaskCompleted = MainTaskCompleted_FileopenerFillPool;
  entry.asynccontext = &fo.asynccontext_fillpool;
  entry.maincontext = &fo;
  PushAsyncTask( 0, &entry );
}

//#if 0
//  auto cwd = AllocContents( fo.cwd.buf );
//
//  pagelist_t mem;
//  Init( mem, 32768 );
//  stack_resizeable_cont_t<dir_or_file_t> objs;
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
  if( !edit.opened.len ) {
    return;
  }
  edit.mode = editmode_t::editfile;

  auto clear_search_on_switch = GetPropFromDb( bool, bool_fileopener_clear_search_on_switch );
  if( clear_search_on_switch ) {
    CmdSelectAll( edit.fileopener.query );
    CmdRemChL( edit.fileopener.query );
  }
}

Inl void
_SwitchToFileopener( edit_t& edit )
{
  edit.mode = editmode_t::fileopener;

  auto clear_search_on_switch = GetPropFromDb( bool, bool_fileopener_clear_search_on_switch );
  if( clear_search_on_switch ) {
    FileopenerUpdateMatches( edit.fileopener );
    FileopenerResetCS( edit.fileopener );
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
  if( !TxtLen( fo.cwd ) ) {
    return;
  }

  auto cwd = AllocContents( fo.cwd.buf );
  idx_t new_cwd_len;
  bool res = MemIdxScanL( &new_cwd_len, ML( cwd ), "/", 1 );
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
  fsobj_t name;
  name.len = 0;
  auto cwd = AllocContents( edit.fileopener.cwd.buf );
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
      EditOpenAndSetActiveTxt( edit, file );
      CmdMode_editfile_from_fileopener( edit );
    }
#if !USE_FILEMAPPED_OPEN
    FileFree( file );
#endif

  } else {
    bool up_dir = MemEqual( ML( row->name ), "..", 2 );
    if( up_dir ) {
      FileopenerCwdUp( edit.fileopener );

    } else {
      fsobj_t fsobj;
      fsobj.len = 0;
      Memmove( AddBack( fsobj, cwd.len ), ML( cwd ) );

      auto oper = FileopenerAddHistorical( edit.fileopener, fileopener_opertype_t::changecwd );
      oper->changecwd.src = fsobj;

      oper->changecwd.dst = name;

      FileopenerChangeCwd( edit.fileopener, SliceFromArray( name ) );
    }

    FileopenerFillPool( edit.fileopener );
    FileopenerUpdateMatches( edit.fileopener );
    FileopenerResetCS( edit.fileopener );
  }

  Free( cwd );
}


__EditCmd( CmdFileopenerUpdateMatches )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  FileopenerUpdateMatches( edit.fileopener );
}

__EditCmd( CmdFileopenerRefresh )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  FileopenerFillPool( edit.fileopener );
  FileopenerUpdateMatches( edit.fileopener );
}

__EditCmd( CmdFileopenerChoose )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  AssertCrash( edit.fileopener.cursor < edit.fileopener.matches.totallen );
  auto row = *LookupElemByLinearIndex( edit.fileopener.matches, edit.fileopener.cursor );
  FileopenerOpenRow( edit, row );
}

__EditCmd( CmdFileopenerFocusD )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  edit.fileopener.focus = NextValue( edit.fileopener.focus );

  // save previous cwd, in case we fail to apply a changecwd.
  if( edit.fileopener.focus == fileopenerfocus_t::dir ) {
    auto cwd = AllocContents( edit.fileopener.cwd.buf );
    Memmove( AddBack( edit.fileopener.last_cwd_for_changecwd_rollback, cwd.len ), ML( cwd ) );
    Free( cwd );
  }
}

__EditCmd( CmdFileopenerFocusU )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  edit.fileopener.focus = PreviousValue( edit.fileopener.focus );

  // save previous cwd, in case we fail to apply a changecwd.
  if( edit.fileopener.focus == fileopenerfocus_t::dir ) {
    auto cwd = AllocContents( edit.fileopener.cwd.buf );
    Memmove( AddBack( edit.fileopener.last_cwd_for_changecwd_rollback, cwd.len ), ML( cwd ) );
    Free( cwd );
  }
}

__EditCmd( CmdFileopenerCursorU )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  auto nlines = misc ? misc : 1;
  edit.fileopener.cursor -= MIN( nlines, edit.fileopener.cursor );
  FileopenerMakeCursorVisible( edit.fileopener );
}

__EditCmd( CmdFileopenerCursorD )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  auto nlines = misc ? misc : 1;
  edit.fileopener.cursor += nlines;
  FileopenerFixupCS( edit.fileopener );
  FileopenerMakeCursorVisible( edit.fileopener );
}

__EditCmd( CmdFileopenerScrollU )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  auto nlines = misc ? misc : 1;
  edit.fileopener.scroll_start -= MIN( nlines, edit.fileopener.scroll_start );
}

__EditCmd( CmdFileopenerScrollD )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  auto nlines = misc ? misc : 1;
  edit.fileopener.scroll_start += nlines;
  FileopenerFixupCS( edit.fileopener );
}



__EditCmd( CmdFileopenerRecycle )
{
  AssertCrash( edit.mode == editmode_t::fileopener );

  if( !edit.fileopener.matches.totallen ) {
    return;
  }

  AssertCrash( edit.fileopener.cursor < edit.fileopener.matches.totallen );
  auto row = *LookupElemByLinearIndex( edit.fileopener.matches, edit.fileopener.cursor );

  fsobj_t name;
  name.len = 0;
  auto cwd = AllocContents( edit.fileopener.cwd.buf );
  Memmove( AddBack( name, cwd.len ), ML( cwd ) );
  Free( cwd );
  Memmove( AddBack( name ), "/", 1 );
  Memmove( AddBack( name, row->name.len ), ML( row->name ) );

  if( row->is_file ) {
    auto oper = FileopenerAddHistorical( edit.fileopener, fileopener_opertype_t::delfile );
    oper->delfile = name;

    if( FileRecycle( ML( name ) ) ) {
      FileopenerFillPool( edit.fileopener );
      FileopenerUpdateMatches( edit.fileopener );
    } else {
      auto tmp = AllocCstr( ML( name ) );
      LogUI( "[DIR] Failed to delete file: \"%s\"!", tmp );
      MemHeapFree( tmp );
    }
  } else {
    auto oper = FileopenerAddHistorical( edit.fileopener, fileopener_opertype_t::deldir );
    oper->deldir = name;

    if( DirRecycle( ML( name ) ) ) {
      FileopenerFillPool( edit.fileopener );
      FileopenerUpdateMatches( edit.fileopener );
    } else {
      auto tmp = AllocCstr( ML( name ) );
      LogUI( "[DIR] Failed to delete edit.fileopener: \"%s\"!", tmp );
      MemHeapFree( tmp );
    }
  }
  FileopenerFixupCS( edit.fileopener );
}


__EditCmd( CmdFileopenerChangeCwdUp )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  FileopenerCwdUp( edit.fileopener );
  FileopenerFillPool( edit.fileopener );
  FileopenerUpdateMatches( edit.fileopener );
  FileopenerResetCS( edit.fileopener );
}

__EditCmd( CmdFileopenerNewFile )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  u8* default_name = Str( "new_file" );
  idx_t default_name_len = CstrLength( default_name );

  fsobj_t name;
  name.len = 0;
  auto cwd = AllocContents( edit.fileopener.cwd.buf );
  Memmove( AddBack( name, cwd.len ), ML( cwd ) );
  Free( cwd );
  Memmove( AddBack( name ), "/", 1 );
  Memmove( AddBack( name, default_name_len ), default_name, default_name_len );
  Memmove( AddBack( name ), "0", 1 );
  Memmove( AddBack( name, 4 ), ".txt", 4 );

  u32 suffix_num = 0;
  idx_t last_suffix_len = 1;
  stack_nonresizeable_stack_t<u8, 64> suffix;
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

  auto oper = FileopenerAddHistorical( edit.fileopener, fileopener_opertype_t::newfile );
  oper->newfile = name;

  file_t file = FileOpen( ML( name ), fileopen_t::only_new, fileop_t::RW, fileop_t::R );
  AssertWarn( file.loaded );
  FileFree( file );
  FileopenerFillPool( edit.fileopener );
  FileopenerUpdateMatches( edit.fileopener );
}

__EditCmd( CmdFileopenerNewDir )
{
  AssertCrash( edit.mode == editmode_t::fileopener );
  u8* default_name = Str( "new_dir" );
  idx_t default_name_len = CstrLength( default_name );

  fsobj_t name;
  name.len = 0;
  auto cwd = AllocContents( edit.fileopener.cwd.buf );
  Memmove( AddBack( name, cwd.len ), ML( cwd ) );
  Free( cwd );
  Memmove( AddBack( name ), "/", 1 );
  Memmove( AddBack( name, default_name_len ), default_name, default_name_len );
  Memmove( AddBack( name ), "0", 1 );

  u32 suffix_num = 0;
  idx_t last_suffix_len = 1;
  stack_nonresizeable_stack_t<u8, 64> suffix;
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

  auto oper = FileopenerAddHistorical( edit.fileopener, fileopener_opertype_t::newdir );
  oper->newfile = name;

  bool created = DirCreate( ML( name ) );
  AssertWarn( created );
  FileopenerFillPool( edit.fileopener );
  FileopenerUpdateMatches( edit.fileopener );
}


__EditCmd( CmdMode_fileopener_renaming_from_fileopener )
{
  AssertCrash( edit.mode == editmode_t::fileopener );

  edit.mode = editmode_t::fileopener_renaming;

  AssertCrash( edit.fileopener.cursor < edit.fileopener.matches.totallen );
  auto row = *LookupElemByLinearIndex( edit.fileopener.matches, edit.fileopener.cursor );
  edit.fileopener.renaming_row = edit.fileopener.cursor;

  CmdSelectAll( edit.fileopener.renaming_txt );
  CmdAddString( edit.fileopener.renaming_txt, Cast( idx_t, row->name.mem ), row->name.len );
  CmdSelectAll( edit.fileopener.renaming_txt );
}

Inl void
FileopenerRename( edit_t& edit, slice_t& src, slice_t& dst )
{
  if( FileMove( ML( dst ), ML( src ) ) ) {
    FileopenerFillPool( edit.fileopener );
    FileopenerUpdateMatches( edit.fileopener );
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

  AssertCrash( edit.fileopener.renaming_row < edit.fileopener.matches.totallen );
  auto row = *LookupElemByLinearIndex( edit.fileopener.matches, edit.fileopener.renaming_row );

  // TODO: prevent entry into mode fileopener_renaming, if the cursor is on '..'
  // Renaming the dummy parent edit.fileopener does bad things!
  if( !MemEqual( "..", 2, ML( row->name ) ) ) {
    auto cwd = AllocContents( edit.fileopener.cwd.buf );

    fsobj_t newname;
    newname.len = 0;
    Memmove( AddBack( newname, cwd.len ), ML( cwd ) );
    Memmove( AddBack( newname ), "/", 1 );
    idx_t newnameonly_len = TxtLen( edit.fileopener.renaming_txt );
    Contents(
      edit.fileopener.renaming_txt.buf,
      GetBOF( edit.fileopener.renaming_txt.buf ),
      AddBack( newname, newnameonly_len ),
      newnameonly_len
      );

    fsobj_t name;
    name.len = 0;
    Memmove( AddBack( name, cwd.len ), ML( cwd ) );
    Memmove( AddBack( name ), "/", 1 );
    Memmove( AddBack( name, row->name.len ), ML( row->name ) );

    auto oper = FileopenerAddHistorical( edit.fileopener, fileopener_opertype_t::rename );
    oper->rename.src = name;
    oper->rename.dst = newname;

    auto src = SliceFromArray( name );
    auto dst = SliceFromArray( newname );
    FileopenerRename( edit, src, dst );

    Free( cwd );
  }

  edit.fileopener.renaming_row = 0;
  CmdSelectAll( edit.fileopener.renaming_txt );
  CmdRemChL( edit.fileopener.renaming_txt );

  FileopenerFixupCS( edit.fileopener );

  edit.mode = editmode_t::fileopener;
}

__EditCmd( CmdFileopenerUndo ) // TODO: finish writing this.
{
//  DEBUG_PrintUndoRedo( dir, "PRE-UNDO\n" );
  AssertCrash( edit.fileopener.history_idx <= edit.fileopener.history.len );
  AssertCrash( edit.fileopener.history_idx == edit.fileopener.history.len  ||  edit.fileopener.history.mem[edit.fileopener.history_idx].type == fileopener_opertype_t::checkpt );
  if( !edit.fileopener.history_idx ) {
    return;
  }

  bool loop = 1;
  while( loop ) {
    edit.fileopener.history_idx -= 1;
    auto oper = edit.fileopener.history.mem + edit.fileopener.history_idx;

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
        FileopenerChangeCwd( edit.fileopener, dst );
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

  AssertCrash( edit.fileopener.history_idx == edit.fileopener.history.len  ||  edit.fileopener.history.mem[edit.fileopener.history_idx].type == fileopener_opertype_t::checkpt );
//  DEBUG_PrintUndoRedo( edit.fileopener, "POST-UNDO\n" );
}

__EditCmd( CmdFileopenerRedo )
{
}






void
EditInit( edit_t& edit )
{
  Init( edit.mem, 32768 );

  edit.mode = editmode_t::fileopener;
  Init( edit.opened, &edit.mem );

  Init( edit.openedmru, &edit.mem );

  edit.active[0] = 0;
  edit.active[1] = 0;
  edit.horzview = 0;
  edit.horzfocus_l = 0;

  Alloc( edit.search_matches, 128 );
  Init( edit.opened_search );
  TxtLoadEmpty( edit.opened_search );
  edit.opened_cursor = 0;
  edit.opened_scroll_start = 0;
  edit.opened_scroll_end = 0;
  edit.nlines_screen = 0;

  Init( edit.findinfiles );

  Init( edit.fileopener );
  CmdFileopenerRefresh( edit );

  Init( edit.gotoline );
  TxtLoadEmpty( edit.gotoline );

  Init( edit.findrepl );

  edit.mode_before_externalmerge = edit.mode;
  edit.horzview_before_externalmerge = edit.horzview;
  edit.active_externalmerge = 0;
  edit.file_externalmerge = {};
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
FindinFilesResetCS( findinfiles_t& fif )
{
  fif.cursor = 0;
  fif.scroll_start = 0;
}

Inl void
FindinFilesMakeCursorVisible( findinfiles_t& fif )
{
  bool offscreen_u = fif.scroll_start  &&  ( fif.cursor < fif.scroll_start );
  bool offscreen_d = fif.scroll_end  &&  ( fif.scroll_end <= fif.cursor );
  if( offscreen_u ) {
    fif.scroll_start = fif.cursor;
  } elif( offscreen_d ) {
    auto dlines = fif.cursor - fif.scroll_end + 1;
    fif.scroll_start += dlines;
  }
}




Inl void
AsyncFileContentSearch( asynccontext_filecontentsearch_t* ac )
{
  For( i, ac->filenames_start, ac->filenames_start + ac->filenames_count ) {
    auto obj = ac->filenames->mem + i;

    if( g_mainthread.signal_quit ) {
      return;
    }

    Prof( tmp_ApplyFilterFiletype );
    auto last_dot = StringScanL( ML( *obj ), '.' );
    slice_t ext = {};
    if( last_dot ) {
      auto fileext = last_dot + 1;
      ext.mem = fileext;
      ext.len = obj->mem + obj->len - fileext;
    }
    bool include = 1;
    if( ext.len ) {
      ForLen( j, *ac->ignored_filetypes_list ) {
        auto filter = ac->ignored_filetypes_list->mem + j;
        if( StringEquals( ML( ext ), ML( *filter ), 0 ) ) {
          include = 0;
          break;
        }
      }
    }
    ProfClose( tmp_ApplyFilterFiletype );
    if( include ) {
      Prof( tmp_FileOpen );
      file_t file = FileOpen( ML( *obj ), fileopen_t::only_existing, fileop_t::R, fileop_t::RW );
      ProfClose( tmp_FileOpen );
      // TODO: log when !file.loaded
      if( file.loaded ) {
        // collect all instances of the key.

#if USE_BUF_FOR_FILECONTENTSEARCH
        buf_t buf;
        Init( buf );
        BufLoad( buf, file );
        // TODO: close file after loading? we don't need to writeback or anything.
        bool found = 1;
        content_ptr_t pos = GetBOF( buf );
        while( found ) {
          content_ptr_t pos_match;
          FindFirstR(
            buf,
            pos,
            ML( ac->key ),
            &pos_match,
            &found,
            ac->case_sens,
            ac->word_boundary
            );
          if( found ) {
            Prof( tmp_AddMatch );
            auto instance = AddBack( ac->matches, 1 );
            instance->name = *obj;
            instance->l = pos_match;
            instance->r = CursorCharR( buf, pos_match, ac->key.len, 0 );
            instance->len = ac->key.len;
            auto sample_start = MIN(
              instance->l,
              CursorSkipSpacetabR( buf, CursorStopAtNewlineL( buf, pos_match, 0 ), 0 )
              );
            auto sample_end = MIN(
              CursorCharR( buf, sample_start, c_max_line_len, 0 ), // Samples don't scroll, so this should be wide enough for anyone.
              CursorSkipSpacetabL( buf, CursorStopAtNewlineR( buf, pos_match, 0 ), 0 )
              );
            instance->sample.len = CountBytesBetween( buf, sample_start, sample_end );
            instance->sample.mem = AddPagelist( ac->mem, u8, 1, instance->sample.len );
            AssertCrash( LEqual( sample_start, instance->l ) );
            instance->sample_match_offset = CountCharsBetween( buf, sample_start, instance->l );
            Contents( buf, sample_start, ML( instance->sample ) );
            ProfClose( tmp_AddMatch );
          }
        }

        Kill( buf );
#else
        Prof( tmp_FileAlloc );
        string_t mem = FileAlloc( file );
        ProfClose( tmp_FileAlloc );
        Prof( tmp_ContentSearch );
        bool found = 1;
        idx_t pos = 0;
        while( found ) {
          idx_t res = 0;
          found = StringIdxScanR( &res, ML( mem ), pos, ML( ac->key ), ac->case_sens, ac->word_boundary );
          if( found ) {
            Prof( tmp_AddMatch );
            pos += res;

            auto instance = AddBack( ac->matches, 1 );
            instance->name = *obj;
            instance->pos_match_l = pos;
            instance->pos_match_r = pos + ac->key.len;
            instance->match_len = ac->key.len;
            auto bol_skip_whitespace = CursorSkipSpacetabR( ML( mem ), CursorStopAtNewlineL( ML( mem ), pos ) );
            auto eol_skip_whitespace = CursorSkipSpacetabL( ML( mem ), CursorStopAtNewlineR( ML( mem ), pos ) );
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
            instance->sample.mem = AddPagelist( ac->mem, u8, 1, instance->sample.len );
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
}

__AsyncTask( AsyncTask_FileContentSearch )
{
  ProfFunc();

  auto ac = Cast( asynccontext_filecontentsearch_t*, asyncqueue_entry->asynccontext );
  AsyncFileContentSearch( ac );

  // note that we don't send signals back to the main thread in a more fine-grained fashion.
  // we just send the final signal here, after everything's been done.
  // this is because the ac's results datastructure is shared across all its results.
  // our strategy of splitting into small, separate ac's at the start seems to work well enough.
  PushMainTaskCompleted( asyncqueue_entry );
}

__MainTaskCompleted( MainTaskCompleted_FileContentSearch )
{
  ProfFunc();

  auto fif = Cast( findinfiles_t*, maincontext );
  auto ac = Cast( asynccontext_filecontentsearch_t*, asynccontext );

  AssertCrash( fif->ncontexts_active );
  fif->ncontexts_active -= 1;

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


__EditCmd( CmdFindinfilesRefresh )
{
  Prof( tmp_CmdFindinfilesRefresh );
  AssertCrash( edit.mode == editmode_t::findinfiles );

  auto& fif = edit.findinfiles;

  // prevent reentrancy while async stuff is executing
  if( fif.ncontexts_active ) {
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
  Free( fif.key );

  Reset( fif.matches );
  Reset( fif.mem );

  if( TxtLen( fif.query ) ) {
    fif.matches_dir.len = TxtLen( fif.dir );
    fif.matches_dir.mem = AddPagelist( fif.mem, u8, 1, fif.matches_dir.len );
    Contents( fif.dir.buf, GetBOF( fif.dir.buf ), ML( fif.matches_dir ) );

    Prof( tmp_MakeFilterFiletypes );

    ParseSpaceSeparatedList(
      fif.mem,
      fif.ignored_filetypes_list,
      fif.ignored_filetypes.buf
      );

    fif.key = AllocContents( fif.query.buf );

    ProfClose( tmp_MakeFilterFiletypes );

    // This call is ~3% of the cost of this function, after the OS / filesys caches filesys metadata.
    // So, we'll do this part on the main thread, then fan out.
    //
    // If we really need to, we could multithread this:
    // - Have some input queues of directories to process.
    // - Have some result queues of files.
    // - Each thread will:
    //   - Pop off a directory.
    //   - Enumerate all child files and directories.
    //   - Push files onto result queues.
    //   - Push child directories onto input queues.
    Prof( tmp_FsFindFiles );
    fif.filenames.len = 0;
    FsFindFiles( fif.filenames, fif.mem, ML( fif.matches_dir ), 1 );
    ProfClose( tmp_FsFindFiles );

    // PERF: optimize chunksize and per-thread-queue sizes
    // PERF: filesize-level chunking, for a more even distribution.
    constant idx_t chunksize = 512;
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
        ac->key = SliceFromString( fif.key );
        ac->case_sens = fif.case_sens;
        ac->word_boundary = fif.word_boundary;
        Init( ac->mem, 128*1024 );
        Init( ac->matches, 256 );

        fif.ncontexts_active += 1;
        fif.max_ncontexts_active = MAX( fif.max_ncontexts_active, fif.ncontexts_active );

        asyncqueue_entry_t entry;
        entry.FnAsyncTask = AsyncTask_FileContentSearch;
        entry.FnMainTaskCompleted = MainTaskCompleted_FileContentSearch;
        entry.asynccontext = ac;
        entry.maincontext = &fif;
        PushAsyncTask( i, &entry );
      }
    }
  }

  FindinFilesResetCS( fif );
}



__EditCmd( CmdMode_findinfiles_from_editfile )
{
  AssertCrash( edit.mode == editmode_t::editfile );
  edit.mode = editmode_t::findinfiles;
//  CmdFindinfilesRefresh( edit );
}

__EditCmd( CmdMode_editfile_from_findinfiles )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  edit.mode = editmode_t::editfile;
//  edit.findinfiles.matches.len = 0;
//  CmdSelectAll( edit.findinfiles.query );
//  CmdRemChL( edit.findinfiles.query );
//  FindinFilesResetCS( edit.findinfiles );
}

__EditCmd( CmdFindinfilesChoose )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  auto foundinfile = LookupElemByLinearIndex( fif.matches, fif.cursor );
#if USE_FILEMAPPED_OPEN
  auto file = FileOpenMappedExistingReadShareRead( ML( foundinfile->name ) );
#else
  auto file = FileOpen( ML( foundinfile->name ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
#endif
  if( file.loaded ) {
    EditOpenAndSetActiveTxt( edit, file );
    CmdMode_editfile_from_findinfiles( edit );
    auto open = GetActiveOpen( edit );
    AssertCrash( open );
#if USE_BUF_FOR_FILECONTENTSEARCH
    CmdSetSelection( open->txt, Cast( idx_t, &foundinfile->l ), Cast( idx_t, &foundinfile->r ) );
#else
    auto bof = GetBOF( open->txt.buf );
    auto sel_l = CursorCharR( open->txt.buf, bof, foundinfile->pos_match_l, 0 );
    auto sel_r = CursorCharR( open->txt.buf, sel_l, foundinfile->match_len, 0 );
    CmdSetSelection( open->txt, Cast( idx_t, &sel_l ), Cast( idx_t, &sel_r ) );
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
  fif.focus = Cast( findinfilesfocus_t, ( Cast( enum_t, fif.focus ) + 1 ) % Cast( enum_t, findinfilesfocus_t::COUNT ) );
  AssertCrash( Cast( enum_t, fif.focus ) < Cast( enum_t, findinfilesfocus_t::COUNT ) );
}

__EditCmd( CmdFindinfilesFocusU )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  fif.focus = Cast( findinfilesfocus_t, ( Cast( enum_t, fif.focus ) - 1 ) % Cast( enum_t, findinfilesfocus_t::COUNT ) );
  AssertCrash( Cast( enum_t, fif.focus ) < Cast( enum_t, findinfilesfocus_t::COUNT ) );
}

__EditCmd( CmdFindinfilesCursorU )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  auto nlines = misc ? misc : 1;
  fif.cursor -= MIN( nlines, fif.cursor );
  FindinFilesMakeCursorVisible( fif );
}

__EditCmd( CmdFindinfilesCursorD )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  auto nlines = misc ? misc : 1;
  fif.cursor += MIN( nlines, fif.matches.totallen - 1 - fif.cursor );
  FindinFilesMakeCursorVisible( fif );
}

__EditCmd( CmdFindinfilesScrollU )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  auto nlines = misc ? misc : 1;
  fif.scroll_start -= MIN( nlines, fif.scroll_start );
}

__EditCmd( CmdFindinfilesScrollD )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  auto nlines = misc ? misc : 1;
  fif.scroll_start += MIN( nlines, fif.matches.totallen - 1 - fif.scroll_start );
}

Inl void
ReplaceInFile( edit_t& edit, foundinfile_t* match, slice_t query, slice_t replacement )
{
  auto open = EditGetOpenedFile( edit, ML( match->name ) );
  if( !open ) {
#if USE_FILEMAPPED_OPEN
    auto file = FileOpenMappedExistingReadShareRead( ML( foundinfile->name ) );
#else
    auto file = FileOpen( ML( match->name ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
#endif
    if( file.loaded ) {
      bool opened_existing = 0;
      EditOpen( edit, file, &open, &opened_existing );
      AssertCrash( open );
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
    CmdSetSelection( open->txt, Cast( idx_t, &match->l ), Cast( idx_t, &match->r ) );
#else
    auto bof = GetBOF( open->txt.buf );
    auto sel_l = CursorCharR( open->txt.buf, bof, match->pos_match_l, 0 );
    auto sel_r = CursorCharR( open->txt.buf, sel_l, match->match_len, 0 );
    CmdSetSelection( open->txt, Cast( idx_t, &sel_l ), Cast( idx_t, &sel_r ) );
#endif

    auto contents = AllocSelection( open->txt );
    if( StringEquals( ML( contents ), ML( query ), 1 ) ) {
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

__EditCmd( CmdFindinfilesReplaceAtCursor )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  auto replacement = AllocContents( fif.replacement.buf );
  auto query = AllocContents( fif.query.buf );
  auto match = LookupElemByLinearIndex( fif.matches, fif.cursor );
  ReplaceInFile( edit, match, SliceFromString( query ), SliceFromString( replacement ) );
  Free( replacement );
  Free( query );
}

__EditCmd( CmdFindinfilesReplaceAll )
{
  AssertCrash( edit.mode == editmode_t::findinfiles );
  auto& fif = edit.findinfiles;
  auto replacement = AllocContents( fif.replacement.buf );
  auto query = AllocContents( fif.query.buf );
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
  XXXXXXXXXXXXXX
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
  auto find = AllocContents( edit.findrepl.find.buf );
  txtfind_t txtfind = { ML( find ), edit.findrepl.case_sens, edit.findrepl.word_boundary };
  CmdFindStringR( open->txt, Cast( idx_t, &txtfind ) );
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
  auto find = AllocContents( edit.findrepl.find.buf );
  txtfind_t txtfind = { ML( find ), edit.findrepl.case_sens, edit.findrepl.word_boundary };
  CmdFindStringL( open->txt, Cast( idx_t, &txtfind ) );
  Free( find );
}

Inl void
ReplaceSelection( txt_t& txt, slice_t find, slice_t replace )
{
  content_ptr_t sl, sr;
  GetSelect( txt, &sl, &sr );
  auto sel = AllocContents( txt.buf, sl, CountBytesBetween( txt.buf, sl, sr ) );
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
  auto find = AllocContents( edit.findrepl.find.buf );
  auto repl = AllocContents( edit.findrepl.replace.buf );
  ReplaceSelection( open->txt, SliceFromString( find ), SliceFromString( repl ) );
  txtfind_t txtfind = { ML( find ), edit.findrepl.case_sens, edit.findrepl.word_boundary };
  CmdFindStringR( open->txt, Cast( idx_t, &txtfind ) );
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
  auto find = AllocContents( edit.findrepl.find.buf );
  auto repl = AllocContents( edit.findrepl.replace.buf );
  ReplaceSelection( open->txt, SliceFromString( find ), SliceFromString( repl ) );
  txtfind_t txtfind = { ML( find ), edit.findrepl.case_sens, edit.findrepl.word_boundary };
  CmdFindStringL( open->txt, Cast( idx_t, &txtfind ) );
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
  auto gotoline = AllocContents( edit.gotoline.buf );
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
  if( !TxtLen( edit.opened_search ) ) {
    ForList( elem, edit.openedmru ) {
      auto open = elem->value;
      *AddBack( edit.search_matches ) = open;
    }
  } else {
    auto key = AllocContents( edit.opened_search.buf );
    ForList( elem, edit.openedmru ) {
      auto open = elem->value;
      idx_t pos;
      if( StringIdxScanR( &pos, ML( open->txt.filename ), 0, ML( key ), 0, 0 ) ) {
        *AddBack( edit.search_matches ) = open;
      }
    }
    Free( key );
  }
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
  auto obj = EditGetOpenedSelection( edit );
  if( obj ) {
#if USE_FILEMAPPED_OPEN
    auto file = FileOpenMappedExistingReadShareRead( ML( *obj ) );
#else
    auto file = FileOpen( ML( *obj ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
#endif
    if( file.loaded ) {
      EditOpenAndSetActiveTxt( edit, file );
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
        cs_undo_absolute_t cs;
        CsUndoAbsoluteFromTxt( open->txt, &cs );
        Kill( open->txt );
        Init( open->txt );
        TxtLoad( open->txt, edit.file_externalmerge );
        ApplyCsUndoAbsolute( open->txt, cs );
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
  cs_undo_absolute_t cs;
  CsUndoAbsoluteFromTxt( open->txt, &cs );
  Kill( open->txt );
  Init( open->txt );
  TxtLoad( open->txt, edit.file_externalmerge );
  ApplyCsUndoAbsolute( open->txt, cs );
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

Enumc( editlayer_t )
{
  bkgd,
  sel,
  txt,
  scroll_bkgd,
  scroll_btn,

  COUNT
};


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

  u8 diff_count[128];
  idx_t diff_count_len = 0;
  CsFromIntegerU( AL( diff_count ), &diff_count_len, open.txt.buf.diffs.len, 1 );
  DrawString(
    stream,
    font,
    bounds.p0,
    GetZ( zrange, editlayer_t::txt ),
    bounds,
    rgba_text,
    spaces_per_tab,
    diff_count, diff_count_len
    );

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
  bounds.p0.y += line_h;
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
  if( edit.horzview ) {
    auto open_r = edit.active[0];
    auto open_l = edit.active[1];

    if( open_l ) {
      auto bounds_l = _rect( bounds.p0, _vec2( bounds.p0.x + 0.5f * ( bounds.p1.x - bounds.p0.x ), bounds.p1.y ) );

      _RenderStatusBar(
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
    if( open_r ) {
      auto bounds_r = _rect( _vec2( bounds.p0.x + 0.5f * ( bounds.p1.x - bounds.p0.x ), bounds.p0.y ), bounds.p1 );

      _RenderStatusBar(
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
  } else {
    auto open = edit.active[edit.horzfocus_l];
    if( open ) {
      _RenderStatusBar(
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
  ProfFunc();

  fileopener_t& fo = edit.fileopener;

  AssertCrash( edit.mode == editmode_t::fileopener  ||  edit.mode == editmode_t::fileopener_renaming );

  auto rgba_text = GetPropFromDb( vec4<f32>, rgba_text );
  auto rgba_cursor_bkgd = GetPropFromDb( vec4<f32>, rgba_cursor_bkgd );
  auto rgba_cursor_size_text = GetPropFromDb( vec4<f32>, rgba_cursor_size_text );
  auto rgba_size_text = GetPropFromDb( vec4<f32>, rgba_size_text );
  auto rgba_cursor_text = GetPropFromDb( vec4<f32>, rgba_cursor_text );
  auto rgba_scroll_btn = GetPropFromDb( vec4<f32>, rgba_scroll_btn );
  auto px_column_spacing = GetPropFromDb( f32, f32_px_column_spacing );
  auto spaces_per_tab = GetPropFromDb( u8, u8_spaces_per_tab );

  auto scroll_pct = GetPropFromDb( f32, f32_scroll_pct );
  auto px_scroll = MAX( 16.0f, Round32( scroll_pct * MinElem( bounds.p1 - bounds.p0 ) ) );

  auto line_h = FontLineH( font );

  static const auto label_dir = SliceFromCStr( "Dir: " );
  auto label_dir_w = LayoutString( font, spaces_per_tab, ML( label_dir ) );

  static const auto label_ignore_ext = SliceFromCStr( "Ext Ignore: " );
  auto label_ignore_ext_w = LayoutString( font, spaces_per_tab, ML( label_ignore_ext ) );

  static const auto label_ignore_substring = SliceFromCStr( "Substring Ignore: " );
  auto label_ignore_substring_w = LayoutString( font, spaces_per_tab, ML( label_ignore_substring ) );

  static const auto label_find = SliceFromCStr( "Find: " );
  auto label_find_w = LayoutString( font, spaces_per_tab, ML( label_find ) );

  auto maxlabelw = MAX4( label_dir_w, label_ignore_ext_w, label_ignore_substring_w, label_find_w );

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
    TxtLayoutSingleLineSubset( \
      _txt, \
      GetBOF( _txt.buf ), \
      TxtLen( _txt ), \
      font \
      ); \
    TxtRenderSingleLineSubset( \
      _txt, \
      stream, \
      font, \
      _rect( bounds.p0 + _vec2( _maxlabelw, 0.0f ), bounds.p1 ), \
      ZRange( zrange, editlayer_t::txt ), \
      0, \
      _infocus, \
      _infocus \
      ); \
    bounds.p0.y += line_h; \



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

    bounds.p0.y += line_h;

  } else {
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

    bounds.p0.y += line_h;
  }

  auto nlines_screen_floored = Cast( idx_t, ( bounds.p1.y - bounds.p0.y ) / line_h );
  fo.pageupdn_distance = MAX( 1, nlines_screen_floored / 2 );
  fo.scroll_end = fo.scroll_start + MIN( nlines_screen_floored, fo.matches.totallen );

  AssertCrash( fo.scroll_start <= fo.matches.totallen );
  auto nlines_render = MIN( 1 + nlines_screen_floored, fo.matches.totallen - fo.scroll_start );

  // render the scrollbar.
  if( ScrollbarVisible( bounds, px_scroll ) ) {
    auto t_start = CLAMP( Cast( f32, fo.scroll_start ) / Cast( f32, fo.matches.totallen ), 0, 1 );
    auto t_end   = CLAMP( Cast( f32, fo.scroll_end   ) / Cast( f32, fo.matches.totallen ), 0, 1 );
    ScrollbarRender(
      stream,
      bounds,
      t_start,
      t_end,
      GetZ( zrange, editlayer_t::scroll_bkgd ),
      GetZ( zrange, editlayer_t::scroll_btn ),
      px_scroll,
      GetPropFromDb( vec4<f32>, rgba_scroll_bkgd ),
      rgba_scroll_btn
      );

    bounds.p1.x -= px_scroll;
  }

  { // draw bkgd
    RenderQuad(
      stream,
      GetPropFromDb( vec4<f32>, rgba_text_bkgd ),
      bounds,
      bounds,
      GetZ( zrange, dirlayer_t::bkgd )
      );
  }

  { // cursor
    if( fo.scroll_start <= fo.cursor  &&  fo.cursor < fo.scroll_end ) {
      auto p0 = bounds.p0 + _vec2( 0.0f, line_h * ( fo.cursor - fo.scroll_start ) );
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


  { // elem sizes
    f32 max_sizetxt_w = 0;

    if( fo.matches.totallen ) {
      auto pa_iter = MakeIteratorAtLinearIndex( fo.matches, fo.scroll_start );
      For( i, 0, nlines_render ) {
        auto elem = *GetElemAtIterator( fo.matches, pa_iter );
        pa_iter = IteratorMoveR( fo.matches, pa_iter );

        auto sizetxt_w = LayoutString( font, spaces_per_tab, ML( elem->sizetxt ) );
        max_sizetxt_w = MAX( max_sizetxt_w, sizetxt_w );
      }
    }
    if( fo.matches.totallen ) {
      auto pa_iter = MakeIteratorAtLinearIndex( fo.matches, fo.scroll_start );
      For( i, 0, nlines_render ) {
        auto elem = *GetElemAtIterator( fo.matches, pa_iter );
        pa_iter = IteratorMoveR( fo.matches, pa_iter );

        auto sizetxt_w = LayoutString( font, spaces_per_tab, ML( elem->sizetxt ) ); // PERF: cache this?
        auto elem_origin = bounds.p0 + _vec2( max_sizetxt_w - sizetxt_w, line_h * i );
        DrawString(
          stream,
          font,
          elem_origin,
          GetZ( zrange, editlayer_t::txt ),
          bounds,
          ( fo.cursor == fo.scroll_start + i )  ?  rgba_cursor_size_text  :  rgba_size_text,
          spaces_per_tab,
          ML( elem->sizetxt )
          );
      }
    }
    bounds.p0.x += max_sizetxt_w + px_column_spacing;
  }

  { // elem readonlys
    static const auto readonly_label = Str( "readonly" );
    static const idx_t readonly_label_len = CstrLength( readonly_label );
    auto readonly_w = LayoutString( font, spaces_per_tab, readonly_label, readonly_label_len );
    bool readonly_visible = 0;

    if( fo.matches.totallen ) {
      auto pa_iter = MakeIteratorAtLinearIndex( fo.matches, fo.scroll_start );
      For( i, 0, nlines_render ) {
        auto elem = *GetElemAtIterator( fo.matches, pa_iter );
        pa_iter = IteratorMoveR( fo.matches, pa_iter );

        auto elem_origin = bounds.p0 + _vec2<f32>( 0, line_h * i );
        if( elem->readonly ) {
          readonly_visible = 1;
          DrawString(
            stream,
            font,
            elem_origin,
            GetZ( zrange, editlayer_t::txt ),
            bounds,
            ( fo.cursor == fo.scroll_start + i )  ?  rgba_cursor_size_text  :  rgba_size_text,
            spaces_per_tab,
            readonly_label, readonly_label_len
            );
        }
      }
    }
    if( readonly_visible ) {
      bounds.p0.x += readonly_w + px_column_spacing;
    }
  }

  { // elem names
    if( fo.matches.totallen ) {
      auto pa_iter = MakeIteratorAtLinearIndex( fo.matches, fo.scroll_start );
      For( i, 0, nlines_render ) {
        auto elem = *GetElemAtIterator( fo.matches, pa_iter );
        pa_iter = IteratorMoveR( fo.matches, pa_iter );

        auto elem_p0 = bounds.p0 + _vec2<f32>( 0, line_h * i );
        auto elem_p1 = _vec2( bounds.p1.x, MIN( bounds.p1.y, elem_p0.y + line_h ) );
        if( edit.mode == editmode_t::fileopener_renaming  &&
            fo.renaming_row == fo.scroll_start + i )
        {
          TxtRender(
            fo.renaming_txt,
            target_valid,
            stream,
            font,
            _rect( elem_p0, elem_p1 ),
            ZRange( zrange, editlayer_t::txt ),
            timestep_realtime,
            timestep_fixed,
            1,
            0,
            0,
            0
            );
        } else {
          DrawString(
            stream,
            font,
            elem_p0,
            GetZ( zrange, editlayer_t::txt ),
            bounds,
            ( fo.cursor == fo.scroll_start + i )  ?  rgba_cursor_text  :  rgba_text,
            spaces_per_tab,
            ML( elem->name )
            );
        }
      }
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
  auto rgba_cursor_size_text = GetPropFromDb( vec4<f32>, rgba_cursor_size_text );
  auto rgba_wordmatch_bkgd = GetPropFromDb( vec4<f32>, rgba_wordmatch_bkgd );
  auto rgba_size_text = GetPropFromDb( vec4<f32>, rgba_size_text );
  auto rgba_cursor_text = GetPropFromDb( vec4<f32>, rgba_cursor_text );
  auto rgba_scroll_btn = GetPropFromDb( vec4<f32>, rgba_scroll_btn );
  auto spaces_per_tab = GetPropFromDb( u8, u8_spaces_per_tab );

  auto scroll_pct = GetPropFromDb( f32, f32_scroll_pct );
  auto px_scroll = MAX( 16.0f, Round32( scroll_pct * MinElem( bounds.p1 - bounds.p0 ) ) );

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
      bounds.p0.y += line_h;

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

      TxtLayoutSingleLineSubset(
        edit.opened_search,
        GetBOF( edit.opened_search.buf ),
        TxtLen( edit.opened_search ),
        font
        );
      TxtRenderSingleLineSubset(
        edit.opened_search,
        stream,
        font,
        _rect( bounds.p0 + _vec2( search_w, 0.0f ), bounds.p1 ),
        ZRange( zrange, editlayer_t::txt ),
        0,
        1,
        1
        );

      bounds.p0.y += line_h;

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
      static const auto unsaved_len = CstrLength( unsaved );
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

        bounds.p0.y += line_h;
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

        bounds.p0.y += line_h;
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

        TxtLayoutSingleLineSubset(
          edit.findrepl.find,
          GetBOF( edit.findrepl.find.buf ),
          TxtLen( edit.findrepl.find ),
          font
          );
        TxtRenderSingleLineSubset(
          edit.findrepl.find,
          stream,
          font,
          _rect( bounds.p0 + _vec2( findlabel_w, 0.0f ), bounds.p1 ),
          zrange,
          0,
          show_cursor,
          1
          );

        bounds.p0.y += line_h;
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

        TxtLayoutSingleLineSubset(
          edit.findrepl.replace,
          GetBOF( edit.findrepl.replace.buf ),
          TxtLen( edit.findrepl.replace ),
          font
          );
        TxtRenderSingleLineSubset(
          edit.findrepl.replace,
          stream,
          font,
          _rect( bounds.p0 + _vec2( replacelabel_w, 0.0f ), bounds.p1 ),
          zrange,
          0,
          show_cursor,
          1
          );

        bounds.p0.y += line_h;
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

      TxtLayoutSingleLineSubset(
        edit.gotoline,
        GetBOF( edit.gotoline.buf ),
        TxtLen( edit.gotoline ),
        font
        );
      TxtRenderSingleLineSubset(
        edit.gotoline,
        stream,
        font,
        _rect( bounds.p0 + _vec2( gotolinelabel_w, 0.0f ), bounds.p1 ),
        zrange,
        1,
        1,
        1
        );

      bounds.p0.y += line_h;

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

      bounds.p0.y += line_h;

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

      bounds.p0.y += line_h;

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

      bounds.p0.y += line_h;

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
      auto& fif = edit.findinfiles;

      { // case sens
        auto bind0 = GetPropFromDb( glwkeybind_t, keybind_findinfiles_toggle_case_sensitive );
        auto key0 = KeyStringFromGlw( bind0.key );
        AssertCrash( !key0.mem[key0.len] ); // cstr generated by compiler.
        auto label = AllocFormattedString( "Case sensitive: %u -- Press [ %s ] to toggle.", fif.case_sens, key0.mem );
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

        bounds.p0.y += line_h;
      }

      { // word boundary
        auto bind0 = GetPropFromDb( glwkeybind_t, keybind_findinfiles_toggle_word_boundary );
        auto key0 = KeyStringFromGlw( bind0.key );
        AssertCrash( !key0.mem[key0.len] ); // cstr generated by compiler.
        auto label = AllocFormattedString( "Whole word: %u -- Press [ %s ] to toggle.", fif.word_boundary, key0.mem );
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

        bounds.p0.y += line_h;
      }

      static const auto label_dir = SliceFromCStr( "Dir: " );
      auto label_dir_w = LayoutString( font, spaces_per_tab, ML( label_dir ) );

      static const auto label_ignore = SliceFromCStr( "Ignore: " );
      auto label_ignore_w = LayoutString( font, spaces_per_tab, ML( label_ignore ) );

      static const auto label_find = SliceFromCStr( "Find: " );
      auto label_find_w = LayoutString( font, spaces_per_tab, ML( label_find ) );

      static const auto label_repl = SliceFromCStr( "Repl: " );
      auto label_repl_w = LayoutString( font, spaces_per_tab, ML( label_repl ) );

      auto maxlabelw = MAX4( label_dir_w, label_ignore_w, label_find_w, label_repl_w );

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
        if( fif.ncontexts_active ) {
          static const auto label = SliceFromCStr( " % files scanned..." );
          auto label_w = LayoutString( font, spaces_per_tab, ML( label ) );

          u8 count[128];
          idx_t count_len = 0;
          auto pct = 100.0f - ( fif.ncontexts_active * 100.0f ) / fif.max_ncontexts_active;
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
        } else {
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

        bounds.p0.y += line_h;
      }

      auto nlines_screen_floored = Cast( idx_t, ( bounds.p1.y - bounds.p0.y ) / line_h );
      fif.pageupdn_distance = MAX( 1, nlines_screen_floored / 2 );
      fif.scroll_end = fif.scroll_start + MIN( nlines_screen_floored, fif.matches.totallen );

      // render the scrollbar.
      if( ScrollbarVisible( bounds, px_scroll ) ) {
        auto t_start = CLAMP( Cast( f32, fif.scroll_start ) / Cast( f32, fif.matches.totallen ), 0, 1 );
        auto t_end   = CLAMP( Cast( f32, fif.scroll_end   ) / Cast( f32, fif.matches.totallen ), 0, 1 );
        ScrollbarRender(
          stream,
          bounds,
          t_start,
          t_end,
          GetZ( zrange, editlayer_t::scroll_bkgd ),
          GetZ( zrange, editlayer_t::scroll_btn ),
          px_scroll,
          GetPropFromDb( vec4<f32>, rgba_scroll_bkgd ),
          rgba_scroll_btn
          );

        bounds.p1.x -= px_scroll;
      }

      { // bkgd
        RenderQuad(
          stream,
          GetPropFromDb( vec4<f32>, rgba_text_bkgd ),
          bounds,
          GetZ( zrange, editlayer_t::bkgd )
          );
      }

      if( fif.matches.totallen ) {
        fontlayout_t layout;
        FontInit( layout );

        AssertCrash( fif.scroll_start <= fif.matches.totallen );
        auto pa_iter = MakeIteratorAtLinearIndex( fif.matches, fif.scroll_start );

        auto nlines_render = MIN( 1 + nlines_screen_floored, fif.matches.totallen - fif.scroll_start );
        For( i, 0, nlines_render ) {
          auto elem = GetElemAtIterator( fif.matches, pa_iter );
          pa_iter = IteratorMoveR( fif.matches, pa_iter );

          auto is_cursor = fif.scroll_start + i == fif.cursor;

          // cursor
          if( is_cursor ) {
            auto p0 = bounds.p0 + _vec2( 0.0f, line_h * i );
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

          // elem names
          {
            auto elem_p0 = bounds.p0 + _vec2( 0.0f, line_h * i );
            auto elem_p1 = _vec2( bounds.p1.x, elem_p0.y + line_h );
            slice_t name = elem->name;
            auto prefix_len = fif.matches_dir.len + 1; // include forwslash
            AssertCrash( name.len >= prefix_len );
            name.mem += prefix_len;
            name.len -= prefix_len;

            f32 name_w = LayoutString( font, spaces_per_tab, ML( name ) );
            f32 space_w = LayoutString( font, spaces_per_tab, Str( "   " ), 3 );
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

            elem_p0.x += name_w + space_w;

            FontAddLayoutLine( font, layout, ML( elem->sample ), spaces_per_tab );
            idx_t line = i;
            f32 match_start = FontSumAdvances( layout, line, 0, elem->sample_match_offset );
            f32 match_w = FontSumAdvances( layout, line, elem->sample_match_offset, elem->sample_match_len );
            f32 match_end = match_start + match_w;

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

    } break;

    default: UnreachableCrash();
  }
}









// =================================================================================
// MOUSE

Inl idx_t
FileopenerMapMouseToCursor(
  fileopener_t& fo,
  rectf32_t bounds,
  vec2<s32> m,
  font_t& font,
  vec2<s8> px_click_correct
  )
{
  auto line_h = FontLineH( font );

  f32 y_frac = ( m.y - bounds.p0.y + px_click_correct.y ) / ( bounds.p1.y - bounds.p0.y );
  idx_t nlines_screen_max = Cast( idx_t, ( bounds.p1.y - bounds.p0.y ) / line_h );
  idx_t cy = Cast( idx_t, y_frac * nlines_screen_max );

  // TODO: same hit test rects as txt.

  cy += fo.scroll_start;
  cy = MAX( cy, 5 ) - 5; // -5 for the cwd, ignore lines, results count, etc.
  idx_t r = MIN( cy, fo.matches.totallen - 1 );
  return r;
}

Inl idx_t
FindinfilesMapMouseToCursor(
  findinfiles_t& fif,
  rectf32_t bounds,
  vec2<s32> m,
  font_t& font,
  vec2<s8> px_click_correct
  )
{
  auto line_h = FontLineH( font );

  f32 y_frac = ( m.y - bounds.p0.y + px_click_correct.y ) / ( bounds.p1.y - bounds.p0.y );
  idx_t nlines_screen_max = Cast( idx_t, ( bounds.p1.y - bounds.p0.y ) / line_h );
  idx_t cy = Cast( idx_t, y_frac * nlines_screen_max );

  // TODO: same hit test rects as txt.

  cy += fif.scroll_start;
  cy = MAX( cy, 7 ) - 7; // -7 for the cwd, ignore lines, results count, etc.
  idx_t r = MIN( cy, fif.matches.totallen - 1 );
  return r;
}

void
FileopenerControlMouse(
  edit_t& edit,
  bool& target_valid,
  font_t& font,
  rectf32_t bounds,
  glwmouseevent_t type,
  glwmousebtn_t btn,
  bool* alreadydn,
  bool* keyalreadydn,
  vec2<s32> m,
  vec2<s32> raw_delta,
  s32 dwheel
  )
{
  ProfFunc();

  auto& fo = edit.fileopener;

  auto px_click_correct = _vec2<s8>(); // TODO: mouse control.
  auto scroll_nlines = GetPropFromDb( u8, u8_scroll_nlines );
  auto scroll_sign = GetPropFromDb( s8, s8_scroll_sign );
  auto dblclick_period_sec = GetPropFromDb( f64, f64_dblclick_period_sec );

  auto keymods = GlwKeyModifiersDown();
  bool mod_isdn = AnyDown( keymods );

  if( !GlwMouseInsideRect( m, bounds ) ) {
    // clear all interactivity state.
    fo.dblclick.first_made = 0;
    return;
  }

  switch( type ) {

    case glwmouseevent_t::wheelmove: {
      if( dwheel  &&  !mod_isdn ) {
        dwheel *= scroll_sign;
        dwheel *= scroll_nlines;
        if( dwheel < 0 ) {
          CmdFileopenerScrollU( edit, Cast( idx_t, -dwheel ) );
        } else {
          CmdFileopenerScrollD( edit, Cast( idx_t, dwheel ) );
        }
        target_valid = 0;
      }
    } break;

    case glwmouseevent_t::dn: {

      switch( btn ) {
        case glwmousebtn_t::l: {

          fo.cursor = FileopenerMapMouseToCursor(
            fo,
            bounds,
            m,
            font,
            px_click_correct
            );
          bool same_cursor = ( fo.cursor == fo.dblclick.first_cursor );
          bool double_click = ( fo.dblclick.first_made & same_cursor );
          if( double_click ) {
            if( TimeSecFromClocks64( TimeClock() - fo.dblclick.first_clock ) <= dblclick_period_sec ) {
              fo.dblclick.first_made = 0;
              CmdFileopenerChoose( edit );
            } else {
              fo.dblclick.first_clock = TimeClock();
            }
          } else {
            fo.dblclick.first_made = 1;
            fo.dblclick.first_clock = TimeClock();
            fo.dblclick.first_cursor = fo.cursor;
          }
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

void
FindinfilesControlMouse(
  edit_t& edit,
  bool& target_valid,
  font_t& font,
  rectf32_t bounds,
  glwmouseevent_t type,
  glwmousebtn_t btn,
  bool* alreadydn,
  bool* keyalreadydn,
  vec2<s32> m,
  vec2<s32> raw_delta,
  s32 dwheel
  )
{
  ProfFunc();

  auto px_click_correct = _vec2<s8>(); // TODO: mouse control.
  auto scroll_nlines = GetPropFromDb( u8, u8_scroll_nlines );
  auto scroll_sign = GetPropFromDb( s8, s8_scroll_sign );

  auto keymods = GlwKeyModifiersDown();
  bool mod_isdn = AnyDown( keymods );

  switch( type ) {

    case glwmouseevent_t::wheelmove: {
      if( dwheel  &&  !mod_isdn ) {
        dwheel *= scroll_sign;
        dwheel *= scroll_nlines;
        if( dwheel < 0 ) {
          CmdFindinfilesScrollU( edit, Cast( idx_t, -dwheel ) );
        } else {
          CmdFindinfilesScrollD( edit, Cast( idx_t, dwheel ) );
        }
        target_valid = 0;
      }
    } break;

    case glwmouseevent_t::dn: {

      switch( btn ) {
        case glwmousebtn_t::l: {
          edit.findinfiles.cursor = FindinfilesMapMouseToCursor(
            edit.findinfiles,
            bounds,
            m,
            font,
            px_click_correct
            );
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
    } break;

    default: UnreachableCrash();
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
  bool* alreadydn,
  bool* keyalreadydn,
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
      // status bar
      bounds.p0.y += line_h;

      edittxtopen_t* open = 0;
      rectf32_t active_bounds = {};

      if( edit.horzview ) {
        auto open_r = edit.active[0];
        auto open_l = edit.active[1];

        auto half_x = 0.5f * ( bounds.p0.x + bounds.p1.x );
        auto bounds_l = _rect( bounds.p0, _vec2( half_x, bounds.p1.y ) );
        auto bounds_r = _rect( _vec2( half_x, bounds.p0.y ), bounds.p1 );

        if( edit.horzfocus_l ) {
          open = open_l;
          active_bounds = bounds_l;
        } else {
          open = open_r;
          active_bounds = bounds_r;
        }
      } else {
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
          alreadydn,
          keyalreadydn,
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
        alreadydn,
        keyalreadydn,
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
        alreadydn,
        keyalreadydn,
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
  bool* alreadydn,
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
  glwkeylocks_t& keylocks,
  bool* alreadydn
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
            ExecuteCmdMap( edit, AL( table ), key, alreadydn, target_valid, ran_cmd );
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
            keylocks,
            alreadydn
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
            ExecuteCmdMap( edit, AL( table ), key, alreadydn, target_valid, ran_cmd );
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
            ExecuteCmdMap( edit, AL( table ), key, alreadydn, target_valid, ran_cmd );
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
          keylocks,
          alreadydn
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
            ExecuteCmdMap( edit, AL( table ), key, alreadydn, target_valid, ran_cmd );
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
          keylocks,
          alreadydn
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
            ExecuteCmdMap( edit, AL( table ), key, alreadydn, target_valid, ran_cmd );
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
            ExecuteCmdMap( edit, AL( table ), key, alreadydn, target_valid, ran_cmd );
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
          keylocks,
          alreadydn
          );
        // auto-update the matches, since it's pretty fast.
        if( content_changed ) {
          CmdUpdateSearchMatches( edit );
        }
      }
    } break;

    case editmode_t::fileopener: {
      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // edit level commands
            edit_cmdmap_t table[] = {
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
            ExecuteCmdMap( edit, AL( table ), key, alreadydn, target_valid, ran_cmd );
          } __fallthrough;

          case glwkeyevent_t::repeat: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_focus_u       ), CmdFileopenerFocusU  ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_focus_d       ), CmdFileopenerFocusD  ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_cursor_u      ), CmdFileopenerCursorU ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_cursor_d      ), CmdFileopenerCursorD ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_cursor_page_u ), CmdFileopenerCursorU , edit.fileopener.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_cursor_page_d ), CmdFileopenerCursorD , edit.fileopener.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_scroll_u      ), CmdFileopenerScrollU , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_scroll_d      ), CmdFileopenerScrollD , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_scroll_page_u ), CmdFileopenerScrollU , edit.fileopener.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_scroll_page_d ), CmdFileopenerScrollD , edit.fileopener.pageupdn_distance ),
            };
            ExecuteCmdMap( edit, AL( table ), key, alreadydn, target_valid, ran_cmd );
          } break;

          case glwkeyevent_t::up: {
          } break;

          default: UnreachableCrash();
        }
      }

      if( !ran_cmd ) {
        switch( edit.fileopener.focus ) {
          case fileopenerfocus_t::dir: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              edit.fileopener.cwd,
              kb_command,
              target_valid,
              content_changed,
              ran_cmd,
              type,
              key,
              keylocks,
              alreadydn
              );
            if( content_changed ) {
//              CmdFileopenerRefresh( edit );
            }
          } break;
          case fileopenerfocus_t::query: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              edit.fileopener.query,
              kb_command,
              target_valid,
              content_changed,
              ran_cmd,
              type,
              key,
              keylocks,
              alreadydn
              );
            // auto-update the matches, since it's pretty fast.
            if( content_changed ) {
              CmdFileopenerUpdateMatches( edit );
            }
          } break;
          case fileopenerfocus_t::ignored_filetypes: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              edit.fileopener.ignored_filetypes,
              kb_command,
              target_valid,
              content_changed,
              ran_cmd,
              type,
              key,
              keylocks,
              alreadydn
              );
            // auto-update the matches, since it's pretty fast.
            if( content_changed ) {
              CmdFileopenerUpdateMatches( edit );
            }
          } break;
          case fileopenerfocus_t::ignored_substrings: {
            bool content_changed = 0;
            TxtControlKeyboardSingleLine(
              edit.fileopener.ignored_substrings,
              kb_command,
              target_valid,
              content_changed,
              ran_cmd,
              type,
              key,
              keylocks,
              alreadydn
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
              ExecuteCmdMap( edit, AL( table ), key, alreadydn, target_valid, ran_cmd );
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
      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // dir level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_fileopener_from_fileopener_renaming ), CmdMode_fileopener_from_fileopener_renaming ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_renaming_apply                ), CmdFileopenerRenamingApply                  ),
            };
            ExecuteCmdMap( edit, AL( table ), key, alreadydn, target_valid, ran_cmd );
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
          edit.fileopener.renaming_txt,
          kb_command,
          target_valid,
          content_changed,
          ran_cmd,
          type,
          key,
          keylocks,
          alreadydn
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
            ExecuteCmdMap( edit, AL( table ), key, alreadydn, target_valid, ran_cmd );
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
          keylocks,
          alreadydn
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
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_editfile_from_findinfiles ), CmdMode_editfile_from_findinfiles ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_replace_at_cursor  ), CmdFindinfilesReplaceAtCursor     ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_replace_all        ), CmdFindinfilesReplaceAll          ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_refresh            ), CmdFindinfilesRefresh             ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_choose             ), CmdFindinfilesChoose              ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_save    ), CmdSave    ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_saveall ), CmdSaveAll ),
            };
            ExecuteCmdMap( edit, AL( table ), key, alreadydn, target_valid, ran_cmd );
          } __fallthrough;

          case glwkeyevent_t::repeat: {
            // edit level commands
            edit_cmdmap_t table[] = {
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_focus_u               ), CmdFindinfilesFocusU             ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_focus_d               ), CmdFindinfilesFocusD             ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_cursor_u              ), CmdFindinfilesCursorU            ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_cursor_d              ), CmdFindinfilesCursorD            ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_cursor_page_u         ), CmdFindinfilesCursorU            , fif.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_cursor_page_d         ), CmdFindinfilesCursorD            , fif.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_scroll_u              ), CmdFindinfilesScrollU            , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_scroll_d              ), CmdFindinfilesScrollD            , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_scroll_page_u         ), CmdFindinfilesScrollU            , fif.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_scroll_page_d         ), CmdFindinfilesScrollD            , fif.pageupdn_distance ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_toggle_case_sensitive ), CmdFindinfilesToggleCaseSens     ),
              _editcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_toggle_word_boundary  ), CmdFindinfilesToggleWordBoundary ),
            };
            ExecuteCmdMap( edit, AL( table ), key, alreadydn, target_valid, ran_cmd );
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
              keylocks,
              alreadydn
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
              keylocks,
              alreadydn
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
              keylocks,
              alreadydn
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
              keylocks,
              alreadydn
              );
          } break;
        }
      }
    } break;

    default: UnreachableCrash();
  }
}

