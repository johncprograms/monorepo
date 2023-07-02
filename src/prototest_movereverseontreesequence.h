// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

// D is a sequence-like datastructure, with the following available:

// constructs a D in O(|D|) operations.
void build(D* d, T* data, size_t data_len);

// inserts the given datum at index i, pushing all elements at indices >= i to the right.
// O(log|D|)
void insert_at(D* d, size_t i, T* datum);

// deletes the datum at index i, moving all elements at indices > i to the left.
// O(log|D|)
T delete_at(D* d, size_t i);

void swap(D* d, size_t i, size_t j)
{
  assert(i < length(d));
  assert(j < length(d));
  if (i == j) return;
  auto elem_i = delete_at(d, i);
  insert_at(d, i, &d[j]);
  delete_at(d, j);
  insert_at(d, j, elem_i);
}

// Reverse the order of k items starting at index i.
// O(k log|D|)
void reverse(D* d, size_t i, size_t k)
{
  // If k in { 0, 1 }, we can leave element i alone.
  if (k <= 1) return;

  for (size_t m = 0; m < k / 2; ++m) {
    swap(d, m, k - 1 - m);
  }
}

// Move the k items starting at i, to be in front of the item at index j.
// Assume: j < i | j >= i + k. That is, j is outside the span we're moving.
// O(k log|D|)
void move(D* d, size_t i, size_t k, size_t j)
{
  assert(!(i <= j && j < i + k)); // j is inside the range we're moving
  if (j == i + k) return; // already in place.
  if (j > i + k) {
    // [ junk ][ move range ][ junk ]|target|[ junk ]
    //         i          i+k        j
    // If we pop from i and push at j, that will eventually rotate the move range right.
    // This is order preserving, as long as we do (j - i) pop/pushes.
    // By symmetry we can pop from j and push at i, which means one fewer -1.
    for (size_t p = 0; p < j - i; ++p) {
      auto elem_j = delete_at(d, j);
      insert_at(d, i, elem_j);
    }
  } else { // j < i
    // [ junk ]|target|[ junk ][ move range ][ junk ]
    //         j               i          i+k
    // Same idea as above.
    for (size_t p = 0; p < i + k - j; ++p) {
      auto elem_i = delete_at(d, i);
      insert_at(d, j, elem_i);
    }
  }
}

#endif
