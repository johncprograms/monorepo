// Copyright (c) John A. Carlos Jr., all rights reserved.

//
// I'm using the terminology of:
// - "histogram" means an un-normalized probability mass function. I.e. just the counts.
// - "pmf" means a normalized probability mass function, so sum( pmf[i] ) over i equals 1.
// - "cmf" means the cumulative mass function, usually called a cdf. It's the accumulation of the pmf up until i.
//
// The basic idea of arithmetic encoding is:
//   let the cmf define interval boundaries on [0,1].
//   for each symbol in the given string,
//     contract the interval to the sub-interval which corresponds to the symbol in the cmf.
//   then we're left with a final interval [a,b].
//   pick some arbitrary value in [a,b] and transmit that
//
// in practice we have to represent the interval boundaries somehow. float or uint are the obvious choices.
// if we use floats, then float error accumulation is a concern. i'm not sure how bad.
// if we use uints, then we might want to renormalize to [0,1] and flush when we overflow. and/or use wide uints.
//
// note we might have representational problems if a cmf sub-interval is empty, i.e. [a,a].
// since the sub-interval is empty, we might start misattributing symbols to sub-intervals on either side of the empty one.
// and that means we'd have an ambiguous decoding situation, i believe.
// so we likely want to round up the cmf interval boundaries in our representation, rather than the usual round to nearest.
// well it's more complicated than that; we might have to continue shifting subsequent intervals so they're all non-empty.
// is this really a problem? empty cmf sub-intervals shouldn't have any effect, since those symbols never happen.
// we should be able to just assert if we see a symbol with an empty cmf sub-interval.
//
// note that we don't need to adjust the cmf in its entirety after each symbol. we can generally just keep track
// of the running sub-interval start, plus a scale factor.
//
// Let's take a simple realization: represent the [0,1] range as [0,MAX_u32].
// We'll have a cmf_lower[i], cmf_upper[i] for each i in the alphabet. Each in that same range.
// Then the idea is that we'll use u64s for intermediate temporaries, so we can avoid overflows.
// boundary_lower = 0
// boundary_upper = MAX_u32
// f_boundary_lower = 0
// f_boundary_upper = 1
// Given a symbol s,
//   lower = cmf_lower[s]
//   upper = cmf_upper[s]
//   f_lower = lower / MAX_u32
//   f_upper = upper / MAX_u32
//   f_boundary_len = f_boundary_upper - f_boundary_lower
//   f_lower_within_boundary = f_boundary_lower + f_lower / f_boundary_len
//   f_upper_within_boundary = f_boundary_lower + f_upper / f_boundary_len
//   f_boundary_lower = f_lower_within_boundary
//   f_boundary_upper = f_upper_within_boundary
//   boundary_lower = f_boundary_lower * MAX_u32
//   boundary_upper = f_boundary_upper * MAX_u32
//
// That's the math version, where f_ indicates infinite precision variables within [0,1].
// Now we can massage that a bit, in the hopes of using finite precision.
//
// f_lower = lower / MAX_u32
// boundary_len = boundary_upper - boundary_lower
// f_boundary_len = boundary_len / MAX_u32
// boundary_lower' = ( f_boundary_lower + f_lower / f_boundary_len ) * MAX_u32
// boundary_lower' = f_boundary_lower * MAX_u32 + f_lower * MAX_u32 / f_boundary_len
// boundary_lower' = boundary_lower + lower / f_boundary_len
// boundary_lower' = boundary_lower + lower * MAX_u32 / boundary_len
//
// By symmetry, we then have this as our answer:
// boundary_lower' = boundary_lower + lower * MAX_u32 / boundary_len
// boundary_upper' = boundary_lower + upper * MAX_u32 / boundary_len
//
// Now we've got a finite precision implementation of the math. There's still some bounds correctness
// to work out, but it's looking good already.
//
// The next problem is: since boundary_lower and boundary_upper are always coming closer to each other,
// and we've got finite precision, at what point do we reset them back wider?
// I think this might be the same theoretical problem as the empty sub-interval problem discussed above.
//
// Independently, once the highest bits of boundary_lower and boundary_upper are the same,
// we can transmit those and reclaim those bits in the range. Is that enough?
//
// The maximum problem case would be, upper - lower = 1.
// bl' = bl + (l + 0) * MAX_u32 / blen
// bu' = bl + (l + 1) * MAX_u32 / blen
// It would be bad if bl' = bu'.
// Since we're using u64 for the result of the multiply, we can't overflow that.
// And we're guaranteed that (lower + 1) <= MAX_u32, and boundary_len <= MAX_u32, so I think we're fine.
// If we wanted to be more register efficient, then we'd have to get smarter about it I think.
// Note that the highest bits emission guarantees (lower + 1) <= MAX_u32, as long as we do that first.
//
// Now what about decoding?
//   use the same cmf, defining interval boundaries on [0,1].
//   start with an initial boundary of [0,1]
//   take the first <=32bits of the code.
//   loop:
//     contract the boundary to the cmf sub-interval that corresponds to the symbol in the cmf.
//     emit that symbol.
//     shift the code left 1 bit, and populate the lowest bit with the next code bit.
//   repeat the loop until we've handled all the code bits.
//
// One question is about ordering of code bits.
// Encoding emits code bits in first symbol to last symbol order.
// We need decoding to read the code bits in the same order, since we're simulating the same recursive
// interval interpolation process. So decoding should read the code bits in forward order.
//

ForceInl void
CmfIntervalFromSymbol(
	u32* cmf, // array of length 256
	u8 byte,
	u32* cmf_lower,
	u32* cmf_upper
	)
{
	*cmf_lower = byte  ?  cmf[byte - 1]  :  0u;
	*cmf_upper = cmf[byte];
}

// emit the highest digit as long as lower and upper have the same highest digit.
ForceInl void
EmitIdenticalHighBits(
	u32& lower,
	u32& upper,
	u64* output,
	idx_t output_bitcapacity,
	idx_t& output_bitlen
	)
{
	Forever {
		auto lower_high_digit = GetBit( &lower, 32u, 31u );
		auto upper_high_digit = GetBit( &upper, 32u, 31u );
		if( lower_high_digit != upper_high_digit ) {
			break;
		}
		
		// emit the highest digit of lower.
		auto bit = GetBit( &lower, 32u, 31u );
		SetBit( output, output_bitcapacity, output_bitlen, bit );
		output_bitlen += 1u;
		
		// left shift lower and upper, shifting in a 0 for lower and 1 for upper.
		lower = lower << 1u;
		upper = ( upper << 1u ) | 1u;
	}
}
Inl void
Encode(
	slice_t input,
	u32* cmf, // array of length 256
	u64* output,
	idx_t output_bitcapacity,
	idx_t& output_bitlen
	)
{
	u32 lower = 0;
	u32 upper = MAX_u32;
	ForLen( i, input ) {
		auto s = input.mem[i];
		
		u32 cmf_lower;
		u32 cmf_upper;
		CmfIntervalFromSymbol( cmf, s, &cmf_lower, &cmf_upper );
		
		// So we've got cmf_lower, cmf_upper in [0,MAX_u32]
		// We want to re-lerp that into [lower, upper]
		// The re-lerp formula from x to y is given: y = y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 )
		// So for our case,
		// new_lower = lower + ( ( upper - lower ) / ( MAX_u32 - 0 ) ) * ( cmf_lower - 0 )
		// new_lower = lower + ( ( upper - lower ) / MAX_u32 ) * cmf_lower
		// new_lower = lower + ( upper - lower ) * cmf_lower / MAX_u32
		// And by symmetry, new_upper is similar.
	
		// WARNING: logic duplicated below in decoder.
		AssertCrash( lower < upper );
		u64 range = Cast( u64, upper ) - Cast( u64, lower ) + 1u;
		u64 new_lower_numer = Cast( u64, lower ) + ( range * Cast( u64, cmf_lower ) );
		u64 new_upper_numer = Cast( u64, lower ) + ( range * Cast( u64, cmf_upper ) );
		u64 new_lower = new_lower_numer / MAX_u32;
		u64 new_upper = new_upper_numer / MAX_u32;
		u64 new_lower_remainder = new_lower_numer % MAX_u32;
		if( new_lower_remainder ) { // always round lower up, so it stays within the idealized mathematical interval.
			new_lower += 1;
		}
		// always round upper down ( which happens implicitly above ), so it stays within the idealized mathematical interval.
		AssertCrash( new_lower <= MAX_u32 );
		AssertCrash( new_upper <= MAX_u32 );
		lower = Cast( u32, new_lower );
		upper = Cast( u32, new_upper );
		
		EmitIdenticalHighBits( lower, upper, output, output_bitcapacity, output_bitlen );
	}
	
	For( i, 0, 32 ) {
		// emit the highest digit of lower.
		auto bit = GetBit( &lower, 32u, 31u );
		SetBit( output, output_bitcapacity, output_bitlen, bit );
		output_bitlen += 1u;
		
		lower = lower << 1u;
	}
}

ForceInl void
SymbolFromCmfValue(
	u32* cmf, // array of length 256
	u32 cmf_value,
	u8* symbol,
	u32* cmf_lower,
	u32* cmf_upper
	)
{
	// ALTERNATE: if we use {0,1} as the alphabet instead of {0,...,255}, this search turns into a single compare.
	// PERF: this is going to be the bulk of the decode loop, so it'd be nice to speed this up.
	//   i'm thinking: round codevalue to a power of 2, and then have separate cmf lists for each power of 2.
	//   also we could elide the bytes that have empty [lower,upper], i.e. pmf == 0.
	//   we're also comparing against each interval boundary twice; i's upper and i+1's lower, for example.
	//   so unrolling that would help.
	//   the cmf is also sorted by definition, so we could use various flavors of binary search to do fewer comparisons.
	For( byte, 0, 256 ) {
		idx_t lower = byte  ?  cmf[byte - 1]  :  0u;
		idx_t upper = cmf[byte];
		if( lower <= cmf_value  &&  cmf_value < upper ) {
			*symbol = Cast( u8, byte );
			*cmf_lower = Cast( u32, lower );
			*cmf_upper = Cast( u32, upper );
			return;
		}
	}
	UnreachableCrash();
}

ForceInl void
ConsumeCodeBit(
	u32& code,
  u64* input,
  idx_t input_bitlen,
  idx_t& input_bitpos
	)
{
	auto bit = GetBit( input, input_bitlen, input_bitpos );
	input_bitpos += 1u;

	code = ( code << 1u ) | Cast( u32, bit );
}
ForceInl void
ConsumeCodeBit0(
	u32& code
	)
{
	code = ( code << 1u );
}

// emit the highest digit as long as lower and upper have the same highest digit.
ForceInl void
ConsumeIdenticalHighBits(
	u32& code,
	u32& lower,
	u32& upper,
  u64* input,
  idx_t input_bitlen,
  idx_t& input_bitpos
	)
{
	Forever {
		auto lower_high_digit = GetBit( &lower, 32u, 31u );
		auto upper_high_digit = GetBit( &upper, 32u, 31u );
		if( lower_high_digit != upper_high_digit ) {
			break;
		}
		// left shift lower and upper, shifting in a 0 for lower and 1 for upper.
		lower = lower << 1u;
		upper = ( upper << 1u ) | 1u;
		
		if( input_bitpos < input_bitlen ) {
			ConsumeCodeBit( code, input, input_bitlen, input_bitpos );
		}
		else {
			ConsumeCodeBit0( code );
		}
	}
}

Inl void
Decode(
  u64* input,
  idx_t input_bitlen,
  idx_t input_num_symbols,
  u32* cmf, // array of length 256
  u8* output,
  idx_t output_capacity,
  idx_t& output_len
	)
{
	u32 lower = 0;
	u32 upper = MAX_u32;
	idx_t input_bitpos = 0;
	
	// Take the first <=32bits.
	u32 code = 0;
	For( i, 0, MIN( input_bitlen, 32 ) ) { // PERF: this could be a bit reverse I think.
		ConsumeCodeBit( code, input, input_bitlen, input_bitpos );
	}
	
	auto num_symbols_remaining = input_num_symbols;
	while( num_symbols_remaining-- ) {
	
		// We need to remap code within [lower, upper] into [0,MAX_u32].
		// The re-lerp formula from x to y is given: y = y0 + ( ( y1 - y0 ) / ( x1 - x0 ) ) * ( x - x0 )
		// So for our case,
		// cmf_code = 0 + ( ( MAX_u32 - 0 ) / ( upper - lower ) ) * ( code - lower )
		// cmf_code = ( code - lower ) * MAX_u32 / ( upper - lower )
		AssertCrash( lower < upper );
		u64 range = Cast( u64, upper ) - Cast( u64, lower ) + 1u;
	#if 1
		u64 cmf_code64 = ( Cast( u64, code - lower ) * MAX_u32 ) / range;
		AssertCrash( cmf_code64 <= MAX_u32 );
		u32 cmf_code = Cast( u32, cmf_code64 );
	#else
		u32 cmf_code = code;
	#endif
		
		u32 cmf_lower;
		u32 cmf_upper;
		u8 symbol;
		SymbolFromCmfValue( cmf, cmf_code, &symbol, &cmf_lower, &cmf_upper );
		
		AssertCrash( output_len < output_capacity );
		output[ output_len ] = symbol;
		output_len += 1;
		
		// WARNING: logic duplicated below in decoder.
		AssertCrash( lower < upper );
		u64 new_lower_numer = Cast( u64, lower ) + ( range * Cast( u64, cmf_lower ) );
		u64 new_upper_numer = Cast( u64, lower ) + ( range * Cast( u64, cmf_upper ) );
		u64 new_lower = new_lower_numer / MAX_u32;
		u64 new_upper = new_upper_numer / MAX_u32;
		u64 new_lower_remainder = new_lower_numer % MAX_u32;
		if( new_lower_remainder ) { // always round lower up, so it stays within the idealized mathematical interval.
			new_lower += 1;
		}
		// always round upper down ( which happens implicitly above ), so it stays within the idealized mathematical interval.
		AssertCrash( new_lower <= MAX_u32 );
		AssertCrash( new_upper <= MAX_u32 );
		lower = Cast( u32, new_lower );
		upper = Cast( u32, new_upper );

		ConsumeIdenticalHighBits( code, lower, upper, input, input_bitlen, input_bitpos );
	}
}

//constant u32 CODE_VALUE_BITS = ( std::numeric_limits<u32>::digits + 3u ) / 2u;
constant u32 CODE_VALUE_BITS = 17u;
constant u32 FREQUENCY_BITS = std::numeric_limits<u32>::digits - CODE_VALUE_BITS;
constant u32 MAX_CODE = ( 1u << CODE_VALUE_BITS ) - 1u;
constant u32 MAX_FREQUENCY = ( 1u << FREQUENCY_BITS ) - 1u;
constant u32 ONE_FOURTH = 1u << ( CODE_VALUE_BITS - 2u );
constant u32 ONE_HALF = 2u * ONE_FOURTH;
constant u32 THREE_FOURTHS = 3u * ONE_FOURTH;

struct prob_t
{
	u32 low;
	u32 high;
	u32 count;
};

ForceInl prob_t
GetProb(
	u32* cmf, // array of length 257
	u8 byte
	)
{
	prob_t p;
	p.low = cmf[byte];
	p.high = cmf[ byte + 1 ];
	p.count = cmf[256];
	return p;
}

ForceInl void
EmitBitPlusPending(
	bool bit,
	u32& pending_bits,
	u64* output,
	idx_t output_bitcapacity,
	idx_t& output_bitlen
	)
{
	SetBit( output, output_bitcapacity, output_bitlen, bit );
	output_bitlen += 1u;
	For( i, 0, pending_bits ) {
		SetBit( output, output_bitcapacity, output_bitlen, !bit );
		output_bitlen += 1u;
	}
	pending_bits = 0;
}

Inl void
REncode(
	slice_t input,
	u32* cmf, // array of length 257
	u64* output,
	idx_t output_bitcapacity,
	idx_t& output_bitlen
	)
{
	u32 pending_bits = 0;
	u32 low = 0;
	u32 high = MAX_CODE;
	ForLen( i, input ) {
		auto s = input.mem[i];
		
		prob_t p = GetProb( cmf, s );
		u32 range = high - low + 1u;
		high = low + ( range * p.high / p.count ) - 1;
		low = low + ( range * p.low / p.count );
		
		Forever {
			if( high < ONE_HALF ) {
				EmitBitPlusPending( 0, pending_bits, output, output_bitcapacity, output_bitlen );
			}
			elif( low >= ONE_HALF ) {
				EmitBitPlusPending( 1, pending_bits, output, output_bitcapacity, output_bitlen );
			}
			elif( low >= ONE_FOURTH && high < THREE_FOURTHS ) {
				pending_bits += 1u;
				low -= ONE_FOURTH;
				high -= ONE_FOURTH;
			}
			else {
				break;
			}
			
			high = ( high << 1u ) | 1u;
			low = low << 1u;
			high &= MAX_CODE;
			low &= MAX_CODE;
		}
	}
	pending_bits += 1;
	if( low < ONE_FOURTH ) {
		EmitBitPlusPending( 0, pending_bits, output, output_bitcapacity, output_bitlen );
	}
	else {
		EmitBitPlusPending( 1, pending_bits, output, output_bitcapacity, output_bitlen );
	}
}

ForceInl prob_t
GetChar(
	u32* cmf, // array of length 257
	u32 scaled_value,
	u8& symbol
	)
{
	For( i, 0, 256 ) {
		if( scaled_value < cmf[i + 1] ) {
			symbol = Cast( u8, i );
			prob_t p = GetProb( cmf, symbol );
			return p;
		}
	}
	UnreachableCrash();
	return {};
}

ForceInl void
ConsumeCodeBitOr0(
	u32& code,
  u64* input,
  idx_t input_bitlen,
  idx_t& input_bitpos
	)
{
	// Because the encoder terminates the bitstream early, we might read past the input_bitlen.
	// The encoder guarantees that implicit 0s are the appropriate thing to do in this case.
	if( input_bitpos < input_bitlen ) {
		ConsumeCodeBit( code, input, input_bitlen, input_bitpos );
	}
	else {
		ConsumeCodeBit0( code );
	}
}

Inl void
RDecode(
  u64* input,
  idx_t input_bitlen,
  idx_t input_num_symbols,
  u32* cmf, // array of length 257
  u8* output,
  idx_t output_capacity,
  idx_t& output_len
	)
{
	AssertCrash( !output_len ); // see the 'return;' loop termination below, which assumes this.
	u32 high = MAX_CODE;
	u32 low = 0;
	u32 value = 0;
	idx_t input_bitpos = 0;
	For( i, 0, CODE_VALUE_BITS ) {
		ConsumeCodeBitOr0( value, input, input_bitlen, input_bitpos );
	}
	Forever {
		u32 range = high - low + 1;
		u32 scaled_value = ( ( value - low + 1u ) * cmf[256] - 1u ) / range;
		u8 s;
		prob_t p = GetChar( cmf, scaled_value, s );
		output[ output_len ] = s;
		output_len += 1;
		if( output_len == input_num_symbols ) {
			return;
		}
		high = low + ( range * p.high ) / p.count - 1;
		low = low + ( range * p.low ) / p.count;
		Forever {
			if( high < ONE_HALF ) {
				// Do nothing
			}
			elif( low >= ONE_HALF ) {
				value -= ONE_HALF;
				low -= ONE_HALF;
				high -= ONE_HALF;
			}
			elif( low >= ONE_FOURTH && high < THREE_FOURTHS ) {
				value -= ONE_FOURTH;
				low -= ONE_FOURTH;
				high -= ONE_FOURTH;
			}
			else {
				break;
			}
			low = low << 1u;
			high = ( high << 1u ) | 1u;
			ConsumeCodeBitOr0( value, input, input_bitlen, input_bitpos );
		}
	}
}

RegisterTest([]()
{
	{
		auto bytestream = SliceFromCStr( "abbccc" );
		u32 cmf[257] = { 0 };
		cmf['b'] = 0x100u;
		cmf['c'] = 0x1000u;
	  For( i, 'd', _countof( cmf ) ) {
			cmf[i] = 0xFFFFu;
	  }
		u64 encoded[1000];
		idx_t encoded_bitlen = 0;
		REncode( bytestream, cmf, encoded, 8 * _countof( encoded ), encoded_bitlen );
		u8 output[1000];
		idx_t output_len = 0;
		RDecode( encoded, encoded_bitlen, bytestream.len, cmf, AL( output ), output_len );
		slice_t output_slice = { output, output_len };
		AssertCrash( EqualContents( bytestream, output_slice ) );
	}
	
	{
		auto bytestream = SliceFromCStr( "abbccc" );
		u32 cmf[257];
		U32_257_CmfOfBytestream( bytestream, cmf, MAX_FREQUENCY );
		u64 encoded[1000];
		idx_t encoded_bitlen = 0;
		REncode( bytestream, cmf, encoded, 8 * _countof( encoded ), encoded_bitlen );
		u8 output[1000];
		idx_t output_len = 0;
		RDecode( encoded, encoded_bitlen, bytestream.len, cmf, AL( output ), output_len );
		slice_t output_slice = { output, output_len };
		AssertCrash( EqualContents( bytestream, output_slice ) );
	}
	
#if 0 // Encode/Decode uses too few CODE_VALUE_BITS
	{
		auto bytestream = SliceFromCStr( "abbccc" );
		u32 cmf[256];
		U32CmfOfBytestream( bytestream, cmf );
		u64 encoded[1000];
		idx_t encoded_bitlen = 0;
		Encode( bytestream, cmf, encoded, 8 * _countof( encoded ), encoded_bitlen );
		u8 output[1000];
		idx_t output_len = 0;
		Decode( encoded, encoded_bitlen, bytestream.len, cmf, AL( output ), output_len );
		slice_t output_slice = { output, output_len };
		AssertCrash( EqualContents( bytestream, output_slice ) );
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
			auto input = SliceFromString( bytestream );
			
			u32 cmf[256];
			U32CmfOfBytestream( input, cmf );
			u64 encoded[max_len] = {};
			idx_t encoded_bitlen = 0;
			Encode( input, cmf, encoded, 8 * _countof( encoded ), encoded_bitlen );
			u8 output[max_len];
			idx_t output_len = 0;
			Decode( encoded, encoded_bitlen, bytestream.len, cmf, AL( output ), output_len );
			slice_t output_slice = { output, output_len };
			AssertCrash( EqualContents( input, output_slice ) );
			auto compress_ratio = Cast( f32, encoded_bitlen ) / ( 8.0f * output_len );
			AssertCrash( compress_ratio <= 1.0f );
			Free( bytestream );
		}
	}
#endif
});

#if 0

encode: given a list of symbols.
	[lower,upper] = [0,1] to start.
	for each symbol s:
		[cmf_lower,cmf_upper] can be calculated for this symbol s and given cmf; see above.
		[lower,upper] <- [lerp(lower, upper, cmf_lower), lerp(lower, upper, cmf_upper)]
	emit an arbitrary number within [lower,upper].

decode: given a codevalue that was the output of encode, and num_symbols_encoded
	while num_symbols_encoded--
		determine which [cmf_lower,cmf_upper] the current codevalue belongs to.
		emit the symbol corresponding to that interval.
		codevalue <- lerp(0, 1, cmf_lower, cmf_upper, codevalue)

these are the infinite precision algorithms i believe.


#endif
