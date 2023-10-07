// Copyright (c) John A. Carlos Jr., all rights reserved.

Inl f32
Eval(
  f32* coeffs,
  f32* vars,
  idx_t N
  )
{
  auto r = coeffs[N];
  For( i, 0, N ) {
    r += vars[i] * coeffs[i];
  }
  return r;
}


Inl f32
CalcAdot(
  f32* A,
  u32 N,
  f32 t
  )
{
  AssertCrash( N );
  kahansum32_t sum = {};
  Fori( u32, i, 0, N - 1 ) {
    f32 term = ( i + 1 ) * A[ i + 1 ] * Pow32( t, Cast( f32, i ) );
    Add( sum, term );
  }
  return sum.sum;
}

Inl void
CalcAdot(
  f32* A,
  u32 N,
  f32* R
  )
{
  AssertCrash( N );
  Fori( u32, i, 0, N - 1 ) {
    R[i] = ( i + 1 ) * A[ i + 1 ];
  }
}

Inl f32
CalcA(
  f32* A,
  u32 N,
  f32 t
  )
{
  AssertCrash( N );
  kahansum32_t sum = {};
  sum.sum = A[0];
  Fori( u32, i, 1, N ) {
    f32 term = A[i] * Pow32( t, Cast( f32, i ) );
    Add( sum, term );
  }
  return sum.sum;
}

Inl void
CalcAsquared(
  f32* A,
  u32 N,
  f32* R
  )
{
  AssertCrash( A != R );
  AssertCrash( N );
  Fori( u32, i, 0, N ) {
    R[i] = 0;
  }
  Fori( u32, i, 0, N ) {
    Fori( u32, j, 0, N ) {
      if( i + j >= N ) {
        break; // drop higher order terms
      }
      R[i+j] += A[i] * A[j];
    }
  }
}

Inl void
CalcAtimesB(
  f32* A,
  u32 N,
  f32* B,
  f32* R
  )
{
  AssertCrash( A != R );
  AssertCrash( B != R );
  AssertCrash( N );
  Fori( u32, i, 0, N ) {
    R[i] = 0;
  }
  Fori( u32, i, 0, N ) {
    Fori( u32, j, 0, N ) {
      if( i + j >= N ) {
        break; // drop higher order terms
      }
      R[i+j] += A[i] * B[j];
    }
  }
}

Inl void
CalcAofA(
  f32* A,
  u32 N,
  f32* R,
  f32* T0,
  f32* T1
  )
{
  AssertCrash( A != R );
  AssertCrash( N );

  // T0 is A^i, starting at i=1
  Fori( u32, i, 0, N ) {
    T0[i] = A[i];
  }

  R[0] = A[0]; // no multiplies necessary for this, see below loop starting at i=1
  Fori( u32, i, 1, N ) {
    R[i] = 0;
  }

  Fori( u32, i, 1, N ) {
    // compute A[i] * A^i, and add to R

    Fori( u32, j, 0, N ) {
      T1[j] = A[i] * T0[j];
    }

    // add A[i] * A^i to R
    Fori( u32, j, 0, N ) {
      R[j] += T1[j];
    }

    // update T0 to be A^i for the next i.
    CalcAtimesB( T0, N, A, T1 );
    Fori( u32, j, 0, N ) { // PERF: unroll outer loop once and swap T0/T1 for second "iter" unrolled ?
      T0[j] = T1[j];
    }
  }
}

// calculates the truncated polynomial c * A( A( t / c ) )
Inl void
CalcCtimesAofAofToverC(
  f32* A,
  u32 N,
  f32* R,
  f32* T0,
  f32* T1,
  f32 C
  )
{
  CalcAofA( A, N, R, T0, T1 );

  // note that we can do a change of variables for A( A( t ) ) -> A( A( t / c ) ), so we can easily do this after figuring out A( A( t ) )
  f32 recC = 1 / C;
  f32 recCtoI = 1 / C;
  Fori( u32, i, 1, N ) {
    R[i] *= recCtoI;
    recCtoI *= recC;
  }

  // now multiply by C
  Fori( u32, i, 0, N ) {
    R[i] *= C;
  }
}

Inl f32
CalcAofA(
  f32* A,
  u32 N,
  f32 t
  )
{
  return CalcA( A, N, CalcA( A, N, t ) );
}

Inl f32
CalcTotalErr(
  f32* t_tests,
  u32 t_tests_len,
  f32* A,
  u32 N
  )
{
  kahansum32_t total_err = {};
  For( i, 0, t_tests_len ) {
    auto t = t_tests[i];
    auto AofA = CalcAofA( A, N, t );
    auto Adot = CalcAdot( A, N, t );
    auto err = Adot - AofA;
    Add( total_err, ABS( err ) );
  }
  return total_err.sum / t_tests_len;
}

Inl f32
CalcTotalErrRenormalization(
  f32* t_tests,
  u32 t_tests_len,
  f32* A,
  u32 N
  )
{
  kahansum32_t total_err = {};
  For( i, 0, t_tests_len ) {
    auto t = t_tests[i];
    // we're storing alpha as A[ N - 1 ]
    auto alpha = A[ N - 1 ];
    auto FofXoverAlpha = CalcA( A, N - 1, t / alpha );
    auto F = CalcA( A, N - 1, t );
    auto FofFofXoverAlpha = CalcA( A, N - 1, FofXoverAlpha );
    auto err = FofFofXoverAlpha - F;
    Add( total_err, ABS( err ) );
  }
  return total_err.sum / t_tests_len;
}

Inl f32
CalcTotalErrRenormalization(
  f32* A,
  u32 N,
  f32* T0,
  f32* T1,
  f32* T2
  )
{
  AssertCrash( N >= 2 );

  auto alpha = A[N-1];
  CalcCtimesAofAofToverC( A, N-1, T0, T1, T2, alpha ); // T0 = alpha * A( A( t / alpha ) )

#if 0
  kahansum32_t total_err = {};
  For( i, 0, N-1 ) {
    auto err = T0[i] - A[i];
    Add( total_err, ABS( err ) );
  }
  return total_err.sum;

#else
  CalcAdot( A, N-1, T1 ); // T1 = Adot( t ), which is N-2 in length
  kahansum32_t total_err = {};
  For( i, 0, N-2 ) {
    auto err = T0[i] - T1[i];
    Add( total_err, ABS( err ) );
  }
  return total_err.sum;

#endif

}

Inl void
PrintArray(
  f32* A,
  u32 N
  )
{
  printf( "array: " );
  Fori( u32, i, 0, N ) {
    printf( "%f ", A[i] );
  }
  printf( "\n" );
}


Inl vec2<f32>
SpringEqns(
  f32 t,
  f32 a,
  f32 n,
  f32 A,
  f32 B
  )
{
  // x(t) = a exp( -n ) t^n exp( -A t ) exp( +-sqrt( n + t^2 ( A^2 - B ) ) )
  // x(t) = a t^n exp( -n - A t +- sqrt( n + t^2 ( A^2 - B ) ) )
  // x(t) = a exp( b t ) t^n, with b t = -n - A t +- sqrt( n + t^2 ( A^2 - B ) )
  // x'(t) = a exp( b t ) [ n t^( n - 1 ) + b t^n ]
  // x'(t) = a exp( b t ) t^n [ n / t + b ]
  // x'(t) = x(t) [ n / t + b ]
  // x'(t) = x(t) [ n + b t ] / t
  // x''(t) = a exp( b t ) [ n ( n - 1 ) t^( n - 2 ) + 2 b n t^( n - 1 ) + b^2 t^n ]
  // x''(t) = a exp( b t ) t^n [ n ( n - 1 ) / t^2 + 2 b n / t + b^2 ]
  // x''(t) = x(t) [ n ( n - 1 ) / t^2 + 2 b n / t + b^2 ]
  // x''(t) = x(t) [ n ( n - 1 ) / t^2 + 2 b t n / t^2 + ( b t )^2 / t^2 ]
  // x''(t) = x(t) [ n ( n - 1 ) + 2 b t n + ( b t )^2 ] / t^2
  auto disc = n + t * t * ( A * A - B );
  AssertCrash( disc >= 0 );
  auto sqroot = Sqrt32( disc );
  auto leadfac = a * Pow32( t, n );
  auto leadexp = -n - A * t;
  auto bt0 = leadexp + sqroot;
  auto bt1 = leadexp - sqroot;
  auto x0 = leadfac * Exp32( bt0 );
  auto x1 = leadfac * Exp32( bt1 );
  auto xp0 = x0 * ( n + bt0 ) / t;
  auto xp1 = x1 * ( n + bt1 ) / t;
  auto xpp0 = x0 * ( n * ( n - 1 ) + 2 * n * bt0 + bt0 * bt0 ) / ( t * t );
  auto xpp1 = x1 * ( n * ( n - 1 ) + 2 * n * bt1 + bt1 * bt1 ) / ( t * t );

  auto soln0 = xpp0 + 2 * A * xp0 + B * x0;
  auto soln1 = xpp1 + 2 * A * xp1 + B * x1;
//  printf( "%f, %f\n", soln0, soln1 );

  return _vec2( soln0, soln1 );
}

Inl void
CalcJunk()
{
  For( i, 1, 10000 ) {
    constant f32 mass = 1.0f;
    constant f32 spring_k = 1000.0f;
    static f32 friction_k = 2.2f * Sqrt32( mass * spring_k ); // 2 * Sqrt( m * k ) is critical damping, but we want a little overdamping.

    auto t = Cast( f32, i );
    f32 a = 1.0f;
    f32 n = 0.0f;
    f32 B = -spring_k;
    f32 A = 0.5f * friction_k;
    auto r = SpringEqns( t, a, n, A, B );
  }

  {
    constant u32 n = 10000;
    stack_resizeable_cont_t<slice_t> lines;
    Alloc( lines, n + 1 );
    kahansum32_t x = {};
    constant idx_t m = 100;
    For( iter, 0, m ) {
      lines.len = 0;
      auto lines_mem = AddBack( lines, n );
      FORLEN( line, i, lines )
        line->mem = Cast( u8*, lines_mem + i );
        line->len = i + 1;
      }
      auto t0 = TimeTSC();
//      *AddAt( lines, 0 ) = { 0, 0 };
      *AddBack( lines ) = { 0, 0 };
      auto t1 = TimeTSC();
      auto dt = TimeSecFromTSC32( t1 - t0 );
      Add( x, dt );
    }
    x.sum /= m;
    Free( lines );
  }

  // for 100K slice_t's:
  // addback and write of a slice_t is ~70 nanoseconds.
  // what about addat(0) ?
  // that's ~50 microseconds.
  // still not too bad.

  // for 1M slice_t's:
  // addat(0) is ~1.3 milliseconds. pretty terrible.
  // addback is ~20 microseconds.




  {
    constant u32 n = 513;
    u32 x[n];
    u32 y[n];
    Fori( u32, i, 0, n ) {
      x[i] = i;
      y[i] = x[i] & ( x[i] - 1 );
      y[i] = __popcnt( i );
      y[i] = __lzcnt( i );
      y[i] = 32 - __lzcnt( i );
      x[i] = 1u << ( 32u - __lzcnt( i ) );
      x[i] = 1u << ( 32u - __lzcnt( i - 1 ) );
      y[i] = Cast( u32, -1 ) >> __lzcnt( i );
      y[i] = 1u + ( Cast( u32, -1 ) >> __lzcnt( i - 1 ) );
      auto t = i - 1;
      y[i] = ( t | (t >> 1) | (t >> 2) | (t >> 4) | (t >> 8) | (t >> 16) ) + 1;
      y[i] = 1u << ( 32u - __lzcnt( i >> 1 ) );
    }
    x[0] = 0;
  }

  // well we know the renormalization functional equation does have a convergent alpha,
  // f( t ) = alpha * f( f( t / alpha ) )
  // so maybe we should try that as a test of our solver

  // well it looks like our solver for renormalization converges to f=0, which isn't all that helpful.
  // we need to add constraints to avoid the null soln.

  // unfortunately even adding constraints isn't quite enough; we just pick the closest thing to the edge of the constraints.
  // maybe we have multiple local minima, but our solver is always finding the global minimum.
  // finding the global min is kind of the point of the metropolis algorithm; perhaps we need a different solver?
  // we could cluster/sort successful transitions or top results, to get an idea of good values in the space.
  //
  // we could do a gradient descent algorithm by looking a tiny step forward in each A[i]+tinystep, computing
  // the difference in error for each i, and then committing to a direction proportional to the difference in error.
  // or, pick a random i, and do the tiny step forward/backwards in the direction of the difference in error.
  //
  // is there some way we can visualize the error as a function of A[i]s?
  // if we pick one i to look at, then yeah it's trivial.

  // visualizing total_err vs. A[2] and A[3] for starting A = { 1, 0, -1.366, -2.732 }, it looks like:
  // A[2] has one min at 0
  // A[3] has one min at -0.1875
  // so our metric doesn't optimize for what we want.
  // it finds the null soln, but that's not helpful.

  // maybe if we change our metric to only sum the coefficient errors, effectively ignoring t?
  // that would avoid "t close to 0" having such an influence; maybe enough to expose other solns?
  // doing this effectively requires a polynomial evaluator system, so we can compute coefficients of a(a(t)) and f(f(t/alpha))
  //
  // maybe we can just divide terms in our functional eqn by t, to achieve the same effect?
  // nope that didn't work. looks like we'll need that polynomial system to match coefficients...

  // matching coefficients appears to converge to multiple solutions, which is good news.
  // if we keep track of the best 10% or something, we should be able to cluster/sort.

  // it appears that people solve for feigenbaum's alpha via picking some t values, constructing f = g - alpha g( g( t / alpha ) ) = 0
  // for each t, which forms a system of equations in coefficients, computing gradients of f at each t value,
  // constructing a jacobian, and then using newton's method to find the root.
  // subsequent iteration of that whole process apparently converges on the right answer.

  {
    f32 F[4] = { 1, 1, 1, 1 };
    f32 G[4] = { 2, 3, 4, 5 };
    f32 R[4];
    f32 T0[4];
    f32 T1[4];
    CalcAsquared( F, _countof( F ), R );
    CalcAtimesB( F, _countof( F ), G, R );

    CalcAofA( F, _countof( F ), R, T0, T1 );

    f32 alpha = -2.5f;
    CalcCtimesAofAofToverC( F, _countof( F ), R, T0, T1, alpha );
    CalcAdot( F, _countof( F ), T0 );

    // f( t ) = alpha * f( f( t / alpha ) )
    PrintArray( R, _countof( R ) );
  }



  if( 0 )
  {
  //  f32 t_tests[] = { -0.5f, -0.4f, -0.3f, -0.2f, -0.1f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };
    f32 t_tests[16] = {  };
    f32 t_tests_bounds = 0.2f;
    auto bounds_loop_len = _countof( t_tests ) / 2;
    For( i, 0, bounds_loop_len ) {
      t_tests[ 2 * i + 0 ] = +Cast( f32, i + 1 ) * t_tests_bounds / bounds_loop_len;
      t_tests[ 2 * i + 1 ] = -Cast( f32, i + 1 ) * t_tests_bounds / bounds_loop_len;
    }

    constant u32 N = 12;
  //  f32 A[N] = { 1, 1.05412, -0.671615, -0.328385 };
  //  f32 A[N] = { 1, 1, 1, 1 };
    f32 A[N];
    Fori( u32, i, 0, N ) {
      A[i] = 1;
    }
//    A[2] = -1.366f;
//    A[3] = -2.732f;

//    A[1] = 0;
//    A[3] = 0;
//    A[5] = 0;

    f32 T0[N];
    f32 T1[N];
    f32 T2[N];

  //  f32 total_err = CalcTotalErr( t_tests, _countof( t_tests ), A, N );
//    f32 total_err = CalcTotalErrRenormalization( t_tests, _countof( t_tests ), A, N );
    f32 total_err = CalcTotalErrRenormalization( A, N, T0, T1, T2 );

    f32 Abest[N] = {};

    rng_xorshift32_t rng;
  //  Init( rng, 0x1234567812345678ULL );
    Init( rng, TimeTSC() );

    u32 num_transitions = 0;

    f32 best_total_err = total_err;
//    For( u, 0, 100000000 ) {
    u64 loops = 0;
    while( best_total_err > 1.0f || loops < 100000000 ) {
      loops += 1;

      if( loops % 10000000 == 0 ) {
        printf( "loops: %llu M\n", loops / 1000000 );
      }

      // leave A[0]=1 unchanged, we're not fitting that param.
      auto idx = 1 + Rand32( rng ) % ( N - 1 );
//      auto idx = 2 + Rand32( rng ) % ( N - 2 );
//      while( ( idx != N - 1 ) && ( idx % 2 == 1 ) ) {
//        idx = 2 + Rand32( rng ) % ( N - 2 );
//      }
      AssertCrash( idx < N );
      auto old = A[idx];
      A[idx] = 10 * ( 2 * Zeta32( rng ) - 1 );

  //    f32 potential_total_err = CalcTotalErr( t_tests, _countof( t_tests ), A, N );
//      f32 potential_total_err = CalcTotalErrRenormalization( t_tests, _countof( t_tests ), A, N );
      f32 potential_total_err = CalcTotalErrRenormalization( A, N, T0, T1, T2 );

      f32 prob_switch = CLAMP( total_err / potential_total_err, 0, 1 );
      if( Zeta32( rng ) < prob_switch ) {
        num_transitions += 1;
        total_err = potential_total_err;
        if( total_err < best_total_err ) {
          Fori( u32, i, 0, N ) {
            Abest[i] = A[i];
          }
          best_total_err = total_err;
          printf( "err: %f   ", total_err );
          PrintArray( Abest, N );
        }
      } else {
        A[idx] = old;
      }
    }

    printf( "best_total_err: %f\n", best_total_err );
    printf( "num_transitions: %u\n", num_transitions );
    PrintArray( Abest, N-1 );
    printf( "alpha: %f\n", Abest[N-1] );
  }

// random lattice walks in a [ -10, 10 ] ^ N cube.

// 10M iterations, N=4
// best_total_err: 0.450952
// num_transitions: 5550948
// A: 1.000000 1.132835 -0.561097 -0.436260

// 10M iterations, N=5
// best_total_err: 0.721790
// num_transitions: 5401105
// A: 1.000000 1.563395 -0.412851 -0.881557 0.300946

// 10M iterations, N=10
// best_total_err: 1.548208
// num_transitions: 6117062
// A: 1.000000 0.097001 -0.154874 0.636585 -0.187020 -2.952207 3.018682 1.502590 -5.194418 2.303742



//best_total_err: 0.015664
//num_transitions: 62495683
//array: 1.000000 0.875630 -0.363438 -0.324348
//alpha: 0.745621

//err: 0.010398   array: 1.000000 0.998591 -1.897126 -0.335768 -4.259650
//err: 0.016358   array: 1.000000 -1.012293 0.257753 -0.006233 -4.222347
//err: 0.268438   array: 1.000000 1.240661 -2.380484 -0.416772 0.232913 -4.032364
//err: 0.147309   array: 1.000000 1.208125 -2.255611 -0.282461 0.118561 -5.492319
//err: 0.167881   array: 1.000000 -1.119583 0.286368 0.011928 -0.010871 -6.784761
//err: 0.074995   array: 1.000000 -0.987400 -3.028404 0.348996 3.029171 -2.681639
}
