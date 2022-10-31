// build:console_x64_debug
// Copyright (c) John A. Carlos Jr., all rights reserved.

#define FINDLEAKS   0
#include "core_cruntime.h"
#include "core_types.h"
#include "core_language_macros.h"
#include "os_mac.h"
#include "os_windows.h"
#include "memory_operations.h"
#include "asserts.h"
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
#include "ds_list.h"
#include "ds_stack_cstyle.h"
#include "cstr_integer.h"
#include "cstr_float.h"
#include "ds_stack_resizeable_cont_addbacks.h"
#include "ds_hashset_cstyle.h"
#include "filesys.h"
#include "timedate.h"
#include "ds_mtqueue_mrmw_nonresizeable.h"
#include "ds_mtqueue_mrsw_nonresizeable.h"
#include "ds_mtqueue_srmw_nonresizeable.h"
#include "ds_mtqueue_srsw_nonresizeable.h"
#include "threading.h"
#define LOGGER_ENABLED   0
#include "logger.h"
#define PROF_ENABLED   0
#define PROF_ENABLED_AT_LAUNCH   0
#include "profile.h"
#include "rand.h"
#include "allocator_heap_findleaks.h"
#include "mainthread.h"


Templ struct
opt_t
{
  T value;
  bool present;
};

static slice_t c_sym_main     = SliceFromCStr( "Main" );

static slice_t c_sym_if_      = SliceFromCStr( "if"       );
static slice_t c_sym_elif_    = SliceFromCStr( "elif"     );
static slice_t c_sym_else_    = SliceFromCStr( "else"     );
static slice_t c_sym_while_   = SliceFromCStr( "while"    );
static slice_t c_sym_continue_= SliceFromCStr( "continue" );
static slice_t c_sym_break_   = SliceFromCStr( "break"    );
static slice_t c_sym_for_     = SliceFromCStr( "for"      );
static slice_t c_sym_ret      = SliceFromCStr( "ret"      );
static slice_t c_sym_switch_  = SliceFromCStr( "switch"   );
static slice_t c_sym_case_    = SliceFromCStr( "case"     );
static slice_t c_sym_struct_  = SliceFromCStr( "struct"   );
static slice_t c_sym_enum_    = SliceFromCStr( "enum"     );
static slice_t c_sym_defer    = SliceFromCStr( "defer"    );
static slice_t c_sym_deref    = SliceFromCStr( "deref"    );
static slice_t c_sym_addrof   = SliceFromCStr( "addrof"   );

static slice_t c_sym_ltlteq = SliceFromCStr( "<<=" );
static slice_t c_sym_gtgteq = SliceFromCStr( ">>=" );

static slice_t c_sym_slashslash    = SliceFromCStr( "//" );
static slice_t c_sym_slashstar     = SliceFromCStr( "/*" );
static slice_t c_sym_starslash     = SliceFromCStr( "*/" );
static slice_t c_sym_eqeq          = SliceFromCStr( "==" );
static slice_t c_sym_lteq          = SliceFromCStr( "<=" );
static slice_t c_sym_gteq          = SliceFromCStr( ">=" );
static slice_t c_sym_arrow_r       = SliceFromCStr( "->" );
static slice_t c_sym_arrow_l       = SliceFromCStr( "<-" );
static slice_t c_sym_pluseq        = SliceFromCStr( "+=" );
static slice_t c_sym_minuseq       = SliceFromCStr( "-=" );
static slice_t c_sym_stareq        = SliceFromCStr( "*=" );
static slice_t c_sym_slasheq       = SliceFromCStr( "/=" );
static slice_t c_sym_percenteq     = SliceFromCStr( "%=" );
static slice_t c_sym_ampersandeq   = SliceFromCStr( "&=" );
static slice_t c_sym_pipeeq        = SliceFromCStr( "|=" );
static slice_t c_sym_careteq       = SliceFromCStr( "^=" );
static slice_t c_sym_ltlt          = SliceFromCStr( "<<" );
static slice_t c_sym_gtgt          = SliceFromCStr( ">>" );
static slice_t c_sym_exclamationeq = SliceFromCStr( "!=" );

static slice_t c_sym_exclamation      = SliceFromCStr( "!"  );
static slice_t c_sym_caret            = SliceFromCStr( "^"  );
static slice_t c_sym_ampersand        = SliceFromCStr( "&"  );
static slice_t c_sym_pipe             = SliceFromCStr( "|"  );
static slice_t c_sym_star             = SliceFromCStr( "*"  );
static slice_t c_sym_plus             = SliceFromCStr( "+"  );
static slice_t c_sym_minus            = SliceFromCStr( "-"  );
static slice_t c_sym_slash            = SliceFromCStr( "/"  );
static slice_t c_sym_backslash        = SliceFromCStr( "\\" );
static slice_t c_sym_percent          = SliceFromCStr( "%"  );
static slice_t c_sym_bracket_curly_l  = SliceFromCStr( "{"  );
static slice_t c_sym_bracket_curly_r  = SliceFromCStr( "}"  );
static slice_t c_sym_bracket_square_l = SliceFromCStr( "["  );
static slice_t c_sym_bracket_square_r = SliceFromCStr( "]"  );
static slice_t c_sym_paren_l          = SliceFromCStr( "("  );
static slice_t c_sym_paren_r          = SliceFromCStr( ")"  );
static slice_t c_sym_dot              = SliceFromCStr( "."  );
static slice_t c_sym_comma            = SliceFromCStr( ","  );
static slice_t c_sym_doublequote      = SliceFromCStr( "\"" );
static slice_t c_sym_singlequote      = SliceFromCStr( "'"  );
static slice_t c_sym_eq               = SliceFromCStr( "="  );
static slice_t c_sym_lt               = SliceFromCStr( "<"  );
static slice_t c_sym_gt               = SliceFromCStr( ">"  );
static slice_t c_sym_semicolon        = SliceFromCStr( ";"  );
static slice_t c_sym_colon            = SliceFromCStr( ":"  );

#define TOKENTYPES( _x ) \
  _x( eol ) \
  _x( number ) \
  _x( string_ ) \
  _x( char_ ) \
  _x( ident ) \
  _x( if_ ) \
  _x( elif_ ) \
  _x( else_ ) \
  _x( while_ ) \
  _x( continue_ ) \
  _x( break_ ) \
  _x( for_ ) \
  _x( ret ) \
  _x( switch_ ) \
  _x( case_ ) \
  _x( struct_ ) \
  _x( enum_ ) \
  _x( defer ) \
  _x( deref ) \
  _x( addrof ) \
  _x( ltlteq ) \
  _x( gtgteq ) \
  _x( slashslash ) \
  _x( slashstar ) \
  _x( starslash ) \
  _x( eqeq ) \
  _x( lteq ) \
  _x( gteq ) \
  _x( arrow_l ) \
  _x( arrow_r ) \
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
Inl slice_t
StringSliceOfTokenType( tokentype_t type )
{
  switch( type ) {
    #define CASE( name )   case tokentype_t::name: return { Str( # name ), CstrLength( Str( # name ) ) };
    TOKENTYPES( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}

struct
token_t
{
  // PERF: packing. could make len/lineno u32
  tokentype_t type;
  idx_t len;
  u8* mem;
  u8* bol;
  idx_t lineno;
};

Inl void
AddBackString(
  stack_resizeable_cont_t<u8>* array,
  slice_t text
  )
{
  AddBackContents( array, text );
}
Inl void
AddBackString(
  stack_resizeable_cont_t<u8>* array,
  const void* cstr
  )
{
  auto slice = SliceFromCStr( cstr );
  AddBackString( array, slice );
}

Inl void
PrintTokens(
  stack_resizeable_cont_t<u8>* out,
  stack_resizeable_cont_t<token_t>* tokens
  )
{
  ForLen( i, *tokens ) {
    auto tkn = tokens->mem + i;
    switch( tkn->type ) {
      case tokentype_t::eol: {
        static auto cr = Str( "CR" );
        static auto lf = Str( "LF" );
        static auto crlf = Str( "CRLF" );
        u8* eol_type;
        if( MemEqual( ML( *tkn ), "\r\n", 2 ) ) {
          eol_type = crlf;
        } elif( MemEqual( ML( *tkn ), "\r", 1 ) ) {
          eol_type = cr;
        } else {
          eol_type = lf;
        }
        AddBackString( out, StringSliceOfTokenType( tkn->type ) );
        AddBackString( out, " = " );
        AddBackString( out, eol_type );
        AddBackString( out, "\n" );
      } break;
      default: {
        AddBackString( out, StringSliceOfTokenType( tkn->type ) );
        AddBackString( out, " = \"" );
        AddBackString( out, { ML( *tkn ) } );
        AddBackString( out, "\"\n" );
      } break;
    }
  }
}


struct
tokenspan_t
{
  idx_t l;
  idx_t r;
};

struct
globaltypes_t;
struct
vartable_t;
struct
function_t;

struct
compilecontext_t
{
  slice_t filename;
  slice_t file;
  stack_resizeable_cont_t<u8> errors;
  stack_resizeable_cont_t<token_t> tokens;
  pagelist_t mem;
  globaltypes_t* globaltypes; // ptr to avoid decl cycle.
  stack_resizeable_cont_t<vartable_t*> scopestack;
  function_t* current_function;
  u8 ptr_bytecount;
  u8 array_bytecount;
};


Inl void
OutputFileAndLine(
  compilecontext_t* ctx,
  u8* bol,
  u8* inline_mem,
  u64 lineno
  )
{
  AddBackString( &ctx->errors, ctx->filename );
  AddBackString( &ctx->errors, ":" );
  stack_nonresizeable_stack_t<u8, 64> tmp;
  CsFrom_u64( tmp.mem, Capacity( tmp ), &tmp.len, lineno );
  AddBackString( &ctx->errors, SliceFromArray( tmp ) );
  AddBackString( &ctx->errors, ":" );
  CsFrom_u64( tmp.mem, Capacity( tmp ), &tmp.len, Cast( u64, inline_mem - bol + 1 ) );
  AddBackString( &ctx->errors, SliceFromArray( tmp ) );
  AddBackString( &ctx->errors, " " );
}
Inl void
OutputFileAndLine(
  compilecontext_t* ctx,
  token_t* tkn
  )
{
  OutputFileAndLine(
    ctx,
    tkn->bol,
    tkn->mem,
    tkn->lineno
    );
}

Inl void
OutputSrcLineAndCaret(
  compilecontext_t* ctx,
  u8* bol,
  u8* inline_mem
  )
{
  auto max_bol = ctx->file.mem + ctx->file.len;
  while( bol < max_bol  &&  AsciiIsSpaceTab( *bol ) ) {
    bol += 1;
  }

  auto eol = bol;
  auto max_eol = MIN( bol + 80, ctx->file.mem + ctx->file.len );
  while( eol < max_eol  &&  !AsciiIsNewlineCh( *eol ) ) {
    eol += 1;
  }

  auto linelen = CstrLength( bol, eol );
  slice_t linecontents = { bol, linelen };
  AddBackString( &ctx->errors, "    " );
  AddBackString( &ctx->errors, linecontents );
  AddBackString( &ctx->errors, "\n    " );

  AssertCrash( inline_mem >= bol );
  sidx_t inlinech = inline_mem - bol;
  while( inlinech-- ) {
    *AddBack( ctx->errors ) = ' ';
  }
  *AddBack( ctx->errors ) = '^';
  *AddBack( ctx->errors ) = '\n';
}
Inl void
OutputSrcLineAndCaret(
  compilecontext_t* ctx,
  token_t* tkn
  )
{
  OutputSrcLineAndCaret(
    ctx,
    tkn->bol,
    tkn->mem
    );
}

Inl void
ErrorCStr(
  compilecontext_t* ctx,
  u8* bol,
  u8* inline_mem,
  u64 lineno,
  const void* errstr
  )
{
  OutputFileAndLine(
    ctx,
    bol,
    inline_mem,
    lineno
    );

  AddBackString( &ctx->errors, errstr );
  *AddBack( ctx->errors ) = '\n';

  OutputSrcLineAndCaret(
    ctx,
    bol,
    inline_mem
    );
}

Inl void
ErrorCStr(
  compilecontext_t* ctx,
  token_t* tkn,
  const void* errstr
  )
{
  ErrorCStr(
    ctx,
    tkn->bol,
    tkn->mem,
    tkn->lineno,
    errstr
    );
}

void
Tokenize(
  compilecontext_t* ctx
  )
{
  auto tokens = &ctx->tokens;

  idx_t last_bol = 0;
  idx_t line_count = 1; // 1-based indexing for user output

  idx_t pos = 0;

  while( pos < ctx->file.len ) {

    auto curr = ctx->file.mem + pos;
    auto curr_len = ctx->file.len - pos;

    // skip over whitespace.
    if( AsciiIsSpaceTab( curr[0] ) ) {
      pos += 1;
      continue;
    }

    token_t tkn;
    tkn.mem = ctx->file.mem + pos;
    tkn.bol = ctx->file.mem + last_bol;
    tkn.lineno = line_count;

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
        *AddBack( *tokens ) = tkn;

        pos += 1;
        last_bol = pos;

        continue;

      } else {
        if( pos + 1 < ctx->file.len ) {
          if( curr[1] == '\n' ) {
            tkn.len = 2;
            tkn.type = tokentype_t::eol;
            *AddBack( *tokens ) = tkn;

            pos += 2;
            last_bol = pos;

            continue;
          }
        }
        tkn.len = 1;
        tkn.type = tokentype_t::eol;
        *AddBack( *tokens ) = tkn;

        pos += 1;
        last_bol = pos;

        continue;
      }
    }

    // tokenize numbers.
    // note we include the negative sign in number, so we can determine possible typings.
    // this is due to the bounds abs-value mismatch for signed ints.
    bool found_num = 0;
    idx_t offset_num_start = 0;
    bool num_negative = 0;
    if( curr[0] == '-' ) {
      idx_t offset = 1;
      while( pos + offset < ctx->file.len  &&  AsciiIsSpaceTab( curr[offset] ) ) {
        offset += 1;
      }
      if( pos + offset < ctx->file.len ) {
        if( AsciiIsNumber( curr[offset] ) ) {
          found_num = 1;
          num_negative = 1;
          offset_num_start = offset;
        }
      }
    }
    elif( AsciiIsNumber( curr[0] ) ) {
      found_num = 1;
      num_negative = 0;
      offset_num_start = 0;
    }
    if( found_num ) {
      // we've determined AsciiIsNumber( curr[offset_num_start] )
      idx_t offset = 1;
      bool seen_dot = 0;
      bool seen_e = 0;
      // TODO: negative exponents
//      bool seen_e_negativesign = 0;
      while( pos + offset < ctx->file.len ) {
        auto c = curr[offset];
        if( AsciiIsNumber( c ) ) {
          offset += 1;
          continue;
        }
        elif( c == '.' ) {
          if( seen_dot ) {
            ErrorCStr(
              ctx,
              tkn.bol,
              tkn.mem,
              tkn.lineno,
              "numbers can only have one decimal point!"
              );
            return;
          }
          if( seen_e ) {
            ErrorCStr(
              ctx,
              tkn.bol,
              tkn.mem,
              tkn.lineno,
              "number exponent can't contain a decimal point!"
              );
            return;
          }
          seen_dot = 1;
          offset += 1;
          continue;
        }
        elif( c == 'e'  ||  c == 'E' ) {
          if( seen_e ) {
            ErrorCStr(
              ctx,
              tkn.bol,
              tkn.mem,
              tkn.lineno,
              "numbers can only have one exponent!"
              );
            return;
          }
          seen_e = 1;
          offset += 1;
          continue;
        }
        break;
      }
      tkn.len = offset;
      tkn.type = tokentype_t::number;
      *AddBack( *tokens ) = tkn;
      pos += offset;
      continue;
    }

    if( ( curr[0] == '_' )  ||  AsciiIsAlpha( curr[0] ) ) {
      idx_t offset = 1;
      while( pos + offset < ctx->file.len ) {
        auto c = curr[offset];
        if( ( c == '_' )  ||  AsciiIsAlpha( c )  ||  AsciiIsNumber( c ) ) {
          offset += 1;
          continue;
        }
        break;
      }
      // NOTE: we're allowing _ as an accepted ident here.

#define TOKENIZE_KEYWORD( _tokenname ) \
  if( MemEqual( curr, offset, ML( NAMEJOIN( c_sym_, _tokenname ) ) ) ) { \
    tkn.len = NAMEJOIN( c_sym_, _tokenname ).len; \
    tkn.type = tokentype_t::_tokenname; \
    *AddBack( *tokens ) = tkn; \
    pos += NAMEJOIN( c_sym_, _tokenname ).len; \
    continue; \
  } \

      // tokenize keywords.
      // NOTE: keywords take precedence over variable idents.
      TOKENIZE_KEYWORD( if_ );
      TOKENIZE_KEYWORD( elif_ );
      TOKENIZE_KEYWORD( else_ );
      TOKENIZE_KEYWORD( while_ );
      TOKENIZE_KEYWORD( continue_ );
      TOKENIZE_KEYWORD( break_ );
      TOKENIZE_KEYWORD( for_ );
      TOKENIZE_KEYWORD( ret );
      TOKENIZE_KEYWORD( switch_ );
      TOKENIZE_KEYWORD( case_ );
      TOKENIZE_KEYWORD( struct_ );
      TOKENIZE_KEYWORD( enum_ );
      TOKENIZE_KEYWORD( defer );
      TOKENIZE_KEYWORD( deref );
      TOKENIZE_KEYWORD( addrof );

#undef TOKENIZE_KEYWORD

      // tokenize idents that aren't keywords.
      tkn.len = offset;
      tkn.type = tokentype_t::ident;
      *AddBack( *tokens ) = tkn;
      pos += offset;
      continue;
    }

#define TOKENIZE_SYMBOL( _tokenname ) \
  if( MemEqual( curr, MIN( curr_len, NAMEJOIN( c_sym_, _tokenname ).len ), ML( NAMEJOIN( c_sym_, _tokenname ) ) ) ) { \
    tkn.len = NAMEJOIN( c_sym_, _tokenname ).len; \
    tkn.type = tokentype_t::_tokenname; \
    *AddBack( *tokens ) = tkn; \
    pos += NAMEJOIN( c_sym_, _tokenname ).len; \
    continue; \
  } \

    // tokenize three-char symbols.
    TOKENIZE_SYMBOL( ltlteq );
    TOKENIZE_SYMBOL( gtgteq );

    // tokenize two-char symbols.
    TOKENIZE_SYMBOL( slashslash    );
    TOKENIZE_SYMBOL( slashstar     );
    TOKENIZE_SYMBOL( starslash     );
    TOKENIZE_SYMBOL( eqeq          );
    TOKENIZE_SYMBOL( lteq          );
    TOKENIZE_SYMBOL( gteq          );
    TOKENIZE_SYMBOL( arrow_l       );
    TOKENIZE_SYMBOL( arrow_r       );
    TOKENIZE_SYMBOL( pluseq        );
    TOKENIZE_SYMBOL( minuseq       );
    TOKENIZE_SYMBOL( stareq        );
    TOKENIZE_SYMBOL( slasheq       );
    TOKENIZE_SYMBOL( percenteq     );
    TOKENIZE_SYMBOL( ampersandeq   );
    TOKENIZE_SYMBOL( pipeeq        );
    TOKENIZE_SYMBOL( careteq       );
    TOKENIZE_SYMBOL( ltlt          );
    TOKENIZE_SYMBOL( gtgt          );
    TOKENIZE_SYMBOL( exclamationeq );

    // tokenize one-char symbols.
    TOKENIZE_SYMBOL( exclamation      );
    TOKENIZE_SYMBOL( caret            );
    TOKENIZE_SYMBOL( ampersand        );
    TOKENIZE_SYMBOL( pipe             );
    TOKENIZE_SYMBOL( star             );
    TOKENIZE_SYMBOL( plus             );
    TOKENIZE_SYMBOL( minus            );
    TOKENIZE_SYMBOL( slash            );
    TOKENIZE_SYMBOL( backslash        );
    TOKENIZE_SYMBOL( percent          );
    TOKENIZE_SYMBOL( bracket_curly_l  );
    TOKENIZE_SYMBOL( bracket_curly_r  );
    TOKENIZE_SYMBOL( bracket_square_l );
    TOKENIZE_SYMBOL( bracket_square_r );
    TOKENIZE_SYMBOL( paren_l          );
    TOKENIZE_SYMBOL( paren_r          );
    TOKENIZE_SYMBOL( dot              );
    TOKENIZE_SYMBOL( comma            );
    TOKENIZE_SYMBOL( doublequote      );
    TOKENIZE_SYMBOL( singlequote      );
    TOKENIZE_SYMBOL( eq               );
    TOKENIZE_SYMBOL( lt               );
    TOKENIZE_SYMBOL( gt               );
    TOKENIZE_SYMBOL( semicolon        );
    TOKENIZE_SYMBOL( colon            );

    // tokenize unrecognized characters!
    ErrorCStr(
      ctx,
      tkn.bol,
      tkn.mem,
      tkn.lineno,
      "unrecognized character!"
      );
    return;
  }

  // removes ( // ... eol ) spans.
  // removes ( /* ... */ ) spans. handles nested comments of this style!
  // removes all eols.
  // convert ( " ... " ) spans into string tokens.
  // convert ( ' ... ' ) spans into char tokens.

  stack_resizeable_cont_t<tokenspan_t> removespans;
  Alloc( removespans, 1024 );

  pos = 0;
  while( pos < tokens->len ) {

    token_t* tkns = tokens->mem + pos;

    // remove ( '//', ..., eol ) style comments.
    if( tkns[0].type == tokentype_t::slashslash ) {
      idx_t offset = 1;
      while( pos + offset < tokens->len  &&  tkns[offset].type != tokentype_t::eol ) {
        offset += 1;
      }
      if( pos + offset >= tokens->len ) {
        // found a '//' but we hit eof before any eol.
        tokens->len = pos;
        break;
      }
      auto remove = AddBack( removespans );
      remove->l = pos;
      remove->r = pos + offset + 1;
      pos += offset + 1;
      continue;
    }

    // remove ( '/*', ..., '*/' ) style comments.
    // NOTE: we handle nested comments of this style correctly!
    if( tkns[0].type == tokentype_t::slashstar ) {
      idx_t ntofind = 1;
      idx_t offset = 1;
      while( pos + offset < tokens->len ) {
        token_t* test = tkns + offset;
        if( test->type == tokentype_t::slashstar ) {
          ntofind += 1;
        }
        elif( test->type == tokentype_t::starslash ) {
          ntofind -= 1;
          if( !ntofind ) {
            break;
          }
        }
        offset += 1;
      }
      if( pos + offset >= tokens->len ) {
        ErrorCStr(
          ctx,
          tkns + 0,
          "missing a right bound on a '/*', '*/' style comment!"
          );
        return;
      }
      auto remove = AddBack( removespans );
      remove->l = pos;
      remove->r = pos + offset + 1;
      pos += offset + 1;
      continue;
    }

    // remove eol sequences.
    if( tkns[0].type == tokentype_t::eol ) {
      idx_t offset = 1;
      while( pos + offset < tokens->len  &&  tkns[offset].type == tokentype_t::eol ) {
        offset += 1;
      }
      if( pos + offset >= tokens->len ) {
        // eol sequence at eof.
        tokens->len = pos;
        break;
      }
      auto remove = AddBack( removespans );
      remove->l = pos;
      remove->r = pos + offset;
      pos += offset;
      continue;
    }

    // replace ( doublequote , ..., doublequote ) by string and remove the extra tokens.
    // note we can't handle nested quotes, since the end token is the same as the start token.
    // TODO: escaped quotes.
    if( tkns[0].type == tokentype_t::doublequote ) {
      idx_t offset = 1;
      while( pos + offset < tokens->len  &&  tkns[offset].type != tokentype_t::doublequote ) {
        offset += 1;
      }
      if( pos + offset >= tokens->len ) {
        ErrorCStr(
          ctx,
          tkns + 0,
          "missing a right bound on a '\"', '\"' string literal!"
          );
        return;
      }
      tkns[0].len = tkns[offset].len; // TODO: this looks wrong...
      tkns[0].type = tokentype_t::string_;
      auto remove = AddBack( removespans );
      remove->l = pos + 1;
      remove->r = pos + offset + 1;
      pos += offset + 1;
      continue;
    }

    // replace( singlequote, ..., singlequote ) by char and remove the extra tokens.
    // note we can't handle nested quotes, since the end token is the same as the start token.
    // TODO: escaped quotes.
    if( tkns[0].type == tokentype_t::singlequote ) {
      idx_t offset = 1;
      while( pos + offset < tokens->len  &&  tkns[offset].type != tokentype_t::singlequote ) {
        offset += 1;
      }
      if( pos + offset >= tokens->len ) {
        ErrorCStr(
          ctx,
          tkns + 0,
          "missing a right bound on a '\'', '\'' char literal!"
          );
        return;
      }
      tkns[0].len = tkns[offset].len; // TODO: this looks wrong...
      tkns[0].type = tokentype_t::string_;
      auto remove = AddBack( removespans );
      remove->l = pos + 1;
      remove->r = pos + offset + 1;
      pos += offset + 1;
      continue;
    }

    pos += 1;
  }

  // TODO: optimal ordered re-contiguouize.
  auto i = removespans.len;
  while( i ) {
    i -= 1;
    auto span = removespans.mem + i;
    auto spanlen = span->r - span->l;
    RemAt( *tokens, span->l, spanlen );
  }

  Free( removespans );
}



struct expr_t;
struct scope_t;
struct statement_t;

// TODO: go harder on optional parens/braces. those take shifttyping time, which is slow / not fun.
//   should be able to just remove some paren-expects for cblock.expr, since expr allows parens.
//   might be able to do the same with all scopes, since statement can be a scope.
//     the extra passthru node isn't great. maybe we should inline scope_t into statement_t?
//   optional parens on fncall is probably the hardest, but could be good too.

// TODO: modify ret to be:
//     ret_elem = expr  |  type
//     ret = "ret"  ,-list of ret_elem
//   so we can pass types as rets.
//   do the same for declassign_const

// TODO: keyword 'type_t' which denotes passing types as args/rets, e.g. 'Foo(type_t int_kind)'
//   maybe use 'auto' or 'any' or 'var' or something else, if this system extends to support fndecl type inference.

// TODO: syntax for fndecl polymorph
//   pass as arg, and use directly: 'Foo(foo_t type_t, foo foo_t)'
//     what about out of order?     'Foo(foo foo_t, foo_t type_t)'
//       probably look for type_t first, then process other args.
//     what about type inference in the fncall?
//       we could first check the explicit type_t-passing version, then the version where all type_t are dropped.
//   inline name:                   'Foo(foo $foo_t)'
//     what about duplicates?       'Foo(foo $foo_t, bar * foo_t, clu [] $foo_t)'
//       probably treat all duplicates as the same type, so they don't matter.

// TODO: extend binassign syntax to allow list of expr as the rhs.

// TODO: collect 'fndefn -> list of fncall' dependency info, probably during typing.
//   we'll need that for recursive call graph analysis.

// TODO: more serious array syntax.
//   syntax for making a slice from an array/slice, i.e. bar := foo[2,*,5..10];

// TODO: implement arrays as struct stack_resizeable_cont_t { T* mem, idx_t len };
//   the capacity is implicit; round the len to the next higher power of 2, and that's the current capacity.
//   this makes for a size-efficient array, which i expect we'll be passing around everywhere.
//   this also does exponential size growth, which is req'd for O(1) amortized insert.
//   we'll probably get very creative when it comes to implementing arrays, since we can have arbitrary
//   dimensions, arbitrary dynamic dimensions, etc.

// TODO: simplify binop code by just converting them to functions, and rely on function overloading
//   to pick the appropriate one to use. as long as we define all appropriate combos, this'll work nicely.
//   can probably do the same for most binassign types.

// TODO: simplify binassign code by unpacking most types into a simple eq during parse?
//   would that cause problems with e.g. foo[Blah()] += 17; ?
//   presumably we only want to run Blah() once, not twice as in: foo[Blah()] = foo[Blah()] + 17;
//   doing this too early in the pipe may introduce this problem.
//   looks like we won't know about this until typing, so we have to do the binassign work.

// TODO: function overloading.
//   when we lookup a fndecl for deduping ( see AddFndecltype ), check that the args/rets types match.
//   if they don't, make a new overload of that same fn name.
//   fncall typing also needs to loop over that list of overloads of the given fn name.

// TODO: IER/IER0 isn't quite right for continued processing after an error.
//   we need some kind of bool new_error, and reset of that bool, for error-skipping to work.
//   the error-skip locations are marked with '// intentionally no IER'

// TODO: eliminate string type, and just use [] u8 ?
//   string literals can become '[N] u8' since len is known.

// TODO: constants of enum type won't always work right now.
//   we currently process all enums first, and declassign_const after.
//   that won't work, for: 'c_which foo_t = foo_t.blah;  foo_t enum { blah }'
//   maybe we do the same thing for enums that we do for structs:
//     add the all the enumdecltypes first, but don't fill them in until later.
//   that won't really work here, since declassign_const needs access to the enum values.
//   and, consider: 'c_which foo_t = foo_t.blah;  foo_t enum { blah=c_blah }  c_blah u32 = 17;'
//   we need to process the c_blah declassign_const, then the foo_t enum, then the c_which declassign_const.
//   not sure how we're going to do this.

// TODO: EvalExprConst for arbitrary types, so we can do declassign_const.
//   maybe we should just wait until we can execute our IR. otherwise, we'd dupe a bunch of typing code for this.
//   i'll do the codegen code first, and see if that's actually a reasonable thing to do.

// TODO: defer_code should be per vartable, not one for the whole function.
//   we'll have defers executing past their scopes, which is strange.

// XXXXXXXXXXXXX
// TODO: in the bc layer, we could eliminate stack frame offsets if we just pass things as pointer values.
//   we could use the existence of a tvars array to hold the actual vars, and just point into it.
//   this would make shuffling vars around easier, which we'll want to do.

// TODO: can we eliminate the bc layer?
//   current benefits:
//   - eliminates dead code not reachable from the entry point
//   - verifies entry point is singular and has the right signature.
//   - linearizes all the code, jumps, calls, etc.
//
//   responses:
//   - in our actual endpoint code generators, we can do similar logic. e.g. executables want that, but
//     modules/libraries don't want that, since it's up to the external callers to use things.
//   - we can do this trivially at the end of typing.
//   - fully-linear representation, esp. for calls, isn't really necessary in an in-memory IR.
//     e.g. for VM execution, we can iterate instructions however we want.
//     but doing this was good practice for other kinds of endpoint code generation.
//
//   looks like we can eliminate it, or at least leave it alone for now.
//   writing a replacement tc execution function is probably next up.

// TODO: C++ code generation from the tc layer.

// TODO: how do we do struct dereferencing?
//   in regular expressions, not too bad. just emit struct_field_from_struct for a stackvar holding the value.
//   in binassigns, more complicated.
//   e.g.
//     foo ptrtype_t;
//     foo.iszero = y;               // (1)
//     foo.ptrlevel[0] = y;          // (2)
//     foo.ptrlevel[foo.iszero] = y; // (3)
//     a := foo.iszero;
//     b := foo.ptrlevel[0];
//     c := foo.ptrlevel[foo.iszero];
//   when the struct deref is on the rhs, relatively easy. struct_field_from_struct to make a stackvar holding the value.
//   when it's on the lhs, sometimes we want something else.
//   in (3), we want foo.iszero as a value, but we want to assign to the foo.ptrlevel field as an address.
//   if we treat foo.ptrlevel as a value, we'll just be assigning to a temporary stackvar, which we don't want.
//   since our syntax for expr_assignable is restrictive, we can limit what we want to:
//     "treat only the first deref in the lhs as an address, not a value."
//   so basically, our soln would be to have a special version of TypeExprAssignable which does that special
//   treatment of the first deref in the lhs.
//
//   is this the best line we can draw ?
//   i.e. is there something better than this expr_assignable boundary ?
//   what happens if we want to allow more complicated lhs ?
//   e.g. "AddBack( foo ) <- bar;", maybe we use "=" here instead.
//   we basically just want a pointer on the lhs, doesn't have to be named.
//   well none of that solves the original problem, consider e.g. "foo.bar[foo.car] + i = 10;"
//   if we allow that kind of pointer arith in the lhs, we still have the first deref problem.
//   so it's probably best to solve it on it's own, and stay with the simplified lhs syntax for now.
//   this first lhs vs. later lhs or rhs problem is just something we have to deal with.
//
//   what about "a := addrof foo.bar;" ?
//   this breaks that first-lhs rule, since i'd expect it to just return the pointer, and addrof is a no-op.
//   this is also a more general expr, so you could do "Foo( addrof foo.bar );".
//   but we also want be able to pass the field value, not the address: "Foo( foo.bar );".
//   i'd bet we can handle this the same way as for binassign.
//   just use the same special version of TypeExprAssignable in the addrof cases. not too bad.
//
//   what if we did some syntax desugaring to help out here?
//   the code for singular vars is relatively simple.
//   could we do some intermediate rewriting of struct derefs into separate stackvars for each field?
//   that would help out for addrof cases, but not for the first-lhs binassign problem.
//   there'd also be extra book-keeping for tracking which vars were fields.
//   on balance so far, not a good solution.
//
//   note that we'll likely disallow arrayidx/brace syntax for raw pointers, since they're not all that necessary.
//   i usually do an "x = array_mem + idx;" syntax in C++ anyways.
//   i also want to experiment with array syntaxes, and including pointers only makes things more complicated.
//   e.g. syntax for a builtin stack_resizeable_cont_t, list_t, stack_resizeable_pagelist_t, pagelist_t, stack_implicitcapacity_t, maybe even hashset_t, etc.
//   maybe that won't be all that fruitful, since i get by just fine with functions in C++.
//   should probably do a fncall count in existing code, and just optimize syntax for that.
//



//
//  GRAMMAR:
//  note that { x } means 0 or more of x repeated.
//  note that [ x ] means 0 or 1 of x.
//  "foo" means the literal contents foo in the sourcefile
//
//  sep =
//    ";" | ","
//
//  list of X =
//    { X  sep { sep } } [ X { sep } ]
//      list_t<X>
//


enum num_type_t { unsigned_, signed_, float_ };

//  num =
//    [ "-" ]  digit  { digit }  [ "."  digit  { digit } ]  [ "e"  [ "-" ]  digit  { digit } ]
//    0x  digit  { digit }
//    0b  "0" | "1"  { "0" | "1" }
struct
num_t
{
  token_t* literal;
  num_type_t type;
  u8 bytecount;
  u64 value_u64;
  s64 value_s64;
  f64 value_f64;
  f32 value_f32;
};

//  str =
//    """  { ascii }  """
struct str_t { token_t* literal; };

//  chr =
//    "'"  ascii  "'"
struct chr_t { token_t* literal; };

//  ident =
//    "_" | alpha  { "_" | alpha | digit }
struct ident_t { slice_t text;  token_t* literal; };

// the "*" signifies a dynamic size for that idx
// for example, [*,*] int is a 2-dimensional array with dynamic sizes in both dimensions.
// [200,*,200] int is 3-dimensional, with the second idx allowing dynamic sizing.
//  arrayidx =
//    expr_const
//    "*"
Enumc( typedecl_arrayidx_type_t ) { expr_const, star };
struct typedecl_arrayidx_t { typedecl_arrayidx_type_t type;  expr_t* expr_const; };

//
// [] signifies a slice of an array, i.e. you can't modify the owner array metadata from a slice.
// it's just a duplicate metadata of { mem, len } that points somewhere inside the array.
//
//  qualifier =
//    "*"
//    "[" list of arrayidx "]"
Enumc( typedecl_qualifier_type_t ) { star, array };
struct typedecl_qualifier_t { typedecl_qualifier_type_t type;  list_t<typedecl_arrayidx_t> arrayidxs; };

//  type =
//    { qualifier }  ident
struct typedecl_t { ident_t ident;  list_t<typedecl_qualifier_t> qualifiers; };





//  decl =
//    ident  type
struct decl_t { ident_t ident;  typedecl_t typedecl; };

//  declassign =
//    ident  type  "="  expr
//    ident  ":"  "="  expr
Enumc( declassign_type_t ) { explicit_, implicit_ };
struct declassign_t {
  declassign_type_t type;
  ident_t ident;
  typedecl_t* typedecl; // only present for explicit_
  expr_t* expr;
};

//  fndecl =
//    ident  "("  list of decl  ")"  list of type
struct fndecl_t { ident_t ident;  list_t<decl_t> decl_args;  list_t<typedecl_t> typedecl_rets; };

//  fndefn =
//    ident  "("  list of decl  ")"  list of type  scope
struct fndefn_t { fndecl_t* fndecl;  scope_t* scope; };

//  structdecl =
//    ident  "struct"  "("  list of decl  ")"
struct structdecl_t { ident_t ident;  list_t<decl_t> decl_fields; };

//  enumdecl =
//    ident  "enum"  "("  list of ident [ "="  expr_const ]  ")"
Enumc( enumdecl_entry_type_t ) { ident, identassign };
struct enumdecl_entry_t { enumdecl_entry_type_t type;  ident_t ident;  expr_t* expr; };
struct enumdecl_t { ident_t ident;  list_t<enumdecl_entry_t> enumdecl_entries; };

//
// note we don't allow decl or declassign in global scope, since i plan to do an explicit, named
// 'globals' struct that you can define, and implicitly access with a keyword.
// this removes the need for "fn" prefix syntax, since we don't have a conflict with fncall anymore.
// it's also a global-non-proliferation strategy. or at least a centralization strategy.
// note that we need to allow constants to be defn'd in global scope, since that's a common pattern.
//
//
//  global_statement =
//    fndecl
//    fndefn
//    structdecl
//    enumdecl
//    declassign_const
Enumc( global_statement_type_t ) { fndecl, fndefn, structdecl, enumdecl, declassign_const, };
struct global_statement_t {
  global_statement_type_t type;
  union {
    fndecl_t* fndecl;
    fndefn_t fndefn;
    structdecl_t structdecl;
    enumdecl_t enumdecl;
    declassign_t declassign_const;
  };
};

//  fncall =
//    ident  "("  list of expr  ")"
struct fncall_t { ident_t ident;  list_t<expr_t*> expr_args; };

//  ret =
//    "ret"  ,-list of expr
struct ret_t { list_t<expr_t*> exprs; };

//  whileloop =
//    "while"  expr  scope
struct cond_scope_t { expr_t* expr;  scope_t* scope; };
struct whileloop_t { cond_scope_t cblock; };

//  ifchain =
//    "if"  expr  scope  { "elif"  expr  scope }  [ "else"  scope ]
struct ifchain_t { cond_scope_t cblock_if;  list_t<cond_scope_t> cblock_elifs;  opt_t<scope_t*> scope_else; };

//  defer =
//    "defer"  statement
struct defer_t { statement_t* statement; };

// i.e. "," sep not allowed here
//  scope =
//    "{"  ;-list of statement  "}"
struct scope_t { list_t<statement_t> statements; };

// i.e. "," sep not allowed here
//  global_scope =
//    "{"  ;-list of global_statement  "}"
struct global_scope_t { list_t<global_statement_t> global_statements; };

//
// a = 2;                  a = 2
// a.foo = 2;              a + offsetof( a, foo ) <- 2
// a[1] = 4;               a.mem + 1 * elementsizeof( a ) <- 4
// a[2].foo = 5;           a.mem + 2 * elementsizeof( a ) + offsetof( a, foo ) <- 5
// a.foo[2] = 3;           a + offsetof( a, foo ) + 2 * elementsizeof( a.foo ) <- 3
// AddBack( &a ) <- foo;   we'll disallow this for now, and tell people to make a var for this. improved strings/arrays should make this less common.
//
//  expr_assignable =
//    ident
//    ident  "."  expr_assignable
//    ident  "["  list of expr  "]"
//    ident  "["  list of expr  "]"  "."  expr_assignable
struct expr_assignable_entry_t { ident_t ident;  list_t<expr_t*> expr_arrayidxs; };
struct expr_assignable_t { list_t<expr_assignable_entry_t> expr_assignable_entries; }; // subsequent entries are dot-accesses.

//
// * int is an addressof int, pointer to int, however you want to think about it.
// [] int is a slice of ints, meaning a fixed-size array.
// [] * int is a slice of addresses of int. you'd write the ints with foo[1] <- 2;
// ** int is an address of an address of an int.
//   you'd write the address of an int with foo <- &bar;
//   you'd write the int with foo <-<- bar;
//
// foo int = 5;
//
// bar *int;
// bar = *foo;
// bar <- 4;
//
// baz **int;
// baz = *bar;
// baz <- *foo;
// baz <-<- 5;
//
// is this a step backwards from the C style **foo = 5 syntax?
//
//  binassignop =
//    "="
//    "<-"  { "<-" }
//    "+=" | "-="
//    "*=" | "/=" | "%="
//    "&=" | "|=" | "^="
//    "<<=" | ">>="
Enumc( binassignop_t ) { eq, arrow_l, addeq, subeq, muleq, diveq, modeq, andeq, oreq, poweq };
static tokentype_t c_binassignop_tokens[] = {
  tokentype_t::eq,
  tokentype_t::arrow_l,
  tokentype_t::pluseq,
  tokentype_t::minuseq,
  tokentype_t::stareq,
  tokentype_t::slasheq,
  tokentype_t::percenteq,
  tokentype_t::ampersandeq,
  tokentype_t::pipeeq,
  tokentype_t::careteq,
  };
Inl binassignop_t
BinassignopFromTokentype( tokentype_t type )
{
  switch( type ) {
    case tokentype_t::eq: return binassignop_t::eq;
    case tokentype_t::arrow_l: return binassignop_t::arrow_l;
    case tokentype_t::pluseq: return binassignop_t::addeq;
    case tokentype_t::minuseq: return binassignop_t::subeq;
    case tokentype_t::stareq: return binassignop_t::muleq;
    case tokentype_t::slasheq: return binassignop_t::diveq;
    case tokentype_t::percenteq: return binassignop_t::modeq;
    case tokentype_t::ampersandeq: return binassignop_t::andeq;
    case tokentype_t::pipeeq: return binassignop_t::oreq;
    case tokentype_t::careteq: return binassignop_t::poweq;
    default: UnreachableCrash(); return {};
  }
}

//  binassign =
//    expr_assignable  binassignop  expr
//    "("  list of expr_assignable  ")"  binassignop  expr
struct binassign_t { binassignop_t type;  list_t<expr_assignable_t> expr_assignables;   expr_t* expr; };

//  statement =
//    fncall
//    decl
//    declassign
//    binassign
//    ret
//    defer
//    // TODO: continue/break secondary outer loop? something like break(1) to skip 1 loop, and break the next one.
//    // i.e. break(0) would be the same as break
//    "continue"
//    "break"
//    whileloop
//    ifchain
//    // TODO: switch/case syntax, with "case;" meaning default, and "case expr;" meaning the usual.
//    scope
Enumc( statement_type_t ) { fncall, decl, declassign, binassign, ret, defer, continue_, break_, whileloop, ifchain, scope };
struct statement_t {
  statement_type_t type;
  union {
    fncall_t fncall;
    decl_t decl;
    declassign_t declassign;
    binassign_t binassign;
    ret_t ret;
    defer_t defer;
    whileloop_t whileloop;
    ifchain_t ifchain;
    scope_t* scope;
  };
};

//  unop =
//    "deref" | "-" | "!"
//
//  unop_addrof =
//    "addrof"
//
// note that addrof is treated separately, since it doesn't accept a general expr, unlike the other unops.
// so we make that a parse distinction, to make typing much easier.
//
// WARNING: ParseExprNotBinop is hardcoded to look for these tokens
//
Enumc( unoptype_t ) { deref, negate_num, negate_bool };
Inl slice_t
StringFromUnoptype( unoptype_t type )
{
  switch( type ) {
    case unoptype_t::deref      :  return SliceFromCStr( "deref" );
    case unoptype_t::negate_num :  return SliceFromCStr( "negate_num" );
    case unoptype_t::negate_bool:  return SliceFromCStr( "negate_bool" );
    default: UnreachableCrash();  return {};
  }
}
Inl unoptype_t
UnopFromTokentype( tokentype_t type )
{
  switch( type ) {
    case tokentype_t::ampersand: return unoptype_t::deref;
    case tokentype_t::minus: return unoptype_t::negate_num;
    case tokentype_t::exclamation: return unoptype_t::negate_bool;
    default: UnreachableCrash(); return {};
  }
}
struct expr_unop_t { unoptype_t type;  expr_t* expr; };

//  binop =
//    "*" | "/" | "%"
//    "+" | "-"
//    "&" | "|" | "^"
//    "==" | "!=" | ">" | ">=" | "<" | "<="
Enumc( binoptype_t ) { mul, div, mod, add, sub, and_, or_, pow_, eqeq, noteq, gt, gteq, lt, lteq };
Inl slice_t
StringFromBinoptype( binoptype_t type )
{
  switch( type ) {
    case binoptype_t::mul  :  return SliceFromCStr( "mul" );
    case binoptype_t::div  :  return SliceFromCStr( "div" );
    case binoptype_t::mod  :  return SliceFromCStr( "mod" );
    case binoptype_t::add  :  return SliceFromCStr( "add" );
    case binoptype_t::sub  :  return SliceFromCStr( "sub" );
    case binoptype_t::and_ :  return SliceFromCStr( "and_" );
    case binoptype_t::or_  :  return SliceFromCStr( "or_" );
    case binoptype_t::pow_ :  return SliceFromCStr( "pow_" );
    case binoptype_t::eqeq :  return SliceFromCStr( "eqeq" );
    case binoptype_t::noteq:  return SliceFromCStr( "noteq" );
    case binoptype_t::gt   :  return SliceFromCStr( "gt" );
    case binoptype_t::gteq :  return SliceFromCStr( "gteq" );
    case binoptype_t::lt   :  return SliceFromCStr( "lt" );
    case binoptype_t::lteq :  return SliceFromCStr( "lteq" );
    default: UnreachableCrash();  return {};
  }
}
static tokentype_t c_binop_tokens[] = {
  tokentype_t::star,
  tokentype_t::slash,
  tokentype_t::percent,
  tokentype_t::plus,
  tokentype_t::minus,
  tokentype_t::ampersand,
  tokentype_t::pipe,
  tokentype_t::caret,
  tokentype_t::eqeq,
  tokentype_t::exclamationeq,
  tokentype_t::gt,
  tokentype_t::gteq,
  tokentype_t::lt,
  tokentype_t::lteq,
  };
Inl binoptype_t
BinopFromTokentype( tokentype_t type )
{
  switch( type ) {
    case tokentype_t::star: return binoptype_t::mul;
    case tokentype_t::slash: return binoptype_t::div;
    case tokentype_t::percent: return binoptype_t::mod;
    case tokentype_t::plus: return binoptype_t::add;
    case tokentype_t::minus: return binoptype_t::sub;
    case tokentype_t::ampersand: return binoptype_t::and_;
    case tokentype_t::pipe: return binoptype_t::or_;
    case tokentype_t::caret: return binoptype_t::pow_;
    case tokentype_t::eqeq: return binoptype_t::eqeq;
    case tokentype_t::exclamationeq: return binoptype_t::noteq;
    case tokentype_t::gt: return binoptype_t::gt;
    case tokentype_t::gteq: return binoptype_t::gteq;
    case tokentype_t::lt: return binoptype_t::lt;
    case tokentype_t::lteq: return binoptype_t::lteq;
    default: UnreachableCrash(); return {};
  }
}
Inl u32
PrecedenceValue( binoptype_t type )
{
  switch( type ) {
    case binoptype_t::or_: return 0;
    case binoptype_t::and_: return 1;
    case binoptype_t::eqeq:
    case binoptype_t::noteq: return 2;
    case binoptype_t::gt:
    case binoptype_t::gteq:
    case binoptype_t::lt:
    case binoptype_t::lteq: return 3;
    case binoptype_t::add:
    case binoptype_t::sub: return 4;
    case binoptype_t::mul:
    case binoptype_t::div:
    case binoptype_t::mod: return 5;
    case binoptype_t::pow_: return 6;
    default: UnreachableCrash();
  }
  return 0;
}
Inl binoptype_t
BinoptypeFromBinassigntype( binassignop_t type )
{
  switch( type ) {
    case binassignop_t::eq:      UnreachableCrash(); return {};
    case binassignop_t::arrow_l: UnreachableCrash(); return {};
    case binassignop_t::addeq: return binoptype_t::add;
    case binassignop_t::subeq: return binoptype_t::sub;
    case binassignop_t::muleq: return binoptype_t::mul;
    case binassignop_t::diveq: return binoptype_t::div;
    case binassignop_t::modeq: return binoptype_t::mod;
    case binassignop_t::andeq: return binoptype_t::and_;
    case binassignop_t::oreq:  return binoptype_t::or_;
    case binassignop_t::poweq: return binoptype_t::pow_;
    default: UnreachableCrash(); return {};
  }
}

struct expr_binop_t { binoptype_t type;  expr_t* expr_l;  expr_t* expr_r; };

//  expr_notbinop =
//    fncall
//    expr_assignable
//    unop  expr
//    unop_addrof  expr_assignable
//    "("  expr  ")"
//    num
//    str
//    chr
//
//  expr =
//    expr_notbinop  [ binop  expr ]
//
// note we merge expr and expr_notbinop in the datastructs, for simpler code.
// the grammar is a little clearer to define how we parse, but storage is simpler as one type.
//
Enumc( expr_type_t ) { fncall, expr_assignable, unop, unop_addrof, binop, num, str, chr };
struct expr_t {
  expr_type_t type;
  union {
    fncall_t fncall;
    expr_assignable_t expr_assignable;
    expr_unop_t unop;
    expr_binop_t binop;
    num_t num;
    str_t str;
    chr_t chr;
  };
};


Inl void
PrintExpr(
  stack_resizeable_cont_t<u8>* out,
  expr_t* expr
  );
Inl void
PrintScope(
  stack_resizeable_cont_t<u8>* out,
  scope_t* scope,
  idx_t indent
  );

Inl void
PrintIdent(
  stack_resizeable_cont_t<u8>* out,
  ident_t* ident
  )
{
  AddBackString( out, ident->text );
}
Inl void
PrintIndent(
  stack_resizeable_cont_t<u8>* out,
  idx_t indent
  )
{
  while( indent-- ) {
    AddBackString( out, " " );
  }
}
Inl void
PrintFncall(
  stack_resizeable_cont_t<u8>* out,
  fncall_t* fncall
  )
{
  PrintIdent( out, &fncall->ident );
  AddBackString( out, "(" );
  FORLIST( expr_arg, elem, fncall->expr_args )
    PrintExpr( out, *expr_arg );
    if( elem != fncall->expr_args.last ) {
      AddBackString( out, ", " );
    }
  }
  AddBackString( out, ")" );
}
Inl void
PrintExprAssignable(
  stack_resizeable_cont_t<u8>* out,
  expr_assignable_t* expr_assignable
  )
{
  FORLIST( entry, elem, expr_assignable->expr_assignable_entries )
    PrintIdent( out, &entry->ident );
    if( entry->expr_arrayidxs.len ) {
      AddBackString( out, "[" );
      FORLIST( expr_arrayidx, elem2, entry->expr_arrayidxs )
        PrintExpr( out, *expr_arrayidx );
        if( elem2 != entry->expr_arrayidxs.last ) {
          AddBackString( out, ", " );
        }
      }
      AddBackString( out, "]" );
    }
    if( elem != expr_assignable->expr_assignable_entries.last ) {
      AddBackString( out, "." );
    }
  }
}
Inl void
PrintExpr(
  stack_resizeable_cont_t<u8>* out,
  expr_t* expr
  )
{
  switch( expr->type ) {
    case expr_type_t::fncall: { PrintFncall( out, &expr->fncall ); } break;
    case expr_type_t::expr_assignable: { PrintExprAssignable( out, &expr->expr_assignable ); } break;
    case expr_type_t::unop: {
      auto unop = &expr->unop;
      switch( unop->type ) {
        case unoptype_t::deref: AddBackString( out, "deref " );  break;
        case unoptype_t::negate_num: AddBackString( out, "-" );  break;
        case unoptype_t::negate_bool: AddBackString( out, "!" );  break;
        default: UnreachableCrash();
      }
      PrintExpr( out, unop->expr );
    } break;
    case expr_type_t::unop_addrof: {
      AddBackString( out, "addrof " );
      PrintExprAssignable( out, &expr->expr_assignable );
    } break;
    case expr_type_t::binop: {
      auto binop = &expr->binop;
      PrintExpr( out, binop->expr_l );
      AddBackString( out, " " );
      switch( binop->type ) {
        case binoptype_t::mul: { AddBackString( out, "*" ); } break;
        case binoptype_t::div: { AddBackString( out, "/" ); } break;
        case binoptype_t::mod: { AddBackString( out, "%" ); } break;
        case binoptype_t::add: { AddBackString( out, "+" ); } break;
        case binoptype_t::sub: { AddBackString( out, "-" ); } break;
        case binoptype_t::and_: { AddBackString( out, "&" ); } break;
        case binoptype_t::or_: { AddBackString( out, "|" ); } break;
        case binoptype_t::pow_: { AddBackString( out, "^" ); } break;
        case binoptype_t::eqeq: { AddBackString( out, "==" ); } break;
        case binoptype_t::noteq: { AddBackString( out, "!=" ); } break;
        case binoptype_t::gt: { AddBackString( out, ">" ); } break;
        case binoptype_t::gteq: { AddBackString( out, ">=" ); } break;
        case binoptype_t::lt: { AddBackString( out, "<" ); } break;
        case binoptype_t::lteq: { AddBackString( out, "<=" ); } break;
        default: UnreachableCrash();
      }
      AddBackString( out, " " );
      PrintExpr( out, binop->expr_r );
    } break;
    case expr_type_t::num: { AddBackString( out, { ML( *expr->num.literal ) } ); } break;
    case expr_type_t::str: {
      AddBackString( out, "\"" );
      AddBackString( out, { ML( *expr->str.literal ) } );
      AddBackString( out, "\"" );
    } break;
    case expr_type_t::chr: {
      AddBackString( out, "'" );
      AddBackString( out, { ML( *expr->chr.literal ) } );
      AddBackString( out, "'" );
    } break;
    default: UnreachableCrash();
  }
}
Inl void
PrintTypedecl(
  stack_resizeable_cont_t<u8>* out,
  typedecl_t* typedecl
  )
{
  FORLIST( qualifier, elem, typedecl->qualifiers )
    switch( qualifier->type ) {
      case typedecl_qualifier_type_t::star: {
        AddBackString( out, "*" );
      } break;
      case typedecl_qualifier_type_t::array: {
        AddBackString( out, "[" );
        FORLIST( arrayidx, elem2, qualifier->arrayidxs )
          switch( arrayidx->type ) {
            case typedecl_arrayidx_type_t::star: {
              AddBackString( out, "*" );
            } break;
            case typedecl_arrayidx_type_t::expr_const: {
              PrintExpr( out, arrayidx->expr_const );
            } break;
            default: UnreachableCrash();
          }
          if( elem2 != qualifier->arrayidxs.last ) {
            AddBackString( out, ", " );
          }
        }
        AddBackString( out, "]" );
      } break;
      default: UnreachableCrash();
    }
  }
  if( typedecl->qualifiers.len ) {
    AddBackString( out, " " );
  }
  PrintIdent( out, &typedecl->ident );
}
Inl void
PrintDecl(
  stack_resizeable_cont_t<u8>* out,
  decl_t* decl
  )
{
  PrintIdent( out, &decl->ident );
  AddBackString( out, " " );
  PrintTypedecl( out, &decl->typedecl );
}
Inl void
PrintDeclassign(
  stack_resizeable_cont_t<u8>* out,
  declassign_t* declassign
  )
{
  PrintIdent( out, &declassign->ident );
  AddBackString( out, " " );
  switch( declassign->type ) {
    case declassign_type_t::explicit_: {
      PrintTypedecl( out, declassign->typedecl );
      AddBackString( out, " = " );
    } break;
    case declassign_type_t::implicit_: {
      AddBackString( out, ":= " );
    } break;
    default: UnreachableCrash();
  }
  PrintExpr( out, declassign->expr );
}
Inl void
PrintFndecl(
  stack_resizeable_cont_t<u8>* out,
  fndecl_t* fndecl
  )
{
  PrintIdent( out, &fndecl->ident );
  AddBackString( out, "(" );
  FORLIST( decl_arg, elem_decl_arg, fndecl->decl_args )
    PrintDecl( out, decl_arg );
    if( elem_decl_arg != fndecl->decl_args.last ) {
      AddBackString( out, ", " );
    }
  }
  AddBackString( out, ")" );
  if( fndecl->typedecl_rets.len ) {
    AddBackString( out, " " );
    FORLIST( typedecl_ret, elem_typedecl_ret, fndecl->typedecl_rets )
      PrintTypedecl( out, typedecl_ret );
      if( elem_typedecl_ret != fndecl->typedecl_rets.last ) {
        AddBackString( out, ", " );
      }
    }
  }
}
Inl void
PrintStatement(
  stack_resizeable_cont_t<u8>* out,
  statement_t* statement,
  idx_t indent
  )
{
  switch( statement->type ) {
    case statement_type_t::fncall: { PrintFncall( out, &statement->fncall ); } break;
    case statement_type_t::decl: { PrintDecl( out, &statement->decl ); } break;
    case statement_type_t::declassign: { PrintDeclassign( out, &statement->declassign ); } break;
    case statement_type_t::binassign: {
      auto binassign = &statement->binassign;
      if( binassign->expr_assignables.len > 1 ) {
        AddBackString( out, "(" );
      }
      FORLIST( expr_assignable, elem, binassign->expr_assignables )
        PrintExprAssignable( out, expr_assignable );
        if( elem != binassign->expr_assignables.last ) {
          AddBackString( out, ", " );
        }
      }
      if( binassign->expr_assignables.len > 1 ) {
        AddBackString( out, ")" );
      }
      AddBackString( out, " " );
      switch( binassign->type ) {
        case binassignop_t::eq: { AddBackString( out, "=" ); } break;
        case binassignop_t::arrow_l: { AddBackString( out, "<-" ); } break;
        case binassignop_t::addeq: { AddBackString( out, "+=" ); } break;
        case binassignop_t::subeq: { AddBackString( out, "-=" ); } break;
        case binassignop_t::muleq: { AddBackString( out, "*=" ); } break;
        case binassignop_t::diveq: { AddBackString( out, "/=" ); } break;
        case binassignop_t::modeq: { AddBackString( out, "%=" ); } break;
        case binassignop_t::andeq: { AddBackString( out, "&=" ); } break;
        case binassignop_t::oreq: { AddBackString( out, "|=" ); } break;
        case binassignop_t::poweq: { AddBackString( out, "^=" ); } break;
        default: UnreachableCrash();
      }
      AddBackString( out, " " );
      PrintExpr( out, binassign->expr );
    } break;
    case statement_type_t::ret: {
      auto ret = &statement->ret;
      AddBackString( out, "ret " );
      FORLIST( expr, elem, ret->exprs )
        PrintExpr( out, *expr );
        if( elem != ret->exprs.last ) {
          AddBackString( out, ", " );
        }
      }
    } break;
    case statement_type_t::defer: {
      AddBackString( out, "defer " );
      PrintStatement( out, statement->defer.statement, indent );
    } break;
    case statement_type_t::continue_: { AddBackString( out, "continue" ); } break;
    case statement_type_t::break_: { AddBackString( out, "break" ); } break;
    case statement_type_t::whileloop: {
      auto whileloop = &statement->whileloop;
      AddBackString( out, "while (" );
      PrintExpr( out, whileloop->cblock.expr );
      AddBackString( out, ") {\n" );
      PrintScope( out, whileloop->cblock.scope, indent + 2 );
      PrintIndent( out, indent );
      AddBackString( out, "}\n" );
    } break;
    case statement_type_t::ifchain: {
      auto ifchain = &statement->ifchain;
      AddBackString( out, "if (" );
      PrintExpr( out, ifchain->cblock_if.expr );
      AddBackString( out, ") {\n" );
      PrintScope( out, ifchain->cblock_if.scope, indent + 2 );
      PrintIndent( out, indent );
      AddBackString( out, "}\n" );
      FORLIST( cblock_elif, elem, ifchain->cblock_elifs )
        PrintIndent( out, indent );
        AddBackString( out, "elif (" );
        PrintExpr( out, cblock_elif->expr );
        AddBackString( out, ") {\n" );
        PrintScope( out, cblock_elif->scope, indent + 2 );
        PrintIndent( out, indent );
        AddBackString( out, "}\n" );
      }
      if( ifchain->scope_else.present ) {
        PrintIndent( out, indent );
        AddBackString( out, "else {\n" );
        PrintScope( out, ifchain->scope_else.value, indent + 2 );
        PrintIndent( out, indent );
        AddBackString( out, "}\n" );
      }
    } break;
    case statement_type_t::scope: {
      AddBackString( out, "{\n" );
      PrintScope( out, statement->scope, indent + 2 );
      PrintIndent( out, indent );
      AddBackString( out, "}\n" );
    } break;
    default: UnreachableCrash();
  }
}
Inl void
PrintScope(
  stack_resizeable_cont_t<u8>* out,
  scope_t* scope,
  idx_t indent
  )
{
  FORLIST( statement, elem, scope->statements )
    PrintIndent( out, indent );
    PrintStatement( out, statement, indent );
    switch( statement->type ) {
      case statement_type_t::fncall:
      case statement_type_t::decl:
      case statement_type_t::declassign:
      case statement_type_t::binassign:
      case statement_type_t::ret:
      case statement_type_t::continue_:
      case statement_type_t::break_: {
        AddBackString( out, ";" );
      } break;
      case statement_type_t::whileloop:
      case statement_type_t::ifchain:
      case statement_type_t::scope: {
      } break;
      case statement_type_t::defer: {
        if( statement->defer.statement->type != statement_type_t::scope ) {
          AddBackString( out, ";" );
        }
      } break;
      default: UnreachableCrash();
    }
    AddBackString( out, "\n" );
  }
}
Inl void
PrintGlobalScope(
  stack_resizeable_cont_t<u8>* out,
  global_scope_t* global_scope
  )
{
  idx_t indent = 0;
  FORLIST( global_statement, elem_global_statement, global_scope->global_statements )
    switch( global_statement->type ) {
      case global_statement_type_t::fndecl: {
        PrintFndecl( out, global_statement->fndecl );
        AddBackString( out, ";\n" );
      } break;

      case global_statement_type_t::fndefn: {
        auto fndefn = &global_statement->fndefn;
        PrintFndecl( out, fndefn->fndecl );
        AddBackString( out, "\n{\n" );
        PrintScope( out, fndefn->scope, indent + 2 );
        PrintIndent( out, indent );
        AddBackString( out, "}\n" );
      } break;

      case global_statement_type_t::structdecl: {
        auto structdecl = &global_statement->structdecl;
        PrintIdent( out, &structdecl->ident );
        AddBackString( out, " struct " );
        if( !structdecl->decl_fields.len ) {
          AddBackString( out, "{ /*empty*/ };\n" );
        }
        else {
          AddBackString( out, "{\n" );
          indent += 2;
          FORLIST( decl_field, elem, structdecl->decl_fields )
            PrintIndent( out, indent );
            PrintDecl( out, decl_field );
            AddBackString( out, ";\n" );
          }
          indent -= 2;
          PrintIndent( out, indent );
          AddBackString( out, "}\n" );
        }
      } break;

      case global_statement_type_t::enumdecl: {
        auto enumdecl = &global_statement->enumdecl;
        PrintIdent( out, &enumdecl->ident );
        AddBackString( out, " enum " );
        if( !enumdecl->enumdecl_entries.len ) {
          AddBackString( out, "{ /*empty*/ };\n" );
        }
        else {
          AddBackString( out, "{\n" );
          indent += 2;
          FORLIST( entry, elem, enumdecl->enumdecl_entries )
            switch( entry->type ) {
              case enumdecl_entry_type_t::ident: {
                PrintIndent( out, indent );
                PrintIdent( out, &entry->ident );
                AddBackString( out, ";\n" );
              } break;

              case enumdecl_entry_type_t::identassign: {
                PrintIndent( out, indent );
                PrintIdent( out, &entry->ident );
                AddBackString( out, " = " );
                PrintExpr( out, entry->expr );
                AddBackString( out, ";\n" );
              } break;

              default: UnreachableCrash();
            }
          }
          indent -= 2;
          PrintIndent( out, indent );
          AddBackString( out, "}\n" );
        }
      } break;

      case global_statement_type_t::declassign_const: {
        auto declassign = &global_statement->declassign_const;
        PrintDeclassign( out, declassign );
        AddBackString( out, ";\n" );
      } break;

      default: UnreachableCrash();
    }
  }
}



Templ Inl T*
_AddNode( compilecontext_t* ctx )
{
  auto r = AddPagelist( ctx->mem, T, _SIZEOF_IDX_T, 1 );
  // PERF: we can zero in bulk when allocating new pagelist pages.
  Typezero( r );
  return r;
}

#define ADD_NODE( _ctx, _type ) \
  _AddNode<_type>( _ctx )


Templ Inl listelem_t<T>*
AddListElem( compilecontext_t* ctx )
{
  auto listelem = AddPagelist( ctx->mem, listelem_t<T>, _SIZEOF_IDX_T, 1 );
  // PERF: we can zero in bulk when allocating new pagelist pages.
  Typezero( listelem );
  return listelem;
}
Templ Inl T*
AddBackList( compilecontext_t* ctx, list_t<T>* list )
{
  auto listelem = AddListElem<T>( ctx );
  InsertLast( *list, listelem );
  return &listelem->value;
}
Templ Inl T*
AddFrontList( compilecontext_t* ctx, list_t<T>* list )
{
  auto listelem = AddListElem<T>( ctx );
  InsertFirst( *list, listelem );
  return &listelem->value;
}
Templ Inl list_t<T>
CopyList( compilecontext_t* ctx, list_t<T>* src )
{
  list_t<T> r = {};
  FORLIST( value, elem, *src )
    *AddBackList( ctx, &r ) = *value;
  }
  return r;
}


#define IER \
  if( ctx->errors.len ) { return; }

#define IER0 \
  if( ctx->errors.len ) { return 0; }


Inl token_t*
PeekToken(
  compilecontext_t* ctx,
  idx_t pos
  )
{
  auto tokens = &ctx->tokens;
  if( pos >= tokens->len ) {
    AddBackString( &ctx->errors, "Expected more, but hit EOF first!\n" );
    return 0;
  }
  auto tkn = tokens->mem + pos;
  return tkn;
}

Inl token_t*
AdvanceOverToken(
  compilecontext_t* ctx,
  idx_t* pos
  )
{
  auto tkn = PeekToken( ctx, *pos );  IER0
  *pos += 1;
  return tkn;
}

Inl token_t*
PeekTokenOfType(
  compilecontext_t* ctx,
  idx_t pos,
  tokentype_t type
  )
{
  auto errors = &ctx->errors;
  auto tokens = &ctx->tokens;
  if( pos >= tokens->len ) {
    AddBackString( errors, "Expected '" );
    AddBackString( errors, StringSliceOfTokenType( type ) );
    AddBackString( errors, "', but hit EOF first.\n" );
    return 0;
  }
  auto tkn = tokens->mem + pos;
  if( tkn->type != type ) {
    OutputFileAndLine( ctx, tkn );
    AddBackString( errors, "Expected '" );
    AddBackString( errors, StringSliceOfTokenType( type ) );
    AddBackString( errors, "', but found '" );
    AddBackString( errors, StringSliceOfTokenType( tkn->type ) );
    AddBackString( errors, "' instead.\n" );
    OutputSrcLineAndCaret( ctx, tkn );
    AddBackString( errors, "\n" );
  }
  return tkn;
}

Inl token_t*
AdvanceOverTokenOfType(
  compilecontext_t* ctx,
  idx_t* pos,
  tokentype_t type
  )
{
  auto tkn = PeekTokenOfType( ctx, *pos, type );  IER0
  *pos += 1;
  return tkn;
}

Inl token_t*
PeekTokenOfType(
  compilecontext_t* ctx,
  idx_t pos,
  tokentype_t* types,
  idx_t types_len
  )
{
  auto errors = &ctx->errors;
  auto tokens = &ctx->tokens;
  if( pos >= tokens->len ) {
    AddBackString( errors, "Expected one of: ( " );
    For( i, 0, types_len ) {
      AddBackString( errors, "'" );
      AddBackString( errors, StringSliceOfTokenType( types[i] ) );
      AddBackString( errors, "'" );
      if( i + 1 < types_len ) {
        AddBackString( errors, ", " );
      }
    }
    AddBackString( errors, ", but hit EOF first.\n" );
  }
  auto tkn = tokens->mem + pos;
  bool matched = TContains( types, types_len, &tkn->type );
  if( !matched ) {
    OutputFileAndLine( ctx, tkn );
    AddBackString( errors, "Expected one of: ( " );
    For( i, 0, types_len ) {
      AddBackString( errors, "'" );
      AddBackString( errors, StringSliceOfTokenType( types[i] ) );
      AddBackString( errors, "'" );
      if( i + 1 < types_len ) {
        AddBackString( errors, ", " );
      }
    }
    AddBackString( errors, " ), but found '" );
    AddBackString( errors, StringSliceOfTokenType( tkn->type ) );
    AddBackString( errors, "' instead.\n" );
    OutputSrcLineAndCaret( ctx, tkn );
  }
  return tkn;
}

Inl token_t*
AdvanceOverTokenOfType(
  compilecontext_t* ctx,
  idx_t* pos,
  tokentype_t* types,
  idx_t types_len
  )
{
  auto tkn = PeekTokenOfType( ctx, *pos, types, types_len );  IER0
  *pos += 1;
  return tkn;
}

Inl void
SkipOverTokensOfType(
  compilecontext_t* ctx,
  idx_t* pos,
  tokentype_t type
  )
{
  auto tokens = &ctx->tokens;
  while( *pos < tokens->len  &&  tokens->mem[*pos].type == type ) {
    *pos += 1;
  }
}

Inl bool
IsSeparator( tokentype_t type )
{
  auto r =
    type == tokentype_t::comma  ||
    type == tokentype_t::semicolon;
  return r;
}

Inl void
MakeIdent(
  token_t* tkn,
  ident_t* ident
  )
{
  ident->literal = tkn;
  ident->text.mem = tkn->mem;
  ident->text.len = tkn->len;
}
Inl void
MakePseudoIdent(
  slice_t text,
  ident_t* ident
  )
{
  ident->literal = 0;
  ident->text = text;
}
Inl void
ParseIdent(
  compilecontext_t* ctx,
  idx_t* pos,
  ident_t* ident
  )
{
  auto tkn = AdvanceOverTokenOfType( ctx, pos, tokentype_t::ident );
  MakeIdent( tkn, ident );
}

Inl void
ParseExpr(
  compilecontext_t* ctx,
  idx_t* pos,
  expr_t* expr
  );

// assumes pos is after '(', and at the first arg or ')'.
Inl void
ParseFncall(
  compilecontext_t* ctx,
  idx_t* pos,
  token_t* tkn_ident,
  fncall_t* fncall
  )
{
  MakeIdent( tkn_ident, &fncall->ident );
  Forever {
    auto tkn = PeekToken( ctx, *pos );  IER
    if( tkn->type == tokentype_t::paren_r ) {
      *pos += 1;
      break;
    }
    if( IsSeparator( tkn->type ) ) {
      *pos += 1;
      continue;
    }
    auto expr_arg = AddBackList( ctx, &fncall->expr_args );
    *expr_arg = ADD_NODE( ctx, expr_t );
    ParseExpr( ctx, pos, *expr_arg );  IER
  }
}

Inl void
ParseExprAssignable(
  compilecontext_t* ctx,
  idx_t* pos,
  token_t* tkn_ident,
  expr_assignable_t* expr_assignable
  )
{
  Forever {
    auto entry = AddBackList( ctx, &expr_assignable->expr_assignable_entries );
    MakeIdent( tkn_ident, &entry->ident );
    auto tkn = PeekToken( ctx, *pos );  IER
    if( tkn->type == tokentype_t::bracket_square_l ) {
      *pos += 1;
      Forever {
        tkn = PeekToken( ctx, *pos );  IER
        if( tkn->type == tokentype_t::bracket_square_r ) {
          *pos += 1;
          break;
        }
        if( IsSeparator( tkn->type ) ) {
          *pos += 1;
          continue;
        }
        auto expr_arrayidx = AddBackList( ctx, &entry->expr_arrayidxs );
        *expr_arrayidx = ADD_NODE( ctx, expr_t );
        ParseExpr( ctx, pos, *expr_arrayidx );  IER
      }
      tkn = PeekToken( ctx, *pos );  IER
    }
    if( tkn->type == tokentype_t::dot ) {
      *pos += 1;
      tkn_ident = AdvanceOverTokenOfType( ctx, pos, tokentype_t::ident );  IER
      continue;
    }
    else {
      break;
    }
  }
}

Inl void
ParseExprNotBinop(
  compilecontext_t* ctx,
  idx_t* pos,
  expr_t* expr
  )
{
  static tokentype_t types0[] = {
    tokentype_t::number,
    tokentype_t::string_,
    tokentype_t::char_,
    tokentype_t::paren_l,
    tokentype_t::plus,
    tokentype_t::addrof,
    tokentype_t::deref,
    tokentype_t::minus,
    tokentype_t::exclamation,
    tokentype_t::ident,
    };
  auto tkn = AdvanceOverTokenOfType( ctx, pos, AL( types0 ) );  IER
  switch( tkn->type ) {
    case tokentype_t::number: {
      expr->type = expr_type_t::num;
      expr->num.literal = tkn;
      AssertCrash( tkn->len );
      // TODO: custom number parsing.
      if( StringScanR( ML( *tkn ), '.' ) ) {
        expr->num.type = num_type_t::float_;
        auto value = CsTo_f64( ML( *tkn ) );
        expr->num.value_f64 = value;
        if( value > MAX_f32  ||  value < MIN_f32 ) {
          expr->num.bytecount = 8;
        }
        else {
          expr->num.value_f32 = Cast( f32, value );
          expr->num.bytecount = 4;
        }
      }
      elif( tkn->mem[0] == '-' ) {
        expr->num.type = num_type_t::signed_;
        auto value = CsToIntegerS<s64>( ML( *tkn ) );
        expr->num.value_s64 = value;
        if( value > MAX_s32  ||  value < MIN_s32 ) {
          expr->num.bytecount = 8;
        }
        elif( value > MAX_s16  ||  value < MIN_s16 ) {
          expr->num.bytecount = 4;
        }
        elif( value > MAX_s8  ||  value < MIN_s8 ) {
          expr->num.bytecount = 2;
        }
        else {
          expr->num.bytecount = 1;
        }
      }
      else {
        expr->num.type = num_type_t::unsigned_;
        auto value = CsToIntegerU<u64>( ML( *tkn ) );
        expr->num.value_u64 = value;
        if( value > MAX_u32 ) {
          expr->num.bytecount = 8;
        }
        elif( value > MAX_u16 ) {
          expr->num.bytecount = 4;
        }
        elif( value > MAX_u8 ) {
          expr->num.bytecount = 2;
        }
        else {
          expr->num.bytecount = 1;
        }
      }
    } break;

    case tokentype_t::string_: {
      expr->type = expr_type_t::str;
      expr->str.literal = tkn;
    } break;

    case tokentype_t::char_: {
      expr->type = expr_type_t::chr;
      expr->chr.literal = tkn;
    } break;

    case tokentype_t::paren_l: {
      ParseExpr( ctx, pos, expr );  IER
      AdvanceOverTokenOfType( ctx, pos, tokentype_t::paren_r );  IER
    } break;

    // ignore unop plus in all cases, since it has no effect
    case tokentype_t::plus: {
      ParseExpr( ctx, pos, expr );  IER
    } break;

    // unop addrof expects an expr_assignable, while other unops expect a full expr.
    case tokentype_t::addrof: {
      expr->type = expr_type_t::unop_addrof;
      auto tkn_ident = AdvanceOverTokenOfType( ctx, pos, tokentype_t::ident );  IER
      ParseExprAssignable( ctx, pos, tkn_ident, &expr->expr_assignable );  IER
    } break;

    case tokentype_t::deref:
    case tokentype_t::minus:
    case tokentype_t::exclamation: {
      expr->type = expr_type_t::unop;
      expr->unop.type = UnopFromTokentype( tkn->type );
      expr->unop.expr = ADD_NODE( ctx, expr_t );
      ParseExpr( ctx, pos, expr->unop.expr );  IER
    } break;

    case tokentype_t::ident: {
      auto tkn_ident = tkn;
      tkn = PeekToken( ctx, *pos );  IER
      if( tkn->type == tokentype_t::paren_l ) {
        *pos += 1;
        expr->type = expr_type_t::fncall;
        ParseFncall( ctx, pos, tkn_ident, &expr->fncall );  IER
      }
      else {
        expr->type = expr_type_t::expr_assignable;
        ParseExprAssignable( ctx, pos, tkn_ident, &expr->expr_assignable );  IER
      }
    } break;

    default: UnreachableCrash();
  }
}

Inl void
ParseExpr(
  compilecontext_t* ctx,
  idx_t* pos,
  expr_t* expr
  )
{
  // we parse directly into expr, since binops probably aren't the most common kind of exprs.
  ParseExprNotBinop( ctx, pos, expr );  IER
  // if we do have a binop, then we have to set expr appropriately while keeping our expr_l.
  auto tkn = PeekToken( ctx, *pos );  IER
  if( TContains( AL( c_binop_tokens ), &tkn->type ) ) {
    *pos += 1;
    auto expr_l = ADD_NODE( ctx, expr_t );
    *expr_l = *expr;
    expr->type = expr_type_t::binop;
    expr->binop.expr_l = expr_l;
    expr->binop.type = BinopFromTokentype( tkn->type );
    expr->binop.expr_r = ADD_NODE( ctx, expr_t );
    // TODO: precedence rearranging!
    ParseExpr( ctx, pos, expr->binop.expr_r );  IER
  }
}

Inl void
ParseTypedecl(
  compilecontext_t* ctx,
  idx_t* pos,
  typedecl_t* typedecl
  )
{
  Forever {
    static tokentype_t types0[] = { tokentype_t::ident, tokentype_t::star, tokentype_t::bracket_square_l };
    auto tkn = AdvanceOverTokenOfType( ctx, pos, AL( types0 ) );  IER
    if( tkn->type == tokentype_t::ident ) {
      MakeIdent( tkn, &typedecl->ident );
      break;
    }
    auto qualifier = AddBackList( ctx, &typedecl->qualifiers );
    if( tkn->type == tokentype_t::star ) {
      qualifier->type = typedecl_qualifier_type_t::star;
    }
    else /*tkn->type == tokentype_t::bracket_square_l*/ {
      qualifier->type = typedecl_qualifier_type_t::array;
      Forever {
        tkn = PeekToken( ctx, *pos );  IER
        if( tkn->type == tokentype_t::bracket_square_r ) {
          *pos += 1;
          break;
        }
        elif( IsSeparator( tkn->type ) ) {
          *pos += 1;
          continue;
        }
        elif( tkn->type == tokentype_t::star ) {
          auto arrayidx = AddBackList( ctx, &qualifier->arrayidxs );
          arrayidx->type = typedecl_arrayidx_type_t::star;
        }
        else {
          auto arrayidx = AddBackList( ctx, &qualifier->arrayidxs );
          arrayidx->type = typedecl_arrayidx_type_t::expr_const;
          arrayidx->expr_const = ADD_NODE( ctx, expr_t );
          ParseExpr( ctx, pos, arrayidx->expr_const );  IER
        }
      }
    }
  }
}

Inl void
ParseScope(
  compilecontext_t* ctx,
  idx_t* pos,
  scope_t* scope
  );

Inl void
ParseCondScope(
  compilecontext_t* ctx,
  idx_t* pos,
  cond_scope_t* cblock
  )
{
  // TODO: optional parens, optional braces.  the verbosity isn't worth it for the rare indentation bugs
  AdvanceOverTokenOfType( ctx, pos, tokentype_t::paren_l );  IER
  cblock->expr = ADD_NODE( ctx, expr_t );
  ParseExpr( ctx, pos, cblock->expr );  IER
  AdvanceOverTokenOfType( ctx, pos, tokentype_t::paren_r );  IER
  AdvanceOverTokenOfType( ctx, pos, tokentype_t::bracket_curly_l );  IER
  cblock->scope = ADD_NODE( ctx, scope_t );
  ParseScope( ctx, pos, cblock->scope );  IER
}

// assumes pos is past the leading ident, which is passed in
Inl void
ParseDeclassign(
  compilecontext_t* ctx,
  idx_t* pos,
  token_t* tkn_ident,
  declassign_t* declassign
  )
{
  MakeIdent( tkn_ident, &declassign->ident );
  auto tkn = PeekToken( ctx, *pos );  IER
  if( tkn->type == tokentype_t::colon ) {
    *pos += 1;
    declassign->type = declassign_type_t::implicit_;
  }
  else {
    declassign->type = declassign_type_t::explicit_;
    declassign->typedecl = ADD_NODE( ctx, typedecl_t );
    ParseTypedecl( ctx, pos, declassign->typedecl );  IER
  }
  AdvanceOverTokenOfType( ctx, pos, tokentype_t::eq );  IER
  declassign->expr = ADD_NODE( ctx, expr_t );
  ParseExpr( ctx, pos, declassign->expr );  IER
}
// assumes pos is past the ident, and at the ':'
Inl void
ParseDeclassignImplicit(
  compilecontext_t* ctx,
  idx_t* pos,
  token_t* tkn_ident,
  declassign_t* declassign
  )
{
  MakeIdent( tkn_ident, &declassign->ident );
  *pos += 1;
  declassign->type = declassign_type_t::implicit_;
  AdvanceOverTokenOfType( ctx, pos, tokentype_t::eq );  IER
  declassign->expr = ADD_NODE( ctx, expr_t );
  ParseExpr( ctx, pos, declassign->expr );  IER
}
// assumes pos is past the 'eq', and at the expr.
Inl void
ParseDeclassignExplicit(
  compilecontext_t* ctx,
  idx_t* pos,
  token_t* tkn_ident,
  typedecl_t* typedecl,
  declassign_t* declassign
  )
{
  MakeIdent( tkn_ident, &declassign->ident );
  declassign->type = declassign_type_t::explicit_;
  declassign->typedecl = typedecl;
  declassign->expr = ADD_NODE( ctx, expr_t );
  ParseExpr( ctx, pos, declassign->expr );  IER
}

Inl void
ParseStatement(
  compilecontext_t* ctx,
  idx_t* pos,
  statement_t* statement
  )
{
  auto errors = &ctx->errors;
  static tokentype_t types0[] = {
    tokentype_t::ident,
    tokentype_t::ret,
    tokentype_t::while_,
    tokentype_t::if_,
    tokentype_t::defer,
    tokentype_t::continue_,
    tokentype_t::break_,
    tokentype_t::bracket_curly_l,
    tokentype_t::paren_l,
    };
  auto tkn = AdvanceOverTokenOfType( ctx, pos, AL( types0 ) );  IER
  switch( tkn->type ) {
    case tokentype_t::ret: {
      // TODO: allow parens around retval list
      statement->type = statement_type_t::ret;
      Forever {
        tkn = PeekToken( ctx, *pos );  IER
        if( tkn->type == tokentype_t::semicolon ) {
          break;
        }
        if( tkn->type == tokentype_t::comma ) {
          *pos += 1;
          continue;
        }
        auto expr_retval = AddBackList( ctx, &statement->ret.exprs );
        *expr_retval = ADD_NODE( ctx, expr_t );
        ParseExpr( ctx, pos, *expr_retval );  IER
      }
    } break;

    case tokentype_t::while_: {
      statement->type = statement_type_t::whileloop;
      ParseCondScope( ctx, pos, &statement->whileloop.cblock );  IER
    } break;

    case tokentype_t::if_: {
      statement->type = statement_type_t::ifchain;
      ParseCondScope( ctx, pos, &statement->ifchain.cblock_if );  IER
      Forever {
        tkn = PeekToken( ctx, *pos );  IER
        if( tkn->type == tokentype_t::elif_ ) {
          *pos += 1;
          auto cblock_elif = AddBackList( ctx, &statement->ifchain.cblock_elifs );
          ParseCondScope( ctx, pos, cblock_elif );  IER
          continue;
        }
        if( tkn->type == tokentype_t::else_ ) {
          *pos += 1;
          AdvanceOverTokenOfType( ctx, pos, tokentype_t::bracket_curly_l );  IER
          statement->ifchain.scope_else.present = 1;
          statement->ifchain.scope_else.value = ADD_NODE( ctx, scope_t );
          ParseScope( ctx, pos, statement->ifchain.scope_else.value );  IER
        }
        break;
      }
    } break;

    case tokentype_t::defer: {
      statement->type = statement_type_t::defer;
      statement->defer.statement = ADD_NODE( ctx, statement_t );
      ParseStatement( ctx, pos, statement->defer.statement );  IER
    } break;

    case tokentype_t::continue_: {
      statement->type = statement_type_t::continue_;
    } break;

    case tokentype_t::break_: {
      statement->type = statement_type_t::break_;
    } break;

    case tokentype_t::bracket_curly_l: {
      statement->type = statement_type_t::scope;
      statement->scope = ADD_NODE( ctx, scope_t );
      ParseScope( ctx, pos, statement->scope );  IER
    } break;

    case tokentype_t::paren_l: {
      statement->type = statement_type_t::binassign;
      Forever {
        tkn = PeekToken( ctx, *pos );  IER
        if( tkn->type == tokentype_t::paren_r ) {
          *pos += 1;
          break;
        }
        if( IsSeparator( tkn->type ) ) {
          *pos += 1;
          continue;
        }
        auto tkn_ident = AdvanceOverTokenOfType( ctx, pos, tokentype_t::ident );  IER
        auto expr_assignable = AddBackList( ctx, &statement->binassign.expr_assignables );
        ParseExprAssignable( ctx, pos, tkn_ident, expr_assignable );  IER
      }
      if( !statement->binassign.expr_assignables.len ) {
        ErrorCStr( ctx, tkn, "binary assignments require one or more assignable expressions!" );
        return;
      }
      tkn = AdvanceOverTokenOfType( ctx, pos, AL( c_binassignop_tokens ) );  IER
      statement->binassign.type = BinassignopFromTokentype( tkn->type );
      statement->binassign.expr = ADD_NODE( ctx, expr_t );
      ParseExpr( ctx, pos, statement->binassign.expr );  IER
    } break;

    case tokentype_t::ident: {
      auto tkn_ident = tkn;
      tkn = PeekToken( ctx, *pos );  IER
      if( tkn->type == tokentype_t::paren_l ) {
        *pos += 1;
        statement->type = statement_type_t::fncall;
        ParseFncall( ctx, pos, tkn_ident, &statement->fncall );  IER
      }
      elif( tkn->type == tokentype_t::colon ) {
        statement->type = statement_type_t::declassign;
        ParseDeclassignImplicit( ctx, pos, tkn_ident, &statement->declassign );  IER
      }
      else {
        // since we have similar syntax for array typedecls and expr_assignable arrayidx,
        // it's easiest to just try parsing the longer one and see if we have errors.
        // if we do, then reset the errors and try the other.
        // this could make for confusing error messaging if the user intended the first,
        // so maybe we should revisit this.
        auto saved_pos = *pos;
        auto saved_errors_len = errors->len;
        auto typedecl = ADD_NODE( ctx, typedecl_t );
        ParseTypedecl( ctx, pos, typedecl ); // intentionally no IER
        if( errors->len > 0 ) {
          *pos = saved_pos;
          errors->len = saved_errors_len;

          statement->type = statement_type_t::binassign;
          auto expr_assignable = AddBackList( ctx, &statement->binassign.expr_assignables );
          ParseExprAssignable( ctx, pos, tkn_ident, expr_assignable );  IER
          tkn = AdvanceOverTokenOfType( ctx, pos, AL( c_binassignop_tokens ) );  IER
          statement->binassign.type = BinassignopFromTokentype( tkn->type );
          statement->binassign.expr = ADD_NODE( ctx, expr_t );
          ParseExpr( ctx, pos, statement->binassign.expr );  IER
        }
        else {
          tkn = PeekToken( ctx, *pos );  IER
          if( tkn->type == tokentype_t::eq ) {
            *pos += 1;
            statement->type = statement_type_t::declassign;
            ParseDeclassignExplicit( ctx, pos, tkn_ident, typedecl, &statement->declassign );  IER
          }
          else {
            statement->type = statement_type_t::decl;
            MakeIdent( tkn_ident, &statement->decl.ident );
            statement->decl.typedecl = *typedecl;
          }
        }
      }
    } break;

    default: UnreachableCrash();
  }
}

// assumes pos is past the leading '{'
Inl void
ParseScope(
  compilecontext_t* ctx,
  idx_t* pos,
  scope_t* scope
  )
{
  auto tkns = SliceFromArray( ctx->tokens );
  SkipOverTokensOfType( ctx, pos, tokentype_t::semicolon );
  // note we allow pos to hit the EOF here, since you can end a file with a scope-close.
  while( *pos < tkns.len ) {
    auto tkn = tkns.mem + *pos;
    if( tkn->type == tokentype_t::bracket_curly_r ) {
      *pos += 1;
      break;
    }
    auto statement = AddBackList( ctx, &scope->statements );
    ParseStatement( ctx, pos, statement );  IER
    SkipOverTokensOfType( ctx, pos, tokentype_t::semicolon );
  }
}

Inl void
ParseGlobalStatement(
  compilecontext_t* ctx,
  idx_t* pos,
  global_statement_t* global_statement
  )
{
  auto tkn_ident = AdvanceOverTokenOfType( ctx, pos, tokentype_t::ident );  IER
  auto tkn = PeekToken( ctx, *pos );  IER
  switch( tkn->type ) {
    case tokentype_t::enum_: {
      *pos += 1; // skip 'enum'
      global_statement->type = global_statement_type_t::enumdecl;
      MakeIdent( tkn_ident, &global_statement->enumdecl.ident );
      AdvanceOverTokenOfType( ctx, pos, tokentype_t::bracket_curly_l );  IER
      Forever {
        static tokentype_t types1[] = { tokentype_t::ident, tokentype_t::bracket_curly_r };
        tkn = AdvanceOverTokenOfType( ctx, pos, AL( types1 ) );  IER
        if( tkn->type == tokentype_t::bracket_curly_r ) {
          break;
        }
        auto enumdecl_entry = AddBackList( ctx, &global_statement->enumdecl.enumdecl_entries );
        MakeIdent( tkn, &enumdecl_entry->ident );
        static tokentype_t types0[] = { tokentype_t::comma, tokentype_t::semicolon, tokentype_t::eq, tokentype_t::bracket_curly_r };
        tkn = AdvanceOverTokenOfType( ctx, pos, AL( types0 ) );  IER
        if( tkn->type == tokentype_t::eq ) {
          enumdecl_entry->type = enumdecl_entry_type_t::identassign;
          enumdecl_entry->expr = ADD_NODE( ctx, expr_t );
          ParseExpr( ctx, pos, enumdecl_entry->expr );  IER
          static tokentype_t types2[] = { tokentype_t::comma, tokentype_t::semicolon, tokentype_t::bracket_curly_r };
          tkn = AdvanceOverTokenOfType( ctx, pos, AL( types2 ) );  IER
          if( tkn->type == tokentype_t::bracket_curly_r ) {
            break;
          }
        }
        elif( tkn->type == tokentype_t::bracket_curly_r ) {
          break;
        }
        else /*IsSeparator( tkn->type )*/ {
          enumdecl_entry->type = enumdecl_entry_type_t::ident;
        }
      }
    } break;

    case tokentype_t::struct_: {
      *pos += 1; // skip 'struct'
      global_statement->type = global_statement_type_t::structdecl;
      MakeIdent( tkn_ident, &global_statement->structdecl.ident );
      AdvanceOverTokenOfType( ctx, pos, tokentype_t::bracket_curly_l );  IER
      Forever {
        static tokentype_t types1[] = { tokentype_t::ident, tokentype_t::bracket_curly_r };
        tkn = AdvanceOverTokenOfType( ctx, pos, AL( types1 ) );  IER
        if( tkn->type == tokentype_t::bracket_curly_r ) {
          break;
        }
        auto decl_field = AddBackList( ctx, &global_statement->structdecl.decl_fields );
        MakeIdent( tkn, &decl_field->ident );  IER
        ParseTypedecl( ctx, pos, &decl_field->typedecl );  IER
        static tokentype_t types0[] = { tokentype_t::comma, tokentype_t::semicolon, tokentype_t::bracket_curly_r };
        tkn = AdvanceOverTokenOfType( ctx, pos, AL( types0 ) );  IER
        if( tkn->type == tokentype_t::bracket_curly_r ) {
          break;
        }
      }
    } break;

    case tokentype_t::paren_l: {
      // first loop is largely duped from structdecl, because it's likely we'll change syntax for one or the other.
      *pos += 1;
      auto fndecl = ADD_NODE( ctx, fndecl_t );
      MakeIdent( tkn_ident, &fndecl->ident );
      Forever {
        static tokentype_t types1[] = { tokentype_t::ident, tokentype_t::paren_r };
        tkn = AdvanceOverTokenOfType( ctx, pos, AL( types1 ) );  IER
        if( tkn->type == tokentype_t::paren_r ) {
          break;
        }
        auto decl_arg = AddBackList( ctx, &fndecl->decl_args );
        MakeIdent( tkn, &decl_arg->ident );  IER
        ParseTypedecl( ctx, pos, &decl_arg->typedecl );  IER
        static tokentype_t types0[] = { tokentype_t::comma, tokentype_t::semicolon, tokentype_t::paren_r };
        tkn = AdvanceOverTokenOfType( ctx, pos, AL( types0 ) );  IER
        if( tkn->type == tokentype_t::paren_r ) {
          break;
        }
      }
      tkn = PeekToken( ctx, *pos );  IER
      if( tkn->type == tokentype_t::semicolon ) {
        *pos += 1;
        global_statement->type = global_statement_type_t::fndecl;
        global_statement->fndecl = fndecl;
      }
      else {
        if( tkn->type != tokentype_t::bracket_curly_l ) {
          Forever {
            auto typedecl_ret = AddBackList( ctx, &fndecl->typedecl_rets );
            ParseTypedecl( ctx, pos, typedecl_ret );  IER
            static tokentype_t types0[] = { tokentype_t::comma, tokentype_t::semicolon, tokentype_t::bracket_curly_l };
            tkn = AdvanceOverTokenOfType( ctx, pos, AL( types0 ) );  IER
            if( tkn->type == tokentype_t::bracket_curly_l ) {
              break;
            }
            if( tkn->type == tokentype_t::semicolon ) {
              global_statement->type = global_statement_type_t::fndecl;
              global_statement->fndecl = fndecl;
              return;
            }
          }
        }
        else {
          *pos += 1;
        }
        global_statement->type = global_statement_type_t::fndefn;
        global_statement->fndefn.fndecl = fndecl;
        global_statement->fndefn.scope = ADD_NODE( ctx, scope_t );
        ParseScope( ctx, pos, global_statement->fndefn.scope );  IER
      }
    } break;

    default: {
      global_statement->type = global_statement_type_t::declassign_const;
      ParseDeclassign( ctx, pos, tkn_ident, &global_statement->declassign_const );  IER
    } break;
  }
}

NoInl void
ParseGlobalScope(
  compilecontext_t* ctx,
  global_scope_t* global_scope
  )
{
  auto tokens = &ctx->tokens;
  idx_t pos = 0;
  SkipOverTokensOfType( ctx, &pos, tokentype_t::semicolon );
  while( pos < tokens->len ) {
    auto global_statement = AddBackList( ctx, &global_scope->global_statements );
    ParseGlobalStatement( ctx, &pos, global_statement );  IER
    SkipOverTokensOfType( ctx, &pos, tokentype_t::semicolon );
  }
}




struct
structdecltype_t;
struct
enumdecltype_t;

// basically the same as typedecl_t, but with expr_const evaluated to a u32 value.
// TODO: more detail around slices, which may not have a value.
struct arrayidx_t { typedecl_arrayidx_type_t type;  u32 value; };
struct qualifier_t { typedecl_qualifier_type_t type;  list_t<arrayidx_t> arrayidxs; };
struct type_num_t { num_type_t type;  u32 bytecount; };

Enumc( type_type_t ) { num, bool_, string_, enum_, struct_ };
struct
type_t
{
  list_t<qualifier_t> qualifiers;
  type_type_t type;
  union {
    type_num_t num;
    structdecltype_t* structdecltype;
    enumdecltype_t* enumdecltype;
  };
};

using
typelist_t = list_t<type_t>;
struct rtype_t { type_t type;  typelist_t autocasts; };

struct namedtype_t { ident_t ident;  type_t type; };

// only non-generic-arg/ret functions will have a fndecltype_t stored in the globaltypes_t
// TODO: consider making rets a list of namedtype_t, so we can have named rets.
struct fndecltype_t { ident_t ident;  tslice_t<namedtype_t> args;  tslice_t<type_t> rets; };
struct structdecltype_t { ident_t ident;  tslice_t<namedtype_t> fields; };
struct enumdecltype_value_t { ident_t ident;  u32 value; };
struct enumdecltype_t { ident_t ident;  type_t type;  tslice_t<enumdecltype_value_t> values; };

// TODO: rename this to stackvarloc_t
struct
bc_var_t
{
  // memory for this var is located at { stack_pointer + offset_into_scope, bytecount }
  // assumed to be aligned as needed already.
  s32 offset_into_scope;
  u32 bytecount;
};

struct
var_t
{
  type_t type;
  bc_var_t stackvarloc;
  u32 idx; // TODO: XXXXXXXXXXXXXX just for printing purposes, probably remove at some point.
};

struct namedvar_t { ident_t ident;  var_t* var; };
// TODO: i think we have tc bugs where we don't change namedvar_t when 'modifying' the var.
// perhaps update namedvar_t.var in binassign eq?
// well, binassign eq doesn't change the labelling of the stackvar, so we don't really want that.

struct
globaltypes_t
{
  list_t<fndecltype_t> fndecltypes;
  list_t<structdecltype_t> structdecltypes;
  list_t<enumdecltype_t> enumdecltypes;
  list_t<namedvar_t> constants;
  list_t<function_t> functions;

  type_t type_bool;
  type_t type_string; // TODO: delete this, in favor of '[N] u8' when we know N, and '[] u8' otherwise.
  type_t type_enum; // TODO: probably structure this differently when we allow arbitrary enum types.
  type_t type_u8;
  type_t type_u16;
  type_t type_u32;
  type_t type_u64;
  type_t type_s8;
  type_t type_s16;
  type_t type_s32;
  type_t type_s64;
  type_t type_f32;
  type_t type_f64;

  rtype_t rtype_bool;
};

struct
tc_t;

// TODO: make this fndecltype_t a function_t?
// need to do a pre-pass for function_t if so, because we make these as we process function_t's
struct tc_fncall_t { fndecltype_t* fndecltype;  tslice_t<var_t*> var_args;  tslice_t<var_t*> var_rets; };
struct tc_move_t { var_t* var_l;  var_t* var_r; };
struct tc_loadconstant_t { var_t* var_l;  void* mem; };
struct tc_store_t { var_t* var_l;  var_t* var_r; };  // deref( typeof(var_l) ) == typeof(var_r)
struct tc_jumplabel_t {};
struct tc_jump_t { tc_t* jumplabel; };
struct tc_jumpcond_t { var_t* var_cond;  tc_jump_t jump; };
struct tc_binop_t { binoptype_t type;  var_t* var_l;  var_t* var_r;  var_t* var_result; };
struct tc_unop_t { unoptype_t type;  var_t* var;  var_t* var_result; };
struct tc_autocast_t { var_t* var;  var_t* var_result; };
struct tc_arrayidx_t { var_t* var_array;  tslice_t<var_t*> var_arrayidxs;  var_t* var_result; };
struct tc_fieldaccess_t { var_t* var_struct;  idx_t field_offset;  ident_t field_name;  var_t* var_result; };
struct tc_addrof_t { var_t* var;  var_t* var_result; };
struct tc_deref_t { var_t* var;  var_t* var_result; };

// array_elem_from_array    elem: T <-  array: []T indexed by [ idx:u32 ]
// array_elem_from_parray   elem: T <- parray:*[]T indexed by [ idx:u32 ]
// array_pelem_from_array  pelem:*T <-  array: []T indexed by [ idx:u32 ]
// array_pelem_from_parray pelem:*T <- parray:*[]T indexed by [ idx:u32 ]
//
// struct_field_from_struct    elem: T <- fieldoffset K into  struct: S
// struct_field_from_pstruct   elem: T <- fieldoffset K into pstruct:*S
// struct_pfield_from_struct  pelem:*T <- fieldoffset K into  struct: S
// struct_pfield_from_pstruct pelem:*T <- fieldoffset K into pstruct:*S

Enumc( tc_type_t )
{
  fncall,
  ret,
  move,
  loadconstant,
  store,
  jumplabel,
  jump,
  jumpnotzero,
  jumpzero,
  binop,
  unop,
  autocast,
  array_elem_from_array,
  array_elem_from_parray,
  array_pelem_from_array,
  array_pelem_from_parray,
  struct_field_from_struct,
  struct_field_from_pstruct,
  struct_pfield_from_struct,
  struct_pfield_from_pstruct,
  addrof,
  deref,
};
struct
tc_t
{
  tc_type_t type;
  union {
    tc_fncall_t fncall;
    tc_move_t move;
    tc_loadconstant_t loadconstant;
    tc_store_t store;
    tc_jumplabel_t jumplabel;
    tc_jump_t jump;
    tc_jumpcond_t jumpcond;
    tc_binop_t binop;
    tc_unop_t unop;
    tc_autocast_t autocast;
    tc_arrayidx_t arrayidx;
    tc_fieldaccess_t fieldaccess;
    tc_addrof_t addrof;
    tc_deref_t deref;
  };
};

struct
vartable_t
{
  // note vartables form a tree, since you can have arbitrary nested scopes.
  list_t<vartable_t> vartables;
  list_t<namedvar_t> namedvars;
#ifdef _DEBUG
  scope_t* scope; // TODO: probably helpful for debugging, but not really necessary.
#endif
};

struct typedeclalias_t { typedecl_t find;  typedecl_t replace; };

// one of these is made by typing for every:
//   non-generic-arg/ret function
//   unique instance of a generic-arg/ret function, called by a fncall inside a non-generic-arg/ret functions.
struct
function_t
{
  fndefn_t* fndefn;
  fndecltype_t* fndecltype;
  list_t<typedeclalias_t> typedecl_aliases; // for typing-generated function_t which are generic-arg/ret instances.
  tslice_t<namedvar_t> namedvar_args;
  vartable_t vartable;
  list_t<var_t> tvars;
  list_t<tc_t> tcode;
  list_t<tc_t> tcode_defers;
  u32 bytecount_locals;
  u32 bytecount_rets;
  u32 bytecount_args;
  u32 bytecount_frame;
  tslice_t<bc_var_t> bvar_args;
  tslice_t<var_t*> var_rets_by_ptr;
  idx_t bc_start;
  bool generated; // cycle detection.
};



Inl void
PrintU64Hex(
  stack_resizeable_cont_t<u8>* out,
  u64 value
  )
{
  stack_nonresizeable_stack_t<u8, 64> tmp;
  CsFromIntegerU( tmp.mem, Capacity( tmp ), &tmp.len, value, 0, 0, 0, 16 );
  AddBackString( out, SliceFromArray( tmp ) );
}
Inl void
PrintU64(
  stack_resizeable_cont_t<u8>* out,
  u64 value
  )
{
  stack_nonresizeable_stack_t<u8, 64> tmp;
  CsFrom_u64( tmp.mem, Capacity( tmp ), &tmp.len, value );
  AddBackString( out, SliceFromArray( tmp ) );
}
Inl void
PrintS64(
  stack_resizeable_cont_t<u8>* out,
  s64 value
  )
{
  stack_nonresizeable_stack_t<u8, 64> tmp;
  CsFrom_s64( tmp.mem, Capacity( tmp ), &tmp.len, value );
  AddBackString( out, SliceFromArray( tmp ) );
}
Inl void
PrintType(
  stack_resizeable_cont_t<u8>* out,
  type_t* type
  )
{
  FORLIST( qualifier, elem, type->qualifiers )
    switch( qualifier->type ) {
      case typedecl_qualifier_type_t::star: {
        AddBackString( out, "*" );
      } break;
      case typedecl_qualifier_type_t::array: {
        AddBackString( out, "[" );
        FORLIST( arrayidx, elem2, qualifier->arrayidxs )
          switch( arrayidx->type ) {
            case typedecl_arrayidx_type_t::star: {
              AddBackString( out, "*" );
            } break;
            case typedecl_arrayidx_type_t::expr_const: {
              stack_nonresizeable_stack_t<u8, 64> tmp;
              CsFromIntegerU( tmp.mem, Capacity( tmp ), &tmp.len, arrayidx->value );
              AddBackString( out, SliceFromArray( tmp ) );
            } break;
            default: UnreachableCrash();
          }
          if( elem2 != qualifier->arrayidxs.last ) {
            AddBackString( out, ", " );
          }
        }
        AddBackString( out, "]" );
      } break;
      default: UnreachableCrash();
    }
  }
  if( type->qualifiers.len ) {
    AddBackString( out, " " );
  }
  switch( type->type ) {
    case type_type_t::num: {
      auto bytecount = type->num.bytecount;
      switch( type->num.type ) {
        case num_type_t::unsigned_: {
          AddBackString( out, "u" );
        } break;
        case num_type_t::signed_: {
          AddBackString( out, "s" );
        } break;
        case num_type_t::float_: {
          AddBackString( out, "f" );
        } break;
        default: UnreachableCrash();
      }
      stack_nonresizeable_stack_t<u8, 64> tmp;
      CsFromIntegerU( tmp.mem, Capacity( tmp ), &tmp.len, bytecount * 8 );
      AddBackString( out, SliceFromArray( tmp ) );
    } break;
    case type_type_t::bool_: {
      AddBackString( out, "bool" );
    } break;
    case type_type_t::string_: {
      AddBackString( out, "string" );
    } break;
    case type_type_t::struct_: {
      PrintIdent( out, &type->structdecltype->ident );
    } break;
    case type_type_t::enum_: {
      PrintIdent( out, &type->enumdecltype->ident );
    } break;
    default: UnreachableCrash();
  }
}
Inl void
PrintVar(
  stack_resizeable_cont_t<u8>* out,
  var_t* var
  )
{
  AddBackString( out, "var" );
  PrintU64( out, var->idx );
  AddBackString( out, ":" );
  PrintType( out, &var->type );
}
Inl void
PrintVartable(
  stack_resizeable_cont_t<u8>* out,
  vartable_t* vartable,
  idx_t indent
  )
{
  PrintIndent( out, indent );
  AddBackString( out, "vartable {\n" );
  FORLIST( namedvar, elem, vartable->namedvars )
    PrintIndent( out, indent + 2 );
    AddBackString( out, "namedvar " );
    PrintIdent( out, &namedvar->ident );
    AddBackString( out, " " );
    PrintVar( out, namedvar->var );
    AddBackString( out, "\n" );
  }
  FORLIST( child, elem, vartable->vartables )
    PrintVartable( out, child, indent + 2 );
  }
  PrintIndent( out, indent );
  AddBackString( out, "}\n" );
}
NoInl void
PrintGlobalScopeTypes(
  stack_resizeable_cont_t<u8>* out,
  compilecontext_t* ctx,
  global_scope_t* global_scope
  )
{
  auto globaltypes = ctx->globaltypes;
  FORLIST( enumdecltype, elem, globaltypes->enumdecltypes )
    AddBackString( out, "enum " );
    PrintIdent( out, &enumdecltype->ident );
    AddBackString( out, " " );
    PrintType( out, &enumdecltype->type );
    AddBackString( out, " {\n" );
    ForLen( j, enumdecltype->values ) {
      auto value = enumdecltype->values.mem + j;
      AddBackString( out, "  " );
      AddBackString( out, "value " );
      PrintIdent( out, &value->ident );
      AddBackString( out, " " );
      stack_nonresizeable_stack_t<u8, 64> tmp;
      CsFromIntegerU( tmp.mem, Capacity( tmp ), &tmp.len, value->value );
      AddBackString( out, SliceFromArray( tmp ) );
      AddBackString( out, "\n" );
    }
    AddBackString( out, "}\n" );
  }
  FORLIST( namedvar_constant, elem, globaltypes->constants )
    AddBackString( out, "constant " );
    PrintIdent( out, &namedvar_constant->ident );
    AddBackString( out, " " );
    PrintVar( out, namedvar_constant->var );
    AddBackString( out, "\n" );
  }
  FORLIST( structdecltype, elem, globaltypes->structdecltypes )
    AddBackString( out, "struct " );
    PrintIdent( out, &structdecltype->ident );
    AddBackString( out, " {\n" );
    FORLEN( type_field, j, structdecltype->fields )
      AddBackString( out, "  " );
      AddBackString( out, "field " );
      PrintIdent( out, &type_field->ident );
      AddBackString( out, " " );
      PrintType( out, &type_field->type );
      AddBackString( out, "\n" );
    }
    AddBackString( out, "}\n" );
  }
  FORLIST( fndecltype, elem, globaltypes->fndecltypes )
    AddBackString( out, "fndecl " );
    PrintIdent( out, &fndecltype->ident );
    AddBackString( out, " {\n" );
    FORLEN( type_arg, j, fndecltype->args )
      AddBackString( out, "  " );
      AddBackString( out, "arg " );
      PrintIdent( out, &type_arg->ident );
      AddBackString( out, " " );
      PrintType( out, &type_arg->type );
      AddBackString( out, "\n" );
    }
    FORLEN( type_ret, j, fndecltype->rets )
      AddBackString( out, "  " );
      AddBackString( out, "ret " );
      PrintType( out, type_ret );
      AddBackString( out, "\n" );
    }
    AddBackString( out, "}\n" );
  }
  FORLIST( function, elem, globaltypes->functions )
    AddBackString( out, "function " );
    PrintIdent( out, &function->fndecltype->ident );
    AddBackString( out, " {\n" );
    FORLEN( namedvar_arg, j, function->namedvar_args )
      AddBackString( out, "  " );
      AddBackString( out, "arg " );
      PrintIdent( out, &namedvar_arg->ident );
      AddBackString( out, " " );
      PrintVar( out, namedvar_arg->var );
      AddBackString( out, "\n" );
    }
    PrintVartable( out, &function->vartable, 2 );
    AddBackString( out, "  " );
    AddBackString( out, "tvars {\n" );
    FORLIST( tvar, j, function->tvars )
      AddBackString( out, "    " );
      PrintVar( out, tvar );
      AddBackString( out, "\n" );
    }
    AddBackString( out, "  " );
    AddBackString( out, "}\n" );
    AddBackString( out, "  " );
    AddBackString( out, "tcode {\n" );
    FORLIST( tc, j, function->tcode )
      switch( tc->type ) {
        case tc_type_t::fncall: {
          AddBackString( out, "    " );
          AddBackString( out, "fncall " );
          PrintIdent( out, &tc->fncall.fndecltype->ident );
          AddBackString( out, " {\n" );
          FORLEN( var_arg, k, tc->fncall.var_args )
            AddBackString( out, "      " );
            AddBackString( out, "arg " );
            PrintVar( out, *var_arg );
            AddBackString( out, "\n" );
          }
          FORLEN( var_ret, k, tc->fncall.var_rets )
            AddBackString( out, "      " );
            AddBackString( out, "ret " );
            PrintVar( out, *var_ret );
            AddBackString( out, "\n" );
          }
          AddBackString( out, "    " );
          AddBackString( out, "}\n" );
        } break;
        case tc_type_t::ret: {
          AddBackString( out, "    " );
          AddBackString( out, "ret\n" );
        } break;
        case tc_type_t::move: {
          AddBackString( out, "    " );
          AddBackString( out, "move " );
          PrintVar( out, tc->move.var_l );
          AddBackString( out, " <- " );
          PrintVar( out, tc->move.var_r );
          AddBackString( out, "\n" );
        } break;
        case tc_type_t::loadconstant: {
          AddBackString( out, "    " );
          AddBackString( out, "loadconstant " );
          PrintVar( out, tc->loadconstant.var_l );
          AddBackString( out, " <- immediate\n" );
        } break;
        case tc_type_t::store: {
          AddBackString( out, "    " );
          AddBackString( out, "store " );
          PrintVar( out, tc->store.var_l );
          AddBackString( out, " <- " );
          PrintVar( out, tc->store.var_r );
          AddBackString( out, "\n" );
        } break;
        case tc_type_t::jumplabel: {
          AddBackString( out, "    " );
          AddBackString( out, "jumplabel " );
          PrintU64Hex( out, Cast( u64, tc ) );
          AddBackString( out, "\n" );
        } break;
        case tc_type_t::jump: {
          AddBackString( out, "    " );
          AddBackString( out, "jump " );
          PrintU64Hex( out, Cast( u64, tc->jump.jumplabel ) );
          AddBackString( out, "\n" );
        } break;
        case tc_type_t::jumpnotzero: {
          AddBackString( out, "    " );
          AddBackString( out, "jumpnotzero " );
          PrintVar( out, tc->jumpcond.var_cond );
          AddBackString( out, " " );
          PrintU64Hex( out, Cast( u64, tc->jumpcond.jump.jumplabel ) );
          AddBackString( out, "\n" );
        } break;
        case tc_type_t::jumpzero: {
          AddBackString( out, "    " );
          AddBackString( out, "jumpzero " );
          PrintVar( out, tc->jumpcond.var_cond );
          AddBackString( out, " " );
          PrintU64Hex( out, Cast( u64, tc->jumpcond.jump.jumplabel ) );
          AddBackString( out, "\n" );
        } break;
        case tc_type_t::binop: {
          AddBackString( out, "    " );
          AddBackString( out, "binop " );
          PrintVar( out, tc->binop.var_result );
          AddBackString( out, " <- " );
          AddBackString( out, StringFromBinoptype( tc->binop.type ) );
          AddBackString( out, "( " );
          PrintVar( out, tc->binop.var_l );
          AddBackString( out, ", " );
          PrintVar( out, tc->binop.var_r );
          AddBackString( out, " )\n" );
        } break;
        case tc_type_t::unop: {
          AddBackString( out, "    " );
          AddBackString( out, "unop " );
          AddBackString( out, StringFromUnoptype( tc->unop.type ) );
          AddBackString( out, " " );
          PrintVar( out, tc->unop.var_result );
          AddBackString( out, " <- " );
          PrintVar( out, tc->unop.var );
          AddBackString( out, "\n" );
        } break;
        case tc_type_t::autocast: {
          AddBackString( out, "    " );
          AddBackString( out, "autocast " );
          PrintVar( out, tc->autocast.var_result );
          AddBackString( out, " <- " );
          PrintVar( out, tc->autocast.var );
          AddBackString( out, "\n" );
        } break;

        #define _ARRAYELEMFROMARRAY( _name ) \
          case tc_type_t::_name: { \
            AddBackString( out, "    " ); \
            AddBackString( out, # _name ); \
            AddBackString( out, " " ); \
            PrintVar( out, tc->arrayidx.var_result ); \
            AddBackString( out, " <- " ); \
            PrintVar( out, tc->arrayidx.var_array ); \
            AddBackString( out, " indexed by [ " ); \
            FORLEN( var_arrayidx, k, tc->arrayidx.var_arrayidxs ) \
              PrintVar( out, *var_arrayidx ); \
              if( k + 1 != tc->arrayidx.var_arrayidxs.len ) { \
                AddBackString( out, ", " ); \
              } \
            } \
            AddBackString( out, " ]" ); \
            AddBackString( out, "\n" ); \
          } break; \

        _ARRAYELEMFROMARRAY( array_elem_from_array )
        _ARRAYELEMFROMARRAY( array_elem_from_parray )
        _ARRAYELEMFROMARRAY( array_pelem_from_array )
        _ARRAYELEMFROMARRAY( array_pelem_from_parray )

        #define _STRUCTFIELDFROMSTRUCT( _name ) \
          case tc_type_t::_name: { \
            AddBackString( out, "    " ); \
            AddBackString( out, # _name ); \
            AddBackString( out, " " ); \
            PrintVar( out, tc->fieldaccess.var_result ); \
            AddBackString( out, " <- fieldoffset " ); \
            PrintU64( out, tc->fieldaccess.field_offset ); \
            AddBackString( out, " into " ); \
            PrintVar( out, tc->fieldaccess.var_struct ); \
            AddBackString( out, "\n" ); \
          } break; \

        _STRUCTFIELDFROMSTRUCT( struct_field_from_struct )
        _STRUCTFIELDFROMSTRUCT( struct_field_from_pstruct )
        _STRUCTFIELDFROMSTRUCT( struct_pfield_from_struct )
        _STRUCTFIELDFROMSTRUCT( struct_pfield_from_pstruct )

        case tc_type_t::addrof: {
          AddBackString( out, "    " );
          AddBackString( out, "addrof " );
          PrintVar( out, tc->addrof.var_result );
          AddBackString( out, " <- " );
          PrintVar( out, tc->addrof.var );
          AddBackString( out, "\n" );
        } break;
        case tc_type_t::deref: {
          AddBackString( out, "    " );
          AddBackString( out, "deref " );
          PrintVar( out, tc->deref.var_result );
          AddBackString( out, " <- " );
          PrintVar( out, tc->deref.var );
          AddBackString( out, "\n" );
        } break;
        default: UnreachableCrash();
      }
    }
    AddBackString( out, "  " );
    AddBackString( out, "}\n" );
    AddBackString( out, "}\n" );
  }
}

Inl bool
IdentsMatch(
  ident_t* a,
  ident_t* b
  )
{
  if( a == b ) return 1;
  if( a->literal == b->literal ) return 1;
  auto r = MemEqual( ML( a->text ), ML( b->text ) );
  return r;
}
Inl bool
TypesEqual(
  type_t* a,
  type_t* b
  )
{
  if( a == b )  return 1;
  if( a->type != b->type )  return 0;
  if( a->qualifiers.len != b->qualifiers.len )  return 0;
  BEGIN_FORTWOLISTS( idx, qualifier_a, a->qualifiers, qualifier_b, b->qualifiers )
    if( qualifier_a->type != qualifier_b->type )  return 0;
    if( qualifier_a->arrayidxs.len != qualifier_b->arrayidxs.len )  return 0;
    BEGIN_FORTWOLISTS2( arrayidx_a, qualifier_a->arrayidxs, arrayidx_b, qualifier_b->arrayidxs )
      if( arrayidx_a->type != arrayidx_b->type )  return 0;
      if( arrayidx_a->value != arrayidx_b->value )  return 0;
    END_FORTWOLISTS2
  END_FORTWOLISTS
  switch( a->type ) {
    case type_type_t::num: {
      if( a->num.bytecount != b->num.bytecount )  return 0;
    } break;
    case type_type_t::struct_: {
      if( a->structdecltype != b->structdecltype )  return 0;
    } break;
    case type_type_t::bool_:    break;
    case type_type_t::string_:  break;
  }
  return 1;
}
Inl bool
IsBasicNumberType(
  compilecontext_t* ctx,
  type_t* type,
  num_type_t* num_type
  )
{
  auto globaltypes = ctx->globaltypes;
  type_t* type_unums[] = {
    &globaltypes->type_u8,
    &globaltypes->type_u16,
    &globaltypes->type_u32,
    &globaltypes->type_u64,
    };
  type_t* type_snums[] = {
    &globaltypes->type_s8,
    &globaltypes->type_s16,
    &globaltypes->type_s32,
    &globaltypes->type_s64,
    };
  type_t* type_fnums[] = {
    &globaltypes->type_f32,
    &globaltypes->type_f64,
    };
  ForEach( type_num, type_unums ) {
    if( TypesEqual( type, type_num ) ) {
      *num_type = num_type_t::unsigned_;
      return 1;
    }
  }
  ForEach( type_num, type_snums ) {
    if( TypesEqual( type, type_num ) ) {
      *num_type = num_type_t::signed_;
      return 1;
    }
  }
  ForEach( type_num, type_fnums ) {
    if( TypesEqual( type, type_num ) ) {
      *num_type = num_type_t::float_;
      return 1;
    }
  }
  return 0;
}
Inl bool
IsBasicBoolType(
  compilecontext_t* ctx,
  type_t* type
  )
{
  auto globaltypes = ctx->globaltypes;
  auto r = TypesEqual( type, &globaltypes->type_bool );
  return r;
}
Inl bool
IsPointerType(
  type_t* type
  )
{
  if( !type->qualifiers.len )  return 0;
  auto first_qualifier = &type->qualifiers.first->value;
  switch( first_qualifier->type ) {
    case typedecl_qualifier_type_t::star:  return 1;
    case typedecl_qualifier_type_t::array:  return 0;
    default: UnreachableCrash();
  }
  return 0;
}
Inl bool
TypelistContainsType(
  typelist_t* list,
  type_t* type
  )
{
  FORLIST( entry, elem, *list )
    if( TypesEqual( entry, type ) )  return 1;
  }
  return 0;
}
Inl void
GenerateAutocast(
  compilecontext_t* ctx,
  list_t<tc_t>* tcode,
  var_t** var, // INOUT
  type_t* target
  )
{
  auto function = ctx->current_function;
  auto var_target = AddBackList( ctx, &function->tvars );
  var_target->type = *target;
  auto tc = AddBackList( ctx, tcode );
  tc->type = tc_type_t::autocast;
  tc->autocast.var = *var;
  tc->autocast.var_result = var_target;
  *var = var_target;
}

Inl bool
RtypeAutocastsToType(
  compilecontext_t* ctx,
  list_t<tc_t>* tcode,
  var_t** var, // INOUT
  rtype_t* rtype,
  type_t* target
  )
{
  if( TypesEqual( &rtype->type, target ) )  return 1;
  if( TypelistContainsType( &rtype->autocasts, target ) ) {
    GenerateAutocast( ctx, tcode, var, target );
    return 1;
  }
  return 0;
}
Inl void
MakeRtypeFromType(
  compilecontext_t* ctx,
  rtype_t* rtype,
  type_t* type
  )
{
  auto globaltypes = ctx->globaltypes;
  rtype->type = *type;
  if( type->qualifiers.len ) {
    auto first_qualifier = &type->qualifiers.first->value;
    switch( first_qualifier->type ) {
      case typedecl_qualifier_type_t::star:
      case typedecl_qualifier_type_t::array: {
        // allow autocast of pointer to bool, by ptr != 0
        // allow autocast of array to bool, by len != 0
        *AddBackList( ctx, &rtype->autocasts ) = globaltypes->type_bool;
        switch( type->type ) {
          case type_type_t::num: {
            // allow autocast of unsigned/signed at the same size.
            if( type->num.type == num_type_t::unsigned_ ) {
              auto signed_type = *type;
              signed_type.num.type = num_type_t::signed_;
              *AddBackList( ctx, &rtype->autocasts ) = signed_type;
            }
            elif( type->num.type == num_type_t::signed_ ) {
              auto unsigned_type = *type;
              unsigned_type.num.type = num_type_t::unsigned_;
              *AddBackList( ctx, &rtype->autocasts ) = unsigned_type;
            }
          } break;
          case type_type_t::bool_: {
            // for now, don't allow autocast of bool -> unsigned/signed.
          } break;
          case type_type_t::string_:
          case type_type_t::struct_: {
            // these can't autocast to anything.
          } break;
          case type_type_t::enum_: {
            // allow autocast of enum to unsigned/signed 4 byte
            // TODO: handle arbitrary enum types via type->enumdecltype.
            auto unsigned_type = AddBackList( ctx, &rtype->autocasts );
            unsigned_type->qualifiers = type->qualifiers;
            unsigned_type->type = type_type_t::num;
            unsigned_type->num.type = num_type_t::unsigned_;
            unsigned_type->num.bytecount = 4;
            auto signed_type = AddBackList( ctx, &rtype->autocasts );
            signed_type->qualifiers = type->qualifiers;
            signed_type->type = type_type_t::num;
            signed_type->num.type = num_type_t::signed_;
            signed_type->num.bytecount = 4;
          } break;
          default: UnreachableCrash();
        }
      } break;
      default: UnreachableCrash();
    }
  }
  else {

    #define ADDAUTOCAST( _type ) \
      *AddBackList( ctx, &rtype->autocasts ) = globaltypes->_type \

    switch( type->type ) {
      case type_type_t::num: {
        auto bytecount = type->num.bytecount;
        switch( type->num.type ) {
          case num_type_t::unsigned_: {
            switch( bytecount ) {
              case 1: ADDAUTOCAST( type_s8  );  ADDAUTOCAST( type_u16 );
              case 2: ADDAUTOCAST( type_s16 );  ADDAUTOCAST( type_u32 );
              case 4: ADDAUTOCAST( type_s32 );  ADDAUTOCAST( type_u64 );
              case 8: ADDAUTOCAST( type_s64 );  break;
              default: UnreachableCrash();
            }
          } break;
          case num_type_t::signed_: {
            switch( bytecount ) {
              case 1: ADDAUTOCAST( type_u8  );  ADDAUTOCAST( type_s16 );
              case 2: ADDAUTOCAST( type_u16 );  ADDAUTOCAST( type_s32 );
              case 4: ADDAUTOCAST( type_u32 );  ADDAUTOCAST( type_s64 );
              case 8: ADDAUTOCAST( type_u64 );  break;
              default: UnreachableCrash();
            }
          } break;
          case num_type_t::float_: {
            switch( bytecount ) {
              case 4: ADDAUTOCAST( type_f64 );  break;
              case 8: break;
              default: UnreachableCrash();
            }
          } break;
          default: UnreachableCrash();
        }
      } break;
      case type_type_t::bool_: {
        // we let bool autocast to all ints, and all floats.
        ADDAUTOCAST( type_u64 );
        ADDAUTOCAST( type_u32 );
        ADDAUTOCAST( type_u16 );
        ADDAUTOCAST( type_u8  );
        ADDAUTOCAST( type_s64 );
        ADDAUTOCAST( type_s32 );
        ADDAUTOCAST( type_s16 );
        ADDAUTOCAST( type_s8  );
        ADDAUTOCAST( type_f64 );
        ADDAUTOCAST( type_f32 );
      } break;
      case type_type_t::string_:
      case type_type_t::struct_: {
        // these can't autocast to anything.
      } break;
      case type_type_t::enum_: {
        // allow autocast of enum to unsigned/signed 4 byte
        // TODO: handle arbitrary enum types via type->enumdecltype.
        auto unsigned_type = AddBackList( ctx, &rtype->autocasts );
        unsigned_type->qualifiers = type->qualifiers;
        unsigned_type->type = type_type_t::num;
        unsigned_type->num.type = num_type_t::unsigned_;
        unsigned_type->num.bytecount = 4;
        auto signed_type = AddBackList( ctx, &rtype->autocasts );
        signed_type->qualifiers = type->qualifiers;
        signed_type->type = type_type_t::num;
        signed_type->num.type = num_type_t::signed_;
        signed_type->num.bytecount = 4;
      } break;
      default: UnreachableCrash();
    }
  }
}
Inl void
InitBuiltinTypes( compilecontext_t* ctx )
{
  auto globaltypes = ctx->globaltypes;
  globaltypes->type_bool.type = type_type_t::bool_;
  globaltypes->type_string.type = type_type_t::string_;
  globaltypes->type_enum.type = type_type_t::enum_;
  globaltypes->type_u8 .type = type_type_t::num;
  globaltypes->type_u8 .num = { num_type_t::unsigned_, 1 };
  globaltypes->type_u16.type = type_type_t::num;
  globaltypes->type_u16.num = { num_type_t::unsigned_, 2 };
  globaltypes->type_u32.type = type_type_t::num;
  globaltypes->type_u32.num = { num_type_t::unsigned_, 4 };
  globaltypes->type_u64.type = type_type_t::num;
  globaltypes->type_u64.num = { num_type_t::unsigned_, 8 };
  globaltypes->type_s8 .type = type_type_t::num;
  globaltypes->type_s8 .num = { num_type_t::signed_, 1 };
  globaltypes->type_s16.type = type_type_t::num;
  globaltypes->type_s16.num = { num_type_t::signed_, 2 };
  globaltypes->type_s32.type = type_type_t::num;
  globaltypes->type_s32.num = { num_type_t::signed_, 4 };
  globaltypes->type_s64.type = type_type_t::num;
  globaltypes->type_s64.num = { num_type_t::signed_, 8 };
  globaltypes->type_f32.type = type_type_t::num;
  globaltypes->type_f32.num = { num_type_t::float_, 4 };
  globaltypes->type_f64.type = type_type_t::num;
  globaltypes->type_f64.num = { num_type_t::float_, 8 };

  MakeRtypeFromType( ctx, &globaltypes->rtype_bool, &globaltypes->type_bool );
}
Inl void
IntersectTypelists(
  compilecontext_t* ctx,
  typelist_t* list0,
  typelist_t* list1,
  typelist_t* result
  )
{
  FORLIST( type0, elem0, *list0 )
    FORLIST( type1, elem1, *list1 )
      if( TypesEqual( type0, type1 ) ) {
        *AddBackList( ctx, result ) = *type0;
        break;
      }
    }
  }
}
Templ Inl void
FindEntryByIdent(
  list_t<T>* list,
  ident_t* ident,
  bool* found,
  T** found_elem
  )
{
  FORLIST( entry, elem, *list )
    if( IdentsMatch( ident, &entry->ident ) ) {
      *found = 1;
      *found_elem = entry;
      return;
    }
  }
  *found = 0;
  *found_elem = 0;
}
Templ Inl void
FindEntryByIdent(
  tslice_t<T>* list,
  ident_t* ident,
  bool* found,
  T** found_elem,
  idx_t* offset_into_list
  )
{
  FORLEN( entry, i, *list )
    if( IdentsMatch( ident, &entry->ident ) ) {
      *found = 1;
      *found_elem = entry;
      *offset_into_list = i;
      return;
    }
  }
  *found = 0;
  *found_elem = 0;
  *offset_into_list = 0;
}
Inl void
TypeExpr(
  compilecontext_t* ctx,
  list_t<tc_t>* tcode,
  expr_t* expr,
  list_t<rtype_t>* rtypes,
  tslice_t<var_t*>* vars
  );
Inl void
TypeScope(
  compilecontext_t* ctx,
  list_t<tc_t>* tcode,
  scope_t* scope,
  tc_t* tc_jumplabel_whileloop_begin,
  tc_t* tc_jumplabel_whileloop_end
  );
Inl void
TypeFncall(
  compilecontext_t* ctx,
  list_t<tc_t>* tcode,
  fncall_t* fncall,
  list_t<rtype_t>* rtypes,
  tslice_t<var_t*>* vars
  )
{
  auto globaltypes = ctx->globaltypes;
  auto function = ctx->current_function;
  bool already_there = 0;
  fndecltype_t* fndecltype = 0;
  FindEntryByIdent( &globaltypes->fndecltypes, &fncall->ident, &already_there, &fndecltype );
  if( !already_there ) {
    ErrorCStr( ctx, fncall->ident.literal, "calling an undeclared function!" );
    return;
  }
  else {
    if( fncall->expr_args.len != fndecltype->args.len ) {
      ErrorCStr( ctx, fncall->ident.literal, "passing a different number of arguments than was declared!" );
      return;
    }
    //
    // TODO: verify that all args/rets in fndecltype are concrete, not generic
    //   if we have a generic, then make sure we generate a new function_t with the types we find here.
    //   or reuse the existing concrete function_t matching our call types.
    //
    auto var_args = AddPagelistSlice( ctx->mem, var_t*, _SIZEOF_IDX_T, fndecltype->args.len );
    auto var_rets = AddPagelistSlice( ctx->mem, var_t*, _SIZEOF_IDX_T, fndecltype->rets.len );
    idx_t idx = 0;
    FORLIST( expr_arg, elem, fncall->expr_args )
      auto arg = fndecltype->args.mem + idx;
      // TODO: cache the type of the expr?
      list_t<rtype_t> rtypes_arg = {};
      tslice_t<var_t*> vars_arg = {};
      TypeExpr( ctx, tcode, *expr_arg, &rtypes_arg, &vars_arg );  IER
      if( rtypes_arg.len > 1 ) {
        ErrorCStr( ctx, fncall->ident.literal, "tuples can't be passed as function arguments!" );
        return;
      }
      auto rtype_arg = &rtypes_arg.first->value;
      auto var_arg = vars_arg.mem[0];
      if( !RtypeAutocastsToType( ctx, tcode, &var_arg, rtype_arg, &arg->type ) ) {
        // TODO: arg location, not the fncall ident location
        ErrorCStr( ctx, fncall->ident.literal, "expression isn't convertible to the declared argument type!" );
        return;
      }
      var_args.mem[idx] = var_arg;
      idx += 1;
    }
    FORLEN( ret, i, fndecltype->rets )
      auto rtype = AddBackList( ctx, rtypes );
      MakeRtypeFromType( ctx, rtype, ret );
      auto var_ret = AddBackList( ctx, &function->tvars );
      var_ret->type = *ret;
      var_rets.mem[i] = var_ret;
    }
    auto tc = AddBackList( ctx, tcode );
    tc->type = tc_type_t::fncall;
    tc->fncall.fndecltype = fndecltype;
    tc->fncall.var_args = var_args;
    tc->fncall.var_rets = var_rets;
    *vars = var_rets;
  }
}
Inl void
LookupVar(
  compilecontext_t* ctx,
  ident_t* ident,
  bool* found,
  var_t** var
  )
{
  auto globaltypes = ctx->globaltypes;
  auto function = ctx->current_function;
  // check the local scopes for regular vars
  namedvar_t* namedvar = 0;
  REVERSEFORLEN( vartable, i, ctx->scopestack )
    FindEntryByIdent( &(*vartable)->namedvars, ident, found, &namedvar );
    if( *found ) {
      *var = namedvar->var;
      return;
    }
  }
  // check the function for arg vars
  idx_t offset_into_list = 0;
  FindEntryByIdent( &function->namedvar_args, ident, found, &namedvar, &offset_into_list );
  if( *found ) {
    *var = namedvar->var;
    return;
  }
  // check the global scope for constants
  FindEntryByIdent( &globaltypes->constants, ident, found, &namedvar );
  if( *found ) {
    *var = namedvar->var;
    return;
  }
  *found = 0;
  *var = 0;
}
Inl void
_TypeArrayIdxs(
  compilecontext_t* ctx,
  list_t<tc_t>* tcode,
  expr_assignable_entry_t* entry,
  tslice_t<var_t*>* var_arrayidxs
  )
{
  auto globaltypes = ctx->globaltypes;
  // TODO: arrayidxs in an expr_assignable should be allowed to be any int type.
  *var_arrayidxs = AddPagelistSlice( ctx->mem, var_t*, _SIZEOF_IDX_T, entry->expr_arrayidxs.len );
  idx_t idx = 0;
  FORLIST( expr_arrayidx, elem2, entry->expr_arrayidxs )
    list_t<rtype_t> rtypes_arrayidx = {};
    tslice_t<var_t*> vars_arrayidx = {};
    TypeExpr( ctx, tcode, *expr_arrayidx, &rtypes_arrayidx, &vars_arrayidx );  IER
    if( rtypes_arrayidx.len > 1 ) {
      ErrorCStr( ctx, entry->ident.literal, "tuples can't be passed as array indices!" );
      return;
    }
    auto rtype_arrayidx = &rtypes_arrayidx.first->value;
    auto var_arrayidx = vars_arrayidx.mem[0];
    if( !RtypeAutocastsToType( ctx, tcode, &var_arrayidx, rtype_arrayidx, &globaltypes->type_u32 ) ) {
      // TODO: better messaging/location.
      ErrorCStr( ctx, entry->ident.literal, "array index expected to be u32 type!" );
      return;
    }
    var_arrayidxs->mem[idx] = var_arrayidx;
    idx += 1;
  }
}

//
// general algorithm for TypeExprAssignable( wants_address_not_value = 0 ):
//
//   start with first ident, which has to be a var or enum.
//   t = lookup var type.
//   if not found
//     t = lookup enum type
//     if not found, error.
//     advance to next in dotchain.
//     verify elem matches an enum value.
//     verify we're not arrayidxing, you can't arrayidx an enum value itself.
//     verify no more in dotchain, you can't dot an enum value.
//     emit a var_t and a loadconstant op of the enum value.
//     return u32 ( type of enum )
// LABEL_LOOP:
//   if we're arrayidxing
//     verify t is an array type.
//     type the given expr arrayidxs.
//     verify expr arrayidxs.len matches the typedecl.
//     t = t with the first array qualifier stripped, since we did an arrayidx.
//     emit a var_t and an arrayidx op.
//     var_entry = the var_t containing the value after doing the arrayidx.
//   if we're the last in the dot chain
//     return t;
//   else we're not the last
//     if t is not a struct or pointer to struct
//       error 'only allowed to dot an enumdecl, struct instance, or pointer to struct instance'
//       return
//     else
//       advance to next in dotchain.
//       verify elem matches a struct field.
//       t = type of struct field.
//       jump LABEL_LOOP
//   else
//     advance to next in dotchain.
//     t = lookup var type.
//     jump LABEL_LOOP
//
//
// this won't work for arrays of pointers...
// we need to follow the same path as the above algorithm, but emit different code.
// and our final var will be a pointer to the thing.
//
// let me work some examples:
//   foo_t struct { bar u8; car [] u8; }
//   ...
//   foo foo_t;
//   foo.bar = 10;
// should turn into something like:
//   namedvars {
//     foo var0:u8
//   }
//   loadconstant var1:u8 <- 10
//   struct_pfield_from_struct var2:*u8 <- field_offset 0 into var0:foo_t
//   store var2:*u8 <- var1:u8
//
// now arrayidx case:
//   foo.car[10] = 20;
// should turn into:
//   loadconstant var1:u8 <- 10
//   loadconstant var2:u8 <- 20
//   struct_pfield_from_struct var3:*[]u8 <- field_offset 1 into var0:foo_t
//   autocast var4:u32 <- var1:u8
//   array_pelem_from_parray var5:*u8 <- var3:*[]u8 indexed by [ var4:u32 ]
//     note the dst_elem and src_array are both pointers.
//   store var5:*u8 <- var2:u8
//
// simple array case:
//   foos [100] foo_t;
//   foos[10].bar = 20;
// should turn into:
//   loadconstant var1:u8 <- 10
//   loadconstant var2:u8 <- 20
//   array_pelem_from_array var3:*foo_t <- var0:[]foo_t indexed by [ var1:u8 ]
//   struct pfield_from_struct var4:*u8 <- field_offset 0 into var3:*foo_t
//   store var4:*u8 <- var2:u8
//
// now a compound case:
//   bar_t struct { value u8; }
//   foo_t struct { elems [] bar_t };
//   ...
//   foo foo_t;
//   foo.elems[10].value = 20;
// should turn into something like:
//   loadconstant var1:u8 <- 10
//   loadconstant var2:u8 <- 20
//   struct_pfield_from_struct var3:*[]bar_t <- field_offset 0 into var0:foo_t
//   autocast var4:u32 <- var1:u8
//   array_pelem_from_parray var5:*bar_t <- var3:*[]bar_t indexed by [ var4:u32 ]
//   struct_pfield_from_pstruct var6:*u8 <- field_offset 0 into var5:*bar_t
//     note dst_field and src_struct are both pointers, different from struct_pfield_from_struct.
//   store var6:*u8 <- var2:u8
//
// another compound case:
//   bar_t struct { value u8; }
//   foo_t struct { elems [] * bar_t };
//   ...
//   foo foo_t;
//   foo.elems[10].value = 20;
// should turn into something like:
//   loadconstant var1:u8 <- 10
//   loadconstant var2:u8 <- 20
//   struct_pfield_from_struct var3:*[]*bar_t <- field_offset 0 into var0:foo_t
//   autocast var4:u32 <- var1:u8
//   array_elem_from_parray var5:*bar_t <- var3:*[]*bar_t indexed by [ var4:u32 ]
//     note we array_elem_from_parray here, not array_pelem_from_parray, since the array elems are pointers and we're about to deref.
//     this is the earliest we can do this check, so it probably belongs here.
//   struct_pfield_from_pstruct var6:*u8 <- field_offset 0 into var5:*bar_t
//   store var6:*u8 <- var2:u8
//
// TODO: should we allow this, bracket array syntax for a pointer to an array?
//   sure, why not. we'll disallow arrayidx syntax for pointers, so this is fine.
// another compound case:
//   bar_t struct { value u8; }
//   foo_t struct { elems * [] * bar_t };
//   ...
//   foo foo_t;
//   foo.elems[10].value = 20;
// should turn into:
//   loadconstant var1:u8 <- 10
//   loadconstant var2:u8 <- 20
//   struct_field_from_struct var3:*[]*bar_t <- field_offset 0 into var0:foo_t
//     note we struct_field_from_struct here, not struct_pfield_from_struct, since the field is a pointer and we're about to deref.
//   autocast var4:u32 <- var1:u8
//   array_elem_from_parray var5:*bar_t <- var3:*[]*bar_t indexed by [ var4:u32 ]
//   struct_pfield_from_pstruct var6:*u8 <- field_offset 0 into var5:*bar_t
//   store var6:*u8 <- var2:u8
//
// very compound case:
//   car_t struct { value u8; }
//   bar_t struct { cars [] * car_t; }
//   foo_t struct { bars * [] bar_t; };
//   ...
//   foo foo_t;
//   foo.bars[10].cars[20].value = 30;
// should turn into:
//   loadconstant var1:u8 <- 10
//   loadconstant var2:u8 <- 20
//   loadconstant var3:u8 <- 30
//   struct_field_from_struct var4:*[]bar_t <- field_offset 0 into var0:foo_t
//   array_pelem_from_parray var5:*bar_t <- var4:*[]bar_t indexed by [ var1:u8 ]
//   struct_pfield_from_pstruct var6:*[]*car_t <- field_offset 0 into var5:*bar_t
//   array_elem_from_parray var7:*car_t <- var6:*[]*car_t indexed by [ var2:u8 ]
//   struct_pfield_from_pstruct var8:*u8 <- field_offset 0 into var7:*car_t
//   store var8:*u8 <- var3:u8
//
// in summary,
//
// set of operations we need to do expr assignables:
//
// array_elem_from_array    elem: T <-  array: []T indexed by [ idx:u32 ]
// array_elem_from_parray   elem: T <- parray:*[]T indexed by [ idx:u32 ]
// array_pelem_from_array  pelem:*T <-  array: []T indexed by [ idx:u32 ]
// array_pelem_from_parray pelem:*T <- parray:*[]T indexed by [ idx:u32 ]
//
// struct_field_from_struct    elem: T <- fieldoffset K into  struct: S
// struct_field_from_pstruct   elem: T <- fieldoffset K into pstruct:*S
// struct_pfield_from_struct  pelem:*T <- fieldoffset K into  struct: S
// struct_pfield_from_pstruct pelem:*T <- fieldoffset K into pstruct:*S
//
//
// wants_address_not_value is true for the "foo.bar" part of "foo.bar[foo.car] = foo.dar;"
// it's also true in cases like "Foo( addrof foo.bar );"
// it's how we distinguish whether we want a new stackvar holding the value,
// or a stackvar holding a pointer to the field.
//
Inl void
TypeExprAssignable(
  compilecontext_t* ctx,
  list_t<tc_t>* tcode,
  expr_assignable_t* expr_assignable,
  rtype_t* rtype,
  var_t** var,
  bool wants_address_not_value,
  bool* just_ident
  )
{
  auto globaltypes = ctx->globaltypes;
  auto function = ctx->current_function;
  // TODO: consider inlining wants_address_not_value with the !wants_address_not_value code, since they're similar.
  if( wants_address_not_value ) {
    var_t* var_entry = 0;
    auto elem = expr_assignable->expr_assignable_entries.first;
    auto entry = &elem->value;
    *just_ident = !elem->next  &&  !entry->expr_arrayidxs.len;
    bool found = 0;
    LookupVar( ctx, &entry->ident, &found, &var_entry );
    if( !found ) {
      enumdecltype_t* enumdecltype = 0;
      FindEntryByIdent( &globaltypes->enumdecltypes, &entry->ident, &found, &enumdecltype );
      if( !found ) {
        ErrorCStr( ctx, entry->ident.literal, "using undefined type!" );
        return;
      }
      ErrorCStr( ctx, entry->ident.literal, "enum values don't have an address!" );
      return;
    }
    while( elem ) {
      auto last_in_dotchain = ( elem == expr_assignable->expr_assignable_entries.last );
      if( entry->expr_arrayidxs.len ) {
        // we're array-accessing the ident
        if( !var_entry->type.qualifiers.len ) {
          // TODO: better messaging/location.
          ErrorCStr( ctx, entry->ident.literal, "basic types dont't allow array indexing!" );
          return;
        }
        auto elem_first_qualifier = var_entry->type.qualifiers.first;
        auto first_qualifier = &elem_first_qualifier->value;
        switch( first_qualifier->type ) {
          case typedecl_qualifier_type_t::star: {
            auto elem_next_qualifier = elem_first_qualifier->next;
            if( !elem_next_qualifier ) {
              // TODO: better messaging/location.
              ErrorCStr( ctx, entry->ident.literal, "only arrays and pointers to arrays can be array-indexed!" );
              return;
            }
            auto next_qualifier = &elem_next_qualifier->value;
            switch( next_qualifier->type ) {
              case typedecl_qualifier_type_t::star: {
                // TODO: better messaging/location.
                ErrorCStr( ctx, entry->ident.literal, "only arrays and pointers to arrays can be array-indexed!" );
                return;
              } break;
              case typedecl_qualifier_type_t::array: {
                // we're array-accessing a pointer to an array.
                if( entry->expr_arrayidxs.len != next_qualifier->arrayidxs.len ) {
                  // TODO: better messaging/location.
                  ErrorCStr( ctx, entry->ident.literal, "passing the wrong number of array indices!" );
                  return;
                }
                tslice_t<var_t*> var_arrayidxs = {};
                _TypeArrayIdxs( ctx, tcode, entry, &var_arrayidxs );  IER
                bool arrayelems_are_pointers = 0;
                auto elem_third_qualifier = elem_next_qualifier->next;
                if( elem_third_qualifier ) {
                  auto third_qualifier = &elem_third_qualifier->value;
                  if( third_qualifier->type == typedecl_qualifier_type_t::star ) {
                    arrayelems_are_pointers = 1;
                  }
                }
                if( arrayelems_are_pointers  &&  !last_in_dotchain ) {
                  auto var_next = AddBackList( ctx, &function->tvars );
                  var_next->type = var_entry->type;
                  var_next->type.qualifiers = CopyList( ctx, &var_entry->type.qualifiers );
                  RemFirst( var_next->type.qualifiers ); // strip pointer
                  RemFirst( var_next->type.qualifiers ); // strip array
                  auto tc = AddBackList( ctx, tcode );
                  tc->type = tc_type_t::array_elem_from_parray;
                  tc->arrayidx.var_array = var_entry; // pointer to array
                  tc->arrayidx.var_arrayidxs = var_arrayidxs;
                  tc->arrayidx.var_result = var_next; // elem value, which is a pointer itself.
                  var_entry = var_next;
                }
                else {
                  auto var_next = AddBackList( ctx, &function->tvars );
                  var_next->type = var_entry->type;
                  var_next->type.qualifiers = CopyList( ctx, &var_entry->type.qualifiers );
                  Rem( var_next->type.qualifiers, var_next->type.qualifiers.first->next ); // *[]T -> *T
                  auto tc = AddBackList( ctx, tcode );
                  tc->type = tc_type_t::array_pelem_from_parray;
                  tc->arrayidx.var_array = var_entry; // pointer to array
                  tc->arrayidx.var_arrayidxs = var_arrayidxs;
                  tc->arrayidx.var_result = var_next; // pointer to elem
                  var_entry = var_next;
                }
              } break;
              default: UnreachableCrash();
            }
          } break;
          case typedecl_qualifier_type_t::array: {
            if( entry->expr_arrayidxs.len != first_qualifier->arrayidxs.len ) {
              // TODO: better messaging/location.
              ErrorCStr( ctx, entry->ident.literal, "passing the wrong number of array indices!" );
              return;
            }
            tslice_t<var_t*> var_arrayidxs = {};
            _TypeArrayIdxs( ctx, tcode, entry, &var_arrayidxs );  IER
            auto var_next = AddBackList( ctx, &function->tvars );
            var_next->type = var_entry->type;
            // []T -> *T
            var_next->type.qualifiers = CopyList( ctx, &var_entry->type.qualifiers );
            auto first_qualifier_next =  &var_next->type.qualifiers.first->value;
            first_qualifier_next->type = typedecl_qualifier_type_t::star;
            first_qualifier_next->arrayidxs = {};
            auto tc = AddBackList( ctx, tcode );
            tc->type = tc_type_t::array_pelem_from_array;
            tc->arrayidx.var_array = var_entry;
            tc->arrayidx.var_arrayidxs = var_arrayidxs;
            tc->arrayidx.var_result = var_next;
            var_entry = var_next;
          } break;
          default: UnreachableCrash();
        }
      }
      if( !last_in_dotchain ) {
        // we're dot-accessing the var_entry
        // see if it's a struct, or pointer-to-struct
        bool can_dot_access = 0;
        bool pointer_to_struct = 0;
        if( var_entry->type.type == type_type_t::struct_ ) {
          if( !var_entry->type.qualifiers.len ) {
            can_dot_access = 1;
          }
          elif( var_entry->type.qualifiers.len == 1 ) {
            auto elem_first_qualifier = var_entry->type.qualifiers.first;
            auto first_qualifier = &elem_first_qualifier->value;
            if( first_qualifier->type == typedecl_qualifier_type_t::star ) {
              can_dot_access = 1;
              pointer_to_struct = 1;
            }
          }
        }
        if( !can_dot_access ) {
          ErrorCStr( ctx, entry->ident.literal, "only enum types, structs, pointers to structs can be dot-accessed!" );
          return;
        }
        auto structdecltype = var_entry->type.structdecltype;
        // verify the next ident is a struct field, and then allow further dots/arrayidxs.
        elem = elem->next;
        entry = &elem->value;
        last_in_dotchain = ( elem == expr_assignable->expr_assignable_entries.last );
        namedtype_t* namedtype_field = 0;
        idx_t offset_into_list = 0;
        FindEntryByIdent( &structdecltype->fields, &entry->ident, &found, &namedtype_field, &offset_into_list );
        if( !found ) {
          // TODO: better messaging/location.
          ErrorCStr( ctx, entry->ident.literal, "field doesn't exist under this struct!" );
          return;
        }
        bool field_is_pointer = 0;
        auto elem_field_first_qualifier = namedtype_field->type.qualifiers.first;
        if( elem_field_first_qualifier ) {
          auto field_first_qualifier = &elem_field_first_qualifier->value;
          if( field_first_qualifier->type == typedecl_qualifier_type_t::star ) {
            field_is_pointer = 1;
          }
        }
        if( field_is_pointer  &&  !last_in_dotchain ) {
          auto var_field = AddBackList( ctx, &function->tvars );
          var_field->type = namedtype_field->type;
          // return the actual field type directly.
          auto tc = AddBackList( ctx, tcode );
          tc->type =
            pointer_to_struct  ?
            tc_type_t::struct_field_from_pstruct  :
            tc_type_t::struct_field_from_struct
            ;
          tc->fieldaccess.var_struct = var_entry;
          tc->fieldaccess.field_offset = offset_into_list;
          tc->fieldaccess.field_name = namedtype_field->ident;
          tc->fieldaccess.var_result = var_field; // field value itself.
          var_entry = var_field;
        }
        else {
          auto var_field = AddBackList( ctx, &function->tvars );
          var_field->type = namedtype_field->type;
          var_field->type.qualifiers = CopyList( ctx, &namedtype_field->type.qualifiers );
          auto added_qualifier = AddFrontList( ctx, &var_field->type.qualifiers );
          added_qualifier->type = typedecl_qualifier_type_t::star;
          auto tc = AddBackList( ctx, tcode );
          tc->type =
            pointer_to_struct  ?
            tc_type_t::struct_pfield_from_pstruct  :
            tc_type_t::struct_pfield_from_struct
            ;
          tc->fieldaccess.var_struct = var_entry;
          tc->fieldaccess.field_offset = offset_into_list;
          tc->fieldaccess.field_name = namedtype_field->ident;
          tc->fieldaccess.var_result = var_field; // pointer to field
          var_entry = var_field;
        }
        continue;
      }
      elem = elem->next;
      if( !elem ) {
        break;
      }
      entry = &elem->value;
      LookupVar( ctx, &entry->ident, &found, &var_entry );
      if( !found ) {
        ErrorCStr( ctx, entry->ident.literal, "using undefined type!" );
        return;
      }
    }
    MakeRtypeFromType( ctx, rtype, &var_entry->type );
    *var = var_entry;
  }
  else {
    var_t* var_entry = 0;
    auto elem = expr_assignable->expr_assignable_entries.first;
    auto entry = &elem->value;
    *just_ident = !elem->next  &&  !entry->expr_arrayidxs.len;
    bool found = 0;
    LookupVar( ctx, &entry->ident, &found, &var_entry );
    if( !found ) {
      enumdecltype_t* enumdecltype = 0;
      FindEntryByIdent( &globaltypes->enumdecltypes, &entry->ident, &found, &enumdecltype );
      if( !found ) {
        ErrorCStr( ctx, entry->ident.literal, "using undefined type!" );
        return;
      }
      if( elem == expr_assignable->expr_assignable_entries.last ) {
        ErrorCStr( ctx, entry->ident.literal, "enum names aren't expressions! use a dot to access individual values." );
        return;
      }
      // verify the next ident is an enum value, and then you're not allowed to dot/arrayidx any further.
      elem = elem->next;
      entry = &elem->value;
      enumdecltype_value_t* enumvalue = 0;
      idx_t offset_into_list = 0;
      FindEntryByIdent( &enumdecltype->values, &entry->ident, &found, &enumvalue, &offset_into_list );
      if( !found ) {
        // TODO: better messaging/location.
        ErrorCStr( ctx, entry->ident.literal, "value doesn't exist under this enum!" );
        return;
      }
      if( elem != expr_assignable->expr_assignable_entries.last ) {
        // TODO: better messaging/location.
        ErrorCStr( ctx, entry->ident.literal, "enum values can't be dotted or array-indexed!" );
        return;
      }
      MakeRtypeFromType( ctx, rtype, &globaltypes->type_u32 );
      var_entry = AddBackList( ctx, &function->tvars );
      var_entry->type = globaltypes->type_u32;
      auto tc = AddBackList( ctx, tcode );
      tc->type = tc_type_t::loadconstant;
      tc->loadconstant.var_l = var_entry;
      tc->loadconstant.mem = &enumvalue->value;
      *var = var_entry;
      return;
    }
    while( elem ) {
      auto last_in_dotchain = ( elem == expr_assignable->expr_assignable_entries.last );
      if( entry->expr_arrayidxs.len ) {
        // we're array-accessing the ident
        if( !var_entry->type.qualifiers.len ) {
          // TODO: better messaging/location.
          ErrorCStr( ctx, entry->ident.literal, "basic types dont't allow array indexing!" );
          return;
        }
        auto elem_first_qualifier = var_entry->type.qualifiers.first;
        auto first_qualifier = &elem_first_qualifier->value;
        switch( first_qualifier->type ) {
          case typedecl_qualifier_type_t::star: {
            auto elem_next_qualifier = elem_first_qualifier->next;
            if( !elem_next_qualifier ) {
              // TODO: better messaging/location.
              ErrorCStr( ctx, entry->ident.literal, "only arrays and pointers to arrays can be array-indexed!" );
              return;
            }
            auto next_qualifier = &elem_next_qualifier->value;
            switch( next_qualifier->type ) {
              case typedecl_qualifier_type_t::star: {
                // TODO: better messaging/location.
                ErrorCStr( ctx, entry->ident.literal, "only arrays and pointers to arrays can be array-indexed!" );
                return;
              } break;
              case typedecl_qualifier_type_t::array: {
                // we're array-accessing a pointer to an array.
                if( entry->expr_arrayidxs.len != next_qualifier->arrayidxs.len ) {
                  // TODO: better messaging/location.
                  ErrorCStr( ctx, entry->ident.literal, "passing the wrong number of array indices!" );
                  return;
                }
                tslice_t<var_t*> var_arrayidxs = {};
                _TypeArrayIdxs( ctx, tcode, entry, &var_arrayidxs );  IER
                auto var_next = AddBackList( ctx, &function->tvars );
                var_next->type = var_entry->type;
                var_next->type.qualifiers = CopyList( ctx, &var_entry->type.qualifiers );
                RemFirst( var_next->type.qualifiers ); // strip pointer
                RemFirst( var_next->type.qualifiers ); // strip array
                auto tc = AddBackList( ctx, tcode );
                tc->type = tc_type_t::array_elem_from_parray;
                tc->arrayidx.var_array = var_entry; // pointer to array
                tc->arrayidx.var_arrayidxs = var_arrayidxs;
                tc->arrayidx.var_result = var_next; // elem value, which is a pointer itself.
                var_entry = var_next;
              } break;
              default: UnreachableCrash();
            }
          } break;
          case typedecl_qualifier_type_t::array: {
            if( entry->expr_arrayidxs.len != first_qualifier->arrayidxs.len ) {
              // TODO: better messaging/location.
              ErrorCStr( ctx, entry->ident.literal, "passing the wrong number of array indices!" );
              return;
            }
            tslice_t<var_t*> var_arrayidxs = {};
            _TypeArrayIdxs( ctx, tcode, entry, &var_arrayidxs );  IER
            auto var_next = AddBackList( ctx, &function->tvars );
            var_next->type = var_entry->type;
            var_next->type.qualifiers = CopyList( ctx, &var_entry->type.qualifiers );
            RemFirst( var_next->type.qualifiers ); // strip the array first_qualifier since we did an arrayidx.
            auto tc = AddBackList( ctx, tcode );
            tc->type = tc_type_t::array_elem_from_array;
            tc->arrayidx.var_array = var_entry;
            tc->arrayidx.var_arrayidxs = var_arrayidxs;
            tc->arrayidx.var_result = var_next;
            var_entry = var_next;
          } break;
          default: UnreachableCrash();
        }
      }
      if( !last_in_dotchain ) {
        // we're dot-accessing the type entry_type
        // see if it's a struct, or pointer-to-struct
        bool can_dot_access = 0;
        bool pointer_to_struct = 0;
        if( var_entry->type.type == type_type_t::struct_ ) {
          if( !var_entry->type.qualifiers.len ) {
            can_dot_access = 1;
          }
          elif( var_entry->type.qualifiers.len == 1 ) {
            auto elem_first_qualifier = var_entry->type.qualifiers.first;
            auto first_qualifier = &elem_first_qualifier->value;
            if( first_qualifier->type == typedecl_qualifier_type_t::star ) {
              can_dot_access = 1;
              pointer_to_struct = 1;
            }
          }
        }
        if( !can_dot_access ) {
          ErrorCStr( ctx, entry->ident.literal, "only enum types, structs, pointers to structs can be dot-accessed!" );
          return;
        }
        auto structdecltype = var_entry->type.structdecltype;
        // verify the next ident is a struct field, and then allow further dots/arrayidxs.
        elem = elem->next;
        entry = &elem->value;
        namedtype_t* namedtype_field = 0;
        idx_t offset_into_list = 0;
        FindEntryByIdent( &structdecltype->fields, &entry->ident, &found, &namedtype_field, &offset_into_list );
        if( !found ) {
          // TODO: better messaging/location.
          ErrorCStr( ctx, entry->ident.literal, "field doesn't exist under this struct!" );
          return;
        }
        auto var_field = AddBackList( ctx, &function->tvars );
        var_field->type = namedtype_field->type;
        auto tc = AddBackList( ctx, tcode );
        tc->type =
          pointer_to_struct  ?
          tc_type_t::struct_field_from_pstruct  :
          tc_type_t::struct_field_from_struct
          ;
        tc->fieldaccess.var_struct = var_entry;
        tc->fieldaccess.field_offset = offset_into_list;
        tc->fieldaccess.field_name = namedtype_field->ident;
        tc->fieldaccess.var_result = var_field;
        var_entry = var_field;
        continue;
      }
      elem = elem->next;
      if( !elem ) {
        break;
      }
      entry = &elem->value;
      LookupVar( ctx, &entry->ident, &found, &var_entry );
      if( !found ) {
        ErrorCStr( ctx, entry->ident.literal, "using undefined type!" );
        return;
      }
    }
    MakeRtypeFromType( ctx, rtype, &var_entry->type );
    *var = var_entry;
  }
}
Inl void
TypeExprConst(
  compilecontext_t* ctx,
  expr_t* expr,
  list_t<rtype_t>* rtypes,
  list_t<tc_t>* tcode
  )
{
  // XXXXXXXXX
  // TODO: we'll want to limit the kinds of exprs you can do, i think.
}
// some exprs can be tuples, e.g. fncall rets like '( a, b, c ) = GetABC();'
Inl void
TypeExpr(
  compilecontext_t* ctx,
  list_t<tc_t>* tcode,
  expr_t* expr,
  list_t<rtype_t>* rtypes,
  tslice_t<var_t*>* vars
  )
{
  auto globaltypes = ctx->globaltypes;
  auto function = ctx->current_function;
  switch( expr->type ) {
    case expr_type_t::fncall: {
      TypeFncall( ctx, tcode, &expr->fncall, rtypes, vars );  IER
    } break;
    case expr_type_t::expr_assignable: {
      auto rtype = AddBackList( ctx, rtypes );
      var_t* var = 0;
      bool just_ident = 0;
      TypeExprAssignable( ctx, tcode, &expr->expr_assignable, rtype, &var, 0, &just_ident );  IER
      *vars = AddPagelistSlice( ctx->mem, var_t*, _SIZEOF_IDX_T, 1 );
      vars->mem[0] = var;
    } break;
    case expr_type_t::unop_addrof: {
      // what can you take the addrof?
      //   not constants, not functions, not structs, not enums.
      //   really just vars in fndefn scope, including args.
      //   and the form they can take is only expr_assignable, enforced by parse.
      rtype_t rtype_expr_assignable = {};
      var_t* var = 0;
      bool just_ident = 0;
      TypeExprAssignable( ctx, tcode, &expr->expr_assignable, &rtype_expr_assignable, &var, 1, &just_ident );  IER
      auto added_qualifier = AddFrontList( ctx, &rtype_expr_assignable.type.qualifiers );
      added_qualifier->type = typedecl_qualifier_type_t::star;
      auto rtype = AddBackList( ctx, rtypes );
      MakeRtypeFromType( ctx, rtype, &rtype_expr_assignable.type );
      auto var_addrof = AddBackList( ctx, &function->tvars );
      var_addrof->type = rtype_expr_assignable.type;
      auto tc = AddBackList( ctx, tcode );
      tc->type = tc_type_t::addrof;
      tc->addrof.var = var;
      tc->addrof.var_result = var_addrof;
      *vars = AddPagelistSlice( ctx->mem, var_t*, _SIZEOF_IDX_T, 1 );
      vars->mem[0] = var_addrof;
    } break;
    case expr_type_t::unop: {
      auto unop = &expr->unop;
      list_t<rtype_t> rtypes_expr = {};
      tslice_t<var_t*> vars_expr = {};
      TypeExpr( ctx, tcode, unop->expr, &rtypes_expr, &vars_expr );  IER
      if( rtypes_expr.len > 1 ) {
        // TODO: output tkn location with ErrorCStr instead
        AddBackString( &ctx->errors, "unops don't take tuples as arguments!\n" );
        return;
      }
      auto rtype_expr = &rtypes_expr.first->value;
      auto var_expr = vars_expr.mem[0];
      auto type_expr = &rtype_expr->type;
      switch( unop->type ) {
        case unoptype_t::deref: {
          if( !IsPointerType( type_expr ) ) {
            // TODO: output tkn location with ErrorCStr instead
            AddBackString( &ctx->errors, "deref expects a pointer as an argument!\n" );
            return;
          }
          auto var_deref = AddBackList( ctx, &function->tvars );
          var_deref->type = var_expr->type;
          RemFirst( var_deref->type.qualifiers ); // strip the pointer first_qualifier since we did a deref.
          auto tc = AddBackList( ctx, tcode );
          tc->type = tc_type_t::deref;
          tc->deref.var = var_expr;
          tc->deref.var_result = var_deref;
          // re-generate autocasts since the type changed.
          auto rtype = AddBackList( ctx, rtypes );
          MakeRtypeFromType( ctx, rtype, &var_deref->type );
          *vars = AddPagelistSlice( ctx->mem, var_t*, _SIZEOF_IDX_T, 1 );
          vars->mem[0] = var_deref;
        } break;
        case unoptype_t::negate_num: {
          num_type_t numtype;
          if( !IsBasicNumberType( ctx, type_expr, &numtype ) ) {
            // TODO: output tkn location with ErrorCStr instead
            AddBackString( &ctx->errors, "unop '-' expects a number as an argument!\n" );
            return;
          }
          auto var_unop = AddBackList( ctx, &function->tvars );
          var_unop->type = var_expr->type;
          auto tc = AddBackList( ctx, tcode );
          tc->type = tc_type_t::unop;
          tc->unop.type = unoptype_t::negate_num;
          tc->unop.var = var_expr;
          tc->unop.var_result = var_unop;
          // since we allow autocasting between signed/unsigned of the same size, just keep the same type.
          // it's unclear what else we would do here; force to signed?
          auto rtype = AddBackList( ctx, rtypes );
          *rtype = *rtype_expr;
          *vars = AddPagelistSlice( ctx->mem, var_t*, _SIZEOF_IDX_T, 1 );
          vars->mem[0] = var_unop;
        } break;
        case unoptype_t::negate_bool: {
          num_type_t numtype;
          if( !IsBasicNumberType( ctx, type_expr, &numtype )  &&
              !IsBasicBoolType( ctx, type_expr ) )
          {
            // TODO: output tkn location with ErrorCStr instead
            AddBackString( &ctx->errors, "unop '!' expects a bool or number as an argument!\n" );
            return;
          }
          auto var_unop = AddBackList( ctx, &function->tvars );
          var_unop->type = var_expr->type;
          auto tc = AddBackList( ctx, tcode );
          tc->type = tc_type_t::unop;
          tc->unop.type = unoptype_t::negate_bool;
          tc->unop.var = var_expr;
          tc->unop.var_result = var_unop;
          // bool stays bool, num stays num. no typing changes from this binop.
          auto rtype = AddBackList( ctx, rtypes );
          *rtype = *rtype_expr;
          *vars = AddPagelistSlice( ctx->mem, var_t*, _SIZEOF_IDX_T, 1 );
          vars->mem[0] = var_unop;
        } break;
        default: UnreachableCrash();
      }
    } break;
    case expr_type_t::binop: {
      auto binop = &expr->binop;
      list_t<rtype_t> rtypes_l = {};
      list_t<rtype_t> rtypes_r = {};
      tslice_t<var_t*> vars_l = {};
      tslice_t<var_t*> vars_r = {};
      TypeExpr( ctx, tcode, binop->expr_l, &rtypes_l, &vars_l );  IER
      TypeExpr( ctx, tcode, binop->expr_r, &rtypes_r, &vars_r );  IER
      //
      // WARNING
      // WARNING: nearly identical code to binassign below.
      // WARNING
      //
      if( rtypes_l.len > 1  ||  rtypes_r.len > 1 ) {
        // TODO: output tkn location with ErrorCStr instead
        AddBackString( &ctx->errors, "binops don't take tuples as arguments!\n" );
        return;
      }
      auto rtype_l = &rtypes_l.first->value;
      auto rtype_r = &rtypes_r.first->value;
      auto type_l = &rtype_l->type;
      auto type_r = &rtype_r->type;
      auto var_l = vars_l.mem[0];
      auto var_r = vars_r.mem[0];
      num_type_t numtype_l;
      num_type_t numtype_r;
      auto isnum_l = IsBasicNumberType( ctx, type_l, &numtype_l );
      auto isnum_r = IsBasicNumberType( ctx, type_r, &numtype_r );
      auto signed_unsigned_mismatch =
        isnum_l  &&  isnum_r  &&
        ( ( numtype_l == num_type_t::unsigned_  &&  numtype_r == num_type_t::signed_ )  ||
          ( numtype_l == num_type_t::signed_  &&  numtype_r == num_type_t::unsigned_ ) );
      rtype_t rtype = {};
      if( TypesEqual( type_l, type_r ) ) {
        // we have to intersect, in case l/r was a literal/bool with more possibilities.
        IntersectTypelists( ctx, &rtype_l->autocasts, &rtype_r->autocasts, &rtype.autocasts );
        rtype.type = *type_l;
      }
      else {
        // try to autocast r to type_l
        // then autocast l to type_r
        // if neither, type mismatch.
        if( TypelistContainsType( &rtype_r->autocasts, type_l ) ) {
          // we have to intersect, in case l/r was a literal/bool with more possibilities.
          IntersectTypelists( ctx, &rtype_l->autocasts, &rtype_r->autocasts, &rtype.autocasts );
          rtype.type = *type_l;
          GenerateAutocast( ctx, tcode, &var_r, type_l );
        }
        elif( TypelistContainsType( &rtype_l->autocasts, type_r ) ) {
          // we have to intersect, in case l/r was a literal/bool with more possibilities.
          IntersectTypelists( ctx, &rtype_r->autocasts, &rtype_l->autocasts, &rtype.autocasts );
          rtype.type = *type_r;
          GenerateAutocast( ctx, tcode, &var_l, type_r );
        }
        else {
          // TODO: output tkn location with ErrorCStr instead
          AddBackString( &ctx->errors, "mismatched argument types passed to this binop! no autocasts worked.\n" );
          return;
        }
      }
      num_type_t numtype;
      auto isnum = IsBasicNumberType( ctx, &rtype.type, &numtype );
      auto isbool = IsBasicBoolType( ctx, &rtype.type );
      auto isptr = IsPointerType( &rtype.type );
      //
      // setup most of the returns, since it's mostly shared.
      // the only remaining things to do are:
      //   fill in rtype_binop
      //   fill in var_binop
      //
      auto rtype_binop = AddBackList( ctx, rtypes );
      auto var_binop = AddBackList( ctx, &function->tvars );
      auto tc = AddBackList( ctx, tcode );
      tc->type = tc_type_t::binop;
      tc->binop.type = binop->type;
      tc->binop.var_l = var_l;
      tc->binop.var_r = var_r;
      tc->binop.var_result = var_binop;
      *vars = AddPagelistSlice( ctx->mem, var_t*, _SIZEOF_IDX_T, 1 );
      vars->mem[0] = var_binop;
      // TODO: allow arrays/slices, e.g. eqeq/noteq?
      // TODO: struct compares, which do elementwise compare?
      switch( binop->type ) {
        // expects numbers
        // ret number
        // signed/unsigned mismatch isn't allowed, since it's ambiguous which to pick.
        // and the two have different results, so.
        case binoptype_t::pow_:
        case binoptype_t::div:
        case binoptype_t::mod: {
          if( !isnum ) {
            // TODO: output tkn location with ErrorCStr instead
            AddBackString( &ctx->errors, "this binop expects numbers as arguments!\n" );
            return;
          }
          if( signed_unsigned_mismatch ) {
            // TODO: output tkn location with ErrorCStr instead
            AddBackString( &ctx->errors, "signed/unsigned ambiguity in the arguments! this binop has different results with 2 unsigned vs. 2 signed arguments.\n" );
            return;
          }
          *rtype_binop = rtype;
          var_binop->type = rtype.type;
        } break;

        // expects numbers
        // ret number
        // signed/unsigned ambiguity is fine, because the result is bitwise identical no matter what choice we make.
        case binoptype_t::mul:
        case binoptype_t::add:
        case binoptype_t::sub: {
          if( !isnum ) {
            // TODO: output tkn location with ErrorCStr instead
            AddBackString( &ctx->errors, "this binop expects numbers as arguments!\n" );
            return;
          }
          *rtype_binop = rtype;
          var_binop->type = rtype.type;
        } break;

        // expects numbers, bools
        // ret bool
        // note these aren't bitwise operators; they're boolean operators.
        // so there's no ambiguity concerns here.
        case binoptype_t::and_:
        case binoptype_t::or_: {
          if( !isnum  &&  !isbool ) {
            // TODO: output tkn location with ErrorCStr instead
            AddBackString( &ctx->errors, "this binop expects numbers or bools as arguments!\n" );
            return;
          }
          *rtype_binop = globaltypes->rtype_bool;
          var_binop->type = globaltypes->rtype_bool.type;
        } break;

        // expects numbers, bools, pointers
        // ret bool
        case binoptype_t::eqeq:
        case binoptype_t::noteq: {
          if( !isnum  &&  !isbool  &&  !isptr ) {
            // TODO: output tkn location with ErrorCStr instead
            AddBackString( &ctx->errors, "this binop expects numbers, bools, or pointers as arguments!\n" );
            return;
          }
          *rtype_binop = globaltypes->rtype_bool;
          var_binop->type = globaltypes->rtype_bool.type;
        } break;

        // expects numbers
        // TODO: allow pointers here?
        // ret bool
        // signed/unsigned mismatch isn't allowed, since it's ambiguous which to pick.
        // and the two have different results, so.
        case binoptype_t::gt:
        case binoptype_t::gteq:
        case binoptype_t::lt:
        case binoptype_t::lteq: {
          if( !isnum ) {
            // TODO: output tkn location with ErrorCStr instead
            AddBackString( &ctx->errors, "this binop expects numbers as arguments!\n" );
            return;
          }
          if( signed_unsigned_mismatch ) {
            // TODO: output tkn location with ErrorCStr instead
            AddBackString( &ctx->errors, "signed/unsigned ambiguity in the arguments! this binop has different results with 2 unsigned vs. 2 signed arguments.\n" );
            return;
          }
          *rtype_binop = globaltypes->rtype_bool;
          var_binop->type = globaltypes->rtype_bool.type;
        } break;

        default: UnreachableCrash();
      }
    } break;
    case expr_type_t::num: {
      auto bytecount = expr->num.bytecount;
      auto rtype = AddBackList( ctx, rtypes );
      void* mem = 0;
      switch( expr->num.type ) {
        case num_type_t::unsigned_: {
          mem = &expr->num.value_u64; // TODO: copy to a constants section?
          switch( bytecount ) {
            case 1: rtype->type = globaltypes->type_u8 ;  break;
            case 2: rtype->type = globaltypes->type_u16;  break;
            case 4: rtype->type = globaltypes->type_u32;  break;
            case 8: rtype->type = globaltypes->type_u64;  break;
            default: UnreachableCrash();
          }
          switch( bytecount ) {
            case 1: ADDAUTOCAST( type_s8  );  ADDAUTOCAST( type_u16 );
            case 2: ADDAUTOCAST( type_s16 );  ADDAUTOCAST( type_u32 );
            case 4: ADDAUTOCAST( type_s32 );  ADDAUTOCAST( type_u64 );
            case 8: ADDAUTOCAST( type_s64 );  break;
            default: UnreachableCrash();
          }
        } break;
        case num_type_t::signed_: {
          mem = &expr->num.value_s64; // TODO: copy to a constants section?
          switch( bytecount ) {
            case 1: rtype->type = globaltypes->type_s8 ;  break;
            case 2: rtype->type = globaltypes->type_s16;  break;
            case 4: rtype->type = globaltypes->type_s32;  break;
            case 8: rtype->type = globaltypes->type_s64;  break;
            default: UnreachableCrash();
          }
          switch( bytecount ) {
            case 1: ADDAUTOCAST( type_u8  );  ADDAUTOCAST( type_s16 );
            case 2: ADDAUTOCAST( type_u16 );  ADDAUTOCAST( type_s32 );
            case 4: ADDAUTOCAST( type_u32 );  ADDAUTOCAST( type_s64 );
            case 8: ADDAUTOCAST( type_u64 );  break;
            default: UnreachableCrash();
          }
        } break;
        case num_type_t::float_: {
          switch( bytecount ) {
            case 4: {
              rtype->type = globaltypes->type_f32;
              mem = &expr->num.value_f32; // TODO: copy to a constants section?
            } break;
            case 8: {
              rtype->type = globaltypes->type_f64;
              mem = &expr->num.value_f64; // TODO: copy to a constants section?
            } break;
            default: UnreachableCrash();
          }
          switch( bytecount ) {
            case 4: ADDAUTOCAST( type_f64 );  break;
            case 8: break;
            default: UnreachableCrash();
          }
        } break;
        default: UnreachableCrash();
      }
      auto var_num = AddBackList( ctx, &function->tvars );
      var_num->type = rtype->type;
      auto tc = AddBackList( ctx, tcode );
      tc->type = tc_type_t::loadconstant;
      tc->loadconstant.var_l = var_num;
      tc->loadconstant.mem = mem;
      *vars = AddPagelistSlice( ctx->mem, var_t*, _SIZEOF_IDX_T, 1 );
      vars->mem[0] = var_num;
    } break;
    case expr_type_t::str: {
      auto rtype = AddBackList( ctx, rtypes );
      // XXXXXXXXX
      // TODO: make this a '[N] u8'
      rtype->type = globaltypes->type_string;
      auto var_str = AddBackList( ctx, &function->tvars );
      var_str->type = globaltypes->type_string;
      auto tc = AddBackList( ctx, tcode );
      tc->type = tc_type_t::loadconstant;
      tc->loadconstant.var_l = var_str;
      tc->loadconstant.mem = expr->str.literal->mem; // TODO: copy to a constants section?
      *vars = AddPagelistSlice( ctx->mem, var_t*, _SIZEOF_IDX_T, 1 );
      vars->mem[0] = var_str;
    } break;
    case expr_type_t::chr: {
      // TODO: treat chars as nums too?
      auto rtype = AddBackList( ctx, rtypes );
      rtype->type = globaltypes->type_string;
      auto var_str = AddBackList( ctx, &function->tvars );
      var_str->type = globaltypes->type_string;
      auto tc = AddBackList( ctx, tcode );
      tc->type = tc_type_t::loadconstant;
      tc->loadconstant.var_l = var_str;
      tc->loadconstant.mem = expr->str.literal->mem; // TODO: copy to a constants section?
      *vars = AddPagelistSlice( ctx->mem, var_t*, _SIZEOF_IDX_T, 1 );
      vars->mem[0] = var_str;
    } break;
    default: UnreachableCrash();
  }
}
Inl u32
EvalExprConstU32(
  compilecontext_t* ctx,
  expr_t* expr
  )
{
  switch( expr->type ) {
    case expr_type_t::fncall: {
      // TODO: output tkn location with ErrorCStr instead
      AddBackString( &ctx->errors, "fncalls aren't allowed in constant expressions!\n" );
    } break;
    case expr_type_t::expr_assignable: {
      // TODO: output tkn location with ErrorCStr instead
      AddBackString( &ctx->errors, "identifiers aren't allowed in constant expressions!\n" );
      // TODO: we could limit to just idents, no dots/arrayidxs, and lookup constants here i think.
    } break;
    case expr_type_t::unop_addrof: {
      // TODO: output tkn location with ErrorCStr instead
      AddBackString( &ctx->errors, "addrof isn't allowed in constant expressions!\n" );
    } break;
    case expr_type_t::unop: {
      auto unop = &expr->unop;
      switch( unop->type ) {
        case unoptype_t::deref: {
          // TODO: output tkn location with ErrorCStr instead
          AddBackString( &ctx->errors, "deref isn't allowed in constant expressions!\n" );
        } break;
        case unoptype_t::negate_num: {
          // TODO: output tkn location with ErrorCStr instead
          AddBackString( &ctx->errors, "negating a u32 isn't allowed in constant expressions!\n" );
        } break;
        case unoptype_t::negate_bool: {
          auto r = EvalExprConstU32( ctx, unop->expr );  IER0
          return !r;
        } break;
        default: UnreachableCrash();
      }
    } break;
    case expr_type_t::binop: {
      auto binop = &expr->binop;
      auto vl = EvalExprConstU32( ctx, binop->expr_l );  IER0
      auto vr = EvalExprConstU32( ctx, binop->expr_r );  IER0
      switch( binop->type ) {
        case binoptype_t::mul:   { return vl *  vr; } break;
        case binoptype_t::div:   { return vl /  vr; } break;
        case binoptype_t::mod:   { return vl %  vr; } break;
        case binoptype_t::add:   { return vl +  vr; } break;
        case binoptype_t::sub:   { return vl -  vr; } break;
        case binoptype_t::and_:  { return vl && vr; } break;
        case binoptype_t::or_:   { return vl || vr; } break;
        case binoptype_t::pow_:  { return vl ^  vr; } break;
        case binoptype_t::eqeq:  { return vl == vr; } break;
        case binoptype_t::noteq: { return vl != vr; } break;
        case binoptype_t::gt:    { return vl >  vr; } break;
        case binoptype_t::gteq:  { return vl >= vr; } break;
        case binoptype_t::lt:    { return vl <  vr; } break;
        case binoptype_t::lteq:  { return vl <= vr; } break;
        default: UnreachableCrash();
      }
    } break;
    case expr_type_t::num: {
      if( expr->num.type != num_type_t::unsigned_ ) {
        ErrorCStr( ctx, expr->num.literal, "expected a u32, not a signed int or float!" );
        return 0;
      }
      if( expr->num.bytecount > 4 ) {
        ErrorCStr( ctx, expr->num.literal, "expected a u32, not a u64!" );
        return 0;
      }
      auto r = Cast( u32, expr->num.value_u64 );
      return r;
    } break;
    case expr_type_t::str: {
      ErrorCStr( ctx, expr->str.literal, "expected a u32, not a string!" );
    } break;
    case expr_type_t::chr: {
      // TODO: use the ascii value of the char?
      ErrorCStr( ctx, expr->chr.literal, "expected a u32, not a char!" );
    } break;
    default: UnreachableCrash();
  }
  return 0;
}
Inl void
TypeFromTypedecl(
  compilecontext_t* ctx,
  type_t* type,
  typedecl_t* typedecl
  )
{
  auto globaltypes = ctx->globaltypes;
  FORLIST( qualifier, elem, typedecl->qualifiers )
    auto _qualifier = AddBackList( ctx, &type->qualifiers );
    _qualifier->type = qualifier->type;
    switch( qualifier->type ) {
      case typedecl_qualifier_type_t::star: {
      } break;
      case typedecl_qualifier_type_t::array: {
        FORLIST( arrayidx, elem2, qualifier->arrayidxs )
          auto _arrayidx = AddBackList( ctx, &_qualifier->arrayidxs );
          _arrayidx->type = arrayidx->type;
          switch( arrayidx->type ) {
            case typedecl_arrayidx_type_t::star: {
            } break;
            case typedecl_arrayidx_type_t::expr_const: {
              _arrayidx->value = EvalExprConstU32( ctx, arrayidx->expr_const );  IER
            } break;
            default: UnreachableCrash();
          }
        }
      } break;
      default: UnreachableCrash();
    }
  }
  bool found = 0;
  enumdecltype_t* enumdecltype = 0;
  FindEntryByIdent( &globaltypes->enumdecltypes, &typedecl->ident, &found, &enumdecltype );
  if( found ) {
    type->type = type_type_t::enum_;
    type->enumdecltype = enumdecltype;
    return;
  }
  structdecltype_t* structdecltype = 0;
  FindEntryByIdent( &globaltypes->structdecltypes, &typedecl->ident, &found, &structdecltype );
  if( found ) {
    type->type = type_type_t::struct_;
    type->structdecltype = structdecltype;
    return;
  }
  if( EqualContents( typedecl->ident.text, SliceFromCStr( "string" ) ) ) {
    type->type = type_type_t::string_;
    return;
  }
  if( EqualContents( typedecl->ident.text, SliceFromCStr( "bool" ) ) ) {
    type->type = type_type_t::bool_;
    return;
  }

  #define NUMBERTYPE( _str, _numtype, _bytecount ) \
    if( EqualContents( typedecl->ident.text, SliceFromCStr( _str ) ) ) { \
      type->type = type_type_t::num; \
      type->num.type = num_type_t::_numtype; \
      type->num.bytecount = _bytecount; \
      return; \
    } \

  NUMBERTYPE( "uint", unsigned_, ctx->ptr_bytecount )
  NUMBERTYPE( "int", signed_, ctx->ptr_bytecount )
  NUMBERTYPE( "u64", unsigned_, 8 )
  NUMBERTYPE( "u32", unsigned_, 4 )
  NUMBERTYPE( "u16", unsigned_, 2 )
  NUMBERTYPE( "u8" , unsigned_, 1 )
  NUMBERTYPE( "s64", signed_, 8 )
  NUMBERTYPE( "s32", signed_, 4 )
  NUMBERTYPE( "s16", signed_, 2 )
  NUMBERTYPE( "s8" , signed_, 1 )
  NUMBERTYPE( "f64", float_, 8 )
  NUMBERTYPE( "f32", float_, 4 )

  ErrorCStr( ctx, typedecl->ident.literal, "using undefined type!" );
}
Inl void
LookForDeclConflicts(
  compilecontext_t* ctx,
  ident_t* ident
  )
{
  auto globaltypes = ctx->globaltypes;
  bool found = 0;
  var_t* var = 0;
  LookupVar( ctx, ident, &found, &var );
  if( found ) {
    // TODO better messaging, print location of shadow.
    ErrorCStr( ctx, ident->literal, "variable already exists with that name!" );
    return;
  }
  enumdecltype_t* enumdecltype = 0;
  FindEntryByIdent( &globaltypes->enumdecltypes, ident, &found, &enumdecltype );
  if( found ) {
    // TODO better messaging, print location of shadow.
    ErrorCStr( ctx, ident->literal, "enum already exists with that name!" );
    return;
  }
  structdecltype_t* structdecltype = 0;
  FindEntryByIdent( &globaltypes->structdecltypes, ident, &found, &structdecltype );
  if( found ) {
    // TODO better messaging, print location of shadow.
    ErrorCStr( ctx, ident->literal, "struct already exists with that name!" );
    return;
  }
}
Inl void
AppendDeferCode(
  compilecontext_t* ctx,
  function_t* function
  )
{
  FORLIST( tcode_defer, j, function->tcode_defers )
    auto tc = AddBackList( ctx, &function->tcode );
    *tc = *tcode_defer;
  }
}
Inl void
TypeStatement(
  compilecontext_t* ctx,
  list_t<tc_t>* tcode,
  statement_t* statement,
  tc_t* tc_jumplabel_whileloop_begin,
  tc_t* tc_jumplabel_whileloop_end
  )
{
  auto function = ctx->current_function;
  auto globaltypes = ctx->globaltypes;
  auto in_defer = ( tcode == &function->tcode_defers );
  bool in_whileloop = ( tc_jumplabel_whileloop_begin );
  AssertCrash( ctx->scopestack.len );
  auto vartable = ctx->scopestack.mem[ ctx->scopestack.len - 1 ];
  switch( statement->type ) {
    case statement_type_t::fncall: {
      list_t<rtype_t> rtype_rets = {};
      tslice_t<var_t*> var_rets = {};
      TypeFncall( ctx, tcode, &statement->fncall, &rtype_rets, &var_rets );  IER
    } break;
    case statement_type_t::ret: {
      auto ret = &statement->ret;
      if( in_defer ) {
        // we disallow rets inside a defer. at function-scope they're definitely bad.
        // e.g. what does 'Foo() int { defer { ret 0; }; ret 1; }' do?
        // at smaller scopes they're not strictly bad, i'm on the fence.
        // i'll disallow them to start, and see how that goes.
        // TODO: output tkn location with ErrorCStr instead
        AddBackString( &ctx->errors, "returning inside a defer, which isn't allowed!\n" );
        return;
      }
      // append all defer block code right before the ret.
      AppendDeferCode( ctx, function );
      if( ret->exprs.len != function->fndecltype->rets.len ) {
        // TODO: output tkn location with ErrorCStr instead
        AddBackString( &ctx->errors, "returning a different number of expressions than was declared!\n" );
        // TODO: output the fndecl list of ret types location as well.
        return;
      }
      idx_t idx = 0;
      FORLIST( expr, elem, ret->exprs )
        auto type_ret = function->fndecltype->rets.mem + idx;
        list_t<rtype_t> rtypes = {};
        tslice_t<var_t*> vars = {};
        TypeExpr( ctx, tcode, *expr, &rtypes, &vars );  IER
        if( rtypes.len > 1 ) {
          // TODO: output tkn location with ErrorCStr instead
          AddBackString( &ctx->errors, "tuples can't be part of a ret tuple!\n" );
          return;
        }
        auto rtype = &rtypes.first->value;
        auto var = vars.mem[0];
        if( !RtypeAutocastsToType( ctx, tcode, &var, rtype, type_ret ) ) {
          // TODO: output tkn location with ErrorCStr instead
          AddBackString( &ctx->errors, "expression isn't convertible to the declared return type!\n" );
          // TODO: output the fndecl ret type location as well.
        }
        auto tc = AddBackList( ctx, tcode );
        tc->type = tc_type_t::store;
        tc->store.var_l = function->var_rets_by_ptr.mem[idx];
        tc->store.var_r = var;
        idx += 1;
      }
      auto tc = AddBackList( ctx, tcode );
      tc->type = tc_type_t::ret;
    } break;
    case statement_type_t::decl: {
      auto decl = &statement->decl;
      LookForDeclConflicts( ctx, &decl->ident );  IER
      auto var = AddBackList( ctx, &function->tvars );
      TypeFromTypedecl( ctx, &var->type, &decl->typedecl );  IER
      auto namedvar = AddBackList( ctx, &vartable->namedvars );
      namedvar->ident = decl->ident;
      namedvar->var = var;
    } break;
    case statement_type_t::declassign: {
      auto declassign = &statement->declassign;
      LookForDeclConflicts( ctx, &declassign->ident );  IER
      list_t<rtype_t> rtypes = {};
      tslice_t<var_t*> vars = {};
      TypeExpr( ctx, tcode, declassign->expr, &rtypes, &vars );  IER
      if( rtypes.len > 1 ) {
        ErrorCStr( ctx, declassign->ident.literal, "tuples can't be named into a variable!" );
        return;
      }
      auto rtype = &rtypes.first->value;
      auto var = vars.mem[0];
      switch( declassign->type ) {
        case declassign_type_t::explicit_: {
          type_t type = {};
          TypeFromTypedecl( ctx, &type, declassign->typedecl );  IER
          if( !RtypeAutocastsToType( ctx, tcode, &var, rtype, &type ) ) {
            ErrorCStr( ctx, declassign->ident.literal, "expression isn't convertible to the declared type!" );
            return;
          }
        } break;
        case declassign_type_t::implicit_: {
          // TODO: should we propagate rtype_t in var_t, so we keep autocast info around?
        } break;
        default: UnreachableCrash();
      }
      auto namedvar = AddBackList( ctx, &vartable->namedvars );
      namedvar->ident = declassign->ident;
      namedvar->var = var;
    } break;
    case statement_type_t::binassign: {
      auto binassign = &statement->binassign;
      list_t<rtype_t> rtypes_r = {};
      tslice_t<var_t*> vars_r = {};
      TypeExpr( ctx, tcode, binassign->expr, &rtypes_r, &vars_r );  IER
      if( binassign->type == binassignop_t::eq ) {
        // eq is the only binassignop_t that we allow tuple exprs on the right.
        if( rtypes_r.len != binassign->expr_assignables.len ) {
          // TODO: output tkn location with ErrorCStr instead
          AddBackString( &ctx->errors, "tuples can't be part of a ret tuple!\n" );
          return;
        }
      }
      else {
        if( rtypes_r.len > 1 ) {
          // TODO: output tkn location with ErrorCStr instead
          AddBackString( &ctx->errors, "this binary assignment doesn't accept tuples! only '=' binary assignment accepts tuples.\n" );
          return;
        }
        if( binassign->expr_assignables.len > 1 ) {
          // TODO: output tkn location with ErrorCStr instead
          AddBackString( &ctx->errors, "this binary assignment doesn't accept tuples! only '=' binary assignment accepts tuples.\n" );
          return;
        }
      }
      AssertCrash( rtypes_r.len == binassign->expr_assignables.len );
      BEGIN_FORTWOLISTS( idx, rtype_r, rtypes_r, expr_assignable, binassign->expr_assignables )
        auto var_r = vars_r.mem[idx];
        rtype_t rtype_expr_assignable = {};
        var_t* var_expr_assignable = 0;
        bool just_ident = 0;
        TypeExprAssignable( ctx, tcode, expr_assignable, &rtype_expr_assignable, &var_expr_assignable, 1, &just_ident );  IER
        rtype_t* rtype_l = &rtype_expr_assignable;
        var_t* var_l = var_expr_assignable;
        switch( binassign->type ) {
          // expects numbers
          // ret number
          // signed/unsigned mismatch isn't allowed, since it's ambiguous which to pick.
          // and the two have different results, so.
          case binassignop_t::poweq:
          case binassignop_t::diveq:
          case binassignop_t::modeq: {
            // deref for the rhs.
            if( !just_ident ) {
              if( !IsPointerType( &rtype_l->type ) ) {
                // TODO: output tkn location with ErrorCStr instead
                AddBackString( &ctx->errors, "arrow assignment expects a pointer type on the left!\n" );
                return;
              }
              auto var_deref = AddBackList( ctx, &function->tvars );
              var_deref->type = var_l->type;
              var_deref->type.qualifiers = CopyList( ctx, &var_l->type.qualifiers );
              RemFirst( var_deref->type.qualifiers ); // strip pointer.
              auto tc_deref = AddBackList( ctx, tcode );
              tc_deref->type = tc_type_t::deref;
              tc_deref->deref.var = var_l;
              tc_deref->deref.var_result = var_deref;
              // re-generate autocasts since the type changed.
              *rtype_l = {};
              MakeRtypeFromType( ctx, rtype_l, &var_deref->type );
              var_l = var_deref;
            }
            auto type_l = &rtype_l->type;
            auto type_r = &rtype_r->type;
            rtype_t rtype = {};
            if( TypesEqual( type_l, type_r ) ) {
              // we have to intersect, in case l/r was a literal/bool with more possibilities.
              IntersectTypelists( ctx, &rtype_l->autocasts, &rtype_r->autocasts, &rtype.autocasts );
              rtype.type = *type_l;
            }
            else {
              // try to autocast r to type_l
              // note we don't autocast l to type_r, since we store the result in l.
              if( TypelistContainsType( &rtype_r->autocasts, type_l ) ) {
                // we have to intersect, in case l/r was a literal/bool with more possibilities.
                IntersectTypelists( ctx, &rtype_l->autocasts, &rtype_r->autocasts, &rtype.autocasts );
                rtype.type = *type_l;
                GenerateAutocast( ctx, tcode, &var_r, type_l );
              }
              else {
                // TODO: output tkn location with ErrorCStr instead
                AddBackString( &ctx->errors, "mismatched argument types passed to this binary assignment! no autocasts worked.\n" );
                AddBackString( &ctx->errors, "     left type: " );
                PrintType( &ctx->errors, type_l );
                AddBackString( &ctx->errors, "\n" );
                AddBackString( &ctx->errors, "    right type: " );
                PrintType( &ctx->errors, type_r );
                AddBackString( &ctx->errors, "\n" );
                return;
              }
            }
            num_type_t numtype;
            auto isnum = IsBasicNumberType( ctx, &rtype.type, &numtype );
            auto isbool = IsBasicBoolType( ctx, &rtype.type );
    //        auto isptr = IsPointerType( &rtype.type );
            if( !isnum ) {
              // TODO: output tkn location with ErrorCStr instead
              AddBackString( &ctx->errors, "this binary assignment expects numbers as arguments!\n" );
              return;
            }
            num_type_t numtype_l;
            num_type_t numtype_r;
            auto isnum_l = IsBasicNumberType( ctx, type_l, &numtype_l );
            auto isnum_r = IsBasicNumberType( ctx, type_r, &numtype_r );
            auto signed_unsigned_mismatch =
              isnum_l  &&  isnum_r  &&
              ( ( numtype_l == num_type_t::unsigned_  &&  numtype_r == num_type_t::signed_ )  ||
                ( numtype_l == num_type_t::signed_  &&  numtype_r == num_type_t::unsigned_ ) )
              ;
            if( signed_unsigned_mismatch ) {
              // TODO: output tkn location with ErrorCStr instead
              AddBackString( &ctx->errors, "signed/unsigned ambiguity in the arguments! this binary assignment has different results with 2 unsigned vs. 2 signed arguments.\n" );
              return;
            }
            auto tc = AddBackList( ctx, tcode );
            tc->type = tc_type_t::binop;
            tc->binop.type = BinoptypeFromBinassigntype( binassign->type );
            tc->binop.var_l = var_l;
            tc->binop.var_r = var_r;
            tc->binop.var_result = var_expr_assignable;
          } break;

          // expects numbers
          // ret number
          // signed/unsigned ambiguity is fine, because the result is bitwise identical no matter what choice we make.
          case binassignop_t::muleq:
          case binassignop_t::addeq:
          case binassignop_t::subeq: {
            // deref for the rhs.
            if( !just_ident ) {
              if( !IsPointerType( &rtype_l->type ) ) {
                // TODO: output tkn location with ErrorCStr instead
                AddBackString( &ctx->errors, "arrow assignment expects a pointer type on the left!\n" );
                return;
              }
              auto var_deref = AddBackList( ctx, &function->tvars );
              var_deref->type = var_l->type;
              var_deref->type.qualifiers = CopyList( ctx, &var_l->type.qualifiers );
              RemFirst( var_deref->type.qualifiers ); // strip pointer.
              auto tc_deref = AddBackList( ctx, tcode );
              tc_deref->type = tc_type_t::deref;
              tc_deref->deref.var = var_l;
              tc_deref->deref.var_result = var_deref;
              // re-generate autocasts since the type changed.
              *rtype_l = {};
              MakeRtypeFromType( ctx, rtype_l, &var_deref->type );
              var_l = var_deref;
            }
            auto type_l = &rtype_l->type;
            auto type_r = &rtype_r->type;
            rtype_t rtype = {};
            if( TypesEqual( type_l, type_r ) ) {
              // we have to intersect, in case l/r was a literal/bool with more possibilities.
              IntersectTypelists( ctx, &rtype_l->autocasts, &rtype_r->autocasts, &rtype.autocasts );
              rtype.type = *type_l;
            }
            else {
              // try to autocast r to type_l
              // note we don't autocast l to type_r, since we store the result in l.
              if( TypelistContainsType( &rtype_r->autocasts, type_l ) ) {
                // we have to intersect, in case l/r was a literal/bool with more possibilities.
                IntersectTypelists( ctx, &rtype_l->autocasts, &rtype_r->autocasts, &rtype.autocasts );
                rtype.type = *type_l;
                GenerateAutocast( ctx, tcode, &var_r, type_l );
              }
              else {
                // TODO: output tkn location with ErrorCStr instead
                AddBackString( &ctx->errors, "mismatched argument types passed to this binary assignment! no autocasts worked.\n" );
                AddBackString( &ctx->errors, "     left type: " );
                PrintType( &ctx->errors, type_l );
                AddBackString( &ctx->errors, "\n" );
                AddBackString( &ctx->errors, "    right type: " );
                PrintType( &ctx->errors, type_r );
                AddBackString( &ctx->errors, "\n" );
                return;
              }
            }
            num_type_t numtype;
            auto isnum = IsBasicNumberType( ctx, &rtype.type, &numtype );
            auto isbool = IsBasicBoolType( ctx, &rtype.type );
    //        auto isptr = IsPointerType( &rtype.type );
            if( !isnum ) {
              // TODO: output tkn location with ErrorCStr instead
              AddBackString( &ctx->errors, "this binary assignment expects numbers as arguments!\n" );
              return;
            }
            auto tc = AddBackList( ctx, tcode );
            tc->type = tc_type_t::binop;
            tc->binop.type = BinoptypeFromBinassigntype( binassign->type );
            tc->binop.var_l = var_l;
            tc->binop.var_r = var_r;
            tc->binop.var_result = var_expr_assignable;
          } break;

          // expects numbers, bools
          // ret bool
          // note these aren't bitwise operators; they're boolean operators.
          // so there's no ambiguity concerns here.
          case binassignop_t::andeq:
          case binassignop_t::oreq: {
            // deref for the rhs.
            if( !just_ident ) {
              if( !IsPointerType( &rtype_l->type ) ) {
                // TODO: output tkn location with ErrorCStr instead
                AddBackString( &ctx->errors, "arrow assignment expects a pointer type on the left!\n" );
                return;
              }
              auto var_deref = AddBackList( ctx, &function->tvars );
              var_deref->type = var_l->type;
              var_deref->type.qualifiers = CopyList( ctx, &var_l->type.qualifiers );
              RemFirst( var_deref->type.qualifiers ); // strip pointer.
              auto tc_deref = AddBackList( ctx, tcode );
              tc_deref->type = tc_type_t::deref;
              tc_deref->deref.var = var_l;
              tc_deref->deref.var_result = var_deref;
              // re-generate autocasts since the type changed.
              *rtype_l = {};
              MakeRtypeFromType( ctx, rtype_l, &var_deref->type );
              var_l = var_deref;
            }
            auto type_l = &rtype_l->type;
            auto type_r = &rtype_r->type;
            rtype_t rtype = {};
            if( TypesEqual( type_l, type_r ) ) {
              // we have to intersect, in case l/r was a literal/bool with more possibilities.
              IntersectTypelists( ctx, &rtype_l->autocasts, &rtype_r->autocasts, &rtype.autocasts );
              rtype.type = *type_l;
            }
            else {
              // try to autocast r to type_l
              // note we don't autocast l to type_r, since we store the result in l.
              if( TypelistContainsType( &rtype_r->autocasts, type_l ) ) {
                // we have to intersect, in case l/r was a literal/bool with more possibilities.
                IntersectTypelists( ctx, &rtype_l->autocasts, &rtype_r->autocasts, &rtype.autocasts );
                rtype.type = *type_l;
                GenerateAutocast( ctx, tcode, &var_r, type_l );
              }
              else {
                // TODO: output tkn location with ErrorCStr instead
                AddBackString( &ctx->errors, "mismatched argument types passed to this binary assignment! no autocasts worked.\n" );
                AddBackString( &ctx->errors, "     left type: " );
                PrintType( &ctx->errors, type_l );
                AddBackString( &ctx->errors, "\n" );
                AddBackString( &ctx->errors, "    right type: " );
                PrintType( &ctx->errors, type_r );
                AddBackString( &ctx->errors, "\n" );
                return;
              }
            }
            num_type_t numtype;
            auto isnum = IsBasicNumberType( ctx, &rtype.type, &numtype );
            auto isbool = IsBasicBoolType( ctx, &rtype.type );
    //        auto isptr = IsPointerType( &rtype.type );
            if( !isbool ) {
              // TODO: output tkn location with ErrorCStr instead
              AddBackString( &ctx->errors, "this binary assignment expects bools as arguments!\n" );
              return;
            }
            auto tc = AddBackList( ctx, tcode );
            tc->type = tc_type_t::binop;
            tc->binop.type = BinoptypeFromBinassigntype( binassign->type );
            tc->binop.var_l = var_l;
            tc->binop.var_r = var_r;
            tc->binop.var_result = var_expr_assignable;
          } break;

          case binassignop_t::eq: {
            // deref the lhs only for type comparison, don't emit code to actually deref.
            rtype_t tmp = *rtype_l;
            if( !just_ident ) {
              if( !IsPointerType( &rtype_l->type ) ) {
                // TODO: output tkn location with ErrorCStr instead
                AddBackString( &ctx->errors, "arrow assignment expects a pointer type on the left!\n" );
                return;
              }
              tmp.type.qualifiers = CopyList( ctx, &rtype_l->type.qualifiers );
              RemFirst( tmp.type.qualifiers ); // strip pointer.
              // NOTE: we don't emit any deref here on purpose, since the final store does that implicitly.
              // re-generate autocasts since the type changed.
              rtype_l = &tmp;
              MakeRtypeFromType( ctx, rtype_l, &tmp.type );
            }
            auto type_l = &rtype_l->type;
            auto type_r = &rtype_r->type;
            rtype_t rtype = {};
            if( TypesEqual( type_l, type_r ) ) {
              // we have to intersect, in case l/r was a literal/bool with more possibilities.
              IntersectTypelists( ctx, &rtype_l->autocasts, &rtype_r->autocasts, &rtype.autocasts );
              rtype.type = *type_l;
            }
            else {
              // try to autocast r to type_l
              // note we don't autocast l to type_r, since we store the result in l.
              if( TypelistContainsType( &rtype_r->autocasts, type_l ) ) {
                // we have to intersect, in case l/r was a literal/bool with more possibilities.
                IntersectTypelists( ctx, &rtype_l->autocasts, &rtype_r->autocasts, &rtype.autocasts );
                rtype.type = *type_l;
                GenerateAutocast( ctx, tcode, &var_r, type_l );
              }
              else {
                // TODO: output tkn location with ErrorCStr instead
                AddBackString( &ctx->errors, "mismatched argument types passed to this binary assignment! no autocasts worked.\n" );
                AddBackString( &ctx->errors, "     left type: " );
                PrintType( &ctx->errors, type_l );
                AddBackString( &ctx->errors, "\n" );
                AddBackString( &ctx->errors, "    right type: " );
                PrintType( &ctx->errors, type_r );
                AddBackString( &ctx->errors, "\n" );
                return;
              }
            }
            if( just_ident ) {
              auto tc = AddBackList( ctx, tcode );
              tc->type = tc_type_t::move;
              tc->move.var_l = var_l;
              tc->move.var_r = var_r;
            }
            else {
              auto tc = AddBackList( ctx, tcode );
              tc->type = tc_type_t::store;
              tc->move.var_l = var_l;
              tc->move.var_r = var_r;
            }
          } break;

          // foo *int;
          // foo <- 10;
          //
          // foo_t struct { bar * u8 }
          // foo foo_t;
          // foo.bar <- 10;
          case binassignop_t::arrow_l: {
            // deref, not for the rhs, but for the lhs.
            // exact same code as above, which is confusing.
            rtype_t tmp = *rtype_l;
            if( !just_ident ) {
              if( !IsPointerType( &rtype_l->type ) ) {
                // TODO: output tkn location with ErrorCStr instead
                AddBackString( &ctx->errors, "arrow assignment expects a pointer type on the left!\n" );
                return;
              }
              tmp.type.qualifiers = CopyList( ctx, &rtype_l->type.qualifiers );
              RemFirst( tmp.type.qualifiers ); // strip pointer.
              // NOTE: we don't emit any deref here on purpose, since the final store does that implicitly.
              // re-generate autocasts since the type changed.
              rtype_l = &tmp;
              MakeRtypeFromType( ctx, rtype_l, &tmp.type );
            }
            auto type_l = &rtype_l->type;
            auto type_r = &rtype_r->type;
            rtype_t rtype = {};
            if( TypesEqual( type_l, type_r ) ) {
              // we have to intersect, in case l/r was a literal/bool with more possibilities.
              IntersectTypelists( ctx, &rtype_l->autocasts, &rtype_r->autocasts, &rtype.autocasts );
              rtype.type = *type_l;
            }
            else {
              // try to autocast r to type_l
              // note we don't autocast l to type_r, since we store the result in l.
              if( TypelistContainsType( &rtype_r->autocasts, type_l ) ) {
                // we have to intersect, in case l/r was a literal/bool with more possibilities.
                IntersectTypelists( ctx, &rtype_l->autocasts, &rtype_r->autocasts, &rtype.autocasts );
                rtype.type = *type_l;
                GenerateAutocast( ctx, tcode, &var_r, type_l );
              }
              else {
                // TODO: output tkn location with ErrorCStr instead
                AddBackString( &ctx->errors, "mismatched argument types passed to this binary assignment! no autocasts worked.\n" );
                AddBackString( &ctx->errors, "     left type: " );
                PrintType( &ctx->errors, type_l );
                AddBackString( &ctx->errors, "\n" );
                AddBackString( &ctx->errors, "    right type: " );
                PrintType( &ctx->errors, type_r );
                AddBackString( &ctx->errors, "\n" );
                return;
              }
            }
            auto tc = AddBackList( ctx, tcode );
            tc->type = tc_type_t::store;
            tc->move.var_l = var_l;
            tc->move.var_r = var_r;
          } break;

          default: UnreachableCrash();
        }
      END_FORTWOLISTS
    } break;
    case statement_type_t::defer: {
      if( in_defer ) {
        // we disallow defer inside a defer.  it just doesn't make sense.
        // TODO: output tkn location with ErrorCStr instead
        AddBackString( &ctx->errors, "defer inside a defer, which isn't allowed!\n" );
        return;
      }
      TypeStatement(
        ctx,
        &function->tcode_defers,
        statement->defer.statement,
        tc_jumplabel_whileloop_begin,
        tc_jumplabel_whileloop_end
        );  IER
    } break;
    case statement_type_t::continue_: {
      if( in_defer  &&  !in_whileloop ) {
        // don't allow continuing out of a defer.
        // TODO: output tkn location with ErrorCStr instead
        AddBackString( &ctx->errors, "continue can't jump out of a defer scope!\n" );
        return;
      }
      // generate jump to top of last loop.
      auto tc = AddBackList( ctx, tcode );
      tc->type = tc_type_t::jump;
      tc->jump.jumplabel = tc_jumplabel_whileloop_begin;
    } break;

    case statement_type_t::break_: {
      if( in_defer  &&  !in_whileloop ) {
        // don't allow breaking out of a defer.
        //   e.g. 'while(foo) { defer{ break } }'
        //   but we want to allow: 'defer{ while(foo) { break } }
        // TODO: output tkn location with ErrorCStr instead
        AddBackString( &ctx->errors, "break can't jump out of a defer scope!\n" );
        return;
      }
      // generate jump to bottom of last loop.
      auto tc = AddBackList( ctx, tcode );
      tc->type = tc_type_t::jump;
      tc->jump.jumplabel = tc_jumplabel_whileloop_end;
    } break;

    case statement_type_t::whileloop: {
      auto whileloop = &statement->whileloop;
      // generate a whileloop_begin label.
      tc_jumplabel_whileloop_begin = AddBackList( ctx, tcode );
      tc_jumplabel_whileloop_begin->type = tc_type_t::jumplabel;
      list_t<rtype_t> rtypes = {};
      tslice_t<var_t*> vars = {};
      TypeExpr( ctx, tcode, whileloop->cblock.expr, &rtypes, &vars );  IER
      if( rtypes.len > 1 ) {
        // TODO: output tkn location with ErrorCStr instead
        AddBackString( &ctx->errors, "tuples can't be a while-condition!\n" );
        return;
      }
      auto rtype = &rtypes.first->value;
      auto var = vars.mem[0];
      if( !RtypeAutocastsToType( ctx, tcode, &var, rtype, &globaltypes->type_bool ) ) {
        // TODO: output tkn location with ErrorCStr instead
        AddBackString( &ctx->errors, "expression isn't convertible to bool!\n" );
        // TODO: output the fndecl ret type location as well.
      }
      // generate a whileloop_end label, but don't append it until we've processed the scope.
      auto elem_tc_jumplabel_whileloop_end = AddListElem<tc_t>( ctx );
      tc_jumplabel_whileloop_end = &elem_tc_jumplabel_whileloop_end->value;
      tc_jumplabel_whileloop_end->type = tc_type_t::jumplabel;
      auto tc_jumpzero_cond = AddBackList( ctx, tcode );
      tc_jumpzero_cond->type = tc_type_t::jumpzero;
      tc_jumpzero_cond->jumpcond.var_cond = var;
      tc_jumpzero_cond->jumpcond.jump.jumplabel = tc_jumplabel_whileloop_end;
      TypeScope(
        ctx,
        tcode,
        whileloop->cblock.scope,
        tc_jumplabel_whileloop_begin,
        tc_jumplabel_whileloop_end
        ); // intentionally no IER
      InsertLast( *tcode, elem_tc_jumplabel_whileloop_end );
    } break;
    case statement_type_t::ifchain: {
      auto ifchain = &statement->ifchain;
      list_t<rtype_t> rtypes_ifcond = {};
      tslice_t<var_t*> vars_ifcond = {};
      TypeExpr( ctx, tcode, ifchain->cblock_if.expr, &rtypes_ifcond, &vars_ifcond );  IER
      if( rtypes_ifcond.len > 1 ) {
        // TODO: output tkn location with ErrorCStr instead
        AddBackString( &ctx->errors, "tuples can't be an if-condition!\n" );
        return;
      }
      auto rtype_ifcond = &rtypes_ifcond.first->value;
      auto var_ifcond = vars_ifcond.mem[0];
      if( !RtypeAutocastsToType( ctx, tcode, &var_ifcond, rtype_ifcond, &globaltypes->type_bool ) ) {
        // TODO: output tkn location with ErrorCStr instead
        AddBackString( &ctx->errors, "expression isn't convertible to bool!\n" );
        // TODO: output the fndecl ret type location as well.
      }
      //
      //   compute ifcond
      //   jumpzero ifcond LABEL_IF_END
      //   ifscope contents
      //   jump LABEL_IFCHAIN_END
      // LABEL_IF_END
      //   compute elif0cond
      //   jumpzero elif0cond LABEL_ELIF0_END
      //   elif0scope contents
      //   jump LABEL_IFCHAIN_END
      // LABEL_ELIF0_END
      //   compute elif1cond
      //   jumpzero elif1cond LABEL_ELIF1_END
      //   elif1scope contents
      //   jump LABEL_IFCHAIN_END
      // LABEL_ELIF1_END
      //   elsescope contents
      // LABEL_IFCHAIN_END
      //
      auto elem_tc_jumplabel_ifchain_end = AddListElem<tc_t>( ctx );
      auto tc_jumplabel_ifchain_end = &elem_tc_jumplabel_ifchain_end->value;
      tc_jumplabel_ifchain_end->type = tc_type_t::jumplabel;
      auto elem_tc_jumplabel_if_end = AddListElem<tc_t>( ctx );
      auto tc_jumplabel_if_end = &elem_tc_jumplabel_if_end->value;
      tc_jumplabel_if_end->type = tc_type_t::jumplabel;
      auto tc_jumpzero_ifcond = AddBackList( ctx, tcode );
      tc_jumpzero_ifcond->type = tc_type_t::jumpzero;
      tc_jumpzero_ifcond->jumpcond.var_cond = var_ifcond;
      tc_jumpzero_ifcond->jumpcond.jump.jumplabel = tc_jumplabel_if_end;
      TypeScope(
        ctx,
        tcode,
        ifchain->cblock_if.scope,
        tc_jumplabel_whileloop_begin,
        tc_jumplabel_whileloop_end
        ); // intentionally no IER
      bool ifchain_has_elifelse = ifchain->cblock_elifs.len  ||  ifchain->scope_else.present;
      if( ifchain_has_elifelse ) {
        auto tc_jump_ifchain_end = AddBackList( ctx, tcode );
        tc_jump_ifchain_end->type = tc_type_t::jump;
        tc_jump_ifchain_end->jump.jumplabel = tc_jumplabel_ifchain_end;
      }
      InsertLast( *tcode, elem_tc_jumplabel_if_end );
      FORLIST( cblock_elif, elem, ifchain->cblock_elifs )
        auto lastelif_and_noelse = ( elem == ifchain->cblock_elifs.last  &&  !ifchain->scope_else.present );
        list_t<rtype_t> rtypes_elifcond = {};
        tslice_t<var_t*> vars_elifcond = {};
        TypeExpr( ctx, tcode, cblock_elif->expr, &rtypes_elifcond, &vars_elifcond );  IER
        if( rtypes_elifcond.len > 1 ) {
          // TODO: output tkn location with ErrorCStr instead
          AddBackString( &ctx->errors, "tuples can't be an if-condition!\n" );
          return;
        }
        auto rtype_elifcond = &rtypes_elifcond.first->value;
        auto var_elifcond = vars_elifcond.mem[0];
        if( !RtypeAutocastsToType( ctx, tcode, &var_elifcond, rtype_elifcond, &globaltypes->type_bool ) ) {
          // TODO: output tkn location with ErrorCStr instead
          AddBackString( &ctx->errors, "expression isn't convertible to bool!\n" );
          // TODO: output the fndecl ret type location as well.
        }
        auto elem_tc_jumplabel_elif_end = AddListElem<tc_t>( ctx );
        auto tc_jumplabel_elif_end = &elem_tc_jumplabel_elif_end->value;
        tc_jumplabel_elif_end->type = tc_type_t::jumplabel;
        auto tc_jumpzero_elifcond = AddBackList( ctx, tcode );
        tc_jumpzero_elifcond->type = tc_type_t::jumpzero;
        tc_jumpzero_elifcond->jumpcond.var_cond = var_ifcond;
        tc_jumpzero_elifcond->jumpcond.jump.jumplabel =
          lastelif_and_noelse  ?
            tc_jumplabel_ifchain_end  :
            tc_jumplabel_elif_end;
        TypeScope(
          ctx,
          tcode,
          cblock_elif->scope,
          tc_jumplabel_whileloop_begin,
          tc_jumplabel_whileloop_end
          ); // intentionally no IER
        if( !lastelif_and_noelse ) {
          // if lastelif_and_noelse, we can just fallthrough, rather than jumping over nothing.
          auto tc_jump_ifchain_end = AddBackList( ctx, tcode );
          tc_jump_ifchain_end->type = tc_type_t::jump;
          tc_jump_ifchain_end->jump.jumplabel = tc_jumplabel_ifchain_end;
          InsertLast( *tcode, elem_tc_jumplabel_elif_end );
        }
      }
      if( ifchain->scope_else.present ) {
        TypeScope(
          ctx,
          tcode,
          ifchain->scope_else.value,
          tc_jumplabel_whileloop_begin,
          tc_jumplabel_whileloop_end
          ); // intentionally no IER
      }
      if( ifchain_has_elifelse ) {
        // we only need an ifchain_end label for ifchains with elif/else
        // if-only ifchains already generate the if_end label, which is sufficient.
        InsertLast( *tcode, elem_tc_jumplabel_ifchain_end );
      }
    } break;
    case statement_type_t::scope: {
      TypeScope(
        ctx,
        tcode,
        statement->scope,
        tc_jumplabel_whileloop_begin,
        tc_jumplabel_whileloop_end
        ); // intentionally no IER
    } break;
    default: UnreachableCrash();
  }
}
Inl void
TypeScope(
  compilecontext_t* ctx,
  list_t<tc_t>* tcode,
  scope_t* scope,
  tc_t* tc_jumplabel_whileloop_begin,
  tc_t* tc_jumplabel_whileloop_end
  )
{
  auto function = ctx->current_function;
  bool in_whileloop = ( tc_jumplabel_whileloop_begin );
  if( !ctx->scopestack.len ) {
    *AddBack( ctx->scopestack ) = &function->vartable;
  }
  else {
    auto vartable_parent = ctx->scopestack.mem[ ctx->scopestack.len - 1 ];
    auto vartable = AddBackList( ctx, &vartable_parent->vartables );
#ifdef _DEBUG
    vartable->scope = scope;
#endif
    *AddBack( ctx->scopestack ) = vartable;
  }
  FORLIST( statement, elem, scope->statements )
    if( elem == scope->statements.last  &&  statement->type == statement_type_t::continue_ ) {
      // 'continue' ending a whileloop's scope doesn't need any typing.
      // we generate the jump back to loop start below, and we don't want a final 'continue'
      //   generating a duplicate.
    }
    else {
      TypeStatement(
        ctx,
        tcode,
        statement,
        tc_jumplabel_whileloop_begin,
        tc_jumplabel_whileloop_end
        ); // intentionally no IER
    }
  }
  if( in_whileloop ) {
    // generate jump to top of last loop.
    auto tc = AddBackList( ctx, tcode );
    tc->type = tc_type_t::jump;
    tc->jump.jumplabel = tc_jumplabel_whileloop_begin;
  }
  RemBack( ctx->scopestack );
}


Inl void
AddFndecltype(
  compilecontext_t* ctx,
  fndecl_t* fndecl
  )
{
  auto globaltypes = ctx->globaltypes;
  bool already_there = 0;
  fndecltype_t* fndecltype = 0;
  FindEntryByIdent( &globaltypes->fndecltypes, &fndecl->ident, &already_there, &fndecltype );
  if( !already_there ) {
    fndecltype = AddBackList( ctx, &globaltypes->fndecltypes );
    fndecltype->ident = fndecl->ident;
    fndecltype->args = AddPagelistSlice( ctx->mem, namedtype_t, _SIZEOF_IDX_T, fndecl->decl_args.len );
    ZeroContents( fndecltype->args );
    idx_t idx = 0;
    FORLIST( decl_arg, elem, fndecl->decl_args )
      auto arg = fndecltype->args.mem + idx++;
      arg->ident = decl_arg->ident;
      TypeFromTypedecl( ctx, &arg->type, &decl_arg->typedecl );  IER
    }
    fndecltype->rets = AddPagelistSlice( ctx->mem, type_t, _SIZEOF_IDX_T, fndecl->typedecl_rets.len );
    ZeroContents( fndecltype->rets );
    idx = 0;
    FORLIST( typedecl_ret, elem, fndecl->typedecl_rets )
      auto ret = fndecltype->rets.mem + idx++;
      TypeFromTypedecl( ctx, ret, typedecl_ret );  IER
    }
  }
}
Inl u32
BytecountType(
  compilecontext_t* ctx,
  type_t* type
  )
{
  // TODO: cache this, since the struct sizing is nontrivial.
  // probably do a global list of structdecltype_t*, and then size all at once.
  if( type->qualifiers.len ) {
    auto first_qualifier = &type->qualifiers.first->value;
    switch( first_qualifier->type ) {
      case typedecl_qualifier_type_t::star:  return ctx->ptr_bytecount;
      case typedecl_qualifier_type_t::array:  return ctx->array_bytecount;
      default: UnreachableCrash(); return 0;
    }
  }
  switch( type->type ) {
    case type_type_t::num:  return type->num.bytecount;
    case type_type_t::bool_:  return 1;
    case type_type_t::string_:  return ctx->array_bytecount;
    case type_type_t::enum_:  return 4;
    case type_type_t::struct_: {
      u32 r = 0;
      FORLEN( namedtype_field, j, type->structdecltype->fields )
        auto bytecount_field = BytecountType( ctx, &namedtype_field->type );
        // TODO: field alignment
        r += bytecount_field;
      }
      return r;
    } break;
    default: UnreachableCrash(); return 0;
  }
}

NoInl void
TypeGlobalScope(
  compilecontext_t* ctx,
  global_scope_t* global_scope
  )
{
  auto globaltypes = ctx->globaltypes;
  // process enums first, since they can be used in structs/fndecls.
  FORLIST( global_statement, elem_global_statement, global_scope->global_statements )
    if( global_statement->type != global_statement_type_t::enumdecl ) {
      continue;
    }
    auto enumdecl = &global_statement->enumdecl;
    bool already_there = 0;
    enumdecltype_t* enumdecltype = 0;
    FindEntryByIdent( &globaltypes->enumdecltypes, &enumdecl->ident, &already_there, &enumdecltype );
    if( already_there ) {
      // TODO: display both enum locations
      ErrorCStr( ctx, enumdecl->ident.literal, "there's already an enum using this name!" );
    }
    else {
      BEGIN_FORLISTALLPAIRS( entry0, entry1, enumdecl->enumdecl_entries )
        if( IdentsMatch( &entry0->ident, &entry1->ident ) ) {
          // TODO: display both value locations
          ErrorCStr( ctx, entry1->ident.literal, "there's already a value using this name!" );
        }
      END_FORLISTALLPAIRS
      enumdecltype = AddBackList( ctx, &globaltypes->enumdecltypes );
      enumdecltype->ident = enumdecl->ident;
      enumdecltype->type = globaltypes->type_u32;
      enumdecltype->values = AddPagelistSlice( ctx->mem, enumdecltype_value_t, _SIZEOF_IDX_T, enumdecl->enumdecl_entries.len );
      ZeroContents( enumdecltype->values );
      idx_t idx = 0;
      u32 next_value = 0;
      FORLIST( entry, elem, enumdecl->enumdecl_entries )
        auto value = enumdecltype->values.mem + idx++;
        value->ident = entry->ident;
        switch( entry->type ) {
          case enumdecl_entry_type_t::ident: {
            value->value = next_value;
            next_value += 1;
          } break;
          case enumdecl_entry_type_t::identassign: {
            auto val = EvalExprConstU32( ctx, entry->expr );  IER
            value->value = val;
            next_value = val + 1;
          } break;
          default: UnreachableCrash();
        }
      }
    }
  }
  // add structdecltype entries for every struct, but leave them blank for now.
  // this is so that we can do struct embedding, pass structs, etc. out of decl order.
  FORLIST( global_statement, elem_global_statement, global_scope->global_statements )
    if( global_statement->type != global_statement_type_t::structdecl ) {
      continue;
    }
    auto structdecl = &global_statement->structdecl;
    BEGIN_FORLISTALLPAIRS( decl_field0, decl_field1, structdecl->decl_fields )
      if( IdentsMatch( &decl_field0->ident, &decl_field1->ident ) ) {
        // TODO: display both field locations
        ErrorCStr( ctx, decl_field1->ident.literal, "there's already a field using this name!" );
      }
    END_FORLISTALLPAIRS
    bool already_there = 0;
    structdecltype_t* structdecltype = 0;
    FindEntryByIdent( &globaltypes->structdecltypes, &structdecl->ident, &already_there, &structdecltype );
    if( already_there ) {
      // TODO: display both struct locations
      ErrorCStr( ctx, structdecl->ident.literal, "there's already a struct using this name!" );
    }
    else {
      structdecltype = AddBackList( ctx, &globaltypes->structdecltypes );
      structdecltype->ident = structdecl->ident;
      // leave structdecltype->fields blank for now, we'll fill it in later, once all structdecltypes are added.
    }
  }
  FORLIST( global_statement, elem_global_statement, global_scope->global_statements )
    switch( global_statement->type ) {
      case global_statement_type_t::fndecl: {
        auto fndecl = global_statement->fndecl;
        AddFndecltype( ctx, fndecl );  IER
      } break;

      case global_statement_type_t::fndefn: {
        auto fndefn = &global_statement->fndefn;
        AddFndecltype( ctx, fndefn->fndecl );  IER
      } break;

      case global_statement_type_t::structdecl: {
        auto structdecl = &global_statement->structdecl;
        bool already_there = 0;
        structdecltype_t* structdecltype = 0;
        FindEntryByIdent( &globaltypes->structdecltypes, &structdecl->ident, &already_there, &structdecltype );
        AssertCrash( already_there );
        // now we fill in structdecltype->fields, now that all structdecltypes have been added, and can resolve to type_t.
        structdecltype->fields = AddPagelistSlice( ctx->mem, namedtype_t, _SIZEOF_IDX_T, structdecl->decl_fields.len );
        ZeroContents( structdecltype->fields );
        idx_t idx = 0;
        FORLIST( decl_field, elem, structdecl->decl_fields )
          auto field = structdecltype->fields.mem + idx++;
          field->ident = decl_field->ident;
          TypeFromTypedecl( ctx, &field->type, &decl_field->typedecl );  IER
        }
      } break;

      case global_statement_type_t::enumdecl: {
      } break;

      case global_statement_type_t::declassign_const: {
#if 0 // XXXXXXXXXXXX
        auto declassign = &global_statement->declassign_const;
        // check for conflicting decls
        bool found = 0;
        namedtype_t* namedtype_constant = 0;
        FindEntryByIdent( &globaltypes->constants, &declassign->ident, &found, &namedtype_constant );
        if( found ) {
          ErrorCStr( ctx, declassign->ident.literal, "there's already a constant using this name!" );
          return;
        }
        list_t<rtype_t> rtypes = {};
        // TODO: should we try to EvalExprConst? i think so, but how does that work w/o a target type?
        //   i guess we just return the typelist, and then verify it's len=1 for implicit declassign here?
        //   or do we do the same as other declassigns; make a var, and allow it to be autocasted?
        TypeExprConst( ctx, declassign->expr, &rtypes );  IER
        if( rtypes.len > 1 ) {
          ErrorCStr( ctx, declassign->ident.literal, "tuples can't be named into a variable!" );
          return;
        }
        auto rtype = &rtypes.first->value;
        namedtype_constant = AddBackList( ctx, &globaltypes->constants );
        namedtype_constant->ident = declassign->ident;
        switch( declassign->type ) {
          case declassign_type_t::explicit_: {
            TypeFromTypedecl( ctx, &namedtype_constant->type, declassign->typedecl );  IER
            if( !RtypeAutocastsToType( rtype, &namedtype_constant->type ) ) {
              ErrorCStr( ctx, declassign->ident.literal, "expression isn't convertible to the declared type!" );
              return;
            }
          } break;
          case declassign_type_t::implicit_: {
            // TODO: should we propagate rtype_t in var_t, so we keep autocast info around?
            namedtype_constant->type = rtype->type;
          } break;
          default: UnreachableCrash();
        }
#endif
      } break;

      default: UnreachableCrash();
    }
  }
  // now we've handled all the global scope stuff, so we can start processing the concrete fndefns.
  // we do this to eliminate the decl-before-first-use requirement of most languages.
  FORLIST( global_statement, elem_global_statement, global_scope->global_statements )
    if( global_statement->type != global_statement_type_t::fndefn ) {
      continue;
    }
    auto fndefn = &global_statement->fndefn;
    // TODO: also skip generic-arg/ret functions here.
    // if( HasGenericArgOrRet( fndefn ) ) {
    //   continue;
    // }
    auto function = AddBackList( ctx, &globaltypes->functions );
    function->fndefn = fndefn;
    bool found = 0;
    fndecltype_t* fndecltype = 0;
    FindEntryByIdent( &globaltypes->fndecltypes, &fndefn->fndecl->ident, &found, &fndecltype );
    AssertCrash( found ); // handled by the fndecl processing above.
    function->fndecltype = fndecltype;
    // allocate stack space for args.
    // also create namedvar_args, so subsequent scope typing can use those vars.
    function->namedvar_args = AddPagelistSlice( ctx->mem, namedvar_t, _SIZEOF_IDX_T, fndecltype->args.len );
    ZeroContents( function->namedvar_args );
    function->bytecount_frame = 0;
    function->bytecount_args = 0;
    FORLEN( namedtype_arg, i, fndecltype->args )
      auto bytecount_arg = BytecountType( ctx, &namedtype_arg->type );
      auto var_arg = AddBackList( ctx, &function->tvars );
      var_arg->type = namedtype_arg->type;
      var_arg->stackvarloc.offset_into_scope = function->bytecount_frame;
      var_arg->stackvarloc.bytecount = bytecount_arg;
      auto namedvar_arg = function->namedvar_args.mem + i;
      namedvar_arg->ident = namedtype_arg->ident;
      namedvar_arg->var = var_arg;
      function->bytecount_args += bytecount_arg;
      function->bytecount_frame += bytecount_arg;
    }
    // allocate stack space for rets, which we pass by pointer.
    // also keep a list of var_t*, so we can generate writes to the pointer.
    function->var_rets_by_ptr = AddPagelistSlice( ctx->mem, var_t*, _SIZEOF_IDX_T, fndecltype->rets.len );
    ZeroContents( function->var_rets_by_ptr );
    function->bytecount_rets = 0;
    FORLEN( type_ret, i, fndecltype->rets )
      auto var_ret_by_ptr = AddBackList( ctx, &function->tvars );
      var_ret_by_ptr->type = *type_ret;
      var_ret_by_ptr->type.qualifiers = CopyList( ctx, &type_ret->qualifiers );
      auto added_qualifier = AddFrontList( ctx, &var_ret_by_ptr->type.qualifiers );
      added_qualifier->type = typedecl_qualifier_type_t::star;
      auto bytecount_ret = BytecountType( ctx, &var_ret_by_ptr->type );
      var_ret_by_ptr->stackvarloc.offset_into_scope = function->bytecount_frame;
      var_ret_by_ptr->stackvarloc.bytecount = bytecount_ret;
      function->var_rets_by_ptr.mem[i] = var_ret_by_ptr;
      function->bytecount_rets += bytecount_ret;
      function->bytecount_frame += bytecount_ret;
    }
#ifdef _DEBUG
    function->vartable.scope = fndefn->scope;
#endif
    ctx->current_function = function;
    TypeScope(
      ctx,
      &function->tcode,
      fndefn->scope,
      0,
      0
      ); // intentionally no IER
    ctx->current_function = 0;
    // make sure all functions end with a ret.
    auto last_tcode_was_ret = function->tcode.len  &&  function->tcode.last->value.type == tc_type_t::ret;
    if( !last_tcode_was_ret ) {
      AppendDeferCode( ctx, function );
      auto tc = AddBackList( ctx, &function->tcode );
      tc->type = tc_type_t::ret;
    }
    function->bytecount_locals = 0;
    idx_t skip = 0;
    auto n_to_skip = fndecltype->args.len + fndecltype->rets.len;
    FORLIST( var, elem, function->tvars )
      // skip args and rets, since we already accounted for them above.
      if( skip < n_to_skip ) {
        skip += 1;
        continue;
      }
      auto bytecount_local = BytecountType( ctx, &var->type );
      var->stackvarloc.offset_into_scope = function->bytecount_frame;
      var->stackvarloc.bytecount = bytecount_local;
      function->bytecount_locals += bytecount_local;
      function->bytecount_frame += bytecount_local;
    }

    // TODO: remove this at some point. we just use the idx for printing.
    u32 idx = 0;
    FORLIST( var, elem, function->tvars )
      var->idx = idx;
      idx += 1;
    }

    // XXXXXXXXXX
    // TODO: we need to keep arg vars separate, so they don't count as locals.
    //   or is it fine, since we pass args on the stack anyways?
    // XXXXXXXXXX
    // TODO: keep ret vars separate?

    // TODO: double allocation of args here?

    // TODO: is bvar_args necessary? i don't think so...
//    function->bvar_args = AddPagelistSlice( ctx->mem, bc_var_t, _SIZEOF_IDX_T, fndecltype->args.len );
//    ZeroContents( function->bvar_args );
//    FORLEN( namedtype_arg, i, fndecltype->args )
//      auto bvar_arg = function->bvar_args.mem + i;
//      bvar_arg->offset_into_scope = top;
//      bvar_arg->bytecount = bytecount_arg;
//    }

//    // point all expressions that are returned to use the ret slot directly.
//    // this eliminates a final move from the calculated expression to the ret slot.
//    // it also means the bc_t doesn't need to do much on return.
//    FORLIST( tc, elem, function->tcode )
//      if( tc->type != tc_type_t::ret ) { // TODO: consider making a separate ret list?
//        continue;
//      }
//      FORLEN( var_ret, k, tc->ret.var_rets )
//        auto bvar_ret = function->bvar_rets.mem + k;
//        (*var_ret)->stackvarloc = *bvar_ret;
//        (*var_ret)->repointed_to_ret_slot = 1;
//      }
//    }
//    // note we remove ret expressions from the locals accounting, so we don't waste stack space.
//    // we've redirected them to all overlap at the ret slots, so we shouldn't allocate further.
//    function->bytecount_locals = 0;
//    FORLIST( var, elem, function->tvars )
//      if( var->repointed_to_ret_slot ) {
//        continue;
//      }
//      auto bytecount_local = BytecountType( ctx, &var->type );
//      var->stackvarloc.offset_into_scope = top;
//      var->stackvarloc.bytecount = bytecount_local;
//      function->bytecount_locals += bytecount_local;
//      top += bytecount_local;
//    }




  }
}






// stack-based bytecode specification.


 /* TODO: does signed int pow even make sense? negative exp -> always 0 or 1. */
  /* TODO: do float and/or even make sense? */
#define BC_BINOP_TYPES( _x ) \
  _x( add_8 ) \
  _x( add_16 ) \
  _x( add_32 ) \
  _x( add_64 ) \
  _x( sub_8 ) \
  _x( sub_16 ) \
  _x( sub_32 ) \
  _x( sub_64 ) \
  _x( mul_8 ) \
  _x( mul_16 ) \
  _x( mul_32 ) \
  _x( mul_64 ) \
  _x( and_8 ) \
  _x( and_16 ) \
  _x( and_32 ) \
  _x( and_64 ) \
  _x( or_8 ) \
  _x( or_16 ) \
  _x( or_32 ) \
  _x( or_64 ) \
  _x( eq_8 ) \
  _x( eq_16 ) \
  _x( eq_32 ) \
  _x( eq_64 ) \
  _x( noteq_8 ) \
  _x( noteq_16 ) \
  _x( noteq_32 ) \
  _x( noteq_64 ) \
  _x( shiftl_8 ) \
  _x( shiftl_16 ) \
  _x( shiftl_32 ) \
  _x( shiftl_64 ) \
  _x( shiftrzero_8 ) \
  _x( shiftrzero_16 ) \
  _x( shiftrzero_32 ) \
  _x( shiftrzero_64 ) \
  _x( shiftrsign_8 ) \
  _x( shiftrsign_16 ) \
  _x( shiftrsign_32 ) \
  _x( shiftrsign_64 ) \
  _x( div_u8 ) \
  _x( div_u16 ) \
  _x( div_u32 ) \
  _x( div_u64 ) \
  _x( div_s8 ) \
  _x( div_s16 ) \
  _x( div_s32 ) \
  _x( div_s64 ) \
  _x( mod_u8 ) \
  _x( mod_u16 ) \
  _x( mod_u32 ) \
  _x( mod_u64 ) \
  _x( mod_s8 ) \
  _x( mod_s16 ) \
  _x( mod_s32 ) \
  _x( mod_s64 ) \
  _x( pow_u8 ) \
  _x( pow_u16 ) \
  _x( pow_u32 ) \
  _x( pow_u64 ) \
  _x( pow_s8 ) \
  _x( pow_s16 ) \
  _x( pow_s32 ) \
  _x( pow_s64 ) \
  _x( gt_u8 ) \
  _x( gt_u16 ) \
  _x( gt_u32 ) \
  _x( gt_u64 ) \
  _x( gt_s8 ) \
  _x( gt_s16 ) \
  _x( gt_s32 ) \
  _x( gt_s64 ) \
  _x( gteq_u8 ) \
  _x( gteq_u16 ) \
  _x( gteq_u32 ) \
  _x( gteq_u64 ) \
  _x( gteq_s8 ) \
  _x( gteq_s16 ) \
  _x( gteq_s32 ) \
  _x( gteq_s64 ) \
  _x( lt_u8 ) \
  _x( lt_u16 ) \
  _x( lt_u32 ) \
  _x( lt_u64 ) \
  _x( lt_s8 ) \
  _x( lt_s16 ) \
  _x( lt_s32 ) \
  _x( lt_s64 ) \
  _x( lteq_u8 ) \
  _x( lteq_u16 ) \
  _x( lteq_u32 ) \
  _x( lteq_u64 ) \
  _x( lteq_s8 ) \
  _x( lteq_s16 ) \
  _x( lteq_s32 ) \
  _x( lteq_s64 ) \
  _x( add_f32 ) \
  _x( add_f64 ) \
  _x( sub_f32 ) \
  _x( sub_f64 ) \
  _x( mul_f32 ) \
  _x( mul_f64 ) \
  _x( div_f32 ) \
  _x( div_f64 ) \
  _x( mod_f32 ) \
  _x( mod_f64 ) \
  _x( pow_f32 ) \
  _x( pow_f64 ) \
  _x( and_f32 ) \
  _x( and_f64 ) \
  _x( or_f32 ) \
  _x( or_f64 ) \
  _x( eq_f32 ) \
  _x( eq_f64 ) \
  _x( noteq_f32 ) \
  _x( noteq_f64 ) \
  _x( gt_f32 ) \
  _x( gt_f64 ) \
  _x( gteq_f32 ) \
  _x( gteq_f64 ) \
  _x( lt_f32 ) \
  _x( lt_f64 ) \
  _x( lteq_f32 ) \
  _x( lteq_f64 ) \

Enumc( bc_binop_type_t ) {
  #define CASE( x )   x,
  BC_BINOP_TYPES( CASE )
  #undef CASE
};
Inl slice_t
StringFromBcbinoptype( bc_binop_type_t type )
{
  switch( type ) {
    #define CASE( x )   case bc_binop_type_t::x: return SliceFromCStr( # x );
    BC_BINOP_TYPES( CASE )
    #undef CASE
    default: UnreachableCrash(); return {};
  }
}
Enumc( bc_unop_type_t ) {
  // untyped
  negate_8, // flips all the bits.
  negate_16,
  negate_32,
  negate_64,
  negateint_8, // as a 2s complement int, returns ~x + 1
  negateint_16,
  negateint_32,
  negateint_64,
  negatefloat_8,
  negatefloat_16,
  negatefloat_32,
  negatefloat_64,
  extendzero_8_16,
  extendzero_8_32,
  extendzero_8_64,
  extendzero_16_32,
  extendzero_16_64,
  extendzero_32_64,
  extendsign_8_16,
  extendsign_8_32,
  extendsign_8_64,
  extendsign_16_32,
  extendsign_16_64,
  extendsign_32_64,
};


struct
bc_t;

struct
bc_fncall_t
{
  idx_t target_fn_bc_start; // TODO: change to pointer?
  tslice_t<bc_var_t> caller_loc_args; // positions are relative to the caller frame.
  tslice_t<bc_var_t> caller_loc_rets; // positions are relative to the caller frame.
  u32 bytecount_locals;
};
struct bc_move_t { bc_var_t var_l;  bc_var_t var_r; };
struct bc_loadconstant_t { bc_var_t var_l;  void* mem; };
struct bc_store_t { bc_var_t var_l;  bc_var_t var_r; }; // note var_l is an address, var_r is the data.
struct bc_jump_t { idx_t target; };
struct bc_jumpcond_t { bc_var_t var_cond;  bc_jump_t jump; };

// note that bool result binops return u8 for now
struct bc_binop_t { bc_binop_type_t type;  bc_var_t var_l;  bc_var_t var_r;  bc_var_t var_result; };
struct bc_unop_t { bc_unop_type_t type;  bc_var_t var;  bc_var_t var_result; };

struct bc_assertvalue_t { bc_var_t var_l;  void* mem; };

// memory ops of stackvars:
// stackvar <- UNOP stackvar
// stackvar <- stackvar BINOP stackvar
// stackvar <- read_stackvar_from_memory( memory )
// write_stackvar_to_memory( memory, stackvar )

Enumc( bc_type_t ) {
  fncall,
  move,
  loadconstant,
  store,
  ret,
  jump,
  jumpnotzero,
  jumpzero,
  binop,
  unop,
  assertvalue,
};
struct
bc_t
{
  bc_type_t type;
  union {
    bc_fncall_t fncall;
    bc_move_t move;
    bc_loadconstant_t loadconstant;
    bc_store_t store;
    bc_jump_t jump;
    bc_jumpcond_t jumpcond;
    bc_binop_t binop;
    bc_unop_t unop;
    bc_assertvalue_t assertvalue;
  };
};

Inl void
PrintStackvarloc(
  stack_resizeable_cont_t<u8>* out,
  bc_var_t* loc
  )
{
  AddBackString( out, "{" );
  PrintS64( out, loc->offset_into_scope );
  AddBackString( out, "," );
  PrintU64( out, loc->bytecount );
  AddBackString( out, "}" );
}
NoInl void
PrintCode(
  stack_resizeable_cont_t<u8>* out,
  tslice_t<bc_t>* code,
  idx_t entry_point
  )
{
  FORLEN( bc, i, *code )
    if( i == entry_point ) {
      AddBackString( out, "ENTRY_POINT:\n" );
    }
    PrintU64( out, i );
    AddBackString( out, ": " );
    switch( bc->type ) {
      case bc_type_t::fncall: {
        auto fncall = &bc->fncall;
        AddBackString( out, "fncall\n" );
        AddBackString( out, "  call_target: " );
        PrintU64( out, fncall->target_fn_bc_start );
        AddBackString( out, "\n" );
        if( fncall->caller_loc_args.len ) {
          AddBackString( out, "  caller_loc_args: ( " );
          FORLEN( caller_loc_arg, k, fncall->caller_loc_args )
            PrintStackvarloc( out, caller_loc_arg );
            if( k + 1 != fncall->caller_loc_args.len ) {
              AddBackString( out, ", " );
            }
          }
          AddBackString( out, " )" );
        }
        else {
          AddBackString( out, "  caller_loc_args: () " );
        }
        AddBackString( out, "\n" );
        if( fncall->caller_loc_rets.len ) {
          AddBackString( out, "  caller_loc_rets: ( " );
          FORLEN( caller_loc_ret, k, fncall->caller_loc_rets )
            PrintStackvarloc( out, caller_loc_ret );
            if( k + 1 != fncall->caller_loc_rets.len ) {
              AddBackString( out, ", " );
            }
          }
          AddBackString( out, " )" );
        }
        else {
          AddBackString( out, "  caller_loc_rets: () " );
        }
        AddBackString( out, "\n" );
        AddBackString( out, "  bytecount_locals: " );
        PrintU64( out, fncall->bytecount_locals );
        AddBackString( out, "\n" );
      } break;
      case bc_type_t::move: {
        AddBackString( out, "move " );
        PrintStackvarloc( out, &bc->move.var_l );
        AddBackString( out, " <- " );
        PrintStackvarloc( out, &bc->move.var_r );
        AddBackString( out, "\n" );
      } break;
      case bc_type_t::loadconstant: {
        // TODO: print the value itself?
        AddBackString( out, "loadconstant " );
        PrintStackvarloc( out, &bc->loadconstant.var_l );
        AddBackString( out, " <- " );
        PrintU64Hex( out, Cast( idx_t, bc->loadconstant.mem ) );
        AddBackString( out, "\n" );
      } break;
      case bc_type_t::store: {
        AddBackString( out, "store " );
        PrintStackvarloc( out, &bc->store.var_l );
        AddBackString( out, " <- " );
        PrintStackvarloc( out, &bc->store.var_r );
        AddBackString( out, "\n" );
      } break;
      case bc_type_t::ret: {
        AddBackString( out, "ret\n" );
      } break;
      case bc_type_t::jump: {
        AddBackString( out, "jump " );
        PrintU64( out, bc->jump.target );
        AddBackString( out, "\n" );
      } break;
      case bc_type_t::jumpnotzero: {
        AddBackString( out, "jumpnotzero " );
        PrintStackvarloc( out, &bc->jumpcond.var_cond );
        AddBackString( out, " " );
        PrintU64( out, bc->jump.target );
        AddBackString( out, "\n" );
      } break;
      case bc_type_t::jumpzero: {
        AddBackString( out, "jumpzero " );
        PrintStackvarloc( out, &bc->jumpcond.var_cond );
        AddBackString( out, " " );
        PrintU64( out, bc->jump.target );
        AddBackString( out, "\n" );
      } break;
      case bc_type_t::binop: {
        AddBackString( out, "binop " );
        PrintStackvarloc( out, &bc->binop.var_result );
        AddBackString( out, " <- " );
        AddBackString( out, StringFromBcbinoptype( bc->binop.type ) );
        AddBackString( out, "( " );
        PrintStackvarloc( out, &bc->binop.var_l );
        AddBackString( out, ", " );
        PrintStackvarloc( out, &bc->binop.var_r );
        AddBackString( out, " )" );
        AddBackString( out, "\n" );
      } break;
      case bc_type_t::unop: {
        AddBackString( out, "unop \n" );
      } break;
      case bc_type_t::assertvalue: {
        AddBackString( out, "assertvalue \n" );
      } break;
      default: UnreachableCrash();
    }
  }
}




// we don't know final locations of subsequent fncall targets until we've generated them.
// so keep a list of patches, and we'll do a post-pass to fix them all up.
// *fn_target = starting bc of fndecltype
struct
fncall_patch_t
{
  function_t* function_called;
  idx_t* target_bc_start;
};

Inl function_t*
LookupFunction(
  compilecontext_t* ctx,
  fndecltype_t* fndecltype
  )
{
  auto globaltypes = ctx->globaltypes;
  FORLIST( function, elem, globaltypes->functions )
    if( function->fndecltype == fndecltype ) {
      return function;
    }
  }
  return 0;
}
// same problem as fncalls; you can jump to labels we haven't processed yet.
// build patches up, and do a post-pass to patch things up.
struct
jump_patch_t
{
  tc_t* jumplabel;
  idx_t* bc_start;
};
NoInl void
Generate(
  compilecontext_t* ctx,
  stack_resizeable_cont_t<bc_t>* code,
  idx_t* entry_point
  )
{
  auto globaltypes = ctx->globaltypes;
  ident_t ident_main;
  MakePseudoIdent( c_sym_main, &ident_main );
  bool found = 0;
  fndecltype_t* fndecltype_main = 0;
  FindEntryByIdent( &globaltypes->fndecltypes, &ident_main, &found, &fndecltype_main );
  if( !found ) {
    AddBackString( &ctx->errors, "no entry point found! define a 'Main()' function.\n" );
    return;
  }
  list_t<fncall_patch_t> fncall_patches = {};
  auto function_main = LookupFunction( ctx, fndecltype_main );
  AssertCrash( function_main );
  list_t<function_t*> functions_queued = {};
  *AddBackList( ctx, &functions_queued ) = function_main;
  list_t<jump_patch_t> jump_patches = {};
  hashset_t jumplabels; // stores ( tc_t* jumplabel -> idx_t offset_into_code )
  Init(
    jumplabels,
    1000,
    sizeof( tc_t* ),
    sizeof( idx_t ),
    0.7f,
    Equal_FirstIdx,
    Hash_FirstIdx
    );
  while( functions_queued.len ) {
    auto function = functions_queued.first->value;
    RemFirst( functions_queued );
    // store entry point into the function, so we know where to patch fncalls to jump to.
    // functions are guaranteed to have at least one ret in them, so this is fine.
    AssertCrash( function->tcode.len );
    function->bc_start = code->len;
    function->generated = 1;
    FORLIST( tc, elem_tc, function->tcode )
      switch( tc->type ) {
        case tc_type_t::fncall: {
          auto bc = AddBack( *code );
          bc->type = bc_type_t::fncall;
          // note bc->fncall.var_args is the source location for args we're passing.
          // it's up to bc_fncall_t.fncall execution to actually copy them to the stack.
          // i.e. the locations are relative to the caller frame.
          auto args_len = tc->fncall.var_args.len;
          bc->fncall.caller_loc_args = AddPagelistSlice( ctx->mem, bc_var_t, _SIZEOF_IDX_T, args_len );
          FORLEN( caller_loc_arg, k, bc->fncall.caller_loc_args )
            auto tc_var_arg = tc->fncall.var_args.mem[k];
            // note: locations are relative to caller frame.
            *caller_loc_arg = tc_var_arg->stackvarloc;
          }
          auto function_called = LookupFunction( ctx, tc->fncall.fndecltype );
          AssertCrash( function_called ); // TODO: should this be an error?
          auto rets_len = function_called->var_rets_by_ptr.len;
          bc->fncall.caller_loc_rets = AddPagelistSlice( ctx->mem, bc_var_t, _SIZEOF_IDX_T, rets_len );
          FORLEN( caller_loc_ret, k, bc->fncall.caller_loc_rets )
            auto tc_var_ret = function_called->var_rets_by_ptr.mem[k];
            *caller_loc_ret = tc_var_ret->stackvarloc;
          }
          bc->fncall.bytecount_locals = function_called->bytecount_locals;
          bc->fncall.target_fn_bc_start = 0;
          auto fncall_patch = AddBackList( ctx, &fncall_patches );
          fncall_patch->function_called = function_called;
          fncall_patch->target_bc_start = &bc->fncall.target_fn_bc_start;
          if( !function_called->generated ) {
            *AddBackList( ctx, &functions_queued ) = function_called;
          }
        } break;
        case tc_type_t::ret: {
          auto bc = AddBack( *code );
          bc->type = bc_type_t::ret;
        } break;
        case tc_type_t::move: {
          auto bc = AddBack( *code );
          bc->type = bc_type_t::move;
          bc->move.var_l = tc->move.var_l->stackvarloc;
          bc->move.var_r = tc->move.var_r->stackvarloc;
        } break;
        case tc_type_t::loadconstant: {
          auto bc = AddBack( *code );
          bc->type = bc_type_t::loadconstant;
          bc->loadconstant.var_l = tc->loadconstant.var_l->stackvarloc;
          bc->loadconstant.mem = tc->loadconstant.mem;
        } break;
        case tc_type_t::store: {
          auto bc = AddBack( *code );
          bc->type = bc_type_t::store;
          bc->store.var_l = tc->store.var_l->stackvarloc;
          bc->store.var_r = tc->store.var_r->stackvarloc;
        } break;
        case tc_type_t::jumplabel: {
          bool already_there = 0;
          Add(
            jumplabels,
            &tc,
            &code->len,
            &already_there,
            0,
            0
            );
          AssertCrash( !already_there );
        } break;
        case tc_type_t::jump: {
          auto bc = AddBack( *code );
          bc->type = bc_type_t::jump;
          bc->jump.target = 0; // will be patched later.
          auto jump_patch = AddBackList( ctx, &jump_patches );
          jump_patch->jumplabel = tc->jump.jumplabel;
          jump_patch->bc_start = &bc->jump.target;
        } break;
        case tc_type_t::jumpnotzero: {
          auto bc = AddBack( *code );
          bc->type = bc_type_t::jumpnotzero;
          bc->jumpcond.var_cond = tc->jumpcond.var_cond->stackvarloc;
          bc->jumpcond.jump.target = 0; // will be patched later.
          auto jump_patch = AddBackList( ctx, &jump_patches );
          jump_patch->jumplabel = tc->jumpcond.jump.jumplabel;
          jump_patch->bc_start = &bc->jumpcond.jump.target;
        } break;
        case tc_type_t::jumpzero: {
          auto bc = AddBack( *code );
          bc->type = bc_type_t::jumpzero;
          bc->jumpcond.var_cond = tc->jumpcond.var_cond->stackvarloc;
          bc->jumpcond.jump.target = 0; // will be patched later.
          auto jump_patch = AddBackList( ctx, &jump_patches );
          jump_patch->jumplabel = tc->jumpcond.jump.jumplabel;
          jump_patch->bc_start = &bc->jumpcond.jump.target;
        } break;
        case tc_type_t::binop: {
          auto binop = &tc->binop;
          switch( binop->type ) {

            #define ARITH_UNTYPED( _op ) \
              AssertCrash( binop->var_l->type.type == type_type_t::num ); \
              AssertCrash( binop->var_r->type.type == type_type_t::num ); \
              AssertCrash( binop->var_result->type.type == type_type_t::num ); \
              auto bytecount = binop->var_l->type.num.bytecount; \
              AssertCrash( binop->var_r->type.num.bytecount == bytecount ); \
              AssertCrash( binop->var_result->type.num.bytecount == bytecount ); \
              AssertCrash( binop->var_l->stackvarloc.bytecount == bytecount ); \
              AssertCrash( binop->var_r->stackvarloc.bytecount == bytecount ); \
              AssertCrash( binop->var_result->stackvarloc.bytecount == bytecount ); \
              auto bc = AddBack( *code ); \
              bc->type = bc_type_t::binop; \
              bc->binop.var_l = binop->var_l->stackvarloc; \
              bc->binop.var_r = binop->var_r->stackvarloc; \
              bc->binop.var_result = binop->var_result->stackvarloc; \
              bool isfloat_l = binop->var_l->type.num.type == num_type_t::float_; \
              bool isfloat_r = binop->var_r->type.num.type == num_type_t::float_; \
              bool isfloat_result = binop->var_result->type.num.type == num_type_t::float_; \
              AssertCrash( isfloat_l == isfloat_r ); \
              AssertCrash( isfloat_l == isfloat_result ); \
              if( isfloat_l ) { \
                switch( bytecount ) { \
                  case 4: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _f32 ); break; \
                  case 8: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _f64 ); break; \
                  default: UnreachableCrash(); \
                } \
              } \
              else { \
                switch( bytecount ) { \
                  case 1: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _8  ); break; \
                  case 2: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _16 ); break; \
                  case 4: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _32 ); break; \
                  case 8: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _64 ); break; \
                  default: UnreachableCrash(); \
                } \
              } \

            case binoptype_t::mul: { ARITH_UNTYPED( mul ) } break;
            case binoptype_t::add: { ARITH_UNTYPED( add ) } break;
            case binoptype_t::sub: { ARITH_UNTYPED( sub ) } break;

            #define BOOL_UNTYPED( _op ) \
              AssertCrash( binop->var_l->type.type == type_type_t::num ); \
              AssertCrash( binop->var_r->type.type == type_type_t::num ); \
              AssertCrash( binop->var_result->type.type == type_type_t::bool_ ); \
              auto bytecount = binop->var_l->type.num.bytecount; \
              AssertCrash( binop->var_r->type.num.bytecount == bytecount ); \
              AssertCrash( binop->var_l->stackvarloc.bytecount == bytecount ); \
              AssertCrash( binop->var_r->stackvarloc.bytecount == bytecount ); \
              AssertCrash( binop->var_result->stackvarloc.bytecount == 1 ); \
              auto bc = AddBack( *code ); \
              bc->type = bc_type_t::binop; \
              bc->binop.var_l = binop->var_l->stackvarloc; \
              bc->binop.var_r = binop->var_r->stackvarloc; \
              bc->binop.var_result = binop->var_result->stackvarloc; \
              bool isfloat_l = binop->var_l->type.num.type == num_type_t::float_; \
              bool isfloat_r = binop->var_r->type.num.type == num_type_t::float_; \
              bool isfloat_result = binop->var_result->type.num.type == num_type_t::float_; \
              AssertCrash( isfloat_l == isfloat_r ); \
              AssertCrash( !isfloat_result ); \
              if( isfloat_l ) { \
                switch( bytecount ) { \
                  case 4: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _f32 ); break; \
                  case 8: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _f64 ); break; \
                  default: UnreachableCrash(); \
                } \
              } \
              else { \
                switch( bytecount ) { \
                  case 1: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _8  ); break; \
                  case 2: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _16 ); break; \
                  case 4: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _32 ); break; \
                  case 8: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _64 ); break; \
                  default: UnreachableCrash(); \
                } \
              } \

            // TODO: does float AND/OR make sense?
            case binoptype_t::and_:  { BOOL_UNTYPED( and ) } break;
            case binoptype_t::or_:   { BOOL_UNTYPED( or ) } break;
            case binoptype_t::eqeq:  { BOOL_UNTYPED( eq ) } break;
            case binoptype_t::noteq: { BOOL_UNTYPED( noteq ) } break;

            #define ARITH_TYPED( _op ) \
              AssertCrash( binop->var_l->type.type == type_type_t::num ); \
              AssertCrash( binop->var_r->type.type == type_type_t::num ); \
              AssertCrash( binop->var_result->type.type == type_type_t::num ); \
              auto bytecount = binop->var_l->type.num.bytecount; \
              AssertCrash( binop->var_r->type.num.bytecount == bytecount ); \
              AssertCrash( binop->var_result->type.num.bytecount == bytecount ); \
              AssertCrash( binop->var_l->stackvarloc.bytecount == bytecount ); \
              AssertCrash( binop->var_r->stackvarloc.bytecount == bytecount ); \
              AssertCrash( binop->var_result->stackvarloc.bytecount == bytecount ); \
              auto bc = AddBack( *code ); \
              bc->type = bc_type_t::binop; \
              bc->binop.var_l = binop->var_l->stackvarloc; \
              bc->binop.var_r = binop->var_r->stackvarloc; \
              bc->binop.var_result = binop->var_result->stackvarloc; \
              bool isfloat_l = binop->var_l->type.num.type == num_type_t::float_; \
              bool isfloat_r = binop->var_r->type.num.type == num_type_t::float_; \
              bool isfloat_result = binop->var_result->type.num.type == num_type_t::float_; \
              bool issigned_l = binop->var_l->type.num.type == num_type_t::signed_; \
              bool issigned_r = binop->var_r->type.num.type == num_type_t::signed_; \
              bool issigned_result = binop->var_result->type.num.type == num_type_t::signed_; \
              AssertCrash( isfloat_l == isfloat_r ); \
              AssertCrash( isfloat_l == isfloat_result ); \
              AssertCrash( issigned_l == issigned_r ); \
              AssertCrash( issigned_l == issigned_result ); \
              if( isfloat_l ) { \
                switch( bytecount ) { \
                  case 4: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _f32 ); break; \
                  case 8: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _f64 ); break; \
                  default: UnreachableCrash(); \
                } \
              } \
              elif( issigned_l ) { \
                switch( bytecount ) { \
                  case 1: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _s8  ); break; \
                  case 2: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _s16 ); break; \
                  case 4: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _s32 ); break; \
                  case 8: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _s64 ); break; \
                  default: UnreachableCrash(); \
                } \
              } \
              else { \
                switch( bytecount ) { \
                  case 1: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _u8  ); break; \
                  case 2: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _u16 ); break; \
                  case 4: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _u32 ); break; \
                  case 8: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _u64 ); break; \
                  default: UnreachableCrash(); \
                } \
              } \

            case binoptype_t::div:  { ARITH_TYPED( div ) } break;
            case binoptype_t::mod:  { ARITH_TYPED( mod ) } break;
            case binoptype_t::pow_: { ARITH_TYPED( pow ) } break;

            #define BOOL_TYPED( _op ) \
              AssertCrash( binop->var_l->type.type == type_type_t::num ); \
              AssertCrash( binop->var_r->type.type == type_type_t::num ); \
              AssertCrash( binop->var_result->type.type == type_type_t::bool_ ); \
              auto bytecount = binop->var_l->type.num.bytecount; \
              AssertCrash( binop->var_r->type.num.bytecount == bytecount ); \
              AssertCrash( binop->var_l->stackvarloc.bytecount == bytecount ); \
              AssertCrash( binop->var_r->stackvarloc.bytecount == bytecount ); \
              AssertCrash( binop->var_result->stackvarloc.bytecount == 1 ); \
              auto bc = AddBack( *code ); \
              bc->type = bc_type_t::binop; \
              bc->binop.var_l = binop->var_l->stackvarloc; \
              bc->binop.var_r = binop->var_r->stackvarloc; \
              bc->binop.var_result = binop->var_result->stackvarloc; \
              bool isfloat_l = binop->var_l->type.num.type == num_type_t::float_; \
              bool isfloat_r = binop->var_r->type.num.type == num_type_t::float_; \
              bool isfloat_result = binop->var_result->type.num.type == num_type_t::float_; \
              bool issigned_l = binop->var_l->type.num.type == num_type_t::signed_; \
              bool issigned_r = binop->var_r->type.num.type == num_type_t::signed_; \
              bool issigned_result = binop->var_result->type.num.type == num_type_t::signed_; \
              AssertCrash( isfloat_l == isfloat_r ); \
              AssertCrash( !isfloat_result ); \
              AssertCrash( issigned_l == issigned_r ); \
              AssertCrash( !issigned_result ); \
              if( isfloat_l ) { \
                switch( bytecount ) { \
                  case 4: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _f32 ); break; \
                  case 8: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _f64 ); break; \
                  default: UnreachableCrash(); \
                } \
              } \
              elif( issigned_l ) { \
                switch( bytecount ) { \
                  case 1: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _s8  ); break; \
                  case 2: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _s16 ); break; \
                  case 4: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _s32 ); break; \
                  case 8: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _s64 ); break; \
                  default: UnreachableCrash(); \
                } \
              } \
              else { \
                switch( bytecount ) { \
                  case 1: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _u8  ); break; \
                  case 2: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _u16 ); break; \
                  case 4: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _u32 ); break; \
                  case 8: bc->binop.type = bc_binop_type_t::NAMEJOIN( _op, _u64 ); break; \
                  default: UnreachableCrash(); \
                } \
              } \

            case binoptype_t::gt:   { BOOL_TYPED( gt ) } break;
            case binoptype_t::gteq: { BOOL_TYPED( gteq ) } break;
            case binoptype_t::lt:   { BOOL_TYPED( lt ) } break;
            case binoptype_t::lteq: { BOOL_TYPED( lteq ) } break;

            default: UnreachableCrash();
          }
        } break;
        case tc_type_t::unop: {
          // XXXXXXXXXXXX

        } break;
        case tc_type_t::autocast: {
          // XXXXXXXXXXXX

        } break;

        case tc_type_t::array_elem_from_array: {
          // XXXXXXXXXXXX

        } break;
        case tc_type_t::array_elem_from_parray: {
          // XXXXXXXXXXXX

        } break;
        case tc_type_t::array_pelem_from_array: {
          // XXXXXXXXXXXX

        } break;
        case tc_type_t::array_pelem_from_parray: {
          // XXXXXXXXXXXX

        } break;
        case tc_type_t::struct_field_from_struct: {
          // XXXXXXXXXXXX

        } break;
        case tc_type_t::struct_field_from_pstruct: {
          // XXXXXXXXXXXX

        } break;
        case tc_type_t::struct_pfield_from_struct: {
          // XXXXXXXXXXXX

        } break;
        case tc_type_t::struct_pfield_from_pstruct: {
          // XXXXXXXXXXXX

        } break;
        case tc_type_t::addrof: {
          // XXXXXXXXXXXX

        } break;
        case tc_type_t::deref: {
          // XXXXXXXXXXXX

        } break;
        default: UnreachableCrash();
      }
    }
  }
  // generate entry point function call, which just calls Main.
  // we do things this way so Main can have stack vars, and potentially args/rets eventually.
  {
    *entry_point = code->len;
    auto bc = AddBack( *code );
    bc->type = bc_type_t::fncall;
    bc->fncall.target_fn_bc_start = 0;
    bc->fncall.caller_loc_args = {};
    bc->fncall.caller_loc_rets = {};
    bc->fncall.bytecount_locals = function_main->bytecount_locals;
    auto fncall_patch = AddBackList( ctx, &fncall_patches );
    fncall_patch->function_called = function_main;
    fncall_patch->target_bc_start = &bc->fncall.target_fn_bc_start;
  }
  // post-pass to patch all the fncalls, now that all functions have been processed.
  FORLIST( fncall_patch, i, fncall_patches )
    *fncall_patch->target_bc_start = fncall_patch->function_called->bc_start;
  }
  // post-pass to patch all the jumps, now that all jumplabels have been processed.
  FORLIST( jump_patch, i, jump_patches )
    found = 0;
    Lookup(
      jumplabels,
      &jump_patch->jumplabel,
      &found,
      jump_patch->bc_start
      );
    AssertCrash( found );
  }
  Kill( jumplabels );
}



u8
ipow( u8 a, u8 exp )
{
  // TODO: XXXXXXXXXXXXX
  return 0;
}
u16
ipow( u16 a, u16 exp )
{
  // TODO: XXXXXXXXXXXXX
  return 0;
}
u32
ipow( u32 a, u32 exp )
{
  // TODO: XXXXXXXXXXXXX
  return 0;
}
u64
ipow( u64 a, u64 exp )
{
  // TODO: XXXXXXXXXXXXX
  return 0;
}
s8
ipow( s8 a, s8 exp )
{
  // TODO: XXXXXXXXXXXXX
  return 0;
}
s16
ipow( s16 a, s16 exp )
{
  // TODO: XXXXXXXXXXXXX
  return 0;
}
s32
ipow( s32 a, s32 exp )
{
  // TODO: XXXXXXXXXXXXX
  return 0;
}
s64
ipow( s64 a, s64 exp )
{
  // TODO: XXXXXXXXXXXXX
  return 0;
}

Inl void
Execute( tslice_t<bc_t> code, idx_t entry_point )
{
  if( !code.len ) {
    return;
  }
  slice_t stack;
  stack.len = 1024*1024;
  stack.mem = MemHeapAlloc( u8, stack.len );
  Memzero( stack.mem, stack.len );
  slice_t frame;
  frame.mem = stack.mem;
  frame.len = 0;
  auto instruction_pointer = code.mem + entry_point;
  auto end = code.mem + code.len;
  while( instruction_pointer < end ) {
    auto c = instruction_pointer;
    switch( c->type ) {
      case bc_type_t::fncall: {
        auto ret_frame = frame;
        auto ret_instruction_pointer = instruction_pointer + 1; // return to next instr.
        frame.mem += frame.len;
        // XXXXXXXXXX
        // TODO: could avoid saving ret_frame.len i think.
        //   since we statically know the bytecounts of rets and locals for every fn,
        //   we could stop dynamically computing frame.len, and just use the fixed value.
        //   we wouldn't even have to make ret know about the fixed length, which is great.
        idx_t save[] = {
          Cast( idx_t, ret_frame.mem ),
          ret_frame.len,
          Cast( idx_t, ret_instruction_pointer ),
          };
        Memmove( frame.mem, save, _countof( save ) * sizeof( save[0] ) );
        frame.mem += _countof( save ) * sizeof( save[0] );
        frame.len = 0;
        // pass args by value.
        FORLEN( caller_loc_arg, i, c->fncall.caller_loc_args )
          auto bytecount = caller_loc_arg->bytecount;
          auto dst = frame.mem + frame.len;
          auto src = ret_frame.mem + caller_loc_arg->offset_into_scope;
          Memmove( dst, src, bytecount );
          frame.len += bytecount;
        }
        // pass rets by reference
        FORLEN( caller_loc_ret, i, c->fncall.caller_loc_rets )
          auto bytecount = caller_loc_ret->bytecount;
          AssertCrash( bytecount == sizeof( void* ) );
          auto dst = frame.mem + frame.len;
          auto src = ret_frame.mem + caller_loc_ret->offset_into_scope;
          Memmove( dst, &src, bytecount );
          frame.len += bytecount;
        }
        frame.len += c->fncall.bytecount_locals;
        instruction_pointer = code.mem + c->fncall.target_fn_bc_start;
        continue;
      }
      case bc_type_t::ret: {
        idx_t restore[3];
        frame.mem -= _countof( restore ) * sizeof( restore[0] );
        Memmove( restore, frame.mem, _countof( restore ) * sizeof( restore[0] ) );
        frame.mem = Cast( u8*, restore[0] );
        frame.len = restore[1];
        instruction_pointer = Cast( bc_t*, restore[2] );
        continue;
      }
      case bc_type_t::move: {
        AssertCrash( c->move.var_l.bytecount == c->move.var_r.bytecount );
        auto bytecount = c->move.var_l.bytecount;
        auto dst = frame.mem + c->move.var_l.offset_into_scope;
        auto src = frame.mem + c->move.var_r.offset_into_scope;
        Memmove( dst, src, bytecount );
      } break;
      case bc_type_t::loadconstant: {
        auto bytecount = c->loadconstant.var_l.bytecount;
        auto dst = frame.mem + c->loadconstant.var_l.offset_into_scope;
        u8* src = Cast( u8*, c->loadconstant.mem );
        Memmove( dst, src, bytecount );
      } break;
      case bc_type_t::store: {
        auto bytecount = c->store.var_r.bytecount;
        auto dst = Cast( u8*, *Cast( idx_t*, frame.mem + c->store.var_l.offset_into_scope ) );
        auto src = frame.mem + c->store.var_r.offset_into_scope;
        Memmove( dst, src, bytecount );
      } break;
      case bc_type_t::assertvalue: {
        auto bytecount = c->assertvalue.var_l.bytecount;
        auto dst = frame.mem + c->assertvalue.var_l.offset_into_scope;
        u8* src = Cast( u8*, c->assertvalue.mem );
        AssertCrash( MemEqual( src, dst, bytecount ) );
      } break;
      case bc_type_t::jump: {
        instruction_pointer = code.mem + c->jump.target;
        continue;
      }
      case bc_type_t::jumpnotzero: {
        slice_t value_cond;
        value_cond.mem = frame.mem + c->jumpcond.var_cond.offset_into_scope;
        value_cond.len = c->jumpcond.var_cond.bytecount;
        if( !MemIsZero( ML( value_cond ) ) ) {
          instruction_pointer = code.mem + c->jumpcond.jump.target;
          continue;
        }
      } break;
      case bc_type_t::jumpzero: {
        slice_t value_cond;
        value_cond.mem = frame.mem + c->jumpcond.var_cond.offset_into_scope;
        value_cond.len = c->jumpcond.var_cond.bytecount;
        if( MemIsZero( ML( value_cond ) ) ) {
          instruction_pointer = code.mem + c->jumpcond.jump.target;
          continue;
        }
      } break;
      case bc_type_t::unop: {
        u8*  value_u8 = frame.mem + c->unop.var.offset_into_scope;
        auto value_u16 = Cast( u16*, value_u8 );
        auto value_u32 = Cast( u32*, value_u8 );
        auto value_u64 = Cast( u64*, value_u8 );
        auto value_s8  = Cast( s8 *, value_u8 );
        auto value_s16 = Cast( s16*, value_u8 );
        auto value_s32 = Cast( s32*, value_u8 );
        auto value_s64 = Cast( s64*, value_u8 );
        u8*  dst_u8 = frame.mem + c->unop.var.offset_into_scope;
        auto dst_u16 = Cast( u16*, dst_u8 );
        auto dst_u32 = Cast( u32*, dst_u8 );
        auto dst_u64 = Cast( u64*, dst_u8 );
//        auto dst_s8  = Cast( s8 *, dst_u8 );
        auto dst_s16 = Cast( s16*, dst_u8 );
        auto dst_s32 = Cast( s32*, dst_u8 );
        auto dst_s64 = Cast( s64*, dst_u8 );
        switch( c->unop.type ) {
          case bc_unop_type_t::negate_8:  { *dst_u8  = ~*value_u8 ; } break;
          case bc_unop_type_t::negate_16: { *dst_u16 = ~*value_u16; } break;
          case bc_unop_type_t::negate_32: { *dst_u32 = ~*value_u32; } break;
          case bc_unop_type_t::negate_64: { *dst_u64 = ~*value_u64; } break;
          case bc_unop_type_t::negateint_8:  { *dst_u8  = ~*value_u8  + 1; } break;
          case bc_unop_type_t::negateint_16: { *dst_u16 = ~*value_u16 + 1; } break;
          case bc_unop_type_t::negateint_32: { *dst_u32 = ~*value_u32 + 1; } break;
          case bc_unop_type_t::negateint_64: { *dst_u64 = ~*value_u64 + 1; } break;
          case bc_unop_type_t::negatefloat_8:  { *dst_u8  = *value_u8  ^ ( 1u   <<  7 ); } break;
          case bc_unop_type_t::negatefloat_16: { *dst_u16 = *value_u16 ^ ( 1u   << 15 ); } break;
          case bc_unop_type_t::negatefloat_32: { *dst_u32 = *value_u32 ^ ( 1u   << 31 ); } break;
          case bc_unop_type_t::negatefloat_64: { *dst_u64 = *value_u64 ^ ( 1ull << 63 ); } break;
          case bc_unop_type_t::extendzero_8_16:  { *dst_u16 = *value_u8 ; } break;
          case bc_unop_type_t::extendzero_8_32:  { *dst_u32 = *value_u16; } break;
          case bc_unop_type_t::extendzero_8_64:  { *dst_u64 = *value_u8 ; } break;
          case bc_unop_type_t::extendzero_16_32: { *dst_u32 = *value_u16; } break;
          case bc_unop_type_t::extendzero_16_64: { *dst_u64 = *value_u32; } break;
          case bc_unop_type_t::extendzero_32_64: { *dst_u64 = *value_u64; } break;
          case bc_unop_type_t::extendsign_8_16:  { *dst_s16 = *value_s8 ; } break;
          case bc_unop_type_t::extendsign_8_32:  { *dst_s32 = *value_s16; } break;
          case bc_unop_type_t::extendsign_8_64:  { *dst_s64 = *value_s8 ; } break;
          case bc_unop_type_t::extendsign_16_32: { *dst_s32 = *value_s16; } break;
          case bc_unop_type_t::extendsign_16_64: { *dst_s64 = *value_s32; } break;
          case bc_unop_type_t::extendsign_32_64: { *dst_s64 = *value_s64; } break;
          default: UnreachableCrash();
        }
      } break;
      case bc_type_t::binop: {
        u8*  lvalue_u8 = frame.mem + c->binop.var_l.offset_into_scope;
        auto lvalue_u16 = Cast( u16*, lvalue_u8 );
        auto lvalue_u32 = Cast( u32*, lvalue_u8 );
        auto lvalue_u64 = Cast( u64*, lvalue_u8 );
        auto lvalue_s8  = Cast( s8 *, lvalue_u8 );
        auto lvalue_s16 = Cast( s16*, lvalue_u8 );
        auto lvalue_s32 = Cast( s32*, lvalue_u8 );
        auto lvalue_s64 = Cast( s64*, lvalue_u8 );
        auto lvalue_f32 = Cast( f32*, lvalue_u8 );
        auto lvalue_f64 = Cast( f64*, lvalue_u8 );
        u8*  rvalue_u8 = frame.mem + c->binop.var_r.offset_into_scope;
        auto rvalue_u16 = Cast( u16*, rvalue_u8 );
        auto rvalue_u32 = Cast( u32*, rvalue_u8 );
        auto rvalue_u64 = Cast( u64*, rvalue_u8 );
        auto rvalue_s8  = Cast( s8 *, rvalue_u8 );
        auto rvalue_s16 = Cast( s16*, rvalue_u8 );
        auto rvalue_s32 = Cast( s32*, rvalue_u8 );
        auto rvalue_s64 = Cast( s64*, rvalue_u8 );
        auto rvalue_f32 = Cast( f32*, rvalue_u8 );
        auto rvalue_f64 = Cast( f64*, rvalue_u8 );
        u8*  dst_u8 = frame.mem + c->binop.var_result.offset_into_scope;
        auto dst_u16 = Cast( u16*, dst_u8 );
        auto dst_u32 = Cast( u32*, dst_u8 );
        auto dst_u64 = Cast( u64*, dst_u8 );
        auto dst_s8  = Cast( s8 *, dst_u8 );
        auto dst_s16 = Cast( s16*, dst_u8 );
        auto dst_s32 = Cast( s32*, dst_u8 );
        auto dst_s64 = Cast( s64*, dst_u8 );
        auto dst_f32 = Cast( f32*, dst_u8 );
        auto dst_f64 = Cast( f64*, dst_u8 );
        switch( c->binop.type ) {
          // untyped
          case bc_binop_type_t::add_8:  { *dst_u8  = *lvalue_u8  + *rvalue_u8 ; } break;
          case bc_binop_type_t::add_16: { *dst_u16 = *lvalue_u16 + *rvalue_u16; } break;
          case bc_binop_type_t::add_32: { *dst_u32 = *lvalue_u32 + *rvalue_u32; } break;
          case bc_binop_type_t::add_64: { *dst_u64 = *lvalue_u64 + *rvalue_u64; } break;
          case bc_binop_type_t::sub_8:  { *dst_u8  = *lvalue_u8  - *rvalue_u8 ; } break;
          case bc_binop_type_t::sub_16: { *dst_u16 = *lvalue_u16 - *rvalue_u16; } break;
          case bc_binop_type_t::sub_32: { *dst_u32 = *lvalue_u32 - *rvalue_u32; } break;
          case bc_binop_type_t::sub_64: { *dst_u64 = *lvalue_u64 - *rvalue_u64; } break;
          case bc_binop_type_t::mul_8:  { *dst_u8  = *lvalue_u8  * *rvalue_u8 ; } break;
          case bc_binop_type_t::mul_16: { *dst_u16 = *lvalue_u16 * *rvalue_u16; } break;
          case bc_binop_type_t::mul_32: { *dst_u32 = *lvalue_u32 * *rvalue_u32; } break;
          case bc_binop_type_t::mul_64: { *dst_u64 = *lvalue_u64 * *rvalue_u64; } break;
          case bc_binop_type_t::and_8:  { *dst_u8  = *lvalue_u8  & *rvalue_u8 ; } break;
          case bc_binop_type_t::and_16: { *dst_u16 = *lvalue_u16 & *rvalue_u16; } break;
          case bc_binop_type_t::and_32: { *dst_u32 = *lvalue_u32 & *rvalue_u32; } break;
          case bc_binop_type_t::and_64: { *dst_u64 = *lvalue_u64 & *rvalue_u64; } break;
          case bc_binop_type_t::or_8:  { *dst_u8  = *lvalue_u8  | *rvalue_u8 ; } break;
          case bc_binop_type_t::or_16: { *dst_u16 = *lvalue_u16 | *rvalue_u16; } break;
          case bc_binop_type_t::or_32: { *dst_u32 = *lvalue_u32 | *rvalue_u32; } break;
          case bc_binop_type_t::or_64: { *dst_u64 = *lvalue_u64 | *rvalue_u64; } break;
          case bc_binop_type_t::eq_8:  { *dst_u8  = *lvalue_u8  == *rvalue_u8 ; } break;
          case bc_binop_type_t::eq_16: { *dst_u16 = *lvalue_u16 == *rvalue_u16; } break;
          case bc_binop_type_t::eq_32: { *dst_u32 = *lvalue_u32 == *rvalue_u32; } break;
          case bc_binop_type_t::eq_64: { *dst_u64 = *lvalue_u64 == *rvalue_u64; } break;
          case bc_binop_type_t::noteq_8:  { *dst_u8  = *lvalue_u8  != *rvalue_u8 ; } break;
          case bc_binop_type_t::noteq_16: { *dst_u16 = *lvalue_u16 != *rvalue_u16; } break;
          case bc_binop_type_t::noteq_32: { *dst_u32 = *lvalue_u32 != *rvalue_u32; } break;
          case bc_binop_type_t::noteq_64: { *dst_u64 = *lvalue_u64 != *rvalue_u64; } break;
          case bc_binop_type_t::shiftl_8:  { *dst_u8  = *lvalue_u8  << *rvalue_u8 ; } break;
          case bc_binop_type_t::shiftl_16: { *dst_u16 = *lvalue_u16 << *rvalue_u16; } break;
          case bc_binop_type_t::shiftl_32: { *dst_u32 = *lvalue_u32 << *rvalue_u32; } break;
          case bc_binop_type_t::shiftl_64: { *dst_u64 = *lvalue_u64 << *rvalue_u64; } break;
          case bc_binop_type_t::shiftrzero_8:  { *dst_u8  = *lvalue_u8  >> *rvalue_u8 ; } break;
          case bc_binop_type_t::shiftrzero_16: { *dst_u16 = *lvalue_u16 >> *rvalue_u16; } break;
          case bc_binop_type_t::shiftrzero_32: { *dst_u32 = *lvalue_u32 >> *rvalue_u32; } break;
          case bc_binop_type_t::shiftrzero_64: { *dst_u64 = *lvalue_u64 >> *rvalue_u64; } break;
          case bc_binop_type_t::shiftrsign_8:  { *dst_s8  = *lvalue_s8  >> *rvalue_s8 ; } break;
          case bc_binop_type_t::shiftrsign_16: { *dst_s16 = *lvalue_s16 >> *rvalue_s16; } break;
          case bc_binop_type_t::shiftrsign_32: { *dst_s32 = *lvalue_s32 >> *rvalue_s32; } break;
          case bc_binop_type_t::shiftrsign_64: { *dst_s64 = *lvalue_s64 >> *rvalue_s64; } break;

          // unsigned/signed
          case bc_binop_type_t::div_u8:  { *dst_u8  = *lvalue_u8  / *rvalue_u8 ; } break;
          case bc_binop_type_t::div_u16: { *dst_u16 = *lvalue_u16 / *rvalue_u16; } break;
          case bc_binop_type_t::div_u32: { *dst_u32 = *lvalue_u32 / *rvalue_u32; } break;
          case bc_binop_type_t::div_u64: { *dst_u64 = *lvalue_u64 / *rvalue_u64; } break;
          case bc_binop_type_t::div_s8:  { *dst_s8  = *lvalue_s8  / *rvalue_s8 ; } break;
          case bc_binop_type_t::div_s16: { *dst_s16 = *lvalue_s16 / *rvalue_s16; } break;
          case bc_binop_type_t::div_s32: { *dst_s32 = *lvalue_s32 / *rvalue_s32; } break;
          case bc_binop_type_t::div_s64: { *dst_s64 = *lvalue_s64 / *rvalue_s64; } break;
          case bc_binop_type_t::mod_u8:  { *dst_u8  = *lvalue_u8  % *rvalue_u8 ; } break;
          case bc_binop_type_t::mod_u16: { *dst_u16 = *lvalue_u16 % *rvalue_u16; } break;
          case bc_binop_type_t::mod_u32: { *dst_u32 = *lvalue_u32 % *rvalue_u32; } break;
          case bc_binop_type_t::mod_u64: { *dst_u64 = *lvalue_u64 % *rvalue_u64; } break;
          case bc_binop_type_t::mod_s8:  { *dst_s8  = *lvalue_s8  % *rvalue_s8 ; } break;
          case bc_binop_type_t::mod_s16: { *dst_s16 = *lvalue_s16 % *rvalue_s16; } break;
          case bc_binop_type_t::mod_s32: { *dst_s32 = *lvalue_s32 % *rvalue_s32; } break;
          case bc_binop_type_t::mod_s64: { *dst_s64 = *lvalue_s64 % *rvalue_s64; } break;
          case bc_binop_type_t::pow_u8:  { *dst_u8  = ipow( *lvalue_u8 , *rvalue_u8  ); } break;
          case bc_binop_type_t::pow_u16: { *dst_u16 = ipow( *lvalue_u16, *rvalue_u16 ); } break;
          case bc_binop_type_t::pow_u32: { *dst_u32 = ipow( *lvalue_u32, *rvalue_u32 ); } break;
          case bc_binop_type_t::pow_u64: { *dst_u64 = ipow( *lvalue_u64, *rvalue_u64 ); } break;
          case bc_binop_type_t::pow_s8:  { *dst_u8  = ipow( *lvalue_s8 , *rvalue_s8  ); } break;
          case bc_binop_type_t::pow_s16: { *dst_u16 = ipow( *lvalue_s16, *rvalue_s16 ); } break;
          case bc_binop_type_t::pow_s32: { *dst_u32 = ipow( *lvalue_s32, *rvalue_s32 ); } break;
          case bc_binop_type_t::pow_s64: { *dst_u64 = ipow( *lvalue_s64, *rvalue_s64 ); } break;
          case bc_binop_type_t::gt_u8:  { *dst_u8  = *lvalue_u8  > *rvalue_u8 ; } break;
          case bc_binop_type_t::gt_u16: { *dst_u8  = *lvalue_u16 > *rvalue_u16; } break;
          case bc_binop_type_t::gt_u32: { *dst_u8  = *lvalue_u32 > *rvalue_u32; } break;
          case bc_binop_type_t::gt_u64: { *dst_u8  = *lvalue_u64 > *rvalue_u64; } break;
          case bc_binop_type_t::gt_s8:  { *dst_u8  = *lvalue_s8  > *rvalue_s8 ; } break;
          case bc_binop_type_t::gt_s16: { *dst_u8  = *lvalue_s16 > *rvalue_s16; } break;
          case bc_binop_type_t::gt_s32: { *dst_u8  = *lvalue_s32 > *rvalue_s32; } break;
          case bc_binop_type_t::gt_s64: { *dst_u8  = *lvalue_s64 > *rvalue_s64; } break;
          case bc_binop_type_t::gteq_u8:  { *dst_u8  = *lvalue_u8  >= *rvalue_u8 ; } break;
          case bc_binop_type_t::gteq_u16: { *dst_u8  = *lvalue_u16 >= *rvalue_u16; } break;
          case bc_binop_type_t::gteq_u32: { *dst_u8  = *lvalue_u32 >= *rvalue_u32; } break;
          case bc_binop_type_t::gteq_u64: { *dst_u8  = *lvalue_u64 >= *rvalue_u64; } break;
          case bc_binop_type_t::gteq_s8:  { *dst_u8  = *lvalue_s8  >= *rvalue_s8 ; } break;
          case bc_binop_type_t::gteq_s16: { *dst_u8  = *lvalue_s16 >= *rvalue_s16; } break;
          case bc_binop_type_t::gteq_s32: { *dst_u8  = *lvalue_s32 >= *rvalue_s32; } break;
          case bc_binop_type_t::gteq_s64: { *dst_u8  = *lvalue_s64 >= *rvalue_s64; } break;
          case bc_binop_type_t::lt_u8:  { *dst_u8  = *lvalue_u8  < *rvalue_u8 ; } break;
          case bc_binop_type_t::lt_u16: { *dst_u8  = *lvalue_u16 < *rvalue_u16; } break;
          case bc_binop_type_t::lt_u32: { *dst_u8  = *lvalue_u32 < *rvalue_u32; } break;
          case bc_binop_type_t::lt_u64: { *dst_u8  = *lvalue_u64 < *rvalue_u64; } break;
          case bc_binop_type_t::lt_s8:  { *dst_u8  = *lvalue_s8  < *rvalue_s8 ; } break;
          case bc_binop_type_t::lt_s16: { *dst_u8  = *lvalue_s16 < *rvalue_s16; } break;
          case bc_binop_type_t::lt_s32: { *dst_u8  = *lvalue_s32 < *rvalue_s32; } break;
          case bc_binop_type_t::lt_s64: { *dst_u8  = *lvalue_s64 < *rvalue_s64; } break;
          case bc_binop_type_t::lteq_u8:  { *dst_u8  = *lvalue_u8  <= *rvalue_u8 ; } break;
          case bc_binop_type_t::lteq_u16: { *dst_u8  = *lvalue_u16 <= *rvalue_u16; } break;
          case bc_binop_type_t::lteq_u32: { *dst_u8  = *lvalue_u32 <= *rvalue_u32; } break;
          case bc_binop_type_t::lteq_u64: { *dst_u8  = *lvalue_u64 <= *rvalue_u64; } break;
          case bc_binop_type_t::lteq_s8:  { *dst_u8  = *lvalue_s8  <= *rvalue_s8 ; } break;
          case bc_binop_type_t::lteq_s16: { *dst_u8  = *lvalue_s16 <= *rvalue_s16; } break;
          case bc_binop_type_t::lteq_s32: { *dst_u8  = *lvalue_s32 <= *rvalue_s32; } break;
          case bc_binop_type_t::lteq_s64: { *dst_u8  = *lvalue_s64 <= *rvalue_s64; } break;

          // float
          case bc_binop_type_t::add_f32:   { *dst_f32 = *lvalue_f32 + *rvalue_f32; } break;
          case bc_binop_type_t::add_f64:   { *dst_f64 = *lvalue_f64 + *rvalue_f64; } break;
          case bc_binop_type_t::sub_f32:   { *dst_f32 = *lvalue_f32 - *rvalue_f32; } break;
          case bc_binop_type_t::sub_f64:   { *dst_f64 = *lvalue_f64 - *rvalue_f64; } break;
          case bc_binop_type_t::mul_f32:   { *dst_f32 = *lvalue_f32 * *rvalue_f32; } break;
          case bc_binop_type_t::mul_f64:   { *dst_f64 = *lvalue_f64 * *rvalue_f64; } break;
          case bc_binop_type_t::div_f32:   { *dst_f32 = *lvalue_f32 / *rvalue_f32; } break;
          case bc_binop_type_t::div_f64:   { *dst_f64 = *lvalue_f64 / *rvalue_f64; } break;
          case bc_binop_type_t::mod_f32:   { *dst_f32 = Mod32( *lvalue_f32, *rvalue_f32 ); } break;
          case bc_binop_type_t::mod_f64:   { *dst_f64 = Mod64( *lvalue_f64, *rvalue_f64 ); } break;
          case bc_binop_type_t::pow_f32:   { *dst_f32 = Pow32( *lvalue_f32, *rvalue_f32 ); } break;
          case bc_binop_type_t::pow_f64:   { *dst_f64 = Pow64( *lvalue_f64, *rvalue_f64 ); } break;
          case bc_binop_type_t::and_f32:   { *dst_u8  = *lvalue_f32 != 0  &&  *rvalue_f32 != 0; } break;
          case bc_binop_type_t::and_f64:   { *dst_u8  = *lvalue_f64 != 0  &&  *rvalue_f64 != 0; } break;
          case bc_binop_type_t::or_f32:    { *dst_u8  = *lvalue_f32 != 0  ||  *rvalue_f32 != 0; } break;
          case bc_binop_type_t::or_f64:    { *dst_u8  = *lvalue_f64 != 0  ||  *rvalue_f64 != 0; } break;
          case bc_binop_type_t::eq_f32:    { *dst_u8  = *lvalue_f32 == *rvalue_f32; } break;
          case bc_binop_type_t::eq_f64:    { *dst_u8  = *lvalue_f64 == *rvalue_f64; } break;
          case bc_binop_type_t::noteq_f32: { *dst_u8  = *lvalue_f32 != *rvalue_f32; } break;
          case bc_binop_type_t::noteq_f64: { *dst_u8  = *lvalue_f64 != *rvalue_f64; } break;
          case bc_binop_type_t::gt_f32:    { *dst_u8  = *lvalue_f32 >  *rvalue_f32; } break;
          case bc_binop_type_t::gt_f64:    { *dst_u8  = *lvalue_f64 >  *rvalue_f64; } break;
          case bc_binop_type_t::gteq_f32:  { *dst_u8  = *lvalue_f32 >= *rvalue_f32; } break;
          case bc_binop_type_t::gteq_f64:  { *dst_u8  = *lvalue_f64 >= *rvalue_f64; } break;
          case bc_binop_type_t::lt_f32:    { *dst_u8  = *lvalue_f32 <  *rvalue_f32; } break;
          case bc_binop_type_t::lt_f64:    { *dst_u8  = *lvalue_f64 <  *rvalue_f64; } break;
          case bc_binop_type_t::lteq_f32:  { *dst_u8  = *lvalue_f32 <= *rvalue_f32; } break;
          case bc_binop_type_t::lteq_f64:  { *dst_u8  = *lvalue_f64 <= *rvalue_f64; } break;
          default: UnreachableCrash();
        }
      } break;
      default: UnreachableCrash();
    }
    instruction_pointer += 1;
  }
  MemHeapFree( stack.mem );
}



// TODO: how do we store fncall, since we need to know the dest code location, possibly before generating it?
// in x86 asm i think you just use the fn name, but i'm not sure about in the executable.
// presumably you have to resolve to a location, unless the OS loader does that for you.
// i'll start with unresolved for now, and we can always do a post-pass to replace fncalls.

  s64 offset_into_scope;


//Inl void
//ComputeStackOffset(
//  vartable_t* vartable,
//  var_t* var
//  )
//{
//  var->offset_into_scope = vartable->vars_bytecount_top;
//  vartable->vars_bytecount_top += var->type.bytecount;
//}





Inl void
PrintCppType(
  stack_resizeable_cont_t<u8>* out,
  type_t* type
  )
{
  switch( type->type ) {
    case type_type_t::num: {
      auto bytecount = type->num.bytecount;
      switch( type->num.type ) {
        case num_type_t::unsigned_: {
          AddBackString( out, "u" );
        } break;
        case num_type_t::signed_: {
          AddBackString( out, "s" );
        } break;
        case num_type_t::float_: {
          AddBackString( out, "f" );
        } break;
        default: UnreachableCrash();
      }
      stack_nonresizeable_stack_t<u8, 64> tmp;
      CsFromIntegerU( tmp.mem, Capacity( tmp ), &tmp.len, bytecount * 8 );
      AddBackString( out, SliceFromArray( tmp ) );
    } break;
    case type_type_t::bool_: {
      AddBackString( out, "bool" );
    } break;
    case type_type_t::string_: {
      AddBackString( out, "string" );
    } break;
    case type_type_t::struct_: {
      PrintIdent( out, &type->structdecltype->ident );
    } break;
    case type_type_t::enum_: {
      PrintIdent( out, &type->enumdecltype->ident );
    } break;
    default: UnreachableCrash();
  }
  if( type->qualifiers.len ) {
    AddBackString( out, " " );
  }
  FORLIST( qualifier, elem, type->qualifiers )
    switch( qualifier->type ) {
      case typedecl_qualifier_type_t::star: {
        AddBackString( out, "*" );
      } break;
      case typedecl_qualifier_type_t::array: {
        AddBackString( out, "[" );
        FORLIST( arrayidx, elem2, qualifier->arrayidxs )
          switch( arrayidx->type ) {
            case typedecl_arrayidx_type_t::star: {
              AddBackString( out, "*" );
            } break;
            case typedecl_arrayidx_type_t::expr_const: {
              stack_nonresizeable_stack_t<u8, 64> tmp;
              CsFromIntegerU( tmp.mem, Capacity( tmp ), &tmp.len, arrayidx->value );
              AddBackString( out, SliceFromArray( tmp ) );
            } break;
            default: UnreachableCrash();
          }
          if( elem2 != qualifier->arrayidxs.last ) {
            AddBackString( out, ", " );
          }
        }
        AddBackString( out, "]" );
      } break;
      default: UnreachableCrash();
    }
  }
}
Inl void
PrintCppVarName(
  stack_resizeable_cont_t<u8>* out,
  var_t* var
  )
{
  AddBackString( out, "var" );
  PrintU64( out, var->idx );
}
Inl void
PrintCppVar(
  stack_resizeable_cont_t<u8>* out,
  var_t* var
  )
{
  PrintCppType( out, &var->type );
  AddBackString( out, " " );
  PrintCppVarName( out, var );
}
Inl void
PrintCppFndecltype(
  stack_resizeable_cont_t<u8>* out,
  fndecltype_t* fndecltype
  )
{
  AddBackString( out, "void " );
  PrintIdent( out, &fndecltype->ident );
  AddBackString( out, "(\n" );
  FORLEN( type_arg, j, fndecltype->args )
    AddBackString( out, "  " );
    PrintCppType( out, &type_arg->type );
    AddBackString( out, " " );
    PrintIdent( out, &type_arg->ident );
    if( j + 1 != fndecltype->args.len  ||  fndecltype->rets.len ) {
      AddBackString( out, "," );
    }
    AddBackString( out, "\n" );
  }
  FORLEN( type_ret, j, fndecltype->rets )
    AddBackString( out, "  " );
    PrintCppType( out, type_ret );
    AddBackString( out, "* " ); // rets are passed as pointer args.
    AddBackString( out, "_ret" );
    PrintU64( out, j );
    if( j + 1 != fndecltype->rets.len ) {
      AddBackString( out, "," );
    }
    AddBackString( out, "\n" );
  }
  AddBackString( out, "  )" );
}
NoInl void
GenerateCpp(
  compilecontext_t* ctx,
  stack_resizeable_cont_t<u8>* out
  )
{
  auto globaltypes = ctx->globaltypes;
  AddBackString( out, "typedef int8_t      s8;\n" );
  AddBackString( out, "typedef int16_t    s16;\n" );
  AddBackString( out, "typedef int32_t    s32;\n" );
  AddBackString( out, "typedef int64_t    s64;\n" );
  AddBackString( out, "typedef uint8_t     u8;\n" );
  AddBackString( out, "typedef uint16_t   u16;\n" );
  AddBackString( out, "typedef uint32_t   u32;\n" );
  AddBackString( out, "typedef uint64_t   u64;\n" );
  AddBackString( out, "typedef float      f32;\n" );
  AddBackString( out, "typedef double     f64;\n" );
  FORLIST( enumdecltype, elem, globaltypes->enumdecltypes )
    AddBackString( out, "enum class " );
    PrintIdent( out, &enumdecltype->ident );
    AddBackString( out, " : " );
    PrintCppType( out, &enumdecltype->type );
    AddBackString( out, " {\n" );
    ForLen( j, enumdecltype->values ) {
      auto value = enumdecltype->values.mem + j;
      AddBackString( out, "  " );
      PrintIdent( out, &value->ident );
      AddBackString( out, " = " );
      PrintU64( out, value->value );
      AddBackString( out, ",\n" );
    }
    AddBackString( out, "}\n" );
  }
  FORLIST( namedvar_constant, elem, globaltypes->constants )
    AddBackString( out, "static " );
    PrintCppType( out, &namedvar_constant->var->type );
    AddBackString( out, " " );
    PrintIdent( out, &namedvar_constant->ident );
    AddBackString( out, "\n" );
  }
  FORLIST( structdecltype, elem, globaltypes->structdecltypes )
    AddBackString( out, "struct " );
    PrintIdent( out, &structdecltype->ident );
    AddBackString( out, " {\n" );
    FORLEN( type_field, j, structdecltype->fields )
      AddBackString( out, "  " );
      PrintCppType( out, &type_field->type );
      AddBackString( out, " " );
      PrintIdent( out, &type_field->ident );
      AddBackString( out, ";\n" );
    }
    AddBackString( out, "};\n" );
  }
  FORLIST( fndecltype, elem, globaltypes->fndecltypes )
    PrintCppFndecltype( out, fndecltype );
    AddBackString( out, ";\n" );
  }
  FORLIST( function, elem, globaltypes->functions )
    AddBackString( out, "void " );
    PrintIdent( out, &function->fndecltype->ident );
    AddBackString( out, "(\n" );
    FORLEN( namedvar_arg, j, function->namedvar_args )
      AddBackString( out, "  " );
      PrintCppVar( out, namedvar_arg->var );
      if( j + 1 != function->namedvar_args.len  ||  function->fndecltype->rets.len ) {
        AddBackString( out, "," );
      }
      AddBackString( out, "\n" );
    }
    FORLEN( var_ret, j, function->var_rets_by_ptr )
      AddBackString( out, "  " );
      PrintCppVar( out, *var_ret );
      if( j + 1 != function->fndecltype->rets.len ) {
        AddBackString( out, "," );
      }
      AddBackString( out, "\n" );
    }
    AddBackString( out, "  )" );
    AddBackString( out, "\n{\n" );
    idx_t idx = 0;
    FORLIST( tvar, j, function->tvars )
      if( idx++ < function->fndecltype->args.len + function->fndecltype->rets.len ) {
        continue;
      }
      AddBackString( out, "  " );
      PrintCppVar( out, tvar );
      AddBackString( out, ";\n" );
    }
    FORLIST( tc, j, function->tcode )
      switch( tc->type ) {
        case tc_type_t::fncall: {
          AddBackString( out, "  " );
          PrintIdent( out, &tc->fncall.fndecltype->ident );
          AddBackString( out, "(\n" );
          FORLEN( var_arg, k, tc->fncall.var_args )
            AddBackString( out, "    " );
            PrintCppVarName( out, *var_arg );
            if( k + 1 != tc->fncall.var_args.len  ||  tc->fncall.var_rets.len ) {
              AddBackString( out, "," );
            }
            AddBackString( out, "\n" );
          }
          FORLEN( var_ret, k, tc->fncall.var_rets )
            AddBackString( out, "    " );
            PrintCppVarName( out, *var_ret );
            if( k + 1 != tc->fncall.var_rets.len ) {
              AddBackString( out, "," );
            }
            AddBackString( out, "\n" );
          }
          AddBackString( out, "    );\n" );
        } break;
        case tc_type_t::ret: {
          AddBackString( out, "  " );
          AddBackString( out, "return;\n" );
        } break;
        case tc_type_t::move: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->move.var_l );
          AddBackString( out, " = " );
          PrintCppVarName( out, tc->move.var_r );
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::loadconstant: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->loadconstant.var_l );
          AddBackString( out, " = " );
          u32 bytecount = BytecountType( ctx, &tc->loadconstant.var_l->type );
          auto mem = Cast( u8*, tc->loadconstant.mem );
          AddBackString( out, "{ " );
          Fori( u32, i, 0, bytecount ) {
            PrintU64( out, mem[i] );
            if( i + 1 != bytecount ) {
              AddBackString( out, ", " );
            }
          }
          AddBackString( out, " }" );
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::store: {
          AddBackString( out, "  " );
          AddBackString( out, "*" );
          PrintCppVarName( out, tc->store.var_l );
          AddBackString( out, " = " );
          PrintCppVarName( out, tc->store.var_r );
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::jumplabel: {
          AddBackString( out, "" );
          AddBackString( out, "jumplabel_" );
          PrintU64Hex( out, Cast( u64, tc ) );
          AddBackString( out, ":\n" );
        } break;
        case tc_type_t::jump: {
          AddBackString( out, "  " );
          AddBackString( out, "goto jumplabel_" );
          PrintU64Hex( out, Cast( u64, tc->jump.jumplabel ) );
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::jumpnotzero: {
          AddBackString( out, "  " );
          AddBackString( out, "if( " );
          PrintCppVarName( out, tc->jumpcond.var_cond );
          AddBackString( out, " ) goto jumplabel_" );
          PrintU64Hex( out, Cast( u64, tc->jumpcond.jump.jumplabel ) );
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::jumpzero: {
          AddBackString( out, "  " );
          AddBackString( out, "if( !" );
          PrintCppVarName( out, tc->jumpcond.var_cond );
          AddBackString( out, " ) goto jumplabel_" );
          PrintU64Hex( out, Cast( u64, tc->jumpcond.jump.jumplabel ) );
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::binop: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->binop.var_result );
          AddBackString( out, " = " );
          switch( tc->binop.type ) {
            case binoptype_t::pow_ :  {
              AddBackString( out, "Pow( " );
              PrintCppVarName( out, tc->binop.var_l );
              AddBackString( out, ", " );
              PrintCppVarName( out, tc->binop.var_r );
              AddBackString( out, " )" );
            } break;
            case binoptype_t::mul  :  PrintCppVarName( out, tc->binop.var_l );  AddBackString( out, " * " );   PrintCppVarName( out, tc->binop.var_r );  break;
            case binoptype_t::div  :  PrintCppVarName( out, tc->binop.var_l );  AddBackString( out, " / " );   PrintCppVarName( out, tc->binop.var_r );  break;
            case binoptype_t::mod  :  PrintCppVarName( out, tc->binop.var_l );  AddBackString( out, " % " );   PrintCppVarName( out, tc->binop.var_r );  break;
            case binoptype_t::add  :  PrintCppVarName( out, tc->binop.var_l );  AddBackString( out, " + " );   PrintCppVarName( out, tc->binop.var_r );  break;
            case binoptype_t::sub  :  PrintCppVarName( out, tc->binop.var_l );  AddBackString( out, " - " );   PrintCppVarName( out, tc->binop.var_r );  break;
            case binoptype_t::and_ :  PrintCppVarName( out, tc->binop.var_l );  AddBackString( out, " && " );  PrintCppVarName( out, tc->binop.var_r );  break;
            case binoptype_t::or_  :  PrintCppVarName( out, tc->binop.var_l );  AddBackString( out, " || " );  PrintCppVarName( out, tc->binop.var_r );  break;
            case binoptype_t::eqeq :  PrintCppVarName( out, tc->binop.var_l );  AddBackString( out, " == " );  PrintCppVarName( out, tc->binop.var_r );  break;
            case binoptype_t::noteq:  PrintCppVarName( out, tc->binop.var_l );  AddBackString( out, " != " );  PrintCppVarName( out, tc->binop.var_r );  break;
            case binoptype_t::gt   :  PrintCppVarName( out, tc->binop.var_l );  AddBackString( out, " > " );   PrintCppVarName( out, tc->binop.var_r );  break;
            case binoptype_t::gteq :  PrintCppVarName( out, tc->binop.var_l );  AddBackString( out, " >= " );  PrintCppVarName( out, tc->binop.var_r );  break;
            case binoptype_t::lt   :  PrintCppVarName( out, tc->binop.var_l );  AddBackString( out, " < " );   PrintCppVarName( out, tc->binop.var_r );  break;
            case binoptype_t::lteq :  PrintCppVarName( out, tc->binop.var_l );  AddBackString( out, " <= " );  PrintCppVarName( out, tc->binop.var_r );  break;
            default: UnreachableCrash();  break;
          }
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::unop: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->unop.var_result );
          AddBackString( out, " = " );
          switch( tc->unop.type ) {
            case unoptype_t::deref      :  AddBackString( out, "*" );  break;
            case unoptype_t::negate_num :  AddBackString( out, "-" );  break;
            case unoptype_t::negate_bool:  AddBackString( out, "!" );  break;
            default: UnreachableCrash();  break;
          }
          PrintCppVarName( out, tc->unop.var );
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::autocast: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->autocast.var_result );
          AddBackString( out, " = ( " );
          PrintCppType( out, &tc->autocast.var_result->type );
          AddBackString( out, " )" );
          PrintCppVarName( out, tc->autocast.var );
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::array_elem_from_array: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->arrayidx.var_result );
          AddBackString( out, " = " );
          PrintCppVarName( out, tc->arrayidx.var_array );
          AddBackString( out, ".mem" );
          // TODO: dimensional flattening, jagged handling, etc.
          AddBackString( out, "[ " );
          FORLEN( var_arrayidx, k, tc->arrayidx.var_arrayidxs )
            PrintCppVarName( out, *var_arrayidx );
            if( k + 1 != tc->arrayidx.var_arrayidxs.len ) {
              AddBackString( out, ", " );
            }
          }
          AddBackString( out, " ];\n" );
        } break;
        case tc_type_t::array_elem_from_parray: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->arrayidx.var_result );
          AddBackString( out, " = " );
          PrintCppVarName( out, tc->arrayidx.var_array );
          AddBackString( out, "->mem" );
          // TODO: dimensional flattening, jagged handling, etc.
          AddBackString( out, "[ " );
          FORLEN( var_arrayidx, k, tc->arrayidx.var_arrayidxs )
            PrintCppVarName( out, *var_arrayidx );
            if( k + 1 != tc->arrayidx.var_arrayidxs.len ) {
              AddBackString( out, ", " );
            }
          }
          AddBackString( out, " ];\n" );
        } break;
        case tc_type_t::array_pelem_from_array: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->arrayidx.var_result );
          AddBackString( out, " = " );
          PrintCppVarName( out, tc->arrayidx.var_array );
          AddBackString( out, ".mem + " );
          // TODO: dimensional flattening, jagged handling, etc.
          FORLEN( var_arrayidx, k, tc->arrayidx.var_arrayidxs )
            PrintCppVarName( out, *var_arrayidx );
            if( k + 1 != tc->arrayidx.var_arrayidxs.len ) {
              AddBackString( out, ", " );
            }
          }
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::array_pelem_from_parray: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->arrayidx.var_result );
          AddBackString( out, " = " );
          PrintCppVarName( out, tc->arrayidx.var_array );
          AddBackString( out, "->mem + " );
          // TODO: dimensional flattening, jagged handling, etc.
          FORLEN( var_arrayidx, k, tc->arrayidx.var_arrayidxs )
            PrintCppVarName( out, *var_arrayidx );
            if( k + 1 != tc->arrayidx.var_arrayidxs.len ) {
              AddBackString( out, ", " );
            }
          }
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::struct_field_from_struct: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->fieldaccess.var_result );
          AddBackString( out, " = " );
          PrintCppVarName( out, tc->fieldaccess.var_struct );
          AddBackString( out, "." );
          PrintIdent( out, &tc->fieldaccess.field_name );
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::struct_field_from_pstruct: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->fieldaccess.var_result );
          AddBackString( out, " = " );
          PrintCppVarName( out, tc->fieldaccess.var_struct );
          AddBackString( out, "->" );
          PrintIdent( out, &tc->fieldaccess.field_name );
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::struct_pfield_from_struct: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->fieldaccess.var_result );
          AddBackString( out, " = &" );
          PrintCppVarName( out, tc->fieldaccess.var_struct );
          AddBackString( out, "." );
          PrintIdent( out, &tc->fieldaccess.field_name );
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::struct_pfield_from_pstruct: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->fieldaccess.var_result );
          AddBackString( out, " = &" );
          PrintCppVarName( out, tc->fieldaccess.var_struct );
          AddBackString( out, "->" );
          PrintIdent( out, &tc->fieldaccess.field_name );
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::addrof: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->addrof.var_result );
          AddBackString( out, " = &" );
          PrintCppVarName( out, tc->addrof.var );
          AddBackString( out, ";\n" );
        } break;
        case tc_type_t::deref: {
          AddBackString( out, "  " );
          PrintCppVarName( out, tc->deref.var_result );
          AddBackString( out, " = *" );
          PrintCppVarName( out, tc->deref.var );
          AddBackString( out, ";\n" );
        } break;
        default: UnreachableCrash();
      }
    }
    AddBackString( out, "}\n" );
  }
}





#if 0

  responsibilities of the typing phase:

  big picture:

  broadly, assign a concrete type to every variable in the program.

  the primary strategy is to maintain a list of possible types for each variable, per-scope.
  we'll gradually narrow those possibilities until we have concrete types at the end.
  note that struct instances are an aggregate of variables, so they've got multiple typelists, one per field.
  note that constants/literals are a variable for the purposes of typing, since we want to allow autocasting.
  note that fncalls are also an aggregate of variables for typing, namely { args, rets }.
  note that fndefn args are variables, but have a fixed type; concrete or generic. this is to make lookup easy.

  there are two kinds of operations we'll do:
    generating typelists
    narrowing typelists given a typelist
  e.g. if we have a u8 foo, and we want to type 'foo + 8', we have to generate typelists for foo and 8.
  note that there's a var table entry for both of these, but there's no entry for the sum.
  foo is simple, since it's declared as { u8 }.
  8 is more complicated, since it's { u8, u16, ..., s8, s16, ..., f32, ... }.
  the idea is that we'll generate typelists for each side of the + binop, realize that the intersect
  is { u8 }, so we then narrow both sides to { u8 }.
  the final narrowing here makes it easy for the codegen to know what size literal we have.
  since we're always narrowing possibilities, we can implement narrowing as just replacement of the typelists
  in the var table, with the given typelist.
  replacement should also work for replacing the generic type_t.

  one open question is whether we should store more intermediate typelists.
  since subsequent Generate calls will return different typelists after Narrow calls, maybe we'll have some
    duplicate work?
  i'd prefer to keep to minimal state, since our datastructs are already complicated as is.
  doing this should only be a late-game play.

  open question: where do we do the tree recursion?
  it seems like we need two calls in sequence per binary node, Generate and Narrow.
  i think the tree recursion is on Typing, so we'll have
    Typing() {
      a = Generate(lhs);
      b = Generate(rhs);
      c = intersect(a,b);
      Narrow(lhs, c);
      Narrow(rhs, c);
      Typing(lhs);
      Typing(rhs);
    }
  what if we do recursive Typing as part of Narrow only?
  i think this gives us incorrect results at the end, unless we repeat the whole process.
  and i'm not certain repeating only once is sufficient. this seems like a terrible idea.
  what if we do recursive Typing as part of Generate?  that would provide the nice recursive "children are done" property, right?
  i think that's wrong, since Narrow affects the children, right?
  i guess we can also do Typing as part of Narrow, if it actually narrows.
  actually if we got past generate w/o errors, and we're replacing the typelists with a nonempty subset,
  then we've already verified the match is fine. so we shouldn't need to recurse again after Narrow.
  so the final structure will be like
    Generate() {
      a = Generate(lhs);
      b = Generate(rhs);
      c = intersect(a,b);
      Narrow(lhs, c);
      Narrow(rhs, c);
    }
  we can use a single stack_resizeable_cont_t<typelist_elem_t> for this, and return tslice_t<typelist_elem_t> into the array,
  and just push/pop the array len in each frame. that would allow for in-place intersect.
  as a simpler alternate, if we can just sequentially generate typelists in the ctx->mem, that might be better.

  another question: do we ever have mixing, where Generate would need to call Narrow, or vise versa?
  i don't think so, but we'll see in impl.

  does the two phase ret processing thing also has the same reprocessing problems as recursive calls?
    e.g. Foo() $T { if(...) { ret 2; } else { ret 3000000000; } }
    i think we'd need to go back to narrow the first '2' literal, since T has to be 8+byte int or 4+byte float.
    i guess we could leave literals alone, and typing would just determine the fndecl args/rets concretely?
    then codegen could come up with the right sized value to match, given the ret typing.
    same goes for unop/binop/binassign i suppose.

  how do we handle this?
    Foo() $T { r := 2;  if(...) { r = 3000000000; }  ret r; }
      create fndecl table entry with (),(type_t T)
      create var table entry for r, with typelist { u8, ..., s8, ..., f32, ... } generated from the '2'
      lookup 'r' entry, and narrow typelist to { u64, s64, f32, f64 } generated from the '3000000000'
      QUESTION: do we need to reprocess statements before? e.g. what if we passed 'r' as a fncall arg?
      create entry for the 'ret r' in the fndefn scope var table.
      intersect all rets in the fndefn scopevar table, and get back { u64, s64, f32, f64 }.
      replace the fndecl table entry with (),({ u64, s64, f32, f64 })
    there's too many "do we need to reprocess now?" happening.
    i think we have to defer most of this work until we've got a concrete type to deal with.
    so, we should probably dial back the generate/narrow ideas, since we'll likely need something else
    to solve polymorphing nicely.

  new idea:
    start with 'Main(string) int' ( and probably all other all-concrete fndefns )
    use the fndefn -> { fncall } dependency info to verify all fncalls have concrete typings.
    if they're still generic, error with 'Anbiguous type, please be more specific { typelist }'.
    clone the generic fndefn matching the fncall, and replace the generic slots with the given concrete.
    point the fncall to that new concrete fndefn.
    recurse: type the entire cloned concrete fndefn, verifying all it's fncalls have concrete typings.

    this has the downside of failing to do typing on unused generic code.
    i'm okay with that, since that's pretty standard i think.
    as long as we get better error messaging than other langs, this is probably fine.
    to do some typing, we'd have to define which subset of errors we want to surface, and which we don't,
    since we don't actually know what the generics are.

    this has the upside of not requiring reprocessing.

    now we need to work out the details of fn/struct cloning, as well as how to fill in typeslots.
      for filling in typeslots, if we dedupe each generic ident in the fndecl+fndefn scope, we can
      just change that singular ident.
      for cloning, it'd be nice to just Memmove. otherwise we need to either:
        in memory explicit datastructure clone.
        serialize to mem, deserialize from mem.
      if we need serialization anyways for other reasons, that's probably reasonable to do here.
      to do Memmove clone:
        we need to know the bounds of the fndecl+fndefn, so we can memmove.
        we can leave most pointers in place, since we're not changing large structures, just the ident.
      actually, could we just change type lookup to account for the change?
      then we could avoid any cloning, which would be sweet.
      something like an alias table.
      we'd still have to keep track of each instance of the fn we make, so codegen knows to make it.

      this means we have to decouple vartables from the parsetree, since we'll have multiple vartable trees
      per fndecl+fndefn parsetree.

      actually we can't just change an ident, since we could be passing T*[], for example.
      so we need a typedecl_t alias, not an ident_t alias, that does type inlining for us.

    how do we point the fncall to the specific alias table / fn instance?
      since we're only modifying fncalls inside known-concrete fns, we're free to just change it.
      i.e. use some string disallowed by the language, and put that in the fncall ident.
      or just expand that datastruct to have two kinds: regular string ident, and polymorphed instance fn.

    what do we do about partial specialization? e.g.
      foo_t struct { foo $S; bar $T }
      Main() {
        bar_t := foo_t(int);
        a : bar_t(uint);
        a.foo = -100;
        a.bar = 100;
      }


  details:

  for every fndecl,
    create an entry in the global_scope fndecl table.  if it's a duplicate, error.
    note that the types in this fndecl table are either a concrete type, or the generic type_t.
    the idea is that codegen will maintain a cache of which generic->concrete fndefns it's generated,
      and generate new instances as needed.

  for every fndefn,
    add all args/rets to the fndefn's scope var table.
    if there's a concrete declared ret type,
      narrow all ret exprs to that type.
    elif there's a generic declared ret type,
      generate typelists for each ret expr.
      intersect those typelists.  if empty, error.
      replace the global_scope fndecl table's entry for the ret type, with the intersect.
    else there's no declared ret type,
      error if any rets have exprs.

  TODO: how does this global_scope fndecl table entry modification affect correctness?
    we need to do this before typing all fncalls to this fn, i think.
    if we type a fncall before the fndefn, and we're in this narrowing case:
      the fncall may be allowed to finish typing successfully, but it's not actually correct.
      we could fix this up later, by typing fncalls again later on, perhaps during codegen.
      the problem with that is we have to retype a lot, since fncalls are exprs and statements.
      since we don't have an up-traversal tree, there's not a lot of easy scoping we can do.
      global_statement_t is about the best scoping we've got. maybe that's good enough.
      i can also imagine a cycle, since recursive calls form a graph, not a tree.
      in that case, we'd have to track global_statement <-> global_statement dependency info,
      and just iterate, retyping each dependent if we ever change the global_scope fndecl table entry.
      more specifically, the dependency info would be: fndefn -> list of fncall, or the equivalent in
      the fndecl table.
      even to make it an error to make recursive calls to generic-ret fns, we need this dependency info.

    recurse typing into the scope.
      this should gather all rets, and

  for every structdecl,
    check that there's no duplicate field names in the struct.
    create an entry in the global_scope structdecl table.  if it's a duplicate, error.
    same as the fndecl table, the types in the table are fixed.

  for every enumdecl,
    check there's no duplicate value names in the enum.
    create an entry in the global_scope enumdecl table.  if duplicate, error.
    typing is trivial: enum is fixed to u32 for now.  we'll allow custom-type enums later.

  for every declassign_const in global_scope,
    create an entry in the global_scope constant table.  if it's a duplicate, error.
    implicit ones should infer the typelist of the rhs.
    same as the other global tables, this is fixed.

  for every num/str/chr literal,
    generate typelist based on the value.
    create an entry in the local scope var table.
      don't dedupe this with other literals, since types may differ.
      store the typelist.

  for every fncall,
    lookup the fndecl typing.  if not present, error.
    generate typelists for all args/rets.
    narrow all args/rets to types that match the fndecl.  if no valid match, error.
    create an entry in the local scope var table.  store the args/rets typelists.

  for every expr_assignable,
    lookup the first ident in the vartable.  if not present, error.
    narrow the var's typelist to those which have a structdecl with matching types.  if no valid match, error.
    note we allow direct offset as well as pointer deref with the dot.
      repeat for subsequent dot accesses in the same expr_assignable.

  for every binassign,
    generate typelists of the lhs and rhs.
    depending on the binassignop, throw out some possibilities, creating a new typelist.  if none left, error.
    narrow lhs and rhs to the narrowed typelist.

  for every binop,
    generate typelists of the lhs and rhs.
    depending on the binop, throw out some possibilities, creating a new typelist.  if none left, error.
    narrow lhs and rhs to the narrowed typelist.

  for every unop,
    generate typelists of the rhs.
    depending on the binop, throw out some possibilities, creating a new typelist.  if none left, error.
    narrow rhs to the narrowed typelist.

  for every decl/declassign in local scopes,
    generate typelist of the var:
      decls should trivially use the given typedecl.
      implicit declassigns should infer the typelist of the rhs.
      explicit declassigns should intersect the given typedecl with the rhs typelist.
    create an entry in it's parent scope var table.  store the typelist.  if duplicate, error.

  for every ret,
    nothing needed.
      the fndefn level will handle all rets at the same time.
      this is because we have to intersect all the rets, and then narrow them all.






  responsibilities of the codegen phase:

  verify there's a fndefn 'Main(string) int'
    arg name shouldn't matter.



  note that we'll verify there's only one possible type in each var's typelist during codegen, since we'd have
    to do another whole traversal to check that anyways.
  it makes sense to just wait until the next phase which does that traversal.
  and codegen has to read that one concrete type anyways, so.

  open question: in an entry the local scope var table, is the one-possible-type allowed to be the generic type_t?
    for fncalls, i think not. we don't want to generate dynamic-typed code. at least to start with.
    for vars/struct instances, i think not. if we didn't
    if yes, then we need some mechanism for filling in the type during codegen.





  we'll do a good amount of 'code generation' during typing.
  but, we'll leave the scope-flattening, stack-offset computations, etc.
    as a separate pass over the vartables.
  that just seems like a cleaner way of doing things.
  it also allows for freedom in choosing calling conventions, which is probably best to have all that code
    grouped together for clarity. lots of 'push this' 'pop this' code which is tricky to get right.

  note we'll do some flattening in typing, e.g. binop exprs will cause intermediate_vars, and linear code.
  that's the best place to do this kind of thing, during typing, while we've got full parsetree context.
  but it's best not to make the typing code even more complicated.

  open question: should we do control flow -> jump conversion during typing?
    i suspect not, because the scope trees are tied to conditionals.
    when we flatten scopes is when it makes sense to convert control flow.

  note we can't really do scope-flattening as part of typing, since we need the ability to exclude
    sister scopes. i.e. '{ { foo:=2 } { foo:=2 } }' is just fine.
  i guess we can have separate vars for the typing, and for the codegen? and then make both at once?

  open question: why not just do scope flattening and controlflow->jump during typing?
    we probably want to avoid stack-offset computations during typing, for the reasons above.
    but 'typing will be too much code' isn't really a good reason, if it's simpler than more datastructs.
    minimal datastructs is the name of the game, not minimal code.

  so our options are:
    1. retype subexpressions etc during codegen, which does a full parsetree traverse.
    2. cache typing info during typing, and read the cached types during codegen parsetree traverse.
    3. do typing AND full scope/controlflow-flattening into separate datastructs, during the typing traverse.
    4. cache typing info and scope/controlflow info during typing, and codegen only traverses the cache,
         doing the flattening.

  downsides:
    1. wasteful
    2. not as wasteful, although still somewhat, since it's a full parsetree traverse. also not a fan of
         modifying the same datastructs in multiple traverses. i'm terrible at that kind of thinking.
    3. typing phase becomes all the code.
    4. more datastructs.

  upsides:
    1. not many.
    2. no need for more intermediate datastructs.
    3. no need for more intermediate datastructs. centralized logic, complicated as it may be.
    4. better ability to change things like stack vs. register based VMs, calling conventions, etc.

  i'm kind of leaning towards 4.

  let me try attacking from the other end, to make this clearer.
  i'll write a little bytecode vm, and see what constraints it has.

  so it turns out getting args/rets passing correct is pretty complicated.
  for a register+stack vm even more complicated.
  since i haven't done a full compile pipe before, i'll do option 4 since it gives more flexibility,
    and breaks things down into smaller problems. once i've got the full thing up and running, we can
    reconsider this if necessary.

          //
          // need a way of going from var_t* to bc_var_t
          // our function->tvars is essentially our ordered list of stack allocations, they just don't have
          // explicit bc_var_t's. it's implicit in ordering and size of type_t's.
          // well we have bvar_locals, etc.
          // why don't we just make that in the first place?
          // the idea was to allow for more freedom in choosing calling conventions.
          // but, there's not much choice in a stack VM. varied x86 calling conventions are about what each
          //   register is used for across fncalls, but a stack VM doesn't have registers at all.
          // best i can tell, the choices for a stack VM fncall return are:
          //   1. fn leaves retvals in place, and the caller calcs the offset ( including fncall stack cost )
          //        so it can use retvals w/o a move.
          //   2. fn takes in negative offsets to where it should place the retvals. this has more moves than 1.
          //        we don't know what stackvar we're returning until the ret statement.  hence we probably put
          //        the stackvar in the locals, not using the designated ret slot.
          //        so we'd want an optimization 'replace target local var with ret var' and un-allocate the var.
          //        that sounds like terrible code to write. can we do that another way?
          //
          // e.g.
          //   Foo() u8 { C := 2+3;  ret C }
          // currently generates the tcode:
          //   locals A, B, C
          //   moveimm A <- 2
          //   moveimm B <- 3
          //   binop C <- add( A, B )
          //   ret C
          //
          // question: how do we output bc ops s.t. C ends up in the ret slot, without a stack allocation?
          // after tc generation, we know every ret statement, and every var that's returned.
          // i guess we can just do a lookup of those kinds of vars, and replace them with the ret slot?
          // actually we don't need to do much per op; as long as we just point C to the ret slot, we don't
          // need to change any var ptrs or anything. we haven't added/removed any vars.
          // we just need to modify the table of var_t* -> bc_var_t, which we needed to make anyways.
          // we just fixup the entry for C to point to the ret slot.
          // and then remove C from the tcode locals accounting, so we don't waste stack space.
          //
          // question: can we do the same thing in the direct-to-bc world?
          // well, removing a stackvar that's already allocated isn't ideal. if we're just pushing onto a
          // top counter, we have to subtract the removed size from all vars's offset, after the one removed.
          // this kind of indicates it's probably best to do stackallocation after the function is all done.
          // it's much easier to remove a tvar from a list_t.
          //   although you do have to iterate to find it.
          //   unless we embed next/prev pointers into tvar/var_t itself.
          // or we can mark the tvar/var_t as being a ret var, so the end-function bc generation doesn't
          //   include it in locals offset accounting.
          // or we can just assume the memory layout of listelem_t<var_t*>, and do the pointer arith.
          //   do we always have var_t allocated as part of a list? if not, this won't work.
          // or we pass around listelem_t<var_t> instead of var_t.
          // well, i guess it's not much easier, so maybe shifting stackallocs down is fine?
          // well you still need some indirection to do the op target replacement.
          // so it looks like var_t* -> bc_var_t is probably a good idea, enabling this optimization.
          //
          // question: should we pass args/rets by actual value, or just the bc_var_t by value?
          // i.e. should we push the whole arg/ret onto the stack when fncall'ing, or just a bc_var_t?
          // passing bc_var_t effectively amounts to 'pass by reference'.
          // since we write to rets, we likely have to pass bc_var_t for those.
          // but for args, you get a local copy in C/C++.
          // is it worthwhile keeping that around? i know i use that feature sometimes.
          // it's kind of standard programming practice across some langs, i think.
          // but if it's faster not to do it...
          // win_x64 ABI seems to do by value for things that fit in a register, and by reference otherwise.
          // but at the C/C++ language level, you can do differently, although it gets generated down to that.
          // Java language did that same design; register types are passed by value, by ref otherwise.
          // that's not an unreasonable design; i spent less time fiddling with star/nostar in java.
          // but i probably had more "oops, didn't mean to change that" bugs.
          // i actually kind of like the idea of always by reference.
          // i haven't programmed in a language like that, so it could be interesting.
          // i also like that it could be faster for the stack VM.
          // although, it's unlikely to be faster in machine code, since the only difference is
          // in passing register-sized things. and staying inside registers is probably better than byref.
          // if we did always-by-ref, we'd want to do register allocation/optimization anyways, but it
          // would be hindered by having to do loads/stores instead of reg->reg i think.
          // looks like it's exceedingly rare for my code to pass structs by value.
          // the only instances i could find: slice_t, content_ptr_t.
          // and those are 16-byte structs, meant to fit in 2 registers. not that they always will be.
          // if i basically always type the star, is it worth keeping around?
          //
          // related question: should we allow autocast / auto-addrof * T <- T, when passing an arg?
          // note that if we allow auto-addrof in passing args, we should probably allow them in assignments.
          // note:
          // the previous question is about skipping typing star at every arg decl, and this one is about
          // skipping typing ampersand at every arg pass.
          // well the most common error for me is probably . / -> mixup, so fixing field access to always
          // use the arrow will clear that up. is getting the ampersand wrong the next highest error?
          // i think so, yeah.
          // since those two are likely some of the highest hitters, let's try fixing those first, before
          // addressing the first question.
          // so for now, pass all args by value.
          //
          // question: should the byref bc_var_t's offsets be relative to the caller's base, or the call's base?
          // a function can be called from many different functions, so it actually can't be relative to the caller.
          // well it could, but you'd also have to pass in the size of the caller frame.
          // and if we can avoid an extra arg, that's good.
          //

#endif


int
Main( u8* cmdline, idx_t cmdline_len )
{
  {
    // test bytecode execution.

    s32 c_0 = 'a';
    s32 c_1 = 2;
  //  s32 c_2 = c_0 / c_1;
    s32 c_3 = c_1 / c_0;

    stack_resizeable_cont_t<bc_t> code;
    Alloc( code, 2000 );

    // f0(a u32, b u32) u32 { ret a / b }
    auto f0_start = code.len;
    auto f0 = AddBack( code );
    {
      f0->type = bc_type_t::binop;
      f0->binop.type = bc_binop_type_t::div_u32;
      f0->binop.var_l = { 0, 4 };
      f0->binop.var_r = { 4, 4 };
      f0->binop.var_result = { 8, 4 };
      auto f1 = AddBack( code );
      f1->type = bc_type_t::ret;
    }

    // a <- 13
    // b <- a
    // f0(a,b), which returns on the stack.
    // c <- ret
    // Assert( c == 169 );
    auto c0_start = code.len;
    auto c0 = AddBack( code );
    {
      c0->type = bc_type_t::loadconstant;
      c0->loadconstant = { { 0, 4 }, &c_0 };
      auto c1 = AddBack( code );
      c1->type = bc_type_t::loadconstant;
      c1->loadconstant = { { 4, 4 }, &c_1 };
      auto c2 = AddBack( code );
      c2->type = bc_type_t::fncall;
      c2->fncall.target_fn_bc_start = f0_start;
      c2->fncall.caller_loc_args.len = 2;
      c2->fncall.caller_loc_args.mem = MemHeapAlloc( bc_var_t, c2->fncall.caller_loc_args.len );
      c2->fncall.caller_loc_args.mem[0] = { 4, 4 }; // note these are source locations, we'll copy to stack on fncall.
      c2->fncall.caller_loc_args.mem[1] = { 0, 4 };
      c2->fncall.caller_loc_rets.len = 1;
      c2->fncall.caller_loc_rets.mem = MemHeapAlloc( bc_var_t, c2->fncall.caller_loc_rets.len );
      c2->fncall.caller_loc_rets.mem[0] = { 8, 4 }; // note these are source locations, we'll copy to stack on fncall.
      c2->fncall.bytecount_locals = 0;
      auto c4 = AddBack( code );
      c4->type = bc_type_t::assertvalue;
      c4->assertvalue.var_l = { 8, 4 };
      c4->assertvalue.mem = &c_3;
      auto c5 = AddBack( code );
      c5->type = bc_type_t::ret;
    }

    auto m0_start = code.len;
    auto m0 = AddBack( code );
    {
      m0->type = bc_type_t::fncall;
      m0->fncall.target_fn_bc_start = c0_start;
      m0->fncall.caller_loc_args = {};
      m0->fncall.caller_loc_rets = {};
      m0->fncall.bytecount_locals = 12;
    }

//    Execute( SliceFromArray( code ), m0_start );
  }








  compilecontext_t ctx;

  // TODO: cmdline options parsing.
  ctx.filename = SliceFromCStr( "c:/doc/dev/cpp/proj/main/test.jc" );

  file_t file = FileOpen( ML( ctx.filename ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
  if( !file.loaded ) {
    auto tmp = AllocCstr( ctx.filename );
    printf( "can't open sourcefile: %s\n", tmp );
    MemHeapFree( tmp );
    return 1;
  }
  AssertCrash( file.loaded );
  auto filemem = FileAlloc( file );
  ctx.file = SliceFromString( filemem );
  AssertCrash( filemem.mem );
  FileFree( file );

  Init( ctx.mem, 1024*1024 );
  Alloc( ctx.tokens, 32000 );
  Alloc( ctx.errors, 32000 );

  globaltypes_t globaltypes = {};
  ctx.globaltypes = &globaltypes;
  InitBuiltinTypes( &ctx );
  Alloc( ctx.scopestack, 1000 );

  ctx.ptr_bytecount = _SIZEOF_IDX_T;
  ctx.array_bytecount = _SIZEOF_IDX_T * 2; // note we'll do a { void* mem;  idx_t len; } impl.

  ctx.current_function = 0;

  #define PRINTERRORS \
    if( ctx.errors.len ) { \
      *AddBack( ctx.errors ) = 0; \
      printf( "%s", ctx.errors.mem ); \
      return 1; \
    } \

  stack_resizeable_cont_t<u8> debugout;
  Alloc( debugout, 32000 );

  #define PRINTDEBUGOUT \
    *AddBack( debugout ) = 0; \
    printf( "%s", debugout.mem ); \
    debugout.len = 0; \

  Tokenize( &ctx );
  PRINTERRORS;
  PrintTokens( &debugout, &ctx.tokens );
  PRINTDEBUGOUT;
  printf( "\n================================\n\n" );

  auto global_scope = ADD_NODE( &ctx, global_scope_t );
  ParseGlobalScope( &ctx, global_scope );
  PRINTERRORS;
  PrintGlobalScope( &debugout, global_scope );
  PRINTDEBUGOUT;
  printf( "\n================================\n\n" );

  TypeGlobalScope( &ctx, global_scope );
  PRINTERRORS;
  PrintGlobalScopeTypes( &debugout, &ctx, global_scope );
  PRINTDEBUGOUT;
  printf( "\n================================\n\n" );

  stack_resizeable_cont_t<bc_t> code; // TODO: pagelist
  Alloc( code, 4000 );

  idx_t entry_point = 0;
  Generate( &ctx, &code, &entry_point );
  PRINTERRORS;
  auto code_slice = SliceFromArray( code );
  PrintCode( &debugout, &code_slice, entry_point );
  PRINTDEBUGOUT;
  printf( "\n================================\n\n" );

  stack_resizeable_cont_t<u8> cpp; // TODO: pagelist
  Alloc( cpp, 4*1000*1000 );

  GenerateCpp( &ctx, &cpp );
  *AddBack( cpp ) = 0;
  printf( "%s", cpp.mem );
  printf( "\n================================\n\n" );

//  Execute( code_slice, entry_point );

  return 0;
}




int
main( int argc, char** argv )
{
  MainInit();

  stack_resizeable_cont_t<u8> cmdline;
  Alloc( cmdline, 512 );
  Fori( int, i, 1, argc ) {
    slice_t arg;
    arg.mem = Cast( u8*, argv[i] );
    arg.len = CstrLength( arg.mem );
    AddBackContents( &cmdline, arg );
    AddBackCStr( &cmdline, " " );
  }
  int r = Main( ML( cmdline ) );
  Free( cmdline );

  //printf( "Main returned: %d\n", r );
//  system( "pause" );

  MainKill();
  return r;
}

