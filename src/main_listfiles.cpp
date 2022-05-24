// build:console_x64_debug
// Copyright (c) John A. Carlos Jr., all rights reserved.

#include "common.h"
#include "math_vec.h"
#include "math_matrix.h"
#include "ds_slice.h"
#include "ds_string.h"
#include "ds_plist.h"
#include "ds_array.h"
#include "ds_embeddedarray.h"
#include "ds_fixedarray.h"
#include "ds_pagearray.h"
#include "ds_list.h"
#include "ds_bytearray.h"
#include "ds_hashset.h"
#include "cstr.h"
#include "filesys.h"
#include "timedate.h"
#include "threading.h"
#define LOGGER_ENABLED   0
#include "logger.h"
#define PROF_ENABLED   0
#define PROF_ENABLED_AT_LAUNCH   0
#include "profile.h"
#include "rand.h"
#include "main.h"


int
Main( u8** args, idx_t args_len )
{
  if( args_len != 2 ) {
    printf( "List all files in a directory, recursively.\n" );
    printf( "Outputs full paths to the specified file.\n" );
    printf( "Ex: \"listfiles c: ./files.txt\"\n" );
  
  } else {
    u8* dir = args[0];
    idx_t dir_len = CsLen( dir );
    u8* filename = args[1];
    idx_t filename_len = CsLen( filename );

    array_t<fsobj_t> files;
    Alloc( files, 8192 );
    Prof( find_files );
    FsFindFiles( files, dir, dir_len, 1 );
    ProfClose( find_files );

    printf( "num files: %I64u\n", Cast( u64, files.len ) );

    array_t<u8> outfile;
    Alloc( outfile, ( files.len + 1 ) * c_fspath_len );
    ForLen( i, files ) {
      auto& file = files.mem[i];
      Memmove( AddBack( outfile, file.len ), file.name, file.len );
      Memmove( AddBack( outfile, 2 ), Str( "\r\n" ), 2 );
    }

    // Don't care if this fails.
    FileRecycle( filename, filename_len );

    file_t file = FileOpenNew( filename, filename_len, fileop_t::RW, fileop_t::R );
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


