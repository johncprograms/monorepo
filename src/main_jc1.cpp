// build:console_x64_debug
// Copyright (c) John A. Carlos Jr., all rights reserved.

#define FINDLEAKS   0
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
#include "threading.h"
#define LOGGER_ENABLED   0
#include "logger.h"
#define PROF_ENABLED   0
#define PROF_ENABLED_AT_LAUNCH   0
#include "profile.h"
#include "rand.h"
#include "allocator_heap_findleaks.h"
#include "mainthread.h"
#include "cstr_integer.h"
#include "cstr_float.h"
#include "ds_hashset_cstyle_indexed.h"

// don't force inline all the recursive calls.
#undef Inl
#define Inl inline

#include "jc1_tokenize.h"
#include "jc1_ast.h"
#include "jc1_parse.h"
#include "jc1_type.h"
#include "jc1_resolve.h"
#include "jc1_codegen.h"


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
  idx_t filename_len = CstrLength( filename );

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
  auto filemem = FileAlloc( file );
  AssertCrash( filemem.mem );
  FileFree( file );

  src.file = filemem;

  t1 = TimeClock();
  printf( "file load time: %f ms\n", 1000 * TimeSecFromClocks64( t1 - t0 ) );



  pagelist_t mem;
  Init( mem, 65000 );


  tlog_t log;
  Init( log );

  stack_resizeable_cont_t<token_t> tokens;
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

  stack_resizeable_cont_t<code_t> code;
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
  idx_t log_filename_len = CstrLength( log_filename );

  if( !log.nerrs ) {
    stack_resizeable_cont_t<u8> ast_print;
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

  stack_resizeable_cont_t<u8> cmdline;
  Alloc( cmdline, 512 );
  Fori( int, i, 1, argc ) {
    u8* arg = Cast( u8*, argv[i] );
    idx_t arg_len = CstrLength( arg );
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

