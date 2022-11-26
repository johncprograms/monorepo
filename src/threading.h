// Copyright (c) John A. Carlos Jr., all rights reserved.


#if _SIZEOF_IDX_T == 4

  #if defined(WIN)
    #define GetThreadIdFast() \
      ( *Cast( u32*, Cast( u8*, __readfsdword( 0x18 ) ) + 0x24 ) )
  #elif defined(MAC)
    #define GetThreadIdFast()   (0)
  #else
    #error Unsupported platform
  #endif

#elif _SIZEOF_IDX_T == 8

  #if defined(WIN)
    #define GetThreadIdFast() \
      ( *Cast( u32*, Cast( u8*, __readgsqword( 0x30 ) ) + 0x48 ) )
  #elif defined(MAC)
    #define GetThreadIdFast()   (0)
  #else
    #error Unsupported platform
  #endif

#else
  #error Unexpected _SIZEOF_IDX_T value!
#endif




// TODO: this is a dumb way of doing TLS.
#if BADTLS
static volatile s32 g_tls_handle = 0;

struct
tls_t
{
  pagelist_t temp;
};

void
Init( tls_t* tls )
{
  Init( tls->temp, 32768 );
}

void
Kill( tls_t* tls )
{
  Kill( tls->temp );
}
#endif

void
ThreadInit()
{
#if BADTLS
  AssertWarn( g_tls_handle != TLS_OUT_OF_INDEXES );
  auto tls = MemHeapAlloc( tls_t, 1 );
  Init( tls );
  AssertWarn( TlsSetValue( g_tls_handle, tls ) );
#endif
}

void
ThreadKill()
{
#if BADTLS
  AssertWarn( g_tls_handle != TLS_OUT_OF_INDEXES );
  auto tls = Cast( tls_t*, TlsGetValue( g_tls_handle ) );
  AssertWarn( tls );
  Kill( tls );
  MemHeapFree( tls );
#endif
}

#if BADTLS
Inl tls_t*
GetTls()
{
  auto tls = Cast( tls_t*, TlsGetValue( g_tls_handle ) );
  AssertWarn( tls );
  return tls;
}
#endif


#define __ExecuteOutput( fnname )   \
  void ( fnname )( \
    slice_t* message, \
    bool internal_failure, \
    void* misc0, \
    void* misc1 \
    )

typedef __ExecuteOutput( *pfn_executeoutput_t );

__ExecuteOutput( EmptyOutputForExecute )
{
}

s32
Execute(
  slice_t command,
  bool show_window,
  pfn_executeoutput_t ExecuteOutput,
  void* misc0,
  void* misc1
  )
{
#if defined(WIN)
  HANDLE child_stdout_r = INVALID_HANDLE_VALUE;
  HANDLE child_stdout_w = INVALID_HANDLE_VALUE;
  HANDLE child_stdin_r = INVALID_HANDLE_VALUE;
  HANDLE child_stdin_w = INVALID_HANDLE_VALUE;

  SECURITY_ATTRIBUTES security;
  security.nLength = sizeof( SECURITY_ATTRIBUTES );
  security.bInheritHandle = TRUE;
  security.lpSecurityDescriptor = 0;

  BOOL res;
  res = CreatePipe( &child_stdout_r, &child_stdout_w, &security, 0 );
  if( !res ) {
    auto err = GetLastError();
    auto str = SliceFromCStr( "failed to make the stdout pipe!\r\n" );
    ExecuteOutput( &str, 1, misc0, misc1 );
    return err;
  }
  res = SetHandleInformation( child_stdout_r, HANDLE_FLAG_INHERIT, 0 );
  if( !res ) {
    auto err = GetLastError();
    auto str = SliceFromCStr( "failed to disable inheritance on the stdout pipe's read handle!\r\n" );
    ExecuteOutput( &str, 1, misc0, misc1 );
    CloseHandle( child_stdout_r );
    CloseHandle( child_stdout_w );
    return err;
  }
  res = CreatePipe( &child_stdin_r, &child_stdin_w, &security, 0 );
  if( !res ) {
    auto err = GetLastError();
    auto str = SliceFromCStr( "failed to make the stdin pipe!\r\n" );
    ExecuteOutput( &str, 1, misc0, misc1 );
    CloseHandle( child_stdout_r );
    CloseHandle( child_stdout_w );
    return err;
  }
  res = SetHandleInformation( child_stdin_w, HANDLE_FLAG_INHERIT, 0 );
  if( !res ) {
    auto err = GetLastError();
    auto str = SliceFromCStr( "failed to disable inheritance on the stdin pipe's read handle!\r\n" );
    ExecuteOutput( &str, 1, misc0, misc1 );
    CloseHandle( child_stdout_r );
    CloseHandle( child_stdout_w );
    CloseHandle( child_stdin_r );
    CloseHandle( child_stdin_w );
    return err;
  }

  stack_nonresizeable_stack_t<u8, 32767> com;
  com.len = 0;
  if( command.len + 1 >= Capacity( com ) ) {
    auto str = SliceFromCStr( "input command is too long!\r\n" );
    ExecuteOutput( &str, 1, misc0, misc1 );
    CloseHandle( child_stdout_r );
    CloseHandle( child_stdout_w );
    CloseHandle( child_stdin_r );
    CloseHandle( child_stdin_w );
    return -1;
  }
  Memmove( AddBack( com, command.len ), ML( command ) );
  *AddBack( com ) = 0;

  PROCESS_INFORMATION process = { 0 };
  STARTUPINFO startup = { 0 };
  startup.cb = sizeof( STARTUPINFO );
  startup.hStdError = child_stdout_w;
  startup.hStdOutput = child_stdout_w;
  startup.hStdInput = child_stdin_r; // GetStdHandle( STD_INPUT_HANDLE );
  startup.wShowWindow = show_window  ?  SW_SHOW  :  SW_HIDE;
  startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

  res = CreateProcessA(
    0,
    Cast( char*, com.mem ),
    0,
    0,
    TRUE,
    0,
    0,
    0,
    &startup,
    &process
    );

  if( !res ) {
    auto err = GetLastError();
    auto str = SliceFromCStr( "failed to create a process with the given command!\r\n" );
    ExecuteOutput( &str, 1, misc0, misc1 );
    CloseHandle( child_stdout_r );
    CloseHandle( child_stdout_w );
    CloseHandle( child_stdin_r );
    CloseHandle( child_stdin_w );
    return err;
  }

  // close the parent's references to the child-side of the pipe.
  CloseHandle( child_stdout_w );
  CloseHandle( child_stdin_r );

  // close the parent's reference to this, since we're not writing anything to the child's stdin.
  // this will free up the child process to stop waiting for input.
  CloseHandle( child_stdin_w );

  Forever {
    u8 pipebuf[4096];
    DWORD nread;
    BOOL read_res = ReadFile( child_stdout_r, pipebuf, _countof( pipebuf ) - 1, &nread, 0 );
    if( !read_res || !nread ) {
      break;
    }
    AssertCrash( nread > 0 );
    slice_t str;
    str.mem = pipebuf;
    str.len = Cast( idx_t, nread );
    ExecuteOutput( &str, 0, misc0, misc1 );
  }

  // we're done reading from the child_stdout pipe, so close it.
  CloseHandle( child_stdout_r );

  // now all the pipe handles are closed.

  // get the process's exit code.
  DWORD exit = 0;
  res = GetExitCodeProcess( process.hProcess, &exit );
  if( !res ) {
    auto err = GetLastError();
    auto str = SliceFromCStr( "failed to get process exit code!\r\n" );
    ExecuteOutput( &str, 1, misc0, misc1 );
    exit = err;
  }
  elif( exit == STILL_ACTIVE ) {
    auto str = SliceFromCStr( "process still open after waiting for exit!\r\n" );
    ExecuteOutput( &str, 1, misc0, misc1 );
    exit = Cast( DWORD, -1 );
  }

  // clean up the process
  CloseHandle( process.hProcess );
  CloseHandle( process.hThread );
  return exit;
#elif defined(MAC)
  return 0;
#else
#error Unsupported platform
#endif
}


Inl void
PinThreadToOneCore()
{
#if defined(WIN)
  DWORD_PTR process_mask, system_mask;
  CompileAssert( sizeof( DWORD_PTR ) == sizeof( sidx_t ) );
  AssertWarn( GetProcessAffinityMask( GetCurrentProcess(), &process_mask, &system_mask ) );
  AssertWarn( process_mask );
  bool found = 0;
  ReverseFor( i, 0, 64 ) {
    if( ( process_mask >> i ) & 1ULL ) {
      if( !found ) {
        found = 1;
      } else {
        process_mask &= ~( 1ULL << i );
      }
    }
  }
  AssertWarn( process_mask );
  AssertWarn( !( process_mask & ( process_mask - 1 ) ) ); // pow of 2

  u64 prev_thread_mask = SetThreadAffinityMask( GetCurrentThread(), process_mask );
  AssertWarn( prev_thread_mask );
#elif defined(MAC)
#else
#error Unsupported platform
#endif
}


#if defined(WIN)
#define STDCALLWIN __stdcall
#else
#define STDCALLWIN
#endif

// Can't do this the normal way, since function pointer syntax is stupid.
typedef u32 ( STDCALLWIN *pfn_threadproc_t )( void* misc );



//
// TODO: we don't currently handle full queues very well.
// basically, the threads will spinlock until the queues have room. not ideal.
//
// i can see two different futures for this code, to solve this problem:
//
// 1) make the queues have effectively infinite size.
//      a single fixed-size ringbuffer obviously can't do this.
//      but, a linked-list of these ringbuffers could.
//      we'd have to write the transactional code to add a new linked-list entry if the previous was full.
//      we'd probably also want a toplevel 'which ll-entry is currently active' pointer, to avoid having
//        to walk the entire ll every time we want to add something to a pretty-full queue.
//      so that's fancy lock-free logic for at least two pointers, in addition to the usual ringbuffer stuff.
//      all in all, sounds complicated, but reasonable.
//
// 2) add more waiting/waking logic to handle these situations.
//      basically, put the queueing thread to sleep if it can't queue a task.
//      and then wake it back up if the other thread starts emptying the queue.
//      we've got to decide if there's an asymmetry here with main <-> task threads.
//      we probably don't want to sleep the main thread, unless _every_ task thread has a full input queue,
//        because we can just schedule the task onto another task thread.
//      but in that situation, we'd want to wake up the main thread as soon as possible.
//      which would be right as the first of any task thread dequeues any input.
//      but, we don't want to wake the main thread every time a task thread dequeues, just this one time.
//      so we'd need some kind of state transition tracker, to tell us when to wake the main thread.
//      we could do that with a volatile counter, and check against maximum value, given that the
//        task thread count is fixed, and so are the queue sizes.
//      that adds a fixed overhead to every enqueue/dequeue, which isn't ideal. maybe it's not that bad.
//      i don't think you can do this with volatile bools, since task threads might dequeue their inputs
//        while the main thread is getting put to sleep.
//      i guess we could add more calls to wake the main thread if we do something like that, but that feels bad.
//
// this all sums up as: don't fill up the queues. it's not a good idea.
// we'll have to do a lot of work if we start hitting full queues a lot.
//
// i want to implement a timeline view in the profiler before going down any of these roads.
//
// note that each task thread has inputs.len possible inputs,
// while the main thread has num_task_threads * inputs.len possible inputs.
// what that means is, the __MainTaskCompleted functions have to take a factor of 1/num_task_threads less time
// than the __AsyncTask functions.
//
// in summary, keep the __MainTaskCompleted functions as tiny as possible.
//


//
// it's convenient to have multiple misc slots, so we don't have to define custom structs just to pass around data.
// the usual pattern is to pass the async thread context in one slot, and the main thread context in the other.
//

struct
taskthread_t;

#define __AsyncTask( name )    \
  void ( name )( \
    taskthread_t* taskthread, \
    void* misc0, \
    void* misc1 \
    ) \

typedef __AsyncTask( *pfn_asynctask_t );

#define __MainTaskCompleted( name )    \
  void ( name )( \
    bool* target_valid, \
    void* misc0, \
    void* misc1, \
    void* misc2 \
    ) \

typedef __MainTaskCompleted( *pfn_maintaskcompleted_t );

struct
asyncqueue_entry_t
{
  pfn_asynctask_t FnAsyncTask;
  void* misc0;
  void* misc1;
  u64 time_generated;
};

struct
maincompletedqueue_entry_t
{
  pfn_maintaskcompleted_t FnMainTaskCompleted;
  void* misc0;
  void* misc1;
  void* misc2;
  u64 time_generated;
};



#define PULLMODEL   1

#if PULLMODEL
#else
#endif



struct
taskthread_t
{
#if defined(WIN)
  HANDLE wake;
#elif defined(MAC)
#else
#error Unsupported platform
#endif

#if PULLMODEL
#else
  mtqueue_srsw_t<asyncqueue_entry_t> input;
#endif
  mtqueue_srsw_t<maincompletedqueue_entry_t> output;

  u8 cache_line_padding_to_avoid_thrashing[64]; // last thing, since this type is packed into an stack_resizeable_cont_t
};

Inl void
Kill( taskthread_t& t )
{
#if PULLMODEL
#else
  Free( t.input );
#endif
  Free( t.output );
}



struct
mainthread_t
{
#if defined(WIN)
  volatile HANDLE wake_asynctaskscompleted;
#elif defined(MAC)
#else
#error Unsupported platform
#endif

  volatile bool signal_quit;

#if PULLMODEL
  mtqueue_mrsw_t<asyncqueue_entry_t> tasks;
#else
#endif

#if defined(WIN)
  stack_resizeable_cont_t<HANDLE> taskthread_handles; // TODO: stack_nonresizeable_t / buffer_t
#elif defined(MAC)
#else
#error Unsupported platform
#endif

  stack_resizeable_cont_t<taskthread_t> taskthreads;  // TODO: stack_nonresizeable_t / buffer_t
};

static mainthread_t g_mainthread = {};



u32 STDCALLWIN
TaskThread( void* misc )
{
  ThreadInit();

#if defined(WIN)
// Only supported on Win10+.
//  HRESULT hr = SetThreadDescription( GetCurrentThread(), L"TaskThread" );
//  AssertWarn( SUCCEEDED( hr ) );

  auto taskthread = Cast( taskthread_t*, misc );

  while( !g_mainthread.signal_quit ) {
    DWORD wait = WaitForSingleObject( taskthread->wake, INFINITE );
    if( wait == WAIT_OBJECT_0 ) {
      while( !g_mainthread.signal_quit ) {
        bool success;
        asyncqueue_entry_t ae;
#if PULLMODEL
        DequeueM( g_mainthread.tasks, &ae, &success );
#else
        DequeueS( taskthread->input, &ae, &success );
#endif
        if( !success ) {
          break;
        }

#if LOGASYNCTASKS
        // TODO: make a new prof buffer type to store these waiting times.
        auto time_waiting = TimeTSC() - ae.time_generated;
        Log( "asyncqueue_entry_t waited for: %llu", time_waiting );
#endif

        ae.FnAsyncTask( taskthread, ae.misc0, ae.misc1 );
      }
    }
    else {
      Log( "Task thread failed WaitForSingleObject with %d", wait );
    }
  }
#elif defined(MAC)
#else
#error Unsupported platform
#endif

  ThreadKill();
  return 0;
}


//
// note this is intentionally a 'construct the thing you want first, then we'll add it' convention.
// usually i avoid this, to save encoding a memory copy, but here we have to be careful.
// we're adding the thing to a mtqueue_srsw_t, so we have to do the copy under a locking mechanism.
// i'm sure i could come up with queue code that could handle this, but it's not important right now.
// something like: add another volatile idx that blocks reads from advancing over allocated but
//   unitialized entries in the queue.
// since we haven't done that, we do the not-as-optimal convention.
//
Inl void
PushAsyncTask( idx_t taskthreadidx, asyncqueue_entry_t* entry )
{
#if PULLMODEL
  // note pushing to a shared taskthread input queue means we don't have to do any choosing.
  // the first thread available to pull this task will do so.
  // that should be a much more efficient scheduling mechanism, letting the task threads feed themselves.
  // i was seeing very asymmetric scheduling with the old way, during findinfiles.
  bool success = 0;
  while( !success ) {
    EnqueueS( g_mainthread.tasks, entry, &success );
  }

  //
  // TODO: we should probably have some more-targeted solution here.
  // waking every taskthread is likely overkill, we probably only need to wake one thread per task push.
  // we're likely wasting some time unnecessarily waking threads and having them fall asleep again.
  //
  // note this would only be a problem for situations where we're pushing async tasks repeatedly over time.
  // right now i think all our async tasks are one-shot deals; push N tasks all in a row, and then do other stuff.
  //
  // it'd be helpful to know if we have any threads that aren't active, so we could wake those ones.
  // maybe we set/reset a bool either side of the Wait call in each taskthread?
  //
  // note that we kind of need to ensure at least one thread gets woken up.
  // i probably need to do more reading/testing of SetEvent/WaitFor calls to make sure we're doing this right.
  //
#if defined(WIN)
  FORLEN( t, i, g_mainthread.taskthreads )
    auto r = SetEvent( t->wake );
    AssertCrash( r );
  }
#elif defined(MAC)
#else
#error Unsupported platform
#endif

#else
  AssertCrash( g_mainthread.taskthreads.len );
  taskthreadidx = taskthreadidx % g_mainthread.taskthreads.len;
  auto t = g_mainthread.taskthreads.mem + taskthreadidx;

  // TODO: handle t-in-use by some better scheduling.
  //   probably something like:
  //   loop:
  //     first try to schedule the task to an already-running thread.
  //     then try to schedule the task on the other threads that are likely inactive.
  //   this is to try to avoid causing OS thread scheduling overheads.
  //   i'm not seeing any good way to do this with public Win32 functions, unfortunately.
  //   so we'll do something a little less optimal.
  //   note we'd have to be careful not to just fill up one thread's queue.

  bool success = 0;
  while( !success ) {
    EnqueueS( t->input, entry, &success );
  }

  auto r = SetEvent( t->wake );
  AssertCrash( r );
#endif
}

// note we push onto a per-taskthread output queue, so any spamming will be isolated to that thread.
// TODO: put some spinlock counters in this and similar loops, so we can log when we're spinning excessively.
Inl void
PushMainTaskCompleted( taskthread_t* t, maincompletedqueue_entry_t* me )
{
  bool success = 0;
  while( !success ) {
    EnqueueS( t->output, me, &success );
  }

#if defined(WIN)
  auto r = SetEvent( g_mainthread.wake_asynctaskscompleted );
  AssertCrash( r );
#elif defined(MAC)
#else
#error Unsupported platform
#endif

#if LOGASYNCTASKS
  Log( "maincompletedqueue_entry_t generated" );
#endif
}


void
MainThreadInit()
{
#if BADTLS
  g_tls_handle = TlsAlloc();
  _ReadWriteBarrier();
  AssertWarn( g_tls_handle != TLS_OUT_OF_INDEXES );
#endif

  ThreadInit();

#if defined(WIN)
  SYSTEM_INFO si = { 0 };
  GetSystemInfo( &si );
  AssertCrash( si.dwNumberOfProcessors > 0 );
  auto num_procs = Cast( idx_t, si.dwNumberOfProcessors );

  // note these taskthreads are in addition to the main thread, so we're actually oversubscribing here by 1.
  // this is in the hope of having the main thread sleep while all taskthreads run.
  // going to a higher thread count would be theoretically suboptimal, due to extra OS context switches.
  auto num_threads = num_procs;

  g_mainthread.signal_quit = 0;

  g_mainthread.wake_asynctaskscompleted = CreateEvent( 0, 0, 0, 0 );
  AssertCrash( g_mainthread.wake_asynctaskscompleted );

  constant idx_t c_perthread_queuesize = 16000;

  Alloc( g_mainthread.taskthread_handles, num_threads );
  Alloc( g_mainthread.taskthreads, num_threads );

#if PULLMODEL
  Alloc( g_mainthread.tasks, num_threads * c_perthread_queuesize );
#else
#endif

  For( i, 0, num_threads ) {
    auto t = AddBack( g_mainthread.taskthreads );
    t->wake = CreateEvent( 0, 0, 0, 0 );
    AssertCrash( t->wake );
#if PULLMODEL
#else
    Alloc( t->input, c_perthread_queuesize );
#endif
    Alloc( t->output, c_perthread_queuesize );
    auto handle = AddBack( g_mainthread.taskthread_handles );
    _ReadWriteBarrier();
    *handle = Cast( HANDLE, _beginthreadex( 0, 0, TaskThread, t, 0, 0 ) );
    AssertCrash( *handle );
  }
#elif defined(MAC)
#else
#error Unsupported platform
#endif
}

void
SignalQuitAndWaitForTaskThreads()
{
  g_mainthread.signal_quit = 1;
  _ReadWriteBarrier();

#if defined(WIN)
  ForLen( i, g_mainthread.taskthreads ) {
    auto taskthread = g_mainthread.taskthreads.mem + i;
    auto r = SetEvent( taskthread->wake );
    AssertCrash( r );
  }

  // TODO: should we use Msg- version of this, to keep flushing our win messages ?
  DWORD waitres = WaitForMultipleObjects(
    Cast( DWORD, g_mainthread.taskthread_handles.len ),
    g_mainthread.taskthread_handles.mem,
    1,
    INFINITE
    );
  AssertCrash( waitres != WAIT_FAILED );
  AssertCrash( waitres != WAIT_TIMEOUT );
#elif defined(MAC)
#else
#error Unsupported platform
#endif
}

void
MainThreadKill()
{
#if defined(WIN)
  Free( g_mainthread.taskthread_handles );
#elif defined(MAC)
#else
#error Unsupported platform
#endif

  FORLEN( t, i, g_mainthread.taskthreads )
    Kill( *t );
  }
  Free( g_mainthread.taskthreads );

#if PULLMODEL
  Free( g_mainthread.tasks );
#else
#endif

  ThreadKill();

#if BADTLS
  TlsFree( g_tls_handle );
  g_tls_handle = 0;
#endif
}




#if defined(TEST)

__ExecuteOutput( OutputForExecute )
{
  using pa_t = stack_resizeable_pagelist_t<u8>;
  auto pa = Cast( pa_t*, misc0 );
  auto dst = AddBack( *pa, message->len );
  Memmove( dst, ML( *message ) );
}

Inl void
TestExecute()
{
  stack_resizeable_pagelist_t<u8> output;
  Init( output, 64 );
  auto cmd = SliceFromCStr( "cmd.exe /c echo hello world!" );
  s32 r = Execute( cmd, 0, OutputForExecute, &output, 0 );
  auto output_pos = MakeIteratorAtLinearIndex( output, 0 );
  auto expected = SliceFromCStr( "hello world!\r\n" );
  AssertCrash( !r );
  AssertCrash( MemEqual( ML( expected ), ML( *output_pos.page ) ) );
  Kill( output );
}

#endif // defined(TEST)
