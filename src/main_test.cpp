// build:console_x64_debug
// build:console_x64_optimized
// Copyright (c) John A. Carlos Jr., all rights reserved.

void
LogUI( const void* cstr ... )
{
}

#define TEST 1

#include "os_mac.h"
#include "os_windows.h"

#define FINDLEAKS 1 // make it a test failure to leak memory.
#define DEBUGSLOW 0 // enable some slower debug checks.
#define WEAKINLINING 1
#include "core_cruntime.h"
#include "core_types.h"
#include "core_language_macros.h"
#include "asserts.h"
#include "memory_operations.h"
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
#include "allocator_ringbuffer.h"
#include "ds_chartree.h"
#include "ds_stack_cstyle.h"
#include "ds_stack_nonresizeable_stack.h"
#include "ds_stack_implicitcapacity.h"
#include "ds_stack_nonresizeable.h"
#include "ds_stack_resizeable_cont.h"
#include "ds_stack_resizeable_pagelist.h"
#include "ds_bitarray_nonresizeable_stack.h"
#include "ds_zipset.h"
#include "ds_queue_resizeable_pagelist.h"
#include "ds_queue_resizeable_cont.h"
#include "ds_queue_nonresizeable.h"
#include "ds_queue_nonresizeable_stack.h"
#include "ds_pagetree.h"
#include "ds_list.h"
#include "ds_deque_nonresizeable_stack.h"
#include "ds_deque_nonresizeable.h"
#include "ds_deque_resizeable_cont.h"
#include "ds_deque_resizeable_pagelist.h"
#include "ds_hashset_cstyle.h"
#include "ds_hashset_complexkey.h"
#include "ds_hashset_nonzeroptrs.h"
#include "rand.h"
#include "ds_minheap_extractable.h"
#include "ds_minheap_decreaseable.h"
#include "ds_minheap_generic.h"
#include "compress_runlength.h"
#include "compress_huffman.h"
#include "compress_arithmetic.h"
#include "allocator_fixedsize.h"
#include "filesys.h"
#include "cstr_integer.h"
#include "cstr_float.h"
#include "timedate.h"
#include "thread_atomics.h"
#include "ds_mtqueue_mrmw_nonresizeable.h"
#include "ds_mtqueue_mrsw_nonresizeable.h"
#include "ds_mtqueue_srmw_nonresizeable.h"
#include "ds_mtqueue_srsw_nonresizeable.h"
#include "threading.h"
#include "ds_btree.h"
#include "ds_stack_resizeable_cont_addbacks.h"
#include "asserts_ship.h"
#include "optimize_string_sequence_alignment.h"

#define LOGGER_ENABLED   1
#include "logger.h"

#define PROF_ENABLED   1
#define PROF_ENABLED_AT_LAUNCH   1
#include "profile.h"

#include "ds_graph.h"
#include "text_parsing.h"
#include "allocator_heap_findleaks.h"
#include "mainthread.h"
#include "optimize_simplex.h"
#include "ds_hashset_cstyle_indexed.h"

#define OPENGL_INSTEAD_OF_SOFTWARE       0
#define GLW_RAWINPUT                     1
#define GLW_RAWINPUT_KEYBOARD            0
#define GLW_RAWINPUT_KEYBOARD_NOHOTKEY   1
#include "glw.h"

#include "ui_propdb.h"
#include "ui_font.h"
#include "ui_render.h"
#include "ui_buf2.h"
#include "ui_txt2.h"
#include "ui_cmd.h"
#include "ui_listview.h"
//#include "ui_findinfiles.h"
//#include "ui_fileopener.h"
//#include "ui_switchopened.h"
//#include "ui_edit2.h"

#if defined(WIN)
  #include <ehdata.h>
  #include "prototest_exeformat.h"
#endif

#include "prototest_renormalization.h"
#include "prototest_sparse2d_compressedsparserow.h"

int
Main( u8* cmdline, idx_t cmdline_len )
{
  GlwInit();

  // we test copy/paste here, so we need to set up the g_client.hwnd
  glwclient_t client = {};
  #if defined(WIN)
    client.hwnd = GetActiveWindow();
  #endif
  g_client = &client;

  TestSimplex();
  TestMathInteger();
  TestString();
  TestStringSequenceAlignment();
  TestArray();
  TestArrayAddBackCustom();
  TestBinSearch();
  TestPagelist();
  TestCstr();
  TestCstrFloat();
  TestCstrInteger();
  TestCoreText();
  TestBuf();
  #if !defined(MAC)
  // TODO: resolve issues around .config file.
  TestTxt();
  // TODO: reimplement filesys for mac
  TestFilesys();
  // TODO: reimplement Execute for mac
  TestExecute();
  #endif
  TestHashset();
  TestHashsetNonzeroptrs();
  TestHashsetComplexkey();
  TestIdxHashset();
  TestChartree();
  TestRng();
  TestMathFloat();
  TestGraph();
  TestMinHeap();
  TestMinHeapGeneric();
  TestRLE();
  TestHuffman();
  TestCompressArith();
  TestBtree();
  TestFsalloc();

  CalcJunk();

#if defined(WIN)
  ExeJunk();
#endif


  // Do this before destroying any datastructures, so other threads stop trying to access things.
  SignalQuitAndWaitForTaskThreads();

  GlwKill();

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
    Memmove( AddBack( cmdline, 2 ), Str( " " ), 2 );
  }
  int r = Main( ML( cmdline ) );
  Free( cmdline );

  MainKill();

  system( "pause" );
  return r;
}
