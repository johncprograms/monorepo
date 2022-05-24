// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// there's a few categories of things we'll talk about here:
// 1. directed trees
// 2. directed forests
// 3. directed acyclic graphs
// 4. directed graphs
// 5. directed multigraphs
//
//
// 1. directed trees
// these are singular connected components, for example:
// - a singular node with no edges
// - { A -> B }
// - { A -> B -> C }
// - { A -> B, A -> C, B -> D }
// the key property is that one root node has indegree 0, and all other nodes have indegree 1.
// so you can't have { A -> C, B -> C }, for example. this _would_ be a directed tree if we
// reversed the edge directions, but it's not a directed tree as is.
// this leads to some subtlety around how you'd represent undirected trees!
// this is a classic problem with undirected trees; how do you do traversal both upwards and
// downwards in an efficient way?
// there are solutions like parent-lists, sibling-lists, etc. in the theory of undirected trees.
// since the structure of undirected trees is usually important, for the purposes of this code,
// we'll just assume it's given in the directed tree form. maybe as we get further along, we'll
// add some stronger undirected tree functionality.
//
// the DFS or BFS will traverse exactly one directed tree, with the starting point as the root.
//
//
// 2. directed forests
// these are simply collections of directed trees. so take any number of them, share the same
// backing datastructures, and you've got a forest.
// an interesting emergent property is the list of root nodes of each directed tree.
//
// the DFSComp will traverse one directed forest, and delineate where each 'component' or
// directed tree is.
//
//
// 3. directed acyclic graphs
// these are more general than directed forests, in that they give up the tree property.
// but, they maintain a weaker property, of containing no cycles. so for example, these aren't
// allowed as directed acyclic graphs:
// - { A -> A } "self-loops"
// - { A -> B, B -> A } "2-cycle"
// - { A -> B, B -> C, C -> A } "3-cycle"
// any other kind of connections are possible, including the diamond patterns like:
// - { A -> C, B -> C }
// - { A -> B, A -> C, B -> D, C -> D }
// cycles are a troublesome property, in that they prevent a total ordering of nodes.
//
// TopologicalSort will traverse one directed acyclic graph, and return the total node ordering.
//
//
// 4. directed graphs
// these are more general than the above; they give up the acyclical property.
// the only key property here is that you aren't allowed to have duplicate edges. so for example,
// these aren't allowed:
// - { A -> A, A -> A }
// - { A -> B, A -> B }
// otherwise; cycles are allowed, self-loops are allowed, diamonds are allowed, etc.
//
//
// 5. directed multigraphs
// these are the most general graphs; same as directed graphs, but all duplicated edges are allowed.
// these aren't usually useful outside of a temporary data scrubbing step, so there's not much of
// algorithmic interest here. duplicate edges don't really get you any extra connectivity, other
// than perhaps as a score per de-duplicated edge. at which point, you'd just have a weighted-edge
// directed graph.
// there's maybe some applications for dynamic graphs, e.g. removing duplicate edges as you traverse
// them.
// but i don't as of yet have any practical reason to worry about these. so we'll largely ignore
// these, and just have some code to transform these into directed graphs.
//
//
//
//
//
// graph statistics:
//
// distributions of:
// - outdegree[N]
// - indegree[N]
// - degree[N]
// - # of nodes in CCs[C]
// - # of edges in CCs[D]
// - all-pairs path lengths[N choose 2]
// - path length starting at a given node[C-1]
// - unique cycle length [Cyc]
//
// diameter
// center
// ratio of edges present over possible, aka E / ( N choose 2 )
//
//
// graph algorithms:
//
// Topological Sort DAG
// DFS
// BFS
// Invert directionality
// ReverseDFS
// ReverseBFS
// DFS Comp
// Kosaraju Comp
// Tarjan Comp
// Find unique cycles
// Break cycles with fewest edge deletions
// Shortest paths
// BFS Shortest path for unweighted
// Longest paths
// Widest paths
// Max flow / min cut
// Find all paths
// Find diameter
// Find center
// Hamiltonian circuits
// Euler circuits / Hierholzer's alg
// TSP
// Spanning tree
// Min spanning tree
// Collapse cliques
// Generate random graph given degree distribution
// Bipartite matching
// Layout / visualization
// Adj. matrix calculations.
// Markov Chain calculations
//

using n_t = u32;
using e_t = u64;

#define MAX_n_t   MAX_u32

#define SETBIT( bitarray, offset )   bitarray[ ( offset ) / 64u ] |= ( 1ULL << ( ( offset ) % 64u ) )
#define GETBIT( bitarray, offset )   bitarray[ ( offset ) / 64u ] & ( 1ULL << ( ( offset ) % 64u ) )
#define CLRBIT( bitarray, offset )   bitarray[ ( offset ) / 64u ] &= ~( 1ULL << ( ( offset ) % 64u ) )

void
DfsIntoArrays(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  n_t start_node,
  u64* bits_visited, // bitarray of length N
  array_t<n_t>* found_nodes,
  array_t<n_t>* stack
  )
{
  ProfFunc();
  stack->len = 0;
  *AddBack( *stack ) = start_node;
  while( stack->len ) {
    auto u = stack->mem[ stack->len - 1 ];
    RemBack( *stack );
    auto div = u / 64u;
    auto rem = u % 64u;
    auto visited = bits_visited[ div ] & ( 1ULL << rem );
    if( !visited ) {
      bits_visited[ div ] |= ( 1ULL << rem );
      auto outdegree = outdegrees[ u ];
      auto outoffset = outoffsets[ u ];
      Fori( e_t, e, 0, outdegree ) {
        auto v = outnodes[ outoffset + e ];
        *AddBack( *stack ) = v;
      }
      *AddBack( *found_nodes ) = u;
    }
  }
}

//
// This version overwrites the buffer, in some cases.
// i.e. we can hit the AssertCrash( stack_len <= found_idx ).
// The root cause is us sticking already-visited nodes on the buffer's stack.
// It's got insufficient checks of bits_visited.
//

#if 0
  n_t stack_len = 1;
  n_t found_idx = N;
  buffer[0] = start_node;
  while( stack_len ) {
    auto u = buffer[ --stack_len ];
    auto div = u / 64u;
    auto rem = u % 64u;
    auto visited = bits_visited[ div ] & ( 1ULL << rem );
    if( !visited ) {
      bits_visited[ div ] |= ( 1ULL << rem );
      auto outdegree = outdegrees[ u ];
      auto outoffset = outoffsets[ u ];
      Fori( e_t, e, 0, outdegree ) {
        auto v = outnodes[ outoffset + e ];
        buffer[ stack_len++ ] = v;
      }
      AssertCrash( stack_len <= found_idx );
      buffer[ --found_idx ] = u;
    }
  }
  found_nodes->mem = buffer + found_idx;
  found_nodes->len = N - found_idx;
#endif

void
DfsIntoBufferBack(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  n_t start_node,
  u64* bits_visited, // bitarray of length N
  n_t* buffer, // length N
  tslice_t<n_t>* found_nodes
  )
{
  ProfFunc();
  AssertCrash( N );

  // Whenever we stick a node in the buffer's stack, we mark the 'visited' bit for it then.
  // This ensures we never stack up more than N nodes in the buffer, and hence always have
  // room to share the buffer with the 'found' list.

  n_t stack_len = 0;
  n_t found_idx = N;

  {
    auto u = start_node;
    SETBIT( bits_visited, u );
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto visited_v = GETBIT( bits_visited, v );
      AssertCrash( !visited_v ); // Unsure if this assumption is worth keeping, just for unrolling.
      buffer[ stack_len++ ] = v;
      SETBIT( bits_visited, v );
    }
    AssertCrash( stack_len <= found_idx );
    buffer[ --found_idx ] = u;
  }

  while( stack_len ) {
    auto u = buffer[ --stack_len ];
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto visited = GETBIT( bits_visited, v );
      if( !visited ) {
        buffer[ stack_len++ ] = v;
        SETBIT( bits_visited, v );
      }
    }
    AssertCrash( stack_len <= found_idx );
    buffer[ --found_idx ] = u;
  }
  found_nodes->mem = buffer + found_idx;
  found_nodes->len = N - found_idx;
}

void
DfsIntoBuffer(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  n_t start_node,
  u64* bits_visited, // bitarray of length N
  n_t* buffer, // length N
  n_t* found_len_ // { buffer, found_len_ } contains all nodes found
  )
{
  ProfFunc();
  AssertCrash( N );

  // Whenever we stick a node in the buffer's stack, we mark the 'visited' bit for it then.
  // This ensures we never stack up more than N nodes in the buffer, and hence always have
  // room to share the buffer with the 'found' list.

  n_t stack_idx = N;
  n_t found_len = 0;

  {
    auto u = start_node;
    SETBIT( bits_visited, u );
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto visited_v = GETBIT( bits_visited, v );
      AssertCrash( !visited_v ); // Unsure if this assumption is worth keeping, just for unrolling.
      buffer[ --stack_idx ] = v;
      SETBIT( bits_visited, v );
    }
    AssertCrash( found_len <= stack_idx );
    buffer[ found_len++ ] = u;
  }

  while( stack_idx < N ) {
    auto u = buffer[ stack_idx++ ];
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto visited = GETBIT( bits_visited, v );
      if( !visited ) {
        buffer[ --stack_idx ] = v;
        SETBIT( bits_visited, v );
      }
    }
    AssertCrash( found_len <= stack_idx );
    buffer[ found_len++ ] = u;
  }
  *found_len_ = found_len;
}

// For use by DfsComp which doesn't want buffer shared by the stack and found nodes.
void
DfsIntoTwoBuffers(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  n_t start_node,
  u64* bits_visited, // bitarray of length N
  n_t* buffer, // length N
  n_t* found, // length N, no bounds checking.
  n_t* found_len_
  )
{
  ProfFunc();
  AssertCrash( N );
  AssertCrash( buffer != found );

  n_t stack_len = 0;
  n_t found_len = 0;

  {
    auto u = start_node;
    SETBIT( bits_visited, u );
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto visited_v = GETBIT( bits_visited, v );
      AssertCrash( !visited_v ); // Unsure if this assumption is worth keeping, just for unrolling.
      buffer[ stack_len++ ] = v;
      SETBIT( bits_visited, v );
    }
    *found++ = u;
    found_len += 1;
  }

  while( stack_len ) {
    auto u = buffer[ --stack_len ];
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto visited = GETBIT( bits_visited, v );
      if( !visited ) {
        buffer[ stack_len++ ] = v;
        SETBIT( bits_visited, v );
      }
    }
    *found++ = u;
    found_len += 1;
  }
  *found_len_ = found_len;
}

// For use by ToplogicalSort
void
DfsTopSort(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  n_t start_node,
  u64* bits_visited, // bitarray of length N
  n_t* buffer, // length N
  n_t* sort_position_idx,
  n_t* sort_position // length N, no bounds checking. [i] is the sorted position of node i.
  )
{
#if 0
  ProfFunc();
  AssertCrash( N );
  AssertCrash( buffer != sort_position );

  n_t stack_len = 0;
  n_t found_len = 0;

  {
    auto u = start_node;
    buffer[ stack_len++ ] = u;
    SETBIT( bits_visited, u );
  }

  while( stack_len ) {
    auto u = buffer[ --stack_len ];
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto visited = GETBIT( bits_visited, v );
      if( !visited ) {
        buffer[ stack_len++ ] = v;
        SETBIT( bits_visited, v );
      }
    }
    sort_position[ u ] = --*sort_position_idx;
  }
#endif
}

void
BfsIntoTwoBuffers(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  n_t start_node,
  u64* bits_visited, // bitarray of length N
  n_t* buffer, // length N
  n_t* found, // length N, no bounds checking.
  n_t* found_len_
  )
{
  ProfFunc();
  AssertCrash( N );

  auto queue = buffer;
  auto queue_len = N;
  n_t pos_wr = 0;
  n_t pos_rd = 0;

  n_t found_len = 0;

  {
    auto u = start_node;
    SETBIT( bits_visited, u );
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
#define BULK_BFS_COPY 0
#if BULK_BFS_COPY
    auto v = outnodes + outoffset + e;
    EnqueueAssumingRoom( queue, queue_len, &pos_wr, pos_rd, v, outdegree );
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      SETBIT( bits_visited, v );
    }
#else
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      // TODO: could do one bulk copy, no need for iterative copying.
      EnqueueAssumingRoom( queue, queue_len, &pos_wr, pos_rd, &v );
      SETBIT( bits_visited, v );
    }
#endif
    *found++ = u;
    found_len += 1;
  }

  while( pos_wr != pos_rd ) {
    n_t u;
    DequeueAssumingRoom( queue, queue_len, pos_wr, &pos_rd, &u );
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto visited = GETBIT( bits_visited, v );
      if( !visited ) {
        EnqueueAssumingRoom( queue, queue_len, &pos_wr, pos_rd, &v );
        SETBIT( bits_visited, v );
      }
    }
    *found++ = u;
    found_len += 1;
  }
  *found_len_ = found_len;
}

void
ShortestPathUnweightedWithBFS(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  n_t start_node,
  u64* bits_visited, // bitarray of length N
  n_t* buffer, // length N
  n_t* found, // length N, no bounds checking.
  n_t* found_len_,
  n_t* shortest_distance, // length N
  n_t* previous_node_in_path // length N
  )
{
  ProfFunc();
  AssertCrash( N );

  auto queue = buffer;
  auto queue_len = N;
  n_t pos_wr = 0;
  n_t pos_rd = 0;

  n_t found_len = 0;

  Fori( n_t, u, 0, N ) {
    shortest_distance[u] = MAX_n_t;
    previous_node_in_path[u] = MAX_n_t;
  }
  shortest_distance[start_node] = 0;

  {
    auto u = start_node;
    SETBIT( bits_visited, u );
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
#define BULK_BFS_COPY 0
#if BULK_BFS_COPY
    auto v = outnodes + outoffset + e;
    EnqueueAssumingRoom( queue, queue_len, &pos_wr, pos_rd, v, outdegree );
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      SETBIT( bits_visited, v );
    }
#else
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      // TODO: could do one bulk copy, no need for iterative copying.
      EnqueueAssumingRoom( queue, queue_len, &pos_wr, pos_rd, &v );
      SETBIT( bits_visited, v );
    }
#endif
    *found++ = u;
    found_len += 1;
  }

  while( pos_wr != pos_rd ) {
    n_t u;
    DequeueAssumingRoom( queue, queue_len, pos_wr, &pos_rd, &u );
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto visited = GETBIT( bits_visited, v );
      if( !visited ) {
        shortest_distance[v] = shortest_distance[u] + 1;
        previous_node_in_path[v] = u;
        EnqueueAssumingRoom( queue, queue_len, &pos_wr, pos_rd, &v );
        SETBIT( bits_visited, v );
      }
    }
    *found++ = u;
    found_len += 1;
  }
  *found_len_ = found_len;
}

//
// given a directed graph, this produces a new directed tree by doing a DFS.
// note that 'result_old_from_new' contains the mapping from the new node values to the old ones.
//
void
DirectedTreeFromDirectedGraphWithDfs(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  n_t start_node,
  u64* bits_visited, // bitarray of length N
  n_t* buffer, // length N
  n_t* result_N_,
  e_t* result_E_,
  n_t* result_old_from_new, // length C, which is O(N), because this is a tree.
  e_t* result_offsets, // length C
  e_t* result_degrees, // length C
  n_t* result_outnodes // length C
  )
{
  ProfFunc();
  AssertCrash( N );

  e_t result_E = 0;
  n_t result_N = 0;

  n_t stack_len = 0;

  {
    auto u = start_node;
    SETBIT( bits_visited, u );
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];

    auto u_new = result_N++;
    result_old_from_new[u_new] = u;
    auto u_outoffset = result_E;
    auto u_outdegree = outdegree; // we know this ahead of time for start_node; not so for other nodes.

    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      buffer[ stack_len++ ] = v;
      SETBIT( bits_visited, v );
      result_outnodes[ result_E++ ] = v;
    }

    result_offsets[u_new] = u_outoffset;
    result_degrees[u_new] = u_outdegree;
  }

  while( stack_len ) {
    auto u = buffer[ --stack_len ];
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];

    auto u_new = result_N++;
    result_old_from_new[u_new] = u;
    auto u_outoffset = result_E;

    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto visited = GETBIT( bits_visited, v );
      if( !visited ) {
        buffer[ stack_len++ ] = v;
        SETBIT( bits_visited, v );
        result_outnodes[ result_E++ ] = v;
      }
    }
    // account for edges we skipped due to the visited check above.
    auto u_outdegree = result_E - u_outoffset;

    result_offsets[u_new] = u_outoffset;
    result_degrees[u_new] = u_outdegree;
  }

  *result_N_ = result_N;
  *result_E_ = result_E;
}

//
// given a directed graph, this produces a new directed tree by doing a BFS.
// note that 'result_old_from_new' contains the mapping from the new node values to the old ones.
//
void
DirectedTreeFromDirectedGraphWithBfs(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  n_t start_node,
  u64* bits_visited, // bitarray of length N
  n_t* buffer, // length N
  n_t* result_N_,
  e_t* result_E_,
  n_t* result_old_from_new, // length C, which is O(N)
  e_t* result_offsets, // length C
  e_t* result_degrees, // length C
  n_t* result_outnodes // length D, which is O(E)
  )
{
  ProfFunc();
  AssertCrash( N );

  e_t result_E = 0;
  n_t result_N = 0;

  auto queue = buffer;
  auto queue_len = N;
  n_t pos_wr = 0;
  n_t pos_rd = 0;

  {
    auto u = start_node;

    SETBIT( bits_visited, u );
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];

    auto u_new = result_N++;
    result_old_from_new[u_new] = u;
    auto u_outoffset = result_E;
    auto u_outdegree = outdegree; // we know this ahead of time for start_node; not so for other nodes.

#if BULK_BFS_COPY
    auto v = outnodes + outoffset + e;
    EnqueueAssumingRoom( queue, queue_len, &pos_wr, pos_rd, v, outdegree );
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      SETBIT( bits_visited, v );
      result_outnodes[ result_E++ ] = v;
    }
#else
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      // TODO: could do one bulk copy, no need for iterative copying.
      EnqueueAssumingRoom( queue, queue_len, &pos_wr, pos_rd, &v );
      SETBIT( bits_visited, v );
      result_outnodes[ result_E++ ] = v;
    }
#endif

    result_offsets[u_new] = u_outoffset;
    result_degrees[u_new] = u_outdegree;
  }

  while( pos_wr != pos_rd ) {
    n_t u;
    DequeueAssumingRoom( queue, queue_len, pos_wr, &pos_rd, &u );
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];

    auto u_new = result_N++;
    result_old_from_new[u_new] = u;
    auto u_outoffset = result_E;

    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto visited = GETBIT( bits_visited, v );
      if( !visited ) {
        EnqueueAssumingRoom( queue, queue_len, &pos_wr, pos_rd, &v );
        SETBIT( bits_visited, v );
        result_outnodes[ result_E++ ] = v;
      }
    }
    // account for edges we skipped due to the visited check above.
    auto u_outdegree = result_E - u_outoffset;

    result_offsets[u_new] = u_outoffset;
    result_degrees[u_new] = u_outdegree;
  }

  *result_N_ = result_N;
  *result_E_ = result_E;
}






// TODO: does this DfsComp actually find the largest possible directed trees?
//   for example, what if the first Dfs starting point isn't a tree root?
//   won't we incorrectly split that subtree into a different component?
// indeed; these algorithms work in theory for undirected graphs, but not directed graphs. boo.

void
DfsCompIntoArray(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  u64* bits_visited, // bitarray of length N
  n_t* buffer, // length N
  array_t<tslice_t<n_t>>* result_comps,
  n_t* result_compnodes // length N
  )
{
  ProfFunc();
  AssertCrash( N );

  auto found = result_compnodes;

  Fori( n_t, u, 0, N ) {
    auto visited_u = GETBIT( bits_visited, u );
    if( visited_u ) continue;

    // Use DfsIntoTwoBuffers to write the found nodes directly to our result_compnodes list.
    n_t found_len;
    DfsIntoTwoBuffers( N, E, outoffsets, outdegrees, outnodes, u, bits_visited, buffer, found, &found_len );
    auto comp = AddBack( *result_comps );
    comp->mem = found;
    comp->len = found_len;
    found += found_len;
    AssertCrash( found <= result_compnodes + N );
  }
}
void
DfsCompIntoBuffer(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  u64* bits_visited, // bitarray of length N
  n_t* buffer, // length N
  n_t* result_compoffsets, // length C, which is O(N)
  n_t* result_compdegrees, // length C, which is O(N)
  n_t* result_compnodes, // length N
  n_t* num_components // C
  )
{
  ProfFunc();
  AssertCrash( N );

  n_t found_offset = 0;
  n_t C = 0;

  Fori( n_t, u, 0, N ) {
    auto visited_u = GETBIT( bits_visited, u );
    if( visited_u ) continue;

    // Use DfsIntoTwoBuffers to write the found nodes directly to our result_compnodes list.
    auto found = result_compnodes + found_offset;
    n_t found_len;
    DfsIntoTwoBuffers( N, E, outoffsets, outdegrees, outnodes, u, bits_visited, buffer, found, &found_len );
    result_compoffsets[C] = found_offset;
    result_compdegrees[C] = found_len;
    found_offset += found_len;
    C += 1;
  }

  *num_components = C;
}



#if 0

void
SpanningTreeDfsWithIsolatedNodes(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  u64* bits_visited, // bitarray of length N
  n_t* buffer, // length N
  n_t* result_offsets, // length C, which is O(N)
  n_t* result_degrees, // length C, which is O(N)
  n_t* result_outnodes, // length N
  e_t* result_E_
  )
{
  AssertCrash( N );

  e_t result_E = 0;
  n_t result_N = 0;

  n_t found_offset = 0;
  n_t C = 0;

  Fori( n_t, u, 0, N ) {
    auto visited_u = GETBIT( bits_visited, u );
    if( visited_u ) continue;

    // Use DfsIntoTwoBuffers to write the found nodes directly to our result_compnodes list.
    auto found = result_compnodes + found_offset;
    n_t found_len;
    DfsIntoTwoBuffers( N, E, outoffsets, outdegrees, outnodes, u, bits_visited, buffer, found, &found_len );
    result_compoffsets[C] = found_offset;
    result_compdegrees[C] = found_len;
    found_offset += found_len;
    C += 1;
  }

  *num_components = C;

}

#endif



Inl void
TopologicalSortDfs(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  u64* bits_visited, // bitarray of length N
  n_t* buffer, // length N
  n_t* sort_position // length N, no bounds checking. [i] is the sorted position of node i.
  )
{
  auto sort_position_idx = N;
  Fori( n_t, u, 0, N ) {
    auto visited_u = GETBIT( bits_visited, u );
    if( visited_u ) continue;
    DfsTopSort( N, E, outoffsets, outdegrees, outnodes, u, bits_visited, buffer, &sort_position_idx, sort_position );
  }
}

// For use by TopologicalSort
void
_TsVisitRecursive(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  n_t u,
  u64* bits_visited, // bitarray of length N
  u64* bits_done, // bitarray of length N
  n_t* sorted_nodes, // length N
  n_t* sorted_nodes_idx,
  bool* found_cycle
  )
{
  AssertCrash( N );

  auto done = GETBIT( bits_done, u );
  if( done ) {
    return;
  }

  auto visited = GETBIT( bits_visited, u );
  if( visited ) {
    *found_cycle = 1;
    return;
  }

  SETBIT( bits_visited, u );
  auto outdegree = outdegrees[ u ];
  auto outoffset = outoffsets[ u ];
  Fori( e_t, e, 0, outdegree ) {
    auto v = outnodes[ outoffset + e ];
    _TsVisitRecursive( N, E, outoffsets, outdegrees, outnodes, u, bits_visited, bits_done, sorted_nodes, sorted_nodes_idx, found_cycle );
    if( *found_cycle ) {
      return;
    }
  }

  CLRBIT( bits_visited, u );
  SETBIT( bits_done, u );

  AssertCrash( *sorted_nodes_idx );
  *sorted_nodes_idx -= 1;
  sorted_nodes[ *sorted_nodes_idx ] = u;
}

//
// given a directed graph, this either:
// 1. returns found_cycle=1, meaning this directed graph has a cycle, and can't be sorted.
// OR
// 2. returns found_cycle=0, and fills sorted_nodes with the node values in order.
//
// takes time: Theta( N )
//
void
TopologicalSortRecursive(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  u64* bits_visited, // bitarray of length N
  u64* bits_done, // bitarray of length N
  n_t* sorted_nodes, // length N
  bool* found_cycle
  )
{
  ProfFunc();
  AssertCrash( N );
  auto sorted_nodes_idx = N;
  *found_cycle = 0;

  Fori( n_t, u, 0, N ) {
    auto visited_u = GETBIT( bits_done, u );
    if( visited_u ) continue;

    _TsVisitRecursive( N, E, outoffsets, outdegrees, outnodes, u, bits_visited, bits_done, sorted_nodes, &sorted_nodes_idx, found_cycle );
    if( *found_cycle ) {
      break;
    }
  }
}






// assumes no compression / aliasing, where outnodes has shared slices.
// we modify outnodes assuming one slice, so that would be incorrect.
// takes time: O( outdegree(u) )
void
_TsKahnRemoveEdge(
  n_t N,
  e_t* E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  e_t* indegrees, // length N
  n_t* outnodes, // length E
  n_t u,
  n_t v
  )
{
  auto outdegree = outdegrees[ u ];
  auto outoffset = outoffsets[ u ];

  Fori( e_t, e, 0, outdegree ) {
    auto maybe_v = outnodes[ outoffset + e ];
    if( v == maybe_v ) {
      if( e + 1 == outdegree ) {
        outdegrees[ u ] = 0;
      } else {
        // unordered remove, i.e. move the last element in the slice into where
        // we're doing the removal.
        auto last_node = outnodes[ outoffset + outdegree - 1 ];
        outnodes[ outoffset + e ] = last_node;
        outdegrees[ u ] = outdegree - 1;
      }
    }
  }

  indegrees[ v ] -= 1;
  *E -= 1;
}

//
// given a directed graph, this either:
// 1. returns found_cycle=1, meaning this directed graph has a cycle, and can't be sorted.
// OR
// 2. returns found_cycle=0, and fills sorted_nodes with the node values in order.
//
// WARNING! this modifies the directed graph in place; specifically outnodes, outdegrees, indegrees.
// copy these if necessary before calling this function.
//
// takes time: O( N + E ) in theory; we're slightly worse because _TsKahnRemoveEdge is suboptimal.
//
void
TopologicalSortKahn(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  e_t* indegrees, // length N
  n_t* outnodes, // length E
  n_t* buffer, // length N
  n_t* sorted_nodes, // length N
  bool* found_cycle
  )
{
  ProfFunc();
  auto nodes_with_indegree_zero = buffer;
  n_t nodes_with_indegree_zero_len = 0;

  // fill nodes_with_indegree_zero
  Fori( n_t, u, 0, N ) {
    auto indegree = indegrees[ u ];
    if( !indegree ) {
      nodes_with_indegree_zero[ nodes_with_indegree_zero_len++ ] = u;
    }
  }

  idx_t sorted_nodes_len = 0;
  while( nodes_with_indegree_zero_len ) {
    auto u = nodes_with_indegree_zero[ --nodes_with_indegree_zero_len ];
    sorted_nodes[ sorted_nodes_len++ ] = u;

    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      _TsKahnRemoveEdge( N, &E, outoffsets, outdegrees, indegrees, outnodes, u, v );
      auto v_indegree = indegrees[v];
      if( !v_indegree ) {
        nodes_with_indegree_zero[ nodes_with_indegree_zero_len++ ] = v;
      }
    }
  }

  if( E ) {
    // edges still left, meaning we couldn't unravel everything; there's a cycle.
    *found_cycle = 1;
    return;
  }

  *found_cycle = 0;
}





// algorithmically slower than necessary, probably don't use this.
void
ShortestPathDijkstraArray_SLOW(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  f32* outedge_weights, // length E
  n_t start_node,
  f32* shortest_distance, // length N
  n_t* previous_node_in_path // length N
  )
{
  ProfFunc();
  AssertCrash( N );

  array_t<n_t> set;
  Alloc( set, N / 16 + 1 );

  Fori( n_t, u, 0, N ) {
    shortest_distance[u] = MAX_f32; // TODO: use INF ?
    previous_node_in_path[u] = MAX_n_t;
    *AddBack( set ) = u;
  }
  shortest_distance[start_node] = 0;

  while( set.len ) {
    idx_t idx = 0;
    auto min_distance = shortest_distance[ set.mem[0] ];
    For( i, 1, set.len ) {
      auto distance = shortest_distance[ set.mem[i] ];
      if( distance < min_distance ) {
        idx = i;
      }
    }

    auto u = set.mem[ idx ];
    UnorderedRemAt( set, idx );

    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto v_in_set = ArrayContains( ML( set ), &v );
      if( v_in_set ) {
        auto edge_weight = outedge_weights[ outoffset + e ];
        auto candidate_distance = shortest_distance[u] + edge_weight;
        if( candidate_distance < shortest_distance[v] ) {
          shortest_distance[v] = candidate_distance;
          previous_node_in_path[v] = u;
        }
      }
    }
  }

  Free( set );
}

//
// takes time: O( ( N + E ) log N )
//
void
ShortestPathDijkstraMinHeap(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  f32* outedge_weights, // length E, same indexing as outnodes.
  n_t start_node,
  f32* shortest_distance, // length N
  n_t* previous_node_in_path, // length N
  n_t* buffer, // length N
  n_t* buffer2, // length N
  u64* bits_visited // bitarray of length N
  )
{
  ProfFunc();
  AssertCrash( N );

  auto minheap_ext_from_int = buffer;
  auto minheap_int_from_ext = buffer2;
  idx_t minheap_len = N;
  Fori( n_t, u, 0, N ) {
    shortest_distance[u] = MAX_f32; // TODO: use INF ?
    previous_node_in_path[u] = MAX_n_t;
    minheap_ext_from_int[u] = u;
    minheap_int_from_ext[u] = u;
  }
  shortest_distance[start_node] = 0;
  // note we don't really need the ordering logic of InitMinHeapInPlace
  //   we know shortest_distance is { 0, MAX, MAX, ... } a priori, so we could
  //   just fill the array like that and skip the O(n) InitMinHeapInPlace stuff.
  // This SWAP does the equivalent of this:
  //   InitMinHeapInPlace( minheap, shortest_distance, N );
  {
    auto ext_0 = minheap_ext_from_int[0];
    auto ext_start = minheap_ext_from_int[start_node];
    SWAP( n_t, minheap_int_from_ext[ ext_0 ], minheap_int_from_ext[ ext_start ] );
    SWAP( n_t, minheap_ext_from_int[0], minheap_ext_from_int[start_node] );
  }

  // TODO: we could delay adding things to the minheap, and only add when not present already.
  //   that would probably mean adding another bitarray, but could be faster.

  while( minheap_len ) {
    n_t u;
    MinHeapExtract( minheap_ext_from_int, minheap_int_from_ext, shortest_distance, &minheap_len, &u );
    SETBIT( bits_visited, u );
    auto shortest_distance_u = shortest_distance[u];
    if( shortest_distance_u == MAX_f32 ) {
      // remaining things in the minheap are unreachable, so just terminate early.
      break;
    }

    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto visited = GETBIT( bits_visited, v );
      if( visited ) continue;
      auto edge_weight = outedge_weights[ outoffset + e ];
      auto candidate_distance = shortest_distance_u + edge_weight;
      if( candidate_distance < shortest_distance[v] ) {
        shortest_distance[v] = candidate_distance;
        previous_node_in_path[v] = u;
        MinHeapDecreasedKey( minheap_ext_from_int, minheap_int_from_ext, shortest_distance, minheap_len, v );
      }
    }
  }
}

//
// takes time: O( N E )
//
void
ShortestPathBellmanFord(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  f32* outedge_weights, // length E, same indexing as outnodes.
  n_t start_node,
  f32* shortest_distance, // length N
  n_t* previous_node_in_path // length N
  )
{
  ProfFunc();
  AssertCrash( N );

  Fori( n_t, u, 0, N ) {
    shortest_distance[u] = MAX_f32; // TODO: use INF ?
    previous_node_in_path[u] = MAX_n_t;
  }
  shortest_distance[start_node] = 0;

  Fori( n_t, n, 1, N ) {
    // now loop over all edges, relaxing the distance values.
    // as soon as we don't find any relaxations, we can stop.
    bool relaxed = 0;

    Fori( n_t, u, 0, N ) {
      auto outdegree = outdegrees[ u ];
      auto outoffset = outoffsets[ u ];
      auto shortest_distance_u = shortest_distance[u];
      if( shortest_distance_u == MAX_f32 ) continue;
      Fori( e_t, e, 0, outdegree ) {
        auto v = outnodes[ outoffset + e ];
        auto edge_weight = outedge_weights[ outoffset + e ];
        auto candidate_distance = shortest_distance_u + edge_weight;
        if( candidate_distance < shortest_distance[v] ) {
          shortest_distance[v] = candidate_distance;
          previous_node_in_path[v] = u;
          relaxed = 1;
        }
      }
    }

    if( !relaxed ) break;
  }
}

//
// takes time: O( N E )
//
void
ShortestPathBellmanFordFaster(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  f32* outedge_weights, // length E, same indexing as outnodes.
  n_t start_node,
  n_t* buffer, // length N
  f32* shortest_distance, // length N
  n_t* previous_node_in_path // length N
  )
{
  ProfFunc();
  AssertCrash( N );

  Fori( n_t, u, 0, N ) {
    shortest_distance[u] = MAX_f32; // TODO: use INF ?
    previous_node_in_path[u] = MAX_n_t;
  }
  shortest_distance[start_node] = 0;

  Fori( n_t, u, 0, N ) {
    buffer[u] = u;
  }
  auto buffer_len = N;

  Fori( n_t, n, 1, N ) {
    // now loop over all edges, relaxing the distance values.
    // as soon as we don't find any relaxations, we can stop.
    bool relaxed = 0;

    Fori( n_t, i, 0, buffer_len ) {
      auto u = buffer[i];
      bool relaxed_u = 0;
      auto outdegree = outdegrees[ u ];
      auto outoffset = outoffsets[ u ];
      auto shortest_distance_u = shortest_distance[u];
      if( shortest_distance_u == MAX_f32 ) continue;
      Fori( e_t, e, 0, outdegree ) {
        auto v = outnodes[ outoffset + e ];
        auto edge_weight = outedge_weights[ outoffset + e ];
        auto candidate_distance = shortest_distance_u + edge_weight;
        if( candidate_distance < shortest_distance[v] ) {
          shortest_distance[v] = candidate_distance;
          previous_node_in_path[v] = u;
          relaxed_u = 1;
        }
      }
      if( !relaxed_u ) {
        if( i + 1 < buffer_len ) {
          buffer[i] = buffer[ buffer_len - 1 ];
          i -= 1;
        }
        buffer_len -= 1;
      }
      relaxed |= relaxed_u;
    }

    if( !relaxed ) break;
  }
}

//
// takes time: O( N E )
//
void
ShortestPathFaster(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  f32* outedge_weights, // length E, same indexing as outnodes.
  n_t start_node,
  n_t* buffer, // length N
  u64* bits_inqueue, // bitarray of length N
  f32* shortest_distance, // length N
  n_t* previous_node_in_path // length N
  )
{
  ProfFunc();
  AssertCrash( N );

  Fori( n_t, u, 0, N ) {
    shortest_distance[u] = MAX_f32; // TODO: use INF ?
    previous_node_in_path[u] = MAX_n_t;
  }
  shortest_distance[start_node] = 0;

  auto queue = buffer;
  auto queue_len = N;
  n_t pos_wr = 0;
  n_t pos_rd = 0;

  EnqueueAssumingRoom( queue, queue_len, &pos_wr, pos_rd, &start_node );
  SETBIT( bits_inqueue, start_node );

  while( pos_wr != pos_rd ) {
    n_t u;
    DequeueAssumingRoom( queue, queue_len, pos_wr, &pos_rd, &u );
    CLRBIT( bits_inqueue, u );
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto edge_weight = outedge_weights[ outoffset + e ];
      auto candidate_distance = shortest_distance[u] + edge_weight;
      if( candidate_distance < shortest_distance[v] ) {
        shortest_distance[v] = candidate_distance;
        previous_node_in_path[v] = u;
        auto v_inqueue = GETBIT( bits_inqueue, v );
        if( !v_inqueue ) {
          EnqueueAssumingRoom( queue, queue_len, &pos_wr, pos_rd, &v );
          SETBIT( bits_inqueue, v );
        }
      }
    }
  }
}

//
// given a directed graph, this finds the shortest paths between all pairs of nodes.
//
// takes time: O( N^3 )
//
void
ShortestPathsFloydWarshall(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  f32* outedge_weights, // length E, same indexing as outnodes.
  f32* shortest_distances // length N^2, [u + v * N] = shortest path from u to v.
  )
{
  ProfFunc();
  AssertCrash( N );

  Fori( n_t, u, 0, N ) {
    Fori( n_t, v, 0, N ) {
      auto uv = u + v * N;
      shortest_distances[ uv ] = MAX_f32;
    }
  }
  Fori( n_t, u, 0, N ) {
    auto uu = u + u * N;
    shortest_distances[ uu ] = 0;

    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto edge_weight = outedge_weights[ outoffset + e ];
      auto uv = u + v * N;
      shortest_distances[ uv ] = edge_weight;
    }
  }

  // 'k' is the inductive dynamic programming variable, indicating which nodes we're allowed to use
  // in our path so far. we're allowed to use nodes { 0 <= k }.
  // distance( u, v, k ) = min(
  //   distance( u, v, k - 1 ), // existing path, not using node k.
  //   distance( u, k, k - 1 ) + distance( k, v, k - 1 ) // using node k in between u and v.
  // )
  Fori( n_t, k, 0, N ) {
    Fori( n_t, u, 0, N ) {
      Fori( n_t, v, 0, N ) {
        auto uk = u + k * N;
        auto kv = k + v * N;
        auto uv = u + v * N;
        auto candidate_distance = shortest_distances[ uk ] + shortest_distances[ kv ];
        if( candidate_distance < shortest_distances[ uv ] ) {
          shortest_distances[ uv ] = candidate_distance;
        }
      }
    }
  }
}
void
ShortestPathsFloydWarshall(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  f32* outedge_weights, // length E, same indexing as outnodes.
  f32* shortest_distances, // length N^2, [u + v * N] = shortest path from u to v.
  n_t* next_node_in_paths // length N^2, [u + v * N] = next node in path from u to v.
  )
{
  ProfFunc();
  AssertCrash( N );
  AssertCrash( N < MAX_n_t );

  Fori( n_t, u, 0, N ) {
    Fori( n_t, v, 0, N ) {
      auto uv = u + v * N;
      shortest_distances[ uv ] = MAX_f32;
      next_node_in_paths[ uv ] = MAX_n_t;
    }
  }
  Fori( n_t, u, 0, N ) {
    auto uu = u + u * N;
    shortest_distances[ uu ] = 0;
    next_node_in_paths[ uu ] = u;

    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto edge_weight = outedge_weights[ outoffset + e ];
      auto uv = u + v * N;
      shortest_distances[ uv ] = edge_weight;
      next_node_in_paths[ uv ] = v;
    }
  }

  Fori( n_t, k, 0, N ) {
    Fori( n_t, u, 0, N ) {
      Fori( n_t, v, 0, N ) {
        auto uk = u + k * N;
        auto kv = k + v * N;
        auto uv = u + v * N;
        auto candidate_distance = shortest_distances[ uk ] + shortest_distances[ kv ];
        if( candidate_distance < shortest_distances[ uv ] ) {
          shortest_distances[ uv ] = candidate_distance;
          next_node_in_paths[ uv ] = next_node_in_paths[ uk ];
        }
      }
    }
  }
}
//
// example code:
//
// ...
// next_node_in_paths = Alloc( N * N );
// ShortestPathsFloydWarshall( N, E, outoffsets, outdegrees, outnodes, outedge_weights, shortest_distances, next_node_in_paths );
// ...
// RecoverShortestPathLengthFloydWarshall( N, next_node_in_paths, &path_len, u, v );
// path = Alloc( path_len );
// RecoverShortestPathFloydWarshall( N, next_node_in_paths, path, path_len, u, v );
// ...
//
void
RecoverShortestPathLengthFloydWarshall(
  n_t N,
  n_t* next_node_in_paths, // length N^2, [u + v * N] = next node in path from u to v.
  n_t* path_len, // # of nodes in shortest path from u to v, including both u and v; OR 0 if no path.
  n_t u,
  n_t v
  )
{
  n_t len = 0;
  auto uv = u + v * N;
  if( next_node_in_paths[ uv ] == MAX_n_t ) {
    *path_len = 0;
    return;
  }
  do {
    len += 1;
    u = next_node_in_paths[ uv ];
    uv = u + v * N;
  } while( u != v );
}
void
RecoverShortestPathFloydWarshall(
  n_t N,
  n_t* next_node_in_paths, // length N^2, [u + v * N] = next node in path from u to v.
  n_t* path,
  n_t path_len, // result from previous RecoverShortestPathLengthFloydWarshall call.
  n_t u,
  n_t v
  )
{
  n_t pos_wr = 0;
  Fori( n_t, i, 0, path_len ) {
    path[ pos_wr++ ] = u;
    auto uv = u + v * N;
    u = next_node_in_paths[ uv ];
  }
}

//
// These assume your node values are in [0, N)
//
// OPEN QUESTION:
// Maybe we should provide a mapping table mechanism?
// Or should we just be more clear these are node indices?
//

void
Outdegrees(
  n_t N,
  e_t E,
  n_t* edgelist, // length 2E, containing edge pairs: (from, to)
  e_t* outdegrees // length N
  )
{
  Fori( e_t, i, 0, E ) {
    auto u = edgelist[ 2 * i + 0 ];
    // auto v = edgelist[ 2 * i + 1 ];
    AssertCrash( u < N );
    outdegrees[u] += 1;
  }
}
void
Indegrees(
  n_t N,
  e_t E,
  n_t* edgelist, // length 2E, containing edge pairs: (from, to)
  e_t* indegrees // length N
  )
{
  Fori( e_t, i, 0, E ) {
    // auto u = edgelist[ 2 * i + 0 ];
    auto v = edgelist[ 2 * i + 1 ];
    AssertCrash( v < N );
    indegrees[v] += 1;
  }
}
void
Degrees(
  n_t N,
  e_t E,
  n_t* edgelist, // length 2E, containing edge pairs: (from, to)
  e_t* degrees // length N
  )
{
  Fori( e_t, i, 0, E ) {
    auto u = edgelist[ 2 * i + 0 ];
    auto v = edgelist[ 2 * i + 1 ];
    AssertCrash( u < N );
    degrees[u] += 1;
    AssertCrash( v < N );
    degrees[v] += 1;
  }
}


// returns ( N choose 2 )
Inl e_t
NumPossibleUndirectedEdges( n_t N )
{
  e_t n = (e_t)N;
  return ( n * ( n - 1 ) ) / 2;
}

// returns ( N permute 2 )
Inl e_t
NumPossibleDirectedEdges( n_t N )
{
  e_t n = (e_t)N;
  return ( n * ( n - 1 ) );
}




// say index_permutation is [ 1, 2, 0 ]
// this writes inverse_index_permutation to [ 2, 0, 1 ]
// the idea being, given some permutation, this generates the inverse permutation.
// this can be useful for things like sorted orders, where we store the order as an index permutation.
Templ Inl void
InverseIndexPermutation(
  T* index_permutation,
  T* inverse_index_permutation,
  idx_t count
  )
{
  For( i, 0, count ) {
    auto index = index_permutation[i];
    inverse_index_permutation[ index ] = i;
  }
}


#if 0
Inl void
HamiltonianPathInDAGWithStartPoint(
  n_t* topological_sort_permutation,
  n_t* topological_sort_permutation_inverse,
  n_t start_node
  )
{
  Fori( n_t, u, 0, N ) {
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    bool found = 0;
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];

      if( topological_sort_permutation[
    }
  }
}
#endif



#if 1

// takes time: O( E * num_iterations )
void
LayoutSimpleSpring(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  vec2<f32>* positions, // length N
  vec2<f32>* velocities, // length N
  vec2<f32>* forces, // length N
  rectf32_t bounds,
  f32 timestep,
  idx_t num_iterations
  )
{
  ProfFunc();
  // TODO: how should we pick initial positions? rectangular grid? random dart throw? blue noise?

  // TODO: parameterize the physical params, so you can choose things. e.g. spring_k, spring_center_dist.

  For( iter, 0, num_iterations ) {
    //
    // accumulate forces
    //
    Memzero( forces, N * sizeof( forces[0] ) );
    Fori( n_t, u, 0, N ) {
      auto u_pos = positions + u;
      auto u_vel = velocities + u;
      auto u_force = forces + u;

      //
      // spring forces from edges
      //
      auto u_outdegree = outdegrees[ u ];
      auto u_outoffset = outoffsets[ u ];
      Fori( e_t, e, 0, u_outdegree ) {
        auto v = outnodes[ u_outoffset + e ];
        auto v_pos = positions + v;
        auto v_vel = velocities + v;
        auto v_force = forces + v;

        // TODO: bigger mass in proportion to total degree?
        //   will have to fix mass+force sharing below.
        f32 mass = 1.0f;
        constant f32 spring_k = 1000.0f;
        constant f32 spring_center_dist = 10.5f;
        f32 friction_k = 2.2f * Sqrt32( mass * spring_k ); // 2 * Sqrt( m * k ) is critical damping, but we want a little overdamping.

        auto v_minus_u = *v_pos - *u_pos;
        auto uv_dist = Length( v_minus_u );
        auto spring_disp_scalar = uv_dist - spring_center_dist;
        auto spring_disp = v_minus_u * ( spring_disp_scalar / uv_dist );
        // TODO: spring damping/friction forces.
        auto u_force_spring = spring_k * spring_disp;
        auto v_force_spring = -u_force_spring;
        *u_force += u_force_spring;
        *v_force += v_force_spring;
      }

      // TODO: velocity damping forces
//      auto force_fric = -friction_k * *vel;
//      *force += force_fric;

      // TODO: neighbor repulsion forces of some kind. E+M ?
      //   we can't really set up all-pairs springs, that would just pull stuff together.
      //   so some repulsive force like E+M probably makes sense, up to some distance cutoff.
      //   probably we'll have to do neighbor binning to make this fast.
      Fori( n_t, v, 0, N ) {
        if( u == v ) continue;
        auto v_pos = positions + v;
        auto v_force = forces + v;
        auto v_minus_u = *v_pos - *u_pos;
        auto uv_dist = Length( v_minus_u );
        constant f32 repulsion_dist = 80.0f;
        constant f32 repulsion_strength = 500.0f;
        // force_scalar = strength * ( 1 - x ), over { 0 <= x <= 1 }
        // we could do something smoother like smoothstep, cosine, etc. if necessary.
        // note this competes with the edge spring force, so tuning constants is key.
        if( uv_dist < repulsion_dist ) {
          auto repulsion_scalar = repulsion_strength * ( 1.0f - uv_dist / repulsion_dist );
          auto u_force_repul = v_minus_u * ( -repulsion_scalar / uv_dist );
          auto v_force_repul = -u_force_repul;
          *u_force += u_force_repul;
          *v_force += v_force_repul;
        }

      }

    }
    //
    // integrate forces
    //
    Fori( n_t, u, 0, N ) {
      auto pos = positions + u;
      auto vel = velocities + u;
      auto force = forces[u];
      // TODO: bigger mass in proportion to total degree?
      //   will have to fix mass+force sharing below.
      f32 mass = 1.0f;
      auto accel = force / mass;
      auto delta_vel = timestep * accel;
      *vel += delta_vel;
      auto delta_pos = timestep * *vel;
      *pos += delta_pos;
    }
  }

  //
  // normalize positions into the given bounds
  //

  // TODO: find actual bounds of positions
  // TODO: map actual aspect ratio to desired aspect ratio, somehow.
  //   uniform stretch? rotate and then stretch? black bars? rotate and black bars?

  //
  // how do you take a given aspect ratio r, and number of grid elements n, and pick grid dimensions?
  // r = x / y -> ry = x
  // n = x * y -> n = ry^2
  // y = sqrt(n/r)
  // x = r y
  //
}

#endif









Inl void
TestGraph()
{
  rng_lcg_t lcg;
  Init( lcg, 0x123456789 );

  // tests on a simple graph:
  {
    // here's the edge list we're using below:
    // 0, 1
    // 1, 0
    // 1, 2
    // 2, 0
    // 3, 0
    // 3, 1
    // 3, 2
    //
    // in visual form, the graph looks like:
    //
    //    0 <-1-> 1
    //    ^^     /^
    //    | \   2 |
    //    |  \ /  |
    //    3   x   2
    //    |  / \  |
    //    | /   4 |
    //    |v     \|
    //    2 <-5-- 3
    //
    constant n_t N = 4;
    constant e_t E = 7;
    e_t outoffsets[N] = { 0, 1, 3, 4 };
    e_t outdegrees[N] = { 1, 2, 1, 3 };
    n_t outnodes[E] = { 1, 0, 2, 0, 0, 1, 2 };
    f32 outedge_weights[E] = { 1, 1, 2, 3, 4, 2, 5 };
    u64 bits_visited[ Bitbuffer64Len( N ) ];
    n_t buffer[N];
    n_t buffer2[N];
    tslice_t<n_t> found_nodes;
    n_t found_len;

    CompileAssert( sizeof( bits_visited ) == Bitbuffer64Len( N ) * sizeof( bits_visited[0] ) );

    // test some DFS functions
    {
      n_t expected0[] = { 2, 1, 0 };
      Arrayzero( bits_visited );
      DfsIntoBufferBack( N, E, outoffsets, outdegrees, outnodes, 0, bits_visited, buffer, &found_nodes );
      AssertCrash( EqualContents( found_nodes, SliceFromCArray( n_t, expected0 ) ) );
      MemReverse( AL( expected0 ) );
      Arrayzero( bits_visited );
      DfsIntoBuffer( N, E, outoffsets, outdegrees, outnodes, 0, bits_visited, buffer, &found_len );
      AssertCrash( EqualContents( { buffer, found_len }, SliceFromCArray( n_t, expected0 ) ) );

      n_t expected1[] = { 0, 2, 1 };
      Arrayzero( bits_visited );
      DfsIntoBufferBack( N, E, outoffsets, outdegrees, outnodes, 1, bits_visited, buffer, &found_nodes );
      AssertCrash( EqualContents( found_nodes, SliceFromCArray( n_t, expected1 ) ) );
      MemReverse( AL( expected1 ) );
      Arrayzero( bits_visited );
      DfsIntoBuffer( N, E, outoffsets, outdegrees, outnodes, 1, bits_visited, buffer, &found_len );
      AssertCrash( EqualContents( { buffer, found_len }, SliceFromCArray( n_t, expected1 ) ) );

      n_t expected2[] = { 1, 0, 2 };
      Arrayzero( bits_visited );
      DfsIntoBufferBack( N, E, outoffsets, outdegrees, outnodes, 2, bits_visited, buffer, &found_nodes );
      AssertCrash( EqualContents( found_nodes, SliceFromCArray( n_t, expected2 ) ) );
      MemReverse( AL( expected2 ) );
      Arrayzero( bits_visited );
      DfsIntoBuffer( N, E, outoffsets, outdegrees, outnodes, 2, bits_visited, buffer, &found_len );
      AssertCrash( EqualContents( { buffer, found_len }, SliceFromCArray( n_t, expected2 ) ) );

      n_t expected3[] = { 0, 1, 2, 3 };
      Arrayzero( bits_visited );
      DfsIntoBufferBack( N, E, outoffsets, outdegrees, outnodes, 3, bits_visited, buffer, &found_nodes );
      AssertCrash( EqualContents( found_nodes, SliceFromCArray( n_t, expected3 ) ) );
      MemReverse( AL( expected3 ) );
      Arrayzero( bits_visited );
      DfsIntoBuffer( N, E, outoffsets, outdegrees, outnodes, 3, bits_visited, buffer, &found_len );
      AssertCrash( EqualContents( { buffer, found_len }, SliceFromCArray( n_t, expected3 ) ) );
    }

  //    ShortestPathUnweightedWithBFS(
  //  n_t N,
  //  e_t E,
  //  e_t* outoffsets, // length N
  //  e_t* outdegrees, // length N
  //  n_t* outnodes, // length E
  //  n_t start_node,
  //  u64* bits_visited, // bitarray of length N
  //  n_t* buffer, // length N
  //  n_t* found, // length N, no bounds checking.
  //  n_t* found_len_,
  //  n_t* shortest_distance, // length N
  //  n_t* previous_node_in_path // length N
  //  )


    // test some shortest path functions
    Fori( n_t, start_node, 0, N ) {

      constant n_t M = 5; // number of functions we're verifying here.
      f32 shortest_distance[ M * N ];
      n_t previous_node_in_path[ M * N ];
      n_t f = 0;

      {
        auto f_offset = N * f++;
        ShortestPathDijkstraArray_SLOW( N, E, outoffsets, outdegrees, outnodes, outedge_weights, start_node, shortest_distance + f_offset, previous_node_in_path + f_offset );
      }

      {
        auto f_offset = N * f++;
        Arrayzero( bits_visited );
        ShortestPathDijkstraMinHeap( N, E, outoffsets, outdegrees, outnodes, outedge_weights, start_node, shortest_distance + f_offset, previous_node_in_path + f_offset, buffer, buffer2, bits_visited );
      }

      {
        auto f_offset = N * f++;
        ShortestPathBellmanFord( N, E, outoffsets, outdegrees, outnodes, outedge_weights, start_node, shortest_distance + f_offset, previous_node_in_path + f_offset );
      }

      {
        auto f_offset = N * f++;
        ShortestPathBellmanFordFaster( N, E, outoffsets, outdegrees, outnodes, outedge_weights, start_node, buffer, shortest_distance + f_offset, previous_node_in_path + f_offset );
      }

      {
        auto f_offset = N * f++;
        Arrayzero( bits_visited );
        ShortestPathFaster( N, E, outoffsets, outdegrees, outnodes, outedge_weights, start_node, buffer, bits_visited, shortest_distance + f_offset, previous_node_in_path + f_offset );
      }

      // make sure all functions generated the same outputs.
      // we compare against the first one, for convenience here.
      tslice_t<f32> distance0 = { shortest_distance, N };
      tslice_t<n_t> prev0 = { previous_node_in_path, N };
      Fori( n_t, m, 1, M ) {
        auto f_offset = N * m;
        tslice_t<f32> distance = { shortest_distance + f_offset, N };
        tslice_t<n_t> prev = { previous_node_in_path + f_offset, N };
        AssertCrash( EqualContents( distance0, distance ) );
        AssertCrash( EqualContents( prev0, prev ) );
      }
    }
  }

  // tests on random graphs:
  {
    // 10,000 nodes causes us to hit ~20M edges, which is high enough to start hitting array_t limits.
    // so until we add sparse edge support, or just allocate huge contiguous sections here,
    // our limit is something like this.
    constant n_t node_sizes[] = { 0, 1, 2, 3, 4, 10, 100, 1000 };
    constant f32 edge_probabilities[] = { 0.001f, 0.01f, 0.1f, 0.2f, 0.4f, 0.5f, 0.7f, 0.9f, 1.0f };
    ForEach( N, node_sizes ) {
      ForEach( P, edge_probabilities ) {
        // we're using the G(n,p) model, where each possible edge is present with probability p.
        tstring_t<e_t> outoffsets;
        tstring_t<e_t> outdegrees;
        array_t<n_t> outnodes;
        array_t<f32> outedge_weights;
        Alloc( outoffsets, N );
        Alloc( outdegrees, N );
        auto estimate_E = Cast( idx_t, 1 + NumPossibleDirectedEdges( N ) * P * 1.2f );
        Alloc( outnodes, estimate_E );
        Alloc( outedge_weights, estimate_E );

        Fori( n_t, u, 0, N ) {
          e_t u_outoffset = outnodes.len;
          e_t u_outdegree = 0;

          // now generate edges coming out of u
          Fori( n_t, v, 0, N ) {
            if( u == v ) continue;
            auto roll = Zeta32( lcg );
            if( roll > P ) continue;
            *AddBack( outnodes ) = v;
            auto edge_weight = Zeta32( lcg );
            *AddBack( outedge_weights ) = edge_weight;
          }

          outoffsets.mem[u] = u_outoffset;
          outdegrees.mem[u] = u_outdegree;
        }

        e_t E = outnodes.len;

        if( E ) {
          Fori( n_t, start_node, 0, N ) {
            tstring_t<u64> bits_visited;
            Alloc( bits_visited, Bitbuffer64Len( N ) );
            tstring_t<n_t> buffer;
            Alloc( buffer, N );
            tstring_t<n_t> buffer2;
            Alloc( buffer2, N );

            // number of functions we're verifying here.
            // we're skipping ShortestPathDijkstraArray_SLOW since it's really slow, N^2 basically.
            constant n_t M = 4;
            tstring_t<f32> shortest_distance;
            Alloc( shortest_distance, M * N );
            tstring_t<n_t> previous_node_in_path;
            Alloc( previous_node_in_path, M * N );
            n_t f = 0;

//            {
//              auto f_offset = N * f++;
//              ShortestPathDijkstraArray_SLOW( N, E, outoffsets.mem, outdegrees.mem, outnodes.mem, outedge_weights.mem, start_node, shortest_distance.mem + f_offset, previous_node_in_path.mem + f_offset );
//            }

            {
              auto f_offset = N * f++;
              ZeroContents( bits_visited );
              ShortestPathDijkstraMinHeap( N, E, outoffsets.mem, outdegrees.mem, outnodes.mem, outedge_weights.mem, start_node, shortest_distance.mem + f_offset, previous_node_in_path.mem + f_offset, buffer.mem, buffer2.mem, bits_visited.mem );
            }

            {
              auto f_offset = N * f++;
              ShortestPathBellmanFord( N, E, outoffsets.mem, outdegrees.mem, outnodes.mem, outedge_weights.mem, start_node, shortest_distance.mem + f_offset, previous_node_in_path.mem + f_offset );
            }

            {
              auto f_offset = N * f++;
              ShortestPathBellmanFordFaster( N, E, outoffsets.mem, outdegrees.mem, outnodes.mem, outedge_weights.mem, start_node, buffer.mem, shortest_distance.mem + f_offset, previous_node_in_path.mem + f_offset );
            }

            {
              auto f_offset = N * f++;
              ZeroContents( bits_visited );
              ShortestPathFaster( N, E, outoffsets.mem, outdegrees.mem, outnodes.mem, outedge_weights.mem, start_node, buffer.mem, bits_visited.mem, shortest_distance.mem + f_offset, previous_node_in_path.mem + f_offset );
            }

            // make sure all functions generated the same outputs.
            // we compare against the first one, for convenience here.
            tslice_t<f32> distance0 = { shortest_distance.mem, N };
            tslice_t<n_t> prev0 = { previous_node_in_path.mem, N };
            Fori( n_t, m, 1, M ) {
              auto f_offset = N * m;
              tslice_t<f32> distance = { shortest_distance.mem + f_offset, N };
              tslice_t<n_t> prev = { previous_node_in_path.mem + f_offset, N };
              AssertCrash( EqualContents( distance0, distance ) );
              AssertCrash( EqualContents( prev0, prev ) );
            }

            Free( bits_visited );
            Free( buffer );
            Free( buffer2 );
            Free( shortest_distance );
            Free( previous_node_in_path );
          }
        }

        Free( outoffsets );
        Free( outdegrees );
        Free( outnodes );
        Free( outedge_weights );
      }
    }
  }
}

//ShortestPathsFloydWarshall(
//  n_t N,
//  e_t E,
//  e_t* outoffsets, // length N
//  e_t* outdegrees, // length N
//  n_t* outnodes, // length E
//  f32* outedge_weights, // length E, same indexing as outnodes.
//  f32* shortest_distances // length N^2, [u + v * N] = shortest path from u to v.
//  )
//
//ShortestPathsFloydWarshall(
//  n_t N,
//  e_t E,
//  e_t* outoffsets, // length N
//  e_t* outdegrees, // length N
//  n_t* outnodes, // length E
//  f32* outedge_weights, // length E, same indexing as outnodes.
//  f32* shortest_distances, // length N^2, [u + v * N] = shortest path from u to v.
//  n_t* next_node_in_paths // length N^2, [u + v * N] = next node in path from u to v.
//  )












#if 0

void
TsVisitWithStack(
  n_t N,
  e_t E,
  e_t* outoffsets, // length N
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  n_t u,
  u64* bits_visited, // bitarray of length N
  u64* bits_done, // bitarray of length N
  n_t* buffer, // length N
  n_t* buffer_idx,
  bool* found_cycle,
  array_t<>* stack
  )
{
  AssertCrash( N );

  while( stack->len ) {
    auto t = stack->mem[ stack->len - 1 ];
    RemBack( *stack );

    auto u = t->u;

    auto done = GETBIT( bits_done, u );
    if( done ) {
      continue;
    }

    auto visited = GETBIT( bits_visited, u );
    if( visited ) {
      // OPEN QUESTION:
      // option to continue algorithm as if this wasn't the case?
      // would that break the algorithm? Probably.
      // maybe there's some way of silently dropping this cyclical edge.
      // but for now, just terminate.
      *found_cycle = 1;
      return;
    }

    SETBIT( bits_visited, u );
    auto outdegree = outdegrees[ u ];
    auto outoffset = outoffsets[ u ];
    Fori( e_t, e, 0, outdegree ) {
      auto v = outnodes[ outoffset + e ];
      auto t2 = AddBack( stack );
      t2->u = v;
      t2->last_u = u;

      TsVisitRecursive(...);
    }

  CLRBIT( bits_visited, u );
  SETBIT( bits_done, u );

  AssertCrash( *buffer_idx );
  *buffer_idx -= 1;
  buffer[ *buffer_idx ] = u;




  idx = 0
  while( idx >= 0 ) {
    a = stack[idx]
    b = A(a)
    if( recurse ) { // some termination condition
      *AddBack( stack ) = b;
      idx += 1;
      continue;
    }
    // not recursing, so do the 'pop'
    c = B(b)
    idx -= 1;
  }
  Clear( stack )


}



simple tail-recursion to iteration:
Recurse(a)
{
  b = A(a);
  Recurse(b)
}
Iterative(stack)
{
  while( stack.len ) {
    a = RemBack(stack)
    b = A(a)
    *AddBack( stack ) = b;
  }
}

now with operations post-recursion:
Recurse(a)
{
  b = A(a);
  if( recurse ) { // some termination condition
    Recurse(b)
  }
  c = B(b)
}
Iterative(stack)
{
  ???
}
say we've got 3 iterations of this post-recursive version.
we want the sequence of ops to be:
A(0) A(1) A(2) B(2) B(1) B(0)
we start with 0 pushed.
we pop 0, call A(0), and push 1.
but, we need the 0 again later!
so we could either: 1) not actually pop, just move an index, OR 2) keep a second stack/queue.
i like the first option more in the abstract i think.
so instead of pop I, we'll just leave the index entirely alone.
so that would look like:
Iterative(stack) // starts with one thing inside.
{
  idx = 0
  while( idx >= 0 ) {
    a = stack[idx]
    b = A(a)
    *AddBack( stack ) = b;
    if( recurse ) { // some termination condition
      idx += 1;
      continue;
    }
    // not recursing, so do the 'pop'
    RemBack( stack )
    c = B(b)
    idx -= 1;
  }
  Clear( stack )
}
note we could avoid the push/pop when not recursing, so:
Iterative(stack) // starts with one thing inside.
{
  idx = 0
  while( idx >= 0 ) {
    a = stack[idx]
    b = A(a)
    if( recurse ) { // some termination condition
      *AddBack( stack ) = b;
      idx += 1;
      continue;
    }
    // not recursing, so do the 'pop'
    c = B(b)
    idx -= 1;
  }
  Clear( stack )
}
note the stack has size O(# of recursive steps) still, same as tail recursion. which is nice.
we just have to remember all previous steps, which makes sense. as we have post-recur work to do!
now does this generalize to looping recursion?
i.e. instead of the termination condition, we had a conditional loop?
Recurse(a, i)
{
  b = A(a, i);
  For( j in recurse ) { // some termination condition
    Recurse(b, j)
  }
  c = B(b, i)
}
say we've got 3 iterations, where A has 3 children, and each child has one child.
the sequence of ops should look like:
A(0, 0)
A(1, 0)
A(2, 0)
B(2, 0)
B(1, 0)
A(1, 1)
A(2, 0)
B(2, 0)
B(1, 1)
A(1, 2)
A(2, 0)
B(2, 0)
B(1, 2)
B(0, 0)
complicated to understand. here's just the As:
A(0, 0)
A(1, 0)
A(2, 0)
A(1, 1)
A(2, 0)
A(1, 2)
A(2, 0)
and just the Bs:
B(2, 0)
B(1, 0)
B(2, 0)
B(1, 1)
B(2, 0)
B(1, 2)
B(0, 0)
you can see a little easier how we do 3 ops for (1, *) alone, since it's recursion level one.
and we have 3 ops for (2, 0), since we have 3 nodes at recursion level 2.
this is sort of past the threshold for logical thinking, so let's just try a simple version,
that queues up all children into the stack before continuing:
Iterative(stack) // starts with one thing inside.
{
  idx = 0
  while( idx >= 0 ) {
    a = stack[idx]
    b = A(a)
    if( recurse.len ) {
      For( j in recurse ) { // some termination condition
        *AddBack( stack ) = b;
      }
      idx += recurse.len;
      continue;
    }
    // not recursing, so do the 'pop'
    c = B(b)
    idx -= 1;
  }
  Clear( stack )
}
presumably we have to queue all children, since we have no other way to store them.
one interesting question is whether this forward order is correct, or do we need the opposite order?
so it turns out ordering isn't the hard problem.
what is hard is: how do we correctly do the idx traversals to cover all children pushed, and still
pop back to where we need to go?
i think we need a bit to tell us which kind of idx traversal to do: go to next child, or the usual -= 1?
or is that insufficient?
or do we really need the secondary stack now? and we'd store the children count on the first one, actual
children on the second one?
even the simple forward ordering is tricky to get right...
so from first principles, the recursive calls other than the first one are technically B calls themselves.
so could we do the above transformation for just the first call, and see what happens?
the problem here is that the number of calls is supposed to be arbitrarily variable, so this might be hard.
let's try the simpler version first, just going down the two-stack road and see if that's got better
chances of being generalized.
as a reminder, here's the recursive version:
Recurse(a)
{
  A(a);
  b = a + 1
  if( recurse ) { // some termination condition
    Recurse(b)
  }
  B(a)
}
and here's our incorrect single-stack version, which we need to augment somehow:
Iterative(stack) // starts with one thing inside.
{
  while( stack.len ) {
    a = RemBack( stack )
    A(a)
    b = a + 1
    if( recurse ) { // some termination condition
      *AddBack( stack ) = b;
      continue;
    }
    B(a)
  }
}
actually, let's just go full instruction-set on this problem, and use simple jumps/one stack.
for that, we just need to write a function start/end, and store enough on the one stack to support that.
we need to store what we use after any recursive calls, and have appropriate pro/epilogues for each.
since we've only got one kind of call, that's relatively nice.
Iterative(stack) // starts with one thing inside.
{
  while( stack.len ) {
    a = RemBack( stack )
    A(a)
    b = a + 1
    if( recurse ) { // some termination condition
      *AddBack( stack ) = b;
      continue;
    }
    c = B(b)
  }
}

#endif



#if 0
void
Parse(
  n_t N,
  e_t E,
  e_t* outdegrees, // length N
  n_t* outnodes, // length E
  slice_t filename
  )
{
  ProfFunc();
  Typezero( g );

  file_t file = FileOpen( ML( filename ), fileopen_t::only_existing, fileop_t::R, fileop_t::R );
  if( !file.loaded ) {
    return;
  }
  auto contents = FileAlloc( file );

  array_t<

  u32 nlines = CountNewlines( ML( csv ) ) + 1;
  array32_t<slice32_t> lines;
  Alloc( lines, nlines );
  array_t<slice_t> entries;
  Alloc( entries, 32 );
  SplitIntoLines( &lines, ML( csv ) );
  FORLEN32( line, y, lines )
    auto nentries = CountCommas( ML( *line ) ) + 1;
    if( !( nentries == 0 || nentries == 2 ) ) {
      return;
    }
    entries.len = 0;
    Reserve( entries, nentries );
    SplitByCommas( &entries, ML( *line ) );
    FORLEN32( entry, x, entries )
      auto rect = AddBack( rectlist );
      rect->p0 = abspos;
      rect->p1 = abspos + _vec2<u32>( 1, 1 );
      AssertCrash( entry->len <= MAX_u32 );
      auto copied = AddPlistSlice32( grid->cellmem, u8, 1, Cast( u32, entry->len ) );
      Memmove( copied.mem, ML( *entry ) );
      InsertOrSetCellContents( grid, &rectlist, copied );
    }
  }
  Free( rectlist );
  Free( entries );
  Free( lines );

  pagearray_t<slice32_t> lines;
  Init( lines, 65000 );
  SplitIntoLines( &lines, ML( contents ) );
  AssertCrash( lines.totallen <= MAX_u32 );
  auto nlines = Cast( u32, lines.totallen );
  auto pa_iter = MakeIteratorAtLinearIndex( lines, 0 );
  Fori( u32, i, 0, nlines ) {
    auto line = *GetElemAtIterator( lines, pa_iter );
    pa_iter = IteratorMoveR( lines, pa_iter );

    while( line.len && isspace( line.mem[0] ) ) {
      line.mem += 1;
      line.len -= 1;
    }
    while( line.len && isspace( line.mem[ line.len - 1 ] ) ) {
      line.len -= 1;
    }
    if( !line.len ) {
      continue;
    }


  }
  Free( contents );
  FileClose( file );
}
#endif
