// build:console_x64_debug
// build:console_x64_optimized
// Copyright (c) John A. Carlos Jr., all rights reserved.

void
LogUI( const void* cstr ... )
{
}

#define TEST 1

using FnTest = void(*)();
constexpr size_t g_ctests = 100;
FnTest g_tests[g_ctests];
size_t g_ntests = 0;

struct RegisterTestObject
{
  RegisterTestObject( FnTest fn )
  {
    if( g_ntests >= g_ctests ) __debugbreak();
    g_tests[g_ntests++] = fn;
  }
};
#define ___JOIN( a, b ) a ## b
#define __JOIN( a, b ) ___JOIN( a, b )

#if defined(TEST)
  #define RegisterTest \
    static RegisterTestObject __JOIN( g_register_test_obj, __COUNTER__ ) = RegisterTestObject
#else
  #define RegisterTest
#endif


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
#include "rand.h"
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
#include "ds_stack_resizeable_pagestack.h"
#include "ds_hashset_cstyle.h"
#include "ds_hashset_complexkey.h"
#include "ds_hashset_nonzeroptrs.h"
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
#include "sparse2d_compressedsparserow.h"
#include "statistics.h"

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

  For( i, 0, g_ntests ) {
    g_tests[i]();
  }

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
  TestStatistics();
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


// Fast DFT:
//   F[n] = sum( k=0..N-1, f[k] * exp( i * -2pi * n * k / N ) )
// Define
//   A = -2pi i
// Then,
//   F[n] = sum( k=0..N-1, f[k] * exp( n k * A / N ) )
// Break into even/odd parts
//        = sum( k even in 0..N-1, f[k] * exp( n k A / N ) )
//        + sum( k odd  in 0..N-1, f[k] * exp( n k A / N ) )
// Redefine k
//        = sum( k=0..N/2-1, f[2k] * exp( 2 n k A / N ) )
//        + sum( k=0..N/2-1, f[2k+1] * exp( n (2k+1) A / N ) )
// Algebra, move 2/N -> 1/(N/2)
//        = sum( k=0..N/2-1, f[2k] * exp( n k A / ( N/2 ) ) )
//        + sum( k=0..N/2-1, f[2k+1] * exp( ( 2 n k + n ) A / N ) )
// Algebra
//        = sum( k=0..N/2-1, f[2k] * exp( n k A / ( N/2 ) ) )
//        + sum( k=0..N/2-1, f[2k+1] * exp( 2 n k A / N + n A / N ) )
// Algebra
//        = sum( k=0..N/2-1, f[2k] * exp( n k A / ( N/2 ) ) )
//        + sum( k=0..N/2-1, f[2k+1] * exp( n k A / ( N/2 ) + n A / N ) )
// Pull constant out of the second sum.
//        = sum( k=0..N/2-1, f[2k] * exp( n k A / ( N/2 ) ) )
//        + sum( k=0..N/2-1, f[2k+1] * exp( n k A / ( N/2 ) ) ) * exp( n A / N )
// Define:
//   F_even[n] = sum( k=0..N/2-1, f[2k] * exp( n k A / ( N/2 ) ) )
//   F_odd[n]  = sum( k=0..N/2-1, f[2k+1] * exp( n k A / ( N/2 ) ) )
// Note these are identical to F[n], just taking the odd/even input subsets, and N' = N/2.
//   F[n,N] = F[n] defined above.
//   F_even[n,N] = F[n,N/2] on the even subset
//   F_odd[n,N]  = F[n,N/2] on the odd  subset
// Then,
//   F[n] = F_even[n] + F_odd[n] exp( n A / N )
//   F[n,N] = F[n,N/2](even) + F[n,N/2](odd) exp( n A / N )
// That's our recurrence relation.
//
// Note that F[n,1] = f[n], when N=1. That's our base case.
// Take N=2:
//   F[n,2] = F[n,1](even) + F[n,1](odd) exp( n A / 2 )
//          = f[0] + f[1] exp( n A / 2 )
// Computing this for n=0..1, we're done:
//   F[0,2] = f[0] + f[1]
//   F[1,2] = f[0] - f[1]
// What about N=4?
//   F[n,4] = F[n,2](even) + F[n,2](odd) exp( n A / 4 )
//          = ( f[0] + f[2] exp( n A / 2 ) )
//          + ( f[1] + f[3] exp( n A / 2 ) ) exp( n A / 4 )
// Again, computing this for n=0...3:
//   F[0,4] = F[0,2](even) + F[0,2](odd)
//   F[1,4] = F[1,2](even) - F[1,2](odd) i
//   F[2,4] = F[2,2](even) - F[2,2](odd)
//   F[3,4] = F[3,2](even) + F[3,2](odd) i
//
//   F[0,4] = F[0,2]{0,2} + F[0,2]{1,3}
//   F[1,4] = F[1,2]{0,2} - F[1,2]{1,3} i
//   F[2,4] = F[2,2]{0,2} - F[2,2]{1,3}
//   F[3,4] = F[3,2]{0,2} + F[3,2]{1,3} i
//
//   F[0,4] = (f[0] + f[2]) + (f[1] + f[3])
//   F[1,4] = (f[0] - f[2]) - (f[1] - f[3]) i
//   F[2,4] = (F[2,2]){0,2} - (F[2,2]){1,3}
//   F[3,4] = (F[3,2]){0,2} + (F[3,2]){1,3} i
//
// And N=8?
//   F[n,8] = F[n,4](even) + F[n,4](odd) exp( n A / 8 )
//          = ( f[0] + f[4] exp( n A / 2 ) )
//          + ( f[2] + f[6] exp( n A / 2 ) ) exp( n A / 4 )
//          + ( ( f[1] + f[5] exp( n A / 2 ) )
//          +   ( f[3] + f[7] exp( n A / 2 ) ) exp( n A / 4 )
//            ) exp( n A / 8 )
// So how do we decompose this into divide and conquer?
// It looks like we're still requiring each n to involve all f[n] values.
// For F[n,8], if we order the array as:
//   { 0, 2, 4, 6, 1, 3, 5, 7 }
// Then we can divide; but what about the n exponent?
//
// Given:
//   F_even[n] = sum( k=0..N/2-1, f[2k] * exp( n k A / ( N/2 ) ) )
//   F_odd[n]  = sum( k=0..N/2-1, f[2k+1] * exp( n k A / ( N/2 ) ) )
// Note we can share the exp terms.
//   complex<T> E[N/2];
//   For( n, 0, N/2 ) {
//     E[n] = exp( n A / (N/2) );
//   }
//   F_even[n] = sum( k=0..N/2-1, f[2k] * E[n]^k )
//   F_odd[n]  = sum( k=0..N/2-1, f[2k+1] * E[n]^k )




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
