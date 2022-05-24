// Copyright (c) John A. Carlos Jr., all rights reserved.


Templ
struct mat4x4r
{
  union
  {
    struct
    {
      vec4<T> row0;
      vec4<T> row1;
      vec4<T> row2;
      vec4<T> row3;
    };
    vec4<T> rows[4];
  };
};

Templ
struct mat4x4c
{
  union
  {
    vec4<T> cols[4];
    struct
    {
      vec4<T> col0;
      vec4<T> col1;
      vec4<T> col2;
      vec4<T> col3;
    };
  };
};


Templ
struct mat3x3r
{
  union
  {
    vec3<T> rows[3];
    struct
    {
      vec3<T> row0;
      vec3<T> row1;
      vec3<T> row2;
    };
  };
};

Templ
struct mat3x3c
{
  union
  {
    vec3<T> cols[3];
    struct
    {
      vec3<T> col0;
      vec3<T> col1;
      vec3<T> col2;
    };
  };
};


Templ
struct mat2x2r
{
  vec2<T> row0;
  vec2<T> row1;
};

Templ
struct mat2x2c
{
  vec2<T> col0;
  vec2<T> col1;
};




#define MAT4x4R( dst,  \
  a, b, c, d,  \
  e, f, g, h,  \
  i, j, k, l,  \
  m, n, o, p )  \
  do { dst->row0.x = a;  dst->row0.y = b;  dst->row0.z = c;  dst->row0.w = d; \
       dst->row1.x = e;  dst->row1.y = f;  dst->row1.z = g;  dst->row1.w = h; \
       dst->row2.x = i;  dst->row2.y = j;  dst->row2.z = k;  dst->row2.w = l; \
       dst->row3.x = m;  dst->row3.y = n;  dst->row3.z = o;  dst->row3.w = p; } while( 0 )

#define MAT4x4C( dst,  \
  a, b, c, d,  \
  e, f, g, h,  \
  i, j, k, l,  \
  m, n, o, p )  \
  do { dst->col0.x = a;  dst->col0.y = e;  dst->col0.z = i;  dst->col0.w = m; \
       dst->col1.x = b;  dst->col1.y = f;  dst->col1.z = j;  dst->col1.w = n; \
       dst->col2.x = c;  dst->col2.y = g;  dst->col2.z = k;  dst->col2.w = o; \
       dst->col3.x = d;  dst->col3.y = h;  dst->col3.z = l;  dst->col3.w = p; } while( 0 )



#define MAT3x3R( dst,  \
  a, b, c,  \
  d, e, f,  \
  g, h, i )  \
  do { dst->row0.x = a;  dst->row0.y = b;  dst->row0.z = c; \
       dst->row1.x = d;  dst->row1.y = e;  dst->row1.z = f; \
       dst->row2.x = g;  dst->row2.y = h;  dst->row2.z = i; } while( 0 )

#define MAT3x3C( dst,  \
  a, b, c,  \
  d, e, f,  \
  g, h, i )  \
  do { dst->col0.x = a;  dst->col0.y = d;  dst->col0.z = g; \
       dst->col1.x = b;  dst->col1.y = e;  dst->col1.z = h; \
       dst->col2.x = c;  dst->col2.y = f;  dst->col2.z = i; } while( 0 )



#define MAT2x2R( dst,  \
  a, b,  \
  c, d )  \
  do { dst->row0.x = a;  dst->row0.y = b; \
       dst->row1.x = c;  dst->row1.y = d; } while( 0 )

#define MAT2x2C( dst,  \
  a, b,  \
  c, d )  \
  do { dst->col0.x = a;  dst->col0.y = c; \
       dst->col1.x = b;  dst->col1.y = d; } while( 0 )





Templ Inl void
Copy( mat4x4r<T>* dst, mat4x4r<T>& src )
{
  Copy( &dst->row0, src.row0 );
  Copy( &dst->row1, src.row1 );
  Copy( &dst->row2, src.row2 );
  Copy( &dst->row3, src.row3 );
}
Templ Inl void
Copy( mat4x4c<T>* dst, mat4x4c<T>& src )
{
  Copy( &dst->col0, src.col0 );
  Copy( &dst->col1, src.col1 );
  Copy( &dst->col2, src.col2 );
  Copy( &dst->col3, src.col3 );
}
Templ Inl void
Copy( mat3x3r<T>* dst, mat3x3r<T>& src )
{
  Copy( &dst->row0, src.row0 );
  Copy( &dst->row1, src.row1 );
  Copy( &dst->row2, src.row2 );
}
Templ Inl void
Copy( mat3x3c<T>* dst, mat3x3c<T>& src )
{
  Copy( &dst->col0, src.col0 );
  Copy( &dst->col1, src.col1 );
  Copy( &dst->col2, src.col2 );
}
Templ Inl void
Copy( mat2x2r<T>* dst, mat2x2r<T>& src )
{
  Copy( &dst->row0, src.row0 );
  Copy( &dst->row1, src.row1 );
}
Templ Inl void
Copy( mat2x2c<T>* dst, mat2x2c<T>& src )
{
  Copy( &dst->col0, src.col0 );
  Copy( &dst->col1, src.col1 );
}



Templ Inl void
Copyr( mat4x4r<T>* dst, T* src )
{
  MAT4x4R( dst,
    src[0],  src[1],  src[2],  src[3],
    src[4],  src[5],  src[6],  src[7],
    src[8],  src[9],  src[10], src[11],
    src[12], src[13], src[14], src[15]
    );
}
Templ Inl void
Copyr( mat4x4c<T>* dst, T* src )
{
  MAT4x4C( dst,
    src[0],  src[1],  src[2],  src[3],
    src[4],  src[5],  src[6],  src[7],
    src[8],  src[9],  src[10], src[11],
    src[12], src[13], src[14], src[15]
    );
}
Templ Inl void
Copyr( mat3x3r<T>* dst, T* src )
{
  MAT3x3R( dst,
    src[0],  src[1],  src[2],
    src[3],  src[4],  src[5],
    src[6],  src[7],  src[8]
    );
}
Templ Inl void
Copyr( mat3x3c<T>* dst, T* src )
{
  MAT3x3C( dst,
    src[0],  src[1],  src[2],
    src[3],  src[4],  src[5],
    src[6],  src[7],  src[8]
    );
}
Templ Inl void
Copyr( mat2x2r<T>* dst, T* src )
{
  MAT2x2R( dst,
    src[0],  src[1],
    src[2],  src[3]
    );
}
Templ Inl void
Copyr( mat2x2c<T>* dst, T* src )
{
  MAT2x2C( dst,
    src[0],  src[1],
    src[2],  src[3]
    );
}



Templ Inl void
Copyc( mat4x4r<T>* dst, T* src )
{
  MAT4x4R( dst,
    src[0], src[4], src[8],  src[12],
    src[1], src[5], src[9],  src[13],
    src[2], src[6], src[10], src[14],
    src[3], src[7], src[11], src[15]
    );
}
Templ Inl void
Copyc( mat4x4c<T>* dst, T* src )
{
  MAT4x4C( dst,
    src[0], src[4], src[8],  src[12],
    src[1], src[5], src[9],  src[13],
    src[2], src[6], src[10], src[14],
    src[3], src[7], src[11], src[15]
    );
}
Templ Inl void
Copyc( mat3x3r<T>* dst, T* src )
{
  MAT3x3R( dst,
    src[0], src[3], src[6],
    src[1], src[4], src[7],
    src[2], src[5], src[8]
    );
}
Templ Inl void
Copyc( mat3x3c<T>* dst, T* src )
{
  MAT3x3C( dst,
    src[0], src[3], src[6],
    src[1], src[4], src[7],
    src[2], src[5], src[8]
    );
}
Templ Inl void
Copyc( mat2x2r<T>* dst, T* src )
{
  MAT2x2R( dst,
    src[0], src[2],
    src[1], src[3]
    );
}
Templ Inl void
Copyc( mat2x2c<T>* dst, T* src )
{
  MAT2x2C( dst,
    src[0], src[2],
    src[1], src[3]
    );
}



Templ Inl void
Copyr( T* dst, mat4x4r<T>& src )
{
  dst[0]  = src.row0.x;  dst[1]  = src.row0.y;  dst[2]  = src.row0.z;  dst[3]  = src.row0.w;
  dst[4]  = src.row1.x;  dst[5]  = src.row1.y;  dst[6]  = src.row1.z;  dst[7]  = src.row1.w;
  dst[8]  = src.row2.x;  dst[9]  = src.row2.y;  dst[10] = src.row2.z;  dst[11] = src.row2.w;
  dst[12] = src.row3.x;  dst[13] = src.row3.y;  dst[14] = src.row3.z;  dst[15] = src.row3.w;
}
Templ Inl void
Copyr( T* dst, mat4x4c<T>& src )
{
  dst[0]  = src.col0.x;  dst[1]  = src.col1.x;  dst[2]  = src.col2.x;  dst[3]  = src.col3.x;
  dst[4]  = src.col0.y;  dst[5]  = src.col1.y;  dst[6]  = src.col2.y;  dst[7]  = src.col3.y;
  dst[8]  = src.col0.z;  dst[9]  = src.col1.z;  dst[10] = src.col2.z;  dst[11] = src.col3.z;
  dst[12] = src.col0.w;  dst[13] = src.col1.w;  dst[14] = src.col2.w;  dst[15] = src.col3.w;
}
Templ Inl void
Copyr( T* dst, mat3x3r<T>& src )
{
  dst[0] = src.row0.x;  dst[1] = src.row0.y;  dst[2] = src.row0.z;
  dst[3] = src.row1.x;  dst[4] = src.row1.y;  dst[5] = src.row1.z;
  dst[6] = src.row2.x;  dst[7] = src.row2.y;  dst[8] = src.row2.z;
}
Templ Inl void
Copyr( T* dst, mat3x3c<T>& src )
{
  dst[0] = src.col0.x;  dst[1] = src.col1.x;  dst[2] = src.col2.x;
  dst[3] = src.col0.y;  dst[4] = src.col1.y;  dst[5] = src.col2.y;
  dst[6] = src.col0.z;  dst[7] = src.col1.z;  dst[8] = src.col2.z;
}
Templ Inl void
Copyr( T* dst, mat2x2r<T>& src )
{
  dst[0] = src.row0.x;  dst[1] = src.row0.y;
  dst[2] = src.row1.x;  dst[3] = src.row1.y;
}
Templ Inl void
Copyr( T* dst, mat2x2c<T>& src )
{
  dst[0] = src.col0.x;  dst[1] = src.col1.x;
  dst[2] = src.col0.y;  dst[3] = src.col1.y;
}



Templ Inl void
Copyc( T* dst, mat4x4r<T>& src )
{
  dst[0] = src.row0.x;  dst[4] = src.row0.y;  dst[8]  = src.row0.z;  dst[12] = src.row0.w;
  dst[1] = src.row1.x;  dst[5] = src.row1.y;  dst[9]  = src.row1.z;  dst[13] = src.row1.w;
  dst[2] = src.row2.x;  dst[6] = src.row2.y;  dst[10] = src.row2.z;  dst[14] = src.row2.w;
  dst[3] = src.row3.x;  dst[7] = src.row3.y;  dst[11] = src.row3.z;  dst[15] = src.row3.w;
}
Templ Inl void
Copyc( T* dst, mat4x4c<T>& src )
{
  dst[0]  = src.col0.x;  dst[1]  = src.col0.y;  dst[2]  = src.col0.z;  dst[3]  = src.col0.w;
  dst[4]  = src.col1.x;  dst[5]  = src.col1.y;  dst[6]  = src.col1.z;  dst[7]  = src.col1.w;
  dst[8]  = src.col2.x;  dst[9]  = src.col2.y;  dst[10] = src.col2.z;  dst[11] = src.col2.w;
  dst[12] = src.col3.x;  dst[13] = src.col3.y;  dst[14] = src.col3.z;  dst[15] = src.col3.w;
}
Templ Inl void
Copyc( T* dst, mat3x3r<T>& src )
{
  dst[0] = src.row0.x;  dst[3] = src.row0.y;  dst[6] = src.row0.z;
  dst[1] = src.row1.x;  dst[4] = src.row1.y;  dst[7] = src.row1.z;
  dst[2] = src.row2.x;  dst[5] = src.row2.y;  dst[8] = src.row2.z;
}
Templ Inl void
Copyc( T* dst, mat3x3c<T>& src )
{
  dst[0] = src.col0.x;  dst[1] = src.col0.y;  dst[2] = src.col0.z;
  dst[3] = src.col1.x;  dst[4] = src.col1.y;  dst[5] = src.col1.z;
  dst[6] = src.col2.x;  dst[7] = src.col2.y;  dst[8] = src.col2.z;
}
Templ Inl void
Copyc( T* dst, mat2x2r<T>& src )
{
  dst[0] = src.row0.x;  dst[2] = src.row0.y;
  dst[1] = src.row1.x;  dst[3] = src.row1.y;
}
Templ Inl void
Copyc( T* dst, mat2x2c<T>& src )
{
  dst[0] = src.col0.x;  dst[1] = src.col0.y;
  dst[2] = src.col1.x;  dst[3] = src.col1.y;
}



Templ Inl void
Zero( mat4x4r<T>* dst, mat4x4r<T>& src )
{
  Zero( &dst->row0, src.row0 );
  Zero( &dst->row1, src.row1 );
  Zero( &dst->row2, src.row2 );
  Zero( &dst->row3, src.row3 );
}
Templ Inl void
Zero( mat4x4c<T>* dst, mat4x4c<T>& src )
{
  Zero( &dst->col0, src.col0 );
  Zero( &dst->col1, src.col1 );
  Zero( &dst->col2, src.col2 );
  Zero( &dst->col3, src.col3 );
}
Templ Inl void
Zero( mat3x3r<T>* dst, mat3x3r<T>& src )
{
  Zero( &dst->row0, src.row0 );
  Zero( &dst->row2, src.row2 );
  Zero( &dst->row1, src.row1 );
}
Templ Inl void
Zero( mat3x3c<T>* dst, mat3x3c<T>& src )
{
  Zero( &dst->col0, src.col0 );
  Zero( &dst->col1, src.col1 );
  Zero( &dst->col2, src.col2 );
}
Templ Inl void
Zero( mat2x2r<T>* dst, mat2x2r<T>& src )
{
  Zero( &dst->row0, src.row0 );
  Zero( &dst->row1, src.row1 );
}
Templ Inl void
Zero( mat2x2c<T>* dst, mat2x2c<T>& src )
{
  Zero( &dst->col0, src.col0 );
  Zero( &dst->col1, src.col1 );
}



Templ Inl void
Add( mat4x4r<T>* dst, mat4x4r<T>& a, mat4x4r<T>& b )
{
  Add( &dst->row0, a.row0, b.row0 );
  Add( &dst->row1, a.row1, b.row1 );
  Add( &dst->row2, a.row2, b.row2 );
  Add( &dst->row3, a.row3, b.row3 );
}
Templ Inl void
Add( mat4x4c<T>* dst, mat4x4c<T>& a, mat4x4c<T>& b )
{
  Add( &dst->col0, a.col0, b.col0 );
  Add( &dst->col1, a.col1, b.col1 );
  Add( &dst->col2, a.col2, b.col2 );
  Add( &dst->col3, a.col3, b.col3 );
}
Templ Inl void
Add( mat3x3r<T>* dst, mat3x3r<T>& a, mat3x3r<T>& b )
{
  Add( &dst->row0, a.row0, b.row0 );
  Add( &dst->row1, a.row1, b.row1 );
  Add( &dst->row2, a.row2, b.row2 );
}
Templ Inl void
Add( mat3x3c<T>* dst, mat3x3c<T>& a, mat3x3c<T>& b )
{
  Add( &dst->col0, a.col0, b.col0 );
  Add( &dst->col1, a.col1, b.col1 );
  Add( &dst->col2, a.col2, b.col2 );
}
Templ Inl void
Add( mat2x2r<T>* dst, mat2x2r<T>& a, mat2x2r<T>& b )
{
  Add( &dst->row0, a.row0, b.row0 );
  Add( &dst->row1, a.row1, b.row1 );
}
Templ Inl void
Add( mat2x2c<T>* dst, mat2x2c<T>& a, mat2x2c<T>& b )
{
  Add( &dst->col0, a.col0, b.col0 );
  Add( &dst->col1, a.col1, b.col1 );
}



Templ Inl void
Sub( mat4x4r<T>* dst, mat4x4r<T>& a, mat4x4r<T>& b )
{
  Sub( &dst->row0, a.row0, b.row0 );
  Sub( &dst->row1, a.row1, b.row1 );
  Sub( &dst->row2, a.row2, b.row2 );
  Sub( &dst->row3, a.row3, b.row3 );
}
Templ Inl void
Sub( mat4x4c<T>* dst, mat4x4c<T>& a, mat4x4c<T>& b )
{
  Sub( &dst->col0, a.col0, b.col0 );
  Sub( &dst->col1, a.col1, b.col1 );
  Sub( &dst->col2, a.col2, b.col2 );
  Sub( &dst->col3, a.col3, b.col3 );
}
Templ Inl void
Sub( mat3x3r<T>* dst, mat3x3r<T>& a, mat3x3r<T>& b )
{
  Sub( &dst->row0, a.row0, b.row0 );
  Sub( &dst->row1, a.row1, b.row1 );
  Sub( &dst->row2, a.row2, b.row2 );
}
Templ Inl void
Sub( mat3x3c<T>* dst, mat3x3c<T>& a, mat3x3c<T>& b )
{
  Sub( &dst->col0, a.col0, b.col0 );
  Sub( &dst->col1, a.col1, b.col1 );
  Sub( &dst->col2, a.col2, b.col2 );
}
Templ Inl void
Sub( mat2x2r<T>* dst, mat2x2r<T>& a, mat2x2r<T>& b )
{
  Sub( &dst->row0, a.row0, b.row0 );
  Sub( &dst->row1, a.row1, b.row1 );
}
Templ Inl void
Sub( mat2x2c<T>* dst, mat2x2c<T>& a, mat2x2c<T>& b )
{
  Sub( &dst->col0, a.col0, b.col0 );
  Sub( &dst->col1, a.col1, b.col1 );
}



Templ Inl void
Mul( mat4x4r<T>* dst, mat4x4r<T>& a, mat4x4r<T>& b )
{
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &a ) );
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &b ) );

  dst->row0.x = a.row0.x * b.row0.x + a.row0.y * b.row1.x + a.row0.z * b.row2.x + a.row0.w * b.row3.x;
  dst->row0.y = a.row0.x * b.row0.y + a.row0.y * b.row1.y + a.row0.z * b.row2.y + a.row0.w * b.row3.y;
  dst->row0.z = a.row0.x * b.row0.z + a.row0.y * b.row1.z + a.row0.z * b.row2.z + a.row0.w * b.row3.z;
  dst->row0.w = a.row0.x * b.row0.w + a.row0.y * b.row1.w + a.row0.z * b.row2.w + a.row0.w * b.row3.w;

  dst->row1.x = a.row1.x * b.row0.x + a.row1.y * b.row1.x + a.row1.z * b.row2.x + a.row1.w * b.row3.x;
  dst->row1.y = a.row1.x * b.row0.y + a.row1.y * b.row1.y + a.row1.z * b.row2.y + a.row1.w * b.row3.y;
  dst->row1.z = a.row1.x * b.row0.z + a.row1.y * b.row1.z + a.row1.z * b.row2.z + a.row1.w * b.row3.z;
  dst->row1.w = a.row1.x * b.row0.w + a.row1.y * b.row1.w + a.row1.z * b.row2.w + a.row1.w * b.row3.w;

  dst->row2.x = a.row2.x * b.row0.x + a.row2.y * b.row1.x + a.row2.z * b.row2.x + a.row2.w * b.row3.x;
  dst->row2.y = a.row2.x * b.row0.y + a.row2.y * b.row1.y + a.row2.z * b.row2.y + a.row2.w * b.row3.y;
  dst->row2.z = a.row2.x * b.row0.z + a.row2.y * b.row1.z + a.row2.z * b.row2.z + a.row2.w * b.row3.z;
  dst->row2.w = a.row2.x * b.row0.w + a.row2.y * b.row1.w + a.row2.z * b.row2.w + a.row2.w * b.row3.w;

  dst->row3.x = a.row3.x * b.row0.x + a.row3.y * b.row1.x + a.row3.z * b.row2.x + a.row3.w * b.row3.x;
  dst->row3.y = a.row3.x * b.row0.y + a.row3.y * b.row1.y + a.row3.z * b.row2.y + a.row3.w * b.row3.y;
  dst->row3.z = a.row3.x * b.row0.z + a.row3.y * b.row1.z + a.row3.z * b.row2.z + a.row3.w * b.row3.z;
  dst->row3.w = a.row3.x * b.row0.w + a.row3.y * b.row1.w + a.row3.z * b.row2.w + a.row3.w * b.row3.w;
}
Templ Inl void
Mul( mat4x4c<T>* dst, mat4x4c<T>& a, mat4x4c<T>& b )
{
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &a ) );
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &b ) );

  dst->col0.x = a.col0.x * b.col0.x + a.col1.x * b.col0.y + a.col2.x * b.col0.z + a.col3.x * b.col0.w;
  dst->col0.y = a.col0.y * b.col0.x + a.col1.y * b.col0.y + a.col2.y * b.col0.z + a.col3.y * b.col0.w;
  dst->col0.z = a.col0.z * b.col0.x + a.col1.z * b.col0.y + a.col2.z * b.col0.z + a.col3.z * b.col0.w;
  dst->col0.w = a.col0.w * b.col0.x + a.col1.w * b.col0.y + a.col2.w * b.col0.z + a.col3.w * b.col0.w;

  dst->col1.x = a.col0.x * b.col1.x + a.col1.x * b.col1.y + a.col2.x * b.col1.z + a.col3.x * b.col1.w;
  dst->col1.y = a.col0.y * b.col1.x + a.col1.y * b.col1.y + a.col2.y * b.col1.z + a.col3.y * b.col1.w;
  dst->col1.z = a.col0.z * b.col1.x + a.col1.z * b.col1.y + a.col2.z * b.col1.z + a.col3.z * b.col1.w;
  dst->col1.w = a.col0.w * b.col1.x + a.col1.w * b.col1.y + a.col2.w * b.col1.z + a.col3.w * b.col1.w;

  dst->col2.x = a.col0.x * b.col2.x + a.col1.x * b.col2.y + a.col2.x * b.col2.z + a.col3.x * b.col2.w;
  dst->col2.y = a.col0.y * b.col2.x + a.col1.y * b.col2.y + a.col2.y * b.col2.z + a.col3.y * b.col2.w;
  dst->col2.z = a.col0.z * b.col2.x + a.col1.z * b.col2.y + a.col2.z * b.col2.z + a.col3.z * b.col2.w;
  dst->col2.w = a.col0.w * b.col2.x + a.col1.w * b.col2.y + a.col2.w * b.col2.z + a.col3.w * b.col2.w;

  dst->col3.x = a.col0.x * b.col3.x + a.col1.x * b.col3.y + a.col2.x * b.col3.z + a.col3.x * b.col3.w;
  dst->col3.y = a.col0.y * b.col3.x + a.col1.y * b.col3.y + a.col2.y * b.col3.z + a.col3.y * b.col3.w;
  dst->col3.z = a.col0.z * b.col3.x + a.col1.z * b.col3.y + a.col2.z * b.col3.z + a.col3.z * b.col3.w;
  dst->col3.w = a.col0.w * b.col3.x + a.col1.w * b.col3.y + a.col2.w * b.col3.z + a.col3.w * b.col3.w;
}
Templ Inl void
Mul( mat3x3r<T>* dst, mat3x3r<T>& a, mat3x3r<T>& b )
{
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &a ) );
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &b ) );

  dst->row0.x = a.row0.x * b.row0.x + a.row0.y * b.row1.x + a.row0.z * b.row2.x;
  dst->row0.y = a.row0.x * b.row0.y + a.row0.y * b.row1.y + a.row0.z * b.row2.y;
  dst->row0.z = a.row0.x * b.row0.z + a.row0.y * b.row1.z + a.row0.z * b.row2.z;

  dst->row1.x = a.row1.x * b.row0.x + a.row1.y * b.row1.x + a.row1.z * b.row2.x;
  dst->row1.y = a.row1.x * b.row0.y + a.row1.y * b.row1.y + a.row1.z * b.row2.y;
  dst->row1.z = a.row1.x * b.row0.z + a.row1.y * b.row1.z + a.row1.z * b.row2.z;

  dst->row2.x = a.row2.x * b.row0.x + a.row2.y * b.row1.x + a.row2.z * b.row2.x;
  dst->row2.y = a.row2.x * b.row0.y + a.row2.y * b.row1.y + a.row2.z * b.row2.y;
  dst->row2.z = a.row2.x * b.row0.z + a.row2.y * b.row1.z + a.row2.z * b.row2.z;
}
Templ Inl void
Mul( mat3x3c<T>* dst, mat3x3c<T>& a, mat3x3c<T>& b )
{
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &a ) );
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &b ) );

  dst->col0.x = a.col0.x * b.col0.x + a.col1.x * b.col0.y + a.col2.x * b.col0.z;
  dst->col0.y = a.col0.y * b.col0.x + a.col1.y * b.col0.y + a.col2.y * b.col0.z;
  dst->col0.z = a.col0.z * b.col0.x + a.col1.z * b.col0.y + a.col2.z * b.col0.z;

  dst->col1.x = a.col0.x * b.col1.x + a.col1.x * b.col1.y + a.col2.x * b.col1.z;
  dst->col1.y = a.col0.y * b.col1.x + a.col1.y * b.col1.y + a.col2.y * b.col1.z;
  dst->col1.z = a.col0.z * b.col1.x + a.col1.z * b.col1.y + a.col2.z * b.col1.z;

  dst->col2.x = a.col0.x * b.col2.x + a.col1.x * b.col2.y + a.col2.x * b.col2.z;
  dst->col2.y = a.col0.y * b.col2.x + a.col1.y * b.col2.y + a.col2.y * b.col2.z;
  dst->col2.z = a.col0.z * b.col2.x + a.col1.z * b.col2.y + a.col2.z * b.col2.z;
}
Templ Inl void
Mul( mat2x2r<T>* dst, mat2x2r<T>& a, mat2x2r<T>& b )
{
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &a ) );
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &b ) );

  dst->row0.x = a.row0.x * b.row0.x + a.row0.y * b.row1.x;
  dst->row0.y = a.row0.x * b.row0.y + a.row0.y * b.row1.y;

  dst->row1.x = a.row1.x * b.row0.x + a.row1.y * b.row1.x;
  dst->row1.y = a.row1.x * b.row0.y + a.row1.y * b.row1.y;
}
Templ Inl void
Mul( mat2x2c<T>* dst, mat2x2c<T>& a, mat2x2c<T>& b )
{
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &a ) );
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &b ) );

  dst->col0.x = a.col0.x * b.col0.x + a.col1.x * b.col0.y;
  dst->col0.y = a.col0.y * b.col0.x + a.col1.y * b.col0.y;

  dst->col1.x = a.col0.x * b.col1.x + a.col1.x * b.col1.y;
  dst->col1.y = a.col0.y * b.col1.x + a.col1.y * b.col1.y;
}



Templ Inl void
Mul( vec4<T>* dst, mat4x4r<T>& a, vec4<T>& b )
{
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &b ) );

  dst->x = Dot( a.row0, b );
  dst->y = Dot( a.row1, b );
  dst->z = Dot( a.row2, b );
  dst->w = Dot( a.row3, b );
}
Templ Inl void
Mul( vec4<T>* dst, mat4x4c<T>& a, vec4<T>& b )
{
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &b ) );

  dst->x = a.col0.x * b.x + a.col1.x * b.y + a.col2.x * b.z + a.col3.x * b.w;
  dst->y = a.col0.y * b.x + a.col1.y * b.y + a.col2.y * b.z + a.col3.y * b.w;
  dst->z = a.col0.z * b.x + a.col1.z * b.y + a.col2.z * b.z + a.col3.z * b.w;
  dst->w = a.col0.w * b.x + a.col1.w * b.y + a.col2.w * b.z + a.col3.w * b.w;
}
Templ Inl void
Mul( vec3<T>* dst, mat3x3r<T>& a, vec3<T>& b )
{
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &b ) );

  dst->x = Dot( a.row0, b );
  dst->y = Dot( a.row1, b );
  dst->z = Dot( a.row2, b );
}
Templ Inl void
Mul( vec3<T>* dst, mat3x3c<T>& a, vec3<T>& b )
{
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &b ) );

  dst->x = a.col0.x * b.x + a.col1.x * b.y + a.col2.x * b.z;
  dst->y = a.col0.y * b.x + a.col1.y * b.y + a.col2.y * b.z;
  dst->z = a.col0.z * b.x + a.col1.z * b.y + a.col2.z * b.z;
}
Templ Inl void
Mul( vec2<T>* dst, mat2x2r<T>& a, vec2<T>& b )
{
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &b ) );

  dst->x = Dot( a.row0, b );
  dst->y = Dot( a.row1, b );
}
Templ Inl void
Mul( vec2<T>* dst, mat2x2c<T>& a, vec2<T>& b )
{
  AssertCrash( Cast( idx_t, dst ) != Cast( idx_t, &b ) );

  dst->x = a.col0.x * b.x + a.col1.x * b.y;
  dst->y = a.col0.y * b.x + a.col1.y * b.y;
}



Templ Inl void
Zero( mat4x4r<T>* dst )
{
  Zero( &dst->row0 );
  Zero( &dst->row1 );
  Zero( &dst->row2 );
  Zero( &dst->row3 );
}
Templ Inl void
Zero( mat4x4c<T>* dst )
{
  Zero( &dst->col0 );
  Zero( &dst->col1 );
  Zero( &dst->col2 );
  Zero( &dst->col3 );
}
Templ Inl void
Zero( mat3x3r<T>* dst )
{
  Zero( &dst->row0 );
  Zero( &dst->row1 );
  Zero( &dst->row2 );
}
Templ Inl void
Zero( mat3x3c<T>* dst )
{
  Zero( &dst->col0 );
  Zero( &dst->col1 );
  Zero( &dst->col2 );
}
Templ Inl void
Zero( mat2x2r<T>* dst )
{
  Zero( &dst->row0 );
  Zero( &dst->row1 );
}
Templ Inl void
Zero( mat2x2c<T>* dst )
{
  Zero( &dst->col0 );
  Zero( &dst->col1 );
}






Templ Inl void
Identity( mat4x4r<T>* dst )
{
  MAT4x4R( dst,
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
    );
}
Templ Inl void
Identity( mat4x4c<T>* dst )
{
  MAT4x4C( dst,
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
    );
}
Templ Inl void
Identity( mat3x3r<T>* dst )
{
  MAT3x3R( dst,
    1, 0, 0,
    0, 1, 0,
    0, 0, 1
    );
}
Templ Inl void
Identity( mat3x3c<T>* dst )
{
  MAT3x3C( dst,
    1, 0, 0,
    0, 1, 0,
    0, 0, 1
    );
}
Templ Inl void
Identity( mat2x2r<T>* dst )
{
  MAT2x2R( dst,
    1, 0,
    0, 1
    );
}
Templ Inl void
Identity( mat2x2c<T>* dst )
{
  MAT2x2C( dst,
    1, 0,
    0, 1
    );
}



Templ Inl void
_ElemSwap( T* a, T* b )
{
  T tmp = *a;  *a = *b;  *b = tmp;
}

Templ Inl void
Transpose( mat4x4r<T>* dst )
{
  _ElemSwap( &dst->row0.y, &dst->row1.x );
  _ElemSwap( &dst->row0.z, &dst->row2.x );
  _ElemSwap( &dst->row0.w, &dst->row3.x );
  _ElemSwap( &dst->row1.z, &dst->row2.y );
  _ElemSwap( &dst->row1.w, &dst->row3.y );
  _ElemSwap( &dst->row2.w, &dst->row3.z );
}
Templ Inl void
Transpose( mat4x4c<T>* dst )
{
  _ElemSwap( &dst->col0.y, &dst->col1.x );
  _ElemSwap( &dst->col0.z, &dst->col2.x );
  _ElemSwap( &dst->col0.w, &dst->col3.x );
  _ElemSwap( &dst->col1.z, &dst->col2.y );
  _ElemSwap( &dst->col1.w, &dst->col3.y );
  _ElemSwap( &dst->col2.w, &dst->col3.z );
}
Templ Inl void
Transpose( mat3x3r<T>* dst )
{
  _ElemSwap( &dst->row0.y, &dst->row1.x );
  _ElemSwap( &dst->row0.z, &dst->row2.x );
  _ElemSwap( &dst->row1.z, &dst->row2.y );
}
Templ Inl void
Transpose( mat3x3c<T>* dst )
{
  _ElemSwap( &dst->col0.y, &dst->col1.x );
  _ElemSwap( &dst->col0.z, &dst->col2.x );
  _ElemSwap( &dst->col1.z, &dst->col2.y );
}
Templ Inl void
Transpose( mat2x2r<T>* dst )
{
  _ElemSwap( &dst->row0.y, &dst->row1.x );
}
Templ Inl void
Transpose( mat2x2c<T>* dst )
{
  _ElemSwap( &dst->col0.y, &dst->col1.x );
}



Templ Inl void Transpose( mat4x4r<T>* dst, mat4x4r<T>& src ) { *dst = src;  Transpose( dst ); }
Templ Inl void Transpose( mat4x4c<T>* dst, mat4x4c<T>& src ) { *dst = src;  Transpose( dst ); }
Templ Inl void Transpose( mat3x3r<T>* dst, mat3x3r<T>& src ) { *dst = src;  Transpose( dst ); }
Templ Inl void Transpose( mat3x3c<T>* dst, mat3x3c<T>& src ) { *dst = src;  Transpose( dst ); }
Templ Inl void Transpose( mat2x2r<T>* dst, mat2x2r<T>& src ) { *dst = src;  Transpose( dst ); }
Templ Inl void Transpose( mat2x2c<T>* dst, mat2x2c<T>& src ) { *dst = src;  Transpose( dst ); }



Templ Inl void
Frustum( mat4x4r<T>* dst, T x0, T x1, T y0, T y1, T z0, T z1 )
{
  T rml_rec = 1 / ( x1 - x0 );
  T tmb_rec = 1 / ( y1 - y0 );
  T fmn_rec = 1 / ( z1 - z0 );

  MAT4x4R( dst,
    2 * z0 * rml_rec,  0,                 ( x1 + x0 ) * rml_rec,  0,
    0,                 2 * z0 * tmb_rec,  ( y1 + y0 ) * tmb_rec,  0,
    0,                 0,                 ( z0 + z1 ) * fmn_rec,  2 * z0 * z1 * fmn_rec,
    0,                 0,                 -1,                     0
    );
}
Templ Inl void
Frustum( mat4x4c<T>* dst, T x0, T x1, T y0, T y1, T z0, T z1 )
{
  T rml_rec = 1 / ( x1 - x0 );
  T tmb_rec = 1 / ( y1 - y0 );
  T fmn_rec = 1 / ( z1 - z0 );

  MAT4x4C( dst,
    2 * z0 * rml_rec,  0,                 ( x1 + x0 ) * rml_rec,  0,
    0,                 2 * z0 * tmb_rec,  ( y1 + y0 ) * tmb_rec,  0,
    0,                 0,                 ( z0 + z1 ) * fmn_rec,  2 * z0 * z1 * fmn_rec,
    0,                 0,                 -1,                     0
    );
}



Templ Inl void
Ortho( mat4x4r<T>* dst, T x0, T x1, T y0, T y1, T z0, T z1 )
{
  T rml_rec = 1 / ( x1 - x0 );
  T tmb_rec = 1 / ( y1 - y0 );
  T fmn_rec = 1 / ( z1 - z0 );

  MAT4x4R( dst,
    2 * rml_rec,  0,            0,            -( x1 + x0 ) * rml_rec,
    0,            2 * tmb_rec,  0,            -( y1 + y0 ) * tmb_rec,
    0,            0,            2 * fmn_rec,  -( z1 + z0 ) * fmn_rec,
    0,            0,            0,            1
    );
}
Templ Inl void
Ortho( mat4x4c<T>* dst, T x0, T x1, T y0, T y1, T z0, T z1 )
{
  T rml_rec = 1 / ( x1 - x0 );
  T tmb_rec = 1 / ( y1 - y0 );
  T fmn_rec = 1 / ( z1 - z0 );

  MAT4x4C( dst,
    2 * rml_rec,  0,            0,            -( x1 + x0 ) * rml_rec,
    0,            2 * tmb_rec,  0,            -( y1 + y0 ) * tmb_rec,
    0,            0,            2 * fmn_rec,  -( z1 + z0 ) * fmn_rec,
    0,            0,            0,            1
    );
}
Templ Inl void
Ortho( mat3x3r<T>* dst, T x0, T x1, T y0, T y1 )
{
  T rml_rec = 1 / ( x1 - x0 );
  T tmb_rec = 1 / ( y1 - y0 );

  MAT3x3R( dst,
    2 * rml_rec,  0,            -( x1 + x0 ) * rml_rec,
    0,            2 * tmb_rec,  -( y1 + y0 ) * tmb_rec,
    0,            0,            1
    );
}
Templ Inl void
Ortho( mat3x3c<T>* dst, T x0, T x1, T y0, T y1 )
{
  T rml_rec = 1 / ( x1 - x0 );
  T tmb_rec = 1 / ( y1 - y0 );

  MAT3x3C( dst,
    2 * rml_rec,  0,            -( x1 + x0 ) * rml_rec,
    0,            2 * tmb_rec,  -( y1 + y0 ) * tmb_rec,
    0,            0,            1
    );
}



Templ Inl void
Translate( mat4x4r<T>* dst, T x, T y, T z )
{
  MAT4x4R( dst,
    1, 0, 0, x,
    0, 1, 0, y,
    0, 0, 1, z,
    0, 0, 0, 1
    );
}
Templ Inl void
Translate( mat4x4c<T>* dst, T x, T y, T z )
{
  MAT4x4C( dst,
    1, 0, 0, x,
    0, 1, 0, y,
    0, 0, 1, z,
    0, 0, 0, 1
    );
}
Templ Inl void
Translate( mat3x3r<T>* dst, T x, T y )
{
  MAT3x3R( dst,
    1, 0, x,
    0, 1, y,
    0, 0, 1
    );
}
Templ Inl void
Translate( mat3x3c<T>* dst, T x, T y )
{
  MAT3x3C( dst,
    1, 0, x,
    0, 1, y,
    0, 0, 1
    );
}

Templ Inl void
Translate( mat4x4r<T>* dst, vec3<T>& xyz )
{
  MAT4x4R( dst,
    1, 0, 0, xyz.x,
    0, 1, 0, xyz.y,
    0, 0, 1, xyz.z,
    0, 0, 0, 1
    );
}
Templ Inl void
Translate( mat4x4c<T>* dst, vec3<T>& xyz )
{
  MAT4x4C( dst,
    1, 0, 0, xyz.x,
    0, 1, 0, xyz.y,
    0, 0, 1, xyz.z,
    0, 0, 0, 1
    );
}
Templ Inl void
Translate( mat3x3r<T>* dst, vec2<T>& xy )
{
  MAT3x3R( dst,
    1, 0, xy.x,
    0, 1, xy.y,
    0, 0, 1
    );
}
Templ Inl void
Translate( mat3x3c<T>* dst, vec2<T>& xy )
{
  MAT3x3C( dst,
    1, 0, xy.x,
    0, 1, xy.y,
    0, 0, 1
    );
}



Templ Inl void
Scale( mat4x4r<T>* dst, T x, T y, T z )
{
  MAT4x4R( dst,
    x, 0, 0, 0,
    0, y, 0, 0,
    0, 0, z, 0,
    0, 0, 0, 1
    );
}
Templ Inl void
Scale( mat4x4c<T>* dst, T x, T y, T z )
{
  MAT4x4C( dst,
    x, 0, 0, 0,
    0, y, 0, 0,
    0, 0, z, 0,
    0, 0, 0, 1
    );
}
Templ Inl void
Scale( mat3x3r<T>* dst, T x, T y, T z )
{
  MAT3x3R( dst,
    x, 0, 0,
    0, y, 0,
    0, 0, z
    );
}
Templ Inl void
Scale( mat3x3c<T>* dst, T x, T y, T z )
{
  MAT3x3C( dst,
    x, 0, 0,
    0, y, 0,
    0, 0, z
    );
}

Templ Inl void
Scale( mat4x4r<T>* dst, vec3<T>& xyz )
{
  MAT4x4R( dst,
    xyz.x, 0,     0,     0,
    0,     xyz.y, 0,     0,
    0,     0,     xyz.z, 0,
    0,     0,     0,     1
    );
}
Templ Inl void
Scale( mat4x4c<T>* dst, vec3<T>& xyz )
{
  MAT4x4C( dst,
    xyz.x, 0,     0,     0,
    0,     xyz.y, 0,     0,
    0,     0,     xyz.z, 0,
    0,     0,     0,     1
    );
}
Templ Inl void
Scale( mat3x3r<T>* dst, vec3<T>& xyz )
{
  MAT3x3R( dst,
    xyz.x, 0,     0,
    0,     xyz.y, 0,
    0,     0,     xyz.z
    );
}
Templ Inl void
Scale( mat3x3c<T>* dst, vec3<T>& xyz )
{
  MAT3x3C( dst,
    xyz.x, 0,     0,
    0,     xyz.y, 0,
    0,     0,     xyz.z
    );
}



Templ Inl void
Rotate( mat4x4r<T>* dst, vec3<T>& axis, T angle_radians )
{
  T xx = axis.x * axis.x;
  T xy = axis.x * axis.y;
  T xz = axis.x * axis.z;
  T yy = axis.y * axis.y;
  T yz = axis.y * axis.z;
  T zz = axis.z * axis.z;
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );
  T xs = axis.x * s;
  T ys = axis.y * s;
  T zs = axis.z * s;
  T mc = 1 - c;

  MAT4x4R( dst,
    xx * mc + c,   xy * mc - zs,  xz * mc + ys,  0,
    xy * mc + zs,  yy * mc + c,   yz * mc - xs,  0,
    xz * mc - ys,  yz * mc + xs,  zz * mc + c,   0,
    0,             0,             0,             1
    );
}
Templ Inl void
Rotate( mat4x4c<T>* dst, vec3<T>& axis, T angle_radians )
{
  T xx = axis.x * axis.x;
  T xy = axis.x * axis.y;
  T xz = axis.x * axis.z;
  T yy = axis.y * axis.y;
  T yz = axis.y * axis.z;
  T zz = axis.z * axis.z;
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );
  T xs = axis.x * s;
  T ys = axis.y * s;
  T zs = axis.z * s;
  T mc = 1 - c;

  MAT4x4C( dst,
    xx * mc + c,   xy * mc - zs,  xz * mc + ys,  0,
    xy * mc + zs,  yy * mc + c,   yz * mc - xs,  0,
    xz * mc - ys,  yz * mc + xs,  zz * mc + c,   0,
    0,             0,             0,             1
    );
}

Templ Inl void
Rotate( mat3x3r<T>* dst, vec3<T>& axis, T angle_radians )
{
  T xx = axis.x * axis.x;
  T xy = axis.x * axis.y;
  T xz = axis.x * axis.z;
  T yy = axis.y * axis.y;
  T yz = axis.y * axis.z;
  T zz = axis.z * axis.z;
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );
  T xs = axis.x * s;
  T ys = axis.y * s;
  T zs = axis.z * s;
  T mc = 1 - c;

  MAT3x3R( dst,
    xx * mc + c,   xy * mc - zs,  xz * mc + ys,
    xy * mc + zs,  yy * mc + c,   yz * mc - xs,
    xz * mc - ys,  yz * mc + xs,  zz * mc + c
    );
}
Templ Inl void
Rotate( mat3x3c<T>* dst, vec3<T>& axis, T angle_radians )
{
  T xx = axis.x * axis.x;
  T xy = axis.x * axis.y;
  T xz = axis.x * axis.z;
  T yy = axis.y * axis.y;
  T yz = axis.y * axis.z;
  T zz = axis.z * axis.z;
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );
  T xs = axis.x * s;
  T ys = axis.y * s;
  T zs = axis.z * s;
  T mc = 1 - c;

  MAT3x3C( dst,
    xx * mc + c,   xy * mc - zs,  xz * mc + ys,
    xy * mc + zs,  yy * mc + c,   yz * mc - xs,
    xz * mc - ys,  yz * mc + xs,  zz * mc + c
    );
}



Templ Inl void
Rotate( mat4x4r<T>* dst, vec4<T>& quaternion )
{
  T qxx = quaternion.x * quaternion.x;
  T qxy = quaternion.x * quaternion.y;
  T qxz = quaternion.x * quaternion.z;
  T qxw = quaternion.x * quaternion.w;
  T qyy = quaternion.y * quaternion.y;
  T qyz = quaternion.y * quaternion.z;
  T qyw = quaternion.y * quaternion.w;
  T qzz = quaternion.z * quaternion.z;
  T qzw = quaternion.z * quaternion.w;

  MAT4x4R( dst,
    1 - 2 * ( qyy + qzz ),  2 * ( qxy - qzw ),      2 * ( qxz + qyw ),      0,
    2 * ( qxy + qzw ),      1 - 2 * ( qxx + qzz ),  2 * ( qyz - qxw ),      0,
    2 * ( qxz - qyw ),      2 * ( qyz + qxw ),      1 - 2 * ( qxx + qyy ),  0,
    0,                      0,                      0,                      1
    );
}
Templ Inl void
Rotate( mat4x4c<T>* dst, vec4<T>& quaternion )
{
  T qxx = quaternion.x * quaternion.x;
  T qxy = quaternion.x * quaternion.y;
  T qxz = quaternion.x * quaternion.z;
  T qxw = quaternion.x * quaternion.w;
  T qyy = quaternion.y * quaternion.y;
  T qyz = quaternion.y * quaternion.z;
  T qyw = quaternion.y * quaternion.w;
  T qzz = quaternion.z * quaternion.z;
  T qzw = quaternion.z * quaternion.w;

  MAT4x4C( dst,
    1 - 2 * ( qyy + qzz ),  2 * ( qxy - qzw ),      2 * ( qxz + qyw ),      0,
    2 * ( qxy + qzw ),      1 - 2 * ( qxx + qzz ),  2 * ( qyz - qxw ),      0,
    2 * ( qxz - qyw ),      2 * ( qyz + qxw ),      1 - 2 * ( qxx + qyy ),  0,
    0,                      0,                      0,                      1
    );
}

Templ Inl void
Rotate( mat3x3r<T>* dst, vec4<T>& quaternion )
{
  T qxx = quaternion.x * quaternion.x;
  T qxy = quaternion.x * quaternion.y;
  T qxz = quaternion.x * quaternion.z;
  T qxw = quaternion.x * quaternion.w;
  T qyy = quaternion.y * quaternion.y;
  T qyz = quaternion.y * quaternion.z;
  T qyw = quaternion.y * quaternion.w;
  T qzz = quaternion.z * quaternion.z;
  T qzw = quaternion.z * quaternion.w;

  MAT3x3R( dst,
    1 - 2 * ( qyy + qzz ),  2 * ( qxy - qzw ),      2 * ( qxz + qyw ),
    2 * ( qxy + qzw ),      1 - 2 * ( qxx + qzz ),  2 * ( qyz - qxw ),
    2 * ( qxz - qyw ),      2 * ( qyz + qxw ),      1 - 2 * ( qxx + qyy )
    );
}
Templ Inl void
Rotate( mat3x3c<T>* dst, vec4<T>& quaternion )
{
  T qxx = quaternion.x * quaternion.x;
  T qxy = quaternion.x * quaternion.y;
  T qxz = quaternion.x * quaternion.z;
  T qxw = quaternion.x * quaternion.w;
  T qyy = quaternion.y * quaternion.y;
  T qyz = quaternion.y * quaternion.z;
  T qyw = quaternion.y * quaternion.w;
  T qzz = quaternion.z * quaternion.z;
  T qzw = quaternion.z * quaternion.w;

  MAT3x3C( dst,
    1 - 2 * ( qyy + qzz ),  2 * ( qxy - qzw ),      2 * ( qxz + qyw ),
    2 * ( qxy + qzw ),      1 - 2 * ( qxx + qzz ),  2 * ( qyz - qxw ),
    2 * ( qxz - qyw ),      2 * ( qyz + qxw ),      1 - 2 * ( qxx + qyy )
    );
}

Templ Inl void
RotateX( mat4x4r<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT4x4R( dst,
    1,  0,  0,   0,
    0,  c,  -s,  0,
    0,  s,  c,   0,
    0,  0,  0,   1
    );
}
Templ Inl void
RotateX( mat4x4c<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT4x4C( dst,
    1,  0,  0,   0,
    0,  c,  -s,  0,
    0,  s,  c,   0,
    0,  0,  0,   1
    );
}

Templ Inl void
RotateX( mat3x3r<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT3x3R( dst,
    1,  0,  0,
    0,  c,  -s,
    0,  s,  c
    );
}
Templ Inl void
RotateX( mat3x3c<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT3x3C( dst,
    1,  0,  0,
    0,  c,  -s,
    0,  s,  c
    );
}

Templ Inl void
RotateY( mat4x4r<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT4x4R( dst,
    c,   0,  s,  0,
    0,   1,  0,  0,
    -s,  0,  c,  0,
    0,   0,  0,  1
    );
}
Templ Inl void
RotateY( mat4x4c<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT4x4C( dst,
    c,   0,  s,  0,
    0,   1,  0,  0,
    -s,  0,  c,  0,
    0,   0,  0,  1
    );
}

Templ Inl void
RotateY( mat3x3r<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT3x3R( dst,
    c,   0,  s,
    0,   1,  0,
    -s,  0,  c
    );
}
Templ Inl void
RotateY( mat3x3c<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT3x3C( dst,
    c,   0,  s,
    0,   1,  0,
    -s,  0,  c
    );
}

Templ Inl void
RotateZ( mat4x4r<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT4x4R( dst,
    c,  -s,  0,  0,
    s,  c,   0,  0,
    0,  0,   1,  0,
    0,  0,   0,  1
    );
}
Templ Inl void
RotateZ( mat4x4c<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT4x4C( dst,
    c,  -s,  0,  0,
    s,  c,   0,  0,
    0,  0,   1,  0,
    0,  0,   0,  1
    );
}

Templ Inl void
RotateZ( mat3x3r<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT3x3R( dst,
    c,  -s,  0,
    s,  c,   0,
    0,  0,   1
    );
}
Templ Inl void
RotateZ( mat3x3c<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT3x3C( dst,
    c,  -s,  0,
    s,  c,   0,
    0,  0,   1
    );
}

Templ Inl void
Rotate( mat3x3r<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT3x3R( dst,
    c,  -s,  0,
    s,  c,   0,
    0,  0,   1
    );
}
Templ Inl void
Rotate( mat3x3c<T>* dst, T angle_radians )
{
  T c = Cos( angle_radians );
  T s = Sin( angle_radians );

  MAT3x3C( dst,
    c,  -s,  0,
    s,  c,   0,
    0,  0,   1
    );
}



Templ Inl void
Cross( mat4x4r<T>* dst, vec3<T>& v )
{
  MAT4x4R( dst,
    0,     -v.z,  v.y,   0,
    v.z,   0,     -v.x,  0,
    -v.y,  v.x,   0,     0,
    0,     0,     0,     1
    );
}
Templ Inl void
Cross( mat4x4c<T>* dst, vec3<T>& v )
{
  MAT4x4C( dst,
    0,     -v.z,  v.y,   0,
    v.z,   0,     -v.x,  0,
    -v.y,  v.x,   0,     0,
    0,     0,     0,     1
    );
}
Templ Inl void
Cross( mat3x3r<T>* dst, vec3<T>& v )
{
  MAT3x3R( dst,
    0,     -v.z,  v.y,
    v.z,   0,     -v.x,
    -v.y,  v.x,   0
    );
}
Templ Inl void
Cross( mat3x3c<T>* dst, vec3<T>& v )
{
  MAT3x3C( dst,
    0,     -v.z,  v.y,
    v.z,   0,     -v.x,
    -v.y,  v.x,   0
    );
}



Templ void
Col( mat4x4r<T>& src, u32 idx, vec4<T>* dst )
{
  dst->x = src.row0[idx];
  dst->y = src.row1[idx];
  dst->z = src.row2[idx];
  dst->w = src.row3[idx];
}

Templ void
Row( mat4x4c<T>& src, u32 idx, vec4<T>* dst )
{
  dst->x = src.cols0[idx];
  dst->y = src.cols1[idx];
  dst->z = src.cols2[idx];
  dst->w = src.cols3[idx];
}

Templ void
Col( mat3x3r<T>& src, u32 idx, vec3<T>* dst )
{
  dst->x = src.row0[idx];
  dst->y = src.row1[idx];
  dst->z = src.row2[idx];
}

Templ void
Row( mat3x3c<T>& src, u32 idx, vec3<T>* dst )
{
  dst->x = src.cols0[idx];
  dst->y = src.cols1[idx];
  dst->z = src.cols2[idx];
}
