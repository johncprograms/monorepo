// Copyright (c) John A. Carlos Jr., all rights reserved.

#if FINDLEAKS

  static volatile bool g_track_active_allocs = 0;
  static lock_t g_active_allocs_lock = 0;
  struct alloc_info_t
  {
    idx_t nbytes;
    // NOTE: we store the original allocation callstack, not the realloc callstacks.
    // TODO: we should probably store everything.
    void* frames[16];
    u16 num_frames;
    bool realloced;
  };
  static hashset_nonzeroptrs_t<void*, alloc_info_t> g_active_allocs;

  Inl void
  FindLeaks_MemHeapAllocBytes( void* mem, idx_t nbytes )
  {
    AssertCrash( mem );
    Lock( &g_active_allocs_lock );
    if( g_track_active_allocs ) {
      g_track_active_allocs = 0; // temporarily turn off alloc tracking for the tracking hashset expansion code.
      {
        alloc_info_t info = {};
        info.nbytes = nbytes;
        info.num_frames = RtlCaptureStackBackTrace( 0u, _countof( info.frames ), info.frames, 0 );

        bool already_there = 0;
        Add(
          &g_active_allocs,
          mem,
          &info,
          &already_there,
          (alloc_info_t*)0 /*val_already_there*/,
          (bool*)0 /*overwrite_val_if_already_there*/
          );
        AssertCrash( !already_there );
      }
      g_track_active_allocs = 1;
    }
    Unlock( &g_active_allocs_lock );

//    if( counter == 7381 ) __debugbreak();
//    Log( "alloc # %llu, nbytes %llu", counter, nbytes );
  }

  Inl void
  FindLeaks_MemHeapReallocBytes( void* oldmem, idx_t oldlen, void* newmem, idx_t newlen )
  {
    AssertCrash( oldmem );
    AssertCrash( newmem );
    Lock( &g_active_allocs_lock );
    if( g_track_active_allocs ) {
      g_track_active_allocs = 0; // temporarily turn off alloc tracking for the tracking hashset expansion code.
      {
        if( oldmem == newmem ) {
          bool found = 0;
          alloc_info_t* info = 0;
          Lookup(
            &g_active_allocs,
            oldmem,
            &found,
            &info
            );
          AssertCrash( found );
          AssertCrash( info->nbytes == oldlen );
          info->nbytes = newlen;
          info->realloced = 1;
        }
        else {
          alloc_info_t info;
          bool found = 0;
          Remove(
            &g_active_allocs,
            oldmem,
            &found,
            &info
            );
          AssertCrash( found );
          AssertCrash( info.nbytes == oldlen );

          info.nbytes = newlen;
          info.realloced = 1;

          bool already_there = 0;
          Add(
            &g_active_allocs,
            newmem,
            &info,
            &already_there,
            (alloc_info_t*)0 /*val_already_there*/,
            (bool*)0 /*overwrite_val_if_already_there*/
            );
          AssertCrash( !already_there );
        }
      }
      g_track_active_allocs = 1;
    }
    Unlock( &g_active_allocs_lock );
  }

  Inl void
  FindLeaks_MemHeapFree( void* mem )
  {
    AssertCrash( mem );
    Lock( &g_active_allocs_lock );
    if( g_track_active_allocs ) {
      g_track_active_allocs = 0; // temporarily turn off alloc tracking for the tracking hashset expansion code.
      {
        alloc_info_t info;
        bool found = 0;
        Remove(
          &g_active_allocs,
          mem,
          &found,
          &info
          );
        AssertCrash( found );
      }
      g_track_active_allocs = 1;
    }
    Unlock( &g_active_allocs_lock );
  }

  Inl void
  FindLeaksInit()
  {
    Init( &g_active_allocs, 10000 );
    g_track_active_allocs = 1;
  }

  Inl void
  FindLeaksKill()
  {
    g_track_active_allocs = 0;

    FORELEM_HASHSET_NONZEROPTRS( g_active_allocs, i, alloc_mem, info )
      printf( "Leaked allocation: ( %llu, %llu )\n", Cast( u64, alloc_mem ), Cast( u64, info.nbytes ) );
      __debugbreak();
    }
    Kill( &g_active_allocs );
  }

#else

  Inl void
  FindLeaksInit()
  {
  }

  Inl void
  FindLeaksKill()
  {
  }

#endif

