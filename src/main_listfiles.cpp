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


int
Main( u8** args, idx_t args_len )
{
  if( args_len != 2 ) {
    printf( "List all files in a directory, recursively.\n" );
    printf( "Outputs full paths to the specified file.\n" );
    printf( "Ex: \"listfiles c: ./files.txt\"\n" );
  }
  else {
    u8* dir = args[0];
    idx_t dir_len = CstrLength( dir );
    u8* filename = args[1];
    idx_t filename_len = CstrLength( filename );

    stack_resizeable_cont_t<slice_t> files;
    Alloc( files, 8192 );
    pagelist_t pagelist;
    Init( pagelist, 64000 );
    Prof( find_files );
    FsFindFiles( files, pagelist, dir, dir_len, 1 );
    ProfClose( find_files );

    printf( "num files: %llu\n", Cast( u64, files.len ) );

    stack_resizeable_cont_t<u8> outfile;
    Alloc( outfile, ( files.len + 1 ) * c_fspath_len );
    ForLen( i, files ) {
      auto& file = files.mem[i];
      Memmove( AddBack( outfile, file.len ), ML( file ) );
      Memmove( AddBack( outfile, 2 ), Str( "\r\n" ), 2 );
    }

    // Don't care if this fails.
    FileRecycle( filename, filename_len );

    file_t file = FileOpen( filename, filename_len, fileopen_t::only_new, fileop_t::RW, fileop_t::R );
    FileWrite( file, 0, ML( outfile ) );
    FileFree( file );
    Free( files );
    Free( outfile );
  }

  return 0;
}




int
main( int argc, char** argv )
{
  MainInit();

#if 1
  int r = Main( Cast( u8**, &argv[1] ), argc - 1 );
#else
  u8* vargs[] = {
    Str( "c:/doc/dev/cpp/proj/main/exe/64d" ),
    Str( "./out.txt" ),
    };
  int r = Main( vargs, _countof( vargs ) );
#endif

  //printf( "Main returned: %d\n", r );
  //system( "pause" );

  MainKill();
  return r;
}


