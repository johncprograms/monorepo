
# Table of Contents
- [Table of Contents](#table-of-contents)
- [What is a computer?](#what-is-a-computer)
- [Memory array](#memory-array)
	- [Memory cell](#memory-cell)
	- [Modulo arithmetic](#modulo-arithmetic)
		- [Overflow](#overflow)
		- [Underflow](#underflow)
		- [Multiplication](#multiplication)
		- [Division](#division)
		- [Detecting overflow](#detecting-overflow)
	- [Base conversions](#base-conversions)
- [Bitmap](#bitmap)
	- [Not (aka flip)](#not-aka-flip)
	- [And](#and)
	- [Or](#or)
	- [Xor](#xor)
	- [Set](#set)
	- [Clear (or reset)](#clear-or-reset)
	- [Conditional not](#conditional-not)
	- [Conditional set](#conditional-set)
	- [Conditional clear](#conditional-clear)
	- [Population count](#population-count)
	- [Shift Left Logical 0](#shift-left-logical-0)
	- [Shift Left Logical 1](#shift-left-logical-1)
	- [Shift Left Arithmetic](#shift-left-arithmetic)
	- [Shift Right Logical 0](#shift-right-logical-0)
	- [Shift Right Logical 1](#shift-right-logical-1)
	- [Shift Right Arithmetic](#shift-right-arithmetic)
	- [Rotate (aka circular shift)](#rotate-aka-circular-shift)
	- [Reverse](#reverse)
	- [Count leading 0s](#count-leading-0s)
	- [Count leading 1s](#count-leading-1s)
	- [Count trailing 0s](#count-trailing-0s)
	- [Count trailing 1s](#count-trailing-1s)
- [Signed integers](#signed-integers)
	- [Sign bit](#sign-bit)
	- [1's complement](#1s-complement)
	- [2's complement](#2s-complement)
	- [Offset binary](#offset-binary)
	- [Zig Zag](#zig-zag)
- [Self-referential memory](#self-referential-memory)
	- [Linked list fundamentals](#linked-list-fundamentals)
	- [Dynamic memory allocation](#dynamic-memory-allocation)
- [Directed lists](#directed-lists)
- [Variable size array](#variable-size-array)
- [Variable size bitmap](#variable-size-bitmap)
- [Graphs](#graphs)
	- [Adjacency matrix](#adjacency-matrix)
	- [Adjacency lists](#adjacency-lists)
	- [Implicit/formulaic](#implicitformulaic)
		- [1D Lattice](#1d-lattice)
		- [2D Lattice](#2d-lattice)
		- [ND Lattice](#nd-lattice)
		- [Hash partitioning](#hash-partitioning)
		- [Rendezvous hashing](#rendezvous-hashing)
- [Dimensional packing](#dimensional-packing)
	- [Horizontal stripe](#horizontal-stripe)
	- [Horizontal striding (sub-stripe)](#horizontal-striding-sub-stripe)
	- [Vertical stripe](#vertical-stripe)
	- [Vertical striding (sub-stripe)](#vertical-striding-sub-stripe)
	- [Lower triangular, row-wise](#lower-triangular-row-wise)
	- [Upper triangular, col-wise](#upper-triangular-col-wise)
	- [Upper triangular, row-wise](#upper-triangular-row-wise)
	- [Lower triangular, col-wise](#lower-triangular-col-wise)
	- [Lower triangular, excluding diagonal, row-wise](#lower-triangular-excluding-diagonal-row-wise)
	- [Upper triangular, excluding diagonal, col-wise](#upper-triangular-excluding-diagonal-col-wise)
	- [Upper triangular, excluding diagonal, row-wise](#upper-triangular-excluding-diagonal-row-wise)
	- [Right-leaning diagonal (Hankel matrix)](#right-leaning-diagonal-hankel-matrix)
	- [Left-leaning diagonal (Toeplitz matrix)](#left-leaning-diagonal-toeplitz-matrix)
	- [Compressed sparse row (CSR)](#compressed-sparse-row-csr)
	- [Compressed sparse column (CSC)](#compressed-sparse-column-csc)
	- [yx Address concatenation](#yx-address-concatenation)
	- [xy Address concatenation](#xy-address-concatenation)
	- [Morton order](#morton-order)
	- [ND address concatenation](#nd-address-concatenation)
	- [Page directory](#page-directory)
	- [Sparse Array Bitmap](#sparse-array-bitmap)
	- [Locality Sensitive Hashing](#locality-sensitive-hashing)
- [Instructions](#instructions)
- [Execution state](#execution-state)
- [Instruction parallelism (SIMD)](#instruction-parallelism-simd)
- [Execution parallelism](#execution-parallelism)
	- [Multi-computer parallelism](#multi-computer-parallelism)
	- [Multi-process parallelism](#multi-process-parallelism)
	- [Multi-thread parallelism](#multi-thread-parallelism)
	- [Lockstep parallelism (SIMT)](#lockstep-parallelism-simt)
		- [Conditionals](#conditionals)
		- [Dynamic lockstep](#dynamic-lockstep)
- [SCRATCH](#scratch)
- [ENDSCRATCH](#endscratch)




# What is a computer?
1. Memory array
2. Sequence of memory modification instructions stored in memory
3. Address of the next instruction to execute

In short: memory, instructions, and execution state.

# Memory array
Think of condo/apartment mailboxes, addressable slots. Fixed number of slots. Each slot can contain a fixed size of something.
By numbering each slot, giving them a fixed address, we can identify and remember what was put where.

Mathematically, take a set of integer slots, `s = { 1, 2, ..., S }`.

Define a configurable function `m` that maps from `s` to the set of numbers. I.e. for each slot `i` in `s`, `m[i]` is the number we're storing in slot `i`.

Say with `S = 3`, we can store 3 numbers, change them via address, read them back, and the array will remember.
Initial state:
```
m[1] = 0
m[2] = 0
m[3] = 7
```
Change `m[2]` to 6.
Change `m[3]` to 12.
Then we can read the memory again and it will reflect the updated values:
```
m[1] = 0
m[2] = 6
m[3] = 12
```
The fundamental idea of computer memory is that it's a configurable number to number map.

## Memory cell
In modern computers, it's a voltage potential stored in a physical wire; either high or low. Mapping to a binary number 1 or 0. Digital logic latches and flip-flops are the basic circuitry constructs that allow for storing a single bit of memory, which can be either 0 or 1. Transistors sit on either side of the memory wire, controlling the storage and retrieval of the high or low voltage and letting it move to the next stage of the wire network.

By laying out a bunch of wires in parallel, we can combine those binary 0s and 1s into one logical unit, a binary number with more digits.

Let's break down the representation of a number, to explore how we can convert it to 0s and 1s.

Say we've got `234`. We can break it down per digit as:
```
234 = 2 * 100 + 3 * 10 + 4 * 1
```
More generally,
```
d = d_100 d_10 d_1 = d_100 * 100 + d_10 * 10 + d_1 * 1
d = d_10^2 d_10^1 d_10^0 = d_10^2 * 10^2 + d_10^1 * 10^1 + d_10^0 * 10^0
d = sum of d_10^k * 10^k, for k = 0, 1, 2, ...
```
We can let k go on as far as we want, and just assume there are implicit leading zeros so eventually it will converge.

We can generalize to arbitrary bases:
```
Binary (base 2): d = sum of d_2^k * 2^k, for k = 0, 1, 2, ...
Octal (base 8): d = sum of d_8^k * 8^k, for k = 0, 1, 2, ...
Hexadecimal (base 16): d = sum of d_16^k * 16^k, for k = 0, 1, 2, ...
Arbitrary base: d = sum of d_base^k * base^k, for k = 0, 1, 2, ...
    where base is any positive integer.
```
Computer hardware is generally base 2, because we can set up electronic circuits where the the voltage potential is either high (mapping to 1) or low (mapping to 0). There are some high density storage systems that use base 3 or 4 or more, by using intermediate levels in between max voltage and ground. 

## Modulo arithmetic
Computer memory is finite, so how do we deal with arbitrarily large numbers? We pick a maximum number of digits. And hence a maximum number we can represent.

With higher-level constructions, you can build arbitrarily large numbers based on smaller fixed-size numbers. See [Arbitrary size integers] section.

Say we pick base 10, and maximum number of digits is 3. Valid range of numbers is 0 (aka 000) up to 999.
```
d = sum of d_10^2 * 10^2 + d_10^1 * 10^1 + d_10^0 * 10^0
d = sum of d_10^k * 10^k, for k = 0, 1, 2.
```
Notice how k is bounded to be less than 3, our maximum number of digits. Now we have a finite set of numbers.

### Overflow
500 + 555 would be 1005, but that's not a valid number in our system.
The modulo comes in, which is that we divide what would be the result by the maximum number plus one, and take the remainder. So,
```
(500 + 555) mod 1000
1055 mod 1000
5 mod 1000
```
Because, 1055 / 1000 = 1 with remainder 55.
So in this kind of system, 500 + 555 = 5. It looks wrong, but in this modulo space it's the truth.
Since most integer math in most programming languages happens in some modulo space (usually 2^32 or 2^64 is the maximum number plus one), the `mod N+1` mathematical notation is dropped for convenience. But it's implicitly there in those languages.

### Underflow
Same ideas apply for subtraction, the inverse of addition.
```
(500 - 555) mod 1000
(-55) mod 1000
945 mod 1000
```
The modulus (x mod y) is defined as: there exists some integer multiple of y, call it q, such that x + q * y lies within [0, y). The result of the modulus is x + q * y. Note that q can be any positive, negative or zero integer, it just has to get us to the [0, y) range.
So if x is less than 0, add multiples of y until you put the result into [0, y) range. That's your result.
Else if x is greater than or equal to y, subtract multiples of y until you put the result in [0, y) range.
Otherwise x is already in the [0, y) range, and the result is x. You can think of it as q = 0.

### Multiplication

### Division

### Detecting overflow

## Base conversions
Given a sequence of digits in some given base, convert to another base.
```
// Converts base 2 strings: "0" up to "11...11" (length 64) into uint64_t form.
uint64_t u64FromBase2(string_view s) {
    PRECONDITION(1 <= s.size() && s.size() <= 64);
    uint64_t result = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        const auto c = s[i];
        PRECONDITION(c == '0' || c == '1');
        result |= (c == '1' ? 1ULL : 0ULL) << (s.size() - i - 1);
    }
    return result;
}
```
```
// Converts uint64_t into base 2 string form: "0" up to "11...11" (length 64)
string base2FromU64(uint64_t u) {
    // If u is zero, emit a single '0'.
    const size_t count = max(1ULL, 64ULL - countl_zero(u));
    string result;
    result.resize(count);
    for (size_t i = 0; i < count; ++i) {
        const auto bit = (u >> (count - i - 1)) & 1ULL;
        result[i] = (bit ? '1' : '0');
    }
    return result;
}
```

```
// Converts base 10 strings: "0" up to "18446744073709551616" (2^64 - 1) into uint64_t form.
uint64_t u64FromBase10(string_view s) {
    PRECONDITION(1 <= s.size());
    uint64_t result = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        const uint64_t c = s[i];
        PRECONDITION('0' <= c && c <= '9');
        const auto digit = c - '0';
        const auto result10 = result * 10;
        PRECONDITION(result10 >= result); // No overflow.
        const auto result10plusDigit = result10 + digit;
        PRECONDITION(result10plusDigit >= result10); // No overflow.
        result = result10plusDigit;
    }
    return result;
}
```
```
// Converts uint64_t into base 10 string form: "0" up to "18446744073709551616" (2^64 - 1)
string base10FromU64(uint64_t u) {
    if (u == 0) return "0";
    string result;
    while (u > 0) {
        const auto digit = u % 10;
        u /= 10;
        result += (char)(digit + '0');
    }
    reverse(begin(result), end(result));
    return result;
}
```

```
uint64_t u64FromBase16(string_view s) {
    PRECONDITION(1 <= s.size() && s.size() <= 16);
    uint64_t result = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        const uint64_t c = s[i];
        uint64_t digit = 0;
        if ('0' <= c && c <= '9') {
            digit = c - '0';
        }
        else if ('a' <= c && c <= 'f') {
            digit = c - 'a' + 10;
        }
        else if ('A' <= c && c <= 'F') {
            digit = c - 'A' + 10;
        }
        else {
            PRECONDITION(false); // Invalid character.
        }
        result |= digit << (s.size() - i - 1) * 4;
    }
    return result;
}
```
```
string base16FromU64(uint64_t u) {
    // If u is zero, emit a single '0'.
    const size_t count = max(1ULL, 16ULL - countl_zero(u) / 4);
    string result;
    result.resize(count);
    for (size_t i = 0; i < count; ++i) {
        const auto digit = (u >> ((count - i - 1) * 4)) & 0xF;
        result[i] = (char)(digit + ((digit < 10) ? '0' : 'A' - 10));
    }
    return result;
}
```

# Bitmap
You can use a single bit to represent True/False, and densely pack them together. A bitmap is just an array of booleans, densely packed. Setting, clearing, flipping bits all turn into efficient bit operations.

In digital logic design, flip-flops get stamped together in arrays to form a bitmap of a fixed size. Like `uint32` meaning 32 bits packed together, or `uint64` meaning 64 bits. These are exposed to software as `registers`, and to programming languages as fixed-width integer types, like `uint32`, `uint64`, etc.

All of the following bit-wise operations work in parallel on all bits in a bitmap. This is just more efficient to build in hardware; if you need to route one circuit around a chip, may as well lay down a bunch of parallel cables and build parallel circuits all taking the same route.

Say we had a bitmap of length 4, holding 4 bits.
```
and(0110,1010) = and(0,1) and(1,0) and(1,1) and(0,0) = 0010
```
That is, the operation is defined bit-wise across the bitmaps.
```
and({...a_i...},{...b_i...}) = and(a_1,b_1) and(a_2,b_2) ... and(a_K,b_K)
```
where K is the maximum number of binary digits.

We can trivially map all of the single-bit operations to operate on bitmaps in this same way. The only caveat is that for operations that take multiple bitmaps, we assume they all have the same number of bits. So `and(uint8,uint64)` doesn't make sense; which 8 bits of the 64 are we applying the `and` to?
Some highly specialized bit operations do define what should happen, but those usually aren't exposed at a programming language level.

With a fixed number of bits, there are some operations that do work across bits, not in an entirely parallel fashion.

## Not (aka flip)
```
not(0) = 1
not(1) = 0
```
Single bit version:
```
uint1_t not(uint1_t a) {
    return ~a;
}
```
Multi bit version:
```
uint32_t not(uint32_t a) {
    return ~a;
}
```

## And
```
and(0,0) = 0
and(0,1) = 0
and(1,0) = 0
and(1,1) = 1
```
Single bit version:
```
uint1_t and(uint1_t a, uint1_t b) {
    return a & b;
}
```
Multi bit version:
```
uint32_t and(uint32_t a, uint32_t b) {
    return a & b;
}
```

## Or
```
or(0,0) = 0
or(0,1) = 1
or(1,0) = 1
or(1,1) = 1
```
Single bit version:
```
uint1_t or(uint1_t a, uint1_t b) {
    return a | b;
}
```
Multi bit version:
```
uint32_t or(uint32_t a, uint32_t b) {
    return a | b;
}
```

## Xor
```
xor(0,0) = 0
xor(0,1) = 1
xor(1,0) = 1
xor(1,1) = 0
```
Single bit version:
```
uint1_t xor(uint1_t a, uint1_t b) {
    return a ^ b;
}
```
Multi bit version:
```
uint32_t xor(uint32_t a, uint32_t b) {
    return a ^ b;
}
```

## Set
By 'set' most programmers mean 'set to 1' in a base 2 context. This is a trivial set to 1. Single bit version:
```
void set(uint1_t& a) {
    a = 1;
}
```
Multi bit version:
```
void set(uint32_t& a) {
    a = numeric_limits<uint32_t>::max();
}
```

## Clear (or reset)
By 'clear' or 'reset', most programmers mean 'set to 0' in a base 2 context. This is a trivial set to 0. Single bit version:
```
void clear(uint1_t& a) {
    a = 0;
}
```
Multi bit version:
```
void clear(uint32_t& a) {
    a = 0;
}
```

## Conditional not
`cnot(a,condition)` means: if condition is true, flip a. Otherwise return a.
```
cnot(0,0) = 0
cnot(0,1) = 1
cnot(1,0) = 1
cnot(1,1) = 0
```
Note this is the same as `xor(a,condition)`. Single bit version:
```
uint1_t cnot(uint1_t a, uint1_t condition) {
    return a ^ condition;
}
```
Multi bit version:
```
uint32_t cnot(uint32_t a, uint32_t condition) {
    return a ^ condition;
}
```

## Conditional set
`cset(a,condition)` means: If condition is 0, we want to leave a alone. Else condition is 1, we're setting a to 1. Single bit version:
```
uint1_t cset(uint1_t a, uint1_t condition) {
    return a | condition;
}
```
Multi bit version:
```
uint32_t cset(uint32_t a, uint32_t condition) {
    return a | condition;
}
```

## Conditional clear
`cclear(a,condition)` means: If condition is 0, we want to leave a alone. Else condition is 1, we're setting a to 0. Single bit version:
```
uint1_t cclear(uint1_t a, uint1_t condition) {
    return a & ~condition;
}
```
Multi bit version:
```
uint32_t cclear(uint32_t a, uint32_t condition) {
    return a & ~condition;
}
```

## Population count
Counts the number of bits set to 1 in the bitmap.
```
result = popcount(b)
```

## Shift Left Logical 0
Shifts the bits by a specified number of bits, adding in zeros (or ones) as needed to fill the rest.
Once you run up against the limits of the bitmap size, bits shift off the end. Say we're in uint8.
```
shiftLeftLogical0(1111,0) =     1111
shiftLeftLogical0(1111,1) =    11110
shiftLeftLogical0(1111,2) =   111100
shiftLeftLogical0(1111,3) =  1111000
shiftLeftLogical0(1111,4) = 11110000
shiftLeftLogical0(1111,5) = 11100000
shiftLeftLogical0(1111,6) = 11000000
shiftLeftLogical0(1111,7) = 10000000
shiftLeftLogical0(1111,8) = 00000000
```
Any higher shift values give 0.
```
uint32_t shiftLeftLogical0(uint32_t a, uint32_t b) {
    return (a << b);
}
```

## Shift Left Logical 1
Same concept as for Left Logical 0, but shifts in 1 bits instead of 0 bits.
```
shiftLeftLogical1(1111,0) =     1111
shiftLeftLogical1(1111,1) =    11111
shiftLeftLogical1(1111,2) =   111111
shiftLeftLogical1(1111,3) =  1111111
shiftLeftLogical1(1111,4) = 11111111
shiftLeftLogical1(1111,5) = 11111111
shiftLeftLogical1(1111,6) = 11111111
shiftLeftLogical1(1111,7) = 11111111
shiftLeftLogical1(1111,8) = 11111111
```
Generally there's no intrinsic for this.
```
uint32_t shiftLeftLogical1(uint32_t a, uint32_t b) {
    if (b >= 32) return numeric_limits<uint32_t>::max();
    const uint32_t mask = (1u << b) - 1;
    return (a << b) | mask;
}
```

## Shift Left Arithmetic
Same concept as for Left Logical 0/1, but shifts in a bit that matches the least significant bit.
```
shiftLeftArithmetic(111x,0) =     111x
shiftLeftArithmetic(111x,1) =    111xx
shiftLeftArithmetic(111x,2) =   111xxx
shiftLeftArithmetic(111x,3) =  111xxxx
shiftLeftArithmetic(111x,4) = 111xxxxx
shiftLeftArithmetic(111x,5) = 11xxxxxx
shiftLeftArithmetic(111x,6) = 1xxxxxxx
shiftLeftArithmetic(111x,7) = xxxxxxxx
shiftLeftArithmetic(111x,8) = xxxxxxxx
```
Where `x` is either `0` or `1`. Generally there's no intrinsic for this.
```
uint32_t shiftLeftArithmetic(uint32_t a, uint32_t b) {
    const uint32_t bit = a & 0b1;
    const uint32_t mask = (bit << b) - 1;
    if (b >= 32) return (mask << 1) | bit;
    return (a << b) | mask;
}
```

## Shift Right Logical 0
Same concept as for Left Logical 0, but shifts in the other direction.
```
shiftRightLogical0(11110000,0) = 11110000
shiftRightLogical0(11110000,1) = 01111000
shiftRightLogical0(11110000,2) = 00111100
shiftRightLogical0(11110000,3) = 00011110
shiftRightLogical0(11110000,4) = 00001111
shiftRightLogical0(11110000,5) = 00000111
shiftRightLogical0(11110000,6) = 00000011
shiftRightLogical0(11110000,7) = 00000001
shiftRightLogical0(11110000,8) = 00000000
```
Any higher shift values give 0.
```
uint32_t shiftRightLogical0(uint32_t a, uint32_t b) {
    return (a >> b);
}
```

## Shift Right Logical 1
Same concept as for Right Logical 0, but shifts in 1 bits instead of 0 bits.
```
shiftRightLogical1(01110000,0) = 01110000
shiftRightLogical1(01110000,1) = 10111000
shiftRightLogical1(01110000,2) = 11011100
shiftRightLogical1(01110000,3) = 11101110
shiftRightLogical1(01110000,4) = 11110111
shiftRightLogical1(01110000,5) = 11111011
shiftRightLogical1(01110000,6) = 11111101
shiftRightLogical1(01110000,7) = 11111110
shiftRightLogical1(01110000,8) = 11111111
```
Generally there's no intrinsic for this.
```
uint32_t shiftRightLogical1(uint32_t a, uint32_t b) {
    if (b >= numeric_limits<uint32_t>::digits) return numeric_limits<uint32_t>::max();
    const uint32_t mask = (1u << b) - 1;
    const uint32_t upper = mask << (numeric_limits<uint32_t>::digits - b);
    return upper | (a >> b);
}
```

## Shift Right Arithmetic
Same concept as for Right Logical 0/1, but shifts in a bit that matches the most significant bit.
```
shiftRightArithmetic(x1110000,0) = x1110000
shiftRightArithmetic(x1110000,1) = xx111000
shiftRightArithmetic(x1110000,2) = xxx11100
shiftRightArithmetic(x1110000,3) = xxxx1110
shiftRightArithmetic(x1110000,4) = xxxxx111
shiftRightArithmetic(x1110000,5) = xxxxxx11
shiftRightArithmetic(x1110000,6) = xxxxxxx1
shiftRightArithmetic(x1110000,7) = xxxxxxxx
shiftRightArithmetic(x1110000,8) = xxxxxxxx
```
Where `x` is either `0` or `1`. Generally, signed integer right shift will do this by default. For unsigned integers, generally there's no intrinsic.
```
int32_t shiftRightArithmetic(int32_t a, int32_t b) {
    return (a >> b);
}
uint32_t shiftRightArithmetic(uint32_t a, uint32_t b) {
    if (b >= numeric_limits<uint32_t>::digits) return numeric_limits<uint32_t>::max();
    const uint32_t bit = (a >> (numeric_limits<uint32_t>::digits - 1)) & 0b1;
    const uint32_t upper = mask << (numeric_limits<uint32_t>::digits - b);
    return upper | (a >> b);
}
```

## Rotate (aka circular shift)
Rotates the bits by a specified number of bits.
```
rotateLeft(1100,0) = 1100
rotateLeft(1100,1) = 1001
rotateLeft(1100,2) = 0011
rotateLeft(1100,3) = 0110
rotateLeft(1100,4) = 1100
```
...the cycling continues.

Intrinsic:
```
result = rotl(a,count)
```
For uint64, it would look like:
```
uint64_t rotl(uint64_t a, uint64_t count) {
    count = count % 64;
    return count == 0 ? a : (a << count) | (a >> (64 - count));
}
```

```
rotateRight(1100,0) = 1100
rotateRight(1100,1) = 0110
rotateRight(1100,2) = 0011
rotateRight(1100,3) = 1001
rotateRight(1100,4) = 1100
```
...the cycling continues.

Intrinsic:
```
		result = rotr(a,count)
```
For uint64, it would look like:
```
uint64_t rotr(uint64_t a, uint64_t count) {
    count = count % 64;
    return count == 0 ? a : (a >> count) | (a << (64 - count));
}
```

## Reverse
Reverses the order of bits.
```
result[i] = b[b.size()-1 - i] for all i in [0, K).
```
There's not usually a built-in intrinsic to do this. You can implement it as a swap network.
```
abcd
cdab, swap first half and second half
dcba, swap within each half
```
For uint8, it would look like:
```
uint8_t reverse(uint8_t b) {
    b = (b & 0b11110000) >> 4 | (b & 0b00001111) << 4;
    b = (b & 0b11001100) >> 2 | (b & 0b00110011) << 2;
    b = (b & 0b10101010) >> 1 | (b & 0b01010101) << 1;
    return b;
}
```
You can trivially extended further to larger bitmap size.

## Count leading 0s
## Count leading 1s
## Count trailing 0s
## Count trailing 1s

# Signed integers
So far we've only considered non-negative numbers. I.e. zero and all positives. But what about negative numbers?

There are many different encodings, all with various trade-offs. Modern computer hardware has mostly stabilized around 2's complement for most purposes. 

## Sign bit
Take a bitmap, choose one bit to represent + or -, and leave the rest for representing the value.

Say we have a 3-bit bitmap:
```
000 0
001 1
010 2
011 3
100 -0
101 -1
110 -2
111 -3
```
There's two things to notice here:
    Mathematically, 0 = -0, but they have different bitmap representations. This complicates equality checks.
    Ordering is strange: it increments positively until 3, where it jumps down and then increments negatively.
```
void deconstructSignedInteger(uint64_t bitmap, uint64_t& u, bool& positive) {
    if (bitmap & (1ULL << 63)) {
        positive = true;
        u = bitmap;
    }
    else {
        positive = false;
        u = (bitmap & ~(1ULL << 63));
    }
}
```

## 1's complement
Take a bitmap, choose the highest bit to signal + or -, and flip negative bitmaps to recover the value.
```
000 0
001 1
010 2
011 3
100 011 -3
101 010 -2
110 001 -1
111 000 -0
```
This also has the 0 = -0 but not in bitmap form issue. Note there's a discontinuity jumping from 3 to -3, but otherwise the ordering is always increasing. This makes ordering slightly easier.
```
void deconstructSignedInteger(uint64_t bitmap, uint64_t& u, bool& positive) {
    if (bitmap & (1ULL << 63)) {
        positive = true;
        u = bitmap;
    }
    else {
        positive = false;
        u = ~bitmap;
    }
}
```

## 2's complement
Choose the highest bit to signal + or -, flip negative bitmaps and add one to recover the value.
```
000 0
001 1
010 2
011 3
100 011 100 -4
101 010 011 -3
110 001 010 -2
111 000 001 -1
```
Notice this representation doesn't have two representations of mathematical zero. It also has only the single jump discontinuity, but otherwise maintains an increasing order.
```
void deconstructSignedInteger(uint64_t bitmap, uint64_t& u, bool& positive) {
    if (bitmap & (1ULL << 63)) {
        positive = true;
        u = bitmap;
    }
    else {
        positive = false;
        u = ~bitmap + 1ULL;
    }
}
```

## Offset binary
Eliminates the jump discontinuity.
```
000 -4
001 -3
010 -2
011 -1
100 0
101 1
110 2
111 3
```
```
void deconstructSignedInteger(uint64_t bitmap, uint64_t& u, bool& positive) {
    if (bitmap & (1ULL << 63)) {
        positive = true;
        u = bitmap & ~(1ULL << 63);
    }
    else {
        positive = false;
        u = (~bitmap) & ~(1ULL << 63) + 1ULL;
    }
}
```

## Zig Zag
Optimizes the number of bits required for small absolute values. Uses the least significant bit as an indicator of + or -.
```
000  0
001 -1
010  1
011 -2
100  2
101 -3
110  3
111 -4
```
```
void deconstructSignedInteger(uint64_t bitmap, uint64_t& u, bool& positive) {
    if (bitmap & 1ULL) {
        positive = false;
        u = (bitmap >> 1) + 1;
    }
    else {
        positive = true;
        u = bitmap >> 1;
    }
}
```

# Self-referential memory
Recall that memory is a configurable number to number map. The reason to use numbers on both sides is so we can store references to memory slots within the slots themselves.
So with one fixed size array of numbers, we can store sets of numbers, as well as arbitrary links between numbers.
It's this arbitrary link storage mechanism that makes memory actually powerful.

## Linked list fundamentals
TODO

This extends indefinitely; we can store arbitrarily-nested maps of maps of maps of ... maps to numbers. This combinatorial power is why we can encode really complicated things in a single set of numbers.

## Dynamic memory allocation
Every computer has a ceiling to the amount of memory onboard. That is, the memory array has a fixed upper limit. In order to write software programs that can run on various computers with different amounts of memory, we need a dynamic addressing scheme.

The other major constraint is that very often the input to a computer program is user-generated (or at least influenced), and so it will be variable-length. Meaning, we need an addressing scheme that adjusts for variable-length things we want to store.

TODO: stack top, partitioning a fixed-size array into two intervals: variable data, and empty space.
```
{variable1, ..., variableN, empty1, ..., emptyE}
```
Note we can run out of space if the variable data grows too long.
Also note that for other things we want to store in memory, now the problem is worse. Instead of trying to come up with address schemes for a fixed size array, now we have to generate address schemes for the empty space interval, which is now variable size.

If our only other storage requirement is one more variable-length data, we could reverse the order and have a symmetric stack for the second variable-length data. That way both variable-length data streams can shrink/grow, and we'll avoid overlaps unless we're really out of space.
```
{variable1, ..., variableN, empty1, ..., emptyE, varSecondM, ..., varSecond1}
```
However, notice that if we have 3 or more variable-length data streams to store, now we've got issues. We'll need to consistently move interior data streams around to make room as the first and second variable data changes length, or choose some other strategy. Like a variable list storing addresses of other variable streams.

The key thing to notice is that we need some kind of indirection layer here to manage addressing of variable-length data. Effectively, turn the variable-length stream into a fixed-length reference, so the variable-length stream can be managed by a memory subsystem. Dynamic memory allocation does precisely this.

The standardized interface for managing variable-length data storage is an acquire/release pair. Also known as allocate/free, new/delete, etc.
```
T* allocate(size_t count);
void free(T* reference);
```
The key thing here is that allocation can fail, due to running out of memory. We're requesting some dynamic size, and only when there's sufficient room will we get an interval of writeable memory back.

# Directed lists
Also known as singly-linked lists

# Variable size array
Also known as: table, variable length array, vector
```
template<typename T> struct Array
{
  T* mem = nullptr;
  size_t len = 0; // # of elements in mem.

  Array() = default;
  Array(size_t lenInitial) {
    mem = new T[lenInitial];
    len = lenInitial;
  }
  Array(size_t lenInitial, const T& valueInitial) {
    mem = new T[lenInitial];
    for (size_t i = 0; i < lenInitial; ++i)
      mem[i] = valueInitial;
    len = lenInitial;
  }
  ~Array() noexcept {
    if (mem)
      delete [] mem;
    mem = nullptr;
    len = 0;
  }
  __forceinline size_t size() const noexcept { return len; }
  __forceinline bool empty() const noexcept { return len == 0; }
  __forceinline void resize(size_t lenNew) {
    if (len == lenNew)
      return;
    auto memNew = new T[lenNew];
    const auto lenCopy = min<size_t>(len, lenNew);
    for (size_t i = 0; i < lenCopy; ++i)
      memNew[i] = std::move(mem[i]);
    delete [] mem;
    mem = memNew;
    len = lenNew;
  }
  __forceinline const T& operator[](size_t i) const {
    assert(i < len);
    return mem[i];
  }
  __forceinline T& operator[](size_t i) {
    assert(i < len);
    return mem[i];
  }
};
```

# Variable size bitmap
```
struct Bitmap
{
	vector<uint64_t> m_v;
	size_t m_cBits = 0;

	__forceinline size_t size() const noexcept { return m_cBits; }
	__forceinline bool empty() const noexcept { return m_cBits == 0; }
	__forceinline void clear() noexcept
	{
		m_cBits = 0;
		m_v.clear();
	}
	__forceinline void resize(size_t cBits)
	{
		m_cBits = cBits;
		m_v.resize((cBits + 63) / 64);

		// Reset the trailing bits in the last word.
		if (cBits)
		{
			const size_t j = cBits - 1;
			const size_t j64 = j / 64;
			const size_t jbit = j % 64;
			const uint64_t jmask = jbit == 63 ? ~0ULL : (1ULL << (jbit + 1)) - 1;
			m_v[j64] &= jmask;
		}
	}
	__forceinline bool get(size_t i) const noexcept
	{
		VEC(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		return (m_v[i64] & (1ULL << ibit)) != 0;
	}
	__forceinline void set(size_t i, bool f) noexcept
	{
		VEC(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		if (f)
			m_v[i64] |= (1ULL << ibit);
		else
			m_v[i64] &= ~(1ULL << ibit);
	}
	__forceinline void set(size_t i) noexcept
	{
		VEC(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		m_v[i64] |= (1ULL << ibit);
	}
	__forceinline void reset(size_t i) noexcept
	{
		VEC(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		m_v[i64] &= ~(1ULL << ibit);
	}
	// Sets the range [i, j] to 1.
	__forceinline void setRange(size_t i, size_t j) noexcept
	{
		VEC(i <= j);
		VEC(j < m_cBits);
		const size_t i64 = i / 64;
		const size_t j64 = j / 64;
		const size_t ibit = i % 64;
		const size_t jbit = j % 64;
		const uint64_t imask = ((1ULL << ibit) - 1);
		const uint64_t jmask = jbit == 63 ? ~0ULL : (1ULL << (jbit + 1)) - 1;
		if (i64 == j64)
		{
			// All bits are in the same 64-bit word.
			const uint64_t mask = imask ^ jmask;
			m_v[i64] |= mask;
		}
		else
		{
			// Set the bits in the first word.
			m_v[i64] |= ~imask;
			// Set the bits in the middle words.
			for (size_t k = i64 + 1; k < j64; ++k)
				m_v[k] = ~0ULL;
			// Set the bits in the last word.
			m_v[j64] |= jmask;
		}
	}
	// Resets the range [i, j] to 0.
	__forceinline void resetRange(size_t i, size_t j) noexcept
	{
		VEC(i <= j);
		VEC(j < m_cBits);
		const size_t i64 = i / 64;
		const size_t j64 = j / 64;
		const size_t ibit = i % 64;
		const size_t jbit = j % 64;
		const uint64_t imask = ((1ULL << ibit) - 1);
		const uint64_t jmask = jbit == 63 ? ~0ULL : (1ULL << (jbit + 1)) - 1;
		if (i64 == j64)
		{
			// All bits are in the same 64-bit word.
			const uint64_t mask = imask ^ jmask;
			m_v[i64] &= ~mask;
		}
		else
		{
			// Reset the bits in the first word.
			m_v[i64] &= imask;
			// Reset the bits in the middle words.
			for (size_t k = i64 + 1; k < j64; ++k)
				m_v[k] = 0ULL;
			// Reset the bits in the last word.
			m_v[j64] &= ~jmask;
		}
	}
	// Returns the number of bits set to 1 in the range [i, j].
	__forceinline size_t popcount(size_t i, size_t j) const noexcept
	{
		VEC(i <= j);
		VEC(j < m_cBits);
		const size_t i64 = i / 64;
		const size_t j64 = j / 64;
		const size_t ibit = i % 64;
		const size_t jbit = j % 64;
		const uint64_t imask = ((1ULL << ibit) - 1);
		const uint64_t jmask = jbit == 63 ? ~0ULL : (1ULL << (jbit + 1)) - 1;
		size_t count = 0;
		if (i64 == j64)
		{
			// All bits are in the same 64-bit word.
			const uint64_t mask = imask ^ jmask;
			count += std::popcount(m_v[i64] & mask);
		}
		else
		{
			// Count the bits in the first word.
			count += std::popcount(m_v[i64] & ~imask);
			// Count the bits in the middle words.
			for (size_t k = i64 + 1; k < j64; ++k)
				count += std::popcount(m_v[k]);
			// Count the bits in the last word.
			count += std::popcount(m_v[j64] & jmask);
		}
		return count;
	}
	// Appends the contents of another bitmap to this one.
	__forceinline void append(const Bitmap& o)
	{
		if (o.empty())
			return;
		if (empty())
		{
			*this = o;
			return;
		}
		const size_t cold = m_cBits / 64;
		const size_t cshift = (m_cBits % 64);
		if (!cshift)
		{
			m_v.insert(end(m_v), begin(o.m_v), end(o.m_v));
			m_cBits += o.m_cBits;
			return;
		}
		const size_t calign = 64 - (m_cBits % 64);
		const size_t mask = calign == 64 ? ~0ULL : ((1ULL << calign) - 1);
		m_v.resize((m_cBits + o.m_cBits + 63) / 64);
		m_cBits += o.m_cBits;
		// Handle all complete words from source
		size_t i = 0;
		for (; i < o.m_v.size() - 1; ++i)
		{
			m_v[cold + i] |= (o.m_v[i] & mask) << cshift;
			m_v[cold + i + 1] |= (o.m_v[i] >> calign);
		}
		// Handle the last word specially to respect o.m_cBits
		const size_t crem = o.m_cBits % 64;
		const uint64_t lastMask = crem == 0 ? ~0ULL : ((1ULL << crem) - 1);
		const uint64_t lastWord = o.m_v[i] & lastMask;
		m_v[cold + i] |= (lastWord & mask) << cshift;
		if ((cold + i + 1) < m_v.size())
			m_v[cold + i + 1] |= (lastWord >> calign);
	}
	__forceinline void emplace_back(bool f)
	{
		const size_t i = m_cBits++;
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		if (i64 == m_v.size())
		{
			m_v.push_back(0);
		}
		m_v[i64] |= (size_t)f << ibit;
	}

	Bitmap() = default;
	Bitmap(size_t cBits) : m_cBits(cBits)
	{
		m_v.resize((cBits + 63) / 64);
	}
	~Bitmap() noexcept = default;
	Bitmap(const Bitmap& o) : m_v(o.m_v), m_cBits(o.m_cBits) {}
	Bitmap& operator=(const Bitmap& o)
	{
		m_v = o.m_v;
		m_cBits = o.m_cBits;
		return *this;
	}
	Bitmap(Bitmap&& o) noexcept
	{
		// clang-format off
		m_v = std::move(o.m_v); o.m_v.clear();
		m_cBits = std::move(o.m_cBits); o.m_cBits = 0;
		// clang-format on
	}
	Bitmap& operator=(Bitmap&& o) noexcept
	{
		// clang-format off
		m_v = std::move(o.m_v); o.m_v.clear();
		m_cBits = std::move(o.m_cBits); o.m_cBits = 0;
		// clang-format on
		return *this;
	}
};

static void VerifyEqual(const Bitmap& b, const vector<bool>& v)
{
	VEC(b.size() == v.size());
	for (size_t i = 0; i < b.size(); ++i)
	{
		VEC(b.get(i) == v[i]);
	}

	if (b.size())
	{
		const size_t cB = b.popcount(0, b.size() - 1);
		size_t cV = 0;
		for (size_t i = 0; i < v.size(); ++i)
			cV += (size_t)v[i];
		VEC(cB == cV);
	}
}
static void TestBitmap()
{
	Bitmap b;
	vector<bool> v;
	minstd_rand gen(1234);
	uniform_int_distribution<size_t> dist;
	function<void()> rgfn[] = {
		[&]()
		{
			b.clear();
			v.clear();
			VerifyEqual(b, v);
		},
		[&]()
		{
			const size_t cBits = dist(gen) % 1000;
			b.resize(cBits);
			v.resize(cBits);
			VerifyEqual(b, v);
		},
		[&]()
		{
			if (b.empty())
				return;
			const size_t i = dist(gen) % b.size();
			const size_t j = dist(gen) % b.size();
			b.set(i, true);
			b.set(j, false);
			v[i] = true;
			v[j] = false;
			VerifyEqual(b, v);
		},
		[&]()
		{
			if (b.empty())
				return;
			const size_t i = dist(gen) % b.size();
			const size_t j = dist(gen) % b.size();
			b.set(i);
			b.reset(j);
			v[i] = true;
			v[j] = false;
			VerifyEqual(b, v);
		},
		[&]()
		{
			if (b.empty())
				return;
			const size_t i = dist(gen) % b.size();
			const size_t j = dist(gen) % b.size();
			const size_t s = min<size_t>(i, j);
			const size_t t = max<size_t>(i, j);
			b.setRange(s, t);
			for (size_t k = s; k <= t; ++k)
				v[k] = true;
			VerifyEqual(b, v);
		},
		[&]()
		{
			if (b.empty())
				return;
			const size_t i = dist(gen) % b.size();
			const size_t j = dist(gen) % b.size();
			const size_t s = min<size_t>(i, j);
			const size_t t = max<size_t>(i, j);
			b.resetRange(s, t);
			for (size_t k = s; k <= t; ++k)
				v[k] = false;
			VerifyEqual(b, v);
		},
		[&]()
		{
			if (b.empty())
				return;
			const size_t i = dist(gen) % b.size();
			const size_t j = dist(gen) % b.size();
			const size_t s = min<size_t>(i, j);
			const size_t t = max<size_t>(i, j);
			const size_t cB = b.popcount(s, t);
			size_t cV = 0;
			for (size_t k = s; k <= t; ++k)
				cV += (size_t)v[k];
			VEC(cB == cV);
		},
		[&]()
		{
			const size_t cAppend = dist(gen) % 1000;
			Bitmap n { cAppend };
			vector<bool> m(cAppend);
			for (size_t i = 0; i < cAppend; ++i)
			{
				const bool f = dist(gen) % 2;
				n.set(i, f);
				m[i] = f;
			}
			VerifyEqual(n, m);
			b.append(n);
			v.insert(end(v), begin(m), end(m));
			VerifyEqual(b, v);
		},
		[&]()
		{
			const size_t cAppend = dist(gen) % 1000;
			for (size_t i = 0; i < cAppend; ++i)
			{
				const bool f = dist(gen) % 2;
				b.emplace_back(f);
				v.push_back(f);
			}
			VerifyEqual(b, v);
		},
	};
	for (size_t i = 0; i < 10000; ++i)
	{
		const size_t j = dist(gen) % (sizeof(rgfn) / sizeof(rgfn[0]));
		const auto& fn = rgfn[j];
		fn();
	}
}
```







# Graphs
Families of connectivity/edge implementations:

## Adjacency matrix
Index the nodes in the graph as `{ 0, ..., N-1 }`. Then define a matrix `E(i,j)` which stores information about the edge from node `i` to `j`. Note that `E` is a square matrix, with forward direction edges stored in the lower triangle `i < j`, reverse direction edges stored in the upper triangle `i > j`, and self-loop edges stored in the diagonal `i == j`.

If the graph is an undirected graph, note that the matrix would be symmetric, `E(i,j) == E(j,i)`, because there's no distinction between forward/reverse edges. So for storage efficientcy we can simplify this down to a triangular matrix, only storing `E(i,j)` for `i <= j`. Access becomes `E(min(i,j), max(i,j))`. For efficient 1D storage, see [Lower triangular, row-wise](#lower-triangular-row-wise).

If the graph is undirected and also doesn't allow self-loops, we can eliminate the diagonal as well. Effectively only storing `E(i,j)` for `i < j`. For efficient 1D storage, see [Lower triangular, excluding diagonal, row-wise](#lower-triangular-excluding-diagonal-row-wise)

## Adjacency lists

## Implicit/formulaic
Per-node, we can use implicit knowledge to deduce which other nodes are connected. The most common type is a lattice, or as it's known in 2D, a grid.

The advantage of these kinds of connectivity is that memory usage can be significantly reduced, due to not needing memory to store connectivity. In cases where there's no need for other long-term per-edge data, this can be a nice win. However, note that a lot of scenarios require per-edge data anyways (e.g. max flow, min-cost path), so it may not matter a whole lot.

### 1D Lattice
Say we have nodes `{ 0 1 ... N-1 }` and each node is connected to its immediate neighbors. Note the endpoint nodes have degree 1, whereas all the interior nodes have degree 2. If you instead extended this to the infinite set of integers, you'd have an infinite 1D lattice graph.
```
vector<uint32_t> Adjacents(uint32_t cNodes, uint32_t node) {
    assert(cNodes > 0);
    if (cNodes == 1) return {}; // One node in graph, no neighbors.
    if (node == 0) return {1}; // Left endpoint
    if (node == N-1) return {N-2}; // Right endpoint
    return {node-1, node+1}; // Interior node.
}
```

### 2D Lattice
`X by Y` grid of nodes. Connectivity might be Manhattan style (interior nodes connect to 4 neighbors in cardinal directions), full (interior nodes connect to 4 cardinal neighbors plus 4 diagonal neighbors), or some other kind of arrangement. It may even be per-node dependent, as long as we can quickly look up the arrangement for a given node.
```
vector<pair<uint32_t, uint32_t>> AdjacentsManhattan(uint32_t X, uint32_t Y, uint32_t x, uint32_t y) {
    assert(X > 0 and Y > 0);
    vector<pair<uint32_t, uint32_t>> result;
    if (x > 0) result.emplace_back(x-1, y);
    if (x + 1 < X) result.emplace_back(x+1, y);
    if (y > 0) result.emplace_back(x, y-1);
    if (y + 1 < Y) result.emplace_back(x, y+1);
    return result;
}
```
```
vector<pair<uint32_t, uint32_t>> AdjacentsFull(uint32_t X, uint32_t Y, uint32_t x, uint32_t y) {
    assert(X > 0 and Y > 0);
    vector<pair<uint32_t, uint32_t>> result;
    const bool xL = x > 0;
    const bool yL = y > 0;
    const bool xR = x + 1 < X;
    const bool yR = y + 1 < Y;
    if (xL) {
        if (yL) result.emplace_back(x-1, y-1);
        result.emplace_back(x-1, y);
        if (yR) result.emplace_back(x-1, y+1);
    }
    {
        if (yL) result.emplace_back(x, y-1);
        result.emplace_back(x, y);
        if (yR) result.emplace_back(x, y+1);
    }
    if (xR) {
        if (yL) result.emplace_back(x+1, y-1);
        result.emplace_back(x+1, y);
        if (yR) result.emplace_back(x+1, y+1);
    }
    return result;
}
```

### ND Lattice
Given we have `D` dimensions, each with extent `N[d]`, we have a `N[0] x N[1] x ... x N[D-1]` lattice of nodes. Again, connectivity might be Manhattan style, or some other scheme.
```
template<size_t D> vector<array<uint32_t, D>> AdjacentsManhattan(array<uint32_t, D> N, array<uint32_t, D> point) {
    vector<array<uint32_t, D>> result;
    for (size_t d = 0; d < D; ++d) {
        if (point[d] > 0) {
            array<uint32_t, D> left = point;
            --left[d];
            result.emplace_back(move(left));
        }
        if (point[d] + 1 < N[d]) {
            array<uint32_t, D> right = point;
            ++right[d];
            result.emplace_back(move(right));
        }
    }
    return result;
}
```
Note that lattice Manhattan connectivity like this goes up linearly, as a function of the number of dimensions `D`. 1D lattice nodes can have at most 2 neighbors; 2D lattice nodes can have at most 4, 3D lattice nodes can have at most 6, ..., ND lattice nodes can have 2D neighbors.

### Hash partitioning
The idea is to hash the node, and use the hash code to choose a partition neighborhood. Within the neighborhood, we can use some other scheme to determine adjacency.

### Rendezvous hashing
The idea is to define a hash combinator function, `h(node1, node2)`, and choose the top K hash codes among all `node2` for a given `node1`. By using a min- or max-heap, this has a time cost of `O(K log N)` per `node1`.

# Dimensional packing
Memory is fundamentally 1-dimensional. I.e. the set of slots `S = { 0 1 ... N-1 }` requires one number to uniquely identify/address each element. A 2-dimensional index would be 2 numbers, ..., N-dimensional index would be N numbers to uniquely address each element.

So what do we do about data that's higher dimensional?

Common 2D data:
- Images (grid of pixels)
- Geographic maps (roads and intersections)
- Telescope orientation (azimuth and altitude angles)

Common 3D data:
- Video (time sequence of 2D images)
- Topographic maps (2D position and elevation)
- Position data (e.g. 3D models, laser scans)
- Color spaces (RGB, HSL, Luv)

Common 4D data:
- Position traces (time sequence of 3D positions)
- Einstein's space-time positions

Note that it's common to have compound objects of higher dimensions. E.g. images are 2D grids of colors, and each color is usually a 3D RGB point (red, green, blue). So in a sense images are really 3D, `X x Y x ColorChannels` with `ColorChannels=3` for this RGB case. I.e. we can slice the image into 3 images; one red, one green, and one blue. 
```
R: X x Y
G: X x Y
B: X x Y
```
Or, we can slice it as one image where each pixel contains the RGB triple.
```
X x Y x (R,G,B)
```
So we can partition or slice higher dimensional compound objects into compounds of lower dimensional objects. Specifically, for N dimensions, we can choose any possible partition of the set of size N. Counting these possibilities, combinatorial mathematics calls this the Bell number `B(N)`. Broadly, it's the sum of all possible combinations of recursively computing `B` for the chosen subset. I.e. iterating all possibilities of all subsets and counting them up. This grows really fast.

Also note we can reorder the dimensions arbitrarily, and still represent the same data. For N dimensions, there are `P(N, N) = n!` permutations of the dimensions that are all equivalent. This alone would take us into exponential growth of possible encodings, as the dimensions grow.

Combining both of these, arbitrary partitioning and arbitrary ordering, is the Ordered Bell numbers. Which accounts for both of the above principles at once. This grows faster than each of the above.

So the design space for representing higher dimensional data is exponentially large. Hence, choosing a dimensional encoding to use for your data is incredibly difficult. A lot of it comes down to performance characteristics of your specific data access patterns, and experimentation with various packing techniques to find what works best.

What follows is a collection of useful dimensional encoding techniques, to map higher dimensional data into 1D memory space.

## Horizontal stripe
Say we have an `X x Y` grid
```
0 1 2 3
4 5 6 7
```
```
index(x, y) = X * y + x
```

## Horizontal striding (sub-stripe)
Given an `X x Y` grid
```
 0  1  2  3
 4  5  6  7
 8  9 10 11
```
And a sub-block `offset = (1,1)` and `extent = (2,2)`, we want to index the sub-block:
```
 5  6
 9 10
```
The idea here is to subtract away the starting offset, and then use the original extent X.
```
index(offset, extent, x, y) = (X * offset.y + offset.x) + (X * y + x)
= X * (offset.y + y) + (offset.x + x)
for all x,y in [0, extent.x) x [0, extent.y)
```
In this way you can index a sub-block of a horizontally striped block.

## Vertical stripe
```
0 2 4 6
1 3 5 7
```
This is a complete transpose of horizontal strip, so swap x,y in all cases to get:
```
index(x, y) = Y * x + y
```

## Vertical striding (sub-stripe)
This is a trivial transpose of the horizontal version.
```
index(offset, extent, x, y) = Y * (offset.x + x) + (offset.y + y)
for all x,y in [0, extent.x) x [0, extent.y)
```

## Lower triangular, row-wise
```
0
1 2
3 4 5
6 7 8 9
```
Note the first element of each row `y` follows a simple accumulation pattern:
```
{ 0 1 3 6 ... }
{ 0 0+1 0+1+2 0+1+2+3 ...}
```
Which has a closed-form summation formula:
```
start(y) = sum { 0, 1, ..., y }
start(y) = y(y+1)/2
```
It's trivial to then tack on the column indexing, once identifying the start of each row. So the 1D-from-2D mapping is:
```
index(x,y) = y(y+1)/2 + x
```
## Upper triangular, col-wise
```
0 1 3 6
  2 4 7
    5 8
      9
```
Note this is the transpose of the above. Just swap x,y and we're done. So the 1D-from-2D mapping is:
```
index(x,y) = x(x+1)/2 + y
```
## Upper triangular, row-wise
```
0 1 2 3
  4 5 6
    7 8
      9
```
The pattern for the first element of each row is a bit trickier. The key idea is that it's the summation of all previous row lengths.
```
{ 0 4 7 9 }
{ 0 N N+(N-1) N+(N-1)+(N-2) }
{ 0 N-0 2N-1 3N-3 }
```
Note that the subtraction accumulation is the same as we saw earlier, but shifted over 1.
```
{ 0 N 2N 3N 4N ... } - { 0 0 1 3 6 ... }
```
Using that decomposition, I get
```
start(y) = Ny - sum { 0, 1, ..., y-1 }
= Ny - (y-1)y/2
= (2Ny - (y-1)y)/2
= (2N - (y-1))y/2
= (2N+1-y)y/2
```
A quick test shows that the `y=0` case isn't affected by the `y-1` shift. So this is indeed the correct formula. 

Column indexing is again just a trivial tack-on. So the 1D-from-2D mapping is:
```
index(x,y) = (2N+1-y)y/2 + x
```

## Lower triangular, col-wise
```
0
1 4
2 5 7
3 6 8 9
```
Note this is the transpose of the above. Just swap x,y and we're done. So the 1D-from-2D mapping is:
```
index(x,y) = (2N+1-x)x/2 + y
```

## Lower triangular, excluding diagonal, row-wise
```
-
0 -
1 2 -
3 4 5 -
```
Note this is identical to [Lower triangular, row-wise](#lower-triangular-row-wise), just shifted down one row. I.e. pass in `(y-1)` instead of `y` to the `index(x,y) = y(y+1)/2 + x` formula. So the 1D-from-2D mapping is:
```
index(x,y) = (y-1)y/2 + x
```

## Upper triangular, excluding diagonal, col-wise
```
- 0 1 3
  - 2 4
    - 5
      -
```
Note this is the transpose of the above. So the 1D-from-2D mapping is:
```
index(x,y) = (x-1)x/2 + y
```

## Upper triangular, excluding diagonal, row-wise
```
- 0 1 2
  - 3 4
    - 5
      -
```
TODO: This is wrong.

Note this is identical to [Upper triangular, row-wise](#upper-triangular-row-wise), just shifted right one column. I.e. pass in `(x-1)` instead of `x` and `(N-1)` instead of `N` to the `index(x,y) = (2N+1-y)y/2 + x` formula. 
```
index(x,y) = (2N+1-(y-1))(y-1)/2 + (x-1)
= (2N+1-y+1)(y-1)/2 + x-1
= (2N+2-y)(y-1)/2 + x-1
```
So the 1D-from-2D mapping is:
```
index(x,y) = (2N-y)(y-1)/2 + x-1
```

## Right-leaning diagonal (Hankel matrix)
A rectangular matrix with skew diagonals (aka right-leaning diagonals) each holding one value.
```
0 1 2 3
1 2 3 4
2 3 4 5
3 4 5 6
```
The number of such diagonals is `N+(N-1) = 2N-1`. So for 1D storage you need an array of length `2N-1`. To get 1D from 2D indices:
```
index(x, y) = x + y
```
Note this is one of the unique matrix maps discussed that's not invertible. I.e. given a 1D index, there's no unique map back to 2D.

## Left-leaning diagonal (Toeplitz matrix)
A rectangular matrix with diagonals (aka left - leaning diagonals) each holding one value. This is the x-mirror of Hankel.
```
3 2 1 0
4 3 2 1
5 4 3 2
6 5 4 3
```
Storage space required: `2N-1`.

To get from 1D to 2D:
```
index(x, y) = (N - 1 - x) + y
```

## Compressed sparse row (CSR)
In horizontal stripe (aka row-wise) order, store only the values that are present (often meaning non-zero) in the uncompressed matrix. That is, elide storage of sparse elements, but preserve the row-wise ordering. And, for each value that is stored, store the x index (aka column index) it belongs to.

Then what's left is to identify row boundaries. To do this, keep an array of length `X+1` that stores the starting index of each row in the compressed data / x-index arrays.
```
template<typename T> struct CompressedSparseRow {
vector<T> values;
	vector<size_t> xIndices; // length values.size()
	vector<size_t> yStarts; // length X+1, last is end of last row
	size_t X;
	size_t Y;
	// Returns a pointer to the value at (x,y), or nullptr if not present
	const T* get(size_t x, size_t y) const {
		if (x >= X || y >= Y) throw out_of_range("Index out of range");
		auto rowStart = begin(xIndices) + yStarts[y];
		auto rowEnd = begin(xIndices) + yStarts[y + 1];
		auto it = lower_bound(rowStart, rowEnd, x);
		if (it != rowEnd && *it == x) {
			const size_t index = it - begin(xIndices);
			return &values[index];
		}
		return nullptr;
	}
};
```
For instance, `X = 5, Y = 2` uncompressed matrix:
```
- 0 1 - -
- 2 - 3 -
```
Will have
```
CompressedSparseRow<int32_t> csr = {
	.values = {0, 1, 2, 3},
	.xIndices = {1, 2, 1, 3},
	.yStarts = {0, 2, 4},
	.X = 5,
	.Y = 2
};
```
## Compressed sparse column (CSC)
Exactly the same as CSR, but transposed. Trivially swap x,y and you'll have the CSC version.

## yx Address concatenation
Given `xx` and `yy`, concatenate the bits to get `yyxx`. For example, with 2 bit length `x` and `y`,
```
     00   01   10   11 x
00 0000 0001 0010 0011
01 0100 0101 0110 0111
10 1000 1001 1010 1011
11 1100 1101 1110 1111
 y
```
Mapping back to base 10,
```
 0  1  2  3
 4  5  6  7
 8  9 10 11
12 13 14 15
```
Note that this is identical to the [Horizontal stripe](#horizontal-stripe) layout. It works when the `X` dimension is a power of two.

## xy Address concatenation
Given `xx` and `yy`, concatenate the bits to get `xxyy`. For example, with 2 bit length `x` and `y`,
```
     00   01   10   11 x
00 0000 0100 1000 1100
01 0001 0101 1001 1101
10 0010 0110 1010 1110
11 0011 0111 1011 1111
 y
```
Mapping back to base 10,
```
 0  4  8 12
 1  5  9 13
 2  6 10 14
 3  7 11 15
```
Note that this is identical to the [Vertical stripe](#vertical-stripe) layout. It works then the `Y` dimension is a power of two.

## Morton order
Given `xx` and `yy`, interleave the bits to get `yxyx`. For example, with 2 bit length `x` and `y`,
```
     00   01   10   11 x
00 0000 0001 0100 0101
01 0010 0011 0110 0111
10 1000 1001 1100 1101
11 1010 1011 1110 1111
 y
```
Mapping back to base 10,
```
 0  1  4  5
 2  3  6  7
 8  9 12 13
10 11 14 15
```
Notice how it's a Z pattern of smaller Z patterns. This self-similar hierarchy continues for every 2 more bits present in `x` and `y`. 

The 1D-from-2D mapping is:
```
uint16_t index(uint8_t x, uint8_t y) {
    const uint16_t a = x;
    const uint16_t b = y;
    const uint16_t A = 
        ((a & 0b00000001) << 0) |
        ((a & 0b00000010) << 2) |
        ((a & 0b00000100) << 4) |
        ((a & 0b00001000) << 6) |
        ((a & 0b00010000) << 8) |
        ((a & 0b00100000) << 10) |
        ((a & 0b01000000) << 12) |
        ((a & 0b10000000) << 14);
    const uint16_t B =
        ((b & 0b00000001) << 1) |
        ((b & 0b00000010) << 3) |
        ((b & 0b00000100) << 5) |
        ((b & 0b00001000) << 7) |
        ((b & 0b00010000) << 9) |
        ((b & 0b00100000) << 11) |
        ((b & 0b01000000) << 13) |
        ((b & 0b10000000) << 15);
    return A | B;
}
```
This is usually implemented in hardware, since it's a simple bit interleave operation, and that can be done by just twisting wires to perform the interleave.

For image downsizing, this is the optimal memory layout. Consider collapsing each 2x2 sub-image down into a 1x1 pixel. Each 2x2 is already contiguous.
```
template<typename T> vector<T> DownsizeMortonByHalf(const span<T> image) {
    const size_t cI = image.size();
    assert(cI % 4 == 0);
    const size_t cR = cI / 4;
    vector<T> R(cR);
    for (size_t i = 0; i < cR; ++i) {
        R[i] = (image[4*i+0] + image[4*i+1] + image[4*i+2] + image[4*i+3]) / 4;
    }
    return R;
}
```
Note that since the Morton order is self-similar, you can trivially downsize again and again, until you reach a 1x1 image. Graphics people call this a Level-of-Detail hierarchy, and it's helpful for when you want to render image thumbnails at any size smaller than the original size.

## ND address concatenation
Given an N-dimensional grid with `E[N]` denoting the extent of dimension N, and each `E[N]` is a power of two, you can interleave and/or concatenate linear address bits for each individual dimension, into a singular 1-dimensional address.

For example, say we have a 4x8x16 grid, each addressed respectively via `xx`, `yyy`, `zzzz`. All possible permutations of all of these bits form all possible address concatenation schemes.
```
xxyyyzzzz
zzzzyyyxx
xyxyyzzzz
zzzzyyxyx
...
```
The bits also don't necessarily have to remain in their original orders per dimension, so it really is all possible permutations of the bits. There are `P(K,K) = K!` permutations, where `K` is the total number of bits across all dimensions. So notice that as the number of dimensions and the extents of the dimensions grows, the possible 1-dimensional encodings according to this scheme grows exponentially.

This technique is particularly powerful, unique in that it can collapse N-dimensions directly down into 1 with one scheme. Most of the other dimensional packing schemes subtract one dimension (e.g. take 2D to 1D), whereas this scheme divides by N to go straight to 1D.

When combined with 1D sparsity techniques, this is one way you can encode arbitrary dimensional data with very little source code.

## Page directory
First, consider 1D lattice sparsity. If I have `S = { 0 1 ... N-1 }` with some arbitrary subsets present, how do I store the subset efficiently for large N?
The page idea is to split the `S` space into chunks of size `C` (aka pages). Then I can make one root directory page, which contains the addresses of chunks that are either present or not. There are multiple directory schemes, but this is the simplest.
```
template<typename T> struct PageDirectory1D {
	constexpr size_t C = 1000;
	using Page = array<T, C>;
	vector<unique_ptr<Page>> pages;
	size_t N = 0;
	PageDirectory1D(size_t N_) : N(N_) {
		pages.resize((N + C - 1) / C); // Allocate directory, initially no pages.
	}
	const T* get(size_t index) const {
		assert(index < N);
		const size_t iPage = index / C;
		const size_t iOffset = index % C;
		assert(iPage < pages.size());
		const auto& page = pages[iPage];
		if (!page) return nullptr; // Page not present
		return &(*page)[iOffset]; // Return pointer to value
	}
	void set(size_t index, const T& value) {
		assert(index < N);
		const size_t iPage = index / C;
		const size_t iOffset = index % C;
		assert(iPage < pages.size());
		auto& page = pages[iPage];
		if (!page) {
			page = make_unique<Page>();
		}
		(*page)[iOffset] = value;
	}
};
```

## Sparse Array Bitmap
Idea: use a register-sized bitmap (e.g. `uint64_t`) to represent presence, and an array of present values (sorted by index order).
For example, shrinking it down to 8 bits for a moment, we have a universe of indexes `S = { 0 1 ... 7 }`. Say I want to store values corresponding to the sparse subset `s = { 0 1 6 }`.
```
bitmap = 01000011
values = { v0, v1, v6 }
```
If I want to retrieve the element at index 6, the bitmap trivially tells me presence information (bit is set or not). That's nice. But note that it also encodes the index into the `values` sorted array. The index into `values` is the number of bits set below bit 6.
```
bitmap = 01000011
		   ^^^^^^
```
There are 2 bits set, so the index into `values` is 2. By constructing an appropriate bitmask and using the popcount instruction, we can do this efficiently. Hence we can go from an uncompressed index to a compressed index into `values`.
```
index(u) = popcount(bitmap & ((1 << u) - 1))
```
Here's the 64bit version of this idea:
```
template<typename T> struct SparseArray64 {
	uint64_t bitmap = 0;
	vector<T> values;
	const T* get(size_t index) const {
		assert(index < 64);
		const size_t bit = 1ULL << index;
		if ((bitmap & bit) == 0) return nullptr; // Not present
		const size_t iValue = popcount(bitmap & (bit - 1));
		return &values[iValue];
	}
	void set(size_t index, const T& value) {
		assert(index < 64);
		const size_t bit = 1ULL << index;
		const size_t iValue = popcount(bitmap & (bit - 1));
		if ((bitmap & bit) == 0) { // Not present, so insert at appropriate position
			bitmap |= bit;
			values.insert(begin(values) + iValue, value);
		} else { // Already present, so update.
			values[iValue] = value;
		}
	}
};
```
You can extend this to an arbitrary - length bitmap, and I'm sure there's some bitmap tree data structure you could use to compute `iValue` in `O(log N)` popcounts instead of `O(N)` popcounts.
Also, for repeated `set` calls causing lots of `values` shifting during `insert`, it may be more efficient to use an ordered tree of some kind instead.

## Locality Sensitive Hashing


# Instructions
# Execution state
# Instruction parallelism (SIMD)

# Execution parallelism
Take the execution state of a computer, stamp out multiple copies of it, fan-out work to them which can be done in parallel, and synchronously join the results back in to one of the instances designated as the `main` thread or sub-computer.

## Multi-computer parallelism
This is execution parallelism in the most general sense. Take distinct computers that can each run fully independently, wire them up for communication, and pass messages between them with no other sharing. In a sense this is the purest form of computational parallelism, where nothing is shared except for messages passed between distinct computers. This is the model used by distributed systems, where each computer is a node in a cluster, and they communicate over a network. Almost all the effort put into parallelism at this level is in designing message protocols to ensure correct and efficient behavior.

## Multi-process parallelism
This is execution parallelism at the process level, where multiple processes run on the same computer (sharing an operating system layer), each with its own memory space and execution state. They can communicate via inter-process communication (IPC) mechanisms like pipes, sockets, or shared memory. Each of these are operating systems (OS) capabilities exposed to the process layer. Most of the effort here is in designing message protocols, while taking advantage of shared memory to avoid copying big data into messages.

## Multi-thread parallelism
This is execution parallelism at the thread level, where multiple threads run within the same process (sharing the same memory space). Each thread has its own execution state, but the primary difference from process parallelism is that the memory space is shared by default. Meaning, everything is shared between threads by default, unless the programmer explicitly uses thread-local storage or other mechanisms to isolate data. Most of the effort here is in building, maintaining, and enforcing thread safety, i.e. preventing threads from accessing memory they shouldn't be.

## Lockstep parallelism (SIMT)
The idea here is to have a bunch of threads execute the same instruction seqeunce in parallel, in lockstep. In this way they can share instruction decoding, translation, and dispatch logic, i.e. the first phases of instruction processing.
This is called Single Instruction Multiple Threads (SIMT) in NVIDIA parlance, since one instruction is shared amongst many threads executing it in lockstep.

### Conditionals 
Won't the lockstep need to break down as soon as some conditional is evaluated? E.g. one thread wants to go into the `if` block and another wants to go into the `else`.
The key insight here is that we can have all threads execute BOTH the `if` and the `else`, and use a condition bit to signal whether each thread should ignore the current instruction or not. This looks like an extra bit associated with each thread in the computer's execution state, which is set by conditional instructions, and reset when conditional scopes are closed.
NVIDIA calls these simultaneous thread groups `warps`, AMD calls these `wavefronts`. The masking off is called divergence, and the restoring to all threads enabled is called convergence.
This was the key insight that allowed GPUs to become massively parallel, and is the basis of all modern GPU architectures.

### Dynamic lockstep
Modern GPUs allow for dynamic divergence and convergence, even automatically, to try and preserve the speedup of simultaneous instruction execution as much as possible. The idea is to allow for some small amount of divergence, minimizing the amount of time spent on masked off instructions. For example, once a conditional is hit and there's the `if` and `else` blocks, the set of threads in the group can be partitioned into two subgroups, one for each branch. In each subgroup, we can remove the dead instructions from the instruction stream and only execute the live branch. Once both branches are done, we can insert a synchronization point for convergence, which will destroy the two subgroups and return us back to just one group. Note this requires support for two instruction streams, one for each subgroup, and the ability to create and delete them dynamically. More than two if the architecture wants to allow for even more independent scheduling. Modern GPUs do this and more, to maximize the amount of useful parallel work.






# SCRATCH





# ENDSCRATCH
