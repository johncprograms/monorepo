// Copyright (c) John A. Carlos Jr., all rights reserved.

struct
ccontext_t
{
  ast_t* ast;
  src_t* src;
  tlog_t* log;
  plist_t* mem;
  u32 ptr_bitcount;
};

void
Error(
  token_t* tkn,
  ccontext_t* context,
  void* errstr
  )
{
  Error( tkn, context->src, context->log, errstr );
}








Templ Inl T*
_AddCode( array_t<code_t>& code )
{
  auto res = Cast( T*, AddBack( code ) );
  Init( res );
  return res;
}

#define AddCode( type, code ) \
  _AddCode<type>( code )


// keep a unique id for every code_var_t, just to make debugging/visualization a little easier.
idx_t g_code_var_id = 0;

Inl
code_var_t*
AddVar(
  ccontext_t* context,
  array_t<code_t>& code,
  node_scope_t* scope,
  ptrtype_t* ptrtype
  )
{
  auto res = Cast( code_var_t*, AddBack( code ) );
  Init( res );
  res->id = g_code_var_id++;
  SizeinfoFromPtrtype( &res->sizeinfo, ptrtype, context->ptr_bitcount );
  res->offset_into_scope = scope->vars_bytecount_top;
  scope->vars_bytecount_top += res->sizeinfo.bitcount / 8;
  return res;
}




#define GEN0( type, name ) \
  Inl void \
  name( \
    ccontext_t* context, \
    array_t<node_scope_t*>& scopestack, \
    type* node, \
    array_t<code_t>& code \
    )

#define GEN1( type, name ) \
  Inl code_var_t* \
  name( \
    ccontext_t* context, \
    array_t<node_scope_t*>& scopestack, \
    type* node, \
    array_t<code_t>& code \
    )


Inl code_var_t*
GenNum(
  ccontext_t* context,
  array_t<node_scope_t*>& scopestack,
  node_num_t* node,
  ptrtype_t* ptrtype,
  array_t<code_t>& code
  )
{
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  auto var_result = AddVar( context, code, scope, ptrtype );
  SizeinfoFromPtrtype( &node->sizeinfo, ptrtype, context->ptr_bitcount );
  auto moveimm = AddCode( code_moveimmediate_t, code );
  moveimm->var_l = var_result;
  moveimm->node_r = node;
  return var_result;
}
GEN1( node_str_t, GenStr )
{
  ImplementCrash();
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  auto var_result = AddVar( context, code, scope, 0 );
  return var_result;
}
GEN1( node_chr_t, GenChr )
{
  ImplementCrash();
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  auto var_result = AddVar( context, code, scope, 0 );
  return var_result;
}

GEN1( node_e0_t, GenE0 );

GEN1( node_e1_t, GenE1 )
{
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  if( node->e1type == e1type_t::none ) {
    return GenE0( context, scopestack, node->e0_l, code );
  } else {
    auto var_l = GenE0( context, scopestack, node->e0_l, code );
    auto var_r = GenE0( context, scopestack, node->e0_r, code );
    auto var_result = AddVar( context, code, scope, &node->ptrtypes.first->value );
    auto binop = AddCode( code_binop_t, code );
    switch( node->e1type ) {
      case e1type_t::mul: { binop->binoptype = binoptype_t::mul; } break;
      case e1type_t::div: { binop->binoptype = binoptype_t::div; } break;
      case e1type_t::rem: { binop->binoptype = binoptype_t::rem; } break;
      default: UnreachableCrash();
    }
    binop->var_r = var_r;
    binop->var_l = var_l;
    binop->var_result = var_result;
    return var_result;
  }
}
GEN1( node_e2_t, GenE2 )
{
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  if( node->e2type == e2type_t::none ) {
    return GenE1( context, scopestack, node->e1_l, code );
  } else {
    auto var_l = GenE1( context, scopestack, node->e1_l, code );
    auto var_r = GenE1( context, scopestack, node->e1_r, code );
    auto var_result = AddVar( context, code, scope, &node->ptrtypes.first->value );
    auto binop = AddCode( code_binop_t, code );
    switch( node->e2type ) {
      case e2type_t::add: { binop->binoptype = binoptype_t::add; } break;
      case e2type_t::sub: { binop->binoptype = binoptype_t::sub; } break;
      default: UnreachableCrash();
    }
    binop->var_r = var_r;
    binop->var_l = var_l;
    binop->var_result = var_result;
    return var_result;
  }
}
GEN1( node_e3_t, GenE3 )
{
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  if( node->e3type == e3type_t::none ) {
    return GenE2( context, scopestack, node->e2_l, code );
  } else {
    auto var_l = GenE2( context, scopestack, node->e2_l, code );
    auto var_r = GenE2( context, scopestack, node->e2_r, code );
    auto var_result = AddVar( context, code, scope, &node->ptrtypes.first->value );
    auto binop = AddCode( code_binop_t, code );
    switch( node->e3type ) {
      case e3type_t::and_: { binop->binoptype = binoptype_t::and_; } break;
      case e3type_t::or_ : { binop->binoptype = binoptype_t::or_ ; } break;
      case e3type_t::xor_: { binop->binoptype = binoptype_t::xor_; } break;
      default: UnreachableCrash();
    }
    binop->var_r = var_r;
    binop->var_l = var_l;
    binop->var_result = var_result;
    return var_result;
  }
}
GEN1( node_e4_t, GenE4 )
{
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  if( node->e4type == e4type_t::none ) {
    return GenE3( context, scopestack, node->e3_l, code );
  } else {
    auto var_l = GenE3( context, scopestack, node->e3_l, code );
    auto var_r = GenE3( context, scopestack, node->e3_r, code );
    auto var_result = AddVar( context, code, scope, &node->ptrtypes.first->value );
    auto binop = AddCode( code_binop_t, code );
    switch( node->e4type ) {
      case e4type_t::shl: { binop->binoptype = binoptype_t::shl; } break;
      case e4type_t::shr: { binop->binoptype = binoptype_t::shr; } break;
      default: UnreachableCrash();
    }
    binop->var_r = var_r;
    binop->var_l = var_l;
    binop->var_result = var_result;
    return var_result;
  }
}
GEN1( node_e5_t, GenE5 )
{
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  if( node->e5type == e5type_t::none ) {
    return GenE4( context, scopestack, node->e4_l, code );
  } else {
    auto var_l = GenE4( context, scopestack, node->e4_l, code );
    auto var_r = GenE4( context, scopestack, node->e4_r, code );
    auto var_result = AddVar( context, code, scope, &node->ptrtypes.first->value );
    auto binop = AddCode( code_binop_t, code );
    switch( node->e5type ) {
      case e5type_t::eq   : { binop->binoptype = binoptype_t::eq   ; } break;
      case e5type_t::noteq: { binop->binoptype = binoptype_t::noteq; } break;
      case e5type_t::gt   : { binop->binoptype = binoptype_t::gt   ; } break;
      case e5type_t::gte  : { binop->binoptype = binoptype_t::gte  ; } break;
      case e5type_t::lt   : { binop->binoptype = binoptype_t::lt   ; } break;
      case e5type_t::lte  : { binop->binoptype = binoptype_t::lte  ; } break;
      default: UnreachableCrash();
    }
    binop->var_r = var_r;
    binop->var_l = var_l;
    binop->var_result = var_result;
    return var_result;
  }
}

Inl varentry_t*
LookupIdent(
  ccontext_t* context,
  array_t<node_scope_t*>& scopestack,
  symbol_t* name
  )
{
  varentry_t* entry = 0;
  auto top = scopestack.len;
  bool found = 0;
  while( top-- ) {
    auto scope = scopestack.mem[top];
    LookupRaw( scope->vartable, name, &found, Cast( void**, &entry ) );
    if( found ) {
      break;
    }
  }
  AssertCrash( found ); // already validated in typechecking.
  return entry;
}

GEN1( node_funccall_t, GenFunccall )
{
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  code_var_t* var_result = 0;

  // we allow len=0 to mean a void-retval.
  if( node->ptrtypes.len > 1 ) {
    Error( node->tkn, context, "ambiguous resolved type!" );
    return var_result;
  }

  if( node->ptrtypes.len ) {
    var_result = AddVar( context, code, scope, &node->ptrtypes.first->value );
  }

  auto varentry_fn = LookupIdent( context, scopestack, &node->ident->name );

  array_t<code_var_t*> var_args;
  Alloc( var_args, node->e5_args.len );
  ForLen( i, node->e5_args ) {
    auto e5_arg = node->e5_args.mem[i];
    auto var_arg = GenE5( context, scopestack, e5_arg, code );
    *AddBack( var_args ) = var_arg;
  }
  auto funccall = AddCode( code_funccall_t, code );
  funccall->var_args = var_args;
  funccall->var_result = var_result;
  funccall->funcdefnstart_offset = varentry_fn->funcdefnstart_offset;
  return var_result;
}
GEN1( node_identdot_t, GenIdentdot )
{
  // TODO: multi-level identdot.
  AssertCrash( node->idents.len <= 2 ); // validated in typechecking.

  if( node->idents.len == 1 ) {
    auto varentry_result = LookupIdent( context, scopestack, &node->idents.mem[0]->name );
    return varentry_result->var;
  } else {
    // TODO: struct deref
    ImplementCrash();
    auto scope = scopestack.mem[ scopestack.len - 1 ];
    auto var = AddVar( context, code, scope, 0 );
    return var;
  }
}
GEN1( node_unop_t, GenUnop )
{
  auto unop = AddCode( code_unop_t, code );
  unop->var = GenE4( context, scopestack, node->e4, code );
  if( node->ptrtypes.len != 1 ) {
    Error( node->tkn, context, "ambiguous resolved type!" );
  }
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  auto result = AddVar( context, code, scope, &node->ptrtypes.first->value );
  //unop->unoptype =
  ImplementCrash();
  return result;
}
GEN1( node_e0_t, GenE0 )
{
  switch( node->e0type ) {
    case e0type_t::funccall: {
      return GenFunccall( context, scopestack, node->funccall, code );
    } break;

    case e0type_t::identdot: {
      return GenIdentdot( context, scopestack, node->identdot, code );
    } break;

    case e0type_t::e5s: {
      // TODO: generalize ptrtype_t to array_t<ptrtype_t>, so we can pass around tuples.
      if( node->e5s.len != 1 ) {
        Error( node->tkn, context, "expected exactly one e5 inside parens!" );
      }
      return GenE5( context, scopestack, node->e5s.mem[0], code );
    } break;

    case e0type_t::unop: {
      return GenUnop( context, scopestack, node->unop, code );
    } break;

    case e0type_t::num: {
      if( node->ptrtypes.len != 1 ) {
        Error( node->tkn, context, "ambiguous resolved type!" );
      }
      AssertCrash( node->ptrtypes.len );
      return GenNum( context, scopestack, node->num, &node->ptrtypes.first->value, code );
    } break;

    case e0type_t::str: {
      return GenStr( context, scopestack, node->str, code );
    } break;

    case e0type_t::chr: {
      return GenChr( context, scopestack, node->chr, code );
    } break;

    default: UnreachableCrash();
  }

  UnreachableCrash();
  return {};
}
GEN0( node_type_t, GenType )
{
  AssertCrash( 0 ); // should have already done this work in TypeType
  //SizeinfoFromPtrtype( &node->sizeinfo, &node->ptrtype, context->ptr_bitcount );
}
GEN0( node_decl_t, GenDecl )
{
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  if( node->e5 ) {
    bool found;
    varentry_t* entry;
    // TODO: should this lookup recursively?
    LookupRaw( scope->vartable, &node->ident->name, &found, Cast( void**, &entry ) );
    AssertCrash( found );
    AssertCrash( entry->var ); // GenScope should have already pre-declared a var for us from the vartable.

    auto var_e5 = GenE5( context, scopestack, node->e5, code );
    auto move = AddCode( code_move_t, code );
    move->var_l = entry->var;
    move->var_r = var_e5;
  }
}

GEN0( node_scope_t, GenScope );

GEN0( node_funcdeclordefn_t, GenFuncdeclordefn )
{
  // TODO: this is broken for forward decls

  auto funcdefnstart_offset = code.len;
  auto funcdefnstart = AddCode( code_funcdefnstart_t, code );
  funcdefnstart->ident_name = node->ident->name;
  funcdefnstart->has_retval = !!node->type;
  if( node->type ) {
    funcdefnstart->retval_sizeinfo = node->type->sizeinfo;
  }

  ForLen( i, node->decls ) {
    auto decl = node->decls.mem[i];
    AssertCrash( !decl->e5 ); // TODO: don't allow default-values for args of functions.

    *AddBack( funcdefnstart->args ) = decl->type->sizeinfo;
  }

  // store the offset into code of this funcdefnstart, so fncalls can lookup where to jump.
  auto varentry_fn = LookupIdent( context, scopestack, &node->ident->name );
  varentry_fn->funcdefnstart_offset = funcdefnstart_offset;

  if( node->scope ) {
    *AddBack( scopestack ) = node->scope;

    // Decls are added to the scope's vartable during type.
//    ForLen( i, node->decls ) {
//      GenDecl( context, scopestack, node->decls.mem[i], code );
//    }

    GenScope( context, scopestack, node->scope, code );

    auto last_stm_was_ret =
      node->scope->statements.len  &&
      node->scope->statements.mem[ node->scope->statements.len - 1 ]->stmtype == stmtype_t::ret;

    if( !last_stm_was_ret ) {
      // gen a synthetic node_ret_t, so every fn-scope ends in a ret.
      AssertCrash( !node->type ); // can't/shouldn't synthetic-generate a retval. only do this for void ret.
      auto ret = AddCode( code_ret_t, code );
    }

    RemBack( scopestack );
  }

  auto funcdefnend = AddCode( code_funcdefnend_t, code );
}
GEN0( node_datadecl_t, GenDatadecl )
{
  ImplementCrash();
}
GEN0( node_binassign_t, GenBinassign )
{
  if( node->implicit_decl ) {
    AssertCrash( node->e0_l->e0type == e0type_t::identdot );
    auto ident = node->e0_l->identdot->idents.mem[0];
    auto scope = scopestack.mem[ scopestack.len - 1 ];
    bool found;
    varentry_t* entry;
    // TODO: should this lookup recursively?
    LookupRaw( scope->vartable, &ident->name, &found, Cast( void**, &entry ) );
    AssertCrash( found );
    AssertCrash( entry->var ); // GenScope should have already pre-declared a var for us from the vartable.
    auto var_r = GenE5( context, scopestack, node->e5_r, code );
    auto move = AddCode( code_move_t, code );
    move->var_l = entry->var;
    move->var_r = var_r;
  } else {
    auto var_l = GenE0( context, scopestack, node->e0_l, code );
    auto var_r = GenE5( context, scopestack, node->e5_r, code );
    auto move = AddCode( code_move_t, code );
    move->var_l = var_l;
    move->var_r = var_r;
  }
}
GEN0( node_ret_t, GenRet )
{
  code_var_t* ret_var = 0;
  if( node->e5 ) {
    ret_var = GenE5( context, scopestack, node->e5, code );
  }
  auto ret = AddCode( code_ret_t, code );
  ret->var = ret_var;
}
GEN0( node_continue_t, GenContinue )
{
  ImplementCrash();
}
GEN0( node_break_t, GenBreak )
{
  ImplementCrash();
}
GEN0( node_while_t, GenWhile )
{
  ImplementCrash();
}
GEN0( node_ifchain_t, GenIfchain )
{
  ImplementCrash();
}
GEN0( node_switch_t, GenSwitch )
{
  ImplementCrash();
}
GEN0( node_case_t, GenCase )
{
  ImplementCrash();
}
GEN0( node_default_t, GenDefault )
{
  ImplementCrash();
}

GEN0( node_statement_t, GenStatement );

GEN0( node_defer_t, GenDefer )
{
  UnreachableCrash();
}
GEN0( node_statement_t, GenStatement )
{
  switch( node->stmtype ) {
    case stmtype_t::funcdeclordefn: {
      GenFuncdeclordefn( context, scopestack, node->funcdeclordefn, code );
    } break;

    case stmtype_t::binassign: {
      GenBinassign( context, scopestack, node->binassign, code );
    } break;

    case stmtype_t::datadecl: {
      GenDatadecl( context, scopestack, node->datadecl, code );
    } break;

    case stmtype_t::decl: {
      GenDecl( context, scopestack, node->decl, code );
    } break;

    case stmtype_t::funccall: {
      GenFunccall( context, scopestack, node->funccall, code );
    } break;

    case stmtype_t::ret: {
      GenRet( context, scopestack, node->ret, code );
    } break;

    case stmtype_t::continue_: {
      GenContinue( context, scopestack, node->continue_, code );
    } break;

    case stmtype_t::break_: {
      GenBreak( context, scopestack, node->break_, code );
    } break;

    case stmtype_t::while_: {
      GenWhile( context, scopestack, node->while_, code );
    } break;

    case stmtype_t::ifchain: {
      GenIfchain( context, scopestack, node->ifchain, code );
    } break;

    case stmtype_t::switch_: {
      GenSwitch( context, scopestack, node->switch_, code );
    } break;

    case stmtype_t::case_: {
      GenCase( context, scopestack, node->case_, code );
    } break;

    case stmtype_t::default_: {
      GenDefault( context, scopestack, node->default_, code );
    } break;

    case stmtype_t::defer: {
      GenDefer( context, scopestack, node->defer, code );
    } break;

    case stmtype_t::scope: {
      *AddBack( scopestack ) = node->scope;
      GenScope( context, scopestack, node->scope, code );
      RemBack( scopestack );
    } break;

    default: {
      UnreachableCrash();
      Error( node->tkn, context, "unknown statement type!" );
    } break;
  }
}
GEN0( node_scope_t, GenScope )
{
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  ForLen( i, node->vartable.elems ) {
    auto elem = ByteArrayElem( hashset_elem_t, node->vartable.elems, i );
    if( elem->has_data ) {
      //auto symbol = Cast( symbol_t*, _GetElemData( node->vartable, elem ) );
      auto entry = Cast( varentry_t*, _GetAssocData( node->vartable, elem ) );
      if( entry->ptrtypes.len != 1 ) {
        Error( node->tkn, context, "ambiguous resolved type!" );
      }
      auto coretype = GetCoretypeFromPtrtype( &entry->ptrtypes.first->value );
      switch( coretype->type ) {
        case coretypetype_t::func: {
          // we don't need stack space for a fn definition.
        } break;
        case coretypetype_t::num:
        case coretypetype_t::struct_: {
          entry->var = AddVar( context, code, scope, &entry->ptrtypes.first->value );
        } break;
        default: UnreachableCrash();
      }
    }
  }
  array_t<code_t> deferred_code;
  Alloc( deferred_code, 64 );
  bool found_ret = 0;
  ForLen( i, node->statements ) {
    auto statement = node->statements.mem[i];
    bool statement_isret = ( statement->stmtype == stmtype_t::ret );
    bool statement_isdefer = ( statement->stmtype == stmtype_t::defer );
    if( statement_isret ) {
      // insert deferred code prior to emitting each ret.
      ForLen( j, deferred_code ) {
        *AddBack( code ) = deferred_code.mem[j];
      }
      found_ret = 1;
      GenStatement( context, scopestack, statement, code );
    } elif( statement_isdefer ) {
      // buffer up deferred code.
      GenStatement( context, scopestack, statement->defer->statement, deferred_code );
    } else {
      GenStatement( context, scopestack, statement, code );
    }
  }
  // emit deferred code if we're default-returning from the scope.
  if( !found_ret ) {
    ForLen( i, deferred_code ) {
      *AddBack( code ) = deferred_code.mem[i];
    }
  }
  Free( deferred_code );
}

Inl void
Gen(
  ccontext_t* context,
  array_t<code_t>& code
  )
{
  array_t<node_scope_t*> scopestack;
  Alloc( scopestack, 256 );
  auto scope = context->ast->scope_toplevel;
  *AddBack( scopestack ) = scope;
  GenScope( context, scopestack, scope, code );

  // gen a synthetic fncall to fn Main as the last code. this is what we'll start execution at.
  {
    auto varentry_fn = LookupIdent( context, scopestack, &Sym_main );
    auto funccall = AddCode( code_funccall_t, code );
    funccall->funcdefnstart_offset = varentry_fn->funcdefnstart_offset;
  }

  RemBack( scopestack );
  AssertCrash( !scopestack.len );
  Free( scopestack );
}









Inl void
PrintResolve( sizeinfo_t& sizeinfo )
{
  printf( "%s/%u/%u", StringFromSizeinfotype( sizeinfo.type ), sizeinfo.bitcount, sizeinfo.bitalign );
  if( sizeinfo.fields.len ) {
    printf( ", fields( " );
    ForLen( i, sizeinfo.fields ) {
      auto field = sizeinfo.fields.mem + i;
      PrintResolve( *field );
      if( i + 1 < sizeinfo.fields.len ) {
        printf( ", " );
      }
    }
    printf( " ), field_bitoffsets( " );
    ForLen( i, sizeinfo.field_bitoffsets ) {
      auto field_bitoffset = sizeinfo.field_bitoffsets.mem[i];
      printf( "%u", field_bitoffset );
      if( i + 1 < sizeinfo.field_bitoffsets.len ) {
        printf( ", " );
      }
    }
    printf( " )" );
  }
}

Inl void
PrintVarDecl( code_var_t& var )
{
  printf( "var%llu: ", var.id );
  PrintResolve( var.sizeinfo );
}

Inl void
PrintVar( code_var_t& var )
{
  printf( "var%llu", var.id );
}

Inl void
PrintGen( array_t<code_t>& code )
{
  ForLen( i, code ) {
    auto c = code.mem + i;
    switch( c->type ) {
      case codetype_t::vardecl: {
        PrintVarDecl( c->var );
      } break;
      case codetype_t::funcdefnstart: {
        auto cstr = AllocCstr( c->funcdefnstart.ident_name );
        printf( "fn %s", cstr );
        MemHeapFree( cstr );
        printf( "(" );
        if( c->funcdefnstart.args.len ) {
          printf( " " );
          For( j, 0, c->funcdefnstart.args.len - 1 ) {
            auto arg = c->funcdefnstart.args.mem + j;
            PrintResolve( *arg );
            printf( ", " );
          }
          PrintResolve( c->funcdefnstart.args.mem[c->funcdefnstart.args.len - 1] );
          printf( " " );
        }
        printf( ")" );
        if( c->funcdefnstart.has_retval ) {
          printf( " " );
          PrintResolve( c->funcdefnstart.retval_sizeinfo );
        }
      } break;
      case codetype_t::funcdefnend: {
        printf( "fn end" );
      } break;
      case codetype_t::funccall: {
        printf( "funccall: " );
        if( c->funccall.has_retval ) {
          PrintVar( *c->funccall.var_result );
          printf( " <- " );
        }
        printf( "func(" );
        if( c->funccall.var_args.len ) {
          printf( " " );
          For( j, 0, c->funccall.var_args.len - 1 ) {
            auto var_arg = c->funccall.var_args.mem[j];
            PrintVar( *var_arg );
            printf( ", " );
          }
          PrintVar( *c->funccall.var_args.mem[c->funccall.var_args.len - 1] );
          printf( " " );
        }
        printf( ")" );
      } break;
      case codetype_t::move: {
        printf( "move: " );
        PrintVar( *c->move.var_l );
        printf( " <- " );
        PrintVar( *c->move.var_r );
      } break;
      case codetype_t::moveimmediate: {
        printf( "moveimm: " );
        PrintVar( *c->moveimm.var_l );
        printf( " <- " );
        printf( "%llu", c->moveimm.node_r->value_u64 );
      } break;
      case codetype_t::ret: {
        if( c->ret.var ) {
          printf( "ret: " );
          PrintVar( *c->ret.var );
        } else {
          printf( "ret" );
        }
      } break;
      case codetype_t::whileloop: {
        printf( "whileloop" );
      } break;
      case codetype_t::ifchain: {
        printf( "ifchain" );
      } break;
      case codetype_t::binop: {
        printf( "binop: " );
        PrintVar( *c->binop.var_result );
        printf( " <- " );
        PrintVar( *c->binop.var_l );
        u8* binoptype = StringFromBinopType( c->binop.binoptype );
        printf( " %s ", binoptype );
        PrintVar( *c->binop.var_r );
      } break;
      case codetype_t::unop: {
        printf( "unop" );
      } break;
      default: UnreachableCrash();
    }
    printf( "\n" );
  }
}




Inl void
Execute( array_t<code_t>& code )
{
  if( !code.len ) {
    return;
  }

  u8* stack = MemHeapAlloc( u8, 1024*1024 );
  // [stack_pointer, stack_pointer + stack_offset) tracks the memory currently in use for this frame.
  idx_t stack_pointer = 0;
  idx_t stack_offset = 0;
  idx_t instruction_pointer = code.len - 1; // start at the final fncall to Main

  while( instruction_pointer < code.len ) {
    auto c = code.mem + instruction_pointer;
    switch( c->type ) {
      case codetype_t::vardecl: {
        // TODO: alignment
        stack_offset += c->var.sizeinfo.bitcount / 8;
      } break;
      case codetype_t::funcdefnstart: {
      } break;
      case codetype_t::funcdefnend: {
      } break;
      case codetype_t::funccall: {
        auto last_stack_pointer = stack_pointer;
        stack_pointer += stack_offset;
        stack_offset = 0;
        // store last_stack_pointer and instruction_pointer on the stack, so we can restore on return from fncall.
        *Cast( idx_t*, stack + stack_pointer ) = last_stack_pointer;
        stack_pointer += sizeof( idx_t );
        *Cast( idx_t*, stack + stack_pointer ) = instruction_pointer + 1;
        stack_pointer += sizeof( idx_t );
        instruction_pointer = c->funccall.funcdefnstart_offset;
        continue;
      } break;
      case codetype_t::move: {
        AssertCrash( c->move.var_l->sizeinfo.bitcount == c->move.var_r->sizeinfo.bitcount );
        auto dst = stack + stack_pointer + c->move.var_l->offset_into_scope;
        auto src = stack + stack_pointer + c->move.var_r->offset_into_scope;
        Memmove( dst, src, c->move.var_l->sizeinfo.bitcount / 8 );
      } break;
      case codetype_t::moveimmediate: {
        AssertCrash( c->moveimm.var_l->sizeinfo.bitcount == c->moveimm.node_r->sizeinfo.bitcount );
        auto dst = stack + stack_pointer + c->moveimm.var_l->offset_into_scope;
        auto src = &c->moveimm.node_r->value_u64;
        Memmove( dst, src, c->moveimm.var_l->sizeinfo.bitcount / 8 );
      } break;
      case codetype_t::ret: {
        stack_offset = 0;
        // load stack_pointer and instruction_pointer to what we stored on the stack before the fncall.
        stack_pointer -= sizeof( idx_t );
        auto popped_instruction_pointer = *Cast( idx_t*, stack + stack_pointer );
        stack_pointer -= sizeof( idx_t );
        auto popped_stack_pointer = *Cast( idx_t*, stack + stack_pointer );
        stack_pointer = popped_stack_pointer;
        instruction_pointer = popped_instruction_pointer;
        continue;
      } break;
      case codetype_t::whileloop: {
      } break;
      case codetype_t::ifchain: {
      } break;
      case codetype_t::binop: {
        auto var_l = c->binop.var_l;
        auto var_r = c->binop.var_r;
        auto var_result = c->binop.var_result;
        auto value_l = stack + stack_pointer + var_l->offset_into_scope;
        auto value_r = stack + stack_pointer + var_r->offset_into_scope;
        auto value_result = stack + stack_pointer + var_result->offset_into_scope;
        AssertCrash( c->binop.var_l->sizeinfo.bitcount == c->binop.var_r->sizeinfo.bitcount );
        auto input_bitcount = var_l->sizeinfo.bitcount;
        AssertCrash( input_bitcount % 8 == 0 );

        switch( c->binop.binoptype ) {

          case binoptype_t::eq    : {
            AssertCrash( c->binop.var_result->sizeinfo.bitcount == 8 );
            switch( c->binop.var_l->sizeinfo.type ) {
              case sizeinfotype_t::unsigned_ :  __fallthrough;
              case sizeinfotype_t::signed_   :  __fallthrough;
              case sizeinfotype_t::ptr       : {
                switch( input_bitcount ) {
                  case 0 : UnreachableCrash();
                  case 8 : *Cast( u8*, value_result ) = Cast( u8, *Cast( u8* , value_l ) == *Cast( u8* , value_r ) ); break;
                  case 16: *Cast( u8*, value_result ) = Cast( u8, *Cast( u16*, value_l ) == *Cast( u16*, value_r ) ); break;
                  case 32: *Cast( u8*, value_result ) = Cast( u8, *Cast( u32*, value_l ) == *Cast( u32*, value_r ) ); break;
                  case 64: *Cast( u8*, value_result ) = Cast( u8, *Cast( u64*, value_l ) == *Cast( u64*, value_r ) ); break;
                  default: *Cast( u8*, value_result ) = Cast( u8, MemEqual( value_l, value_r, input_bitcount / 8 ) ); break;
                }
              } break;

              case sizeinfotype_t::float_    : {
                switch( input_bitcount ) {
                  case 32: *Cast( u8*, value_result ) = Cast( u8, *Cast( f32*, value_l ) == *Cast( f32*, value_r ) ); break;
                  case 64: *Cast( u8*, value_result ) = Cast( u8, *Cast( f64*, value_l ) == *Cast( f64*, value_r ) ); break;
                  default: UnreachableCrash();
                }
              } break;

              case sizeinfotype_t::struct_   : {
                *Cast( u8*, value_result ) = Cast( u8, MemEqual( value_l, value_r, input_bitcount / 8 ) );
              } break;

              case sizeinfotype_t::none      : UnreachableCrash();
              default: UnreachableCrash();
            }
          } break;

          case binoptype_t::noteq :  break;



          #define BOOLBINOP( _type, _op ) \
            case binoptype_t::_type: { \
              AssertCrash( c->binop.var_result->sizeinfo.bitcount == 8 ); \
              switch( c->binop.var_l->sizeinfo.type ) { \
                case sizeinfotype_t::unsigned_ :  __fallthrough; \
                case sizeinfotype_t::signed_   :  __fallthrough; \
                case sizeinfotype_t::ptr       : { \
                  switch( input_bitcount ) { \
                    case 8 : *Cast( u8*, value_result ) = Cast( u8, *Cast( u8* , value_l ) _op *Cast( u8* , value_r ) ); break; \
                    case 16: *Cast( u8*, value_result ) = Cast( u8, *Cast( u16*, value_l ) _op *Cast( u16*, value_r ) ); break; \
                    case 32: *Cast( u8*, value_result ) = Cast( u8, *Cast( u32*, value_l ) _op *Cast( u32*, value_r ) ); break; \
                    case 64: *Cast( u8*, value_result ) = Cast( u8, *Cast( u64*, value_l ) _op *Cast( u64*, value_r ) ); break; \
                    default: UnreachableCrash(); \
                  } \
                } break; \
                case sizeinfotype_t::float_    : { \
                  switch( input_bitcount ) { \
                    case 32: *Cast( u8*, value_result ) = Cast( u8, *Cast( f32*, value_l ) _op *Cast( f32*, value_r ) ); break; \
                    case 64: *Cast( u8*, value_result ) = Cast( u8, *Cast( f64*, value_l ) _op *Cast( f64*, value_r ) ); break; \
                    default: UnreachableCrash(); \
                  } \
                } break; \
                case sizeinfotype_t::struct_   : \
                case sizeinfotype_t::none      : \
                default: UnreachableCrash(); \
              } \
            } break; \

          BOOLBINOP( gt, > );
          BOOLBINOP( lt, < );
          BOOLBINOP( gte, >= );
          BOOLBINOP( lte, <= );


          #define ARITHBINOP( _type, _op ) \
            case binoptype_t::_type: { \
              AssertCrash( c->binop.var_result->sizeinfo.bitcount % 8 == 0 ); \
              switch( c->binop.var_l->sizeinfo.type ) { \
                case sizeinfotype_t::unsigned_ :  __fallthrough; \
                case sizeinfotype_t::signed_   :  __fallthrough; \
                case sizeinfotype_t::ptr       : { \
                  switch( input_bitcount ) { \
                    default: UnreachableCrash(); \
                    case 8 : *Cast( u8* , value_result ) = *Cast( u8* , value_l ) _op *Cast( u8* , value_r ); break; \
                    case 16: *Cast( u16*, value_result ) = *Cast( u16*, value_l ) _op *Cast( u16*, value_r ); break; \
                    case 32: *Cast( u32*, value_result ) = *Cast( u32*, value_l ) _op *Cast( u32*, value_r ); break; \
                    case 64: *Cast( u64*, value_result ) = *Cast( u64*, value_l ) _op *Cast( u64*, value_r ); break; \
                  } \
                } break; \
                case sizeinfotype_t::float_    : { \
                  switch( input_bitcount ) { \
                    case 32: *Cast( f32*, value_result ) = *Cast( f32*, value_l ) _op *Cast( f32*, value_r ); break; \
                    case 64: *Cast( f64*, value_result ) = *Cast( f64*, value_l ) _op *Cast( f64*, value_r ); break; \
                    default: UnreachableCrash(); \
                  } \
                } break; \
                case sizeinfotype_t::struct_   : \
                case sizeinfotype_t::none      : \
                default: UnreachableCrash(); \
              } \
            } break; \

          ARITHBINOP( add, + );
          ARITHBINOP( sub, - );
          ARITHBINOP( mul, * );
          ARITHBINOP( div, / );
//          ARITHBINOP( rem, % );


          case binoptype_t::and_  :
          case binoptype_t::or_   :
          case binoptype_t::xor_  :
          case binoptype_t::shl   :
          case binoptype_t::shr   :

          default: UnreachableCrash();
        }

        int x = 12;
      } break;
      case codetype_t::unop: {
      } break;
      default: UnreachableCrash();
    }
    instruction_pointer += 1;
  }

  MemHeapFree( stack );
}
