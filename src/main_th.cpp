// build:console_x64_debug
// Copyright (c) John A. Carlos Jr., all rights reserved.

#include "math_vec.h"
#include "math_matrix.h"
#include "ds_slice.h"
#include "ds_string.h"
#include "ds_plist.h"
#include "ds_array.h"
#include "ds_embeddedarray.h"
#include "ds_fixedarray.h"
#include "ds_pagearray.h"
#include "ds_list.h"
#include "ds_bytearray.h"
#include "ds_hashset.h"
#include "cstr.h"
#include "filesys.h"
#include "timedate.h"
#include "threading.h"
#define LOGGER_ENABLED   0
#include "logger.h"
#define PROF_ENABLED   0
#define PROF_ENABLED_AT_LAUNCH   0
#include "profile.h"
#include "rand.h"
#include "main.h"


// TODO: move this to test!


static const u32 num_threads = 7;

static volatile bool g_started = 0;
static volatile bool g_quit = 0;
static volatile idx_t g_counter = 0;

static const u32 thread_data_len = 200u*1024*1024 / num_threads;


struct
m2w_t
{
  u32* data;
  idx_t data_len;
};

struct
w2m_t
{
  u32 sum;
};


#define SINGLEm2w   0
#define SINGLEw2m   0

struct
worker_t
{
  u32 tidx;

#if SINGLEm2w
  queue_srsw_t<m2w_t> m2w;
#else
  queue_mrsw_t<m2w_t>* m2w;
#endif

#if SINGLEw2m
  queue_srsw_t<w2m_t> w2m;
#else
  queue_srmw_t<w2m_t>* w2m;
#endif
};


Enumc( workerstate_t )
{
  pullwork,
  pushwork,
};

u32 __stdcall
MainWorker( LPVOID data )
{
  ThreadInit();

  auto& worker = *Cast( worker_t*, data );

  auto state = workerstate_t::pullwork;
  u32 sum = 0;

  Forever {
    if( !g_started ) {
      Prof( worker_startup_sleep );
      TimeSleep( 1 );
      continue;
    }
    Prof( worker_mainloop );
    if( g_quit ) {
      break;
    }

    // State-switch and transitions.
    switch( state ) {
      case workerstate_t::pullwork: {
        m2w_t in;
        bool success;

        Prof( worker_dequeue );
#if SINGLEm2w
        DequeueS( worker.m2w, &in, &success );
#else
        DequeueM( *worker.m2w, &in, &success );
#endif
        ProfClose( worker_dequeue );

        if( success ) {
          Prof( worker_dowork );
          sum = 0;
          for( idx_t i = 0;  i < in.data_len;  ++i ) {
            sum += in.data[i];
          }
          ProfClose( worker_dowork );
          state = workerstate_t::pushwork;
        }
      } break;

      case workerstate_t::pushwork: {
        w2m_t out;
        out.sum = sum;
        bool success;

        Prof( worker_enqueue );
#if SINGLEw2m
        EnqueueS( worker.w2m, &out, &success );
#else
        EnqueueM( *worker.w2m, &out, &success );
#endif
        ProfClose( worker_enqueue );

        if( success ) {
          state = workerstate_t::pullwork;
        }
      } break;

      default: UnreachableCrash();
    }
  }

  ThreadKill();

  return 0;
}


void
TestThreadedQueues()
{
  array_t<u32*> threaddata;
  Alloc( threaddata, num_threads );
  threaddata.len = num_threads;
  for( idx_t tidx = 0;  tidx < num_threads;  ++tidx ) {
    threaddata.mem[tidx] = MemVirtualAlloc( u32, thread_data_len );
    for( idx_t i = 0;  i < thread_data_len;  ++i ) {
      threaddata.mem[tidx][i] = 1;
    }
  }

  Prof( queue_init );
  worker_t workers[num_threads];
#if !SINGLEm2w
  queue_mrsw_t<m2w_t> qm2w;
  AllocQueue( qm2w, num_threads * 8 );
#endif
#if !SINGLEw2m
  queue_srmw_t<w2m_t> qw2m;
  AllocQueue( qw2m, num_threads * 8 );
#endif

  for( u32 i = 0;  i < num_threads;  ++i ) {
    workers[i].tidx = 1 + i;

#if SINGLEm2w
    Alloc( workers[i].m2w, 8 );
#else
    workers[i].m2w = &qm2w;
#endif

#if SINGLEw2m
    Alloc( workers[i].w2m, 8 );
#else
    workers[i].w2m = &qw2m;
#endif

    m2w_t m2w;
    m2w.data = threaddata.mem[i];
    m2w.data_len = thread_data_len;
    bool success;
#if SINGLEm2w
    EnqueueS( workers[i].m2w, &m2w, &success );
#else
    EnqueueS( *workers[i].m2w, &m2w, &success );
#endif
    AssertWarn( success );
  }
  ProfClose( queue_init );

  Prof( thread_init );
  HANDLE threads_w[num_threads];
  for( u32 i = 0;  i < num_threads;  ++i ) {
    threads_w[i] = Cast( HANDLE, _beginthreadex( 0, 0, MainWorker, &workers[i], 0, 0 ) );
    AssertWarn( threads_w[i] );
  }
  ProfClose( thread_init );

  TimeSleep( 1 );

  printf( "start\n" );
  g_started = 1;

  //TimeSleep( 1000 );

  Prof( main_collectresponses );
  u32 sum = 0;
  u32 num_responses = 0;
  while( num_responses < num_threads ) {

    for( u32 i = 0;  i < num_threads;  ++i ) {
      bool success;
      w2m_t w2m;

      Prof( main_dequeue );
#if SINGLEw2m
      DequeueS( workers[i].w2m, &w2m, &success );
#else
      DequeueS( *workers[i].w2m, &w2m, &success );
#endif
      ProfClose( main_dequeue );

      if( success ) {
        num_responses += 1;
        sum += w2m.sum;
      }
    }
  }
  ProfClose( main_collectresponses );

  AssertWarn( sum == num_threads * thread_data_len );
  printf( "%u, %u\n", sum, num_threads * thread_data_len );

  Prof( thread_kill );
  g_quit = 1;
  printf( "quit\n" );
  for( u32 i = 0;  i < num_threads;  ++i ) {
    DWORD wait = WaitForSingleObject( threads_w[i], INFINITE );
    AssertWarn( wait == WAIT_OBJECT_0 );
  }
  ProfClose( thread_kill );

  Prof( queue_kill );
  for( u32 i = 0;  i < num_threads;  ++i ) {
#if SINGLEm2w
    Free( workers[i].m2w );
#endif
#if SINGLEw2m
    Free( workers[i].w2m );
#endif
  }
#if !SINGLEm2w
  Free( qm2w );
#endif
#if !SINGLEw2m
  Free( qw2m );
#endif
  ProfClose( queue_kill );


  For( i, 0, num_threads ) {
    MemVirtualFree( threaddata.mem[i] );
  }
  Free( threaddata );
}


static const s64 delay_over_sec = -10000000;
static const s64 delay_over_millisec = -10000;
static const s32 timeout_period_millisec = 5000;


void
TestTimerDelay()
{
  HANDLE timer = CreateWaitableTimer( 0, 0, 0 );
  AssertWarn( timer );

  HANDLE wait_timers[] = {
    timer,
  };

  f32 test_timeout_sec = 15;
  u64 time0 = TimeClock();
  u64 count = 0;
  s64 period = delay_over_millisec / 1;
  while( TimeSecFromClocks32( time0, TimeClock() ) < test_timeout_sec ) {
    Prof( mainloop );
#if 1
    bool success = !!SetWaitableTimer( timer, Cast( LARGE_INTEGER*, &period ), 0, 0, 0, 0 );
    AssertWarn( success );
    DWORD waitres = MsgWaitForMultipleObjects( 1, wait_timers, 0, timeout_period_millisec, QS_ALLINPUT );
    if( waitres == WAIT_FAILED ) {
      AssertWarn( 0 );
    } elif( waitres == WAIT_TIMEOUT ) {
      AssertWarn( 0 );
    } elif( waitres == WAIT_OBJECT_0 ) {
      //printf( "tick!\n" );
      ++count;
    } else {
      AssertWarn( 0 );
    }
#else
    ++count;
    TimeSleep( 1 );
#endif
  }
  printf( "done! %llu\n", count );

  CloseHandle( timer );
}


void
TestTimerDelayThenPeriodic()
{
  HANDLE timer = CreateWaitableTimer( 0, 0, 0 );
  AssertWarn( timer );

  HANDLE wait_timers[] = {
    timer,
  };

  s32 period_millisec = 17;
  s64 delay = delay_over_millisec * 1500;
  bool success = !!SetWaitableTimer( timer, Cast( LARGE_INTEGER*, &delay ), period_millisec, 0, 0, 0 );
  AssertWarn( success );

  kahan32_t fdelay = {};
  kahan32_t fperiod = {};

  f32 test_timeout_sec = 5;
  u64 time0 = TimeClock();
  u64 time_start = time0;
  u64 count = 0;
  while( TimeSecFromClocks32( time_start, TimeClock() ) < test_timeout_sec ) {
    Prof( mainloop );
    DWORD waitres = MsgWaitForMultipleObjects( 1, wait_timers, 0, timeout_period_millisec, QS_ALLINPUT );
    if( waitres == WAIT_FAILED ) {
      AssertWarn( 0 );
    } elif( waitres == WAIT_TIMEOUT ) {
      AssertWarn( 0 );
    } elif( waitres == WAIT_OBJECT_0 ) {
      if( count == 0 ) {
        u64 time1 = TimeClock();
        Add( fdelay, TimeSecFromClocks32( time0, time1 ) );
        time0 = time1;
      } else {
        u64 time1 = TimeClock();
        Add( fperiod, TimeSecFromClocks32( time0, time1 ) );
        time0 = time1;
      }
      //printf( "tick!\n" );
      ++count;
    } else {
      AssertWarn( 0 );
    }
  }
  printf( "delay_sec %f\n", fdelay.sum );
  printf( "period_millisec %f\n", 1000.0f * fperiod.sum / ( count - 1 ) );
  printf( "done! %llu\n", count );

  CloseHandle( timer );
}

void
TestMultiplePeriodicTimers()
{
  HANDLE timer0 = CreateWaitableTimer( 0, 0, 0 );
  AssertWarn( timer0 );
  HANDLE timer1 = CreateWaitableTimer( 0, 0, 0 );
  AssertWarn( timer1 );

  HANDLE wait_timers[] = {
    timer0,
    timer1,
  };

  s32 period_millisec = 3;
  s64 delay = delay_over_millisec * 1500;
  bool success = !!SetWaitableTimer( timer0, Cast( LARGE_INTEGER*, &delay ), period_millisec, 0, 0, 0 );
  AssertWarn( success );

  period_millisec = 12;
  delay = delay_over_millisec * 327;
  success = !!SetWaitableTimer( timer1, Cast( LARGE_INTEGER*, &delay ), period_millisec, 0, 0, 0 );
  AssertWarn( success );

  kahan32_t fdelay[2] = {};
  kahan32_t fperiod[2] = {};

  f32 test_timeout_sec = 15;
  u64 time_start = TimeClock();
  u64 time0[2] = { time_start, time_start };
  u64 count[2] = {};
  while( TimeSecFromClocks32( time_start, TimeClock() ) < test_timeout_sec ) {
    Prof( mainloop );
    DWORD waitres = MsgWaitForMultipleObjects( 2, wait_timers, 0, timeout_period_millisec, QS_ALLINPUT );
    if( waitres == WAIT_FAILED ) {
      AssertWarn( 0 );
    } elif( waitres == WAIT_TIMEOUT ) {
      AssertWarn( 0 );
    } elif( waitres == WAIT_OBJECT_0 + 0 ) {
      if( count[0] == 0 ) {
        u64 time1 = TimeClock();
        Add( fdelay[0], TimeSecFromClocks32( time0[0], time1 ) );
        time0[0] = time1;
      } else {
        u64 time1 = TimeClock();
        Add( fperiod[0], TimeSecFromClocks32( time0[0], time1 ) );
        time0[0] = time1;
      }
      //printf( "tick!\n" );
      ++count[0];
    } elif( waitres == WAIT_OBJECT_0 + 1 ) {
      if( count[1] == 0 ) {
        u64 time1 = TimeClock();
        Add( fdelay[1], TimeSecFromClocks32( time0[1], time1 ) );
        time0[1] = time1;
      } else {
        u64 time1 = TimeClock();
        Add( fperiod[1], TimeSecFromClocks32( time0[1], time1 ) );
        time0[1] = time1;
      }
      //printf( "tick!\n" );
      ++count[1];
    } else {
      AssertWarn( 0 );
    }
  }
  printf( "[0] delay_sec %f\n", fdelay[0].sum );
  printf( "[0] period_millisec %f\n", 1000.0f * fperiod[0].sum / ( count[0] - 1 ) );
  printf( "[0] done! %llu\n", count[0] );
  printf( "\n" );
  printf( "[1] delay_sec %f\n", fdelay[1].sum );
  printf( "[1] period_millisec %f\n", 1000.0f * fperiod[1].sum / ( count[1] - 1 ) );
  printf( "[1] done! %llu\n", count[1] );

  CloseHandle( timer0 );
  CloseHandle( timer1 );
}



int
Main( u8* cmdline, idx_t cmdline_len )
{
  TestThreadedQueues();
  TestTimerDelay();
  TestTimerDelayThenPeriodic();
  TestMultiplePeriodicTimers();

  return 0;
}




#ifdef _DEBUG

int
main( int argc, char** argv )
{
  MainInit();

  array_t<u8> cmdline;
  Alloc( cmdline, 512 );
  for( int i = 1;  i < argc;  ++i ) {
    u8* arg = Cast( u8*, argv[i] );
    idx_t arg_len = CsLen( arg );
    Memmove( AddBack( cmdline, arg_len ), arg, arg_len );
    Memmove( AddBack( cmdline, 2 ), " ", 2 );
  }
  int r = Main( ML( cmdline ) );
  Free( cmdline );

  MainKill();

  printf( "Main returned: %d\n", r );
  system( "pause" );

  return r;
}

#else

int WINAPI
WinMain( HINSTANCE prog_inst, HINSTANCE prog_inst_prev, LPSTR prog_cmd_line, int prog_cmd_show )
{
  MainInit();

  u8* cmdline = Str( prog_cmd_line );
  idx_t cmdline_len = CsLen( Str( prog_cmd_line ) );

  AllocConsole();
  freopen("CONOUT$", "wb", stdout);
  freopen("CONOUT$", "wb", stderr);

  int r = Main( cmdline, cmdline_len );

  printf( "Main returned: %d\n", r );
  system( "pause" );

  MainKill();
  return r;
}

#endif

