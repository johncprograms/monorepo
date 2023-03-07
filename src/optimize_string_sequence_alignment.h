// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// F Matrix recursion:
//   ..... Ai
//   .. a  b
//   Bj c  d
//
// a->d means: match Ai and Bj together.
// c->d means: match Ai with nothing from B
// b->d means: match Bj with nothing from A
//
// Note the matrix has dimensions: ( A.len + 1 ) * ( B.len + 1 )
//
// Since we need to match each character from A and B, we iterate until we hit the bottom right.
// E.g. if we never have an a->d style of match, and no character pairs match, it's a Manhattan walk
// until we hit the bottom right.
//
// We're going to use a scoring system to decide which of the three kinds of pair matchings is desired.
// They are:
//   a->d: score_walk_AB(a,d)
//   c->d: score_walk_A(c,d)
//   b->d: score_walk_B(b,d)
//
// And then the maximum of those three decides the direction. Note there might be ties.
//
// Another complication is that the scoring functions don't always have an actual previous character.
// This happens in the base case of the recursion. So for full generality we'd want to account for that
// in the scoring functions. But for now I'm not sure we need it, so I'll just drop the previous character
// from these scoring functions. This effectively means we can't encode path memory into scoring.
//
// F[i+1,j+1] = MAX3(
//   F[i,j] + score_walk_AB( A[i], B[j] )
//   F[i,j+1] + score_walk_A( A[i] )
//   F[i+1,j] + score_walk_B( B[j] )
// )
//


// TODO: implement the hirschberg space optimization variants, w/o requiring full F matrix.
//   1. Only store two rows of the F matrix
//   2. Divide and conquer the inputs for smaller multiplicative effects.
//
// How necessary is the divide+conquer? I feel like simple backtracking would be better...
// At worst simple backtracking is O( A.len + B.len ) to recover the full path.
// Aha! you can't do backtracking at all if we don't store the full F matrix, or at least
// the optimal paths. And I'm not sure we can do much in the way of path-trimming in the general case.
//

// TODO: emit diffs from a hirschberg variant.
//   this will require a second pass over the diffs to combine adjacent ones, because of the "divide"
//   part of the divide+conquer design.

// TODO: figure out how to implement affine gap scoring: score new gap, score extending gap.
//   do we need some path memory here?
//
// One reference has this as the effective definition, creating multiple matrices:
// F[i+1,j+1] = MAX3(
//   G[i+1,j+1]
//   H[i+1,j+1]
//   I[i+1,j+1]
// )
// G[i+1,j+1] = F[i,j] + score_walk_AB( A[i], B[i] )
// H[i+1,j+1] = MAX(
//   H[i+1,j] + score_walk_B_extend( B[j] )
//   F[i+1,j] + score_walk_B_extend( B[j] ) + score_walk_B_open( B[j] )
// )
// I[i+1,j+1] = MAX(
//   I[i,j+1] + score_walk_A_extend( A[i] )
//   F[i,j+1] + score_walk_A_extend( A[i] ) + score_walk_A_open( A[i] )
// )
// It looks like we could eliminate the G matrix; it's the only incremental data dependency on F[i,j].
// Probably we can't merge the H and I matrices; e.g. alternating gaps in aligning A,B.

// TODO: Smith-Waterman variant, which differs from the usual by:
//   1. start with 0s in the first row+col
//   2. disallow negative cell values
//   3. backtracking starts at the maximum cell value(s), not the bottom right;
//      and terminates at any 0 cell value(s), not the top left.


// bitfield because there might be ties.
enum walk_direction_t : u32
{
  walk_start = 0u,
  walk_AB = 1u,
  walk_A = 2u,
  walk_B = 4u,
};
ForceInl walk_direction_t& operator|=( walk_direction_t& a, walk_direction_t b )
{
  a = Cast( walk_direction_t, Cast( u32, a ) | Cast( u32, b ) );
  return a;
}
// TODO: elide this with SOA; we can pack things better if we do.
//   also the code can be more separable.
template< typename Score >
struct
fcell_t
{
  Score score;
  walk_direction_t incoming_walkdir; // how we got to this cell.
};
template< typename Char, typename Score, typename ScoreWalkAB, typename ScoreWalkA, typename ScoreWalkB >
Inl void
FMatrix(
  tslice_t<Char> A,
  tslice_t<Char> B,
  fcell_t<Score>* F, // length ( A.len + 1 ) * ( B.len + 1 )
  ScoreWalkAB score_walk_AB,
  ScoreWalkA score_walk_A,
  ScoreWalkB score_walk_B
  )
{
  auto stride = A.len + 1;
  {
    auto F00 = F + 0 + 0 * stride;
    F00->score = 0;
    F00->incoming_walkdir = walk_start;
  }
  For( i, 0, A.len ) {
    auto Fc = F + i + 0 * stride;
    auto Fd = F + ( i + 1 ) + 0 * stride;
    Fd->score = Fc->score + score_walk_A( A.mem[i] );
    Fd->incoming_walkdir = walk_A;
  }
  For( j, 0, B.len ) {
    auto Fb = F + 0 + j * stride;
    auto Fd = F + 0 + ( j + 1 ) * stride;
    Fd->score = Fb->score + score_walk_B( B.mem[j] );
    Fd->incoming_walkdir = walk_B;
  }
  For( i, 0, A.len ) {
    For( j, 0, B.len ) {
      auto Fa = F + i + j * stride;
      auto Fb = F + ( i + 1 ) + j * stride;
      auto Fc = F + i + ( j + 1 ) * stride;
      auto Fd = F + ( i + 1 ) + ( j + 1 ) * stride;
      auto score_AB = Fa->score + score_walk_AB( A.mem[i], B.mem[j] ); // a->d
      auto score_A = Fc->score + score_walk_A( A.mem[i] );             // c->d
      auto score_B = Fb->score + score_walk_B( B.mem[j] );             // b->d
      auto max_score = MAX3( score_AB, score_A, score_B );
      auto max_AB = max_score == score_AB;
      auto max_A = max_score == score_A;
      auto max_B = max_score == score_B;
      auto incoming_walkdir = walk_start;
      if( max_A ) {
        incoming_walkdir |= walk_A;
      }
      if( max_B ) {
        incoming_walkdir |= walk_B;
      }
      if( max_AB ) {
        incoming_walkdir |= walk_AB;
      }
      Fd->score = max_score;
      Fd->incoming_walkdir = incoming_walkdir;
    }
  }
}

template< typename Char, typename Score >
Inl bool
HasOptimalMatching(
  tslice_t<Char> A,
  tslice_t<Char> B,
  fcell_t<Score>* F // length ( A.len + 1 ) * ( B.len + 1 )
  )
{
  auto stride = A.len + 1;
  auto i = A.len;
  auto j = B.len;
  while( i  ||  j ) {
    auto Fij = F + i + j * stride;
    auto incoming_walkdir = Fij->incoming_walkdir;
    if( !IsPowerOf2( Cast( u32, incoming_walkdir ) ) ) {
      return 0;
    }
    switch( incoming_walkdir ) {
      case walk_AB: {
        AssertCrash( i );
        AssertCrash( j );
        i -= 1;
        j -= 1;
        continue;
      }
      case walk_A: {
        AssertCrash( i );
        i -= 1;
        continue;
      }
      case walk_B: {
        AssertCrash( j );
        j -= 1;
        continue;
      }
      default: UnreachableCrash();
    }
  }
  return 1;
}

template< typename Char, typename Score >
Inl idx_t
NumberOfOptimalMatchings(
  tslice_t<Char> A,
  tslice_t<Char> B,
  fcell_t<Score>* F // length ( A.len + 1 ) * ( B.len + 1 )
  )
{
  auto stride = A.len + 1;
  idx_t npaths = 0;

  struct
  ij_t
  {
    idx_t i;
    idx_t j;
  };
  stack_resizeable_cont_t<ij_t> cells;
  Alloc( cells, A.len + B.len );
  {
    auto i = A.len;
    auto j = B.len;
    if( i  ||  j ) {
      *AddBack( cells ) = { i, j };
    }
  }
  while( cells.len ) {
    auto cell = cells.mem[ cells.len - 1 ];
    RemBack( cells );
    auto i = cell.i;
    auto j = cell.j;
    if( !i  &&  !j ) {
      npaths += 1;
      continue;
    }
    auto Fij = F + i + j * stride;
    auto incoming_walkdir = Fij->incoming_walkdir;
    AssertCrash( !( incoming_walkdir & walk_start ) );
    if( incoming_walkdir & walk_AB ) {
      AssertCrash( i );
      AssertCrash( j );
      *AddBack( cells ) = { i - 1, j - 1 };
    }
    if( incoming_walkdir & walk_A ) {
      AssertCrash( i );
      *AddBack( cells ) = { i - 1, j };
    }
    if( incoming_walkdir & walk_B ) {
      AssertCrash( j );
      *AddBack( cells ) = { i, j - 1 };
    }
  }
  Free( cells );
  return npaths;
}

template< typename Char, typename Score >
Inl void
GetArbitraryOptimalMatching(
  tslice_t<Char> A,
  tslice_t<Char> B,
  fcell_t<Score>* F, // length ( A.len + 1 ) * ( B.len + 1 )
  Char invalid_char,
  Char* buffer, // length 2 * ( A.len + B.len )
  tslice_t<Char>* resultA, // points into buffer
  tslice_t<Char>* resultB  // points into buffer
  )
{
  auto stride = A.len + 1;
  auto i = A.len;
  auto j = B.len;
  auto result_idx = A.len + B.len;
  auto bufferA = buffer;
  auto bufferB = buffer + ( A.len + B.len );
  while( i  ||  j ) {
    auto Fij = F + i + j * stride;
    auto incoming_walkdir = Fij->incoming_walkdir;
    AssertCrash( !( incoming_walkdir & walk_start ) );
    if( incoming_walkdir & walk_AB ) {
      AssertCrash( i );
      AssertCrash( j );
      i -= 1;
      j -= 1;
      result_idx -= 1;
      bufferA[result_idx] = A.mem[i];
      bufferB[result_idx] = B.mem[j];
      continue;
    }
    if( incoming_walkdir & walk_A ) {
      AssertCrash( i );
      i -= 1;
      result_idx -= 1;
      bufferA[result_idx] = A.mem[i];
      bufferB[result_idx] = invalid_char;
      continue;
    }
    if( incoming_walkdir & walk_B ) {
      AssertCrash( j );
      j -= 1;
      result_idx -= 1;
      bufferA[result_idx] = invalid_char;
      bufferB[result_idx] = B.mem[j];
      continue;
    }
    UnreachableCrash();
  }
  *resultA = { bufferA + result_idx, A.len + B.len - result_idx };
  *resultB = { bufferB + result_idx, A.len + B.len - result_idx };
}

template< typename Char >
struct
fmatching_t
{
  tslice_t<Char> resultA;
  tslice_t<Char> resultB;
};
template< typename Char >
Inl fmatching_t<Char>*
_NewMatching(
  pagelist_t* pagelist,
  stack_resizeable_cont_t<fmatching_t<Char>>* results,
  idx_t length
  )
{
  auto matching = AddBack( *results );
  auto mem = AddPagelist( *pagelist, Char, _SIZEOF_IDX_T, 2 * length );
  auto memA = mem;
  auto memB = mem + length;
  //
  // Since we're adding to the slices backwards, we can use the .mem field as our
  // insertion iterator. We're guaranteed not to write more than A.len + B.len
  // characters, so we can allocate for that size and then under-utilize sometimes.
  //
  matching->resultA = { memA + length, 0 };
  matching->resultB = { memB + length, 0 };
  return matching;
}
template< typename Char >
Inl fmatching_t<Char>*
_CopyMatching(
  pagelist_t* pagelist,
  stack_resizeable_cont_t<fmatching_t<Char>>* results,
  idx_t length,
  fmatching_t<Char>* src
  )
{
  auto copied_matching = _NewMatching( pagelist, results, length );
  auto lenA = src->resultA.len;
  copied_matching->resultA.mem -= lenA;
  copied_matching->resultA.len += lenA;
  TMove(
    copied_matching->resultA.mem,
    src->resultA.mem,
    lenA
    );
  auto lenB = src->resultB.len;
  copied_matching->resultB.mem -= lenB;
  copied_matching->resultB.len += lenB;
  TMove(
    copied_matching->resultB.mem,
    src->resultB.mem,
    lenB
    );
  return copied_matching;
};
//
// Returns all possible optimal matchings of A and B.
// Note that the order of results is somewhat arbitrary, although deterministic.
//
// Implementation details:
// This uses a copy-on-write strategy for the results as it does the backtracking.
// This gives us a somewhat minimal memory footprint, given the problem complexity.
//
template< typename Char, typename Score >
Inl void
GetAllOptimalMatchings(
  tslice_t<Char> A,
  tslice_t<Char> B,
  fcell_t<Score>* F, // length ( A.len + 1 ) * ( B.len + 1 )
  Char invalid_char,
  pagelist_t* pagelist,
  stack_resizeable_cont_t<fmatching_t<Char>>* results
  )
{
  auto stride = A.len + 1;

  struct
  ij_t
  {
    idx_t i;
    idx_t j;
    fmatching_t<Char>* matching; // points into results
  };
  stack_resizeable_cont_t<ij_t> cells;
  Alloc( cells, A.len + B.len );
  {
    auto i = A.len;
    auto j = B.len;
    if( i  ||  j ) {
      auto matching = _NewMatching( pagelist, results, A.len + B.len );
      *AddBack( cells ) = { i, j, matching };
    }
  }
  while( cells.len ) {
    auto cell = cells.mem[ cells.len - 1 ];
    RemBack( cells );
    auto i = cell.i;
    auto j = cell.j;
    if( !i  &&  !j ) {
      // Done with this path traversal.
      continue;
    }
    auto Fij = F + i + j * stride;
    auto incoming_walkdir = Fij->incoming_walkdir;
    AssertCrash( !( incoming_walkdir & walk_start ) );
    auto num_walkdirs = _mm_popcnt_u32( incoming_walkdir );
    AssertCrash( num_walkdirs <= 3u );
    if( incoming_walkdir & walk_AB ) {
      AssertCrash( i );
      AssertCrash( j );
      auto u = i - 1;
      auto v = j - 1;
      AssertCrash( num_walkdirs );
      num_walkdirs -= 1;
      auto matching = cell.matching;
      if( num_walkdirs ) {
        matching = _CopyMatching( pagelist, results, A.len + B.len, matching );
      }
      matching->resultA.mem -= 1;
      matching->resultA.len += 1;
      matching->resultA.mem[0] = A.mem[u];
      matching->resultB.mem -= 1;
      matching->resultB.len += 1;
      matching->resultB.mem[0] = B.mem[v];
      *AddBack( cells ) = { u, v, matching };
    }
    if( incoming_walkdir & walk_A ) {
      AssertCrash( i );
      auto u = i - 1;
      auto v = j;
      AssertCrash( num_walkdirs );
      num_walkdirs -= 1;
      auto matching = cell.matching;
      if( num_walkdirs ) {
        matching = _CopyMatching( pagelist, results, A.len + B.len, matching );
      }
      matching->resultA.mem -= 1;
      matching->resultA.len += 1;
      matching->resultA.mem[0] = A.mem[u];
      matching->resultB.mem -= 1;
      matching->resultB.len += 1;
      matching->resultB.mem[0] = invalid_char;
      *AddBack( cells ) = { u, v, matching };
    }
    if( incoming_walkdir & walk_B ) {
      AssertCrash( j );
      auto u = i;
      auto v = j - 1;
      AssertCrash( num_walkdirs );
      num_walkdirs -= 1;
      auto matching = cell.matching;
      if( num_walkdirs ) {
        matching = _CopyMatching( pagelist, results, A.len + B.len, matching );
      }
      matching->resultA.mem -= 1;
      matching->resultA.len += 1;
      matching->resultA.mem[0] = invalid_char;
      matching->resultB.mem -= 1;
      matching->resultB.len += 1;
      matching->resultB.mem[0] = B.mem[v];
      *AddBack( cells ) = { u, v, matching };
    }
  }
  Free( cells );
}

Enumc( fdiff_type_t )
{
  AB,
  A,
  B,
};
template< typename Char >
struct
fdiff_string_t
{
  fdiff_type_t type;
  tslice_t<Char> valueA;
  tslice_t<Char> valueB;
};
// Emits diffs in reverse order because that's the natural order of the backtracking.
// And since we don't know how many there will be, it's faster to leave them backwards.
// The caller can reverse if they want, or just iterate backwards.
template< typename Char, typename Score >
Inl void
GetArbitraryOptimalDiffs(
  tslice_t<Char> A,
  tslice_t<Char> B,
  fcell_t<Score>* F, // length ( A.len + 1 ) * ( B.len + 1 )
  stack_resizeable_cont_t<fdiff_string_t<Char>>* reversed_diffs // points into A and B.
  )
{
  auto stride = A.len + 1;
  auto i = A.len;
  auto j = B.len;
  while( i  ||  j ) {
    auto Fij = F + i + j * stride;
    auto incoming_walkdir = Fij->incoming_walkdir;
    AssertCrash( !( incoming_walkdir & walk_start ) );
    if( incoming_walkdir & walk_AB ) {
      AssertCrash( i );
      AssertCrash( j );
      i -= 1;
      j -= 1;
      auto Ai = A.mem[i];
      auto Bj = B.mem[j];
      if( Ai != Bj ) {
        bool add_new_diff = 1;
        if( reversed_diffs->len ) {
          auto last_diff = reversed_diffs->mem + reversed_diffs->len - 1;
          if( last_diff->type == fdiff_type_t::AB  &&
              last_diff->valueA.mem == A.mem + i + 1  &&
              last_diff->valueB.mem == B.mem + j + 1 )
          {
            last_diff->valueA.mem = A.mem + i;
            last_diff->valueA.len += 1;
            last_diff->valueB.mem = B.mem + j;
            last_diff->valueB.len += 1;
            add_new_diff = 0;
          }
        }
        if( add_new_diff ) {
          auto diff = AddBack( *reversed_diffs );
          diff->type = fdiff_type_t::AB;
          diff->valueA = { A.mem + i, 1 };
          diff->valueB = { B.mem + j, 1 };
        }
      }
      continue;
    }
    if( incoming_walkdir & walk_A ) {
      AssertCrash( i );
      i -= 1;
      bool add_new_diff = 1;
      if( reversed_diffs->len ) {
        auto last_diff = reversed_diffs->mem + reversed_diffs->len - 1;
        if( last_diff->type == fdiff_type_t::A  &&
            last_diff->valueA.mem == A.mem + i + 1 )
        {
          last_diff->valueA.mem = A.mem + i;
          last_diff->valueA.len += 1;
          add_new_diff = 0;
        }
      }
      if( add_new_diff ) {
        auto diff = AddBack( *reversed_diffs );
        diff->type = fdiff_type_t::A;
        diff->valueA = { A.mem + i, 1 };
        diff->valueB = {};
      }
      continue;
    }
    if( incoming_walkdir & walk_B ) {
      AssertCrash( j );
      j -= 1;
      bool add_new_diff = 1;
      if( reversed_diffs->len ) {
        auto last_diff = reversed_diffs->mem + reversed_diffs->len - 1;
        if( last_diff->type == fdiff_type_t::B  &&
            last_diff->valueB.mem == B.mem + j + 1 )
        {
          last_diff->valueB.mem = B.mem + j;
          last_diff->valueB.len += 1;
          add_new_diff = 0;
        }
      }
      if( add_new_diff ) {
        auto diff = AddBack( *reversed_diffs );
        diff->type = fdiff_type_t::B;
        diff->valueA = {};
        diff->valueB = { B.mem + j, 1 };
      }
      continue;
    }
    UnreachableCrash();
  }
}

template< typename Char >
Inl idx_t
_NewDiffs(
  stack_resizeable_cont_t<stack_resizeable_cont_t<fdiff_string_t<Char>>>* results,
  idx_t initial_capacity
  )
{
  auto which_path = results->len;
  auto diffs = AddBack( *results );
  Alloc( *diffs, initial_capacity );
  return which_path;
}
template< typename Char >
Inl idx_t
_CopyDiffs(
  stack_resizeable_cont_t<stack_resizeable_cont_t<fdiff_string_t<Char>>>* results,
  idx_t which_path
  )
{
  AssertCrash( which_path < results->len );
  auto diffs = results->mem + which_path;

  auto copied_which_path = _NewDiffs( results, diffs->capacity );
  auto copied_diffs = results->mem + copied_which_path;
  auto num_copy = diffs->len;
  TMove(
    AddBack( *copied_diffs, num_copy ),
    diffs->mem,
    num_copy
    );
  return copied_which_path;
}
template< typename Char, typename Score >
Inl void
GetAllOptimalDiffs(
  tslice_t<Char> A,
  tslice_t<Char> B,
  fcell_t<Score>* F, // length ( A.len + 1 ) * ( B.len + 1 )
  stack_resizeable_cont_t<stack_resizeable_cont_t<fdiff_string_t<Char>>>* results
  )
{
  auto stride = A.len + 1;
  constant auto num_diffs_estimate = 10u; // TODO: what's a good value here?

  struct
  ij_t
  {
    idx_t i;
    idx_t j;
    idx_t which_path; // index into results
  };
  stack_resizeable_cont_t<ij_t> cells;
  Alloc( cells, A.len + B.len );
  {
    auto i = A.len;
    auto j = B.len;
    if( i  ||  j ) {
      auto which_path = _NewDiffs( results, num_diffs_estimate );
      *AddBack( cells ) = { i, j, which_path };
    }
  }
  while( cells.len ) {
    auto cell = cells.mem[ cells.len - 1 ];
    RemBack( cells );
    auto i = cell.i;
    auto j = cell.j;
    if( !i  &&  !j ) {
      // Done with this path traversal.
      continue;
    }
    auto Fij = F + i + j * stride;
    auto incoming_walkdir = Fij->incoming_walkdir;
    AssertCrash( !( incoming_walkdir & walk_start ) );
    auto num_walkdirs = _mm_popcnt_u32( incoming_walkdir );
    AssertCrash( num_walkdirs <= 3u );
    if( incoming_walkdir & walk_AB ) {
      AssertCrash( i );
      AssertCrash( j );
      auto u = i - 1;
      auto v = j - 1;
      AssertCrash( num_walkdirs );
      num_walkdirs -= 1;
      auto which_path = cell.which_path;
      if( num_walkdirs ) {
        which_path = _CopyDiffs( results, which_path );
      }
      AssertCrash( which_path < results->len );
      auto diffs = results->mem + which_path;
      auto Au = A.mem[u];
      auto Bv = B.mem[v];
      if( Au != Bv ) {
        bool add_new_diff = 1;
        if( diffs->len ) {
          auto last_diff = diffs->mem + diffs->len - 1;
          if( last_diff->type == fdiff_type_t::AB  &&
              last_diff->valueA.mem == A.mem + u + 1  &&
              last_diff->valueB.mem == B.mem + v + 1 )
          {
            last_diff->valueA.mem = A.mem + u;
            last_diff->valueA.len += 1;
            last_diff->valueB.mem = B.mem + v;
            last_diff->valueB.len += 1;
            add_new_diff = 0;
          }
        }
        if( add_new_diff ) {
          auto diff = AddBack( *diffs );
          diff->type = fdiff_type_t::AB;
          diff->valueA = { A.mem + u, 1 };
          diff->valueB = { B.mem + v, 1 };
        }
      }
      *AddBack( cells ) = { u, v, which_path };
    }
    if( incoming_walkdir & walk_A ) {
      AssertCrash( i );
      auto u = i - 1;
      auto v = j;
      AssertCrash( num_walkdirs );
      num_walkdirs -= 1;
      auto which_path = cell.which_path;
      if( num_walkdirs ) {
        which_path = _CopyDiffs( results, which_path );
      }
      AssertCrash( which_path < results->len );
      auto diffs = results->mem + which_path;
      bool add_new_diff = 1;
      if( diffs->len ) {
        auto last_diff = diffs->mem + diffs->len - 1;
        if( last_diff->type == fdiff_type_t::A  &&
            last_diff->valueA.mem == A.mem + u + 1 )
        {
          last_diff->valueA.mem = A.mem + u;
          last_diff->valueA.len += 1;
          add_new_diff = 0;
        }
      }
      if( add_new_diff ) {
        auto diff = AddBack( *diffs );
        diff->type = fdiff_type_t::A;
        diff->valueA = { A.mem + u, 1 };
        diff->valueB = {};
      }
      *AddBack( cells ) = { u, v, which_path };
    }
    if( incoming_walkdir & walk_B ) {
      AssertCrash( j );
      auto u = i;
      auto v = j - 1;
      AssertCrash( num_walkdirs );
      num_walkdirs -= 1;
      auto which_path = cell.which_path;
      if( num_walkdirs ) {
        which_path = _CopyDiffs( results, which_path );
      }
      AssertCrash( which_path < results->len );
      auto diffs = results->mem + which_path;
      bool add_new_diff = 1;
      if( diffs->len ) {
        auto last_diff = diffs->mem + diffs->len - 1;
        if( last_diff->type == fdiff_type_t::B  &&
            last_diff->valueB.mem == B.mem + v + 1 )
        {
          last_diff->valueB.mem = B.mem + v;
          last_diff->valueB.len += 1;
          add_new_diff = 0;
        }
      }
      if( add_new_diff ) {
        auto diff = AddBack( *diffs );
        diff->type = fdiff_type_t::B;
        diff->valueA = {};
        diff->valueB = { B.mem + v, 1 };
      }
      *AddBack( cells ) = { u, v, which_path };
    }
  }
  Free( cells );
}




template< typename Char, typename Score, typename ScoreWalkAB, typename ScoreWalkA, typename ScoreWalkB >
Inl void
FMatrixLastRow(
  tslice_t<Char> A,
  tslice_t<Char> B,
  Score* F, // length 2 * ( A.len + 1 )
  ScoreWalkAB score_walk_AB,
  ScoreWalkA score_walk_A,
  ScoreWalkB score_walk_B,
  Score* last_score_line // length ( A.len + 1 )
  )
{
  auto stride = A.len + 1;
  {
    auto F00 = F + 0 + 0 * stride;
    *F00 = 0;
  }
  For( i, 0, A.len ) {
    auto Fc = F + i + 0 * stride;
    auto Fd = F + ( i + 1 ) + 0 * stride;
    *Fd = *Fc + score_walk_A( A.mem[i] );
  }
  For( j, 0, B.len ) {
    {
      auto Fb = F + 0 + 0 * stride;
      auto Fd = F + 0 + 1 * stride;
      *Fd = *Fb + score_walk_B( B.mem[j] );
    }
    For( i, 0, A.len ) {
      auto Fa = F + ( i + 0 ) + 0 * stride;
      auto Fb = F + ( i + 1 ) + 0 * stride;
      auto Fc = F + ( i + 0 ) + 1 * stride;
      auto Fd = F + ( i + 1 ) + 1 * stride;
      auto score_AB = *Fa + score_walk_AB( A.mem[i], B.mem[j] ); // a->d
      auto score_A = *Fc + score_walk_A( A.mem[i] );             // c->d
      auto score_B = *Fb + score_walk_B( B.mem[j] );             // b->d
      auto max_score = MAX3( score_AB, score_A, score_B );
      *Fd = max_score;
    }
    // PERF: pointer-swapping rows might be faster.
    TMove(
      F + 0 + 0 * stride,
      F + 0 + 1 * stride,
      A.len + 1
      );
  }
  // TODO: no need to copy out of F, the caller can do that if they want.
  For( i, 0, A.len + 1 ) {
    auto Fi = F + i + 1 * stride;
    last_score_line[i] = *Fi;
  }
}

// Identical to FMatrixLastRow, except that all accesses to A and B are reversed.
// With smarter iterators we could probably eliminate this.
template< typename Char, typename Score, typename ScoreWalkAB, typename ScoreWalkA, typename ScoreWalkB >
Inl void
FMatrixLastRowReverse(
  tslice_t<Char> A,
  tslice_t<Char> B,
  Score* F, // length 2 * ( B.len + 1 )
  ScoreWalkAB score_walk_AB,
  ScoreWalkA score_walk_A,
  ScoreWalkB score_walk_B,
  Score* last_score_line // length ( B.len + 1 )
  )
{
  auto stride = A.len + 1;
  auto last_A = A.len - 1;
  auto last_B = B.len - 1;
  {
    auto F00 = F + 0 + 0 * stride;
    *F00 = 0;
  }
  For( i, 0, A.len ) {
    auto Fc = F + i + 0 * stride;
    auto Fd = F + ( i + 1 ) + 0 * stride;
    *Fd = *Fc + score_walk_A( A.mem[ last_A - i ] );
  }
  For( j, 0, B.len ) {
    {
      auto Fb = F + 0 + 0 * stride;
      auto Fd = F + 0 + 1 * stride;
      *Fd = *Fb + score_walk_B( B.mem[ last_B - j ] );
    }
    For( i, 0, A.len ) {
      auto Fa = F + ( i + 0 ) + 0 * stride;
      auto Fb = F + ( i + 1 ) + 0 * stride;
      auto Fc = F + ( i + 0 ) + 1 * stride;
      auto Fd = F + ( i + 1 ) + 1 * stride;
      auto score_AB = *Fa + score_walk_AB( A.mem[ last_A - i ], B.mem[ last_B - j ] ); // a->d
      auto score_A = *Fc + score_walk_A( A.mem[ last_A - i ] );             // c->d
      auto score_B = *Fb + score_walk_B( B.mem[ last_B - j ] );             // b->d
      auto max_score = MAX3( score_AB, score_A, score_B );
      *Fd = max_score;
    }
    // PERF: pointer-swapping rows might be faster.
    TMove(
      F + 0 + 0 * stride,
      F + 0 + 1 * stride,
      A.len + 1
      );
  }
  // TODO: no need to copy out of F, the caller can do that if they want.
  For( i, 0, A.len + 1 ) {
    auto Fi = F + i + 1 * stride;
    last_score_line[i] = *Fi;
  }
}




// TODO: rewrite

//
// TODO: compute tree of maximal matchings
//   look for cases where ins/del/rep have equal cost, and fork.
//   is the tree node count limited? how many possible paths are there?
//   i think it's O( 3 * ( x.len * ( y.len + 1 ) )^2 ), since there's usually 3 next-node choices,
//   for every element in the score matrix. the 2 terminal edges and 1 corner have fewer choices.
//
template< typename Char, typename Score, typename CostInsDel, typename CostRep >
Inl void
Hirschberg(
  tslice_t<Char> x,
  tslice_t<Char> y,
  Char invalid_constant,
  tslice_t<Char> z,
  tslice_t<Char> w, // TODO: duped len. also, it's unnecessary for z too.
  Score* score, // length 2 * ( y.len + 1 )
  CostInsDel cost_insert,
  CostInsDel cost_delete,
  CostRep cost_replace,
  Score* buffer, // length ( y.len + 1 )
  Score* buffer2 // length ( y.len + 1 )
  )
{
  if( !x.len ) {
    For( j, 0, y.len ) {
      z.mem[j] = invalid_constant;
      w.mem[j] = y.mem[j];
    }
    AssertCrash( z.len == y.len );
    AssertCrash( w.len == y.len );
    return;
  }
  if( !y.len ) {
    For( i, 0, x.len ) {
      z.mem[i] = x.mem[i];
      w.mem[i] = invalid_constant;
    }
    AssertCrash( z.len == x.len );
    AssertCrash( w.len == x.len );
    return;
  }
  if( y.len == 1 ) {
LSingleCharY:
    FMatrixLastRow( x, y, score, cost_replace, cost_insert, cost_delete, buffer );
    auto i = x.len;
    auto j = y.len;
    auto stride = y.len + 1;
    Forever { // while( i != 0  &&  j != 0 )
      auto i_pos = i != 0;
      auto j_pos = j != 0;
      if( !( i_pos | j_pos ) ) break;
      auto score_ij = score[ i + j * stride ];
      if( i_pos  &&  j_pos ) {
        // PERF: is it better to check for del and ins, rather than rep? probably for instruction count.
        //   which two are cheapest to check for memory wise? rep and ins are contiguous with row major.
        auto score_rep = score[ ( i - 1 ) + ( j - 1 ) * stride ] + cost_replace( x.mem[i], y.mem[j] );
        if( score_ij == score_rep ) {
          // note we use i to index z and w, because we know x is larger than y, and so don't need to do MAX.
          AssertCrash( i );
          AssertCrash( j );
          i -= 1;
          j -= 1;
          z.mem[i] = x.mem[i];
          w.mem[i] = y.mem[j];
          // PERF: we could enter a simpler loop here, knowing j == 0.
          continue;
        }
      }
      if( i_pos ) {
        auto score_del = score[ ( i - 1 ) + j * stride ] + cost_delete( x.mem[i] );
        if( score_ij == score_del ) {
          // note we use i to index z and w, because we know x is larger than y, and so don't need to do MAX.
          AssertCrash( i );
          i -= 1;
          z.mem[i] = x.mem[i];
          w.mem[i] = invalid_constant;
          continue;
        }
      }

      // note we use 0 to index z and w, because we know we're done here.
      AssertCrash( j );
      j -= 1;
      z.mem[0] = invalid_constant;
      w.mem[0] = y.mem[j];
      // PERF: we could enter a simpler loop here, knowing j == 0.
      continue;
    }
    return;
  }
  if( x.len == 1 ) {
    // we treat x as the longer string, and in this case y is at least as long as x.
    // since this maximum alignment problem is symmetric, we're free to swap here.
    // this should help eliminate perf variance, to some extent.
    SWAP( tslice_t<Char>, x, y );
    SWAP( tslice_t<Char>, z, w );
    goto LSingleCharY;
  }

  auto score_l = buffer;
  auto score_r = buffer2;
  auto x_mid = x.len / 2;
  tslice_t<Char> x_left = { x.mem, x_mid };
  tslice_t<Char> x_rght = { x.mem + x_mid, x.len - x_mid };
  FMatrixLastRow( x_left, y, score, cost_replace, cost_insert, cost_delete, score_l );
  FMatrixLastRowReverse( x_rght, y, score, cost_replace, cost_insert, cost_delete, score_r );
  auto score_max = score_l[0] + score_r[ y.len - 0 ];
  idx_t j_max = 0;
  For( j, 1, y.len + 1 ) {
    auto score_j = score_l[j] + score_r[ y.len - j ];
    if( score_j >= score_max ) {
      score_max = score_j;
      j_max = j;
    }
  }
  auto y_mid = j_max;
  tslice_t<Char> y_left = { y.mem, y_mid };
  tslice_t<Char> y_rght = { y.mem + y_mid, y.len - y_mid };
  auto xy_max = MAX( x_mid, y_mid );
  auto zw_left_len = xy_max;
  // auto zw_rght_len = MAX( x_rght.len, y_rght.len );
  tslice_t<Char> z_left = { z.mem, zw_left_len };
  tslice_t<Char> w_left = { w.mem, zw_left_len };
  Hirschberg( x_left, y_left, invalid_constant, z_left, w_left, score, cost_insert, cost_delete, cost_replace, buffer, buffer2 );
  tslice_t<Char> z_rght = { z.mem + zw_left_len, z.len - zw_left_len };
  tslice_t<Char> w_rght = { w.mem + zw_left_len, w.len - zw_left_len };
  Hirschberg( x_rght, y_rght, invalid_constant, z_rght, w_rght, score, cost_insert, cost_delete, cost_replace, buffer, buffer2 );
}



#if defined(TEST)

struct
test_hirschberg_t
{
  const void* x;
  const void* y;
  const void* z;
  const void* w;
  // TODO: cost params?
};

Inl s32
CostInsDel( u8 c )
{
  return -2;
}
Inl s32
CostRep( u8 a, u8 b )
{
  return a == b  ?  2  :  -1;
}

static void
TestStringSequenceAlignment()
{
  {
    s32 expected[] = {
      // Reference result F matrix:
      //           T    A    T    G    C
              0,  -2,  -4,  -6,  -8, -10,
      /*A*/  -2,  -1,   0,  -2,  -4,  -6,
      /*G*/  -4,  -3,  -2,  -1,   0,  -2,
      /*T*/  -6,  -2,  -4,   0,  -2,  -1,
      /*A*/  -8,  -4,   0,  -2,  -1,  -3,
      /*C*/ -10,  -6,  -2,  -1,  -3,   1,
      /*G*/ -12,  -8,  -4,  -3,   1,  -1,
      /*C*/ -14, -10,  -6,  -5,  -1,   3,
      /*A*/ -16, -12,  -8,  -7,  -3,   1,
    };
    auto A = SliceFromCStr( "TATGC" );
    auto B = SliceFromCStr( "AGTACGCA" );
    auto F = AllocString<fcell_t<s32>>( ( A.len + 1 ) * ( B.len + 1 ) );
    FMatrix( A, B, F.mem, CostRep, CostInsDel, CostInsDel );
    AssertCrash( F.len == _countof( expected ) );
    For( i, 0, _countof( expected ) ) {
      auto expect = expected[i];
      auto Fi = F.mem + i;
      auto actual = Fi->score;
      AssertCrash( actual == expect );
    }

    auto score = AllocString<s32>( 2 * ( A.len + 1 ) );
    auto last_line = AllocString<s32>( A.len + 1 );
    FMatrixLastRow( A, B, score.mem, CostRep, CostInsDel, CostInsDel, last_line.mem );
    For( i, 0, A.len + 1 ) {
      auto expect = expected[ i + B.len * ( A.len + 1 ) ];
      auto actual = last_line.mem[i];
      AssertCrash( actual == expect );
    }

    auto has_optimum = HasOptimalMatching( A, B, F.mem );
    AssertCrash( has_optimum );

    auto num_optimums = NumberOfOptimalMatchings( A, B, F.mem );
    AssertCrash( num_optimums == 1 );

    auto buffer = AllocString<u8>( 2 * ( A.len + B.len ) );
    TSet( ML( buffer ), Cast( u8, 'x' ) );
    slice_t alignA;
    slice_t alignB;
    GetArbitraryOptimalMatching( A, B, F.mem, Cast( u8, '-' ), buffer.mem, &alignA, &alignB );

    auto expectA = SliceFromCStr( "--TATGC-" );
    auto expectB = SliceFromCStr( "AGTACGCA" );
    AssertCrash( EqualContents( alignA, expectA ) );
    AssertCrash( EqualContents( alignB, expectB ) );

    stack_resizeable_cont_t<fdiff_string_t<u8>> reversed_diffs;
    Alloc( reversed_diffs, 20 );
    GetArbitraryOptimalDiffs( A, B, F.mem, &reversed_diffs );

    Free( reversed_diffs );
    Free( buffer );
    Free( last_line );
    Free( score );
    Free( F );
  }

  {
    auto A = SliceFromCStr( "AAA" );
    auto B = SliceFromCStr( "A" );
    auto F = AllocString<fcell_t<s32>>( ( A.len + 1 ) * ( B.len + 1 ) );
    FMatrix( A, B, F.mem, CostRep, CostInsDel, CostInsDel );

    auto num_optimums = NumberOfOptimalMatchings( A, B, F.mem );
    AssertCrash( num_optimums == 3 );

    pagelist_t pagelist;
    Init( pagelist, 32000 );
    stack_resizeable_cont_t<fmatching_t<u8>> matchings;
    Alloc( matchings, 10 );
    GetAllOptimalMatchings( A, B, F.mem, Cast( u8, '-' ), &pagelist, &matchings );
    AssertCrash( matchings.len == 3 );
    auto expected_A = SliceFromCStr( "AAA" );
    slice_t expected_Bs[] = {
      SliceFromCStr( "--A" ),
      SliceFromCStr( "-A-" ),
      SliceFromCStr( "A--" ),
    };
    idx_t count_expected_Bs[] = { 0u, 0u, 0u };
    ForLen( i, matchings ) {
      auto matching = matchings.mem + i;
      AssertCrash( EqualContents( expected_A, matching->resultA ) );
      For( j, 0, _countof( expected_Bs ) ) {
        auto expected_B = expected_Bs[j];
        if( EqualContents( expected_B, matching->resultB ) ) {
          count_expected_Bs[j] += 1;
        }
      }
    }
    ForEach( count, count_expected_Bs ) {
      AssertCrash( count == 1 );
    }

    stack_resizeable_cont_t<stack_resizeable_cont_t<fdiff_string_t<u8>>> alldiffs;
    Alloc( alldiffs, 8 );
    GetAllOptimalDiffs( A, B, F.mem, &alldiffs );
    AssertCrash( alldiffs.len == 3 );
    idx_t count_expected_diffs[] = { 0u, 0u, 0u };
    slice_t first_two_As = { A.mem, 2 };
    slice_t last_two_As = { A.mem + 1, 2 };
    slice_t first_A = { A.mem, 1 };
    slice_t last_A = { A.mem + 2, 1 };
    ForLen( i, alldiffs ) {
      auto diffs = alldiffs.mem + i;
      if( diffs->len == 1 ) {
        auto diff = diffs->mem + 0;
        AssertCrash( diff->type == fdiff_type_t::A );
        if( EqualBounds( diff->valueA, first_two_As ) ) {
          count_expected_diffs[0] += 1;
        }
        elif( EqualBounds( diff->valueA, last_two_As ) ) {
          count_expected_diffs[2] += 1;
        }
        else {
          UnreachableCrash();
        }
      }
      else {
        AssertCrash( diffs->len == 2 );
        auto diff0 = diffs->mem + 0;
        auto diff1 = diffs->mem + 1;
        AssertCrash( diff0->type == fdiff_type_t::A );
        AssertCrash( diff1->type == fdiff_type_t::A );
        // Diffs are in reverse order by contract.
        AssertCrash( EqualBounds( diff0->valueA, last_A ) );
        AssertCrash( EqualBounds( diff1->valueA, first_A ) );
        count_expected_diffs[1] += 1;
      }
    }
    ForEach( count, count_expected_diffs ) {
      AssertCrash( count == 1 );
    }

    ForLen( i, alldiffs ) {
      auto diffs = alldiffs.mem + i;
      Free( *diffs );
    }
    Free( alldiffs );

    Free( matchings );
    Kill( pagelist );
    Free( F );
  }

  {
    s32 expected[] = {
      -8, -4, 0, -2, -1, -3,
    };
    auto A = SliceFromCStr( "TATGC" );
    auto B = SliceFromCStr( "AGTA" );
    auto score = AllocString<s32>( 2 * ( A.len + 1 ) );
    auto last_line = AllocString<s32>( A.len + 1 );
    FMatrixLastRow( A, B, score.mem, CostRep, CostInsDel, CostInsDel, last_line.mem );
    AssertCrash( EqualContents( SliceFromCArray( s32, expected ), SliceFromString( last_line ) ) );
    Free( last_line );
    Free( score );
  }

  {
    s32 expected[] = {
      -8, -4, 0, 1, -1, -3,
    };
    auto A = SliceFromCStr( "TATGC" );
    auto B = SliceFromCStr( "CGCA" );
    auto score = AllocString<s32>( 2 * ( A.len + 1 ) );
    auto last_line = AllocString<s32>( A.len + 1 );
    FMatrixLastRowReverse( A, B, score.mem, CostRep, CostInsDel, CostInsDel, last_line.mem );
    AssertCrash( EqualContents( SliceFromCArray( s32, expected ), SliceFromString( last_line ) ) );
    Free( last_line );
    Free( score );
  }

  if( 0 ) { // TODO: crashing bug
    test_hirschberg_t tests[] = { { "AGTACGCA", "TATGC", "--TATGC-", "AGTACGCA" } };
    ForEach( test, tests ) {
      auto x = SliceFromCStr( test.x );
      auto y = SliceFromCStr( test.y );
      auto z_expected = SliceFromCStr( test.z );
      auto w_expected = SliceFromCStr( test.w );
      AssertCrash( MAX( x.len, y.len ) == w_expected.len );
      AssertCrash( z_expected.len == w_expected.len );
      auto z = AllocString<u8, allocator_heap_t, allocation_heap_t>( z_expected.len );
      auto w = AllocString<u8, allocator_heap_t, allocation_heap_t>( z_expected.len );
      ZeroContents( z );
      ZeroContents( w );
      auto score = AllocString<s32>( 2 * ( y.len + 1 ) );
      auto buffer = AllocString<s32>( y.len + 1 );
      auto buffer2 = AllocString<s32>( y.len + 1 );
      Hirschberg( x, y, Cast( u8, '-' ), SliceFromString( z ), SliceFromString( w ), score.mem, CostInsDel, CostInsDel, CostRep, buffer.mem, buffer2.mem );
      AssertCrash( EqualContents( SliceFromString( z ), z_expected ) );
      AssertCrash( EqualContents( SliceFromString( w ), w_expected ) );
      Free( z );
      Free( w );
      Free( score );
      Free( buffer );
      Free( buffer2 );
    }
  }
}

#endif // defined(TEST)
