// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// comment text that i'll use for searchability later:
//     TODO: something left to do here.
//     PERF: pointing out a perf problem.
// we should resolve all the TODOs before considering any kind of general release.
// i'm okay with PERFs sticking around; we probably shouldn't resolve them until they become a measurable problem.
//


#if !(defined(WIN) || defined(MAC))
  #error Make a platform choice in the makefiles!
#endif


// ============================================================================
// CORE LANGUAGE MACROS

#define CompileAssert( error_if_false )    static_assert( error_if_false, # error_if_false )

#define elif \
  else if

// TODO: too many existing uses of 'ret' in enums, vars, etc.
//#define ret \
//  return


#define Cast( type, var )   ( ( type )( var ) )

// for string literals, we need to cast to u8* because there's no compiler option to make literals unsigned!
#define Str( x )   Cast( u8*, x )


#define ML( a ) \
  ( a ).mem, ( a ).len

#define AL( a ) \
  ( a + 0 ), Cast( idx_t, _countof( a ) )


// dear god, please save us from the infinitesimal wisdom of c++ macros.
// these are needed due to the order of evaluation of macros.
// so, never use ## directly unless you know what you're doing, use this NAMEJOIN instead!
// the _NAMEJOIN is intended to be private and shouldn't be used.
#define _NAMEJOIN( a, b ) a ## b
#define NAMEJOIN( a, b ) _NAMEJOIN( a, b )


#define ForceInl   __forceinline
#define ForceNoInl   __declspec( noinline )

#define NoInl   __declspec( noinline ) static

#if WEAKINLINING
  #define Inl   inline static
#else
  #define Inl   __forceinline static
#endif

#define constant      static constexpr
#define constantold   static const


// NOTE: for aligning some set of structs of arbitrary size that you're packing contiguously into an array.
//   we need each field to have aligned loads/stores, which is done by forcing _SIZEOF_IDX_T alignment.
#define ALIGNTOIDX   __declspec( align( _SIZEOF_IDX_T ) )


#define Templ        template< typename T >
#define TemplTIdxN   template< typename T, idx_t N >
#define TemplIdxN    template< idx_t N >
#define TA    template< typename Allocator = allocator_heap_or_virtual_t, typename Allocation = allocation_heap_or_virtual_t >
#define TEA   template< typename T, typename Allocator = allocator_heap_or_virtual_t, typename Allocation = allocation_heap_or_virtual_t >

#define Implies( p, q ) \
  ( !( p )  ||  ( q ) )

#define LTEandLTE( x, x0, x1 )   ( ( x0 <= x )  &&  ( x <= x1 ) )
#define LTandLTE(  x, x0, x1 )   ( ( x0 <  x )  &&  ( x <= x1 ) )
#define LTEandLT(  x, x0, x1 )   ( ( x0 <= x )  &&  ( x <  x1 ) )

#define MIN( a, b )   ( ( ( a ) <= ( b ) )  ?  ( a )  :  ( b ) )
#define MAX( a, b )   ( ( ( a ) >  ( b ) )  ?  ( a )  :  ( b ) )

Templ constexpr T Min( T a, T b ) { return MIN( a, b ); }
Templ constexpr T Max( T a, T b ) { return MAX( a, b ); }

#define MIN3( a, b, c )   ( MIN( MIN( ( a ), ( b ) ), ( c ) ) )
#define MAX3( a, b, c )   ( MAX( MAX( ( a ), ( b ) ), ( c ) ) )

#define MIN4( a, b, c, d )   ( MIN( MIN( ( a ), ( b ) ), MIN( ( c ), ( d ) ) ) )
#define MAX4( a, b, c, d )   ( MAX( MAX( ( a ), ( b ) ), MAX( ( c ), ( d ) ) ) )

#define MIN5( a, b, c, d, e )   MIN( ( MIN( MIN( ( a ), ( b ) ), MIN( ( c ), ( d ) ) ) ), ( e ) )
#define MAX5( a, b, c, d, e )   MAX( ( MAX( MAX( ( a ), ( b ) ), MAX( ( c ), ( d ) ) ) ), ( e ) )

#define CLAMP( x, r0, r1 )   MAX( r0, MIN( r1, x ) )

#define ABS( a )   ( ( ( a ) < 0 )  ?  -( a )  :  ( a ) )

#define SWAP( type, a, b )   do { type tmp = a;  a = b;  b = tmp; } while( 0 )

#define PERMUTELEFT3( type, a, b, c )    do { type tmp = a;  a = b;  b = c;  c = tmp; } while( 0 )
#define PERMUTERIGHT3( type, a, b, c )   do { type tmp = a;  a = c;  c = b;  b = tmp; } while( 0 )


// ============================================================================
// ENUM MACROS

typedef u32 enum_t;

#define Enumc( name ) \
  enum class name : enum_t

#define ENUM_IS_BITMASK( type ) \
  ForceInl type operator|( type a, type b ) \
  { \
    return Cast( type, Cast( enum_t, a ) | Cast( enum_t, b ) ); \
  } \
  ForceInl type& operator|=( type& a, type b ) \
  { \
    a = operator|( a, b ); \
    return a; \
  } \
  ForceInl type operator&( type a, type b ) \
  { \
    return Cast( type, Cast( enum_t, a ) & Cast( enum_t, b ) ); \
  } \
  ForceInl type& operator&=( type& a, type b ) \
  { \
    a = operator&( a, b ); \
    return a; \
  } \


// ============================================================================
// FOR LOOP MACROS

#define Fori( idxtype, idx, start, end ) \
  for( idxtype idx = start;  idx < end;  ++idx )

// we can declare things outside the for(), then we can use different types.
// for now, it's fine to use idxtype for the bool too.
#define ReverseFori( idxtype, idx, start, end ) \
  for( \
    idxtype __loop = start < end, idx = MAX( 1, end ) - 1; \
    __loop  &&  idx >= start; \
    idx == start  ?  __loop = 0  :  --idx )

#define Forinc( idxtype, idx, start, end, inc ) \
  for( idxtype idx = start;  idx < end;  idx += inc )

#define ReverseForPrev( elem, lastelem ) \
  for( auto elem = lastelem;  elem;  elem = elem->prev )

#define ForLen( idx, arr ) \
  Fori( idx_t, idx, 0, ( arr ).len )

#define ForLen32( idx, arr ) \
  Fori( u32, idx, 0, ( arr ).len )

#define ReverseForLen( idx, arr ) \
  ReverseFori( idx_t, idx, 0, ( arr ).len )

#define ReverseForLen32( idx, arr ) \
  ReverseFori( u32, idx, 0, ( arr ).len )

// TODO: outlaw this!
//   i always get the end vs. count part wrong, and waste time debugging to figure that out.
// TODO: rename to ForRange ?
#define For( idx, start, end ) \
  Fori( idx_t, idx, start, end )

#define ReverseFor( idx, start, end ) \
  ReverseFori( idx_t, idx, start, end )

#define ForEach( elem, list ) \
  for( auto& elem : list )

#define Forever \
  for( ;; )


// WARNING: causes mismatched { }
#define FORLEN( elem, idx, list ) \
  ForLen( idx, list ) { \
    auto elem = ( list ).mem + ( idx ); \

// WARNING: causes mismatched { }
#define FORLEN32( elem, idx, list ) \
  ForLen32( idx, list ) { \
    auto elem = ( list ).mem + ( idx ); \

// WARNING: causes mismatched { }
#define REVERSEFORLEN( elem, idx, list ) \
  ReverseForLen( idx, list ) { \
    auto elem = ( list ).mem + ( idx ); \

// WARNING: causes mismatched { }
#define REVERSEFORLEN32( elem, idx, list ) \
  ReverseForLen32( idx, list ) { \
    auto elem = ( list ).mem + ( idx ); \


// ideally, we would add a T* val param, so you don't need to do: auto val = &elem->value;
// but, you can't declare two things of different types in a for-decl.
// so we're stuck with this for now.
#define ForList( elem, list ) \
  for( auto elem = ( list ).first;  elem;  elem = ( elem  ?  elem->next  :  0 ) )

// WARNING: causes mismatched { }
#define FORLIST( val, elem, list ) \
  ForList( elem, list ) { \
    auto val = &elem->value; \

#define ReverseForList( elem, list ) \
  for( auto elem = ( list ).last;  elem;  elem = ( elem  ?  elem->prev  :  0 ) )

// WARNING: causes mismatched { }
#define REVERSEFORLIST( val, elem, list ) \
  ReverseForList( elem, list ) { \
    auto val = &elem->value; \


#define ForNext( elem, firstelem ) \
  for( auto elem = firstelem;  elem;  elem = elem->next )

// WARNING: causes mismatched { }
#define FORNEXT( val, elem, elemfirst ) \
  ForNext( elem, elemfirst ) { \
    auto val = &elem->value; \



#define BEGIN_FORLISTALLPAIRS( elem0, elem1, list ) \
  FORLIST( elem0, __forlistallpairs_elem0, list ) \
    auto __forlistallpairs_elem1 = __forlistallpairs_elem0->next; \
    while( __forlistallpairs_elem1 ) { \
      auto elem1 = &__forlistallpairs_elem1->value; \

#define END_FORLISTALLPAIRS \
      __forlistallpairs_elem1 = __forlistallpairs_elem1->next; \
    } \
  } \


// assumes list0.len == list1.len
#define BEGIN_FORTWOLISTS( idx, elem0, list0, elem1, list1 ) \
  auto __fortwolists_listelem0 = ( list0 ).first; \
  auto __fortwolists_listelem1 = ( list1 ).first; \
  ForLen( idx, list0 ) { \
    auto elem0 = &__fortwolists_listelem0->value; \
    auto elem1 = &__fortwolists_listelem1->value; \

#define END_FORTWOLISTS \
    __fortwolists_listelem0 = __fortwolists_listelem0->next; \
    __fortwolists_listelem1 = __fortwolists_listelem1->next; \
  } \

#define BEGIN_FORTWOLISTS2( elem0, list0, elem1, list1 ) \
  auto __fortwolists2_listelem0 = ( list0 ).first; \
  auto __fortwolists2_listelem1 = ( list1 ).first; \
  ForLen( __fortwolists2_idx, list0 ) { \
    auto elem0 = &__fortwolists2_listelem0->value; \
    auto elem1 = &__fortwolists2_listelem1->value; \

#define END_FORTWOLISTS2 \
    __fortwolists2_listelem0 = __fortwolists2_listelem0->next; \
    __fortwolists2_listelem1 = __fortwolists2_listelem1->next; \
  } \
