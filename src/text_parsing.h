// Copyright (c) John A. Carlos Jr., all rights reserved.

// This text component is intended to contain higher level text operations, like:
// - cursor movement
// - newline counting/parsing/splitting
// - comma counting/parsing/splitting
// - word boundary counting/parsing/splitting
// - space/tab boundary counting/parsing/splitting

#define CURSORMOVE( _idx_type, _name, _cond ) \
  ForceInl _idx_type \
  NAMEJOIN( _name, L )( u8* src, _idx_type src_len, _idx_type pos ) \
  { \
    while( pos ) { \
      pos -= 1; \
      if( _cond( src[pos] ) ) { \
        pos += 1; \
        break; \
      } \
    } \
    return pos; \
  } \
  ForceInl _idx_type \
  NAMEJOIN( _name, R )( u8* src, _idx_type src_len, _idx_type pos ) \
  { \
    while( pos != src_len ) { \
      if( _cond( src[pos] ) ) { \
        break; \
      } \
      pos += 1; \
    } \
    return pos; \
  } \

#define COND( c )   ( c == '\r' )  ||  ( c == '\n' )
CURSORMOVE( u32, CursorStopAtNewline, COND );
CURSORMOVE( u64, CursorStopAtNewline, COND );
#undef COND

#define COND( c )   ( c == ' '  )  ||  ( c == '\t' )
CURSORMOVE( u32, CursorStopAtSpacetab, COND );
CURSORMOVE( u64, CursorStopAtSpacetab, COND );
#undef COND

#define COND( c )   ( c == ','  )
CURSORMOVE( u32, CursorStopAtComma, COND );
CURSORMOVE( u64, CursorStopAtComma, COND );
#undef COND

#define COND( c )   ( c == '/'  )
CURSORMOVE( u32, CursorStopAtForwSlash, COND );
CURSORMOVE( u64, CursorStopAtForwSlash, COND );
#undef COND

#define COND( c )   ( c != ' '  )  &&  ( c != '\t' )
CURSORMOVE( u32, CursorSkipSpacetab, COND );
CURSORMOVE( u64, CursorSkipSpacetab, COND );
#undef COND

#define COND( c )   ( !AsciiInWord( c ) )
CURSORMOVE( u32, CursorStopAtNonWordChar, COND );
CURSORMOVE( u64, CursorStopAtNonWordChar, COND );
#undef COND

#define COND( c )   ( AsciiInWord( c ) )
CURSORMOVE( u32, CursorStopAtWordChar, COND );
CURSORMOVE( u64, CursorStopAtWordChar, COND );
#undef COND

#undef CURSORMOVE

template<typename _idx_type, typename StopCondition>
ForceInl _idx_type
GenericCursorStopAtL( u8* src, _idx_type src_len, _idx_type pos, StopCondition fn_stop_condition )
{
  while( pos ) {
    pos -= 1;
    if( fn_stop_condition( src[pos] ) ) {
      pos += 1;
      break;
    }
  }
  return pos;
}
template<typename _idx_type, typename StopCondition>
ForceInl _idx_type
GenericCursorStopAtR( u8* src, _idx_type src_len, _idx_type pos, StopCondition fn_stop_condition )
{
  while( pos != src_len ) {
    if( fn_stop_condition( src[pos] ) ) {
      break;
    }
    pos += 1;
  }
  return pos;
}



Templ Inl T
CursorInlineEnd(
  u8* line,
  T line_len,
  T pos
  )
{
  // note the first 'end' should take us to actual eol.
  // this is different than 'home' behavior, but it's what i tend to expect.
  auto eol = line_len;
  auto eol_whitespace = CursorSkipSpacetabL( line, line_len, eol );
  if( pos == eol ) {
    return eol_whitespace;
  }
  else {
    return eol;
  }
}

Templ Inl T
CursorInlineHome(
  u8* line,
  T line_len,
  T pos
  )
{
  // note the first 'home' should take us to the bol_whitespace.
  // this is different from 'end' behavior, but it's what i tend to expect.
  auto bol_whitespace = CursorSkipSpacetabR( line, line_len, 0 );
  if( pos == bol_whitespace ) {
    return 0;
  }
  else {
    return bol_whitespace;
  }
}


Inl idx_t
CursorCharL( u8* src, idx_t src_len, idx_t pos )
{
  if( !pos ) {
    return 0;
  } else {
    return pos - 1;
  }
}

Inl idx_t
CursorCharR( u8* src, idx_t src_len, idx_t pos )
{
  if( pos == src_len ) {
    return pos;
  } else {
    return pos + 1;
  }
}

Inl u32
CountNewlines(
  u8* src,
  idx_t src_len
  )
{
#if 0
  u32 count = 0;
  idx_t idx = 0;
  while( idx < src_len ) {
    while( idx < src_len  &&  ( src[idx] != '\r'  &&  src[idx] != '\n' ) ) {
      idx += 1;
    }
    // skip a single CR, CRLF, or LF
    if( idx + 1 < src_len  &&  MemEqual( src + idx, "\r\n", 2 ) ) {
      idx += 2;
      count += 1;
    }
    elif( idx < src_len  &&  src[idx] == '\r' ) {
      idx += 1;
      count += 1;
    }
    elif( idx < src_len  &&  src[idx] == '\n' ) {
      idx += 1;
      count += 1;
    }
  }
  return count;
#else
  u32 count = 0;
  auto last = src + src_len;
  Forever {
    if( src == last ) {
      break;
    }

    auto c = *src++;
    if( c == '\r' ) {
      count += 1;

      if( src != last  &&  *src == '\n' ) {
        src += 1;
      }
    }
    elif( c == '\n' ) {
      count += 1;
    }
  }
  return count;
#endif
}

Inl void
SplitIntoLines(
  stack_resizeable_cont_t<slice32_t>* lines,
  u8* src,
  idx_t src_len
  )
{
  idx_t idx = 0;
  idx_t num_cr = 0;
  idx_t num_lf = 0;
  idx_t num_crlf = 0;
  idx_t line_start = 0;
  Forever {
    if( idx == src_len ) {
      break;
    }

    auto c = src[idx];
    if( c == '\r' || c == '\n' ) {
      // found an eol, so emit a line.
      auto line_end = idx;
      auto line_len = line_end - line_start;
      auto line = AddBack( *lines );
      line->mem = src + line_start;
      AssertCrash( line_len <= MAX_u32 );
      line->len = Cast( u32, line_len );

      // now advance line_start past the eol for the next line.
      if( idx + 1 < src_len  &&  c == '\r'  &&  src[idx + 1] == '\n' ) {
        idx += 2;
        num_crlf += 1;
      }
      elif( c == '\r' ) {
        idx += 1;
        num_cr += 1;
      }
      else {
        idx += 1;
        num_lf += 1;
      }
      line_start = idx;
    }
    else {
      idx += 1;
    }
  }

  // emit the final line.
  {
    auto line_end = idx;
    auto line_len = line_end - line_start;
    auto line = AddBack( *lines );
    line->mem = src + line_start;
    AssertCrash( line_len <= MAX_u32 );
    line->len = Cast( u32, line_len );
  }
}

Inl void
SplitIntoLines(
  stack_resizeable_pagelist_t<slice32_t>* lines,
  u8* src,
  idx_t src_len,
  idx_t* num_cr_,
  idx_t* num_lf_,
  idx_t* num_crlf_
  )
{
  idx_t idx = 0;
  idx_t num_cr = 0;
  idx_t num_lf = 0;
  idx_t num_crlf = 0;
  idx_t line_start = 0;
  Forever {
    if( idx == src_len ) {
      break;
    }

    auto c = src[idx];
    if( c == '\r' || c == '\n' ) {
      // found an eol, so emit a line.
      auto line_end = idx;
      auto line_len = line_end - line_start;
      auto line = AddBack( *lines );
      line->mem = src + line_start;
      AssertCrash( line_len <= MAX_u32 );
      line->len = Cast( u32, line_len );

      // now advance line_start past the eol for the next line.
      if( idx + 1 < src_len  &&  c == '\r'  &&  src[idx + 1] == '\n' ) {
        idx += 2;
        num_crlf += 1;
      }
      elif( c == '\r' ) {
        idx += 1;
        num_cr += 1;
      }
      else {
        idx += 1;
        num_lf += 1;
      }
      line_start = idx;
    }
    else {
      idx += 1;
    }
  }

  // emit the final line.
  {
    auto line_end = idx;
    auto line_len = line_end - line_start;
    auto line = AddBack( *lines );
    line->mem = src + line_start;
    AssertCrash( line_len <= MAX_u32 );
    line->len = Cast( u32, line_len );
  }

  *num_cr_ = num_cr;
  *num_lf_ = num_lf;
  *num_crlf_ = num_crlf;
}

Inl idx_t
CountCommas(
  u8* src,
  idx_t src_len
  )
{
  idx_t count = 0;
  For( i, 0, src_len ) {
    if( src[i] == ',' ) {
      count += 1;
    }
  }
  return count;
}

Inl void
SplitByCommas(
  stack_resizeable_cont_t<slice_t>* entries,
  u8* src,
  idx_t src_len
  )
{
  idx_t idx = 0;
  while( idx < src_len ) {
    auto elem_start = idx;
    auto elem_end = CursorStopAtCommaR( src, src_len, idx );
    auto elem_len = elem_end - elem_start;
    auto elem = AddBack( *entries );
    elem->mem = src + elem_start;
    elem->len = elem_len;
    idx = elem_end + 1;
    // final trailing comma.
    if( idx == src_len ) {
      elem = AddBack( *entries );
      elem->mem = src + idx;
      elem->len = 0;
      break;
    }
  }
}
Inl void
SplitByForwSlashes(
  stack_resizeable_cont_t<slice_t>* entries,
  u8* src,
  idx_t src_len
  )
{
  idx_t idx = 0;
  while( idx < src_len ) {
    auto elem_start = idx;
    auto elem_end = CursorStopAtForwSlashR( src, src_len, idx );
    auto elem_len = elem_end - elem_start;
    auto elem = AddBack( *entries );
    elem->mem = src + elem_start;
    elem->len = elem_len;
    idx = elem_end + 1;
    // final trailing forwslash.
    if( idx == src_len ) {
      elem = AddBack( *entries );
      elem->mem = src + idx;
      elem->len = 0;
      break;
    }
  }
}
Inl void
SplitBy(
  stack_resizeable_cont_t<slice_t>* entries,
  slice_t split_chars,
  u8* src,
  idx_t src_len
  )
{
  auto StopCondition = [split_chars](u8 c)
  {
    ForLen( i, split_chars ) {
      if( split_chars.mem[i] == c ) return 1;
    }
    return 0;
  };
  idx_t idx = 0;
  while( idx < src_len ) {
    auto elem_start = idx;
    auto elem_end = GenericCursorStopAtR( src, src_len, idx, StopCondition );
    auto elem_len = elem_end - elem_start;
    auto elem = AddBack( *entries );
    elem->mem = src + elem_start;
    elem->len = elem_len;
    idx = elem_end + 1;
    // final trailing forwslash.
    if( idx == src_len ) {
      elem = AddBack( *entries );
      elem->mem = src + idx;
      elem->len = 0;
      break;
    }
  }
}

struct
wordspan_t
{
  idx_t l;
  idx_t r;
  bool inword;
};
Inl void
SplitIntoWords(
  stack_resizeable_cont_t<wordspan_t>* words,
  u8* src,
  idx_t src_len
  )
{
  idx_t idx = 0;
  while( idx < src_len ) {
    auto span = AddBack( *words );
    span->l = idx;
    auto word_end = CursorStopAtNonWordCharR( src, src_len, idx );
    auto nonword_end = CursorStopAtWordCharR( src, src_len, idx );
    AssertCrash( word_end != nonword_end );
    if( idx == word_end ) {
      // nonword.
      span->r = nonword_end;
      span->inword = 0;
      idx = nonword_end;
    }
    else {
      // word.
      span->r = word_end;
      span->inword = 1;
      idx = word_end;
    }
  }
}

Inl void
SplitBySpacesAndCopyContents(
  pagelist_t* dst_mem,
  stack_resizeable_cont_t<slice_t>* dst,
  u8* src,
  idx_t src_len
  )
{
  Reserve( *dst, src_len / 2 );
  idx_t idx = 0;
  while( idx < src_len ) {
    auto elem_start = CursorSkipSpacetabR( src, src_len, idx );
    auto elem_end = CursorStopAtSpacetabR( src, src_len, elem_start );
    auto elem_len = elem_end - elem_start;
    if( !elem_len ) {
      break;
    }
    auto elem = AddBack( *dst );
    elem->mem = AddPagelist( *dst_mem, u8, 1, elem_len );
    elem->len = elem_len;
    Memmove( elem->mem, src + elem_start, elem_len );
    idx = elem_end;
  }
}

ForceInl slice_t
TrimSpacetabsPrefixAndSuffix( slice_t value )
{
  auto start = CursorSkipSpacetabR( ML( value ), 0 );
  AssertCrash( start <= value.len );
  value.mem += start;
  value.len -= start;
  auto end = CursorSkipSpacetabL( ML( value ), value.len );
  value.len = end;
  return value;
}
ForceInl slice_t
TrimSpacetabsPrefix( slice_t value )
{
  auto start = CursorSkipSpacetabR( ML( value ), 0 );
  AssertCrash( start <= value.len );
  value.mem += start;
  value.len -= start;
  return value;
}


#if defined(TEST)

static void
TestCoreText()
{
  {
    stack_resizeable_cont_t<slice32_t> lines;
    Alloc( lines, 1024 );
    SplitIntoLines( &lines, ML( SliceFromCStr( "abc\ndef\nghi\n\njkl\n" ) ) );
    AssertCrash( lines.len == 6 );
    AssertCrash( EqualContents( lines.mem[0], Slice32FromCStr( "abc" ) ) );
    AssertCrash( EqualContents( lines.mem[1], Slice32FromCStr( "def" ) ) );
    AssertCrash( EqualContents( lines.mem[2], Slice32FromCStr( "ghi" ) ) );
    AssertCrash( EqualContents( lines.mem[3], Slice32FromCStr( "" ) ) );
    AssertCrash( EqualContents( lines.mem[4], Slice32FromCStr( "jkl" ) ) );
    AssertCrash( EqualContents( lines.mem[5], Slice32FromCStr( "" ) ) );
    Free( lines );
  }

  {
    stack_resizeable_cont_t<slice_t> entries;
    Alloc( entries, 1024 );
    SplitByCommas( &entries, ML( SliceFromCStr( "abc,def,ghi,,jkl," ) ) );
    AssertCrash( entries.len == 6 );
    AssertCrash( EqualContents( entries.mem[0], SliceFromCStr( "abc" ) ) );
    AssertCrash( EqualContents( entries.mem[1], SliceFromCStr( "def" ) ) );
    AssertCrash( EqualContents( entries.mem[2], SliceFromCStr( "ghi" ) ) );
    AssertCrash( EqualContents( entries.mem[3], SliceFromCStr( "" ) ) );
    AssertCrash( EqualContents( entries.mem[4], SliceFromCStr( "jkl" ) ) );
    AssertCrash( EqualContents( entries.mem[5], SliceFromCStr( "" ) ) );
    Free( entries );
  }

  {
    struct
    {
      u32 value;
      u8* str;
    } cstrs[] = {
      { 0, Str( "" ) },
      { 0, Str( " " ) },
      { 1, Str( "\n" ) },
      { 1, Str( "\r" ) },
      { 1, Str( "\r\n" ) },
      { 2, Str( "\n\n" ) },
      { 2, Str( "\n\r" ) },
      { 2, Str( "\n\r\n" ) },
      { 2, Str( "\r\r" ) },
      { 2, Str( "\r\r\n" ) },
      { 2, Str( "\r\n\r" ) },
      { 2, Str( "\r\n\n" ) },
      { 2, Str( "\r\n\r\n" ) },
      };
    ForEach( cstr, cstrs ) {
      u32 count = CountNewlines( cstr.str, CstrLength( cstr.str ) );
      AssertCrash( count == cstr.value );
    }
  }
}

#endif // defined(TEST)
