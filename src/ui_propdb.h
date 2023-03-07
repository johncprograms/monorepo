// Copyright (c) John A. Carlos Jr., all rights reserved.


constant idx_t c_propsize = 24;

CompileAssert( c_propsize >= sizeof( vec4<f32> ) );
CompileAssert( c_propsize >= sizeof( glwkeybind_t ) );


struct
propdb_t
{
  fsobj_t filename;
  hashset_t set;
  pagelist_t strdata;
  u64 time_lastwrite;
};

static propdb_t g_db = {};
static bool g_db_init = 0;


Inl void
_Kill( propdb_t& db )
{
  Kill( db.set );
  Kill( db.strdata );
}



// Assumes a file format s.t.
//   The following will exist on a line, with n >= 1:
//     key = value0 , value1 , ... , valueN-1 ;


Enumc( pdbtokentype_t )
{
  comma,
  equals,
  semicolon,
  expr, // ident string or numeral string.

  COUNT
};


struct
pdbtoken_t
{
  idx_t l;
  idx_t r;
  pdbtokentype_t type;
};


Inl bool
IsNumeralStart( u8 c )
{
  bool r = AsciiIsNumber( c )  |  ( c == '-' );
  return r;
}



bool
Tokenize( stack_resizeable_cont_t<pdbtoken_t>& tokens, slice_t& src )
{
  idx_t pos = 0;
  pdbtoken_t last_tkn;
  last_tkn.l = 0;
  last_tkn.r = 0;
  last_tkn.type = pdbtokentype_t::expr;

  Forever {

    if( pos >= src.len ) {
      break;
    }

    u8 curr_src = src.mem[pos];
    switch( curr_src ) {
      case ',': {
        last_tkn.r = pos;
        *AddBack( tokens ) = last_tkn;

        pdbtoken_t tkn;
        tkn.l = pos;
        tkn.r = pos + 1;
        tkn.type = pdbtokentype_t::comma;
        *AddBack( tokens ) = tkn;

        last_tkn.l = pos + 1;
      } break;

      case '=': {
        last_tkn.r = pos;
        *AddBack( tokens ) = last_tkn;

        pdbtoken_t tkn;
        tkn.l = pos;
        tkn.r = pos + 1;
        tkn.type = pdbtokentype_t::equals;
        *AddBack( tokens ) = tkn;

        last_tkn.l = pos + 1;
      } break;

      case ';': {
        last_tkn.r = pos;
        *AddBack( tokens ) = last_tkn;

        pdbtoken_t tkn;
        tkn.l = pos;
        tkn.r = pos + 1;
        tkn.type = pdbtokentype_t::semicolon;
        *AddBack( tokens ) = tkn;

        last_tkn.l = pos + 1;
      } break;

      default: {

      } break;
    }

    pos += 1;
  }

  // remove lead/trail whitespace from pdbtokentype_t::expr.
  ForLen( i, tokens ) {
    auto tkn = tokens.mem + i;
    if( tkn->type == pdbtokentype_t::expr ) {
      while( tkn->l < tkn->r  &&  AsciiIsWhitespace( src.mem[tkn->l] ) ) {
        tkn->l += 1;
      }
      while( tkn->l < tkn->r  &&  AsciiIsWhitespace( src.mem[tkn->r - 1] ) ) {
        tkn->r -= 1;
      }
    }
  }

  return 1;
}




Inl u8*
StringOfTokenType( pdbtokentype_t type )
{
#define CASEPRINTTKN( x )   case x: return Str( # x );
  switch( type ) {
    CASEPRINTTKN( pdbtokentype_t::comma );
    CASEPRINTTKN( pdbtokentype_t::equals );
    CASEPRINTTKN( pdbtokentype_t::semicolon );
    CASEPRINTTKN( pdbtokentype_t::expr );
    default: UnreachableCrash();
  }
#undef CASEPTRINTTKN
  return 0;
}



#if 0
Inl void
PrintTokenStream( stack_resizeable_cont_t<pdbtoken_t>& tokens, slice_t& src )
{
  ForLen( i, tokens ) {
    auto tkn = tokens.mem + i;
    idx_t tkn_len = tkn->r - tkn->l;
    auto tmp = MemHeapAlloc( u8, tkn_len );
    Memmove( &tmp[0], src.mem + tkn->l, tkn_len );
    tmp[tkn_len] = 0;
    printf(
      "( %s ) ( %llu, %llu ) ( %s )"
      "\n",
      StringOfTokenType( tkn->type ),
      tkn->l, tkn_len,
      &tmp[0]
      );
    MemHeapFree( tmp );
  }
}
#endif


struct
statement_t
{
  slice_t lhs;
  stack_resizeable_cont_t<slice_t> rhs;
};

Inl void
Init( statement_t& stm )
{
  stm.lhs = {};
  Alloc( stm.rhs, 8 );
}

Inl void
Kill( statement_t& stm )
{
  stm.lhs = {};
  Free( stm.rhs );
}

struct
ast_t
{
  stack_resizeable_cont_t<statement_t> stms;
};

Inl void
Init( ast_t& ast, idx_t capacity )
{
  Alloc( ast.stms, capacity );
}

Inl void
Kill( ast_t& ast )
{
  ForLen( i, ast.stms ) {
    auto stm = ast.stms.mem + i;
    Kill( *stm );
  }
  Free( ast.stms );
}


Inl pdbtoken_t*
ExpectToken( stack_resizeable_cont_t<pdbtoken_t>& tokens, idx_t i )
{
  if( i >= tokens.len ) {
    Log( "Expected a token, but hit EOF instead!" );
    return 0;
  }
  return tokens.mem + i;
}

Inl pdbtoken_t*
ExpectTokenOfType( stack_resizeable_cont_t<pdbtoken_t>& tokens, idx_t i, pdbtokentype_t type )
{
  auto tkn = ExpectToken( tokens, i );
  if( !tkn ) return 0;
  if( tkn->type != type ) {
    Log( "Expected '%s', but found '%s' instead!", StringOfTokenType( type ), StringOfTokenType( tkn->type ) );
    return 0;
  }
  return tkn;
}

bool
Parse( ast_t& ast, stack_resizeable_cont_t<pdbtoken_t>& tokens, slice_t& src )
{
  idx_t i = 0;
  Forever {
    if( i >= tokens.len ) {
      break;
    }

    auto tkn_key = ExpectTokenOfType( tokens, i, pdbtokentype_t::expr );
    if( !tkn_key ) return 0;
    i += 1;

    statement_t stm;
    Init( stm );

    stm.lhs.mem = src.mem + tkn_key->l;
    stm.lhs.len = tkn_key->r - tkn_key->l;

    auto tkn_equals = ExpectTokenOfType( tokens, i, pdbtokentype_t::equals );
    if( !tkn_equals ) return 0;
    i += 1;

    bool read_values = 1;
    while( read_values ) {
      auto tkn_value = ExpectTokenOfType( tokens, i, pdbtokentype_t::expr );
      if( !tkn_value ) return 0;
      i += 1;

      auto tkn_comma_or_semicolon = ExpectToken( tokens, i );
      if( !tkn_comma_or_semicolon ) return 0;
      i += 1;

      switch( tkn_comma_or_semicolon->type ) {
        case pdbtokentype_t::comma: {
          // continue reading values
        } break;
        case pdbtokentype_t::semicolon: {
          read_values = 0;
        } break;
        default: {
          Log( "Expected '%s' or '%s', but found '%s' instead!",
            StringOfTokenType( pdbtokentype_t::comma ),
            StringOfTokenType( pdbtokentype_t::semicolon ),
            StringOfTokenType( tkn_comma_or_semicolon->type )
            );
          return 0;
        } break;
      } // switch

      auto buf = AddBack( stm.rhs );
      buf->mem = src.mem + tkn_value->l;
      buf->len = tkn_value->r - tkn_value->l;
    } // while( read_values )

    *AddBack( ast.stms ) = stm;

  } // Forever
  return 1;
}


Templ Inl void
_AddToDb( propdb_t& db, slice_t& lhs, T& val )
{
  u8 propmem[c_propsize] = {};
  Memmove( propmem, &val, sizeof( val ) );
  slice_t name;
  name.len = lhs.len;
  name.mem = AddPagelist( db.strdata, u8, 1, name.len );
  Memmove( name.mem, lhs.mem, name.len );
  Add( db.set, &name, propmem, 0, 0, 1 );
}


Inl bool
_LoadFromMem( propdb_t& db, slice_t& mem )
{
  idx_t statement_estimate = mem.len / 10;
  stack_resizeable_cont_t<pdbtoken_t> tokens;
  Alloc( tokens, statement_estimate );
  ast_t ast;
  bool r = Tokenize( tokens, mem );
  if( r ) {
    Init( ast, statement_estimate );
    r = Parse( ast, tokens, mem );
    if( r ) {
      ForLen( i, ast.stms ) {
        auto stm = ast.stms.mem + i;
        if( 0 ) {
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 7 ), "keybind", 7 ) ) {
          if( stm->rhs.len <= 6 ) {
            bool add_keybind = 1;
            glwkey_t keys[6] = {};
            ForLen( j, stm->rhs ) {
              auto keystring = stm->rhs.mem + j;
              glwkey_t found_key;
              bool found;
              KeyGlwFromString( *keystring, &found, &found_key );
              if( found ) {
                keys[j] = found_key;
              } else {
                auto cstr = AllocCstr( *keystring );
                Log( "Not a valid key string: %s", cstr );
                MemHeapFree( cstr );
                r = 0;
                add_keybind = 0;
                break;
              }
            }
            if( add_keybind ) {
              auto val = _glwkeybind( keys[0], keys[1], keys[2], keys[3], keys[4], keys[5] );
              _AddToDb( db, stm->lhs, val );
            }
          } else {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected up to 6 key values: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 6 ), "string", 6 ) ) {
          if( stm->rhs.len != 1 ) {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected a single value: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          } else {
            auto rhs = stm->rhs.mem + 0;
            if( rhs->len < 2 ) {
              auto cstr = AllocCstr( stm->lhs );
              Log( "Expected double quotes around your string: %s", cstr );
              MemHeapFree( cstr );
              r = 0;
            } else {
              slice_t val;
              val.len = rhs->len - 2;
              val.mem = AddPagelist( db.strdata, u8, 1, val.len );
              Memmove( val.mem, rhs->mem + 1, val.len );
              _AddToDb( db, stm->lhs, val );
            }
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 4 ), "bool", 4 ) ) {
          if( stm->rhs.len == 1 ) {
            auto rhs = stm->rhs.mem + 0;
            auto val = !!CsTo_u64( rhs->mem, rhs->len );
            _AddToDb( db, stm->lhs, val );
          } else {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected a single value: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 4 ), "rgba", 4 ) ) {
          if( stm->rhs.len != 4 ) {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected four numeric values in [0.0, 1.0]: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          } else {
            vec4<f32> val;
            bool success = 1;
            ForLen( j, stm->rhs ) {
              auto rhs = stm->rhs.mem + j;
              auto component = Cast( f32*, &val ) + j;
              if( !CsToFloat32( rhs->mem, rhs->len, *component ) ) {
                auto cstr = AllocCstr( stm->lhs );
                Log( "Unable to parse floating point value at index %llu: %s", j, cstr );
                MemHeapFree( cstr );
                success = 0;
                break;
              }
            }
            if( !success ) {
              r = 0;
            } else {
              _AddToDb( db, stm->lhs, val );
            }
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 3 ), "f32", 3 ) ) {
          if( stm->rhs.len == 1 ) {
            auto rhs = stm->rhs.mem + 0;
            auto val = CsTo_f32( rhs->mem, rhs->len );
            _AddToDb( db, stm->lhs, val );
          } else {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected a single value: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 3 ), "f64", 3 ) ) {
          if( stm->rhs.len == 1 ) {
            auto rhs = stm->rhs.mem + 0;
            auto val = CsTo_f64( rhs->mem, rhs->len );
            _AddToDb( db, stm->lhs, val );
          } else {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected a single value: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 3 ), "u64", 3 ) ) {
          if( stm->rhs.len == 1 ) {
            auto rhs = stm->rhs.mem + 0;
            auto val = CsTo_u64( rhs->mem, rhs->len );
            _AddToDb( db, stm->lhs, val );
          } else {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected a single value: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 3 ), "s64", 3 ) ) {
          if( stm->rhs.len == 1 ) {
            auto rhs = stm->rhs.mem + 0;
            auto val = CsTo_s64( rhs->mem, rhs->len );
            _AddToDb( db, stm->lhs, val );
          } else {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected a single value: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 3 ), "u32", 3 ) ) {
          if( stm->rhs.len == 1 ) {
            auto rhs = stm->rhs.mem + 0;
            auto val = CsTo_u32( rhs->mem, rhs->len );
            _AddToDb( db, stm->lhs, val );
          } else {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected a single value: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 3 ), "s32", 3 ) ) {
          if( stm->rhs.len == 1 ) {
            auto rhs = stm->rhs.mem + 0;
            auto val = CsTo_s32( rhs->mem, rhs->len );
            _AddToDb( db, stm->lhs, val );
          } else {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected a single value: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 3 ), "u16", 3 ) ) {
          if( stm->rhs.len == 1 ) {
            auto rhs = stm->rhs.mem + 0;
            auto val = CsTo_u16( rhs->mem, rhs->len );
            _AddToDb( db, stm->lhs, val );
          } else {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected a single value: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 3 ), "s16", 3 ) ) {
          if( stm->rhs.len == 1 ) {
            auto rhs = stm->rhs.mem + 0;
            auto val = CsTo_s16( rhs->mem, rhs->len );
            _AddToDb( db, stm->lhs, val );
          } else {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected a single value: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 2 ), "u8", 2 ) ) {
          if( stm->rhs.len == 1 ) {
            auto rhs = stm->rhs.mem + 0;
            auto val = CsTo_u8( rhs->mem, rhs->len );
            _AddToDb( db, stm->lhs, val );
          } else {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected a single value: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 2 ), "s8", 2 ) ) {
          if( stm->rhs.len == 1 ) {
            auto rhs = stm->rhs.mem + 0;
            auto val = CsTo_s8( rhs->mem, rhs->len );
            _AddToDb( db, stm->lhs, val );
          } else {
            auto cstr = AllocCstr( stm->lhs );
            Log( "Expected a single value: %s", cstr );
            MemHeapFree( cstr );
            r = 0;
          }
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 2 ), "v2", 2 ) ) {
          // TODO: implement
          auto cstr = AllocCstr( stm->lhs );
          Log( "We don't handle this property type yet: %s", cstr );
          MemHeapFree( cstr );
          r = 0;
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 2 ), "v3", 2 ) ) {
          // TODO: implement
          auto cstr = AllocCstr( stm->lhs );
          Log( "We don't handle this property type yet: %s", cstr );
          MemHeapFree( cstr );
          r = 0;
        } elif( MemEqual( stm->lhs.mem, MIN( stm->lhs.len, 2 ), "v4", 2 ) ) {
          // TODO: implement
          auto cstr = AllocCstr( stm->lhs );
          Log( "We don't handle this property type yet: %s", cstr );
          MemHeapFree( cstr );
          r = 0;
        } else {
          auto cstr = AllocCstr( stm->lhs );
          Log( "Unrecognized property type: %s", cstr );
          MemHeapFree( cstr );
          r = 0;
        }
      }
    }
  }

  Kill( ast );
  Free( tokens );
  return r;
}

void
_Init( propdb_t& db )
{
  ProfFunc();

  Init(
    db.set,
    512,
    sizeof( slice_t ),
    c_propsize,
    0.8f,
    Equal_SliceContents,
    Hash_SliceContents
    );

  Init( db.strdata, 32768 );

  db.time_lastwrite = 0;

  db.filename = FsGetExe();
  if( db.filename.len ) {
    auto last_dot = StringScanL( ML( db.filename ), '.' );
    if( last_dot ) {
			AssertCrash( last_dot );
			db.filename.len = ( last_dot - db.filename.mem );
			db.filename.len += 1; // include dot.
		}
		else {
			*AddBack( db.filename ) = '.';
		}
		Memmove( AddBack( db.filename, 6 ), "config", 6 );
  }

  // TODO: timestep_fixed ?
  // TODO: font, with a fixed backup.
  // TODO: font size

  Log( "PROPDB INITIALIZE" );
  LogAddIndent( +1 );

  auto cstr = AllocCstr( ML( db.filename ) );
  Log( "Opening config file: %s", cstr );
  MemHeapFree( cstr );

  file_t file = FileOpen( ML( db.filename ), fileopen_t::always, fileop_t::RW, fileop_t::R );
  string_t mem;
  Zero( mem );
  if( file.loaded ) {
    mem = FileAlloc( file );
  } else {
    cstr = AllocCstr( ML( db.filename ) );
    Log( "Can't open the config file: %s", cstr );
    MemHeapFree( cstr );
  }
  FileFree( file );

  auto slice = SliceFromString( mem );
  if( !_LoadFromMem( db, slice ) ) {
    Log( "Failed to _LoadFromMem!" );
  }
  Free( mem );

  LogAddIndent( -1 );
  Log( "" );
}

Inl void*
_GetProp( propdb_t& db, u8* name )
{
  bool found;
  void* prop;
  auto slice = SliceFromCStr( name );
  LookupRaw( db.set, &slice, &found, Cast( void**, &prop ) );
  if( !found ) {
    Log( "PROPDB ACCESS FAILURE" );
    LogAddIndent( +1 );
    Log( "Property not found: %s", name );
    LogAddIndent( -1 );
    Log( "" );
    constant u8 c_zeroprop[c_propsize] = {};
    return Cast( void*, c_zeroprop );
  } else {
    return prop;
  }
}

__OnMainKill( KillPropdb )
{
  auto db = Cast( propdb_t*, user );
  _Kill( *db );
}

Inl void
_InitPropdb()
{
  if( !g_db_init ) {
    g_db_init = 1;
    _Init( g_db );
    RegisterOnMainKill( KillPropdb, &g_db );
  }
}

Inl void*
GetProp( u8* name )
{
  _InitPropdb();
  return _GetProp( g_db, name );
}



#define GetPropFromDb( T, name ) \
  *Cast( T*, GetProp( Str( #name ) ) )


