// Copyright (c) John A. Carlos Jr., all rights reserved.

struct avl {
  avl* child[2];
  int8_t balance;
  int val;
};
ForceInl int8_t& Balance(avl* t) { return t->balance; }
ForceInl avl** Child(avl* t) { return &t->child[0]; }
ForceInl int& Val(avl* t) { return t->val; }

void _assertAvlBalanced(avl* t, sidx_t& height) {
  auto left = Child(t)[0];
  auto right = Child(t)[1];
  sidx_t left_h = 0;
  sidx_t right_h = 0;
  if (left) _assertAvlBalanced(left, left_h);
  if (right) _assertAvlBalanced(right, right_h);
  height = 1 + MAX(left_h, right_h);
  auto balance = right_h - left_h;
  AssertCrash(-1 <= balance && balance <= 1);
  AssertCrash(balance == Balance(t));
}
void assertAvlBalanced(avl* t) {
  if (!t) return;
  sidx_t h = 0;
  _assertAvlBalanced(t, h);
}

avl* find(avl* tree, int val) {
  for (auto p = tree; p; ) {
    auto cmp = val <=> Val(p);
    if (cmp == 0) return p;
    if (cmp < 0) p = Child(p)[0];
    else /*cmp > 0*/ p = Child(p)[1];
  }
  return 0;
}
constant idx_t cMaxDepth = 32;
avl* emplace(avl** tree, avl* item) {
  if (!*tree) {
    *tree = item;
    AssertCrash(!Child(item)[0]);
    AssertCrash(!Child(item)[1]);
    AssertCrash(Balance(item) == 0);
    return item;
  }

  bool da[cMaxDepth]; // FUTURE: can be uint32_t
  uint8_t k = 0;

  static_assert(offsetof(avl, child) == 0);
  avl* z = (avl*)tree;
  avl* y = *tree;
  bool dir = 0;
  avl* q = z;
  for (auto p = y; p; p = Child(p)[dir]) {
    auto cmp = Val(item) <=> Val(p);
    if (cmp == 0) return p;
    if (Balance(p)) {
      z = q;
      y = p;
      k = 0;
    }

    dir = (cmp > 0);
    da[k++] = dir;
    q = p;
  }

  Child(q)[dir] = item;
  auto n = item;

  Child(n)[0] = 0;
  Child(n)[1] = 0;
  Balance(n) = 0;
  if (!y) return n;

  k = 0;
  for (auto p = y; p != n;) {
    if (!da[k]) Balance(p) -= 1;
    else Balance(p) += 1;
    p = Child(p)[da[k]];
    k += 1;
  }

  avl* w = 0;
  if (Balance(y) == -2) {
    auto x = Child(y)[0];
    if (Balance(x) == -1) {
      w = x;
      Child(y)[0] = Child(x)[1];
      Child(x)[1] = y;
      Balance(x) = 0;
      Balance(y) = 0;
    }
    else {
      AssertDebug(Balance(x) == 1);
      w = Child(x)[1];
      Child(x)[1] = Child(w)[0];
      Child(w)[0] = x;
      Child(y)[0] = Child(w)[1];
      Child(w)[1] = y;
      if (Balance(w) == -1) {
        Balance(x) = 0;
        Balance(y) = 1;
      }
      elif (Balance(w) == 0) {
        Balance(x) = 0;
        Balance(y) = 0;
      }
      else {
        AssertDebug(Balance(x) == 1);
        Balance(x) = -1;
        Balance(y) = 0;
      }
      Balance(w) = 0;
    }
  }
  elif (Balance(y) == 2) {
    auto x = Child(y)[1];
    if (Balance(x) == 1) {
      w = x;
      Child(y)[1] = Child(x)[0];
      Child(x)[0] = y;
      Balance(x) = 0;
      Balance(y) = 0;
    }
    else {
      w = Child(x)[0];
      Child(x)[0] = Child(w)[1];
      Child(w)[1] = x;
      Child(y)[1] = Child(w)[0];
      Child(w)[0] = y;
      if (Balance(w) == 1) {
        Balance(x) = 0;
        Balance(y) = -1;
      }
      elif (Balance(w) == 0) {
        Balance(x) = 0;
        Balance(y) = 0;
      }
      else {
        AssertDebug(Balance(w) == -1);
        Balance(x) = 1;
        Balance(y) = 0;
      }
      Balance(w) = 0;
    }
  }
  else {
    return n;
  }

  Child(z)[y != Child(z)[0]] = w;
  return n;
}
avl* erase(avl** tree, avl* item) {
  avl* pa[cMaxDepth];
  bool da[cMaxDepth]; // FUTURE: can be uint32_t

  uint32_t k = 0;
  static_assert(offsetof(avl, child) == 0);
  avl* p = (avl*)tree;
  std::strong_ordering cmp = std::strong_ordering::less;
  while (cmp != 0) {
    auto dir = (cmp > 0);
    pa[k] = p;
    da[k] = dir;
    k += 1;
    p = Child(p)[dir];
    if (!p) return 0;
    cmp = Val(item) <=> Val(p);
  }

  item = p;

  if (!Child(p)[1]) {
    Child(pa[k-1])[da[k-1]] = Child(p)[0];
  }
  else {
    auto r = Child(p)[1];
    if (!Child(r)[0]) {
      Child(r)[0] = Child(p)[0];
      Balance(r) = Balance(p);
      Child(pa[k-1])[da[k-1]] = r;
      da[k] = 1;
      pa[k] = r;
      k += 1;
    }
    else {
      avl* s = 0;
      auto j = k++;
      Forever {
        da[k] = 0;
        pa[k] = r;
        k += 1;
        s = Child(r)[0];
        if (!Child(s)[0]) break;
        r = s;
      }
      Child(s)[0] = Child(p)[0];
      Child(r)[0] = Child(s)[1];
      Child(s)[1] = Child(p)[1];
      Balance(s) = Balance(p);
      Child(pa[j-1])[da[j-1]] = s;
      da[j] = 1;
      pa[j] = s;
    }
  }

  AssertCrash(k > 0);
  while (--k) {
    auto y = pa[k];
    if (da[k] == 0) {
      Balance(y) += 1;
      if (Balance(y) == 1) break;
      if (Balance(y) == 2) {
        auto x = Child(y)[1];
        if (Balance(x) == -1) {
          auto w = Child(x)[0];
          Child(x)[0] = Child(w)[1];
          Child(w)[1] = x;
          Child(y)[1] = Child(w)[0];
          Child(w)[0] = y;
          if (Balance(w) == 1) {
            Balance(x) = 0;
            Balance(y) = -1;
          }
          elif (Balance(w) == 0) {
            Balance(x) = 0;
            Balance(y) = 0;
          }
          else {
            Balance(x) = 1;
            Balance(y) = 0;
          }
          Balance(w) = 0;
          Child(pa[k-1])[da[k-1]] = w;
        }
        else {
          Child(y)[1] = Child(x)[0];
          Child(x)[0] = y;
          Child(pa[k-1])[da[k-1]] = x;
          if (Balance(x) == 0) {
            Balance(x) = -1;
            Balance(y) = 1;
            break;
          }
          Balance(x) = 0;
          Balance(y) = 0;
        }
      }
    }
    else { // (da[k] != 0)
      Balance(y) -= 1;
      if (Balance(y) == -1) break;
      if (Balance(y) == -2) {
        auto x = Child(y)[0];
        if (Balance(x) == 1) {
          auto w = Child(x)[1];
          Child(x)[1] = Child(w)[0];
          Child(w)[0] = x;
          Child(y)[0] = Child(w)[1];
          Child(w)[1] = y;
          if (Balance(w) == -1) {
            Balance(x) = 0;
            Balance(y) = 1;
          }
          elif (Balance(w) == 0) {
            Balance(x) = 0;
            Balance(y) = 0;
          }
          else {
            Balance(x) = -1;
            Balance(y) = 0;
          }
          Balance(w) = 0;
          Child(pa[k-1])[da[k-1]] = w;
        }
        else {
          Child(y)[0] = Child(x)[1];
          Child(x)[1] = y;
          Child(pa[k-1])[da[k-1]] = x;
          if (Balance(x) == 0) {
            Balance(x) = 1;
            Balance(y) = -1;
            break;
          }
          Balance(x) = 0;
          Balance(y) = 0;
        }
      }
    }
  }

  return item;
}

#pragma warning(disable:4530)
RegisterTest([]()
{
  rng_lcg_t lcg;
  Init( lcg, 0x123456789 );

  auto makenode = [](int val) {
    avl r;
    r.val = val;
    r.child[0] = 0;
    r.child[1] = 0;
    r.balance = 0;
    return r;
  };
  constant idx_t cNodes = 1000;
  std::vector<avl> nodes;
  nodes.resize(cNodes);
  for (idx_t i = 0; i < cNodes; ++i) {
    nodes[i] = makenode(Rand32(lcg));
  }
  avl* root = 0;
  for (idx_t i = 0; i < cNodes; ++i) {
    assertAvlBalanced(root);
    emplace(&root, &nodes[i]);
    assertAvlBalanced(root);
  }

  // FUTURE: random order erase, not the same as insert.
  for (idx_t i = 0; i < cNodes; ++i) {
    assertAvlBalanced(root);
    erase(&root, &nodes[i]);
    assertAvlBalanced(root);
  }

  AssertCrash(root == 0);

  constant idx_t cIters = 100000;
  std::unordered_set<int> set;
  nodes.resize(0);
  nodes.reserve(cIters);
  for (idx_t i = 0; i < cIters; ++i) {
    assertAvlBalanced(root);
    auto z = Zeta32(lcg);
    if (z < 0.3 && set.size()) {
      auto s = *std::begin(set);
      auto snode = find(root, s);
      erase(&root, snode);
      set.erase(s);
    }
    else {
      int s = Rand32(lcg);
      auto snode = makenode(s);
      nodes.emplace_back(std::move(snode));
      emplace(&root, &nodes[nodes.size()-1]);
      set.insert(s);
    }
    assertAvlBalanced(root);
  }
  
  assertAvlBalanced(root);
});




struct node {
  node* left = 0;
  node* right = 0;
  node* parent = 0;
  int val = 0;
  bool flagL = 0;
  bool flagR = 0;
};
ForceInl node*& Left(node* t) { return t->left; }
ForceInl node*& Right(node* t) { return t->right; }
ForceInl node*& Parent(node* t) { return t->parent; }
ForceInl int& Val(node* t) { return t->val; }
ForceInl bool Balanced(node* t) { return  t->flagL &&  t->flagR; }
ForceInl bool IsLeft(node* t)   { return  t->flagL && !t->flagR; }
ForceInl bool IsRight(node* t)  { return !t->flagL &&  t->flagR; }
ForceInl void SetBalanced(node* t) { t->flagL = 1; t->flagR = 1; }
ForceInl void SetLeft(node* t)     { t->flagL = 1; t->flagR = 0; }
ForceInl void SetRight(node* t)    { t->flagL = 0; t->flagR = 1; }

void _assertAvlBalanced(node* t, sidx_t& height) {
  if (!t) {
    height = 0;
    return;
  }
  sidx_t left_h = 0;
  auto left = Left(t);
  if (left) _assertAvlBalanced(left, left_h);
  sidx_t right_h = 0;
  auto right = Right(t);
  if (right) _assertAvlBalanced(right, right_h);
  height = 1 + MAX(left_h, right_h);
  auto balance = right_h - left_h;
  AssertCrash(-1 <= balance && balance <= 1);
  if (balance < 0) { AssertCrash(IsLeft(t)); }
  elif (balance > 0) { AssertCrash(IsRight(t)); }
  else AssertCrash(Balanced(t));
}
void assertAvlBalanced(node* t) {
  if (!t) return;
  sidx_t h = 0;
  _assertAvlBalanced(t, h);
}

// Returns the address of the node with the given value.
// Or if not present, returns the address of the left/right field where it would be added.
node** find(node** pt, int val) {
  AssertCrash(pt);
  Forever {
    auto t = *pt;
    if (!t) break;
    auto cmp = Val(t) <=> val;
    if (cmp == 0) break;
    pt = (cmp < 0) ? &Left(t) : &Right(t);
  }
  return pt;
}
// Returns the left-most node in the given subtree.
node* minimum(node* t) {
  AssertCrash(t);
  while (Left(t)) t = Left(t);
  return t;
}
// Returns the right-most node in the given subtree.
node* maximum(node* t) {
  AssertCrash(t);
  while (Right(t)) t = Right(t);
  return t;
}
// Returns the next node in the linear order.
node* successor(node* t) {
  if (Right(t)) {
    return minimum(Right(t));
  }
  auto p = Parent(t);
  while (p && t == Parent(p)) {
    t = p;
    p = Parent(p);
  }
  return p;
}
// Returns the previous node in the linear order.
node* predecessor(node* t) {
  if (Left(t)) {
    return maximum(Left(t));
  }
  auto p = Parent(t);
  while (p && t == Parent(p)) {
    t = p;
    p = Parent(p);
  }
  return p;
}
// Given:
//     A
//   *   C
//      B ^
//
// Rotates the tree to:
//     C
//   A   ^
//  * B
//
node* rotateL(node* A, node* C) {
  // FUTURE: we can reorder a lot of these ops so we write to each node at once.
  auto B = Left(C);
  Left(C) = A;
  Right(A) = B;
  Parent(A) = C;
  if (B) Parent(B) = A;
  if (Balanced(C)) {
    SetRight(A);
    SetLeft(C);
  }
  else {
    SetBalanced(A);
    SetBalanced(C);
  }
  return C;
}
// Given:
//     A
//   C   *
//  ^ B
//
// Rotates the tree to:
//     C
//   ^   A
//      B *
//
node* rotateR(node* A, node* C) {
  // FUTURE: we can reorder a lot of these ops so we write to each node at once.
  auto B = Right(C);
  Right(C) = A;
  Left(A) = B;
  Parent(A) = C;
  if (B) Parent(B) = A;
  if (Balanced(C)) {
    SetLeft(A);
    SetRight(C);
  }
  else {
    SetBalanced(A);
    SetBalanced(C);
  }
  return C;
}
// Given:
//       A
//   *       E
//         C   ^
//        B D
//
// Rotates the tree to:
//       C
//   A       E
// *   B   D   ^
//
node* rotateRL(node* A, node* E) {
  // FUTURE: we can reorder a lot of these ops so we write to each node at once.
  auto C = Left(E);
  auto D = Right(C);
  auto B = Left(C);
  Right(C) = E;
  Left(C) = A;
  Left(E) = D;
  Parent(E) = C;
  Right(A) = B;
  Parent(A) = C;
  if (D) Parent(D) = E;
  if (B) Parent(B) = A;
  if (Balanced(C)) {
    SetBalanced(A);
    SetBalanced(E);
  }
  elif (IsRight(C)) {
    SetLeft(A);
    SetBalanced(E);
  }
  else {
    SetBalanced(A);
    SetRight(E);
  }
  SetBalanced(C);
  return C;
}
// Given:
//       A
//   E       *
// ^   C
//    D B
//
// Rotates the tree to:
//       C
//   E       A
// *   D   B   ^
//
node* rotateLR(node* A, node* E) {
  // FUTURE: we can reorder a lot of these ops so we write to each node at once.
  auto C = Right(E);
  auto D = Left(C);
  auto B = Right(C);
  Left(C) = E;
  Right(C) = A;
  Right(E) = D;
  Parent(E) = C;
  Left(A) = B;
  Parent(A) = C;
  if (D) Parent(D) = E;
  if (B) Parent(B) = A;
  if (Balanced(C)) {
    SetBalanced(A);
    SetBalanced(E);
  }
  elif (IsLeft(C)) {
    SetRight(A);
    SetBalanced(E);
  }
  else {
    SetBalanced(A);
    SetLeft(E);
  }
  SetBalanced(C);
  return C;
}
void avlBalanceAfterInsert(node** root, node* z) {
  for (auto x = Parent(z); x; x = Parent(z)) {
    node* g = 0;
    node* n = 0;
    if (z == Right(x)) {
      if (IsRight(x)) {
        g = Parent(x);
        if (IsLeft(z)) {
          n = rotateRL(x, z);
        }
        else {
          n = rotateL(x, z);
        }
      }
      else { // IsLeft(x) || Balanced(x)
        if (IsLeft(x)) {
          SetBalanced(x);
          break;
        }
        SetRight(x);
        z = x;
        continue;
      }
    }
    else { // z == Left(x)
      if (IsLeft(x)) {
        g = Parent(x);
        if (IsRight(z)) {
          n = rotateLR(x, z);
        }
        else {
          n = rotateR(x, z);
        }
      }
      else { // IsRight(x) || Balanced(x)
        if (IsRight(x)) {
          SetBalanced(x);
          break;
        }
        SetLeft(x);
        z = x;
        continue;
      }
    }
    Parent(n) = g;
    if (g) {
      if (x == Left(g)) {
        Left(g) = n;
      }
      else {
        Right(g) = n;
      }
    }
    else {
      *root = n;
    }
    break;
  } // end for
}
void insert(node** root, node* z) {
#if DBG
  assertAvlBalanced(*root);
#endif

  node* y = 0;
  node* x = *root;
  std::strong_ordering cmp = std::strong_ordering::less;
  while (x) {
    y = x;
    cmp = Val(z) <=> Val(x);
    x = (cmp < 0) ? Left(x) : Right(x);
  }
  Parent(z) = y;
  if (!y) {
    SetBalanced(z);
    *root = z;
    return;
  }
  if (cmp < 0) {
    Left(y) = z;
  }
  else {
    Right(y) = z;
  }

  avlBalanceAfterInsert(root, z);

#if DBG
  assertAvlBalanced(*root);
#endif
}
// Replace node u with node v.
void repointParent(node** root, node* u, node* v) {
  auto parent_u = Parent(u);
  if (!parent_u) {
    *root = v;
  }
  elif (u == Left(parent_u)) {
    Left(parent_u) = v;
  }
  else {
    Right(parent_u) = v;
  }
  if (v) {
    Parent(v) = parent_u;
  }
}
void avlBalanceAfterDelete(node** root, node* n) {
  node* g = 0;
  bool balanced = false;
  for (auto x = Parent(n); x; x = g) {
    g = Parent(x);
    if (n == Left(x)) {
      if (IsRight(x)) {
        auto z = Right(x);
        balanced = Balanced(z);
        if (IsLeft(z)) {
          n = rotateRL(x, z);
        }
        else {
          n = rotateL(x, z);
        }
      }
      else { // IsLeft(x) || Balanced(x)
        if (Balanced(x)) {
          SetRight(x);
          break;
        }
        n = x;
        SetBalanced(n);
        continue;
      }
    }
    else { // n == Right(x)
      if (IsLeft(x)) {
        auto z = Left(x);
        balanced = Balanced(z);
        if (IsRight(z)) {
          n = rotateLR(x, z);
        }
        else {
          n = rotateR(x, z);
        }
      }
      else { // IsRight(x) || Balanced(x)
        if (Balanced(x)) {
          SetLeft(x);
          break;
        }
        n = x;
        SetBalanced(n);
        continue;
      }
    }
    Parent(n) = g;
    if (g) {
      if (x == Left(g)) {
        Left(g) = n;
      }
      else {
        Right(g) = n;
      }
    }
    else {
      *root = n;
    }

    if (balanced) break;
  } // end for
}
void erase(node** root, node* d) {
#if DBG
//  assertAvlBalanced(*root);
#endif

  AssertCrash(d);
  node* n = 0;
  if (!Left(d)) {
    auto right_d = Right(d);
    repointParent(root, d, right_d);
//    if (right_d) n = Parent(right_d);
    n = right_d;
  }
  elif (!Right(d)) {
    auto left_d = Left(d);
    repointParent(root, d, left_d);
//    if (left_d) n = Parent(left_d);
    n = left_d;
  }
  else {
    auto e = successor(d);
    if (Parent(e) != d) {
      repointParent(root, e, Right(e));
      Right(e) = Right(d);
      Parent(Right(e)) = e;
    }
    repointParent(root, d, e);
    Left(e) = Left(d);
    Parent(Left(e)) = e;
//    n = Parent(e);
    n = e;
  }

//  if (n) avlBalanceAfterDelete(root, n);

#if DBG
//  assertAvlBalanced(*root);
#endif
}

// the binary tree layout we're using is:
//    L0      0
//    L1    1   2
//    L2   3 4 5 6
//    L3  ...
// which in sequential memory looks like:
//    |0|1 2|3 4 5 6|...
// this means our mapping functions are:
//    left-child(i) = 2i + 1
//    rght-child(i) = 2i + 2
//    parent(i) = (i - 1) / 2
//    level(i) = 63 - countl_zero(i+1)

//ForceInl idx_t _Level( idx_t i ) { return _SIZEOF_IDX_T - 1 - _lzcnt_idx_t( i + 1 ); }
//ForceInl idx_t _Parent( idx_t i ) { return ( i - 1 ) / 2; }
//ForceInl idx_t _Left( idx_t i ) { return 2 * i + 1; }
//ForceInl idx_t _Right( idx_t i ) { return 2 * i + 2; }

#pragma warning(disable:4530)
RegisterTest([]()
{
  rng_lcg_t lcg;
  Init( lcg, 0x123456789 );

  auto makenode = [](int val) {
    node r;
    r.val = val;
    SetBalanced(&r);
    return r;
  };
  constant idx_t cNodes = 1000;
  std::vector<node> nodes;
  nodes.resize(cNodes);
  for (idx_t i = 0; i < cNodes; ++i) {
    nodes[i] = makenode(Rand32(lcg));
  }
  node* root = 0;
  for (idx_t i = 0; i < cNodes; ++i) {
    insert(&root, &nodes[i]);
  }

  // FUTURE: random order erase, not the same as insert.
  for (idx_t i = 0; i < cNodes; ++i) {
    erase(&root, &nodes[i]);
  }

  AssertCrash(root == 0);
});