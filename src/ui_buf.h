// Copyright (c) John A. Carlos Jr., all rights reserved.


// this is the equivalent of an idx_t offset for a contiguous array.
// the complexity is here to support arbitrary-sized paging, which we rely on for quick insert/delete.
//
// we used to have a public idx_t offset, and then internally cache transformations to this kind of page ptr.
// that was getting kind of slow for the content-dependent cursor move functions ( e.g. CursorLineD ).
// so we switched to this, and are still feeling it out.
// it's definitely less convenient / more complicated, since you now have to pass in concurrent ptrs.
// i'm still on the fence about this, whether it's better or not.
//
struct
content_ptr_t
{
  idx_t diff_idx;
  idx_t offset_into_diff;
};

Inl bool
Equal( content_ptr_t a, content_ptr_t b )
{
  bool r =
    ( a.diff_idx == b.diff_idx )  &&
    ( a.offset_into_diff == b.offset_into_diff );
  return r;
}

Inl bool
LEqual( content_ptr_t a, content_ptr_t b )
{
  bool r =
    ( a.diff_idx < b.diff_idx ) ||
    ( a.diff_idx == b.diff_idx  &&  a.offset_into_diff <= b.offset_into_diff );
  return r;
}

Inl bool
GEqual( content_ptr_t a, content_ptr_t b )
{
  // a >= b
  // b <= a
  return LEqual( b, a );
}

Inl bool
Less( content_ptr_t a, content_ptr_t b )
{
  // a < b
  // !( a >= b )
  // !( b <= a )
  return !LEqual( b, a );
}

Inl bool
Greater( content_ptr_t a, content_ptr_t b )
{
  // a > b
  // !( a <= b )
  return !LEqual( a, b );
}

Inl content_ptr_t
Max( content_ptr_t a, content_ptr_t b )
{
  if( a.diff_idx == b.diff_idx ) {
    a.offset_into_diff = MAX( a.offset_into_diff, b.offset_into_diff );
    return a;
  } elif( a.diff_idx < b.diff_idx ) {
    return b;
  } else {
    return a;
  }
}

Inl content_ptr_t
Max3( content_ptr_t a, content_ptr_t b, content_ptr_t c )
{
  return Max( Max( a, b ), c );
}

Inl content_ptr_t
Min( content_ptr_t a, content_ptr_t b )
{
  if( a.diff_idx == b.diff_idx ) {
    a.offset_into_diff = MIN( a.offset_into_diff, b.offset_into_diff );
    return a;
  } elif( a.diff_idx < b.diff_idx ) {
    return a;
  } else {
    return b;
  }
}

Inl content_ptr_t
Min3( content_ptr_t a, content_ptr_t b, content_ptr_t c )
{
  return Min( Min( a, b ), c );
}




struct
diff_t
{
  slice_t slice;

#if LNCACHE
  // we maintain all the beginning-of-line positions, at all times.
  // this is a sorted list, so getting the i'th element corresponds to the i'th sequential line.
  // we'll update these during insert/delete/etc. operations.
  // PERF: use stack_resizeable_pagelist_t / skipagelist_t if this is too slow!
  // PERF: u32 ?
  stack_resizeable_cont_t<idx_t> ln_starts;
#endif
};

Inl void
Kill( diff_t& diff )
{
  diff.slice = {};
#if LNCACHE
  Free( diff.ln_starts );
#endif
}

Inl bool
Equal( diff_t& a, diff_t& b )
{
  bool r =
    ( a.slice.mem == b.slice.mem )  &&
    ( a.slice.len == b.slice.len );
  return r;
}

Inl void
_AllocAndFillLnStarts( diff_t& diff )
{
#if LNCACHE
  idx_t count = 0;
  For( j, 0, diff.slice.len ) {
    if( diff.slice.mem[j] == '\n' ) {
      count += 1;
    }
  }
  Alloc( diff.ln_starts, MAX( 8, count ) );
  For( j, 0, diff.slice.len ) {
    if( diff.slice.mem[j] == '\n' ) {
      *AddBack( diff.ln_starts ) = j;
    }
  }
#endif
}



Enumc( undoableopertype_t )
{
  checkpt,
  add, // new diff stored in diff_new
  mod, // change stored in diff_old -> diff_new
  rem, // old diff stored in diff_old
};

struct
undoableoper_t
{
  undoableopertype_t type;
  diff_t diff_old;
  diff_t diff_new;
  idx_t idx; // in buf_t.diffs
};


Enumc( loadstate_t )
{
  none,
  file,
  emptyfile,
};


struct
buf_t
{
  stack_resizeable_cont_t<diff_t> diffs;
  pagelist_t pagelist;
  idx_t content_len;
  stack_resizeable_cont_t<undoableoper_t> history;
  idx_t history_idx;

  // external users of the buf_t can attach their persistent content_ptrs here.
  // we'll update these content_ptrs during insert/delete/etc. operations.
//  stack_resizeable_cont_t<content_ptr_t> user_ptrs; // TODO: replace txt content tracking with this ?
};



// =================================================================================
// FIRST / LAST CALLS
//

Inl void
Zero( buf_t& buf )
{
  Zero( buf.pagelist );
  Zero( buf.diffs );
  buf.content_len = 0;
  Zero( buf.history );
  buf.history_idx = 0;
}

Inl void
Init( buf_t& buf )
{
  Zero( buf );
}

Inl void
Kill( buf_t& buf )
{
  ForLen( i, buf.diffs ) {
    auto diff = buf.diffs.mem + i;
    Kill( *diff );
  }
  Free( buf.diffs );
  Kill( buf.pagelist );
  Free( buf.history );

  Zero( buf );
}



// =================================================================================
// CONTENT READ CALLS
//

Inl content_ptr_t
GetBOF( buf_t& buf )
{
  content_ptr_t r;
  r.diff_idx = 0;
  r.offset_into_diff = 0;
  return r;
}

Inl content_ptr_t
GetEOF( buf_t& buf )
{
  content_ptr_t r;
  r.diff_idx = buf.diffs.len;
  r.offset_into_diff = 0;
  return r;
}



Inl bool
IsBOF( buf_t& buf, content_ptr_t pos )
{
  auto r = !pos.diff_idx  &&  !pos.offset_into_diff;
  return r;
}

Inl bool
IsEOF( buf_t& buf, content_ptr_t pos )
{
  auto r = pos.diff_idx >= buf.diffs.len;
  return r;
}



// returns the position past the end of what you read.
// since we're already iterating the diff list, may as well give that final position back.
Inl content_ptr_t
Contents(
  buf_t& buf,
  content_ptr_t start,
  u8* str,
  idx_t len
  )
{
  if( !buf.content_len || !len ) {
    return start;
  }

  AssertCrash( str );
  AssertCrash( start.diff_idx < buf.diffs.len );

  auto x = start;
  Forever {
    if( !len ) {
      break;
    }
    if( x.diff_idx >= buf.diffs.len ) {
      break;
    }
    auto diff = buf.diffs.mem + x.diff_idx;
    AssertCrash( x.offset_into_diff < diff->slice.len );
    auto rem = diff->slice.len - x.offset_into_diff;
    auto copy_done_with_this_diff = len < rem;
    auto ncopy = MIN( len, rem );
    Memmove( str, diff->slice.mem + x.offset_into_diff, ncopy );
    str += ncopy;
    len -= ncopy;

    if( !len  &&  copy_done_with_this_diff ) {
      x.offset_into_diff += ncopy;
      break;
    }

    x.diff_idx += 1;
    x.offset_into_diff = 0;
  }

  return x;
}

Inl idx_t
Contents(
  buf_t& buf,
  content_ptr_t start,
  content_ptr_t end,
  u8* str,
  idx_t len
  )
{
  if( !buf.content_len || !len ) {
    return 0;
  }

  AssertCrash( str );
  AssertCrash( start.diff_idx < buf.diffs.len );

  idx_t ncopied = 0;
  auto x = start;
  Forever {
    if( !len ) {
      break;
    }
    if( x.diff_idx >= buf.diffs.len ) {
      break;
    }
    auto diff = buf.diffs.mem + x.diff_idx;
    AssertCrash( x.offset_into_diff < diff->slice.len );
    auto ncopy = MIN( len, diff->slice.len - x.offset_into_diff );
    AssertCrash( ncopy <= len ); // caller must supply a buffer big enough to receive all contents.
    Memmove( str + ncopied, diff->slice.mem + x.offset_into_diff, ncopy );
    ncopied += ncopy;
    len -= ncopy;

    x.diff_idx += 1;
    x.offset_into_diff = 0;
  }

  return ncopied;
}


bool
Equal(
  buf_t& buf,
  content_ptr_t start,
  u8* __restrict str,
  idx_t len,
  bool case_sensitive
  )
{
  AssertCrash( start.diff_idx < buf.diffs.len );

  auto len_orig = len;
  idx_t ncompared = 0;
  auto diff_idx = start.diff_idx;
  auto offset_into_diff = start.offset_into_diff;
  Forever {
    if( !len ) {
      break;
    }
    if( diff_idx >= buf.diffs.len ) {
      break;
    }
    diff_t* __restrict diff = buf.diffs.mem + diff_idx;
    u8* __restrict slice_mem = diff->slice.mem;
    auto slice_len = diff->slice.len;

    AssertCrash( offset_into_diff < slice_len );
    auto ncompare = MIN( len, slice_len - offset_into_diff );
    AssertCrash( ncompare <= len ); // caller must supply a buffer big enough to receive all contents.

    u8* __restrict curr = slice_mem + offset_into_diff;
    u8* __restrict curr_end = curr + ncompare;

    if( case_sensitive ) {
      while( curr < curr_end ) {
        if( *curr != *str ) {
          return 0;
        }
        curr += 1;
        str += 1;
      }
    } else {
      while( curr < curr_end ) {
        if( CsToLower( *curr ) != CsToLower( *str ) ) {
          return 0;
        }
        curr += 1;
        str += 1;
      }
    }

    ncompared += ncompare;
    len -= ncompare;

    diff_idx += 1;
    offset_into_diff = 0;
  }

  AssertCrash( ncompared <= len_orig );
  return ncompared == len_orig;
}



// =================================================================================
// FILESYS INTERFACE CALLS
//

void
BufLoadEmpty( buf_t& buf )
{
  ProfFunc();

  Init( buf.pagelist, 256 );

  Alloc( buf.diffs, 8 );

  buf.content_len = 0;

  Alloc( buf.history, 8 );
  buf.history_idx = 0;
}


void
BufLoad( buf_t& buf, file_t& file )
{
  ProfFunc();
  AssertCrash( file.size < MAX_idx );

  constant u64 c_chunk_size = 200*1024*1024 - 256; // leave a little space for pagelistheader_t
  Init( buf.pagelist, CLAMP( Cast( idx_t, file.size ), 256, c_chunk_size ) );

  Alloc( buf.diffs, 256 );

  if( file.size ) {
    u64 nchunks = file.size / c_chunk_size;
    u64 nremain = file.size % c_chunk_size;

    Fori( u64, i, 0, nchunks + 1 ) {
      auto chunk_size = ( i == nchunks )  ?  nremain  :  c_chunk_size;
      if( chunk_size ) {
        auto diff = AddBack( buf.diffs );
        diff->slice.len = Cast( idx_t, chunk_size );
        diff->slice.mem = AddPagelist( buf.pagelist, u8, 1, diff->slice.len );
        FileRead( file, i * c_chunk_size, ML( diff->slice ), Cast( u32, chunk_size ) );
        _AllocAndFillLnStarts( *diff );
      }
    }
  }

  buf.content_len = Cast( idx_t, file.size );

  Alloc( buf.history, 256 );
  buf.history_idx = 0;
}

#if USE_FILEMAPPED_OPEN
  void
  BufLoad( buf_t& buf, filemapped_t& file )
  {
    ProfFunc();

  }
#endif

void
BufSave( buf_t& buf, file_t& file )
{
  ProfFunc();

  // PERF: we shouldn't bother copying chunks that are already appropriately sized.
  // i.e. if a diff is already a reasonable medium size, we shouldn't copy it for chunking.
  constant idx_t c_chunk_size = 200*1024*1024;
  string_t chunk;
  Alloc( chunk, MIN( buf.content_len, c_chunk_size ) );

  idx_t contentlen = buf.content_len;
  idx_t nchunks = contentlen / c_chunk_size;
  idx_t nrem    = contentlen % c_chunk_size;

  idx_t bytepos = 0;
  content_ptr_t bufpos = GetBOF( buf );
  For( i, 0, nchunks + 1 ) {
    auto chunk_size = ( i == nchunks )  ?  nrem  :  c_chunk_size;
    if( chunk_size ) {
      bufpos = Contents( buf, bufpos, chunk.mem, chunk_size );
      FileWrite( file, bytepos, chunk.mem, chunk_size );
      bytepos += chunk_size;
    }
  }
  FileSetEOF( file, contentlen );

  Free( chunk );
}



// =================================================================================
// CONTENT-INDEPENDENT NAVIGATION CALLS
//   these only depend on file size.
//

Inl content_ptr_t
CursorCharL( buf_t& buf, content_ptr_t pos, idx_t len, idx_t* nchars_moved )
{
  if( !IsEOF( buf, pos ) ) {
    // since it can be easy to introduce an invalid content_ptr_t bug, we check here, in a common function.
    // look upstream for who created the 'pos' passed in here; they've made an invalid content_ptr_t!
    auto diff = buf.diffs.mem + pos.diff_idx;
    AssertCrash( pos.offset_into_diff < diff->slice.len );
  }

  idx_t nmoved = 0;
  Forever {
    if( !len  ||  IsBOF( buf, pos ) ) {
      break;
    }

    if( len <= pos.offset_into_diff ) {
      pos.offset_into_diff -= len;
      nmoved += len;
      break;

    } elif( pos.diff_idx ) {
      len -= pos.offset_into_diff + 1;
      nmoved += pos.offset_into_diff + 1;
      pos.diff_idx -= 1;
      auto diff = buf.diffs.mem + pos.diff_idx;
      AssertCrash( diff->slice.len );
      pos.offset_into_diff = diff->slice.len - 1;

    } else {
      // already at BOF
      break;
    }
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return pos;
}

Inl content_ptr_t
CursorCharR( buf_t& buf, content_ptr_t pos, idx_t len, idx_t* nchars_moved )
{
  if( !IsEOF( buf, pos ) ) {
    // since it can be easy to introduce an invalid content_ptr_t bug, we check here, in a common function.
    // look upstream for who created the 'pos' passed in here; they've made an invalid content_ptr_t!
    auto diff = buf.diffs.mem + pos.diff_idx;
    AssertCrash( pos.offset_into_diff < diff->slice.len );
  }

  idx_t nmoved = 0;
  Forever {
    if( !len  ||  IsEOF( buf, pos ) ) {
      break;
    }
    AssertCrash( pos.diff_idx < buf.diffs.len );
    auto diff = buf.diffs.mem + pos.diff_idx;
    AssertCrash( pos.offset_into_diff <= diff->slice.len );
    if( pos.offset_into_diff + len < diff->slice.len ) {
      pos.offset_into_diff += len;
      nmoved += len;
      break;

    } elif( pos.diff_idx + 1 <= buf.diffs.len ) {
      len -= diff->slice.len - pos.offset_into_diff;
      nmoved += diff->slice.len - pos.offset_into_diff;
      pos.diff_idx += 1;
      pos.offset_into_diff = 0;

    } else {
      // already at EOF
      break;
    }
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return pos;
}



// =================================================================================
// CONTENT SEARCH CALLS
//

#if 1

void
FindFirstR(
  buf_t& buf,
  content_ptr_t start,
  u8* str,
  idx_t len,
  content_ptr_t* r,
  bool* found,
  bool case_sensitive,
  bool word_boundary
  )
{
  ProfFunc();

  // XXXXXXXXXXXXXXXXXXXXXX
  // XXXXXXXXXXXXXXXXXXXXXX
  // TODO: PERF: This is wicked slow; just way too much overhead for each byte.
  // XXXXXXXXXXXXXXXXXXXXXX
  // XXXXXXXXXXXXXXXXXXXXXX
  // try finding the firstchar/firstchar match quickly first, and then iterate on failure.
  // hopefully we can cut down loopiter overhead to the bare minimum.
  //
  // we should definitely split to different routines for case_sensitive, word_boundary options.
  // the toolchain isn't smart enough to pull those up out of the loops, so we're wasting time.

  *found = 0;

  if( !buf.content_len || !len ) {
    return;
  }
  if( IsEOF( buf, start ) ) {
    return;
  }

  AssertCrash( str );

  auto firstchar = str[0];
  auto firstchar_lower = CsToLower( firstchar );
  diff_t* __restrict diff_end = buf.diffs.mem + buf.diffs.len;
  diff_t* __restrict diff = buf.diffs.mem + start.diff_idx;
  auto offset_into_diff = start.offset_into_diff;
  idx_t foundoffset = 0;

  while( diff < diff_end ) {

    // quickly iterate, looking for a firstchar match.
    bool foundfirst = 0;
    while( diff < diff_end ) {
      u8* __restrict slice_mem = diff->slice.mem;
      auto slice_len = diff->slice.len;

      u8* __restrict curr = slice_mem + offset_into_diff;
      u8* __restrict curr_end = slice_mem + slice_len;

      if( case_sensitive ) {
        while( curr < curr_end ) {
          if( *curr == firstchar ) {
            foundfirst = 1;
            foundoffset = curr - slice_mem;
            break;
          }
          curr += 1;
        }
      }
      else
      {
        while( curr < curr_end ) {
          if( CsToLower( *curr ) == firstchar_lower ) {
            foundfirst = 1;
            foundoffset = curr - slice_mem;
            break;
          }
          curr += 1;
        }
      }

      if( foundfirst ) {
        break;
      }

      offset_into_diff = 0;
      diff += 1;
    }

    // unable to find a firstchar match.
    if( !foundfirst ) {
      return;
    }

    content_ptr_t match_start;
    match_start.diff_idx = diff - buf.diffs.mem;
    match_start.offset_into_diff = foundoffset;

    auto next = CursorCharR( buf, match_start, 1, 0 );

    // now we have a firstchar match; verify the rest also matches.
    if( len > 1 ) {

      // not enough file left to complete our rest match.
      if( IsEOF( buf, next ) ) {
        diff = buf.diffs.mem + next.diff_idx;
        offset_into_diff = next.offset_into_diff;
        continue;
      }

      bool rest_matches = Equal(
        buf,
        next,
        str + 1,
        len - 1,
        case_sensitive
        );
      if( !rest_matches ) {
        diff = buf.diffs.mem + next.diff_idx;
        offset_into_diff = next.offset_into_diff;
        continue;
      }
    }

    // now we have a fullstring match!

    // eliminate the fullstring match, if it fails the word_boundary test.
    // if we're searching "0123456789" for "234", the test is:
    //   keep-match <=> AsciiInWord( "1" ) != AsciiInWord( "2" ) && AsciiInWord( "4" ) != AsciiInWord( "5" )
    // i.e. the first and last chars in the match have to have different "wordiness" than the chars outside.
    //
    if( word_boundary ) {
      idx_t nmoved;
      auto before_match_start = CursorCharL( buf, match_start, 1, &nmoved );
      if( nmoved ) {
        AssertCrash( before_match_start.diff_idx < buf.diffs.len );
        auto diff_before_match_start = buf.diffs.mem + before_match_start.diff_idx;
        AssertCrash( before_match_start.offset_into_diff < diff_before_match_start->slice.len );
        u8 c = diff_before_match_start->slice.mem[ before_match_start.offset_into_diff ];
        if( AsciiInWord( c ) == AsciiInWord( firstchar ) ) {
          diff = buf.diffs.mem + next.diff_idx;
          offset_into_diff = next.offset_into_diff;
          continue;
        }
      }
      auto after_match = CursorCharR( buf, match_start, len, &nmoved );
      if( nmoved == len ) {
        AssertCrash( after_match.diff_idx < buf.diffs.len );
        auto diff_before_x = buf.diffs.mem + after_match.diff_idx;
        AssertCrash( after_match.offset_into_diff < diff_before_x->slice.len );
        u8 c = diff_before_x->slice.mem[ after_match.offset_into_diff ];
        if( AsciiInWord( c ) == AsciiInWord( str[ len - 1 ] ) ) {
          diff = buf.diffs.mem + next.diff_idx;
          offset_into_diff = next.offset_into_diff;
          continue;
        }
      }
    }

    // our fullstring match passed the word_boundary test!
    *found = 1;
    *r = match_start;
    return;
  }


#if 0


  // PERF: is it faster to just copy chunks, and do linear search on those chunks?

  idx_t nmatched = 0;
  auto x = start;
  auto match_start = start;
  while( x.diff_idx < buf.diffs.len ) {

    auto diff = buf.diffs.mem + x.diff_idx;
    auto slice_mem = diff->slice.mem;
    auto slice_len = diff->slice.len;
    auto curr = slice_mem;

    For( i, x.offset_into_diff, slice_len ) {
      u8 match = str[ nmatched ];
      bool matched =
        ( case_sensitive  &&  *curr == match )  ||
        ( !case_sensitive  &&  CsToLower( *curr ) == CsToLower( match ) );
      if( matched ) {
        if( !nmatched ) {
          match_start = x;
        }
        nmatched += 1;
      } else {
        if( nmatched ) {
          nmatched = 0;
          x = match_start;
        }
      }

      curr += 1;

      if( nmatched == len ) {
        if( word_boundary ) {
          idx_t nmoved;
          auto before_match_start = CursorCharL( buf, match_start, 1, &nmoved );
          if( nmoved ) {
            AssertCrash( before_match_start.diff_idx < buf.diffs.len );
            auto diff_before_match_start = buf.diffs.mem + before_match_start.diff_idx;
            AssertCrash( before_match_start.offset_into_diff < diff_before_match_start->slice.len );
            u8 c = diff_before_match_start->slice.mem[ before_match_start.offset_into_diff ];
            // TODO: option to compare AsciiInWord( before_match_start ) to AsciiInWord( match_start )
            if( AsciiInWord( c ) ) {
              x = match_start;
              nmatched = 0;
            }
          }
          if( nmatched ) {
            auto before_x = CursorCharL( buf, x, 1, &nmoved );
            if( nmoved ) {
              // TODO: option to compare AsciiInWord( before_x ) to AsciiInWord( x )
              AssertCrash( before_x.diff_idx < buf.diffs.len );
              auto diff_before_x = buf.diffs.mem + before_x.diff_idx;
              AssertCrash( before_x.offset_into_diff < diff_before_x->slice.len );
              u8 c = diff_before_x->slice.mem[ before_x.offset_into_diff ];
              if( AsciiInWord( c ) ) {
                // no need to check before_match_start_valid, since it will only be invalid on a match starting at BOF.
                // i.e. match_start == BOF == before_match_start.
                // that's what we want for before_x, since we're incrementing by one below.
                x = match_start;
                nmatched = 0;
              }
            }
          }
          if( nmatched ) {
            *r = match_start;
            *found = 1;
            return;
          }
        } else {
          *r = match_start;
          *found = 1;
          return;
        }
      }

      x.offset_into_diff += 1;
    }

    x.diff_idx += 1;
    x.offset_into_diff = 0;
  }

#endif

}

#else

void
FindFirstR(
  buf_t& buf,
  content_ptr_t start,
  u8* str,
  idx_t len,
  content_ptr_t* r,
  bool* found,
  bool case_sensitive,
  bool word_boundary
  )
{
  ProfFunc();

  *found = 0;

  if( !buf.content_len || !len ) {
    return;
  }
  if( IsEOF( buf, start ) ) {
    return;
  }

  AssertCrash( str );

  // PERF: is it faster to just copy chunks, and do linear search on those chunks?

  idx_t nmatched = 0;
  auto x = start;
  auto before_x = start;
  bool before_x_valid = 0;
  auto match_start = start;
  auto before_match_start = start;
  bool before_match_start_valid = 0;
  Forever {
    if( x.diff_idx >= buf.diffs.len ) {
      break;
    }

    Forever {
      auto diff = buf.diffs.mem + x.diff_idx;

      AssertCrash( x.offset_into_diff < diff->slice.len );
      u8 curr = diff->slice.mem[ x.offset_into_diff ];
      u8 match = str[ nmatched ];
      bool case_match = ( case_sensitive  &&  curr == match );
      bool ignore_match = ( !case_sensitive  &&  ( CsToLower( curr ) == CsToLower( match ) ) );
      if( case_match  ||  ignore_match ) {
        if( !nmatched ) {
          if( before_x_valid ) {
            before_match_start_valid = 1;
            before_match_start = before_x;
          }
          match_start = x;
        }
        nmatched += 1;
      } else {
        if( nmatched ) {
          nmatched = 0;
          if( before_match_start_valid ) {
            before_x_valid = 1;
            before_x = before_match_start;
          }
          x = match_start;
        }
      }

      if( nmatched == len ) {
        if( word_boundary ) {
          if( before_match_start_valid ) {
            AssertCrash( before_match_start.diff_idx < buf.diffs.len );
            auto diff_before_match_start = buf.diffs.mem + before_match_start.diff_idx;
            AssertCrash( before_match_start.offset_into_diff < diff_before_match_start->slice.len );
            u8 c = diff_before_match_start->slice.mem[ before_match_start.offset_into_diff ];
            // TODO: option to compare AsciiInWord( before_match_start ) to AsciiInWord( match_start )
            if( AsciiInWord( c ) ) {
              before_x_valid = 1;
              before_x = before_match_start;
              x = match_start;
              nmatched = 0;
            }
          }
          if( nmatched ) {
            // TODO: option to compare AsciiInWord( before_x ) to AsciiInWord( x )
            AssertCrash( before_x.diff_idx < buf.diffs.len );
            auto diff_before_x = buf.diffs.mem + before_x.diff_idx;
            AssertCrash( before_x.offset_into_diff < diff_before_x->slice.len );
            u8 c = diff_before_x->slice.mem[ before_x.offset_into_diff ];
            if( AsciiInWord( c ) ) {
              // no need to check before_match_start_valid, since it will only be invalid on a match starting at BOF.
              // i.e. match_start == BOF == before_match_start.
              // that's what we want for before_x, since we're incrementing by one below.
              before_x_valid = 1;
              before_x = before_match_start;
              x = match_start;
              nmatched = 0;
            }
          }
          if( nmatched ) {
            *r = match_start;
            *found = 1;
            return;
          }
        } else {
          *r = match_start;
          *found = 1;
          return;
        }
      }

      if( x.offset_into_diff + 1 < diff->slice.len ) {
        before_x_valid = 1;
        before_x = x;
        x.offset_into_diff += 1;
      } else {
        break;
      }
    }

    before_x_valid = 1;
    before_x = x;
    x.diff_idx += 1;
    x.offset_into_diff = 0;
  }
}

#endif


void
FindFirstL(
  buf_t& buf,
  content_ptr_t start,
  u8* str,
  idx_t len,
  content_ptr_t* l,
  bool* found,
  bool case_sensitive,
  bool word_boundary
  )
{
  ProfFunc();

  *found = 0;

  if( !buf.content_len || !len ) {
    return;
  }
  if( IsBOF( buf, start ) ) {
    return;
  }

  AssertCrash( str );

  idx_t nmatched = 0;
  auto x = start;
  auto match_end = start;
  auto before_match_end = x;

  diff_t* diff; // TODO: buggy to maintain this, as well as x. should probably just recompute as needed.
  if( x.offset_into_diff ) {
    x.offset_into_diff -= 1;
    diff = buf.diffs.mem + x.diff_idx;
  } else {
    AssertCrash( x.diff_idx );
    x.diff_idx -= 1;
    diff = buf.diffs.mem + x.diff_idx;
    x.offset_into_diff = diff->slice.len;
  }

  Forever {
    Forever {
      if( !x.offset_into_diff ) {
        break;
      }
      x.offset_into_diff -= 1;

      AssertCrash( x.offset_into_diff < diff->slice.len );
      u8 curr = diff->slice.mem[ x.offset_into_diff ];
      u8 match = str[ len - nmatched - 1 ];
      bool case_match = ( case_sensitive  &&  curr == match );
      bool ignore_match = ( !case_sensitive  &&  ( CsToLower( curr ) == CsToLower( match ) ) );
      if( case_match  ||  ignore_match ) {
        if( !nmatched ) {
          before_match_end = x;
          match_end = CursorCharR( buf, x, 1, 0 );
        }
        nmatched += 1;
      } else {
        if( nmatched ) {
          x = before_match_end;
          diff = buf.diffs.mem + x.diff_idx;
          nmatched = 0;
        }
      }

      if( nmatched == len ) {
        if( word_boundary ) {
          // TODO: option to compare AsciiInWord( before_x ) to AsciiInWord( x )
          auto before_x = CursorCharL( buf, x, 1, 0 );
          if( !Equal( before_x, x ) ) {
            AssertCrash( before_x.diff_idx < buf.diffs.len );
            auto diff_before_x = buf.diffs.mem + before_x.diff_idx;
            AssertCrash( before_x.offset_into_diff < diff_before_x->slice.len );
            u8 c = diff_before_x->slice.mem[ before_x.offset_into_diff ];
            if( AsciiInWord( c ) ) {
              x = before_match_end;
              diff = buf.diffs.mem + x.diff_idx;
              nmatched = 0;
              continue;
            }
          }
          // TODO: option to compare AsciiInWord( before_match_end ) to AsciiInWord( match_end )
          if( !IsEOF( buf, match_end ) ) {
            AssertCrash( match_end.diff_idx < buf.diffs.len );
            auto diff_match_end = buf.diffs.mem + match_end.diff_idx;
            AssertCrash( match_end.offset_into_diff < diff_match_end->slice.len );
            u8 c = diff_match_end->slice.mem[ match_end.offset_into_diff ];
            if( AsciiInWord( c ) ) {
              x = before_match_end;
              diff = buf.diffs.mem + x.diff_idx;
              nmatched = 0;
              continue;
            }
          }
        }
        *l = match_end;
        *found = 1;
        return;
      }

    }
    if( !x.diff_idx ) {
      break;
    }
    x.diff_idx -= 1;
    diff = buf.diffs.mem + x.diff_idx;
    x.offset_into_diff = diff->slice.len;
  }
}



// =================================================================================
// CONTENT MODIFY CALLS
//

Inl void
DiffInsert(
  buf_t& buf,
  idx_t idx,
  diff_t* diff
  )
{
  AssertCrash( diff->slice.len );
#if LNCACHE
  AssertCrash( diff->ln_starts.capacity );
#endif
  *AddAt( buf.diffs, idx ) = *diff;
  buf.content_len += diff->slice.len;
}

Inl void
DiffRemove(
  buf_t& buf,
  idx_t idx,
  diff_t* verify_removed
  )
{
  auto removed = buf.diffs.mem + idx;
  AssertCrash( removed->slice.len );
  AssertCrash( buf.content_len >= removed->slice.len );
  buf.content_len -= removed->slice.len;

  if( verify_removed ) {
    AssertCrash( Equal( *removed, *verify_removed ) );
  }

  Kill( *removed );
  RemAt( buf.diffs, idx );
}

Inl void
DiffReplace(
  buf_t& buf,
  idx_t idx,
  diff_t* dst_verify,
  diff_t* src
  )
{
  auto dst = buf.diffs.mem + idx;
  idx_t content_len_dst = dst->slice.len;

  if( dst_verify ) {
    AssertCrash( Equal( *dst, *dst_verify ) );
  }

  idx_t content_len_src = src->slice.len;
  Kill( *dst );
  *dst = *src;

  AssertCrash( buf.content_len + content_len_src >= content_len_dst );
  buf.content_len += content_len_src;
  buf.content_len -= content_len_dst;
}


Inl void
AddHistorical( buf_t& buf, undoableoper_t& undoable )
{
  // invalidate previous futures.
  AssertCrash( buf.history_idx <= buf.history.len );
  buf.history.len = buf.history_idx;
  *AddBack( buf.history ) = undoable;
  buf.history_idx += 1;
}

Inl void
WipeHistories( buf_t& buf )
{
  AssertCrash( buf.history_idx <= buf.history.len );
  buf.history.len = 0;
  buf.history_idx = 0;
}

Inl void
ForwardDiffOper(
  buf_t& buf,
  idx_t diff_idx,
  diff_t* diff_old,
  diff_t* diff_new,
  undoableopertype_t type
  )
{
  undoableoper_t oper;
  oper.type = type;
  if( diff_old ) {
    oper.diff_old.slice = diff_old->slice;
#if LNCACHE
    auto ncopy = diff_old->ln_starts.len;
    if( ncopy ) {
      Alloc( oper.diff_old.ln_starts, ncopy );
      Copy( oper.diff_old.ln_starts, diff_old->ln_starts );
    } else {
      Alloc( oper.diff_old.ln_starts, diff_old->ln_starts.capacity );
    }
#endif
  } else {
    oper.diff_old.slice = {};
#if LNCACHE
    Zero( oper.diff_old.ln_starts );
#endif
  }
  if( diff_new ) {
    oper.diff_new.slice = diff_new->slice;
#if LNCACHE
    auto ncopy = diff_new->ln_starts.len;
    if( ncopy ) {
      Alloc( oper.diff_new.ln_starts, ncopy );
      Copy( oper.diff_new.ln_starts, diff_new->ln_starts );
    } else {
      Alloc( oper.diff_new.ln_starts, diff_new->ln_starts.capacity );
    }
#endif
  } else {
    oper.diff_new.slice = {};
#if LNCACHE
    Zero( oper.diff_new.ln_starts );
#endif
  }
  oper.idx = diff_idx;
  AddHistorical( buf, oper );

  // actually perform the operation:

  switch( type ) {
    case undoableopertype_t::add: {
      DiffInsert( buf, diff_idx, diff_new );
    } break;
    case undoableopertype_t::mod: {
      DiffReplace( buf, diff_idx, diff_old, diff_new );
    } break;
    case undoableopertype_t::rem: {
      DiffRemove( buf, diff_idx, diff_old );
    } break;
    case undoableopertype_t::checkpt: __fallthrough;
    default: UnreachableCrash();
  }
}


//
// split the pos.diff_idx diff at pos.offset_into_diff
//       [         diff         ]
//       [   left   |   right   ]
// returns a content_ptr_t to `right`
//
Inl content_ptr_t
_SplitDiffAt(
  buf_t& buf,
  content_ptr_t pos,
  content_ptr_t** concurrents,
  idx_t concurrents_len
  )
{
  AssertCrash( pos.diff_idx < buf.diffs.len );
  auto diff = buf.diffs.mem + pos.diff_idx;

  auto add_diff_idx = pos.diff_idx + 1;
  diff_t right;
  right.slice.mem = diff->slice.mem + pos.offset_into_diff;
  right.slice.len = diff->slice.len - pos.offset_into_diff;
  AssertCrash( right.slice.len );
  _AllocAndFillLnStarts( right );
  ForwardDiffOper(
    buf,
    add_diff_idx,
    0,
    &right,
    undoableopertype_t::add
    );

  // since buf.diffs is an stack_resizeable_cont_t, we can realloc after the add.
  // so re-query diff.
  diff = buf.diffs.mem + pos.diff_idx;

  diff_t left;
  left.slice.mem = diff->slice.mem;
  left.slice.len = pos.offset_into_diff;
  AssertCrash( left.slice.len );
  _AllocAndFillLnStarts( left );
  ForwardDiffOper(
    buf,
    pos.diff_idx,
    diff,
    &left,
    undoableopertype_t::mod
    );

  // update any given ptrs to the same actual contents, after the split.
  // this is necessary for copy / move / swap, which require multiple concurrent ptrs.
  For( i, 0, concurrents_len ) {
    auto concurrent = concurrents[i];
    if( concurrent->diff_idx == pos.diff_idx ) {
      if( concurrent->offset_into_diff >= left.slice.len ) {
        concurrent->offset_into_diff -= left.slice.len;
        concurrent->diff_idx += 1;
        AssertCrash( concurrent->offset_into_diff < right.slice.len );
      }
    } elif( concurrent->diff_idx >= add_diff_idx ) {
      concurrent->diff_idx += 1;
    }
  }

  content_ptr_t r;
  r.diff_idx = add_diff_idx;
  r.offset_into_diff = 0;
  return r;
}


//
// insert contents at the given position.
// e.g. Insert( "0123456789", 4, "asdf" )' -> "0123asdf456789"
//
// here's what we do to all elements in the 'concurrents' list:
//
//   pointers BEFORE the inserted section remain unchanged.
//      [aaabbbb]
//        ^
//      [aaaXXXXbbbb]
//        ^
//
//   pointers AFTER the inserted section move right, to point to the same content as they did before.
//      [aaabbbb]
//           ^
//      [aaaXXXXbbbb]
//               ^
//
content_ptr_t
Insert(
  buf_t& buf,
  content_ptr_t pos,
  u8* str,
  idx_t len,
  content_ptr_t** concurrents,
  idx_t concurrents_len
  )
{
  AssertCrash( pos.diff_idx <= buf.diffs.len );

  if( !len ) {
    return pos;
  }

  if( pos.offset_into_diff ) {
    pos = _SplitDiffAt( buf, pos, concurrents, concurrents_len );
  }

  // ensure maximum-size chunks, so our per-chunk datastructures don't get too huge individually.
  constant idx_t c_chunk_size = 2*1024*1024 - 256; // leave a little space for pagelistheader_t
  idx_t nchunks = len / c_chunk_size;
  idx_t nremain = len % c_chunk_size;

  content_ptr_t end;
  end.diff_idx = pos.diff_idx + nchunks + 1;
  end.offset_into_diff = 0;

  // insert N new diffs at diff_idx.
  For( i, 0, nchunks + 1 ) {
    auto chunk_size = ( i == nchunks )  ?  nremain  :  c_chunk_size;
    auto add_diff_idx = pos.diff_idx + i;
    if( chunk_size ) {
      diff_t add;
      add.slice.len = chunk_size;
      add.slice.mem = AddPagelist( buf.pagelist, u8, 1, add.slice.len );
      AssertCrash( add.slice.len );
      Memmove( add.slice.mem, str + i * c_chunk_size, add.slice.len );
      _AllocAndFillLnStarts( add );
      ForwardDiffOper(
        buf,
        add_diff_idx,
        0,
        &add,
        undoableopertype_t::add
        );

      // update any given ptrs to the same actual contents, after the insertion.
      // PERF: move this out after this nchunks loop, so we aren't N^2
      For( j, 0, concurrents_len ) {
        auto concurrent = concurrents[j];
        if( concurrent->diff_idx >= add_diff_idx ) {
          concurrent->diff_idx += 1;
        }
      }
    }
  }

  //
  // to keep the overall diff count low, concat adjacent diff_t's if we can do it quickly.
  // we only merge the first newly-inserted diff with it's previous, and only if the total len is small.
  // this is because people actually type 1,000s of characters, and we don't want that many diffs.
  // it'd be cool if we could stream new characters right into existing diffs, i.e. avoid this
  // 'merge-after-the-fact', but that requires some deep introspection into pagelist_t that we don't have right now.
  // if we switched to stack_resizeable_pagelist_t, we probably could do that introspection. something to consider for later.
  //
  if( pos.diff_idx ) {
    AssertCrash( pos.diff_idx < buf.diffs.len );
    auto diff = buf.diffs.mem + pos.diff_idx;

    content_ptr_t prev;
    prev.diff_idx = pos.diff_idx - 1;
    prev.offset_into_diff = 0;
    AssertCrash( prev.diff_idx < buf.diffs.len );
    auto diff_prev = buf.diffs.mem + prev.diff_idx;

    constant idx_t c_combine_threshold = 128;
    auto combined_len = diff->slice.len + diff_prev->slice.len;
    if( combined_len < c_combine_threshold ) {
      diff_t repl;
      repl.slice.len = combined_len;
      repl.slice.mem = AddPagelist( buf.pagelist, u8, 1, repl.slice.len );
      AssertCrash( combined_len );
      Memmove( repl.slice.mem, diff_prev->slice.mem, diff_prev->slice.len );
      Memmove( repl.slice.mem + diff_prev->slice.len, diff->slice.mem, diff->slice.len );
      _AllocAndFillLnStarts( repl );

      auto repl_diff_slice_len = diff_prev->slice.len;
      auto repl_diff_idx = prev.diff_idx;
      auto del_diff_idx = pos.diff_idx;
      ForwardDiffOper(
        buf,
        repl_diff_idx,
        diff_prev,
        &repl,
        undoableopertype_t::mod
        );
      ForwardDiffOper(
        buf,
        del_diff_idx,
        diff,
        0,
        undoableopertype_t::rem
        );

      // don't use 'diff' or 'diff_prev' here, after the ForwardDiffOper calls above.

      // update any given ptrs to the same actual contents, after the insertion.
      For( j, 0, concurrents_len ) {
        auto concurrent = concurrents[j];
        if( concurrent->diff_idx == del_diff_idx ) {
          concurrent->diff_idx -= 1;
          concurrent->offset_into_diff += repl_diff_slice_len;
        } elif( concurrent->diff_idx > del_diff_idx ) {
          concurrent->diff_idx -= 1;
        }
      }
    }
  }

  return end;
}

Inl content_ptr_t
Insert(
  buf_t& buf,
  content_ptr_t pos,
  u8 c,
  content_ptr_t** concurrents,
  idx_t concurrents_len
  )
{
  return Insert(
    buf,
    pos,
    &c,
    1,
    concurrents,
    concurrents_len
    );
}


//
// delete contents in the range: [a, b)
// e.g. Delete( "0123456789", 4, 8 ) -> "012389"
//
// here's what we do to all elements in the 'concurrents' list:
//
//   pointers BEFORE the deleted section remain unchanged.
//      [aaaXXXXbbbb]
//        ^
//      [aaabbbb]
//        ^
//
//   pointers INTO a deleted section get moved left, to point to the content that was after the deleted section.
//      [aaaXXXXbbbb]
//            ^
//      [aaabbbb]
//          ^
//
//   pointers AFTER the deleted section move left, to point to the same content as they did before.
//      [aaaXXXXbbbb]
//               ^
//      [aaabbbb]
//           ^
//
void
Delete(
  buf_t& buf,
  content_ptr_t start,
  content_ptr_t end,
  content_ptr_t** concurrents,
  idx_t concurrents_len
  )
{
  if( IsEOF( buf, start ) ) {
    return;
  }

  // first, split diffs at all boundaries.

  // passing arbitrary numbers of outparams isn't pretty, when we have to tack some on.
  // PERF: allocating these outparam lists isn't great.
  stack_nonresizeable_t<content_ptr_t*> concurrents_and_start;
  stack_nonresizeable_t<content_ptr_t*> concurrents_and_end;
  Alloc( concurrents_and_start, concurrents_len + 1 );
  Alloc( concurrents_and_end,   concurrents_len + 1 );
  Memmove( AddBack( concurrents_and_start, concurrents_len ), concurrents, concurrents_len * sizeof( concurrents[0] ) );
  Memmove( AddBack( concurrents_and_end,   concurrents_len ), concurrents, concurrents_len * sizeof( concurrents[0] ) );
  *AddBack( concurrents_and_start ) = &start;
  *AddBack( concurrents_and_end ) = &end;

  if( start.offset_into_diff ) {
    start = _SplitDiffAt( buf, start, ML( concurrents_and_end ) );
  }
  if( end.offset_into_diff ) {
    end = _SplitDiffAt( buf, end, ML( concurrents_and_start ) );
  }

  Free( concurrents_and_end );
  Free( concurrents_and_start );

  // now the problem is transformed into diff space.

  ReverseFor( rem_diff_idx, start.diff_idx, end.diff_idx ) {
    ForwardDiffOper(
      buf,
      rem_diff_idx,
      buf.diffs.mem + rem_diff_idx,
      0,
      undoableopertype_t::rem
      );

    // update any given ptrs to the same actual contents ( or left-nearest if contents are deleted ), after the deletion.
    // PERF: move this out after this start->end loop, so we aren't N^2
    For( i, 0, concurrents_len ) {
      auto concurrent = concurrents[i];
      if( concurrent->diff_idx == rem_diff_idx ) {
        concurrent->offset_into_diff = 0;
      } elif( concurrent->diff_idx > rem_diff_idx ) {
        concurrent->diff_idx -= 1;
      }
    }
  }
}


Inl content_ptr_t
Replace(
  buf_t& buf,
  content_ptr_t start,
  content_ptr_t end,
  u8* str,
  idx_t len,
  content_ptr_t** concurrents,
  idx_t concurrents_len
  )
{
  // passing arbitrary numbers of outparams isn't pretty, when we have to tack some on.
  // PERF: allocating these outparam lists isn't great.
  stack_nonresizeable_t<content_ptr_t*> new_concurrents;
  Alloc( new_concurrents, concurrents_len + 1 );
  Memmove( AddBack( new_concurrents, concurrents_len ), concurrents, concurrents_len * sizeof( concurrents[0] ) );
  *AddBack( new_concurrents ) = &start;

  Delete( buf, start, end, ML( new_concurrents ) );

  Free( new_concurrents );

  return Insert( buf, start, str, len, concurrents, concurrents_len );
}


//
//   pointers BEFORE the DST section remain unchanged.
//      [aaaabbcc]
//       ^
//      [aabbaabbcc]
//       ^
//
//   pointers INTO a SRC section move, to point to the same SRC content as they did before.
//   there's no mechanism to make duplicate pointers to the DST content. maybe there should be, if someone needs it.
//      [aaaabbcc]
//            ^
//      [aabbaabbcc]
//              ^
//
//   pointers AFTER the DST section move, to point to the same content as they did before.
//      [aaaabbcc]
//             ^
//      [aabbaabbcc]
//               ^
//
void
Copy(
  buf_t& buf,
  content_ptr_t src_start,
  content_ptr_t src_end,
  content_ptr_t dst,
  content_ptr_t** concurrents,
  idx_t concurrents_len
  )
{
  AssertCrash( LEqual( src_start, src_end ) );

  if( IsEOF( buf, src_start ) ) {
    return;
  }
  if( Equal( src_start, src_end ) ) {
    return;
  }

  // TODO: first part is a dupe of code from Move.

  // first, split diffs at all src/dst boundaries.

  // passing arbitrary numbers of outparams isn't pretty, when we have to tack some on.
  // PERF: allocating these outparam lists isn't great.
  stack_nonresizeable_t<content_ptr_t*> concurrents_dst_srcend;
  stack_nonresizeable_t<content_ptr_t*> concurrents_dst_srcstart;
  stack_nonresizeable_t<content_ptr_t*> concurrents_srcstart_srcend;
  Alloc( concurrents_dst_srcend,      concurrents_len + 2 );
  Alloc( concurrents_dst_srcstart,    concurrents_len + 2 );
  Alloc( concurrents_srcstart_srcend, concurrents_len + 2 );
  Memmove( AddBack( concurrents_dst_srcend,      concurrents_len ), concurrents, concurrents_len * sizeof( concurrents[0] ) );
  Memmove( AddBack( concurrents_dst_srcstart,    concurrents_len ), concurrents, concurrents_len * sizeof( concurrents[0] ) );
  Memmove( AddBack( concurrents_srcstart_srcend, concurrents_len ), concurrents, concurrents_len * sizeof( concurrents[0] ) );
  *AddBack( concurrents_dst_srcend ) = &dst;
  *AddBack( concurrents_dst_srcend ) = &src_end;
  *AddBack( concurrents_dst_srcstart ) = &dst;
  *AddBack( concurrents_dst_srcstart ) = &src_start;
  *AddBack( concurrents_srcstart_srcend ) = &src_start;
  *AddBack( concurrents_srcstart_srcend ) = &src_end;

  if( src_start.offset_into_diff ) {
    src_start = _SplitDiffAt( buf, src_start, ML( concurrents_dst_srcend ) );
  }
  if( src_end.offset_into_diff ) {
    src_end = _SplitDiffAt( buf, src_end, ML( concurrents_dst_srcstart ) );
  }
  if( dst.offset_into_diff ) {
    dst = _SplitDiffAt( buf, dst, ML( concurrents_srcstart_srcend ) );
  }

  Free( concurrents_dst_srcend );
  Free( concurrents_dst_srcstart );
  Free( concurrents_srcstart_srcend );

  // now the problem is transformed into diff space.

  //
  // 0123456789         initial
  //    ^----           src
  //       ^            dst
  // 0123453456789      after left loop
  // 012345345676789    after rght loop
  //
  //     auto src_end = src_idx + src_len;
  //     auto left = ( dst_idx > src_idx )  ?  dst_idx - src_idx  :  0;
  //     auto rght = ( src_end > dst_idx )  ?  src_end - dst_idx  :  0;
  //     For( i, 0, left ) {
  //       auto diff_src = buf.diffs.mem + src_idx;
  //       AddCopyAt( diff_src, dst_idx );
  //       dst_idx += 1;
  //       src_idx += 1;
  //     }
  //     src_idx += left;
  //     For( i, 0, rght ) {
  //       auto diff_src = buf.diffs.mem + src_idx;
  //       AddCopyAt( diff_src, dst_idx );
  //       dst_idx += 1;
  //       src_idx += 1;
  //       src_idx += 1;
  //     }
  //
  // transforms into:
  //
  //     For( i, 0, src_len ) {
  //       if( i == left ) {
  //         src_idx += left;
  //       }
  //
  //       auto diff_src = buf.diffs.mem + src_idx;
  //       AddCopyAt( diff_src, dst_idx );
  //       dst_idx += 1;
  //       src_idx += 1;
  //
  //       if( i >= left ) {
  //         src_idx += 1;
  //       }
  //     }
  //

  auto dst_idx = dst.diff_idx;
  auto src_idx = src_start.diff_idx;
  auto src_len = src_end.diff_idx - src_start.diff_idx;
  auto left = ( dst_idx > src_idx )  ?  dst_idx - src_idx  :  0;

  For( i, 0, src_len ) {
    // compensate src iterator for crossing dst's location
    if( i == left ) {
      src_idx += left;
    }

    auto diff_src = buf.diffs.mem + src_idx;

    diff_t copy;
    copy.slice = diff_src->slice;
    _AllocAndFillLnStarts( copy );
    auto add_diff_idx = dst_idx;
    ForwardDiffOper(
      buf,
      add_diff_idx,
      0,
      &copy,
      undoableopertype_t::add
      );

    // update any given ptrs to the same actual contents, after the insertion.
    // PERF: move this out after this loop, so we aren't N^2
    For( j, 0, concurrents_len ) {
      auto concurrent = concurrents[j];
      if( concurrent->diff_idx >= add_diff_idx ) {
        concurrent->diff_idx += 1;
      }
    }

    dst_idx += 1;
    src_idx += 1;

    // compensate src iterator for inserting a diff
    if( i >= left ) {
      src_idx += 1;
    }
  }
}


//
// pointers in 'concurrent' are updated to point to the same actual content they did before.
//
// note that if you are moving left ( i.e. dst < src_start ), then a concurrent with a value of src_end won't move.
// if you're tracking src_start and src_end, that means your start will update, but not your end.
// this is because this function doesn't know whether a concurrent ptr is a "list-end" ptr, or a real "content" ptr.
// since it'd be ugly to annotate the difference between these things, it's just up to the caller to pick.
// we return the moved src_end, so you can easily take that if you want.
//
content_ptr_t
Move(
  buf_t& buf,
  content_ptr_t src_start,
  content_ptr_t src_end,
  content_ptr_t dst,
  content_ptr_t** concurrents,
  idx_t concurrents_len
  )
{
  AssertCrash( LEqual( src_start, src_end ) );

  if( IsEOF( buf, src_start ) ) {
    return src_end;
  }
  if( Equal( src_start, src_end ) ) {
    return src_end;
  }
  if( LEqual( src_start, dst )  &&  LEqual( dst, src_end ) ) {
    return src_end;
  }

  // TODO: first part is a dupe of code from Copy.

  // first, split diffs at all src/dst boundaries.

  // passing arbitrary numbers of outparams isn't pretty, when we have to tack some on.
  // PERF: allocating these outparam lists isn't great.
  stack_nonresizeable_t<content_ptr_t*> concurrents_dst_srcend;
  stack_nonresizeable_t<content_ptr_t*> concurrents_dst_srcstart;
  stack_nonresizeable_t<content_ptr_t*> concurrents_srcstart_srcend;
  Alloc( concurrents_dst_srcend,      concurrents_len + 2 );
  Alloc( concurrents_dst_srcstart,    concurrents_len + 2 );
  Alloc( concurrents_srcstart_srcend, concurrents_len + 2 );
  Memmove( AddBack( concurrents_dst_srcend,      concurrents_len ), concurrents, concurrents_len * sizeof( concurrents[0] ) );
  Memmove( AddBack( concurrents_dst_srcstart,    concurrents_len ), concurrents, concurrents_len * sizeof( concurrents[0] ) );
  Memmove( AddBack( concurrents_srcstart_srcend, concurrents_len ), concurrents, concurrents_len * sizeof( concurrents[0] ) );
  *AddBack( concurrents_dst_srcend ) = &dst;
  *AddBack( concurrents_dst_srcend ) = &src_end;
  *AddBack( concurrents_dst_srcstart ) = &dst;
  *AddBack( concurrents_dst_srcstart ) = &src_start;
  *AddBack( concurrents_srcstart_srcend ) = &src_start;
  *AddBack( concurrents_srcstart_srcend ) = &src_end;

  if( src_start.offset_into_diff ) {
    src_start = _SplitDiffAt( buf, src_start, ML( concurrents_dst_srcend ) );
  }
  if( src_end.offset_into_diff ) {
    src_end = _SplitDiffAt( buf, src_end, ML( concurrents_dst_srcstart ) );
  }
  if( dst.offset_into_diff ) {
    dst = _SplitDiffAt( buf, dst, ML( concurrents_srcstart_srcend ) );
  }

  Free( concurrents_dst_srcend );
  Free( concurrents_dst_srcstart );
  Free( concurrents_srcstart_srcend );

  // now the problem is transformed into diff space.

  auto dst_idx = dst.diff_idx;
  auto src_idx = src_start.diff_idx;
  auto src_len = src_end.diff_idx - src_start.diff_idx;

  // we check the equivalent in content_ptr_t space above, as an early no-op.
  AssertCrash( !( src_idx <= dst_idx  &&  dst_idx <= src_idx + src_len ) );

  bool swap = dst_idx >= src_idx;
  if( swap ) {
    auto new_dst_idx = src_idx;
    auto new_src_idx = src_idx + src_len;
    auto new_src_len = dst_idx - src_idx - src_len;

    dst_idx = new_dst_idx;
    src_idx = new_src_idx;
    src_len = new_src_len;
  }

  auto dst_len = src_idx - dst_idx;
  auto total_len = dst_len + src_len;

  // XXXXXXXXXXXXXXXXXXXXXXX
  // XXXXXXXXXXXXXXXXXXXXXXX
  // XXXXXXXXXXXXXXXXXXXXXXX
  // TODO: this is wrong.
//  ImplementCrash();

  content_ptr_t moved_src_end;
  moved_src_end.offset_into_diff = 0;
  moved_src_end.diff_idx = swap  ?  dst_idx + dst_len  :  src_idx + src_len;
  if( moved_src_end.diff_idx < dst_idx  ||  moved_src_end.diff_idx >= src_idx + src_len ) {
    // no-op
  } elif( moved_src_end.diff_idx < src_idx ) {
    moved_src_end.diff_idx += src_len;
  } else {
    moved_src_end.diff_idx -= dst_len;
  }

  idx_t nreordered = 0;

  // bitlist for cycle detection.
  stack_nonresizeable_t<u64> bitlist;
  {
    auto quo = total_len / 64;
    auto rem = total_len % 64;
    auto nchunks = quo + ( rem > 0 );
    Alloc( bitlist, nchunks );
    For( i, 0, nchunks ) {
      bitlist.mem[i] = 0;
    }
  }

  // bitlist for tracking which concurrents we've already updated.
  stack_nonresizeable_t<u64> bitlist_concurrents;
  {
    auto quo = concurrents_len / 64;
    auto rem = concurrents_len % 64;
    auto nchunks = quo + ( rem > 0 );
    Alloc( bitlist_concurrents, nchunks );
    For( i, 0, nchunks ) {
      bitlist_concurrents.mem[i] = 0;
    }
  }

  For( i, 0, total_len ) {

    if( nreordered >= total_len ) {
      break;
    }

    // skip cycles we've already traversed.
    auto bit = bitlist.mem[ i / 64 ] & ( 1ULL << ( i % 64 ) );
    if( bit ) {
      continue;
    }

    auto s = src_idx + i;
    auto d = s - dst_len;

    idx_t t0_diff_idx = s;
    diff_t t0 = buf.diffs.mem[ s ];
    _AllocAndFillLnStarts( t0 );

    idx_t t1_diff_idx;
    diff_t t1;
    Forever {
      auto bit_idx = d - dst_idx;
      // detect cycle, and terminate cycle traversal.
      if( bitlist.mem[ bit_idx / 64 ] & ( 1ULL << ( bit_idx % 64 ) ) ) {
        break;
      }
      // mark dst as traversed.
      bitlist.mem[ bit_idx / 64 ] |= ( 1ULL << ( bit_idx % 64 ) );

      t1_diff_idx = d;
      t1 = buf.diffs.mem[ d ];
      _AllocAndFillLnStarts( t1 );
      ForwardDiffOper(
        buf,
        t1_diff_idx,
        buf.diffs.mem + t1_diff_idx,
        &t0,
        undoableopertype_t::mod
        );

      // update concurrents pointing at t0 to point to t1, since that's where we've shoved the content.
      For( j, 0, concurrents_len ) {
        // skip concurrents we've already updated.
        if( bitlist_concurrents.mem[ j / 64 ] & ( 1ULL << ( j % 64 ) ) ) {
          continue;
        }
        auto concurrent = concurrents[j];
        if( concurrent->diff_idx == t0_diff_idx ) {
          concurrent->diff_idx = t1_diff_idx;
          bitlist_concurrents.mem[ j / 64 ] |= ( 1ULL << ( j % 64 ) );
        }
      }

      nreordered += 1;
      if( nreordered >= total_len ) {
        break;
      }

      if( d < src_idx ) {
        d += src_len;
      } else {
        d -= dst_len;
      }

      bit_idx = d - dst_idx;
      // detect cycle, and terminate cycle traversal.
      if( bitlist.mem[ bit_idx / 64 ] & ( 1ULL << ( bit_idx % 64 ) ) ) {
        break;
      }
      // mark dst as traversed.
      bitlist.mem[ bit_idx / 64 ] |= ( 1ULL << ( bit_idx % 64 ) );

      t0_diff_idx = d;
      t0 = buf.diffs.mem[ d ];
      _AllocAndFillLnStarts( t0 );
      ForwardDiffOper(
        buf,
        t0_diff_idx,
        buf.diffs.mem + t0_diff_idx,
        &t1,
        undoableopertype_t::mod
        );

      // update concurrents pointing at t1 to point to t0, since that's where we've shoved the content.
      For( j, 0, concurrents_len ) {
        // skip concurrents we've already updated.
        if( bitlist_concurrents.mem[ j / 64 ] & ( 1ULL << ( j % 64 ) ) ) {
          continue;
        }
        auto concurrent = concurrents[j];
        if( concurrent->diff_idx == t1_diff_idx ) {
          concurrent->diff_idx = t0_diff_idx;
          bitlist_concurrents.mem[ j / 64 ] |= ( 1ULL << ( j % 64 ) );
        }
      }

      nreordered += 1;
      if( nreordered >= total_len ) {
        break;
      }

      if( d < src_idx ) {
        d += src_len;
      } else {
        d -= dst_len;
      }
    }
  }

  Free( bitlist_concurrents );
  Free( bitlist );

  return moved_src_end;
}

//
// pointers in 'concurrent' are updated to point to the same actual content they did before.
//
// note that your rightmost range's end ptr won't move.
// if you're tracking a/b_start, a/b_end, that means your rightmost of a/b start will update, but not your end.
// this is because this function doesn't know whether a concurrent ptr is a "list-end" ptr, or a real "content" ptr.
// since it'd be ugly to annotate the difference between these things, it's just up to the caller to pick.
// we return the moved a/b ends, so you can easily take them if you want.
//
void
Swap(
  buf_t& buf,
  content_ptr_t a_start,
  content_ptr_t a_end,
  content_ptr_t b_start,
  content_ptr_t b_end,
  content_ptr_t* moved_a_end,
  content_ptr_t* moved_b_end,
  content_ptr_t** concurrents,
  idx_t concurrents_len
  )
{
  AssertCrash(
    !( LEqual( b_start, a_start )  &&  LEqual( a_start, b_end ) ) &&
    !( LEqual( a_start, b_start )  &&  LEqual( b_start, a_end ) )
    );

  // passing arbitrary numbers of outparams isn't pretty, when we have to tack some on.
  // PERF: allocating these outparam lists isn't great.
  stack_nonresizeable_t<content_ptr_t*> concurrents_aend_bstart_bend;
  stack_nonresizeable_t<content_ptr_t*> concurrents_astart_aend_bend;
  stack_nonresizeable_t<content_ptr_t*> concurrents_astart_bstart_bend;
  stack_nonresizeable_t<content_ptr_t*> concurrents_astart_aend_bstart;
  Alloc( concurrents_aend_bstart_bend,   concurrents_len + 3 );
  Alloc( concurrents_astart_aend_bend,   concurrents_len + 3 );
  Alloc( concurrents_astart_bstart_bend, concurrents_len + 3 );
  Alloc( concurrents_astart_aend_bstart, concurrents_len + 3 );
  Memmove( AddBack( concurrents_aend_bstart_bend,   concurrents_len ), concurrents, concurrents_len * sizeof( concurrents[0] ) );
  Memmove( AddBack( concurrents_astart_aend_bend,   concurrents_len ), concurrents, concurrents_len * sizeof( concurrents[0] ) );
  Memmove( AddBack( concurrents_astart_bstart_bend, concurrents_len ), concurrents, concurrents_len * sizeof( concurrents[0] ) );
  Memmove( AddBack( concurrents_astart_aend_bstart, concurrents_len ), concurrents, concurrents_len * sizeof( concurrents[0] ) );
  *AddBack( concurrents_aend_bstart_bend ) = &a_end;
  *AddBack( concurrents_aend_bstart_bend ) = &b_start;
  *AddBack( concurrents_aend_bstart_bend ) = &b_end;
  *AddBack( concurrents_astart_aend_bend ) = &a_start;
  *AddBack( concurrents_astart_aend_bend ) = &a_end;
  *AddBack( concurrents_astart_aend_bend ) = &b_end;
  *AddBack( concurrents_astart_bstart_bend ) = &a_start;
  *AddBack( concurrents_astart_bstart_bend ) = &b_start;
  *AddBack( concurrents_astart_bstart_bend ) = &b_end;
  *AddBack( concurrents_astart_aend_bstart ) = &a_start;
  *AddBack( concurrents_astart_aend_bstart ) = &a_end;
  *AddBack( concurrents_astart_aend_bstart ) = &b_start;

  // split diffs so a, b have offset_into_diff=0.

  if( a_start.offset_into_diff ) {
    a_start = _SplitDiffAt( buf, a_start, ML( concurrents_aend_bstart_bend ) );
  }
  if( b_start.offset_into_diff ) {
    b_start = _SplitDiffAt( buf, b_start, ML( concurrents_astart_aend_bend ) );
  }
  if( a_end.offset_into_diff ) {
    a_end = _SplitDiffAt( buf, a_end, ML( concurrents_astart_bstart_bend ) );
  }
  if( b_end.offset_into_diff ) {
    b_end = _SplitDiffAt( buf, b_end, ML( concurrents_astart_aend_bstart ) );
  }

  Free( concurrents_astart_aend_bstart );
  Free( concurrents_astart_bstart_bend );
  Free( concurrents_astart_aend_bend );
  Free( concurrents_aend_bstart_bend );

  // now the problem is transformed into diff space.

  auto a_idx = a_start.diff_idx;
  auto b_idx = b_start.diff_idx;
  auto a_len = a_end.diff_idx - a_start.diff_idx;
  auto b_len = b_end.diff_idx - b_start.diff_idx;

  // we check the equivalent in content_ptr_t space above, as an early crash.
  AssertCrash(
    !( b_idx <= a_idx  &&  a_idx <= b_idx + b_len ) &&
    !( a_idx <= b_idx  &&  b_idx <= a_idx + a_len )
    );

  auto swap = a_idx >= b_idx;
  if( swap ) {
    auto new_a_idx = b_idx;
    auto new_a_len = b_len;
    auto new_b_idx = a_idx;
    auto new_b_len = a_len;

    a_idx = new_a_idx;
    a_len = new_a_len;
    b_idx = new_b_idx;
    b_len = new_b_len;
  }

  auto m_idx = a_idx + a_len;
  auto m_len = b_idx - m_idx;

  auto total_len = a_len + m_len + b_len;

  // XXXXXXXXXXXXXXXXXXXXXXX
  // XXXXXXXXXXXXXXXXXXXXXXX
  // XXXXXXXXXXXXXXXXXXXXXXX
  // TODO: this is wrong.
//  ImplementCrash();

  moved_a_end->offset_into_diff = 0;
  moved_a_end->diff_idx = swap  ?  a_idx + a_len  :  b_idx + b_len; // TODO: this is backwards, isn't it?
  if( moved_a_end->diff_idx < a_idx  ||  moved_a_end->diff_idx >= b_idx + b_len ) {
    // no-op
  } elif( moved_a_end->diff_idx >= b_idx ) {
    moved_a_end->diff_idx -= m_len + a_len;
  } elif( moved_a_end->diff_idx >= m_idx ) {
    moved_a_end->diff_idx += b_len - a_len;
  } else {
    moved_a_end->diff_idx += m_len + b_len;
  }

  moved_b_end->offset_into_diff = 0;
  moved_b_end->diff_idx = swap  ?  b_idx + b_len  :  a_idx + a_len; // TODO: this is backwards, isn't it?
  if( moved_b_end->diff_idx < a_idx  ||  moved_b_end->diff_idx >= b_idx + b_len ) {
    // no-op
  } elif( moved_b_end->diff_idx >= b_idx ) {
    moved_b_end->diff_idx -= m_len + a_len;
  } elif( moved_b_end->diff_idx >= m_idx ) {
    moved_b_end->diff_idx += b_len - a_len;
  } else {
    moved_b_end->diff_idx += m_len + b_len;
  }

  idx_t nreordered = 0;

  // bitlist for cycle detection.
  stack_nonresizeable_t<u64> bitlist;
  {
    auto quo = total_len / 64;
    auto rem = total_len % 64;
    auto nchunks = quo + ( rem > 0 );
    Alloc( bitlist, nchunks );
    For( i, 0, nchunks ) {
      bitlist.mem[i] = 0;
    }
  }

  // bitlist for tracking which concurrents we've already updated.
  stack_nonresizeable_t<u64> bitlist_concurrents;
  {
    auto quo = concurrents_len / 64;
    auto rem = concurrents_len % 64;
    auto nchunks = quo + ( rem > 0 );
    Alloc( bitlist_concurrents, nchunks );
    For( i, 0, nchunks ) {
      bitlist_concurrents.mem[i] = 0;
    }
  }

  For( i, 0, total_len ) {

    if( nreordered >= total_len ) {
      break;
    }

    // skip cycles we've already traversed.
    auto bit = bitlist.mem[ i / 64 ] & ( 1ULL << ( i % 64 ) );
    if( bit ) {
      continue;
    }

    auto s = b_idx + i;
    auto d = s - a_len - m_len;

    idx_t t0_diff_idx = s;
    diff_t t0 = buf.diffs.mem[ s ];
    _AllocAndFillLnStarts( t0 );

    idx_t t1_diff_idx;
    diff_t t1;
    Forever {
      auto bit_idx = d - a_idx;
      // detect cycle, and terminate cycle traversal.
      if( bitlist.mem[ bit_idx / 64 ] & ( 1ULL << ( bit_idx % 64 ) ) ) {
        break;
      }
      // mark dst as traversed.
      bitlist.mem[ bit_idx / 64 ] |= ( 1ULL << ( bit_idx % 64 ) );

      t1_diff_idx = d;
      t1 = buf.diffs.mem[ d ];
      _AllocAndFillLnStarts( t1 );
      ForwardDiffOper(
        buf,
        t1_diff_idx,
        buf.diffs.mem + t1_diff_idx,
        &t0,
        undoableopertype_t::mod
        );

      // update concurrents pointing at t0 to point to t1, since that's where we've shoved the content.
      For( j, 0, concurrents_len ) {
        // skip concurrents we've already updated.
        if( bitlist_concurrents.mem[ j / 64 ] & ( 1ULL << ( j % 64 ) ) ) {
          continue;
        }
        auto concurrent = concurrents[j];
        if( concurrent->diff_idx == t0_diff_idx ) {
          concurrent->diff_idx = t1_diff_idx;
          bitlist_concurrents.mem[ j / 64 ] |= ( 1ULL << ( j % 64 ) );
        }
      }

      nreordered += 1;
      if( nreordered >= total_len ) {
        break;
      }

      if( d >= b_idx ) {
        d -= m_len + a_len;
      } elif( d >= m_idx ) {
        d += b_len - a_len;
      } else {
        d += m_len + b_len;
      }

      bit_idx = d - a_idx;
      // detect cycle, and terminate cycle traversal.
      if( bitlist.mem[ bit_idx / 64 ] & ( 1ULL << ( bit_idx % 64 ) ) ) {
        break;
      }
      // mark dst as traversed.
      bitlist.mem[ bit_idx / 64 ] |= ( 1ULL << ( bit_idx % 64 ) );

      t0_diff_idx = d;
      t0 = buf.diffs.mem[ d ];
      _AllocAndFillLnStarts( t0 );
      ForwardDiffOper(
        buf,
        t0_diff_idx,
        buf.diffs.mem + t0_diff_idx,
        &t1,
        undoableopertype_t::mod
        );

      // update concurrents pointing at t1 to point to t0, since that's where we've shoved the content.
      For( j, 0, concurrents_len ) {
        // skip concurrents we've already updated.
        if( bitlist_concurrents.mem[ j / 64 ] & ( 1ULL << ( j % 64 ) ) ) {
          continue;
        }
        auto concurrent = concurrents[j];
        if( concurrent->diff_idx == t1_diff_idx ) {
          concurrent->diff_idx = t0_diff_idx;
          bitlist_concurrents.mem[ j / 64 ] |= ( 1ULL << ( j % 64 ) );
        }
      }

      nreordered += 1;
      if( nreordered >= total_len ) {
        break;
      }

      if( d >= b_idx ) {
        d -= m_len + a_len;
      } elif( d >= m_idx ) {
        d += b_len - a_len;
      } else {
        d += m_len + b_len;
      }
    }
  }

  Free( bitlist_concurrents );
  Free( bitlist );
}


// =================================================================================
// CONTENT MODIFY UNDO CALLS
//

void
UndoCheckpt( buf_t& buf )
{
  undoableoper_t checkpt = { undoableopertype_t::checkpt };
  AddHistorical( buf, checkpt );
}


void
Undo( buf_t& buf )
{
  AssertCrash( buf.history_idx <= buf.history.len );
  AssertCrash( buf.history_idx == buf.history.len  ||  buf.history.mem[buf.history_idx].type == undoableopertype_t::checkpt );
  if( !buf.history_idx ) {
    return;
  }

  bool loop = 1;
  while( loop ) {
    buf.history_idx -= 1;
    auto oper = buf.history.mem + buf.history_idx;

    // undo this diff operation:
    switch( oper->type ) {
      case undoableopertype_t::checkpt: {
        loop = 0;
      } break;
      case undoableopertype_t::add: {
        DiffRemove( buf, oper->idx, &oper->diff_new );
      } break;
      case undoableopertype_t::mod: {
        DiffReplace( buf, oper->idx, &oper->diff_new, &oper->diff_old );
      } break;
      case undoableopertype_t::rem: {
        DiffInsert( buf, oper->idx, &oper->diff_old );
      } break;
      default: UnreachableCrash();
    }
  }

  AssertCrash( buf.history_idx == buf.history.len  ||  buf.history.mem[buf.history_idx].type == undoableopertype_t::checkpt );
}



void
Redo( buf_t& buf )
{
  AssertCrash( buf.history_idx <= buf.history.len );
  AssertCrash( buf.history_idx == buf.history.len  ||  buf.history.mem[buf.history_idx].type == undoableopertype_t::checkpt );
  if( buf.history_idx == buf.history.len ) {
    return;
  }

  buf.history_idx += 1;

  bool loop = 1;
  while( loop ) {
    if( buf.history_idx == buf.history.len ) {
      break;
    }
    auto oper = buf.history.mem + buf.history_idx;

    // redo this diff operation:
    switch( oper->type ) {
      case undoableopertype_t::checkpt: {
        loop = 0;
      } break;
      case undoableopertype_t::add: {
        DiffInsert( buf, oper->idx, &oper->diff_new );
      } break;
      case undoableopertype_t::mod: {
        DiffReplace( buf, oper->idx, &oper->diff_old, &oper->diff_new );
      } break;
      case undoableopertype_t::rem: {
        DiffRemove( buf, oper->idx, &oper->diff_old );
      } break;
      default: UnreachableCrash();
    }

    if( loop ) {
      buf.history_idx += 1;
    }
  }

  AssertCrash( buf.history_idx == buf.history.len  ||  buf.history.mem[buf.history_idx].type == undoableopertype_t::checkpt );
}



// =================================================================================
// CONTENT-DEPENDENT NAVIGATION CALLS
//   TODO: should we expose: pfn_move_stop_t, MoveL, MoveR? some funcs below just call those with different pfn_move_stop_t's.
//

typedef bool ( *pfn_move_stop_t )( u8 c, void* data ); // PERF: macro-ize this

#if 0
                    Inl idx_t
                    MoveR( buf_t& buf, idx_t pos, pfn_move_stop_t MoveStop, void* data )
                    {
                      u8 c;
                      Forever {
                        if( IsEOF( buf, pos ) ) {
                          break;
                        }
                        Contents( buf, pos, &c );
                        if( MoveStop( c, data ) ) {
                          break;
                        }
                        ++pos;
                      }
                      return pos;
                    }
#endif


Inl content_ptr_t
MoveR(
  buf_t& buf,
  content_ptr_t pos,
  pfn_move_stop_t MoveStop,
  void* data,
  idx_t* nchars_moved
  )
{

  // TODO: which is better?
#if 0
  auto x = pos;
  for(
    ;
    x.diff_idx < buf.diffs.len;
    x.offset_into_diff = 0, x.diff_idx += 1)
  {
    auto diff = buf.diffs.mem + x.diff_idx;

    while( x.offset_into_diff++ < diff->slice.len ) {
      u8 c = diff->slice.mem[ x.offset_into_diff ];

      if( MoveStop( c, data ) ) {
        return x;
      }
    }
  }
  return x;
#endif


#if 0
  auto x = pos;

  Forever {
    if( x.diff_idx >= buf.diffs.len ) {
      break;
    }

    auto diff = buf.diffs.mem + x.diff_idx;

    Forever {
      AssertCrash( x.offset_into_diff < diff->slice.len );
      u8 c = diff->slice.mem[ x.offset_into_diff ];

      if( MoveStop( c, data ) ) {
        return x;
      }

      x.offset_into_diff += 1;
      if( x.offset_into_diff == diff->slice.len ) {
        break;
      }
    }
    x.offset_into_diff = 0;
    x.diff_idx += 1;
  }
  return x;
#endif


#if 1
  idx_t nmoved = 0;
  auto x = pos;
  Forever {
    if( IsEOF( buf, x ) ) {
      break;
    }

    auto after_x = CursorCharR( buf, x, 1, 0 );

    AssertCrash( x.diff_idx < buf.diffs.len );
    auto diff = buf.diffs.mem + x.diff_idx;
    AssertCrash( x.offset_into_diff < diff->slice.len );
    u8 c = diff->slice.mem[ x.offset_into_diff ];

    if( MoveStop( c, data ) ) {
      break;
    }

    x = after_x;
    nmoved += 1;
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return x;
#endif

}









#if 0
                      Inl idx_t
                      MoveL( buf_t& buf, idx_t pos, pfn_move_stop_t MoveStop, void* data )
                      {
                        u8 c;
                        Forever {
                          if( IsBOF( buf, pos ) ) {
                            break;
                          }
                          --pos;
                          Contents( buf, pos, &c );
                          if( MoveStop( c, data ) ) {
                            ++pos;
                            break;
                          }
                        }
                        return pos;
                      }
#endif


#if 0
  ReverseFori( idx_t, diff_idx, 0, MIN( pos.diff_idx + 1, buf.diffs.len ) ) {
    auto diff = buf.diffs.mem + diff_idx;
    ReverseFori( idx_t, offset_into_diff, 0, diff->slice.len ) {

    }
  }

  auto x = pos;
  Forever {
    if( IsBOF( buf, x ) ) {
      break;
    }
  }


  auto x = pos;
  for(
    ;
    !IsBOF( buf, x );
    x.offset_into_diff = 0, x.diff_idx -= 1)
  {
    auto diff = buf.diffs.mem + x.diff_idx;

    while( x.offset_into_diff++ < diff->slice.len ) {
      u8 c = diff->slice.mem[ x.offset_into_diff ];

      if( MoveStop( c, data ) ) {
        return x;
      }
    }
  }
  return x;
#endif


Inl content_ptr_t
MoveL(
  buf_t& buf,
  content_ptr_t pos,
  pfn_move_stop_t MoveStop,
  void* data,
  idx_t* nchars_moved
  )
{

  // TODO: which is better?

#if 0
  auto x = pos;
  AssertCrash( x.diff_idx < buf.diffs.len );

  Forever {
    auto diff = buf.diffs.mem + x.diff_idx;

    AssertCrash( x.offset_into_diff < diff->slice.len );
    u8 c = diff->slice.mem[ x.offset_into_diff ];

    if( MoveStop( c, data ) ) {
      x = CursorCharR( buf, x, 1 );
      return x;
    }

    if( !x.diff_idx ) {
      break;
    }

    x.diff_idx -= 1;
    auto diff = buf.diffs.mem + x.diff_idx;
    AssertCrash( diff->slice.len );
    x.offset_into_diff = diff->slice.len - 1;
  }
  return x;
#endif


#if 1
  idx_t nmoved = 0;
  auto x = pos;
  Forever {
    if( IsBOF( buf, x ) ) {
      break;
    }
    auto before_x = CursorCharL( buf, x, 1, 0 );
    auto after_x = x;
    x = before_x;

    AssertCrash( x.diff_idx < buf.diffs.len );
    auto diff = buf.diffs.mem + x.diff_idx;
    AssertCrash( x.offset_into_diff < diff->slice.len );
    u8 c = diff->slice.mem[ x.offset_into_diff ];

    if( MoveStop( c, data ) ) {
      x = after_x; // setup retval
      break;
    }

    nmoved += 1;
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return x;
#endif

}










Inl bool
MoveStopAtNonWordChar( u8 c, void* data )
{
  return !AsciiInWord( c );
}

content_ptr_t
CursorStopAtNonWordCharL( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved )
{
  return MoveL( buf, pos, MoveStopAtNonWordChar, 0, nchars_moved );
}

content_ptr_t
CursorStopAtNonWordCharR( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved )
{
  return MoveR( buf, pos, MoveStopAtNonWordChar, 0, nchars_moved );
}



Inl bool
MoveStopAtNewline( u8 c, void* data )
{
  return ( c == '\r' )  |  ( c == '\n' );
}

content_ptr_t
CursorStopAtNewlineL( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved )
{
  return MoveL( buf, pos, MoveStopAtNewline, 0, nchars_moved );
}

content_ptr_t
CursorStopAtNewlineR( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved )
{
  return MoveR( buf, pos, MoveStopAtNewline, 0, nchars_moved );
}



content_ptr_t
CursorCharInlineL( buf_t& buf, content_ptr_t pos, idx_t len, idx_t* nchars_moved )
{
  idx_t nmoved0;
  idx_t nmoved1;
  auto l0 = CursorCharL( buf, pos, len, &nmoved0 );
  auto l1 = CursorStopAtNewlineL( buf, pos, &nmoved1 );
  pos = Max( l0, l1 );
  if( nchars_moved ) {
    *nchars_moved = MAX( nmoved0, nmoved1 );
  }
  return pos;
}

content_ptr_t
CursorCharInlineR( buf_t& buf, content_ptr_t pos, idx_t len, idx_t* nchars_moved )
{
  idx_t nmoved0;
  idx_t nmoved1;
  auto r0 = CursorCharR( buf, pos, len, &nmoved0 );
  auto r1 = CursorStopAtNewlineR( buf, pos, &nmoved1 );
  pos = Min( r0, r1 );
  if( nchars_moved ) {
    *nchars_moved = MIN( nmoved0, nmoved1 );
  }
  return pos;
}



Inl bool
MoveSkipSpacetab( u8 c, void* data )
{
  return ( c != ' '  )  &  ( c != '\t' );
}

content_ptr_t
CursorSkipSpacetabL( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved )
{
  return MoveL( buf, pos, MoveSkipSpacetab, 0, nchars_moved );
}

content_ptr_t
CursorSkipSpacetabR( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved )
{
  return MoveR( buf, pos, MoveSkipSpacetab, 0, nchars_moved );
}



Inl bool
MoveStopAtSpacetab( u8 c, void* data )
{
  return ( c == ' '  )  ||  ( c == '\t' );
}

content_ptr_t
CursorStopAtSpacetabL( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved )
{
  return MoveL( buf, pos, MoveStopAtSpacetab, 0, nchars_moved );
}

content_ptr_t
CursorStopAtSpacetabR( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved )
{
  return MoveR( buf, pos, MoveStopAtSpacetab, 0, nchars_moved );
}


Inl bool
MoveStopAtBeforeNewlineL( u8 c, void* data )
{
  idx_t* state = Cast( idx_t*, data );
  if( *state == 2 ) {
    return 1;
  } elif( *state == 1 ) {
    if( c == '\r' ) {
      *state = 2;
      return 0;
    } else {
      return 1;
    }
  } else {
    if( c == '\n' ) {
      *state = 1;
      return 0;
    } elif( c == '\r' ) {
      *state = 2;
      return 0;
    } else {
      return 1;
    }
  }
}

content_ptr_t
CursorSingleNewlineL( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved )
{
  idx_t state = 0;
  return MoveL( buf, pos, MoveStopAtBeforeNewlineL, &state, nchars_moved );
}



Inl bool
MoveStopAtAfterNewlineR( u8 c, void* data )
{
  idx_t* state = Cast( idx_t*, data );
  if( *state == 2 ) {
    if( c == '\n' ) {
      *state = 1;
      return 0;
    } else {
      return 1;
    }
  } elif( *state == 1 ) {
    return 1;
  } else {
    if( c == '\n' ) {
      *state = 1;
      return 0;
    } elif( c == '\r' ) {
      *state = 2;
      return 0;
    } else {
      return 1;
    }
  }
}

content_ptr_t
CursorSingleNewlineR( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved )
{
  idx_t state = 0;
  return MoveR( buf, pos, MoveStopAtAfterNewlineR, &state, nchars_moved );
}


content_ptr_t
CursorSkipCharNewlineL( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved ) // TODO: rename!
{
  idx_t nmoved;
  idx_t nmoved0;
  idx_t nmoved1;
  auto l0 = CursorCharL( buf, pos, 1, &nmoved0 );
  auto l1 = CursorSingleNewlineL( buf, pos, &nmoved1 );
  if( Equal( pos, l0 ) ) {
    pos = l1;
    nmoved = nmoved1;
  } else {
    if( Equal( pos, l1 ) ) {
      pos = l0;
      nmoved = nmoved0;
    } else {
      pos = Min( l0, l1 );
      nmoved = MIN( nmoved0, nmoved1 );
    }
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return pos;
}

content_ptr_t
CursorSkipCharNewlineR( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved ) // TODO: rename!
{
  idx_t nmoved;
  idx_t nmoved0;
  idx_t nmoved1;
  auto r0 = CursorCharR( buf, pos, 1, &nmoved0 );
  auto r1 = CursorSingleNewlineR( buf, pos, &nmoved1 );
  if( Equal( pos, r0 ) ) {
    pos = r1;
    nmoved = nmoved1;
  } else {
    if( Equal( pos, r1 ) ) {
      pos = r0;
      nmoved = nmoved0;
    } else {
      pos = Max( r0, r1 );
      nmoved = MAX( nmoved0, nmoved1 );
    }
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return pos;
}



content_ptr_t
CursorSkipWordSpacetabNewlineL( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved ) // TODO: rename!
{
  idx_t nmoved;
  idx_t nmoved0;
  idx_t nmoved1;
  idx_t nmoved2;
  auto l0 = CursorSkipCharNewlineL( buf, pos, &nmoved0 );
  auto l1 = CursorStopAtNonWordCharL( buf, pos, &nmoved1 );
  auto l2 = CursorSkipSpacetabL( buf, pos, &nmoved2 );
  // pick min of { l0, l1, l2 } s.t. pos != min.
  if( Equal( pos, l0 ) ) {
    if( Equal( pos, l1 ) ) {
      pos = l2;
      nmoved = nmoved2;
    } else {
      if( Equal( pos, l2 ) ) {
        pos = l1;
        nmoved = nmoved1;
      } else {
        pos = Min( l1, l2 );
        nmoved = MIN( nmoved1, nmoved2 );
      }
    }
  } else {
    if( Equal( pos, l1 ) ) {
      if( Equal( pos, l2 ) ) {
        pos = l0;
        nmoved = nmoved0;
      } else {
        pos = Min( l0, l2 );
        nmoved = MIN( nmoved0, nmoved2 );
      }
    } else {
      if( Equal( pos, l2 ) ) {
        pos = Min( l0, l1 );
        nmoved = MIN( nmoved0, nmoved1 );
      } else {
        pos = Min3( l0, l1, l2 );
        nmoved = MIN3( nmoved0, nmoved1, nmoved2 );
      }
    }
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return pos;
}

content_ptr_t
CursorSkipWordSpacetabNewlineR( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved ) // TODO: rename!
{
  idx_t nmoved;
  idx_t nmoved0;
  idx_t nmoved1;
  idx_t nmoved2;
  auto r0 = CursorSkipCharNewlineR( buf, pos, &nmoved0 );
  auto r1 = CursorStopAtNonWordCharR( buf, pos, &nmoved1 );
  auto r2 = CursorSkipSpacetabR( buf, pos, &nmoved2 );
  // pick max of { r0, r1, r2 } s.t. pos != max.
  if( Equal( pos, r0 ) ) {
    if( Equal( pos, r1 ) ) {
      pos = r2;
      nmoved = nmoved2;
    } else {
      if( Equal( pos, r2 ) ) {
        pos = r1;
        nmoved = nmoved1;
      } else {
        pos = Max( r1, r2 );
        nmoved = MAX( nmoved1, nmoved2 );
      }
    }
  } else {
    if( Equal( pos, r1 ) ) {
      if( Equal( pos, r2 ) ) {
        pos = r0;
        nmoved = nmoved0;
      } else {
        pos = Max( r0, r2 );
        nmoved = MAX( nmoved0, nmoved2 );
      }
    } else {
      if( Equal( pos, r2 ) ) {
        pos = Max( r0, r1 );
        nmoved = MAX( nmoved0, nmoved1 );
      } else {
        pos = Max3( r0, r1, r2 );
        nmoved = MAX3( nmoved0, nmoved1, nmoved2 );
      }
    }
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return pos;
}



content_ptr_t
CursorHome( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved )
{
  idx_t nmoved;
  idx_t nmoved0;
  idx_t nmoved1;
  auto line = CursorStopAtNewlineL( buf, pos, &nmoved0 );
  auto whitespace = CursorSkipSpacetabR( buf, line, &nmoved1 );
  if( Equal( pos, whitespace ) ) {
    pos = line;
    nmoved = nmoved0;
  } else {
    pos = whitespace;
    nmoved = nmoved1;
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return pos;
}

content_ptr_t
CursorEnd( buf_t& buf, content_ptr_t pos, idx_t* nchars_moved )
{
  idx_t nmoved;
  idx_t nmoved0;
  idx_t nmoved1;
  auto line = CursorStopAtNewlineR( buf, pos, &nmoved0 );
  if( Equal( pos, line ) ) {
    pos = CursorSkipSpacetabL( buf, pos, &nmoved1 );
    nmoved =
      ( nmoved0 <= nmoved1 )  ?
        nmoved1 - nmoved0  :
        nmoved0 - nmoved1;
  } else {
    pos = line;
    nmoved = nmoved0;
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return pos;
}



Inl content_ptr_t
CursorLineU(
  buf_t& buf,
  content_ptr_t pos,
  idx_t pos_inline,
  idx_t* nchars_moved
  )
{
  ProfFunc();

  AssertCrash( !nchars_moved ); // TODO: implement
  idx_t nmoved = 0;

  auto line0_start = CursorStopAtNewlineL( buf, pos, 0 );
  auto line1_end = CursorSingleNewlineL( buf, line0_start, 0 );
  if( !Equal( line0_start, line1_end ) ) {
    auto line1_start = CursorStopAtNewlineL( buf, line1_end, 0 );
    auto inl = CursorCharR( buf, line1_start, pos_inline, 0 );
    pos = Min( line1_end, inl );
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return pos;
}

Inl content_ptr_t
CursorLineD(
  buf_t& buf,
  content_ptr_t pos,
  idx_t pos_inline,
  idx_t* nchars_moved
  )
{
  ProfFunc();

  AssertCrash( !nchars_moved ); // TODO: implement
  idx_t nmoved = 0;

  auto line0_end = CursorStopAtNewlineR( buf, pos, 0 );
  auto line1_start = CursorSingleNewlineR( buf, line0_end, 0 );
  if( !Equal( line0_end, line1_start ) ) {
    auto line1_end = CursorStopAtNewlineR( buf, line1_start, 0 );
    auto inl = CursorCharR( buf, line1_start, pos_inline, 0 );
    pos = Min( line1_end, inl );
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return pos;
}

content_ptr_t
CursorLineU(
  buf_t& buf,
  content_ptr_t pos,
  idx_t pos_inline,
  idx_t nlines,
  idx_t* dlines,
  idx_t* nchars_moved
  )
{
  ProfFunc();

  AssertCrash( !nchars_moved ); // TODO: implement
  idx_t nmoved = 0;

  if( dlines ) {
    *dlines = 0;
  }
  while( nlines-- ) {
    auto pre = pos;
    pos = CursorLineU( buf, pos, pos_inline, 0 );
    auto post = pos;
    if( Equal( pre, post ) ) {
      break;
    } else {
      if( dlines ) {
        *dlines += 1;
      }
    }
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return pos;
}

content_ptr_t
CursorLineD(
  buf_t& buf,
  content_ptr_t pos,
  idx_t pos_inline,
  idx_t nlines,
  idx_t* dlines,
  idx_t* nchars_moved
  )
{
  ProfFunc();

  AssertCrash( !nchars_moved ); // TODO: implement
  idx_t nmoved = 0;

  if( dlines ) {
    *dlines = 0;
  }
  while( nlines-- ) {
    auto pre = pos;
    pos = CursorLineD( buf, pos, pos_inline, 0 );
    auto post = pos;
    if( Equal( pre, post ) ) {
      break;
    } else {
      if( dlines ) {
        *dlines += 1;
      }
    }
  }
  if( nchars_moved ) {
    *nchars_moved = nmoved;
  }
  return pos;
}


Inl idx_t
CountLinesBetween(
  buf_t& buf,
  content_ptr_t ln_start0,
  content_ptr_t ln_start1
  )
{
  ProfFunc();

  // TODO: is this necessary?
  // Must pass in pos at start of line!
  auto test0 = CursorStopAtNewlineL( buf, ln_start0, 0 );
  auto test1 = CursorStopAtNewlineL( buf, ln_start1, 0 );
  AssertCrash( Equal( ln_start0, test0 ) );
  AssertCrash( Equal( ln_start1, test1 ) );

  auto yl = ln_start0;
  auto yr = ln_start1;

  idx_t count = 0;
  auto y = yr;
  while( Greater( y, yl ) ) {
    y = CursorLineU( buf, y, 0, 1, 0, 0 );
    count += 1;
  }

  return count;
}

Inl idx_t
CountBytesBetween(
  buf_t& buf,
  content_ptr_t a,
  content_ptr_t b
  )
{
  AssertCrash( LEqual( a, b ) );

  if( a.diff_idx == b.diff_idx ) {
    idx_t count = b.offset_into_diff - a.offset_into_diff;
    return count;
  }

  idx_t count = 0;
  AssertCrash( a.diff_idx < buf.diffs.len );
  auto diff_a = buf.diffs.mem + a.diff_idx;
  count += diff_a->slice.len - a.offset_into_diff;

  For( i, a.diff_idx + 1, b.diff_idx ) {
    AssertCrash( i < buf.diffs.len );
    auto diff = buf.diffs.mem + i;
    count += diff->slice.len;
  }

  if( b.diff_idx < buf.diffs.len ) {
    count += b.offset_into_diff;
  }
  return count;
}

Inl idx_t
CountCharsBetween(
  buf_t& buf,
  content_ptr_t a,
  content_ptr_t b
  )
{
  // TODO: wrong for UTF8!
  return CountBytesBetween( buf, a, b );
}



Inl string_t
AllocContents( buf_t& buf, content_ptr_t start, content_ptr_t end )
{
  string_t r;
  auto len = CountBytesBetween( buf, start, end );
  Alloc( r, len );
  Contents( buf, start, r.mem, len );
  return r;
}

Inl string_t
AllocContents( buf_t& buf, content_ptr_t ptr, idx_t len )
{
  string_t r;
  Alloc( r, len );
  Contents( buf, ptr, r.mem, len );
  return r;
}

Inl string_t
AllocContents( buf_t& buf )
{
  if( !buf.diffs.len ) {
    string_t r = {};
    return r;
  }
  content_ptr_t ptr = GetBOF( buf );
  return AllocContents( buf, ptr, buf.content_len );
}
