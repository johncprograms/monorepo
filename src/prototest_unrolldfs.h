// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

Inl void
VerifyFoundDFS( tslice_t<n_t> found_nodes, n_t* expected, idx_t expected_len )
{
//  AssertCrash( found_nodes.len == expected_len );
//  AssertCrash( MemEqual( ML( found_nodes ), expected, expected_len
}

void A(u32 a, u32 i)
{
  printf( "A(%u, %u)\n", a, i );
}
void B(u32 a, u32 i)
{
  printf( "B(%u, %u)\n", a, i );
}
void Recurse(u32 a, u32 i)
{
  A(a, i);

  u32 count = 0;
  if( a == 0 ) count = 3;
  elif( a == 1 ) count = 1;

  u32 b = a + 1;
  Fori( u32, j, 0, count ) {
    Recurse(b, j);
  }

  B(a, i);
}


// TODO: the i and k are all wrong.
// at least the a is correct!
// and our termination condition is also correct!
// k is correct for A, but not for B.

struct pair_t { u32 a; u32 i; u32 k; u32 c; };
void A(pair_t* p)
{
  printf( "A(%u, %u, %u)\n", p->a, p->i, p->k );
}
void B(pair_t* p)
{
  printf( "B(%u, %u, %u)\n", p->a, p->i, p->k );
}
void Iterative()
{
#define SETC(x) \
  if( x.a == 0 ) x.c = 3; \
  elif( x.a == 1 ) x.c = 1; \
  else x.c = 0; \

  pair_t buffer[100];
  u32 idx = 0;
  pair_t a = { 0, 0, 0 };
  SETC( a );
  pair_t b;

Entry:
  A( &a );

  if( a.c ) {
    // Construct the first child:
    b = { a.a + 1, 0, 0 };
    SETC( b );

Loop:
    buffer[idx++] = a; // we'll restore to this on 'exit'
    a = b;
    goto Entry;

Exit:
    if( a.k + 1 < a.c ) {
      // Construct the next sibling:
      b = { a.a + 1, a.i + 1, a.k + 1 };
      SETC( b );
      a.k += 1;
      goto Loop;
    }

    goto LoopDone;
  }

LoopDone:
  B( &a );

  if( idx ) {
    idx -= 1;
    a = buffer[idx];
    goto Exit;
  }
}
static void TestXXXXXX()
{
  printf( "Recursive:\n" );
  Recurse(0, 0);

  printf( "Iterative:\n" );
  Iterative();
}
#endif
