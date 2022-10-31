// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// the great simplex algorithm!
// this is the maximization form.
// given M linear constraint functions and one linear objective function over N variables,
// this performs an exponential-time search for the variable-assignments that maximize the objective value.
//
// note that this assumes non-negativity constraints as well, so:
//   x0 >= 0
//   x1 >= 0
//   ...
//   xN-1 >= 0
//
// slack_matrix is a row-major table of the form:
//   [   c(0,0),   c(1,0), ...,   c(N,0) ]
//   [                  ...              ]
//   [ c(0,M-1), c(1,M-1), ..., c(N,M-1) ]
//   [   o(0,M),   o(1,M), ...,   o(N,M) ]
//
// note that the matrix is (N+1) columns by (M+1) rows.
//
// the first M rows of c(i,j) values are linear constraint functions with the following definition:
//   c(0,j) * x0 + c(1,j) * x1 + ... + c(N,j) <= 0
// note that c(N,j) is the fixed offset, and c(i,j) for all other i's are coefficients.
//
// the last row of o(i,M) values is the linear objective function with the following definition:
//   objective = o(0,M) * x0 + o(1,M) * x1 + ... + o(N,M)
// note that o(N,M) is the fixed offset, and o(i,M) for all other i's are coefficients.
//
f32
SimplexMaximize(
  idx_t N,
  idx_t M,
  f32* slack_matrix
  )
{
  auto stride = N + 1;
  auto obj = slack_matrix + stride * M;
  // TODO: choose the decision variable according to some rule?
  //   maybe the biggest min_upper_bound?
  // TODO: infinite loop detection?
LOOP:
  For( chosen_decision, 0, N ) {
    if( obj[chosen_decision] <= 0.0f ) {
      continue;
    }
    //
    // we can increase obj by increasing the chosen_decision variable.
    // go through the slack formulas, finding bounds for our variable increase.
    // we want the slack formula that's limiting our increase, so we don't overstep.
    //   slackI_value = slackI[N] + slackI[chosen_decision] * x >= 0
    //   x <= -slackI[N] / slackI[chosen_decision];
    //
    f32 min_upper_bound = MAX_f32;
    auto chosen_slack = M;
    For( j, 0, M ) {
      auto upper_bound = -slack_matrix[ stride * j + N ] / slack_matrix[ stride * j + chosen_decision ];
      if( upper_bound <= 0.0f ) continue;
      if( upper_bound < min_upper_bound ) {
        min_upper_bound = upper_bound;
        chosen_slack = j;
      }
    }
    if( chosen_slack == M ) {
      continue;
    }
    //
    // move the chosen_decision var to the left side of the chosen_slack formula.
    // and move the chosen_slack var to the right side.
    // note we divide out the coefficient, so the new LHS has coeff=1.
    //   slackI_value = slackI[0] * decision_vars[0] + ... + slackI[N];
    //
    auto factor = -1.0f / slack_matrix[ stride * chosen_slack + chosen_decision ];
    slack_matrix[ stride * chosen_slack + chosen_decision ] = -1.0f;
    For( i, 0, N + 1 ) {
      slack_matrix[ stride * chosen_slack + i ] *= factor;
    }
    //
    // now use that new slack formula to expand into the rest of the slack formulas.
    // note we want M+1 here, to also expand into the objective formula.
    //
    For( j, 0, M + 1 ) {
      if( j == chosen_slack ) continue;
      auto factor_j = slack_matrix[ stride * j + chosen_decision ];
      slack_matrix[ stride * j + chosen_decision ] = 0.0f;
      For( i, 0, N + 1 ) {
        slack_matrix[ stride * j + i ] += factor_j * slack_matrix[ stride * chosen_slack + i ];
      }
    }
    goto LOOP;
  }
  return slack_matrix[ stride * M + N ];
}


void
TestSimplex()
{
  f32 slack_matrix[4*4] = {
    -2.0f, -3.0f, -1.0f, 5.0f,
    -4.0f, -1.0f, -2.0f, 11.0f,
    -3.0f, -4.0f, -2.0f, 8.0f,
    5.0f, 4.0f, 3.0f, 0.0f // objective
  };
  auto z = SimplexMaximize( 3, 3, slack_matrix );
  AssertCrash( z == 13.0f );
}


#if 0
  idx_t N = 3;
  // maximize objective
//  f32 obj[] = { 5.0f, 4.0f, 3.0f, 0.0f };
  //               x1    x2    x3   const
  f32 con1[] = { 2.0f, 3.0f, 1.0f, -5.0f }; // <= 0
  f32 con2[] = { 4.0f, 1.0f, 2.0f, -11.0f }; // <= 0
  f32 con3[] = { 3.0f, 4.0f, 2.0f, -8.0f }; // <= 0
  // implicit x0, x1, x2 >= 0.0f
  f32 slack[4][4] = {
    { -2.0f, -3.0f, -1.0f, 5.0f },
    { -4.0f, -1.0f, -2.0f, 11.0f },
    { -3.0f, -4.0f, -2.0f, 8.0f },
    { 5.0f, 4.0f, 3.0f, 0.0f } // objective
    };
  idx_t M = 3;
  auto obj = slack[M];
  // implicit slack0,1,2 >= 0.0f
//  f32 decision_vars[] = { 0.0f, 0.0f, 0.0f };
//  f32 slack_vars[3];
//  slack_vars[0] = Eval( slack0, decision_vars, N );
//  slack_vars[1] = Eval( slack1, decision_vars, N );
//  slack_vars[2] = Eval( slack2, decision_vars, N );
//  auto z = Eval( obj, decision_vars, N );
  // TODO: choose the decision variable according to some rule?
  //   maybe the biggest min_upper_bound ?
LOOP:
  For( chosen_decision, 0, N ) {
    if( obj[chosen_decision] > 0.0f ) {
      // we can increase obj by increasing decision_vars[0]
      // note that by construction, decision_vars is all zeros, making our math easy.
      // go through the slack formulas, finding bounds for our variable increase.
      // slackI_value = slackI[N] + slackI[chosen_decision] * x >= 0
      // x <= -slackI[N] / slackI[chosen_decision];
//      f32 upper_bounds[] = { 0.0f, 0.0f, 0.0f };
//      upper_bounds[0] = -slack[0][N] / slack[0][chosen_decision];
//      upper_bounds[1] = -slack[1][N] / slack[1][chosen_decision];
//      upper_bounds[2] = -slack[2][N] / slack[2][chosen_decision];
      f32 min_upper_bound = MAX_f32;
      auto chosen_slack = M;
      For( j, 0, M ) {
        auto upper_bound = -slack[j][N] / slack[j][chosen_decision];
        if( upper_bound <= 0.0f ) continue;
        if( upper_bound < min_upper_bound ) {
          min_upper_bound = upper_bound;
          chosen_slack = j;
        }
      }
      if( chosen_slack < M ) {
        // we can increase.
//        decision_vars[chosen_decision] = min_upper_bound;
//        auto new_z = Eval( obj, decision_vars, N );
        // slackI_value = Eval( slackI, decision_vars, N );
        // slackI_value = slackI[0] * decision_vars[0] + ... + slackI[N];

        // move the chosen_decision var to the left side of the chosen slack formula.
        // and move the chosen slack var to the right side.
        // note we divide out the coefficient, so the new LHS has coeff=1.
        auto factor = -1.0f / slack[ chosen_slack ][ chosen_decision ];
        slack[ chosen_slack ][ chosen_decision ] = -1.0f;
        For( i, 0, N + 1 ) {
          slack[ chosen_slack ][i] *= factor;
        }
        // now use that new slack formula to expand into the rest of the slack formulas.
        // note we want M+1 here, to also expand into the objective formula.
        For( j, 0, M + 1 ) {
          if( j == chosen_slack ) continue;
          auto factor_j = slack[j][ chosen_decision ];
          slack[j][ chosen_decision ] = 0.0f;
          For( i, 0, N + 1 ) {
            slack[j][i] += factor_j * slack[ chosen_slack ][i];
          }
        }
        goto LOOP;
      }
    }
  }
  auto z = slack[M][N];
  AssertCrash( z == 13.0f );
#endif
