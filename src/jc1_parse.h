// Copyright (c) John A. Carlos Jr., all rights reserved.


Inl symbol_t
SymbolFromToken( token_t* tkn, src_t* src )
{
  symbol_t sym;
  sym.mem = tkn->mem;
  sym.len = tkn->len;
  return sym;
}


#if 0

Inl void
ErrorExpectedTokenOfType(
  tokentype_t* expected,
  idx_t expected_len,
  token_t* tkn,
  src_t& src,
  log_t& log
  )
{
  log.nerrs += 1;

  OutputFileAndLine( src.filename, log.errs, tkn );

  u8* foundtype = StringOfTokenType( tkn->type );
  idx_t foundtype_len = CstrLength( foundtype );

  Memmove( AddBack( log.errs, 19 ), "Expected one of: ( ", 19 );
  For( i, 0, expected_len ) {
    Memmove( AddBack( log.errs, 1 ), "'", 1 );
    u8* exptype = StringOfTokenType( expected[i] );
    idx_t exptype_len = CstrLength( exptype );
    Memmove( AddBack( log.errs, exptype_len ), exptype, exptype_len );
    Memmove( AddBack( log.errs, 1 ), "'", 1 );
    if( i + 1 < expected_len ) {
      Memmove( AddBack( log.errs, 2 ), ", ", 2 );
    }
  }
  Memmove( AddBack( log.errs, 15 ), " ), but found '", 15 );
  Memmove( AddBack( log.errs, foundtype_len ), foundtype, foundtype_len );
  Memmove( AddBack( log.errs, 10 ), "' instead.", 10 );
  Memmove( AddBack( log.errs, 1 ), "\n", 1 );

  OutputSrcLineAndCaret( log.errs, src, tkn );
}

Inl void
ErrorIntraScopeNameConflict(
  token_t* tkn_i,
  token_t* tkn_j,
  src_t& src,
  log_t& log
  )
{
  log.nerrs += 1;

  OutputFileAndLine( src.filename, log.errs, tkn_j );

  u8* name = &src.file.mem[tkn_j->l];
  idx_t len = tkn_j->r - tkn_j->l;

  Memmove( AddBack( log.errs, 18 ), "Redeclaration of '", 18 );
  Memmove( AddBack( log.errs, len ), name, len );
  Memmove( AddBack( log.errs, 2 ), "'.", 2 );
  Memmove( AddBack( log.errs, 1 ), "\n", 1 );

  OutputSrcLineAndCaret( log.errs, src, tkn_i );
  OutputSrcLineAndCaret( log.errs, src, tkn_j );
}

#endif


struct
pcontext_t
{
  stack_resizeable_cont_t<token_t>* tokens;
  src_t* src;
  tlog_t* log;
  pagelist_t* mem;
};


Inl token_t*
ExpectTokenOfType(
  pcontext_t* context,
  tokentype_t type,
  idx_t* pos
  )
{
  auto log = context->log;
  auto tokens = context->tokens;
  auto src = context->src;

  if( *pos >= tokens->len ) {
    u8* str_type = StringOfTokenType( type );
    idx_t str_type_len = CstrLength( str_type );

    log->nerrs += 1;

    Memmove( AddBack( log->errs, 10 ), "Expected '", 10 );
    Memmove( AddBack( log->errs, str_type_len ), str_type, str_type_len );
    Memmove( AddBack( log->errs, 21 ), "', but hit EOF first.", 21 );
    Memmove( AddBack( log->errs, 1 ), "\n", 1 );
  }
  auto tkn = tokens->mem + *pos;
  if( tkn->type != type ) {
    u8* str_type = StringOfTokenType( type );
    idx_t str_type_len = CstrLength( str_type );
    u8* str_foundtype = StringOfTokenType( tkn->type );
    idx_t str_foundtype_len = CstrLength( str_foundtype );

    log->nerrs += 1;

    OutputFileAndLine( src->filename, log->errs, tkn );

    Memmove( AddBack( log->errs, 10 ), "Expected '", 10 );
    Memmove( AddBack( log->errs, str_type_len ), str_type, str_type_len );
    Memmove( AddBack( log->errs, 14 ), "', but found '", 14 );
    Memmove( AddBack( log->errs, str_foundtype_len ), str_foundtype, str_foundtype_len );
    Memmove( AddBack( log->errs, 10 ), "' instead.", 10 );
    Memmove( AddBack( log->errs, 1 ), "\n", 1 );

    OutputSrcLineAndCaret( log->errs, src, tkn );
    Memmove( AddBack( log->errs, 1 ), "\n", 1 );
  }
  *pos += 1;
  return tkn;
}

Inl token_t*
ExpectTokenOfType(
  pcontext_t* context,
  tokentype_t* types,
  idx_t types_len,
  idx_t* pos
  )
{
  auto log = context->log;
  auto tokens = context->tokens;
  auto src = context->src;

  if( *pos >= tokens->len ) {
    log->nerrs += 1;

    Memmove( AddBack( log->errs, 19 ), "Expected one of: ( ", 19 );
    For( i, 0, types_len ) {
      Memmove( AddBack( log->errs, 1 ), "'", 1 );
      u8* exptype = StringOfTokenType( types[i] );
      idx_t exptype_len = CstrLength( exptype );
      Memmove( AddBack( log->errs, exptype_len ), exptype, exptype_len );
      Memmove( AddBack( log->errs, 1 ), "'", 1 );
      if( i + 1 < types_len ) {
        Memmove( AddBack( log->errs, 2 ), ", ", 2 );
      }
    }
    Memmove( AddBack( log->errs, 21 ), ", but hit EOF first.", 21 );
    Memmove( AddBack( log->errs, 1 ), "\n", 1 );
  }
  auto tkn = tokens->mem + *pos;
  bool matched = TContains( types, types_len, &tkn->type );
  if( !matched ) {
    log->nerrs += 1;

    OutputFileAndLine( src->filename, log->errs, tkn );

    u8* foundtype = StringOfTokenType( tkn->type );
    idx_t foundtype_len = CstrLength( foundtype );

    Memmove( AddBack( log->errs, 19 ), "Expected one of: ( ", 19 );
    For( i, 0, types_len ) {
      Memmove( AddBack( log->errs, 1 ), "'", 1 );
      u8* exptype = StringOfTokenType( types[i] );
      idx_t exptype_len = CstrLength( exptype );
      Memmove( AddBack( log->errs, exptype_len ), exptype, exptype_len );
      Memmove( AddBack( log->errs, 1 ), "'", 1 );
      if( i + 1 < types_len ) {
        Memmove( AddBack( log->errs, 2 ), ", ", 2 );
      }
    }
    Memmove( AddBack( log->errs, 15 ), " ), but found '", 15 );
    Memmove( AddBack( log->errs, foundtype_len ), foundtype, foundtype_len );
    Memmove( AddBack( log->errs, 10 ), "' instead.", 10 );
    Memmove( AddBack( log->errs, 1 ), "\n", 1 );

    OutputSrcLineAndCaret( log->errs, src, tkn );
  }
  *pos += 1;
  return tkn;
}


Inl token_t*
ExpectToken(
  pcontext_t* context,
  idx_t pos
  )
{
  auto log = context->log;
  auto tokens = context->tokens;
  if( pos >= tokens->len ) {
    log->nerrs += 1;
    Memmove( AddBack( log->errs, 34 ), "Expected token, but hit EOF first.", 34 );
    Memmove( AddBack( log->errs, 1 ), "\n", 1 );
  }
  auto tkn = tokens->mem + pos;
  return tkn;
}




Inl void
Error(
  token_t* tkn,
  pcontext_t* context,
  void* errstr
  )
{
  Error( tkn, context->src, context->log, errstr );
}


Inl void
SkipSemicolons(
  pcontext_t* context,
  idx_t* pos
  )
{
  Forever {
    if( context->log->nerrs  ||  *pos >= context->tokens->len ) {
      break;
    }
    auto tkn = ExpectToken( context, *pos );
    if( tkn->type == tokentype_t::semicolon ) {
      *pos += 1;
    } else {
      break;
    }
  }
}




#define PARSE( type, name ) \
  Inl type* \
  name( \
    ast_t& ast, \
    pcontext_t* context, \
    idx_t* pos \
    )



PARSE( node_num_t, ParseNum )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_num_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectTokenOfType( context, tokentype_t::numeral, pos );
  node->literal = SymbolFromToken( node->tkn, context->src );
  return node;
}
PARSE( node_str_t, ParseStr )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_str_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectTokenOfType( context, tokentype_t::string_, pos );
  node->literal = SymbolFromToken( node->tkn, context->src );
  return node;
}
PARSE( node_chr_t, ParseChr )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_chr_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectTokenOfType( context, tokentype_t::char_, pos );
  node->literal = SymbolFromToken( node->tkn, context->src );
  return node;
}
PARSE( node_ident_t, ParseIdent )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_ident_t, _SIZEOF_IDX_T, 1 );
  Init( node );
  node->tkn = ExpectTokenOfType( context, tokentype_t::ident, pos );
  node->name = SymbolFromToken( node->tkn, context->src );
  return node;
}

PARSE( node_e0_t, ParseE0 );

PARSE( node_e1_t, ParseE1 )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_e1_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectToken( context, *pos );
  node->e0_l = ParseE0( ast, context, pos );
  auto tkn = ExpectToken( context, *pos );
  switch( tkn->type ) {
    case tokentype_t::star    : { node->e1type = e1type_t::mul; } break;
    case tokentype_t::slash   : { node->e1type = e1type_t::div; } break;
    case tokentype_t::percent : { node->e1type = e1type_t::rem; } break;
    default                   : { node->e1type = e1type_t::none; } break;
  }
  if( node->e1type != e1type_t::none ) {
    *pos += 1;
    node->e0_r = ParseE0( ast, context, pos );
  }
  return node;
}
PARSE( node_e2_t, ParseE2 )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_e2_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectToken( context, *pos );
  node->e1_l = ParseE1( ast, context, pos );
  auto tkn = ExpectToken( context, *pos );
  switch( tkn->type ) {
    case tokentype_t::plus : { node->e2type = e2type_t::add;  } break;
    case tokentype_t::minus: { node->e2type = e2type_t::sub;  } break;
    default                : { node->e2type = e2type_t::none; } break;
  }
  if( node->e2type != e2type_t::none ) {
    *pos += 1;
    node->e1_r = ParseE1( ast, context, pos );
  }
  return node;
}
PARSE( node_e3_t, ParseE3 )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_e3_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectToken( context, *pos );
  node->e2_l = ParseE2( ast, context, pos );
  auto tkn = ExpectToken( context, *pos );
  switch( tkn->type ) {
    case tokentype_t::ampersand: { node->e3type = e3type_t::and_; } break;
    case tokentype_t::pipe     : { node->e3type = e3type_t::or_ ; } break;
    case tokentype_t::caret    : { node->e3type = e3type_t::xor_; } break;
    default                    : { node->e3type = e3type_t::none; } break;
  }
  if( node->e3type != e3type_t::none ) {
    *pos += 1;
    node->e2_r = ParseE2( ast, context, pos );
  }
  return node;
}
PARSE( node_e4_t, ParseE4 )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_e4_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectToken( context, *pos );
  node->e3_l = ParseE3( ast, context, pos );
  auto tkn = ExpectToken( context, *pos );
  switch( tkn->type ) {
    case tokentype_t::ltlt: { node->e4type = e4type_t::shl;  } break;
    case tokentype_t::gtgt: { node->e4type = e4type_t::shr;  } break;
    default               : { node->e4type = e4type_t::none; } break;
  }
  if( node->e4type != e4type_t::none ) {
    *pos += 1;
    node->e3_r = ParseE3( ast, context, pos );
  }
  return node;
}
PARSE( node_e5_t, ParseE5 )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_e5_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectToken( context, *pos );
  node->e4_l = ParseE4( ast, context, pos );
  auto tkn = ExpectToken( context, *pos );
  switch( tkn->type ) {
    case tokentype_t::eqeq         : { node->e5type = e5type_t::eq   ; } break;
    case tokentype_t::exclamationeq: { node->e5type = e5type_t::noteq; } break;
    case tokentype_t::gt           : { node->e5type = e5type_t::gt   ; } break;
    case tokentype_t::gteq         : { node->e5type = e5type_t::gte  ; } break;
    case tokentype_t::lt           : { node->e5type = e5type_t::lt   ; } break;
    case tokentype_t::lteq         : { node->e5type = e5type_t::lte  ; } break;
    default                        : { node->e5type = e5type_t::none ; } break;
  }
  if( node->e5type != e5type_t::none ) {
    *pos += 1;
    node->e4_r = ParseE4( ast, context, pos );
  }
  return node;
}
PARSE( node_funccall_t, ParseFunccall )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_funccall_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectToken( context, *pos );
  node->ident = ParseIdent( ast, context, pos );
  ExpectTokenOfType( context, tokentype_t::paren_l, pos );
  bool loop = 1;
  while( loop ) {
    auto tkn = ExpectToken( context, *pos );
    if( tkn->type == tokentype_t::paren_r ) {
      *pos += 1;
      break;
    }
    *AddBack( node->e5_args ) = ParseE5( ast, context, pos );
    tkn = ExpectToken( context, *pos );
    switch( tkn->type ) {
      case tokentype_t::semicolon:
      case tokentype_t::comma: {
        *pos += 1;
      } break;

      case tokentype_t::paren_r: {
        *pos += 1;
        loop = 0;
      } break;

      default: {
        Error( node->tkn, context, "Bad function-call!" );
        loop = 0;
      } break;
    }
  }
  return node;
}
PARSE( node_identdot_t, ParseIdentdot )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_identdot_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectToken( context, *pos );
  *AddBack( node->idents ) = ParseIdent( ast, context, pos );
  Forever {
    auto tkn = ExpectToken( context, *pos );
    if( tkn->type != tokentype_t::dot ) {
      break;
    }
    *pos += 1;
    *AddBack( node->idents ) = ParseIdent( ast, context, pos );
  }
  return node;
}
PARSE( node_unop_t, ParseUnop )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_unop_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  static tokentype_t expected[] = {
    tokentype_t::star       ,
    tokentype_t::ampersand  ,
    tokentype_t::minus      ,
    tokentype_t::exclamation,
  };
  node->tkn = ExpectTokenOfType( context, AL( expected ), pos );
  switch( node->tkn->type ) {
    case tokentype_t::star       : { node->unoptype = unoptype_t::deref ; } break;
    case tokentype_t::ampersand  : { node->unoptype = unoptype_t::addrof; } break;
    case tokentype_t::minus      : { node->unoptype = unoptype_t::negate; } break;
    case tokentype_t::exclamation: { node->unoptype = unoptype_t::not_  ; } break;
  }
  node->e4 = ParseE4( ast, context, pos );
  return node;
}
PARSE( node_e0_t, ParseE0 )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_e0_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectToken( context, *pos );
  switch( node->tkn->type ) {
    case tokentype_t::numeral: {
      node->e0type = e0type_t::num;
      node->num = ParseNum( ast, context, pos );
    } break;

    case tokentype_t::string_: {
      node->e0type = e0type_t::str;
      node->str = ParseStr( ast, context, pos );
    } break;

    case tokentype_t::char_: {
      node->e0type = e0type_t::chr;
      node->chr = ParseChr( ast, context, pos );
    } break;

    case tokentype_t::paren_l: {
      *pos += 1;
      node->e0type = e0type_t::e5s;
      Alloc( node->e5s, 16 );
      *AddBack( node->e5s ) = ParseE5( ast, context, pos );
      bool loop = 1;
      while( loop ) {
        static tokentype_t expected[] = {
          tokentype_t::paren_r  ,
          tokentype_t::semicolon,
          tokentype_t::comma    ,
        };
        auto tkn = ExpectTokenOfType( context, AL( expected ), pos );
        switch( tkn->type ) {
          case tokentype_t::paren_r  : {
            loop = 0;
          } break;

          case tokentype_t::semicolon:
          case tokentype_t::comma    : {
            *AddBack( node->e5s ) = ParseE5( ast, context, pos );
          } break;

          default: {
            Error( node->tkn, context, "Bad e5 expression-list!" );
            loop = 0;
          } break;
        }
      }
    } break;

    case tokentype_t::star       :
    case tokentype_t::ampersand  :
    case tokentype_t::minus      :
    case tokentype_t::exclamation: {
      node->e0type = e0type_t::unop;
      node->unop = ParseUnop( ast, context, pos );
    } break;

    case tokentype_t::ident: {
      auto tkn = ExpectToken( context, *pos + 1 );
      if( tkn->type == tokentype_t::paren_l ) {
        node->e0type = e0type_t::funccall;
        node->funccall = ParseFunccall( ast, context, pos );
      } else {
        node->e0type = e0type_t::identdot;
        node->identdot = ParseIdentdot( ast, context, pos );
      }
    } break;
  }
  return node;
}
PARSE( node_type_t, ParseType )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_type_t, _SIZEOF_IDX_T, 1 );
  Init( node );
  node->tkn = ExpectToken( context, *pos );
  node->ptrlevel = 0;
  node->ident = 0;
  bool loop = 1;
  while( loop ) {
    auto tkn = ExpectToken( context, *pos );
    switch( tkn->type ) {
      case tokentype_t::star: {
        node->ptrlevel += 1;
        *pos += 1;
      } break;

      case tokentype_t::ident: {
        node->ident = ParseIdent( ast, context, pos );
        loop = 0;
      } break;

      default: {
        Error( node->tkn, context, "Bad type!" );
        loop = 0;
      } break;
    }
  }
  return node;
}
PARSE( node_decl_t, ParseDecl )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_decl_t, _SIZEOF_IDX_T, 1 );
  Init( node );
  node->tkn = ExpectToken( context, *pos );
  node->ident = ParseIdent( ast, context, pos );
  node->type = ParseType( ast, context, pos );
  auto tkn = ExpectToken( context, *pos );
  if( tkn->type == tokentype_t::eq ) {
    *pos += 1;
    node->e5 = ParseE5( ast, context, pos );
  }
  return node;
}

PARSE( node_scope_t, ParseScope );

PARSE( node_funcdeclordefn_t, ParseFuncdeclordefn )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_funcdeclordefn_t, _SIZEOF_IDX_T, 1 );
  Init( node );
  node->tkn = ExpectTokenOfType( context, tokentype_t::fn, pos );
  node->ident = ParseIdent( ast, context, pos );
  ExpectTokenOfType( context, tokentype_t::paren_l, pos );
  bool loop = 1;
  while( loop ) {
    auto tkn = ExpectToken( context, *pos );
    if( tkn->type == tokentype_t::paren_r ) {
      *pos += 1;
      break;
    }
    *AddBack( node->decls ) = ParseDecl( ast, context, pos );
    tkn = ExpectToken( context, *pos );
    switch( tkn->type ) {
      case tokentype_t::semicolon:
      case tokentype_t::comma: {
        *pos += 1;
      } break;

      case tokentype_t::paren_r: {
        *pos += 1;
        loop = 0;
      } break;

      default: {
        Error( node->tkn, context, "Bad function-decl or function-defn!" );
        loop = 0;
      } break;
    }
  }
  auto tkn = ExpectToken( context, *pos );
  switch( tkn->type ) {
    case tokentype_t::star:
    case tokentype_t::ident: {
      node->type = ParseType( ast, context, pos );
      tkn = ExpectToken( context, *pos );
    } break;
  }
  switch( tkn->type ) {
    case tokentype_t::bracket_curly_l:
    case tokentype_t::paren_l: {
      node->scope = ParseScope( ast, context, pos );
    } break;
  }
  return node;
}
PARSE( node_datadecl_t, ParseDatadecl )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_datadecl_t, _SIZEOF_IDX_T, 1 );
  Init( node );
  node->tkn = ExpectTokenOfType( context, tokentype_t::data, pos );
  node->ident = ParseIdent( ast, context, pos );
  ExpectTokenOfType( context, tokentype_t::paren_l, pos );
  bool loop = 1;
  while( loop ) {
    auto tkn = ExpectToken( context, *pos );
    if( tkn->type == tokentype_t::paren_r ) {
      *pos += 1;
      break;
    }
    *AddBack( node->decls ) = ParseDecl( ast, context, pos );
    tkn = ExpectToken( context, *pos );
    switch( tkn->type ) {
      case tokentype_t::semicolon:
      case tokentype_t::comma: {
        *pos += 1;
      } break;

      case tokentype_t::paren_r: {
        *pos += 1;
        loop = 0;
      } break;

      default: {
        Error( node->tkn, context, "Bad data-decl!" );
        loop = 0;
      } break;
    }
  }
  return node;
}

tokentype_t tkns_binassign[] = {
  tokentype_t::eq         ,
  tokentype_t::pluseq     ,
  tokentype_t::minuseq    ,
  tokentype_t::stareq     ,
  tokentype_t::slasheq    ,
  tokentype_t::percenteq  ,
  tokentype_t::ampersandeq,
  tokentype_t::pipeeq     ,
  tokentype_t::careteq    ,
  tokentype_t::ltlteq     ,
  tokentype_t::gtgteq     ,
};

PARSE( node_binassign_t, ParseBinassign )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_binassign_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectToken( context, *pos );
  node->e0_l = ParseE0( ast, context, pos );

  auto tkn = ExpectTokenOfType( context, AL( tkns_binassign ), pos );
  switch( tkn->type ) {
    case tokentype_t::eq         : { node->binassigntype = binassigntype_t::eq   ; } break;
    case tokentype_t::pluseq     : { node->binassigntype = binassigntype_t::addeq; } break;
    case tokentype_t::minuseq    : { node->binassigntype = binassigntype_t::subeq; } break;
    case tokentype_t::stareq     : { node->binassigntype = binassigntype_t::muleq; } break;
    case tokentype_t::slasheq    : { node->binassigntype = binassigntype_t::diveq; } break;
    case tokentype_t::percenteq  : { node->binassigntype = binassigntype_t::remeq; } break;
    case tokentype_t::ampersandeq: { node->binassigntype = binassigntype_t::andeq; } break;
    case tokentype_t::pipeeq     : { node->binassigntype = binassigntype_t::oreq ; } break;
    case tokentype_t::careteq    : { node->binassigntype = binassigntype_t::xoreq; } break;
    case tokentype_t::ltlteq     : { node->binassigntype = binassigntype_t::shleq; } break;
    case tokentype_t::gtgteq     : { node->binassigntype = binassigntype_t::shreq; } break;
  }
  node->e5_r = ParseE5( ast, context, pos );
  return node;
}
PARSE( node_ret_t, ParseRet )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_ret_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectTokenOfType( context, tokentype_t::return_, pos );
  node->e5 = ParseE5( ast, context, pos );
  return node;
}
PARSE( node_continue_t, ParseContinue )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_continue_t, _SIZEOF_IDX_T, 1 );
  Init( node );
  node->tkn = ExpectTokenOfType( context, tokentype_t::continue_, pos );
  return node;
}
PARSE( node_break_t, ParseBreak )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_break_t, _SIZEOF_IDX_T, 1 );
  Init( node );
  node->tkn = ExpectTokenOfType( context, tokentype_t::break_, pos );
  return node;
}
PARSE( node_while_t, ParseWhile )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_while_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectTokenOfType( context, tokentype_t::while_, pos );
  node->e5 = ParseE5( ast, context, pos );
  node->scope = ParseScope( ast, context, pos );
  return node;
}
PARSE( node_ifchain_t, ParseIfchain )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_ifchain_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectTokenOfType( context, tokentype_t::if_, pos );
  node->if_e5 = ParseE5( ast, context, pos );
  node->if_scope = ParseScope( ast, context, pos );
  token_t* tkn = 0;
  Forever {
    tkn = ExpectToken( context, *pos );
    if( tkn->type != tokentype_t::elif_ ) {
      break;
    }
    *pos += 1;
    *AddBack( node->elif_e5s ) = ParseE5( ast, context, pos );
    *AddBack( node->elif_scopes ) = ParseScope( ast, context, pos );
  }
  if( tkn->type == tokentype_t::else_ ) {
    *pos += 1;
    node->else_scope = ParseScope( ast, context, pos );
  }
  return node;
}
PARSE( node_switch_t, ParseSwitch )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_switch_t, _SIZEOF_IDX_T, 1 );
  Init( node );
  node->tkn = ExpectTokenOfType( context, tokentype_t::switch_, pos );
  node->e5 = ParseE5( ast, context, pos );
  node->scope = ParseScope( ast, context, pos );
  return node;
}
PARSE( node_case_t, ParseCase )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_case_t, _SIZEOF_IDX_T, 1 );
  Init( node );
  node->tkn = ExpectTokenOfType( context, tokentype_t::case_, pos );
  node->e5 = ParseE5( ast, context, pos );
  node->scope = ParseScope( ast, context, pos );
  return node;
}
PARSE( node_default_t, ParseDefault )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_default_t, _SIZEOF_IDX_T, 1 );
  Init( node );
  node->tkn = ExpectTokenOfType( context, tokentype_t::default_, pos );
  node->scope = ParseScope( ast, context, pos );
  return node;
}

PARSE( node_statement_t, ParseStatement );

PARSE( node_defer_t, ParseDefer )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_defer_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectTokenOfType( context, tokentype_t::defer, pos );
  node->statement = ParseStatement( ast, context, pos );
  switch( node->statement->stmtype ) {
    case stmtype_t::ret:
    case stmtype_t::funccall:
    case stmtype_t::binassign: {
    } break;

    default: {
      Error( node->statement->tkn, context, "can't defer that statement type!" );
    } break;
  }
  return node;
}
PARSE( node_statement_t, ParseStatement )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_statement_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  node->tkn = ExpectToken( context, *pos );
  switch( node->tkn->type ) {
    case tokentype_t::fn: {
      node->stmtype = stmtype_t::funcdeclordefn;
      node->funcdeclordefn = ParseFuncdeclordefn( ast, context, pos );
    } break;

    case tokentype_t::data: {
      node->stmtype = stmtype_t::datadecl;
      node->datadecl = ParseDatadecl( ast, context, pos );
    } break;

    case tokentype_t::return_: {
      node->stmtype = stmtype_t::ret;
      node->ret = ParseRet( ast, context, pos );
    } break;

    case tokentype_t::continue_: {
      node->stmtype = stmtype_t::continue_;
      node->continue_ = ParseContinue( ast, context, pos );
    } break;

    case tokentype_t::break_: {
      node->stmtype = stmtype_t::break_;
      node->break_ = ParseBreak( ast, context, pos );
    } break;

    case tokentype_t::while_: {
      node->stmtype = stmtype_t::while_;
      node->while_ = ParseWhile( ast, context, pos );
    } break;

    case tokentype_t::if_: {
      node->stmtype = stmtype_t::ifchain;
      node->ifchain = ParseIfchain( ast, context, pos );
    } break;

    case tokentype_t::switch_: {
      node->stmtype = stmtype_t::switch_;
      node->switch_ = ParseSwitch( ast, context, pos );
    } break;

    case tokentype_t::case_: {
      node->stmtype = stmtype_t::case_;
      node->case_ = ParseCase( ast, context, pos );
    } break;

    case tokentype_t::default_: {
      node->stmtype = stmtype_t::default_;
      node->default_ = ParseDefault( ast, context, pos );
    } break;

    case tokentype_t::defer: {
      node->stmtype = stmtype_t::defer;
      node->defer = ParseDefer( ast, context, pos );
    } break;

    case tokentype_t::bracket_curly_l: {
      node->stmtype = stmtype_t::scope;
      node->scope = ParseScope( ast, context, pos );
    } break;

    case tokentype_t::ident: {
      auto tkn = ExpectToken( context, *pos + 1 );
      if( tkn->type == tokentype_t::paren_l ) {
        node->stmtype = stmtype_t::funccall;
        node->funccall = ParseFunccall( ast, context, pos );
      } else {
        if( TContains( AL( tkns_binassign ), &tkn->type ) ) {
          node->stmtype = stmtype_t::binassign;
          node->binassign = ParseBinassign( ast, context, pos );
        } else {
          node->stmtype = stmtype_t::decl;
          node->decl = ParseDecl( ast, context, pos );
        }
      }
    } break;

    default: {
      node->stmtype = stmtype_t::binassign;
      node->binassign = ParseBinassign( ast, context, pos );
    } break;
  }
  return node;
}
PARSE( node_scope_t, ParseScope )
{
  if( context->log->nerrs ) return 0;
  auto node = AddPagelist( *context->mem, node_scope_t, _SIZEOF_IDX_T, 1 );
  Init( node, context->mem );
  static tokentype_t expected_l[] = {
    tokentype_t::bracket_curly_l,
    tokentype_t::paren_l,
  };
  node->tkn = ExpectTokenOfType( context, AL( expected_l ), pos );
  bool loop = 1;
  while( loop ) {
    if( context->log->nerrs ) {
      break;
    }
    auto tkn = ExpectToken( context, *pos );
    switch( tkn->type ) {
      case tokentype_t::bracket_curly_r:
      case tokentype_t::paren_r: {
        *pos += 1;
        loop = 0;
      } break;
    }
    if( loop ) {
      SkipSemicolons( context, pos );
      *AddBack( node->statements ) = ParseStatement( ast, context, pos );
      SkipSemicolons( context, pos );
    }
  }
  return node;
}




void
Parse(
  ast_t& ast,
  pcontext_t* context
  )
{
  idx_t pos = 0;

  auto toplevel = AddPagelist( *context->mem, node_scope_t, _SIZEOF_IDX_T, 1 );
  Init( toplevel, context->mem );
  Forever {
    if( context->log->nerrs  ||  pos >= context->tokens->len ) {
      break;
    }
    SkipSemicolons( context, &pos );
    *AddBack( toplevel->statements ) = ParseStatement( ast, context, &pos );
    SkipSemicolons( context, &pos );
  }
  ast.scope_toplevel = toplevel;
}




#if 0

#define AST_OP( name )   void ( name )( ast_t& ast, src_t& src, nodetype_t* elem, tlog_t& log )
typedef AST_OP( *pfn_ast_op_t );

void
AstOpPerNode(
  ast_t& ast,
  src_t& src,
  void* elem,
  pfn_ast_op_t* funcs,
  idx_t nfuncs,
  tlog_t& log
  )
{
  auto& type = *Cast( nodetype_t*, elem );
  switch( type ) {
    case nodetype_t::literalnum: {
      //auto& node = *Cast( node_literalnum_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
    } break;

    case nodetype_t::literalstring: {
      //auto& node = *Cast( node_literalstring_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
    } break;

    case nodetype_t::literalchar: {
      //auto& node = *Cast( node_literalchar_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
    } break;

    case nodetype_t::ident: {
      //auto& node = *Cast( node_ident_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
    } break;

    case nodetype_t::unop: {
      auto& node = *Cast( node_unop_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
      AstOpPerNode( ast, src, node.varg0, funcs, nfuncs, log );
    } break;

    case nodetype_t::binop: {
      auto& node = *Cast( node_binop_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
      AstOpPerNode( ast, src, node.varg0, funcs, nfuncs, log );
      AstOpPerNode( ast, src, node.varg1, funcs, nfuncs, log );
    } break;

    case nodetype_t::funccall: {
      auto& node = *Cast( node_funccall_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
      AstOpPerNode( ast, src, node.ident, funcs, nfuncs, log );
      ForLen( i, node.vargs ) {
        AstOpPerNode( ast, src, node.vargs.mem[i], funcs, nfuncs, log );
      }
    } break;

    case nodetype_t::decl: {
      auto& node = *Cast( node_decl_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
      AstOpPerNode( ast, src, node.ident, funcs, nfuncs, log );
      AstOpPerNode( ast, src, node.vtype, funcs, nfuncs, log );
    } break;

    case nodetype_t::declassignexpr: {
      auto& node = *Cast( node_declassignexpr_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
      AstOpPerNode( ast, src, node.ident, funcs, nfuncs, log );
      AstOpPerNode( ast, src, node.vtype, funcs, nfuncs, log );
      AstOpPerNode( ast, src, node.vexpr, funcs, nfuncs, log );
    } break;

    case nodetype_t::declassignscope: {
      auto& node = *Cast( node_declassignscope_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
      AstOpPerNode( ast, src, node.ident, funcs, nfuncs, log );
      AstOpPerNode( ast, src, node.vtype, funcs, nfuncs, log );
      AstOpPerNode( ast, src, node.scope, funcs, nfuncs, log );
    } break;

    case nodetype_t::scope: {
      auto& node = *Cast( node_scope_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
      if( node.vstatements.len ) {
        ForLen( i, node.vstatements ) {
          AstOpPerNode( ast, src, node.vstatements.mem[i], funcs, nfuncs, log );
        }
      }
    } break;

    case nodetype_t::func: {
      auto& node = *Cast( node_typefunc_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
      // node.ptrlevel_return
      AstOpPerNode( ast, src, node.typenonfunc_return, funcs, nfuncs, log );
      AssertCrash( node.ident_args.len == node.vtype_args.len );
      ForLen( i, node.ident_args ) {
        AstOpPerNode( ast, src, node.ident_args.mem[i], funcs, nfuncs, log );
        AstOpPerNode( ast, src, node.vtype_args.mem[i], funcs, nfuncs, log );
      }
    } break;

    case nodetype_t::nonfunc: {
      //auto& node = *Cast( node_typenonfunc_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
    } break;

    case nodetype_t::return_: {
      auto& node = *Cast( node_return_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
      AstOpPerNode( ast, src, node.vexpr, funcs, nfuncs, log );
    } break;

    case nodetype_t::continue_: {
      //auto& node = *Cast( node_continue_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
    } break;

    case nodetype_t::break_: {
      //auto& node = *Cast( node_break_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
    } break;

    case nodetype_t::while_: {
      auto& node = *Cast( node_while_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
      AstOpPerNode( ast, src, node.vexpr, funcs, nfuncs, log );
      AstOpPerNode( ast, src, node.scope, funcs, nfuncs, log );
    } break;

    case nodetype_t::ifchain: {
      auto& node = *Cast( node_ifchain_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
      AstOpPerNode( ast, src, node.vexpr_if, funcs, nfuncs, log );
      AstOpPerNode( ast, src, node.scope_if, funcs, nfuncs, log );
      if( node.scope_elifs.len ) {
        AssertCrash( node.scope_elifs.len == node.vexpr_elifs_.len );
        ForLen( i, node.scope_elifs ) {
          auto vexpr_elif = node.vexpr_elifs_.mem[i];
          auto scope_elif = node.scope_elifs.mem[i];
          AstOpPerNode( ast, src, vexpr_elif, funcs, nfuncs, log );
          AstOpPerNode( ast, src, scope_elif, funcs, nfuncs, log );
        }
      }
      if( node.has_else ) {
        AstOpPerNode( ast, src, node.scope_else, funcs, nfuncs, log );
      }
    } break;

    case nodetype_t::binassign: {
      auto& node = *Cast( node_binassign_t*, elem );
      For( i, 0, nfuncs ) {
        funcs[i]( ast, src, &type, log );
      }
      AstOpPerNode( ast, src, node.vexpr_l, funcs, nfuncs, log );
      AstOpPerNode( ast, src, node.vexpr_r, funcs, nfuncs, log );
    } break;

    default: UnreachableCrash();
  }
}

#endif

