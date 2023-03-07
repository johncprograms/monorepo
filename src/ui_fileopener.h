// Copyright (c) John A. Carlos Jr., all rights reserved.

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
  stack_resizeable_pagelist_t<fileopener_row_t> pool;
  pagelist_t pool_mem;
};

Enumc( fileopenermode_t )
{
	normal,
	renaming,
};

struct
fileopener_t
{
  stack_resizeable_cont_t<fileopener_oper_t> history;
  idx_t history_idx;

  // pool is the backing store of actual files/dirs given our cwd and query.
  // based on the query, we'll filter down to some subset, stored in matches.
  // ui just shows matches.
  fileopenermode_t mode;
  fileopenerfocus_t focus;
  txt_t cwd;
  fsobj_t last_cwd_for_changecwd_rollback;
  txt_t query;
  txt_t ignored_filetypes;
  txt_t included_filetypes;
  txt_t ignored_substrings;
  stack_resizeable_pagelist_t<fileopener_row_t> pool;
  pagelist_t pool_mem; // reset everytime we fillpool.
  stack_resizeable_pagelist_t<fileopener_row_t*> matches; // points into pool.
  listview_t listview;

  u8 cache_line_padding_to_avoid_thrashing[64];
  asynccontext_fileopenerfillpool_t asynccontext_fillpool;
  u8 cache_line_padding_to_avoid_thrashing2[64];
  idx_t ncontexts_active;

  pagelist_t matches_mem; // reset everytime we regenerate matches.
  stack_resizeable_cont_t<slice_t> ignored_filetypes_list; // uses matches_mem as backing memory.
  stack_resizeable_cont_t<slice_t> included_filetypes_list;
  stack_resizeable_cont_t<slice_t> ignored_substrings_list;

  idx_t renaming_row; // which row we're renaming, if in renaming mode.
  txt_t renaming_txt;
};

void
Init( fileopener_t& fo )
{
  Alloc( fo.history, 512 );
  fo.history_idx = 0;

	fo.mode = fileopenermode_t::normal;
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

	fo.mode = fileopenermode_t::normal;
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

#define __FileopenerCmd( name )   void ( name )( fileopener_t& fo, idx_t misc = 0, idx_t misc2 = 0 )
#define __FileopenerCmdDef( name )   void ( name )( fileopener_t& fo, idx_t misc, idx_t misc2 )
typedef __FileopenerCmdDef( *pfn_fileopenercmd_t );

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
          if( StringEquals( ML( ext ), ML( *filter ), 0 ) ) {
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
          if( StringEquals( ML( ext ), ML( *filter ), 0 ) ) {
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
        if( StringIdxScanR( &pos, ML( elem->name ), 0, ML( *filter ), 0, 0 ) ) {
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
        if( !StringIdxScanR( &pos, ML( elem->name ), 0, ML( key ), 0, 0 ) ) {
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
  row->name.mem = AddPagelist( ac->pool_mem, u8, 1, len );
  row->name.len = len;
  Memmove( row->name.mem, name, len );

  row->is_file = is_file;
  if( is_file ) {
    row->size = filesize;
    row->readonly = readonly;
    row->sizetxt.mem = AddPagelist( ac->pool_mem, u8, 1, 32 );
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
  // auto fo = Cast( fileopener_t*, misc1 );

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
    elem->name.mem = AddPagelist( fo.pool_mem, u8, 1, elem->name.len );
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

#define __FileopenerOpenFileFromRow( name )   void ( name )( idx_t misc, slice_t filename, bool* loaded )
typedef __FileopenerOpenFileFromRow( *FnOpenFileFromRow ); // TODO: rename to snake case.

Inl void
FileopenerOpenRow(
	fileopener_t& fo,
	fileopener_row_t* row,
	FnOpenFileFromRow fnOpenFileFromRow,
	idx_t misc
	)
{
  fsobj_t name;
  name.len = 0;
  auto cwd = AllocContents( &fo.cwd.buf, eoltype_t::crlf );
  Memmove( AddBack( name, cwd.len ), ML( cwd ) );
  Memmove( AddBack( name ), "/", 1 );
  Memmove( AddBack( name, row->name.len ), ML( row->name ) );

  if( row->is_file ) {
		auto slice = SliceFromArray( name );
		bool loaded;
		fnOpenFileFromRow( misc, slice, &loaded );
		if( !loaded ) {
			auto cstr = AllocCstr( slice );
			LogUI( "[EDIT] Fileopener couldn't open file: \"%s\"", cstr );
			MemHeapFree( cstr );
		}
  }
  else {
    bool up_dir = MemEqual( ML( row->name ), "..", 2 );
    if( up_dir ) {
      FileopenerCwdUp( fo );
    }
    else {
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


__FileopenerCmd( CmdFileopenerUpdateMatches )
{
  FileopenerUpdateMatches( fo );
}

__FileopenerCmd( CmdFileopenerRefresh )
{
  FileopenerFillPool( fo );
  FileopenerUpdateMatches( fo );
}

__FileopenerCmd( CmdFileopenerChoose )
{
  if( fo.matches.totallen ) {
    AssertCrash( fo.listview.cursor < fo.matches.totallen );
    auto row = *LookupElemByLinearIndex( fo.matches, fo.listview.cursor );
    auto fnOpenFileFromRow = Cast( FnOpenFileFromRow, misc );
    FileopenerOpenRow( fo, row, fnOpenFileFromRow, misc2 );
  }
}

__FileopenerCmd( CmdFileopenerFocusD )
{
  fo.focus = NextValue( fo.focus );

  // save previous cwd, in case we fail to apply a changecwd.
  if( fo.focus == fileopenerfocus_t::dir ) {
    auto cwd = AllocContents( &fo.cwd.buf, eoltype_t::crlf );
    fo.last_cwd_for_changecwd_rollback.len = 0;
    Memmove( AddBack( fo.last_cwd_for_changecwd_rollback, cwd.len ), ML( cwd ) );
    Free( cwd );
  }
}

__FileopenerCmd( CmdFileopenerFocusU )
{
  fo.focus = PreviousValue( fo.focus );

  // save previous cwd, in case we fail to apply a changecwd.
  if( fo.focus == fileopenerfocus_t::dir ) {
    auto cwd = AllocContents( &fo.cwd.buf, eoltype_t::crlf );
    fo.last_cwd_for_changecwd_rollback.len = 0;
    Memmove( AddBack( fo.last_cwd_for_changecwd_rollback, cwd.len ), ML( cwd ) );
    Free( cwd );
  }
}

__FileopenerCmd( CmdFileopenerCursorU )
{
  ListviewCursorU( &fo.listview, misc );
}
__FileopenerCmd( CmdFileopenerCursorD )
{
  ListviewCursorD( &fo.listview, misc );
}
__FileopenerCmd( CmdFileopenerScrollU )
{
  ListviewScrollU( &fo.listview, misc );
}
__FileopenerCmd( CmdFileopenerScrollD )
{
  ListviewScrollD( &fo.listview, misc );
}

__FileopenerCmd( CmdFileopenerRecycle )
{
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


__FileopenerCmd( CmdFileopenerChangeCwdUp )
{
  FileopenerCwdUp( fo );
  FileopenerFillPool( fo );
  FileopenerUpdateMatches( fo );
  ListviewResetCS( &fo.listview );
}

__FileopenerCmd( CmdFileopenerNewFile )
{
  u8* default_name = Str( "new_file" );
  idx_t default_name_len = CstrLength( default_name );

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

  auto oper = FileopenerAddHistorical( fo, fileopener_opertype_t::newfile );
  oper->newfile = name;

  file_t file = FileOpen( ML( name ), fileopen_t::only_new, fileop_t::RW, fileop_t::R );
  AssertWarn( file.loaded );
  FileFree( file );
  FileopenerFillPool( fo );
  FileopenerUpdateMatches( fo );
}

__FileopenerCmd( CmdFileopenerNewDir )
{
  u8* default_name = Str( "new_dir" );
  idx_t default_name_len = CstrLength( default_name );

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

  auto oper = FileopenerAddHistorical( fo, fileopener_opertype_t::newdir );
  oper->newfile = name;

  bool created = DirCreate( ML( name ) );
  AssertWarn( created );
  FileopenerFillPool( fo );
  FileopenerUpdateMatches( fo );
}


__FileopenerCmd( CmdMode_fileopener_renaming_from_fileopener )
{
  if( !fo.matches.totallen ) {
    return;
  }

  fo.mode = fileopenermode_t::renaming;

  AssertCrash( fo.listview.cursor < fo.matches.totallen );
  auto row = *LookupElemByLinearIndex( fo.matches, fo.listview.cursor );
  fo.renaming_row = fo.listview.cursor;

  CmdSelectAll( fo.renaming_txt );
  CmdAddString( fo.renaming_txt, Cast( idx_t, row->name.mem ), row->name.len );
  CmdSelectAll( fo.renaming_txt );
}

Inl void
FileopenerRename( fileopener_t& fo, slice_t& src, slice_t& dst )
{
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

__FileopenerCmd( CmdFileopenerRenamingApply )
{
	AssertCrash( fo.mode == fileopenermode_t::renaming );
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
    FileopenerRename( fo, src, dst );

    Free( cwd );
  }

  fo.renaming_row = 0;
  CmdSelectAll( fo.renaming_txt );
  CmdRemChL( fo.renaming_txt );

  ListviewFixupCS( &fo.listview );

  fo.mode = fileopenermode_t::normal;
}

__FileopenerCmd( CmdMode_fileopener_from_fileopener_renaming )
{
	fo.mode = fileopenermode_t::normal;
}

__FileopenerCmd( CmdFileopenerUndo ) // TODO: finish writing this.
{
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
        FileopenerRename( fo, src, dst );
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

__FileopenerCmd( CmdFileopenerRedo )
{
}

Enumc( dirlayer_t ) // TODO: rename to folayer_t or something.
{
  bkgd,
  sel,
  txt,

  COUNT
};

void
FileopenerRender(
  fileopener_t& fo,
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

  auto timestep = MIN( timestep_realtime, timestep_fixed );

  auto rgba_text = GetPropFromDb( vec4<f32>, rgba_text );
  auto rgba_cursor_size_text = GetPropFromDb( vec4<f32>, rgba_cursor_size_text );
  auto rgba_size_text = GetPropFromDb( vec4<f32>, rgba_size_text );
  auto rgba_cursor_text = GetPropFromDb( vec4<f32>, rgba_cursor_text );
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
      GetZ( zrange, dirlayer_t::txt ), \
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
      ZRange( zrange, dirlayer_t::txt ), \
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
      GetZ( zrange, dirlayer_t::txt ),
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
      GetZ( zrange, dirlayer_t::txt ),
      bounds,
      rgba_text,
      spaces_per_tab,
      count, count_len
      );

    DrawString(
      stream,
      font,
      AlignCenter( bounds, label_w + count_w ) + _vec2( count_w, 0.0f ),
      GetZ( zrange, dirlayer_t::txt ),
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
		auto label_w = LayoutString( font, spaces_per_tab, ML( label_nothing_found ) );
		DrawString(
			stream,
			font,
			AlignCenter( bounds, label_w, line_h ),
			GetZ( zrange, dirlayer_t::txt ),
			bounds,
			rgba_text,
			spaces_per_tab,
			ML( label_nothing_found )
			);
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
          GetZ( zrange, dirlayer_t::txt ),
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
    static const idx_t readonly_label_len = CstrLength( readonly_label );
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
            GetZ( zrange, dirlayer_t::txt ),
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
        if( fo.mode == fileopenermode_t::renaming  &&
            fo.renaming_row == first_line + i )
        {
          TxtRenderSingleLineSubset(
            fo.renaming_txt,
            stream,
            font,
            0,
            _rect(elem_p0, elem_p1 ),
            ZRange( zrange, dirlayer_t::txt ),
            1,
            1,
            1
            );
        }
        else {
          idx_t last_slash = 0;
          auto found = StringIdxScanL( &last_slash, ML( elem->name ), elem->name.len, '/' );
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
              GetZ( zrange, dirlayer_t::txt ),
              bounds,
              rgba_size_text,
              spaces_per_tab,
              ML( path )
              );
            DrawString(
              stream,
              font,
              elem_p0 + _vec2( path_w, 0.0f ),
              GetZ( zrange, dirlayer_t::txt ),
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
              GetZ( zrange, dirlayer_t::txt ),
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

#undef DRAW_TEXTBOXLINE
}

void
FileopenerControlMouse(
  fileopener_t& fo,
  bool& target_valid,
  font_t& font,
  rectf32_t bounds,
  glwmouseevent_t type,
  glwmousebtn_t btn,
  vec2<s32> m,
  vec2<s32> raw_delta,
  s32 dwheel,
  FnOpenFileFromRow fnOpenFileFromRow,
  idx_t miscForOpenFile
  )
{
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
    CmdFileopenerChoose( fo, Cast( idx_t, fnOpenFileFromRow ), miscForOpenFile );
  }
}

struct
fileopener_cmdmap_t
{
  glwkeybind_t keybind;
  pfn_fileopenercmd_t fn;
  idx_t misc;
  idx_t misc2;
};

// TODO: eliminate with modern c++ versions?
Inl fileopener_cmdmap_t
_focmdmap(
  glwkeybind_t keybind,
  pfn_fileopenercmd_t fn,
  idx_t misc = 0,
  idx_t misc2 = 0
  )
{
  fileopener_cmdmap_t r;
  r.keybind = keybind;
  r.fn = fn;
  r.misc = misc;
  r.misc2 = misc2;
  return r;
}

Inl void
ExecuteCmdMap(
  fileopener_t& fo,
  fileopener_cmdmap_t* table,
  idx_t table_len,
  glwkey_t key,
  bool& target_valid,
  bool& ran_cmd
  )
{
  For( i, 0, table_len ) {
    auto entry = table + i;
    if( GlwKeybind( key, entry->keybind ) ) {
      entry->fn( fo, entry->misc, entry->misc2 );
      target_valid = 0;
      ran_cmd = 1;
    }
  }
}

void
FileopenerControlKeyboard(
  fileopener_t& fo,
  bool kb_command,
  bool& target_valid,
  bool& ran_cmd,
  glwkeyevent_t type,
  glwkey_t key,
  glwkeylocks_t& keylocks,
  FnOpenFileFromRow fnOpenFileFromRow,
  idx_t miscForOpenFile
  )
{
  ProfFunc();

	switch (fo.mode) {
    case fileopenermode_t::normal: {
      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // edit level commands
            fileopener_cmdmap_t table[] = {
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_fileopener_renaming_from_fileopener ), CmdMode_fileopener_renaming_from_fileopener ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_recycle_file_or_dir           ), CmdFileopenerRecycle                        ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_change_cwd_up                 ), CmdFileopenerChangeCwdUp                    ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_refresh                       ), CmdFileopenerRefresh                        ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_newfile                       ), CmdFileopenerNewFile                        ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_newdir                        ), CmdFileopenerNewDir                         ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_choose                        ), CmdFileopenerChoose                         , Cast( idx_t, fnOpenFileFromRow ), miscForOpenFile ),
            };
            ExecuteCmdMap( fo, AL( table ), key, target_valid, ran_cmd );
          } __fallthrough;

          case glwkeyevent_t::repeat: {
            // edit level commands
            fileopener_cmdmap_t table[] = {
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_focus_u       ), CmdFileopenerFocusU  ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_focus_d       ), CmdFileopenerFocusD  ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_cursor_u      ), CmdFileopenerCursorU ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_cursor_d      ), CmdFileopenerCursorD ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_cursor_page_u ), CmdFileopenerCursorU , fo.listview.pageupdn_distance ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_cursor_page_d ), CmdFileopenerCursorD , fo.listview.pageupdn_distance ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_scroll_u      ), CmdFileopenerScrollU , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_scroll_d      ), CmdFileopenerScrollD , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_scroll_page_u ), CmdFileopenerScrollU , fo.listview.pageupdn_distance ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_scroll_page_d ), CmdFileopenerScrollD , fo.listview.pageupdn_distance ),
            };
            ExecuteCmdMap( fo, AL( table ), key, target_valid, ran_cmd );
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
              CmdFileopenerUpdateMatches( fo );
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
              CmdFileopenerUpdateMatches( fo );
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
              CmdFileopenerUpdateMatches( fo );
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
              CmdFileopenerUpdateMatches( fo );
            }
          } break;
          default: UnreachableCrash();
        }
      }
    } break;

    case fileopenermode_t::renaming: {
      if( kb_command ) {
        switch( type ) {
          case glwkeyevent_t::dn: {
            // dir level commands
            fileopener_cmdmap_t table[] = {
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_mode_fileopener_from_fileopener_renaming ), CmdMode_fileopener_from_fileopener_renaming ),
              _focmdmap( GetPropFromDb( glwkeybind_t, keybind_fileopener_renaming_apply                ), CmdFileopenerRenamingApply                  ),
            };
            ExecuteCmdMap( fo, AL( table ), key, target_valid, ran_cmd );
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
	}
}

