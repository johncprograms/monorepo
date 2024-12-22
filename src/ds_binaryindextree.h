// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

We're given a 1-dimensional interval, model(i).
For now assume 1-based indexing.

Define:
A(i,j] = model(i+1) + ... + model(j)

LSB(i) = (i & (1 + ~i))
  LSB(1) = 1 & (1 + b..1110) = 1
  LSB(2) = 2 & (1 + b..1101) = 2
  LSB(3) = 3 & (1 + b..1100) = 1
  LSB(4) = 4 & (1 + b..1011) = 4
  ...
  
  alternate defn:
  LSB(i) = 1 << countr_zero(i)
  
V(i) = A(i-LSB(i), i]
  V(1) = A(1-1,1] = A(0,1] = model[1]
  V(2) = A(2-2,2] = A(0,2] = model[1] + model[2]
  V(3) = A(3-1,3] = A(2,3] = model[3]
  V(4) = A(4-4,4] = A(0,4] = model[1] + model[2] + model[3] + model[4]
  V(5) =                     model[5]
  V(6) =                     model[5] + model[6]
  V(7) =                     model[7]
  V(8) =                     model[1] + ... + model[8]

We store the V(i) array

#endif

struct binaryindextree_t {
  vector<int> v;
  void build(span<int> s) {
    v.assign(s.begin(), s.end());
    const size_t V = v.size();
    for (size_t i = 1; i <= V; ++i) {
      const auto parent = i + (i & (1 + ~i));
      if (parent <= V) {
        v[parent-1] += v[i-1];
      }
    }
  }
  // model[i] += delta
  void update(size_t i, int delta) {
    ++i; // make it 1-based.
    const size_t V = v.size();
    while (i < V) {
      v[i-1] += delta;
      i += (i & (1 + ~i));
    }
  }
  // model[i:j] += delta
  void updaterange(size_t i, size_t j, int delta) {
    assert(i <= j);
    update(j + 1, -delta);
    update(i, delta);
  }
  // model[0] + ... + model[i]
  int cumulativesum(size_t i) {
    ++i; // make it 1-based.
    int r = 0;
    while (i > 0) {
      r += v[i-1];
      i -= (i & (1 + ~i));
    }
    return r;
  }
  // model[i] + ... + model[j]
  int rangesum(size_t i, size_t j) {
    assert(i <= j);
    const int prefix = (i == 0) ? 0 : cumulativesum(i-1);
    return cumulativesum(j) - prefix;
  }
  // model[i]
  int value(size_t i) {
    ++i; // make it 1-based.
    int r = v[i-1];
    auto z = i - (i & (1 + ~i));
    auto y = i - 1;
    while (z != y) {
      r -= v[y-1];
      y -= (y & (1 + ~y));
    }
    return r;
  }
};

struct binaryindextree2d_t {
  
};
