// Copyright (c) John A. Carlos Jr., all rights reserved.


Templ
struct vec2
{
  T x;
  T y;
};

Templ
struct vec3
{
  T x;
  T y;
  T z;
};

Templ
struct vec4
{
  T x;
  T y;
  T z;
  T w;
};



Templ Inl void Set( vec4<T>* dst, T x, T y, T z, T w ) { dst->x = x;  dst->y = y;  dst->z = z;  dst->w = w; }
Templ Inl void Set( vec3<T>* dst, T x, T y, T z )      { dst->x = x;  dst->y = y;  dst->z = z; }
Templ Inl void Set( vec2<T>* dst, T x, T y )           { dst->x = x;  dst->y = y; }


Templ Inl void Copy( vec4<T>* dst, vec4<T>& src )  { dst->x = src.x;  dst->y = src.y;  dst->z = src.z;  dst->w = src.w; }
Templ Inl void Copy( vec3<T>* dst, vec3<T>& src )  { dst->x = src.x;  dst->y = src.y;  dst->z = src.z; }
Templ Inl void Copy( vec2<T>* dst, vec2<T>& src )  { dst->x = src.x;  dst->y = src.y; }


Templ Inl void Zero( vec4<T>* dst )  { dst->x = 0;  dst->y = 0;  dst->z = 0;  dst->w = 0; }
Templ Inl void Zero( vec3<T>* dst )  { dst->x = 0;  dst->y = 0;  dst->z = 0; }
Templ Inl void Zero( vec2<T>* dst )  { dst->x = 0;  dst->y = 0; }


Templ Inl void
QuatMul( vec4<T>* dst, vec4<T>& a, vec4<T>& b )
{
  dst->x = a.w * b.x + a.x * b.w - a.y * b.z + a.z * b.y;
  dst->y = a.w * b.y + a.x * b.z + a.y * b.w - a.z * b.x;
  dst->z = a.w * b.z - a.x * b.y + a.y * b.x + a.z * b.w;
  dst->w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
}


Templ Inl void Add( vec4<T>* dst, vec4<T>& a, vec4<T>& b )  { dst->x = a.x + b.x;  dst->y = a.y + b.y;  dst->z = a.z + b.z;  dst->w = a.w + b.w; }
Templ Inl void Add( vec3<T>* dst, vec3<T>& a, vec3<T>& b )  { dst->x = a.x + b.x;  dst->y = a.y + b.y;  dst->z = a.z + b.z; }
Templ Inl void Add( vec2<T>* dst, vec2<T>& a, vec2<T>& b )  { dst->x = a.x + b.x;  dst->y = a.y + b.y; }

Templ Inl void Sub( vec4<T>* dst, vec4<T>& a, vec4<T>& b )  { dst->x = a.x - b.x;  dst->y = a.y - b.y;  dst->z = a.z - b.z;  dst->w = a.w - b.w; }
Templ Inl void Sub( vec3<T>* dst, vec3<T>& a, vec3<T>& b )  { dst->x = a.x - b.x;  dst->y = a.y - b.y;  dst->z = a.z - b.z; }
Templ Inl void Sub( vec2<T>* dst, vec2<T>& a, vec2<T>& b )  { dst->x = a.x - b.x;  dst->y = a.y - b.y; }

Templ Inl void Mul( vec4<T>* dst, vec4<T>& a, vec4<T>& b )  { dst->x = a.x * b.x;  dst->y = a.y * b.y;  dst->z = a.z * b.z;  dst->w = a.w * b.w; }
Templ Inl void Mul( vec3<T>* dst, vec3<T>& a, vec3<T>& b )  { dst->x = a.x * b.x;  dst->y = a.y * b.y;  dst->z = a.z * b.z; }
Templ Inl void Mul( vec2<T>* dst, vec2<T>& a, vec2<T>& b )  { dst->x = a.x * b.x;  dst->y = a.y * b.y; }

Templ Inl void Div( vec4<T>* dst, vec4<T>& a, vec4<T>& b )  { dst->x = a.x / b.x;  dst->y = a.y / b.y;  dst->z = a.z / b.z;  dst->w = a.w / b.w; }
Templ Inl void Div( vec3<T>* dst, vec3<T>& a, vec3<T>& b )  { dst->x = a.x / b.x;  dst->y = a.y / b.y;  dst->z = a.z / b.z; }
Templ Inl void Div( vec2<T>* dst, vec2<T>& a, vec2<T>& b )  { dst->x = a.x / b.x;  dst->y = a.y / b.y; }


Templ Inl void Add( vec4<T>* dst, vec4<T>& a, T b )  { dst->x = a.x + b;  dst->y = a.y + b;  dst->z = a.z + b;  dst->w = a.w + b; }
Templ Inl void Add( vec3<T>* dst, vec3<T>& a, T b )  { dst->x = a.x + b;  dst->y = a.y + b;  dst->z = a.z + b; }
Templ Inl void Add( vec2<T>* dst, vec2<T>& a, T b )  { dst->x = a.x + b;  dst->y = a.y + b; }

Templ Inl void Sub( vec4<T>* dst, vec4<T>& a, T b )  { dst->x = a.x - b;  dst->y = a.y - b;  dst->z = a.z - b;  dst->w = a.w - b; }
Templ Inl void Sub( vec3<T>* dst, vec3<T>& a, T b )  { dst->x = a.x - b;  dst->y = a.y - b;  dst->z = a.z - b; }
Templ Inl void Sub( vec2<T>* dst, vec2<T>& a, T b )  { dst->x = a.x - b;  dst->y = a.y - b; }

Templ Inl void Mul( vec4<T>* dst, vec4<T>& a, T b )  { dst->x = a.x * b;  dst->y = a.y * b;  dst->z = a.z * b;  dst->w = a.w * b; }
Templ Inl void Mul( vec3<T>* dst, vec3<T>& a, T b )  { dst->x = a.x * b;  dst->y = a.y * b;  dst->z = a.z * b; }
Templ Inl void Mul( vec2<T>* dst, vec2<T>& a, T b )  { dst->x = a.x * b;  dst->y = a.y * b; }

Templ Inl void Div( vec4<T>* dst, vec4<T>& a, T b )  { dst->x = a.x / b;  dst->y = a.y / b;  dst->z = a.z / b;  dst->w = a.w / b; }
Templ Inl void Div( vec3<T>* dst, vec3<T>& a, T b )  { dst->x = a.x / b;  dst->y = a.y / b;  dst->z = a.z / b; }
Templ Inl void Div( vec2<T>* dst, vec2<T>& a, T b )  { dst->x = a.x / b;  dst->y = a.y / b; }


Templ Inl void Add( vec4<T>* dst, T a, vec4<T>& b )  { dst->x = a + b.x;  dst->y = a + b.y;  dst->z = a + b.z;  dst->w = a + b.w; }
Templ Inl void Add( vec3<T>* dst, T a, vec3<T>& b )  { dst->x = a + b.x;  dst->y = a + b.y;  dst->z = a + b.z; }
Templ Inl void Add( vec2<T>* dst, T a, vec2<T>& b )  { dst->x = a + b.x;  dst->y = a + b.y; }

Templ Inl void Sub( vec4<T>* dst, T a, vec4<T>& b )  { dst->x = a - b.x;  dst->y = a - b.y;  dst->z = a - b.z;  dst->w = a - b.w; }
Templ Inl void Sub( vec3<T>* dst, T a, vec3<T>& b )  { dst->x = a - b.x;  dst->y = a - b.y;  dst->z = a - b.z; }
Templ Inl void Sub( vec2<T>* dst, T a, vec2<T>& b )  { dst->x = a - b.x;  dst->y = a - b.y; }

Templ Inl void Mul( vec4<T>* dst, T a, vec4<T>& b )  { dst->x = a * b.x;  dst->y = a * b.y;  dst->z = a * b.z;  dst->w = a * b.w; }
Templ Inl void Mul( vec3<T>* dst, T a, vec3<T>& b )  { dst->x = a * b.x;  dst->y = a * b.y;  dst->z = a * b.z; }
Templ Inl void Mul( vec2<T>* dst, T a, vec2<T>& b )  { dst->x = a * b.x;  dst->y = a * b.y; }

Templ Inl void Div( vec4<T>* dst, T a, vec4<T>& b )  { dst->x = a / b.x;  dst->y = a / b.y;  dst->z = a / b.z;  dst->w = a / b.w; }
Templ Inl void Div( vec3<T>* dst, T a, vec3<T>& b )  { dst->x = a / b.x;  dst->y = a / b.y;  dst->z = a / b.z; }
Templ Inl void Div( vec2<T>* dst, T a, vec2<T>& b )  { dst->x = a / b.x;  dst->y = a / b.y; }


Templ Inl void Add( vec4<T>* dst, vec4<T>& a )  { Add( dst, *dst, a ); }
Templ Inl void Add( vec3<T>* dst, vec3<T>& a )  { Add( dst, *dst, a ); }
Templ Inl void Add( vec2<T>* dst, vec2<T>& a )  { Add( dst, *dst, a ); }

Templ Inl void Sub( vec4<T>* dst, vec4<T>& a )  { Sub( dst, *dst, a ); }
Templ Inl void Sub( vec3<T>* dst, vec3<T>& a )  { Sub( dst, *dst, a ); }
Templ Inl void Sub( vec2<T>* dst, vec2<T>& a )  { Sub( dst, *dst, a ); }

Templ Inl void Mul( vec4<T>* dst, vec4<T>& a )  { Mul( dst, *dst, a ); }
Templ Inl void Mul( vec3<T>* dst, vec3<T>& a )  { Mul( dst, *dst, a ); }
Templ Inl void Mul( vec2<T>* dst, vec2<T>& a )  { Mul( dst, *dst, a ); }

Templ Inl void Div( vec4<T>* dst, vec4<T>& a )  { Div( dst, *dst, a ); }
Templ Inl void Div( vec3<T>* dst, vec3<T>& a )  { Div( dst, *dst, a ); }
Templ Inl void Div( vec2<T>* dst, vec2<T>& a )  { Div( dst, *dst, a ); }


Templ Inl void Add( vec4<T>* dst, T a )  { Add( dst, *dst, a ); }
Templ Inl void Add( vec3<T>* dst, T a )  { Add( dst, *dst, a ); }
Templ Inl void Add( vec2<T>* dst, T a )  { Add( dst, *dst, a ); }

Templ Inl void Sub( vec4<T>* dst, T a )  { Sub( dst, *dst, a ); }
Templ Inl void Sub( vec3<T>* dst, T a )  { Sub( dst, *dst, a ); }
Templ Inl void Sub( vec2<T>* dst, T a )  { Sub( dst, *dst, a ); }

Templ Inl void Mul( vec4<T>* dst, T a )  { Mul( dst, *dst, a ); }
Templ Inl void Mul( vec3<T>* dst, T a )  { Mul( dst, *dst, a ); }
Templ Inl void Mul( vec2<T>* dst, T a )  { Mul( dst, *dst, a ); }

Templ Inl void Div( vec4<T>* dst, T a )  { Div( dst, *dst, a ); }
Templ Inl void Div( vec3<T>* dst, T a )  { Div( dst, *dst, a ); }
Templ Inl void Div( vec2<T>* dst, T a )  { Div( dst, *dst, a ); }


Templ Inl void Floor( vec4<T>* dst, vec4<T>& a )  { dst->x = Floor( a.x );  dst->y = Floor( a.y );  dst->z = Floor( a.z );  dst->w = Floor( a.w ); }
Templ Inl void Floor( vec3<T>* dst, vec3<T>& a )  { dst->x = Floor( a.x );  dst->y = Floor( a.y );  dst->z = Floor( a.z ); }
Templ Inl void Floor( vec2<T>* dst, vec2<T>& a )  { dst->x = Floor( a.x );  dst->y = Floor( a.y ); }

Templ Inl void Ceil( vec4<T>* dst, vec4<T>& a )  { dst->x = Ceil( a.x );  dst->y = Ceil( a.y );  dst->z = Ceil( a.z );  dst->w = Ceil( a.w ); }
Templ Inl void Ceil( vec3<T>* dst, vec3<T>& a )  { dst->x = Ceil( a.x );  dst->y = Ceil( a.y );  dst->z = Ceil( a.z ); }
Templ Inl void Ceil( vec2<T>* dst, vec2<T>& a )  { dst->x = Ceil( a.x );  dst->y = Ceil( a.y ); }

Templ Inl void Floor( vec4<T>* dst )  { Floor( dst, *dst ); }
Templ Inl void Floor( vec3<T>* dst )  { Floor( dst, *dst ); }
Templ Inl void Floor( vec2<T>* dst )  { Floor( dst, *dst ); }

Templ Inl void Ceil( vec4<T>* dst )  { Ceil( dst, *dst ); }
Templ Inl void Ceil( vec3<T>* dst )  { Ceil( dst, *dst ); }
Templ Inl void Ceil( vec2<T>* dst )  { Ceil( dst, *dst ); }


Templ Inl T Dot( vec4<T> a, vec4<T> b )  { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
Templ Inl T Dot( vec3<T> a, vec3<T> b )  { return a.x*b.x + a.y*b.y + a.z*b.z; }
Templ Inl T Dot( vec2<T> a, vec2<T> b )  { return a.x*b.x + a.y*b.y; }

Templ Inl T ProjFrac( vec4<T>& v, vec4<T>& axis )  { return Dot( v, axis ) * RecLength( axis ); }
Templ Inl T ProjFrac( vec3<T>& v, vec3<T>& axis )  { return Dot( v, axis ) * RecLength( axis ); }
Templ Inl T ProjFrac( vec2<T>& v, vec2<T>& axis )  { return Dot( v, axis ) * RecLength( axis ); }

Templ Inl T MaxElem( vec4<T> v ) { return MAX4( v.x, v.y, v.z, v.w ); }
Templ Inl T MaxElem( vec3<T> v ) { return MAX3( v.x, v.y, v.z ); }
Templ Inl T MaxElem( vec2<T> v ) { return MAX( v.x, v.y ); }

Templ Inl T MinElem( vec4<T> v ) { return MIN4( v.x, v.y, v.z, v.w ); }
Templ Inl T MinElem( vec3<T> v ) { return MIN3( v.x, v.y, v.z ); }
Templ Inl T MinElem( vec2<T> v ) { return MIN( v.x, v.y ); }

Templ Inl T CrossX( vec3<T>& a, vec3<T>& b )  { return a.y*b.z - a.z*b.y; }
Templ Inl T CrossY( vec3<T>& a, vec3<T>& b )  { return a.z*b.x - a.x*b.z; }
Templ Inl T CrossZ( vec3<T>& a, vec3<T>& b )  { return a.x*b.y - a.y*b.x; }
Templ Inl T CrossZ( vec2<T>& a, vec2<T>& b )  { return a.x*b.y - a.y*b.x; }

Templ Inl void
Cross( vec3<T>* dst, vec3<T>& a, vec3<T>& b )
{
  T cx = CrossX( a, b );
  T cy = CrossY( a, b );
  T cz = CrossZ( a, b );
  dst->x = cx;
  dst->y = cy;
  dst->z = cz;
}

Templ Inl void Cross( vec3<T>* dst, vec3<T>& a ) { Cross( dst, *dst, a ); }



Templ Inl void
Perp( vec2<T>* dst, vec2<T>& a )
{
  dst->x = -a.y;
  dst->y = a.x;
}



Templ Inl void Abs( vec4<T>* dst, vec4<T>& a )  { dst->x = ABS( a.x );  dst->y = ABS( a.y );  dst->z = ABS( a.z );  dst->w = ABS( a.w ); }
Templ Inl void Abs( vec3<T>* dst, vec3<T>& a )  { dst->x = ABS( a.x );  dst->y = ABS( a.y );  dst->z = ABS( a.z ); }
Templ Inl void Abs( vec2<T>* dst, vec2<T>& a )  { dst->x = ABS( a.x );  dst->y = ABS( a.y ); }

Templ Inl void Abs( vec4<T>* dst )  { Abs( dst, *dst ); }
Templ Inl void Abs( vec3<T>* dst )  { Abs( dst, *dst ); }
Templ Inl void Abs( vec2<T>* dst )  { Abs( dst, *dst ); }

Templ Inl T Squared( vec4<T> v )  { return v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w; }
Templ Inl T Squared( vec3<T> v )  { return v.x*v.x + v.y*v.y + v.z*v.z; }
Templ Inl T Squared( vec2<T> v )  { return v.x*v.x + v.y*v.y; }

Templ Inl T Length( vec4<T> v )  { return Sqrt32( v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w ); }
Templ Inl T Length( vec3<T> v )  { return Sqrt32( v.x*v.x + v.y*v.y + v.z*v.z ); }
Templ Inl T Length( vec2<T> v )  { return Sqrt32( v.x*v.x + v.y*v.y ); }

Templ Inl T RecLength( vec4<T> v )  { return rsqrt( v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w ); }
Templ Inl T RecLength( vec3<T> v )  { return rsqrt( v.x*v.x + v.y*v.y + v.z*v.z ); }
Templ Inl T RecLength( vec2<T> v )  { return rsqrt( v.x*v.x + v.y*v.y ); }

Templ Inl T ComponentSum( vec4<T> v )  { return v.x + v.y + v.z + v.w; }
Templ Inl T ComponentSum( vec3<T> v )  { return v.x + v.y + v.z; }
Templ Inl T ComponentSum( vec2<T> v )  { return v.x + v.y; }

Templ Inl void Normalize( vec4<T>* dst, vec4<T>& a )  { Mul( dst, a, RecLength( a ) ); }
Templ Inl void Normalize( vec3<T>* dst, vec3<T>& a )  { Mul( dst, a, RecLength( a ) ); }
Templ Inl void Normalize( vec2<T>* dst, vec2<T>& a )  { Mul( dst, a, RecLength( a ) ); }

Templ Inl void Normalize( vec4<T>* dst )  { Normalize( dst, *dst ); }
Templ Inl void Normalize( vec3<T>* dst )  { Normalize( dst, *dst ); }
Templ Inl void Normalize( vec2<T>* dst )  { Normalize( dst, *dst ); }

Templ Inl void AddMul( vec4<T>* dst, vec4<T>& v, vec4<T>& s, vec4<T>& t )  { dst->x = v.x + s.x*t.x;  dst->y = v.y + s.y*t.y;  dst->z = v.z + s.z*t.z;  dst->w = v.w + s.w*t.w; }
Templ Inl void AddMul( vec3<T>* dst, vec3<T>& v, vec3<T>& s, vec3<T>& t )  { dst->x = v.x + s.x*t.x;  dst->y = v.y + s.y*t.y;  dst->z = v.z + s.z*t.z; }
Templ Inl void AddMul( vec2<T>* dst, vec2<T>& v, vec2<T>& s, vec2<T>& t )  { dst->x = v.x + s.x*t.x;  dst->y = v.y + s.y*t.y; }

Templ Inl void AddMul( vec4<T>* dst, vec4<T>& v, T s, vec4<T>& t )  { dst->x = v.x + s*t.x;  dst->y = v.y + s*t.y;  dst->z = v.z + s*t.z;  dst->w = v.w + s*t.w; }
Templ Inl void AddMul( vec3<T>* dst, vec3<T>& v, T s, vec3<T>& t )  { dst->x = v.x + s*t.x;  dst->y = v.y + s*t.y;  dst->z = v.z + s*t.z; }
Templ Inl void AddMul( vec2<T>* dst, vec2<T>& v, T s, vec2<T>& t )  { dst->x = v.x + s*t.x;  dst->y = v.y + s*t.y; }

Templ Inl void AddMul( vec4<T>* dst, vec4<T>& v, vec4<T>& s, T t )  { dst->x = v.x + s.x*t;  dst->y = v.y + s.y*t;  dst->z = v.z + s.z*t;  dst->w = v.w + s.w*t; }
Templ Inl void AddMul( vec3<T>* dst, vec3<T>& v, vec3<T>& s, T t )  { dst->x = v.x + s.x*t;  dst->y = v.y + s.y*t;  dst->z = v.z + s.z*t; }
Templ Inl void AddMul( vec2<T>* dst, vec2<T>& v, vec2<T>& s, T t )  { dst->x = v.x + s.x*t;  dst->y = v.y + s.y*t; }


Templ Inl void AddMul( vec4<T>* dst, vec4<T>& s, vec4<T>& t )  { AddMul( dst, *dst, s, t ); }
Templ Inl void AddMul( vec3<T>* dst, vec3<T>& s, vec3<T>& t )  { AddMul( dst, *dst, s, t ); }
Templ Inl void AddMul( vec2<T>* dst, vec2<T>& s, vec2<T>& t )  { AddMul( dst, *dst, s, t ); }

Templ Inl void AddMul( vec4<T>* dst, T s, vec4<T>& t )  { AddMul( dst, *dst, s, t ); }
Templ Inl void AddMul( vec3<T>* dst, T s, vec3<T>& t )  { AddMul( dst, *dst, s, t ); }
Templ Inl void AddMul( vec2<T>* dst, T s, vec2<T>& t )  { AddMul( dst, *dst, s, t ); }

Templ Inl void AddMul( vec4<T>* dst, vec4<T>& s, T t )  { AddMul( dst, *dst, s, t ); }
Templ Inl void AddMul( vec3<T>* dst, vec3<T>& s, T t )  { AddMul( dst, *dst, s, t ); }
Templ Inl void AddMul( vec2<T>* dst, vec2<T>& s, T t )  { AddMul( dst, *dst, s, t ); }



Templ Inl vec4<T> _vec4( T a, T b, T c, T d ) { vec4<T> r = { a, b, c, d };  return r; }
Templ Inl vec4<T> _vec4( T a ) { return _vec4( a, a, a, a ); }
Templ Inl vec4<T> _vec4() { return _vec4( Cast( T, 0 ) ); }

Templ Inl vec3<T> _vec3( T a, T b, T c ) { vec3<T> r = { a, b, c };  return r; }
Templ Inl vec3<T> _vec3( T a ) { return _vec3( a, a, a ); }
Templ Inl vec3<T> _vec3() { return _vec3( Cast( T, 0 ) ); }

Templ Inl vec2<T> _vec2( T a, T b ) { vec2<T> r = { a, b };  return r; }
Templ Inl vec2<T> _vec2( T a ) { return _vec2( a, a ); }
Templ Inl vec2<T> _vec2() { return _vec2( Cast( T, 0 ) ); }



Templ Inl vec4<T>
QuatMul( vec4<T>& a, vec4<T>& b )
{
  return _vec4(
    a.w * b.x + a.x * b.w - a.y * b.z + a.z * b.y,
    a.w * b.y + a.x * b.z + a.y * b.w - a.z * b.x,
    a.w * b.z - a.x * b.y + a.y * b.x + a.z * b.w,
    a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    );
}



Templ Inl vec4<T> operator+( vec4<T> a, vec4<T> b ) { return _vec4<T>( a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w ); }
Templ Inl vec3<T> operator+( vec3<T> a, vec3<T> b ) { return _vec3<T>( a.x + b.x, a.y + b.y, a.z + b.z ); }
Templ Inl vec2<T> operator+( vec2<T> a, vec2<T> b ) { return _vec2<T>( a.x + b.x, a.y + b.y ); }
Templ Inl vec4<T> operator-( vec4<T> a, vec4<T> b ) { return _vec4<T>( a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w ); }
Templ Inl vec3<T> operator-( vec3<T> a, vec3<T> b ) { return _vec3<T>( a.x - b.x, a.y - b.y, a.z - b.z ); }
Templ Inl vec2<T> operator-( vec2<T> a, vec2<T> b ) { return _vec2<T>( a.x - b.x, a.y - b.y ); }
Templ Inl vec4<T> operator*( vec4<T> a, vec4<T> b ) { return _vec4<T>( a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w ); }
Templ Inl vec3<T> operator*( vec3<T> a, vec3<T> b ) { return _vec3<T>( a.x * b.x, a.y * b.y, a.z * b.z ); }
Templ Inl vec2<T> operator*( vec2<T> a, vec2<T> b ) { return _vec2<T>( a.x * b.x, a.y * b.y ); }
Templ Inl vec4<T> operator/( vec4<T> a, vec4<T> b ) { return _vec4<T>( a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w ); }
Templ Inl vec3<T> operator/( vec3<T> a, vec3<T> b ) { return _vec3<T>( a.x / b.x, a.y / b.y, a.z / b.z ); }
Templ Inl vec2<T> operator/( vec2<T> a, vec2<T> b ) { return _vec2<T>( a.x / b.x, a.y / b.y ); }


// NOTE: very easy to misuse these guys; accidentally add something to all dimensions.
#if 0
Templ Inl vec4<T> operator+( vec4<T> a, T b ) { return _vec4<T>( a.x + b, a.y + b, a.z + b, a.w + b ); }
Templ Inl vec3<T> operator+( vec3<T> a, T b ) { return _vec3<T>( a.x + b, a.y + b, a.z + b ); }
Templ Inl vec2<T> operator+( vec2<T> a, T b ) { return _vec2<T>( a.x + b, a.y + b ); }
Templ Inl vec4<T> operator-( vec4<T> a, T b ) { return _vec4<T>( a.x - b, a.y - b, a.z - b, a.w - b ); }
Templ Inl vec3<T> operator-( vec3<T> a, T b ) { return _vec3<T>( a.x - b, a.y - b, a.z - b ); }
Templ Inl vec2<T> operator-( vec2<T> a, T b ) { return _vec2<T>( a.x - b, a.y - b ); }
Templ Inl vec4<T> operator+( T a, vec4<T> b ) { return _vec4<T>( a + b.x, a + b.y, a + b.z, a + b.w ); }
Templ Inl vec3<T> operator+( T a, vec3<T> b ) { return _vec3<T>( a + b.x, a + b.y, a + b.z ); }
Templ Inl vec2<T> operator+( T a, vec2<T> b ) { return _vec2<T>( a + b.x, a + b.y ); }
Templ Inl vec4<T> operator-( T a, vec4<T> b ) { return _vec4<T>( a - b.x, a - b.y, a - b.z, a - b.w ); }
Templ Inl vec3<T> operator-( T a, vec3<T> b ) { return _vec3<T>( a - b.x, a - b.y, a - b.z ); }
Templ Inl vec2<T> operator-( T a, vec2<T> b ) { return _vec2<T>( a - b.x, a - b.y ); }
Templ Inl vec4<T>& operator+=( vec4<T>& a, T b ) { a = a + b;  return a; }
Templ Inl vec3<T>& operator+=( vec3<T>& a, T b ) { a = a + b;  return a; }
Templ Inl vec2<T>& operator+=( vec2<T>& a, T b ) { a = a + b;  return a; }
Templ Inl vec4<T>& operator-=( vec4<T>& a, T b ) { a = a - b;  return a; }
Templ Inl vec3<T>& operator-=( vec3<T>& a, T b ) { a = a - b;  return a; }
Templ Inl vec2<T>& operator-=( vec2<T>& a, T b ) { a = a - b;  return a; }
#endif


Templ Inl vec4<T> operator*( vec4<T> a, T b ) { return _vec4<T>( a.x * b, a.y * b, a.z * b, a.w * b ); }
Templ Inl vec3<T> operator*( vec3<T> a, T b ) { return _vec3<T>( a.x * b, a.y * b, a.z * b ); }
Templ Inl vec2<T> operator*( vec2<T> a, T b ) { return _vec2<T>( a.x * b, a.y * b ); }
Templ Inl vec4<T> operator/( vec4<T> a, T b ) { return _vec4<T>( a.x / b, a.y / b, a.z / b, a.w / b ); }
Templ Inl vec3<T> operator/( vec3<T> a, T b ) { return _vec3<T>( a.x / b, a.y / b, a.z / b ); }
Templ Inl vec2<T> operator/( vec2<T> a, T b ) { return _vec2<T>( a.x / b, a.y / b ); }
Templ Inl vec4<T> operator*( T a, vec4<T> b ) { return _vec4<T>( a * b.x, a * b.y, a * b.z, a * b.w ); }
Templ Inl vec3<T> operator*( T a, vec3<T> b ) { return _vec3<T>( a * b.x, a * b.y, a * b.z ); }
Templ Inl vec2<T> operator*( T a, vec2<T> b ) { return _vec2<T>( a * b.x, a * b.y ); }
Templ Inl vec4<T> operator/( T a, vec4<T> b ) { return _vec4<T>( a / b.x, a / b.y, a / b.z, a / b.w ); }
Templ Inl vec3<T> operator/( T a, vec3<T> b ) { return _vec3<T>( a / b.x, a / b.y, a / b.z ); }
Templ Inl vec2<T> operator/( T a, vec2<T> b ) { return _vec2<T>( a / b.x, a / b.y ); }
Templ Inl vec4<T>& operator*=( vec4<T>& a, T b ) { a = a * b;  return a; }
Templ Inl vec3<T>& operator*=( vec3<T>& a, T b ) { a = a * b;  return a; }
Templ Inl vec2<T>& operator*=( vec2<T>& a, T b ) { a = a * b;  return a; }
Templ Inl vec4<T>& operator/=( vec4<T>& a, T b ) { a = a / b;  return a; }
Templ Inl vec3<T>& operator/=( vec3<T>& a, T b ) { a = a / b;  return a; }
Templ Inl vec2<T>& operator/=( vec2<T>& a, T b ) { a = a / b;  return a; }


Templ Inl vec4<T>& operator+=( vec4<T>& a, vec4<T> b ) { a = a + b;  return a; }
Templ Inl vec3<T>& operator+=( vec3<T>& a, vec3<T> b ) { a = a + b;  return a; }
Templ Inl vec2<T>& operator+=( vec2<T>& a, vec2<T> b ) { a = a + b;  return a; }
Templ Inl vec4<T>& operator-=( vec4<T>& a, vec4<T> b ) { a = a - b;  return a; }
Templ Inl vec3<T>& operator-=( vec3<T>& a, vec3<T> b ) { a = a - b;  return a; }
Templ Inl vec2<T>& operator-=( vec2<T>& a, vec2<T> b ) { a = a - b;  return a; }
Templ Inl vec4<T>& operator*=( vec4<T>& a, vec4<T> b ) { a = a * b;  return a; }
Templ Inl vec3<T>& operator*=( vec3<T>& a, vec3<T> b ) { a = a * b;  return a; }
Templ Inl vec2<T>& operator*=( vec2<T>& a, vec2<T> b ) { a = a * b;  return a; }
Templ Inl vec4<T>& operator/=( vec4<T>& a, vec4<T> b ) { a = a / b;  return a; }
Templ Inl vec3<T>& operator/=( vec3<T>& a, vec3<T> b ) { a = a / b;  return a; }
Templ Inl vec2<T>& operator/=( vec2<T>& a, vec2<T> b ) { a = a / b;  return a; }


Templ Inl bool operator==( vec4<T>& a, vec4<T> b ) { return ( a.x == b.x )  &  ( a.y == b.y )  &  ( a.z == b.z )  &  ( a.w == b.w ); }
Templ Inl bool operator==( vec3<T>& a, vec3<T> b ) { return ( a.x == b.x )  &  ( a.y == b.y )  &  ( a.z == b.z ); }
Templ Inl bool operator==( vec2<T>& a, vec2<T> b ) { return ( a.x == b.x )  &  ( a.y == b.y ); }


Templ Inl vec4<T> operator-( vec4<T> a ) { return _vec4( -a.x, -a.y, -a.z, -a.w ); }
Templ Inl vec3<T> operator-( vec3<T> a ) { return _vec3( -a.x, -a.y, -a.z ); }
Templ Inl vec2<T> operator-( vec2<T> a ) { return _vec2( -a.x, -a.y ); }



Templ Inl vec4<T> Floor( vec4<T> a ) { return _vec4( Floor32( a.x ), Floor32( a.y ), Floor32( a.z ), Floor32( a.w ) ); }
Templ Inl vec3<T> Floor( vec3<T> a ) { return _vec3( Floor32( a.x ), Floor32( a.y ), Floor32( a.z ) ); }
Templ Inl vec2<T> Floor( vec2<T> a ) { return _vec2( Floor32( a.x ), Floor32( a.y ) ); }

Templ Inl vec4<T> Ceil( vec4<T> a ) { return _vec4( Ceil32( a.x ), Ceil32( a.y ), Ceil32( a.z ), Ceil32( a.w ) ); }
Templ Inl vec3<T> Ceil( vec3<T> a ) { return _vec3( Ceil32( a.x ), Ceil32( a.y ), Ceil32( a.z ) ); }
Templ Inl vec2<T> Ceil( vec2<T> a ) { return _vec2( Ceil32( a.x ), Ceil32( a.y ) ); }

Templ Inl vec4<T> Abs( vec4<T> a ) { return _vec4( ABS( a.x ), ABS( a.y ), ABS( a.z ), ABS( a.w ) ); }
Templ Inl vec3<T> Abs( vec3<T> a ) { return _vec3( ABS( a.x ), ABS( a.y ), ABS( a.z ) ); }
Templ Inl vec2<T> Abs( vec2<T> a ) { return _vec2( ABS( a.x ), ABS( a.y ) ); }

Templ Inl vec4<T> Max( vec4<T> a, vec4<T> b ) { return _vec4( MAX( a.x, b.x ), MAX( a.y, b.y ), MAX( a.z, b.z ), MAX( a.w, b.w ) ); }
Templ Inl vec3<T> Max( vec3<T> a, vec3<T> b ) { return _vec3( MAX( a.x, b.x ), MAX( a.y, b.y ), MAX( a.z, b.z ) ); }
Templ Inl vec2<T> Max( vec2<T> a, vec2<T> b ) { return _vec2( MAX( a.x, b.x ), MAX( a.y, b.y ) ); }

Templ Inl vec4<T> Min( vec4<T> a, vec4<T> b ) { return _vec4( MIN( a.x, b.x ), MIN( a.y, b.y ), MIN( a.z, b.z ), MIN( a.w, b.w ) ); }
Templ Inl vec3<T> Min( vec3<T> a, vec3<T> b ) { return _vec3( MIN( a.x, b.x ), MIN( a.y, b.y ), MIN( a.z, b.z ) ); }
Templ Inl vec2<T> Min( vec2<T> a, vec2<T> b ) { return _vec2( MIN( a.x, b.x ), MIN( a.y, b.y ) ); }


Templ Inl vec4<T> Clamp( vec4<T> a, T s, T t ) { return _vec4( CLAMP( a.x, s, t ), CLAMP( a.y, s, t ), CLAMP( a.z, s, t ), CLAMP( a.w, s, t ) ); }
Templ Inl vec3<T> Clamp( vec3<T> a, T s, T t ) { return _vec3( CLAMP( a.x, s, t ), CLAMP( a.y, s, t ), CLAMP( a.z, s, t ) ); }
Templ Inl vec2<T> Clamp( vec2<T> a, T s, T t ) { return _vec2( CLAMP( a.x, s, t ), CLAMP( a.y, s, t ) ); }

Templ Inl vec4<T> Clamp( vec4<T> a, vec4<T> s, vec4<T> t ) { return _vec4( CLAMP( a.x, s.x, t.x ), CLAMP( a.y, s.y, t.y ), CLAMP( a.z, s.z, t.z ), CLAMP( a.w, s.w, t.w ) ); }
Templ Inl vec3<T> Clamp( vec3<T> a, vec3<T> s, vec3<T> t ) { return _vec3( CLAMP( a.x, s.x, t.x ), CLAMP( a.y, s.y, t.y ), CLAMP( a.z, s.z, t.z ) ); }
Templ Inl vec2<T> Clamp( vec2<T> a, vec2<T> s, vec2<T> t ) { return _vec2( CLAMP( a.x, s.x, t.x ), CLAMP( a.y, s.y, t.y ) ); }

Templ Inl vec4<T> Normalize( vec4<T> a ) { return a * RecLength( a ); }
Templ Inl vec3<T> Normalize( vec3<T> a ) { return a * RecLength( a ); }
Templ Inl vec2<T> Normalize( vec2<T> a ) { return a * RecLength( a ); }

#define CastVec4( T, /*vec4<S>*/ a ) _vec4( Cast( T, a.x ), Cast( T, a.y ), Cast( T, a.z ), Cast( T, a.w ) )
#define CastVec3( T, /*vec3<S>*/ a ) _vec3( Cast( T, a.x ), Cast( T, a.y ), Cast( T, a.z ) )
#define CastVec2( T, /*vec2<S>*/ a ) _vec2( Cast( T, a.x ), Cast( T, a.y ) )



Templ Inl vec3<T>
Cross( vec3<T> a, vec3<T> b )
{
  return _vec3( CrossX( a, b ), CrossY( a, b ), CrossZ( a, b ) );
}


Templ Inl vec2<T>
Perp( vec2<T> a )
{
  return _vec2( -a.y, a.x );
}


// REFERENCE:
//   http://orbit.dtu.dk/files/57573287/onb_frisvad_jgt2012.pdf
//
Templ Inl void
OrthonormalBasisGivenNorm(
  vec3<T>& t,
  vec3<T>& b,
  vec3<T> n, // assumed unit length.
  T epsilon
  )
{
  if( n.z < -1 + epsilon ) {
    t = _vec3<T>( 0, -1, 0 );
    b = _vec3<T>( -1, 0, 0 );
  } else {
    T c = 1 / ( 1 + n.z );
    T d = -n.x * n.y * c;
    t = _vec3<T>( 1 - n.x * n.x * c, d, -n.x );
    b = _vec3<T>( d, 1 - n.y * n.y * c, -n.y );
  }
}



Inl vec4<f64>
Lerp_from_vec4f64(
  vec4<f64>& y0,
  vec4<f64>& y1,
  vec4<f64>& x,
  vec4<f64>& x0,
  vec4<f64>& x1 )
{
  return _vec4(
    Lerp_from_f64( y0.x, y1.x, x.x, x0.x, x1.x ),
    Lerp_from_f64( y0.y, y1.y, x.y, x0.y, x1.y ),
    Lerp_from_f64( y0.z, y1.z, x.z, x0.z, x1.z ),
    Lerp_from_f64( y0.w, y1.w, x.w, x0.w, x1.w )
  );
}

Inl vec4<f32>
Lerp_from_vec4f32(
  vec4<f32>& y0,
  vec4<f32>& y1,
  vec4<f32>& x,
  vec4<f32>& x0,
  vec4<f32>& x1 )
{
  return _vec4(
    Lerp_from_f32( y0.x, y1.x, x.x, x0.x, x1.x ),
    Lerp_from_f32( y0.y, y1.y, x.y, x0.y, x1.y ),
    Lerp_from_f32( y0.z, y1.z, x.z, x0.z, x1.z ),
    Lerp_from_f32( y0.w, y1.w, x.w, x0.w, x1.w )
  );
}



Templ Inl vec4<T>
Lerp_from_s32(
  vec4<T>& y0,
  vec4<T>& y1,
  s32 x,
  s32 x0,
  s32 x1 )
{
  return _vec4<T>(
    Lerp_from_s32( y0.x, y1.x, x, x0, x1 ),
    Lerp_from_s32( y0.y, y1.y, x, x0, x1 ),
    Lerp_from_s32( y0.z, y1.z, x, x0, x1 ),
    Lerp_from_s32( y0.w, y1.w, x, x0, x1 )
  );
}

Templ Inl vec4<T>
Lerp_from_idx(
  vec4<T>& y0,
  vec4<T>& y1,
  idx_t x,
  idx_t x0,
  idx_t x1 )
{
  return _vec4<T>(
    Lerp_from_idx( y0.x, y1.x, x, x0, x1 ),
    Lerp_from_idx( y0.y, y1.y, x, x0, x1 ),
    Lerp_from_idx( y0.z, y1.z, x, x0, x1 ),
    Lerp_from_idx( y0.w, y1.w, x, x0, x1 )
  );
}



Inl vec2<f64>
Lerp_from_vec2f64(
  vec2<f64>& y0,
  vec2<f64>& y1,
  vec2<f64>& x,
  vec2<f64>& x0,
  vec2<f64>& x1 )
{
  return _vec2(
    Lerp_from_f64( y0.x, y1.x, x.x, x0.x, x1.x ),
    Lerp_from_f64( y0.y, y1.y, x.y, x0.y, x1.y )
  );
}

Inl vec2<f32>
Lerp_from_vec2f32(
  vec2<f32>& y0,
  vec2<f32>& y1,
  vec2<f32>& x,
  vec2<f32>& x0,
  vec2<f32>& x1 )
{
  return _vec2(
    Lerp_from_f32( y0.x, y1.x, x.x, x0.x, x1.x ),
    Lerp_from_f32( y0.y, y1.y, x.y, x0.y, x1.y )
  );
}



Inl vec2<f64>
Lerp_from_s32(
  vec2<f64>& y0,
  vec2<f64>& y1,
  s32 x,
  s32 x0,
  s32 x1 )
{
  return _vec2(
    Lerp_from_s32( y0.x, y1.x, x, x0, x1 ),
    Lerp_from_s32( y0.y, y1.y, x, x0, x1 )
  );
}

Inl vec2<f32>
Lerp_from_s32(
  vec2<f32>& y0,
  vec2<f32>& y1,
  s32 x,
  s32 x0,
  s32 x1 )
{
  return _vec2(
    Lerp_from_s32( y0.x, y1.x, x, x0, x1 ),
    Lerp_from_s32( y0.y, y1.y, x, x0, x1 )
  );
}



Inl vec2<f64>
Lerp_from_f64(
  vec2<f64>& y0,
  vec2<f64>& y1,
  f64 x,
  f64 x0,
  f64 x1 )
{
  return _vec2(
    Lerp_from_f64( y0.x, y1.x, x, x0, x1 ),
    Lerp_from_f64( y0.y, y1.y, x, x0, x1 )
  );
}

Inl vec2<f32>
Lerp_from_f32(
  vec2<f32>& y0,
  vec2<f32>& y1,
  f32 x,
  f32 x0,
  f32 x1 )
{
  return _vec2(
    Lerp_from_f32( y0.x, y1.x, x, x0, x1 ),
    Lerp_from_f32( y0.y, y1.y, x, x0, x1 )
  );
}



Inl bool
PtInBox( vec2<f32>& p, vec2<f32>& p00, vec2<f32>& p11, f32 epsilon )
{
  bool r =
    PtInInterval( p.x, p00.x, p11.x, epsilon )  &&
    PtInInterval( p.y, p00.y, p11.y, epsilon );
  return r;
}

Inl f32
DistanceToBox( vec2<f32>& p, vec2<f32>& p00, vec2<f32>& p11 )
{
  f32 r = 0.0f;
  if( p.x <= p00.x  &&  p.y <= p00.y ) {
    r = Length( p - p00 );
  } elif( p.x <= p00.x  &&  p11.y <= p.y ) {
    r = Length( p - _vec2( p00.x, p11.y ) );
  } elif( p11.x <= p.x  &&  p.y <= p00.y ) {
    r = Length( p - _vec2( p11.x, p00.y ) );
  } elif( p11.x <= p.x  &&  p11.y <= p.y ) {
    r = Length( p - p11 );
  } elif( p.x <= p00.x ) {
    r = p00.x - p.x;
  } elif( p11.x <= p.x ) {
    r = p.x - p11.x;
  } elif( p.y <= p00.y ) {
    r = p00.y - p.y;
  } elif( p11.y <= p.y ) {
    r = p.y - p11.y;
  } else {
    // inside the box.
  }
  return r;
}
Inl f32
DistanceToInterval( f32 p, f32 p0, f32 p1 )
{
  AssertCrash( p0 <= p1 );
  f32 r = 0.0f;
  if( p <= p0 ) {
    r = p0 - p;
  } elif( p1 <= p ) {
    r = p - p1;
  } else {
    // inside the interval.
  }
  return r;
}


Inl bool
PtInTriangle(
  vec2<f32>& p,
  vec2<f32>& p0,
  vec2<f32>& p1,
  vec2<f32>& p2,
  vec2<f32>& e0,
  vec2<f32>& e1,
  vec2<f32>& e2,
  f32 winding // either -1 or 1.
  )
{
  auto c0 = p - p0;
  auto c1 = p - p1;
  auto c2 = p - p2;
  auto f0 = winding * CrossZ( e0, c0 );
  auto f1 = winding * CrossZ( e1, c1 );
  auto f2 = winding * CrossZ( e2, c2 );
  bool r = ( f0 >= 0 )  &&  ( f1 >= 0 )  &&  ( f2 >= 0 );
  return r;
}



// bounds are [p0, p1), i.e. p1 is last_plus_one.
// this is to support nice mathematical tiling of space.
struct
rectf32_t
{
  vec2<f32> p0;
  vec2<f32> p1;
};

Inl rectf32_t
_rect( vec2<f32> p0, vec2<f32> p1 )
{
  rectf32_t r;
  r.p0 = p0;
  r.p1 = p1;
  return r;
}

Inl f32
AlignR(
  f32 x0,
  f32 x1,
  f32 w
  )
{
  return CLAMP( x1 - w, x0, x1 );
}

Inl vec2<f32>
AlignRight(
  rectf32_t bounds,
  f32 w
  )
{
  vec2<f32> r;
  auto x = bounds.p1.x - w;
  r.x = CLAMP( x, bounds.p0.x, bounds.p1.x );
  r.y = bounds.p0.y;
  return r;
}
Inl vec2<f32>
AlignRight(
  vec2<f32> p0,
  f32 x1,
  f32 w
  )
{
  vec2<f32> r;
  auto x = x1 - w;
  r.x = CLAMP( x, p0.x, x1 );
  r.y = p0.y;
  return r;
}
Inl rectf32_t
MoveEdgeXL(
  rectf32_t bounds,
  f32 w
  )
{
  rectf32_t r = bounds;
  r.p0.x = CLAMP( bounds.p0.x + w, bounds.p0.x, bounds.p1.x );
  return r;
}
Inl rectf32_t
MoveEdgeYL(
  rectf32_t bounds,
  f32 h
  )
{
  rectf32_t r = bounds;
  r.p0.y = CLAMP( bounds.p0.y + h, bounds.p0.y, bounds.p1.y );
  return r;
}

Inl vec2<f32>
AlignCenter(
  rectf32_t bounds,
  f32 w
  )
{
  vec2<f32> r;
  auto x = bounds.p0.x + 0.5f * ( ( bounds.p1.x - bounds.p0.x ) - w );
  r.x = CLAMP( x, bounds.p0.x, bounds.p1.x );
  r.y = bounds.p0.y;
  return r;
}

Inl vec2<f32>
AlignCenter(
  rectf32_t bounds,
  f32 w,
  f32 h
  )
{
  vec2<f32> r;
  auto x = bounds.p0.x + 0.5f * ( ( bounds.p1.x - bounds.p0.x ) - w );
  auto y = bounds.p0.y + 0.5f * ( ( bounds.p1.y - bounds.p0.y ) - h );
  r.x = CLAMP( x, bounds.p0.x, bounds.p1.x );
  r.y = CLAMP( y, bounds.p0.y, bounds.p1.y );
  return r;
}
