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
#include "ds_binarysearchtree.h"
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


write code to remove duplicates from an unsorted linked list
follow up: how would you do this if a temporary buffer is not allowed?
struct node { node* next; int value; };
void removeDupes(node** phead)
{
  unordered_set<int> buffer;
  for (auto iter = *phead; iter; iter = iter->next) {
    buffer.insert(iter->value);
  }
  node* prev = 0;
  auto piter = phead;
  for (; *piter; ) {
    auto iter = *piter;
    auto value = iter->value;
    if (buffer.contains(value)) {
      // leave it in the list
      // remove it from the set, so that subsequent duplicates will hit the else-clause below.
      buffer.erase(value);
    }
    else {
      // remove from the list
      auto next = iter->next;
      if (!prev) {
        *phead = next;
      }
      else {
        prev->next = next;
      }
      delete node;
    }

    prev = iter;
    iter = iter->next;
  }
}

void insertSorted(node** head, node* n)
{
  if (!*head) {
    *head = n;
    n->next = 0;
    return;
  }
  if (n->value <= (*head)->value) {
    n->next = *head;
    *head = n;
    return;
  }
  insertSorted(&(*head)->next, n);
}
void removeDupes(node* head)
{
  node* sorted = 0;
  for (auto iter = head; iter; ) {
    auto next = iter->next;
    iter->next = 0;
    if (sorted) {
      insertSorted(&sorted, iter);
    }
    else {
      sorted = iter;
    }
    iter = next;
  }
}


return k-th to last
implement an algorithm to find the k-th to last element of a singly linked list.
struct node { node* next; int value; };
node* kthToLast(node* head, size_t k)
{
  size_t len = 0;
  for (auto iter = head; iter; iter = iter->next) {
    len += 1;
  }
  if (k > len) return 0;
  for (size_t i = 0; i <= len - k; ++i) {
    head = head->next;
  }
  return head;
}


delete middle node
implement an algorithm to delete a node in the middle (any node but the first and last, not the precise middle),
given only access to that node.
e.g. given a->b->c->d, remove(c), we should have: a->b->d
struct node { node* next; int value; };
void remove(node** pmiddle)
{
  if (!*pmiddle) return;
  if ((*pmiddle)->next) {
    (*pmiddle)->next = (*pmiddle)->next->next;
    *pmiddle = (*pmiddle)->next;
  }
  else {
    *pmiddle = 0;
  }
}


partition
partition a linked list around a value x, s.t. all nodes < x come before all nodes >= x.
the partition element can appear anywhere in the resulting right partition.
e.g. given 3,4,8,5,10,2,1; we should return something like: 3,1,2,10,5,5,8 where the partition happens at 10 in this result for instance.
struct node { node* next; int value; };
void partition(node** phead, int x)
{
  node* less = 0;
  node* grea = 0;
  node* last_less = 0;
  for (auto iter = *phead; iter; ) {
    auto next = iter->next;
    // remove the iter from the list, and then insert to the appropriate partition list.
    iter->next = 0;
    if (iter->value < x) {
      if (less) {
        iter->next = less;
      }
      else {
        last_less = iter;
      }
      less = iter;
    }
    else {
      if (grea) {
        iter->next = grea;
      }
      grea = iter;
    }

    iter = next;
  }

  // Join the less,grea lists.
  if (!less) {
    *phead = grea;
    return;
  }
  *phead = less;
  if (last_less) {
    last_less->next = grea;
  }
}


sum lists
two numbers are represented by a linked list.
each node contains a single digit
the digits are in reverse order; last digit is at the list head.
write a function to add two such list numbers and return the sum as a linked list.
e.g.
given (7,1,6) and (5,9,2), aka 617+295, return (2,1,9) aka 912.
struct node { node* next; int value; };
int evaluate(node* a)
{
  assert(a);
  size_t factor = 1;
  int r = 0;
  for (auto iter = a; iter; iter = iter->next) {
    r += factor * iter->value;
    factor *= 10;
  }
  return r;
}
node* sum(node* a, node* b)
{
  assert(a);
  assert(b);
  int av = evaluate(a);
  int bv = evaluate(b);
  int r = av + bv;
  node* result = 0;
  do {
    auto digit = r % 10;
    r /= 10;
    auto n = new node;
    n->next = 0;
    n->value = digit;
    if (result) {
      n->next = result;
    }
    else {
      result = n;
    }
  } while (r);
  return result;
}


palindrome
check if a linked list is a palindrome
e.g. (1,2,3,2,1) or (a,b,b,a)
struct node { node* next; int value; };
node* reverse(node* head)
{
  if (!head) return 0;
  node* r = 0;
  while(head) {
    auto n = new node;
    n->value = head->value;
    n->next = r;
    r = n;
    head = head->next;
  }
  return r;
}
void free(node* head)
{
  while (head) {
    auto next = head->next;
    delete head;
    head = next;
  }
}
bool isPalindrome(node* head)
{
  node* reversed = reverse(head);
  auto iterf = head;
  auto iterr = reversed;
  bool palindrome = true;
  while (iterf && iterr) {
    if (iterf->value != iterr->value) {
      palindrome = false;
      break;
    }
    iterf = iterf->next;
    iterr = iterr->next;
  }
  free(reversed);
  return palindrome;
}


intersection
given two singly linked lists, check if any nodes are shared (by reference) among the two lists.
we mean the node itself, not the value stored in the node.
struct node { node* next; int value; };
bool intersectingReferences(node** a, node** b)
{
  unordered_set<node*> set;
  for (auto piter = a; *piter; *piter = (*piter)->next) {
    set.insert(piter);
  }
  for (auto piter = b; *piter; *piter = (*piter)->next) {
    if (set.contains(piter)) {
      return true;
    }
  }
  return false;
}


loop detect
given a linked list that loops, return the node at the beginning of the loop.
e.g. given (a,b,c,d,e,c), return c
struct node { node* next; int value; };
node* loop(node* head)
{
  unordered_set<node*> set;
  for (auto iter = head; iter; iter = iter->next) {
    auto inserted = set.insert(iter);
    if (!inserted.second) return iter;
  }
  return 0;
}



use a single array to implement three stacks
  index % 3, interleaving the stacks into the array.
  or contiguous sections, where we'd have to reposition the middle one (or more) to ensure space for contiguous expansion.
struct threestack {
private:
  vector<int> mem;
  size_t cA = 0;
  size_t cB = 0;
  size_t cC = 0;
  size_t maxCapacity() { return max(max(cA, cB), cC); }
  template<size_t offset> void pushX(int v) {
    cA += 1;
    mem.reserve(maxCapacity());
    mem[3 * cA + 0] = v;
  }
  template<size_t offset> int popX() {
    assert(cA);
    auto result = mem[3 * cA + 0];
    cA -= 1;
    return result;
  }
public:
  void pushA(int v) { return pushX<0>(v); }
  int popA() { return popX<0>(); }
  void pushB(int v) { return pushX<1>(v); }
  int popB() { return popX<1>(); }
  void pushC(int v) { return pushX<2>(v); }
  int popC() { return popX<2>(); }
};


implement a min stack: push(x), pop(&x), min(&x)
each is O(1), and min returns the minimum value in the container.
struct minstack {
  stack<int> mins;
  stack<int> s;

  void push(int v) {
    if (mins.size()) {
      auto nextmin = min(mins.top(), v);
      mins.push(nextmin);
    }
    else {
      mins.push(v);
    }
    s.push(v);
  }
  int pop() {
    assert(mins.size());
    mins.pop();
    auto result = s.top();
    s.pop();
    return result;
  }
  int min() {
    assert(mins.size());
    return mins.top();
  }
};


implement a set of stacks, where push/pop behaves as if there's just one stack.
yet, each individual stack is limited to a maximum capacity.
template<size_t cMax>
struct setofstacks {
  vector<stack<int>> set;

  void push(int v) {
    if (!set.size()) {
      stack<int> s;
      s.push(v);
      set.emplace_back(move(s));
    }
    else {
      auto& s = set[set.size() - 1];
      if (s.size() >= cMax) {
        stack<int> s;
        s.push(v);
        set.emplace_back(move(s));
      }
      else {
        s.push(v);
      }
    }
  }
  int pop() {
    assert(set.size() > 0);
    auto& s = set[set.size() - 1];
    int result = s.top();
    if (s.size() == 1) {
      set.pop_back();
      return result;
    }
    else {
      s.pop();
      return result;
    }
  }
};


implement a queue using 2 stacks
struct queue {
  stack<int> left;
  stack<int> right;
  bool isright = true;

  void enqueue(int v) {
    if (!isright) {
      while (!left.empty()) {
        right.push(left.top());
        left.pop();
      }
      isright = true;
    }
    right.push(v);
  }
  int dequeue() {
    if (isright) {
      while (!right.empty()) {
        left.push(right.top());
        right.pop();
      }
      isright = false;
    }
    assert(left.size() > 0);
    int result = left.top();
    left.pop();
    return result;
  }
};


sort a stack, using only push/pop operations and one temporary additional stack.
void sort(stack<int>& a)
{
  if (a.empty()) return;
  stack<int> b;

  auto helper = [](stack<int>& a, stack<int>& b, size_t cA) -> size_t {
    int maxv = a.top();
    size_t cMaxes = 0;
    for (size_t i = 0; i < cA; ++i) {
      assert(!a.empty());
      int v = a.top();
      a.pop();
      b.push(v);
      if (v > maxv) {
        cMaxes = 1;
        maxv = v;
      }
      else if (v == maxv) {
        cMaxes += 1;
      }
    }

    for (size_t i = 0; i < cMaxes; ++i) {
      a.push(maxv);
    }
    for (size_t i = 0; i < cA - cMaxes; ++i) {
      assert(!b.empty());
      int v = b.top();
      b.pop();
      if (v == maxv) continue;
      a.push(v);
    }

    // Now A looks like: top{ ... unsorted ..., ... cMaxes sorted }.
    return cMaxes;
  };

  // Get the initial cA, since we're pretending stack doesn't maintain a count.
  size_t cA = 0;
  while (!a.empty()) {
    int v = a.top();
    a.pop();
    cA += 1;
    b.push(v);
  }

  // Iteratively move the maximums to the bottom of the stack.
  size_t cMaxes = 0;
  while (cA - cMaxes > 0) {
    cA -= cMaxes;
    cMaxes = helper(b, a, cA);
  }

  // Move back to A.
  while (!b.empty()) {
    int v = b.top();
    b.pop();
    a.push(v);
  }
}


implement a doubly-FIFO system that allows for dequeueing according to a total order, and two disjoint orders (which depends on the type of the element)
  enqueue
  dequeueEither
  dequeueA
  dequeueB

struct twofifosystem {
  // using integer 0 and non-zero to mean the two different kinds.
  // A = 0
  // B = some value that's not 0

  struct timed_value { int time; int value; };
  stack<timed_value> a;
  stack<timed_value> b;
  int time = 0;

  void enqueue(int kind, int value) {
    stack<timed_value>& s = kind ? b : a;
    timed_value v;
    assert(time <= numeric_limits<decltype(time)>::max() - 1);
    v.time = time++;
    v.value = value;
    s.emplace(move(value));
  }
  void dequeueEither(int* kind, int* value) {
    assert(a.size() > 0 || b.size() > 0);
    if (a.empty()) { dequeueB(kind, value); return; }
    if (b.empty()) { dequeueA(kind, value); return; }
    const auto& at = a.top();
    const auto& bt = b.top();
    if (at.time > bt.time) {
      *kind = 0;
      *value = at.value;
      a.pop();
    }
    else {
      *kind = 1;
      *value = bt.value;
      b.pop();
    }
  }
  void dequeueA(int* kind, int* value) {
    assert(a.size() > 0);
    const auto& top = a.top();
    *kind = 0;
    *value = top.value;
    a.pop();
  }
  void dequeueB(int* kind, int* value) {
    assert(b.size() > 0);
    const auto& top = b.top();
    *kind = 1;
    *value = top.value;
    b.pop();
  }
};


given a directed graph, write an algorithm to decide whether there exists a route between two given nodes
struct node {
  vector<node*> nexts;
};
bool routeExists(node* a, node* b)
{
  if (a == b) return true;
  unordered_set<node*> visited;
  stack<node*> queue;
  queue.push(a);
  while (!queue.empty()) {
    auto n = queue.top();
    queue.pop();
    auto inserted = visited.insert(n);
    if (!inserted.second) continue; // already visited.
    for (auto next : n->nexts) {
      if (next == b) return true;
      queue.push(next);
    }
  }
  return false;
}


given a sorted ascending array of unique integers, create a binary search tree with minimal height.
struct node {
  node* left;
  node* right;
  int value;
};
// unique binary search tree invariant is:
//   all nodes n in subtree(A.left) have n.value < A.value
//   all nodes n in subtree(A.right) have n.value > A.value
// ex:
//   2
//  1 3
//
//     4
//   2   6
//  1 3 5 7
//
//         8
//     4        12
//   2   6   10    14
//  1 3 5 7 9 11 13 15
//
// level binary tree encoding is:
//     0
//   1   2
//  3 4 5 6
// left(i) = 2i+1
// right(i) = 2i+2
// parent(i) = (i-1)/2
// level(i) = 64 - countl_zero(i+1) - 1
// level-start(level) = (1 << level) - 1
//
// obviously the odd nodes are sequential in the last level.
// what's the structure of the even nodes?
// every 4th starting at i=2 is at last level - 1   i = 2 + 4*k lives on last_level-1
// every 8th starting at i=4 is at last level - 2   i = 4 + 8*k lives on last_level-2
// every 16th starting at i=8 is at last level - 3  i = 8 + 16*k lives on last_level-3
// generic structure:   i = 2^e + 2^(e+1)*k lives on level = last_level - e
// then we can iterate the e values, since we know the given size, rounded-up to a power of two. (i.e. iterate levels)
// and then at each level, we can iterate k to generate the index i.
// and then span[i], we can write into the level binary tree encoding.

vector<int> makeBst(span<int> values)
{
  size_t V = values.size();
  if (!V) return {};
  vector<int> r(V);

  size_t cLevels = sizeof(size_t)*8 - countl_zero(V - 1) + 1;
  for (size_t level = 0; level < cLevels; ++level) {
    e = cLevels - 1 - level;
    uint64_t two_e = 1ull << e;
    uint64_t two_ep1 = 1ull << (e+1);
    uint64_t cNodesOnLevel = 1ull << (level+1);
    for (uint64_t k = 0; k < cNodesOnLevel; ++k) {
      uint64_t i = two_e + two_ep1*k;
      assert(i < V);
      // values[i] belongs at the k'th index in level 'level'
      auto level_start = (1 << level) - 1;
      auto level_encoding_i = level_start + k;
      r[level_encoding_i] = values[i];
    }
  }

  return r;
}


given a binary tree, create a linked list of all the nodes at each depth. (one list per level of the tree)
struct node {
  node* left;
  node* right;
};
struct lnode { lnode* next; node* value; };
vector<lnode*> makeLevelLists(node* tree)
{
  auto helper = [](vector<lnode*>& result, node* subtree, size_t depth)
  {
    if (!subtree) return;
    if (result.size() <= depth) {
      result.resize(depth + 1);
    }
    lnode** list = &result[depth];
    auto n = new lnode;
    n->next = *list;
    n->value = subtree;
    *list = n;
    helper(result, subtree->left, depth + 1);
    helper(result, subtree->right, depth + 1);
  };
  vector<lnode*> result;
  helper(result, tree, 0);
  return result;
}


given a binary tree, decide if it is balanced.
  balanced means: for every node, the two subtrees have +/-1 the same heights.
struct node {
  node* left;
  node* right;
};
size_t cNodes(node* tree)
{
  if (!tree) return 0;
  return cNodes(tree->left) + cNodes(tree->right);
}
bool isbalanced(node* tree)
{
  if (!tree) return true;
  auto cLeft = cNodes(tree->left);
  auto cRight = cNodes(tree->right);
  return ((cLeft < cRight) ? (cRight - cLeft) : (cLeft - cRight)) < 1;
}


given a binary tree, decide if it is a binary search tree.
struct node {
  node* left;
  node* right;
  int value;
}
bool isBst(node* tree)
{
  auto treeLessThan = [](node* tree, int value)
  {
    if (tree->value >= value) return false;
    if (tree->left && !treeLessThan(tree->left, value)) return false;
    if (tree->right && !treeLessThan(tree->right, value)) return false;
    return true;
  };
  auto treeGeThan = [](node* tree, int value)
  {
    if (tree->value < value) return false;
    if (tree->left && !treeGeThan(tree->left, value)) return false;
    if (tree->right && !treeGeThan(tree->right, value)) return false;
    return true;
  };
  if (tree->left && !treeLessThan(tree->left, tree->value)) return false;
  if (tree->right && !treeGeThan(tree->right, tree->value)) return false;
  return true;
}


given a binary search tree (with parent links), return the next node in the in-order traversal order.
struct node {
  node* left;
  node* right;
  node* parent;
};
node* bstNextInOrder(node* cur)
{
  //         8
  //     4        12
  //   2   6   10    14
  //  1 3 5 7 9 11 13 15
  if (cur->right) {
    // Follow the left chain down.
    auto next = cur->right;
    while (next->left) {
      next = next->left;
    }
    return next;
  }
  while (cur->parent) {
    if (cur == cur->parent->left) {
      // was the left child of the parent
      return cur->parent;
    }
    // was the right child of the parent
    // move up and check again for left/right child of the parent's parent.
    cur = cur->parent;
  }
  return 0;
}


given a list of projects and (a,b) pairs where b depends on a, return a project sequence that satisfies all dependencies.
  if it doesn't exist, return an error.
this is just topological sort.


given two nodes in a binary tree, return the first common ancestor.
  avoid storing additional nodes.
  you're not given a binary search tree.
struct node {
  node* left;
  node* right;
  node* parent;
}
node* commonAncestor(node* a, node* b)
{
  // Two linked lists, the parent chains of A and B.
  // We want to find the first common node.
  // It's dynamic how far up the parent chains we'll have to iterate. So just do it.
  vector<node*> a_parents;
  vector<node*> b_parents;

  auto helper = [](vector<node*>& result, node* n)
  {
    for (auto iter = n; iter; iter = iter->parent) {
      result.push_back(iter);
    }
  };
  helper(a_parents, a);
  helper(b_parents, b);
  // Right-align the parent chains, since the root should be common, and then we can ignore the jagged prefix.
  // e.g.
  //   { a, b, c, d, root }
  //            { e, root }
  size_t cToCheck = min(a_parents.size(), b_parents.size());
  size_t a_start = a_parents.size() - cToCheck;
  size_t b_start = b_parents.size() - cToCheck;
  for (size_t i = 0; i < cToCheck; ++i) {
    const auto& a_parent = a_parents[a_start + i];
    const auto& b_parent = b_parents[b_start + i];
    if (a_parent == b_parent) return a_parent;
  }
  // unreachable. The root should always be common.
  assert(false);
  return 0;
}


a binary search tree was created by reading an array left-to-right and inserting the elements.
there are no duplicate elements.
given such a BST, print all possible arrays that could have led to this tree.
e.g. given
  2
 1 3
prints
  (2 1 3) and (2 3 1)
classical BSTs are constructed without rebalancing, new elements always getting added as leaves.
so by definition, parents must come before children in the possible arrays.
but the left/right subtrees can be totally independent. i.e. there's a doubling of possibilities for each node with left+right.
  //         8
  //     4        12
  //   2   6   10    14
  //  1 3 5 7 9 11 13 15

  //     4
  //   2  6
  //  1 3
  // (4 2 1 3 6), (4 2 3 1 6), (4 6 2 1 3), (4 6 2 3 1), (4 2 6 1 3), (4 2 6 3 1), (4 2 1 6 3), (4 2 3 6 1)
  // note 6 can be interleaved at any place after 4
  // this is effectively preorder traversal plus reverse preorder (right before left).
  // and we need to make all possible choices of those two orders.
struct node {
  node* left;
  node* right;
  node* parent;
  int value;
};
void printPossibilities(node* tree)
{
  if (!tree) return;
  auto leaf = !tree->left && !tree->right;
  if (leaf) {
    deque<int> chain;
    for (auto iter = tree; iter; iter = iter->parent) {
      chain.push_back(iter->value);
    }
    cout << "(";
    while (!chain.empty()) {
      cout << to_string(chain.pop_front());
      if (!chain.empty()) cout << " ";
    }
    cout << ")\n";
  }
  else {
    printPossibilities(tree->
  }
}


given two binary trees, A.size() >> B.size(), return if B is a subtree of A.
  B is a subtree of A iff:
    there exists a node n in A which forms a tree identical to B.
struct node {
  node* left;
  node* right;
  int value;
};
bool isSubtree(node* b, node* a)
{
  auto treesEqual = [](node* a, node* b)
  {
    if (a == b) return true;
    if (!a ^ !b) return false;
    if (a->value != b->value) return false;
    if (!a->left ^ !b->left) return false;
    if (!a->right ^ !b->right) return false;
    if (a->left && b->left) {
      if (!treesEqual(a->left, b->left)) return false;
      if (!treesEqual(a->right, b->right)) return false;
    }
    return true;
  };
  assert(a);
  if (treesEqual(a, b)) return true;
  if (a->left && treesEqual(a->left, b)) return true;
  if (a->right && treesEqual(a->right, b)) return true;
  return false;
}


implement getRandomNode() for a binary tree. Uniformly random pdf.
// level order encoding makes this trivial: uniform random variable in { 0, 1, ..., # nodes - 1 }, return the node at that index.
// external indexing also makes this trivial.
// if it was leaves only, and complete, you could flip a coin at every node and decide whether to go left/right until hitting a leaf.
//   but, we also need to include the internal nodes. can we just say Pr(internal node is chosen) = 1 / # of nodes?
//   and if that doesn't hit, we do the 0.5 left/right?
// or we could do the leaf traversal, and then a separate parent-chain walk up. Walk up probability would be... what. 1 / some power of 2?



given a binary tree, where each node contains an arbitrary integer (could be +/-)
count the number of parent sub-chains that sum to a given value.
the terminals of the sub-chains don't have to be the root, nor leaves of the tree.
but it has to be a parent sub-chain.
struct node {
  node* left;
  node* right;
  node* parent;
  int value;
};
size_t cParentSubChainsThatSumToX(node* tree, int x)
{
  vector<node*> nodes;
  auto helper = [](vector<node*>& result, node* tree)
  {
    if (!tree) return;
    result.push_back(tree);
    helper(result, tree->left);
    helper(result, tree->right);
  };

  // All parent chains starts at all nodes.
  // For each starting node, we walk all parent sub-chains. Which means, every step until we hit the root.
  size_t r = 0;
  for (auto node : nodes) {
    int sum = 0;
    for (auto iter = node; iter; iter = iter->parent) {
      sum += iter->value;
      if (sum == x) {
        r += 1;
      }
    }
  }
}
// If we instead start top-down, then:
size_t cParentSubChainsThatSumToX(node* tree, int x)
{
  auto helper = [](size_t& result, node* tree, int x)
  {
    if (!tree) return;
    auto sum = x - tree->value;
    if (sum == 0) {
      result += 1;
    }
    helper(result, tree->left, sum);
    helper(result, tree->right, sum);
  };
  size_t result = 0;
  helper(result, tree, x);
  return result;
}



given u32 n,m; two bit positions i,j.
insert m into n s.t. m starts at bit j of n, and ends at bit i.
assume i,j are wide enough to fit m.
e.g.
n = 0000_1000_0000_0000
m = 0000_0000_0001_0011
i = 2
j = 6
r = 0000_1000_0100_1100
u32 f(u32 n, u32 m, u32 i, u32 j)
{
  assert(i <= j);
  auto nbits_m = j - i;
  auto nbits_hiclear_m = 32 - nbits_m;
  auto cleared_m = (m << nbits_hiclear_m) >> nbits_hiclear_m;
  auto r = n | (cleared_m << i);
  return r;
}


given f64, print binary representation.
void f(f64 vf)
{
  u64 v = *(u64*)(&vf);
  for (size_t i = 0; i < 64; ++i) {
    auto ri = 64 - i - 1;
    auto bit = (v >> ri) & 1u;
    cout << to_string(bit);
  }
}


given an integer. you're allowed to flip one 0 bit to a 1.
find the length of the longest sequence of 1 bits you could possibly make.
e.g.
0000_0000 -> 1 by flipping any bit
0000_0001 -> 2 by flipping bit 1.
0000_0011 -> 3
0000_0101 -> 3
there's only so many bits, so just set and check?
bool bit(u64 v, size_t i)
{
  return (v >> 63 - i) & 1;
}
size_t cMaxContiguousOneBits(u64 v)
{
  size_t max_c = 0;
  size_t c = 0;
  bool one = false;
  for (size_t i = 0; i < 64; ++i) {
    if (bit(v,i)) {
      if (!one) {
        one = true;
        c = 1;
      }
      else {
        c += 1;
        max_c = max(max_c, c);
      }
    }
    else {
      one = false;
      max_c = max(max_c, c);
      c = 0;
    }
  }
}
size_t f(u64 v)
{
  size_t c = 0;
  for (size_t i = 0; i < 64; ++i) {
    auto possibility_i = v | (1ull << i);
    c = max(c, cMaxContiguousOneBits(possibility_i));
  }
  return c;
}


given an integer. print the next smallest and next largest that have the same number of 1 bits.
e.g.
0011 ->
  0100
  0101 next largest
0110 ->
  0111
  1000
  1001 next largest
void f(u64 v)
{
  auto cOnesV = popcount(v);
  u64 larger = v + 1;
  for (; popcount(larger) != cOnesV; ++larger) {}
  u64 smaller = v - 1;
  for (; popcount(smaller) != cOnesV; --smaller) {}
  cout << smaller << "," << larger;
}


determine the number of bits you have to flip to convert given integer a to given b.
size_t f(u64 a, u64 b)
{
  u64 x = a ^ b;
  return popcount(x);
}


swap odd and even bits in an integer with as few instructions as possible.
u64 swapEvenOddBits(u64 v)
{
  auto odds  = v & 0b1010'1010'1010'1010ull;
  auto evens = v & 0b0101'0101'0101'0101ull;
  return (odds >> 1) | (evens << 1);
}


given a bitarray representing a 2d grid. each bit is a pixel.
assume the width of the grid is a multiple of 8.
given width.
draw a horizontal line from x0,y to x1,y.
void drawLine(u8* array, size_t array_len, size_t w, size_t x0, size_t x1, size_t y)
{
  assert(w % 8 == 0);
  assert(y < array_len);
  auto x0_byte = x0 / 8;
  auto x0_bit = x0 % 8;
  auto x1_byte = x1 / 8;
  auto x1_bit = x1 % 8;
  auto line_stride = w * 8;
  auto line = array + line_stride * y;
  line[x0_byte] |= 0b1111'1111u >> x0_bit;
  for (auto mid = x0_byte + 1; mid < x1_byte; ++mid) {
    line[mid] = 0b1111'1111u;
  }
  line[x1_byte] |= 0b1111'1111u << (8 - x1_bit);
}
0b____'____'____'____'____'____
  byte0    |byte1    |byte2
fill(2,17) ->
0b__11'1111'1111'1111'1___'____


given 20 bottles of pills.
19 bottles have 1 gram pills.
1 bottle has 1.1 gram pills.
given a scale, how would you find the heavy bottle?
scale can only be used once.


game 1: one shot success
game 2: 3 shots to make 2 success.
probability p to make 1 shot.
for which values of p should you choose game 1 or 2?
000 q^3
001 q^2 p
010 q^2 p
100 q^2 p
011 q p^2
101 q p^2
110 q p^2
111 p^3

g1: Pr(win) = p
g2: Pr(win) = p^3 + 3 (1-p) p^2

when g1 is favorable, choose g1.
that decision point is:
  p < p^3 + 3 (1-p) p^2
  1 < p^2 + 3 (1-p) p
  0 < p^2 + 3 p - 3 p^2 - 1
  0 < -2 p^2 + 3p - 1
quadratic fmla to solve for p.
  0 > p^2 - 1.5 p + 0.5

-b/2 +- sqrt((b/2)^2-c)
b=-1.5
c=0.5


staircase of n steps
can hop 1,2, or 3 steps at a time.
count how many possible ways to climb the stairs.
one step to go:
  1 step
two steps to go:
  1 step and recurse
  2 steps
three steps to go:
  1 step and recurse
  2 steps and recurse
  3 steps
size_t cWays(size_t n)
{
  if (n == 0) return 0;
  if (n == 1) return 1;
  if (n == 2) {
    // 2-step or 1-step.
    // return 1 + cWays(n-1);
    return 2;
  }
  if (n == 3) {
    // 3-step or 2-step or 1-step.
    // return 1 + cWays(n-2) + cWays(n-1);
    // return 1 + 1 + 2;
    return 4;
  }
  return cWays(n-3) + cWays(n-2) + cWays(n-1);
}
size_t cWays(size_t n)
{
  static constexpr N = 100;
  assert(n < N);
  size_t cWays[N];
  cWays[0] = 0;
  cWays[1] = 1;
  cWays[2] = 2;
  cWays[3] = 4;
  for (size_t i = 4; i < n; ++i) {
    cWays[i] = cWays[i-3] + cWays[i-2] + cWays[i-1];
  }
  return cWays[n];
}


turtle can move R and D, on a grid of R rows, C cols.
starts at top left.
some cells are non-traversible.
find a path from the top left to the bottom right, avoiding non-traversible cells.
e.g.
[s    ]
[ xxxx]
[ x   ]
[    f]
the path has to go down first, then right.

enum Move { R, D };
struct Path {
  deque<Move> v;
  void append(Move m) { v.push_back(m); }
  void prepend(Move m) { v.push_front(m); }
};
optional<Path> f(Path p, uint tx, uint ty)
{
  if (tx + 1 == fx && ty == fy) { // move R would finish
    auto r = p;
    r.append(R);
    return r;
  }
  else if (ty + 1 == fy && tx == fx) { // move D would finish
    auto r = p;
    r.append(D);
    return r;
  }
  if (tx + 1 < dim_x && is_traversible(tx+1, ty)) {
    auto pathR = f(p, tx + 1, ty);
    if (pathR.has_value()) {
      auto r = p;
      r.prepend(*pathR);
      return r;
    }
  }
  if (ty + 1 < dim_y && is_traversible(tx, ty+1)) {
    auto pathD = f(p, tx, ty + 1);
    if (pathD.has_value()) {
      auto r = p;
      r.prepend(*pathD);
      return r;
    }
  }
  return nullopt;
}

vector<pair<size_t,size_t>> f(
  bool* traversible, // 2d array of dim_x,dim_y
  size_t dim_x,
  size_t dim_y
  )
{
  auto is_traversible = [](bool* traversible, size_t x, size_t y) {
    return traversible[x + y * dim_x];
  };
  graph g;
  size_t cNodes = dim_x * dim_y;
  g.reserve_nodes(cNodes);
  for (size_t n = 0; n < cNodes; ++n) {
    g.add_node(n);
  }
  g.reserve_edges(cNodes * 2);
  for (size_t y = 0; y < dim_y; ++y) {
    for (size_t x = 0; x < dim_x; ++x) {
      auto n = x + y * dim_x;
      if (x+1 < dim_x && is_traversible(traversible, x+1, y)) {
        auto nR = (x+1) + y * dim_x;
        g.add_edge(n, nR);
      }
      if (y+1 < dim_y && is_traversible(traversible, x, y+1)) {
        auto nD = x + (y+1) * dim_x;
        g.add_edge(n, nD);
      }
    }
  }
  auto n_start = 0;
  auto n_finish = (dim_x-1) + (dim_y-1)*dim_x;
  vector<size_t> nodes = g.anyPathBetween(n_start, n_finish);
  if (nodes.empty()) {
    // error, no path.
    return {};
  }
  vector<Move> r;
  r.reserve(nodes.size());
  for (const auto& n : nodes) {
    size_t x = n % dim_x;
    size_t y = n / dim_x;
    r.emplace_back(x,y);
  }
  return r;
}









#endif
