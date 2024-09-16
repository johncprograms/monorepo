// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// min-heap, that allows this sequence of ops:
// 1. InitMinHeapInPlace
// 2. any number of calls to these functions in any order:
//    - MinHeapExtract
//    - MinHeapInsert
//
// the k-ary tree layout we're using is:
//            0
//     1      2       3
//   3 4 5  6 7 8  9 10 11
//   ...
// example 4-ary layout:
//                     0
//      1        2           3            4
//   3 4 5 6  7 8 9 10  11 12 13 14  15 16 17 18
//   ...
// which in sequential memory looks like:
//    0 1 2 3 4 5 6 ...
// this means our mapping functions are:
//    child(i,j,k) = k * i + 1 + j
//      where i: node
//            j: which child, e.g. one of { 0, 1, 2 } for a 3-ary tree.
//            k: k-ary, the max number of children per node.
//    parent(i,k) = ?
//
// 
//
//
// note this is a value-based data array, meaning we're not using auxilliary indexing.
// so, you should probably only use this for small data types, like integers.
//
// you should only use this for short to medium size heaps, since it's fundamentally an stack_resizeable_cont_t<Data>.
// huge heaps should use a smarter paging approach, like a B-heap, veb-tree, etc.
//
