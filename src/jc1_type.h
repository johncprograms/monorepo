// Copyright (c) John A. Carlos Jr., all rights reserved.


static symbol_t Typ_u8     = { Str( "u8"     ), 2 };
static symbol_t Typ_u16    = { Str( "u16"    ), 3 };
static symbol_t Typ_u32    = { Str( "u32"    ), 3 };
static symbol_t Typ_u64    = { Str( "u64"    ), 3 };
static symbol_t Typ_uint   = { Str( "uint"   ), 4 };
static symbol_t Typ_s8     = { Str( "s8"     ), 2 };
static symbol_t Typ_s16    = { Str( "s16"    ), 3 };
static symbol_t Typ_s32    = { Str( "s32"    ), 3 };
static symbol_t Typ_s64    = { Str( "s64"    ), 3 };
static symbol_t Typ_int    = { Str( "int"    ), 3 };
static symbol_t Typ_f32    = { Str( "f32"    ), 3 };
static symbol_t Typ_f64    = { Str( "f64"    ), 3 };
static symbol_t Typ_bool   = { Str( "bool"   ), 4 };
//static symbol_t Typ_string = { Str( "string" ), 6 };

struct
builtin_num_t
{
  symbol_t name;
  numtype_t type;
};

static builtin_num_t g_builtin_numtypes[] = {
  { Typ_u8    , numtype_t::u8_   },
  { Typ_u16   , numtype_t::u16_  },
  { Typ_u32   , numtype_t::u32_  },
  { Typ_u64   , numtype_t::u64_  },
  { Typ_uint  , numtype_t::uint_ },
  { Typ_s8    , numtype_t::s8_   },
  { Typ_s16   , numtype_t::s16_  },
  { Typ_s32   , numtype_t::s32_  },
  { Typ_s64   , numtype_t::s64_  },
  { Typ_int   , numtype_t::int_  },
  { Typ_f32   , numtype_t::f32_  },
  { Typ_f64   , numtype_t::f64_  },
  { Typ_bool  , numtype_t::bool_ },
  };


Inl void
AddBuiltinTypes(
  node_scope_t* scope
  )
{
  ForEach( builtin_num, g_builtin_numtypes ) {
    coretype_t coretype;
    Init( &coretype.num );
    coretype.num.numtype = builtin_num.type;

    bool already_there;
    idx_t idx;
    Add( scope->defined_types, &coretype, 0, &idx, &already_there, 0 );
    AssertCrash( !already_there );

    Add( scope->types_from_typenames, Cast( void*, &builtin_num.name ), &idx, &already_there, 0, 0 );
    AssertCrash( !already_there );
  }
}

Inl coretype_t*
GetCoretypeFromPtrtype(
  ptrtype_t* ptrtype
  )
{
  AssertCrash( ptrtype->scope );
  AssertCrash( !ptrtype->iszero );
  bool found;
  coretype_t* coretype;
  GetElement( ptrtype->scope->defined_types, ptrtype->idx, &found, Cast( void**, &coretype ), 0 );
  if( found ) {
    return coretype;
  }
  return 0;
}


Inl ptrtype_t
LookupPtrtype(
  stack_resizeable_cont_t<node_scope_t*>& scopestack,
  symbol_t& type_name,
  bool* dst_found
  )
{
  AssertCrash( scopestack.len );
  *dst_found = 0;
  ptrtype_t r = {};
  auto top = scopestack.len;
  while( top-- ) {
    auto scope = scopestack.mem[top];
    bool found;
    idx_t idx;
    Lookup( scope->types_from_typenames, &type_name, &found, &idx );
    if( found ) {
      r.idx = idx;
      r.scope = scope;
      *dst_found = 1;
      break;
    }
  }
  return r;
}


struct
tcontext_t
{
  ast_t* ast;
  src_t* src;
  tlog_t* log;
  pagelist_t* mem;
  u32 ptr_bitcount;
};

void
Error(
  token_t* tkn,
  tcontext_t* context,
  void* errstr
  )
{
  Error( tkn, context->src, context->log, errstr );
}



// TODO: do we need constraints?

#define TYPE( type, name ) \
  Inl void \
  name( \
    tcontext_t* context, \
    stack_resizeable_cont_t<node_scope_t*>& scopestack, \
    type* node, \
    bool has_constraints = 0, \
    stack_resizeable_cont_t<ptrtype_t>* constraints = 0 \
    )

#define TYPE_DEFN( type, name ) \
  Inl void \
  name( \
    tcontext_t* context, \
    stack_resizeable_cont_t<node_scope_t*>& scopestack, \
    type* node, \
    bool has_constraints, \
    stack_resizeable_cont_t<ptrtype_t>* constraints \
    )



struct
addnumtype_t
{
  u64 max;
  symbol_t typname;
};

TYPE( node_num_t, TypeNum )
{
  // determine which numtype_t this could be.
  bool has_decimal = 0;
  For( i, 0, node->literal.len ) {
    auto c = node->literal.mem[i];
    if( c == '.' ) {
      has_decimal = 1;
    }
  }
  static symbol_t zero = { Str( "0" ), 1 };
  static symbol_t one  = { Str( "1" ), 1 };
  bool iszero = SymbolEqual( node->literal, zero );
  bool isone  = SymbolEqual( node->literal, one );
  if( iszero  |  isone ) {
    static symbol_t types[] = {
      Typ_u8, Typ_u16, Typ_u32, Typ_u64,
      Typ_s8, Typ_s16, Typ_s32, Typ_s64,
      Typ_int, Typ_uint,
      Typ_f32, Typ_f64,
      Typ_bool,
    };
    ForEach( type, types ) {
      bool found;
      ptrtype_t ptrtype = LookupPtrtype( scopestack, type, &found );
      AssertCrash( found );
      AddLast( node->ptrtypes )->value = ptrtype;
    }
    // special case:
    //   zero is convertible to ptrtype's of all ptrlevels > 0.
    if( iszero ) {
      ptrtype_t ptrtype = {};
      ptrtype.iszero = 1;
      AddLast( node->ptrtypes )->value = ptrtype;

      node->value_f64 = 0.0;
      node->value_f32 = 0.0f;
      node->value_u64 = 0;

    } else {
      AssertCrash( isone );
      node->value_f64 = 1.0;
      node->value_f32 = 1.0f;
      node->value_u64 = 1;
    }
  } elif( has_decimal ) {
    static symbol_t types[] = {
      Typ_f32, Typ_f64,
    };
    ForEach( type, types ) {
      bool found;
      ptrtype_t ptrtype = LookupPtrtype( scopestack, type, &found );
      AssertCrash( found );
      AddLast( node->ptrtypes )->value = ptrtype;
    }
    node->value_f64 = CsTo_f64( ML( node->literal ) );
    node->value_f32 = CsTo_f32( ML( node->literal ) );
    node->value_u64 = MAX_u64;

  } else {

    // TODO: binary, hex, octal

    u64 value = 0;
    u64 digit_factor = 1;
    ReverseFor( i, 0, node->literal.len ) {
      auto c = node->literal.mem[i];
      if( c == ','  ||  c == '_'  ||  c == ' ' ) {
        continue;
      }
      AssertCrash( '0' <= c  &&  c <= '9' );
      u64 digit = c - '0';
      u8 carry = 0;
      carry = _addcarry_u64( carry, value, digit * digit_factor, &value );
      if( carry ) {
        Error( node->tkn, context, "integer is larger than u64 can represent!" );
        return;
      }
      digit_factor *= 10;
    }

    node->value_u64 = value;
    node->value_f64 = Cast( f64, value );
    node->value_f32 = Cast( f32, value );

    // limit possibilities by size of number.

    // TODO: shoot, we also need the negative sign as part of the literal, to get
    // the min value as well. since | MIN_sX | = MAX_sX + 1
//    ImplementCrash();

    u64 max_int  = context->ptr_bitcount == 64  ?  MAX_s64  :  AllOnes( context->ptr_bitcount );
    u64 max_uint = context->ptr_bitcount == 64  ?  MAX_u64  :  AllOnes( context->ptr_bitcount );
    addnumtype_t cases[] = {
      { MAX_u8  , Typ_u8   },
      { MAX_u16 , Typ_u16  },
      { MAX_u32 , Typ_u32  },
      { MAX_u64 , Typ_u64  },
      { MAX_s8  , Typ_s8   },
      { MAX_s16 , Typ_s16  },
      { MAX_s32 , Typ_s32  },
      { MAX_s64 , Typ_s64  },
      { max_int , Typ_int  },
      { max_uint, Typ_uint },
    };
    ForEach( entry, cases ) {
      if( value <= entry.max ) {
        bool found;
        ptrtype_t ptrtype = LookupPtrtype( scopestack, entry.typname, &found );
        AssertCrash( found );
        AddLast( node->ptrtypes )->value = ptrtype;
      }
    }

    static symbol_t types[] = {
      Typ_f64,
      Typ_f32,
    };
    ForEach( type, types ) {
      bool found;
      ptrtype_t ptrtype = LookupPtrtype( scopestack, type, &found );
      AssertCrash( found );
      AddLast( node->ptrtypes )->value = ptrtype;
    }
  }
}
TYPE( node_str_t, TypeStr )
{
  bool found;
  auto ptrtype = LookupPtrtype( scopestack, Typ_u8, &found ); // TODO: switch to string
  AssertCrash( found );
  ptrtype.ptrlevel = 1;
  AddLast( node->ptrtypes )->value = ptrtype;
}
TYPE( node_chr_t, TypeChr )
{
  bool found;
  AddLast( node->ptrtypes )->value = LookupPtrtype( scopestack, Typ_u8, &found );
  AssertCrash( found );
}

Inl listwalloc_t<ptrtype_t>*
LookupIdentPtrtypes(
  tcontext_t* context,
  stack_resizeable_cont_t<node_scope_t*>& scopestack,
  node_ident_t* node
  )
{
  AssertCrash( node );
  varentry_t* entry = 0;
  auto top = scopestack.len;
  bool found = 0;
  while( top-- ) {
    auto scope = scopestack.mem[top];
    LookupRaw( scope->vartable, &node->name, &found, Cast( void**, &entry ) );
    if( found ) {
      break;
    }
  }
  if( !found ) {
    Error( node->tkn, context, "undeclared identifier." );
    return 0;
  } else {
    return &entry->ptrtypes;
  }
}

TYPE( node_e0_t, TypeE0 );

Inl bool
IsIntegerType(
  stack_resizeable_cont_t<node_scope_t*>& scopestack,
  ptrtype_t* ptrtype
  )
{
  static symbol_t numtypes[] = {
    Typ_u8, Typ_u16, Typ_u32, Typ_u64,
    Typ_s8, Typ_s16, Typ_s32, Typ_s64,
    Typ_int, Typ_uint,
    Typ_bool,
  };
  For( i, 0, _countof( numtypes ) ) {
    bool found;
    ptrtype_t numtype = LookupPtrtype( scopestack, numtypes[i], &found );
    AssertCrash( found );
    if( PtrtypeConvertible( numtype, *ptrtype ) ) {
      return 1;
    }
  }
  return 0;
}

Inl bool
IsFloatType(
  stack_resizeable_cont_t<node_scope_t*>& scopestack,
  ptrtype_t* ptrtype
  )
{
  static symbol_t numtypes[] = {
    Typ_f32, Typ_f64,
  };
  For( i, 0, _countof( numtypes ) ) {
    bool found;
    ptrtype_t numtype = LookupPtrtype( scopestack, numtypes[i], &found );
    AssertCrash( found );
    if( PtrtypeConvertible( numtype, *ptrtype ) ) {
      return 1;
    }
  }
  return 0;
}

Inl bool
IsNumType(
  stack_resizeable_cont_t<node_scope_t*>& scopestack,
  ptrtype_t* ptrtype
  )
{
  static symbol_t numtypes[] = {
    Typ_u8, Typ_u16, Typ_u32, Typ_u64,
    Typ_s8, Typ_s16, Typ_s32, Typ_s64,
    Typ_int, Typ_uint,
    Typ_f32, Typ_f64,
    Typ_bool,
  };
  For( i, 0, _countof( numtypes ) ) {
    bool found;
    ptrtype_t numtype = LookupPtrtype( scopestack, numtypes[i], &found );
    AssertCrash( found );
    if( PtrtypeConvertible( numtype, *ptrtype ) ) {
      return 1;
    }
  }
  return 0;
}

Inl bool
IsPtrType( ptrtype_t* ptrtype )
{
  bool r = ( ptrtype->ptrlevel > 0 )  |  ptrtype->iszero;
  return r;
}

Inl void
CopyList( listwalloc_t<ptrtype_t>& dst, listwalloc_t<ptrtype_t>& src )
{
  ForList( elem, src ) {
    AddLast( dst )->value = elem->value;
  }
}


TYPE( node_e1_t, TypeE1 )
{
  if( node->e1type == e1type_t::none ) {
    TypeE0( context, scopestack, node->e0_l );
    CopyList( node->ptrtypes, node->e0_l->ptrtypes );
  } else {
    hashset_t results;
    Init(
      results,
      16,
      sizeof( ptrtype_t ),
      0,
      1,
      HashsetPtrtypeEqual_,
      HashsetPtrtypeHash
      );
    TypeE0( context, scopestack, node->e0_l );
    TypeE0( context, scopestack, node->e0_r );
    ForList( elem, node->e0_l->ptrtypes ) {
      auto type = &elem->value;
      if( !IsNumType( scopestack, type ) ) {
        continue;
      }
      ForList( elem2, node->e0_r->ptrtypes ) {
        auto type2 = &elem2->value;
        if( !IsNumType( scopestack, type2 ) ) {
          continue;
        }
        if( PtrtypeConvertible( *type, *type2 ) ) {
          Add( results, type, 0, 0, 0, 0 );
        }
      }
    }
    if( !results.cardinality ) {
      Error( node->tkn, context, "type mismatch across expr!" );
    }
    Clear( node->e0_l->ptrtypes );
    Clear( node->e0_r->ptrtypes );
    ForLen( i, results.elems ) {
      auto elem = ByteArrayElem( hashset_elem_t, results.elems, i );
      if( elem->has_data ) {
        auto ptrtype = *Cast( ptrtype_t*, _GetElemData( results, elem ) );
        AddLast( node->ptrtypes )->value = ptrtype;
        AddLast( node->e0_l->ptrtypes )->value = ptrtype;
        AddLast( node->e0_r->ptrtypes )->value = ptrtype;
      }
    }
    Kill( results );
  }
}
TYPE( node_e2_t, TypeE2 )
{
  if( node->e2type == e2type_t::none ) {
    TypeE1( context, scopestack, node->e1_l );
    CopyList( node->ptrtypes, node->e1_l->ptrtypes );
  } else {
    // this needs to handle "ptr op int" and vice-versa.
    hashset_t results;
    Init(
      results,
      16,
      sizeof( ptrtype_t ),
      0,
      1,
      HashsetPtrtypeEqual_,
      HashsetPtrtypeHash
      );
    TypeE1( context, scopestack, node->e1_l );
    TypeE1( context, scopestack, node->e1_r );
    ForList( elem, node->e1_l->ptrtypes ) {
      auto type = &elem->value;
      bool type_isint = IsIntegerType( scopestack, type );
      bool type_isnum = type_isint  ||  IsFloatType( scopestack, type );
      bool type_isptr = IsPtrType( type );
      ForList( elem2, node->e1_r->ptrtypes ) {
        auto type2 = &elem2->value;
        bool type2_isint = IsIntegerType( scopestack, type2 );
        bool type2_isnum = type_isint  ||  IsFloatType( scopestack, type2 );
        bool type2_isptr = IsPtrType( type2 );
        if( type_isptr  &  type2_isint ) {
          Add( results, type, 0, 0, 0, 0 );
        } elif( type_isint  &  type2_isptr ) {
          Add( results, type2, 0, 0, 0, 0 );
        } elif( type_isnum  &  type2_isnum ) {
          if( PtrtypeConvertible( *type, *type2 ) ) {
            Add( results, type, 0, 0, 0, 0 );
          }
          break;
        }
      }
    }
    if( !results.cardinality ) {
      Error( node->tkn, context, "type mismatch across expr!" );
    }
    Clear( node->e1_l->ptrtypes );
    Clear( node->e1_r->ptrtypes );
    ForLen( i, results.elems ) {
      auto elem = ByteArrayElem( hashset_elem_t, results.elems, i );
      if( elem->has_data ) {
        auto ptrtype = *Cast( ptrtype_t*, _GetElemData( results, elem ) );
        AddLast( node->ptrtypes )->value = ptrtype;
        AddLast( node->e1_l->ptrtypes )->value = ptrtype;
        AddLast( node->e1_r->ptrtypes )->value = ptrtype;
      }
    }
    Kill( results );
  }
}
TYPE( node_e3_t, TypeE3 )
{
  if( node->e3type == e3type_t::none ) {
    TypeE2( context, scopestack, node->e2_l );
    CopyList( node->ptrtypes, node->e2_l->ptrtypes );
  } else {
    hashset_t results;
    Init(
      results,
      16,
      sizeof( ptrtype_t ),
      0,
      1,
      HashsetPtrtypeEqual_,
      HashsetPtrtypeHash
      );
    TypeE2( context, scopestack, node->e2_l );
    TypeE2( context, scopestack, node->e2_r );
    ForList( elem, node->e2_l->ptrtypes ) {
      auto type = &elem->value;
      if( !IsNumType( scopestack, type ) ) {
        continue;
      }
      ForList( elem2, node->e2_r->ptrtypes ) {
        auto type2 = &elem2->value;
        if( !IsNumType( scopestack, type2 ) ) {
          continue;
        }
        if( PtrtypeConvertible( *type, *type2 ) ) {
          Add( results, type, 0, 0, 0, 0 );
        }
      }
    }
    if( !results.cardinality ) {
      Error( node->tkn, context, "type mismatch across expr!" );
    }
    Clear( node->e2_l->ptrtypes );
    Clear( node->e2_r->ptrtypes );
    ForLen( i, results.elems ) {
      auto elem = ByteArrayElem( hashset_elem_t, results.elems, i );
      if( elem->has_data ) {
        auto ptrtype = *Cast( ptrtype_t*, _GetElemData( results, elem ) );
        AddLast( node->ptrtypes )->value = ptrtype;
        AddLast( node->e2_l->ptrtypes )->value = ptrtype;
        AddLast( node->e2_r->ptrtypes )->value = ptrtype;
      }
    }
    Kill( results );
  }
}
TYPE( node_e4_t, TypeE4 )
{
  if( node->e4type == e4type_t::none ) {
    TypeE3( context, scopestack, node->e3_l );
    CopyList( node->ptrtypes, node->e3_l->ptrtypes );
  } else {
    hashset_t results;
    Init(
      results,
      16,
      sizeof( ptrtype_t ),
      0,
      1,
      HashsetPtrtypeEqual_,
      HashsetPtrtypeHash
      );
    TypeE3( context, scopestack, node->e3_l );
    TypeE3( context, scopestack, node->e3_r );
    ForList( elem, node->e3_l->ptrtypes ) {
      auto type = &elem->value;
      if( !IsNumType( scopestack, type ) ) {
        continue;
      }
      ForList( elem2, node->e3_r->ptrtypes ) {
        auto type2 = &elem2->value;
        if( !IsNumType( scopestack, type2 ) ) {
          continue;
        }
        if( PtrtypeConvertible( *type, *type2 ) ) {
          Add( results, type, 0, 0, 0, 0 );
        }
      }
    }
    if( !results.cardinality ) {
      Error( node->tkn, context, "type mismatch across expr!" );
    }
    Clear( node->e3_l->ptrtypes );
    Clear( node->e3_r->ptrtypes );
    ForLen( i, results.elems ) {
      auto elem = ByteArrayElem( hashset_elem_t, results.elems, i );
      if( elem->has_data ) {
        auto ptrtype = *Cast( ptrtype_t*, _GetElemData( results, elem ) );
        AddLast( node->ptrtypes )->value = ptrtype;
        AddLast( node->e3_l->ptrtypes )->value = ptrtype;
        AddLast( node->e3_r->ptrtypes )->value = ptrtype;
      }
    }
    Kill( results );
  }
}
TYPE( node_e5_t, TypeE5 )
{
  if( node->e5type == e5type_t::none ) {
    TypeE4( context, scopestack, node->e4_l );
    CopyList( node->ptrtypes, node->e4_l->ptrtypes );
  } else {
    TypeE4( context, scopestack, node->e4_l );
    TypeE4( context, scopestack, node->e4_r );
    ForList( elem, node->e4_l->ptrtypes ) {
      auto type = &elem->value;
      auto type_isnum = IsNumType( scopestack, type );
      auto type_isptr = IsPtrType( type );

      // TODO: match E4, E3, etc. ?
      // we also reset e4_r in those.

      bool remove = 1;
      if( type_isnum  |  type_isptr ) {
        ForList( elem2, node->e4_r->ptrtypes ) {
          auto type2 = &elem2->value;
          auto type2_isnum = IsNumType( scopestack, type2 );
          auto type2_isptr = IsPtrType( type2 );
          if( type2_isnum  |  type2_isptr ) {
            auto match = ( type_isnum  &  type2_isnum )  |  ( type_isptr  &  type2_isptr );
            if( match  &&  PtrtypeConvertible( *type, *type2 ) ) {
              remove = 0;
              break;
            }
          }
        }
      }
      if( remove ) {
        auto torem = elem;
        elem = elem->prev;
        Rem( node->e4_l->ptrtypes, torem );
      }
    }
    if( !node->e4_l->ptrtypes.len ) {
      Error( node->tkn, context, "type mismatch across expr!" );
    }

    bool found;
    AddLast( node->ptrtypes )->value = LookupPtrtype( scopestack, Typ_bool, &found );
    AssertCrash( found );
  }
}
TYPE( node_funccall_t, TypeFunccall )
{
  auto idents = LookupIdentPtrtypes( context, scopestack, node->ident );
  if( !idents  ||  idents->len != 1 ) {
    Error( node->tkn, context, "identifier has either no type or an ambiguous type!" );
    if( idents  &&  idents->len ) {
      AddLast( node->ptrtypes )->value = idents->first->value;
    }
    return;
  }
  auto coretype_ident = GetCoretypeFromPtrtype( &idents->first->value );
  if( !coretype_ident ) {
    AssertCrash( context->log->nerrs );
    AddLast( node->ptrtypes )->value = idents->first->value;
    return;
  }
  if( coretype_ident->type != coretypetype_t::func ) {
    Error( node->tkn, context, "identifier isn't a function-type!" );
    AddLast( node->ptrtypes )->value = coretype_ident->func.ret;
    return;
  }
  if( coretype_ident->func.args.len != node->e5_args.len ) {
    Error( node->tkn, context, "incorrect number of parameters!" );
    AddLast( node->ptrtypes )->value = coretype_ident->func.ret;
    return;
  }
  ForLen( i, coretype_ident->func.args ) {
    auto arg = coretype_ident->func.args.mem + i;
    auto node_arg = node->e5_args.mem[i];
    TypeE5( context, scopestack, node_arg );
    bool found_convertible = 0;
    ForList( elem2, node_arg->ptrtypes ) {
      auto arg2 = &elem2->value;
      if( PtrtypeConvertible( *arg, *arg2 ) ) {
        if( found_convertible ) {
          Error( node_arg->tkn, context, "parameter type is ambiguous!" );
          AddLast( node->ptrtypes )->value = coretype_ident->func.ret;
          return;
        }
        found_convertible = 1;
      }
    }
    if( !found_convertible ) {
      Error( node_arg->tkn, context, "parameter type doesn't match!" );
      AddLast( node->ptrtypes )->value = coretype_ident->func.ret;
      return;
    }
  }
  if( coretype_ident->func.has_ret ) {
    AddLast( node->ptrtypes )->value = coretype_ident->func.ret;
  }
}
TYPE( node_identdot_t, TypeIdentdot )
{
  // TODO: multi-level identdot.

  if( node->idents.len > 2 ) {
    Error( node->tkn, context, "can't chain dot-dereference!" );
    return;
  }
  auto idents = LookupIdentPtrtypes( context, scopestack, node->idents.mem[0] );
  if( !idents ) {
    Error( node->tkn, context, "identifier has no type!" );
    return;
  }
  if( node->idents.len == 1 ) {
    ForList( elem, *idents ) {
      auto ident = &elem->value;
      AddLast( node->ptrtypes )->value = *ident;
    }
    return;
  }
  bool found_one_match = 0;
  ForList( elem, *idents ) {
    auto ident = &elem->value;
    auto coretype_ident = GetCoretypeFromPtrtype( ident );
    if( !coretype_ident ) {
      AssertCrash( context->log->nerrs );
      continue;
    }
    // narrow down to struct types.
    if( coretype_ident->type != coretypetype_t::struct_ ) {
      continue;
    }
    // narrow down to structs with the named field, and output the named field's ptrtype.
    auto given_ident = node->idents.mem[1];
    bool found_match = 0;
    idx_t match = 0;
    ForLen( j, coretype_ident->struct_.fieldnames ) {
      auto fieldname = coretype_ident->struct_.fieldnames.mem + j;
      if( SymbolEqual( given_ident->name, *fieldname ) ) {
        found_match = 1;
        match = j;
        break;
      }
    }
    if( found_match ) {
      AddLast( node->ptrtypes )->value = coretype_ident->struct_.fields.mem[match];
      found_one_match = 1;
    }
  }
  if( !found_one_match ) {
    Error( node->tkn, context, "identifier doesn't match a struct field!" );
  }
}
TYPE( node_unop_t, TypeUnop )
{
  TypeE4( context, scopestack, node->e4 );
  switch( node->unoptype ) {
    case unoptype_t::deref: {
      ForList( elem, node->ptrtypes ) {
        auto type = &elem->value;
        if( !type->ptrlevel ) {
          auto torem = elem;
          elem = elem->prev;
          Rem( node->ptrtypes, torem );
        }
        type->ptrlevel -= 1;
      }
      if( !node->ptrtypes.len ) {
        Error( node->tkn, context, "can't deref a non-pointer type!" );
      }
    } break;

    case unoptype_t::addrof: {

      // TODO: make sure we can actually take an address.

      ForList( elem, node->ptrtypes ) {
        auto type = &elem->value;
        type->ptrlevel += 1;
      }
      if( !node->ptrtypes.len ) {
        Error( node->tkn, context, "can't take an address of an unknown type!" );
      }
    } break;

    case unoptype_t::negate:
    case unoptype_t::not_: {
      ForList( elem, node->ptrtypes ) {
        auto type = &elem->value;
        if( !IsNumType( scopestack, type ) ) {
          auto torem = elem;
          elem = elem->prev;
          Rem( node->ptrtypes, torem );
        }
      }
      if( !node->ptrtypes.len ) {
        Error( node->tkn, context, "unop expected a number type!" );
      }
    } break;

    default: UnreachableCrash();
  }
}
TYPE_DEFN( node_e0_t, TypeE0 )
{
  switch( node->e0type ) {
    case e0type_t::funccall: {
      TypeFunccall( context, scopestack, node->funccall );
      CopyList( node->ptrtypes, node->funccall->ptrtypes );
    } break;

    case e0type_t::identdot: {
      TypeIdentdot( context, scopestack, node->identdot );
      CopyList( node->ptrtypes, node->identdot->ptrtypes );
    } break;

    case e0type_t::e5s: {
      // TODO: generalize ptrtype_t to stack_resizeable_cont_t<ptrtype_t>, so we can pass around tuples.
      if( node->e5s.len != 1 ) {
        Error( node->tkn, context, "expected exactly one e5 inside parens!" );
      }
      TypeE5( context, scopestack, node->e5s.mem[0] );
      CopyList( node->ptrtypes, node->e5s.mem[0]->ptrtypes );
    } break;

    case e0type_t::unop: {
      TypeUnop( context, scopestack, node->unop );
      CopyList( node->ptrtypes, node->unop->ptrtypes );
    } break;

    case e0type_t::num: {
      TypeNum( context, scopestack, node->num );
      CopyList( node->ptrtypes, node->num->ptrtypes );
    } break;

    case e0type_t::str: {
      TypeStr( context, scopestack, node->str );
      CopyList( node->ptrtypes, node->str->ptrtypes );
    } break;

    case e0type_t::chr: {
      TypeChr( context, scopestack, node->chr );
      CopyList( node->ptrtypes, node->chr->ptrtypes );
    } break;

    default: UnreachableCrash();
  }
}
TYPE( node_type_t, TypeType )
{
  bool found;
  node->ptrtype = LookupPtrtype( scopestack, node->ident->name, &found );
  if( !found ) {
    Error( node->tkn, context, "using undefined type!" );
    // make sure we don't a/v later.
    node->ptrtype.scope = scopestack.mem[ scopestack.len - 1 ];
  }
  node->ptrtype.ptrlevel = node->ptrlevel;
  SizeinfoFromPtrtype( &node->sizeinfo, &node->ptrtype, context->ptr_bitcount );
}
Inl ptrtype_t // TODO: does a decl always have one ptrtype_t ?
TypeDecl(
  tcontext_t* context,
  stack_resizeable_cont_t<node_scope_t*>& scopestack,
  node_decl_t* node,
  bool add_to_scope
  )
{
  AssertCrash( scopestack.len );
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  TypeType( context, scopestack, node->type );
  if( node->e5 ) {
    TypeE5( context, scopestack, node->e5 );
    ForList( elem, node->e5->ptrtypes ) {
      auto e5 = &elem->value;
      if( !PtrtypeConvertible( node->type->ptrtype, *e5 ) ) {
        auto torem = elem;
        elem = elem->prev;
        Rem( node->e5->ptrtypes, torem );
      }
    }
    if( !node->e5->ptrtypes.len ) {
      Error( node->tkn, context, "type of decl doesn't match the default-assigned expr!" );
      return node->type->ptrtype;
    }
  }
  // a function parameter node_decl as a part of node_funcdefn will inject into the function body scope.
  // a struct field node_decl as part of node_datadecl won't inject into the global scope.
  if( add_to_scope ) {
    bool already_there;
    varentry_t entry = {};
    Init( entry.ptrtypes, context->mem );
    AddLast( entry.ptrtypes )->value = node->type->ptrtype;
    Add( scope->vartable, &node->ident->name, &entry, &already_there, 0, 0 );
    if( already_there ) {
      Error( node->tkn, context, "redeclaring identifier!" );
    }
  }
  return node->type->ptrtype;
}

TYPE( node_scope_t, TypeScope );

TYPE( node_funcdeclordefn_t, TypeFuncdeclordefn )
{
  AssertCrash( scopestack.len );
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  if( !node->scope ) {
    // func decl
    coretype_t coretype;
    Init( &coretype.func );
    coretype.func.name = node->ident->name;
    ForLen( i, node->decls ) {
      auto decl = node->decls.mem[i];
      *AddBack( coretype.func.args ) = TypeDecl( context, scopestack, decl, 0 );
    }
    if( node->type ) {
      coretype.func.has_ret = 1;
      TypeType( context, scopestack, node->type );
      coretype.func.ret = node->type->ptrtype;
    }
    bool already_there;
    ptrtype_t ptrtype = {};
    ptrtype.scope = scope;
    Add( scope->defined_types, &coretype, 0, &ptrtype.idx, 0, 0 );
    varentry_t entry = {};
    Init( entry.ptrtypes, context->mem );
    AddLast( entry.ptrtypes )->value = ptrtype;
    Add( scope->vartable, &node->ident->name, &entry, &already_there, 0, 0 );
    if( already_there ) {
      Error( node->tkn, context, "redeclaring identifier!" );
    }
  } else {
    // func defn
    *AddBack( scopestack ) = node->scope;

    coretype_t coretype;
    Init( &coretype.func );
    coretype.func.name = node->ident->name;
    ForLen( i, node->decls ) {
      auto decl = node->decls.mem[i];
      *AddBack( coretype.func.args ) = TypeDecl( context, scopestack, decl, 1 );
    }

    TypeScope( context, scopestack, node->scope );

    RemBack( scopestack );

    if( node->type ) {
      coretype.func.has_ret = 1;
      TypeType( context, scopestack, node->type );
      coretype.func.ret = node->type->ptrtype;
      ForList( elem, node->scope->ptrtypes ) {
        auto ret = &elem->value;
        if( !PtrtypeConvertible( *ret, coretype.func.ret ) ) {
          auto torem = elem;
          elem = elem->prev;
          Rem( node->scope->ptrtypes, torem );
        }
      }
      if( !node->scope->ptrtypes.len ) {
        Error( node->tkn, context, "function ret type doesn't match function body ret type!" );
      }
    } else {
      if( node->scope->ptrtypes.len ) {
        Error( node->tkn, context, "function body isn't declared to return a type!" );
      }
    }
    bool already_there;
    ptrtype_t ptrtype = {};
    ptrtype.scope = scope;
    Add( scope->defined_types, &coretype, 0, &ptrtype.idx, 0, 0 );
    varentry_t entry = {};
    Init( entry.ptrtypes, context->mem );
    AddLast( entry.ptrtypes )->value = ptrtype;
    Add( scope->vartable, &node->ident->name, &entry, &already_there, 0, 0 );
    AssertCrash( !already_there );
  }
}
TYPE( node_datadecl_t, TypeDatadecl )
{
  AssertCrash( scopestack.len );
  auto scope = scopestack.mem[ scopestack.len - 1 ];
  coretype_t coretype;
  Init( &coretype.struct_ );
  coretype.struct_.name = node->ident->name;
  ForLen( i, node->decls ) {
    auto decl = node->decls.mem[i];
    *AddBack( coretype.struct_.fields ) = TypeDecl( context, scopestack, decl, 0 );
    *AddBack( coretype.struct_.fieldnames ) = decl->ident->name;
  }
  ForLen( i, coretype.struct_.fieldnames ) {
    auto fieldname = coretype.struct_.fieldnames.mem + i;
    For( j, i + 1, coretype.struct_.fieldnames.len ) {
      auto fieldname2 = coretype.struct_.fieldnames.mem + j;
      if( SymbolEqual( *fieldname, *fieldname2 ) ) {
        Error( node->decls.mem[i]->tkn, context, "already declared a field with that name!" );
      }
    }
  }
  bool already_there;
  ptrtype_t ptrtype = {};
  ptrtype.scope = scope;
  Add( scope->defined_types, &coretype, 0, &ptrtype.idx, &already_there, 0 );
  if( already_there ) {
    Error( node->tkn, context, "redeclaring data type!" );
    return;
  }
  Add( scope->types_from_typenames, &node->ident->name, &ptrtype.idx, &already_there, 0, 0 );
  if( already_there ) {
    Error( node->tkn, context, "already declared a data type with that name!" );
  }
}
TYPE( node_binassign_t, TypeBinassign )
{
  bool found = 0;
  bool try_implicit_decl =
    node->binassigntype == binassigntype_t::eq  &&
    node->e0_l->e0type == e0type_t::identdot  &&
    node->e0_l->identdot->idents.len == 1;
  if( try_implicit_decl ) {
    auto ident = node->e0_l->identdot->idents.mem[0];
    auto scope = scopestack.mem[ scopestack.len - 1 ];
    LookupRaw( scope->vartable, &ident->name, &found, 0 );
    if( !found ) {
      // implicit decl
      node->implicit_decl = 1;
      varentry_t entry = {};
      Init( entry.ptrtypes, context->mem );
      TypeE5( context, scopestack, node->e5_r );
      CopyList( node->ptrtypes, node->e5_r->ptrtypes );
      bool already_there;
      Add( scope->vartable, &ident->name, &entry, &already_there, 0, 0 );
      if( already_there ) {
        Error( node->tkn, context, "redeclaring identifier!" );
      }
    }
  }
  if( !node->implicit_decl ) {
    TypeE0( context, scopestack, node->e0_l );
    TypeE5( context, scopestack, node->e5_r );
    hashset_t results;
    Init(
      results,
      16,
      sizeof( ptrtype_t ),
      0,
      1,
      HashsetPtrtypeEqual_,
      HashsetPtrtypeHash
      );
    switch( node->binassigntype ) {
      case binassigntype_t::eq: {
        ForList( elem, node->e0_l->ptrtypes ) {
          auto type = &elem->value;
          ForList( elem2, node->e5_r->ptrtypes ) {
            auto type2 = &elem2->value;
            if( PtrtypeConvertible( *type, *type2 ) ) {
              Add( results, &type, 0, 0, 0, 0 );
            }
          }
        }
      } break;

      case binassigntype_t::addeq:
      case binassigntype_t::subeq: {
        ForList( elem, node->e0_l->ptrtypes ) {
          auto type = &elem->value;
          bool type_isint = IsIntegerType( scopestack, type );
          bool type_isnum = type_isint  ||  IsFloatType( scopestack, type );
          bool type_isptr = IsPtrType( type );
          ForList( elem2, node->e5_r->ptrtypes ) {
            auto type2 = &elem2->value;
            bool type2_isint = IsIntegerType( scopestack, type2 );
            bool type2_isnum = type2_isint  ||  IsFloatType( scopestack, type );
            bool type2_isptr = IsPtrType( type2 );
            if( type_isptr  &  type2_isint ) {
              Add( results, type, 0, 0, 0, 0 );
            } elif( type_isint  &  type2_isptr ) {
              // can't binassign a num by a ptr!
            } elif( type_isnum  &  type2_isnum ) {
              // TODO: pick appropriate numtype for op of 2 numtypes.
              Add( results, type, 0, 0, 0, 0 );
              break;
            }
          }
        }
      } break;

      case binassigntype_t::muleq:
      case binassigntype_t::diveq:
      case binassigntype_t::remeq: {
        ForList( elem, node->e0_l->ptrtypes ) {
          auto type = &elem->value;
          bool type_isnum = IsIntegerType( scopestack, type )  ||  IsFloatType( scopestack, type );
          if( !type_isnum ) {
            continue;
          }
          ForList( elem2, node->e5_r->ptrtypes ) {
            auto type2 = &elem2->value;
            bool type2_isnum = IsIntegerType( scopestack, type2 )  ||  IsFloatType( scopestack, type );
            if( !type2_isnum ) {
              continue;
            }
            // TODO: pick appropriate numtype for op of 2 numtypes.
            Add( results, type, 0, 0, 0, 0 );
          }
        }
      } break;

      case binassigntype_t::andeq:
      case binassigntype_t::oreq:
      case binassigntype_t::xoreq:
      case binassigntype_t::shleq:
      case binassigntype_t::shreq: {
        ForList( elem, node->e0_l->ptrtypes ) {
          auto type = &elem->value;
          if( !IsIntegerType( scopestack, type ) ) {
            continue;
          }
          ForList( elem2, node->e5_r->ptrtypes ) {
            auto type2 = &elem2->value;
            if( !IsIntegerType( scopestack, type2 ) ) {
              continue;
            }
            // TODO: pick appropriate numtype for op of 2 numtypes.
            Add( results, type, 0, 0, 0, 0 );
          }
        }
      } break;

      default: UnreachableCrash();
    }
    if( !results.cardinality ) {
      Error( node->tkn, context, "type mismatch across binassign!" );
    }
    Clear( node->e5_r->ptrtypes );
    ForLen( i, results.elems ) {
      auto elem = ByteArrayElem( hashset_elem_t, results.elems, i );
      if( elem->has_data ) {
        auto ptrtype = *Cast( ptrtype_t*, _GetElemData( results, elem ) );
        AddLast( node->ptrtypes )->value = ptrtype;
        AddLast( node->e5_r->ptrtypes )->value = ptrtype;
      }
    }
    Kill( results );
  }
}
TYPE( node_ret_t, TypeRet )
{
  if( node->e5 ) {
    TypeE5( context, scopestack, node->e5 );
    CopyList( node->ptrtypes, node->e5->ptrtypes );
  }
}
TYPE( node_continue_t, TypeContinue )
{
}
TYPE( node_break_t, TypeBreak )
{
}

Inl bool
IsCondType(
  stack_resizeable_cont_t<node_scope_t*>& scopestack,
  ptrtype_t* ptrtype
  )
{
  auto isptr = IsPtrType( ptrtype );
  auto isnum = IsNumType( scopestack, ptrtype );
  return ( isptr  |  isnum );
}

TYPE( node_while_t, TypeWhile )
{
  TypeE5( context, scopestack, node->e5 );
  bool found = 0;
  ForList( elem, node->e5->ptrtypes ) {
    auto cond = &elem->value;
    if( IsCondType( scopestack, cond ) ) {
      found = 1;
    }
  }
  if( !found ) {
    Error( node->e5->tkn, context, "expr type isn't a conditional!" );
  }
  TypeScope( context, scopestack, node->scope );
}
TYPE( node_ifchain_t, TypeIfchain )
{
  AssertCrash( node->elif_e5s.len == node->elif_scopes.len );
  bool found = 0;

  *AddBack( node->elif_e5s ) = node->if_e5;

  ForLen( i, node->elif_e5s ) {
    auto expr = node->elif_e5s.mem[i];
    found = 0;
    TypeE5( context, scopestack, expr );
    ForList( elem, expr->ptrtypes ) {
      auto cond = &elem->value;
      if( IsCondType( scopestack, cond ) ) {
        found = 1;
      }
    }
    if( !found ) {
      Error( expr->tkn, context, "expr type isn't a conditional!" );
    }
  }

  RemBack( node->elif_e5s );

  bool found_first_ret = 0;

  *AddBack( node->elif_scopes ) = node->if_scope;
  if( node->else_scope ) {
    *AddBack( node->elif_scopes ) = node->else_scope;
  }

  ForLen( i, node->elif_scopes ) {
    auto scope = node->elif_scopes.mem[i];
    if( !found_first_ret ) {
      TypeScope( context, scopestack, scope );
      found_first_ret = 1;
    } else {
      TypeScope( context, scopestack, scope );
      if( found_first_ret  &&  !node->ptrtypes.len  &&  scope->ptrtypes.len ) {
        Error( scope->tkn, context, "earlier scopes didn't ret anything!" );
      }
      ForList( elem, node->ptrtypes ) {
        auto ptrtype = &elem->value;
        found = 0;
        ForList( elem2, scope->ptrtypes ) {
          auto ptrtype2 = &elem2->value;
          if( PtrtypeConvertible( *ptrtype, *ptrtype2 ) ) {
            found = 1;
            break;
          }
        }
        if( !found ) {
          auto torem = elem;
          elem = elem->prev;
          Rem( node->ptrtypes, torem );
        }
      }
    }
  }

  if( node->else_scope ) {
    RemBack( node->elif_scopes );
  }
  RemBack( node->elif_scopes );
}
TYPE( node_switch_t, TypeSwitch )
{ // TODO:
}
TYPE( node_case_t, TypeCase )
{ // TODO:
}
TYPE( node_default_t, TypeDefault )
{ // TODO:
}

TYPE( node_statement_t, TypeStatement );

TYPE( node_defer_t, TypeDefer )
{
  TypeStatement( context, scopestack, node->statement );
}
TYPE_DEFN( node_statement_t, TypeStatement )
{
  switch( node->stmtype ) {
    case stmtype_t::funcdeclordefn: {
      TypeFuncdeclordefn( context, scopestack, node->funcdeclordefn );
    } break;

    case stmtype_t::binassign: {
      TypeBinassign( context, scopestack, node->binassign );
      CopyList( node->ptrtypes, node->binassign->ptrtypes );
    } break;

    case stmtype_t::datadecl: {
      TypeDatadecl( context, scopestack, node->datadecl );
    } break;

    case stmtype_t::decl: {
      TypeDecl( context, scopestack, node->decl, 1 );
    } break;

    case stmtype_t::funccall: {
      TypeFunccall( context, scopestack, node->funccall );
      CopyList( node->ptrtypes, node->funccall->ptrtypes );
    } break;

    case stmtype_t::ret: {
      TypeRet( context, scopestack, node->ret );
      CopyList( node->ptrtypes, node->ret->ptrtypes );
    } break;

    case stmtype_t::continue_: {
      TypeContinue( context, scopestack, node->continue_ );
    } break;

    case stmtype_t::break_: {
      TypeBreak( context, scopestack, node->break_ );
    } break;

    case stmtype_t::while_: {
      TypeWhile( context, scopestack, node->while_ );
    } break;

    case stmtype_t::ifchain: {
      TypeIfchain( context, scopestack, node->ifchain );
    } break;

    case stmtype_t::switch_: {
      TypeSwitch( context, scopestack, node->switch_ );
    } break;

    case stmtype_t::case_: {
      TypeCase( context, scopestack, node->case_ );
    } break;

    case stmtype_t::default_: {
      TypeDefault( context, scopestack, node->default_ );
    } break;

    case stmtype_t::defer: {
      TypeDefer( context, scopestack, node->defer );
    } break;

    case stmtype_t::scope: {
      *AddBack( scopestack ) = node->scope;
      TypeScope( context, scopestack, node->scope );
      RemBack( scopestack );
    } break;

    default: {
      UnreachableCrash();
      Error( node->tkn, context, "unknown statement type!" );
    } break;
  }
}
TYPE_DEFN( node_scope_t, TypeScope )
{
  bool found_first_ret = 0;
  ForLen( i, node->statements ) {
    auto statement = node->statements.mem[i];
    bool statement_isret = ( statement->stmtype == stmtype_t::ret );
    if( !found_first_ret  &&  statement_isret ) {
      TypeStatement( context, scopestack, statement );
      CopyList( node->ptrtypes, statement->ptrtypes );
      found_first_ret = 1;
    } else {
      TypeStatement( context, scopestack, statement );
      if( found_first_ret  &&  !node->ptrtypes.len  &&  statement->ptrtypes.len ) {
        Error( statement->tkn, context, "earlier rets didn't pass anything!" );
      }
      ForList( elem, node->ptrtypes ) {
        auto ptrtype = &elem->value;
        bool found = 0;
        ForList( elem2, statement->ptrtypes ) {
          auto ptrtype2 = &elem2->value;
          if( PtrtypeConvertible( *ptrtype, *ptrtype2 ) ) {
            found = 1;
            break;
          }
        }
        if( !found ) {
          auto torem = elem;
          elem = elem->prev;
          Rem( node->ptrtypes, torem );
        }
      }
    }
  }
}



Inl void
Type(
  tcontext_t* context
  )
{
  stack_resizeable_cont_t<node_scope_t*> scopestack;
  Alloc( scopestack, 256 );
  auto scope = context->ast->scope_toplevel;
  AddBuiltinTypes( scope );
  *AddBack( scopestack ) = scope;
  TypeScope( context, scopestack, scope );
  if( scope->ptrtypes.len ) {
    Error( scope->tkn, context, "global scope must have a void type!" );
  }
  RemBack( scopestack );
  AssertCrash( !scopestack.len );
  Free( scopestack );
}



#undef TYPE



#if 0

Inl void
LiteralnumCandidatesFromRaw(
  token_t& tkn,
  literalnum_raw_t& raw,
  stack_resizeable_cont_t<literalnum_candidate_t>& dst,
  src_t& src,
  tlog_t_t& log
  )
{
  literalnum_candidate_t candidate;

  if( raw.is_float ) {
    if( raw.negate ) {
      raw.accumf = -raw.accumf;
    }
    candidate.type = literalnumtype_t::f64_;
    candidate.num._f64 = raw.accumf;
    *AddBack( dst ) = candidate;

    candidate.type = literalnumtype_t::f32_;
    candidate.num._f32 = Cast( f32, raw.accumf );
    *AddBack( dst ) = candidate;

  } else {
    if( raw.negate ) {
      if( raw.accumu > Cast( u64, MAX_s64 ) + 1 ) {
        Error( tkn, src, log, "Cannot negate such a large number. largest signed int is s64!" );
        return;
      }
      s64 accums = -Cast( s64, raw.accumu );

      if( accums >= Cast( s64, MIN_s64 ) ) {
        candidate.type = literalnumtype_t::s64_;
        candidate.num._s64 = Cast( s64, accums );
        *AddBack( dst ) = candidate;
      }
      if( accums >= Cast( s64, MIN_s32 ) ) {
        candidate.type = literalnumtype_t::s32_;
        candidate.num._s32 = Cast( s32, accums );
        *AddBack( dst ) = candidate;
      }
      if( accums >= Cast( s64, MIN_s16 ) ) {
        candidate.type = literalnumtype_t::s16_;
        candidate.num._s16 = Cast( s16, accums );
        *AddBack( dst ) = candidate;
      }
      if( accums >= Cast( s64, MIN_s8 ) ) {
        candidate.type = literalnumtype_t::s8_;
        candidate.num._s8 = Cast( s8, accums );
        *AddBack( dst ) = candidate;
      }

    } else {
      if( raw.accumu <= Cast( u64, MAX_u64 ) ) {
        candidate.type = literalnumtype_t::u64_;
        candidate.num._u64 = Cast( u64, raw.accumu );
        *AddBack( dst ) = candidate;
      }
      if( raw.accumu <= Cast( u64, MAX_s64 ) ) {
        candidate.type = literalnumtype_t::s64_;
        candidate.num._s64 = Cast( s64, raw.accumu );
        *AddBack( dst ) = candidate;
      }
      if( raw.accumu <= Cast( u64, MAX_u32 ) ) {
        candidate.type = literalnumtype_t::u32_;
        candidate.num._u32 = Cast( u32, raw.accumu );
        *AddBack( dst ) = candidate;
      }
      if( raw.accumu <= Cast( u64, MAX_s32 ) ) {
        candidate.type = literalnumtype_t::s32_;
        candidate.num._s32 = Cast( s32, raw.accumu );
        *AddBack( dst ) = candidate;
      }
      if( raw.accumu <= Cast( u64, MAX_u16 ) ) {
        candidate.type = literalnumtype_t::u16_;
        candidate.num._u16 = Cast( u16, raw.accumu );
        *AddBack( dst ) = candidate;
      }
      if( raw.accumu <= Cast( u64, MAX_s16 ) ) {
        candidate.type = literalnumtype_t::s16_;
        candidate.num._s16 = Cast( s16, raw.accumu );
        *AddBack( dst ) = candidate;
      }
      if( raw.accumu <= Cast( u64, MAX_u8 ) ) {
        candidate.type = literalnumtype_t::u8_;
        candidate.num._u8 = Cast( u8, raw.accumu );
        *AddBack( dst ) = candidate;
      }
      if( raw.accumu <= Cast( u64, MAX_s8 ) ) {
        candidate.type = literalnumtype_t::s8_;
        candidate.num._s8 = Cast( s8, raw.accumu );
        *AddBack( dst ) = candidate;
      }
    }
  }
}





  hashset_t matchtable;
  Init(
    matchtable,
    16,
    sizeof( ptrtype_t ),
    0,
    0.75f,
    HashsetPtrtypeEqual,
    HashsetPtrtypeHash
    );

  ForLen( i, node->statements ) {
    auto statement = node->statements.mem[i];
    statements.len = 0;
    TypeStatement( context, scopestack, statement, statements );
    if( statements.len ) {
      if( !matchtable.cardinality ) {
        // populate matchtable with statements
        ForLen( j, statements ) {
          bool already_there;
          Add( matchtable, statements.mem + j, 0, &already_there, 0 );
          AssertCrash( !already_there );
        }
      } else {
        // match rets against matchtable
        rets.len = 0;
        ForLen( j, statements ) {
          auto ptrtype = statements.mem + j;
          bool found;
          Lookup( matchtable, ptrtype, &found, 0 );
          if( found ) {
            *AddBack( rets ) = *ptrtype;
          }
        }
        if( !rets.len ) {
          Error( node->tkn, context, "ret statements in this scope have different types!" );
          break;
        }
        if( matchtable.cardinality > rets.len ) {
          // re-populate matchtable with rets
          Clear( matchtable );
          ForLen( j, rets ) {
            bool already_there;
            Add( matchtable, rets.mem + j, 0, &already_there, 0 );
            AssertCrash( !already_there );
          }
        }
      }
    }
  }
  // pull out all matches from the matchtable
  ForLen( i, matchtable.elems ) {
    auto elem = ByteArrayElem( hashset_elem_t, matchtable.elems, i );
    if( elem->has_data ) {
      AddLast( node->ptrtypes )->value = *Cast( ptrtype_t*, _GetElemData( matchtable, elem ) );
    }
  }
  Kill( matchtable );
#endif
