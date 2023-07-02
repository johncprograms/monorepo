// build:console_x64_debug
// build:console_x64_optimized
// Copyright (c) John A. Carlos Jr., all rights reserved.

#include "os_mac.h"
#include "os_windows.h"

#define FINDLEAKS   0
#define WEAKINLINING   0
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
#include "cstr.h"
#include "ds_slice.h"
#include "ds_string.h"
#include "allocator_pagelist.h"
#include "ds_stack_resizeable_cont.h"
#include "ds_stack_nonresizeable_stack.h"
#include "ds_stack_nonresizeable.h"
#include "ds_stack_resizeable_pagelist.h"
#include "ds_pagetree.h"
#include "ds_list.h"
#include "ds_stack_cstyle.h"
#include "ds_hashset_cstyle.h"
#include "filesys.h"
#include "cstr_integer.h"
#include "cstr_float.h"
#include "timedate.h"
#include "thread_atomics.h"
#include "ds_mtqueue_srsw_nonresizeable.h"
#include "ds_mtqueue_srmw_nonresizeable.h"
#include "ds_mtqueue_mrsw_nonresizeable.h"
#include "ds_mtqueue_mrmw_nonresizeable.h"
#include "threading.h"
#define LOGGER_ENABLED   0
#include "logger.h"
#define PROF_ENABLED   0
#define PROF_ENABLED_AT_LAUNCH   0
#include "profile.h"
#include "rand.h"
#include "allocator_heap_findleaks.h"
#include "mainthread.h"
#include "text_parsing.h"
#include "ds_stack_resizeable_cont_addbacks.h"

void TestGradientDescent()
{

#if 0
  // Gradient descent
  // a_n+1 = a_n - learning_rate * grad(F(a_n))
  // where:
  //   a_i: R^n
  //   learning_rate: R > 0
  //   F: R^n -> R
  //   grad(F(x)) = [ d/dx1 F(x) ... d/dxn F(x) ]

  f32 a = -100.0f;
  auto F = [](f32 x) {
    return (x - 1.0f) * (x - 1.0f);
  };
  auto grad_F = [](f32 x) {
    return 2.0f * (x - 1.0f);
  };
  constexpr size_t E = 10000;
  constexpr auto learning_rate = 10.0f / E;
  FOR( size_t, e, 0, E ) {
    auto grad_F_a = grad_F(a);
    a = a - learning_rate * grad_F_a;
    if( (e * 100) % E == 0 ) {
      cout << "[" << e << "]" << a << endl;
    }
  }
  cout << "[last]" << a << endl;

#elif 0

  // Gradient descent with adaptive step-sizing.

  auto F = [](f32 x) {
    return (x - 1.0f) * (x - 1.0f);
  };
  auto grad_F = [](f32 x) {
    return 2.0f * (x - 1.0f);
  };
  constexpr f32 c_limits = 1e3f;
  std::minstd_rand generator(1234); // deterministic and well-defined for reproducibility
  std::uniform_real_distribution<f32> rand_weights{-c_limits, c_limits};
  constexpr size_t N = 1000;
  FOR( size_t, n, 0, N ) {
    auto a0 = rand_weights(generator);

    // Barzilai-Borwein method:
    // Keep track of the previous two points: n and n-1, and then use:
    // grad_diff = grad(F(a_n)) - grad(F(a_nm1))
    // learning_rate = |(a_n - a_nm1)^T grad_diff| / grad_diff^2
    // This gives us adaptive step sizing, which is absolutely critical for converging.

    constexpr size_t E = 10000;
    constexpr f32 c_default_learning_rate = 1e-5f;
    auto a_nm1 = a0;
    auto grad_nm1 = grad_F(a_nm1);
    auto a1 = a_nm1 - c_default_learning_rate * grad_nm1;
    auto a_n = a1;
    auto grad_n = grad_F(a_n);
    auto a = a0;
    size_t e = 1;
    for( ; e < E; ++e) {
      if( grad_n == 0.0f ) {
        // We converged already.
        break;
      }
      auto gard_diff = grad_n - grad_nm1;
      auto grad_diff2 = gard_diff * gard_diff;
      auto numerator = std::abs((a_n - a_nm1) * grad_diff);
      auto learning_rate = c_default_learning_rate;
      if( grad_diff2 != 0.0f && std::isfinite(grad_diff2) && std::isfinite(numerator) ) {
        learning_rate = numerator / grad_diff2;
      }
      auto a_np1 = a_n - learning_rate * grad_n;
      if( a_np1 == a_n ) {
        // No progress made this iteration.
        // We're likely as close as we can get with the given c_default_learning_rate.
        // i.e. learning_rate is really small, and grad_n is also really small.
        // So, let's try increasing the learning_rate for just this step to see if that helps.
        learning_rate = 0.5f;
        a_np1 = a_n - learning_rate * grad_n;
        if( a_np1 == a_n ) {
          // Even increasing the learning rate couldn't get us closer, so bail.
          // We're likely super close to a minimum anyways.
          break;
        }
      }
      auto grad_np1 = grad_F(a_np1);

      AssertCrash( a_np1 != a_n );

      a = a_np1;

      a_nm1 = a_n;
      grad_nm1 = grad_n;
      a_n = a_np1;
      grad_n = grad_np1;
    }

    cout << "gd iters required: " << e << endl;
    cout << "iter[" << n << "] result: " << a << endl;
  }

#elif 0

  // Multi-dimensional gradient descent.
  // F(x) = (x - 1)^2 + (y - 1)^2 + ...
  // grad_F(x) = [ 2(x-1) 2(y-1) ... ]
  // Note the minimum is by definition at x = 1v, the vector of all 1s.
  // constexpr size_t D = 300; // how many dimensions

  auto CopyArray = [](f32* dst, f32* src, size_t D)
  {
    TMove( dst, src, D );
  };
  auto F = [](f32* dst, f32* src, size_t D)
  {
    FOR( size_t, d, 0, D ) {
      auto src_d_m1 = src[d] - 1.0f;
      dst[d] = src_d_m1 * src_d_m1;
    }
  };
  auto grad_F = [](f32* dst, f32* src, size_t D)
  {
    FOR( size_t, d, 0, D ) {
      dst[d] = 2.0f * ( src[d] - 1.0f );
    }
  };
  auto dot = [](f32* a, f32* b, size_t D)
  {
    // TODO: kahan?
    f32 sum = 0.0f;
    FOR( size_t, d, 0, D ) {
      sum += a[d] * b[d];
    }
    return sum;
  };
  auto equal = [](f32* a, f32* b, size_t D)
  {
    FOR( size_t, d, 0, D ) {
      if( a[d] != b[d] ) return 0;
    }
    return 1;
  };
  auto print = [](f32* a, size_t D, const char* name)
  {
    cout << name << ":{";
    cout << std::fixed;
    FOR( size_t, d, 0, D ) {
      cout.width(12);
      cout << a[d];
      if( d + 1 != D ) {
        cout << ", ";
      }
    }
    cout << "}";
    cout << std::defaultfloat;
  };

  constexpr f32 c_limits = 1e5f;
  std::minstd_rand generator(1234); // deterministic and well-defined for reproducibility
  std::uniform_real_distribution<f32> rand_weights{-c_limits, c_limits};
  constexpr size_t N = 1000;
  FOR( size_t, n, 0, N ) {

    f32 a0[D];
    FOR( size_t, d, 0, D ) {
      a0[d] = rand_weights(generator);
    }

    // Barzilai-Borwein method:
    // Keep track of the previous two points: n and n-1, and then use:
    // grad_diff = grad(F(a_n)) - grad(F(a_nm1))
    // learning_rate = |(a_n - a_nm1)^T grad_diff| / grad_diff^2
    // This gives us adaptive step sizing, which is absolutely critical for converging.

    constexpr size_t E = 10000;
    constexpr f32 c_default_learning_rate = 1e-5f;
    f32 a_nm1[D];   CopyArray(a_nm1, AL(a0));
    f32 grad_nm1[D];   grad_F(grad_nm1, a_nm1, D);
    f32 a1[D];
    FOR( size_t, d, 0, D ) {
      a1[d] = a_nm1[d] - c_default_learning_rate * grad_nm1[d];
    }
    f32 a_n[D];   CopyArray(a_n, AL(a1));
    f32 grad_n[D];   grad_F(grad_n, a_n, D);
    f32 a[D];   CopyArray(a, AL(a0));
    size_t e = 1;
    for( ; e < E; ++e) {
      auto grad_n2 = dot(grad_n, grad_n, D);
      if( grad_n2 == 0.0f ) {
        // We converged already.
        break;
      }
      f32 grad_diff[D];
      FOR( size_t, d, 0, D ) {
        grad_diff[d] = grad_n[d] - grad_nm1[d];
      }
      auto grad_diff2 = dot(grad_diff, grad_diff, D);
      f32 numeratorv[D];
      FOR( size_t, d, 0, D ) {
        numeratorv[d] = std::abs((a_n[d] - a_nm1[d]) * grad_diff[d]);
      }
      auto numerator = std::sqrt(dot(numeratorv, numeratorv, D));
      auto learning_rate = c_default_learning_rate;
      // FUTURE: We can actually handle some cases just fine by continuing with c_default_learning_rate.
      // But in other cases, we cannot. Since I'm not sure how we can really tell, limiting input range is
      // really the best option. Hence we shouldn't have inf/nans here.
      AssertCrash( std::isfinite(grad_diff2) && std::isfinite(numerator) );
      if( grad_diff2 != 0.0f ) {
        learning_rate = numerator / grad_diff2;
      }
      f32 a_np1[D];
      FOR( size_t, d, 0, D ) {
        a_np1[d] = a_n[d] - learning_rate * grad_n[d];
      }
      if( equal(a_np1, a_n, D) ) {
        // No progress made this iteration.
        // We're likely as close as we can get with the given c_default_learning_rate.
        // i.e. learning_rate is really small, and grad_n is also really small.
        // So, let's try increasing the learning_rate for just this step to see if that helps.
        learning_rate = 0.5f;
        FOR( size_t, d, 0, D ) {
          a_np1[d] = a_n[d] - learning_rate * grad_n[d];
        }
        if( equal(a_np1, a_n, D) ) {
          // Even increasing the learning rate couldn't get us closer, so bail.
          // We're likely super close to a minimum anyways.
          break;
        }
      }
      f32 grad_np1[D];   grad_F(grad_np1, a_np1, D);

      AssertCrash( !equal(a_np1, a_n, D) );

      CopyArray(a, a_np1, D);

      CopyArray(a_nm1, a_n, D);
      CopyArray(grad_nm1, grad_n, D);
      CopyArray(a_n, a_np1, D);
      CopyArray(grad_n, grad_np1, D);
    }
    cout << "gd iters required: " << e << endl;
    f32 all1s[D];
    FOR( size_t, d, 0, D ) {
      all1s[d] = 1.0f;
    }
    if( !equal(all1s, a, D) ) {
      cout << "case: " << n << " ";
      print(AL(a0), "a0");
      cout << " ";
      print(AL(a), "result");
      cout << endl;
    }
  }

#elif 0

  // Neuron definition:
  // y = Phi(sum(a_k * x_k))
  // where a_k are the weight parameters of the neuron
  // Phi is the activation function: R -> R.
  // x_k are the input values fed to the neuron
  // y is the neuron's output value.
  // We're using the loss function: L(y,z) = (z - y)^2 which is the usual sum of squared error.

  auto CopyArray = [](f32* dst, f32* src, size_t D)
  {
    TMove( dst, src, D );
  };
  auto dot = [](f32* a, f32* b, size_t D)
  {
    // TODO: kahan?
    f32 sum = 0.0f;
    FOR( size_t, d, 0, D ) {
      sum += a[d] * b[d];
    }
    return sum;
  };
  auto equal = [](f32* a, f32* b, size_t D)
  {
    FOR( size_t, d, 0, D ) {
      if( a[d] != b[d] ) return 0;
    }
    return 1;
  };
  auto print = [](f32* a, size_t D, const char* name)
  {
    cout << name << ":{";
    cout << std::fixed;
    FOR( size_t, d, 0, D ) {
      cout.width(12);
      cout << a[d];
      if( d + 1 != D ) {
        cout << ", ";
      }
    }
    cout << "}";
    cout << std::defaultfloat;
  };

  constexpr f32 c_initial_weight_bounds = 1e3f;
  constexpr f32 c_weight_bounds = 1e5f;
  std::minstd_rand generator(1234); // deterministic and well-defined for reproducibility
  std::uniform_real_distribution<f32> rand_weights{-c_limits, c_limits};
  std::uniform_int_distribution<size_t> rand_size{};

  constexpr size_t D = 3;
  // TODO: it seems impossible to learn 0s, when the derivative of the loss only includes a single
  //   coordinate and it's 0. So we want a constant 1 input per coordinate I think, not just one per vector.
  //   Or, we need to change to a loss function whose derivative includes multiple coordinates.

  // By definition, we need X[D-1] = 1 for the constant bias.
  f32 training_Xs[] = {
    10, 10, 1,
    99, 10, 1,
    10, 99, 1,
    99, 99, 1,
  };
  f32 training_Zs[] = {
    -1,
    -1,
    1,
    1,
  };
  static_assert(_countof(training_Xs) == D * _countof(training_Zs));
  constexpr size_t T = _countof(training_Zs);

  // Init weights to random values.
  f32 a0[D];
  FOR( size_t, d, 0, D ) {
    a0[d] = rand_weights(generator);
  }
  f32 a[D];   CopyArray(a, AL(a0));

  // We're using F(y) = L(y,z)
  // Note that F is particular to a given training sample tuple, (x,z).
  // So our gradient field, grad(F(x)), is different for each training sample.
  // That poses a problem for our multi-point method for adaptive step sizing!
  // Options:
  // 1. converge on one training sample before moving to the next
  // 2. move to a new training sample on every gradient step
  //   2.1. give up adaptive step sizing with our method, and use some other method
  //   2.2. pretend the multi-points are from the same field even though they're not.
  // 3. some mix of the above, e.g. doing K gradient steps per training sample, stopping early, and moving on.
  // 4. use F = sum( L(y_i,z_i), i=1..T ). That is, use the total loss, and not per-sample loss. And compute
  //    the gradient of that.
  //
  // I'm liking option 4. What does that look like?
  // L(w) = sum[ (z(x) - phi(x,w))^2, x in T ]
  // d/dw_k[ L(w) ] = -2 sum[ (z(x) - phi(x,w)) d/dw_k[ phi(x,w) ], x in T ]
  // So F = L, and grad(F) = [ d/dw1[L] ... d/dwD[L] ]

  auto phi = [](const f32* x, f32* w, size_t D)
  {
    // TODO: kahan?
    f32 sum = 0.0f;
    FOR( size_t, d, 0, D ) {
      sum += a[d] * b[d];
    }
    return sum;
  };
  auto ddw_k_phi = [](const f32* x, f32* w, size_t k, size_t D)
  {
    return x[k];
  };

  auto F_k = [training_Xs, training_Zs, T, phi](f32* w, size_t t, size_t D)
  {
    auto x = training_Xs + t * D;
    auto z = tarining_Zs[t];
    auto term = z - phi(x, w, D);
    return term * term;
  };
  auto F = [training_Xs, training_Zs, T, phi](f32* w, size_t D)
  {
    f32 sum = 0.0f;
    FOR( size_t, t, 0, T ) {
      auto x = training_Xs + t * D;
      auto z = tarining_Zs[t];
      auto term = z - phi(x, w, D);
      sum += term & term;
    }
    return sum;
  };
  auto grad_F = [](f32* grad, f32* w, size_t D)
  {
    FOR( size_t, d, 0, D ) {
      grad[d] = 0.0f;
    }
    FOR( size_t, k, 0, D ) {
      f32 ddw_k_L = 0.0f;
      FOR( size_t, t, 0, T ) {
        auto x = training_Xs + t * D;
        auto z = tarining_Zs[t];
        auto term = ( z - phi(x, w, D) ) * ddw_k_phi(x, w, k, D);
        ddw_k_L += term;
      }
      ddw_k_L *= -2.0f;
      grad[k] = ddw_k_L;
    }
  };

  // Barzilai-Borwein method:
  // Keep track of the previous two points: n and n-1, and then use:
  // grad_diff = grad(F(a_n)) - grad(F(a_nm1))
  // learning_rate = |(a_n - a_nm1)^T grad_diff| / grad_diff^2
  // This gives us adaptive step sizing, which is absolutely critical for converging.

  constexpr size_t E = 10000;
  constexpr f32 c_default_learning_rate = 1e-5f;
  f32 a_nm1[D];   CopyArray(a_nm1, AL(a0));
  f32 grad_nm1[D];   grad_F(grad_nm1, a_nm1, D);
  f32 a1[D];
  FOR( size_t, d, 0, D ) {
    a1[d] = a_nm1[d] - c_default_learning_rate * grad_nm1[d];
  }
  f32 a_n[D];   CopyArray(a_n, AL(a1));
  f32 grad_n[D];   grad_F(grad_n, a_n, D);
  size_t e = 1;
  for( ; e < E; ++e) {
    if( e <= 20 || (e & 10) % E == 0 ) {
      cout << "Iter[" << e << "] ";
      print(a, D, "W");
      cout << " Ltotal: " << F(a, D);
      cout << endl;
    }

    auto grad_n2 = dot(grad_n, grad_n, D);
    if( grad_n2 == 0.0f ) {
      // We converged already.
      break;
    }
    f32 grad_diff[D];
    FOR( size_t, d, 0, D ) {
      grad_diff[d] = grad_n[d] - grad_nm1[d];
    }
    auto grad_diff2 = dot(grad_diff, grad_diff, D);
    f32 numerator2 = 0.0f;
    FOR( size_t, d, 0, D ) {
      auto term = std::abs((a_n[d] - a_nm1[d]) * grad_diff[d]);
      numerator2 += term * term;
    }
    auto numerator = std::sqrt(numerator2);
    auto learning_rate = c_default_learning_rate;
    // FUTURE: We can actually handle some cases just fine by continuing with c_default_learning_rate.
    // But in other cases, we cannot. Since I'm not sure how we can really tell, limiting input range is
    // really the best option. Hence we shouldn't have inf/nans here.
    AssertCrash( std::isfinite(grad_diff2) && std::isfinite(numerator) );
    if( grad_diff2 != 0.0f ) {
      learning_rate = numerator / grad_diff2;
    }
    f32 a_np1[D];
    FOR( size_t, d, 0, D ) {
      a_np1[d] = a_n[d] - learning_rate * grad_n[d];
    }
    if( equal(a_np1, a_n, D) ) {
      // No progress made this iteration.
      // We're likely as close as we can get with the given c_default_learning_rate.
      // i.e. learning_rate is really small, and grad_n is also really small.
      // So, let's try increasing the learning_rate for just this step to see if that helps.
      learning_rate = 0.5f;
      FOR( size_t, d, 0, D ) {
        a_np1[d] = a_n[d] - learning_rate * grad_n[d];
      }
      if( equal(a_np1, a_n, D) ) {
        // Even increasing the learning rate couldn't get us closer, so bail.
        // We're likely super close to a minimum anyways.
        break;
      }
    }
    f32 grad_np1[D];   grad_F(grad_np1, a_np1, D);

    AssertCrash( !equal(a_np1, a_n, D) );

    CopyArray(a, a_np1, D);

    CopyArray(a_nm1, a_n, D);
    CopyArray(grad_nm1, grad_n, D);
    CopyArray(a_n, a_np1, D);
    CopyArray(grad_n, grad_np1, D);
  }
  cout << "gd iters required: " << e << endl;
  print(a, D, "W");
  cout << endl;
  FOR( size_t, t, 0, T ) {
    auto x = training_Xs + t * D;
    auto z = tarining_Zs[t];
    cout << " [" << t << "]" << " Prediction: " << setw(8) << phi(x, a, D)
      << " Actual: " << setw(8) << z << ": " << F_k(a, t, D) << endl;
  }
  cout << "Ltotal: " << F(a, D) << endl;

#endif


// graph layer representation:
// nodes[layer][# nodes in layer]
// forw_edges[layer][j][] adjacency-list
// back_edges[layer][j][] adjacency-list
//
// note that for bidirectional edges:
// forw_edges[layer][j][k] = back_edges[layer+1][k][j]
//
// QUESTION: is this layer splitting a good idea in general?
// dynamic child/parent queues/stacks are just as functional.
// - the BFS queues grow to O(# nodes in a layer).
// - the DFS stacks might be even worse for fully connected networks, O(# nodes in graph).
// - layer splitting uses O(# layers) for the layer indirection table.
// since we're doing BFS + reverse BFS, option 2 is out.
// interesting trade-off between options 1 and 3. basically which dimension is bigger/smaller.
// there's also the read/write nature of option 1, vs. write-once/read-many nature of option 3.
// for similar dimension sizes it probably doesn't matter which we use.
//
// IDEA: bidirectional edge indexing:
// given [i][j], always take [u][v] where:
// u = min(i,j)
// v = max(i,j)
// that way u,v are always sorted s.t. u <= v.
// note it'll also work for self loops (u==v), since those are symmetric.
// this only works if we have matrix indexing; doesn't work right away for adjacency-list.
// i should really look into efficient sparse matrix storage. that would solve lots of graph storage problems.
//
// in our layered structure, we can take advantage of layer order to use this.
// we can elide storage of back_edges, and look up in the previous layer's forw_edges instead.
// note that means layer=0 is the input layer, since back_edges[0] doesn't exist.
// similarly, layer=num_layers + 1 is the output layer. we don't need to store anything for this though.

struct
layer_t
{
  idx_t num_nodes;

  // per-node arrays:
  f32* o; // aka phi
  f32* m; // aka dPhi
  f32* T; // dynamic programming table

  // adjacency-list of node indices in layer-1
  u16* forw_offsets; // length = num_nodes
  u16* forw_degrees; // length = num_nodes
  u16* forw_nodes; // length = sum forw_degrees[i] for i in [0, num_nodes)
  f32* forw_O;  // index-matched to forw_nodes
  f32* forw_dL; // index-matched to forw_nodes

  // adjacency-list of node indices in layer+1
  u16* back_offsets; // length = num_nodes
  u16* back_degrees; // length = num_nodes
  u16* back_nodes; // length = sum forw_degrees[i] for i in [0, num_nodes)
  f32* back_O; // TODO: switch to sparse matrix and use bidirectional indexing to elide this duplicate.
  // note we don't need to store back_dL, it's accessed unidirectionally.
};

// TODO: graph construction methods and formats

int
Main( stack_resizeable_cont_t<slice_t>& args )
{
  // num_layers: how many neurons deep is our network
  // output_layer = num_layers, synthetic / virtual layer that's not really a neuron, just used for packing.
  // num_parent_nodes(layer, k) =
  //   |x| for layer==0
  //   # of connected parents to node k in layer, else.
  // num_child_nodes(layer, j) =
  //
  // o_layer_j[num_layers + 1][]: per-node output
  //   j identifies node in layer+1
  //   layer==0 is the input data
  // O_layer_j_k[num_layers][][]: per-edge weight
  //   j identifies node in layer-1
  //   k identifies node in layer
  // For training:
  // T_layer_j[num_layers + 1][]: per-node dynamic programming table
  //   j identifies node in layer
  //   layer==num_layer is the base case of the final output.
  // m_layer_j[num_layers][]: per-node evaluated derivative of phi // TODO: rename dPhi
  //   j identifies node in layer
  // dL_layer_j_k[num_layers][][]: per-edge derivative of loss w.r.t. the edge weight.

  // Forward:
  o_layer_j[0] = x
  for layer in [0, num_layers)
    O_layer_j_k[layer][j][k] = random non-zero; for all j,k edges.
  for layer in [1, num_layers + 1)
    for k in [0, num_nodes(layer))
      auto dot = 0
      for j in [0, num_parent_nodes(layer, k))
        dot += o_layer_j[layer - 1][j] * O_layer_j_k[layer][j][k]
      o_layer_j[layer][k] = phi(dot)
      m_layer_j[layer][k] = dPhi(dot)

  y = o_layer_j[num_layers]

  // Backward:
  for j in num_nodes(output_layer)
    T_layer_j[output_layer][j] = -2 * ( z[j] - y[j] )
  for layer in reverse [0, num_layers)
    for j in [0, num_nodes(layer))
      auto dot = 0
      if layer + 1 == num_layers
        for k in [0, num_child_nodes(layer, j))
          dot += T_layer_j[layer + 1][k]
      else
        for k in [0, num_child_nodes(layer, j))
          dot += O_layer_j_k[layer + 1][j][k] * T_layer_j[layer + 1][k]
      auto dPhi = m_layer_j[layer][j]
      auto T = dot * dPhi
      T_layer_j[layer][j] = T
      for k in [0, num_child_nodes(layer, j))
        dL_layer_j_k[layer + 1][j][k] += T * o_layer_j[layer][k]





  return 0;
}



int
main( int argc, char** argv )
{
  MainInit();

  stack_resizeable_cont_t<slice_t> args;
  Alloc( args, argc - 1 );
  Fori( int, i, 1, argc ) {
    auto arg = AddBack( args );
    arg->mem = Cast( u8*, argv[i] );
    arg->len = CstrLength( arg->mem );
  }
  int r = Main( args );
  Free( args );

//  system( "pause" );
  MainKill();
  return r;
}
