// Copyright (c) John A. Carlos Jr., all rights reserved.

#define __OnMainKill( name ) \
  void ( name )( void* user )

typedef __OnMainKill( *pfn_onmainkill_t );

struct
onmainkill_entry_t
{
  pfn_onmainkill_t fn;
  void* user;
};

static array_t<onmainkill_entry_t> g_onmainkill_entries = {};

Inl void
RegisterOnMainKill(
  pfn_onmainkill_t fn,
  void* user
  )
{
  onmainkill_entry_t entry;
  entry.fn = fn;
  entry.user = user;
  *AddBack( g_onmainkill_entries ) = entry;
}



typedef BOOL ( WINAPI *pfn_minidumpwritedump_t )(
  HANDLE hProcess,
  DWORD dwPid,
  HANDLE hFile,
  MINIDUMP_TYPE DumpType,
  PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
  PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
  PMINIDUMP_CALLBACK_INFORMATION CallbackParam
  );

void
WriteDump( _EXCEPTION_POINTERS* exc_ptrs )
{
  HMODULE dbghelp_dll = LoadLibraryA( "dbghelp.dll" );
  pfn_minidumpwritedump_t pfn = Cast( pfn_minidumpwritedump_t, GetProcAddress( dbghelp_dll, "MiniDumpWriteDump" ) );

  fsobj_t filename;

  filename.len = GetModuleFileName( 0, Cast( LPSTR, filename.mem ), Cast( DWORD, Capacity( filename ) ) );
  filename = _StandardFilename( ML( filename ) );

  if( filename.len ) {
    auto last_slash = CsScanL( ML( filename ), '/' );
    AssertCrash( last_slash );
    filename.len = ( last_slash - filename.mem );
    filename.len += 1; // include slash.

    embeddedarray_t<u8, 64> tmp;
    TimeDate( tmp.mem, Capacity( tmp ), &tmp.len );
    AssertWarn( tmp.len );

    Memmove( AddBack( filename, 8 ), "te_logs/", 8 );
    Memmove( AddBack( filename, tmp.len ), ML( tmp ) );
    Memmove( AddBack( filename, 4 ), ".dmp", 4 );

    *AddBack( filename ) = 0;

    HANDLE hFile = CreateFileA(
      Cast( LPCSTR, filename.mem ),
      GENERIC_WRITE,
      FILE_SHARE_WRITE,
      0,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      0
      );

    _MINIDUMP_EXCEPTION_INFORMATION exc_info;
    exc_info.ThreadId = GetCurrentThreadId();
    exc_info.ExceptionPointers = exc_ptrs;
    exc_info.ClientPointers = 0; // TODO: should this be 1 ?

    pfn(
      GetCurrentProcess(),
      GetCurrentProcessId(),
      hFile,
      Cast( MINIDUMP_TYPE, MiniDumpWithFullMemory | MiniDumpIgnoreInaccessibleMemory | MiniDumpWithThreadInfo ),
      &exc_info,
      0,
      0
      );

    CloseHandle( hFile );
  }
}

LONG WINAPI
UnhandledSEHWriteDump( _EXCEPTION_POINTERS* exc_ptrs )
{
  WriteDump( exc_ptrs );
//  return EXCEPTION_CONTINUE_SEARCH;
  return EXCEPTION_EXECUTE_HANDLER;
}


void __cdecl
HandleCRTPureCall()
{
  s32 crazy_vftable_empty_function_invoked = 0;
  AssertCrash( crazy_vftable_empty_function_invoked );
}


void HandleCRTInvalidParameter(
  const wchar_t* expression,
  const wchar_t* function,
  const wchar_t* file,
  u32 line,
  idx_t reserved
  )
{
  s32 crt_function_given_invalid_param = 0;
  AssertCrash( crt_function_given_invalid_param );
}


#ifndef _DEBUG

  Inl void
  BuildLocationString(
    embeddedarray_t<u8, 512>* str,
    char* break_if_false,
    char* file,
    u32 line,
    char* function
    )
  {
    AddBackCStr( str, file );
    AddBackCStr( str, " | " );
    idx_t num_len = 0;
    CsFromIntegerU( str->mem + str->len, Capacity( *str ) - str->len, &num_len, line, 1 );
    str->len += num_len;
    AddBackCStr( str, " | " );
    AddBackCStr( str, function );
    AddBackCStr( str, " | " );
    AddBackCStr( str, break_if_false );
    *AddBack( *str ) = 0;
  }

  NoInl void
  _WarningTriggered( char* break_if_false, char* file, u32 line, char* function )
  {
    embeddedarray_t<u8, 512> str;
    Zero( str );
    BuildLocationString( &str, break_if_false, file, line, function );

    Log( "WARNING: %s", str );
  }

  NoInl void
  _CrashTriggered( char* break_if_false, char* file, u32 line, char* function )
  {
    embeddedarray_t<u8, 512> str;
    Zero( str );
    BuildLocationString( &str, break_if_false, file, line, function );

    Log( "CRASH: %s", str );

    // TODO: abort/retry/ignore kind of thing here?
    //   probably a hashtable of some arg subset to identify manually ignored asserts.
    //   we could also save that out to some file eventually, if we want to ignore asserts across program runs.

    MessageBoxA( 0, Cast( LPCSTR, str.mem ), "Crash!", 0 );

    __debugbreak();

    *(int*)0 = 0;
  }

#endif



#if FINDLEAKS

  static constexpr idx_t c_extra = 64;
  static volatile u64 __counter = 0;
  static volatile bool g_track_active_allocs = 0;
  static lock_t g_active_allocs_lock = 0;
  static hashset_t g_active_allocs;

  Inl void*
  MemHeapAllocBytes( idx_t nbytes )
  {
    nbytes += c_extra;
    auto m = Cast( u8*, _aligned_malloc( nbytes, DEFAULT_ALIGN ) );
    auto counter = ( InterlockedIncrement( &__counter ) - 1 ); // GetValueBeforeAtomicInc
    *Cast( u64*, m ) = counter;
    m += c_extra;

    Lock( &g_active_allocs_lock );
    if( g_track_active_allocs ) {
      g_track_active_allocs = 0; // temporarily turn off alloc tracking for the tracking hashset expansion code.
      {
        bool already_there = 0;
        Add(
          g_active_allocs,
          &counter,
          0,
          &already_there,
          0,
          0
          );
        AssertCrash( !already_there );
      }
      g_track_active_allocs = 1;
    }
    Unlock( &g_active_allocs_lock );

//    if( counter == 7381 ) __debugbreak();
//    Log( "alloc # %llu, nbytes %llu", counter, nbytes );

    return m;
  }

  Inl void*
  MemHeapReallocBytes( void* mem, idx_t oldlen, idx_t newlen )
  {
    auto m = Cast( u8*, mem );
    m -= c_extra;
    newlen += c_extra;
    AssertCrash( !( Cast( idx_t, m ) & DEFAULT_ALIGNMASK ) );
    auto mnew = Cast( u8*, _aligned_realloc( m, newlen, DEFAULT_ALIGN ) );
    mnew += c_extra;
    return mnew;
  }

  Inl void
  MemHeapFree( void* mem )
  {
    if( !mem ) {
      return;
    }
    auto m = Cast( u8*, mem );
    m -= c_extra;
    auto counter = *Cast( u64*, m );
    AssertCrash( !( Cast( idx_t, m ) & DEFAULT_ALIGNMASK ) );
    _aligned_free( m );

    Lock( &g_active_allocs_lock );
    if( g_track_active_allocs ) {
      g_track_active_allocs = 0; // temporarily turn off alloc tracking for the tracking hashset expansion code.
      {
        bool found = 0;
        Remove(
          g_active_allocs,
          &counter,
          &found,
          0
          );
        AssertCrash( found );
      }
      g_track_active_allocs = 1;
    }
    Unlock( &g_active_allocs_lock );
  }

#else

  Inl void*
  MemHeapAllocBytes( idx_t nbytes )
  {
    // NOTE: ~1 Gb heap allocations tended to cause a hang on my old laptop.
    // Anything remotely close to that size should be made by VirtualAlloc, not by the CRT heap!
    AssertCrash( nbytes <= 1ULL*1024*1024*1024 );

    void* mem = _aligned_malloc( nbytes, DEFAULT_ALIGN );
    AssertCrash( !( Cast( idx_t, mem ) & DEFAULT_ALIGNMASK ) );
    return mem;
  }


  Inl void*
  MemHeapReallocBytes( void* mem, idx_t oldlen, idx_t newlen )
  {
    ProfFunc();

    // NOTE: ~1 Gb heap allocations tended to cause a hang on my old laptop.
    // Anything remotely close to that size should be made by VirtualAlloc, not by the CRT heap!
    AssertCrash( newlen <= 1ULL*1024*1024*1024 );
    AssertCrash( !( Cast( idx_t, mem ) & DEFAULT_ALIGNMASK ) );

  #if 0
    void* memnew = _aligned_malloc( newlen, DEFAULT_ALIGN );
    Memmove( memnew, mem, MIN( oldlen, newlen ) );
    _aligned_free( mem );
  #else
    void* memnew = _aligned_realloc( mem, newlen, DEFAULT_ALIGN );
  #endif
    AssertCrash( !( Cast( idx_t, memnew ) & DEFAULT_ALIGNMASK ) );
    return memnew;
  }

  // TODO: zero mem after free for everyone!

  Inl void
  MemHeapFree( void* mem )
  {
    AssertCrash( !( Cast( idx_t, mem ) & DEFAULT_ALIGNMASK ) );
    _aligned_free( mem );
  }

#endif



Inl void
MainInit()
{
  CrtShowMemleaks();

  SetUnhandledExceptionFilter( UnhandledSEHWriteDump );
  _set_purecall_handler( HandleCRTPureCall );
  _set_invalid_parameter_handler( HandleCRTInvalidParameter );

  SetErrorMode(SEM_FAILCRITICALERRORS);

#if FINDLEAKS
  Init(
    g_active_allocs,
    10000,
    8,
    0,
    0.9f,
    Equal_FirstU64,
    Hash_FirstU64
    );

  g_track_active_allocs = 1;
#endif

  ProfInit();
  LoggerInit();
  MainThreadInit();
  TimeInit();

  Alloc( g_onmainkill_entries, 16 );

  // caused problems with Execute wiring of stdout/stdin, so turn this off.
  // keeping for future reference if we need it again.
#if 0 // #ifdef _DEBUG
  AllocConsole();
  FILE* tmp;
  freopen_s( &tmp, "CONOUT$", "wb", stdout );
  freopen_s( &tmp, "CONOUT$", "wb", stderr );
#endif
}

Inl void
MainKill()
{
  ReverseForLen( i, g_onmainkill_entries ) {
    auto entry = g_onmainkill_entries.mem + i;
    entry->fn( entry->user );
  }
  Free( g_onmainkill_entries );

  ProfOutputZoneStats();
  ProfOutputTimeline();
  ProfKill();
  LoggerKill();
  TimeKill();
  MainThreadKill();

#if FINDLEAKS
  g_track_active_allocs = 0;

  ForLen( i, g_active_allocs.elems ) {
    auto elem = ByteArrayElem( hashset_elem_t, g_active_allocs.elems, i );
    if( elem->has_data ) {
      auto counter = *Cast( u64*, _GetElemData( g_active_allocs, elem ) );
      printf( "Leaked allocation # %llu\n", counter );
      __debugbreak();
    }
  }
  Kill( g_active_allocs );
#endif
}

