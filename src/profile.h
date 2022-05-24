// Copyright (c) John A. Carlos Jr., all rights reserved.


#ifndef PROF_ENABLED
#error make a choice for PROF_ENABLED; either 0 or 1.
#endif

#ifndef PROF_ENABLED_AT_LAUNCH
#error make a choice for PROF_ENABLED_AT_LAUNCH; either 0 or 1.
#endif


#if !PROF_ENABLED

#define ProfInit() // nothing
#define ProfFunc() // nothing
#define Prof( zone_label ) // nothing
#define ProfClose( zone_label ) // nothing
#define ProfOutputZoneStats() // nothing
#define ProfOutputTimeline() // nothing
#define ProfKill() // nothing

#else // PROF_ENABLED



// size of the one prof_elem_t buffer, which stores all the records.
#define PROF_STORE_MAX   1ULL*1024*1024*1024




#define PROF_SPLITSCOPES   0

#define PROF_DEPTHCOUNT    1

#if PROF_DEPTHCOUNT
#else
#endif


//
// i'm thinking we really only need: { time_start, time_end, tid, id }
// that'd take us from 44bytes -> 20bytes, and eliminate a subtraction.
// assuming memory/caching is the bottleneck, that should be ~2x as fast.
//
// we can store { name, file, line } per-id, into a separate structure.
// and we only need to store those once, so maybe we do a static bool or something per-id.
// hopefully branch prediction would eat most of the cost of those static bools per-id.
// we'd have to be careful with multiple threads writing to that separate structure, so that's something to
//   think about going forward.
//
// there's also alternate methods for { name, file, line }, like source-scanning, if we really want to
// get rid of all the runtime cost.
// i'm hopeful the static bool would make most of that unnecessary, but you never know.
// we could try just dropping the { name, file, line } temporarily, just to see what the cost of the
//   static bool approach is.
// we wouldn't be able to do any analysis that way, but it's fine just for a quick perf test.
//
// for realtime ui, it'd be nice to see scopes that are currently running too.
// to do that, we'd need to split this into scope-start and scope-end.
// unfortunately i think that means logging tid/id redundantly, i.e.
//   scope-start { tid, id, time_start };
//   scope-end   { tid, id, time_end };
// we'd also have to encode start/end somehow, probably as the high bit in id/tid.
// that would make each of these 16bytes, so we'd have 16*2=32bytes per scope, instead of 20bytes per scope.
// that's not actually 2x worse, so it's probably worth having this as an option.
// e.g. for offline logs, the smaller 20byte representation is probably what we want.
// but for online ui, the scope-start/end is useful, and it's not a huge price to pay.
//
// TODO: implement split scope entries as described above.
//
// TODO: how do we actually match scope-starts with scope-ends?
//
// i need to remember the last scope-start { id, tid, time }, so i can match and subtract.
// note we'd need a stack, to handle nested scopes with the same id,tid values.
// simplest thing would be one stack, and we just push on scope-start, and reverse search on scope-end.
// presumably we could make that a little faster if we had multiple stacks, one per id or tid or id,tid.
//
struct
prof_elem_t
{
#if PROF_SPLITSCOPES
  u64 time;
  u32 tid;
  u32 start :  1,
         id : 31;
#else
  u64 time_start;
  u64 time_end;
  u32 tid;
  u32 id;

#if PROF_DEPTHCOUNT
  u32 depth;
#else
#endif

#endif
};

// TODO: buffer_t
volatile prof_elem_t* g_prof_buffer;
volatile idx_t g_prof_buffer_len;
volatile idx_t g_prof_pos;
volatile idx_t g_saved_prof_pos;
volatile bool g_prof_enabled;

#if PROF_DEPTHCOUNT
// start at MAX_u32, because we add one on scope-entry, and subtract on scope-exit.
// the toplevel scopes should have depth=0, and MAX_u32+1 is 0.
__declspec(thread) u32 g_depthcount = MAX_u32;
#else
#endif

struct
prof_loc_t
{
  u8* name;
  u8* file;
  u32 line;
};


#define PROF_MAX_LOCATIONS   32000

volatile prof_loc_t g_prof_locations[PROF_MAX_LOCATIONS] = {};

#define AddLocation( id, name, file, line ) \
  if( id < _countof( g_prof_locations ) ) { \
    auto loc = Cast( prof_loc_t*, g_prof_locations + id ); \
    loc->name = name; \
    loc->file = file; \
    loc->line = line; \
  } \




Inl void
ProfDisable()
{
  // TODO: not correct if multiple threads try to disable
  //   if that's not the intention, why is g_prof_enabled volatile?
  if( g_prof_enabled ) {
    g_prof_enabled = 0;
    g_saved_prof_pos = g_prof_pos;
    g_prof_pos = g_prof_buffer_len;
  }
}

Inl void
ProfEnable()
{
  // TODO: not correct if multiple threads try to disable
  //   if that's not the intention, why is g_prof_enabled volatile?
  if( !g_prof_enabled ) {
    g_prof_enabled = 1;
    g_prof_pos = g_saved_prof_pos;
  }
}

Inl void
ProfReset()
{
  if( g_prof_enabled ) {
    g_prof_pos = 0;
  }
  else {
    g_saved_prof_pos = 0;
  }
}

Inl void
ProfZero()
{
  g_prof_buffer = 0;
  g_prof_buffer_len = 0;
  g_prof_pos = 0;
  g_saved_prof_pos = 0;
}

Inl void
ProfInit()
{
  ProfZero();

  g_prof_buffer_len = PROF_STORE_MAX / sizeof( prof_elem_t );
  g_prof_buffer = MemVirtualAlloc( prof_elem_t, g_prof_buffer_len );
  AssertCrash( g_prof_buffer );

  ProfEnable();

  // note we always enable first, and then disable if we need to.
  // this is so we can share the same live save/restore logic as manual enable/disable.
  #if !PROF_ENABLED_AT_LAUNCH
    ProfDisable();
  #endif
}

Inl void
ProfKill()
{
  MemVirtualFree( Cast( void*, g_prof_buffer ) );
  ProfZero();
}




#if PROF_SPLITSCOPES

  #define AddScopeStart( _id ) \
    do { \
      if( g_prof_pos < g_prof_buffer_len ) { /* avoids overflow, as long as g_prof_buffer_len isn't close to the bounds. */ \
        idx_t pos = GetValueBeforeAtomicInc( &g_prof_pos ); \
        if( pos < g_prof_buffer_len ) { \
          auto elem = Cast( prof_elem_t*, g_prof_buffer + pos ); \
          elem->time = TimeTSC(); \
          elem->tid = GetThreadIdFast(); \
          elem->id = _id; \
          elem->start = 1; \
        } \
      } \
    } while( 0 )

  #define AddScopeEnd( _id ) \
    do { \
      if( g_prof_pos < g_prof_buffer_len ) { /* avoids overflow, as long as g_prof_buffer_len isn't close to the bounds. */ \
        idx_t pos = GetValueBeforeAtomicInc( &g_prof_pos ); \
        if( pos < g_prof_buffer_len ) { \
          auto elem = Cast( prof_elem_t*, g_prof_buffer + pos ); \
          elem->time = TimeTSC(); \
          elem->tid = GetThreadIdFast(); \
          elem->id = _id; \
          elem->start = 0; \
        } \
      } \
    } while( 0 )

#else

  #if PROF_DEPTHCOUNT
    #define AddProfRecord( time_start, id ) \
      do { \
        if( g_prof_pos < g_prof_buffer_len ) { /* avoids overflow, as long as g_prof_buffer_len isn't close to the bounds. */ \
          idx_t pos = GetValueBeforeAtomicInc( &g_prof_pos ); \
          if( pos < g_prof_buffer_len ) { \
            auto elem = Cast( prof_elem_t*, g_prof_buffer + pos ); \
            elem->time_start = time_start; \
            elem->time_end = TimeTSC(); \
            elem->tid = GetThreadIdFast(); \
            elem->id = id; \
            elem->depth = g_depthcount--; \
          } \
        } \
      } while( 0 )

  #else
    #define AddProfRecord( time_start, id ) \
      do { \
        if( g_prof_pos < g_prof_buffer_len ) { /* avoids overflow, as long as g_prof_buffer_len isn't close to the bounds. */ \
          idx_t pos = GetValueBeforeAtomicInc( &g_prof_pos ); \
          if( pos < g_prof_buffer_len ) { \
            auto elem = Cast( prof_elem_t*, g_prof_buffer + pos ); \
            elem->time_start = time_start; \
            elem->time_end = TimeTSC(); \
            elem->tid = GetThreadIdFast(); \
            elem->id = id; \
          } \
        } \
      } while( 0 )
  #endif

#endif

// TODO: try unpacking this, so we macro define a bunch of local vars instead.
//   not sure i trust the toolchains to unpack this.
// TODO: make a different version for ProfClose, so we don't need a runtime !closed check.
struct
prof_zone_t
{
  u64 time_start;
  u32 id;
  bool closed;

  ForceInl
  prof_zone_t(
    u8* zonename,
    u8* filename,
    u32 lineno,
    u32 zoneid,
    bool* location_logged
    )
  {
    if( !g_prof_enabled ) {
      closed = 1;
      return;
    }

    if( !*location_logged ) {
      *location_logged = 1;
      auto loc = Cast( prof_loc_t*, g_prof_locations + zoneid );
      loc->name = zonename;
      loc->file = filename;
      loc->line = lineno;
    }

    id = zoneid;
    closed = 0;

#if PROF_DEPTHCOUNT
    g_depthcount += 1;
#else
#endif

#if PROF_SPLITSCOPES
    AddScopeStart( id );
#else
    time_start = TimeTSC();
#endif
  }

  ForceInl
  ~prof_zone_t()
  {
    if( closed ) {
      return;
    }

#if PROF_SPLITSCOPES
    AddScopeEnd( id );
#else
    AddProfRecord( time_start, id );
#endif
  }

  ForceInl void
  Close()
  {
    if( closed ) {
      return;
    }

#if PROF_SPLITSCOPES
    AddScopeEnd( id );
#else
    AddProfRecord( time_start, id );
#endif
    closed = 1;
  }
};

#define ProfFunc() \
  static bool NAMEJOIN( s_prof_loc_logged_, __LINE__ ) = 0; \
  prof_zone_t NAMEJOIN( prof_zone_, __LINE__ ) ( \
    Str( __FUNCTION__ ), \
    Str( __FILE__ ), \
    __LINE__, \
    __COUNTER__, \
    &NAMEJOIN( s_prof_loc_logged_, __LINE__ ) \
    ) \

#define Prof( name ) \
  static bool NAMEJOIN( s_prof_loc_logged_, name ) = 0; \
  prof_zone_t NAMEJOIN( prof_zone_, name ) ( \
    Str( # name ), \
    Str( __FILE__ ), \
    __LINE__, \
    __COUNTER__, \
    &NAMEJOIN( s_prof_loc_logged_, name ) \
    ) \

#define ProfClose( name ) \
  NAMEJOIN( prof_zone_, name ).Close()







// TODO: output raw prof_elem_t records to a separate CSV file, not the logger .log file.
// TODO: it might also make sense to just add a plot output function.
//   e.g. ProfDrawThreadTimelines( array_t<u8>& stream, ... )
void
ProfOutputTimeline()
{
  // since we disable profiling by messing with g_prof_pos, we have to reset it here for things to function.
  ProfEnable();

  // log calls are file writes, so chunk stuff together to minimize the overhead.
  // est. size per row is probably 100bytes, so we chunk ~100,000 rows at a time.
  idx_t rows_per_chunk = 100000;
  array_t<u8> stage;
  Alloc( stage, 10*1000*1000 );

  // output column headers
#if PROF_SPLITSCOPES
  AddBackCStr( &stage, "ID,THREAD,FILE,LINE,SCOPE,OPEN_ELSE_CLOSE,TIMESTAMP\n" );
#else
  #if PROF_DEPTHCOUNT
    AddBackCStr( &stage, "ID,THREAD,FILE,LINE,SCOPE,DEPTH,T_START,T_DURATION\n" );
  #else
    AddBackCStr( &stage, "ID,THREAD,FILE,LINE,SCOPE,T_START,T_DURATION\n" );
  #endif
#endif


  idx_t nelems = MIN( g_prof_pos, g_prof_buffer_len );
  For( i, 0, nelems ) {
    auto elem = *Cast( prof_elem_t*, g_prof_buffer + i );

    auto loc = *Cast( prof_loc_t*, g_prof_locations + elem.id );

    AddBackUInt( &stage, elem.id );
    *AddBack( stage ) = ',';
    AddBackUInt( &stage, elem.tid );
    *AddBack( stage ) = ',';
    auto file_slice = SliceFromCStr( loc.file );
    auto file = _StandardFilename( loc.file, CsLen( loc.file ) );
    auto filename = FileNameAndExt( ML( file ) );
    AddBackContents( &stage, filename );
    *AddBack( stage ) = ',';
    AddBackUInt( &stage, loc.line );
    *AddBack( stage ) = ',';
    AddBackCStr( &stage, loc.name );
    *AddBack( stage ) = ',';
#if PROF_SPLITSCOPES
    AddBackUInt( &stage, elem.start );
    *AddBack( stage ) = ',';
    AddBackUInt( &stage, elem.time );
#else
  #if PROF_DEPTHCOUNT
    AddBackUInt( &stage, elem.depth );
    *AddBack( stage ) = ',';
  #else
  #endif
    AddBackUInt( &stage, elem.time_start );
    *AddBack( stage ) = ',';
    AddBackUInt( &stage, elem.time_end - elem.time_start );
#endif
    *AddBack( stage ) = '\n';

    if( i != 0  &&  i % rows_per_chunk == 0 ) {
      LogDirect( SliceFromArray( stage ) );
      stage.len = 0;
    }
  }

  // write remaining chunk
  if( stage.len ) {
    LogDirect( SliceFromArray( stage ) );
  }

  Free( stage );
}





struct
prof_zonestats_t
{
  u8* name;
  slice_t file;
  u32 line;
  u32 tid;
  u32 id;
  u64 n_invocs;
  f64 time_total;
  f64 time_total_err;
  f64 time_mean;
  f64 time_variance;
};

Inl s32
CompareZonestats( const void* a, const void* b )
{
  auto zone0 = Cast( prof_zonestats_t*, a );
  auto zone1 = Cast( prof_zonestats_t*, b );
  s32 r = ( zone0->time_total <= zone1->time_total )  ?  1  :  -1;
  return r;
}

struct
zoneid_t
{
  u32 id;
  u32 tid;
};

#define FUNCTION_W   24
#define FUNCTION_WS "24"

#define FILE_W   15
#define FILE_WS "15"

void
ProfOutputZoneStats()
{
  // since we disable profiling by messing with g_prof_pos, we have to reset it here for things to function.
  ProfEnable();

  // TODO: hashset iteration so I don't have to keep an extra array in sync.
  array_t<zoneid_t> set;
  Alloc( set, 256 );

  auto nelems = MIN( g_prof_pos, g_prof_buffer_len );

  hashset_t map;
  Init(
    map,
    1024,
    sizeof( zoneid_t ),
    sizeof( prof_zonestats_t ),
    0.75f,
    Equal_FirstU64,
    Hash_FirstU64
  );
  CompileAssert( sizeof( zoneid_t ) == sizeof( u64 ) ); // we use u64 hash functions, so zoneid_t better be 64bits.

#if PROF_SPLITSCOPES
  array_t<prof_elem_t> open_scopes;
  Alloc( open_scopes, 32000 );
#else
#endif

  For( i, 0, nelems ) {
    auto elem = *Cast( prof_elem_t*, g_prof_buffer + i );
    auto loc = *Cast( prof_loc_t*, g_prof_locations + elem.id );

#if PROF_SPLITSCOPES
    if( elem.start ) {
      *AddBack( open_scopes ) = elem;
      continue;
    }

    idx_t opened_idx = 0;
    prof_elem_t* popen = 0;
    REVERSEFORLEN( open_scope, j, open_scopes )
      if( elem.id == open_scope->id  &&  elem.tid == open_scope->tid ) {
        opened_idx = j;
        popen = open_scope;
        break;
      }
    }
    if( !popen ) {
      // TODO: should we need this early-out? somehow pushing a scope-end as the first elem?
      continue;
    }
    auto open = *popen;
    UnorderedRemAt( open_scopes, opened_idx );
#else
#endif

    bool found;
    prof_zonestats_t* rawstats;
    zoneid_t zoneid = { elem.id, elem.tid };
    LookupRaw( map, &zoneid, &found, Cast( void**, &rawstats ) );

    // calculate the all-important value: time elapsed since start.
#if PROF_SPLITSCOPES
    auto time_elapsed = TimeSecFromTSC64( elem.time - open.time );
#else
    auto time_elapsed = TimeSecFromTSC64( elem.time_end - elem.time_start );
#endif

    if( !found ) {
      prof_zonestats_t newstats;
      newstats.n_invocs = 1;
      newstats.time_mean = time_elapsed;
      newstats.time_total = time_elapsed;
      newstats.time_total_err = 0;
      newstats.time_variance = 0;
      newstats.name = loc.name;
      newstats.file = SliceFromCStr( loc.file );
      newstats.line = loc.line;
      newstats.tid = elem.tid;
      newstats.id = elem.id;

      bool already_there;
      Add( map, &zoneid, &newstats, &already_there, 0, 0 );
      AssertCrash( !already_there );

      *AddBack( set ) = zoneid;
    }
    else {
      auto stats = *rawstats;

      // calculate incrementally the total time ( using kahan summation )
      auto time_elapsed_cor = time_elapsed - stats.time_total_err; // TODO: convert to kahan64_t
      auto time_total = stats.time_total + time_elapsed_cor;
      stats.time_total_err = ( time_total - stats.time_total ) - time_elapsed_cor;
      stats.time_total = time_total;

      // calculate incrementally the mean.
      auto prev_n = stats.n_invocs;
      auto rec_n = 1.0 / ( prev_n + 1 );
      auto prev_mu = stats.time_mean;
      auto prev_diff = time_elapsed - prev_mu;
      auto mu = prev_mu + rec_n * prev_diff;
      stats.time_mean = mu;

      // calculate incrementally the variance.
      auto prev_sn = prev_n * stats.time_variance;
      stats.time_variance = rec_n * ( prev_sn + prev_diff * ( time_elapsed - mu ) );

      stats.n_invocs += 1;

      *rawstats = stats;
    }
  }

#if PROF_SPLITSCOPES
  Free( open_scopes );
#else
#endif

  array_t<prof_zonestats_t> zonestats;
  Alloc( zonestats, set.len );
  zonestats.len = set.len;

  FORLEN( zoneid, i, set )
    bool found;
    prof_zonestats_t stats;
    Lookup( map, zoneid, &found, &stats );
    AssertCrash( found );

    zonestats.mem[i] = stats;
  }

  Kill( map );
  Free( set );

  if( !zonestats.len ) {
    Free( zonestats );
    return;
  }

  // sort zonestats however we want:
  qsort(
    ML( zonestats ),
    sizeof( prof_zonestats_t ),
    CompareZonestats
    );

//  LogUseConsole( 1 );
  Log( "PROFILE" );
  LogAddIndent( +1 );
  Log( "Clocks / S: %llu", Cast( u64, 1 / g_sec_per_tsc64 ) );
  Log( "Clocks / MS: %llu", Cast( u64, 1 / ( 1000 * g_sec_per_tsc64 ) ) );
  LogInline( "%5s   ",             "ID" );
  LogInline( "%7s   ",             "THREAD" );
  LogInline( "%-" FILE_WS "s",     "FILE" );
  LogInline( "%7s   ",             "LINE" );
  LogInline( "%-" FUNCTION_WS "s", "FUNCTION" );
  LogInline( "%12s   ",            "INVOCS" );
  LogInline( "%-12s",              "TIME(S)" );
  LogInline( "%-12s",              "MEAN(MS)" );
  LogInline( "%-12s",              "SDEV(MS)" );
  LogInline( "%-12s",              "WIDTH" );
  Log( "" );
  embeddedarray_t<u8, 4096> tmp;
  FORLEN( pstats, i, zonestats )
    auto stats = *pstats;
    if( !stats.n_invocs ) {
      continue;
    }

    tmp.len = 0;

    LogInline( "%5u   ", stats.id );
    LogInline( "%7u   ", stats.tid );

    u8* filename = CsScanL( ML( stats.file ), '\\' );
    if( !filename )
      filename = CsScanL( ML( stats.file ), '/' );
    if( !filename )
      filename = stats.file.mem - 1;
    filename += 1; // skip slash
    auto tmpsize = MIN( CsLen( filename ), FILE_W );
    Clear( tmp );
    Memmove( AddBack( tmp, tmpsize ), filename, tmpsize );
    *AddBack( tmp ) = 0;
    LogInline( "%-" FILE_WS "s", tmp.mem );
    RemBack( tmp );

    LogInline( "%7u   ", stats.line );

    tmpsize = MIN( CsLen( stats.name ), FUNCTION_W );
    Clear( tmp );
    Memmove( AddBack( tmp, tmpsize ), stats.name, tmpsize );
    *AddBack( tmp ) = 0;
    LogInline( "%-" FUNCTION_WS "s", tmp.mem );

    LogInline( "%12llu   ", stats.n_invocs );
    LogInline( "%-12.4g", stats.time_total );
    LogInline( "%-12.4g", 1000 * stats.time_mean );
    LogInline( "%-12.4g", 1000 * Sqrt64( stats.time_variance ) );
    if( stats.time_mean != 0.0 ) {
      LogInline( "%-12.4g", Sqrt64( stats.time_variance ) / stats.time_mean );
    }
    Log( "" );
  }
  LogAddIndent( -1 );
  Log( "" );
//  LogUseConsole( 0 );

  Free( zonestats );
}

#undef FUNCTION_W
#undef FUNCTION_WS
#undef FILE_W
#undef FILE_WS




#endif // PROF_ENABLED
