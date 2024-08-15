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

static stack_resizeable_cont_t<onmainkill_entry_t> g_onmainkill_entries = {};

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


#if defined(WIN)
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

    fsobj_t filename = FsGetExe();
    if( filename.len ) {
      auto last_slash = StringScanL( ML( filename ), '/' );
      AssertCrash( last_slash );
      filename.len = ( last_slash - filename.mem );
      filename.len += 1; // include slash.

      stack_nonresizeable_stack_t<u8, 64> tmp;
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

#elif defined(MAC)
#else
#error Unsupported platform
#endif

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




Inl void
MainInit()
{
#if defined(WIN)
  CrtShowMemleaks();
  SetUnhandledExceptionFilter( UnhandledSEHWriteDump );

  _set_purecall_handler( HandleCRTPureCall );
  _set_invalid_parameter_handler( HandleCRTInvalidParameter );

  SetErrorMode(SEM_FAILCRITICALERRORS);
#endif

  FindLeaksInit();
  ProfInit();
  LoggerInit();
  MainThreadInit();
  TimeInit();

  Alloc( g_onmainkill_entries, 16 );

  // caused problems with Execute wiring of stdout/stdin, so turn this off.
  // keeping for future reference if we need it again.
#if 0 // #if defined(_DEBUG)
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
//  ProfOutputTimeline();
  ProfKill();
  LoggerKill();
  TimeKill();
  MainThreadKill();
  FindLeaksKill();
}

