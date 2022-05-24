// build:console_x64_debug
// Copyright (c) John A. Carlos Jr., all rights reserved.

#include "common.h"
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

int
main( int argc, char** argv )
{
  MainInit();

  embeddedarray_t<u8, 64> tmp;
  CsFrom_u64( tmp.mem, Capacity( tmp ), &tmp.len, TimeTSC() );
  *AddBack( tmp ) = 0;
  printf( "%s", tmp.mem );

  MainKill();
  return 0;
}
