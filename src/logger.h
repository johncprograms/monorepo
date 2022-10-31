// Copyright (c) John A. Carlos Jr., all rights reserved.

#ifndef LOGGER_ENABLED
#error make a choice for LOGGER_ENABLED; either 0 or 1.
#endif


// TODO: high-intensity logger perf is pretty terrible, mostly because of tiny little file writes.
//   probably get rid of the printf format functions, esp. the LogInline one.
//   force callers to construct bigger chunks of what they want, and then pass a simple slice here.
//   hopefully large slices.



#if !LOGGER_ENABLED

Inl void
LoggerInit()
{
}

Inl void
LoggerKill()
{
}

Inl void
LogAddIndent( s32 delta )
{
}

Inl void
LogUseConsole( bool x )
{
}

void
Log( void* cstr ... )
{
}

void
LogInline( void* cstr ... )
{
}

#else // LOGGER_ENABLED


struct
log_t
{
  stack_nonresizeable_stack_t<u8, 65536> buffer;
  file_t file;
  s32 indent;
  bool initialized;
  bool console_override;
};


static log_t g_log = {};

// TODO: convert to lock_t/Lock/Unlock
volatile static u64 g_log_inuse = 0;

Inl log_t*
AcquireLog()
{
  AssertCrash( g_log.initialized ); // Using the logger before it's initialized by the main thread!
  while( !CAS( &g_log_inuse, 0, 1 ) )
  {
    _mm_pause();
  }
  auto log = &g_log;
  AssertCrash( log->initialized ); // Using the logger before it's initialized by the main thread!
  return log;
}

Inl void
ReleaseLog( log_t*& log )
{
  AssertCrash( log->initialized ); // Using the logger before it's initialized by the main thread!
  _ReadWriteBarrier();
  g_log_inuse = 0;
  log = 0;
}

Inl void
LoggerInit()
{
  AssertCrash( CAS( &g_log_inuse, 0, 1 ) ); // Some other thread is using the logger before it's initialized!
  auto log = &g_log;

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
    Memmove( AddBack( filename, 4 ), ".log", 4 );

    log->file = FileOpen( ML( filename ), fileopen_t::always, fileop_t::W, fileop_t::RW );

    if( log->file.loaded ) {
      FileSetEOF( log->file, 0 );
    } else {
      printf( "WARNING: failed to open log file!\n" );
      printf( "\tRedirecting log output to stdout...\n" );
    }
  } else {
    printf( "WARNING: failed to open log file!\n" );
    printf( "\tRedirecting log output to stdout...\n" );
  }

  log->initialized = 1;
  log->console_override = 0;

  _ReadWriteBarrier();
  g_log_inuse = 0;
}

Inl void
LoggerKill()
{
  AssertCrash( CAS( &g_log_inuse, 0, 1 ) ); // Some other thread is using the logger while we're destroying it!
  auto log = &g_log;

  FileFree( log->file );
  log->initialized = 0;

  _ReadWriteBarrier();
  g_log_inuse = 0;
}


Inl void
LogAddIndent( s32 delta )
{
  auto log = AcquireLog();
  log->indent += delta;
  ReleaseLog( log );
}

Inl void
LogUseConsole( bool x )
{
  auto log = AcquireLog();
  log->console_override = x;
  ReleaseLog( log );
}


void
LogDirect( slice_t slice )
{
  auto log = AcquireLog();

  if( log->file.loaded  &&  !log->console_override ) {
    FileWriteAppend( log->file, ML( slice ) );
    FileSetEOF( log->file );
  }
  else {
    // TODO: console output?
  }

  ReleaseLog( log );
}

void
_Log( void* cstr, va_list& args )
{
  auto log = AcquireLog();

  if( log->file.loaded  &&  !log->console_override ) {
    For( i, 0, log->indent ) {
      Memmove( AddBack( log->buffer ), "\t", 1 );
    }
    log->buffer.len += vsprintf_s(
      Cast( char* const, log->buffer.mem + log->buffer.len ),
      MAX( Capacity( log->buffer ), log->buffer.len ) - log->buffer.len,
      Cast( const char* const, cstr ),
      args
      );
    Memmove( AddBack( log->buffer ), "\r\n", 2 );
    FileWriteAppend( log->file, ML( log->buffer ) );
    FileSetEOF( log->file );
    log->buffer.len = 0;
  }
  else {
    For( i, 0, log->indent ) {
      printf( "\t" );
    }
    vprintf( Cast( const char* const, cstr ), args );
    printf( "\r\n" );
  }

  ReleaseLog( log );
}

void
_LogInline( void* cstr, va_list& args )
{
  auto log = AcquireLog();

  if( log->file.loaded  &&  !log->console_override ) {
    log->buffer.len += vsprintf_s(
      Cast( char* const, log->buffer.mem + log->buffer.len ),
      MAX( Capacity( log->buffer ), log->buffer.len ) - log->buffer.len,
      Cast( const char* const, cstr ),
      args
      );
    FileWriteAppend( log->file, ML( log->buffer ) );
    FileSetEOF( log->file );
    log->buffer.len = 0;
  } else {
    vprintf( Cast( const char* const, cstr ), args );
  }

  ReleaseLog( log );
}

// TODO: deprecate this, or rename to LogCriticalAndFlushToFile
void
Log( void* cstr ... )
{
  va_list args;
  va_start( args, cstr );
  _Log( cstr, args );
  va_end( args );
}

// TODO: deprecate this, or rename to LogCriticalAndFlushToFile
void
LogInline( void* cstr ... )
{
  va_list args;
  va_start( args, cstr );
  _LogInline( cstr, args );
  va_end( args );
}

#endif // LOGGER_ENABLED
