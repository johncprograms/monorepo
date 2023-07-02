// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

Define L as a list of spans (where each span stores how many rooms are needed in total for that span).
The spans in the list are pairwise non-overlapping.

struct
span_t
{
  size_t num_rooms;
  T t0;
  T t1;
};

L = list of span_t.

Given two separate such lists, merge them into one list.
The room requests from the two lists are independent, so num_rooms is additive across the two lists.

// l0:  |   | |    | |
// l1: | |     |    |

vector<ssize_t>
boundariesFromList(const vector<span_t>& l0)
{
  vector<ssize_t> boundaries0;
  boundaries0.reserve(2 * l0.size());
  for (const auto& s : l0) {
    boundaries0.push_back(s.t0);
    boundaries0.push_back(-s.t1);
  }
  return boundaries0;
}

vector<span_t>
problem(const vector<span_t>& l0, const vector<span_t>& l1)
{
  vector<ssize_t> boundaries0 = boundariesFromList(l0);
  vector<ssize_t> boundaries1 = boundariesFromList(l1);
  const auto L0 = boundaries0.size();
  const auto L1 = boundaries1.size();
  vector<ssize_t> boundaries;
  boundaries.reserve(L0 + L1);

  // Zipper merge the two boundariesI lists.
  size_t i0 = 0;
  size_t i1 = 0;
  while (i0 != L0 || i1 != L1) {
    if (i0 != L0 && i1 != L1) {
      auto e0 = abs(boundaries0[i0]);
      auto e1 = abs(boundaries1[i1]);
      // == matters because we should close things before we open. I think. To preserve minimal rooms req'd.
      if (e0 == e1) {
        if (boundaries0[i0] < 0) {
          boundaries.push_back(boundaries0[i0]);
          ++i0;
          boundaries.push_back(boundaries1[i1]);
          ++i1;
        } else { // it's fine that we use this order for boundaries1[i1] < 0 or > 0.
          boundaries.push_back(boundaries1[i1]);
          ++i1;
          boundaries.push_back(boundaries0[i0]);
          ++i0;
        }
      } else if (e0 < e1) {
        boundaries.push_back(boundaries0[i0]);
        ++i0;
      } else { // e0 > e1
        boundaries.push_back(boundaries1[i1]);
        ++i1;
      }
    } else if (i0 != L0) {
      // TODO: short circuit and bulk copy.
      boundaries.push_back(boundaries0[i0]);
      ++i0;
    } else { // (i1 != L1)
      // TODO: short circuit and bulk copy.
      boundaries.push_back(boundaries1[i1]);
      ++i1;
    }
  }

  // Reconstruct a span list from the merged boundaries list.
  vector<span_t> result;
  result.reserve(boundaries.size() / 2);
  size_t level = 0;
  for (const auto b : boundaries) {
    if (b >= 0) {
      ++level;
      if (!result.size() || result[level - 1].t0 == b
    }
  }

  return result;
}


#endif
