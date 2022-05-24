// Copyright (c) John A. Carlos Jr., all rights reserved.


#if 0

TODO:

enum syntax

for syntax

do_while syntax

can probably eliminate "<<" ">>" "<<=" ">>=". they don't really deserve custom syntax, functions are fine.

make semicolons optional

passing types as params

builtin funcs
  cast, sizeof

add the type resolution pass.

codegen!

file-literal strings via ident tokenizing

unify contexts across passes.

do we need constraints?

allow optional data order optimization

eliminate array_t and list_t usage; we can probably contiguously store everything in a single plist_t

we can eliminate "fn" syntax if we only allow function decls/defns at toplevel scope.
  that's a simpler paradigm, probably what we want anyways.



GRAMMAR:
note that { x } means 0 or more of x repeated.
note that [ x ] means 0 or 1 of x.

scope =
  "{"  { statement  ";" }  "}"

statement =
  "fn" ident  "("  { decl ";" }  [ decl  [ ";" ] ]  ")"  [ type ]  [ scope ]
  "data" ident  "("  { decl ";" }  [ decl  [ ";" ] ]  ")"
  decl
  ident  "("  { e5 ";" }  [ e5  [ ";" ] ]  ")"
  e0  "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^=" | "<<=" | ">>="  e5
  "ret"  e5
  "continue"
  "break"
  "while"  e5  scope
  "if"  e5  scope  { "elif"  e5  scope }  [ "else"  scope ]
  "switch"  e5  scope
  "case"  e5  scope
  "default"  scope
  "defer"  statement
  scope

type =
  { "*" }  ident

decl =
  ident  type  [ "="  e5 ]

num =
  [ "-" ]  digit  { digit }  [ "."  digit  { digit } ]  [ "e"  [ "-" ]  digit  { digit } ]
  0x  digit  { digit }
  0b  "0" | "1"  { "0" | "1" }

str =
  """  { ascii }  """

chr =
  "'"  ascii  "'"

ident =
  "_" | alpha  { "_" | alpha | digit }

e0 =
  ident  "("  { e5 ";" }  [ e5  [ ";" ] ]  ")"
  ident  { "."  ident }
  "("  e5  { ";"  e5 }  [ ";" ]  ")"
  "*" | "&" | "-" | "!"  e4
  num
  str
  chr

e1 =
  e0  [ "*" | "/" | "%"  e0 ]

e2 =
  e1  [ "+" | "-"  e1 ]

e3 =
  e2  [ "&" | "|" | "^"  e2 ]

e4 =
  e3  [ "<<" | ">>"  e3 ]

e5 =
  e4  [ "==" | "!=" | ">" | ">=" | "<" | "<="  e4 ]

#endif






struct
node_scope_t;

struct
ptrtype_t
{
  node_scope_t* scope;
  idx_t idx; // into scope's defined_types.
  u8 ptrlevel;
  bool iszero; // special; can convert to any pointer ptrtype.  only set for 0 literals.
};


// sizeinfo_t identifies a typed bit-layout of a specific element.

#define SIZEINFOTYPE( _x ) \
  _x( none      ) \
  _x( unsigned_ ) \
  _x( signed_   ) \
  _x( float_    ) \
  _x( ptr       ) \
  _x( struct_   ) \


Enumc( sizeinfotype_t )
{
  #define ENUM( type )  type,
  SIZEINFOTYPE( ENUM )
  #undef ENUM
};

Inl u8*
StringFromSizeinfotype( sizeinfotype_t& type )
{
  switch( type ) {
    #define CASE( type )   case sizeinfotype_t::type: return Str( # type );
    SIZEINFOTYPE( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}

struct
sizeinfo_t
{
  sizeinfotype_t type;
  u32 bitcount;
  u32 bitalign;
  array_t<sizeinfo_t> fields;
  array_t<u32> field_bitoffsets;
};
Inl void
Init( sizeinfo_t* sizeinfo, sizeinfotype_t type )
{
  Typezero( sizeinfo );
  sizeinfo->type = type;
  if( type == sizeinfotype_t::struct_ ) {
    Alloc( sizeinfo->fields, 16 );
    Alloc( sizeinfo->field_bitoffsets, 16 );
  }
}



#define NODETYPES(x) \
  x( scope             ) \
  x( statement         ) \
  x( type              ) \
  x( decl              ) \
  x( num               ) \
  x( str               ) \
  x( chr               ) \
  x( ident             ) \
  x( e0                ) \
  x( binop             ) \
                         \
  x( funcdeclordefn    ) \
  x( datadecl          ) \
  x( binassign         ) \
  x( ret               ) \
  x( continue_         ) \
  x( break_            ) \
  x( while_            ) \
  x( ifchain           ) \
  x( switch_           ) \
  x( case_             ) \
  x( default_          ) \
  x( defer             ) \
                         \
  x( funccall          ) \
  x( identdot          ) \
  x( unop              ) \


Enumc( nodetype_t )
{
  #define CASE( x )   x,
  NODETYPES( CASE )
  #undef CASE
};


struct
node_num_t
{
  nodetype_t nodetype;
  token_t* tkn;
  symbol_t literal;
  u64 value_u64;
  f64 value_f64;
  f32 value_f32;
  listwalloc_t<ptrtype_t> ptrtypes; // TODO: TypeE1 determines what the E0's ptrtypes is, but doesn't change this.
  sizeinfo_t sizeinfo;
};
Inl void
Init( node_num_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::num;
  node->ptrtypes.elems = mem;
}

struct
node_str_t
{
  nodetype_t nodetype;
  token_t* tkn;
  symbol_t literal;
  listwalloc_t<ptrtype_t> ptrtypes;
  sizeinfo_t sizeinfo;
};
Inl void
Init( node_str_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::str;
  node->ptrtypes.elems = mem;
}

struct
node_chr_t
{
  nodetype_t nodetype;
  token_t* tkn;
  symbol_t literal;
  listwalloc_t<ptrtype_t> ptrtypes;
  sizeinfo_t sizeinfo;
};
Inl void
Init( node_chr_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::chr;
  node->ptrtypes.elems = mem;
}

struct
node_ident_t
{
  nodetype_t nodetype;
  token_t* tkn;
  symbol_t name;
};
Inl void
Init( node_ident_t* node )
{
  Typezero( node );
  node->nodetype = nodetype_t::ident;
}

struct
node_e0_t;

#define BINOPTYPES( x ) \
  x( none ) \
  x( mul ) \
  x( div ) \
  x( rem ) \
  x( add ) \
  x( sub ) \
  x( and_ ) \
  x( or_ ) \
  x( xor_ ) \
  x( shl ) \
  x( shr ) \
  x( eq ) \
  x( noteq ) \
  x( gt ) \
  x( gte ) \
  x( lt ) \
  x( lte ) \

Enumc( binoptype_t )
{
  #define CASE( x )   x,
  BINOPTYPES( x )
  #undef CASE
};
struct
node_binop_t
{
  nodetype_t nodetype;
  token_t* tkn;
  node_e0_t* e0_l;
  node_e0_t* e0_r;
  binoptype_t binoptype;
  listwalloc_t<ptrtype_t> ptrtypes;
  sizeinfo_t sizeinfo;
};
Inl void
Init( node_binop_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::binop;
  node->ptrtypes.elems = mem;
}
Inl u32
PrecedenceValue( binoptype_t type )
{
  switch( type ) {
    case binoptype_t::add: __fallthrough;
    case binoptype_t::sub: { return 0; } break;
    case binoptype_t::mul: __fallthrough;
    case binoptype_t::div: __fallthrough;
    case binoptype_t::rem: { return 1; } break;
    case binoptype_t::pow: { return 2; } break;


    case binoptype_t::and_:
    case binoptype_t::or_:
    case binoptype_t::xor_: { return ; } break;

    case binoptype_t::shl:
    case binoptype_t::shr: { return ; } break;

    case binoptype_t::eq:
    case binoptype_t::noteq:
    case binoptype_t::gt:
    case binoptype_t::gte:
    case binoptype_t::lt:
    case binoptype_t::lte: { return 0; } break;

    case binoptype_t::add:
    case binoptype_t::sub: { return 0; } break;

    case binoptype_t::mul:
    case binoptype_t::div:
    case binoptype_t::rem: { return ; } break;

    default: UnreachableCrash();
  }
  return 0;
}

struct
node_funccall_t
{
  nodetype_t nodetype;
  token_t* tkn;
  node_ident_t* ident;
  array_t<node_e5_t*> e5_args;
  listwalloc_t<ptrtype_t> ptrtypes;
  sizeinfo_t sizeinfo;
};
Inl void
Init( node_funccall_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::funccall;
  Alloc( node->e5_args, 8 );
  node->ptrtypes.elems = mem;
}

struct
node_identdot_t
{
  nodetype_t nodetype;
  token_t* tkn;
  array_t<node_ident_t*> idents;
  listwalloc_t<ptrtype_t> ptrtypes;
  sizeinfo_t sizeinfo;
};
Inl void
Init( node_identdot_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::identdot;
  Alloc( node->idents, 4 );
  node->ptrtypes.elems = mem;
}

#define UNOPTYPES( x ) \
  x( deref  ) \
  x( addrof ) \
  x( negate ) \
  x( not_   ) \

Enumc( unoptype_t )
{
  #define CASE( x )   x,
  UNOPTYPES( CASE )
  #undef CASE
};
struct
node_unop_t
{
  nodetype_t nodetype;
  token_t* tkn;
  unoptype_t unoptype;
  node_e4_t* e4;
  listwalloc_t<ptrtype_t> ptrtypes;
  sizeinfo_t sizeinfo;
};
Inl void
Init( node_unop_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::unop;
  node->ptrtypes.elems = mem;
}

Enumc( e0type_t )
{
  funccall,
  identdot,
  e5s,
  unop,
  binop,
  num,
  str,
  chr,
};
struct
node_e0_t
{
  nodetype_t nodetype;
  token_t* tkn;
  e0type_t e0type;
  union {
    node_funccall_t* funccall;
    node_identdot_t* identdot;
    array_t<node_e5_t*> e5s; // tuple; unfinished for now.
    node_unop_t* unop;
    node_num_t* num;
    node_str_t* str;
    node_chr_t* chr;
  };
  listwalloc_t<ptrtype_t> ptrtypes;
  sizeinfo_t sizeinfo;
};
Inl void
Init( node_e0_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::e0;
  node->ptrtypes.elems = mem;
}

struct
node_type_t
{
  nodetype_t nodetype;
  token_t* tkn;
  u8 ptrlevel;
  node_ident_t* ident;
  ptrtype_t ptrtype;
  sizeinfo_t sizeinfo;
};
Inl void
Init( node_type_t* node )
{
  Typezero( node );
  node->nodetype = nodetype_t::type;
}

struct
node_decl_t
{
  nodetype_t nodetype;
  token_t* tkn;
  node_ident_t* ident;
  node_type_t* type;
  node_e5_t* e5;
};
Inl void
Init( node_decl_t* node )
{
  Typezero( node );
  node->nodetype = nodetype_t::decl;
}

struct
node_funcdeclordefn_t
{
  nodetype_t nodetype;
  token_t* tkn;
  node_ident_t* ident;
  array_t<node_decl_t*> decls;
  node_type_t* type;
  node_scope_t* scope;
};
Inl void
Init( node_funcdeclordefn_t* node )
{
  Typezero( node );
  node->nodetype = nodetype_t::funcdeclordefn;
  Alloc( node->decls, 4 );
}

struct
node_datadecl_t
{
  nodetype_t nodetype;
  token_t* tkn;
  node_ident_t* ident;
  array_t<node_decl_t*> decls;
};
Inl void
Init( node_datadecl_t* node )
{
  Typezero( node );
  node->nodetype = nodetype_t::datadecl;
  Alloc( node->decls, 4 );
}

#define BINASSIGNTYPES( x ) \
  x( eq    ) \
  x( addeq ) \
  x( subeq ) \
  x( muleq ) \
  x( diveq ) \
  x( remeq ) \
  x( andeq ) \
  x( oreq  ) \
  x( xoreq ) \
  x( shleq ) \
  x( shreq ) \

Enumc( binassigntype_t )
{
  #define CASE( x )   x,
  BINASSIGNTYPES( CASE )
  #undef CASE
};

struct
node_binassign_t
{
  nodetype_t nodetype;
  token_t* tkn;
  binassigntype_t binassigntype;
  node_e0_t* e0_l;
  node_e5_t* e5_r;
  listwalloc_t<ptrtype_t> ptrtypes;
  sizeinfo_t sizeinfo;
  bool implicit_decl;
};
Inl void
Init( node_binassign_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::binassign;
  node->ptrtypes.elems = mem;
}

struct
node_ret_t
{
  nodetype_t nodetype;
  token_t* tkn;
  node_e5_t* e5;
  listwalloc_t<ptrtype_t> ptrtypes;
};
Inl void
Init( node_ret_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::ret;
  node->ptrtypes.elems = mem;
}

struct
node_continue_t
{
  nodetype_t nodetype;
  token_t* tkn;
};
Inl void
Init( node_continue_t* node )
{
  Typezero( node );
  node->nodetype = nodetype_t::continue_;
}

struct
node_break_t
{
  nodetype_t nodetype;
  token_t* tkn;
};
Inl void
Init( node_break_t* node )
{
  Typezero( node );
  node->nodetype = nodetype_t::break_;
}

struct
node_while_t
{
  nodetype_t nodetype;
  token_t* tkn;
  node_e5_t* e5;
  node_scope_t* scope;
  listwalloc_t<ptrtype_t> ptrtypes;
};
Inl void
Init( node_while_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::while_;
  node->ptrtypes.elems = mem;
}

struct
node_ifchain_t
{
  nodetype_t nodetype;
  token_t* tkn;
  node_e5_t* if_e5;
  node_scope_t* if_scope;
  array_t<node_e5_t*> elif_e5s;
  array_t<node_scope_t*> elif_scopes;
  node_scope_t* else_scope;
  listwalloc_t<ptrtype_t> ptrtypes;
  sizeinfo_t sizeinfo;
};
Inl void
Init( node_ifchain_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::ifchain;
  Alloc( node->elif_e5s, 4 );
  Alloc( node->elif_scopes, 4 );
  node->ptrtypes.elems = mem;
}

struct
node_case_t
{
  nodetype_t nodetype;
  token_t* tkn;
  node_e5_t* e5;
  node_scope_t* scope;
};
Inl void
Init( node_case_t* node )
{
  Typezero( node );
  node->nodetype = nodetype_t::case_;
}

struct
node_default_t
{
  nodetype_t nodetype;
  token_t* tkn;
  node_scope_t* scope;
};
Inl void
Init( node_default_t* node )
{
  Typezero( node );
  node->nodetype = nodetype_t::default_;
}

struct
node_switch_t
{
  nodetype_t nodetype;
  token_t* tkn;
  node_e5_t* e5;
  node_scope_t* scope;
  array_t<node_case_t*> cases; // TODO:
  node_default_t* default_; // TODO:
};
Inl void
Init( node_switch_t* node )
{
  Typezero( node );
  node->nodetype = nodetype_t::switch_;
}

struct
node_statement_t;

struct
node_defer_t
{
  nodetype_t nodetype;
  token_t* tkn;
  node_statement_t* statement;
  listwalloc_t<ptrtype_t> ptrtypes;
};
Inl void
Init( node_defer_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::defer;
  node->ptrtypes.elems = mem;
}

Enumc( stmtype_t )
{
  funcdeclordefn,
  binassign,
  datadecl,
  decl,
  funccall,
  ret,
  continue_,
  break_,
  while_,
  ifchain,
  switch_,
  case_,
  default_,
  defer,
  scope,
};

struct
node_statement_t
{
  nodetype_t nodetype;
  token_t* tkn;
  stmtype_t stmtype;
  union {
    node_funcdeclordefn_t* funcdeclordefn;
    node_binassign_t* binassign;
    node_datadecl_t* datadecl;
    node_decl_t* decl;
    node_funccall_t* funccall;
    node_ret_t* ret;
    node_continue_t* continue_;
    node_break_t* break_;
    node_while_t* while_;
    node_ifchain_t* ifchain;
    node_switch_t* switch_;
    node_case_t* case_;
    node_default_t* default_;
    node_defer_t* defer;
    node_scope_t* scope;
  };
  listwalloc_t<ptrtype_t> ptrtypes;
};
Inl void
Init( node_statement_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::statement;
  node->ptrtypes.elems = mem;
}


Enumc( coretypetype_t )
{
  num,
  func,
  struct_,
};

#define NUMTYPES( x ) \
  x( u8_   ) \
  x( u16_  ) \
  x( u32_  ) \
  x( u64_  ) \
  x( s8_   ) \
  x( s16_  ) \
  x( s32_  ) \
  x( s64_  ) \
  x( int_  ) \
  x( uint_ ) \
  x( f32_  ) \
  x( f64_  ) \
  x( bool_ ) \

Enumc( numtype_t )
{
  #define CASE( x )   x,
  NUMTYPES( CASE )
  #undef CASE
};

struct
coretype_num_t
{
  coretypetype_t type;
  numtype_t numtype;
};
Inl void
Init( coretype_num_t* type )
{
  Typezero( type );
  type->type = coretypetype_t::num;
}

struct
coretype_func_t
{
  coretypetype_t type;
  symbol_t name;
  array_t<ptrtype_t> args;
  bool has_ret;
  ptrtype_t ret;
};
Inl void Init( coretype_func_t* type )
{
  Typezero( type );
  type->type = coretypetype_t::func;
  Alloc( type->args, 8 );
}

struct
coretype_struct_t
{
  coretypetype_t type;
  symbol_t name;
  array_t<ptrtype_t> fields;
  array_t<symbol_t> fieldnames;
};
Inl void Init( coretype_struct_t* type )
{
  Typezero( type );
  type->type = coretypetype_t::struct_;
  Alloc( type->fields, 8 );
  Alloc( type->fieldnames, 8 );
}


struct
coretype_t
{
  union {
    coretypetype_t type;
    coretype_num_t num;
    coretype_func_t func;
    coretype_struct_t struct_;
  };
};




Inl bool
SymbolEqual( symbol_t& a, symbol_t& b )
{
  bool r = EqualContents( a, b );
  return r;
}

Inl idx_t
SymbolHash( symbol_t& symbol )
{
  return StringHash( ML( symbol ) );
}


HASHSET_ELEM_EQUAL( HashsetSymbolEqual )
{
  AssertCrash( elem_size == sizeof( symbol_t ) );
  auto& a = *Cast( symbol_t*, elem0 );
  auto& b = *Cast( symbol_t*, elem1 );
  return SymbolEqual( a, b );
}

HASHSET_ELEM_HASH( HashsetSymbolHash )
{
  AssertCrash( elem_len == sizeof( symbol_t ) );
  auto& symbol = *Cast( symbol_t*, elem );
  return SymbolHash( symbol );
}



Inl bool
PtrtypeConvertible( ptrtype_t& a, ptrtype_t& b )
{
  // zero constant is convertible to ptrtype's of all ptrlevels > 0.
  if( ( a.iszero  &  ( b.iszero  |  ( b.ptrlevel > 0 ) ) )  ||
      ( b.iszero  &  ( a.iszero  |  ( a.ptrlevel > 0 ) ) )
    )
  {
    return 1;
  }
  bool r =
    ( a.idx == b.idx )  &
    ( a.scope == b.scope )  &
    ( a.ptrlevel == b.ptrlevel );
  return r;
}

Inl bool
PtrtypeEqual_( ptrtype_t& a, ptrtype_t& b )
{
  bool r =
    ( a.iszero == b.iszero )  &
    ( a.idx == b.idx )  &
    ( a.scope == b.scope )  &
    ( a.ptrlevel == b.ptrlevel );
  return r;

}

Inl idx_t
PtrtypeHash( ptrtype_t& a )
{
  idx_t r = 7 * a.idx + a.iszero;
  r += 13 * Cast( idx_t, a.scope );
  r += 5 * a.ptrlevel;
  return r;
}



HASHSET_ELEM_EQUAL( HashsetPtrtypeEqual_ )
{
  AssertCrash( elem_size == sizeof( ptrtype_t ) );
  auto& a = *Cast( ptrtype_t*, elem0 );
  auto& b = *Cast( ptrtype_t*, elem1 );
  return PtrtypeEqual_( a, b );
}

HASHSET_ELEM_HASH( HashsetPtrtypeHash )
{
  AssertCrash( elem_len == sizeof( ptrtype_t ) );
  auto& ptrtype = *Cast( ptrtype_t*, elem );
  return PtrtypeHash( ptrtype );
}



Inl bool
TypeEqual( coretype_t& a, coretype_t& b )
{
  if( a.type != b.type )
    return 0;

  switch( a.type ) {
    case coretypetype_t::num: {
      if( a.num.numtype != b.num.numtype )
        return 0;
    } break;

    case coretypetype_t::func: {
      if( !SymbolEqual( a.func.name, b.func.name )  ||
          a.func.has_ret != b.func.has_ret  ||
          a.func.args.len != b.func.args.len  ||
          !PtrtypeConvertible( a.func.ret, b.func.ret )
        )
      {
        return 0;
      }
      ForLen( i, a.func.args ) {
        if( !PtrtypeConvertible( a.func.args.mem[i], b.func.args.mem[i] ) ) {
          return 0;
        }
      }
    } break;

    case coretypetype_t::struct_: {
      if( !SymbolEqual( a.struct_.name, b.struct_.name )  ||
          a.struct_.fields.len != b.struct_.fields.len  ||
          a.struct_.fieldnames.len != b.struct_.fieldnames.len
        )
      {
        return 0;
      }
      ForLen( i, a.struct_.fieldnames ) {
        if( !SymbolEqual( a.struct_.fieldnames.mem[i], b.struct_.fieldnames.mem[i] ) ) {
          return 0;
        }
      }
      ForLen( i, a.struct_.fields ) {
        if( !PtrtypeConvertible( a.struct_.fields.mem[i], b.struct_.fields.mem[i] ) ) {
          return 0;
        }
      }
    } break;

    default: {
      UnreachableCrash();
      return 0;
    } break;
  }
  return 1;
}

Inl idx_t
TypeHash( coretype_t& a )
{
  idx_t r = Cast( idx_t, a.type );

#if 0
  switch( a.type ) {

    case coretypetype_t::func: {
      r += a.func.ptrtype_return_nonfunc.idx;
      r += Cast( idx_t, a.func.ptrtype_return_nonfunc.scope );
      ForLen( i, a.func.ptrtype_args ) {
        ptrtype_t ai = a.func.ptrtype_args.mem[i];
        r += ai.idx;
        r += Cast( idx_t, ai.scope );
        r += ai.ptrlevel;
      }
    } break;

    case coretypetype_t::nonfunc: {
      r += SymbolHash( a.nonfunc.name );
    } break;

    default: UnreachableCrash();
  }
#endif
  return r;
}



HASHSET_ELEM_EQUAL( HashsetTypeEqual )
{
  AssertCrash( elem_size == sizeof( coretype_t ) );
  coretype_t& a = *Cast( coretype_t*, elem0 );
  coretype_t& b = *Cast( coretype_t*, elem1 );
  return TypeEqual( a, b );
}

HASHSET_ELEM_HASH( HashsetTypeHash )
{
  AssertCrash( elem_len == sizeof( coretype_t ) );
  coretype_t& a = *Cast( coretype_t*, elem );
  return TypeHash( a );
}




struct
ast_t
{
  node_scope_t* scope_toplevel;
};





Enumc( codetype_t )
{
  vardecl,
  funcdefnstart,
  funcdefnend,
  funccall,
  move,
  moveimmediate,
  ret,
  whileloop, // TODO: breakdown to conditionals and jumps? yeah, probably.
  ifchain, // TODO: breakdown to conditionals and jumps? yeah, probably.
  binop,
  unop,
};

struct
code_var_t
{
  codetype_t type;
  sizeinfo_t sizeinfo;
  idx_t id; // keep a unique id for every code_var_t, just to make debugging/visualization a little easier.
  idx_t offset_into_scope; // scope-relative position of this variable in the stack. can think of this as "stack pointer + 'offset_into_scope'"
};
Inl void Init( code_var_t* code ) { Typezero( code );  code->type = codetype_t::vardecl; }

struct
code_funcdefnstart_t
{
  codetype_t type;
  symbol_t ident_name;
  sizeinfo_t retval_sizeinfo;
  array_t<sizeinfo_t> args; // TODO: how does the fnbody's codegen know how to address these args?
  bool has_retval;
};
Inl void Init( code_funcdefnstart_t* code )
{
  Typezero( code );
  code->type = codetype_t::funcdefnstart;
  Alloc( code->args, 8 );
}

struct
code_funcdefnend_t
{
  codetype_t type;
};
Inl void Init( code_funcdefnend_t* code ) { Typezero( code );  code->type = codetype_t::funcdefnend; }

struct
code_funccall_t
{
  codetype_t type;
  array_t<code_var_t*> var_args; // PERF: terrible.
  code_var_t* var_result;
  bool has_retval;
  idx_t funcdefnstart_offset;
};
Inl void Init( code_funccall_t* code )
{
  Typezero( code );
  code->type = codetype_t::funccall;
  // array_t is set up elsewhere
}

struct
code_move_t
{
  codetype_t type;
  code_var_t* var_l;
  code_var_t* var_r;
};
Inl void Init( code_move_t* code ) { Typezero( code );  code->type = codetype_t::move; }

struct
code_moveimmediate_t
{
  codetype_t type;
  code_var_t* var_l;
  // TODO: probably convert this to be non-node.
  node_num_t* node_r;
};
Inl void Init( code_moveimmediate_t* code ) { Typezero( code );  code->type = codetype_t::moveimmediate; }

struct
code_ret_t
{
  codetype_t type;
  code_var_t* var;
};
Inl void Init( code_ret_t* code ) { Typezero( code );  code->type = codetype_t::ret; }

struct
code_whileloop_t
{
  codetype_t type;
};
Inl void Init( code_whileloop_t* code ) { Typezero( code );  code->type = codetype_t::whileloop; }

struct
code_ifchain_t
{
  codetype_t type;
};
Inl void Init( code_ifchain_t* code ) { Typezero( code );  code->type = codetype_t::ifchain; }

struct
code_binop_t
{
  codetype_t type;
  binoptype_t binoptype;
  code_var_t* var_l;
  code_var_t* var_r;
  code_var_t* var_result;
};
Inl void Init( code_binop_t* code ) { Typezero( code );  code->type = codetype_t::binop; }

Enumc( code_unoptype_t )
{
  negate,
  not_,
  extend_zero,
  extend_sign,
  shl,
  shr_zero,
  shr_sign,
};

struct
code_unop_t
{
  codetype_t type;
  code_unoptype_t unoptype;
  code_var_t* var;
};
Inl void Init( code_unop_t* code ) { Typezero( code );  code->type = codetype_t::unop; }


struct
code_t
{
  union {
    codetype_t type;
    code_var_t var;
    code_funcdefnstart_t funcdefnstart;
    code_funcdefnend_t funcdefnend;
    code_funccall_t funccall;
    code_move_t move;
    code_moveimmediate_t moveimm;
    code_ret_t ret;
    code_whileloop_t whileloop;
    code_ifchain_t ifchain;
    code_binop_t binop;
    code_unop_t unop;
  };
};




struct
varentry_t
{
  listwalloc_t<ptrtype_t> ptrtypes;

  // note coretypetype_t::func doesn't get a var, since fns don't need stack space themselves.
  // instead, we store an idx offset into the code array, so we know where to jump to for execution.
  code_var_t* var;
  idx_t funcdefnstart_offset;
};

struct
node_scope_t
{
  nodetype_t nodetype;
  token_t* tkn;
  array_t<node_statement_t*> statements;
  idx_hashset_t defined_types; // holds ( coretype_t, 0 )
  hashset_t types_from_typenames; // holds ( symbol_t, idx_t into this scope's defined_types )
  hashset_t vartable; // holds ( symbol_t , varentry_t )
  listwalloc_t<ptrtype_t> ptrtypes;
  sizeinfo_t sizeinfo;
  idx_t vars_bytecount_top;
};

Inl void
Init( node_scope_t* node, plist_t* mem )
{
  Typezero( node );
  node->nodetype = nodetype_t::scope;
  Alloc( node->statements, 32 );
  Init( node->ptrtypes, mem );

  Init(
    node->vartable,
    32,
    sizeof( symbol_t ),
    sizeof( varentry_t ),
    0.75f,
    HashsetSymbolEqual,
    HashsetSymbolHash
    );

  Init(
    node->defined_types,
    16,
    sizeof( coretype_t ),
    0,
    0.75f,
    HashsetTypeEqual,
    HashsetTypeHash
    );

  Init(
    node->types_from_typenames,
    16,
    sizeof( symbol_t ),
    sizeof( idx_t ),
    0.75f,
    HashsetSymbolEqual,
    HashsetSymbolHash
    );
}



Inl void
SizeinfoFromPtrtype(
  sizeinfo_t* sizeinfo,
  ptrtype_t* ptrtype,
  u32 ptr_bitcount
  )
{
  if( ptrtype->ptrlevel ) {
    Init( sizeinfo, sizeinfotype_t::ptr );
    sizeinfo->bitcount = ptr_bitcount;
    sizeinfo->bitalign = sizeinfo->bitcount;
    return;
  }

  coretype_t* coretype;
  bool found;
  AssertCrash( ptrtype->scope );
  GetElement( ptrtype->scope->defined_types, ptrtype->idx, &found, Cast( void**, &coretype ), 0 );
  AssertCrash( found );
  switch( coretype->type ) {
    case coretypetype_t::num: {
      switch( coretype->num.numtype ) {
        case numtype_t::u8_:
        case numtype_t::u16_:
        case numtype_t::u32_:
        case numtype_t::u64_:
        case numtype_t::uint_:
        case numtype_t::bool_: {
          Init( sizeinfo, sizeinfotype_t::unsigned_ );
        } break;

        case numtype_t::s8_:
        case numtype_t::s16_:
        case numtype_t::s32_:
        case numtype_t::s64_:
        case numtype_t::int_: {
          Init( sizeinfo, sizeinfotype_t::signed_ );
        } break;

        case numtype_t::f32_:
        case numtype_t::f64_: {
          Init( sizeinfo, sizeinfotype_t::float_ );
        } break;

        default: UnreachableCrash();
      }

      switch( coretype->num.numtype ) {
        case numtype_t::u8_:
        case numtype_t::s8_:
        case numtype_t::bool_: {
          sizeinfo->bitcount = 8;
        } break;

        case numtype_t::u16_:
        case numtype_t::s16_: {
          sizeinfo->bitcount = 16;
        } break;

        case numtype_t::u32_:
        case numtype_t::s32_:
        case numtype_t::f32_: {
          sizeinfo->bitcount = 32;
        } break;

        case numtype_t::u64_:
        case numtype_t::s64_:
        case numtype_t::f64_: {
          sizeinfo->bitcount = 64;
        } break;

        case numtype_t::int_:
        case numtype_t::uint_: {
          sizeinfo->bitcount = ptr_bitcount;
        } break;

        default: UnreachableCrash();
      }

      sizeinfo->bitalign = sizeinfo->bitcount;
    } return;

    case coretypetype_t::func: {
      Init( sizeinfo, sizeinfotype_t::none );
    } return;

    case coretypetype_t::struct_: {
      Init( sizeinfo, sizeinfotype_t::struct_ );
      ForLen( i, coretype->struct_.fields ) {
        auto field_ptrtype = coretype->struct_.fields.mem + i;
        auto field_sizeinfo = AddBack( sizeinfo->fields );
        SizeinfoFromPtrtype( field_sizeinfo, field_ptrtype, ptr_bitcount );
        // align sizeinfo->bitcount for the field.
        auto rem = sizeinfo->bitcount % field_sizeinfo->bitalign;
        if( rem ) {
          sizeinfo->bitcount += ( field_sizeinfo->bitalign - rem );
        }
        *AddBack( sizeinfo->field_bitoffsets ) = sizeinfo->bitcount;
        sizeinfo->bitcount += field_sizeinfo->bitcount;
        sizeinfo->bitalign = MAX( sizeinfo->bitalign, field_sizeinfo->bitalign );
      }
      // align sizeinfo->bitcount for contiguous arrays of this struct
      auto rem = sizeinfo->bitcount % sizeinfo->bitalign;
      if( rem ) {
        sizeinfo->bitcount += ( sizeinfo->bitalign - rem );
      }
    } return;

    default: UnreachableCrash();
  }
  return;
}











#if 1

// ============================================================================
//
// NOTE: debug code below here.
//

Inl u8*
StringFromNodeType( nodetype_t& type )
{
  switch( type ) {
    #define CASE( x )   case nodetype_t::x: return Str( # x );
    NODETYPES( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}

Inl u8*
StringFromLiteralnumType( numtype_t& type )
{
  switch( type ) {
    #define CASE( x )   case numtype_t::x: return Str( # x );
    NUMTYPES( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}

Inl u8*
StringFromUnopType( unoptype_t& type )
{
  switch( type ) {
    #define CASE( x )   case unoptype_t::x: return Str( # x );
    UNOPTYPES( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}

Inl u8*
StringFromBinopType( binoptype_t& type )
{
  switch( type ) {
    #define CASE( x )   case binoptype_t::x: return Str( # x );
    BINOPTYPES( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}

Inl u8*
StringFromBinassignType( binassigntype_t& type )
{
  switch( type ) {
    #define CASE( x )   case binassigntype_t::x: return Str( # x );
    BINASSIGNTYPES( CASE )
    #undef CASE
    default: UnreachableCrash();
  }
  return {};
}



Inl void
PrintAstNode( ast_t& ast, slice_t& src, array_t<u8>& dst, void* elem )
{
  nodetype_t& type = *Cast( nodetype_t*, elem );
  switch( type ) {
    case nodetype_t::num: {
      auto& node = *Cast( node_num_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
      Memmove( AddBack( dst, 2 ), "( ", 2 );
      //u8* numtype = StringFromLiteralnumType( node. );
      //AddBack( dst, numtype, CsLen( numtype ) );
      //AddBack( dst, "; ", 2 );
      Memmove( AddBack( dst, node.literal.len ), ML( node.literal ) );
      Memmove( AddBack( dst, 2 ), " )", 2 );
    } break;

    case nodetype_t::str: {
      node_str_t& node = *Cast( node_str_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
      Memmove( AddBack( dst, 2 ), "( ", 2 );
      Memmove( AddBack( dst, node.literal.len ), ML( node.literal ) );
      Memmove( AddBack( dst, 2 ), " )", 2 );
    } break;

    case nodetype_t::chr: {
      node_chr_t& node = *Cast( node_chr_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
      Memmove( AddBack( dst, 2 ), "( ", 2 );
      Memmove( AddBack( dst, node.literal.len ), ML( node.literal ) );
      Memmove( AddBack( dst, 2 ), " )", 2 );
    } break;

    case nodetype_t::ident: {
      node_ident_t& node = *Cast( node_ident_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
      Memmove( AddBack( dst, 2 ), "( ", 2 );
      Memmove( AddBack( dst, node.name.len ), ML( node.name ) );
      Memmove( AddBack( dst, 2 ), " )", 2 );
    } break;

    case nodetype_t::unop: {
      node_unop_t& node = *Cast( node_unop_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
      Memmove( AddBack( dst, 2 ), "::", 2 );
      u8* unoptype = StringFromUnopType( node.unoptype );
      auto unoptype_len = CsLen( unoptype );
      Memmove( AddBack( dst, unoptype_len ), unoptype, unoptype_len );
      Memmove( AddBack( dst, 2 ), "( ", 2 );
      PrintAstNode( ast, src, dst, node.e4 );
      Memmove( AddBack( dst, 2 ), " )", 2 );
    } break;

//    case nodetype_t::binop: {
//      node_binop_t& node = *Cast( node_binop_t*, elem );
//      u8* nodetype = StringFromNodeType( type );
//      auto nodetype_len = CsLen( nodetype );
//      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
//      Memmove( AddBack( dst, 2 ), "::", 2 );
//      u8* binoptype = StringFromBinopType( node.binoptype );
//      auto binoptype_len = CsLen( binoptype );
//      Memmove( AddBack( dst, binoptype_len ), binoptype, binoptype_len );
//      Memmove( AddBack( dst, 2 ), "( ", 2 );
//      PrintAstNode( ast, src, dst, node.varg0 );
//      Memmove( AddBack( dst, 3 ), " , ", 3 );
//      PrintAstNode( ast, src, dst, node.varg1 );
//      Memmove( AddBack( dst, 2 ), " )", 2 );
//    } break;

    case nodetype_t::funccall: {
      node_funccall_t& node = *Cast( node_funccall_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
      Memmove( AddBack( dst, 2 ), "::", 2 );
      PrintAstNode( ast, src, dst, node.ident );
      if( node.e5_args.len ) {
        Memmove( AddBack( dst, 2 ), "( ", 2 );
        For( i, 0, node.e5_args.len - 1 ) {
          PrintAstNode( ast, src, dst, node.e5_args.mem[i] );
          Memmove( AddBack( dst, 3 ), " , ", 3 );
        }
        PrintAstNode( ast, src, dst, node.e5_args.mem[node.e5_args.len-1] );
        Memmove( AddBack( dst, 2 ), " )", 2 );
      } else {
        Memmove( AddBack( dst, 2 ), "()", 2 );
      }
    } break;

    case nodetype_t::decl: {
      node_decl_t& node = *Cast( node_decl_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
      Memmove( AddBack( dst, 2 ), "( ", 2 );
      PrintAstNode( ast, src, dst, node.ident );
      Memmove( AddBack( dst, 3 ), " ; ", 3 );
      PrintAstNode( ast, src, dst, node.e5 );
      Memmove( AddBack( dst, 2 ), " )", 2 );
    } break;

//    case nodetype_t::declassignexpr: {
//      node_declassignexpr_t& node = *Cast( node_declassignexpr_t*, elem );
//      u8* nodetype = StringFromNodeType( type );
//      auto nodetype_len = CsLen( nodetype );
//      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
//      Memmove( AddBack( dst, 2 ), "( ", 2 );
//      PrintAstNode( ast, src, dst, node.ident );
//      Memmove( AddBack( dst, 3 ), " ; ", 3 );
//      PrintAstNode( ast, src, dst, node.vtype );
//      Memmove( AddBack( dst, 3 ), " ; ", 3 );
//      PrintAstNode( ast, src, dst, node.vexpr );
//      Memmove( AddBack( dst, 2 ), " )", 2 );
//    } break;

//    case nodetype_t::declassignscope: {
//      node_declassignscope_t& node = *Cast( node_declassignscope_t*, elem );
//      u8* nodetype = StringFromNodeType( type );
//      auto nodetype_len = CsLen( nodetype );
//      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
//      Memmove( AddBack( dst, 2 ), "( ", 2 );
//      PrintAstNode( ast, src, dst, node.ident );
//      Memmove( AddBack( dst, 3 ), " , ", 3 );
//      PrintAstNode( ast, src, dst, node.vtype );
//      Memmove( AddBack( dst, 3 ), " , ", 3 );
//      PrintAstNode( ast, src, dst, node.scope );
//      Memmove( AddBack( dst, 2 ), " )", 2 );
//    } break;

    case nodetype_t::scope: {
      node_scope_t& node = *Cast( node_scope_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
      if( node.statements.len ) {
        Memmove( AddBack( dst, 2 ), "( ", 2 );
        For( i, 0, node.statements.len - 1 ) {
          PrintAstNode( ast, src, dst, node.statements.mem[i] );
          Memmove( AddBack( dst, 3 ), " ; ", 3 );
        }
        PrintAstNode( ast, src, dst, node.statements.mem[node.statements.len-1] );
        Memmove( AddBack( dst, 2 ), " )", 2 );
      } else {
        Memmove( AddBack( dst, 2 ), "()", 2 );
      }
    } break;

//    case nodetype_t::func: {
//      node_typefunc_t& node = *Cast( node_typefunc_t*, elem );
//      u8* nodetype = StringFromNodeType( type );
//      auto nodetype_len = CsLen( nodetype );
//      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
//      Memmove( AddBack( dst, 2 ), "( ", 2 );
//      Memmove( AddBack( dst, 8 ), "returns ", 8 );
//      PrintAstNode( ast, src, dst, node.typenonfunc_return );
//      Memmove( AddBack( dst, 3 ), " ; ", 3 );
//      AssertCrash( node.vtype_args.len == node.ident_args.len );
//      ForLen( i, node.ident_args ) {
//        PrintAstNode( ast, src, dst, node.ident_args.mem[i] );
//        Memmove( AddBack( dst, 3 ), " : ", 3 );
//        PrintAstNode( ast, src, dst, node.vtype_args.mem[i+1] );
//        if( i + 1 < node.ident_args.len ) {
//          Memmove( AddBack( dst, 3 ), " , ", 3 );
//        }
//      }
//      Memmove( AddBack( dst, 2 ), " )", 2 );
//    } break;

//    case nodetype_t::nonfunc: {
//      node_typenonfunc_t& node = *Cast( node_typenonfunc_t*, elem );
//      u8* nodetype = StringFromNodeType( type );
//      auto nodetype_len = CsLen( nodetype );
//      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
//      Memmove( AddBack( dst, 2 ), "( ", 2 );
//      Fori( u8, i, 0, node.ptrlevel ) {
//        Memmove( AddBack( dst, 1 ), "&", 1 );
//      }
//      Memmove( AddBack( dst, node.name.len ), node.name.name, node.name.len );
//      Memmove( AddBack( dst, 2 ), " )", 2 );
//    } break;

    case nodetype_t::ret: {
      node_ret_t& node = *Cast( node_ret_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
      Memmove( AddBack( dst, 2 ), "( ", 2 );
      PrintAstNode( ast, src, dst, node.e5 );
      Memmove( AddBack( dst, 2 ), " )", 2 );
    } break;

    case nodetype_t::continue_: {
//      node_continue_t& node = *Cast( node_continue_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
    } break;

    case nodetype_t::break_: {
//      node_break_t& node = *Cast( node_break_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
    } break;

    case nodetype_t::while_: {
      node_while_t& node = *Cast( node_while_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
      Memmove( AddBack( dst, 2 ), "( ", 2 );
      PrintAstNode( ast, src, dst, node.e5 );
      Memmove( AddBack( dst, 3 ), " ; ", 3 );
      PrintAstNode( ast, src, dst, node.scope );
      Memmove( AddBack( dst, 2 ), " )", 2 );
    } break;

    case nodetype_t::ifchain: {
      node_ifchain_t& node = *Cast( node_ifchain_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
      Memmove( AddBack( dst, 2 ), "( ", 2 );
      Memmove( AddBack( dst, 2 ), "if", 2 );
      Memmove( AddBack( dst, 2 ), "( ", 2 );
      PrintAstNode( ast, src, dst, node.if_e5 );
      Memmove( AddBack( dst, 3 ), " ; ", 3 );
      PrintAstNode( ast, src, dst, node.if_scope );
      Memmove( AddBack( dst, 2 ), " )", 2 );
      Memmove( AddBack( dst, 3 ), " ; ", 3 );
      AssertCrash( node.elif_e5s.len == node.elif_scopes.len );
      if( node.elif_scopes.len ) {
        For( i, 0, node.elif_scopes.len - 1 ) {
          auto vexpr_elif = node.elif_e5s.mem[i];
          auto scope_elif = node.elif_scopes.mem[i];
          Memmove( AddBack( dst, 4 ), "elif", 4 );
          Memmove( AddBack( dst, 2 ), "( ", 2 );
          PrintAstNode( ast, src, dst, vexpr_elif );
          Memmove( AddBack( dst, 3 ), " ; ", 3 );
          PrintAstNode( ast, src, dst, scope_elif );
          Memmove( AddBack( dst, 2 ), " )", 2 );
          Memmove( AddBack( dst, 3 ), " ; ", 3 );
        }
        auto vexpr_elif = node.elif_e5s.mem[node.elif_e5s.len - 1];
        auto scope_elif = node.elif_scopes.mem[node.elif_scopes.len - 1];
        Memmove( AddBack( dst, 4 ), "elif", 4 );
        Memmove( AddBack( dst, 2 ), "( ", 2 );
        PrintAstNode( ast, src, dst, vexpr_elif );
        Memmove( AddBack( dst, 3 ), " ; ", 3 );
        PrintAstNode( ast, src, dst, scope_elif );
        Memmove( AddBack( dst, 2 ), " )", 2 );
      }
      if( node.else_scope ) {
        Memmove( AddBack( dst, 4 ), "else", 4 );
        Memmove( AddBack( dst, 2 ), "( ", 2 );
        PrintAstNode( ast, src, dst, node.else_scope );
        Memmove( AddBack( dst, 2 ), " )", 2 );
      }
      Memmove( AddBack( dst, 2 ), " )", 2 );
    } break;

    case nodetype_t::binassign: {
      node_binassign_t& node = *Cast( node_binassign_t*, elem );
      u8* nodetype = StringFromNodeType( type );
      auto nodetype_len = CsLen( nodetype );
      Memmove( AddBack( dst, nodetype_len ), nodetype, nodetype_len );
      Memmove( AddBack( dst, 2 ), "::", 2 );
      u8* binassigntype = StringFromBinassignType( node.binassigntype );
      auto binassigntype_len = CsLen( binassigntype );
      Memmove( AddBack( dst, binassigntype_len ), binassigntype, binassigntype_len );
      Memmove( AddBack( dst, 2 ), "( ", 2 );
      PrintAstNode( ast, src, dst, node.e0_l );
      Memmove( AddBack( dst, 3 ), " , ", 3 );
      PrintAstNode( ast, src, dst, node.e5_r );
      Memmove( AddBack( dst, 2 ), " )", 2 );
    } break;

    default: UnreachableCrash();
  }
}

Inl void
PrintAst( ast_t& ast, slice_t& src, array_t<u8>& dst )
{
  PrintAstNode( ast, src, dst, ast.scope_toplevel );
}

#endif
