// Copyright (c) John A. Carlos Jr., all rights reserved.

template< typename Char, typename Score, typename CostInsDel, typename CostRep >
Inl void
NeedlemanWunschScore(
  tslice_t<Char> x,
  tslice_t<Char> y,
  Score* score, // length 2 * ( y.len + 1 )
  CostInsDel cost_insert,
  CostInsDel cost_delete,
  CostRep cost_replace,
  Score* last_score_line // length ( y.len + 1 )
  )
{
  auto stride = y.len + 1;
  score[ 0 + 0 * stride ] = 0;
  For( j, 0, y.len ) {
    score[ 0 + ( j + 1 ) * stride ] = score[ 0 + j * stride ] + cost_insert( y.mem[j] );
  }
  For( i, 1, x.len ) {
    score[ 1 + 0 * stride ] = score[ 0 + 0 * stride ] + cost_delete( x.mem[i] );
    For( j, 0, y.len ) {
      auto rep = score[ 0 + j * stride ] + cost_replace( x.mem[i], y.mem[j] );
      auto del = score[ 0 + ( j + 1 ) * stride ] + cost_delete( x.mem[i] );
      auto ins = score[ 1 + j * stride ] + cost_insert( y.mem[j] );
      score[ 1 + ( j + 1 ) * stride ] = MAX3( rep, del, ins );
    }
    // copies col 1 to col 0
    // PERF: row-wise, and just pointer swap / index swap?
    For( j, 0, y.len + 1 ) {
      score[ 0 + j * stride ] = score[ 1 + j * stride ];
    }
  }
  // PERF: elide this copy
  For( j, 0, y.len + 1 ) {
    last_score_line[j] = score[ 1 + j * stride ];
  }
}
template< typename Char, typename Score, typename CostInsDel, typename CostRep >
Inl void
NeedlemanWunschScoreReverse(
  tslice_t<Char> x,
  tslice_t<Char> y,
  Score* score, // length 2 * ( y.len + 1 )
  CostInsDel cost_insert,
  CostInsDel cost_delete,
  CostRep cost_replace,
  Score* last_score_line // length ( y.len + 1 )
  )
{
  auto last_y = y.len - 1;
  auto last_x = x.len - 1;
  auto stride = y.len + 1;
  score[ 0 + 0 * stride ] = 0;
  // PERF: make score row-major
  For( j, 0, y.len ) {
    score[ 0 + ( j + 1 ) * stride ] = score[ 0 + j * stride ] + cost_insert( y.mem[ last_y - j ] );
  }
  For( i, 1, x.len ) {
    score[ 1 + 0 * stride ] = score[ 0 + 0 * stride ] + cost_delete( x.mem[ last_x - i ] );
    For( j, 0, y.len ) {
      auto rep = score[ 0 + j * stride ] + cost_replace( x.mem[ last_x - i ], y.mem[ last_y - j ] );
      auto del = score[ 0 + ( j + 1 ) * stride ] + cost_delete( x.mem[ last_x - i ] );
      auto ins = score[ 1 + j * stride ] + cost_insert( y.mem[ last_y - j ] );
      score[ 1 + ( j + 1 ) * stride ] = MAX3( rep, del, ins );
    }
    // copies col 1 to col 0
    // PERF: just pointer swap / index swap?
    For( j, 0, y.len + 1 ) {
      score[ 0 + j * stride ] = score[ 1 + j * stride ];
    }
  }
  // PERF: elide this copy
  For( j, 0, y.len + 1 ) {
    last_score_line[j] = score[ 1 + j * stride ];
  }
}
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
    NeedlemanWunschScore( x, y, score, cost_insert, cost_delete, cost_replace, buffer );
    auto i = x.len;
    auto j = y.len;
    auto stride = y.len + 1;
    Forever {
      if( !( i | j ) ) break;
      auto i_pos = i != 0;
      auto j_pos = j != 0;
      auto score_ij = score[ i + j * stride ];
      if( i_pos  &&  j_pos ) {
        // PERF: is it better to check for del and ins, rather than rep? probably for instruction count.
        //   which two are cheapest to check for memory wise? rep and ins are contiguous with row major.
        auto score_rep = score[ i + j * stride ];
        auto score_potential = score_rep + cost_replace( x.mem[i], y.mem[j] );
        if( score_ij == score_potential ) {
          // note we use i to index z and w, because we know x is larger than y, and so don't need to do MAX.
          i -= 1;
          j -= 1;
          z.mem[i] = x.mem[i];
          w.mem[i] = y.mem[j];
          // PERF: we could enter a simpler loop here, knowing j == 0.
          continue;
        }
      }
      if( i_pos ) {
        auto score_del = score[ ( i - 1 ) + j * stride ];
        auto score_potential = score_del + cost_delete( x.mem[i] );
        if( score_ij == score_potential ) {
          // note we use i to index z and w, because we know x is larger than y, and so don't need to do MAX.
          i -= 1;
          z.mem[i] = x.mem[i];
          w.mem[i] = invalid_constant;
          continue;
        }
      }

      // note we use 0 to index z and w, because we know we're done here.
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
  NeedlemanWunschScore( x_left, y, score, cost_insert, cost_delete, cost_replace, score_l );
  NeedlemanWunschScoreReverse( x_rght, y, score, cost_insert, cost_delete, cost_replace, score_r );
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


struct
test_bool_void_t
{
  bool b;
  void* p;
};

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
TestString()
{
  idx_t indices[] = {
    10, 5, 7, 8, 1, 3, 5, 5000, 1221, 200, 0, 20,
  };
  u8 values[] = {
    0, 1, 2, 255, 128, 50, 254, 253, 3, 100, 200, 222,
  };
  AssertCrash( _countof( indices ) == _countof( values ) );
  idx_t fill_count = _countof( indices );

  idx_t sizes[] = {
    1, 2, 3, 4, 5, 8, 10, 16, 24, 1024, 65536, 100000,
  };
  ForEach( size, sizes ) {
    auto str = AllocString<u8, allocator_heap_t, allocation_heap_t>( size );
    For( i, 0, fill_count ) {
      auto idx = indices[i];
      auto value = values[i];
      if( idx < size ) {
        str.mem[idx] = value;
        AssertCrash( str.mem[idx] == value );
      }
    }

    test_bool_void_t testcases[] = {
      { 0, str.mem - 5000 },
      { 0, str.mem - 50 },
      { 0, str.mem - 2 },
      { 0, str.mem - 1 },
      { 1, str.mem + 0 },
      { 1, str.mem + str.len / 2 },
      { 1, str.mem + str.len - 1 },
      { 0, str.mem + str.len + 0 },
      { 0, str.mem + str.len + 1 },
      { 0, str.mem + str.len + 2 },
      { 0, str.mem + str.len + 200 },
    };
    ForEach( testcase, testcases ) {
      AssertCrash( testcase.b == PtrInsideMem( str, testcase.p ) );
    }

    idx_t onesize = str.len;
    idx_t twosize = 2 * str.len;
    ExpandTo( str, twosize );
    AssertCrash( str.len == twosize );
    ShrinkTo( str, onesize );
    AssertCrash( str.len == onesize );

    Reserve( str, 0 );
    AssertCrash( str.len == onesize );
    Reserve( str, 1 );
    AssertCrash( str.len == onesize );
    Reserve( str, str.len - 1 );
    AssertCrash( str.len == onesize );
    Reserve( str, str.len + 0 );
    AssertCrash( str.len == onesize );
    Reserve( str, str.len + 1 );
    AssertCrash( str.len == twosize );
    Reserve( str, str.len + 1 );
    AssertCrash( str.len == 4 * onesize );

    Free( str );
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
      auto score = AllocString<int, allocator_heap_t, allocation_heap_t>( 2 * ( y.len + 1 ) );
      auto buffer = AllocString<int, allocator_heap_t, allocation_heap_t>( y.len + 1 );
      auto buffer2 = AllocString<int, allocator_heap_t, allocation_heap_t>( y.len + 1 );
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
