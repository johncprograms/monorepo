// build:console_x64_debug
// Copyright (c) John A. Carlos Jr., all rights reserved.

#define FINDLEAKS   0
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

#include "ds_idx_hashset.h"

// don't force inline all the recursive calls.
#undef Inl
#define Inl inline

#include "c_tokenize.h"
#include "c_ast.h"
#include "c_parse.h"
#include "c_type.h"
#include "c_resolve.h"
#include "c_codegen.h"


#if 0 // TODOs



#endif


int
Main( u8* cmdline, idx_t cmdline_len )
{
  u64 t0;
  u64 t1;

  t0 = TimeClock();

  // TODO: cmdline options parsing.
  u8* filename = Str( "c:/doc/dev/cpp/proj/main/test.jc" );
  idx_t filename_len = CsLen( filename );

  src_t src;
  src.filename = { filename, filename_len };

  file_t file = FileOpen( ML( src.filename ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
  if( !file.loaded ) {
    auto tmp = AllocCstr( src.filename );
    printf( "can't open sourcefile: %s\n", tmp );
    MemHeapFree( tmp );
    return 1;
  }
  AssertWarn( file.loaded );
  string_t filemem = FileAlloc( file );
  AssertCrash( filemem.mem );
  FileFree( file );

  src.file = filemem;

  t1 = TimeClock();
  printf( "file load time: %f ms\n", 1000 * TimeSecFromClocks64( t1 - t0 ) );



  plist_t mem;
  Init( mem, 65000 );


  tlog_t log;
  Init( log );

  array_t<token_t> tokens;
  Alloc( tokens, 32768 );

  t0 = TimeClock();
  Tokenize( tokens, &src, &log );
  t1 = TimeClock();
  printf( "tokenize time: %f ms\n", 1000 * TimeSecFromClocks64( t1 - t0 ) );

  if( log.nerrs ) {
    *AddBack( log.errs ) = 0;
    printf( "TOKENIZE ERRORS:\n%s", log.errs.mem );
    RemBack( log.errs );
  }
  if( log.nwarns ) {
    *AddBack( log.warns ) = 0;
    printf( "TOKENIZE WARNINGS:\n%s", log.warns.mem );
    log.warns.len = 0;
  }

  //PrintTokens( tokens, filemem );
  //printf( "\n================================\n\n" );

  if( log.nerrs )
    return 1;

  ast_t ast = {};

  pcontext_t pcontext;
  pcontext.tokens = &tokens;
  pcontext.src = &src;
  pcontext.log = &log;
  pcontext.mem = &mem;
  t0 = TimeClock();
  Parse( ast, &pcontext );
  t1 = TimeClock();
  printf( "parse time: %f ms\n", 1000 * TimeSecFromClocks64( t1 - t0 ) );

  if( log.nerrs ) {
    *AddBack( log.errs ) = 0;
    printf( "PARSE ERRORS:\n%s", log.errs.mem );
    RemBack( log.errs );
  }
  if( log.nwarns ) {
    *AddBack( log.warns ) = 0;
    printf( "PARSE WARNINGS:\n%s", log.warns.mem );
    log.warns.len = 0;
  }

  if( log.nerrs )
    return 1;

  tcontext_t tcontext;
  tcontext.ast = &ast;
  tcontext.src = &src;
  tcontext.log = &log;
  tcontext.mem = &mem;
  tcontext.ptr_bitcount = 64;
  t0 = TimeClock();
  Type( &tcontext );
  t1 = TimeClock();

  printf( "typecheck time: %f ms\n", 1000 * TimeSecFromClocks64( t1 - t0 ) );

  if( log.nerrs ) {
    *AddBack( log.errs ) = 0;
    printf( "TYPE ERRORS:\n%s", log.errs.mem );
    RemBack( log.errs );
  }
  if( log.nwarns ) {
    *AddBack( log.warns ) = 0;
    printf( "TYPE WARNINGS:\n%s", log.warns.mem );
    log.warns.len = 0;
  }

  if( log.nerrs )
    return 1;

  rcontext_t rcontext;
  rcontext.ast = &ast;
  rcontext.src = &src;
  rcontext.log = &log;
  rcontext.mem = &mem;

  t0 = TimeClock();
  Resolve( &rcontext );
  t1 = TimeClock();

  printf( "resolve time: %f ms\n", 1000 * TimeSecFromClocks64( t1 - t0 ) );

  if( log.nerrs ) {
    *AddBack( log.errs ) = 0;
    printf( "RESOLVE ERRORS:\n%s", log.errs.mem );
    RemBack( log.errs );
  }
  if( log.nwarns ) {
    *AddBack( log.warns ) = 0;
    printf( "RESOLVE WARNINGS:\n%s", log.warns.mem );
    log.warns.len = 0;
  }

  if( log.nerrs )
    return 1;

  array_t<code_t> code;
  Alloc( code, 32768 );

  ccontext_t ccontext;
  ccontext.ast = &ast;
  ccontext.src = &src;
  ccontext.log = &log;
  ccontext.mem = &mem;
  ccontext.ptr_bitcount = tcontext.ptr_bitcount;

  t0 = TimeClock();
  Gen( &ccontext, code );
  t1 = TimeClock();

  printf( "codegen time: %f ms\n", 1000 * TimeSecFromClocks64( t1 - t0 ) );

  if( log.nerrs ) {
    *AddBack( log.errs ) = 0;
    printf( "CODEGEN ERRORS:\n%s", log.errs.mem );
    RemBack( log.errs );
  }
  if( log.nwarns ) {
    *AddBack( log.warns ) = 0;
    printf( "CODEGEN WARNINGS:\n%s", log.warns.mem );
    log.warns.len = 0;
  }

  PrintGen( code );

  Execute( code );

  Free( code );

#if 0
  u8* log_filename = Str( "c:/doc/dev/cpp/proj/main/exe/64d/log.txt" );
  idx_t log_filename_len = CsLen( log_filename );

  if( !log.nerrs ) {
    array_t<u8> ast_print;
    Alloc( ast_print, 32768 );
    PrintAst( ast, src.file, ast_print );

    AssertWarn( FileDelete( log_filename, log_filename_len ) );
    file_t file = FileOpenNew( log_filename, log_filename_len, fileop_t::RW, fileop_t::R );
    AssertWarn( file.loaded );
    FileWrite( file, 0, ML( ast_print ) );
    FileFree( file );

    Free( ast_print );
  }
#endif

  Free( tokens );
  Kill( log );
  Free( src.file );

  Kill( mem );

  return 0;
}




int
main( int argc, char** argv )
{
  MainInit();

  array_t<u8> cmdline;
  Alloc( cmdline, 512 );
  Fori( int, i, 1, argc ) {
    u8* arg = Cast( u8*, argv[i] );
    idx_t arg_len = CsLen( arg );
    Memmove( AddBack( cmdline, arg_len ), arg, arg_len );
    Memmove( AddBack( cmdline, 2 ), " ", 2 );
  }
  int r = Main( ML( cmdline ) );
  Free( cmdline );

  //printf( "Main returned: %d\n", r );
  system( "pause" );

  MainKill();
  return r;
}

