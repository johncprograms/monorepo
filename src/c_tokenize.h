// Copyright (c) John A. Carlos Jr., all rights reserved.

#define TOKENTYPES( _x ) \
  _x( eol ) \
  _x( numeral ) \
  _x( string_ ) \
  _x( char_ ) \
  _x( ident ) \
  _x( global ) \
  _x( alias ) \
  _x( include_ ) \
  _x( if_ ) \
  _x( elif_ ) \
  _x( else_ ) \
  _x( while_ ) \
  _x( continue_ ) \
  _x( break_ ) \
  _x( for_ ) \
  _x( return_ ) \
  _x( switch_ ) \
  _x( case_ ) \
  _x( default_ ) \
  _x( struct_ ) \
  _x( enum_ ) \
  _x( fn ) \
  _x( data ) \
  _x( defer ) \
  _x( ltlteq ) \
  _x( gtgteq ) \
  _x( slashslash ) \
  _x( slashstar ) \
  _x( starslash ) \
  _x( eqeq ) \
  _x( lteq ) \
  _x( gteq ) \
  _x( arrow ) \
  _x( pluseq ) \
  _x( minuseq ) \
  _x( stareq ) \
  _x( slasheq ) \
  _x( percenteq ) \
  _x( ampersandeq ) \
  _x( pipeeq ) \
  _x( careteq ) \
  _x( ltlt ) \
  _x( gtgt ) \
  _x( exclamationeq ) \
  _x( exclamation ) \
  _x( caret ) \
  _x( ampersand ) \
  _x( pipe ) \
  _x( star ) \
  _x( plus ) \
  _x( minus ) \
  _x( slash ) \
  _x( backslash ) \
  _x( percent ) \
  _x( bracket_curly_l ) \
  _x( bracket_curly_r ) \
  _x( bracket_square_l ) \
  _x( bracket_square_r ) \
  _x( paren_l ) \
  _x( paren_r ) \
  _x( dot ) \
  _x( comma ) \
  _x( doublequote ) \
  _x( singlequote ) \
  _x( eq ) \
  _x( lt ) \
  _x( gt ) \
  _x( semicolon ) \
  _x( colon ) \
  _x( unrecognized ) \


Enumc( tokentype_t )
{
  #define CASE( name )   name,
  TOKENTYPES( CASE )
  #undef CASE
};

struct
token_t
{
  u8* mem;
  idx_t len;
  u8* bol;
  idx_t lineno;
  tokentype_t type;
};



struct
tlog_t
{
  idx_t nerrs;
  idx_t nwarns;
  array_t<u8> errs;
  array_t<u8> warns;
};

Inl void
Init( tlog_t& log )
{
  log.nerrs = 0;
  log.nwarns = 0;
  Alloc( log.errs, 16384 );
  Alloc( log.warns, 16384 );
}

Inl void
Kill( tlog_t& log )
{
  Free( log.errs );
  Free( log.warns );
}



struct
src_t
{
  slice_t filename;
  string_t file;
};


Inl void
OutputFileAndLine(
  slice_t& filename,
  array_t<u8>& file,
  token_t* tkn
  )
{
  embeddedarray_t<u8, 64> lineno;
  CsFrom_u64( lineno.mem, Capacity( lineno ), &lineno.len, Cast( u64, tkn->lineno ) );
  embeddedarray_t<u8, 64> inlinech;
  CsFrom_u64( inlinech.mem, Capacity( inlinech ), &inlinech.len, Cast( u64, tkn->mem - tkn->bol + 1 ) );

  Memmove( AddBack( file, filename.len ), ML( filename ) );
  Memmove( AddBack( file, 3 ), " : ", 3 );
  Memmove( AddBack( file, lineno.len ), ML( lineno ) );
  Memmove( AddBack( file, 3 ), " : ", 3 );
  Memmove( AddBack( file, inlinech.len ), ML( inlinech ) );
  Memmove( AddBack( file, 3 ), " : ", 3 );
}


Inl void
OutputSrcLineAndCaret(
  array_t<u8>& file,
  src_t* src,
  token_t* tkn
  )
{
  auto bol = tkn->bol;

  Forever {
    AssertCrash( bol < src->file.mem + src->file.len );
    u8 c = *bol;
    if( c == ' '  ||  c == '\t' ) {
      bol += 1;
    } else {
      break;
    }
  }
  u8* eol = Cast( u8*, MemScan( bol, src->file.len - ( bol - src->file.mem ), "\n", 1 ) );
  idx_t linelen = CsLen( bol, eol );

  Memmove( AddBack( file, 4 ), "    ", 4 );
  Memmove( AddBack( file, linelen ), bol, linelen );
  *AddBack( file ) = '\n';
  Memmove( AddBack( file, 4 ), "    ", 4 );

  AssertCrash( tkn->mem >= bol );
  sidx_t inlinech = tkn->mem - bol;
  while( inlinech-- ) {
    *AddBack( file ) = ' ';
  }
  *AddBack( file ) = '^';
  *AddBack( file ) = '\n';
}


void
Error(
  token_t* tkn,
  src_t* src,
  tlog_t* log,
  void* errstr
  )
{
  log->nerrs += 1;

  OutputFileAndLine( src->filename, log->errs, tkn );

  auto errstr_len = CsLen( Cast( u8*, errstr ) );
  Memmove( AddBack( log->errs, errstr_len ), errstr, errstr_len );
  *AddBack( log->errs ) = '\n';

  OutputSrcLineAndCaret( log->errs, src, tkn );
}


using symbol_t = slice_t;

static symbol_t Sym_main     = { Str( "Main" ), 4 };

static symbol_t Sym_global   = { Str( "global"   ), 6 };
static symbol_t Sym_alias    = { Str( "alias"    ), 5 };
static symbol_t Sym_include_ = { Str( "include"  ), 7 };
static symbol_t Sym_if_      = { Str( "if"       ), 2 };
static symbol_t Sym_elif_    = { Str( "elif"     ), 4 };
static symbol_t Sym_else_    = { Str( "else"     ), 4 };
static symbol_t Sym_while_   = { Str( "while"    ), 5 };
static symbol_t Sym_continue_= { Str( "continue" ), 8 };
static symbol_t Sym_break_   = { Str( "break"    ), 5 };
static symbol_t Sym_for_     = { Str( "for"      ), 3 };
static symbol_t Sym_return_  = { Str( "ret"      ), 3 };
static symbol_t Sym_switch_  = { Str( "switch"   ), 6 };
static symbol_t Sym_case_    = { Str( "case"     ), 4 };
static symbol_t Sym_default_ = { Str( "default"  ), 7 };
static symbol_t Sym_struct_  = { Str( "struct"   ), 6 };
static symbol_t Sym_enum_    = { Str( "enum"     ), 4 };
static symbol_t Sym_fn       = { Str( "fn"       ), 2 };
static symbol_t Sym_data     = { Str( "data"     ), 4 };
static symbol_t Sym_defer    = { Str( "defer"    ), 5 };

static symbol_t Sym_ltlteq = { Str( "<<=" ), 3 };
static symbol_t Sym_gtgteq = { Str( ">>=" ), 3 };

static symbol_t Sym_slashslash    = { Str( "//" ), 2 };
static symbol_t Sym_slashstar     = { Str( "/*" ), 2 };
static symbol_t Sym_starslash     = { Str( "*/" ), 2 };
static symbol_t Sym_eqeq          = { Str( "==" ), 2 };
static symbol_t Sym_lteq          = { Str( "<=" ), 2 };
static symbol_t Sym_gteq          = { Str( ">=" ), 2 };
static symbol_t Sym_arrow         = { Str( "->" ), 2 };
static symbol_t Sym_pluseq        = { Str( "+=" ), 2 };
static symbol_t Sym_minuseq       = { Str( "-=" ), 2 };
static symbol_t Sym_stareq        = { Str( "*=" ), 2 };
static symbol_t Sym_slasheq       = { Str( "/=" ), 2 };
static symbol_t Sym_percenteq     = { Str( "%=" ), 2 };
static symbol_t Sym_ampersandeq   = { Str( "&=" ), 2 };
static symbol_t Sym_pipeeq        = { Str( "|=" ), 2 };
static symbol_t Sym_careteq       = { Str( "^=" ), 2 };
static symbol_t Sym_ltlt          = { Str( "<<" ), 2 };
static symbol_t Sym_gtgt          = { Str( ">>" ), 2 };
static symbol_t Sym_exclamationeq = { Str( "!=" ), 2 };

static symbol_t Sym_exclamation      = { Str( "!"  ), 1 };
static symbol_t Sym_caret            = { Str( "^"  ), 1 };
static symbol_t Sym_ampersand        = { Str( "&"  ), 1 };
static symbol_t Sym_pipe             = { Str( "|"  ), 1 };
static symbol_t Sym_star             = { Str( "*"  ), 1 };
static symbol_t Sym_plus             = { Str( "+"  ), 1 };
static symbol_t Sym_minus            = { Str( "-"  ), 1 };
static symbol_t Sym_slash            = { Str( "/"  ), 1 };
static symbol_t Sym_backslash        = { Str( "\\" ), 1 };
static symbol_t Sym_percent          = { Str( "%"  ), 1 };
static symbol_t Sym_bracket_curly_l  = { Str( "{"  ), 1 };
static symbol_t Sym_bracket_curly_r  = { Str( "}"  ), 1 };
static symbol_t Sym_bracket_square_l = { Str( "["  ), 1 };
static symbol_t Sym_bracket_square_r = { Str( "]"  ), 1 };
static symbol_t Sym_paren_l          = { Str( "("  ), 1 };
static symbol_t Sym_paren_r          = { Str( ")"  ), 1 };
static symbol_t Sym_dot              = { Str( "."  ), 1 };
static symbol_t Sym_comma            = { Str( ","  ), 1 };
static symbol_t Sym_doublequote      = { Str( "\"" ), 1 };
static symbol_t Sym_singlequote      = { Str( "'"  ), 1 };
static symbol_t Sym_eq               = { Str( "="  ), 1 };
static symbol_t Sym_lt               = { Str( "<"  ), 1 };
static symbol_t Sym_gt               = { Str( ">"  ), 1 };
static symbol_t Sym_semicolon        = { Str( ";"  ), 1 };
static symbol_t Sym_colon            = { Str( ":"  ), 1 };


Inl bool
IsFirstIdentChar( u8 c )
{
  bool r =
    ( c == '_' )  |
    IsAlpha( c );
  return r;
}

Inl bool
IsIdentChar( u8 c )
{
  bool r =
    ( c == '_' )  |
    IsAlpha( c )  |
    IsNumber( c );
  return r;
}



struct
tokenspan_t
{
  idx_t l;
  idx_t r;
};

// removes ( // ... eol ) spans.
// removes ( /* ... */ ) spans. handles nested comments of this style!
// removes all eols.
// convert ( " ... " ) spans into string tokens.
// convert ( ' ... ' ) spans into char tokens.
static void
PostProcessTokens( array_t<token_t>& tokens, src_t* src, tlog_t* log )
{
  array_t<tokenspan_t> removespans;
  Alloc( removespans, 1024 );

  idx_t pos = 0;

  while( pos < tokens.len ) {

    token_t* tkns = tokens.mem + pos;

    // remove ( '//', ..., eol ) style comments.
    if( tkns[0].type == tokentype_t::slashslash ) {
      idx_t offset = 1;
      while( pos + offset < tokens.len ) {
        token_t* test = tkns + offset;
        if( test->type == tokentype_t::eol ) {
          break;
        }
        offset += 1;
      }
      if( pos + offset >= tokens.len ) {
        tokens.len = pos;
        break;
      }
      tokenspan_t remove;
      remove.l = pos;
      remove.r = pos + offset + 1;
      *AddBack( removespans ) = remove;

      pos += offset + 1;

      continue;
    }

    // remove ( '/*', ..., '*/' ) style comments.
    // NOTE: we handle nested comments of this style correctly!
    if( tkns[0].type == tokentype_t::slashstar ) {
      idx_t ntofind = 1;
      idx_t offset = 1;
      while( pos + offset < tokens.len ) {
        token_t* test = tkns + offset;
        if( test->type == tokentype_t::slashstar ) {
          ntofind += 1;
          offset += 1;
          continue;
        }
        if( test->type == tokentype_t::starslash ) {
          ntofind -= 1;
          if( !ntofind ) {
            break;
          }
          offset += 1;
          continue;
        }
        offset += 1;
      }
      if( pos + offset >= tokens.len ) {
        Error( tkns + 0, src, log, Str( "missing a right bound on a '/*', '*/' style comment!" ) );
        return;
      }
      tokenspan_t remove;
      remove.l = pos;
      remove.r = pos + offset + 1;
      *AddBack( removespans ) = remove;

      pos += offset + 1;

      continue;
    }

    // remove eol sequences.
    if( tkns[0].type == tokentype_t::eol ) {
      idx_t offset = 1;
      while( pos + offset < tokens.len ) {
        token_t* test = tkns + offset;
        if( test->type == tokentype_t::eol ) {
          offset += 1;
          continue;
        }
        break;
      }
      if( pos + offset >= tokens.len ) {
        tokens.len = pos;
        break;
      }
      tokenspan_t remove;
      remove.l = pos;
      remove.r = pos + offset;
      *AddBack( removespans ) = remove;

      pos += offset;

      continue;
    }

    // replace ( doublequote , ..., doublequote ) by string and remove the extra tokens.
    if( tkns[0].type == tokentype_t::doublequote ) {
      idx_t offset = 1;
      while( pos + offset < tokens.len ) {
        token_t* test = tkns + offset;
        if( test->type == tokentype_t::doublequote ) {
          break;
        }
        offset += 1;
        continue;
      }
      if( pos + offset >= tokens.len ) {
        Error( tkns + 0, src, log, Str( "Missing a right bound on a '\"', '\"' string literal!" ) );
        return;
      }
      tkns[0].len = tkns[offset].len;
      tkns[0].type = tokentype_t::string_;
      tokenspan_t remove;
      remove.l = pos + 1;
      remove.r = pos + offset + 1;
      *AddBack( removespans ) = remove;

      pos += offset + 1;

      continue;
    }

    // replace( singlequote, ..., singlequote ) by char and remove the extra tokens.
    if( tkns[0].type == tokentype_t::singlequote ) {
      idx_t offset = 1;
      while( pos + offset < tokens.len ) {
        token_t* test = tkns + offset;
        if( test->type == tokentype_t::singlequote ) {
          break;
        }
        offset += 1;
        continue;
      }
      if( pos + offset >= tokens.len ) {
        Error( tkns + 0, src, log, Str( "missing a right bound on a '\'', '\'' char literal!" ) );
        return;
      }
      tkns[0].len = tkns[offset].len;
      tkns[0].type = tokentype_t::string_;
      tokenspan_t remove;
      remove.l = pos + 1;
      remove.r = pos + offset + 1;
      if( remove.r > remove.l ) {
        *AddBack( removespans ) = remove;
      }

      pos += offset + 1;

      continue;
    }

    pos += 1;
  }

  // TODO: optimal ordered re-contiguouize.
  idx_t i = removespans.len;
  while( i ) {
    i -= 1;
    tokenspan_t* span = removespans.mem + i;
    idx_t spanlen = span->r - span->l;
    RemAt( tokens, span->l, spanlen );
  }

  Free( removespans );
}



Inl void
TokenizeNumeral(
  src_t* src,
  array_t<token_t>& tokens,
  idx_t pos,
  u8* curr,
  token_t* tkn,
  idx_t* pos_offset,
  tlog_t* log
  )
{
  // TODO: allow scientific notation, ie 1.2e9 syntax.

  // this assumes IsNumber( curr[0] ), which is checked by the caller.
  idx_t offset = 1;
  idx_t ndots = 0;
  bool last_was_dot = 0;
  while( pos + offset < src->file.len ) {

    if( IsNumber( curr[offset] ) ) {
      last_was_dot = 0;
      offset += 1;
      continue;
    }
    if( curr[offset] == '.' ) {
      if( ndots >= 1 ) {
        Error( tkn, src, log, Str( "numeral can have only one decimal point!" ) );
      }
      ndots += 1;
      last_was_dot = 1;
      offset += 1;
      continue;
    }
    break;
  }
  if( last_was_dot ) {
    Error( tkn, src, log, Str( "numeral can't end in a decimal point!" ) );
  }

  tkn->len = offset;
  tkn->type = tokentype_t::numeral;
  *AddBack( tokens ) = *tkn;

  *pos_offset = offset;
}




#define TokenizeSymbol( pos, tkn, tokens, curr, curr_len, tokenname ) \
  if( MemEqual( curr, MIN( curr_len, NAMEJOIN( Sym_, tokenname ).len ), ML( NAMEJOIN( Sym_, tokenname ) ) ) ) { \
    tkn.len = NAMEJOIN( Sym_, tokenname ).len; \
    tkn.type = NAMEJOIN( tokentype_t::, tokenname ); \
    *AddBack( tokens ) = tkn; \
    pos += NAMEJOIN( Sym_, tokenname ).len; \
    continue; \
  }

#define TokenizeKeyword( pos, tkn, tokens, curr, curr_len, tokenname ) \
  if( MemEqual( curr, curr_len, ML( NAMEJOIN( Sym_, tokenname ) ) ) ) { \
    tkn.len = NAMEJOIN( Sym_, tokenname ).len; \
    tkn.type = NAMEJOIN( tokentype_t::, tokenname ); \
    *AddBack( tokens ) = tkn; \
    pos += NAMEJOIN( Sym_, tokenname ).len; \
    continue; \
  }



void
Tokenize( array_t<token_t>& tokens, src_t* src, tlog_t* log )
{
  idx_t last_bol = 0;
  idx_t line_count = 1;

  idx_t pos = 0;

  while( pos < src->file.len ) {

    token_t tkn;
    tkn.mem = src->file.mem + pos;
    tkn.bol = src->file.mem + last_bol;
    tkn.lineno = line_count;

    u8* curr = src->file.mem + pos;
    idx_t curr_len = src->file.len - pos;

    // skip over whitespace.
    if( ( curr[0] == ' ' )  |  ( curr[0] == '\t' )  ) {
      pos += 1;
      continue;
    }

    // tokenize eol.
    // NOTE: possible EOL symbols:
    //   LF: \n
    //   CR: \r
    //   CRLF: \r\n
    if( ( curr[0] == '\r' )  |  ( curr[0] == '\n' ) ) {

      line_count += 1;

      if( curr[0] == '\n' ) {
        tkn.len = 1;
        tkn.type = tokentype_t::eol;
        *AddBack( tokens ) = tkn;

        pos += 1;
        last_bol = pos;

        continue;

      } else {
        if( pos + 1 < src->file.len ) {
          if( curr[1] == '\n' ) {
            tkn.len = 2;
            tkn.type = tokentype_t::eol;
            *AddBack( tokens ) = tkn;

            pos += 2;
            last_bol = pos;

            continue;
          }
        }
        tkn.len = 1;
        tkn.type = tokentype_t::eol;
        *AddBack( tokens ) = tkn;

        pos += 1;
        last_bol = pos;

        continue;
      }
    }

    // tokenize numerals.
    // TODO: include negative sign in numeral, so we can determine possible typings.
    if( IsNumber( curr[0] ) ) {
      idx_t offset;
      TokenizeNumeral( src, tokens, pos, curr, &tkn, &offset, log );
      if( log->nerrs ) return;
      pos += offset;
      continue;
    }
    //if( ( curr[0] == '-' )  |  IsNumber( curr[0] ) ) {
    //  if( curr[0] == '-' ) {
    //    if( pos + 1 < src->file.len ) {
    //      if( IsNumber( curr[1] ) ) {
    //        idx_t offset;
    //        TokenizeNumeral( src, tokens, pos, curr, tkn, &offset, log );
    //        if( log->nerrs ) return;
    //        pos += offset;
    //        continue;
    //      }
    //    }
    //  } else {
    //    idx_t offset;
    //    TokenizeNumeral( src, tokens, pos, curr, tkn, &offset, log );
    //    if( log->nerrs ) return;
    //    pos += offset;
    //    continue;
    //  }
    //}

    if( IsFirstIdentChar( curr[0] ) ) {
      idx_t offset = 1;
      while( pos + offset < src->file.len ) {
        if( IsIdentChar( curr[offset] ) ) {
          offset += 1;
          continue;
        }
        break;
      }
      // NOTE: we're allowing _ as an accepted ident here.

      // tokenize keywords.
      // NOTE: keywords take precedence over variable idents.
      TokenizeKeyword( pos, tkn, tokens, curr, offset, global );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, alias );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, include_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, if_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, elif_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, else_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, while_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, continue_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, break_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, for_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, return_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, switch_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, case_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, default_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, struct_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, enum_ );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, fn );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, data );
      TokenizeKeyword( pos, tkn, tokens, curr, offset, defer );

      // tokenize idents that aren't keywords.
      tkn.len = offset;
      tkn.type = tokentype_t::ident;
      *AddBack( tokens ) = tkn;
      pos += offset;
      continue;
    }

    // tokenize three-char symbols.
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, ltlteq );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, gtgteq );

    // tokenize two-char symbols.
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, slashslash    );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, slashstar     );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, starslash     );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, eqeq          );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, lteq          );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, gteq          );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, arrow         );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, pluseq        );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, minuseq       );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, stareq        );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, slasheq       );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, percenteq     );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, ampersandeq   );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, pipeeq        );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, careteq       );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, ltlt          );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, gtgt          );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, exclamationeq );

    // tokenize one-char symbols.
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, exclamation      );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, caret            );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, ampersand        );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, pipe             );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, star             );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, plus             );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, minus            );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, slash            );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, backslash        );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, percent          );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, bracket_curly_l  );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, bracket_curly_r  );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, bracket_square_l );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, bracket_square_r );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, paren_l          );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, paren_r          );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, dot              );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, comma            );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, doublequote      );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, singlequote      );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, eq               );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, lt               );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, gt               );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, semicolon        );
    TokenizeSymbol( pos, tkn, tokens, curr, curr_len, colon            );

    // tokenize unrecognized characters!
    tkn.len = 1;
    tkn.type = tokentype_t::unrecognized;
    *AddBack( tokens ) = tkn;
    pos += 1;
    continue;
  }

  PostProcessTokens( tokens, src, log );
}

#undef TokenizeSymbol
#undef TokenizeKeyword












// ============================================================================
//
// NOTE: debug code below here.
//


Inl u8*
StringOfTokenType( tokentype_t type )
{
  switch( type ) {
    #define CASE( name )   case tokentype_t::name: return Str( # name );
    TOKENTYPES( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}



Inl void
PrintTokens( array_t<token_t>& tokens, slice_t& src )
{
  ForLen( i, tokens ) {
    auto& tkn = tokens.mem[i];

    switch( tkn.type ) {

      case tokentype_t::eol: {
        static auto cr = Str( "CR" );
        static auto lf = Str( "LF" );
        static auto crlf = Str( "CRLF" );
        u8* eol_type;
        if( MemEqual( ML( tkn ), "\r\n", 2 ) ) {
          eol_type = crlf;
        } elif( MemEqual( ML( tkn ), "\r", 1 ) ) {
          eol_type = cr;
        } else {
          eol_type = lf;
        }
        printf(
          "%s = %s"
          "\n",
          StringOfTokenType( tkn.type ),
          eol_type
          );
      } break;

      default: {
        auto tmp = MemHeapAlloc( u8, tkn.len + 1 );
        Memmove( tmp, ML( tkn ) );
        tmp[tkn.len] = 0;
        printf(
          "%s = \"%s\""
          "\n",
          StringOfTokenType( tkn.type ),
          tmp
          );
        MemHeapFree( tmp );
      } break;
    }
  }
}
