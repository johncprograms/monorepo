// Copyright (c) John A. Carlos Jr., all rights reserved.

#if FINDLEAKS

  // TODO: snap the callstacks as well, and store that as a value in the hashset.
  
  static volatile bool g_track_active_allocs = 0;
  static lock_t g_active_allocs_lock = 0;
  static hashset_nonzeroptrs_t<void*, idx_t> g_active_allocs;
  
  Inl void
  FindLeaks_MemHeapAllocBytes( void* mem, idx_t nbytes )
  {
    AssertCrash( mem );
    Lock( &g_active_allocs_lock );
    if( g_track_active_allocs ) {
      g_track_active_allocs = 0; // temporarily turn off alloc tracking for the tracking hashset expansion code.
      {
        bool already_there = 0;
        Add(
          &g_active_allocs,
          mem,
          &nbytes,
          &already_there,
          (idx_t*)0 /*val_already_there*/,
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
          idx_t* value = 0;
          Lookup(
            &g_active_allocs,
            oldmem,
            &found,
            &value
            );
          AssertCrash( found );
          AssertCrash( *value == oldlen );
          *value = newlen;
        }
        else {
          idx_t value = 0;
          bool found = 0;
          Remove(
            &g_active_allocs,
            oldmem,
            &found,
            &value
            );
          AssertCrash( found );
          AssertCrash( value == oldlen );
          
          bool already_there = 0;
          Add(
            &g_active_allocs,
            newmem,
            &newlen,
            &already_there,
            (idx_t*)0 /*val_already_there*/,
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
        bool found = 0;
        Remove(
          &g_active_allocs,
          mem,
          &found,
          (idx_t*)0 /*found_val*/
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

    FORELEM_HASHSET_NONZEROPTRS( g_active_allocs, i, allocation_mem, allocation_len )
      printf( "Leaked allocation: ( %llu, %llu )\n", Cast( u64, allocation_mem ), Cast( u64, allocation_len ) );
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

