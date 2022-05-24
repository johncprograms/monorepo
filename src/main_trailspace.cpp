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

#include "ui_buf.h"


static void
RemTrailingSpaces( buf_t& file )
{
  idx_t pos = 0;
  Forever {
    idx_t eol = CursorStopAtNewlineR( file, pos );

    idx_t new_eol = CursorSkipSpacetabL( file, eol );

    if( new_eol < eol ) {
      Delete( file, new_eol, eol );
    }

    idx_t new_pos = CursorLineD( file, pos, 0 );
    if( new_pos == pos ) {
      break;
    }
    pos = new_pos;
  }
}


int
Main( u8** args, idx_t args_len )
{
  if( args_len == 0 ) {
    printf( "Removes trailing whitespace from ANSI files.\n" );
    printf( "Checks all files in the CWD and recurses down.\n" );
    printf( "Could very well mangle Unicode files, so review the diffs afterwards.\n" );
    printf( "Specify filtering file extensions as arguments.\n" );
    printf( "Ex: \"trailspace h c cpp\"\n" );

  } else {
    err_t err = 0;
    u64 t0;
    u64 t1;

    array_t<fsobj_t> files;
    Alloc( files, 1024 );

    u8 cwd [c_fspath_len];
    idx_t cwd_len = c_fspath_len;
    FsGetCwd( cwd, cwd_len, &cwd_len );

//    u8* cwd = Str( "C:\\doc\\dev\\cpp\\master\\te\\src" );
//    idx_t cwd_len = CsLen( cwd );
//    u8* newargs[] = { Str( "h" ), Str( "cpp" ) };
//    vargs = newargs;
//    args_len = 2;

    t0 = TimeClock();
    FsFindFiles( files, cwd, cwd_len, 0 );
    t1 = TimeClock();
    //printf( "findfiles time: %f\n", TimeSecFromClocks64( t0, t1 ) );

    for( idx_t i = 0;  i < files.len;  ++i ) {
      auto& fileobj = files.mem[i];

      u8* ext = CsScanL( fileobj.name, fileobj.len, '.' );
      if( ext ) {
        idx_t ext_len = Cast( idx_t, fileobj.name + fileobj.len - ext );
        AssertCrash( ext_len );
        ext += 1;
        ext_len -= 1;

        bool process_file = 0;
        for( idx_t j = 0;  j < args_len;  ++j ) {
          u8* arg = args[j];
          idx_t arg_len = CsLen( arg );

          if( MemEqual( ext, ext_len, arg, arg_len ) ) {
            process_file = 1;
            break;
          }
        }

        if( process_file ) {
          u8* filename = fileobj.name;
          idx_t filename_len = fileobj.len;

          t0 = TimeClock();

          buf_t file;
          Init( file );
          LoadMapped( file, filename, filename_len );

          t0 = TimeClock();
          idx_t len_before = file.content_len;
          RemTrailingSpaces( file );
          idx_t len_after = file.content_len;
          t1 = TimeClock();
          //printf( "trailspace time: %f\n", TimeSecFromClocks64( t0, t1 ) );

          if( len_after < len_before ) {
            t0 = TimeClock();
            err = SaveAndUnload( file, filename, filename_len );
            if( !err ) {
              t1 = TimeClock();
              //printf( "filestore time: %f\n", TimeSecFromClocks64( t0, t1 ) );
            } else {
              Unload( file );
              UnreachableCrash();
            }
          } else {
            Unload( file );
          }
        }
      }
    }

    Free( files );
  }

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


