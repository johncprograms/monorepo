// build:console_x64_debug
// build:console_x64_optimized
// Copyright (c) John A. Carlos Jr., all rights reserved.

void
LogUI( const void* cstr ... )
{
}


#include "os_mac.h"
#include "os_windows.h"

#define TEST 1
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

#include <cassert>
#include <map>
#include <span>
using namespace std;

string longestPalindrome(string s) {
    auto L = s.size();
    size_t cmaxO = 0;
    size_t maxIL, maxIR;
    for (size_t i = 0; i < L; ++i) {
        // find the largest palindrome centered at i.
        // [1 2 3 4]
        //    ^
        // cL = 1
        // cR = 2
        // we can search min(cL,cR) and find the max palin.
        // BUGFIX:
        // case like "abccba". there is no central i.
        // in fact the central part is {i}=[2,3].
        // we can just extend i into an interval, and we'll only need to look for repeats to the right.
        // since we're maxing over all possible starting i.
        size_t j = i + 1;
        while (j < L && s[i] == s[j]) ++j;
        --j;
        // Now [i,j] is the central part.
        // [a b c c b a]
        //      i j
        // i=2
        // j=3
        // cL = 2;
        // cR = 2;
        auto cL = i;
        auto cR = L-j-1;
        auto c = min<size_t>(cL,cR);
        size_t k = 1;
        for (; k <= c; ++k) {
            auto iL = i - k;
            auto iR = j + k;
            if (s[iL] != s[iR]) break;
        }
        --k;

        size_t cmax = j-i+1 + 2*k;
        if (cmax > cmaxO) {
            cmaxO = cmax;
            maxIL = i - k;
            maxIR = j + k;
        }
    }
    if (!cmaxO) return "";
    auto sv = string_view{s.c_str()+maxIL, maxIR-maxIL+1};
    return string{sv};
}

string convert(string s, int n) {
    if (n == 1) return s;
    map<uint64_t,char> map;
    uint32_t y = 0;
    uint32_t x = 0;
    bool diag = false;
    for (auto c : s) {
        map[((uint64_t)y << 32u) | x] = c;
        if (diag) {
            assert(y > 0);
            --y;
            ++x;
            diag = (y > 0);
        }
        else {
            ++y;
            if (y == n) {
                y -= 2;
                x += 1;
                diag = (y > 0);
            }
        }
    }
    string r;
    for (const auto& it : map) {
        r += it.second;
    }
    return r;
}
constexpr bool isnum(char c) { return '0' <= c && c <= '9'; }
int myAtoi(string s) {
    auto it = begin(s);
    auto itend = end(s);
    while (it != itend && *it == ' ') ++it;
    if (it == itend) return 0;

    bool neg = false;
    if (*it == '-') {
        neg = true;
        ++it;
    }
    else if (*it == '+') ++it;

    if (it == itend) return 0;

    vector<char> digits;
    for (; it != itend && isnum(*it); ++it) {
        digits.push_back(*it);
    }
    int64_t v = 0;
    int64_t place = 1;
    // once we start adding 10B and above, we're definitely saturating.
    const int64_t maxplace = 10000000000; // 10B
    const size_t D = digits.size();
    const int64_t L = numeric_limits<int>::min();
    const int64_t R = numeric_limits<int>::max();
    for (size_t i = 0; i < D && place <= maxplace; ++i) {
        auto ri = D-i-1;
        v += place * digits[ri];
        place *= 10;
        if (v < L) { v = L; break; }
        if (v > R) { v = R; break; }
    }
    if (neg) v = -v;
    return (int)v;
}
namespace n1 {
bool match(string_view s, string_view p) {
    const size_t P = p.size();
    const size_t S = s.size();
    // S   P    result
    // ""  ".*" true
    // ""  "p"  false
    // "a" ""   false
    // ""  ""   true
    if (!P) return !S;
    // s: 012345
    // p: 0*12.*
    // s0
    auto p0 = p[0];
    auto m0 = p0 == '.' || p0 == s[0];
    if (P > 1 && p[1] == '*') {
        // either we:
        // 1. use the * 0 times, OR,
        // 2. advance and keep trying to use the *
        return match(s, p.substr(2)) || (m0 && match(s.substr(1), p));
    }
    if (!m0) return false;
    return match(s.substr(1), p.substr(1));
}
bool isMatch(string s, string p) {
    return match(s, p);
}
} // n1
vector<vector<int>> threeSum2(vector<int>& n) {
    const auto N = n.size();
    unordered_map<int,size_t> map;
    for (size_t i = 0; i < N; ++i) {
        map.emplace(n[i], i);
    }
    vector<vector<int>> ss;
    for (size_t j = 0; j < N; ++j) {
        auto target = 0 - n[j];
        for (size_t i = 0; i < N; ++i) {
            if (i == j) continue;
            auto it = map.find(target - n[i]);
            if (it == end(map) || i == it->second || j == it->second) continue;
            vector<int> r;
            r.push_back(n[i]);
            r.push_back(n[j]);
            r.push_back(n[it->second]);
            ss.emplace_back(move(r));
        }
    }
    set<vector<int>> sv;
    for (auto& s : ss) {
        sort(begin(s), end(s));
        sv.insert(s);
    }

    vector<vector<int>> r;
    for (const auto& s : sv) {
        r.emplace_back(move(s));
    }
    return r;
    // n[i]+n[j]+n[k] == 0
    // n[i] = -(n[j]+n[k])
}
vector<vector<int>> threeSum(vector<int>& n) {
    unordered_map<int,int> map;
    const size_t N = n.size();
    for (size_t i = 0; i < N; ++i) {
        auto it = map.find(n[i]);
        if (it == end(map)) map[n[i]] = 1;
        else it->second = min(it->second+1, 3);
    }
    n.clear();
    for (const auto& it : map) {
        auto c = it.second;
        while (c--) { n.push_back(it.first); }
    }
    sort(begin(n), end(n));
    return threeSum2(n);
}
struct ListNode {
  int val;
  ListNode *next;
  ListNode() : val(0), next(nullptr) {}
  ListNode(int x) : val(x), next(nullptr) {}
  ListNode(int x, ListNode *next) : val(x), next(next) {}
};
ListNode* create(const vector<int>& s) {
  ListNode* head = 0;
  ListNode* prev = 0;
  for (auto c : s) {
    auto n = new ListNode;
    n->val = c;
    n->next = 0;
    if (prev) prev->next = n;
    prev = n;
    if (!head) head = n;
  }
  return head;
}
ListNode* mergeTwoLists(ListNode* a, ListNode* b) {
    if (!a && !b) return 0;
    if (!a) return b;
    if (!b) return a;
    ListNode* head = 0;
    ListNode* prev = 0;
    while (a && b) {
        auto& m = (a->val < b->val) ? a : b;
        if (!head) head = m;
        if (prev) prev->next = m;
        prev = m;
        m = m->next;
    }
    while (a) {
        prev->next = a;
        prev = a;
        a = a->next;
    }
    while (b) {
        prev->next = b;
        prev = b;
        b = b->next;
    }
    return head;
}
int searchInsert(vector<int>& n, int target) {
    auto N = n.size();
    // [L,i,R)
    size_t L = 0;
    auto R = N;
    auto i = N/2;
    while (L != R) {
        if (n[i] == target) return i;
        if (n[i] < target) {
            L = i+1;
        }
        else {
            R = i;
        }
        i = L+(R-L)/2;
    }
    return i;
}

string addBinary(string a, string b) {
    string r;
    auto ita = rbegin(a);
    auto ea = rend(a);
    auto itb = rbegin(b);
    auto eb = rend(b);
    auto A = a.size();
    auto B = b.size();
    auto M = max(A,B);
    char carry = 0;
    for (size_t i = 0; i < M; ++i) {
        auto sum = carry;
        if (ita != ea) sum += *ita++ - '0';
        if (itb != eb) sum += *itb++ - '0';
        r += '0' + (sum & 1);
        carry = sum >> 1;
    }
    if (carry) r += '0' + carry;
    return r;
}
int threeSumClosest(vector<int>& n, int target) {
    const auto N = n.size();
    #if 0
    int64_t minsum = numeric_limits<int64_t>::max();
    int64_t mindiff = numeric_limits<int64_t>::max();
    for (size_t i = 0; i < N-2; ++i) {
        for (size_t j = i+1; j < N-1; ++j) {
            for (size_t k = j+1; k < N; ++k) {
                auto sum = (int64_t)n[i]+n[j]+n[k];
                auto diff = (target-sum);
                diff = (diff < 0) ? -diff : diff;
                if (diff < mindiff) {
                    mindiff = diff;
                    minsum = sum;
                }
            }
        }
    }
    return minsum;
    #else
    // n^2 lg n
    //map<int,size_t> m;
    //for (auto c : n) ++m[c];
    sort(begin(n),end(n));

    int64_t minsum = numeric_limits<int64_t>::max();
    int64_t mindiff = numeric_limits<int64_t>::max();
    for (size_t i = 0; i < N-1; ++i) {
        for (size_t j = i+1; j < N; ++j) {
            auto two = (int64_t)n[i]+n[j];
            // find closest to two in n, s.t. k is not i or j.
            auto it = lower_bound(begin(n),end(n), target-two);
            auto k = (it - begin(n));
            if (k == i || k == j || k == N) {
                // try both sides, moving k left and right.
                // each might be not valid (hitting end of buffer)
                auto R = k;
                while (R == i || R == k) ++R;
                auto L = k;
                while (L == i || L == k || L == N) --L;
                if (R < N) {
                    auto sum = two+n[R];
                    auto diff = abs(target - sum);
                    if (diff < mindiff) {
                        mindiff = diff;
                        minsum = sum;
                    }
                }
                if (L >= 0) {
                    auto sum = two+n[L];
                    auto diff = abs(target - sum);
                    if (diff < mindiff) {
                        mindiff = diff;
                        minsum = sum;
                    }
                }
            }
            else {
                auto sum = two+n[k];
                auto diff = abs(target - sum);
                if (diff < mindiff) {
                    mindiff = diff;
                    minsum = sum;
                }

            }
        }
    }
    return minsum;
    #endif
}

vector<vector<int>> fourSum(vector<int>& n, int target) {

    {
        vector<int> s;
        unordered_map<int,char> h;
        for (auto c : n) {
            h[c] = min(4,h[c]+1);
        }
        for (const auto& it : h) {
            for (char i = 0; i < it.second; ++i) {
                s.push_back(it.first);
            }
        }
        n = s;
    }

    const size_t N = n.size();
    if (N < 4) return vector<vector<int>>{};

    unordered_map<int,set<size_t>> map; // value -> list of indices where that value appears in n.
    for (size_t i = 0; i < N; ++i) {
        auto it = map.find(n[i]);
        if (it == end(map)) map[n[i]] = set<size_t>{{i}};
        else it->second.insert(i);
    }

    set<vector<int>> rs;
    for (size_t a = 0; a < N-3; ++a) {
    for (size_t b = a+1; b < N-2; ++b) {
    for (size_t c = b+1; c < N-1; ++c) {
        auto three = (int64_t)n[a]+(int64_t)n[b]+(int64_t)n[c];
        auto it = map.find((int)((int64_t)target-three));
        if (it == end(map)) continue;
        set<size_t> s = it->second;
        s.erase(a);
        s.erase(b);
        s.erase(c);
        if (s.empty()) continue;
        vector<int> rv; rv.resize(4);
        rv[0] = n[a];
        rv[1] = n[b];
        rv[2] = n[c];
        rv[3] = n[*begin(s)];
        sort(begin(rv),end(rv));
        rs.emplace(move(rv));
    }
    }
    }

    vector<vector<int>> r;
    for (const auto& v : rs) {
        r.push_back(v);
    }
    return r;
}
void fn(set<string>& r, int n, string_view sofar, int copen) {
    if (!n) {
        string c{sofar};
        while (copen--) c += ")";
        r.emplace(move(c));
        return;
    }
    if (copen) {
        string c{sofar};
        c += ")";
        fn(r, n-1, c, copen-1);
    }
    string s{sofar};
    s += "(";
    fn(r, n-1, s, copen+1);
}
vector<string> generateParenthesis(int n) {
    // cases:
    // ()-prefix
    // suffix-()
    // (surround)
    // we can do any of those for any of the previous set of results.
    // and there will be dupes, e.g. () -> ()() is generated by suffix and prefix rules.
    // BUGBUG: (())(()) won't be generated by these rules.
    // alternate:
    // stack<> opens;
    // for each i up to n, we can either open a new pair, or close an existing one.
    set<string> s;
    fn(s, n, "", 0);
    vector<string> r;
    for (const auto& c : s) { r.push_back(c); }
    return r;

    #if 0
    set<string> s;
    s.insert("");
    for (int i = 0; i < n; ++i) {
        set<string> t;
        for (const auto& c : s) {
            string t0 = "()"+c;
            string t1 = c+"()";
            string t2 = "("+c+")";
            t.insert(t0);
            t.insert(t1);
            t.insert(t2);
        }
        s = move(t);
    }
    vector<string> r;
    for (const auto& c : s) { r.push_back(c); }
    return r;
    #endif
}
int search(vector<int>& n, int target) {
    const auto N = n.size();
    size_t L = 0;
    size_t R = N-1;
    while (L+1 != R) {
        auto i = L+(R-L)/2;
        if (n[L] < n[i]) L = i;
        else R = i;
    }
    const bool foundPivot = n[L] > n[R];
    auto linear = [=](size_t i) {
        // 3 4 5 0 1 2
        //     L R
        // L=2
        // R=3
        // to linearize / wrap around,
        // 0 1 2 3 4 5
        // shift by +R, mod by N.
        return foundPivot ? (i+R)%N : i;
    };
    L = 0;
    R = N-1;
    while (L < R) {
        auto i = L+(R-L)/2;
        if (n[linear(i)] == target) return i;
        if (n[linear(i)] < target) L = i+1;
        else R = i;
    }
    return -1;
}

vector<int> spiralOrder(vector<vector<int>>& m) {
    const auto Y = m.size();
    const auto X = m[0].size();

    vector<int> r;
    int yl = 0;
    int yr = Y-1;
    int xl = 0;
    int xr = X-1;
    int x = xl;
    int y = yl;
    if (xl <= x && x <= xr && yl <= y && y <= yr) {
        for (;;) {
            for (; x <= xr; ++x) r.push_back(m[y][x]);
            ++yl;
            ++y;
            if (y > yr) break;
            for (; y <= yr; ++y) r.push_back(m[y][x]);
            --x;
            --xr;
            if (x < xl) break;
            for (; x >= xl; --x) r.push_back(m[y][x]);
            --yr;
            --y;
            if (y < yl) break;
            for (; y >= yl; --y) r.push_back(m[y][x]);
            ++x;
            ++xl;
            if (x > xr) break;
        }
    }
    return r;
}
namespace n2 {
bool less00(const vector<int>& a, const vector<int>& b){ return a[0] < b[0]; }
bool less01(const vector<int>& a, const vector<int>& b){ return a[0] < b[1]; }
bool less10(const vector<int>& a, const vector<int>& b){ return a[1] < b[0]; }
bool less11(const vector<int>& a, const vector<int>& b){ return a[1] < b[1]; }
bool overlap(const vector<int>& a, const vector<int>& b){
    return !((a[0] > b[1]) || (b[0] > a[1]));
}
vector<int> mergeoverlapping(const vector<int>& a, const vector<int>& b){
    vector<int> r; r.resize(2);
    r[0] = min(a[0],b[0]);
    r[1] = max(a[1],b[1]);
    return r;
}
vector<vector<int>> insert(vector<vector<int>>& intervals, vector<int>& n) {
    if (intervals.empty()) {
        intervals.push_back(n);
        return intervals;
    }
    // bsearch for new.l in ints.l
    // bsearch for new.r in ints.r
    //auto L00 = lower_bound(begin(intervals),end(intervals), n, less00);
    auto L01 = lower_bound(begin(intervals),end(intervals), n, less10);
    //auto L10 = lower_bound(begin(intervals),end(intervals), n, less10);
    auto L11 = lower_bound(begin(intervals),end(intervals), n, less11);
    // we know n[0] belongs between L0,L1.
    if (L01 == end(intervals)) {
        intervals.push_back(n);
        return intervals;
    }
    // [[1,3],[5,6],[7,7],[8,10],[12,16]]
    // e.g. n=[3,10]
    //        L00
    // L01
    //                           L10
    //                    L11
    // We want L01 and L11 to determine proper overlap.
    //
    // [[1,3],[5,6],[7,7],[8,10],[12,16]]
    // e.g. n=[4,11]
    //        L01                L11
    //
    // [[1,3],[50,60]]
    // e.g. n=[4,11]
    //        L01
    //        L11
    if (L11 == end(intervals)) {
        auto last = L11-1;
        if (overlap(*last, n)) {
            *last = mergeoverlapping(*last, n);
        }
        else {
            intervals.push_back(n);
        }
        return intervals;
    }
    assert(overlap(*L01, n));

    if (!overlap(*L11, n)) {
        --L11;
    }
    assert(overlap(*L11, n));

    if (L01==L11) {
        *L01 = mergeoverlapping(*L01, n);
        return intervals;
    }
    else {
        *L01 = mergeoverlapping(*L01, n);
        *L01 = mergeoverlapping(*L01, *L11);
        intervals.erase(L01+1, L11);
    }
    return intervals;
}

string minWindow(string s, string t) {
    unordered_map<char,int> mref;
    for (char c : t) ++mref[c];
    string_view sv(s);
    const auto S = sv.size();
    const auto T = t.size();
    if (T > S) return "";
    size_t L = 0;
    size_t R = 0;
    auto m = mref;
    for (; R<S; ++R) {
        auto it = m.find(s[R]);
        if (it == end(m)) continue;
        if (it->second == 1) { m.erase(it); if (m.empty()) break; }
        else --it->second;
    }
    if (!m.empty()) return "";
    while (L < S && !mref.contains(s[L])) ++L;

    // [L,R] is the first window.
    string_view smin = sv.substr(L,R-L+1);

    for (;;) {
        char c = sv[L];
        ++L;
        while (L<S && !mref.contains(s[L])) ++L;
        ++R;
        while (R<S && s[R] != c) {
            if (mref.contains(s[R])) ++m[s[R]];
            ++R;
        }
        if (!m.empty()) {
            ++L;
            for (; L<S && !m.empty(); ++L) {
                auto it = m.find(sv[L]);
                if (it == end(m)) continue;
                if (it->second == 1) { m.erase(it); if (m.empty()) break; }
                else --it->second;
            }
        }
        if (L == S || R == S || !m.empty()) break;

        string_view win = sv.substr(L,R-L+1);
        if (win.size() < smin.size()) smin = win;
    }
    return string(smin);
}

enum ext { num, unaryparen, unaryneg, binadd, binsub };
struct ex {
    ext type;
    union {
        int num;
        ex* unary;
        ex* binary[2];
    };
};
int eval(ex* t) {
    switch (t->type) {
        case num: return t->num;
        case unaryparen: return eval(t->unary);
        case unaryneg: return -eval(t->unary);
        case binadd: return eval(t->binary[0]) + eval(t->binary[1]);
        case binsub: return eval(t->binary[0]) - eval(t->binary[1]);
    }
    assert(false);
    return 0;
}
constexpr bool isnum(char c) { return '0' <= c && c <= '9'; }

ex* parseex(string_view s, size_t& p) {
    auto r = new ex;
    switch (s[p]) {
        case '-': {
            ++p;
            r->type = unaryneg;
            r->unary = parseex(s, p);
        } break;
        case '(': {
            ++p;
            r->type = unaryparen;
            r->unary = parseex(s,p);
            assert(s[p] == ')');
            ++p;
        } break;
        default: {
            if (isnum(s[p])) {
                auto q = p+1;
                while (q < s.size() && isnum(s[q])) ++q;
                // [p,q) is the number.
                string snum(s.substr(p,q-p));
                r->type = num;
                r->num = atoi(snum.c_str());
                p = q;
            }
        } break;
    }

    // look for binop
    bool loop = true;
    while (loop && p < s.size()) {
        switch (s[p]) {
            case '+': {
                ++p;
                auto b = new ex;
                b->type = binadd;
                b->binary[0] = r;
                b->binary[1] = parseex(s,p);
                r = b;
            } break;
            case '-': {
                ++p;
                auto b = new ex;
                b->type = binsub;
                b->binary[0] = r;
                b->binary[1] = parseex(s,p);
                r = b;
            } break;
            default: loop = false; break;
        }
    }

    return r;
}
int calculate(string s) {
    string t;
    for (char c : s) {
        if (c == ' ') continue;
        t += c;
    }
    size_t p = 0;
    string_view sv(t);
    ex* e = parseex(sv, p);
    return eval(e);
}
int numIslands(vector<vector<char>>& g) {
    const auto X = g[0].size();
    const auto Y = g.size();
    #define lin(c,d) ((d)*X+(c))
    vector<bool> visited(X*Y, false);
    int r = 0;
    struct pt { size_t x; size_t y; };
    for (size_t y = 0; y < Y; ++y) {
        for (size_t x = 0; x < X; ++x) {
            if (visited[lin(x,y)]) continue;
            if (g[y][x]=='0') continue;
            r += 1;
            vector<pt> q;
            q.push_back(pt{x,y});
            while (!q.empty()) {
                pt p = q.back();
                auto a = p.x;
                auto b = p.y;
                q.pop_back();
                visited[lin(a,b)] = true;
                if (a     && g[b][a-1]=='1' && !visited[lin(a-1,b)]) q.push_back(pt{a-1,b});
                if (a<X-1 && g[b][a+1]=='1' && !visited[lin(a+1,b)]) q.push_back(pt{a+1,b});
                if (b     && g[b-1][a]=='1' && !visited[lin(a,b-1)]) q.push_back(pt{a,b-1});
                if (b<Y-1 && g[b+1][a]=='1' && !visited[lin(a,b+1)]) q.push_back(pt{a,b+1});
            }
        }
    }
    return r;
}
}
void Junk() {
  vector<vector<char>> g;
  g.push_back(vector<char>{{'1','1','0','0','0'}});
  g.push_back(vector<char>{{'1','1','0','0','0'}});
  g.push_back(vector<char>{{'0','0','1','0','0'}});
  g.push_back(vector<char>{{'0','0','0','1','1'}});
  n2::numIslands(g);


  n2::calculate(" 2-1 + 2 ");

  n2::minWindow("ADOBECODEBANC", "ABC");

  vector<vector<int>> vv;
  vv.push_back(vector<int>{{1,2}});
  vv.push_back(vector<int>{{3,5}});
  vv.push_back(vector<int>{{6,7}});
  vv.push_back(vector<int>{{8,10}});
  vv.push_back(vector<int>{{12,16}});
  vector<int> v{{4,8}};
  n2::insert(vv, v);

  vv.clear();
  vv.push_back(vector<int>{{1,2,3}});
  vv.push_back(vector<int>{{4,5,6}});
  vv.push_back(vector<int>{{7,8,9}});
  //spiralOrder(vv);
  longestPalindrome("cbbd");
  convert("abc",2);
  myAtoi("42");
  //n1::isMatch("ab", ".*c");
//  vector<int> v{{-1,0,1,2,-1,-4}};
//  threeSum(v);
  //auto l1 = create(vector<int>{{1,2,4}});
  //auto l2 = create(vector<int>{{1,3,4}});
  //mergeTwoLists(l1,l2);
  //vector<int> v{{1,3,5,6,7}};
  //searchInsert(v,1);
  //addBinary("11","1");
  //vector<int> tsc{{-1,2,1,-4}};
  //threeSumClosest(tsc,1);
  vector<int> t{{4,5,6,7,0,1,2}};
  //fourSum(t,-294967296);
  //generateParenthesis(3);
  search(t,0);
}




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

//  CalcJunk();

//#if defined(WIN)
//  ExeJunk();
//#endif

//  Junk();

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


flood fill u8[dx][dy]
void flood(
  u8* img,
  size_t dx,
  size_t dy,
  size_t x,
  size_t y,
  u8 value)
{
  vector<bool> visited{dx*dy, false};
  queue<size_t> q;
  q.emplace(dx * y + x);
  while (!q.empty()) {
    size_t i = q.pop();
    if (visited[i]) continue;
    visited[i] = true;
    auto ix = i / dx;
    auto iy = i % dx;
    img[dx * iy + ix] = value;
    if (ix > 0) q.emplace(dx * iy + (ix - 1));
    if (ix + 1 < dx) q.emplace(dx * iy + (ix + 1));
    if (iy > 0) q.emplace(dx * (iy - 1) + ix);
    if (iy + 1 < dy) q.emplace(dx * (iy + 1) + ix);
  }
}



define 'magic index' as the 'i' where
  a[i] = i
given a sorted array 'a', return a magic index if it exists.
p1.
// [-1, 0, 2, 4] -> 2
// a[i]-i
// [-1, -1, 0, 1]
// so by using i-a[i], that gives us the natural direction to search.
// positive is to the right, negative to the left.
bool findMagic(int32_t* a, size_t len, size_t* result) {
  size_t L = 0;
  size_t R = len - 1;
  while (L != R) {
    const size_t i = L + (R+1-L)/2;
    int64_t diff = i - (int64_t)a[i];
    if (diff > 0) {
      L = i + 1;
    }
    else if (diff < 0) {
      R = (i > 0) ? i-1 : 0;
    }
    else {
      *result = i;
      return true;
    }
  }
  return false;
}

p2.
what if the array contains duplicates?
same solution.


return all subsets of a set. aka power set.
// for each character, we can include it or exclude it.
// we can bite the first char off 's', and recurse into the two possibilities.
void subsets_(string_view s, string_view result, vector<string>& v) {
  if (!s.length()) {
    v.emplace_back(result);
    return;
  }
  string t0 = result;
  string t1 = result;
  t1 += s[0];
  auto sn = string_view{s.data()+1, s.length()-1};
  subsets_(sn, t0, v);
  subsets_(sn, t1, v);
}
void subsets(string_view s, vector<string>& v) {
  subsets_(s, string_view{}, v);
}


recursive fn to multiply two positive integers w/o using multiplication operator.
add/sub/shifts are allowed.
minimize the number of those ops.
u64 mul(u32 a, u32 b) {
  auto A = min(a,b);
  auto B = max(a,b);
  u64 r = 0;
  while (A--) r += B;
  return r;
}
// By adding r to itself we could get { 0, B, 2B, 4B, 8B, ... }
// With shifts we can do multiplication by powers of 2. Even better, since we can jump straight to 8B, 4ex.
// So we want to find the largest power of 2 that's <= A.
// And then recurse on the remainder.
// E.g. if A=11, then P1=8, P2=2, P3=1, and we're done.
u32 CShiftLargestSmallerPowerOf2(u32 A) {
  // E.g.
  // (A, lzcnt(A))
  // (0b0, 32)
  // (0b1, 31)
  // (0b11, 30)
  // ...
  // (0xFFFFFFFF, 0)
  assert(A);
  u32 lzcnt = countl_zero(A);
  u32 cShift = 32 - lzcnt - 1;
  return cShift;
}
u64 mul(u32 a, u32 b) {
  auto A = min(a,b);
  auto B = max(a,b);
  u64 r = 0;
  while (A) {
    u32 cShift = CShiftLargestSmallerPowerOf2(A);
    A -= 1 << cShift;
    r += (u64)B << cShift;
  }
  return r;
}


towers of hanoi
3 towers, n ordered discs on the towers.
constraints:
1. only one disc can be moved at a time
2. LIFO towers
3. larger discs cannot be placed over smaller discs.
implement the solution to move a full stack from the first tower to the third.


return all permutations of a string.
assume the string contains no duplicate characters within it.
// n! permutations, since we can pick any character as the first, then any other char as the second, etc.
// e.g. abc ->
//   abc
//   acb
//   bac
//   bca
//   cab
//   cba
// Recursively, perm("", abc) =
//   perm(a, bc)
//   perm(b, ac)
//   perm(c, ab)
// i.e. all choices of prefix chars from the right param.
// Base case,
//   perm(f, "") = f
void perm(string_view pre, string_view s, vector<string>& result) {
  if (!s.length()) {
    result.emplace_back(pre);
    return;
  }
  for (size_t i = 0; i < s.length(); ++i) {
    string_view prefix { &s[i], 1 };
    string_view before = s.substr(0, i);
    string_view after = s.substr(i+1, s.length() - (i+1));
    string remainder = before;
    remainder += after;
    perm(prefix, remainder, result);
  }
}
bool allPerms(string_view s, vector<string>& result) {
  result.clear();
  bool overflow = false;
  size_t cPerms = factorial(s.length(), &overflow);
  if (overflow) return false;
  result.reserve(cPerms);
  perm("", s, result);
  return true;
}


p2. same as above, but duplicates are allowed in the string. the result shouldn't contain duplicate perms.
same solution, just replace vector<string> with set<string> or unordered_set<string>
.emplace_back turns into .insert or .emplace

print all valid combinations of n paren-pairs.
e.g.
given 1, print ()
               01
given 2, print (()), ()()
               0011, 0101
given 3, print ((())), (()()), ()(()), (())(), ()()()
               000111  001011  010011  001101  010101
one option is:
  for all results at n-1, we can either:
  1. wrap the result entirely, (<old-result>)
  2. prefix, ()<old-result>
  3. postfix, <old-result>()
  and then we need to de-dupe. E.g. prefix, postfix of the 0101 case are identical.
void recurStep(const set<string>& old, set<string>& n) {
  assert(n.empty());
  for (const auto& s : old) {
    string wrap = "(";
    wrap += s;
    wrap += ")";
    n.emplace(move(wrap));

    string prefix = "()";
    prefix += s;
    n.emplace(move(prefix));

    string postfix = s;
    postfix + "()";
    n.emplace(move(postfix));
  }
}
void printParenCombos(size_t n) {
  if (!n) return;
  set<string> sets[2];
  sets[0].emplace("()");
  for (size_t i = 1; i < n; ++i) {
    size_t iOld = (i+1) % 2;
    size_t iNew = i % 2;
    recurStep(sets[iOld], sets[iNew]);
  }
  auto psetPrint = &sets[(n-1)%2];
  for (const auto& s : *psetPrint) {
    cout << s << endl;
  }
}



given infinite 25c, 10c, 5c, 1c coins, return the number of ways of representing n cents.
f1(n) = 1
  always just use n 1c coins.
f5_1(n) =
  can choose 0..n/5 5c coins, and then the remainder f1 each time.
so for 7c, I can choose either:
  1. 5c, f1(2)
  2. f1(7)
12c would have one more option with 2 5c coins. And so on.
Since f1(n)=1, that means we have 1+n/5 ways. That many choices of 5c coins, basically.
f5_1(n) = 1+n/5

for f10_5_1(n) we can do the same, we have all possible choices of 10c coins up to n/10.
e.g. for 33c,
  1. f5_1(33)
  2. 10c, f5_1(23)
  3. 2*10c, f5_1(13)
  4. 3*10c, f5_1(3)
size_t f10_5_1(size_t n) {
  size_t r = 0;
  for (size_t i10 = 0; i10 <= n/10; ++i10) {
    const auto remainder10 = n - 10*i10;
    if (remainder10 > 0) {
      r += 1 + remainder10 / 5;
    }
  }
  return r;
}

by symmetry, 25c is going to be basically the same as 10c.
size_t f25_10_5_1(size_t n) {
  size_t r = 0;
  for (size_t i25 = 0; i25 <= n/25; ++i25) {
    const auto remainder25 = n - 25*i25;
    if (remainder25 > 0) {
      r += f10_5_1(remainder25);
    }
  }
  return r;
}


given n boxes.
boxes can be stacked, but only if all 3 dims are smaller than the box beneath.
return the height of the tallest possible stack.
//
struct box { float dim[3]; };
float height(const box& a) { return dim[1]; }
bool canStack(const box& a, const box& b) {
  auto ad = a.dim;
  auto bd = b.dim;
  return ad[0] < bd[0] && ad[1] < bd[1] && ad[2] < bd[2];
}
float height(span<box> boxes, span<size_t> included) {
  float r = 0;
  for (auto i : included) r += height(boxes[i]);
  return r;
}
bool is_contained(span<size_t> s, size_t i) {
  auto it = lower_bound(begin(s), end(s), i);
  return (it != end(s) && *it == s);
}
void f(
  span<box> boxes,
  size_t choice,
  span<size_t> included_so_far,
  float& highest)
{
  bool fFound = false;
  for (size_t i = 0; i < boxes.length(); ++i) {
    if (is_contained(included_so_far, i)) continue;
    bool canStack = included_so_far.length() && canStack(boxes[i], included_so_far.back());
    if (!canStack) continue;
    found = true;
    vector<size_t> next = included_so_far;
    next.push_back(i);
    f(boxes, choice+1, next, highest);
  }
  if (!fFound) {
    highest = max(highest, height(boxes, included_so_far));
    return;
  }
}



given a bool expression of 1, 0, &, |, ^, and a desired result,
return the number of ways of parenthesizing the expr s.t. it evaluates to the given result.
enum ex { b0, b1, A, O, X };
ex eval(ex lhs, ex op, ex rhs) {
  switch (op) {
    case A: return lhs & rhs;
    case O: return lhs | rhs;
    case X: return lhs ^ rhs;
  }
  assert(false);
  return 0;
}
void fn(span<ex> e, ex desired, size_t& num) {
  // even indexes are 0/1
  // odd indexes are the operators.
  // the length is always odd.
  assert(e.length() & 1);
  //
  if (e.length() == 1) {
    if (e[0] == desired) ++num;
    return;
  }
  auto nOperators = e.length() / 2;
  for (size_t i = 0; i < nOperators; ++i) {
    auto lhs = e[2*i];
    auto op  = e[2*i+1];
    auto rhs = e[2*i+2];
    auto r = eval(lhs, op, rhs);
    vector<ex> v;
    span<ex> prefix { &e[0], 2*i };
    span<ex> suffix { &e[2*i+3], e.length() - 2*i+3 };
    v.append_range(prefix);
    v.emplace_back(r);
    v.append_range(suffix);
    fn(v, desired, num);
  }
}



given a,b both sorted arrays.
a has space for b concat'ed onto it.
merge b into a, maintaining the sort.
void zipper(
  uint32_t* a,
  size_t a_len,
  uint32_t* b,
  size_t b_len
  )
{
  if (!a_len || !b_len) return;
  if (!a_len) {
    memmove(a, b, sizeof(*b)*b_len);
    return;
  }
  // we have to merge into the end, and advance towards the beginning, so as not to stomp on a.
  // e.g. if the first value of b belongs in the first slot, we don't want to have to shift
  // all of a over just so we can do that insertion.
  // by merging at the end, we're guaranteed to have moved the last element of a by the time we
  // reach that slot. worst case is we've placed all of b, and then a can be left in place.
  auto dst = a + a_len + b_len - 1;
  auto readA = a + a_len - 1;
  auto readB = b + b_len - 1;
  while (readA != a && readB != b) {
    auto cmp = *readA <=> *readB;
    i
      *dst = *readB;
      --dst;
      --readB;
    }
    else /*(cmp >= 0)*/ { // arbitrarily take from A in the =0 case.
      *dst = *readA;
      --dst;
      --readA;
    }
  }
  while (readA != a) {
    *dst = *readA;
    --dst;
    --readA;
  }
  while (readB != b) {
    *dst = *readB;
    --dst;
    --readB;
  }
  assert(dst == a);
  assert(readA == a);
  assert(readB == b);
}


implement binary search (traditional decreasing steps)


implement binary search (increasing from 0 and then decreasing steps)


implement array packing (re-contiguoize), based on valid[i].
void packContiguous(
  uint32_t* arr,
  size_t len,
  const vector<bool>& valid,
  size_t* newlen
  )
{
  *newlen = 0;
  if (!len) {
    return;
  }
  // Invariant:
  //   write is the first invalid element.
  //   read is the first valid element following write.
  // terminate when read or write falls off the end.
  size_t write = 0;
  while (;;) {
    while (write < len && valid[write]) ++write;
    if (write == len) {
      return;
    }
    size_t read = write + 1;
    while (read < len && !valid[read]) ++read;
    if (read == len) {
      return;
    }
    arr[write] = arr[read];
    ++(*newlen);
    write = read + 1;
  }
}


given a file with <4 billion u32 values, generate a u32 not present in the file.
p1. assume you can use 1GB of memory.
idea: total sort, and then look for a gap.
could do filtering to value intervals, and then sort within each.
the initial filtering passes can load just <1GB chunks at a time.
u32 is 4B, so to stay under 1GB we can only look at 256M u32s at a time.
that means k*[0,256M) are our intervals, where k is [0,16).
16 files in total.
file_t fopen(string_view filename, string_view mode);
// clips iOffset,cRead to the actual file size, and returns the clipped results.
void fread(file_t file, size_t iOffset, size_t cRead, vector<uint32_t>& result);
void fappend(file_t file, uint32_t* data, size_t len);
void fclose(file_t file);
bool main(string_view bigfile_name, uint32_t* result) {
  constexpr size_t cF = 16;
  constexpr size_t cElements = 256*1024*1024;
  vector<string> filenameG;
  filenameG.reserve(cF);
  for (size_t f = 0; f < cF; ++f) {
    string name {"tempfile"};
    name += to_string(f);
    filenameG.emplace_back(move(name));
  }
  auto bigfile = fopen(bigfile_name, "r");
  auto cleanup = Defer([&]() { fclose(bigfile); });
  vector<uint32_t> buffer{cElements};
  for (size_t f = 0; f < cF; ++f) {
    fread(bigfile, cElements * f, cElements, buffer);
    sort(begin(buffer), end(buffer));
    for (size_t g = 0; g < cF; ++g) {
      size_t valueL = cElements * g;
      size_t valueR = cElements * (g+1);
      auto itL = lower_bound(begin(buffer), end(buffer), valueL);
      auto itR = lower_bound(itL, end(buffer), valueR);
      auto fileG = fopen(filenameG[g], "w+");
      auto cleanupG = Defer([&]() { fclose(fileG); });
      fappend(fileG, itL, itR - itL);
    }
  }
  for (size_t g = 0; g < cF; ++g) {
    auto fileG = fopen(filenameG[g], "r");
    auto cleanupG = Defer([&]() { fclose(fileG); });
    fread(fileG, 0, cElements, buffer);
    sort(begin(buffer), end(buffer));
    size_t valueL = cElements * g;
    size_t valueR = cElements * (g+1);
    const size_t cBuffer = buffer.size();
    if (cBuffer > 0) {
      if (valueL != buffer[0]) {
        *result = valueL;
        return true;
      }
      if (valueR != buffer[cBuffer-1]) {
        *result = valueR;
        return true;
      }
    }
    else {
      *result = valueL;
      return true;
    }
    for (size_t i = 1; i < cBuffer; ++i) {
      if (buffer[i-1] + 1 != buffer[i]) {
        *result = buffer[i-1]+1;
        return true;
      }
    }
  }
  return false;
}

p2. you can use <10MB of memory. but you can assume <1 billion u32 values.
same as above, but lower the limits.
10MB means 2.5M u32s, and roughly 1600 temporary files.


print duplicates + rep-counts in a sorted array. RLE style.
void printDupes(uint32_t* arr, size_t len) {
  if (len <= 1) return;
  auto prev = arr[0];
  size_t rep = 1;
  for (size_t i = 1; i < len; ++i) {
    auto cur = arr[i];
    if (prev == cur) {
      ++rep;
    }
    else {
      if (rep > 1) {
        cout << to_string(prev) << "," << to_string(rep) << endl;
      }
      rep = 1;
      prev = cur;
    }
  }
  if (rep > 1) {
    cout << to_string(prev) << "," << to_string(rep) << endl;
  }
}


given a 2d matrix of u32s, where each row and col is sorted ascending
given a value, return true if it's in the matrix.


there's a stream of u32s, given you at various points in time.
  push(u32 x)
implement ds+alg to support a getRank(u32 x) method.
it returns the number of pushed elements <= the given x.
there may be duplicates in the stream, and those count towards the rank.
// rank(x) is defined as the number of items that have been pushed with a value <= x.
struct foo {
  vector<u32> v;

  // Push is O(lgN + N) = O(N)
  // Rank is O(lgN)
  void push(u32 x) {
    auto it = lower_bound(begin(v), end(v), x);
    v.insert(it, x);
  }
  size_t rank(u32 x) {
    auto it = lower_bound(begin(v), end(v), x+1);
    auto r = it - begin(v);
    return r;
  }

  // Push is O(1) amortized
  // Rank is O(N lgN + lgN) = O(N lgN). We could make it O(N + lgN) = O(N) with radix sort.
  void push(u32 x) {
    v.emplace_back(x);
  }
  size_t rank(u32 x) {
    sort(begin(v), end(v));
    return lower_bound(begin(v), end(v), x+1) - begin(v);
  }
};
// you can do better with a balanced tree that has the size of the subtree stored in each node.
// aka an 'order statistic tree'.
struct node {
  node* children[2];
  size_t cSubtree;
  u32 balanceData; // For AVL bits, or red-black bit, etc.
  u32 value;
};
// Balanced insert. May rewrite the root during rebalancing.
void insert(node** root, u32 value);
// Finds the lowest-value node where node.value >= the given value.
// It may be equal in the case of an exact match.
void lower_bound(node* root, u32 value, vector<node*>& parents);
size_t CSubtree(node* root) {
  if (!root) return 0;
  return root->cSubtree;
}

struct foo {
  node* root = nullptr;
  void push(u32 x) {
    insert(&root, x);
  }
  size_t rank(u32 x) {
    vector<node*> parents;
    parents.reserve(32);
    lower_bound(root, x, parents);
    // parent->children[0] is guaranteed to be < x. Due to the defn of lower_bound.
    // parent->children[1] is guaranteed to be > x. Same reason.
    if (parents.empty()) return 0;
    size_t r = CSubtree(parents[0]->children[0]);
    if (parents[0]->value == x) r += 1;
    // As we walk up the parent chain; we only want to count the other side of the tree we haven't
    // considered yet. And the parent itself.
    auto lastP = parents[0];
    for (size_t i = 1; i < parents.size(); ++i) {
      auto p = parents[i];
      if (lastP == p->children[0]) {
        auto notConsidered = p->children[1];
        ...
        if (notConsidered->value <= x) r += CSubtree(notConsidered);
      }
    }
  }
};



given two arrays of ints, find the pair with the smallest abs diff. return the diff.
bool diff(span<int> A, span<int> B, int& result) {
  bool found = false;
  int smallest = numeric_limits<int>::max();
  for (auto a : A) {
    for (auto b : B) {
      if (!found) {
        smallest = abs(a-b);
        found = true;
      }
      else {
        smallest = min(smallest, abs(a-b));
      }
    }
  }
  if (found) result = smallest;
  return found;
}



struct person { int b; int d; };
struct node { int x; bool b_else_d; };
bool operator<(const node& a, const node& b) { return a.x < b.x; }
size_t maxPopSize(span<person> s) {
  vector<node> v;
  v.reserve(s.length() * 2);
  for (const auto& p : s) {
    v.emplace_back(node{p.b, true});
    v.emplace_back(node{p.d, false});
  }
  sort(begin(v), end(v));
  size_t c = 0;
  size_t maxp = 0;
  for (const auto& n : v) {
    if (n.b_else_d) ++c;
    else --c;
    maxp = max(maxp, c);
  }
  return maxp;
}



2 int choices: short, long.
make k choices, and note the sum of your choices.
generate all possible sums, given k.
void fn(int k, int short, int long, unordered_set<int>& result) {
  struct node { int sum; int c; };
  // PERF: could be unordered_set<uint64_t>, since we're finding only unique sums.
  stack<node> s;
  s.emplace(node{0,0});
  while (!s.empty()) {
    auto n = s.pop();
    if (n.c == k) {
      result.insert(n.sum);
    }
    else {
      s.emplace(node{n.sum + short, n.c+1});
      s.emplace(node{n.sum + long, n.c+1});
    }
  }
}



given a set of 2d points, find a line that passes thru the most points.

struct pt { float p[2]; };
bool intersects(pt x, pt L, pt R) {
  // line(t) = L + t(R-L)
  // x intersects when
  // x = L + t(R-L)
  // if that holds,
  // x[0] = L[0] + t(R[0]-L[0])
  // x[1] = L[1] + t(R[1]-L[1])
  auto t0 = (x[0]-L[0]) / (R[0]-L[0]);
  auto t1 = (x[1]-L[1]) / (R[1]-L[1]);
  return abs(t0-t1) < 1e-5;
}
size_t cIntersections(span<pt> s, pt L, pt R) {
  size_t r = 0;
  for (auto p : s) r += intersects(p, L, R);
  return r;
}
bool fn(span<pt> s, size_t& resultL, size_t& resultR) {
  if (s.empty()) return false;
  // (s.length() == 1) is handled by writing (0,0) and returning true below.
  size_t maxc = 0;
  size_t maxi = 0;
  size_t maxj = 0;
  // NOTE: we're guaranteed that every line passes through two points.
  // So maxc = 0 initially acts as our 'not found' sentinel, and we don't need
  // to check for a separate 'are we on the first match' bool.
  for (size_t i = 0; i < s.length(); ++i) {
    for (size_t j = i + 1; j < s.length(); ++j) {
      auto L = s[i];
      auto R = s[j];
      size_t c = 0;
      for (const auto& x : s) {
        c += intersects(x, L, R);
      }
      if (c > maxc) {
        maxc = c;
        maxi = i;
        maxj = j;
      }
    }
  }
  resultL = maxi;
  resultR = maxj;
  return true;
}



mastermind:
4 independent options in 4 slots, repeats allowed.
a hit is an exact slot match.
a pseudo hit is a missed slot, same option match.
hits don't count as pseudo hits.
given a solution and a guess, return cHits and cPsuedoHits

void mastermind(span<u8> soln, span<u8> guess, u8& cHits, u8& cPseudoHits) {
  assert(soln.length() == 4);
  assert(guess.length() == 4);
  cHits = 0;
  cPseudoHits = 0;
  for (u8 i = 0; i < 4; ++i) {
    cHits += (soln[i] == guess[i])
  }

  u8 histogramS[4];
  u8 histogramG[4];
  for (u8 i = 0; i < 4; ++i) {
    ++histogramS[soln[i]];
    ++histogramG[guess[i]];
  }
  for (u8 i = 0; i < 4; ++i) {
    cPseudoHits += min(histogramS[i], histogramG[i]);
  }
  cPseudoHits -= cHits;
}



given span<int>, find indices m,n s.t. if you sorted [m,n], the whole span would be sorted.
bool findSort(span<int> s, size_t& m, size_t& n) {
  if (s.empty()) return false;
  // can skip the prefix p, as long as p <= s[m..]
  // b[i] = s[i] is <= all subsequent values in s.
  // b[last] = true, by defn.
  // And then m is the first index where b[m]==false.
  vector<bool> b{s.length(), false};
  auto minsuffix = s.back();
  b[s.length()-1] = true;
  for (size_t i = 1; i < s.length(); ++i) {
    auto ri = s.length() - 1 - i;
    if (s[ri] <= minsuffix) {
      minsuffix = s[ri];
      b[ri] = true;
    }
  }
  m = s.length();
  for (size_t i = 0; i < s.length(); ++i) {
    if (!b[i]) {
      m = i;
      break;
    }
  }
  if (m == s.length()) return false;

  // Same thing in reverse, for n.
  // Reset b to reuse it.
  for (size_t i = 0; i < s.length(); ++i) b[i] = false;

  // b[i] = s[i] is >= all previous values in s.
  // b[first] = true, by defn.
  // And then n is the last index where b[n]==false.
  auto maxprefix = s.front();
  b[0] = true;
  for (size_t i = 1; i < s.length(); ++i) {
    if (s[i] >= maxprefix) {
      maxprefix = s[i];
      b[i] = true;
    }
  }
  n = s.length();
  for (size_t i = 0; i < s.length(); ++i) {
    auto ri = s.length() - 1 - i;
    if (!b[ri]) {
      n = i;
      break;
    }
  }
  if (n == s.length()) return false;

  return true;
}



contig sequence.
given span<int>, find the sub-span with the largest sum. return the sum.
mathematically,
sum(i,j) = s[i] + ... + s[j]
find the max over all i,j.

trivial answer is all pairs:
int64_t sum(span<int> s, size_t i, size_t j) {
  int64_t r = 0;
  for (size_t k = i; k <= j; ++k) {
    r += s[k];
  }
  return r;
}
bool maxsum(span<int> s, int64_t& maxs) {
  const auto L = s.length();
  if (!L) return false;
  maxs = 0;
  for (size_t i = 0; i < L; ++i) {
    for (size_t j = i; j < L; ++j) {
      maxs = max(maxs, sum(s, i, j));
    }
  }
  return true;
}

terrible perf, N^3.
other ideas?
if there's adjacent positives, it's always worth expanding the span.
so the solution always has negatives adjacent, or the end of the given span adjacent.
so we don't have to test all pairs, just all pairs that have negatives / boundaries adjacent to them.

can we do even better?
there's got to be something with cumulative sums.
precompute
prefix(i) = sum(0,i)   = s[0] + ... + s[i]
suffix(i) = sum(i,L-1) = s[i] + ... + s[L-1]
can we just optimize the simple approach with these?

  maxs = 0;
  for (size_t i = 0; i < L; ++i) {
    for (size_t j = i; j < L; ++j) {
      maxs = max(maxs, sum(s, i, j));
    }
  }

can we write sum(i,j) in terms of suffix,prefix?
s[i] + ... + s[j]
= (s[0] + ... + s[j]) - (s[0] + ... + s[i-1])
= prefix(j) - prefix(i-1)
then we need to be careful about i=0 case.
in i=0, we just want prefix(j).
so,
sum(i,j) = prefix(j) - (i==0 ? 0 : prefix(i-1))
N^2 algorithm:

bool maxsum(span<int> s, int64_t& maxs) {
  const auto L = s.length();
  if (!L) return false;
  vector<int64_t> prefix{L};
  int64_t sum = 0;
  for (size_t i = 0; i < L; ++i) {
    sum += s[i];
    prefix[i] = sum;
  }
  maxs = 0;
  for (size_t i = 0; i < L; ++i) {
    for (size_t j = i; j < L; ++j) {
      auto sum_ij = prefix[j] - (i == 0 ? 0 : prefix[i-1]);
      maxs = max(maxs, sum_ij);
    }
  }
  return true;
}

garbage below:

we can trivially compute sum(s,i,j) in the j loop.
  maxs = 0;
  for (size_t i = 0; i < L; ++i) {
    int64_t sum_ij = 0;
    for (size_t j = i; j < L; ++j) {
      sum_ij += s[j];
      maxs = max(maxs, sum_ij);
    }
  }
better, N^2.

then note sum_ij(i) is just suffix(i).

bool maxsum(span<int> s, int64_t& maxs) {
  const auto L = s.length();
  if (!L) return false;
  vector<int64_t> suffix{L};
  vector<int64_t> suffixS{L};
  // suffix(i) = sum(i,L-1) = s[i] + ... + s[L-1].
  // aka the cumulative sum from the end back to i.
  // And,
  // suffixS(i) = max of suffix(i..L-1).
  suffix.back() = s.back();
  suffixS.back() = s.back();
  for (size_t i = 1; i < L; ++i) {
    auto ri = L - 1 - i;
    suffix[ri] = suffix[ri+1] + s[ri];
    suffixS[ri] = max(suffixS[ri+1], suffix[ri]);
  }
  maxs = 0;
  for (size_t i = 0; i < L; ++i) {
    int64_t sum_ij = suffixS[j];
    maxs = max(maxs, sum_ij);
  }
  return true;
}



patmatch
given pattern, string consisting of only a and b. E.g. "aabab"
given value, string arbitrary.
value matches pattern iff there exist string assignments to a,b s.t. when the pattern is assigned those values
in place of a,b, you get the value.
E.g.
pattern = "aabab"
value = "catcatdogcatdog"
matches, with a=cat, b=dog
return if there's a match.
bool all(string_view s, char t) {
  for (char c : s) {
    if (c != t) return false;
  }
  return true;
}
bool matching(string_view pattern, string_view aValue, string_view bValue, string_view value) {
  assert(aValue.length() > 0);
  assert(bValue.length() > 0);
  for (char c : pattern) {
    const auto& prefix = (c == 'a') ? aValue : bValue;
    if (!value.starts_with(prefix)) return false;
    value = value.substring(prefix.length());
  }
  return true;
}
bool match(string_view pattern, string_view value) {
  // if pattern is all a or all b, trivial match true.
  // This covers empty pattern as well.
  if (all(pattern, 'a') || all(pattern, 'b')) return true;

  // "" doesn't match any mixed a/b pattern.
  // Same for a single character value.
  const size_t L = value.length();
  if (L <= 1) return false;
  // So we know value has to have >=2 chars.

  // 'a' is an arbitrary contig. subset of value.
  // same for 'b'.
  // we know the first a/b in the pattern has to match the front of value.
  // WLOG, we can flip a/b and just say it's 'a' for a moment.
  // we may have to implement said flip.
  // then 'a' is some prefix of value, and 'b' is some contig. subset starting after it.
  // possible a: [0..i], for i=0..L-2
  // note we have to leave space at the end for b, since we know we have a mixed pattern of length>=2.
  // possible b: [i+1 .. L-1] given the a's i.
  for (size_t i = 0; i < L-1; ++i) {
    // a is defined as value[0..i]
    string_view aValue { value.data(), i+1 };
    for (size_t j = i+1; j < L; ++j) {
      // b is defined as value[i+1,j]
      string_view bValue { value.data()+i+1, j+1-(i+1) };
      if (matching(pattern, aValue, bValue, value)) return true;
    }
  }
  return false;
}



given 2d heightmap, integer.
0 is sea level.
>0 is land, <0 is water.
in any order, print the size of each pond.
SCCs with <0 condition, and then print the size of each SCC.
implicit 2d graph, diagonals aren't connected.

void print(int* grid, size_t dx, size_t dy) {
  unordered_map<size_t, size_t> sccFromGrid; // PERF: could be a union-find.
  vector<size_t> sccs;
  vector<bool> visited{dx*dy, false};
  stack<size_t> s;
  for (size_t y = 0; y < dy; ++y) {
  for (size_t x = 0; x < dx; ++x) {
    s.push(x+dx*y);
    while (!s.empty()) {
      auto i = s.pop();
      if (visited[i]) continue;
      visited[i] = true;
      if (grid[i] > 0) continue;
      // unvisited water.
      // either:
      // - one of the neighbors has already been visited, is also water, and is part of a SCC. Or,
      // - we're the first water in an undiscovered pond.
      bool first = true;
      auto helper = [&](size_t iN) {
        if (!visited[iN]) s.push(iN);
        if (visited[iN] && grid[iN] <= 0) {
          auto it = sccFromGrid.find(iN);
          assert(it != sccFromGrid.end());
          auto iscc = it->second;
          ++sccs[iscc];
          sccFromGrid[i] = iscc;
          first = false;
        }
      };
      if (x > 0) helper((x-1)+dx*y);
      if (x < dx-1) helper((x+1)+dx*y);
      if (y > 0) helper(x+dx*(y-1));
      if (y < dy-1) helper(x+dx*(y+1));
      if (!first) continue;
      // we're the first water in an undiscovered pond.
      auto iscc = sccs.size();
      sccs.push_back(1);
      sccFromGrid.emplace(make_pair<size_t, size_t>(i, iscc));
    }
  }
  }
  for (size_t c : sccs) {
    cout << c << endl;
  }
}




given set<string_view> validWords;
given a digit sequence, e.g. "8733"
assume this mapping of digits to possible chars
2 -> abc
3 -> def
4 -> ghi
5 -> jkl
6 -> mno
7 -> pqrs
8 -> tuv
9 -> wxyz
return the list of valid words that match the mapped digit sequence.
string_view mapping(int digit) {
  switch (digit) {
    case 2: return "abc";
    case 3: return "def";
    case 4: return "ghi";
    case 5: return "jkl";
    case 6: return "mno";
    case 7: return "pqrs";
    case 8: return "tuv";
    case 9: return "wxyz";
  }
  return string_view{};
}
void helper(const set<string_view>& validWords, string_view prefix, span<int> digits, vector<string_view>& result) {
  if (digits.empty()) {
    // base case.
    // prefix is the test string.
    if (validWords.contains(prefix)) result.push_back(prefix);
    return;
  }
  auto map = mapping(digits[0]);
  if (map.empty()) return; // TODO: error?
  digits = digits.subspan(1);
  for (char c : map) {
    string s = prefix;
    s += c;
    helper(validWords, s, digits, result);
  }
}
void foo(const set<string_view>& validWords, span<int> digits, vector<string_view>& result) {
  result.clear();
  if (digits.empty()) return;
  helper(validWords, "", digits, result);
}



sum swap
given two span<int>, find a pair, one from each, you can swap to make the two spans sum to the same value.
// diff = sum(a) - sum(b)
// we want diff == 0
// given i,j, the effect of swapping on the sums is:
// sum(a') = sum(a) - a[i] + b[j]
// sum(b') = sum(b) + a[i] - b[j]
// diff' = sum(a') - sum(b')
//   = sum(a) - a[i] + b[j] - (sum(b) + a[i] - b[j])
//   = sum(a) - a[i] + b[j] - sum(b) - a[i] + b[j]
//   = sum(a) - sum(b) - 2 a[i] + 2 b[j]
//   = diff - 2 a[i] + 2 b[j]
// So we want to check for diff'(i,j) == 0, for all i,j.
int64_t sum(span<int> s) {
  int64_t r = 0;
  for (int c : s) r += c;
  return r;
}
bool sumswap(span<int> a, span<int> b, size_t& rA, size_t& rB) {
  if (a.empty() || b.empty()) return false;
  auto diff = sum(a) - sum(b);
  auto aL = a.length();
  auto bL = b.length();
  for (size_t i = 0; i < aL; ++i) {
    for (size_t j = 0; j < bL; ++j) {
      auto diffp = diff - 2ll * a[i] + 2ll * b[j];
      if (diffp == 0) {
        rA = i;
        rB = j;
        return true;
      }
    }
  }
  return false;
}




ant grid:
infinite grid of white/black squares. starts all black.
initially faces right.
at each step:
- flip the current square color
- turn to the (was white square before flip ? right : left)
- move forward one
simulate K moves, and print the state of the grid.
struct pt { int32_t x; int32_t y; };
pt operator+(const pt& a, const pt& b) {
  return pt { a.x + b.x, a.y + b.y };
}
pt Min(const pt& a, const pt& b) {
  return pt { min(a.x, b.x), min(a.y, b.y) };
}
pt Max(const pt& a, const pt& b) {
  return pt { max(a.x, b.x), max(a.y, b.y) };
}
struct grid {
  unordered_map<uint64_t, bool> m_map;
  pt m_min;
  pt m_max;

  // TODO: combine getAndFlip, to reduce redundancy.
  bool get(pt x) {
    uint64_t i = iMap(x);
    auto it = m_map.find(i);
    if (it == end(m_map)) return false;
    return it->second;
  }
  void flip(pt x) {
    m_min = Min(m_min, x);
    m_max = Max(m_max, x);
    auto i = iMap(x);
    auto it = m_map.find(i);
    if (it == end(m_map)) {
      m_map.insert(it, true);
    }
    else {
      it->second = !it->second;
    }
  }
  void print() {
    for (int32_t y = m_min.y; y <= m_max.y; ++y) {
      for (int32_t x = m_min.x; x <= m_max.x; ++x) {
        pt p { x, y };
        auto value = get(p);
        cout << value ? '_' : 'W';
      }
      cout << endl;
    }
  }

private:
  uint64_t iMap(pt x) {
    return ((uint64_t)x.x << 32) | ((uint64_t)x.y);
  }
};
void print(size_t K) {
  const pt dirs[4] = {
    { 1, 0 },
    { 0, -1 },
    { -1, 0 },
    { 0, 1 },
  };
  pt r = { 0, 0 };
  uint8_t dir = 0;
  for (size_t i = 0; i < K; ++i) {
    auto value = grid.get(r);
    grid.flip(r);
    dir = (value ? dir + 1 : dir + 3) % 4;
    r = r + dirs[dir];
  }
  grid.print();
}

TODO: chunking, so we're doing fewer map lookups.



given rand5, implement rand7
int rand5(); returns [0..4]
int rand7() { // returns [0..6]

}



find all pairs of ints within a span which sum to a given value.
void find(span<int> s, int64_t given, vector<pair<int,int>>& result) {
  const auto L = s.length();
  for (size_t i = 0; i < L; ++i) {
    const auto si = s[i];
    for (size_t j = i + 1; j < L; ++j) {
      const auto sj = s[j];
      if (si + (int64_t)sj == given) {
        result.emplace_back(si,sj);
      }
    }
  }
}


LRU cache:
map from keys to values
struct lru {
  constexpr size_t INVALID = numeric_limits<size_t>::max();
  struct node {
    node* next;
    node* prev;
    int value;
  };
  unordered_map<int, node*> map; // key -> node
  node* head = nullptr;
  node* tail = nullptr;
  size_t cCapacity;

  lru(size_t cCapacity_) {
    assert(cCapacity_ > 0);
    cCapacity = cCapacity_;
  }
  ~lru() {
    for (auto& [key, node] : map) {
      delete node;
    }
  }
  void LLRemove(node* n) {
    // Remove from linked list
    if (head == n) head = n->next;
    if (tail == n) tail = n->prev;
    if (n->prev) n->prev->next = n->next;
    if (n->next) n->next->prev = n->prev;
    n->prev = nullptr;
    n->next = nullptr;
  }
  void LLInsert(node* n) {
    // Insert into linked list.
    n->next = head;
    if (head) head->prev = n;
    if (!tail) tail = n;
  }
  void insert(int key, int value) {
    auto it = map.find(key);
    if (it == end(map)) {
      if (map.size() == cCapacity) {
        // cache full, we have to evict.
        auto evict = head;
        LLRemove(evict);
        map.erase(map.find(evict->value));
        delete evict;
      }
      node* n = new node;
      n->value = value;
      LLInsert(n);
      map.emplace(make_pair<int, node*>(value, n));
    }
    else {
      node* n = it->second;
      n->value = value;
      LLRemove(n);
      LLInsert(n);
    }
  }
  bool get(int key, int& value) {
    auto it = map.find(key);
    if (it == end(map)) return false;
    auto n = it->second;
    value = n->value;
    LLRemove(n);
    LLInsert(n);
    return true;
  }
};



given an arithmetic expression, compute the result.
enum extype : int { add, sub, mul, div };
struct node {
  node* lhs = nullptr;
  node* rhs = nullptr;
  extype ex;
};
int Precedence(extype op) {
  switch (op) {
    case add:
    case sub: return 1;
    case mul:
    case div: return 2;
  }
  assert(false);
  return 0;
}
int64_t eval(node* tree) {
  if (tree->lhs && tree->rhs) {
    auto lhs = eval(tree->lhs);
    auto rhs = eval(tree->rhs);
    int64_t v = 0;
    switch (tree->ex) {
      case add: v = lhs + rhs; break;
      case sub: v = lhs - rhs; break;
      case mul: v = lhs * rhs; break;
      case div: v = lhs / rhs; break;
      default: assert(false);
    }
    return v;
  }
  return ex;
}
// 2+3*4
//
//    *
//  +   4
// 2 3
//
//   +
// 2   *
//    3 4
void reordertree(node** pparent) {
  auto parent = *pparent;
  if (!(parent->lhs && parent->rhs)) return;
  reordertree(&parent->lhs);
  reordertree(&parent->rhs);

  // higher precedence ops should be farther from the root.
  // lower precedence ops should be closer to the root.
  // i.e.
  // parent should be lower precedence.
  // if they're not, we swap.
  auto lhs = parent->lhs;
  if (Precedence(parent) >= Precedence(lhs)) {
    // rotate as above.
    *pparent = lhs;
    parent->lhs = lhs->rhs;
    lhs->rhs = parent;
  }
}
void freeTree(node* t) {
  if (!(parent->lhs && parent->rhs)) return;
  freeTree(t->lhs);
  freeTree(t->rhs);
  delete t;
}
int64_t eval(span<extype> eq) {
  // even indices are integer numbers
  // odd  indices are extype operators
  auto L = eq.length();
  assert(L & 1);
  if (L == 1) return eq[0];
  node* last = new node;
  last->ex = eq[0];
  for (size_t i = 0; i < L/2; ++i) {
    node* rhs = new node;
    rhs->ex = eq[2*i+2];
    node* op = new node;
    op->ex = eq[2*i+1];
    op->lhs = last;
    op->rhs = rhs;
    last = nop;
  }
  reordertree(&last, last->lhs);
  eval(last);
  freeTree(last);
}



add two numbers without using arithmetic ops.
uint8_t add(uint8_t A, uint8_t B) {
  uint8_t r = 0;
  auto getcarry = [](uint8_t a, uint8_t b, uint8_t c, uint8_t i) {
    return
      ((((a>>i)&1)<<1) | ((b>>i)&1)) == 3 ||
      ((((a>>i)&1)<<1) | ((c>>i)&1)) == 3 ||
      ((((b>>i)&1)<<1) | ((c>>i)&1)) == 3;
  };
  // 000         -> 0
  // 001/010/100 -> 1
  // 011/101/110 -> 0
  // 111         -> 1
  //
  //   0 1
  // 0 0 1
  // 1 1 0c
  auto getsum = [](uint8_t a, uint8_t b, uint8_t c, uint8_t i) {
    return ((a>>i)&1)^((b>>i)&1)^((c>>i)&1);
  };
  uint8_t c = 0;
  for (size_t i = 0; i < 8; ++i) {
    r |= getsum( ((a>>i)&1), ((b>>i)&1), ((c>>i)&1), 0 ) << i;
    c = getcarry( ((a>>i)&1), ((b>>i)&1), ((c>>i)&1), 0 );
  }
  return r;
}



perfect shuffle, assuming a perfect rng
void shuffle(span<int> d, rng_t& rng) {
  const auto D = d.length();
  size_t top = D;
  while (top--) {
    size_t select = rng.uniform_uint(0, top); // [0,top]
    auto tmp = d[select];
    d[select] = d[top];
    d[top] = tmp;
  }
}



random set of m integers from an array of size n.
uniformly random prob per elem.
unordered_set<int> subset(span<int> a, size_t m) {
  const auto L = a.length();
  unordered_set<int> r;
  if (!L) return r;

  rng_t rng;
  for (size_t i = 0; i < m; ++i) {
    size_t select = rng.uniform_uint(0, L); // [0,L]
    r.insert(a[select]);
  }
  return r;
}



A contains all integers from 0 to n, except one number.
only operation we have is:
bool getbit(size_t jBit, size_t iElem);

if A is sorted, we can binary search on the LSB matching even/odd index.
we can count parity of a single j bit across all elements, and repeat across bits to reconstruct.
error correcting code idea.
// LSB is at index 0 of the result.
// MSB is at the end of the result vector.
vector<bool> reconstruct(size_t AL, size_t B) {
  vector<bool> r;
  const auto n = AL + 1;
  // if n is even,
  //   parity of the j'th bit across all i elements should be 0.
  // e.g. n=4
  // 00
  // 01
  // 10
  // 11
  //
  // if 00 is missing, we'll see more 1s than 0s in each place, so we can reconstruct it should be 00.
  // we can +1 for 1s, and -1 for 0s, to keep track of which is more common.
  // can we do it even simpler with bit op reductions?
  // I think it's just an xor reduction.
  // TODO: with register-sized chunking, we could do this reduction in bulk much faster.
  for (size_t i = 0; i < AL; ++i) {
    uint8_t b;
    for (size_t j = 0; j < B; ++j) {
      auto A_ij = getbit(j, i);
      b ^= A_ij;
    }
    r.push_back(b);
  }
  return r;
}



given a string of letters and numbers, find the longest span with an equal number of each.
bool longest(string_view s, size_t& rL, size_t& rR) {
  const auto L = s.length();
  if (!L) return false;
  // we could just iterate all spans... N^3
  // for (size_t i = 0; i < L - 1; ++i) {
  //   for (size_t j = i + 1; j < L; ++j) {
  //     ...
  //   }
  // }
  //
  // aaa000
  //   --
  // the pair of a0 is interesting, since we know that's part of any potential span.
  // and then we can look at expanding it / merging it with adjacent potential spans.
  // a0a0a0a0
  // --__--__
  // a0aa00
  // -- --
  // So we could find all the a0 pairs first.
  // Then try to maximally expand them left/right as long as left/right are opposite.
  // Then merge adjacent spans.
  // Then loop.
  //    Do we need to expand l/r again?
  //    aa0a00
  //     --__
  //    yes, indeed.
  struct sp { size_t l; size_t r; };
  vector<sp> spans;
  for (size_t i = 0; i < L-1; ++i) {
    if (isalpha(s[i]) == isnum(s[i+1])) {
      spans.emplace_back(i, j);
    }
  }

  if (spans.empty()) return false;

  bool loop;
  do {
    loop = false;

    {
      // merge adjacent spans.
      // note we zipper merge because we can remove as we're iterating.
      // 'write' points to the last valid span.
      // 'read' points to the first unconsidered span after that.
      // note there can be a gap built up in between, as long as we keep merging into 'write'.
      sp* write = begin(spans);
      sp* read = write+1;
      while (read != end(spans)) {
        if (write->r + 1 == read->l) {
          loop = true;
          write->r = read->r;
          // don't advance write, advance read.
          ++read;
        }
        else {
          // advance write, read.
          // also copy read to write, to bridge the invalid gap that's built up.
          // we could avoid the write if write==read, but I don't now if it's worth the branch.
          ++write;
          *write = *read;
          ++read;
        }
      }
    }

    // expand spans.
    {
      for (size_t i = 0; i < spans.size(); ++i) {
        auto tryAgain = true;
        while (tryAgain) {
          tryAgain = false;

          bool lNum = false;
          bool lAlpha = false;
          bool lHas = false;
          auto si = spans[i];
          if (si.l > 0 && i > 0 && spans[i-1].r + 1 < si.l) {
            lHas = true;
            lNum = isnum(s[si.l-1]);
            lAlpha = isalpha(s[si.l-1]);
          }
          bool rNum = false;
          bool rAlpha = false;
          bool rHas = false;
          if (si.r + 1 < L && i+1 < spans.size() && si.r+1 < spans[i+1].l) {
            rHas = true;
            rNum = isnum(s[si.r+1]);
            rAlpha = isalpha(s[si.r+1]);
          }
          if (rHas && lHas) {
            assert(lAlpha || lNum);
            assert(rAlpha || rNum);
            if (lAlpha == rNum) {
              si.l -= 1;
              si.r += 1;
              loop = true;
              tryAgain = true;
            }
          }
        } // end while (tryAgain)
      } // end for
    }
  } while (loop);

  size_t ispan = spans.size();
  size_t maxW = 0;
  for (size_t i = 0; i < spans.size(); ++i) {
    auto si = spans[i];
    auto w = si.r - si.l + 1;
    if (w >= maxW) {
      maxW = w;
      ispan = i;
    }
  }
  if (ispan == spans.size()) return false;
  rL = spans[ispan].l;
  rR = spans[ispan].r;
  return true;
}



count the number of 2s in the base10 reps of all in [0,n], given n.
e.g.
n=24
{2, 12, 20, 21, 22, 23, 24} -> 8
note that 22 contains 2 2s, so it's 8, not 7.

[0,9] has 1.
[10,19] has 1
[20,29] has 10+1
[30,39] has 1
...
[100,109] has 1
[110,119] has 1
[120,129] has 10+1
[130,139] has 1
...
[200,209] has 10+1
[210,219] has 10+1

we could just build a big table to binary search.

closed form would be better...

ones place: (d >= 2) ? 1 : 0




void dedupe(
  span<pair<string_view, size_t>> histogramWithDupes,
  span<pair<string_view, string_view>> aliases, // left/right are unordered.
  vector<pair<string_view, size_t>>& result)
{
  struct node { node* parent; string_view value; };
  map<string_view, node*> unionfind;
  for (auto& [alias0, alias1] : aliases) {
    auto it = unionfind.find(alias0);
    if (it == end(unionfind)) {
      node* n = new node;
      n->parent = nullptr;
      n->value = alias0;
      unionfind.emplace(make_pair<string_view, node*>(alias0, n));
      unionfind.emplace(make_pair<string_view, node*>(alias1, n));
    }
    else {
      auto n0 = it->second;
      auto it1 = unionfind.find(alias1);
      if (it1 == end(unionfind)) {
        unionfind.emplace(make_pair<string_view, node*>(alias1, n0));
      }
      else {
        auto n1 = it1->second;
        // union find reparent the whole n1 parent chain to n0.
        vector<node*> v;
        v.push_back(n1);
        while (n1->parent) {
          n1 = n1->parent;
          v.push_back(n1);
        }
        for (node* n : v) {
          n->parent = n0;
        }
      }
    }
  } // end for

  map<string_view, size_t> histogramDedupe;
  for (auto& [name, count] : histogramWithDupes) {
    auto nameDedupe = name;
    auto it = unionfind.find(name);
    if (it != end(unionfind)) {
      auto n = it->second;
      while (n->parent) n = n->parent; // PERF: could be an if. Or we could make a map<sv,sv> above.
      nameDedupe = n->value;
    }
    auto itd = histogramDedupe.find(nameDedupe);
    if (itd == end(histogramDedupe)) {
      histogramDedupe.emplace(make_pair<string_view, size_t>(nameDedupe, count));
    }
    else {
      it->second += count;
    }
  }

  result.clear();
  for (auto& [nameDedupe, count] : histogramDedupe) {
    result.emplace_back(nameDedupe, count);
  }

  for (auto& [alias, n] : unionfind) {
    delete n;
  }
}




// find min distance between two given words in a document.
vector<size_t> match(span<int> words, int w) {
  vector<size_t> r;
  for (size_t i = 0; i < words.size(); ++i) {
    if (words[i] == w) r.push_back(i);
  }
  return r;
}
size_t absdiff(size_t a, size_t b) {
  return max(a,b) - min(a,b);
}
bool foo(
  span<int> words,
  int w0,
  int w1,
  size_t& minD
  )
{
  // PERF: could merge the two document loops.
  auto match0 = match(words, w0);
  auto match1 = match(words, w1);
  minD = numeric_limits<size_t>::max();
  bool found = false;
  // For perf reasons, outer loop here should be the shorter of the two lists.
  // This is N lg M, so we want the N<M case, not N>M.
  const bool larger0 = match0.size() > match1.size();
  const auto& larger = (larger0 ? match0 : match1);
  const auto& smaller = (larger0 ? match1 : match0);
  for (auto m0 : smaller) {
    auto it1 = lower_bound(begin(larger), end(larger), m0);
    // it1 points to the first element >= m0.
    // it1-1 will be the first element < m0 if it exists.
    // so these are the two we need to check distance for.
    // the rest are guaranteed to be farther away.
    if (it1 != end(larger)) {
      auto d = absdiff(*it1, m0);
      if (!found || d < minD) {
        found = true;
        minD = d;
      }
    }
    if (it1 != begin(larger)) {
      auto d = absdiff(*(it1-1), m0);
      if (!found || d < minD) {
        found = true;
        minD = d;
      }
    }
  }
  return found;
}



convert a BST to an ordered double LL.
struct node { node* l; node* r; int value; };
node* successor(node* n) {
  if (n->r) {
    n = n->r;
    while (n->l) n = n->l;
    return r;
  }
  else {
    // while (n) {
    //   auto p = n->parent;
    //   if (n == p->left) return p;
    //   n = p;
    // }
    // return n;
  }
}

node* rightmost(node* n) {
  while (n->r) n = n->r;
  return n;
}
node* leftmost(node* n) {
  while (n->l) n = n->l;
  return n;
}
void relink(node* t) {
  auto l = t->l;
  auto r = t->r;
  // TODO: can I return this up from relink instead?
  // it's a BST so I could do compares I think.
  // can I do it w/o compares?
  auto lR = rightmost(l);
  auto rL = leftmost(r);
  if (l) relink(l);
  t->l = lR;
  t->r = rL;
  if (r) relink(r);
}
void relink2(node* t, node** rMost, node** lMost) {
  auto l = t->l;
  auto r = t->r;
  if (!l && lMost) *lMost = t;
  if (!r && rMost) *rMost = t;
  node* lR;
  node* rL;
  if (l) relink2(l, &lr, (node**)nullptr);
  if (r) relink2(r, (node**)nullptr, &rL);
  t->l = lR;
  t->r = rL;
}
void relink3(node* t) {
  auto l = t->l;
  auto r = t->r;
  node* lR;
  node* rL;
  if (l) relink2(l, &lr, (node**)nullptr);
  if (r) relink2(r, (node**)nullptr, &rL);
  t->l = lR;
  t->r = rL;
}




given a string that had the spaces removed, and a dict of valid strings,
reconstruct the original string with minimal invalid characters.

void fn(const set<string_view>& valid, string_view s, string& result) {
  // for each character, we can choose to include it as a valid char or not.
  // and then we can pick any valid subset starting at that char, or not.
  // effectively a maximum matching.
  // choice:
  // 1.
  const auto L = s.length();
  vector<bool> invalid{L, false};

}



smallest k of an array:
void fn(span<int> s, size_t k, vector<int>& result) {
  priority_queue_max<int> pq;
  for (int c : s) {
    if (pq.size() < k) {
      pq.insert(c);
    }
    else {
      auto m = pq.pop();
      pq.insert((c < m) ? c : m);
    }
  }

  result.clear();
  for (int c : pq) {
    result.push_back(c);
  }
}



longest compound word, given a list of words. e.g. { dog, cat, dogcat } should return dogcat
bool fn(span<string_view> list, size_t& result) {
  // we can check all pairs.
  map<string_view, size_t> map;
  const auto L = list.length();
  for (size_t i = 0; i < L; ++i) {
    map.emplace(make_pair<string_view, size_t>(list[i], i));
  }

  size_t cmax = 0;
  size_t imax = 0;
  for (size_t i = 0; i < L; ++i) {
    auto si = list[i];
    for (size_t j = 0; j < L; ++j) {
      if (i == j) continue;
      auto sj = list[j];
      string tmp = si;
      tmp += sj;
      auto it = map.find(tmp);
      if (it == end(map)) continue;
      if (tmp.size() > cmax) {
        cmax = tmp.size();
        imax = it->second;
      }
    }
  }
  if (cmax == 0) return false;
  result = imax;
  return true;
}



appt scheduling:
given a contig. slicing of time w/o gaps.
all multiples of 15mins.
there has to be 15min gaps between chosen appts.
find the optimal assignment of appts (most total minutes).
e.g. { 30, 15, 60, 75, 45, 15, 15, 45 }
        ^       ^       ^           ^
optimal: { 30, 60, 45, 45 }

score(i..k) =
  0 if i>k.
  max {
    minutes(i) + score(i+2..k) if we choose i
    score(i+1..k) if we don't choose i
  }

just finding the score (most total minutes):
int64_t optimal(size_t i, span<int> s) {
  const auto L = s.length();
  if (i > L) return 0; // base case
  const auto opt1 = s[i] + optimal(i+2, s);
  const auto opt2 = optimal(i+1, s);
  return max(opt1, opt2);
}
can we dynamic program it? compute in reverse.

int64_t optimal2(span<int> s, vector<size_t>& result) {
  result.clear();
  const auto L = s.length();
  if (!L) return 0;
  vector<bool> which{L, false};
  vector<int> opt{L+2};
  opt[L+1] = 0;
  opt[L] = 0;
  for (size_t i = 0; i < L; ++i) {
    auto ri = L - i - 1;
    auto opt1 = s[ri] + opt[ri+2];
    auto opt2 = opt[ri+1];
    which[ri] = opt1 >= opt2;
    opt[ri] = max(opt1, opt2);
  }
  // which says if the appt was chosen, or not.
  for (size_t i = 0; i < L; ++i) {
    if (which[i]) {
      result.push_back(i);
    }
  }
  return opt[0];
}



given a large string, and a set of small strings, find the instances in the large string.

void fn(
  string_view large,
  span<string_view> small,
  map<size_t, size_t>& matches) // index into large, index into small.
{
  matches.clear();

  const auto S = small.length();
  trie t;
  for (const auto& s : small) {
    t.insert(s);
  }

  struct mstate {
    trie_cursor* c;
    size_t match_start;
  };
  vector<mstate> cursors;
  for (size_t i = 0; i < S; ++i) {
    cursors.push_back(mstate{begin(t), 0});
  }

  const auto L = large.length();
  for (size_t i = 0; i < L; ++i) {
    const auto li = large[i];
    for (size_t s = 0; s < S; ++s) {
      auto& m = cursors[s];
      auto nextc = m.c->next(li);
      // cases:
      // 1. m.c was pointing to root
      // 2. m.c was pointing to subtree node.
      // and for each of those,
      // 1. nextc is not a valid child of m.c.
      // 2. nextc is a valid child of m.c
      // 3. nextc is a terminal node.
      const auto cwasroot = m.c == begin(t);
      const auto foundchild = nextc != nullptr;
      const auto finishedmatch = foundchild && nextc->terminal;
      // We could simplify these cases, I'm sure.
      if (cwasroot) {
        if (finishedmatch) {
          matches.emplace(make_pair<size_t,size_t>(m.match_start, s));
          m.c = begin(t);
        }
        else if (foundchild) {
          m.match_start = i;
          m.c = nextc;
        }
        else {
          // leave m.c as root.
        }
      }
      else {
        if (finishedmatch) {
          matches.emplace(make_pair<size_t,size_t>(m.match_start, s));
          m.c = begin(t);
        }
        else if (foundchild) {
          m.c = nextc;
        }
        else {
          m.c = begin(t);
        }
      }
    }
  }
}




given two arrays:
  one short (no dupes)
  one long (maybe dupes, unsorted)
find the shortest span in long that contains all of short.

unordered_map<int, size_t> histogram(span<int> s) {
  unordered_map<int, size_t> r;
  for (int c : s) {
    auto it = r.find(c);
    if (it == end(r)) {
      r.emplace(make_pair<int,size_t>(c, 1));
    }
    else {
      ++it->second;
    }
  }
  return r;
}
bool fn(span<int> short, span<int> long, size_t& rL, size_t& rR) {
  //   1 1 1 1
  //  2   2
  // 3      3
  //    4
  // given some position x, for each element in S, we need the closest instance of s to x in both directions.
  // then we need the max of the closest distances in both directions, and that's our smallest span, given x.
  // then we take the min over all x.
  //
  // we could simplify by only looking right, and iterating x to exclude the prefix long.
  //
  // does it help to transpose to index space?
  // i.e. element of s -> list of long indices
  // unsure.
  // i'll start with the simple soln above.

  const auto L = long.length();
  const auto S = short.length();
  if (S > L) return false;
  unordered_set<int> s;
  for (int c : s) {
    c.insert(c);
  }
  size_t minL = 0;
  size_t minR = 0;
  bool found = false;
  for (size_t x = 0; x < L-S; ++x) {
    auto sT = s; // clone
    size_t y = x;
    while (y < L && !sT.empty()) {
      auto it = sT.find(long[y]);
      if (it != end(sT)) {
        sT.erase(it);
      }
      ++y;
    }
    if (!sT.empty()) {
      // no such span exists.
      // we don't have to check spans starting at x+1, since we know that won't work.
      // excluding x from the span won't make any elements magically appear.
      break;
    }
    // [x,y) is the span.
    if (!found || y-x < minR-minL) {
      found = true;
      minL = x;
      minR = y;
    }
  }
  if (!found) return false;
  rL = minL;
  rR = minR;
}



maintain a median value as numbers are added to a stream.
that requires storing infinite unique history, since early values may recur as the median later.

struct stream {
  priority_queue_min<int> minq;
  priority_queue_max<int> maxq;
  // all elements of maxq <= all elements of minq.
  // invariant: |minq.size() - maxq.size()| <= 1
  // that is, the median is either minq.top() or maxq.top(), depending on size parity.
  //

  // { ..., A } { B, ... }
  // inserting v, we have cases:
  // 1. |L| == |R|
  // 2. |L| < |R|
  // 3. |L| > |R|
  // WLOG, we can say A <= v < B via PQ push/pop swapping v with A,B as appropriate.
  // Once we've done that, then we just choose the PQ of smaller size to push to.
  void insert(int value) {
    if (maxq.empty()) {
      maxq.push(value);
      return;
    }
    if (minq.empty()) {
      auto L = maxq.top();
      maxq.pop();
      auto At = min(L, value);
      auto Bt = max(L, value);
      maxq.push(At);
      minq.push(Bt);
      return;
    }
    auto L = maxq.size();
    auto R = minq.size();

    auto A = maxq.top();
    auto B = minq.top();
    if (value < A) {
      maxq.pop();
      maxq.push(value);
      swap(value, A);
    }
    else if (value > B) {
      minq.pop();
      minq.push(value);
      swap(value, B);
    }

    auto& q = (L < R) ? maxq : minq;
    q.push(value);
  }
  int median() {
    assertCrash(!minq.empty() || !maxq.empty());
    if (minq.empty()) {
      return maxq.top();
    }
    auto A = maxq.top();
    auto B = minq.top();
    return (int)(((int64_t)A+(int64_t)B)/2);
  }
};



histogram volume.
given a column chart, columns are width=1.
            |
            |  x  |
      |  x  |  x  |
      |  x  |  |  |  x  |
0, 0, 2, 0, 4, 1, 3, 0, 1
should return 5, the x being water.

we need to find the tallest column on either side, to know if we have water at a given cell.
we also need to consider positions with columns, since they can be buried in water.

could precompute tallest on the right, tallest on the left.
tallR(i) = max { heights[j] } over j in {i+1..}
tallL(i) = max { heights[j] } over j in {..i-1}

then at position i, the amount of water we can stack is:
  min(tallR(i), tallL(i)) - height[i]

int64_t volume(span<int> heights) {
  auto H = heights.size();
  if (!H) return 0;
  vector<int> tallR{H};
  vector<int> tallL{H};
  tallL[0] = heights[0];
  tallR[H-1] = heights[H-1];
  for (size_t i = 1; i < H; ++i) {
    tallL[i] = max(tallL[i-1], heights[i]);
    auto ri = H-1-i;
    tallR[ri] = max(tallR[ri+1], heights[ri]);
  }

  int64_t r = 0;
  for (size_t i = 0; i < H; ++i) {
    auto waterline = min(tallL[i], tallR[i]);
    auto hi = height[i];
    if (waterline > height) {
      r += waterline - height;
    }
  }
  return r;
}



given two words of equal length,
given a dictionary,
transform the first word into the second by changing one char at a time.
each intermediate word must also be in the dictionary.

bool matchSub1(string_view a, string_view b) {
  assert(a.size() == b.size();
  size_t c = 0;
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) {
      ++c;
      if (c == 2) break;
    }
  }
  return c == 1;
}
vector<node*> pathSearch(
  const vector<node*>& nodes,
  node* source,
  node* sink);
void fn(
  string_view w1,
  string_view w2,
  const set<string_view>& dict,
  vector<string_view>& result)
{
  // we can't go directly, targeting w2 characters directly.
  // aaa
  // baa
  // bba
  // bbc
  // bdc
  // ddc
  // so i think we basically need to do a graph search.
  struct node {
    string_view w;
    vector<node*> nbrs;
  };
  vector<node*> nodes;
  auto source = new node;
  source->w = w1;
  nodes.push_back(source);
  auto sink = new node;
  sink->w = w2;
  nodes.push_back(sink);
  for (const auto& s : dict) {
    node* n = new node;
    n->w = s;
    nodes.push_back(n);
  }

  size_t N = nodes.size();
  for (size_t i = 0; i < N; ++i) {
    node* ni = nodes[i];
    for (size_t j = 0; j < N; ++j) {
      auto nj = nodes[j];
      if (i == j) continue;
      if (matchSub1(ni->w, nj->w)) {
        ni->nbrs.push_back(nj);
      }
    }
  }

  // Path search
  result.clear();
  vector<node*> path = pathSearch(nodes, source, sink);
  for (const auto& n : path) {
    result.push_back(n->w);
  }
}



max black square
given a 2d grid of black/white values
find the max subsquare s.t. the border is all black.

struct pt { size_t x; size_t y; };
size_t latestTrueXR(
  const vector<bool>& grid, // black=true.
  size_t dx,
  size_t dy,
  size_t x,
  size_t y)
{
  size_t xR = x;
  for (; xR + 1 < dx; ++xR) {
    if (!grid[xR+1+dx*y]) break;
  }
  return xR;
}
bool isBorder(
  const vector<bool>& grid, // black=true.
  size_t dx,
  size_t dy,
  size_t x,
  size_t y,
  size_t xR,
  size_t yR)
{
  for (size_t i = x; i <= xR; ++i) {
    if (!grid[i+dx*y]) return false;
    if (!grid[i+dx*yR]) return false;
  }
  for (size_t j = y; j <= yR; ++j) {
    if (!grid[x+dx*y]) return false;
    if (!grid[xR+dx*y]) return false;
  }
  return true;
}
bool maxSubsq(
  const vector<bool>& grid, // black=true.
  size_t dx,
  size_t dy,
  pt& rL,
  pt& rR)
{
  // if we collect max spans, does that help?
  //
  //   b b b b  0,3
  //   _ b _ b  1,1  3,3
  //   _ b _ b  1,1  3,3
  //   b b b b  0,3
  //
  //   0,0
  //   3,3
  //     0,3
  //       0,0
  //       3,3
  //         0,3
  //
  // i'm not sure it does.
  // i think we just have to check the possibilities.

  size_t maxArea = 0;
  pt maxL;
  pt maxR;
  for (size_t y = 0; y < dy; ++y) {
    for (size_t x = 0; x < dx; ++x) {
      if (!grid[x+dx*y]) continue;

      for (size_t yR = y; yR < dy; ++yR) {
        for (size_t xR = x; xR < dx; ++xR) {
          if (!isBorder(grid, dx, dy, x, y, xR, yR)) continue;
          const auto area = (yR-y+1)*(xR-x+1);
          if (area > maxArea) {
            maxArea = area;
            maxL = pt{x,y};
            maxR = pt{xR,yR};
          }
        }
      }
    }
  }
  if (maxArea == 0) return false;
  rL = maxL;
  rR = maxR;
  return true;
}




max submatrix

int64_t sum(
  span<int> grid,
  size_t dx,
  size_t dy,
  pt L,
  pt R)
{

}
bool maxsum(
  span<int> grid,
  size_t dx,
  size_t dy,
  pt& rL,
  pt& rR)
{
  size_t L = dx*dy;
  vector<int64_t> cumuL{L};
  vector<int64_t> cumuR{L};
  cumuL[0] = grid[0];
  cumuR[0] = grid[L-1];
  for (size_t x = 1; x < dx; ++x) {
    cumuL[x] = grid[x] + cumuL[x-1];
    auto rx = dx - x;
    cumuR[rx] = grid[rx] + cumuR[rx-1];
  }
  for (size_t y = 1; y < dy; ++y) {
    cumuL[dx*y] = grid[dx*y] + cumuL[dx*(y-1)];
    auto ry = dy - y;
    cumuR[dx*ry] = grid[dx*ry] + cumuR[dx*(ry-1)];
  }
  for (size_t y = 1; y < dy; ++y) {
    auto ry = dy - y;
    for (size_t x = 1; x < dx; ++x) {
      auto rx = dx - x;
      cumuL[x+dx*y] = grid[x-1+dx*y] + grid[x+dx*(y-1)];
      cumuR[rx+dx*ry] = grid[rx-1+dx*ry] + grid[rx+dx*(ry-1)];
    }
  }

  int64_t maxSum = 0;
  bool found = false;
  for (size_t y = 0; y < dy; ++y) {
    for (size_t x = 0; x < dx; ++x) {
      for (size_t yR = y; yR < dy; ++yR) {
        for (size_t xR = x; xR < dx; ++xR) {
          auto sum = cumuR[x+dx*y] - cumuL[xR+dx*yR];
          if (!found || sum > maxSum) {
            found = true;
            maxSum = sum;
            rL = pt{x,y};
            rR = pt{xR,yR};
          }
        }
      }
    }
  }
  return found;
}



word rectangle
given millions of words, form the largest fully filled crossword, where every row/col is a word in the list.
given { ab, ba, aa }
we can return {ab, ba}
  ab
  ba







#endif
