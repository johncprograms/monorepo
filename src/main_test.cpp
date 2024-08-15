// build:console_x64_optimized
// build:console_x64_debug
// Copyright (c) John A. Carlos Jr., all rights reserved.

void
LogUI( const void* cstr ... )
{
}

#define TEST 1

// NOTE: we may have to move this RegisterTest defn and such to a separate header, for other binaries to opt out with TEST=0.
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
#include "ds_minheap_extractable.h"
#include "ds_minheap_decreaseable.h"
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

#include "ds_hashset_nonzeroptrs.h"
#include "ds_hashset_complexkey.h"
#include "ds_hashset_chain_complexkey.h"
#include "ds_minheap_generic.h"
#include "ds_minmaxheap_generic.h"
#include "ds_graph.h"
#include "compress_runlength.h"
#include "compress_huffman.h"
#include "compress_arithmetic.h"
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



#if 0

implement an algorithm to determine if a string has all unique characters. what if you can't use add'l data structures?

using namespace std;
bool isUnique(const string_view& s)
{
  const size_t len = s.length();
  if (!len) return true;
  string sorted = s;
  sort(begin(sorted), end(sorted));
  for (size_t i = 1; i < len; ++i) {
    auto prev = sorted[i-1];
    auto cur = sorted[i];
    if (prev == cur) return false;
  }
  return true;
}
bool isUnique(const string_view& s)
{
  const size_t len = s.length();
  if (!len) return true;
  vector<bool> present{256, false}; // initializes to length=256, all values initially false.
  for (const auto& c : s) {
    if (present[c]) return false;
    present[c] = true;
  }
  return true;
}


given two strings, write a method to decide if one is a permutation of the other.

bool isPermutation(const string_view& a, const string_view& b)
{
  string as = a;
  string bs = b;
  sort(begin(as), end(as));
  sort(begin(bs), end(bs));
  return as == bs;
}

vector<size_t> countPerChar(const string_view& a)
{
  vector<size_t> countPerChar{256, 0}; // length=256, all values initially 0
  for (const auto& c : a) {
    countPerChar[c] += 1;
  }
  return countPerChar;
}
bool isPermutation(const string_view& a, const string_view& b)
{
  vector<size_t> countsA = countPerChar(a);
  vector<size_t> countsB = countPerChar(b);
  return countsA == countsB;
}


write a method to replace all spaces in a string with %20. assume the string has sufficient space at the end to hold the new chars, and we're given the true length accounting for the change.
ex: given 'a b c' return 'a%20b%20c'

string replaceSpaces(const string_view& s, size_t actualOutputLen)
{
  string r;
  r.reserve(actualOutputLen);
  for (const auto& c : s) {
    if (c == ' ') {
      r += "%20";
    }
    else {
      r += c;
    }
  }
  return r;
}
void replaceSpacesInPlace(char* s, size_t s_len, size_t output_len)
{
  if (!s || !output_len || !s_len) return;
  char* write = s + output_len - 1;
  // security: consider underflowing read pointer.
  for (
    char* read = s + s_len - 1;
    read >= s;
    --read)
  {
    auto c = *read;
    if (c == ' ') {
      *write-- = '0';
      *write-- = '2';
      *write-- = '%';
    }
    else {
      *write = c;
    }
  }
}


given a string, check if it is a permutation of a palindrome (defn: same forw/back).
ex: 'tactcoa' returns true, because a permutation exists: 'tacocat'
essentially you're allowed to have one pmf value be odd; all the others have to be even.

vector<size_t> charPmf(const string_view& s)
{
  vector<size_t> pmf{256, 0}; // length=256 of initial 0s
  for (const auto& c : s) pmf[c] += 1;
  return pmf;
}
bool isPermutationOfPalindrome(const string_view& s)
{
  vector<size_t> pmf = charPmf(s);
  size_t nOdd = 0;
  for (const auto& pmfValue : pmf) {
    if (pmfValue % 2 == 1) {
      nOdd += 1;
      if (nOdd > 1) return false;
    }
  }
}


given three edit types:
  insert char
  remove char
  replace char
  given two strings, check if they are 0 or 1 edits away from each other.

bool editDistance1core(const string_view& a, const string_view& b)
{
  const size_t a_len = a.length();
  const size_t b_len = b.length();
  assert(a_len + 1 == b_len);
  // strings have to be equivalent except for one missing character in the shorter string.
  // a is shorter, so we're allowed to move pa forward one time before calling it quits and not a match.
  const char* pa = start(a);
  const char* pb = start(b);
  const char* pb_end = end(b);
  size_t nAttempts = 0;
  while (pb != pb_end) {
    if (*pb == *pa) {
      ++pa;
      ++pb;
    }
    else {
      // attempt to move pa forward once.
      if (nAttempts > 0) return false;
      nAttempts += 1;
      ++pa;
      // don't move pb, we'll retry the loop.
    }
  }
  return true;
}
bool editDistance0or1(const string_view& a, const string_view& b)
{
  const size_t a_len = a.length();
  const size_t b_len = b.length();
  const size_t maxL = max(a_len, b_len);
  const size_t minL = min(a_len, b_len);
  if (maxL - minL >= 2) return false;
  if (maxL - minL == 1) {
    if (a_len < b_len) return editDistance1core(a, b);
    else return editDistance1core(b, a);
  }
  else { // equivalent length
    assert(a_len == b_len);
    // you're only allowed one character difference.
    size_t nDiffs = 0;
    for (size_t i = 0; i < a_len; ++i) {
      if (a[i] != b[i]) {
        nDiffs += 1;
        if (nDiffs > 1) return false;
      }
    }
    return true;
  }
}


implement basic string compression using repcount.
ex: 'aabcccccaaa' -> 'a2b1c5a3'
if the compressed data is larger, return the original instead.
assume only alpha chars in the original.

string compress(string_view input)
{
  string output;
  const char* ci = begin(input);
  const char* input_end = end(input);
  while (ci != input_end) {
    auto c = *ci;
    assert(isalpha(c));
    output += c;
    ++ci;
    size_t count = 1;
    // Once count hits 9, we have to emit 'c9c9c9...' since this doesn't do multi-char integer encoding.
    while (ci < intput_end && count < 9 && *ci == c) {
      ++count;
      ++ci;
    }
    output += to_string(count);
  }
  if (output.length() >= input.length()) return input;
  return output;
}
string decompress(string_view input)
{
  string output;
  output.reserve(input.length());
  const char* ci = begin(input);
  const char* input_end = end(input);
  while (ci != input_end) {
    auto c = *ci;
    output += c;
    ++ci;
    if (ci < input_end) {
      auto charCount = *ci;
      ++ci;
      assert(isnum(charCount));
      size_t count = charCount - '0';
      for (size_t i = 0; i < count; ++i) output += c;
    }
  }
  return output;
}


given an image represented by an n by n matrix, with each pixel a uint32_t, rotate the image by 90 degrees clockwise. in place?
ex:
0 1
2 3
goes to:
2 0
3 1

0 1 2
3 4 5
6 7 8
goes to:
6 3 0
7 4 1
8 5 2

2d rotation matrix is:
cos(t) -sin(t)
sin(t) cos(t)
for t=90deg,
0 -1
1 0
so { x, y } rotates to { -y, x }.

void rotate90degclockwise(uint32_t* input, uint32_t* output, size_t n)
{
  assert(input != output);
  for (size_t y = 0; y < n; ++y) {
    for (size_t x = 0; x < n; ++x) {
      // first row of input turns into last column of output.
      auto ox = n-1-y;
      auto oy = x;
      output[n*oy+ox] = input[n*y+x];
    }
  }
}

to do in place, you have to do cycle following and O(1) extra space for the temporary copy.
for n=1, no cycles.
for n=2, one cycle: the corners.
for n=3, two cycles: the corners, the plus. note the center stays still.
for n=4, same as n=2 (inner n=2 rect) plus three cycles: corners, left skew plus, right skew plus.
for n=5, same as n=3 (inner n=3 rect) plus four cycles: corners, and the three skew pluses.
for n=r, same as n=r-2 (inner rect) plus r-1 cycles.

TODO: implement.

void rotate90degclockwiseinplace(
  uint32_t* m,
  size_t row_stride,
  size_t x_offset,
  size_t y_offset,
  size_t n)
{
  if (n <= 1) return;
  if (n == 2) {
    // corner rotation
    uint32_t temp;
    auto sx = x_offset;
    auto sy = y_offset;
    auto dx = x_offset + 1;
    auto dy =
  }
  else {
  }
}


given an n by m matrix, if an element is 0, set that element's row and col to 0.

void zeroZeroElementsRowAndCols(uint32_t* matrix, size_t x_dim, size_t y_dim)
{
  vector<bool> rows{y_dim, false};
  vector<bool> cols{x_dim, false};
  for (size_t y = 0; y < y_dim; ++y) {
    for (size_t x = 0; x < x_dim; ++x) {
      auto elem = matrix[x_dim*y+x];
      if (elem == 0) {
        rows[y] = true;
        cols[x] = true;
      }
    }
  }
  // PERF: double-zeroing on the overlaps. Worst case is 2*x_dim*y_dim zeroing writes.
  for (size_t x = 0; x < x_dim; ++x) {
    if (!cols[x]) continue;
    for (size_t y = 0; y < y_dim; ++y) matrix[x_dim*y+x] = 0;
  }
  for (size_t y = 0; y < y_dim; ++y) {
    if (!rows[y]) continue;
    for (size_t x = 0; x < x_dim; ++x) matrix[x_dim*y+x] = 0;
  }
}
// D'oh! We can avoid the bitvecs by reusing the first row and col of the matrix as that signal.
void zeroZeroElementsRowAndCols(uint32_t* matrix, size_t x_dim, size_t y_dim)
{
  for (size_t y = 1; y < y_dim; ++y) {
    for (size_t x = 1; x < x_dim; ++x) {
      auto elem = matrix[x_dim*y+x];
      if (elem == 0) {
        matrix[x_dim*0+x] = 0;
        matrix[x_dim*y+0] = 0;
      }
    }
  }
  // PERF: double-zeroing on the overlaps. Worst case is 2*x_dim*y_dim zeroing writes.
  // Iterate first row, zeroing cols that we marked.
  bool zeroFirstRow = false;
  for (size_t x = 0; x < x_dim; ++x) {
    if (matrix[x_dim*0+x]) continue;
    for (size_t y = 1; y < y_dim; ++y) matrix[x_dim*y+x] = 0;
    zeroFirstRow = true;
  }
  // Iterate first col, zeroing rows that we marked.
  bool zeroFirstCol = false;
  for (size_t y = 0; y < y_dim; ++y) {
    if (matrix[x_dim*y+0]) continue;
    for (size_t x = 1; x < x_dim; ++x) matrix[x_dim*y+x] = 0;
    zeroFirstCol = true;
  }
  if (zeroFirstRow) {
    for (size_t x = 0; x < x_dim; ++x) matrix[x_dim*0+x] = 0;
  }
  if (zeroFirstCol) {
    for (size_t y = 1; y < y_dim; ++y) matrix[x_dim*y+0] = 0;
  }
}


given:
bool isSubstring(string_view super, string_view sub);
given two strings a,b, check if b is a rotation of a.
you're only allowed to call isSubstring once.
ex: 'waterbottle' is a rotation of 'erbottlewat'
ex: 'ttbttf' is not a rotation of 'ttattb'

bool isRotation(string_view a, string_view b)
{
  const auto a_len = a.length();
  const auto b_len = b.length();
  if (a_len != b_len) return false;
  if (a_len <= 1) return true;
  for (size_t i = 0; i < a_len; ++i) {
    // i is the split position.
    // given
    // a='waterbottle',
    // b='erbottlewat',
    //            ^
    // the split position that should match is: i=8.
    auto rotationEqual = [&]() {
      for (size_t j = 0; j < a_len; ++j) {
        auto k = (j + i) % a_len;
        if (a[j] != b[k]) return false;
      }
      return true;
    };
    if (rotationEqual()) return true;
  }
  return false;
}
// D'oh!
bool isRotation(string_view a, string_view b)
{
  // if b is a rotation, it will be contained within aa.
  string aa = a;
  aa += b;
  return isSubstring(aa, b);
}

#endif
