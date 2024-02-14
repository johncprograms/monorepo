// Copyright (c) John A. Carlos Jr., all rights reserved.

Templ Inl void
AddBackContents(
  stack_resizeable_cont_t<T>* array,
  tslice_t<T> contents
  )
{
  Memmove( AddBack( *array, contents.len ), contents.mem, contents.len * sizeof( T ) );
}
Templ Inl void
AddBackContents(
  stack_resizeable_cont_t<T>* array,
  tslice32_t<T> contents
  )
{
  Memmove( AddBack( *array, contents.len ), contents.mem, contents.len * sizeof( T ) );
}

Inl void
AddBackCStr(
  stack_resizeable_cont_t<u8>* array,
  const void* cstr
  )
{
  auto text = SliceFromCStr( cstr );
  AddBackContents( array, text );
}

Inl void
AddBackF64(
  stack_resizeable_cont_t<u8>* array,
  f64 src,
  u8 num_decimal_places = 7
  )
{
  stack_nonresizeable_stack_t<u8, 32> tmp;
  CsFrom_f64( tmp.mem, Capacity( tmp ), &tmp.len, src, num_decimal_places );
  AddBackContents( array, SliceFromArray( tmp ) );
}
Inl void
AddBackF32(
  stack_resizeable_cont_t<u8>* array,
  f32 src,
  u8 num_decimal_places = 7
  )
{
  stack_nonresizeable_stack_t<u8, 32> tmp;
  CsFrom_f32( tmp.mem, Capacity( tmp ), &tmp.len, src, num_decimal_places );
  AddBackContents( array, SliceFromArray( tmp ) );
}

Templ Inl void
AddBackUInt(
  stack_resizeable_cont_t<u8>* array,
  T src,
  bool use_separator = 0
  )
{
  stack_nonresizeable_stack_t<u8, 32> tmp;
  CsFromIntegerU( tmp.mem, Capacity( tmp ), &tmp.len, src, use_separator );
  AddBackContents( array, SliceFromArray( tmp ) );
}
Templ Inl void
AddBackSInt(
  stack_resizeable_cont_t<u8>* array,
  T src,
  bool use_separator = 0
  )
{
  stack_nonresizeable_stack_t<u8, 32> tmp;
  CsFromIntegerS( tmp.mem, Capacity( tmp ), &tmp.len, src, use_separator );
  AddBackContents( array, SliceFromArray( tmp ) );
}

