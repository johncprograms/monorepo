// Copyright (c) John A. Carlos Jr., all rights reserved.

// TODO: finish implementing this.

struct
radixtree_edge_t
{
  slice_t text;
};
struct
radixtree_t
{
  pagelist_t* mem;
  n_t root;
  n_t N;
  n_t capacity_N;
  e_t E;
  e_t capacity_E; // TODO: array isn't ideal for per-edge data, since E can be huge
  e_t* outoffsets; // length N
  e_t* outdegrees; // length N
  e_t* outdegcaps; // length N
  n_t* outnodes; // length E
  radixtree_edge_t* outedgeprops; // length E
};
Inl void
LookupRaw(
  radixtree_t* t,
  slice_t s,
  idx_t* num_chars_found_,
  n_t* last_node_found_
  )
{
  auto node = t->root;
  idx_t num_chars_found = 0;
  auto last_node_found = node;
  Forever {
LOOP_NODE:
    if( num_chars_found >= s.len ) {
      break;
    }
    auto outdegree = t->outdegrees[node];
    if( !outdegree ) {
      break;
    }
    auto outoffset = t->outoffsets[node];
    Fori( e_t, e, 0, outdegree ) {
      auto outnode = t->outnodes[ outoffset + e ];
      auto outedgeprop = t->outedgeprops[ outoffset + e ];
      auto edge_text = outedgeprop->text;
      // compare suffix( s, num_chars_found ) against edge_text
      auto suffix_len = MIN( s.len, num_chars_found );
      auto suffix_offset = s.len - suffix_len;
      auto max_prefix_len = MIN( edge_text.len, suffix_len );
      auto suffix_mem = s.mem + suffix_offset;
      auto edge_text_mem = edge_text.mem;
      auto prefix_match = MemEqual( suffix_mem, edge_text_mem, max_prefix_len );
      if( prefix_match ) {
        last_node_found = node;
        node = outnode;
        num_chars_found += max_prefix_len;
        goto LOOP_NODE;
      }
    }
    // no matching prefix found!
    break;
  }
  *num_chars_found_ = num_chars_found;
  *last_node_found_ = last_node_found;
}
Inl void
Insert(
  radixtree_t* t,
  slice_t s,
  bool* already_there,
  n_t* node_found
  )
{
  //
  // traverse the tree to find the existing prefix-matching chain.
  // we may have a suffix of s to deal with afterwards.
  //
  idx_t num_chars_found;
  n_t last_node_found;
  LookupRaw( t, s, &num_chars_found, &last_node_found );
  if( num_chars_found == s.len ) {
    *already_there = true;
    *node_found = last_node_found;
    return;
  }
  //
  // check for a partial-prefix match against our remaining suffix of s.
  // we may have to split an edge, introducing a new node, to deal with this.
  //
  auto outdegree = t->outdegrees[node];
  auto outoffset = t->outoffsets[node];
  Fori( e_t, e, 0, outdegree ) {
    auto outnode = t->outnodes[ outoffset + e ];
    auto outedgeprop = t->outedgeprops[ outoffset + e ];
    auto edge_text = outedgeprop->text;
    // compare suffix( s, num_chars_found ) against edge_text
    auto suffix_len = MIN( s.len, num_chars_found );
    auto suffix_offset = s.len - suffix_len;
    auto max_prefix_len = MIN( edge_text.len, suffix_len );
    auto suffix_mem = s.mem + suffix_offset;
    auto edge_text_mem = edge_text.mem;
    idx_t prefix_len = 0;
    while( prefix_len < max_prefix_len ) {
      if( *suffix_mem++ != *edge_text_mem++ ) {
        break;
      }
      prefix_len += 1;
    }
    if( prefix_len ) {
      //
      // we're guaranteed that only one edge will match here, because we'll
      // split all other edges to maintain that unique-prefix property.
      //
      TODO
      return;
    }
  }
  //
  // no existing children had a matching prefix, so we can simply add a new child.
  //
  auto capacity_N = t->capacity_N;
  Reserve( t->outoffsets, &capacity_N, capacity_N, N, N + 1 );
  Reserve( t->outdegrees, &capacity_N, capacity_N, N, N + 1 );
  Reserve( t->outdegcaps, &capacity_N, capacity_N, N, N + 1 );
  t->capacity_N = capacity_N;
  auto node_add = t->N;
  t->N += 1;
  t->outoffsets[node_add] = 0;
  t->outdegrees[node_add] = 0;
  t->outdegcaps[node_add] = 0;
  auto outdegcap = t->outdegcaps[node];
  if( outdegcap < outdegree + 1 ) {
    t->out
  }

  auto capacity_E = t->capacity_E;
  Reserve( t->outnodes    , &capacity_E, capacity_E, E, E + 1 );
  Reserve( t->outedgeprops, &capacity_E, capacity_E, E, E + 1 );
  t->capacity_E = capacity_E;
  auto edge_add = t->E;
  t->E += 1;

  n_t* outnodes; // length E
  radixtree_edge_t* outedgeprops; // length E


      // compare suffix( s, num_chars_found ) against edge_text
      auto suffix_len = MIN( s.len, num_chars_found );
      auto suffix_offset = s.len - suffix_len;
      auto max_prefix_len = MIN( edge_text.len, suffix_len );
      idx_t prefix_len = 0;
      auto suffix_mem = s.mem + suffix_offset;
      auto edge_text_mem = edge_text.mem;
      while( prefix_len < max_prefix_len ) {
        if( *suffix_mem++ != *edge_text_mem++ ) {
          break;
        }
        prefix_len += 1;
      }
      if( prefix_len ) {
        last_node_found = node;
        node = outnode;
        num_chars_found += prefix_len;
        goto LOOP_NODE;
      }
    }
}
