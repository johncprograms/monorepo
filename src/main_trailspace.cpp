// build:console_x64_debug
// Copyright (c) John A. Carlos Jr., all rights reserved.

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
#include "ds_mtqueue_mrmw_nonresizeable.h"
#include "ds_mtqueue_mrsw_nonresizeable.h"
#include "ds_mtqueue_srmw_nonresizeable.h"
#include "ds_mtqueue_srsw_nonresizeable.h"
#include "threading.h"
#define LOGGER_ENABLED   0
#include "logger.h"
#define PROF_ENABLED   0
#define PROF_ENABLED_AT_LAUNCH   0
#include "profile.h"
#include "rand.h"
#include "allocator_heap_findleaks.h"
#include "mainthread.h"
#include "text_parsing.h"

int
Main( u8** args, idx_t args_len )
{
  if( args_len == 0 ) {
    printf( "Removes trailing whitespace from ANSI files.\n" );
    printf( "Checks all files in the CWD and recurses down.\n" );
    printf( "Could very well mangle Unicode files, so review the diffs afterwards.\n" );
    printf( "Specify filtering file extensions as arguments.\n" );
    printf( "Ex: \"trailspace h c cpp\"\n" );
    return 0;
  }
  
  fsobj_t cwd;
  FsGetCwd( cwd );

//    u8* cwd = Str( "C:\\doc\\dev\\cpp\\master\\te\\src" );
//    idx_t cwd_len = CstrLength( cwd );
//    u8* newargs[] = { Str( "h" ), Str( "cpp" ) };
//    vargs = newargs;
//    args_len = 2;

  stack_resizeable_cont_t<slice_t> files;
  Alloc( files, 1024 );
  pagelist_t pagelist;
  Init( pagelist, 64000 );
  FsFindFiles(files, pagelist, ML( cwd ), 1 );
  //printf( "findfiles time: %f\n", TimeSecFromClocks64( t0, t1 ) );

  ForLen( i, files ) {
    auto fileobj = files.mem[i];
    auto ext = FileExtension( ML( fileobj ) );
    if( !ext.len ) continue;
  
    bool process_file = 0;
    for( idx_t j = 0;  j < args_len;  ++j ) {
      u8* arg = args[j];
      idx_t arg_len = CstrLength( arg ); // TODO: don't recompute this every time.
      if( MemEqual( ML( ext ), arg, arg_len ) ) {
        process_file = 1;
        break;
      }
    }

    if( process_file ) {
      auto file = FileOpen( ML( fileobj ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
      if( !file.loaded ) continue;
      
      auto contents = FileAlloc( file );
      FileFree( file );
      
      struct range_t
      {
        idx_t l;
        idx_t r;
      };
      stack_resizeable_cont_t<range_t> deletes;
      Alloc( deletes, contents.len / 50u );
      
      idx_t pos = 0;
      while( pos != contents.len ) {
        auto eol = CursorStopAtNewlineR( ML( contents ), pos );
        auto new_eol = CursorSkipSpacetabL( ML( contents ), eol );
        if( eol != new_eol ) {
          range_t r = { new_eol, eol };
          *AddBack( deletes ) = r;
        }
        pos = CursorCharR( ML( contents ), eol );
      }
      
      // 0 to first range's l
      // for each range pair a,b: a.r to b.l
      // the last range's r to eof.
      if( deletes.len ) {
        file = FileOpen( ML( fileobj ), fileopen_t::only_existing, fileop_t::W, fileop_t::none );
        AssertCrash( file.loaded );
        u64 offset = 0;
        auto range0 = deletes.mem[0];
        FileWrite( file, offset, contents.mem + 0, range0.l );
        offset += range0.l;
        
        For( i, 1, deletes.len ) {
          auto rangeA = deletes.mem[ i - 1 ];
          auto rangeB = deletes.mem[ i ];
          FileWrite( file, offset, contents.mem + rangeA.r, rangeB.l - rangeA.r );
          offset += rangeB.l - rangeA.r;
        }
        
        auto rangeLast = deletes.mem[ deletes.len - 1 ];
        FileWrite( file, offset, contents.mem + rangeLast.r, contents.len - rangeLast.r );
        offset += contents.len - rangeLast.r;
        
        FileSetEOF( file, offset );
        FileFree( file );
      }
      
      Free( deletes );
      Free( contents );
    }
  }

  Free( files );

  return 0;
}




int
main( int argc, char** argv )
{
  MainInit();

  int r = Main( Cast( u8**, &argv[1] ), argc - 1 );

  //printf( "Main returned: %d\n", r );
  //system( "pause" );

  MainKill();
  return r;
}


