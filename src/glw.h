// Copyright (c) John A. Carlos Jr., all rights reserved.

// TODO: we basically need to rewrite this for MAC.

// COMPILE-TIME OPTIONS

#if !defined(OPENGL_INSTEAD_OF_SOFTWARE)
#error make a choice for OPENGL_INSTEAD_OF_SOFTWARE; either 0 or 1.
#endif


#include "glw_font_stb_truetype.h"

Enumc( glwkey_t );

struct
glwkeybind_t
{
  glwkey_t key;
  glwkey_t alreadydn;
  glwkey_t alreadydn2;
  glwkey_t conflict;
  glwkey_t conflict2;
  glwkey_t conflict3;
};



#if defined(WIN)

  #define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
  #define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

  #if OPENGL_INSTEAD_OF_SOFTWARE

    #include <gl/gl.h>
    #include "glw_glext.h"
    #include "glw_wglext.h"

    #pragma comment( lib, "opengl32" )

    #define glVerify() \
      do { \
        u32 err = glGetError(); \
        AssertWarn( !err ); \
      } while( 0 )

  #endif

#endif





// ==============================================================================
//
// WINDOWING / INPUT INTERFACE:
//

//   name, val, win_MK_val,  win_VK_val
#define MOUSEBTNMAP( _x ) \
  _x( none, 0,           0,           0 ) \
  _x( l   , 1, MK_LBUTTON , VK_LBUTTON  ) \
  _x( r   , 2, MK_RBUTTON , VK_RBUTTON  ) \
  _x( m   , 3, MK_MBUTTON , VK_MBUTTON  ) \
  _x( b4  , 4, MK_XBUTTON1, VK_XBUTTON1 ) \
  _x( b5  , 5, MK_XBUTTON2, VK_XBUTTON2 ) \
  
Enumc( glwmousebtn_t )
{
#define ENTRY( _name, _val, _win_mk, _win_vk ) _name = _val,
MOUSEBTNMAP( ENTRY )
#undef ENTRY

  count
};
CompileAssert( Cast( enum_t, glwmousebtn_t::count ) == 6 );

Inl slice_t
SliceFromMouseBtn( glwmousebtn_t t )
{
  switch( t ) {
    #define ENTRY( _name, _val, _win_mk, _win_vk )   case glwmousebtn_t::_name: return SliceFromCStr( #_name );
    MOUSEBTNMAP( ENTRY )
    #undef ENTRY
    default: UnreachableCrash(); return {};
  }
}

#if defined(WIN)
  u32 c_mk_flags[] = {
    #define ENTRY( _name, _val, _win_mk, _win_vk )   _win_mk,
    MOUSEBTNMAP( ENTRY )
    #undef ENTRY
    };
  CompileAssert( _countof( c_mk_flags ) == Cast( enum_t, glwmousebtn_t::count ) );

  int c_vks[] = {
    #define ENTRY( _name, _val, _win_mk, _win_vk )   _win_vk,
    MOUSEBTNMAP( ENTRY )
    #undef ENTRY
    };
  CompileAssert( _countof( c_vks ) == Cast( enum_t, glwmousebtn_t::count ) );
#endif

struct
glwkeylocks_t
{
  bool caps;
  bool num;
  bool scroll;
};



#define KEYVALMAP( _x ) \
  _x( none             ,   0 , 0               ) \
  _x( backspace        ,   1 , VK_BACK         ) \
  _x( tab              ,   2 , VK_TAB          ) \
  _x( clear            ,   3 , VK_CLEAR        ) \
  _x( enter            ,   4 , VK_RETURN       ) \
  _x( shift            ,   5 , VK_SHIFT        ) \
  _x( ctrl             ,   6 , VK_CONTROL      ) \
  _x( alt              ,   7 , VK_MENU         ) \
  _x( pause            ,   8 , VK_PAUSE        ) \
  _x( capslock         ,   9 , VK_CAPITAL      ) \
  _x( esc              ,  10 , VK_ESCAPE       ) \
  _x( space            ,  11 , VK_SPACE        ) \
  _x( page_u           ,  12 , VK_PRIOR        ) \
  _x( page_d           ,  13 , VK_NEXT         ) \
  _x( end              ,  14 , VK_END          ) \
  _x( home             ,  15 , VK_HOME         ) \
  _x( arrow_l          ,  16 , VK_LEFT         ) \
  _x( arrow_u          ,  17 , VK_UP           ) \
  _x( arrow_r          ,  18 , VK_RIGHT        ) \
  _x( arrow_d          ,  19 , VK_DOWN         ) \
  _x( select           ,  20 , VK_SELECT       ) \
  _x( print            ,  21 , VK_PRINT        ) \
  _x( printscrn        ,  22 , VK_SNAPSHOT     ) \
  _x( insert           ,  23 , VK_INSERT       ) \
  _x( del              ,  24 , VK_DELETE       ) \
  _x( help             ,  25 , VK_HELP         ) \
  _x( numlock          ,  26 , VK_NUMLOCK      ) \
  _x( scrolllock       ,  27 , VK_SCROLL       ) \
  _x( shift_l          ,  28 , VK_LSHIFT       ) \
  _x( shift_r          ,  29 , VK_RSHIFT       ) \
  _x( ctrl_l           ,  30 , VK_LCONTROL     ) \
  _x( ctrl_r           ,  31 , VK_RCONTROL     ) \
  _x( alt_l            ,  32 , VK_LMENU        ) \
  _x( alt_r            ,  33 , VK_RMENU        ) \
  _x( volume_mute      ,  34 , VK_VOLUME_MUTE  ) \
  _x( volume_u         ,  35 , VK_VOLUME_UP    ) \
  _x( volume_d         ,  36 , VK_VOLUME_DOWN  ) \
  _x( semicolon        ,  37 , VK_OEM_1        ) \
  _x( equals           ,  38 , VK_OEM_PLUS     ) \
  _x( minus            ,  39 , VK_OEM_MINUS    ) \
  _x( comma            ,  40 , VK_OEM_COMMA    ) \
  _x( period           ,  41 , VK_OEM_PERIOD   ) \
  _x( slash_forw       ,  42 , VK_OEM_2        ) \
  _x( slash_back       ,  43 , VK_OEM_5        ) \
  _x( tilde            ,  44 , VK_OEM_3        ) \
  _x( brace_l          ,  45 , VK_OEM_4        ) \
  _x( brace_r          ,  46 , VK_OEM_6        ) \
  _x( quote            ,  47 , VK_OEM_7        ) \
  _x( num_0            ,  48 , 0x30            ) \
  _x( num_1            ,  49 , 0x31            ) \
  _x( num_2            ,  50 , 0x32            ) \
  _x( num_3            ,  51 , 0x33            ) \
  _x( num_4            ,  52 , 0x34            ) \
  _x( num_5            ,  53 , 0x35            ) \
  _x( num_6            ,  54 , 0x36            ) \
  _x( num_7            ,  55 , 0x37            ) \
  _x( num_8            ,  56 , 0x38            ) \
  _x( num_9            ,  57 , 0x39            ) \
  _x( a                ,  58 , 0x41            ) \
  _x( b                ,  59 , 0x42            ) \
  _x( c                ,  60 , 0x43            ) \
  _x( d                ,  61 , 0x44            ) \
  _x( e                ,  62 , 0x45            ) \
  _x( f                ,  63 , 0x46            ) \
  _x( g                ,  64 , 0x47            ) \
  _x( h                ,  65 , 0x48            ) \
  _x( i                ,  66 , 0x49            ) \
  _x( j                ,  67 , 0x4A            ) \
  _x( k                ,  68 , 0x4B            ) \
  _x( l                ,  69 , 0x4C            ) \
  _x( m                ,  70 , 0x4D            ) \
  _x( n                ,  71 , 0x4E            ) \
  _x( o                ,  72 , 0x4F            ) \
  _x( p                ,  73 , 0x50            ) \
  _x( q                ,  74 , 0x51            ) \
  _x( r                ,  75 , 0x52            ) \
  _x( s                ,  76 , 0x53            ) \
  _x( t                ,  77 , 0x54            ) \
  _x( u                ,  78 , 0x55            ) \
  _x( v                ,  79 , 0x56            ) \
  _x( w                ,  80 , 0x57            ) \
  _x( x                ,  81 , 0x58            ) \
  _x( y                ,  82 , 0x59            ) \
  _x( z                ,  83 , 0x5A            ) \
  _x( numpad_0         ,  84 , VK_NUMPAD0      ) \
  _x( numpad_1         ,  85 , VK_NUMPAD1      ) \
  _x( numpad_2         ,  86 , VK_NUMPAD2      ) \
  _x( numpad_3         ,  87 , VK_NUMPAD3      ) \
  _x( numpad_4         ,  88 , VK_NUMPAD4      ) \
  _x( numpad_5         ,  89 , VK_NUMPAD5      ) \
  _x( numpad_6         ,  90 , VK_NUMPAD6      ) \
  _x( numpad_7         ,  91 , VK_NUMPAD7      ) \
  _x( numpad_8         ,  92 , VK_NUMPAD8      ) \
  _x( numpad_9         ,  93 , VK_NUMPAD9      ) \
  _x( numpad_mul       ,  94 , VK_MULTIPLY     ) \
  _x( numpad_add       ,  95 , VK_ADD          ) \
  _x( numpad_sub       ,  96 , VK_SUBTRACT     ) \
  _x( numpad_decimal   ,  97 , VK_DECIMAL      ) \
  _x( numpad_div       ,  98 , VK_DIVIDE       ) \
  _x( fn_1             ,  99 , VK_F1           ) \
  _x( fn_2             , 100 , VK_F2           ) \
  _x( fn_3             , 101 , VK_F3           ) \
  _x( fn_4             , 102 , VK_F4           ) \
  _x( fn_5             , 103 , VK_F5           ) \
  _x( fn_6             , 104 , VK_F6           ) \
  _x( fn_7             , 105 , VK_F7           ) \
  _x( fn_8             , 106 , VK_F8           ) \
  _x( fn_9             , 107 , VK_F9           ) \
  _x( fn_10            , 108 , VK_F10          ) \
  _x( fn_11            , 109 , VK_F11          ) \
  _x( fn_12            , 110 , VK_F12          ) \
  _x( count            , 111 , 0               ) \

Enumc( glwkey_t )
{
  #define ENTRY( name, value, win_value )   name = value,
  KEYVALMAP( ENTRY )
  #undef ENTRY
};


#if OPENGL_INSTEAD_OF_SOFTWARE

  #define D_GLPROC( pfunc_type, func )   pfunc_type func = {};

  D_GLPROC( PFNWGLGETEXTENSIONSSTRINGARBPROC , wglGetExtensionsStringARB  )
  D_GLPROC( PFNWGLSWAPINTERVALEXTPROC        , wglSwapIntervalEXT         )
  D_GLPROC( PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB )

  D_GLPROC( PFNGLGENFRAMEBUFFERSPROC         , glGenFramebuffers          )
  D_GLPROC( PFNGLDELETEFRAMEBUFFERSPROC      , glDeleteFramebuffers       )
  D_GLPROC( PFNGLBINDFRAMEBUFFERPROC         , glBindFramebuffer          )
  D_GLPROC( PFNGLGENRENDERBUFFERSPROC        , glGenRenderbuffers         )
  D_GLPROC( PFNGLDELETERENDERBUFFERSPROC     , glDeleteRenderbuffers      )
  D_GLPROC( PFNGLBINDRENDERBUFFERPROC        , glBindRenderbuffer         )
  D_GLPROC( PFNGLRENDERBUFFERSTORAGEPROC     , glRenderbufferStorage      )
  D_GLPROC( PFNGLFRAMEBUFFERRENDERBUFFERPROC , glFramebufferRenderbuffer  )
  D_GLPROC( PFNGLFRAMEBUFFERTEXTURE2DPROC    , glFramebufferTexture2D     )
  D_GLPROC( PFNGLDRAWBUFFERSPROC             , glDrawBuffers              )
  D_GLPROC( PFNGLCHECKFRAMEBUFFERSTATUSPROC  , glCheckFramebufferStatus   )
  D_GLPROC( PFNGLCREATEPROGRAMPROC           , glCreateProgram            )
  D_GLPROC( PFNGLCREATESHADERPROC            , glCreateShader             )
  D_GLPROC( PFNGLGETUNIFORMLOCATIONPROC      , glGetUniformLocation       )
  D_GLPROC( PFNGLSHADERSOURCEPROC            , glShaderSource             )
  D_GLPROC( PFNGLCOMPILESHADERPROC           , glCompileShader            )
  D_GLPROC( PFNGLATTACHSHADERPROC            , glAttachShader             )
  D_GLPROC( PFNGLLINKPROGRAMPROC             , glLinkProgram              )
  D_GLPROC( PFNGLUSEPROGRAMPROC              , glUseProgram               )
  D_GLPROC( PFNGLVALIDATEPROGRAMPROC         , glValidateProgram          )
  D_GLPROC( PFNGLUNIFORMMATRIX4FVPROC        , glUniformMatrix4fv         )
  D_GLPROC( PFNGLUNIFORM1IPROC               , glUniform1i                )
  D_GLPROC( PFNGLUNIFORM4FVPROC              , glUniform4fv               )
  D_GLPROC( PFNGLGETPROGRAMIVPROC            , glGetProgramiv             )
  D_GLPROC( PFNGLGETPROGRAMINFOLOGPROC       , glGetProgramInfoLog        )
  D_GLPROC( PFNGLGENBUFFERSPROC              , glGenBuffers               )
  D_GLPROC( PFNGLBINDBUFFERPROC              , glBindBuffer               )
  D_GLPROC( PFNGLBUFFERDATAPROC              , glBufferData               )
  D_GLPROC( PFNGLENABLEVERTEXATTRIBARRAYPROC , glEnableVertexAttribArray  )
  D_GLPROC( PFNGLVERTEXATTRIBPOINTERPROC     , glVertexAttribPointer      )
  D_GLPROC( PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray )
  D_GLPROC( PFNGLDELETEBUFFERSPROC           , glDeleteBuffers            )
  D_GLPROC( PFNGLGETATTRIBLOCATIONPROC       , glGetAttribLocation        )
  D_GLPROC( PFNGLGENVERTEXARRAYSPROC         , glGenVertexArrays          )
  D_GLPROC( PFNGLBINDVERTEXARRAYPROC         , glBindVertexArray          )
  D_GLPROC( PFNGLDELETEVERTEXARRAYSPROC      , glDeleteVertexArrays       )
  D_GLPROC( PFNGLACTIVETEXTUREPROC           , glActiveTexture            )
  D_GLPROC( PFNGLGENERATEMIPMAPPROC          , glGenerateMipmap           )
  D_GLPROC( PFNGLDELETESHADERPROC            , glDeleteShader             )
  D_GLPROC( PFNGLDELETEPROGRAMPROC           , glDeleteProgram            )
  D_GLPROC( PFNGLGETSHADERIVPROC             , glGetShaderiv              )
  D_GLPROC( PFNGLGETSHADERINFOLOGPROC        , glGetShaderInfoLog         )

  #undef D_GLPROC

  #define L_GLPROC( pfunc_type, func ) \
    do { \
      func = Cast( pfunc_type, wglGetProcAddress( # func ) ); \
    } while( 0 );


  void
  LoadWGLFunctions( HDC dc )
  {
    ProfFunc();
    // load wgl functions from a dummy context.
    HGLRC dummy_context = wglCreateContext( dc );
    AssertWarn( dummy_context );
    AssertWarn( wglMakeCurrent( dc, dummy_context ) );

    L_GLPROC( PFNWGLGETEXTENSIONSSTRINGARBPROC , wglGetExtensionsStringARB  )
    L_GLPROC( PFNWGLSWAPINTERVALEXTPROC        , wglSwapIntervalEXT         )
    L_GLPROC( PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB )

    AssertWarn( wglMakeCurrent( 0, 0 ) );
    AssertWarn( wglDeleteContext( dummy_context ) );
  }

  void
  LoadOpenGLFunctions( HDC dc )
  {
    ProfFunc();
  // load gl functions after we've created the right wgl context
  L_GLPROC( PFNGLGENFRAMEBUFFERSPROC         , glGenFramebuffers          )
  L_GLPROC( PFNGLDELETEFRAMEBUFFERSPROC      , glDeleteFramebuffers       )
  L_GLPROC( PFNGLBINDFRAMEBUFFERPROC         , glBindFramebuffer          )
  L_GLPROC( PFNGLGENRENDERBUFFERSPROC        , glGenRenderbuffers         )
  L_GLPROC( PFNGLDELETERENDERBUFFERSPROC     , glDeleteRenderbuffers      )
  L_GLPROC( PFNGLBINDRENDERBUFFERPROC        , glBindRenderbuffer         )
  L_GLPROC( PFNGLRENDERBUFFERSTORAGEPROC     , glRenderbufferStorage      )
  L_GLPROC( PFNGLFRAMEBUFFERRENDERBUFFERPROC , glFramebufferRenderbuffer  )
  L_GLPROC( PFNGLFRAMEBUFFERTEXTURE2DPROC    , glFramebufferTexture2D     )
  L_GLPROC( PFNGLDRAWBUFFERSPROC             , glDrawBuffers              )
  L_GLPROC( PFNGLCHECKFRAMEBUFFERSTATUSPROC  , glCheckFramebufferStatus   )
  L_GLPROC( PFNGLCREATEPROGRAMPROC           , glCreateProgram            )
  L_GLPROC( PFNGLCREATESHADERPROC            , glCreateShader             )
  L_GLPROC( PFNGLGETUNIFORMLOCATIONPROC      , glGetUniformLocation       )
  L_GLPROC( PFNGLSHADERSOURCEPROC            , glShaderSource             )
  L_GLPROC( PFNGLCOMPILESHADERPROC           , glCompileShader            )
  L_GLPROC( PFNGLATTACHSHADERPROC            , glAttachShader             )
  L_GLPROC( PFNGLLINKPROGRAMPROC             , glLinkProgram              )
  L_GLPROC( PFNGLUSEPROGRAMPROC              , glUseProgram               )
  L_GLPROC( PFNGLVALIDATEPROGRAMPROC         , glValidateProgram          )
  L_GLPROC( PFNGLUNIFORMMATRIX4FVPROC        , glUniformMatrix4fv         )
  L_GLPROC( PFNGLUNIFORM1IPROC               , glUniform1i                )
  L_GLPROC( PFNGLUNIFORM4FVPROC              , glUniform4fv               )
  L_GLPROC( PFNGLGETPROGRAMIVPROC            , glGetProgramiv             )
  L_GLPROC( PFNGLGETPROGRAMINFOLOGPROC       , glGetProgramInfoLog        )
  L_GLPROC( PFNGLGENBUFFERSPROC              , glGenBuffers               )
  L_GLPROC( PFNGLBINDBUFFERPROC              , glBindBuffer               )
  L_GLPROC( PFNGLBUFFERDATAPROC              , glBufferData               )
  L_GLPROC( PFNGLENABLEVERTEXATTRIBARRAYPROC , glEnableVertexAttribArray  )
  L_GLPROC( PFNGLVERTEXATTRIBPOINTERPROC     , glVertexAttribPointer      )
  L_GLPROC( PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray )
  L_GLPROC( PFNGLDELETEBUFFERSPROC           , glDeleteBuffers            )
  L_GLPROC( PFNGLGETATTRIBLOCATIONPROC       , glGetAttribLocation        )
  L_GLPROC( PFNGLGENVERTEXARRAYSPROC         , glGenVertexArrays          )
  L_GLPROC( PFNGLBINDVERTEXARRAYPROC         , glBindVertexArray          )
  L_GLPROC( PFNGLDELETEVERTEXARRAYSPROC      , glDeleteVertexArrays       )
  L_GLPROC( PFNGLACTIVETEXTUREPROC           , glActiveTexture            )
  L_GLPROC( PFNGLGENERATEMIPMAPPROC          , glGenerateMipmap           )
  L_GLPROC( PFNGLDELETESHADERPROC            , glDeleteShader             )
  L_GLPROC( PFNGLDELETEPROGRAMPROC           , glDeleteProgram            )
  L_GLPROC( PFNGLGETSHADERIVPROC             , glGetShaderiv              )
  L_GLPROC( PFNGLGETSHADERINFOLOGPROC        , glGetShaderInfoLog         )
  }

  #undef L_GLPROC

#endif // OPENGL_INSTEAD_OF_SOFTWARE








#if OPENGL_INSTEAD_OF_SOFTWARE

  struct
  glwshader_t
  {
    u32 program;
    u32 vs;
    u32 fs;
  };

  Inl void
  _VerifyShader( u32 shader, char* prefix )
  {
    s32 loglen = 0;
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &loglen );  glVerify();
    if( loglen ) {
      auto log = MemHeapAlloc( char, loglen );
      s32 nwrite = 0;
      glGetShaderInfoLog( shader, loglen, &nwrite, log );  glVerify();
      Log( "%s%s", prefix, log );
      MemHeapFree( log );
      UnreachableCrash();
    }
  }

  Inl void
  _VerifyProgram( u32 program, char* prefix )
  {
    s32 loglen = 0;
    glGetProgramiv( program, GL_INFO_LOG_LENGTH, &loglen );  glVerify();
    if( loglen ) {
      auto log = MemHeapAlloc( char, loglen );
      s32 nwrite = 0;
      glGetProgramInfoLog( program, loglen, &nwrite, log );  glVerify();
      Log( "%s%s", prefix, log );
      MemHeapFree( log );
      UnreachableCrash();
    }
  }

  glwshader_t
  GlwLoadShader( char* vs, char* fs )
  {
    ProfFunc();
    glwshader_t shader;

    shader.vs = glCreateShader( GL_VERTEX_SHADER );  glVerify();
    shader.fs = glCreateShader( GL_FRAGMENT_SHADER );  glVerify();
    s32 vs_len = Cast( s32, CstrLength( Cast( u8*, vs ) ) );
    s32 fs_len = Cast( s32, CstrLength( Cast( u8*, fs ) ) );
    glShaderSource( shader.vs, 1, &vs, &vs_len );  glVerify();
    glShaderSource( shader.fs, 1, &fs, &fs_len );  glVerify();
    glCompileShader( shader.vs );  glVerify();
    glCompileShader( shader.fs );  glVerify();
    _VerifyShader( shader.vs, "Vertex shader: " );
    _VerifyShader( shader.fs, "Fragment shader: " );

    shader.program = glCreateProgram();  glVerify();
    glAttachShader( shader.program, shader.vs );  glVerify();
    glAttachShader( shader.program, shader.fs );  glVerify();
    glLinkProgram( shader.program );  glVerify();
    _VerifyProgram( shader.program, "Shader program: " );

    glUseProgram( shader.program );  glVerify();
    glValidateProgram( shader.program );  glVerify();

    return shader;
  }

  void
  GlwUnloadShader( glwshader_t& shader )
  {
    glDeleteShader( shader.vs );  glVerify();
    glDeleteShader( shader.fs );  glVerify();
    glDeleteProgram( shader.program );  glVerify();
  }





  struct
  glwtarget_t
  {
    u32 id;
    u32 color;
    u32 depth;
  };

  void
  Init( glwtarget_t& target )
  {
    glGenFramebuffers( 1, &target.id );  glVerify();
  }

  void
  Resize( glwtarget_t& target, u32 x, u32 y )
  {
    glBindFramebuffer( GL_FRAMEBUFFER, target.id );  glVerify();

    glDeleteTextures( 1, &target.color );
    glGenTextures( 1, &target.color );  glVerify();
    glBindTexture( GL_TEXTURE_2D, target.color );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glBindTexture( GL_TEXTURE_2D, 0 );  glVerify();

    glDeleteRenderbuffers( 1, &target.depth );
    glGenRenderbuffers( 1, &target.depth );  glVerify();
    glBindRenderbuffer( GL_RENDERBUFFER, target.depth );
    glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, x, y );
    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target.depth );
    glBindRenderbuffer( GL_RENDERBUFFER, 0 );  glVerify();

    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.color, 0 );
    u32 drawing_buffers[] = {
      GL_COLOR_ATTACHMENT0,
    };
    glDrawBuffers( _countof( drawing_buffers ), drawing_buffers );  glVerify();

    glReadBuffer( GL_NONE );  glVerify();

    u32 res = glCheckFramebufferStatus( GL_FRAMEBUFFER );  glVerify();
    AssertWarn( res == GL_FRAMEBUFFER_COMPLETE );
  }

  void
  Kill( glwtarget_t& target )
  {
    glDeleteTextures( 1, &target.color );
    glDeleteRenderbuffers( 1, &target.depth );
    glDeleteFramebuffers( 1, &target.id );  glVerify();
  }


  // special case shaders:

  struct
  shader_color_t
  {
    glwshader_t core;
    s32 loc_ndc_from_client;
    s32 loc_color;
    s32 attribloc_pos;
  };

  void
  Init( shader_color_t& shader )
  {
    shader.core = GlwLoadShader(
      R"STRING(
        #version 330 core
        layout( location = 0 ) in vec3 vertex_pos;
        uniform mat4 ndc_from_client;
        void main() {
          gl_Position = ndc_from_client * vec4( vertex_pos, 1 );
        }
      )STRING",

      R"STRING(
        #version 330 core
        layout( location = 0 ) out vec4 pixel;
        uniform vec4 color;
        void main() {
          pixel = color;
        }
      )STRING"
      );

    // uniforms
    shader.loc_ndc_from_client = glGetUniformLocation( shader.core.program, "ndc_from_client" );  glVerify();
    shader.loc_color = glGetUniformLocation( shader.core.program, "color" );  glVerify();

    // attribs
    shader.attribloc_pos = glGetAttribLocation( shader.core.program, "vertex_pos" );  glVerify();
  }

  void
  Kill( shader_color_t& shader )
  {
    GlwUnloadShader( shader.core );
    shader.loc_ndc_from_client = -1;
    shader.loc_color = -1;
    shader.attribloc_pos = -1;
  }



  struct
  shader_tex_t
  {
    glwshader_t core;
    s32 loc_ndc_from_client;
    s32 loc_tex_sampler;
    s32 loc_color;
    s32 attribloc_pos;
    s32 attribloc_tc;
  };

  void
  Init( shader_tex_t& shader )
  {
    shader.core = GlwLoadShader(
      R"STRING(
        #version 330 core
        layout( location = 0 ) in vec3 vertex_pos;
        layout( location = 1 ) in vec2 vertex_tc;
        out vec2 tc;
        uniform mat4 ndc_from_client;
        void main() {
          gl_Position = ndc_from_client * vec4( vertex_pos, 1 );
          tc = vertex_tc;
        }
      )STRING",

      R"STRING(
        #version 330 core
        in vec2 tc;
        layout( location = 0 ) out vec4 pixel;
        uniform sampler2D tex_sampler;
        uniform vec4 color;
        void main() {
          vec4 texel = texture( tex_sampler, tc );
          pixel = color * texel;
        }
      )STRING"
      );

    // uniforms
    shader.loc_ndc_from_client = glGetUniformLocation( shader.core.program, "ndc_from_client" );  glVerify();
    shader.loc_tex_sampler = glGetUniformLocation( shader.core.program, "tex_sampler" );  glVerify();
    shader.loc_color = glGetUniformLocation( shader.core.program, "color" );  glVerify();

    // attribs
    shader.attribloc_pos = glGetAttribLocation( shader.core.program, "vertex_pos" );  glVerify();
    shader.attribloc_tc = glGetAttribLocation( shader.core.program, "vertex_tc" );  glVerify();
  }

  void
  Kill( shader_tex_t& shader )
  {
    GlwUnloadShader( shader.core );
    shader.loc_ndc_from_client = -1;
    shader.loc_tex_sampler = -1;
    shader.loc_color = -1;
    shader.attribloc_pos = -1;
    shader.attribloc_tc = -1;
  }

#endif // OPENGL_INSTEAD_OF_SOFTWARE




Enumc( glwcursortype_t )
{
  arrow,
  ibeam,
  hand,
  wait,
};

Inl void
_SetCursortype( glwcursortype_t type )
{
#if defined(WIN)
  switch( type ) {
    case glwcursortype_t::arrow:  SetCursor( LoadCursor( 0, IDC_ARROW ) );  break;
    case glwcursortype_t::ibeam:  SetCursor( LoadCursor( 0, IDC_IBEAM ) );  break;
    case glwcursortype_t::hand :  SetCursor( LoadCursor( 0, IDC_HAND  ) );  break;
    case glwcursortype_t::wait :  SetCursor( LoadCursor( 0, IDC_WAIT  ) );  break;
    default: UnreachableCrash();
  }
#elif defined(MAC)
  ImplementCrash();
#else
#error Unsupported platform
#endif
}



Enumc( glwcallbacktype_t )
{
  keyevent,
  mouseevent,
  windowevent,
  render,
};

struct
glwcallback_t
{
  glwcallbacktype_t type;
  void* misc;
  void* fn;
};


struct
glwclient_t
{
  bool alive;
  glwcursortype_t cursortype;
  stack_resizeable_cont_t<u8> cstr_title;
  stack_resizeable_cont_t<glwcallback_t> callbacks;

#if OPENGL_INSTEAD_OF_SOFTWARE
  hashset_t texid_map;

  u32 vao;
  shader_tex_t shader_fullscreen;
  u32 glstream_fullscreen;
  stack_resizeable_cont_t<f32> stream_fullscreen;
#endif // OPENGL_INSTEAD_OF_SOFTWARE

  //u32 keyrepeat_delay_millisec; // TODO: implement custom timer keyrepeating.
  //u32 keyrepeat_interval_millisec;

  bool fullscreen;
  vec2<u32> dim;
  vec2<f32> dimf;
  vec2<s32> m;
  u32 dpi;

#if defined(WIN)
  HANDLE timer_anim;
#elif defined(MAC)
#else
#error Unexpected platform
#endif

  f64 timestep_fixed; // seconds
  u64 time_render0;
  u64 time_render1;

  u32* fullscreen_bitmap_argb;
  #if RENDER_UNPACKED
    __m128* fullscreen_bitmap_argb_unpacked;
  #endif
  
#if defined(WIN)
  HDC window_dc; // handle to display context.
  #if OPENGL_INSTEAD_OF_SOFTWARE
    HGLRC hgl; // handle to opengl context.
    glwtarget_t target;
  #else
    HDC fullscreen_bitmap_dc;
    HBITMAP fullscreen_bitmap;
  #endif // OPENGL_INSTEAD_OF_SOFTWARE
  HWND hwnd; // handle to window.
  HINSTANCE hi; // handle to WinAPI module the window is running under.
#elif defined(MAC)
#else
#error Unexpected platform
#endif

  bool target_valid;
};

// TODO: can we eliminate this? currently needed for clipboard nonsense.
glwclient_t* g_client = 0;



Enumc( glwkeyevent_t )
{
  dn,
  up,
  repeat,
};
Inl slice_t
SliceFromKeyEventType( glwkeyevent_t t )
{
  switch( t ) {
    case glwkeyevent_t::dn: return SliceFromCStr( "dn" );
    case glwkeyevent_t::up: return SliceFromCStr( "up" );
    case glwkeyevent_t::repeat: return SliceFromCStr( "repeat" );
    default: UnreachableCrash(); return {};
  }
}

Enumc( glwmouseevent_t )
{
  dn,
  up,
  move,
  wheelmove,
};
Inl slice_t
SliceFromMouseEventType( glwmouseevent_t t )
{
  switch( t ) {
    case glwmouseevent_t::dn: return SliceFromCStr( "dn" );
    case glwmouseevent_t::up: return SliceFromCStr( "up" );
    case glwmouseevent_t::move: return SliceFromCStr( "move" );
    case glwmouseevent_t::wheelmove: return SliceFromCStr( "wheelmove" );
    default: UnreachableCrash(); return {};
  }
}

constant enum_t glwwindowevent_resize = ( 1 << 0 );
constant enum_t glwwindowevent_dpichange = ( 1 << 1 );
constant enum_t glwwindowevent_focuschange = ( 1 << 2 );

#define __OnKeyEvent( name )      \
  void ( name )(                  \
    void* misc,                   \
    rectf32_t bounds,             \
    bool& fullscreen,             \
    bool& target_valid,           \
    glwkeyevent_t type,           \
    glwkey_t key                  \
    )

#define __OnMouseEvent( name )      \
  void ( name )(                    \
    void* misc,                     \
    rectf32_t bounds,               \
    bool& target_valid,             \
    glwcursortype_t& cursortype,    \
    glwmouseevent_t type,           \
    glwmousebtn_t btn,              \
    vec2<s32> m,                    \
    vec2<s32> raw_delta,            \
    s32 dwheel                      \
    )

#define __OnWindowEvent( name )      \
  void ( name )(                     \
    void* misc,                      \
    enum_t type,                     \
    vec2<u32> dim,                   \
    u32 dpi,                         \
    bool focused,                    \
    bool& target_valid               \
    )

#define __OnRender( name )      \
  void ( name )(                \
    void* misc,                 \
    rectf32_t bounds,           \
    vec2<s32> m,                \
    f64 timestep_realtime,      \
    f64 timestep_fixed,         \
    bool& target_valid          \
    )

typedef __OnKeyEvent( *pfn_onkeyevent_t );
typedef __OnMouseEvent( *pfn_onmouseevent_t );
typedef __OnWindowEvent( *pfn_onwindowevent_t );
typedef __OnRender( *pfn_onrender_t );




void
GlwRegisterCallback( glwclient_t& client, glwcallback_t& callback )
{
  *AddBack( client.callbacks ) = callback;
}



bool
GlwMouseInside(
  vec2<s32> m,
  vec2<f32> origin,
  vec2<f32> dim
  )
{
  auto mx = Cast( f32, m.x );
  auto my = Cast( f32, m.y );
  return
    ( origin.x <= mx )  &&  ( mx < origin.x + dim.x )  &&
    ( origin.y <= my )  &&  ( my < origin.y + dim.y );
}

Inl bool
GlwMouseInsideRect(
  vec2<s32> m,
  vec2<f32> p0,
  vec2<f32> p1
  )
{
  auto mx = Cast( f32, m.x );
  auto my = Cast( f32, m.y );
  return
    ( p0.x <= mx )  &&  ( mx < p1.x )  &&
    ( p0.y <= my )  &&  ( my < p1.y );
}
Inl bool
GlwMouseInsideRect(
  vec2<s32> m,
  rectf32_t bounds
  )
{
  return GlwMouseInsideRect(
    m,
    bounds.p0,
    bounds.p1
    );
}



#if OPENGL_INSTEAD_OF_SOFTWARE

  // We map u32 id_client to one of these mapping structs.
  struct
  glwtexid_mapping_t
  {
    u32 glid;
    u32 img_x;
    u32 img_y;
  };

#endif // OPENGL_INSTEAD_OF_SOFTWARE



void
GlwEarlyKill( glwclient_t& client )
{
  client.alive = 0;

  g_mainthread.signal_quit = 1;
  _ReadWriteBarrier();

//  BOOL released = ReleaseCapture();
//  AssertCrash( released );
}


void
GlwSetCursorType( glwclient_t& client, glwcursortype_t type )
{
  client.cursortype = type;
}



#if OPENGL_INSTEAD_OF_SOFTWARE

  void
  GlwUploadTexture(
    glwclient_t& client,
    u32 texid_client,
    void* mem,
    idx_t nbytes,
    u32 x,
    u32 y
    )
  {
    glwtexid_mapping_t mapping;
    mapping.img_x = x;
    mapping.img_y = y;
    glGenTextures( 1, &mapping.glid );  glVerify();

    // Insert ( texid_client, mapping ) into the texid_map, making sure it is a new texid_client.
    bool already_there;
    Add( client.texid_map, &texid_client, &mapping, &already_there, 0, 0 );
    AssertCrash( !already_there );

    glBindTexture( GL_TEXTURE_2D, mapping.glid );  glVerify();

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );  glVerify();

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, mem );  glVerify();

    glGenerateMipmap( GL_TEXTURE_2D );  glVerify();

    glBindTexture( GL_TEXTURE_2D, 0 );  glVerify();
  }

  void
  GlwUpdateTexture(
    glwclient_t& client,
    u32 texid_client,
    void* mem,
    idx_t nbytes,
    u32 x,
    u32 y
    )
  {
    bool found;
    glwtexid_mapping_t mapping;
    Lookup( client.texid_map, &texid_client, &found, &mapping );
    AssertCrash( found );

    glBindTexture( GL_TEXTURE_2D, mapping.glid );  glVerify();

    // if x,y dims change, we have to do glTexImage to overwrite the old-size texture.
    if( mapping.img_x != x  ||  mapping.img_y != y ) {
      glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, mem );  glVerify();

      // Update the mapping in texid_map, since we updated img_x, img_y.
      glwtexid_mapping_t* inplace_mapping;
      LookupRaw( client.texid_map, &texid_client, &found, Cast( void**, &inplace_mapping ) );
      AssertCrash( found );
      inplace_mapping->img_x = x;
      inplace_mapping->img_y = y;

    } else {
      glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, GL_RGBA, GL_UNSIGNED_BYTE, mem );  glVerify();
    }

    glBindTexture( GL_TEXTURE_2D, 0 );  glVerify();
  }

  void
  GlwDeleteTexture( glwclient_t& client, u32 texid_client )
  {
    bool found;
    glwtexid_mapping_t mapping;
    Remove( client.texid_map, &texid_client, &found, &mapping );
    AssertCrash( found );

    glDeleteTextures( 1, &mapping.glid );  glVerify();
  }

  void
  GlwSetSwapInterval( glwclient_t& client, s32 swap_interval )
  {
    wglSwapIntervalEXT( swap_interval );  glVerify();
  }

#endif // OPENGL_INSTEAD_OF_SOFTWARE


void
GlwSetCursorVisible( glwclient_t& client, bool visible )
{
#if defined(WIN)
  ShowCursor( visible );
#elif defined(MAC)
  ImplementCrash();
#else
#error Unsupported platform
#endif
}


#if OPENGL_INSTEAD_OF_SOFTWARE

  Inl void
  GlwBindTexture(
    u8 tiu,
    u32 glid
    )
  {
    glActiveTexture( GL_TEXTURE0 + tiu );  glVerify();
    glBindTexture( GL_TEXTURE_2D, glid );  glVerify();
  }

#endif // OPENGL_INSTEAD_OF_SOFTWARE




#if defined(WIN)
  struct
  glwkeyvkcode_t
  {
    glwkey_t key;
    u8 vkcode;
  };

  static const glwkeyvkcode_t g_glwkeyvkcodes[] =
  {
    #define ENTRY( name, value, win_value )   { glwkey_t::name, win_value },
    KEYVALMAP( ENTRY )
    #undef ENTRY
  };
#endif

struct
glwkeystring_t
{
  glwkey_t key;
  slice_t string;
};

static const glwkeystring_t g_glwkeystrings[] =
{
  #define ENTRY( name, value, win_value )   { glwkey_t::name, SliceFromCStr( #name ) },
  KEYVALMAP( ENTRY )
  #undef ENTRY
};

static bool g_init_keytables = 0;
static glwkey_t g_key_glw_from_os [ 256 ] = {};
static u8 g_key_os_from_glw [ 256 ] = {};
static slice_t g_string_from_key [ 256 ] = {};

#define CPPHASHSET 1

DEFINE_HASHSET_TRAITS( HashsetTraits_SliceContents, slice_t, EqualContents, HashContents );

static hashset_complexkey_t<
  slice_t,
  glwkey_t,
  HashsetTraits_SliceContents
  >
  g_key_from_string = {};
#if CPPHASHSET
#else
#endif


Inl void
_InitKeyTable()
{
  auto keycount = Cast( enum_t, glwkey_t::count );

#if defined(WIN)
  Fori( enum_t, i, 0, keycount ) { // exclude 'count' as an entry
    auto keyvkcode = g_glwkeyvkcodes[i];
    g_key_os_from_glw[Cast( enum_t, keyvkcode.key )] = keyvkcode.vkcode;
    g_key_glw_from_os[keyvkcode.vkcode] = keyvkcode.key;
  }
#elif defined(MAC)
  ImplementCrash();
#else
#error Unsupported platform
#endif


#if CPPHASHSET
  Init( &g_key_from_string, 2 * keycount );
#else
  Init(
    g_key_from_string,
    256,
    sizeof( slice_t ),
    sizeof( glwkey_t ),
    0.8f,
    Equal_SliceContents,
    Hash_SliceContents
    );
#endif

  Fori( enum_t, i, 0, keycount ) { // exclude 'count' as an entry
    auto keystring = g_glwkeystrings[i];
    g_string_from_key[Cast( enum_t, keystring.key )] = keystring.string;

    bool already_there;
#if CPPHASHSET
    Add( &g_key_from_string, &keystring.string, &keystring.key, &already_there, (glwkey_t*)0, 0 );
#else
    Add( g_key_from_string,
      Cast( void*, &keystring.string ),
      Cast( void*, &keystring.key ),
      &already_there,
      0,
      0
      );
#endif
    AssertCrash( !already_there );
  }
}
Inl void
_KillKeyTable()
{
  Kill( &g_key_from_string );
}

#if defined(WIN)
  Inl glwkey_t
  KeyGlwFromOS( WPARAM key )
  {
    AssertCrash( key < _countof( g_key_glw_from_os ) );
    return g_key_glw_from_os[key];
  }
#endif

Inl u8
KeyOSFromGlw( glwkey_t key )
{
  return g_key_os_from_glw[Cast( enum_t, key )];
}

Inl void
KeyGlwFromString(
  slice_t keystring,
  bool* found,
  glwkey_t* found_key
  )
{
  glwkey_t* key = 0;
  Lookup(
    &g_key_from_string,
    &keystring,
    found,
    &key
    );
  if( found  &&  found_key ) {
    *found_key = *key;
  }
}

Inl slice_t
KeyStringFromGlw( glwkey_t key )
{
  return g_string_from_key[Cast( enum_t, key )];
}


Inl glwkeybind_t
_glwkeybind(
  glwkey_t key,
  glwkey_t alreadydn = glwkey_t::none,
  glwkey_t alreadydn2 = glwkey_t::none,
  glwkey_t conflict = glwkey_t::none,
  glwkey_t conflict2 = glwkey_t::none,
  glwkey_t conflict3 = glwkey_t::none
  )
{
  glwkeybind_t r = {};
  r.key = key;
  r.alreadydn = alreadydn;
  r.alreadydn2 = alreadydn2;
  r.conflict = conflict;
  r.conflict2 = conflict2;
  r.conflict3 = conflict3;
  return r;
}

bool
GlwKeybind( glwkey_t key, glwkeybind_t& bind )
{
  // make sure primary key matches event key.
  if( bind.key != key ) {
    return 0;
  }

  // make sure specified modifier keys are already dn.
#if defined(WIN)
  #define ELIMINATE_ALREADYDN( name_ ) \
    if( bind.name_ != glwkey_t::none ) { \
      u16 key_state = GetKeyState( KeyOSFromGlw( bind.name_ ) ); \
      bool isdn = key_state & ( 1 << 15 ); \
      if( !isdn ) { /* if not down, eliminate. */ \
        return 0; \
      } \
    } \

  ELIMINATE_ALREADYDN( alreadydn )
  ELIMINATE_ALREADYDN( alreadydn2 )

  #undef ELIMINATE_ALREADYDN

  // make sure conflict keys are not already dn.

  #define ELIMINATE_CONFLICT( name_ ) \
    if( bind.name_ != glwkey_t::none ) { \
      u16 key_state = GetKeyState( KeyOSFromGlw( bind.name_ ) ); \
      bool isdn = key_state & ( 1 << 15 ); \
      if( isdn ) { /* if down, eliminate. */ \
        return 0; \
      } \
    } \

  ELIMINATE_CONFLICT( conflict )
  ELIMINATE_CONFLICT( conflict2 )
  ELIMINATE_CONFLICT( conflict3 )

  #undef ELIMINATE_CONFLICT

#elif defined(MAC)
  ImplementCrash();
#else
#error Unsupported platform
#endif

  return 1;
}



#if OPENGL_INSTEAD_OF_SOFTWARE

  Inl void
  OutputQuad(
    stack_resizeable_cont_t<f32>& stream,
    vec2<f32> p0,
    vec2<f32> p1,
    f32 z,
    vec2<f32> tc0,
    vec2<f32> tc1
    )
  {
    auto pos = AddBack( stream, 3*6 + 2*6 );
    *Cast( vec3<f32>*, pos ) = _vec3( p0.x, p0.y, z );  pos += 3;
    *Cast( vec2<f32>*, pos ) = _vec2( tc0.x, tc0.y );  pos += 2;
    *Cast( vec3<f32>*, pos ) = _vec3( p1.x, p0.y, z );  pos += 3;
    *Cast( vec2<f32>*, pos ) = _vec2( tc1.x, tc0.y );  pos += 2;
    *Cast( vec3<f32>*, pos ) = _vec3( p0.x, p1.y, z );  pos += 3;
    *Cast( vec2<f32>*, pos ) = _vec2( tc0.x, tc1.y );  pos += 2;
    *Cast( vec3<f32>*, pos ) = _vec3( p1.x, p1.y, z );  pos += 3;
    *Cast( vec2<f32>*, pos ) = _vec2( tc1.x, tc1.y );  pos += 2;
    *Cast( vec3<f32>*, pos ) = _vec3( p0.x, p1.y, z );  pos += 3;
    *Cast( vec2<f32>*, pos ) = _vec2( tc0.x, tc1.y );  pos += 2;
    *Cast( vec3<f32>*, pos ) = _vec3( p1.x, p0.y, z );  pos += 3;
    *Cast( vec2<f32>*, pos ) = _vec2( tc1.x, tc0.y );  pos += 2;
  }

#endif // OPENGL_INSTEAD_OF_SOFTWARE



#if !OPENGL_INSTEAD_OF_SOFTWARE

  Inl void
  DeleteTarget( glwclient_t& client )
  {
  #if RENDER_UNPACKED
    MemHeapFree( client.fullscreen_bitmap_argb_unpacked );
  #endif
  #if defined(WIN)
    DeleteObject( client.fullscreen_bitmap );
    client.fullscreen_bitmap = 0;
    SelectObject( client.fullscreen_bitmap_dc, 0 );
    DeleteDC( client.fullscreen_bitmap_dc );
    client.fullscreen_bitmap_dc = 0;
  #elif defined(MAC)
    ImplementCrash();
  #else
    #error Unsupported platform
  #endif
  }

  Inl void
  ResizeTargetToMatchDim( glwclient_t& client )
  {
#if defined(WIN)
    DeleteTarget( client );

    client.fullscreen_bitmap_dc = CreateCompatibleDC( client.window_dc );
    AssertCrash( client.fullscreen_bitmap_dc );

    BITMAPINFO bi = { 0 };
    bi.bmiHeader.biSize = sizeof( bi.bmiHeader );
    bi.bmiHeader.biWidth = client.dim.x;
    AssertCrash( client.dim.y <= MAX_s32 );
    bi.bmiHeader.biHeight = -Cast( s32, client.dim.y ); // negative <=> top-down.
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    client.fullscreen_bitmap = CreateDIBSection(
      client.fullscreen_bitmap_dc,
      &bi,
      DIB_RGB_COLORS,
      Cast( void**, &client.fullscreen_bitmap_argb ),
      0,
      0
      );
    AssertCrash( client.fullscreen_bitmap );

#if RENDER_UNPACKED
    client.fullscreen_bitmap_argb_unpacked = MemHeapAlloc( __m128, client.dim.x * client.dim.y );
#endif

    DIBSECTION ds;
    GetObject( client.fullscreen_bitmap, sizeof( ds ), &ds );

    SelectObject( client.fullscreen_bitmap_dc, client.fullscreen_bitmap );
#elif defined(MAC)
    ImplementCrash();
#else
#error Unsupported platform
#endif
  }

#endif // !OPENGL_INSTEAD_OF_SOFTWARE


Inl void
Viewport( glwclient_t& client )
{
#if OPENGL_INSTEAD_OF_SOFTWARE
  glViewport( 0, 0, client.dim.x, client.dim.y );  glVerify();
#endif // OPENGL_INSTEAD_OF_SOFTWARE

  if( client.dim.x && client.dim.y ) {

#if OPENGL_INSTEAD_OF_SOFTWARE
    Resize( client.target, client.dim.x, client.dim.y );
#else
    ResizeTargetToMatchDim( client );
#endif

    client.target_valid = 0;

#if OPENGL_INSTEAD_OF_SOFTWARE
    glUseProgram( client.shader_fullscreen.core.program );  glVerify();

    mat4x4r<f32> ndc_from_client;
    Ortho( &ndc_from_client, 0.0f, client.dimf.x, 0.0f, client.dimf.y, 0.0f, 1.0f );
    glUniformMatrix4fv( client.shader_fullscreen.loc_ndc_from_client, 1, 1, &ndc_from_client.row0.x );  glVerify();

    glUniform1i( client.shader_fullscreen.loc_tex_sampler, 0 );  glVerify();

    auto color = _vec4<f32>( 1, 1, 1, 1 );
    glUniform4fv( client.shader_fullscreen.loc_color, 1, &color.x );  glVerify();

    client.stream_fullscreen.len = 0;

    OutputQuad(
      client.stream_fullscreen,
      _vec2<f32>( 0, 0 ),
      client.dimf,
      0.0f,
      _vec2<f32>( 0, 0 ),
      _vec2<f32>( 1, 1 )
      );
#endif // OPENGL_INSTEAD_OF_SOFTWARE
  }
}


#if defined(WIN)
  Inl vec2<u32>
  _GetMonitorSize( HWND hwnd )
  {
    MONITORINFO monitor_info = { sizeof( MONITORINFO ) /* other fields 0 */ };
    AssertWarn( GetMonitorInfo( MonitorFromWindow( hwnd, MONITOR_DEFAULTTONEAREST ), &monitor_info ) );
    auto w = Cast( u32, monitor_info.rcMonitor.right  - monitor_info.rcMonitor.left );
    auto h = Cast( u32, monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top  );
    return _vec2( w, h );
  }

  Inl void
  _SetWindowSize( HWND hwnd, vec2<u32> dim )
  {
    DWORD style = GetWindowLong( hwnd, GWL_STYLE );
    DWORD styleex = GetWindowLong( hwnd, GWL_EXSTYLE );
    RECT rect = { 0, 0, Cast( LONG, dim.x ), Cast( LONG, dim.y ) };
    AdjustWindowRectEx( &rect, style, 0, styleex ); // get actual win size.
    auto outer_w = rect.right  - rect.left;
    auto outer_h = rect.bottom - rect.top;
    BOOL r = SetWindowPos(
      hwnd, // window
      0, // zorder flag ( unused )
      0, 0, // x, y in client space ( unused )
      outer_w, outer_h, // w, h in client space
      SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_SHOWWINDOW | SWP_FRAMECHANGED ); // size/positioning flags
    AssertWarn( r );
  }

  static void
  _SetFullscreen( HWND hwnd, vec2<u32>& dim, vec2<f32>& dimf, bool fs )
  {
    static bool place_prev_stored = 0;
    static WINDOWPLACEMENT place_prev = { sizeof( WINDOWPLACEMENT ) /* other fields 0 */ };
    static vec2<u32> windowdim = {};

    DWORD style = GetWindowLong( hwnd, GWL_STYLE );
    DWORD styleex = GetWindowLong( hwnd, GWL_EXSTYLE );

    if( fs ) {
      // ENTER FULLSCREEN MODE
      AssertWarn( GetWindowPlacement( hwnd, &place_prev ) );
      windowdim = dim;
      place_prev_stored = 1;

      AssertWarn( style & WS_OVERLAPPEDWINDOW );
      style &= ~WS_OVERLAPPEDWINDOW;
      style |= WS_POPUP;
      AssertWarn( SetWindowLong( hwnd, GWL_STYLE, style ) );

      MONITORINFO monitor_info = { sizeof( MONITORINFO ) /* other fields 0 */ };
      AssertWarn( GetMonitorInfo( MonitorFromWindow( hwnd, MONITOR_DEFAULTTOPRIMARY ), &monitor_info ) );
      LONG x = monitor_info.rcMonitor.left;
      LONG y = monitor_info.rcMonitor.top;
      LONG w = monitor_info.rcMonitor.right  - monitor_info.rcMonitor.left;
      LONG h = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

      dim = _vec2( Cast( u32, w ), Cast( u32, h ) );
      dimf.x = Cast( f32, dim.x );
      dimf.y = Cast( f32, dim.y );
      BOOL r = SetWindowPos(
        hwnd, // window
        HWND_TOP, // zorder flag
        x, y, // x, y in client space
        w, h, // w, h in client space
        SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW ); // size / positioning flags
      AssertWarn( r );

    } else {
      // ENTER WINDOWED MODE
      style &= ~WS_POPUP;
      style |= WS_OVERLAPPEDWINDOW;
      AssertWarn( SetWindowLong( hwnd, GWL_STYLE, style ) );
      if( place_prev_stored ) {
        AssertWarn( SetWindowPlacement( hwnd, &place_prev ) );
        dim = windowdim;
        dimf.x = Cast( f32, dim.x );
        dimf.y = Cast( f32, dim.y );
        RECT rect = { 0, 0, Cast( LONG, windowdim.x ), Cast( LONG, windowdim.y ) };
        AdjustWindowRectEx( &rect, style, FALSE, styleex ); // get actual win size.
        auto outer_w = rect.right  - rect.left;
        auto outer_h = rect.bottom - rect.top;
        BOOL r = SetWindowPos(
          hwnd, // window
          0, // zorder flag ( unused )
          0, 0, // x, y in client space ( unused )
          outer_w, outer_h, // w, h in client space
          SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_SHOWWINDOW | SWP_FRAMECHANGED ); // size/positioning flags
        AssertWarn( r );
      }
    }
  }
#endif




void
_Render( glwclient_t& client )
{
  // TimeClock has more variance, and so it makes for jittery animation ( esp. on Win Server! ).
  // TimeTSC seems to be much smoother, so we use it instead.
  client.time_render0 = client.time_render1;
  client.time_render1 = TimeTSC();
  f64 timestep_realtime = TimeSecFromTSC64( client.time_render1 - client.time_render0 );

  if( !client.target_valid ) {
    // NOTE: anything that outputs drawcalls has the option of invalidating the target, if it knows
    //   the next frame also requires a render ( e.g. animation )
    client.target_valid = 1;

    Prof( MakeTargetValid );

#if OPENGL_INSTEAD_OF_SOFTWARE
    // render to texture.
    glBindFramebuffer( GL_FRAMEBUFFER, client.target.id );  glVerify();
    //glBindFramebuffer( GL_FRAMEBUFFER, 0 );  glVerify();

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );  glVerify();
#else // !OPENGL_INSTEAD_OF_SOFTWARE
    Prof( ZeroBitmap );
    Memzero( client.fullscreen_bitmap_argb, client.dim.x * client.dim.y * sizeof( client.fullscreen_bitmap_argb[0] ) );
    ProfClose( ZeroBitmap );

#if RENDER_UNPACKED
    Prof( ZeroUnpackedBitmap );
  #if 1
    MemzeroAligned16( client.fullscreen_bitmap_argb_unpacked, client.dim.x * client.dim.y * sizeof( client.fullscreen_bitmap_argb_unpacked[0] ) );
  #else
    Memzero( client.fullscreen_bitmap_argb_unpacked, client.dim.x * client.dim.y * sizeof( client.fullscreen_bitmap_argb_unpacked[0] ) );
  #endif
    ProfClose( ZeroUnpackedBitmap );
#endif

#endif // !OPENGL_INSTEAD_OF_SOFTWARE

    ForLen( j, client.callbacks ) {
      auto& callback = client.callbacks.mem[j];
      if( callback.type == glwcallbacktype_t::render ) {
        Cast( pfn_onrender_t, callback.fn )(
          callback.misc,
          _rect( _vec2<f32>( 0, 0 ), client.dimf ),
          client.m,
          timestep_realtime,
          client.timestep_fixed,
          client.target_valid
          );
      }
    }

#if OPENGL_INSTEAD_OF_SOFTWARE
    glFlush();  glVerify();
#else // !OPENGL_INSTEAD_OF_SOFTWARE

#if defined(WIN)
    Prof( BlitToScreen );
    BOOL bitblt_success = BitBlt(
      client.window_dc,
      0, 0,
      client.dim.x, client.dim.y,
      client.fullscreen_bitmap_dc,
      0, 0,
      SRCCOPY
      );
    AssertWarn( bitblt_success );
    ProfClose( BlitToScreen );
#elif defined(MAC)
    ImplementCrash();
#else
#error Unsupported platform
#endif

#endif // !OPENGL_INSTEAD_OF_SOFTWARE

  } // end if( !client.target_valid )

  // TODO: it seems we don't really need this RenderToScreenTex second pass on all calls to _Render.
  // i.e. we can do this only when !client.target_valid, and everything seems to work fine.
  // that'd save some cycles, and maybe we could simplify to avoid the offscreen->screen draw call.

#if OPENGL_INSTEAD_OF_SOFTWARE
  // render to screen.
  Prof( RenderToScreenTex );
  glBindFramebuffer( GL_FRAMEBUFFER, 0 );  glVerify();
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );  glVerify();

  glUseProgram( client.shader_fullscreen.core.program );  glVerify();

  GlwBindTexture( 0, client.target.color );

  glBindBuffer( GL_ARRAY_BUFFER, client.glstream_fullscreen );
  glBufferData( GL_ARRAY_BUFFER, client.stream_fullscreen.len * sizeof( f32 ), client.stream_fullscreen.mem, GL_STREAM_DRAW );  glVerify();
  glEnableVertexAttribArray( client.shader_fullscreen.attribloc_pos );  glVerify();
  glEnableVertexAttribArray( client.shader_fullscreen.attribloc_tc );  glVerify();
  glVertexAttribPointer( client.shader_fullscreen.attribloc_pos, 3, GL_FLOAT, 0, 5 * sizeof( f32 ), 0 );  glVerify();
  glVertexAttribPointer( client.shader_fullscreen.attribloc_tc, 2, GL_FLOAT, 0, 5 * sizeof( f32 ), Cast( void*, 3 * sizeof( f32 ) ) );  glVerify();

  auto vert_count = client.stream_fullscreen.len / 5;
  AssertCrash( client.stream_fullscreen.len % 5 == 0 );
  AssertCrash( vert_count <= MAX_s32 );
  glDrawArrays( GL_TRIANGLES, 0, Cast( s32, vert_count ) );  glVerify();

  glDisableVertexAttribArray( client.shader_fullscreen.attribloc_pos );  glVerify();
  glDisableVertexAttribArray( client.shader_fullscreen.attribloc_tc );  glVerify();

  glFlush();  glVerify();
  ProfClose( RenderToScreenTex );

  Prof( SwapBuffers );
  SwapBuffers( client.window_dc );
  ProfClose( SwapBuffers );
#endif // OPENGL_INSTEAD_OF_SOFTWARE
}

#if defined(WIN)
  Inl void
  CheckForMouseMoveAndTriggerEvent(
    glwclient_t* client,
    LPARAM lp
    )
  {
    auto x = GET_X_LPARAM( lp );
    auto y = GET_Y_LPARAM( lp );
    auto new_m = _vec2( x, y );
    auto raw_delta = new_m - client->m;
    client->m = new_m;
    if( raw_delta.x  ||  raw_delta.y ) {
      ForLen( j, client->callbacks ) {
        auto& callback = client->callbacks.mem[j];
        if( callback.type == glwcallbacktype_t::mouseevent ) {
          Cast( pfn_onmouseevent_t, callback.fn )(
            callback.misc,
            _rect( _vec2<f32>( 0, 0 ), client->dimf ),
            client->target_valid,
            client->cursortype,
            glwmouseevent_t::move,
            glwmousebtn_t::none,
            client->m,
            raw_delta,
            0
            );
        }
      }
    }
  }
#endif

Inl void
FireMouseButtonEvent(
  glwclient_t* client,
  glwmousebtn_t btn,
  bool dn_else_up
  )
{
  ForLen( j, client->callbacks ) {
    auto& callback = client->callbacks.mem[j];
    if( callback.type == glwcallbacktype_t::mouseevent ) {
      Cast( pfn_onmouseevent_t, callback.fn )(
        callback.misc,
        _rect( _vec2<f32>( 0, 0 ), client->dimf ),
        client->target_valid,
        client->cursortype,
        dn_else_up  ?  glwmouseevent_t::dn  :  glwmouseevent_t::up,
        btn,
        client->m,
        _vec2<s32>( 0, 0 ),
        0
        );
    }
  }
}

#if defined(WIN)
  LRESULT CALLBACK
  WindowProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
  {
    static glwclient_t* client = 0;

    //Log( "msg( %u )", msg );

    switch( msg ) {
      case WM_CREATE: {
        auto data = Cast( CREATESTRUCT*, lp );
        client = Cast( glwclient_t*, data->lpCreateParams );
      } return 0;

      case WM_DESTROY: // fallthrough
      case WM_CLOSE: {
        PostQuitMessage( 0 );
        GlwEarlyKill( *client );
      } return 0;

      case WM_DPICHANGED: {
        auto dpi_x = LOWORD( wp );
        auto dpi_y = HIWORD( wp );
        auto rect = *Cast( RECT*, lp );

        AssertWarn( dpi_x == dpi_y );
        auto old_dpi = client->dpi;
        auto new_dpi = dpi_x;
        client->dpi = new_dpi;

        Log( "[DPICHANGED] dpi: %d -> %d", old_dpi, new_dpi );

        // Win32 gives us a very specific rect, and we shouldn't change it.
        BOOL r = SetWindowPos(
          client->hwnd, // window
          0, // zorder flag
          rect.left, rect.top,
          rect.right - rect.left, rect.bottom - rect.top,
          SWP_NOZORDER | SWP_NOACTIVATE
          );
        AssertWarn( r );

        ForLen( j, client->callbacks ) {
          auto& callback = client->callbacks.mem[j];
          if( callback.type == glwcallbacktype_t::windowevent ) {
            Cast( pfn_onwindowevent_t, callback.fn )(
              callback.misc,
              glwwindowevent_resize | glwwindowevent_dpichange,
              client->dim,
              client->dpi,
              1,
              client->target_valid
              );
          }
        }

        Viewport( *client );

      } return 0;

      case WM_PAINT: {
        _Render( *client );
        AssertWarn( ValidateRect( client->hwnd, 0 ) );
      } return 0;

      case WM_ERASEBKGND: {
        // do nothing.
      } return 1;

      case WM_SIZE: {
        client->dim.x = LOWORD( lp );
        client->dim.y = HIWORD( lp );
        client->dimf.x = Cast( f32, client->dim.x );
        client->dimf.y = Cast( f32, client->dim.y );

        ForLen( j, client->callbacks ) {
          auto& callback = client->callbacks.mem[j];
          if( callback.type == glwcallbacktype_t::windowevent ) {
            Cast( pfn_onwindowevent_t, callback.fn )(
              callback.misc,
              glwwindowevent_resize,
              client->dim,
              client->dpi,
              1,
              client->target_valid
              );
          }
        }

        Viewport( *client );
      } return 0;

      case WM_SYSCOMMAND: {
        switch( wp ) {
          case SC_MONITORPOWER:
          case SC_SCREENSAVE:
            return 0; // ignore the above commands
        }
      } break; // forward other cmds to DefWindowProc.

      case WM_KILLFOCUS: {
        if( client->alive ) { // ignore this event if we're quitting.
          ForLen( j, client->callbacks ) {
            auto& callback = client->callbacks.mem[j];
            if( callback.type == glwcallbacktype_t::windowevent ) {
              Cast( pfn_onwindowevent_t, callback.fn )(
                callback.misc,
                glwwindowevent_focuschange,
                client->dim,
                client->dpi,
                0,
                client->target_valid
                );
            }
          }
        }
      } return 0;

      case WM_SETFOCUS: {
        ForLen( j, client->callbacks ) {
          auto& callback = client->callbacks.mem[j];
          if( callback.type == glwcallbacktype_t::windowevent ) {
            Cast( pfn_onwindowevent_t, callback.fn )(
              callback.misc,
              glwwindowevent_focuschange,
              client->dim,
              client->dpi,
              1,
              client->target_valid
              );
          }
        }
      } return 0;

      case WM_KEYDOWN:
      case WM_SYSKEYDOWN: {
        BOOL repeat = ( lp & ( 1 << 30 ) );
        glwkeyevent_t type = ( repeat  ?  glwkeyevent_t::repeat  :  glwkeyevent_t::dn );
        auto key = KeyGlwFromOS( wp );
        if( key != glwkey_t::none ) {
          ForLen( j, client->callbacks ) {
            auto& callback = client->callbacks.mem[j];
            if( callback.type == glwcallbacktype_t::keyevent ) {
              auto fullscreen = client->fullscreen;
              Cast( pfn_onkeyevent_t, callback.fn )(
                callback.misc,
                _rect( _vec2<f32>( 0, 0 ), client->dimf ),
                fullscreen,
                client->target_valid,
                type,
                key
                );
              if( fullscreen != client->fullscreen ) {
                _SetFullscreen( client->hwnd, client->dim, client->dimf, fullscreen );
                client->fullscreen = fullscreen;
              }
            }
          }
        }
      } return 0;

      case WM_KEYUP:
      case WM_SYSKEYUP: {
        auto key = KeyGlwFromOS( wp );
        if( key != glwkey_t::none ) {
          ForLen( j, client->callbacks ) {
            auto& callback = client->callbacks.mem[j];
            if( callback.type == glwcallbacktype_t::keyevent ) {
              auto fullscreen = client->fullscreen;
              Cast( pfn_onkeyevent_t, callback.fn )(
                callback.misc,
                _rect( _vec2<f32>( 0, 0 ), client->dimf ),
                fullscreen,
                client->target_valid,
                glwkeyevent_t::up,
                key
                );
              if( fullscreen != client->fullscreen ) {
                _SetFullscreen( client->hwnd, client->dim, client->dimf, fullscreen );
                client->fullscreen = fullscreen;
              }
            }
          }
        }
      } return 0;

      case WM_MOUSEMOVE: {
  //      auto fkeys = GET_KEYSTATE_WPARAM( wp );
        CheckForMouseMoveAndTriggerEvent( client, lp );
      } return 0;

      case WM_MOUSEWHEEL: {
        // NOTE: lp is the position of the mouse in screen-space, not in client-space!
        //   so don't do the same kind of CheckForMouseMoveAndTriggerEvent logic without a space conversion.
  //      auto fkeys = GET_KEYSTATE_WPARAM( wp );

        auto dwheel = GET_WHEEL_DELTA_WPARAM( wp ) / WHEEL_DELTA; // TODO: float for this.
        if( dwheel ) {
          ForLen( j, client->callbacks ) {
            auto& callback = client->callbacks.mem[j];
            if( callback.type == glwcallbacktype_t::mouseevent ) {
              Cast( pfn_onmouseevent_t, callback.fn )(
                callback.misc,
                _rect( _vec2<f32>( 0, 0 ), client->dimf ),
                client->target_valid,
                client->cursortype,
                glwmouseevent_t::wheelmove,
                glwmousebtn_t::none,
                client->m,
                _vec2<s32>( 0, 0 ),
                Cast( s32, dwheel ) // TODO: float
                );
            }
          }
        }
      } return 0;

      case WM_LBUTTONDOWN: {
        auto btn = glwmousebtn_t::l;
        CheckForMouseMoveAndTriggerEvent( client, lp );
        SetCapture( client->hwnd );
        FireMouseButtonEvent( client, btn, 1 );
      } return 0;

      case WM_LBUTTONUP: {
        auto btn = glwmousebtn_t::l;
        CheckForMouseMoveAndTriggerEvent( client, lp );
        BOOL released = ReleaseCapture();
        AssertCrash( released );
        FireMouseButtonEvent( client, btn, 0 );
      } return 0;

      case WM_RBUTTONDOWN: {
        auto btn = glwmousebtn_t::r;
        CheckForMouseMoveAndTriggerEvent( client, lp );
        FireMouseButtonEvent( client, btn, 1 );
      } return 0;

      case WM_RBUTTONUP: {
        auto btn = glwmousebtn_t::r;
        CheckForMouseMoveAndTriggerEvent( client, lp );
        FireMouseButtonEvent( client, btn, 0 );
      } return 0;

      case WM_MBUTTONDOWN: {
        auto btn = glwmousebtn_t::m;
        CheckForMouseMoveAndTriggerEvent( client, lp );
        FireMouseButtonEvent( client, btn, 1 );
      } return 0;

      case WM_MBUTTONUP: {
        auto btn = glwmousebtn_t::m;
        CheckForMouseMoveAndTriggerEvent( client, lp );
        FireMouseButtonEvent( client, btn, 0 );
      } return 0;

      case WM_XBUTTONDOWN: {
        auto xbtn = Cast( u16, GET_XBUTTON_WPARAM( wp ) );
        AssertCrash( ( xbtn & XBUTTON1 ) ^ ( xbtn & XBUTTON2 ) );
        auto btn = ( xbtn & XBUTTON1 )  ?  glwmousebtn_t::b4  :  glwmousebtn_t::b5;
        CheckForMouseMoveAndTriggerEvent( client, lp );
        FireMouseButtonEvent( client, btn, 1 );
      } return 1; // NOTE: this is different from the other mouse button events, so says the msdn docs

      case WM_XBUTTONUP: {
        auto xbtn = Cast( u16, GET_XBUTTON_WPARAM( wp ) );
        AssertCrash( ( xbtn & XBUTTON1 ) ^ ( xbtn & XBUTTON2 ) );
        auto btn = ( xbtn & XBUTTON1 )  ?  glwmousebtn_t::b4  :  glwmousebtn_t::b5;
        CheckForMouseMoveAndTriggerEvent( client, lp );
        FireMouseButtonEvent( client, btn, 0 );
      } return 1; // NOTE: this is different from the other mouse button events, so says the msdn docs

      case WM_NCHITTEST: {
        LRESULT r = DefWindowProc( hwnd, msg, wp, lp );
        bool cursor_in_client = ( r == HTCLIENT );
        if( cursor_in_client ) {
          _SetCursortype( client->cursortype );
        } else {
          //SetCursor( LoadCursor( 0, IDC_ARROW ) );
        }
        return r;
      }

  //    case WM_CAPTURECHANGED: {
  //      For( i, 0, Cast( idx_t, glwmousebtn_t::count ) ) {
  //        SetBit( client->mousealreadydn, i, 0 );
  //      }
  //    } return 0;

    }
    return DefWindowProc( hwnd, msg, wp, lp );
  }

  // gets the current state of the keylocks
  Inl glwkeylocks_t
  GlwKeylocks()
  {
    glwkeylocks_t r;
    r.caps   = GetKeyState( KeyOSFromGlw( glwkey_t::capslock   ) ) & 1;
    r.num    = GetKeyState( KeyOSFromGlw( glwkey_t::numlock    ) ) & 1;
    r.scroll = GetKeyState( KeyOSFromGlw( glwkey_t::scrolllock ) ) & 1;
    return r;
  }

  Inl bool
  OSKeyIsDown( int os_key )
  {
    u16 key_state = GetKeyState( os_key );
    bool r = key_state & ( 1 << 15 );
    return r;
  }
  Inl bool
  GlwKeyIsDown( glwkey_t key )
  {
    auto os_key = KeyOSFromGlw( key );
    bool r = OSKeyIsDown( os_key );
    return r;
  }
  Inl bool
  GlwMouseBtnIsDown( glwmousebtn_t btn )
  {
    auto os_key = c_vks[ Cast( enum_t, btn ) ];
    bool r = OSKeyIsDown( os_key );
    return r;
  }
#endif

struct
glwkeymodifiersdown_t
{
  bool ctrl;
  bool shift;
  bool alt;
};
// gets the current state of the keylocks
Inl glwkeymodifiersdown_t
GlwKeyModifiersDown()
{
  glwkeymodifiersdown_t r;
#if defined(WIN)
  r.ctrl  = OSKeyIsDown( KeyOSFromGlw( glwkey_t::ctrl  ) );
  r.shift = OSKeyIsDown( KeyOSFromGlw( glwkey_t::shift ) );
  r.alt   = OSKeyIsDown( KeyOSFromGlw( glwkey_t::alt   ) );
#elif defined(MAC)
  ImplementCrash();
#else
#error Unsupported platform
#endif
  return r;
}
Inl bool
AnyDown( glwkeymodifiersdown_t m )
{
  bool any = ( m.ctrl | m.shift | m.alt );
  return any;
}

void
GlwInit()
{
#if defined(WIN)
  // Opt out of windows's dpi auto-scaling, so we can have hi-res fonts on hi-dpi screens!
  auto res = SetProcessDpiAwareness( PROCESS_PER_MONITOR_DPI_AWARE );
  if( res != S_OK ) {
    Log( "SetProcessDpiAwareness to PER_MONITOR_DPI_AWARE failed!" );
  }
  auto old_dpictx = SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );
  if( !old_dpictx ) {
    Log( "SetThreadDpiAwarenessContext to PER_MONITOR_AWARE_V2 failed!" );
    old_dpictx = SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
  }
  if( !old_dpictx ) {
    Log( "SetThreadDpiAwarenessContext to PER_MONITOR_AWARE failed!" );
  }
#endif

  _InitKeyTable();
}


void
GlwInitWindow(
  glwclient_t& client,
  u8* title,
  idx_t title_len,
  bool auto_dim_windowed = 1,
  vec2<u32> dim_windowed = { 0, 0 },
  u16 anim_interval_millisec = 16,
  bool fullscreen = 0,
  bool cursor_visible = 1,
  glwcursortype_t cursortype = glwcursortype_t::arrow,
  u8 bits_rgba = 32,
  u8 bits_depth = 24,
  u8 bits_stencil = 8
  )
{
  ProfFunc();

  // Client initialization.
  Alloc( client.cstr_title, 32 );
  Alloc( client.callbacks, 8 );

#if OPENGL_INSTEAD_OF_SOFTWARE
  Alloc( client.stream_fullscreen, 6*3 + 6*2 );
#endif

  client.alive = 1;

  client.m = _vec2<s32>( 0, 0 );

#if defined(WIN)
  // TODO: abstract this a bit, so it's not so flaky initializing a waitable timer.
  constant s64 delay_over_millisec = -10000;
  client.timer_anim = CreateWaitableTimer( 0, 0, 0 );
  AssertWarn( client.timer_anim );
  s32 period_millisec = Cast( s32, anim_interval_millisec );
  s64 delay = delay_over_millisec * period_millisec;
  AssertWarn( SetWaitableTimer( client.timer_anim, Cast( LARGE_INTEGER*, &delay ), period_millisec, 0, 0, 0 ) );
#endif

  client.timestep_fixed = Cast( f64, anim_interval_millisec ) / 1000.0;


#if OPENGL_INSTEAD_OF_SOFTWARE
  Init(
    client.texid_map,
    512,
    sizeof( u32 ),
    sizeof( glwtexid_mapping_t ),
    0.75f,
    Equal_FirstU32,
    Hash_FirstU32
    );
#endif // OPENGL_INSTEAD_OF_SOFTWARE

#if defined(WIN)
  client.hi = GetModuleHandle( 0 );

  // CREATE THE WINDOW
  DWORD style = WS_OVERLAPPEDWINDOW;
  DWORD styleex = WS_EX_APPWINDOW;
  // start with dummy size; we'll resize before finally showing.
  // we do this so we can ask for the monitor size, which requires a window.
  RECT rect = { 0, 0, 800, 600 };
  AdjustWindowRectEx( &rect, style, FALSE, styleex );
#endif

  // Make a cstr title.
  if( !title || title_len == 0 ) {
    title = Str( "default title" );
    title_len = CstrLength( title );
  }
  auto dst_title = AddBack( client.cstr_title, title_len + 1 );
  CstrCopy( dst_title, title, title_len );

#if defined(WIN)
  // Autogen an icon with a random color
  vec3<f32> palette[] = {
    _vec3<f32>( 1, 0, 0 ),
    _vec3<f32>( 1, 1, 0 ),
    _vec3<f32>( 1, 0, 1 ),
    _vec3<f32>( 0, 1, 0 ),
    _vec3<f32>( 0, 1, 1 ),
    _vec3<f32>( 0, 0, 1 ),
    _vec3<f32>( 238 / 255.0f, 197 / 255.0f, 55 / 255.0f ),
    _vec3<f32>( 32 / 255.0f, 127 / 255.0f, 244 / 255.0f ),
    _vec3<f32>( 44 / 255.0f, 57 / 255.0f, 238 / 255.0f ),
    _vec3<f32>( 68 / 255.0f, 190 / 255.0f, 112 / 255.0f ),
    _vec3<f32>( 205 / 255.0f, 163 / 255.0f, 195 / 255.0f ),
    };
  rng_lcg_t lcg;
  Init( lcg, TimeTSC() );
  u64 val;
  val = Rand64( lcg ) >> ( val & AllOnes( 3 ) );
  auto color = palette[ val % _countof( palette ) ];
  u8 bitmask_xor[32*32*4];
  For( i, 0, 32*32 ) {
    auto x = i / 32;
    auto y = i % 32;
    auto mod = CLAMP( Pow32( 2 * MIN4( x, y, 31 - x, 31 - y ) / 31.0f, 0.75f ), 0, 1 );
    bitmask_xor[4*i+0] = Round_u32_from_f32( 255.0f * mod * color.x ) & AllOnes( 8 );
    bitmask_xor[4*i+1] = Round_u32_from_f32( 255.0f * mod * color.y ) & AllOnes( 8 );
    bitmask_xor[4*i+2] = Round_u32_from_f32( 255.0f * mod * color.z ) & AllOnes( 8 );
    bitmask_xor[4*i+3] = Round_u32_from_f32( 255.0f * mod ) & AllOnes( 8 );
  }
  u8 bitmask_and[32*32];
  memset( bitmask_and, 0xFF, _countof( bitmask_and ) );
  auto icon = CreateIcon(
    client.hi,
    32,
    32,
    4,
    8,
    bitmask_and,
    bitmask_xor
    );

  // register this window.
  WNDCLASS wc;
  wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = client.hi;
//  wc.hIcon = LoadIcon( 0, IDI_WINLOGO );
  wc.hIcon = icon;
  wc.hCursor = 0;
  wc.hbrBackground = 0;
  wc.lpszMenuName = 0;
  wc.lpszClassName = Cast( char*, client.cstr_title.mem );
  AssertWarn( RegisterClass( &wc ) );

  client.hwnd = CreateWindowEx(
    styleex,
    Cast( char*, client.cstr_title.mem ),
    Cast( char*, client.cstr_title.mem ),
    style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, // window style
    //0, 0, // x, y
    CW_USEDEFAULT, CW_USEDEFAULT, // x, y
    rect.right  - rect.left, // w
    rect.bottom - rect.top, // h
    0, // parent window
    0, // menu
    client.hi, // hinstance for this window
    &client // lp for WM_CREATE
    );

  AssertCrash( client.hwnd );

  // get the device context from the window.
  client.window_dc = GetDC( client.hwnd );
  AssertCrash( client.window_dc );

#if OPENGL_INSTEAD_OF_SOFTWARE
  // TODO: do we even need to ChoosePixelFormat when on software rendering?
  // PERF: ChoosePixelFormat is super slow, on the order of 0.5 seconds.
  Prof( ChoosePixelFormat );


#if OPENGL_INSTEAD_OF_SOFTWARE
#else // !OPENGL_INSTEAD_OF_SOFTWARE
  bits_depth = 0;
  bits_stencil = 0;
#endif // !OPENGL_INSTEAD_OF_SOFTWARE

  // apply a pixel format.
  PIXELFORMATDESCRIPTOR pfd = { 0 };
  pfd.nSize = sizeof( PIXELFORMATDESCRIPTOR );
  pfd.nVersion = 1;
#if OPENGL_INSTEAD_OF_SOFTWARE
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
#else // !OPENGL_INSTEAD_OF_SOFTWARE
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_GDI | PFD_DEPTH_DONTCARE;
#endif // !OPENGL_INSTEAD_OF_SOFTWARE
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = bits_rgba;
  pfd.cDepthBits = bits_depth;
  pfd.cStencilBits = bits_stencil;
  pfd.iLayerType = PFD_MAIN_PLANE;
  int pixformat = ChoosePixelFormat( client.window_dc, &pfd );
  AssertWarn( pixformat );

  ProfClose( ChoosePixelFormat );

#if 0 // Interesting pixel format investigations:
  PIXELFORMATDESCRIPTOR pfd2 = { 0 };
  pfd2.nSize = sizeof( PIXELFORMATDESCRIPTOR );
  pfd2.nVersion = 1;
  auto pfd_count = DescribePixelFormat( client.window_dc, 0, sizeof( PIXELFORMATDESCRIPTOR ), 0 );
  AssertWarn( pfd_count );
  Fori( s32, i, 1, pfd_count + 1 ) {
    AssertWarn( DescribePixelFormat( client.window_dc, i, sizeof( PIXELFORMATDESCRIPTOR ), &pfd2 ) );
    if( pfd2.dwFlags & PFD_DRAW_TO_WINDOW  &&
//        pfd2.dwFlags & PFD_SUPPORT_OPENGL  &&
//        pfd2.dwFlags & PFD_DOUBLEBUFFER  &&
        pfd2.dwFlags & PFD_SUPPORT_COMPOSITION  &&
        !( pfd2.dwFlags & PFD_NEED_PALETTE )  &&
        !( pfd2.dwFlags & PFD_NEED_SYSTEM_PALETTE )  &&
        pfd2.iLayerType == 0  &&
        pfd2.iPixelType == 0 ) {

      u8 tmp[65];
      idx_t tmp_len;
      CsFromIntegerU(
        AL( tmp ),
        &tmp_len,
        Cast( u32, pfd2.dwFlags ),
        0,
        0,
        0,
        2,
        Cast( u8*, "01" )
        );

      int x = 0;
    }
  }

//  pixformat = 207;
//  pixformat = 213;
//  AssertWarn( DescribePixelFormat( client.window_dc, pixformat, sizeof( PIXELFORMATDESCRIPTOR ), &pfd2 ) );
#endif

  AssertWarn( DescribePixelFormat( client.window_dc, pixformat, sizeof( PIXELFORMATDESCRIPTOR ), &pfd ) );
  AssertWarn( SetPixelFormat( client.window_dc, pixformat, &pfd ) );
#endif // OPENGL_INSTEAD_OF_SOFTWARE


#if OPENGL_INSTEAD_OF_SOFTWARE
  // load wgl functions so we can create a wgl context.
  LoadWGLFunctions( client.window_dc );

  // create the wgl context.
  static const int context_attribs[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 3,
    WGL_CONTEXT_FLAGS_ARB, 0
      | WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if defined(_DEBUG)
      | WGL_CONTEXT_DEBUG_BIT_ARB
#endif
    ,
    WGL_CONTEXT_PROFILE_MASK_ARB, 0
      | WGL_CONTEXT_CORE_PROFILE_BIT_ARB
      //| WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
    ,
    0 // terminator
  };
  client.hgl = wglCreateContextAttribsARB( client.window_dc, 0, context_attribs );
  if( !client.hgl ) {
    AssertWarn( 0 );
    client.hgl = wglCreateContext( client.window_dc );
    Log( "GL VERSION 3.3 context creation failed. falling back to default..." );
  }
  AssertCrash( client.hgl );

  AssertWarn( wglMakeCurrent( client.window_dc, client.hgl ) );  glVerify();

  LoadOpenGLFunctions( client.window_dc );

  auto versionstr = glGetString( GL_VERSION );
  int major, minor;
  glGetIntegerv( GL_MAJOR_VERSION, &major );
  glGetIntegerv( GL_MINOR_VERSION, &minor );
  Log( "[GL VERSION] string: %s, ints: %d.%d", versionstr, major, minor );

  glEnable( GL_BLEND );  glVerify();
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  glVerify();

  glEnable( GL_DEPTH_TEST );  glVerify();
  glClearDepth( 0 );  glVerify();
  glDepthFunc( GL_GEQUAL );  glVerify();

  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );  glVerify();

  glDisable( GL_CULL_FACE );  glVerify();

  Init( client.shader_fullscreen );

  glGenBuffers( 1, &client.glstream_fullscreen );

  glGenVertexArrays( 1, &client.vao );  glVerify();
  glBindVertexArray( client.vao );  glVerify();
#endif // OPENGL_INSTEAD_OF_SOFTWARE

#if OPENGL_INSTEAD_OF_SOFTWARE
  Init( client.target );
#else // !OPENGL_INSTEAD_OF_SOFTWARE
  client.fullscreen_bitmap_dc = 0;
  client.fullscreen_bitmap = 0;
#endif // !OPENGL_INSTEAD_OF_SOFTWARE

#endif // WIN

  client.target_valid = 0;

  if( auto_dim_windowed ) {
#if defined(WIN)
    auto monitor_size = _GetMonitorSize( client.hwnd );
    dim_windowed = monitor_size / 2u;
#endif
  }
  AssertCrash( dim_windowed.x );
  AssertCrash( dim_windowed.y );
  client.dim = dim_windowed;
  client.dimf.x = Cast( f32, client.dim.x );
  client.dimf.y = Cast( f32, client.dim.y );
  
#if defined(WIN)
  // WARNING!!!
  // this triggers a WM_SIZE message, which we also use for handling resize.
  // our methodology here is to create everything with a dummy size, then resize at the last possible moment, here.
//  _SetWindowSize( client.hwnd, client.dim );
//  ShowWindow( client.hwnd, SW_SHOW );
  ShowWindow( client.hwnd, SW_SHOWMAXIMIZED );
  SetForegroundWindow( client.hwnd );
  SetFocus( client.hwnd );

  _SetFullscreen( client.hwnd, client.dim, client.dimf, fullscreen );
#endif

  client.fullscreen = fullscreen;

  _SetCursortype( cursortype );
  client.cursortype = cursortype;
#if defined(WIN)
  ShowCursor( cursor_visible );

  client.dpi = GetDpiForWindow( client.hwnd );
#endif

//  int real_pxfmt = GetPixelFormat( client.window_dc );
//  if( !real_pxfmt ) {
//    int err = GetLastError();
//    AssertCrash( !err );
//  }
//  PIXELFORMATDESCRIPTOR pfd3 = { 0 };
//  pfd3.nSize = sizeof( PIXELFORMATDESCRIPTOR );
//  pfd3.nVersion = 1;
//  AssertWarn( DescribePixelFormat( client.window_dc, real_pxfmt, sizeof( PIXELFORMATDESCRIPTOR ), &pfd3 ) );
}


void
GlwKillWindow( glwclient_t& client )
{
#if OPENGL_INSTEAD_OF_SOFTWARE
  Kill( client.shader_fullscreen );
  glDeleteBuffers( 1, &client.glstream_fullscreen );
  Free( client.stream_fullscreen );

  glDeleteVertexArrays( 1, &client.vao );  glVerify();

  Kill( client.target );
#else // !OPENGL_INSTEAD_OF_SOFTWARE
  DeleteTarget( client );
#endif // !OPENGL_INSTEAD_OF_SOFTWARE

  client.target_valid = 0;

#if OPENGL_INSTEAD_OF_SOFTWARE
  AssertWarn( wglMakeCurrent( 0, 0 ) );
  AssertWarn( wglDeleteContext( client.hgl ) );
#endif // !OPENGL_INSTEAD_OF_SOFTWARE

#if defined(WIN)
  AssertWarn( ReleaseDC( client.hwnd, client.window_dc ) );
  AssertWarn( DestroyWindow( client.hwnd ) );
  AssertWarn( UnregisterClass( Cast( char*, client.cstr_title.mem ), client.hi ) );

#if OPENGL_INSTEAD_OF_SOFTWARE
  client.hgl = 0;
#endif // OPENGL_INSTEAD_OF_SOFTWARE

  client.window_dc = 0;
  client.hwnd = 0;
  client.hi = 0;
#endif

  AssertCrash( !client.alive );
  Free( client.cstr_title );
  Free( client.callbacks );

#if OPENGL_INSTEAD_OF_SOFTWARE
  Kill( client.texid_map );
#endif // OPENGL_INSTEAD_OF_SOFTWARE

#if defined(WIN)
  CloseHandle( client.timer_anim );
#endif
}

void
GlwKill()
{
  _KillKeyTable();
}


Inl void
SendToClipboardText( glwclient_t& client, u8* text, idx_t text_len )
{
#if defined(WIN)
  HGLOBAL win_mem = {};

#if 0
  HWND hwnd = GetForegroundWindow();
  AssertWarn( hwnd );
  if( !hwnd ) {
    return;
  }
#endif

  idx_t retry = 0;
  while( retry < 10  &&  !OpenClipboard( client.hwnd ) ) {
    TimeSleep( 1 ); // block until we get the clipboard.
    retry += 1;
  }
  if( retry == 10 ) {
    AssertWarn( false );
    return;
  }

  AssertWarn( EmptyClipboard() );

  // we have to go thru windows-specific allocator.
  win_mem = GlobalAlloc( GMEM_MOVEABLE, text_len + 1 );
  AssertWarn( win_mem );

  u8* tmp_mem = Cast( u8*, GlobalLock( win_mem ) );
  AssertWarn( tmp_mem );
  Memmove( tmp_mem, text, text_len );
  Memmove( tmp_mem + text_len, "", 1 ); // insert nul-terminator for CF_OEMTEXT prereq.
  GlobalUnlock( win_mem );

  HANDLE hclip = SetClipboardData( CF_OEMTEXT, win_mem );
  AssertWarn( hclip );

  AssertWarn( CloseClipboard() );

  // cannot be freed before CloseClipboard call.
  GlobalFree( win_mem );
#elif defined(MAC)
  ImplementCrash();
#else
#error Unsupported platform
#endif
}


#define USECLIPBOARDTXT( name )   void ( name )( u8* text, idx_t text_len, void* misc )
typedef USECLIPBOARDTXT( *pfn_useclipboardtxt_t );

Inl void
GetFromClipboardText( glwclient_t& client, pfn_useclipboardtxt_t UseClipboardTxt, void* misc )
{
#if defined(WIN)
#if 0
  HWND hwnd = GetForegroundWindow();
  AssertWarn( hwnd );
  if( !hwnd ) {
    return;
  }
#endif

  idx_t retry = 0;
  while( retry < 10  &&  !OpenClipboard( client.hwnd ) ) {
    TimeSleep( 1 ); // block until we get the clipboard.
    retry += 1;
  }
  if( retry == 10 ) {
    AssertWarn( false );
    return;
  }

  // TODO: why can't we retrieve OEMTEXT?
  HANDLE clip = GetClipboardData( CF_TEXT );
  if( clip ) {
    u8* text = Cast( u8*, GlobalLock( clip ) );
    idx_t text_len = CstrLength( text );
    UseClipboardTxt( text, text_len, misc );
    GlobalUnlock( clip );
  } else {
    UseClipboardTxt( 0, 0, misc );
  }

  AssertWarn( CloseClipboard() );
#elif defined(MAC)
  ImplementCrash();
#else
#error Unsupported platform
#endif
}


#if 0
Inl void
ReserveGlStreams( glwclient_t& client, idx_t count )
{
  if( client.gl_streams.len < count ) {
    auto orig_size = client.gl_streams.len;
    auto num_add = count - orig_size;
    client.gl_streams.len += num_add;
    Reserve( client.gl_streams, client.gl_streams.len );
    AssertCrash( num_add <= MAX_s32 );
    glGenBuffers( Cast( s32, num_add ), &client.gl_streams[orig_size] );  glVerify();
  }
}
#endif

#if defined(WIN)

#if OPENGL_INSTEAD_OF_SOFTWARE

  u32
  GlwLookupGlid(
    glwclient_t& client,
    u32 texid_client
    )
  {
    glwtexid_mapping_t mapping;
    bool found;
    Lookup( client.texid_map, &texid_client, &found, &mapping );
    AssertCrash( found );
    return mapping.glid;
  }

#endif // OPENGL_INSTEAD_OF_SOFTWARE


void
GlwMainLoop( glwclient_t& client )
{
  HANDLE wait_timers[] = {
    client.timer_anim,
    g_mainthread.wake_asynctaskscompleted,
  };

  client.time_render1 = TimeTSC();

  do { // while( client.alive )
    bool do_queue = 0;
    bool do_render = 0;
    bool do_asynctaskscompleted = 0;

    {
      Prof( MainWait );
      DWORD waitres = MsgWaitForMultipleObjectsEx(
        _countof( wait_timers ),
        wait_timers,
        INFINITE,
        QS_ALLINPUT,
        0
        );
      switch( waitres ) {
        case WAIT_FAILED:
        case WAIT_TIMEOUT: {
          AssertWarn( 0 );
        } break;
        case WAIT_OBJECT_0 + 0: { // client.timer_anim
          if( !client.target_valid ) {
            do_render = 1;
          }
          do_queue = 1;
          do_asynctaskscompleted = 1;
//          Log( "mainwake: timer_anim" );
        } break;
        case WAIT_OBJECT_0 + 1: { // g_mainthread.wake_asynctaskscompleted
          do_queue = 1;
          do_asynctaskscompleted = 1;
//          Log( "mainwake: asynctaskscompleted" );
        } break;
        case WAIT_OBJECT_0 + 2: { // Extra return value, for when some input/window message is available.
          do_queue = 1;
          do_asynctaskscompleted = 1;
//          Log( "mainwake: winevent" );
        } break;
        default: UnreachableCrash();
      }
    }

    if( do_queue ) {
      Prof( WinApiMessages );
      // process winapi messages.
      MSG msg;
      while( PeekMessage( &msg, client.hwnd, 0, 0, PM_REMOVE ) ) {
        if( LOWORD( msg.message ) == WM_QUIT ) {
          GlwEarlyKill( client );
        }
        TranslateMessage( &msg );
        DispatchMessage( &msg );
      }
    }

    if( do_asynctaskscompleted ) {
      Prof( MainTaskCompleteds );
      FORLEN( t, t_idx, g_mainthread.taskthreads )
        Forever {
          bool success;
          maincompletedqueue_entry_t me;
          DequeueS( t->output, &me, &success );
          if( !success ) {
            // this is the only reader on this queue, so we know the queue is empty here.
            break;
          }

#if LOGASYNCTASKS
          // TODO: make a new prof buffer type to store these waiting times.
          auto time_waiting = TimeTSC() - me.time_generated;
          Log( "maincompletedqueue_entry_t waited for: %llu", time_waiting );
#endif

          me.FnMainTaskCompleted( &client.target_valid, me.misc0, me.misc1, me.misc2 );
        }
      }
    }

    do_render &= ( client.dim.x != 0 );
    do_render &= ( client.dim.y != 0 );
    if( do_render ) {
      Prof( MainRender );
      _Render( client );
    }

  } while( client.alive );
}

#elif defined(MAC)
#else
#error Unsupported platform
#endif
