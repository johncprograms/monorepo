
// arbitrary-length contiguous string of memory.
// short-lived view on other memory.
// note this is zero-initialized
Templ struct
tslice_t
{
  T* mem;
  idx_t len;
};
Templ struct
tslice32_t
{
  T* mem;
  u32 len;
};

using slice_t = tslice_t<u8>;
using slice32_t = tslice32_t<u8>;

Templ ForceInl bool
EqualContents( tslice_t<T> a, tslice_t<T> b )
{
  return MemEqual( ML( a ), ML( b ) );
}
Templ ForceInl bool
EqualContents( tslice32_t<T> a, tslice32_t<T> b )
{
  return MemEqual( ML( a ), ML( b ) );
}
Templ ForceInl bool
EqualContents( tslice_t<T>* a, tslice_t<T>* b )
{
  return MemEqual( ML( *a ), ML( *b ) );
}
Templ ForceInl bool
EqualContents( tslice32_t<T>* a, tslice32_t<T>* b )
{
  return MemEqual( ML( *a ), ML( *b ) );
}

Templ ForceInl idx_t
HashContents( tslice_t<T> a )
{
  return StringHash( ML( a ) );
}
Templ ForceInl idx_t
HashContents( tslice32_t<T> a )
{
  return StringHash( ML( a ) );
}
Templ ForceInl idx_t
HashContents( tslice_t<T>* a )
{
  return StringHash( ML( *a ) );
}
Templ ForceInl idx_t
HashContents( tslice32_t<T>* a )
{
  return StringHash( ML( *a ) );
}

Templ Inl bool
EqualBounds( tslice_t<T> a, tslice_t<T> b )
{
  bool r = a.mem == b.mem  &&  a.len == b.len;
  return r;
}

Templ Inl void
ZeroContents( tslice_t<T> a )
{
  Memzero( a.mem, a.len * sizeof( T ) );
}

Templ Inl tslice32_t<T>
Slice32FromSlice( tslice_t<T> a )
{
  AssertCrash( a.len <= MAX_u32 );
  tslice32_t<T> r;
  r.mem = a.mem;
  r.len = Cast( u32, a.len );
  return r;
}

#define SliceFromCArray( T, carray )   tslice_t<T>{ AL( carray ) }