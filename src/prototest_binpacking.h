// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

bin packing problem:
given a set of items with sizes, a bin capacity C, and a number of sets S,
find a disjoint partition of the set of items s.t. each set partition has total size less than C.
E.g.
set of items are simple uints: { 0, 1, 2, ..., N-1 }.
struct partition_t
{
  unordered_set<size_t> contained;
  int size = 0;
};
// Returns true if we constructed a valid packing.
// Note that since we're not doing optimal packing, this may be false in cases where we could have actually packed.
bool
BinPacking(
  int* sizes, // array of length N
  size_t N,
  int C,
  partition_t* partitions, // array of length S
  size_t S)
{
  // A naive solution is something like:
  for (size_t i = 0; i < N; ++i) {
    auto item_size = sizes[i];
    // Decide where to pack item i into.
    // Let's just say we pack it into the first partition that has room:
    bool packed = false;
    for (size_t s = 0; s < S; ++s) {
      auto partition = partitions + s;
      bool partition_has_room = C - partition->size >= item_size;
      if (partition_has_room) {
        partition->contained.insert(i);
        partition->size += item_size;
        assert(partition->size <= C);
        packed = true;
      }
    }
    if (!packed) {
      return false;
    }
  }
  return true;
}

#endif
