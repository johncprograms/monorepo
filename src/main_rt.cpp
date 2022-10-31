// build:window_x64_debug
// Copyright (c) John A. Carlos Jr., all rights reserved.

#ifdef WIN

#define FINDLEAKS 0
#include "core_cruntime.h"
#include "core_types.h"
#include "core_language_macros.h"
#include "os_mac.h"
#include "os_windows.h"
#include "memory_operations.h"
#include "asserts.h"
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
#include "ds_list.h"
#include "ds_stack_cstyle.h"
#include "ds_hashset_cstyle.h"
#include "filesys.h"
#include "timedate.h"
#include "threading.h"
#define LOGGER_ENABLED   0
#include "logger.h"
#define PROF_ENABLED   0
#define PROF_ENABLED_AT_LAUNCH   0
#include "profile.h"
#include "rand.h"
#include "allocator_heap_findleaks.h"
#include "mainthread.h"

#define OPENGL_INSTEAD_OF_SOFTWARE       1
#define GLW_RAWINPUT_KEYBOARD            0
#define GLW_RAWINPUT_KEYBOARD_NOHOTKEY   1
#include "glw.h"

#include "ui_font.h"
#include "ui_render.h"

#include "img_write.h"
#include "img.h"
#include "render.h"


//static void
//Image_u8x4_from_f32x4( u8* dst, img_t& img, bool flip )
//{
//  static __m128 f32x4_255 = _mm_set1_ps( 255 );
//  u32* dst32 = Cast( u32*, dst );
//
//  if( flip ) {
//    for( u32 y = 0;  y < img.y;  ++y ) {
//    for( u32 x = 0;  x < img.x;  ++x ) {
//      __m128 src = _mm_load_ps( &LookupAs( f32, img, x, img.y - y - 1 ) );
//      __m128i src_int = _mm_cvtps_epi32( _mm_mul_ps( f32x4_255, src ) );
//      src_int = _mm_packus_epi32( src_int, src_int );
//      src_int = _mm_packus_epi16( src_int, src_int );
//      *dst32++ = src_int.m128i_u32[0];
//    }
//    }
//  } else {
//    for( u32 y = 0;  y < img.y;  ++y ) {
//    for( u32 x = 0;  x < img.x;  ++x ) {
//      __m128 src = _mm_load_ps( &LookupAs( f32, img, x, y ) );
//      __m128i src_int = _mm_cvtps_epi32( _mm_mul_ps( f32x4_255, src ) );
//      src_int = _mm_packus_epi32( src_int, src_int );
//      src_int = _mm_packus_epi16( src_int, src_int );
//      *dst32++ = src_int.m128i_u32[0];
//    }
//    }
//  }
//}


Inl void
ConvertImageUpsize( u8* dst, u32 dst_x, u32 dst_y, img_t& img, u32 upsize_factor )
{
  ProfFunc();
  AssertCrash( img.bytes_per_px == 16 );
  AssertCrash( img.y * upsize_factor <= dst_y );

  u32 dst_nbytes_row = dst_x * 4;
  u32 src_nbytes_row = img.stride_x * img.bytes_per_px;
  auto src = &LookupAs( u8, img, 0, 0 );
  u8* src_row = src;
  u8* dst_row = dst;
  Fori( u32, y, 0, img.y ) {
    u8* src_px = src_row;
    u8* dst_px = dst_row;
    Fori( u32, x, 0, img.x ) {
      auto& src_f32x4 = *Cast( vec4<f32>*, src_px );
      auto src_u8x4 = Convert_u8x4_from_f32x4( src_f32x4 );
      Fori( u32, i, 0, upsize_factor ) {
        Memmove( dst_px, &src_u8x4, 4 );
        dst_px += 4;
      }
      src_px += img.bytes_per_px;
    }
    u8* next_dst_row = dst_row + dst_nbytes_row;
    Fori( u32, j, 1, upsize_factor ) {
      Memmove( next_dst_row, dst_row, dst_nbytes_row );
      next_dst_row += dst_nbytes_row;
    }
    dst_row = next_dst_row;
    src_row += src_nbytes_row;
  }
}

Inl void
ConvertImage( u8* dst, img_t& img )
{
  ProfFunc();
  u32 dst_nbytes_row = img.x * img.bytes_per_px;
  u32 src_nbytes_row = img.stride_x * img.bytes_per_px;
  auto src = &LookupAs( u8, img, 0, 0 );
  Fori( u32, y, 0, img.y ) {
    Memmove( dst, src, dst_nbytes_row );
    dst += dst_nbytes_row;
    src += src_nbytes_row;
  }
}
Inl void
ConvertImageFlip( u8* dst, img_t& img )
{
  ProfFunc();
  u32 dst_nbytes_row = img.x * img.bytes_per_px;
  u32 src_nbytes_row = img.stride_x * img.bytes_per_px;
  auto src = &LookupAs( u8, img, 0, img.y_m1 );
  Fori( u32, y, 0, img.y ) {
    Memmove( dst, src, dst_nbytes_row );
    dst += dst_nbytes_row;
    src -= src_nbytes_row;
  }
}


struct
app_t
{
  glwclient_t client;
  bool fullscreen;
  bool initialized;
  bool reshape_required;
  rendermode_t rendermode;
  u32 upsize_factor;
  u32 samples_per_pixel;
  u32 max_upsize_factor;
  u32 min_upsize_factor;
  f32 time_anim;
  f32 time_anim_period;
  rng_xorshift32_t rt_rng;

  vec3<f32> rot;
  vec3<f32> pos;
  f32 camera_fov_y;
  f32 camera_near_z;
  f32 camera_far_z;
  f32 camera_half_y;

  img_t target_img4;
  img_t target_img16;
  img_t target_dep4;
  u32 texid_target;

  u32 raster_tile_x;
  u32 raster_tile_y;
  u32 raster_tris_per_tile;
  raster_tiles_t raster_tiles;

  stack_resizeable_cont_t<f32> stream;
  u32 glstream;
  shader_tex2_t shader;

  img_t checker_tex;
  img_t white_tex;

  stack_resizeable_cont_t<mesh_t> meshes;
  stack_resizeable_cont_t<rtmesh_t*> lights;
};

Inl void
AppInit( app_t& app )
{
  GlwInit();

  app.fullscreen = 0;
  app.initialized = 0;
  app.reshape_required = 1;
  app.rendermode = rendermode_t::ray;
#ifdef _DEBUG
  app.upsize_factor = 16;
#else
  app.upsize_factor = 4;
#endif
  app.samples_per_pixel = 50;
  app.time_anim = 0;
  app.time_anim_period = 1000;
  Init( app.rt_rng, 1234567890 );

  app.rot = _vec3<f32>( 0.12f, -0.12f, 0 );
  app.pos = _vec3<f32>( 5, 5, 50 );

  app.camera_fov_y = 0.34f * f32_PI;
  app.camera_near_z = 1.0;
  app.camera_far_z = 1000.0;
  app.camera_half_y = app.camera_near_z * Tan32( 0.5f * app.camera_fov_y );

  Init( app.target_img4, 512, 512, 512, 4 ); // arbitrary starting size; will probably realloc when we get real window size.
  Init( app.target_img16, 512, 512, 512, 16 );
  Init( app.target_dep4, 512, 512, 512, 4 );
  Alloc( app.target_img4 );
  Alloc( app.target_img16 );
  Alloc( app.target_dep4 );
  app.texid_target = 1;


  app.raster_tile_x = 64;
  app.raster_tile_y = 64;
  app.raster_tris_per_tile = 8;
  Init(
    app.raster_tiles,
    app.target_img4.x,
    app.target_img4.y,
    app.target_img4.stride_x,
    app.raster_tile_x,
    app.raster_tile_y,
    app.raster_tris_per_tile
    );

  GlwInitWindow(
    app.client,
    Str( "rt" ),
    2,
    _vec2<u32>( 600, 400 )
    );

  Alloc( app.stream, 65536 );
  glGenBuffers( 1, &app.glstream );

  ShaderInit( app.shader );


  Prof( checker_tex_init );
//  u8* filename = Str( "c:/doc/dev/cpp/master/rt/output.png" );
  Init( app.checker_tex, 512, 512, 512, 4 );
  //Init( app.checker_tex, 2, 2, 4, 4 );
  Alloc( app.checker_tex );

#if 1
  Fori( u32, y, 0, app.checker_tex.y ) {
  Fori( u32, x, 0, app.checker_tex.x ) {
    auto& tx = LookupAs( vec4<u8>, app.checker_tex, x, y );
//    if( ( x / Cast( u32, 0.1f * app.checker_tex.x ) + y / Cast( u32, 0.1f * app.checker_tex.y ) ) % 2 ) {
//    if( ( x + y ) % 2 ) {
    if( ( ( x / 64 ) + ( y / 64 ) ) % 2 ) {
//    if( ( ( 2 * x / app.checker_tex.x ) + ( 2 * y / app.checker_tex.y ) ) % 2 ) {
      tx = _vec4<u8>( 255, 255, 255, 255 );
    } else {
      tx = _vec4<u8>( 128, 128, 255, 255 );
    }
  }
  }
#else
  LookupAs( vec4<u8>, app.checker_tex, 0, 0 ) = _vec4<u8>( 255,  10,  10, 255 );
  LookupAs( vec4<u8>, app.checker_tex, 1, 1 ) = _vec4<u8>( 255,  10,  10, 255 );
  LookupAs( vec4<u8>, app.checker_tex, 0, 1 ) = _vec4<u8>(  10, 255,  10, 255 );
  LookupAs( vec4<u8>, app.checker_tex, 1, 0 ) = _vec4<u8>(  10, 255,  10, 255 );
#endif
  ProfClose( checker_tex_init );




  Init( app.white_tex, 1, 1, 4, 4 );
  Alloc( app.white_tex );
  LookupAs( vec4<u8>, app.white_tex, 0, 0 ) = _vec4<u8>( 255, 255, 255, 255 );


  static vec4<f32> box_positions[] =
  {
    { -1, -1, -1, 1 },
    { -1,  1, -1, 1 },
    { -1, -1,  1, 1 },
    { -1,  1,  1, 1 },

    {  1, -1, -1, 1 },
    {  1,  1, -1, 1 },
    {  1, -1,  1, 1 },
    {  1,  1,  1, 1 },

    { -1, -1, -1, 1 },
    {  1, -1, -1, 1 },
    { -1, -1,  1, 1 },
    {  1, -1,  1, 1 },

    { -1,  1, -1, 1 },
    {  1,  1, -1, 1 },
    { -1,  1,  1, 1 },
    {  1,  1,  1, 1 },

    { -1, -1, -1, 1 },
    {  1, -1, -1, 1 },
    { -1,  1, -1, 1 },
    {  1,  1, -1, 1 },

    { -1, -1,  1, 1 },
    {  1, -1,  1, 1 },
    { -1,  1,  1, 1 },
    {  1,  1,  1, 1 },
  };

  f32 x0 = 0.0f;
  f32 x1 = 1.0f / 6.0f;
  f32 x2 = 2.0f / 6.0f;
  f32 x3 = 0.5f;
  f32 x4 = 4.0f / 6.0f;
  f32 x5 = 5.0f / 6.0f;
  f32 x6 = 1.0f;

  f32 y0 = 0.0f;
  f32 y1 = 1.0f;

  static vec2<f32> box_texcoords[] =
  {
    { x0, y0 },
    { x0, y1 },
    { x1, y0 },
    { x1, y1 },

    { x1, y0 },
    { x2, y0 },
    { x1, y1 },
    { x2, y1 },

    { x2, y0 },
    { x3, y0 },
    { x2, y1 },
    { x3, y1 },

    { x3, y0 },
    { x4, y0 },
    { x3, y1 },
    { x4, y1 },

    { x4, y0 },
    { x5, y0 },
    { x4, y1 },
    { x5, y1 },

    { x5, y0 },
    { x6, y0 },
    { x5, y1 },
    { x6, y1 },
  };

  idx_t box_vert_len = 24;


  static u32 box_idxs[] =
  {
    0, 1, 2,
    1, 2, 3,

    4, 5, 6,
    5, 6, 7,

    8, 9, 10,
    9, 10, 11,

    12, 13, 14,
    13, 14, 15,

    16, 17, 18,
    17, 18, 19,

    20, 21, 22,
    21, 22, 23,
  };
  //idx_t box_idxs_len = 36;
  idx_t box_idxs_len = 30;


  mesh_t mesh_box;
  mesh_box.texture = &box_tex;
  mesh_box.positions = box_positions;
  mesh_box.texcoords = box_texcoords;
  mesh_box.vert_len = box_vert_len;
  mesh_box.idxs = box_idxs;
  mesh_box.idxs_len = box_idxs_len;
  Identity( &mesh_box.rotation_world_from_model );
  Identity( &mesh_box.rotation_model_from_world );
  mesh_box.scale_world_from_model = 15.0f;
  mesh_box.scale_model_from_world = 1 / mesh_box.scale_world_from_model;
  mesh_box.translation = {};


  mesh_t mesh_light;
  mesh_light.texture = &app.white_tex;
  mesh_light.positions = box_positions;
  mesh_light.texcoords = box_texcoords;
  mesh_light.vert_len = box_vert_len;
  mesh_light.idxs = box_idxs;
  mesh_light.idxs_len = box_idxs_len;
  Identity( &mesh_light.rotation_world_from_model );
  Identity( &mesh_light.rotation_model_from_world );
  mesh_light.scale_world_from_model = 1.0f;
  mesh_light.scale_model_from_world = 1 / mesh_light.scale_world_from_model;
  mesh_light.translation = _vec3<f32>( -8, 8, 0 );


  static vec4<f32> tri_positions[] =
  {
    { -1.0f, 0.0f,  0.0f, 1.0f },
    {  1.0f, 0.0f,  0.0f, 1.0f },
    {  0.0f, 1.0f,  0.0f, 1.0f },
  };
  static vec2<f32> tri_texcoords[] =
  {
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 0.5f, 1.0f },
  };
  idx_t tri_vert_len = 3;

  static u32 tri_idxs[] =
  {
    0, 1, 2,
  };
  idx_t tri_idxs_len = 3;

  mesh_t mesh_tri;
  mesh_tri.texture = &app.checker_tex;
  mesh_tri.positions = tri_positions;
  mesh_tri.texcoords = tri_texcoords;
  mesh_tri.vert_len = tri_vert_len;
  mesh_tri.idxs = tri_idxs;
  mesh_tri.idxs_len = tri_idxs_len;
  Identity( &mesh_tri.rotation_world_from_model );
  Identity( &mesh_tri.rotation_model_from_world );
  mesh_tri.scale_world_from_model = 12;
  mesh_tri.scale_model_from_world = 1 / mesh_tri.scale_world_from_model;
  mesh_tri.translation = _vec3<f32>( 0, 0, -8 );


  static vec4<f32> quad_positions[] =
  {
    { -1.0f, -1.0f,  0.0f, 1.0f },
    {  1.0f, -1.0f,  0.0f, 1.0f },
    {  1.0f,  1.0f,  0.0f, 1.0f },
    { -1.0f,  1.0f,  0.0f, 1.0f },
  };
  static vec2<f32> quad_texcoords[] =
  {
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 1.0f, 1.0f },
    { 0.0f, 1.0f },
  };
  idx_t quad_vert_len = 4;

  static u32 quad_idxs[] =
  {
    0, 1, 2,
    2, 3, 0,
  };
  idx_t quad_idxs_len = 6;

  mesh_t mesh_quad;
  mesh_quad.texture = &box_tex;
  mesh_quad.positions = quad_positions;
  mesh_quad.texcoords = quad_texcoords;
  mesh_quad.vert_len = quad_vert_len;
  mesh_quad.idxs = quad_idxs;
  mesh_quad.idxs_len = quad_idxs_len;
  Identity( &mesh_quad.rotation_world_from_model );
  Identity( &mesh_quad.rotation_model_from_world );
  mesh_quad.scale_world_from_model = 1;
  mesh_quad.scale_model_from_world = 1 / mesh_quad.scale_world_from_model;
  mesh_quad.translation = _vec3<f32>( 0, 0, -0.5f );


  Alloc( app.meshes, 16 );
#if 1
  *AddBack( app.meshes ) = mesh_tri;
  *AddBack( app.meshes ) = mesh_box;
  *AddBack( app.meshes ) = mesh_light;
#else
  *AddBack( app.meshes ) = mesh_quad;
#endif

  // TODO: de-global this!

  // makes accessible the statics:
  //   rtmeshes*[]
  //   rtmeshes_len
  InitScene();

  Alloc( app.lights, rtmeshes_len );
  For( i, 0, rtmeshes_len ) {
    auto& mesh = *rtmeshes[i];
    if( ComponentSum( mesh.radiance_emit ) > 0 ) {
      auto pmesh = &mesh;
      *AddBack( app.lights ) = pmesh;
    }
  }

  AssertWarn( app.lights.len );
  printf( "num lights: %llu\n", app.lights.len );
}

Inl void
AppKill( app_t& app )
{
  Free( app.target_dep4 );
  Free( app.target_img4 );
  Free( app.target_img16 );

  Kill( app.raster_tiles );

  ShaderKill( app.shader );
  glDeleteBuffers( 1, &app.glstream );
  Free( app.stream );

  Free( app.white_tex );
  Free( app.checker_tex );
  Free( app.meshes );
}


Inl mat4x4r<f32>
GetRotation( app_t& app )
{
  mat4x4r<f32> rotation;
  Prof( matrix_rotation );
  mat4x4r<f32> rotation_x, rotation_y, rotation_z;
  RotateX( &rotation_x, app.rot.x );
  RotateY( &rotation_y, app.rot.y );
  RotateZ( &rotation_z, app.rot.z );
  mat4x4r<f32> rotation_xy;
  Mul( &rotation_xy, rotation_x, rotation_y );
  Mul( &rotation, rotation_z, rotation_xy );
  return rotation;
}


Inl bool
IsTarget4( app_t& app )
{
  bool r = ( app.rendermode != rendermode_t::ray );
  return r;
}


__OnRender( AppOnRender )
{
  ProfFunc();
  auto& app = *Cast( app_t*, misc );

  // always invalidate.
  target_valid = 0;

  app.max_upsize_factor = Cast( u32, 0.25f * MIN( dim.x, dim.y ) );
  app.min_upsize_factor = 1;

  auto timestep = MIN( timestep_realtime, timestep_fixed );
  app.time_anim = Remainder32( app.time_anim + Cast( f32, timestep ), app.time_anim_period );

  u32 window_x = Round_u32_from_f32( dim.x );
  u32 window_y = Round_u32_from_f32( dim.y );

  if( IsTarget4( app ) ) {
    app.reshape_required |= ( window_x != app.target_img4.x );
    app.reshape_required |= ( window_y != app.target_img4.y );
  } else {
    app.reshape_required |= ( window_x != app.target_img16.x );
    app.reshape_required |= ( window_y != app.target_img16.y );
  }
  if( app.reshape_required ) {
    app.reshape_required = 0;

    Prof( rendertarget_reshape );

    if( !IsTarget4( app ) ) {
      Free( app.target_img16 );
      u32 new_stride_x = RoundUpToMultipleOf4( window_x / app.upsize_factor );
      Init( app.target_img16, window_x / app.upsize_factor, window_y / app.upsize_factor, new_stride_x, 16 );
      Alloc( app.target_img16 );

      printf( " reshape done: glw=( %u, %u ), target=( %u, %u, [%u] )\n", window_x, window_y, app.target_img16.x, app.target_img16.y, app.target_img16.stride_x );

    } else {
      Free( app.target_dep4 );
      Free( app.target_img4 );
      u32 new_stride_x = RoundUpToMultipleOf4( window_x );
      Init( app.target_img4, window_x, window_y, new_stride_x, 4 );
      Init( app.target_dep4, window_x, window_y, new_stride_x, 4 );
      Alloc( app.target_img4 );
      Alloc( app.target_dep4 );

      Kill( app.raster_tiles );
      Init( app.raster_tiles, app.target_img4.x, app.target_img4.y, app.target_img4.stride_x, app.raster_tile_x, app.raster_tile_y, app.raster_tris_per_tile );

      printf( " reshape done: glw=( %u, %u ), target=( %u, %u, [%u] )\n", window_x, window_y, app.target_img4.x, app.target_img4.y, app.target_img4.stride_x );
    }

    app.upsize_factor = CLAMP( app.upsize_factor, app.min_upsize_factor, app.max_upsize_factor );
  }

  if( !app.initialized ) {
    app.initialized = 1;

    Prof( glw_upload_initial_img );

    idx_t nbytes_upload = 4 * window_x * window_y;
    u8* write_mem = MemHeapAlloc( u8, nbytes_upload );

    if( !IsTarget4( app ) ) {
      ConvertImageUpsize( write_mem, window_x, window_y, app.target_img16, app.upsize_factor );
    } else {
      ConvertImage( write_mem, app.target_img4 );
    }

    GlwUploadTexture(
      app.client,
      app.texid_target,
      write_mem,
      nbytes_upload,
      window_x,
      window_y
      );

    MemHeapFree( write_mem );
  }

  static bool g_do_edge_blurring = 0;
//  if( KeyUp( client, glwkey_t::b ) ) {
//    g_do_edge_blurring = !g_do_edge_blurring;
//  }


  if( !IsTarget4( app ) ) {
    f32 camera_half_x = app.camera_half_y * app.target_img16.aspect_ratio;

    Prof( target_clear );

    __m128 color_clear = _mm_setr_ps( 0, 0, 0, 1.0f );
    StorePx16B( app.target_img16, color_clear );

    ProfClose( target_clear );

    vec3<f32> translate_world_from_camera = app.pos;
    mat3x3r<f32> rotation_camera_from_world;
    {
      Prof( matrix_rotation );
      mat3x3r<f32> rotation_x, rotation_y, rotation_z;
      RotateX( &rotation_x, app.rot.x );
      RotateY( &rotation_y, app.rot.y );
      RotateZ( &rotation_z, app.rot.z );
      mat3x3r<f32> rotation_xy;
      Mul( &rotation_xy, rotation_x, rotation_y );
      Mul( &rotation_camera_from_world, rotation_z, rotation_xy );
    }
    mat3x3r<f32> rotation_world_from_camera = rotation_camera_from_world;
    Transpose( &rotation_world_from_camera );

    //Metropolis(
    Raytrace(
      app.target_img16,
      app.rt_rng,
      app.samples_per_pixel,
      translate_world_from_camera,
      rotation_world_from_camera,
      camera_half_x,
      app.camera_half_y,
      app.camera_near_z,
      rtmeshes,
      rtmeshes_len,
      app.lights.mem,
      app.lights.len
      );

    if( g_do_edge_blurring ) {
      BlurEdges( app.target_img16 );
    }

  } else {
    f32 camera_half_x = app.camera_half_y * app.target_img4.aspect_ratio;

    mat4x4r<f32> rotation = GetRotation( app );

    mat4x4r<f32> clip_from_world;
    #if 1
    {
      //Prof( matrix_clipfromworld );
      mat4x4r<f32> translation;
      vec3<f32> transl = app.pos;
      Mul( &transl, -1.0f );
      Translate( &translation, transl );

      mat4x4r<f32> camera_from_world;
      Mul( &camera_from_world, rotation, translation );

      mat4x4r<f32> projection;
      Frustum(
        &projection,
        -camera_half_x,
        camera_half_x,
        -app.camera_half_y,
        app.camera_half_y,
        app.camera_near_z,
        app.camera_far_z
        );

      Mul( &clip_from_world, projection, camera_from_world );
    }
    #else
    {
      //Ortho( &clip_from_world, 0.0f, app.target_img4.xf, 0.0f, app.target_img4.yf, -1.0f, 1.0f );
      Ortho( &clip_from_world, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f );
    }
    #endif

    // Geometry animation.
    //Set( &tri_positions[0], -5.0f * Cos( time_anim ), 0.0f, -5.0f * Sin( time_anim ), 1.0f );
    //Set( &tri_positions[1],  5.0f * Cos( time_anim ), 0.0f,  5.0f * Sin( time_anim ), 1.0f );

    //printf( "dt(ms): %f\n", 1000 * loop_dt );


    Prof( target_clear );

    __m128 depth_clear = _mm_set1_ps( MIN_f32 );
    Store4Px4B( app.target_dep4, depth_clear );

    __m128i color_clear = _mm_set1_epi32( 0x000000FF );
    Store4Px4Bi( app.target_img4, color_clear );

    ProfClose( target_clear );

    Render(
      app.target_img4,
      app.target_dep4,
      app.raster_tiles,
      clip_from_world,
      app.meshes.mem,
      app.meshes.len,
      app.rendermode
      );

    if( g_do_edge_blurring ) {
      BlurEdges( app.target_img4 );
    }
  }

  Prof( glw_update_img );
  idx_t nbytes_update = 4 * window_x * window_y;
  auto write_mem = MemHeapAlloc( u8, nbytes_update );

  if( !IsTarget4( app ) ) {
    ConvertImageUpsize( write_mem, window_x, window_y, app.target_img16, app.upsize_factor );
  } else {
    ConvertImage( write_mem, app.target_img4 );
  }

//  int res = img_write_png(
//    Cast( char*, filename ),
//    target.img.x,
//    target.img.y,
//    4, // #components
//    write_mem, // data
//    0 ); // write_stride
//  AssertWarn( res );

  GlwUpdateTexture(
    app.client,
    app.texid_target,
    write_mem,
    nbytes_update,
    window_x,
    window_y
    );

  MemHeapFree( write_mem );

  auto color = _vec4<f32>( 1, 1, 1, 1 );
  RenderTQuad(
    app.stream,
    color,
    origin,
    origin + dim,
    _vec2<f32>( 1, 0 ),
    _vec2<f32>( 0, 1 ),
    origin,
    origin + dim,
    0.0f
    );

  ProfClose( glw_update_img );

  if( app.stream.len ) {
    glUseProgram( app.shader.core.program );  glVerify();

    // we invert y, since we layout from the top-left as the origin.
    mat4x4r<f32> ndc_from_client;
    Ortho( &ndc_from_client, 0.0f, dim.x, dim.y, 0.0f, 0.0f, 1.0f );
    glUniformMatrix4fv( app.shader.loc_ndc_from_client, 1, 1, &ndc_from_client.row0.x );  glVerify();

    glUniform1i( app.shader.loc_tex_sampler, 0 );  glVerify();
    GlwBindTexture( 0, GlwLookupGlid( app.client, app.texid_target ) );

    glBindBuffer( GL_ARRAY_BUFFER, app.glstream );
    glBufferData( GL_ARRAY_BUFFER, app.stream.len * sizeof( f32 ), app.stream.mem, GL_STREAM_DRAW );  glVerify();
    glEnableVertexAttribArray( app.shader.attribloc_pos );  glVerify();
    glEnableVertexAttribArray( app.shader.attribloc_tccolor );  glVerify();
    glVertexAttribPointer( app.shader.attribloc_pos, 3, GL_FLOAT, 0, 6 * sizeof( f32 ), 0 );  glVerify();
    glVertexAttribPointer( app.shader.attribloc_tccolor, 3, GL_FLOAT, 0, 6 * sizeof( f32 ), Cast( void*, 3 * sizeof( f32 ) ) );  glVerify();

    auto vert_count = app.stream.len / 6;
    AssertCrash( app.stream.len % 6 == 0 );
    AssertCrash( vert_count <= MAX_s32 );
    glDrawArrays( GL_TRIANGLES, 0, Cast( s32, vert_count ) );  glVerify();

    glDisableVertexAttribArray( app.shader.attribloc_pos );  glVerify();
    glDisableVertexAttribArray( app.shader.attribloc_tccolor );  glVerify();

    //printf( "bytes sent to gpu: %llu\n", sizeof( f32 ) * app.stream.len );

    app.stream.len = 0;
  }
}

__OnKeyEvent( AppOnKeyEvent )
{
  ProfFunc();
  auto& app = *Cast( app_t*, misc );

  // always invalidate.
  target_valid = 0;

  mat4x4r<f32> rotation = GetRotation( app );
  vec3<f32> cam_axis[3];
  For( i, 0, 3 ) {
    auto& row = rotation.rows[i];
    cam_axis[i] = _vec3( row.x, row.y, row.z );
  }
  vec3<f32> motion = {};

  switch( type ) {
    case glwkeyevent_t::dn:
    case glwkeyevent_t::repeat: {
      switch( key ) {
        case glwkey_t::f: { motion -= cam_axis[0]; } break;
        case glwkey_t::s: { motion += cam_axis[0]; } break;
        case glwkey_t::g: { motion += cam_axis[1]; } break;
        case glwkey_t::t: { motion -= cam_axis[1]; } break;
        case glwkey_t::d: { motion += cam_axis[2]; } break;
        case glwkey_t::e: { motion -= cam_axis[2]; } break;
      }
    } break;

    case glwkeyevent_t::up: {
      switch( key ) {
#if 1
        case glwkey_t::fn_1: {
          GlwEarlyKill( app.client );
        } break;
#endif

        case glwkey_t::fn_11: {
          app.fullscreen = !app.fullscreen;
          fullscreen = app.fullscreen;
          app.reshape_required = 1;
        } break;

        case glwkey_t::brace_r: {
          app.upsize_factor = MIN( app.upsize_factor + 1, app.max_upsize_factor );
          app.reshape_required = 1;
          printf( "upsize_factor: %u\n", app.upsize_factor );
        } break;

        case glwkey_t::brace_l: {
          app.upsize_factor = MAX( app.upsize_factor - 1, app.min_upsize_factor );
          app.reshape_required = 1;
          printf( "upsize_factor: %u\n", app.upsize_factor );
        } break;

        case glwkey_t::minus: {
          app.samples_per_pixel *= 2;
          app.reshape_required = 1;
          printf( "samples_per_pixel: %u\n", app.samples_per_pixel );
        } break;

        case glwkey_t::equals: {
          app.samples_per_pixel = MAX( app.samples_per_pixel, 2 ) / 2;
          app.reshape_required = 1;
          printf( "samples_per_pixel: %u\n", app.samples_per_pixel );
        } break;

        case glwkey_t::num_1: {
          app.rendermode = rendermode_t::ref;
          app.reshape_required = 1;
          printf( "rendermode: ref\n" );
        } break;

        case glwkey_t::num_2: {
          app.rendermode = rendermode_t::opt;
          app.reshape_required = 1;
          printf( "rendermode: opt\n" );
        } break;

        case glwkey_t::num_3: {
          app.rendermode = rendermode_t::simd;
          app.reshape_required = 1;
          printf( "rendermode: simd\n" );
        } break;

        case glwkey_t::num_4: {
          app.rendermode = rendermode_t::ray;
          app.reshape_required = 1;
          printf( "rendermode: ray\n" );
        } break;
      }
    } break;

    default: UnreachableCrash();
  }

  if( Squared( motion ) > 0 ) {
    Normalize( &motion );
  }

  // TODO: pass dt_render_last ?
  f32 vel = 40;
  motion *= 0.01666f * vel;

  app.pos += motion;
}

__OnMouseEvent( AppOnMouseEvent )
{
  ProfFunc();
  auto& app = *Cast( app_t*, misc );

  // always invalidate.
  target_valid = 0;

  // TODO: should this be resolution-independent?
  constant f32 sens_x = 0.001f;
  constant f32 sens_y = 0.001f;
  app.rot.x -= sens_x * raw_delta.y;
  app.rot.y -= sens_y * raw_delta.x;
  app.rot.x = CLAMP( app.rot.x, -0.5f * f32_PI, 0.5f * f32_PI );
  app.rot.y = Remainder32( app.rot.y, f32_2PI );

  //printf( "( %f, %f, %f ) ( %f, %f )\n", app.pos.x, app.pos.y, app.pos.z, app.rot.x, app.rot.y );
  //app.rot = { -0.666, -0.141 };
  //app.pos = { 3.151122, -2.279725, 2.588225 };
}


int
Main( u8* cmdline, idx_t cmdline_len )
{
//  u8* filename = Str( "c:/doc/dev/cpp/master/rt/output.png" );

  app_t app;
  AppInit( app );

  GlwSetSwapInterval( app.client, 0 );

  glwcallback_t callbacks[] = {
    { glwcallbacktype_t::keyevent    , &app, AppOnKeyEvent    },
    { glwcallbacktype_t::mouseevent  , &app, AppOnMouseEvent  },
    { glwcallbacktype_t::render      , &app, AppOnRender      },
  };
  ForEach( callback, callbacks ) {
    GlwRegisterCallback( app.client, callback );
  }

  GlwMainLoop( app.client );

  // Do this before destroying any datastructures, so other threads stop trying to access things.
  SignalQuitAndWaitForTaskThreads();

  AppKill( app );

  GlwKillWindow( app.client );
  GlwKill();

  return 0;
}




#ifdef _DEBUG

int
main( int argc, char** argv )
{
  MainInit();

  stack_resizeable_cont_t<u8> cmdline;
  Alloc( cmdline, 512 );
  Fori( int, i, 1, argc ) {
    u8* arg = Cast( u8*, argv[i] );
    idx_t arg_len = CstrLength( arg );
    Memmove( AddBack( cmdline, arg_len ), arg, arg_len );
    Memmove( AddBack( cmdline, 2 ), " ", 2 );
  }
  int r = Main( ML( cmdline ) );
  Free( cmdline );

  printf( "Main returned: %d\n", r );
  system( "pause" );

  MainKill();
  return r;
}

#else

int WINAPI
WinMain( HINSTANCE prog_inst, HINSTANCE prog_inst_prev, LPSTR prog_cmd_line, int prog_cmd_show )
{
  MainInit();

  u8* cmdline = Str( prog_cmd_line );
  idx_t cmdline_len = CstrLength( Str( prog_cmd_line ) );

  AllocConsole();
  FILE* ignored = 0;
  freopen_s( &ignored, "CONOUT$", "wb", stdout );
  freopen_s( &ignored, "CONOUT$", "wb", stderr );

  int r = Main( cmdline, cmdline_len );

  printf( "Main returned: %d\n", r );
  system( "pause" );

  MainKill();
  return r;
}

#endif












// Pixel drawing test:


//  u32 width = img.x;
//  u32 quarter_w = width / 4;
//  u32 half_w = width / 2;
//  u32 threequarter_w = 3 * width / 4;
//
//  u32 height = img.y;
//  u32 quarter_h = height / 4;
//  u32 half_h = height / 2;
//  u32 threequarter_h = 3 * height / 4;
//
//  d256_t d;
//  Set( d, 0, 0, 0, 1 );
//  Set( img, d );
//
//  Set( d, 0, 1, 1, 1 );
//  Set( img, quarter_w + width * half_w, d );
//  Set( img, half_w, half_w, d );
//
//  Set( d, 0, 0, 1, 1 );
//  Rect( img, quarter_w, quarter_h, threequarter_w, threequarter_h, d );
//
//  Set( d, 1, 0, 0, 1 );
//  Rect( img, half_w, half_h, threequarter_w, threequarter_h, d );
//
//  Set( d, 1, 1, 1, 1 );
//  Line( img, quarter_w, threequarter_h, width, height, d );
//  Line( img, quarter_w, half_h, threequarter_w, quarter_h, d );
//
//  Set( d, 0, 0.5, 0, 1 );
//  Tri(
//    img,
//    10, 10,
//    50, 100,
//    100, 50,
//    d );
//
//  Set( d, 1, 1, 1, 1 );
//  TriOutline(
//    img,
//    10, 10,
//    50, 100,
//    100, 50,
//    d );
//
//  d256_t d0, d1, d2;
//  Set( d0, 0, 0, 1, 1 );
//  Set( d1, 0, 1, 0, 1 );
//  Set( d2, 1, 0, 0, 1 );
//
//  LineHorizontalGouraud( img, 100, 200, 100, d0, d1 );
//  LineHorizontalGouraud( img, 100, 200, 101, d0, d1 );
//  LineHorizontalGouraud( img, 100, 200, 102, d0, d1 );
//  LineHorizontalGouraud( img, 100, 200, 103, d0, d1 );
//
//  TriGouraud(
//    img,
//    100, 10, d0,
//    300, 50, d1,
//    200, 100, d2 );

#endif // WIN
