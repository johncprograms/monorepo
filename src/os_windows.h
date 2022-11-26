// Copyright (c) John A. Carlos Jr., all rights reserved.

#if defined(WIN)

  // for memory leak checking, put CrtShowMemleaks() first in every entry point.
  #if defined(_DEBUG)
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
    #define CrtShowMemleaks()   _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF )
  #else
    #define CrtShowMemleaks()    // nothing
  #endif

  #pragma comment( lib, "kernel32" )
  #pragma comment( lib, "user32"   )
  #pragma comment( lib, "gdi32"    )
  //#pragma comment( lib, "winspool" )
  //#pragma comment( lib, "comdlg32" )
  //#pragma comment( lib, "advapi32" )
  #pragma comment( lib, "shell32"  )
  #pragma comment( lib, "shcore"   )
  //#pragma comment( lib, "ole32"    )
  //#pragma comment( lib, "oleaut32" )
  //#pragma comment( lib, "uuid"     )
  //#pragma comment( lib, "odbc32"   )
  //#pragma comment( lib, "odbccp32" )
  #pragma comment( lib, "winmm"    )


  //#define _CRT_SECURE_NO_WARNINGS


  // TODO: move these includes to crt_platform.h
  // TODO: move platform specific headers out of this file!

  //#define WIN32_LEAN_AND_MEAN
  #define NOMINMAX

  #include <windows.h>
  #include <ShellScalingApi.h>
  #include <intrin.h>
  #include <immintrin.h>
  #include <process.h>
  #include <dbghelp.h>

  // TODO: more undefs

  #undef GetProp
  #undef min
  #undef Min
  #undef MIN
  #undef max
  #undef Max
  #undef MAX
  #undef clamp
  #undef Clamp
  #undef CLAMP
  #undef abs
  #undef Abs
  #undef ABS
  #undef swap
  #undef Swap
  #undef SWAP

#endif
