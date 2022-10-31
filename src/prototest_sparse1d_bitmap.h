// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

bitarray = 0100 0011
conceptual_array = { 0, 1, 2, 3, 4, 5, 6, 7 }
array = { 0, 1, 6 }
array_len = popcount( bitarray )
say we want conceptual index 6, which should come back as 2 in this case.
     bitarray = 0100 0011
            6 = 0000 0110
       1 << 6 = 0100 0000
which is a power of two by definition.
so we can subtract one to get the mask of the bits to the right.
 (1 << 6) - 1 = 0011 1111
then if we AND that with bitarray, we can then count the number of bits to the right.
array_idx_from_conceptual_idx(ci) = popcount( ( ( 1 << ci ) - 1 ) & bitarray )

what about the inverse mapping?
given array index 2, we should return conceptual index 6 in this case.
     bitarray = 0100 0011
       ai = 2 = 0000 0010
we could use a loop and lzcnt to iterate to find the 2'th bit that's set.
or if the 2'th bit is closer to the end, we could do the reverse loop. Taking us to O(W/2).
i think that's the best we could do, unless we start storing an extra table so we get O(1) access.
e.g.
  idx_t ci_from_ai[] = { 0, 1, 6 }.
then we just have a simple table lookup:
conceptual_idx_from_array_idx(ai) = ci_from_ai[ai]

#endif

// fixed length array
//
struct
sparse_array_t
{
  u64 bitarray;
  idx_t len;
//  idx_t* conceptual_index_from_array_index; // length len
  T* elems; // length len
};
Inl void
Alloc( sparse_array_t* a, u64 bitarray )
{
  auto len = _popcnt_idx_t( bitarray );
  a->len = len;
  a->bitarray = bitarray;
//  a->conceptual_index_from_array_index = MemHeapAlloc( idx_t, len );
  a->elems = MemHeapAlloc( T, len );
}
Inl void
Free( sparse_array_t* a )
{
  MemHeapFree( a->elems );
//  MemHeapFree( a->conceptual_index_from_array_index );
}
Inl T*
Element( sparse_array_t* a, idx_t conceptual_index )
{
  auto bitarray = a->bitarray;
  auto elems = a->elems;
  auto array_index = _popcnt_idx_t( ( ( 1 << conceptual_index ) - 1 ) & bitarray );
  AssertCrash( array_index < 64 );
  auto result = elems + array_index;
  return result;
}
