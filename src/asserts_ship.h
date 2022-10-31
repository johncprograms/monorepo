// Copyright (c) John A. Carlos Jr., all rights reserved.

#ifndef _DEBUG

  Inl void
  BuildLocationString(
    stack_nonresizeable_stack_t<u8, 512>* str,
    const char* break_if_false,
    const char* file,
    u32 line,
    const char* function
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
  _WarningTriggered( const char* break_if_false, const char* file, u32 line, const char* function )
  {
    stack_nonresizeable_stack_t<u8, 512> str;
    Zero( str );
    BuildLocationString( &str, break_if_false, file, line, function );

    Log( "WARNING: %s", str );
  }

  NoInl void
  _CrashTriggered( const char* break_if_false, const char* file, u32 line, const char* function )
  {
    stack_nonresizeable_stack_t<u8, 512> str;
    Zero( str );
    BuildLocationString( &str, break_if_false, file, line, function );

    Log( "CRASH: %s", str );

    // TODO: abort/retry/ignore kind of thing here?
    //   probably a hashtable of some arg subset to identify manually ignored asserts.
    //   we could also save that out to some file eventually, if we want to ignore asserts across program runs.
#ifdef WIN
    MessageBoxA( 0, Cast( LPCSTR, str.mem ), "Crash!", 0 );
#elifdef MAC
#else
#error Unsupported platform
#endif

    __debugbreak();

    *(volatile int*)0 = 0;
  }

#endif
