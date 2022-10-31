// Copyright (c) John A. Carlos Jr., all rights reserved.

// TODO: runtime error when output_rundata isn't long enough. or variants with variably-sized return containers.
Templ void
RunLengthEncode(
  T* input, // array of length input_len
  idx_t input_len,
  T* output_rundata, // array of length output_len
  idx_t* output_runlengths, // array of length output_len
  idx_t* output_len_
  )
{
  idx_t output_len = 0;
  Fori( idx_t, i, 0, input_len ) {
    auto input_i = input[i];
    idx_t j = i + 1;
    for( ; j < input_len; ++j ) {
      auto input_j = input[j];
      if( input_i != input_j ) break;
    }
    output_rundata[ output_len ] = input_i;
    output_runlengths[ output_len ] = j - i;
    output_len += 1;
    i = j - 1;
  }
  *output_len_ = output_len;
}

// TODO: runtime error when output isn't long enough. or variants with variably-sized return containers.
Templ void
RunLengthDecode(
  T* input_rundata, // array of length input_len
  idx_t* input_runlengths, // array of length input_len
  idx_t input_len,
  T* output, // array of length output_len
  idx_t* output_len_
  )
{
  idx_t output_len = 0;
  For( i, 0, input_len ) {
    auto rundata = input_rundata[i];
    auto runlength = input_runlengths[i];
    For( j, 0, runlength ) {
      output[ output_len + j ] = rundata;
    }
    output_len += runlength;
  }
  *output_len_ = output_len;
}

static void
TestRLE()
{
  constant idx_t N = 50;
  u8 input[N];
  u8 encoded_rundata[N];
  idx_t encoded_runlengths[N];
  u8 decoded[N];

  rng_xorshift32_t rng;
  Init( rng, 0x1234567812345678ULL );

  constant idx_t num_sequences = 1000;
  For( seq, 0, num_sequences ) {

    For( i, 0, N ) {
      input[i] = Rand32( rng ) % 4;
    }
    Arrayzero( encoded_rundata );
    Arrayzero( encoded_runlengths );
    Arrayzero( decoded );

    idx_t encoded_len;
    RunLengthEncode( AL( input ), encoded_rundata, encoded_runlengths, &encoded_len );

    idx_t decoded_len;
    RunLengthDecode( encoded_rundata, encoded_runlengths, encoded_len, decoded, &decoded_len );
    AssertCrash( decoded_len == N );
    For( i, 0, N ) {
      auto input_i = input[i];
      auto decoded_i = decoded[i];
      AssertCrash( input_i == decoded_i );
    }
  }
}
