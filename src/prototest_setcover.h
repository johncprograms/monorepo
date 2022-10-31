// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

set cover problem:
given a set of multisets,
construct the smallest set of elements of the multisets, s.t. every input set has at least one element represented.
for example,
given
  { { 0, 0, 0 }, { 1, 1, 1 }, { 0, 2, 3 } }
we should construct
  { 0, 1 }
since 0 is covering the first and third input sets, and 1 is covering the second input set.
we can't use fewer elements than two.

struct
element_count_t
{
  int element;
  unordered_set<size_t> multisets_covered;
};
bool CompareCount(const element_count_t& a, const element_count_t& b)
{
  return a.multisets_covered.size() < b.multisets_covered.size();
}

unordered_set<int>
SetCovering(
  unordered_multiset<int>* multisets,
  size_t N)
{
  // So we could insert all possible elements into result, and then remove redundant ones.
  // Or we could build up result through set unions / intersections / etc.
  // So my set intersection idea is:
  // Construct the set of elements for each multiset. Call these elements_i.
  // Look for elements_i which are entirely subsumed by another elements_j, for i != j. Ignore these elements_i.
  //   This would eliminate the redundant input multisets from consideration.
  // Then we still have redundant elements to deal with, i.e. elements that appear in multiple elements_i.
  // We could pick the elements that appear in the largest number of elements_i, which would then eliminate
  //   those elements_i from future consideration.
  // So going by sorted frequency, that'd give us some benefit. Not optimal, but probably good.

  // And actually the elements_i subsuming test is entirely orthogonal here, so I'll leave that out.
  // We can always add that later I think.

  unordered_map<int, unordered_set<size_t>> element_counts;
  for (size_t i = 0; i < N; ++i) {
    const auto& multiset = multisets[i];
    for (int element : multiset) {
      auto iter = find(begin(element_counts), end(element_counts), element);
      if (iter == end(element_counts)) {
        element_counts[element] = unordered_set<size_t>{i};
      }
      else {
        iter->second.insert(i);
        element_counts[element] = iter->second;
      }
    }
  }

  vector<element_count_t> sorted_element_counts;
  sorted_element_counts.reserve(element_counts.size());
  for (const auto& iter : element_counts) {
    sorted_element_counts.emplace_back(iter->first, move(iter->second));
  }
  sort(begin(sorted_element_counts), end(sorted_element_counts), CompareCount);

  unordered_set<int> result;

  unordered_set<size_t> covered_multisets;
  for (const auto& element_count : sorted_element_counts) {

    // This element could be redundant due to previous iterations of this loop!
    // So we need to check if it's redundant.
    // Or, we need to remove future-iteration elements when choosing this one as a covering element.

    int element = element_count.element;
    bool redundant = true;
    for (size_t covered_multiset : element_count.multisets_covered) {
      if (!covered_multisets.contains(covered_multiset)) {
        redundant = false;
        break;
      }
    }
    if (redundant) {
      continue;
    }
    // Now we know this element covers a multiset we haven't yet covered.
    // So, use this element as a covering element.

    result.insert(element);
    for (size_t covered_multiset : element_count.multisets_covered) {
      covered_multisets.insert(covered_multiset);
    }
  }

  return result;
}

#endif
