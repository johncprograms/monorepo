// Copyright (c) John A. Carlos Jr., all rights reserved.

typedef int8_t     s8;
typedef int16_t    s16;
typedef int32_t    s32;
typedef int64_t    s64;

#define MIN_s8    Cast( s8, -128 )
#define MAX_s8    Cast( s8, 0x7F )

#define MIN_s16   Cast( s16, -32768 )
#define MAX_s16   Cast( s16, 0x7FFF )

#define MIN_s32   Cast( s32, 0x80000000 ) // -2147483648, INT_MIN VC compiler doesn't parse this min value for whatever reason; see C4146 docs.
#define MAX_s32   Cast( s32, 0x7FFFFFFF )

#define MIN_s64   Cast( s64, 0x8000000000000000LL ) // -9223372036854775808, LLONG_MIN
#define MAX_s64   Cast( s64, 0x7FFFFFFFFFFFFFFFLL )


typedef uint8_t    u8;
typedef uint16_t   u16;
typedef uint32_t   u32;
typedef uint64_t   u64;

#define MAX_u8    Cast( u8, 0xFFU )
#define MAX_u16   Cast( u16, 0xFFFFU )
#define MAX_u32   Cast( u32, 0xFFFFFFFFU )
#define MAX_u64   Cast( u64, 0xFFFFFFFFFFFFFFFFULL )

#define HIGHBIT_u8    Cast( u8, 0x80U )
#define HIGHBIT_u16   Cast( u16, 0x8000U )
#define HIGHBIT_u32   Cast( u32, 0x80000000U )
#define HIGHBIT_u64   Cast( u64, 0x8000000000000000ULL )


typedef float    f32;
typedef double   f64;

#define MIN_f32   ( -3.402823466e+38F )
#define MAX_f32   (  3.402823466e+38F )

#define MAX_INT_REPRESENTABLE_IN_f32 ( 1u << 24 )
#define MAX_INT_REPRESENTABLE_IN_f64 ( 1ull << 53 )

#define MIN_f64   ( -1.7976931348623158e+308 )
#define MAX_f64   (  1.7976931348623158e+308 )

#if defined( _M_AMD64 ) || defined( MAC ) // TODO: better mac specifications.

  typedef uint64_t   idx_t;
  typedef int64_t   sidx_t;

  #define _SIZEOF_IDX_T   8
  #define NUMBITS_idx     64
  #define MAX_idx         MAX_u64
  #define HIGHBIT_idx     HIGHBIT_u64
  #define MAX_sidx        MAX_s64
  #define MIN_sidx        MIN_s64

#elif _M_IX86

  typedef uint32_t   idx_t;
  typedef int32_t   sidx_t;

  #define _SIZEOF_IDX_T   4
  #define NUMBITS_idx     32
  #define MAX_idx         MAX_u32
  #define HIGHBIT_idx     HIGHBIT_u32
  #define MAX_sidx        MAX_s32
  #define MIN_sidx        MIN_s32

#else
  #error Define _SIZEOF_IDX_T based on a predefined macro for your new platform.
#endif
