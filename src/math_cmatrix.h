// Copyright (c) John A. Carlos Jr., all rights reserved.


// NOTE: each row is stored contiguously, one row after another. this allows initialization of the form:
//   f32 matrix [] = {
//     1, 0, 0
//     0, 1, 0
//     0, 0, 1
//     };



#define CMAT_SWAP( T, dst, i, j )   \
  do{ \
    T tmp; \
    tmp = dst[i]; \
    dst[i] = dst[j]; \
    dst[j] = tmp; \
  } while( 0 )



Templ Inl void
m2x2zero( T* a )
{
  For( i, 0, 4 )
    a[i] = 0;
}

Templ Inl void
m3x3zero( T* a )
{
  For( i, 0, 9 )
    a[i] = 0;
}

Templ Inl void
m4x4zero( T* a )
{
  For( i, 0, 16 )
    a[i] = 0;
}


Templ Inl void
m2x2identity( T* a )
{
  a[0] = 1;  a[1] = 0;
  a[2] = 0;  a[3] = 1;
}

Templ Inl void
m3x3identity( T* a )
{
  a[0] = 1;  a[1] = 0;  a[2] = 0;
  a[3] = 0;  a[4] = 1;  a[5] = 0;
  a[6] = 0;  a[7] = 0;  a[8] = 1;
}

Templ Inl void
m4x4identity( T* a )
{
  a[0 ] = 1;  a[1 ] = 0;  a[2 ] = 0;  a[3 ] = 0;
  a[4 ] = 0;  a[5 ] = 1;  a[6 ] = 0;  a[7 ] = 0;
  a[8 ] = 0;  a[9 ] = 0;  a[10] = 1;  a[11] = 0;
  a[12] = 0;  a[13] = 0;  a[14] = 0;  a[15] = 1;
}


Templ Inl void
m2x2copy( T* dst, T* src )
{
  For( i, 0, 4 )
    dst[i] = src[i];
}

Templ Inl void
m3x3copy( T* dst, T* src )
{
  For( i, 0, 9 )
    dst[i] = src[i];
}

Templ Inl void
m4x4copy( T* dst, T* src )
{
  For( i, 0, 16 )
    dst[i] = src[i];
}


Templ Inl void
m2x2transpose( T* dst )
{
  CMAT_SWAP( T, dst, 1, 2 );
}

Templ Inl void
m3x3transpose( T* dst )
{
  CMAT_SWAP( T, dst, 1, 3 );
  CMAT_SWAP( T, dst, 2, 6 );
  CMAT_SWAP( T, dst, 5, 7 );
}

Templ Inl void
m4x4transpose( T* dst )
{
  CMAT_SWAP( T, dst, 1, 4 );
  CMAT_SWAP( T, dst, 2, 8 );
  CMAT_SWAP( T, dst, 3, 12 );
  CMAT_SWAP( T, dst, 6, 9 );
  CMAT_SWAP( T, dst, 7, 13 );
  CMAT_SWAP( T, dst, 11, 14 );
}



Templ Inl void
m2x2add( T* dst, T* src )
{
  For( i, 0, 4 )
    dst[i] += src[i];
}

Templ Inl void
m3x3add( T* dst, T* src )
{
  For( i, 0, 9 )
    dst[i] += src[i];
}

Templ Inl void
m4x4add( T* dst, T* src )
{
  For( i, 0, 16 )
    dst[i] += src[i];
}


Templ Inl void
m2x2sub( T* dst, T* src )
{
  For( i, 0, 4 )
    dst[i] -= src[i];
}

Templ Inl void
m3x3sub( T* dst, T* src )
{
  For( i, 0, 9 )
    dst[i] -= src[i];
}

Templ Inl void
m4x4sub( T* dst, T* src )
{
  For( i, 0, 16 )
    dst[i] -= src[i];
}



Templ Inl void
m2x2add( T* dst, T* a, T* b )
{
  For( i, 0, 4 )
    dst[i] = a[i] + b[i];
}

Templ Inl void
m3x3add( T* dst, T* a, T* b )
{
  For( i, 0, 9 )
    dst[i] = a[i] + b[i];
}

Templ Inl void
m4x4add( T* dst, T* a, T* b )
{
  For( i, 0, 16 )
    dst[i] = a[i] + b[i];
}


Templ Inl void
m2x2sub( T* dst, T* a, T* b )
{
  For( i, 0, 4 )
    dst[i] = a[i] - b[i];
}

Templ Inl void
m3x3sub( T* dst, T* a, T* b )
{
  For( i, 0, 9 )
    dst[i] = a[i] - b[i];
}

Templ Inl void
m4x4sub( T* dst, T* a, T* b )
{
  For( i, 0, 16 )
    dst[i] = a[i] - b[i];
}


Templ Inl void
m2x2mul( T* dst, T* a, T* b )
{
  dst[0] = a[0]*b[0] + a[1]*b[2];
  dst[1] = a[0]*b[1] + a[1]*b[3];
  dst[2] = a[2]*b[0] + a[3]*b[2];
  dst[3] = a[2]*b[1] + a[3]*b[3];
}

Templ Inl void
m3x3mul( T* dst, T* a, T* b )
{
  dst[0] = a[0]*b[0] + a[1]*b[3] + a[2]*b[6];
  dst[1] = a[0]*b[1] + a[1]*b[4] + a[2]*b[7];
  dst[2] = a[0]*b[2] + a[1]*b[5] + a[2]*b[8];
  dst[3] = a[3]*b[0] + a[4]*b[3] + a[5]*b[6];
  dst[4] = a[3]*b[1] + a[4]*b[4] + a[5]*b[7];
  dst[5] = a[3]*b[2] + a[4]*b[5] + a[5]*b[8];
  dst[6] = a[6]*b[0] + a[7]*b[3] + a[8]*b[6];
  dst[7] = a[6]*b[1] + a[7]*b[4] + a[8]*b[7];
  dst[8] = a[6]*b[2] + a[7]*b[5] + a[8]*b[8];
}

Templ Inl void
m4x4mul( T* dst, T* a, T* b )
{
  dst[0 ] = a[0 ]*b[0 ] + a[1 ]*b[4 ] + a[2 ]*b[8 ] + a[3 ]*b[12];
  dst[1 ] = a[0 ]*b[1 ] + a[1 ]*b[5 ] + a[2 ]*b[9 ] + a[3 ]*b[13];
  dst[2 ] = a[0 ]*b[2 ] + a[1 ]*b[6 ] + a[2 ]*b[10] + a[3 ]*b[14];
  dst[3 ] = a[0 ]*b[3 ] + a[1 ]*b[7 ] + a[2 ]*b[11] + a[3 ]*b[15];
  dst[4 ] = a[4 ]*b[0 ] + a[5 ]*b[4 ] + a[6 ]*b[8 ] + a[7 ]*b[12];
  dst[5 ] = a[4 ]*b[1 ] + a[5 ]*b[5 ] + a[6 ]*b[9 ] + a[7 ]*b[13];
  dst[6 ] = a[4 ]*b[2 ] + a[5 ]*b[6 ] + a[6 ]*b[10] + a[7 ]*b[14];
  dst[7 ] = a[4 ]*b[3 ] + a[5 ]*b[7 ] + a[6 ]*b[11] + a[7 ]*b[15];
  dst[8 ] = a[8 ]*b[0 ] + a[9 ]*b[4 ] + a[10]*b[8 ] + a[11]*b[12];
  dst[9 ] = a[8 ]*b[1 ] + a[9 ]*b[5 ] + a[10]*b[9 ] + a[11]*b[13];
  dst[10] = a[8 ]*b[2 ] + a[9 ]*b[6 ] + a[10]*b[10] + a[11]*b[14];
  dst[11] = a[8 ]*b[3 ] + a[9 ]*b[7 ] + a[10]*b[11] + a[11]*b[15];
  dst[12] = a[12]*b[0 ] + a[13]*b[4 ] + a[14]*b[8 ] + a[15]*b[12];
  dst[13] = a[12]*b[1 ] + a[13]*b[5 ] + a[14]*b[9 ] + a[15]*b[13];
  dst[14] = a[12]*b[2 ] + a[13]*b[6 ] + a[14]*b[10] + a[15]*b[14];
  dst[15] = a[12]*b[3 ] + a[13]*b[7 ] + a[14]*b[11] + a[15]*b[15];
}



Templ Inl void
m4x4frustum( T* dst, T x0, T x1, T y0, T y1, T z0, T z1 )
{
  T rml_rec = 1 / ( x1 - x0 );
  T tmb_rec = 1 / ( y1 - y0 );
  T nmf_rec = 1 / ( z0 - z1 );
  dst[0]  = 2 * z0 * rml_rec;
  dst[1]  = 0;
  dst[2]  = ( x1 + x0 ) * rml_rec;
  dst[3]  = 0;
  dst[4]  = 0;
  dst[5]  = 2 * z0 * tmb_rec;
  dst[6]  = ( y1 + y0 ) * tmb_rec;
  dst[7]  = 0;
  dst[8]  = 0;
  dst[9]  = 0;
  dst[10] = -(z0 + z1 ) * nmf_rec;
  dst[11] = -2 * z0 * z1 * nmf_rec;
  dst[12] = 0;
  dst[13] = 0;
  dst[14] = -1;
  dst[15] = 0;
}

Templ Inl void
m4x4ortho  ( T* dst, T x0, T x1, T y0, T y1, T z0, T z1 )
{
  T rml_rec = 1 / ( x1 - x0 );
  T tmb_rec = 1 / ( y1 - y0 );
  T nmf_rec = 1 / ( z0 - z1 );
  dst[0]  = 2 * rml_rec;
  dst[1]  = 0;
  dst[2]  = 0;
  dst[3]  = -(x1 + x0 ) * rml_rec;
  dst[4]  = 0;
  dst[5]  = 2 * tmb_rec;
  dst[6]  = 0;
  dst[7]  = -(y1 + y0 ) * tmb_rec;
  dst[8]  = 0;
  dst[9]  = 0;
  dst[10] = 2 * nmf_rec;
  dst[11] = ( z0 + z1 ) * nmf_rec;
  dst[12] = 0;
  dst[13] = 0;
  dst[14] = 0;
  dst[15] = 1;
}

Templ Inl void
m4x4translate( T* dst, T x, T y, T z )
{
  dst[0 ] = 1;  dst[1 ] = 0;  dst[2 ] = 0;  dst[3 ] = x;
  dst[4 ] = 0;  dst[5 ] = 1;  dst[6 ] = 0;  dst[7 ] = y;
  dst[8 ] = 0;  dst[9 ] = 0;  dst[10] = 1;  dst[11] = z;
  dst[12] = 0;  dst[13] = 0;  dst[14] = 0;  dst[15] = 1;
}

Templ Inl void
m4x4translate( T* dst, T* xyz )
{
  dst[0 ] = 1;  dst[1 ] = 0;  dst[2 ] = 0;  dst[3 ] = xyz[0];
  dst[4 ] = 0;  dst[5 ] = 1;  dst[6 ] = 0;  dst[7 ] = xyz[1];
  dst[8 ] = 0;  dst[9 ] = 0;  dst[10] = 1;  dst[11] = xyz[2];
  dst[12] = 0;  dst[13] = 0;  dst[14] = 0;  dst[15] = 1;
}


Templ Inl void
m4x4scale( T* dst, T x, T y, T z )
{
  dst[0 ] = x;  dst[1 ] = 0;  dst[2 ] = 0;  dst[3 ] = 0;
  dst[4 ] = 0;  dst[5 ] = y;  dst[6 ] = 0;  dst[7 ] = 0;
  dst[8 ] = 0;  dst[9 ] = 0;  dst[10] = z;  dst[11] = 0;
  dst[12] = 0;  dst[13] = 0;  dst[14] = 0;  dst[15] = 1;
}

Templ Inl void
m4x4scale( T* dst, T* xyz )
{
  dst[0 ] = xyz[0];  dst[1 ] = 0;       dst[2 ] = 0;       dst[3 ] = 0;
  dst[4 ] = 0;       dst[5 ] = xyz[1];  dst[6 ] = 0;       dst[7 ] = 0;
  dst[8 ] = 0;       dst[9 ] = 0;       dst[10] = xyz[2];  dst[11] = 0;
  dst[12] = 0;       dst[13] = 0;       dst[14] = 0;       dst[15] = 1;
}
