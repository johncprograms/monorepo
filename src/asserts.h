// Copyright (c) John A. Carlos Jr., all rights reserved.

// ASSERT MACROS

#ifdef _DEBUG

  #define AssertWarn( break_if_false )   \
    do{ \
      if( !( break_if_false ) ) {  \
        __debugbreak(); \
      } \
    } while( 0 ) \

  #define AssertCrash( break_if_false )   \
    do{ \
      if( !( break_if_false ) ) {  \
        __debugbreak(); \
      } \
    } while( 0 ) \

#else

  NoInl void
  _WarningTriggered( const char* break_if_false, const char* file, u32 line, const char* function );

  NoInl void
  _CrashTriggered( const char* break_if_false, const char* file, u32 line, const char* function );

  #define AssertWarn( break_if_false )   \
    { \
      if( !( break_if_false ) ) {  \
        _WarningTriggered( #break_if_false, __FILE__, __LINE__, __FUNCTION__ ); \
      } \
    } \

  #define AssertCrash( break_if_false )   \
    { \
      if( !( break_if_false ) ) {  \
        _CrashTriggered( #break_if_false, __FILE__, __LINE__, __FUNCTION__ ); \
      } \
    } \

#endif

#define ImplementWarn()    AssertWarn( !"Implement!" );
#define ImplementCrash()   AssertCrash( !"Implement!" );

#define UnreachableWarn()    AssertWarn( !"Unreachable" );
#define UnreachableCrash()   AssertCrash( !"Unreachable" );
