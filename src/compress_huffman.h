// Copyright (c) John A. Carlos Jr., all rights reserved.

// I'm using the terminology of:
// - "histogram" means an un-normalized probability mass function. I.e. just the counts.
// - "pmf" means a normalized probability mass function, so sum( pmf[i] ) over i equals 1.

// For starters, I'm simplifying the problem space down to the alphabet of bytes.
// This means this code won't be all that practical generally, but it's a reasonable starting point,
// as I learn the space.

Templ Inl void
HistogramOfBytestream(
  slice_t bytes,
  T* histogram // array of length 256
  )
{
  TZero( histogram, 256 );
  ForLen( i, bytes ) {
    auto byte = bytes.mem[i];
    histogram[byte] += 1;
  }
}

//
// PMF functions
//

Templ Inl void
F32PmfFromHistogram(
  T* histogram, // array of length 256
  f32* pmf // array of length 256
  )
{
  TZero( pmf, 256 );
  kahansum32_t sum = {};
  For( i, 0, 256 ) {
    Add( sum, histogram[i] );
  }
  auto normalization_factor = 1.0f / sum.sum;
  For( i, 0, 256 ) {
    pmf[i] = histogram[i] * normalization_factor;
  }
}
Inl void
F32PmfOfBytestream(
  slice_t bytes,
  f32* pmf // array of length 256
  )
{
  AssertCrash( bytes.len <= MAX_INT_REPRESENTABLE_IN_f32 );
  TZero( pmf, 256 );
  ForLen( i, bytes ) {
    auto byte = bytes.mem[i];
    pmf[byte] += 1;
  }
  auto normalization_factor = 1.0f / bytes.len;
  For( i, 0, 256 ) {
    pmf[i] *= normalization_factor;
  }
}
Inl void
F64PmfOfBytestream(
  slice_t bytes,
  f64* pmf // array of length 256
  )
{
  AssertCrash( bytes.len <= MAX_INT_REPRESENTABLE_IN_f64 );
  TZero( pmf, 256 );
  ForLen( i, bytes ) {
    auto byte = bytes.mem[i];
    pmf[byte] += 1;
  }
  auto normalization_factor = 1.0 / bytes.len;
  For( i, 0, 256 ) {
    pmf[i] *= normalization_factor;
  }
}

//
// CMF functions
// We're using the definition: CMF[i] = sum over j in [0,i] of PMF[i]
// We're guaranteed to have CMF[N] = 1 by the normalization property of the PMF.
// Note that there's an implicit 0 value at index -1 in theory; we just don't store it out of convenience.
//

Inl void
F32CmfOfBytestream(
	slice_t bytes,
	f32* cmf // array of length 256
	)
{
	F32PmfOfBytestream( bytes, cmf );
	kahansum32_t sum = {};
  For( i, 0, 256 ) {
		Add( sum, cmf[i] );
		cmf[i] = sum.sum;
  }
}
Inl void
F64CmfOfBytestream(
	slice_t bytes,
	f64* cmf // array of length 256
	)
{
	F64PmfOfBytestream( bytes, cmf );
	kahansum64_t sum = {};
  For( i, 0, 256 ) {
		Add( sum, cmf[i] );
		cmf[i] = sum.sum;
  }
}
Inl void
U32CmfOfBytestream(
	slice_t bytes,
	u32* cmf // array of length 256
	)
{
	f64 float_cmf[256];
	F64CmfOfBytestream( bytes, float_cmf );
	For( i, 0, 256 ) {
		cmf[i] = Cast( u32, float_cmf[i] * MAX_u32 );
	}
}
Inl void
U32_257_CmfOfBytestream(
	slice_t bytes,
	u32* cmf, // array of length 257
	u32 max_scale
	)
{
	f64 float_cmf[256];
	F64CmfOfBytestream( bytes, float_cmf );
	cmf[0] = 0;
	For( i, 0, 256 ) {
		cmf[i + 1] = Cast( u32, float_cmf[i] * max_scale );
	}
}

Enumc( huffman_nodetype_t )
{
	leaf,
	internal,
};
struct
huffman_node_t
{
	huffman_nodetype_t type;
  f32 weight;
	union {
		struct {
			u8 symbol;
		} leaf;
		struct {
			// TODO: implicit binary tree, rather than explicit?
			idx_t left; // index into tree
			idx_t rght; // index into tree
		} internal;
	};
};
ForceInl bool operator<( const huffman_node_t& a, const huffman_node_t& b )
{
  return a.weight < b.weight;
}
Inl void
GenerateHuffmanTree(
  f32* pmf, // array of length 256
  stack_resizeable_cont_t<huffman_node_t>* tree,
  huffman_node_t** root
  )
{
  stack_resizeable_cont_t<huffman_node_t> buffer;
  Alloc( buffer, 512 );

  For( i, 0, 256 ) {
    auto pmf_i = pmf[i];
    AssertCrash( pmf_i >= 0.0f );
    if( pmf_i == 0.0f ) {
      // skip symbols that aren't in use.
      continue;
    }
    auto node = AddBack( buffer );
    node->type = huffman_nodetype_t::leaf;
    node->weight = pmf[i];
    node->leaf.symbol = Cast( u8, i );
  }

  InitMinHeapInPlace( ML( buffer ) );

  while( buffer.len > 1 ) {
		auto idx_lowest = tree->len;
    auto lowest = AddBack( *tree );
    auto idx_next_lowest = tree->len;
    auto next_lowest = AddBack( *tree );
    MinHeapExtract( &buffer, lowest );
    MinHeapExtract( &buffer, next_lowest );
    huffman_node_t internal;
    internal.type = huffman_nodetype_t::internal;
    internal.weight = lowest->weight + next_lowest->weight;
    internal.internal.left = idx_lowest;
    internal.internal.rght = idx_next_lowest;
    MinHeapInsert( &buffer, &internal );
  }

  *AddBack( *tree ) = buffer.mem[0];
  *root = tree->mem + tree->len - 1;

  Free( buffer );
}

//
// This is the coded representation of a single alphabet symbol.
// Which symbol is implicit in the table ordering of these codevalues, since we're assuming 256 bytes.
//
// Important question: how many bits do we need per codevalue?
// there are <= 256 leaf nodes in the tree.
// so there are <= 255 internal nodes.
// since the tree can be arbitrarily unbalanced ( this is how the algorithm can be optimal ),
// we can have path lengths <= 256 nodes, or 255 edges.
// so in theory, we need 255 bits maximum per codevalue.
//
// however, it feels strange to "compress" a single char into something larger.
// in theory that's still the optimal thing to do, since it will only happen for infrequent symbols.
//
// mathematical question: what does the pmf have to look like for this to happen?
//
struct
huffman_codevalue_t
{
  u8 nbits;
  bitarray_nonresizeable_stack_t<255> value;
};
Inl void
GenerateHuffmanTable(
	tslice_t<huffman_node_t> tree,
  huffman_node_t* root,
  huffman_codevalue_t* table // array of length 256
  )
{
	struct
	elem_t
	{
		huffman_node_t* node;
		huffman_codevalue_t codevalue;
	};
	stack_resizeable_cont_t<elem_t> queue;
	Alloc( queue, 512 );
	{
		auto elem = AddBack( queue );
		elem->node = root;
		elem->codevalue.nbits = 0;
		Zero( elem->codevalue.value );
	}
	while( queue.len ) {
		auto elem = queue.mem[ queue.len - 1 ];
		RemBack( queue );
		auto node = elem.node;
		auto codevalue = elem.codevalue;
		AssertCrash( node );
		switch( node->type ) {
			case huffman_nodetype_t::leaf: {
				table[ node->leaf.symbol ] = codevalue;
			} break;
			case huffman_nodetype_t::internal: {
				AssertCrash( node->internal.left < tree.len );
				AssertCrash( node->internal.rght < tree.len );
				auto left = tree.mem + node->internal.left;
				auto rght = tree.mem + node->internal.rght;
				// The left node gets a 0 bit set, the right node gets a 1 bit set.
				AssertCrash( codevalue.nbits < 255 );
				auto cv_left = codevalue;
				cv_left.nbits += 1;
				auto cv_rght = codevalue;
				cv_rght.nbits += 1;
				SetBit1( cv_rght.value, codevalue.nbits );
				auto elem_left = AddBack( queue );
				auto elem_rght = AddBack( queue );
				elem_left->node = left;
				elem_left->codevalue = cv_left;
				elem_rght->node = rght;
				elem_rght->codevalue = cv_rght;
			} break;
		}
	}
	Free( queue );
}

Inl void
HuffmanEncode(
  huffman_codevalue_t* table, // array of length 256
  slice_t input,
  u64* output,
  idx_t output_bitcapacity,
  idx_t& output_bitlen
  )
{
	// TODO: dynamic failure for output_capacity too small. that's probably somewhat common.
	// PERF: we can use byte sized writes for nbits_write >= 8
	auto bitlen = 0;
  ForLen( i, input ) {
    auto input_byte = input.mem[i];
    auto codevalue = table + input_byte;
    auto nbits_write = codevalue->nbits;
    auto value_write = codevalue->value;
    For( b, 0, nbits_write ) {
			auto bit = GetBit( value_write, b );
			SetBit( output, output_bitcapacity, bitlen + b, bit );
    }
    bitlen += nbits_write;
  }
  output_bitlen = bitlen;
}

Inl void
HuffmanDecode(
	tslice_t<huffman_node_t> tree,
  huffman_node_t* root,
  u64* input,
  idx_t input_bitlen,
  u8* output,
  idx_t output_capacity,
  idx_t& output_len
  )
{
	// TODO: depointer accesses to output_len for this scope.
  auto node = root;
  idx_t idx_bit = 0;
  // For inf-loop safety here, we're relying on the structure of the huffman tree
	Forever {
		switch( node->type ) {
			case huffman_nodetype_t::leaf: {
        AssertCrash( output_len < output_capacity );
        output[output_len] = node->leaf.symbol;
        output_len += 1;
        node = root;
        if( idx_bit == input_bitlen ) {
					return;
        }
			} break;
			case huffman_nodetype_t::internal: {
				AssertCrash( node->internal.left < tree.len ); // PERF elide this.
				AssertCrash( node->internal.rght < tree.len );
				auto left = tree.mem + node->internal.left;
				auto rght = tree.mem + node->internal.rght;
				auto bit = GetBit( input, input_bitlen, idx_bit );
				if( bit ) {
					node = rght;
				}
				else {
					node = left;
				}
				idx_bit += 1;
			} break;
		}
  }
}


#if defined(TEST)

void
TestHuffman()
{
	{
		auto bytestream = SliceFromCStr( "abbcccddddeeeee" );
		f32 pmf[256];
		F32PmfOfBytestream( bytestream, pmf );
		stack_resizeable_cont_t<huffman_node_t> tree;
		Alloc( tree, 512 );
		huffman_node_t* root = 0;
		GenerateHuffmanTree( pmf, &tree, &root );
		huffman_codevalue_t table[256] = {};
		GenerateHuffmanTable( SliceFromArray( tree ), root, table );
		u64 encoded[1000] = {};
		idx_t encoded_bitlen = 0;
		HuffmanEncode( table, bytestream, encoded, 8u * _countof( encoded ), encoded_bitlen );
		u8 decoded[1000] = {};
		idx_t decoded_len = 0;
		HuffmanDecode( SliceFromArray( tree ), root, encoded, encoded_bitlen, AL( decoded ), decoded_len );
		slice_t decoded_slice = { decoded, decoded_len };
		AssertCrash( EqualContents( bytestream, decoded_slice ) );
		Free( tree );
	}
	
	{
		rng_xorshift32_t rng;
		Init( rng, 1234u );
		constant idx_t n = 1000u;
		For( i, 0, n ) {
			constant idx_t min_len = 10u;
			constant idx_t max_len = 1000u;
			idx_t len = Rand32( rng ) % max_len;
			len = MAX( min_len, len );
			auto bytestream = AllocString( len );
			For( j, 0, len ) {
				bytestream.mem[j] = Rand32( rng ) % 256u;
			}
			
			f32 pmf[256];
			F32PmfOfBytestream( SliceFromString( bytestream ), pmf );
			stack_resizeable_cont_t<huffman_node_t> tree;
			Alloc( tree, max_len );
			huffman_node_t* root = 0;
			GenerateHuffmanTree( pmf, &tree, &root );
			huffman_codevalue_t table[256] = {};
			GenerateHuffmanTable( SliceFromArray( tree ), root, table );
			u64 encoded[max_len] = {};
			idx_t encoded_bitlen = 0;
			HuffmanEncode( table, SliceFromString( bytestream ), encoded, 8u * _countof( encoded ), encoded_bitlen );
			u8 decoded[max_len] = {};
			idx_t decoded_len = 0;
			HuffmanDecode( SliceFromArray( tree ), root, encoded, encoded_bitlen, AL( decoded ), decoded_len );
			slice_t decoded_slice = { decoded, decoded_len };
			AssertCrash( EqualContents( SliceFromString( bytestream ), decoded_slice ) );
			auto compress_ratio = Cast( f32, encoded_bitlen ) / ( 8.0f * decoded_len );
			AssertCrash( compress_ratio <= 1.0f );
			Free( tree );
			Free( bytestream );
		}
	}
}

#endif // defined(TEST)
