// build:console_x64_debug
// build:console_x64_optimized
// Copyright (c) John A. Carlos Jr., all rights reserved.

#if defined(WIN)

#include "os_mac.h"
#include "os_windows.h"

#define FINDLEAKS   0
#define WEAKINLINING   0
#include "core_cruntime.h"
#include "core_types.h"
#include "core_language_macros.h"
#include "asserts.h"
#include "memory_operations.h"
#include "math_integer.h"
#include "math_float.h"
#include "math_lerp.h"
#include "math_floatvec.h"
#include "math_matrix.h"
#include "math_kahansummation.h"
#include "allocator_heap.h"
#include "allocator_virtual.h"
#include "allocator_heap_or_virtual.h"
#include "cstr.h"
#include "ds_slice.h"
#include "ds_string.h"
#include "allocator_pagelist.h"
#include "ds_stack_resizeable_cont.h"
#include "ds_stack_nonresizeable_stack.h"
#include "ds_stack_nonresizeable.h"
#include "ds_stack_resizeable_pagelist.h"
#include "ds_pagetree.h"
#include "ds_list.h"
#include "ds_stack_cstyle.h"
#include "ds_hashset_cstyle.h"
#include "filesys.h"
#include "cstr_integer.h"
#include "cstr_float.h"
#include "timedate.h"
#include "thread_atomics.h"
#include "ds_mtqueue_srsw_nonresizeable.h"
#include "ds_mtqueue_srmw_nonresizeable.h"
#include "ds_mtqueue_mrsw_nonresizeable.h"
#include "ds_mtqueue_mrmw_nonresizeable.h"
#include "threading.h"
#define LOGGER_ENABLED   0
#include "logger.h"
#define PROF_ENABLED   0
#define PROF_ENABLED_AT_LAUNCH   0
#include "profile.h"
#include "rand.h"
#include "allocator_heap_findleaks.h"
#include "mainthread.h"
#include "text_parsing.h"
#include "ds_stack_resizeable_cont_addbacks.h"


#define PRINT_TOOL_COMMANDS   0



Enumc( target_plat_t )
{
  x64,
  x86,
};
Enumc( target_type_t )
{
  window,
  console,
};
Enumc( target_flavor_t )
{
  debug,
  optimized,
  releaseversion,
};

struct
envvar_and_value_t
{
  LPCSTR var;
  LPCSTR value;
};

// note there's no concerns here about stalling UI.
// the printfs will be immediately captured by the parent cmd.exe that's running this build.exe process,
// and display them asap. so no real concerns here.
__ExecuteOutput( OutputForToolExecute )
{
  auto tmp = AllocCstr( *message );
  if( internal_failure ) {
    printf( "Internal error: %s", tmp );
  }
  printf( "%s", tmp );
  MemHeapFree( tmp );
}

int
Main( stack_resizeable_cont_t<slice_t>& args )
{
  printf( "[Build started]\n" );
  auto t0 = TimeTSC();

  if( args.len != 1 ) {
    printf( "Expected one argument, the name of a primary cpp file to build! e.g. 'te' for building 'main_te.cpp'\n" );
    return 1;
  }
  auto name = args.mem + 0;
  auto cpp_prefix = SliceFromCStr( "src/main_" );
  auto cpp_suffix = SliceFromCStr( ".cpp" );
  stack_nonresizeable_stack_t<u8, 1024> filename_cpp;
  filename_cpp.len = 0;
  Memmove( AddBack( filename_cpp, cpp_prefix.len ), ML( cpp_prefix ) );
  Memmove( AddBack( filename_cpp, name->len ), ML( *name ) );
  Memmove( AddBack( filename_cpp, cpp_suffix.len ), ML( cpp_suffix ) );

  // note this is tied to compiler options below which direct where the obj file goes.
  auto obj_prefix = SliceFromCStr( "exe/main_" );
  auto obj_suffix = SliceFromCStr( ".obj" );
  stack_nonresizeable_stack_t<u8, 1024> filename_obj;
  filename_obj.len = 0;
  Memmove( AddBack( filename_obj, obj_prefix.len ), ML( obj_prefix ) );
  Memmove( AddBack( filename_obj, name->len ), ML( *name ) );
  Memmove( AddBack( filename_obj, obj_suffix.len ), ML( obj_suffix ) );

  file_t file = FileOpen( ML( filename_cpp ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
  if( !file.loaded ) {
    auto tmpslice = SliceFromArray( filename_cpp );
    auto tmp = AllocCstr( tmpslice );
    printf( "Couldn't open file: %s\n", tmp );
    MemHeapFree( tmp );
    return 1;
  }
  auto contents = FileAlloc( file );
  auto eol = CursorStopAtNewlineR( ML( contents ), 0 );
  slice_t firstline = { contents.mem, eol };

  auto cmd_window_x64_debug          = SliceFromCStr( "// build:window_x64_debug" );
  auto cmd_consol_x64_debug          = SliceFromCStr( "// build:console_x64_debug" );
  auto cmd_window_x64_optimized      = SliceFromCStr( "// build:window_x64_optimized" );
  auto cmd_consol_x64_optimized      = SliceFromCStr( "// build:console_x64_optimized" );
  auto cmd_window_x64_releaseversion = SliceFromCStr( "// build:window_x64_releaseversion" );
  auto cmd_consol_x64_releaseversion = SliceFromCStr( "// build:console_x64_releaseversion" );
  auto cmd_window_x86_debug          = SliceFromCStr( "// build:window_x86_debug" );
  auto cmd_consol_x86_debug          = SliceFromCStr( "// build:console_x86_debug" );
  auto cmd_window_x86_optimized      = SliceFromCStr( "// build:window_x86_optimized" );
  auto cmd_consol_x86_optimized      = SliceFromCStr( "// build:console_x86_optimized" );
  auto cmd_window_x86_releaseversion = SliceFromCStr( "// build:window_x86_releaseversion" );
  auto cmd_consol_x86_releaseversion = SliceFromCStr( "// build:console_x86_releaseversion" );

  target_plat_t plat;
  if( EqualContents( firstline, cmd_window_x64_debug          )  ||
      EqualContents( firstline, cmd_consol_x64_debug          )  ||
      EqualContents( firstline, cmd_window_x64_optimized      )  ||
      EqualContents( firstline, cmd_consol_x64_optimized      )  ||
      EqualContents( firstline, cmd_window_x64_releaseversion )  ||
      EqualContents( firstline, cmd_consol_x64_releaseversion ) )
  {
    plat = target_plat_t::x64;
  }
  elif( EqualContents( firstline, cmd_window_x86_debug          )  ||
        EqualContents( firstline, cmd_consol_x86_debug          )  ||
        EqualContents( firstline, cmd_window_x86_optimized      )  ||
        EqualContents( firstline, cmd_consol_x86_optimized      )  ||
        EqualContents( firstline, cmd_window_x86_releaseversion )  ||
        EqualContents( firstline, cmd_consol_x86_releaseversion ) )
  {
    plat = target_plat_t::x86;
  }
  else {
    printf( "Unrecognized target platform!\n" );
    return 1;
  }

  target_type_t type;
  if( EqualContents( firstline, cmd_window_x64_debug          )  ||
      EqualContents( firstline, cmd_window_x64_optimized      )  ||
      EqualContents( firstline, cmd_window_x64_releaseversion )  ||
      EqualContents( firstline, cmd_window_x86_debug          )  ||
      EqualContents( firstline, cmd_window_x86_optimized      )  ||
      EqualContents( firstline, cmd_window_x86_releaseversion ) )
  {
    type = target_type_t::window;
  }
  elif( EqualContents( firstline, cmd_consol_x64_debug          )  ||
        EqualContents( firstline, cmd_consol_x64_optimized      )  ||
        EqualContents( firstline, cmd_consol_x64_releaseversion )  ||
        EqualContents( firstline, cmd_consol_x86_debug          )  ||
        EqualContents( firstline, cmd_consol_x86_optimized      )  ||
        EqualContents( firstline, cmd_consol_x86_releaseversion ) )
  {
    type = target_type_t::console;
  }
  else {
    printf( "Unrecognized target type!\n" );
    return 1;
  }

  target_flavor_t flavor;
  if( EqualContents( firstline, cmd_window_x64_debug )  ||
      EqualContents( firstline, cmd_consol_x64_debug )  ||
      EqualContents( firstline, cmd_window_x86_debug )  ||
      EqualContents( firstline, cmd_consol_x86_debug ) )
  {
    flavor = target_flavor_t::debug;
  }
  elif( EqualContents( firstline, cmd_window_x64_optimized )  ||
        EqualContents( firstline, cmd_consol_x64_optimized )  ||
        EqualContents( firstline, cmd_window_x86_optimized )  ||
        EqualContents( firstline, cmd_consol_x86_optimized ) )
  {
    flavor = target_flavor_t::optimized;
  }
  elif( EqualContents( firstline, cmd_window_x64_releaseversion )  ||
        EqualContents( firstline, cmd_consol_x64_releaseversion )  ||
        EqualContents( firstline, cmd_window_x86_releaseversion )  ||
        EqualContents( firstline, cmd_consol_x86_releaseversion ) )
  {
    flavor = target_flavor_t::releaseversion;
  }
  else {
    printf( "Unrecognized target flavor!\n" );
    return 1;
  }

  Free( contents );
  FileFree( file );

  auto rc_exe = Str( "C:/Program Files (x86)/Windows Kits/10/bin/10.0.18362.0/x64/rc.exe" );

  auto toolchain_path = Str( "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/" );

  stack_resizeable_cont_t<u8> compile;
  Alloc( compile, 64000 );
  AddBackCStr( &compile, "\"" );
  AddBackCStr( &compile, toolchain_path );
  switch( plat ) {
    case target_plat_t::x64: AddBackCStr( &compile, "x64" ); break;
    case target_plat_t::x86: AddBackCStr( &compile, "x86" ); break;
    default: UnreachableCrash();
  }
  AddBackCStr( &compile, "/cl.exe" );
  AddBackCStr( &compile, "\"" );
  AddBackCStr( &compile, " " );

  stack_resizeable_cont_t<u8> link;
  Alloc( link, 64000 );
  AddBackCStr( &link, "\"" );
  AddBackCStr( &link, toolchain_path );
  switch( plat ) {
    case target_plat_t::x64: AddBackCStr( &link, "x64" ); break;
    case target_plat_t::x86: AddBackCStr( &link, "x86" ); break;
    default: UnreachableCrash();
  }
  AddBackCStr( &link, "/link.exe" );
  AddBackCStr( &link, "\"" );
  AddBackCStr( &link, " " );

  // use the current datetime, since we're building right now.
  auto time = LocalTimeDate();
  stack_nonresizeable_stack_t<u8, 64> now;
  FormatTimeDate( now.mem, Capacity( now ), &now.len, &time );

  // generate the string macro defn /DJCVERSION="yyyy.mm.dd.hh.mm.ss"
  stack_nonresizeable_stack_t<u8, 128> define_version;
  define_version.len = 0;
  auto ver_prefix = SliceFromCStr( "/DJCVERSION=\\\"" );
  auto ver_suffix = SliceFromCStr( "\\\"" );
  Memmove( AddBack( define_version, ver_prefix.len ), ML( ver_prefix ) );
  Memmove( AddBack( define_version, now.len ), ML( now ) );
  Memmove( AddBack( define_version, ver_suffix.len ), ML( ver_suffix ) );
  *AddBack( define_version ) = 0;

  // set the env vars up to match what vsdevcmd.bat / vcvarsall.bat do.
  // to come up with these, run "vscmd19_64.bat" or the equivalent and use 'set' to observe what env changes it makes.
  // then just hardcode them here.
  // the reason we do all this is so we can avoid having to run vsdevcmd.bat for each compile/link invocation,
  // since it takes ~1 second to run. super slow and terrible.
  // so we set the equivalent env vars here in the build process, and the compile/links are child processes,
  // which inherit the env vars, and hence will work just fine.

  // note we'll have to update these every time we update VS.
  // we could make an automated update script, something that ran set, then vscmd19_64 and set, did the diff,
  // constructed some variable-defn file somewhere. then we could just parse that file here.
  // since i update VS probably once a year or less, i'll wait on doing this work.
  // it'd also be annoying to have to test VS updates.

  stack_nonresizeable_t<u8> path;
  Alloc( path, 32768 );
  path.len = GetEnvironmentVariableA( "Path", Cast( LPSTR, path.mem ), Cast( DWORD, Capacity( path ) ) );
  AssertCrash( path.len );
  slice_t path_concat = {};
  switch( plat ) {
    case target_plat_t::x64: {
      path_concat = SliceFromCStr(
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/Extensions/Microsoft/IntelliCode/CLI;"
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/bin/HostX64/x64;"
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/VC/VCPackages;"
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/CommonExtensions/Microsoft/TestWindow;"
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/CommonExtensions/Microsoft/TeamFoundation/Team Explorer;"
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/MSBuild/Current/bin/Roslyn;"
        "C:/Program Files (x86)/Microsoft SDKs/Windows/v10.0A/bin/NETFX 4.7.1 Tools/x64/;"
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/Tools/devinit;"
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.18362.0/x64;"
        "C:/Program Files (x86)/Windows Kits/10/bin/x64;"
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/MSBuild/Current/Bin;"
        "C:/Windows/Microsoft.NET/Framework64/v4.0.30319;"
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/;"
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/Tools/;"
        );
    } break;
    case target_plat_t::x86: {
      printf( "x86 is deprecated!" );
      return 1;
//      path_concat = SliceFromCStr(
//        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/Extensions/Microsoft/IntelliCode/CLI;"
//        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29333/bin/HostX64/x86;"
//        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29333/bin/HostX64/x64;"
//        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/VC/VCPackages;"
//        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/CommonExtensions/Microsoft/TestWindow;"
//        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/CommonExtensions/Microsoft/TeamFoundation/Team Explorer;"
//        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/MSBuild/Current/bin/Roslyn;"
//        "C:/Program Files (x86)/Microsoft SDKs/Windows/v10.0A/bin/NETFX 4.7.1 Tools/x64/;"
//        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.18362.0/x64;"
//        "C:/Program Files (x86)/Windows Kits/10/bin/x64;"
//        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/MSBuild/Current/Bin;"
//        "C:/Windows/Microsoft.NET/Framework64/v4.0.30319;"
//        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/;"
//        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/Tools/;"
//        );
    } break;
    default: UnreachableCrash();
  }
  Memmove( AddAt( path, 0, path_concat.len ), ML( path_concat ) );
  *AddBack( path ) = 0;
  BOOL r = SetEnvironmentVariableA( "Path", Cast( LPCSTR, path.mem ) );
  AssertCrash( r );
  Free( path );

  switch( plat ) {
    case target_plat_t::x64: {
      envvar_and_value_t vars_and_values[] = {
        { "CommandPromptType", "Native" },
        { "DevEnvDir", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/" },
        { "ExtensionSdkDir", "C:/Program Files (x86)/Microsoft SDKs/Windows Kits/10/ExtensionSDKs" },
        { "Framework40Version", "v4.0" },
        { "FrameworkDir", "C:/Windows/Microsoft.NET/Framework64/" },
        { "FrameworkDIR64", "C:/Windows/Microsoft.NET/Framework64" },
        { "FrameworkVersion", "v4.0.30319" },
        { "FrameworkVersion64", "v4.0.30319" },
        { "INCLUDE",
          "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/include;"
          "C:/Program Files (x86)/Windows Kits/NETFXSDK/4.7.1/include/um;"
          "C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/ucrt;"
          "C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/shared;"
          "C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/um;"
          "C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/winrt;"
          "C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/cppwinrt"
          },
        { "LIB",
          "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/lib/x64;"
          "C:/Program Files (x86)/Windows Kits/NETFXSDK/4.7.1/lib/um/x64;"
          "C:/Program Files (x86)/Windows Kits/10/lib/10.0.18362.0/ucrt/x64;"
          "C:/Program Files (x86)/Windows Kits/10/lib/10.0.18362.0/um/x64"
          },
        { "LIBPATH",
          "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/lib/x64;"
          "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/lib/x86/store/references;"
          "C:/Program Files (x86)/Windows Kits/10/UnionMetadata/10.0.18362.0;"
          "C:/Program Files (x86)/Windows Kits/10/References/10.0.18362.0;"
          "C:/Windows/Microsoft.NET/Framework64/v4.0.30319"
          },
        { "NETFXSDKDir", "C:/Program Files (x86)/Windows Kits/NETFXSDK/4.7.1/" },
        { "UCRTVersion", "10.0.18362.0" },
        { "UniversalCRTSdkDir", "C:/Program Files (x86)/Windows Kits/10/" },
        { "VCIDEInstallDir", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/VC/" },
        { "VCINSTALLDIR", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/" },
        { "VCToolsInstallDir", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/" },
        { "VCToolsRedistDir", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Redist/MSVC/14.29.30133/" },
        { "VCToolsVersion", "14.29.30133" },
        { "VisualStudioVersion", "16.0" },
        { "VS140COMNTOOLS", "C:/Program Files (x86)/Microsoft Visual Studio 14.0/Common7/Tools/" },
        { "VS160COMNTOOLS", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/Tools/" },
        { "VSCMD_ARG_app_plat", "Desktop" },
        { "VSCMD_ARG_HOST_ARCH", "x64" },
        { "VSCMD_ARG_no_logo", "1" },
        { "VSCMD_ARG_TGT_ARCH", "x64" },
        { "VSCMD_VER", "16.11.24" },
        { "VSINSTALLDIR", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/" },
        { "WindowsLibPath",
          "C:/Program Files (x86)/Windows Kits/10/UnionMetadata/10.0.18362.0;"
          "C:/Program Files (x86)/Windows Kits/10/References/10.0.18362.0"
          },
        { "WindowsSdkBinPath", "C:/Program Files (x86)/Windows Kits/10/bin/" },
        { "WindowsSdkDir", "C:/Program Files (x86)/Windows Kits/10/" },
        { "WindowsSDKLibVersion", "10.0.18362.0/" },
        { "WindowsSdkVerBinPath", "C:/Program Files (x86)/Windows Kits/10/bin/10.0.18362.0/" },
        { "WindowsSDKVersion", "10.0.18362.0/" },
        { "WindowsSDK_ExecutablePath_x64", "C:/Program Files (x86)/Microsoft SDKs/Windows/v10.0A/bin/NETFX 4.7.1 Tools/x64/" },
        { "WindowsSDK_ExecutablePath_x86", "C:/Program Files (x86)/Microsoft SDKs/Windows/v10.0A/bin/NETFX 4.7.1 Tools/" },
        { "__devinit_path", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/Tools/devinit/devinit.exe" },
        { "__DOTNET_ADD_64BIT", "1" },
        { "__DOTNET_PREFERRED_BITNESS", "64" },
        };
      ForEach( var_and_value, vars_and_values ) {
        auto var = var_and_value.var;
        auto value = var_and_value.value;
        BOOL r2 = SetEnvironmentVariableA( var, value );
        AssertCrash( r2 );
      }
    } break;

    case target_plat_t::x86: {
      printf( "x86 is deprecated!" );
      return 1;
//      envvar_and_value_t vars_and_values[] = {
//        { "CommandPromptType", "Cross" },
//        { "DevEnvDir", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/" },
//        { "ExtensionSdkDir", "C:/Program Files (x86)/Microsoft SDKs/Windows Kits/10/ExtensionSDKs" },
//        { "Framework40Version", "v4.0" },
//        { "FrameworkDir", "C:/Windows/Microsoft.NET/Framework64/" },
//        { "FrameworkDIR32", "C:/Windows/Microsoft.NET/Framework/" },
//        { "FrameworkDIR64", "C:/Windows/Microsoft.NET/Framework64" },
//        { "FrameworkVersion", "v4.0.30319" },
//        { "FrameworkVersion32", "v4.0.30319" },
//        { "FrameworkVersion64", "v4.0.30319" },
//        { "INCLUDE",
//          "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29333/include;"
//          "C:/Program Files (x86)/Windows Kits/NETFXSDK/4.7.1/include/um;"
//          "C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/ucrt;"
//          "C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/shared;"
//          "C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/um;"
//          "C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/winrt;"
//          "C:/Program Files (x86)/Windows Kits/10/include/10.0.18362.0/cppwinrt"
//          },
//        { "LIB",
//          "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29333/lib/x86;"
//          "C:/Program Files (x86)/Windows Kits/NETFXSDK/4.7.1/lib/um/x86;"
//          "C:/Program Files (x86)/Windows Kits/10/lib/10.0.18362.0/ucrt/x86;"
//          "C:/Program Files (x86)/Windows Kits/10/lib/10.0.18362.0/um/x86"
//          },
//        { "LIBPATH",
//          "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29333/lib/x86;"
//          "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29333/lib/x86/store/references;"
//          "C:/Program Files (x86)/Windows Kits/10/UnionMetadata/10.0.18362.0;"
//          "C:/Program Files (x86)/Windows Kits/10/References/10.0.18362.0;"
//          "C:/Windows/Microsoft.NET/Framework64/v4.0.30319"
//          },
//        { "NETFXSDKDir", "C:/Program Files (x86)/Windows Kits/NETFXSDK/4.7.1/" },
//        { "PreferredToolArchitecture", "x64" },
//        { "UCRTVersion", "10.0.18362.0" },
//        { "UniversalCRTSdkDir", "C:/Program Files (x86)/Windows Kits/10/" },
//        { "VCIDEInstallDir", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/VC/" },
//        { "VCINSTALLDIR", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/" },
//        { "VCToolsInstallDir", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29333/" },
//        { "VCToolsRedistDir", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Redist/MSVC/14.28.29325/" },
//        { "VCToolsVersion", "14.28.29333" },
//        { "VisualStudioVersion", "16.0" },
//        { "VS160COMNTOOLS", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/Tools/" },
//        { "VSCMD_ARG_app_plat", "Desktop" },
//        { "VSCMD_ARG_HOST_ARCH", "x64" },
//        { "VSCMD_ARG_no_logo", "1" },
//        { "VSCMD_ARG_TGT_ARCH", "x86" },
//        { "VSCMD_VER", "16.8.3" },
//        { "VSINSTALLDIR", "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/" },
//        { "WindowsLibPath",
//          "C:/Program Files (x86)/Windows Kits/10/UnionMetadata/10.0.18362.0;"
//          "C:/Program Files (x86)/Windows Kits/10/References/10.0.18362.0"
//          },
//        { "WindowsSdkBinPath", "C:/Program Files (x86)/Windows Kits/10/bin/" },
//        { "WindowsSdkDir", "C:/Program Files (x86)/Windows Kits/10/" },
//        { "WindowsSDKLibVersion", "10.0.18362.0/" },
//        { "WindowsSdkVerBinPath", "C:/Program Files (x86)/Windows Kits/10/bin/10.0.18362.0/" },
//        { "WindowsSDKVersion", "10.0.18362.0/" },
//        { "WindowsSDK_ExecutablePath_x64", "C:/Program Files (x86)/Microsoft SDKs/Windows/v10.0A/bin/NETFX 4.7.1 Tools/x64/" },
//        { "WindowsSDK_ExecutablePath_x86", "C:/Program Files (x86)/Microsoft SDKs/Windows/v10.0A/bin/NETFX 4.7.1 Tools/" },
//        };
//      ForEach( var_and_value, vars_and_values ) {
//        auto var = var_and_value.var;
//        auto value = var_and_value.value;
//        BOOL r2 = SetEnvironmentVariableA( var, value );
//        AssertCrash( r2 );
//      }
    } break;
    default: UnreachableCrash();
  }

  // needed as a destination location by .natvis copying below.
  fsobj_t vs_install_dir = {};
  DWORD vcount = ExpandEnvironmentStringsA(
    "%VSInstallDir%",
    Cast( LPSTR, vs_install_dir.mem ),
    Cast( DWORD, Capacity( vs_install_dir ) )
    );
  if( !vcount ) {
    printf( "Can't find 'VSInstallDir' environment variable!\n" );
    return 1;
  }
  AssertCrash( vcount > 0 );
  vs_install_dir.len = CstrLength( vs_install_dir.mem );

  printf( "[Environment] %.3f seconds\n", TimeSecFromTSC32( TimeTSC() - t0 ) );

  // =========================================
  //        GENERATE RESOURCE FILE

  // windows has a strange versioning scheme, u16x4.
  // to make our time version more readable, that means we have to merge some of our dividers.
  // the natural choice are the least important: seconds, minutes, hours. this gets us down to 4 sections.
  // but, hhmmss can exceed u16 bounds! the max value is 235960, which is > 65535.
  // so to make it fit, we'll also drop the least significant second. Ex:
  // 2021.03.24.19.30.37
  // 2021.03.24.19303
  auto year = time.tm_year + 1900;
  auto month = time.tm_mon + 1; // one-based.
  auto day = time.tm_mday; // already one-based, see tm defn.
  auto hour = time.tm_hour;
  auto min = time.tm_min;
  auto sec = time.tm_sec;
  auto last_section = 1000 * hour + 10 * min + sec / 10;

  stack_resizeable_cont_t<u8> winver_comma;
  Alloc( winver_comma, 64 );
  AddBackSInt( &winver_comma, year );
  AddBackCStr( &winver_comma, "," );
  AddBackSInt( &winver_comma, month );
  AddBackCStr( &winver_comma, "," );
  AddBackSInt( &winver_comma, day );
  AddBackCStr( &winver_comma, "," );
  AddBackSInt( &winver_comma, last_section );

  stack_resizeable_cont_t<u8> winver_period;
  Alloc( winver_period, 64 );
  AddBackSInt( &winver_period, year );
  AddBackCStr( &winver_period, "." );
  AddBackSInt( &winver_period, month );
  AddBackCStr( &winver_period, "." );
  AddBackSInt( &winver_period, day );
  AddBackCStr( &winver_period, "." );
  AddBackSInt( &winver_period, last_section );

  stack_resizeable_cont_t<u8> rs;
  Alloc( rs, 1024 );
  AddBackCStr( &rs, "1 VERSIONINFO" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "FILEVERSION " );
  AddBackContents( &rs, SliceFromArray( winver_comma ) );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "PRODUCTVERSION " );
  AddBackContents( &rs, SliceFromArray( winver_comma ) );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "FILEFLAGSMASK 0x3F" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "FILEFLAGS 0" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "FILEOS 4" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "FILETYPE 1" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "FILESUBTYPE 0" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "BEGIN" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "BLOCK \"StringFileInfo\"" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "BEGIN" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  // 0409 is en-US
  // 04E4 is 1252 standard codepage
  AddBackCStr( &rs, "BLOCK \"040904E4\"" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "BEGIN" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "VALUE \"CompanyName\", \"John A. Carlos Jr.\\0\"" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "VALUE \"FileDescription\", \"" );
  AddBackContents( &rs, *name );
  AddBackCStr( &rs, "\\0\"" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "VALUE \"FileVersion\", \"" );
  AddBackContents( &rs, SliceFromArray( winver_period ) );
  AddBackCStr( &rs, "\\0\"" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "VALUE \"InternalName\", \"" );
  AddBackContents( &rs, *name );
  AddBackCStr( &rs, "\\0\"" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "VALUE \"LegalCopyright\", \"(c) John A. Carlos Jr., all rights reserved.\\0\"" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "VALUE \"OriginalFilename\", \"" );
  AddBackContents( &rs, *name );
  AddBackCStr( &rs, ".exe\\0\"" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "VALUE \"ProductName\", \"" );
  AddBackContents( &rs, *name );
  AddBackCStr( &rs, "\\0\"" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "VALUE \"ProductVersion\", \"" );
  AddBackContents( &rs, SliceFromArray( winver_period ) );
  AddBackCStr( &rs, "\\0\"" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "END" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "END" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "BLOCK \"VarFileInfo\"" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "BEGIN" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "VALUE \"Translation\", 0x0409, 0x04E4" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "\t" );
  AddBackCStr( &rs, "END" );
  AddBackCStr( &rs, "\r\n" );
  AddBackCStr( &rs, "END" );
  AddBackCStr( &rs, "\r\n" );

  // TODO: go back to resource.rc, it's simpler. maybe call it version.rc

//  auto rs_filename = SliceFromCStr( "exe/resource.rc" );
//  auto res_filename = SliceFromCStr( "exe/resource.res" );
  slice_t rs_filename = {};
  slice_t res_filename = {};
  switch( plat ) {
    case target_plat_t::x64: {

      switch( flavor ) {
        case target_flavor_t::debug: {
          rs_filename = SliceFromCStr( "exe/proj64d.rc" );
          res_filename = SliceFromCStr( "exe/proj64d.res" );
        } break;
        case target_flavor_t::optimized:
        case target_flavor_t::releaseversion: {
          rs_filename = SliceFromCStr( "exe/proj64s.rc" );
          res_filename = SliceFromCStr( "exe/proj64s.res" );
        } break;
        default: UnreachableCrash();
      }
    } break;
    case target_plat_t::x86: {

      switch( flavor ) {
        case target_flavor_t::debug: {
          rs_filename = SliceFromCStr( "exe/proj32d.rc" );
          res_filename = SliceFromCStr( "exe/proj32d.res" );
        } break;
        case target_flavor_t::optimized:
        case target_flavor_t::releaseversion: {
          rs_filename = SliceFromCStr( "exe/proj32s.rc" );
          res_filename = SliceFromCStr( "exe/proj32s.res" );
        } break;
        default: UnreachableCrash();
      }
    } break;
    default: UnreachableCrash();
  }

  file_t rs_file = FileOpen( ML( rs_filename ), fileopen_t::always, fileop_t::W, fileop_t::R );
  if( !rs_file.loaded ) {
    printf( "Failed to write resource file: %s\n", rs_filename.mem );
    return 1;
  }
  FileWrite( rs_file, 0, ML( rs ) );
  FileSetEOF( rs_file, rs.len );
  FileFree( rs_file );

  stack_resizeable_cont_t<u8> resource_compile;
  Alloc( resource_compile, 64000 );
  AddBackCStr( &resource_compile, "\"" );
  AddBackCStr( &resource_compile, rc_exe );
  AddBackCStr( &resource_compile, "\"" );
  AddBackCStr( &resource_compile, " " );
  AddBackCStr( &resource_compile, rs_filename.mem );

#if PRINT_TOOL_COMMANDS
  *AddBack( resource_compile ) = 0;
  printf( "%s\n", resource_compile.mem );
  RemBack( resource_compile );
#endif

  s32 resource_compile_result = Execute(
    SliceFromArray( resource_compile ),
    0,
    OutputForToolExecute,
    0,
    0
    );

  if( resource_compile_result ) {
    printf( "Resource compilation failed: %d\n", resource_compile_result );
    return 1;
  }

  if( !FileExists( ML( res_filename ) ) ) {
    printf( "Resource output file not produced: %s\n", res_filename.mem );
    return 1;
  }

  Free( resource_compile );
  Free( winver_comma );
  Free( winver_period );
  Free( rs );

  printf( "[Resource] %.3f seconds\n", TimeSecFromTSC32( TimeTSC() - t0 ) );

  // =========================================
  //                 COMPILE

  const void* compiler_flags_shared[] = {
    "/c",
    "/arch:AVX2",
    "/bigobj",
    "/DWIN", // our custom switch to denote windows builds.
    "/D_USING_V110_SDK71_",
    "/D_MBCS",
    "/errorReport:none",
    "/FAs",
    "/Faexe\\",
    "/Foexe\\",
    "/Fdexe\\",
    "/FC",
    "/fp:precise",
    "/Gm-",
    "/GR-",
    "/GS",
    "/guard:cf",
    "/Isrc",
    "/J",
    "/Oi",
    "/nologo",
    "/permissive-",
    "/Qfast_transcendentals",
    "/sdl",
    "/std:c++20",
    "/W4",
    "/WX-",
    "/wd4100", // warning C4100: 'foo': unreferenced formal parameter
    "/wd4127", // warning C4127: conditional expression is constant
    "/wd4189", // warning C4189: 'foo': local variable is initialized but not referenced
    "/wd4201", // warning C4201: nonstandard extension used: nameless struct/union
    "/Zc:inline",
    "/Zc:wchar_t",
    "/Zc:strictStrings-",
    "/Z7",
//    "/Zi",
//    "/d1reportTime",
    };

  const void* compiler_flags_x86[] = {
    "/Oy-",
    };

  const void* compiler_flags_debug[] = {
    "/DDBG=1",
    "/MTd",
    "/Od",
//    "/Ob1", // win back some inlining, since RTC and CFG cause really slow fn pro/epi-logues.
    "/RTCs",
    };

  const void* compiler_flags_optimized_and_releaseversion[] = {
    "/DDBG=0",
    "/GL",
    "/Gw",
    "/Gy",
    "/Ob2",
    "/Og",
    "/Ot",
    "/MT",
    };

  const void* compiler_flags_releaseversion[] = {
    define_version.mem,
    };

  #define ADDCOMPILERFLAGS( _flags ) \
    ForEach( flag, _flags ) { \
      AddBackCStr( &compile, flag ); \
      AddBackCStr( &compile, " " ); \
    } \

  ADDCOMPILERFLAGS( compiler_flags_shared );

  switch( plat ) {
    case target_plat_t::x64: {
    } break;
    case target_plat_t::x86: {
      ADDCOMPILERFLAGS( compiler_flags_x86 );
    } break;
    default: UnreachableCrash();
  }

  switch( flavor ) {
    case target_flavor_t::debug: {
      ADDCOMPILERFLAGS( compiler_flags_debug );
    } break;
    case target_flavor_t::optimized: {
      ADDCOMPILERFLAGS( compiler_flags_optimized_and_releaseversion );
    } break;
    case target_flavor_t::releaseversion: {
      ADDCOMPILERFLAGS( compiler_flags_optimized_and_releaseversion );
      ADDCOMPILERFLAGS( compiler_flags_releaseversion );
    } break;
    default: UnreachableCrash();
  }

  Memmove( AddBack( compile, filename_cpp.len ), ML( filename_cpp ) );

#if PRINT_TOOL_COMMANDS
  *AddBack( compile ) = 0;
  printf( "%s\n", compile.mem );
  RemBack( compile );
#endif

  s32 compile_result = Execute(
    SliceFromArray( compile ),
    0,
    OutputForToolExecute,
    0,
    0
    );

  if( compile_result ) {
    printf( "Compilation failed: %d\n", compile_result );
    return 1;
  }

  Free( compile );

  printf( "[Compile] %.3f seconds\n", TimeSecFromTSC32( TimeTSC() - t0 ) );


  // =========================================
  //                  LINK

  const void* linker_flags_shared[] = {
    "/CGTHREADS:8",
    "/DYNAMICBASE",
    "/ERRORREPORT:NONE",
    "/GUARD:CF",
    "/INCREMENTAL:NO",
    "/MANIFEST:NO",
    "/NOLOGO",
    "/NXCOMPAT",
    "/DEBUG:FULL",
    };

//  void* linker_flags_debug[] = {
//    };

  const void* linker_flags_optimized_and_releaseversion[] = {
    "/LTCG",
    "/OPT:REF,ICF=4",
    "/MAP",
    "/MAPINFO:EXPORTS",
    };

  const void* linker_flags_debug_x64[] = {
    "/OUT:\"exe/proj64d.exe\"",
    "/PDB:\"exe/proj64d.pdb\"",
    };

  const void* linker_flags_debug_x86[] = {
    "/OUT:\"exe/proj32d.exe\"",
    "/PDB:\"exe/proj32d.pdb\"",
    };

  const void* linker_flags_optimized_and_releaseversion_x64[] = {
    "/OUT:\"exe/proj64s.exe\"",
    "/PDB:\"exe/proj64s.pdb\"",
    };

  const void* linker_flags_optimized_and_releaseversion_x86[] = {
    "/OUT:\"exe/proj32s.exe\"",
    "/PDB:\"exe/proj32s.pdb\"",
    };

  const void* linker_flags_x64[] = {
    "/MACHINE:X64",
    "/HIGHENTROPYVA",
    };

  const void* linker_flags_x86[] = {
    "/MACHINE:X86",
    "/LARGEADDRESSAWARE",
    "/SAFESEH",
    };

  const void* linker_flags_window_x64[] = {
    "/SUBSYSTEM:WINDOWS\",5.02\"",
    };

  const void* linker_flags_console_x64[] = {
    "/SUBSYSTEM:CONSOLE\",5.02\"",
    };

  const void* linker_flags_window_x86[] = {
    "/SUBSYSTEM:WINDOWS\",5.01\"",
    };

  const void* linker_flags_console_x86[] = {
    "/SUBSYSTEM:CONSOLE\",5.01\"",
    };

  #define ADDLINKERFLAGS( _flags ) \
    ForEach( flag, _flags ) { \
      AddBackCStr( &link, flag ); \
      AddBackCStr( &link, " " ); \
    } \

  ADDLINKERFLAGS( linker_flags_shared );

  switch( flavor ) {
    case target_flavor_t::debug: {
//      ADDLINKERFLAGS( linker_flags_debug );
    } break;
    case target_flavor_t::optimized:
    case target_flavor_t::releaseversion: {
      ADDLINKERFLAGS( linker_flags_optimized_and_releaseversion );
    } break;
    default: UnreachableCrash();
  }

  switch( plat ) {
    case target_plat_t::x64: {
      ADDLINKERFLAGS( linker_flags_x64 );

      switch( flavor ) {
        case target_flavor_t::debug: {
          ADDLINKERFLAGS( linker_flags_debug_x64 );
        } break;
        case target_flavor_t::optimized:
        case target_flavor_t::releaseversion: {
          ADDLINKERFLAGS( linker_flags_optimized_and_releaseversion_x64 );
        } break;
        default: UnreachableCrash();
      }

      switch( type ) {
        case target_type_t::window: ADDLINKERFLAGS( linker_flags_window_x64 ); break;
        case target_type_t::console: ADDLINKERFLAGS( linker_flags_console_x64 ); break;
        default: UnreachableCrash();
      }
    } break;
    case target_plat_t::x86: {
      ADDLINKERFLAGS( linker_flags_x86 );

      switch( flavor ) {
        case target_flavor_t::debug: {
          ADDLINKERFLAGS( linker_flags_debug_x86 );
        } break;
        case target_flavor_t::optimized:
        case target_flavor_t::releaseversion: {
          ADDLINKERFLAGS( linker_flags_optimized_and_releaseversion_x86 );
        } break;
        default: UnreachableCrash();
      }

      switch( type ) {
        case target_type_t::window: ADDLINKERFLAGS( linker_flags_window_x86 ); break;
        case target_type_t::console: ADDLINKERFLAGS( linker_flags_console_x86 ); break;
        default: UnreachableCrash();
      }
    } break;
    default: UnreachableCrash();
  }

  Memmove( AddBack( link, filename_obj.len ), ML( filename_obj ) );
  AddBackCStr( &link, " " );
  AddBackContents( &link, res_filename );

#if PRINT_TOOL_COMMANDS
  *AddBack( link ) = 0;
  printf( "%s\n", link.mem );
  RemBack( link );
#endif

  s32 link_result = Execute(
    SliceFromArray( link ),
    0,
    OutputForToolExecute,
    0,
    0
    );

  if( link_result ) {
    printf( "Linking failed: %d\n", link_result );
    return 1;
  }

  Free( link );

  printf( "[Link] %.3f seconds\n", TimeSecFromTSC32( TimeTSC() - t0 ) );


  // =========================================
  //       COPY APP.CONFIG TO EXE/

  auto source_filename = SliceFromCStr( "app.config" );
  slice_t target_filename = {};
  switch( plat ) {
    case target_plat_t::x64: {

      switch( flavor ) {
        case target_flavor_t::debug: {
          target_filename = SliceFromCStr( "exe/proj64d.config" );
        } break;
        case target_flavor_t::optimized:
        case target_flavor_t::releaseversion: {
          target_filename = SliceFromCStr( "exe/proj64s.config" );
        } break;
        default: UnreachableCrash();
      }
    } break;
    case target_plat_t::x86: {

      switch( flavor ) {
        case target_flavor_t::debug: {
          target_filename = SliceFromCStr( "exe/proj32d.config" );
        } break;
        case target_flavor_t::optimized:
        case target_flavor_t::releaseversion: {
          target_filename = SliceFromCStr( "exe/proj32s.config" );
        } break;
        default: UnreachableCrash();
      }
    } break;
    default: UnreachableCrash();
  }
  auto copied = FileCopyOverwrite( ML(  target_filename ), ML( source_filename ) );
  if( !copied ) {
    printf( "Failed to copy file: %s -> %s\n", source_filename.mem, target_filename.mem );
    return 1;
  }

  printf( "[Config] %.3f seconds\n", TimeSecFromTSC32( TimeTSC() - t0 ) );


  // =========================================
  //  COPY .NATVIS FILES TO DEBUGGER LOCATION

  fsobj_t natvis_dst = {};
  AddBackContents( &natvis_dst, SliceFromArray( vs_install_dir ) );
  AddBackCStr( &natvis_dst, "common7/packages/debugger/visualizers/natvisfile.natvis" );
  auto natvis_dst_exists = FileExists( ML( natvis_dst ) );
  if( natvis_dst_exists ) {
    auto deleted = FileDelete( ML( natvis_dst ) );
    if( !deleted ) {
      printf( "Failed to delete file: %s\n", natvis_dst.mem );
      return 1;
    }
  }

  stack_resizeable_cont_t<u8> natvis;
  Alloc( natvis, 32000 );
  AddBackCStr( &natvis, "cmd.exe /c mklink" );
  AddBackCStr( &natvis, " " );
  AddBackCStr( &natvis, "\"" );
  AddBackContents( &natvis, SliceFromArray( natvis_dst ) );
  AddBackCStr( &natvis, "\"" );
  AddBackCStr( &natvis, " " );
  // TODO: better logic here to find the full path of the natvis file or files.
  AddBackCStr( &natvis, "\"" );
  AddBackCStr( &natvis, "C:/doc/git/monorepo/natvisfile.natvis" );
  AddBackCStr( &natvis, "\"" );

#if PRINT_TOOL_COMMANDS
  *AddBack( natvis ) = 0;
  printf( "%s\n", natvis.mem );
  RemBack( natvis );
#endif

  s32 natvis_result = Execute(
    SliceFromArray( natvis ),
    0,
//    OutputForToolExecute,
    EmptyOutputForExecute,
    0,
    0
    );

  if( natvis_result ) {
    printf( "Failed to link .natvis file: %d\n", natvis_result );
    return 1;
  }

  Free( natvis );

  printf( "[Natvis] %.3f seconds\n", TimeSecFromTSC32( TimeTSC() - t0 ) );


  // =========================================
  //       BUILD A RELEASE PACKAGE

  switch( flavor ) {
    case target_flavor_t::debug:
    case target_flavor_t::optimized: {
    } break;
    case target_flavor_t::releaseversion: {
      stack_resizeable_cont_t<u8> zip;
      Alloc( zip, 32000 );
      AddBackCStr( &zip, "\"" );
      AddBackCStr( &zip, "C:/Program Files/7-Zip/7z.exe" );
      AddBackCStr( &zip, "\"" );
      AddBackCStr( &zip, " a " );
      AddBackCStr( &zip, "github/release" );
      AddBackContents( &zip, SliceFromArray( now ) );
      AddBackCStr( &zip, ".zip" );
      AddBackCStr( &zip, " " );
      switch( plat ) {
        case target_plat_t::x64: AddBackCStr( &zip, "exe/proj64s.*" ); break;
        case target_plat_t::x86: AddBackCStr( &zip, "exe/proj32s.*" ); break;
        default: UnreachableCrash();
      }
      AddBackCStr( &zip, " " );
      AddBackCStr( &zip, "src/*" );

#if PRINT_TOOL_COMMANDS
      *AddBack( zip ) = 0;
      printf( "%s\n", zip.mem );
      RemBack( zip );
#endif

      s32 zip_result = Execute(
        SliceFromArray( zip ),
        0,
        OutputForToolExecute,
        0,
        0
        );

      if( zip_result ) {
        printf( "Zipping failed: %d\n", zip_result );
        return 1;
      }

      Free( zip );

      printf( "[Zipping] %.3f seconds\n", TimeSecFromTSC32( TimeTSC() - t0 ) );

    } break;
    default: UnreachableCrash();
  }

  printf( "[Build finished] %.3f seconds\n", TimeSecFromTSC32( TimeTSC() - t0 ) );

  return 0;
}

Inl void
IgnoreSurroundingSpaces( u8*& a, idx_t& a_len )
{
  while( a_len  &&  AsciiIsWhitespace( a[0] ) ) {
    a += 1;
    a_len -= 1;
  }
  // TODO: prog_cmd_line actually has two 0-terms!
  while( a_len  &&  ( !a[a_len - 1]  ||  AsciiIsWhitespace( a[a_len - 1] ) ) ) {
    a_len -= 1;
  }
}

int
main( int argc, char** argv )
{
  MainInit();

  stack_resizeable_cont_t<slice_t> args;
  Alloc( args, 512 );
  Fori( int, i, 1, argc ) {
    auto arg = AddBack( args );
    arg->mem = Cast( u8*, argv[i] );
    arg->len = CstrLength( arg->mem );
  }
  int r = Main( args );
  Free( args );

//  system( "pause" );
  MainKill();
  return r;
}

#endif // WIN
