// Copyright (c) John A. Carlos Jr., all rights reserved.



struct
rcontext_t
{
  ast_t* ast;
  src_t* src;
  tlog_t* log;
  plist_t* mem;
};

void
Error(
  token_t* tkn,
  rcontext_t* context,
  void* errstr
  )
{
  Error( tkn, context->src, context->log, errstr );
}



#define RESOLVE0( type, name ) \
  Inl void \
  name( \
    rcontext_t* context, \
    array_t<node_scope_t*>& scopestack, \
    type* node \
    )

#define RESOLVE1( type, name ) \
  Inl void \
  name( \
    rcontext_t* context, \
    array_t<node_scope_t*>& scopestack, \
    type* node \
    )



RESOLVE1( node_num_t, ResolveNum )
{
}
RESOLVE1( node_str_t, ResolveStr )
{
}
RESOLVE1( node_chr_t, ResolveChr )
{
}

RESOLVE1( node_e0_t, ResolveE0 );

RESOLVE1( node_e1_t, ResolveE1 )
{
}
RESOLVE1( node_e2_t, ResolveE2 )
{
}
RESOLVE1( node_e3_t, ResolveE3 )
{
}
RESOLVE1( node_e4_t, ResolveE4 )
{
}
RESOLVE1( node_e5_t, ResolveE5 )
{
}
RESOLVE1( node_funccall_t, ResolveFunccall )
{
}
RESOLVE1( node_identdot_t, ResolveIdentdot )
{
}
RESOLVE1( node_unop_t, ResolveUnop )
{
}
RESOLVE1( node_e0_t, ResolveE0 )
{
}
RESOLVE0( node_type_t, ResolveType )
{
}
RESOLVE0( node_decl_t, ResolveDecl )
{
}
RESOLVE0( node_funcdeclordefn_t, ResolveFuncdeclordefn )
{

}
RESOLVE0( node_datadecl_t, ResolveDatadecl )
{
}
RESOLVE1( node_binassign_t, ResolveBinassign )
{

}
RESOLVE0( node_ret_t, ResolveRet )
{
}
RESOLVE0( node_continue_t, ResolveContinue )
{
}
RESOLVE0( node_break_t, ResolveBreak )
{
}
RESOLVE0( node_while_t, ResolveWhile )
{
}
RESOLVE0( node_ifchain_t, ResolveIfchain )
{
}
RESOLVE0( node_switch_t, ResolveSwitch )
{
}
RESOLVE0( node_case_t, ResolveCase )
{
}
RESOLVE0( node_default_t, ResolveDefault )
{
}

RESOLVE0( node_statement_t, ResolveStatement );

RESOLVE0( node_defer_t, ResolveDefer )
{
}

RESOLVE0( node_scope_t, ResolveScope );

RESOLVE0( node_statement_t, ResolveStatement )
{
  switch( node->stmtype ) {
    case stmtype_t::funcdeclordefn: {
      ResolveFuncdeclordefn( context, scopestack, node->funcdeclordefn );
    } break;

    case stmtype_t::datadecl: {
      ResolveDatadecl( context, scopestack, node->datadecl );
    } break;

    case stmtype_t::binassign: {
      ResolveBinassign( context, scopestack, node->binassign );
    } break;

    case stmtype_t::funccall: {
      ResolveFunccall( context, scopestack, node->funccall );
    } break;

    case stmtype_t::ret: {
      ResolveRet( context, scopestack, node->ret );
    } break;

    case stmtype_t::continue_: {
      ResolveContinue( context, scopestack, node->continue_ );
    } break;

    case stmtype_t::break_: {
      ResolveBreak( context, scopestack, node->break_ );
    } break;

    case stmtype_t::while_: {
      ResolveWhile( context, scopestack, node->while_ );
    } break;

    case stmtype_t::ifchain: {
      ResolveIfchain( context, scopestack, node->ifchain );
    } break;

    case stmtype_t::switch_: {
      ResolveSwitch( context, scopestack, node->switch_ );
    } break;

    case stmtype_t::case_: {
      ResolveCase( context, scopestack, node->case_ );
    } break;

    case stmtype_t::default_: {
      ResolveDefault( context, scopestack, node->default_ );
    } break;

    case stmtype_t::defer: {
      ResolveDefer( context, scopestack, node->defer );
    } break;

    case stmtype_t::scope: {
      ResolveScope( context, scopestack, node->scope );
    } break;

    default: {
      UnreachableCrash();
      Error( node->tkn, context, "unknown statement type!" );
    } break;
  }
}
RESOLVE0( node_scope_t, ResolveScope )
{
}

Inl void
Resolve(
  rcontext_t* context
  )
{
  array_t<node_scope_t*> scopestack;
  Alloc( scopestack, 256 );

  auto scope = context->ast->scope_toplevel;

  *AddBack( scopestack ) = scope;

  ResolveScope( context, scopestack, scope );

  RemBack( scopestack );

  AssertCrash( !scopestack.len );

  Free( scopestack );
}


