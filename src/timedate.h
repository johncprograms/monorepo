// Copyright (c) John A. Carlos Jr., all rights reserved.

Inl void
LogAddIndent( s32 delta );
void
Log( const void* cstr ... );
void
LogInline( const void* cstr ... );


static f32 g_sec_per_tsc32;
static f64 g_sec_per_tsc64;

static f32 g_sec_per_qpc32;
static f64 g_sec_per_qpc64;

static bool g_has_sleep_prec_period_min;
static u32 g_sleep_prec_period_min;


// TODO: define struct types around cycle_t and clock_t so we can't mix up the apis.
//   already had one bug like that.

u64 __rdtsc();

Inl u64
TimeTSC()
{
  return __rdtsc();
}


Inl void
TimeSleep( u32 milliseconds )
{
#if defined(WIN)
  Sleep( milliseconds ); // arg, winAPI! let me have a nano TimeSleep!
#elif defined(MAC)
#else
#error Unsupported platform
#endif
}


void
TimeInit()
{
#if defined(WIN)
  u64 t0 = __rdtsc();

  u64 qpc_per_sec;
  QueryPerformanceFrequency( Cast( LARGE_INTEGER*, &qpc_per_sec ) );
  g_sec_per_qpc32 = 1.0f / qpc_per_sec;
  g_sec_per_qpc64 = 1.0 / qpc_per_sec;

  bool init_sec_per_tsc = 0;

  Log( "CPUID" );
  LogAddIndent( +1 );

  s32 tmp[4];
  __cpuid( tmp, 0 );
  auto max_id = tmp[0];
  __cpuid( tmp, 1 << 31 );
  auto max_extid = tmp[0];

  stack_resizeable_cont_t<s32> info, extinfo;
  Alloc( info, 256 );
  Alloc( extinfo, 256 );
  Fori( s32, i, 0, max_id + 1 ) {
    __cpuidex( AddBack( info, 4 ), i, 0 );
  }
  tmp[0] = info.mem[1];
  tmp[1] = info.mem[3];
  tmp[2] = info.mem[2];
  slice_t vendor;
  vendor.mem = Cast( u8*, tmp );
  vendor.len = CstrLengthOrMax( Cast( u8*, tmp ), _countof( tmp ) * sizeof( tmp[0] ) );
  {
    auto cs = AllocCstr( vendor );
    Log( "vendor: %s", cs );
    MemHeapFree( cs );
  }
  Fori( s32, i, 1 << 31, max_extid + 1 ) {
    __cpuidex( AddBack( extinfo, 4 ), i, 0 );
  }
  if( extinfo.len > 16 ) {
    auto brand = SliceFromCStr( extinfo.mem + 8 );
    Log( "brand: %s", brand.mem );
    auto space = StringScanL( ML( brand ), ' ' );
    auto decimal = StringScanL( ML( brand ), '.' );
    auto ghz = StringScanL( ML( brand ), 'G' );
    if( space  &&  decimal  &&  ghz ) {
      auto integer = CsToIntegerU<u32>( space + 1, decimal - ( space + 1 ) );
      auto frac = CsToIntegerU<u32>( decimal + 1, ghz - ( decimal + 1 ) );
      if( 10 <= frac  &&  frac <= 99 ) {
        auto tsc_per_sec = Cast( f64, integer ) * 1e9 + Cast( f64, frac ) * 1e7;
        g_sec_per_tsc64 = 1.0  / tsc_per_sec;
        g_sec_per_tsc32 = 1.0f / Cast( f32, tsc_per_sec );
        init_sec_per_tsc = 1;
      }
    }
  }

  if( info.len > 4 ) {
    Log( "stepping: %X", info.mem[4] & AllOnes( 4 ) );
    auto model = ( info.mem[4] >> 4 ) & AllOnes( 4 );
    auto family = ( info.mem[4] >> 8 ) & AllOnes( 4 );
    auto extmodel = ( info.mem[4] >> 16 ) & AllOnes( 4 );
    auto extfamily = ( info.mem[4] >> 20 ) & AllOnes( 8 );
    Log( "model: 0x%X", ( family == 0x6 || family == 0xF )  ?  ( extmodel << 4 ) + model  :  model );
    Log( "family: 0x%X", ( family == 0xF )  ?  ( extfamily + family )  :  family );
    Log( "processor type: 0x%X", ( info.mem[4] >> 12 ) & AllOnes( 2 ) );

    Log( "x87 FPU: %X", ( info.mem[7] >> 0 ) & 1 );
    Log( "TSC: %X", ( info.mem[7] >> 4 ) & 1 );
    Log( "CMOV: %X", ( info.mem[7] >> 15 ) & 1 );
    Log( "MMX: %X", ( info.mem[7] >> 23 ) & 1 );
    Log( "SSE: %X", ( info.mem[7] >> 25 ) & 1 );
    Log( "SSE2: %X", ( info.mem[7] >> 26 ) & 1 );

    Log( "SSE3: %X", ( info.mem[6] >> 0 ) & 1 );
    Log( "SSSE3: %X", ( info.mem[6] >> 9 ) & 1 );
    Log( "FMA: %X", ( info.mem[6] >> 12 ) & 1 );
    Log( "SSE4.1: %X", ( info.mem[6] >> 19 ) & 1 );
    Log( "SSE4.2: %X", ( info.mem[6] >> 20 ) & 1 );
    Log( "MOVBE: %X", ( info.mem[6] >> 22 ) & 1 );
    Log( "POPCNT: %X", ( info.mem[6] >> 23 ) & 1 );
    Log( "AES: %X", ( info.mem[6] >> 25 ) & 1 );
    Log( "AVX: %X", ( info.mem[6] >> 28 ) & 1 );
    Log( "F16C: %X", ( info.mem[6] >> 29 ) & 1 );
    Log( "RDRAND: %X", ( info.mem[6] >> 30 ) & 1 );
  }
  if( info.len > 28 ) {
    Log( "SGX: %X", ( info.mem[29] >> 2 ) & 1 );
    Log( "AVX2: %X", ( info.mem[29] >> 5 ) & 1 );
    Log( "AVX512F: %X", ( info.mem[29] >> 16 ) & 1 );
    Log( "RDSEED: %X", ( info.mem[29] >> 18 ) & 1 );
    Log( "AVX512PF: %X", ( info.mem[29] >> 26 ) & 1 );
    Log( "AVX512ER: %X", ( info.mem[29] >> 27 ) & 1 );
    Log( "AVX512CD: %X", ( info.mem[29] >> 28 ) & 1 );
    Log( "SHA: %X", ( info.mem[29] >> 29 ) & 1 );
  }
  if( info.len > 40 ) {
    auto version = info.mem[40] & AllOnes( 8 );
    Log( "general-purpose perf counters per logical processor: %u", ( info.mem[40] >> 8 ) & AllOnes( 8 ) );
    Log( "general-purpose perf counter bit width: %u", ( info.mem[40] >> 16 ) & AllOnes( 8 ) );
    Log( "core cycle event: %X", !( ( info.mem[41] >> 0 ) & 1 ) );
    Log( "instruction retired event: %X", !( ( info.mem[41] >> 1 ) & 1 ) );
    Log( "reference cycles event: %X", !( ( info.mem[41] >> 2 ) & 1 ) );
    Log( "last-level cache reference event: %X", !( ( info.mem[41] >> 3 ) & 1 ) );
    Log( "last-level cache miss event: %X", !( ( info.mem[41] >> 4 ) & 1 ) );
    Log( "branch instruction retired event: %X", !( ( info.mem[41] >> 5 ) & 1 ) );
    Log( "branch mispredict retired event: %X", !( ( info.mem[41] >> 6 ) & 1 ) );
    if( version > 1 ) {
      Log( "fixed-function perf counters: %u", info.mem[43] & AllOnes( 5 ) );
      Log( "fixed-function perf counter bit width: %u", ( info.mem[43] >> 5 ) & AllOnes( 7 ) );
    }
  }
  if( info.len > 84 ) {
    auto crystal_freq = Cast( u32, info.mem[86] );
    auto tsc_over_crystal_numer = Cast( u32, info.mem[85] );
    auto tsc_over_crystal_denom = Cast( u32, info.mem[84] );
    auto tsc_over_crystal = Cast( f64, tsc_over_crystal_numer ) / Cast( f64, tsc_over_crystal_denom );
    auto tsc_freq = Cast( f64, crystal_freq ) * tsc_over_crystal;
    if( crystal_freq ) {
      Log( "core crystal clock frequency ( Hz ): %u", crystal_freq );
    } else {
      Log( "core crystal clock frequency ( Hz ): UNKNOWN" );
    }
    if( tsc_over_crystal_numer ) {
      Log(
        "tsc frequency over core crystal clock frequency: ( %u / %u ) = %f",
        tsc_over_crystal_numer,
        tsc_over_crystal_denom,
        tsc_over_crystal
        );
    } else {
      Log( "tsc frequency over core crystal clock frequency: UNKNOWN" );
    }
    if( crystal_freq && tsc_over_crystal_numer ) {
      Log( "tsc frequency ( Hz ): %f", tsc_freq );
    } else {
      Log( "tsc frequency ( Hz ): UNKNOWN" );
    }
  }
  if( info.len > 88 ) {
    auto base_freq = info.mem[88] & AllOnes( 16 );
    auto max_freq = info.mem[89] & AllOnes( 16 );
    auto bus_freq = info.mem[90] & AllOnes( 16 );
    Log( "base frequency ( MHz ): %u", base_freq );
    Log( "max frequency ( MHz ): %u", max_freq );
    Log( "bus frequency ( MHz ): %u", bus_freq );

    // NOTE: we can't rely solely on this, since not all processors with TSC have this info.
    if( !init_sec_per_tsc ) {
      init_sec_per_tsc = 1;
      g_sec_per_tsc32 = 1.0f / Cast( f32, 1000000 * Cast( u64, base_freq ) );
      g_sec_per_tsc64 = 1.0  / Cast( f64, 1000000 * Cast( u64, base_freq ) );
    }
  }
  if( extinfo.len > 4 ) {
    Log( "LZCNT: %X", ( extinfo.mem[6] >> 5 ) & 1 );
    Log( "execute disable bit: %X", ( extinfo.mem[7] >> 20 ) & 1 );
    Log( "1GB pages: %X", ( extinfo.mem[7] >> 26 ) & 1 );
    Log( "RDTSCP: %X", ( extinfo.mem[7] >> 27 ) & 1 );
  }
  if( extinfo.len > 24 ) {
    Log( "cache line size ( B ): %u", ( extinfo.mem[26] >> 0 ) & AllOnes( 8 ) );
    Log( "L2 cache size per core ( KB ): %u", ( extinfo.mem[26] >> 16 ) & AllOnes( 16 ) );
    auto l2_assoc = ( extinfo.mem[26] >> 12 ) & AllOnes( 3 );
    switch( l2_assoc ) {
      case 0x0: Log( "L2 cache associativity: disabled" ); break;
      case 0x1: Log( "L2 cache associativity: direct-mapped" ); break;
      case 0x2: Log( "L2 cache associativity: 2-way" ); break;
      case 0x4: Log( "L2 cache associativity: 4-way" ); break;
      case 0x6: Log( "L2 cache associativity: 8-way" ); break;
      case 0x8: Log( "L2 cache associativity: 16-way" ); break;
      case 0xF: Log( "L2 cache associativity: fully-associative" ); break;
      default:  Log( "L2 cache associativity: UNKNOWN" ); break;
    }
  }
  if( extinfo.len > 28 ) {
    Log( "invariant TSC: %X", ( extinfo.mem[31] >> 8 ) & 1 );
  }
  if( extinfo.len > 32 ) {
    Log( "physical address bits: %u", ( extinfo.mem[32] >> 0 ) & AllOnes( 8 ) );
    Log( "linear address bits: %u", ( extinfo.mem[32] >> 8 ) & AllOnes( 8 ) );
  }
  Free( info );
  Free( extinfo );

  // TODO: this is no good!
  //   we hit this on AMD chips, since they don't report GHz as above Intel chips do.

  // NOTE: this is a last-chance measure.
  //   we're just stalling 0.25 seconds on startup :(
  //   waitiable timer is much more accurate timing mechanism than TimeSleep, so we could reduce that timing.
  if( !init_sec_per_tsc ) {
    init_sec_per_tsc = 1;

    Log( "WARNING: Falling back to slow sec_per_tsc calculation on startup" );

    u64 tsc_dticks;
    u64 qpc_period;
    u64 qpc_start;
    u64 tsc_start = TimeTSC();
    QueryPerformanceCounter( Cast( LARGE_INTEGER*, &qpc_start ) );
    Forever {
      TimeSleep( 10 );

      u64 qpc_cur;
      QueryPerformanceCounter( Cast( LARGE_INTEGER*, &qpc_cur ) );
      u64 qpc_dticks = qpc_cur - qpc_start;
      if( qpc_dticks > ( qpc_per_sec / 4 ) ) {
        tsc_dticks = TimeTSC() - tsc_start;
        qpc_period = qpc_dticks;
        break;
      }
    }
    g_sec_per_tsc32 = Cast( f32, qpc_period ) / ( Cast( f32, tsc_dticks ) * Cast( f32, qpc_per_sec ) );
    g_sec_per_tsc64 = Cast( f64, qpc_period ) / ( Cast( f64, tsc_dticks ) * Cast( f64, qpc_per_sec ) );
  }

  Log( "large page size: %llu", GetLargePageMinimum() );

#define FASTER_OSCLOCK  0

#if FASTER_OSCLOCK
  TIMECAPS time_caps;
  MMRESULT res = timeGetDevCaps( &time_caps, sizeof( TIMECAPS ) );
  g_has_sleep_prec_period_min = ( res == MMSYSERR_NOERROR );
  if( g_has_sleep_prec_period_min ) {
    g_sleep_prec_period_min = time_caps.wPeriodMin;
    AssertWarn( timeBeginPeriod( g_sleep_prec_period_min ) == TIMERR_NOERROR );
  }
#endif

  u64 t1 = __rdtsc();
  Log( "TimeInit: %f", ( t1 - t0 ) * g_sec_per_tsc64 );

  LogAddIndent( -1 );
  Log( "" );
#elif defined(MAC)
#else
#error Unsupported platform
#endif
}


void
TimeKill()
{
#if FASTER_OSCLOCK
  if( g_has_sleep_prec_period_min ) {
    AssertWarn( timeEndPeriod( g_sleep_prec_period_min ) == TIMERR_NOERROR );
  }
#endif
}


Inl void
FormatTimeDate( u8* dst, idx_t dst_len, idx_t* written_size, struct tm* time_data )
{
  dst[0] = 0;
  u8* date_str = Str( "%Y.%m.%d.%H.%M.%S" ); // Has 19 chars, not counting nul-terminator.
  idx_t date_str_len = ( 19 + 1 ); // +1 for nul-terminator.
  idx_t written = strftime(
    Cast( char*, dst ),
    MIN( dst_len, date_str_len ),
    Cast( char*, date_str ),
    time_data
    );
  *written_size = written;
}

#if defined(MAC)
  void localtime_s( struct tm* time_data, time_t* dst );
#endif

Inl struct tm
LocalTimeDate()
{
  time_t time_raw;
  struct tm time_data;
  time( &time_raw );
  localtime_s( &time_data, &time_raw );
  return time_data;
}
Inl void
TimeDate( u8* dst, idx_t dst_len, idx_t* written_size )
{
  auto time_data = LocalTimeDate();
  FormatTimeDate( dst, dst_len, written_size, &time_data );
}


// NOTE: don't use for wall-clock time, because RDTSC supposedly can't accomplish that.
Inl f32
TimeSecFromTSC32( u64 delta )
{
  return Cast( f32, delta ) * g_sec_per_tsc32;
}

Inl f64
TimeSecFromTSC64( u64 delta )
{
  return Cast( f64, delta ) * g_sec_per_tsc64;
}



// TODO:
// TimeSec32() seems to report at multiples of .015625.
//   the SecondsFromCycles32( CurrentCycle() ) path also seems to do this.
//   this goes away when we use the 64 versions... simple printf issue maybe? or something deeper?
//

// wall-clock time that can be converted to seconds via SecFromClocksX.
Inl u64
TimeClock()
{
#if defined(WIN)
  u64 qpc;
  QueryPerformanceCounter( Cast( LARGE_INTEGER*, &qpc ) );
  return qpc;
#elif defined(MAC)
  ImplementCrash();
  return 0;
#else
#error Unsupported platform
#endif
}


Inl f32
TimeSecFromClocks32( u64 delta )
{
  return Cast( f32, delta ) * g_sec_per_qpc32;
}

Inl f64
TimeSecFromClocks64( u64 delta )
{
  return Cast( f64, delta ) * g_sec_per_qpc64;
}


