// Copyright (c) John A. Carlos Jr., all rights reserved.

// TODO: consolidate naming to findinfiles or filecontentsearch

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
  stack_resizeable_cont_t<slice_t>* filenames;
  idx_t filenames_start;
  idx_t filenames_count;
  stack_resizeable_cont_t<slice_t>* ignored_filetypes_list;
  stack_resizeable_cont_t<slice_t>* included_filetypes_list;
  slice32_t key;
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
  txt_t included_filetypes;
  string_t matches_dir;
  listview_t listview;

  u8 cache_line_padding_to_avoid_thrashing[64];
  stack_resizeable_cont_t<asynccontext_filecontentsearch_t> asynccontexts;
  idx_t progress_num_files;
  idx_t progress_max;

  // used by all other threads as readonly:
  stack_resizeable_cont_t<slice_t> filenames;
  stack_resizeable_cont_t<slice_t> ignored_filetypes_list;
  stack_resizeable_cont_t<slice_t> included_filetypes_list;
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

#define __FindinfilesCmd( name )   void ( name )( findinfiles_t& fif, idx_t misc = 0, idx_t misc2 = 0 )
#define __FindinfilesCmdDef( name )   void ( name )( findinfiles_t& fif, idx_t misc, idx_t misc2 )
typedef __FindinfilesCmdDef( *pfn_findinfilescmd_t );

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
        if( StringEquals( ML( ext ), ML( *filter ), 0 ) ) {
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
        if( StringEquals( ML( ext ), ML( *filter ), 0 ) ) {
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
          instance->sample.mem = AddPagelist( ac->mem, u8, 1, instance->sample.len );
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
      auto mem = FileAlloc( file );
      ProfClose( tmp_FileAlloc );
      Prof( tmp_ContentSearch );
      bool found = 1;
      idx_t pos = 0;
      while( found ) {
        idx_t res = 0;
        found = StringIdxScanR( &res, ML( mem ), pos, ML( ac->key ), ac->case_sens, ac->word_boundary );
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
  // auto fif = Cast( findinfiles_t*, misc1 );

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


__FindinfilesCmd( CmdFindinfilesRefresh )
{
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
__FindinfilesCmd( CmdSelectAllQueryText )
{
  if( !IsEmpty( &fif.query.buf ) ) {
    CmdSelectAll( fif.query );
  }
  if( !IsEmpty( &fif.replacement.buf ) ) {
    CmdSelectAll( fif.replacement );
  }
}

// TODO: necessary? Should we factor things to use this instead?
Inl void
_ActiveFoundinfile( findinfiles_t& fif, foundinfile_t** result, bool* found )
{
	if( !fif.matches.totallen ) {
		*found = 0;
    return;
  }
  
  *result = LookupElemByLinearIndex( fif.matches, fif.listview.cursor );
  *found = 1;
}

#define __FindinfilesOpenFileForChoose( name )   void ( name )( idx_t misc, slice_t filename, bool* loaded, txt_t** txt, bool* opened_existing )
typedef __FindinfilesOpenFileForChoose( *FnOpenFileForChoose ); // TODO: rename to snake case.

__FindinfilesCmd( CmdFindinfilesChoose )
{
  if( !fif.matches.totallen ) {
    return;
  }
  auto foundinfile = LookupElemByLinearIndex( fif.matches, fif.listview.cursor );
  
  auto fnOpenFileForChoose = Cast( FnOpenFileForChoose, misc );
  bool loaded;
  txt_t* txt;
  bool opened_existing;
  fnOpenFileForChoose( misc2, foundinfile->name, &loaded, &txt, &opened_existing );
  if( !loaded ) {
		auto cstr = AllocCstr( foundinfile->name );
		LogUI( "[EDIT] FifChoose couldn't load file: \"%s\"", cstr );
		MemHeapFree( cstr );
		return;
  }
  
#if USE_BUF_FOR_FILECONTENTSEARCH
	txt_setsel_t setsel;
	setsel.x_start = foundinfile->l_x;
	setsel.y_start = foundinfile->l_y;
	setsel.x_end = foundinfile->r_x;
	setsel.y_end = foundinfile->r_y;
	// TODO: update the foundinfile positions when buf contents change.
	// for now, just clamp them to inbounds so it puts you near.
	ForceInbounds( &txt->buf, &setsel.x_start, &setsel.y_start );
	ForceInbounds( &txt->buf, &setsel.x_end, &setsel.y_end );
	CmdSetSelection( *txt, Cast( idx_t, &setsel ), 0 );
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
		// we're using the txt->buf.orig_file_contents, meaning we rely on
		// that not changing after we open this file above! this is so that we can make
		// a faster findinfiles that doesn't require opening a buf_t for everything.
		// here is where we'll convert our linear file map into a 2d range.
		//
		auto file_contents = txt->buf.orig_file_contents;
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
		ForceInbounds( &txt->buf, &setsel.x_start, &setsel.y_start );
		ForceInbounds( &txt->buf, &setsel.x_end, &setsel.y_end );
		CmdSetSelection( *txt, Cast( idx_t, &setsel ), 0 );
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
		auto eol_len = EolString( txt->eoltype ).len;
		bool converted_l = 0;
		bool converted_r = 0;
		FORALLLINES( &txt->buf, line, y )
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
			auto slice = SliceFromArray( txt->filename );
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
			ForceInbounds( &txt->buf, &setsel.x_start, &setsel.y_start );
			ForceInbounds( &txt->buf, &setsel.x_end, &setsel.y_end );
			CmdSetSelection( *txt, Cast( idx_t, &setsel ), 0 );
		}
	}
#endif
}

__FindinfilesCmd( CmdFindinfilesFocusD )
{
  fif.focus = NextValue( fif.focus );
}
__FindinfilesCmd( CmdFindinfilesFocusU )
{
  fif.focus = PreviousValue( fif.focus );
}
__FindinfilesCmd( CmdFindinfilesCursorU )
{
  ListviewCursorU( &fif.listview, misc );
}
__FindinfilesCmd( CmdFindinfilesCursorD )
{
  ListviewCursorD( &fif.listview, misc );
}
__FindinfilesCmd( CmdFindinfilesScrollU )
{
  ListviewScrollU( &fif.listview, misc );
}
__FindinfilesCmd( CmdFindinfilesScrollD )
{
  ListviewScrollD( &fif.listview, misc );
}

#define __FindinfilesOpenFileForReplace( name )   void ( name )( idx_t misc, slice_t filename, bool* loaded, txt_t** txt, bool** unsaved, bool* opened_existing )
typedef __FindinfilesOpenFileForReplace( *FnOpenFileForReplace ); // TODO: rename to snake case.

// TODO: group foundinfile_t's by filename, so we don't emit the same message a bunch of times for the same file.
Inl void
_ReplaceInFile(
	findinfiles_t& fif,
	foundinfile_t* foundinfile,
	slice_t query,
	slice_t replacement,
	FnOpenFileForReplace fnOpenFileForReplace,
	idx_t misc
	)
{
	bool loaded;
	txt_t* txt;
	bool* unsaved;
	bool opened_existing;
	fnOpenFileForReplace( misc, foundinfile->name, &loaded, &txt, &unsaved, &opened_existing );
	if( !loaded ) {
		auto cstr = AllocCstr( foundinfile->name );
		LogUI( "[EDIT] ReplaceInFile failed to load file: \"%s\"", cstr );
		MemHeapFree( cstr );
		return;
	}

#if USE_BUF_FOR_FILECONTENTSEARCH
	txt_setsel_t setsel;
	setsel.x_start = foundinfile->l_x;
	setsel.y_start = foundinfile->l_y;
	setsel.x_end = foundinfile->r_x;
	setsel.y_end = foundinfile->r_y;
	CmdSetSelection( *txt, Cast( idx_t, &setsel ), 0 );

	// WARNING! Duplicated below.
	auto contents = AllocSelection( *txt, eoltype_t::crlf );
	if( StringEquals( ML( contents ), ML( query ), 1 ) ) {
		CmdAddString( *txt, Cast( idx_t, replacement.mem ), replacement.len );
		*unsaved = 1;
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
		// we're using the txt->buf.orig_file_contents, meaning we rely on
		// that not changing after we open this file above! this is so that we can make
		// a faster findinfiles that doesn't require opening a buf_t for everything.
		// here is where we'll convert our linear file map into a 2d range.
		//
		auto file_contents = txt->buf.orig_file_contents;
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
		ForceInbounds( &txt->buf, &setsel.x_start, &setsel.y_start );
		ForceInbounds( &txt->buf, &setsel.x_end, &setsel.y_end );
		CmdSetSelection( *txt, Cast( idx_t, &setsel ), 0 );

		// WARNING! Duplicated above and below.
		auto contents = AllocSelection( *txt, eoltype_t::crlf );
		if( StringEquals( ML( contents ), ML( query ), 1 ) ) {
			CmdAddString( *txt, Cast( idx_t, replacement.mem ), replacement.len );
			*unsaved = 1;
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
		auto match_l = foundinfile->pos_match_l;
		auto match_len = foundinfile->match_len;
		auto match_r = match_l + match_len;
		auto cmatch_l = match_l;
		auto cmatch_r = match_r;
		u32 match_l_x = 0;
		u32 match_l_y = 0;
		u32 match_r_x = 0;
		u32 match_r_y = 0;
		auto eol_len = EolString( txt->eoltype ).len;
		bool converted_l = 0;
		bool converted_r = 0;
		FORALLLINES( &txt->buf, line, y )
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
			auto slice = SliceFromArray( txt->filename );
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
			ForceInbounds( &txt->buf, &setsel.x_start, &setsel.y_start );
			ForceInbounds( &txt->buf, &setsel.x_end, &setsel.y_end );
			CmdSetSelection( *txt, Cast( idx_t, &setsel ), 0 );

			// WARNING! Duplicated above.
			auto contents = AllocSelection( *txt, eoltype_t::crlf );
			if( StringEquals( ML( contents ), ML( query ), 1 ) ) {
				CmdAddString( *txt, Cast( idx_t, replacement.mem ), replacement.len );
				*unsaved = 1;
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

__FindinfilesCmd( CmdFindinfilesReplaceAtCursor )
{
  if( !fif.matches.totallen ) {
    return;
  }
  auto replacement = AllocContents( &fif.replacement.buf, eoltype_t::crlf );
  auto query = AllocContents( &fif.query.buf, eoltype_t::crlf );
  auto match = LookupElemByLinearIndex( fif.matches, fif.listview.cursor );
  auto fnOpenFileForReplace = Cast( FnOpenFileForReplace, misc );
  _ReplaceInFile( fif, match, SliceFromString( query ), SliceFromString( replacement ), fnOpenFileForReplace, misc2 );
  Free( replacement );
  Free( query );
}

__FindinfilesCmd( CmdFindinfilesReplaceAll )
{
  auto replacement = AllocContents( &fif.replacement.buf, eoltype_t::crlf );
  auto query = AllocContents( &fif.query.buf, eoltype_t::crlf );
  auto fnOpenFileForReplace = Cast( FnOpenFileForReplace, misc );
  ReverseForPrev( page, fif.matches.current_page ) {
    ReverseForLen( i, *page ) {
      auto match = Cast( foundinfile_t*, page->mem ) + i;
			_ReplaceInFile( fif, match, SliceFromString( query ), SliceFromString( replacement ), fnOpenFileForReplace, misc2 );
    }
  }
  Free( replacement );
  Free( query );
}

__FindinfilesCmd( CmdFindinfilesToggleCaseSens )
{
  fif.case_sens = !fif.case_sens;
}
__FindinfilesCmd( CmdFindinfilesToggleWordBoundary )
{
  fif.word_boundary = !fif.word_boundary;
}

Enumc( fiflayer_t )
{
  bkgd,
  sel,
  txt,

  COUNT
};

void
FindinfilesRender(
  findinfiles_t& fif,
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
    auto label = AllocFormattedString( "Case sensitive: %u -- Press [ %s ] to toggle.", fif.case_sens, key0.mem );
    auto label_w = LayoutString( font, spaces_per_tab, ML( label ) );
    DrawString(
      stream,
      font,
      AlignRight( bounds, label_w ),
      GetZ( zrange, fiflayer_t::txt ),
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
    auto label = AllocFormattedString( "Whole word: %u -- Press [ %s ] to toggle.", fif.word_boundary, key0.mem );
    auto label_w = LayoutString( font, spaces_per_tab, ML( label ) );
    DrawString(
      stream,
      font,
      AlignRight( bounds, label_w ),
      GetZ( zrange, fiflayer_t::txt ),
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


  #define DRAW_TEXTBOXLINE( _txt, _label, _labelw, _maxlabelw, _infocus ) \
    DrawString( \
      stream, \
      font, \
      bounds.p0 + _vec2( _maxlabelw - _labelw, 0.0f ), \
      GetZ( zrange, fiflayer_t::txt ), \
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
      ZRange( zrange, fiflayer_t::txt ), \
      0, \
      _infocus, \
      _infocus \
      ); \
    bounds.p0.y = MIN( bounds.p0.y + line_h, bounds.p1.y ); \


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
        GetZ( zrange, fiflayer_t::txt ),
        bounds,
        rgba_text,
        spaces_per_tab,
        count, count_len
        );

      DrawString(
        stream,
        font,
        AlignCenter( bounds, count_w + label_w ) + _vec2( count_w, 0.0f ),
        GetZ( zrange, fiflayer_t::txt ),
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
        GetZ( zrange, fiflayer_t::txt ),
        bounds,
        rgba_text,
        spaces_per_tab,
        count, count_len
        );

      DrawString(
        stream,
        font,
        AlignCenter( bounds, count_w + label_w ) + _vec2( count_w, 0.0f ),
        GetZ( zrange, fiflayer_t::txt ),
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
		auto label_w = LayoutString( font, spaces_per_tab, ML( label_no_results ) );
		DrawString(
			stream,
			font,
			AlignCenter( bounds, label_w, line_h ),
			GetZ( zrange, fiflayer_t::txt ),
			bounds,
			rgba_text,
			spaces_per_tab,
			ML( label_no_results )
			);
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
      auto found = StringIdxScanL( &last_slash, ML( name ), name.len, '/' );
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
          GetZ( zrange, fiflayer_t::txt ),
          bounds,
          rgba_size_text,
          spaces_per_tab,
          ML( path )
          );
        DrawString(
          stream,
          font,
          elem_p0 + _vec2( path_w, 0.0f ),
          GetZ( zrange, fiflayer_t::txt ),
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
          GetZ( zrange, fiflayer_t::txt ),
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
        GetZ( zrange, fiflayer_t::sel )
        );

      RenderText(
        stream,
        font,
        layout,
        elem_p0,
        GetZ( zrange, fiflayer_t::txt ),
        bounds,
        is_cursor  ?  rgba_cursor_text  :  rgba_text,
        line,
        0, elem->sample.len
        );
    }
  }

  FontKill( layout );
  
#undef DRAW_TEXTBOXLINE
}

void
FindinfilesControlMouse(
  findinfiles_t& fif,
  bool& target_valid,
  font_t& font,
  rectf32_t bounds,
  glwmouseevent_t type,
  glwmousebtn_t btn,
  vec2<s32> m,
  vec2<s32> raw_delta,
  s32 dwheel,
  FnOpenFileForChoose fnOpenFileForChoose,
  idx_t miscForChoose
  )
{
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
    CmdFindinfilesChoose( fif, Cast( idx_t, fnOpenFileForChoose ), miscForChoose );
  }
}


// =================================================================================
// KEYBOARD

struct
findinfiles_cmdmap_t
{
  glwkeybind_t keybind;
  pfn_findinfilescmd_t fn;
  idx_t misc;
  idx_t misc2;
};

Inl findinfiles_cmdmap_t
_fifcmdmap(
  glwkeybind_t keybind,
  pfn_findinfilescmd_t fn,
  idx_t misc = 0,
  idx_t misc2 = 0
  )
{
  findinfiles_cmdmap_t r;
  r.keybind = keybind;
  r.fn = fn;
  r.misc = misc;
  r.misc2 = misc;
  return r;
}

Inl void
ExecuteCmdMap(
  findinfiles_t& fif,
  findinfiles_cmdmap_t* table,
  idx_t table_len,
  glwkey_t key,
  bool& target_valid,
  bool& ran_cmd
  )
{
  For( i, 0, table_len ) {
    auto entry = table + i;
    if( GlwKeybind( key, entry->keybind ) ) {
      entry->fn( fif, entry->misc, entry->misc2 );
      target_valid = 0;
      ran_cmd = 1;
    }
  }
}

void
FindinfilesControlKeyboard(
  findinfiles_t& fif,
  bool kb_command,
  bool& target_valid,
  bool& ran_cmd,
  glwkeyevent_t type,
  glwkey_t key,
  glwkeylocks_t& keylocks,
  FnOpenFileForReplace fnOpenFileForReplace,
  idx_t miscForReplace,
  FnOpenFileForChoose fnOpenFileForChoose,
  idx_t miscForChoose
  )
{
  ProfFunc();

	if( kb_command ) {
		switch( type ) {
			case glwkeyevent_t::dn: {
				findinfiles_cmdmap_t table[] = {
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_replace_at_cursor  ), CmdFindinfilesReplaceAtCursor , Cast( idx_t, fnOpenFileForReplace ), miscForReplace ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_replace_all        ), CmdFindinfilesReplaceAll      , Cast( idx_t, fnOpenFileForReplace ), miscForReplace ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_refresh            ), CmdFindinfilesRefresh         ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_choose             ), CmdFindinfilesChoose          , Cast( idx_t, fnOpenFileForChoose  ), miscForChoose  ),
				};
				ExecuteCmdMap( fif, AL( table ), key, target_valid, ran_cmd );
			} __fallthrough;

			case glwkeyevent_t::repeat: {
				findinfiles_cmdmap_t table[] = {
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_focus_u               ), CmdFindinfilesFocusU             ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_focus_d               ), CmdFindinfilesFocusD             ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_cursor_u              ), CmdFindinfilesCursorU            ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_cursor_d              ), CmdFindinfilesCursorD            ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_cursor_page_u         ), CmdFindinfilesCursorU            , fif.listview.pageupdn_distance ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_cursor_page_d         ), CmdFindinfilesCursorD            , fif.listview.pageupdn_distance ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_scroll_u              ), CmdFindinfilesScrollU            , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_scroll_d              ), CmdFindinfilesScrollD            , Cast( idx_t, GetPropFromDb( f32, f32_lines_per_jump ) ) ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_scroll_page_u         ), CmdFindinfilesScrollU            , fif.listview.pageupdn_distance ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_scroll_page_d         ), CmdFindinfilesScrollD            , fif.listview.pageupdn_distance ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_toggle_case_sensitive ), CmdFindinfilesToggleCaseSens     ),
					_fifcmdmap( GetPropFromDb( glwkeybind_t, keybind_findinfiles_toggle_word_boundary  ), CmdFindinfilesToggleWordBoundary ),
				};
				ExecuteCmdMap( fif, AL( table ), key, target_valid, ran_cmd );
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
			case findinfilesfocus_t::COUNT: UnreachableCrash(); break;
		}
	}
} 
