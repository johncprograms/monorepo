// Copyright (c) John A. Carlos Jr., all rights reserved.


Templ Inl void v2clear( T* a ) { a[0] = a[1] = 0; }
Templ Inl void v3clear( T* a ) { a[0] = a[1] = a[2] = 0; }
Templ Inl void v4clear( T* a ) { a[0] = a[1] = a[2] = a[3] = 0; }

Templ Inl void v2copy( T* a, T b ) { a[0] = a[1] = b; }
Templ Inl void v3copy( T* a, T b ) { a[0] = a[1] = a[2] = b; }
Templ Inl void v4copy( T* a, T b ) { a[0] = a[1] = a[2] = a[3] = b; }

Templ Inl void v2copy( T* a, T bx, T by )             { a[0] = bx;  a[1] = by; }
Templ Inl void v3copy( T* a, T bx, T by, T bz )       { a[0] = bx;  a[1] = by;  a[2] = bz; }
Templ Inl void v4copy( T* a, T bx, T by, T bz, T bw ) { a[0] = bx;  a[1] = by;  a[2] = bz;  a[3] = bw; }

Templ Inl void v2copy( T* a, T* b ) { a[0] = b[0];  a[1] = b[1]; }
Templ Inl void v3copy( T* a, T* b ) { a[0] = b[0];  a[1] = b[1];  a[2] = b[2]; }
Templ Inl void v4copy( T* a, T* b ) { a[0] = b[0];  a[1] = b[1];  a[2] = b[2];  a[3] = b[3]; }

Templ Inl T v2dot( T* a, T* b ) { return a[0] * b[0] + a[1] * b[1]; }
Templ Inl T v3dot( T* a, T* b ) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
Templ Inl T v4dot( T* a, T* b ) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]; }

Templ Inl T v2lensq( T* a ) { return v2dot( a, a ); }
Templ Inl T v3lensq( T* a ) { return v3dot( a, a ); }
Templ Inl T v4lensq( T* a ) { return v4dot( a, a ); }

Templ Inl T v2len( T* a ) { return Sqrt( v2dot( a, a ) ); }
Templ Inl T v3len( T* a ) { return Sqrt( v3dot( a, a ) ); }
Templ Inl T v4len( T* a ) { return Sqrt( v4dot( a, a ) ); }

Templ Inl T v2lenrec( T* a ) { return 1 / Sqrt( v2dot( a, a ) ); }
Templ Inl T v3lenrec( T* a ) { return 1 / Sqrt( v3dot( a, a ) ); }
Templ Inl T v4lenrec( T* a ) { return 1 / Sqrt( v4dot( a, a ) ); }

Templ Inl T v2projfrac( T* a, T* b ) { return v2dot( a, b ) / v2dot( a, a ); }
Templ Inl T v3projfrac( T* a, T* b ) { return v3dot( a, b ) / v3dot( a, a ); }
Templ Inl T v4projfrac( T* a, T* b ) { return v4dot( a, b ) / v4dot( a, a ); }

Templ Inl void v2add( T* a, T b ) { a[0] += b;  a[1] += b; }
Templ Inl void v3add( T* a, T b ) { a[0] += b;  a[1] += b;  a[2] += b; }
Templ Inl void v4add( T* a, T b ) { a[0] += b;  a[1] += b;  a[2] += b;  a[3] += b; }

Templ Inl void v2sub( T* a, T b ) { a[0] -= b;  a[1] -= b; }
Templ Inl void v3sub( T* a, T b ) { a[0] -= b;  a[1] -= b;  a[2] -= b; }
Templ Inl void v4sub( T* a, T b ) { a[0] -= b;  a[1] -= b;  a[2] -= b;  a[3] -= b; }

Templ Inl void v2mul( T* a, T b ) { a[0] *= b;  a[1] *= b; }
Templ Inl void v3mul( T* a, T b ) { a[0] *= b;  a[1] *= b;  a[2] *= b; }
Templ Inl void v4mul( T* a, T b ) { a[0] *= b;  a[1] *= b;  a[2] *= b;  a[3] *= b; }

Templ Inl void v2div( T* a, T b ) { T brec = 1 / b;  v2mul( a, brec ); }
Templ Inl void v3div( T* a, T b ) { T brec = 1 / b;  v3mul( a, brec ); }
Templ Inl void v4div( T* a, T b ) { T brec = 1 / b;  v4mul( a, brec ); }

Templ Inl void v2add( T* a, T* b ) { a[0] += b[0];  a[1] += b[1]; }
Templ Inl void v3add( T* a, T* b ) { a[0] += b[0];  a[1] += b[1];  a[2] += b[2]; }
Templ Inl void v4add( T* a, T* b ) { a[0] += b[0];  a[1] += b[1];  a[2] += b[2];  a[3] += b[3]; }

Templ Inl void v2sub( T* a, T* b ) { a[0] -= b[0];  a[1] -= b[1]; }
Templ Inl void v3sub( T* a, T* b ) { a[0] -= b[0];  a[1] -= b[1];  a[2] -= b[2]; }
Templ Inl void v4sub( T* a, T* b ) { a[0] -= b[0];  a[1] -= b[1];  a[2] -= b[2];  a[3] -= b[3]; }

Templ Inl void v2mul( T* a, T* b ) { a[0] *= b[0];  a[1] *= b[1]; }
Templ Inl void v3mul( T* a, T* b ) { a[0] *= b[0];  a[1] *= b[1];  a[2] *= b[2]; }
Templ Inl void v4mul( T* a, T* b ) { a[0] *= b[0];  a[1] *= b[1];  a[2] *= b[2];  a[3] *= b[3]; }

Templ Inl void v2div( T* a, T* b ) { a[0] /= b[0];  a[1] /= b[1]; }
Templ Inl void v3div( T* a, T* b ) { a[0] /= b[0];  a[1] /= b[1];  a[2] /= b[2]; }
Templ Inl void v4div( T* a, T* b ) { a[0] /= b[0];  a[1] /= b[1];  a[2] /= b[2];  a[3] /= b[3]; }

Templ Inl void v2abs( T* a ) { a[0] = abs( a[0] );  a[1] = abs( a[1] ); }
Templ Inl void v3abs( T* a ) { a[0] = abs( a[0] );  a[1] = abs( a[1] );  a[2] = abs( a[2] ); }
Templ Inl void v4abs( T* a ) { a[0] = abs( a[0] );  a[1] = abs( a[1] );  a[2] = abs( a[2] );  a[3] = abs( a[3] ); }

Templ Inl void v2norm( T* a ) { v2mul( a, v2lenrec( a ) ); }
Templ Inl void v3norm( T* a ) { v3mul( a, v3lenrec( a ) ); }
Templ Inl void v4norm( T* a ) { v4mul( a, v4lenrec( a ) ); }

Templ Inl T v3cross_x( T* a, T* b ) { return a[1] * b[2] - a[2] * b[1]; }
Templ Inl T v3cross_y( T* a, T* b ) { return a[2] * b[0] - a[0] * b[2]; }
Templ Inl T v3cross_z( T* a, T* b ) { return a[0] * b[1] - a[1] * b[0]; }

Templ Inl void v3cross( T* dst, T* a, T* b )
{
  dst[0] = v3cross_x( a, b );
  dst[1] = v3cross_y( a, b );
  dst[2] = v3cross_z( a, b );
}
