// Copyright (c) John A. Carlos Jr., all rights reserved.

// I'm using the terminology of:
// - "histogram" means an un-normalized probability mass function. I.e. just the counts.
// - "pmf" means a normalized probability mass function, so sum( pmf[i] ) over i equals 1.

// For starters, I'm simplifying the problem space down to the alphabet of bytes.
// This means this code won't be all that practical generally, but it's a reasonable starting point,
// as I learn the space. I'll generalize later.

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

Templ Inl void
PmfFromHistogram(
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
PmfOfBytestream(
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

struct
huffman_node_t
{
  u8 symbol;
  f32 weight;

  // TODO: implicit binary tree, rather than explicit?
  huffman_node_t* left;
  huffman_node_t* rght;
};
Inl bool operator<( const huffman_node_t& a, const huffman_node_t& b )
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
    node->symbol = Cast( u8, i );
    node->weight = pmf[i];
    node->left = 0;
    node->rght = 0;
  }

  InitMinHeapInPlace( ML( buffer ) );

  while( buffer.len > 1 ) {
    auto lowest = AddBack( *tree );
    auto next_lowest = AddBack( *tree );
    MinHeapExtract( &buffer, lowest );
    MinHeapExtract( &buffer, next_lowest );
    huffman_node_t internal;
    internal.symbol = 0;
    internal.weight = lowest->weight + next_lowest->weight;
    internal.left = lowest;
    internal.rght = next_lowest;
    MinHeapInsert( &buffer, &internal );
  }

  *AddBack( *tree ) = buffer.mem[0];
  *root = tree->mem + tree->len - 1;

  Free( buffer );
}

struct
huffman_codevalue_t
{
  // symbol is implicit in table ordering, since we're assuming 256 bytes.
  // TODO: is u8 enough bits to store these?
  //   the tree should have <= 2^8=256 levels since it's unbalanced binary.
  //   since the root->leaf path lengths equate to 'nbits', we should only need <= 8 bits per value.
  //   so in the worst-case, we're just re-labeling all the bytes arbitrarily.
  //   but, otherwise, we're likely using fewer bits, which is the whole point.
  u8 nbits;
  u8 value;
};
Inl void
_GenerateHuffmanTable(
  huffman_node_t* root,
  huffman_codevalue_t* table, // array of length 256
  idx_t nbits,
  idx_t value
  )
{
  AssertCrash( root );
  auto left = root->left;
  auto rght = root->rght;
  if( left ) {
    _GenerateHuffmanTable( left, table, nbits + 1, value << 1 );
  }
  if( rght ) {
    _GenerateHuffmanTable( rght, table, nbits + 1, ( value << 1 ) | 1 );
  }
  if( !( Cast( idx_t, left ) | Cast( idx_t, rght ) ) ) {
    AssertCrash( nbits < 256 );
    AssertCrash( value < 256 );
    auto codevalue = table + root->symbol;
    codevalue->nbits = Cast( u8, nbits );
    codevalue->value = Cast( u8, value );
  }
}
Inl void
GenerateHuffmanTable(
  huffman_node_t* root,
  huffman_codevalue_t* table // array of length 256
  )
{
  _GenerateHuffmanTable( root, table, 0, 0 );
}

Inl void
WriteBit(
  u8* output,
  idx_t output_capacity,
  idx_t& output_last_byte,
  idx_t& output_last_bitlen,
  bool bit
  )
{
  AssertCrash( output_last_byte < output_capacity );
  AssertCrash( output_last_bitlen < 8 );
  output[output_last_byte] |= ( bit << ( 8 - output_last_bitlen ) );
  output_last_bitlen += 1;
  if(output_last_bitlen == 8 ) {
    output_last_bitlen = 0;
    output_last_byte += 1;
  }
}
Inl bool
ReadBit(
  u8* input,
  idx_t& input_last_byte,
  idx_t& input_last_bitlen
  )
{
  AssertCrash( input_last_bitlen < 8 );
  auto bit = ( input[input_last_byte] >> ( 8u - input_last_bitlen ) ) & 0b1u;
  input_last_bitlen += 1;
  if( input_last_bitlen == 8 ) {
    input_last_bitlen = 0;
    input_last_byte += 1;
  }
  return bit;
}
Inl void
WriteBits(
  u8* output,
  idx_t output_capacity,
  idx_t& output_last_byte,
  idx_t& output_last_bitlen,
  idx_t bits_write,
  idx_t nbits_write
  )
{
  AssertCrash( nbits_write <= 8u );
  AssertCrash( output_last_byte < output_capacity );
  AssertCrash( output_last_bitlen < 8 );
  auto first_byte = output[output_last_byte];
  if( output_last_bitlen + nbits_write <= 8u ) {
    auto shift = 8u - output_last_bitlen - nbits_write;
    output[output_last_byte] = first_byte | Cast( u8, bits_write << shift );
    output_last_bitlen += nbits_write;
    if( output_last_bitlen == 8u ) {
      output_last_bitlen = 0;
      output_last_byte += 1;
    }
  }
  else {
    AssertCrash( output_last_byte + 1 < output_capacity );
    auto second_byte = output[output_last_byte + 1];
    auto rshift = output_last_bitlen + nbits_write - 8u;
    output[output_last_byte] = first_byte | Cast( u8, bits_write >> rshift );
    output_last_byte += 1;
    auto lshift = 16u - output_last_bitlen - nbits_write;
    output[output_last_byte] = second_byte | Cast( u8, bits_write << lshift );
    output_last_bitlen = output_last_bitlen + nbits_write - 8u;
    AssertCrash( output_last_bitlen < 8u );
  }

  // |-------|-------|-------|-------|...
  // 01234567012345670123456701234567 ...
  // Let's take the example of:
  // len: 2          ^
  // last_bitlen: 3     ^
  // nbits_write: 7
  // value_write: 1111111
  // so we should end up with:
  // len: 3                  ^
  // last_bitlen: 2            ^
  //
}



Inl void
HuffmanEncode(
  huffman_codevalue_t* table, // array of length 256
  slice_t input,
  u8* output,
  idx_t output_capacity,
  idx_t& output_last_byte,
  idx_t& output_last_bitlen
  )
{
  // TODO: elide this if the caller's already done this.
  TZero( output, output_capacity );

  //
  // memory layout for output on a bit level looks like:
  // |-------|-------|-------|...
  // 012345670123456701234567 ...
  //
  // note that 'len' indexes at the byte boundaries,
  // whereas 'last_bitlen' indexes bits inside a single byte. specifically, the last one.
  //
  output_last_byte = 0;
  output_last_bitlen = 0;
  ForLen( i, input ) {
    auto input_byte = input.mem[i];
    auto codevalue = table + input_byte;
    auto nbits_write = codevalue->nbits;
    auto value_write = codevalue->value;
    //
    // now we shift value_write some number of bits to the left, to align with last_bitlen.
    // then we can OR in the value_write.
    // note we should always zero the output first, so we can OR trivially.
    // ORing means we don't have to do masked writes or anything tricky like that.
    // well actually direct copying is faster, so maybe we just OR for the first and last partial bytes.
    // well since nbits_write is <= 8, we only ever have 2 partial bytes, so copying is out, practically.
    //
    // TODO: dynamic failure for output_capacity too small. that's probably somewhat common.
    WriteBits( output, output_capacity, output_last_byte, output_last_bitlen, value_write, nbits_write );
  }
}

Inl void
HuffmanDecode(
  huffman_node_t* root,
  u8* input,
  idx_t input_last_byte,
  idx_t input_last_bitlen,
  u8* output,
  idx_t output_capacity,
  idx_t& output_len
  )
{
  auto node = root;
  idx_t last_byte = 0;
  idx_t last_bitlen = 0;
  Forever {
    if( last_byte == input_last_byte && last_bitlen == input_last_bitlen ) {
      break;
    }
    auto left = node->left;
    auto rght = node->rght;
    AssertCrash( Cast( idx_t, left ) | Cast( idx_t, rght ) );
    auto bit = ReadBit( input, last_byte, last_bitlen );
    if( bit ) {
      if( rght ) {
        node = rght;
      }
      else {
        AssertCrash( output_len < output_capacity );
        output[output_len] = node->symbol;
        output_len += 1;
        node = root;
      }
    }
    else {
      if( left ) {
        node = left;
      }
      else {
        AssertCrash( output_len < output_capacity );
        output[output_len] = node->symbol;
        output_len += 1;
        node = root;
      }
    }
  }
}



#if defined(TEST)

void
TestHuffman()
{
  if(0) { // TODO: fix crash.
    auto bytestream = SliceFromCStr( "abbcccddddeeeee" );
    f32 pmf[256];
    PmfOfBytestream( bytestream, pmf );
    stack_resizeable_cont_t<huffman_node_t> tree;
    Alloc( tree, 512 );
    huffman_node_t* root = 0;
    GenerateHuffmanTree( pmf, &tree, &root );
    huffman_codevalue_t table[256] = {};
    GenerateHuffmanTable( root, table );
    u8 encoded[1000] = {};
    idx_t encoded_last_byte = 0;
    idx_t encoded_last_bitlen = 0;
    HuffmanEncode( table, bytestream, AL( encoded ), encoded_last_byte, encoded_last_bitlen );
    u8 decoded[1000] = {};
    idx_t decoded_len = 0;
    HuffmanDecode( root, encoded, encoded_last_byte, encoded_last_bitlen, AL( decoded ), decoded_len );
    slice_t decoded_slice = { decoded, decoded_len };
    AssertCrash( EqualContents( bytestream, decoded_slice ) );
    Free( tree );
  }
}

#endif // defined(TEST)
