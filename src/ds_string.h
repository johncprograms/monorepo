// Copyright (c) John A. Carlos Jr., all rights reserved.

Enumc( alloctype_t )
{
  no_alloc,
  crt_heap,
  virtualalloc,
};

// arbitrary-length contiguous string of memory.
Templ struct
tstring_t
{
  T* mem;
  idx_t len; // # of bytes mem can possibly hold.
  alloctype_t alloctype;
};

using string_t = tstring_t<u8>;


Templ Inl void
Zero( tstring_t<T>& dst )
{
  dst.mem = 0;
  dst.len = 0;
  dst.alloctype = alloctype_t::no_alloc;
}
Templ Inl void
ZeroContents( tstring_t<T>& dst )
{
  Memzero( dst.mem, dst.len * sizeof( T ) );
}

Templ Inl tslice_t<T>
SliceFromString( tstring_t<T>& str )
{
  tslice_t<T> r;
  r.mem = str.mem;
  r.len = str.len;
  return r;
}

Templ Inl tslice32_t<T>
Slice32FromString( tstring_t<T>& str )
{
  tslice32_t<T> r;
  r.mem = str.mem;
  AssertCrash( str.len <= MAX_u32 );
  r.len = Cast( u32, str.len );
  return r;
}


constant idx_t c_virtualalloc_threshold = 100*1024*1024;

Templ Inl void
Alloc( tstring_t<T>& dst, idx_t len )
{
  dst.len = len;
  auto nbytes = len * sizeof( T );
  if( nbytes >= c_virtualalloc_threshold ) {
    dst.alloctype = alloctype_t::virtualalloc;
    dst.mem = Cast( T*, MemVirtualAlloc( u8, nbytes ) );
  }
  else {
    dst.alloctype = alloctype_t::crt_heap;
    dst.mem = Cast( T*, MemHeapAlloc( u8, nbytes ) );
  }
}

Templ Inl void
Free( tstring_t<T>& dst )
{
  if( dst.mem ) {
    switch( dst.alloctype ) {
      case alloctype_t::crt_heap:
        MemHeapFree( dst.mem );
        break;

      case alloctype_t::virtualalloc:
        MemVirtualFree( dst.mem );
        break;

      case alloctype_t::no_alloc:
        break;

      default: UnreachableCrash();
    }
  }
  Zero( dst );
}


Templ Inl void
ExpandTo( tstring_t<T>& dst, idx_t len_new )
{
  AssertCrash( len_new > dst.len );
  auto nbytes_old = dst.len * sizeof( T );
  auto nbytes_new = len_new * sizeof( T );
  switch( dst.alloctype ) {
    case alloctype_t::crt_heap:
      dst.mem = MemHeapRealloc( u8, dst.mem, nbytes_old, nbytes_new );
      break;

    case alloctype_t::virtualalloc:
      dst.mem = MemVirtualRealloc( u8, dst.mem, nbytes_old, nbytes_new );
      break;

    default: UnreachableCrash();
  }
  dst.len = len_new;
}

Templ Inl void
ShrinkTo( tstring_t<T>& dst, idx_t len_new )
{
  AssertCrash( len_new < dst.len );
  AssertCrash( len_new > 0 );
  auto nbytes_old = dst.len * sizeof( T );
  auto nbytes_new = len_new * sizeof( T );
  switch( dst.alloctype ) {
    case alloctype_t::crt_heap:
      dst.mem = MemHeapRealloc( u8, dst.mem, nbytes_old, nbytes_new );
      break;

    case alloctype_t::virtualalloc:
      dst.mem = MemVirtualRealloc( u8, dst.mem, nbytes_old, nbytes_new );
      break;

    default: UnreachableCrash();
  }
  dst.len = len_new;
}


Templ Inl void
Reserve( tstring_t<T>& dst, idx_t enforce_capacity )
{
  AssertCrash( dst.len );
  if( dst.len < enforce_capacity ) {
    auto len_new = 2 * dst.len;
    while( len_new < enforce_capacity ) {
      len_new *= 2;
    }
    ExpandTo( dst, len_new );
  }
}


Templ Inl bool
PtrInsideMem( tstring_t<T>& str, void* ptr )
{
  AssertCrash( str.mem );
  AssertCrash( str.len );

  void* start = str.mem;
  void* end = str.mem + str.len;
  bool inside = ( start <= ptr )  &&  ( ptr < end );
  return inside;
}

Templ Inl bool
Equal( tstring_t<T>& a, tstring_t<T>& b )
{
  return MemEqual( ML( a ), ML( b ) );
}




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
  auto zw_rght_len = MAX( x_rght.len, y_rght.len );
  tslice_t<Char> z_left = { z.mem, zw_left_len };
  tslice_t<Char> w_left = { w.mem, zw_left_len };
  Hirschberg( x_left, y_left, invalid_constant, z_left, w_left, score, cost_insert, cost_delete, cost_replace, buffer, buffer2 );
  tslice_t<Char> z_rght = { z.mem + zw_left_len, z.len - zw_left_len };
  tslice_t<Char> w_rght = { w.mem + zw_left_len, w.len - zw_left_len };
  Hirschberg( x_rght, y_rght, invalid_constant, z_rght, w_rght, score, cost_insert, cost_delete, cost_replace, buffer, buffer2 );
}

