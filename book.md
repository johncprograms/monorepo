
# Table of Contents
- [Table of Contents](#table-of-contents)
- [What is a computer?](#what-is-a-computer)
- [Memory array](#memory-array)
	- [Memory cell](#memory-cell)
- [Unsigned integers](#unsigned-integers)
	- [Modulo arithmetic](#modulo-arithmetic)
	- [Overflow](#overflow)
	- [Underflow](#underflow)
	- [Multiplication](#multiplication)
	- [Division](#division)
	- [Greatest common divisor](#greatest-common-divisor)
	- [Least common multiple](#least-common-multiple)
	- [Detecting overflow](#detecting-overflow)
	- [Base conversions](#base-conversions)
- [Bit operations](#bit-operations)
	- [Not (aka flip)](#not-aka-flip)
	- [And](#and)
	- [Or](#or)
	- [Xor](#xor)
	- [Set](#set)
	- [Clear (aka reset)](#clear-aka-reset)
	- [Conditional not](#conditional-not)
	- [Conditional set](#conditional-set)
	- [Conditional clear](#conditional-clear)
	- [Shift Left Logical 0](#shift-left-logical-0)
	- [Shift Left Logical 1](#shift-left-logical-1)
	- [Shift Left Arithmetic](#shift-left-arithmetic)
	- [Shift Right Logical 0](#shift-right-logical-0)
	- [Shift Right Logical 1](#shift-right-logical-1)
	- [Shift Right Arithmetic](#shift-right-arithmetic)
	- [Rotate (aka circular shift)](#rotate-aka-circular-shift)
	- [Reverse](#reverse)
	- [Population count](#population-count)
	- [Is power of 2](#is-power-of-2)
	- [K trailing 1s](#k-trailing-1s)
	- [K trailing 0s](#k-trailing-0s)
	- [K leading 1s](#k-leading-1s)
	- [K leading 0s](#k-leading-0s)
	- [Count trailing 1s](#count-trailing-1s)
	- [Count trailing 0s](#count-trailing-0s)
	- [Count leading 1s](#count-leading-1s)
	- [Count leading 0s](#count-leading-0s)
	- [Floor to power of 2](#floor-to-power-of-2)
	- [Ceiling to power of 2](#ceiling-to-power-of-2)
	- [Floor to multiple of power of 2](#floor-to-multiple-of-power-of-2)
	- [Ceiling to multiple of power of 2](#ceiling-to-multiple-of-power-of-2)
	- [Floor to multiple of N](#floor-to-multiple-of-n)
	- [Ceiling to multiple of N](#ceiling-to-multiple-of-n)
- [Signed integers](#signed-integers)
	- [Sign bit](#sign-bit)
	- [1's complement](#1s-complement)
	- [2's complement](#2s-complement)
	- [Offset binary](#offset-binary)
	- [Zig Zag](#zig-zag)
- [Self-referential memory](#self-referential-memory)
	- [Internal indexing](#internal-indexing)
	- [External indexing](#external-indexing)
- [Dynamic memory allocation](#dynamic-memory-allocation)
- [Variable length array](#variable-length-array)
- [Variable length stack](#variable-length-stack)
- [Variable length bitmap](#variable-length-bitmap)
- [Sequences (aka strings)](#sequences-aka-strings)
	- [Subsequences (aka substrings)](#subsequences-aka-substrings)
	- [Views](#views)
	- [Contains](#contains)
	- [Count](#count)
	- [Find](#find)
	- [Replace](#replace)
	- [Concatenate](#concatenate)
	- [Zipper merge](#zipper-merge)
- [Intervals](#intervals)
	- [Discrete vs continuous](#discrete-vs-continuous)
	- [Tiling](#tiling)
	- [Overlap](#overlap)
	- [Intersect](#intersect)
	- [Union](#union)
	- [Difference](#difference)
	- [Symmetric difference](#symmetric-difference)
	- [Contains](#contains-1)
	- [Covering](#covering)
	- [Merging](#merging)
- [Fixed point](#fixed-point)
- [Floating point](#floating-point)
	- [Kahan summation](#kahan-summation)
- [Non modulo numbers](#non-modulo-numbers)
- [Rational numbers](#rational-numbers)
- [Linked lists](#linked-lists)
	- [Internal indexing](#internal-indexing-1)
	- [External indexing](#external-indexing-1)
- [Trees](#trees)
	- [Succinct static trees](#succinct-static-trees)
	- [Left-child only binary tree encoding](#left-child-only-binary-tree-encoding)
	- [Label isomorphic subtrees](#label-isomorphic-subtrees)
- [Forests](#forests)
- [Directed Acyclic Graphs (aka DAGs)](#directed-acyclic-graphs-aka-dags)
- [Graphs](#graphs)
	- [Adjacency matrix](#adjacency-matrix)
	- [Adjacency lists](#adjacency-lists)
	- [Implicit/formulaic](#implicitformulaic)
		- [1D Lattice](#1d-lattice)
		- [2D Lattice](#2d-lattice)
		- [ND Lattice](#nd-lattice)
		- [Hash partitioning](#hash-partitioning)
		- [Rendezvous hashing](#rendezvous-hashing)
	- [Topological sort](#topological-sort)
	- [Minimum spanning tree](#minimum-spanning-tree)
	- [Shortest path](#shortest-path)
	- [Minimum cut (aka maximum flow)](#minimum-cut-aka-maximum-flow)
	- [Connected components](#connected-components)
	- [Bipartite matching](#bipartite-matching)
- [Hypergraphs (aka multigraphs)](#hypergraphs-aka-multigraphs)
	- [Directed hyperedges](#directed-hyperedges)
	- [Undirected hyperedges](#undirected-hyperedges)
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




# What is a computer?
1. [Memory array](#memory-array) An addressable information store
2. [Instructions](#instructions) A sequence of memory modification instructions stored in memory
3. [Execution state](#execution-state) Namely, the address of the next instruction to execute

In short: memory, instructions, and execution state.

# Memory array
Think of condo/apartment mailboxes, addressable slots. Fixed number of slots. Each slot can contain a fixed size of something.
By numbering each slot, giving them a fixed address, we can identify and remember what was put where.

Mathematically, take a set of integer slots, `s = { 1, 2, ..., S }`.

Define a configurable function `m` that maps from `s` to the set of numbers. I.e. for each slot `i` in `s`, let `m[i]` be the number we're storing in slot `i`.

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
The fundamental idea of computer memory is that it's a configurable number (slot address) to number (value) map.

## Memory cell
In modern computers, it's a voltage potential stored in a physical wire; either high or low. Mapping to a binary number 1 or 0. Digital logic latches and flip-flops are the basic circuitry constructs that allow for storing a single bit of memory, which can be either 0 or 1. Transistors sit on either side of the memory wire, controlling the storage and retrieval of the high or low voltage and letting it move to the next stage of the wire network.

By laying out a bunch of wires in parallel, we can combine those binary 0s and 1s into one logical unit, a binary number with more digits.

# Unsigned integers
Let's break down the representation of a number, to explore how we can convert it to 0s and 1s.

Say we've got `234`. We can break it down per digit as:
```
234 = 2 * 100 + 3 * 10 + 4 * 1
```
More generally,
```
d = digit(100) digit(10) digit(1) = digit(100) * 100 + digit(10) * 10 + digit(1) * 1
d = digit(10^2) digit(10^1) digit(10^0) = digit(10^2) * 10^2 + digit(10^1) * 10^1 + digit(10^0) * 10^0
d = sum of digit(10^k) * 10^k, for k = 0, 1, 2, ...
```
We can let k go on as far as we want, and just assume there are implicit leading zeros so eventually it will converge.

We can generalize to arbitrary bases:

Binary (base 2):
```
d = sum of digit(2^k) * 2^k, for k = 0, 1, 2, ...
```
Octal (base 8): 
```
d = sum of digit(8^k) * 8^k, for k = 0, 1, 2, ...
```
Hexadecimal (base 16): 
```
d = sum of digit(16^k) * 16^k, for k = 0, 1, 2, ...
```
Arbitrary base, where `base` is any positive integer:
```
d = sum of digit(base^k) * base^k, for k = 0, 1, 2, ...
```
Computer hardware is generally base 2, because we can set up electronic circuits where the the voltage potential is either high (mapping to 1) or low (mapping to 0). There are some high density storage systems that use base 3 or 4 or more, by using intermediate levels in between max voltage and ground. 

## Modulo arithmetic
Computer memory is finite, so how do we deal with arbitrarily large numbers? We pick a maximum number of digits. And hence a maximum number we can represent.

With higher-level constructions, you can build arbitrarily large numbers based on smaller fixed-size numbers. See [Arbitrary size integers] section.

Say we pick base 10, and maximum number of digits is 3. Valid range of numbers is 0 (aka 000) up to 999.
```
d = sum of digit(10^2) * 10^2 + digit(10^1) * 10^1 + digit(10^0) * 10^0
d = sum of digit(10^k) * 10^k, for k = 0, 1, 2.
```
Notice how k is bounded to be less than 3, our maximum number of digits. Now we have a finite set of numbers.

## Overflow
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

## Underflow
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

## Multiplication
TODO
https://en.wikipedia.org/wiki/Multiplication_algorithm

## Division
TODO: Long division algorithm

## Greatest common divisor
This is the problem of: given a set of numbers, find the largest number that divides every number in the set with zero remainder. Mathematically, 
```
GCD(S) = max { D | for all s in S, s % D == 0 }
```
For example, given `{ 4, 6, 8 }`, the greatest common divisor would be `2`.

First, note that when `|S| = 1`, `GCD(s) = s`. Trivially, `s` divides itself.

When `|S| = 2`, Euclid gives us an algorithm that runs in `O(d)` steps, where `d` is the number of digits of the input. Classically, it's given by:
```
uint64_t GCD(uint64_t A, uint64_t B) {
	auto a = max<uint64_t>(A,B);
	auto b = min<uint64_t>(A,B);
	assert(b);
	while (b) {
		const auto t = b;
		b = a % b;
		a = t;
	}
	return a;
}
```
When `|S| > 2`, note that we can use the `|S| = 2` algorithm to collapse pairs together and replace that set element, since `GCD(s) = s`. E.g. if we have `{ 4, 6, 8, 12 }`, we could collapse adjacents to get `{ GCD(4,6), GCD(8,12) } = { 2, 4 }` and again, `{ GCD(2,4) } = { 2 }` which is the right answer, `2`. Note you can apply the pairwise collapsing in any order. Maximum throughput would be to do it in parallel:
```
uint64_t GCD(span<uint64_t> s) {
	size_t c = s.size();
	if (!c) return 0;
	if (c == 1) return s[0];
	vector<uint64_t> S(begin(s), end(s));
	c = CeilingToPowerOf2(c);
	S.resize(c); // Zero extended
	for (size_t offset = 2; offset <= c; offset *= 2) {
		parallel_for (size_t i = 0; i < c; i += offset) {
			S[i] = GCD(S[i], S[i+offset/2]);
		}
	}
	return S[0];
}
```
A slightly better version would be to use `FloorToPowerOf2` and combine it with the following serial version to handle the non-power of 2 remainder:
```
uint64_t GCD(span<uint64_t> s) {
	const size_t c = s.size();
	if (!c) return 0;
	if (c == 1) return s[0];
	auto R = GCD(s[0], s[1]);
	for (size_t i = 2; i < c; ++i) {
		R = GCD(R, s[i]);
	}
	return R;
}
```

## Least common multiple
This is the problem of: given a set of numbers, find the smallest number that's divisible by every number in the set. Mathematically, 
```
LCM(S) = min { M | for all s in S, M % s == 0 }
```
For example, given `{ 4, 5, 6 }`, the least common multiple would be `60`.

The first observation is that we could multiply all numbers in the set and get a common multiple, although it may not be the minimum. To get to the minimum, we'd need to take that common multiple and divide it by the `GCD` of the numbers in the set. I.e.
```
LCM(S) = (s1 * s2 * ... * sS) / GCD(S)
```
For overflow reasons it's advantageous to distribute the `/ GCD(S)` divide to each `si` first, and then multiply the terms together. This is perfectly safe since we know the `GCD(S)` will divide each member of `S` with `0` remainder.

If we had a prime factorization for each of the numbers, e.g. `{ 4, 5, 6 } = { 2^2, 5^1, 2^1 3^1 }`, we can take the set of primes that cover the prime factors, in this case `{ 2, 3, 5 }`, and the maximum exponent of each, `{ 2^2, 3^1, 5^1 }`, and multiply those together to get the `LCM`. Unfortunately, computing a prime factorization takes time that grows exponentially as the number of digits grows. So in practice, most software will use the `GCD` formulation to compute the `LCM`.

```
uint64_t LCM(span<uint64_t> s) {
	const auto gcd = GCD(s);
	uint64_t R = 1;
	for (const auto& num : s) {
		R *= (num / gcd);
	}
	return R;
}
```

## Detecting overflow
`a + b` overflows when `a + b >= numeric_limits<>::max()`. Rearranging to avoid overflow in the detection, `a >= numeric_limits<>::max() - b`.
```
bool IsOverflowingAdd(uint32_t a, uint32_t b) {
	return a >= (numeric_limits<uint32_t>::max() - b);
}
```
`a - b` underflows when `a < b`.
```
bool IsUnderflowingSub(uint32_t a, uint32_t b) {
	return a < b;
}
```

TODO
`a * b` overflows when ``

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

# Bit operations
You can use a single bit to represent a Boolean value: True/False. A bitmap is defined as an array of Booleans, densely packed.

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

## Clear (aka reset)
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

## Population count
Counts the number of bits set to 1 in the bitmap. This usually requires an iteration over bits:
```
uint32_t PopulationCount(uint32_t a) {
	uint32_t R = 0;
	for (size_t i = 0; i < numeric_limits<uint32_t>::digits; ++i) {
		R += (a & 0b1);
		a = a >> 1;
	}
	return R;
}
```
Thankfully, modern computer hardware has implemented this operation as an intrinsic instruction.
```
uint32_t PopulationCount(uint32_t a) {
	return popcount(a);
}
```

## Is power of 2
```
IsPowerOf2(0) = IsPowerOf2(000) = 0
IsPowerOf2(1) = IsPowerOf2(001) = 1
IsPowerOf2(2) = IsPowerOf2(010) = 1
IsPowerOf2(3) = IsPowerOf2(011) = 0
IsPowerOf2(4) = IsPowerOf2(100) = 1
IsPowerOf2(5) = IsPowerOf2(101) = 0
...
```
Notice that `IsPowerOf2` returns True when there's precisely one `1` bit present. And if you subtract 1, you'll get a sequence of 1s trailing the original 1 bit.
```
  4 = 0100
4-1 = 0011
```
Treating that as a mask, we can check if any of those lower bits are set via [And](#and).
```
bool IsPowerOf2(uint32_t a) {
	return (a & (a-1)) == 0;
}
```
Or by using [Population count](#population-count) directly,
```
bool IsPowerOf2(uint32_t a) {
	return popcount(a) == 1;
}
```
The C++ standard library has `has_single_bit` which does this.
```
bool IsPowerOf2(uint32_t a) {
	return has_single_bit(a);
}
```

## K trailing 1s
Given a bitmap width `N`, and a number `K <= N`, generate a bitmap of `(N-K)` 0s followed by `K` trailing 1s. For instance with `N=4`,
```
KTrailingOnes(0) = 0000
KTrailingOnes(1) = 0001
KTrailingOnes(2) = 0011
KTrailingOnes(3) = 0111
KTrailingOnes(4) = 1111
```

This came up as a trick in [Shift Left Logical 1](#shift-left-logical-1). The key insight is that a power of two has a single bit set, followed by zeros. And if you then subtract 1, you'll get a sequence of ones.
```
  8 = 0b1000
8-1 = 0b0111
  8 = 1u << 3
```
So note that I can create a mask of K trailing 1s via:
```
uint32_t KTrailingOnes(uint32_t k) {
	assert(k <= numeric_limits<uint32_t>::digits);
	if (k == numeric_limits<uint32_t>::digits) return numeric_limits<uint32_t>::max();
	return (1u << k) - 1;
}
```
An alternate strategy is to take all 1s, zero out a number of leading bits via shifting them off, and then shifting back in 0s.
```
uint32_t KTrailingOnes(uint32_t k) {
	assert(k <= numeric_limits<uint32_t>::digits);
	if (k == 0) return 0;
	if (k == numeric_limits<uint32_t>::digits) return numeric_limits<uint32_t>::max();
	return (numeric_limits<uint32_t>::max() << (numeric_limits<uint32_t>::digits - k)) >> k;
}
```

## K trailing 0s
Given a bitmap width `N`, and a number `K <= N`, generate a bitmap of `(N-K)` 1s followed by `K` trailing 0s. For instance with `N=4`,
```
KTrailingZeros(0) = 1111
KTrailingZeros(1) = 1110
KTrailingZeros(2) = 1100
KTrailingZeros(3) = 1000
KTrailingZeros(4) = 0000
```
Note we can simply take [K trailing 1s](#k-trailing-1s) and flip the bits.
```
uint32_t KTrailingZeros(uint32_t k) {
	return ~KTrailingOnes(k);
}
```

## K leading 1s
Given a bitmap width `N`, and a number `K <= N`, generate a bitmap of `K` 1s followed by `(N-K)` trailing 0s. For instance with `N=4`,
```
KLeadingOnes(0) = 0000
KLeadingOnes(1) = 1000
KLeadingOnes(2) = 1100
KLeadingOnes(3) = 1110
KLeadingOnes(4) = 1111
```
Note we can write this in terms of [K trailing 0s](#k-trailing-0s) by reversing order.
```
uint32_t KLeadingOnes(uint32_t k) {
	return KTrailingZeros(numeric_limits<uint32_t>::digits - k);
}
```

## K leading 0s
Given a bitmap width `N`, and a number `K <= N`, generate a bitmap of `K` 0s followed by `(N-K)` trailing 1s. For instance with `N=4`,
```
KLeadingZeros(0) = 1111
KLeadingZeros(1) = 0111
KLeadingZeros(2) = 0011
KLeadingZeros(3) = 0001
KLeadingZeros(4) = 0000
```
Note we can simply take [K leading 1s](#k-leading-1s) and flip the bits.
```
uint32_t KLeadingZeros(uint32_t k) {
	return ~KLeadingOnes(k);
}
```

## Count trailing 1s
From the least significant bit, count how many contiguous 1 bits there are in the bitmap.
```
CountTrailingOnes(xxx0) = 0
CountTrailingOnes(xx01) = 1
CountTrailingOnes(x011) = 2
CountTrailingOnes(0111) = 3
CountTrailingOnes(1111) = 4
```
Note this is a nontrivial computation, requiring actual bit iteration/accumulation in some way.
```
uint32_t CountTrailingOnes(uint32_t a) {
	for (uint32_t R = 0; R != numeric_limits<uint32_t>::digits; ++R) {
		if ((a & 0b1) == 0) return R;
		a = a >> 1u;
	}
	return numeric_limits<uint32_t>::digits;
}
```
Thankfully, modern computer hardware has implemented this operation as an intrinsic instruction.
```
uint32_t CountTrailingOnes(uint32_t a) {
	return countr_one(a);
}
```

## Count trailing 0s
From the least significant bit, count how many contiguous 0 bits there are in the bitmap.
```
CountTrailingZeros(xxx1) = 0
CountTrailingZeros(xx10) = 1
CountTrailingZeros(x100) = 2
CountTrailingZeros(1000) = 3
CountTrailingZeros(0000) = 4
```
This can be implemented as CountTrailingOnes via a simple negation:
```
uint32_t CountTrailingZeros(uint32_t a) {
	return CountTrailingOnes(~a);
}
```
The standard intrinsic:
```
uint32_t CountTrailingZeros(uint32_t a) {
	return countr_zero(a);
}
```

## Count leading 1s
From the most significant bit, count how many contiguous 1 bits there are in the bitmap.
```
CountLeadingOnes(0xxx) = 0
CountLeadingOnes(10xx) = 1
CountLeadingOnes(110x) = 2
CountLeadingOnes(1110) = 3
CountLeadingOnes(1111) = 4
```
The standard intrinsic:
```
uint32_t CountLeadingOnes(uint32_t a) {
	return countl_one(a);
}
```

## Count leading 0s
From the most significant bit, count how many contiguous 0 bits there are in the bitmap.
```
CountLeadingZeros(1xxx) = 0
CountLeadingZeros(01xx) = 1
CountLeadingZeros(001x) = 2
CountLeadingZeros(0001) = 3
CountLeadingZeros(0000) = 4
```
The standard intrinsic:
```
uint32_t CountLeadingZeros(uint32_t a) {
	return countl_zero(a);
}
```

## Floor to power of 2
Rounds a given integer down to a power of 2. If it's already a power of 2, leave it alone. Let 0 stay 0.
```
FloorToPowerOf2(0) = FloorToPowerOf2(000) = 0
FloorToPowerOf2(1) = FloorToPowerOf2(001) = 1
FloorToPowerOf2(2) = FloorToPowerOf2(010) = 2
FloorToPowerOf2(3) = FloorToPowerOf2(011) = 2
FloorToPowerOf2(4) = FloorToPowerOf2(100) = 4
FloorToPowerOf2(5) = FloorToPowerOf2(101) = 4
```
We only have to count the number of leading zeros, and from that we can generate a power of two with the same number of leading zeros.
|a|CountLeadingZeros(a)|3-CountLeadingZeros(a)|2-CountLeadingZeros(a)|1 << (2-CountLeadingZeros(a))|
|-|-|-|-|-|
|000|3|0|3 (sub underflow)|000 (shift overflow)
|001|2|1|0|001
|010|1|2|1|010
|011|1|2|1|010
|100|0|3|2|100
|101|0|3|2|100
...

Since shift overflow is `undefined behavior` in C++, we handle the `0` case explicitly.
```
uint32_t FloorToPowerOf2(uint32_t a) {
	if (a == 0) return 0;
	return 1u << (numeric_limits<uint32_t>::digits - CountLeadingZeros(a) - 1u);
}
```
The standard C++ intrinsic:
```
uint32_t FloorToPowerOf2(uint32_t a) {
	return bit_floor(a);
}
```

## Ceiling to power of 2
Rounds a given integer up to a power of 2. If it's already a power of 2, leave it alone.
```
CeilingToPowerOf2(0) = CeilingToPowerOf2(000) = 1
CeilingToPowerOf2(1) = CeilingToPowerOf2(001) = 1
CeilingToPowerOf2(2) = CeilingToPowerOf2(010) = 2
CeilingToPowerOf2(3) = CeilingToPowerOf2(011) = 4
CeilingToPowerOf2(4) = CeilingToPowerOf2(100) = 4
CeilingToPowerOf2(5) = CeilingToPowerOf2(101) = 8
```
The `0` case is somewhat exceptional, so that's handled separately. For the rest, the key insight is that powers of 2 are unique in that if you subtract one, the number of leading zeros will change. For all other numbers, non powers of 2, subtracting one will not change the number of leading zeros. After doing the count, we can reverse the count and reconstruct a power of 2. E.g. for 4 bits,

| a | a-1 | CountLeadingZeros(a-1) | 4-CountLeadingZeros(a-1) | 1 << (4-CountLeadingZeros(a-1)) |
|-|-|-|-|-|
0000|1111 (sub underflow)|0|4|0000 (shift overflow)|
0001|0000|4|0|0001|
0010|0001|3|1|0010|
0011|0010|2|2|0100|
0100|0011|2|2|0100|
0101|0100|1|3|1000|
0110|0101|1|3|1000|
0111|0110|1|3|1000|
1000|0111|1|3|1000|
1001|1000|0|4|0000 (shift overflow)|
...

Notice that we get the wrong answer for `1001` because the shift overflows. The correct answer would be `10000`, but that doesn't fit in the 4 bit integer. The same happens with all numbers up to `1111`. We can either use a larger integer as the intermediate and return value, or make success conditional, or force the caller to have already dealt with it.
```
uint64_t CeilingToPowerOf2(uint32_t a) {
	if (a == 0) return 1;
	return 1ull << (numeric_limits<uint64_t>::digits - CountLeadingZeros(static_cast<uint64_t>(a) - 1));
}
```
```
bool CeilingToPowerOf2(uint32_t a, uint32_t& result) {
	if (a == 0) {
		result = 1;
		return true;
	}
	const uint32_t cShift = numeric_limits<uint32_t>::digits - CountLeadingZeros(a - 1);
	if (cShift >= numeric_limits<uint32_t>::digits) {
		return false;
	}
	result = 1u << cShift;
	return true;
}
```
```
uint32_t CeilingToPowerOf2(uint32_t a) {
	if (a == 0) return 1;
	const uint32_t cShift = numeric_limits<uint32_t>::digits - CountLeadingZeros(a - 1);
	assert(cShift < numeric_limits<uint32_t>::digits);
	return 1u << cShift;
}
```
The standard C++ intrinsic takes the third approach, with so-called `undefined behavior`. Essentially, leaving it up to the caller to never pass in something that would cause the overflow case.
```
uint32_t CeilingToPowerOf2(uint32_t a) {
	return bit_ceil(a);
}
```

## Floor to multiple of power of 2
Rounds a given integer down to a multiple of the given power of 2. If it's already a multiple of a power of 2, leave it alone.
```
FloorToMultipleOfPowerOf2(0,2) = 0
FloorToMultipleOfPowerOf2(1,2) = 0
FloorToMultipleOfPowerOf2(2,2) = 2
FloorToMultipleOfPowerOf2(3,2) = 2
FloorToMultipleOfPowerOf2(4,2) = 4
FloorToMultipleOfPowerOf2(5,2) = 4
FloorToMultipleOfPowerOf2(6,2) = 6
FloorToMultipleOfPowerOf2(7,2) = 6
...
```
```
FloorToMultipleOfPowerOf2(0,4) = 0
FloorToMultipleOfPowerOf2(1,4) = 0
FloorToMultipleOfPowerOf2(2,4) = 0
FloorToMultipleOfPowerOf2(3,4) = 0
FloorToMultipleOfPowerOf2(4,4) = 4
FloorToMultipleOfPowerOf2(5,4) = 4
FloorToMultipleOfPowerOf2(6,4) = 4
FloorToMultipleOfPowerOf2(7,4) = 4
...
```
Note how useful this is for bucketing; `FloorToMultipleOfPowerOf2(a, powerOf2) / powerOf2` gives you a bucket number, where each bucket holds `powerOf2` unique integers. In this way, you can partition a 1D lattice `{0, 1, ..., N-1}` (where `N` is a power of 2) into uniform buckets of some smaller size (`N/2`, `N/4`, `N/8`, etc.). This technique comes up often in sparse storage designs.

Getting back to the implementation, the key idea is that the least significant bits are what decide numbers in between one multiple and another. E.g. 
```
xxx000 multiple of 4
xxx001
xxx010
xxx011
xxx100 multiple of 4
xxx101
xxx110
xxx111
xx1000 multiple of 4
```
Aligning to `0` as a multiple, we can AND away the least significant bits to redirect intermediate entries above into their `multiple of 4` entries above them. So `xxx011 & 11` takes us to `xxx000 multiple of 4`. The mask we use is simply the `powerOf2 - 1`, in this case `4-1 = 3 = 11 binary`. All put together,
```
uint32_t FloorToMultipleOfPowerOf2(uint32_t a, uint32_t powerOf2) {
	assert(IsPowerOf2(powerOf2));
	return a & ~(powerOf2 - 1u);
}
```
Note an alternate is to shift right and then [Shift Left Logical 0](#shift-left-logical-0), to clear the least significant bits. The amount to shift by is `CountTrailingZeros(powerOf2)`. This is generally slower than the above approach, since the `~(powerOf2 - 1u)` mask can generally be precomputed and the whole thing can be just a single `AND` instruction above, whereas the below is a serial chain of 3 instructions: `CountTrailingZeros`, `Shift Right`, `Shift Left Logical 0`. But I've included it here for reference.
```
uint32_t FloorToMultipleOfPowerOf2(uint32_t a, uint32_t powerOf2) {
	assert(IsPowerOf2(powerOf2));
	const uint32_t cShift = CountTrailingZeros(powerOf2);
	return (a >> cShift) << cShift;
}
```

## Ceiling to multiple of power of 2
Rounds a given integer up to a multiple of the given power of 2. If it's already a multiple of a power of 2, leave it alone.
```
CeilingToMultipleOfPowerOf2(0,2) = 0
CeilingToMultipleOfPowerOf2(1,2) = 2
CeilingToMultipleOfPowerOf2(2,2) = 2
CeilingToMultipleOfPowerOf2(3,2) = 4
CeilingToMultipleOfPowerOf2(4,2) = 4
CeilingToMultipleOfPowerOf2(5,2) = 6
CeilingToMultipleOfPowerOf2(6,2) = 6
CeilingToMultipleOfPowerOf2(7,2) = 8
...
```
```
CeilingToMultipleOfPowerOf2(0,4) = 0
CeilingToMultipleOfPowerOf2(1,4) = 4
CeilingToMultipleOfPowerOf2(2,4) = 4
CeilingToMultipleOfPowerOf2(3,4) = 4
CeilingToMultipleOfPowerOf2(4,4) = 4
CeilingToMultipleOfPowerOf2(5,4) = 8
CeilingToMultipleOfPowerOf2(6,4) = 8
CeilingToMultipleOfPowerOf2(7,4) = 8
...
```
Take the example from above:
```
xxx000 multiple of 4
xxx001
xxx010
xxx011
xxx100 multiple of 4
xxx101
xxx110
xxx111
xx1000 multiple of 4
```
The key insight here is that we can use the same floor calculation, but offset the input so that the floor aligns to the intended ceiling result. In this case, `FloorToMultipleOfPowerOf2(a + 3, 4)`. The `+3` offset takes `x01`, `x10`, `x11` down beyond the next multiple of 4, so the floor will take us back.
```
uint32_t CeilingToMultipleOfPowerOf2(uint32_t a, uint32_t powerOf2) {
	assert(IsPowerOf2(powerOf2));
	assert(!IsOverflowingAdd(a, powerOf2 - 1));
	return FloorToMultipleOfPowerOf2(a + (powerOf2 - 1), powerOf2);
}
```
Inlining,
```
uint32_t CeilingToMultipleOfPowerOf2(uint32_t a, uint32_t powerOf2) {
	assert(IsPowerOf2(powerOf2));
	const uint32_t mask = powerOf2 - 1;
	assert(!IsOverflowingAdd(a, mask));
	return (a + mask) & ~mask;
}
```
Note that since this is a ceiling function, it has a potential to overflow the input space. Again we have the option to just assert (crash) and force callers to avoid this situation, change the contract to return an integer in a wider space, or return a success conditional alongside the result and fail on overflow. The assert option is above, and here's the wider return option:
```
uint64_t CeilingToMultipleOfPowerOf2(uint32_t a, uint32_t powerOf2) {
	assert(IsPowerOf2(powerOf2));
	const uint64_t mask = static_cast<uint64_t>(powerOf2) - 1ull;
	return (a + mask) & ~mask;
}
```
And the conditional success option:
```
bool CeilingToMultipleOfPowerOf2(uint32_t a, uint32_t powerOf2, uint32_t& result) {
	assert(IsPowerOf2(powerOf2));
	const uint32_t mask = powerOf2 - 1;
	if (IsOverflowingAdd(a, mask)) {
		return false;
	}
	result = (a + mask) & ~mask;
	return true;
}
```

## Floor to multiple of N
Rounds a given integer down to a multiple of the given `N`. If it's already a multiple of `N`, leave it alone.
```
FloorToMultipleOfN(0,3) = 0
FloorToMultipleOfN(1,3) = 0
FloorToMultipleOfN(2,3) = 0
FloorToMultipleOfN(3,3) = 3
FloorToMultipleOfN(4,3) = 3
FloorToMultipleOfN(5,3) = 3
FloorToMultipleOfN(6,3) = 6
FloorToMultipleOfN(7,3) = 6
...
```
For `N` non powers of 2, we have to use full integer division and remainder. We can't manipulate digits in the natural base directly, so we're left with full division.

There's basically two options:
1. divide, drop the remainder, and multiply back
1. divide and subtract the remainder from the input

```
uint32_t FloorToMultipleOfN(uint32_t a, uint32_t n) {
	assert(n > 0);
	return (a / n) * n;
}
```
```
uint32_t FloorToMultipleOfN(uint32_t a, uint32_t n) {
	assert(n > 0);
	return a - (a % n);
}
```
Which is faster is going to depend on microarchitectural details.

## Ceiling to multiple of N
Rounds a given integer up to a multiple of the given `N`. If it's already a multiple of `N`, leave it alone.
```
CeilingToMultipleOfN(0,3) = 0
CeilingToMultipleOfN(1,3) = 3
CeilingToMultipleOfN(2,3) = 3
CeilingToMultipleOfN(3,3) = 3
CeilingToMultipleOfN(4,3) = 6
CeilingToMultipleOfN(5,3) = 6
CeilingToMultipleOfN(6,3) = 6
CeilingToMultipleOfN(7,3) = 9
...
```
One way to write this is in terms of `FloorToMultipleOfN`, i.e. `FloorToMultipleOfN(a+n-1, n)`. However, it's hard to guarantee we overflow only in the cases where the result would overflow; i.e. avoiding overflow in intermediate terms. So instead, we'll compute the ceiling more directly as a term we add to the given number, so the overflow check triggers only when the result requires overflow. Here's the multiple versions for how you want to handle the overflow case:
```
uint32_t CeilingToMultipleOfN(uint32_t a, uint32_t n) {
	const uint32_t remainder = a % n;
	const uint32_t up = (remainder != 0) ? (n - remainder) : 0;
	assert(!IsOverflowingAdd(a, up));
	return a + up;
}
```
```
uint64_t CeilingToMultipleOfN(uint32_t a, uint32_t n) {
	const auto A = static_cast<uint64_t>(a);
	const auto N = static_cast<uint64_t>(n);
	const auto remainder = A % N;
	const auto up = (remainder != 0) ? (N - remainder) : 0;
	return A + up;
}
```
```
bool CeilingToMultipleOfN(uint32_t a, uint32_t n, uint32_t& result) {
	const uint32_t remainder = a % n;
	const uint32_t up = (remainder != 0) ? (n - remainder) : 0;
	if (IsOverflowingAdd(a, up)) {
		return false;
	}
	result = a + up;
	return true;
}
```

# Signed integers
So far we've only considered non-negative numbers. I.e. zero and all positives. But what about negative numbers?

There are many different encodings, all with various trade-offs. Modern computer hardware has mostly stabilized around 2's complement for most purposes. 

## Sign bit
Take a bitmap, choose one bit to represent `+` or `-`, and leave the rest for representing the value.

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
    Mathematically, `0 = -0`, but they have different bitmap representations. This complicates equality checks.
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
Take a bitmap, choose the highest bit to signal `+` or `-`, and flip negative bitmaps to recover the value.
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
This also has the `0 = -0` but not in bitmap form issue. Note there's a discontinuity jumping from 3 to -3, but otherwise the ordering is always increasing. This makes ordering slightly easier.
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
Choose the highest bit to signal `+` or `-`, flip negative bitmaps and add one to recover the value.
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
Optimizes the number of bits required for small absolute values. Uses the least significant bit as an indicator of `+` or `-`.
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

This extends indefinitely; we can store arbitrarily-nested maps of maps of maps of ... maps to numbers. I.e. structures which hold references to structures which hold references to ... numbers. This combinatorial power is why we can encode really complicated things in a single memory. 

We'll start with the simplest list encoding. Say we have 4 slots, `S = { 0, 1, 2, 3 }`. Say `m[i]`'s value refers to the next slot to look at, with `m[0]` denoting the head of the list. And define `0` as our encoded value to mean `no link`. We can store the list `1 -> 2 -> 3` as:
```
m[0] = 1
m[1] = 2
m[2] = 3
m[3] = 0
```
By reading `m[0]` we figure out where the start of the list is, and then we can traverse the subsequent `m[i]`s until reaching an `m[k] == 0` denoting the end of the list, which in this example is `m[3]`.

With this scheme we can encode the empty list as:
```
m[0] = 0
```
List containing one element:
```
m[0] = 1
m[1] = 0
```
List `3 -> 1 -> 2`:
```
m[0] = 3
m[1] = 2
m[2] = 0
m[3] = 1
```
Say we want to extend this encoding scheme to store a number at each node. We could either:
1. store per-node values alongside the `next` link, so called [Internal indexing](#internal-indexing). Or,
1. store per-node values after the links, so called [External indexing](#external-indexing).

## Internal indexing
Say we want to store `(3,c) -> (1,a) -> (2,b)` with values `a, b, c` on those nodes. 

We will store the value of node `i` at `m[1+2*i]`, with the `next` link stored at the subsequent location `m[1+2*i+1]`. I.e. `{ starting node, node 1 value, node 1 next, node 2 value, node 2 next, ... }`.
```
m[0] = 5
m[1] = a
m[2] = 3
m[3] = b
m[4] = 0
m[5] = c
m[6] = 1
```
Note the same list could alternately be encoded as `{ starting node, node 3 value, node 3 next, node 1 value, node 1 next, node 2 value, node 2 next }`. Or any other permutation of the nodes. Or even with gaps.
```
m[0] = 1
m[1] = c
m[2] = 3
m[3] = a
m[4] = 7
m[5] = unused
m[6] = unused
m[7] = b
m[8] = 0
```
Note we can extend the associated values of each ndoe by expanding the size of the `node i value`. E.g. `(3,c1,c2) -> (1,a1,a2) -> (2,b1,b2)` can be encoded as:
```
m[0] = 1
m[1] = c1
m[2] = c2
m[3] = 4
m[4] = a1
m[5] = a2
m[6] = 7
m[7] = b1
m[8] = b2
m[9] = 0
```

## External indexing
Say we want to store `(3,c) -> (1,a) -> (2,b)` with values `a, b, c` on those nodes. 

Say `m[0]` is the head of the list to start with. `m[i]` will store the links for `i in { 1, 2, 3 }`, and `m[i+3]` will store the values. I.e. `{ starting node, node 1 next, node 2 next, ..., node 1 value, node 2 value, ... }`.
```
m[0] = 3
m[1] = 2
m[2] = 0
m[3] = 1
m[4] = a
m[5] = b
m[6] = c
```
So at any node `i` to get the corresponding value, we just look up `m[i+3]`, or in general, `m[i+cNodes]` where `cNodes` is the number of nodes we've allocated as our maximum list capacity. Note that we could arbitrarily separate the links block and the values block, and just change the lookup offset to accomodate that.

This strategy is called external indexing, where we align two independent blocks; one block stores the connectivity information, and one block stores associated data. This is powerful, because we can trivially add more associated data (imagine adding a third block to store an additional number per-node, etc.). And we can write code that modifies connectivity without considering the value data; having to copy it, move it, etc.

# Dynamic memory allocation
Every computer has a ceiling to the amount of memory onboard. That is, the memory array has a fixed upper limit. In order to write software programs that can run on various computers with different amounts of memory, we need a dynamic addressing scheme.

The other major constraint is that very often the input to a computer program is user-generated (or at least influenced), and so it will be variable-length. Meaning, we need an addressing scheme that adjusts for variable-length things we want to store.

The first idea is a stack, where we take a fixed size array, designate a variable stack top, and allow pushing/popping to/from the top. With this partitioning, we get two intervals: variable data, and empty space.
```
{variable1, ..., variableN, empty1, ..., emptyE}
```
```
template<typename T, size_t Capacity> struct Stack {
	array<T, Capacity> stack;
	size_t top = 0; // index of first empty slot at the top of the stack.
	void push(const T& value) {
		assert(top < stack.size());
		stack[top] = value;
		++top;
	}
	const T& top() {
		assert(top > 0);
		return stack[top-1];
	}
	void pop() {
		assert(top > 0);
		--top;
	}
};
```
Note we can run out of space if the variable data grows too long.
Also note that for other things we want to store in memory, now the problem is worse. Instead of trying to come up with address schemes for a fixed size array, now we have to generate address schemes for the empty space interval that's unused by the stack, which is now variable size (as the stack grows/shrinks).

If our only other storage requirement is one more variable-length data, we could reverse the order and have a symmetric stack for the second variable-length data. That way both variable-length data streams can shrink/grow, and we'll avoid overlaps unless we're really out of space.
```
{variable1, ..., variableN, empty1, ..., emptyE, varSecondM, ..., varSecond1}
```
```
template<typename T, size_t Capacity> struct DoubleStack {
	array<T, Capacity> memory;
	size_t top1 = 0; // index of first empty slot at the top of the first stack.
	size_t top2 = 0; // index of first occupied slot at the top of the second stack (or Capacity if not occupied).
	void push1(const T& value) {
		assert(top1 < top2);
		memory[top1] = value;
		++top1;
	}
	const T& top1() {
		assert(top1 > 0);
		return memory[top1-1];
	}
	void pop1() {
		assert(top1 > 0);
		--top1;
	}
	void push2(const T& value) {
		assert(top1 < top2);
		--top2;
		memory[top2] = value;
	}
	const T& top2() {
		assert(top2 < memory.size());
		return memory[top2];
	}
	void pop2() {
		assert(top2 < memory.size());
		++top2;
	}
};
```
However, notice that if we have 3 or more variable-length data streams to store, now we've got issues. We'll need to consistently move interior data streams around to make room as the first and second variable data changes length, or choose some other strategy. Like a variable list storing addresses of other variable streams.

The key thing to notice is that we need some kind of indirection layer here to manage addressing of variable-length data. Effectively, turn the variable-length stream into a fixed-length reference, so the variable-length stream can be managed by a memory subsystem. Dynamic memory allocation does precisely this.

The standardized interface for managing variable-length data storage is an acquire/release pair. Also known as allocate/free, new/delete, etc.
```
T* allocate(size_t count);
void free(T* reference);
```
```
T* new(size_t count);
void delete(T* reference);
```
The key thing here is that allocation can fail, due to running out of memory. We're requesting some dynamic size, and only when there's sufficient room will we get an interval of writeable memory back.

On modern computers, this fundamental memory management is a capability provided by the operating system (OS). On Windows, see `VirtualAlloc` documentation. On Unix, see `mmap` documentation. You request a block of memory, and the OS provides the memory and address to it. And when you're done, you hand it back so the OS can reclaim the memory space for other things.

There are many details to dive into for how the OS implements this, as well as layers built on top of the OS layer for specific memory usage patterns.

# Variable length array
Now that we can rely on the OS layer to implement memory allocation requests, we can build the most basic variable length structure, the array. Also known as: table, resizeable array. We request one contiguous memory block, keep track of the size, and allow indexing into the allocated block. We can also accomodate a `resize()` operation that preserves values that were there (except for dropped values if you resize to smaller), while initializing new empty slots to default values (if you resize to larger).

This is the fundamental data structure for encoding a variable length memory block into a fixed size indirection block (length and starting address of the variable block). 
```
template<typename T> struct Array {
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

# Variable length stack
This is the first trivial extension of the [Variable length array](#variable-length-array), often used in its place because of the increased capability. The idea is to add a stack top index, and allow a subset of the array's memory to be used. 

The C++ standard library calls this `std::vector`.

As the stack grows/shrinks, it's important to use a geometric growth strategy. The motivating example is a sequence of `P` pushes; if we reallocate to grow by a constant factor (e.g. always make 10 new spaces), then we will reallocate `P/10` times. If `P` is `O(N)`, each reallocation is `O(N)`, and so the total sequence will be `O(N^2)` which leads to horrible performance. The resolution is to grow by a multiplicative factor, e.g. double/halve the capacity. Then for `P` pushes, we'll reallocate `O(log P)` times. With `P` being `O(N)`, the total sequence will thus be `O(N log N)`. Which is good enough to handle dynamic sizing in practice. Note that doubling/halving will maintain a utilization ratio of `[0.5, 1]`, which on average is `0.75`. You can improve on that utilization at the cost of more frequent reallocations by choosing some factor other than 2, like `1.5`. The factor of 2 is often used though, so you can use [Ceiling to power of 2](#ceiling-to-power-of-2) and similar strategies to infer next/previous capacities.


This strategy usually starts breaking down at huge sizes (e.g. many gigabytes or larger), due to address space fragmentation. I.e. the OS may eventually not be able to find a contiguous block of memory that's gigabytes large, because smaller allocations have been requested and serviced all over the address space. This is less common nowadays in the age of 48bit virtual address space, but it can still happen. At these levels, you'll start wanting to use more complex data structures to allow for non-contiguity.
```
template<typename T> struct Stack {
  T* mem = nullptr;
  size_t len = 0; // # of elements currently on the stack.
  size_t capacity = 0;

  Stack() = default;
  Stack(size_t capacityInitial) {
    mem = new T[capacityInitial];
    len = 0;
	capacity = capacityInitial;
  }
  Stack(size_t capacityInitial, const T& valueInitial) {
    mem = new T[capacityInitial];
    for (size_t i = 0; i < capacity; ++i)
      mem[i] = valueInitial;
    len = 0;
	capacity = capacityInitial;
  }
  ~Stack() noexcept {
    if (mem)
      delete [] mem;
    mem = nullptr;
    len = 0;
	capacity = 0;
  }
  __forceinline size_t capacity() const noexcept { return capacity; }
  __forceinline size_t size() const noexcept { return len; }
  __forceinline bool empty() const noexcept { return len == 0; }
  __forceinline void reallocate(size_t capacityNew) {
	assert(len <= capacityNew);
	if (capacity == capacityNew) return;
    auto memNew = new T[capacityNew];
    for (size_t i = 0; i < len; ++i)
      memNew[i] = std::move(mem[i]);
    delete [] mem;
    mem = memNew;
    capacity = capacityNew;
  }
  __forceinline void resize(size_t lenNew) {
    if (len == lenNew) return;
	if (len > lenNew) {
		// Reset values at the end to default values.
		for (size_t i = lenNew; i < len; ++i) {
			mem[i] = {};
		}
	}
	len = lenNew;
	reallocate(CeilingToPowerOf2(len));
  }
  __forceinline const T& operator[](size_t i) const {
    assert(i < len);
    return mem[i];
  }
  __forceinline T& operator[](size_t i) {
    assert(i < len);
    return mem[i];
  }
  __forceinline void push(const T& value) {
	if (len == capacity) {
		reallocate(2 * capacity);
	}
	mem[len] = value;
	++len;
  }
  __forceinline const T& top() {
	assert(len > 0);
	return mem[len];
  }
  __forceinline void pop() {
	assert(len > 0);
	--len;
	if (len < capacity / 2) {
		reallocate(capacity / 2);
	}
  }
};
```

# Variable length bitmap
As described in [Bit operations](#bit-operations), lots of things operate in parallel across bits. A Boolean flag per-input, for example. Since [Variable length stack](#variable-length-stack) is so flexible, we often want that, but only a single bit (or a couple) per-element.

We can implement this as an adapter around a [Variable length stack](#variable-length-stack) of `uint64_t`, the natural register width of today's 64bit CPUs.
```
struct Bitmap {
	vector<uint64_t> m_v;
	size_t m_cBits = 0;

	__forceinline size_t size() const noexcept { return m_cBits; }
	__forceinline bool empty() const noexcept { return m_cBits == 0; }
	__forceinline void clear() noexcept {
		m_cBits = 0;
		m_v.clear();
	}
	__forceinline void resize(size_t cBits) {
		m_cBits = cBits;
		m_v.resize((cBits + 63) / 64);

		// Reset the trailing bits in the last word.
		if (cBits) {
			const size_t j = cBits - 1;
			const size_t j64 = j / 64;
			const size_t jbit = j % 64;
			const uint64_t jmask = jbit == 63 ? ~0ULL : (1ULL << (jbit + 1)) - 1;
			m_v[j64] &= jmask;
		}
	}
	__forceinline bool get(size_t i) const noexcept {
		assert(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		return (m_v[i64] & (1ULL << ibit)) != 0;
	}
	__forceinline void set(size_t i, bool f) noexcept {
		assert(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		if (f)
			m_v[i64] |= (1ULL << ibit);
		else
			m_v[i64] &= ~(1ULL << ibit);
	}
	__forceinline void set(size_t i) noexcept {
		assert(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		m_v[i64] |= (1ULL << ibit);
	}
	__forceinline void reset(size_t i) noexcept {
		assert(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		m_v[i64] &= ~(1ULL << ibit);
	}
	__forceinline bool getThenSet(size_t i) noexcept {
		assert(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		const bool f = (m_v[i64] & (1ULL << ibit)) != 0;
		m_v[i64] |= (1ULL << ibit);
		return f;
	}
	// Sets the range [i, j] to 1.
	__forceinline void setRange(size_t i, size_t j) noexcept {
		assert(i <= j);
		assert(j < m_cBits);
		const size_t i64 = i / 64;
		const size_t j64 = j / 64;
		const size_t ibit = i % 64;
		const size_t jbit = j % 64;
		const uint64_t imask = ((1ULL << ibit) - 1);
		const uint64_t jmask = jbit == 63 ? ~0ULL : (1ULL << (jbit + 1)) - 1;
		if (i64 == j64) {
			// All bits are in the same 64-bit word.
			const uint64_t mask = imask ^ jmask;
			m_v[i64] |= mask;
		}
		else {
			// Set the bits in the first word.
			m_v[i64] |= ~imask;
			// Set the bits in the middle words.
			for (size_t k = i64 + 1; k < j64; ++k)
				m_v[k] = ~0ULL;
			// Set the bits in the last word.
			m_v[j64] |= jmask;
		}
	}
	__forceinline void setAll() noexcept {
		if (m_cBits)
			setRange(0, m_cBits - 1);
	}
	// Resets the range [i, j] to 0.
	__forceinline void resetRange(size_t i, size_t j) noexcept {
		assert(i <= j);
		assert(j < m_cBits);
		const size_t i64 = i / 64;
		const size_t j64 = j / 64;
		const size_t ibit = i % 64;
		const size_t jbit = j % 64;
		const uint64_t imask = ((1ULL << ibit) - 1);
		const uint64_t jmask = jbit == 63 ? ~0ULL : (1ULL << (jbit + 1)) - 1;
		if (i64 == j64) {
			// All bits are in the same 64-bit word.
			const uint64_t mask = imask ^ jmask;
			m_v[i64] &= ~mask;
		}
		else {
			// Reset the bits in the first word.
			m_v[i64] &= imask;
			// Reset the bits in the middle words.
			for (size_t k = i64 + 1; k < j64; ++k)
				m_v[k] = 0ULL;
			// Reset the bits in the last word.
			m_v[j64] &= ~jmask;
		}
	}
	__forceinline void resetAll() noexcept {
		if (m_cBits)
			resetRange(0, m_cBits - 1);
	}
	// Returns the number of bits set to 1 in the range [i, j].
	__forceinline size_t popcount(size_t i, size_t j) const noexcept {
		assert(i <= j);
		assert(j < m_cBits);
		const size_t i64 = i / 64;
		const size_t j64 = j / 64;
		const size_t ibit = i % 64;
		const size_t jbit = j % 64;
		const uint64_t imask = ((1ULL << ibit) - 1);
		const uint64_t jmask = jbit == 63 ? ~0ULL : (1ULL << (jbit + 1)) - 1;
		size_t count = 0;
		if (i64 == j64) {
			// All bits are in the same 64-bit word.
			const uint64_t mask = imask ^ jmask;
			count += std::popcount(m_v[i64] & mask);
		}
		else {
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
	__forceinline void append(const Bitmap& o) {
		if (o.empty())
			return;
		if (empty()) {
			*this = o;
			return;
		}
		const size_t cold = m_cBits / 64;
		const size_t cshift = (m_cBits % 64);
		if (!cshift) {
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
		for (; i < o.m_v.size() - 1; ++i) {
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
	__forceinline void emplace_back(bool f) {
		const size_t i = m_cBits++;
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		if (i64 == m_v.size()) {
			m_v.push_back(0);
		}
		m_v[i64] |= (size_t)f << ibit;
	}

	~Bitmap() noexcept = default;
	Bitmap() = default;
	Bitmap(size_t cBits) : m_cBits(cBits) {
		m_v.resize((cBits + 63) / 64);
	}
	Bitmap(const Bitmap& o) : m_v(o.m_v), m_cBits(o.m_cBits) {}
	Bitmap& operator=(const Bitmap& o) {
		m_v = o.m_v;
		m_cBits = o.m_cBits;
		return *this;
	}
	Bitmap(Bitmap&& o) noexcept {
		// clang-format off
		m_v = std::move(o.m_v); o.m_v.clear();
		m_cBits = std::move(o.m_cBits); o.m_cBits = 0;
		// clang-format on
	}
	Bitmap& operator=(Bitmap&& o) noexcept {
		// clang-format off
		m_v = std::move(o.m_v); o.m_v.clear();
		m_cBits = std::move(o.m_cBits); o.m_cBits = 0;
		// clang-format on
		return *this;
	}
};
static void TestBitmap() {
	auto VerifyEqual = [](const Bitmap& b, const vector<bool>& v) {
		assert(b.size() == v.size());
		for (size_t i = 0; i < b.size(); ++i) {
			assert(b.get(i) == v[i]);
		}
		if (b.size()) {
			const size_t cB = b.popcount(0, b.size() - 1);
			size_t cV = 0;
			for (size_t i = 0; i < v.size(); ++i)
				cV += (size_t)v[i];
			assert(cB == cV);
		}
	};
	Bitmap b;
	vector<bool> v;
	minstd_rand gen(1234);
	uniform_int_distribution<size_t> dist;
	function<void()> rgfn[] = {
		[&]() {
			b.clear();
			v.clear();
			VerifyEqual(b, v);
		},
		[&]() {
			const size_t cBits = dist(gen) % 1000;
			b.resize(cBits);
			v.resize(cBits);
			VerifyEqual(b, v);
		},
		[&]() {
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
		[&]() {
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
		[&]() {
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
		[&]() {
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
		[&]() {
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
			assert(cB == cV);
		},
		[&]() {
			const size_t cAppend = dist(gen) % 1000;
			Bitmap n { cAppend };
			vector<bool> m(cAppend);
			for (size_t i = 0; i < cAppend; ++i) {
				const bool f = dist(gen) % 2;
				n.set(i, f);
				m[i] = f;
			}
			VerifyEqual(n, m);
			b.append(n);
			v.insert(end(v), begin(m), end(m));
			VerifyEqual(b, v);
		},
		[&]() {
			const size_t cAppend = dist(gen) % 1000;
			for (size_t i = 0; i < cAppend; ++i) {
				const bool f = dist(gen) % 2;
				b.emplace_back(f);
				v.push_back(f);
			}
			VerifyEqual(b, v);
		},
	};
	for (size_t i = 0; i < 10000; ++i) {
		const size_t j = dist(gen) % _countof(rgfn);
		const auto& fn = rgfn[j];
		fn();
	}
}
```

# Sequences (aka strings)
TODO

## Subsequences (aka substrings)
## Views
External length
Internal length
## Contains
## Count
## Find
Forward
Reverse
## Replace
## Concatenate
```
template<typename T> void concatenate(const span<const span<T>> spans, vector<T>& result) {
	size_t cResult = 0;
	for (const auto& s : spans)
		cResult += s.size();
	result.resize(cResult);
	auto r = begin(result);
	for (const auto& s : spans) {
		for (const auto& e : s) {
			*r = e;
			++r;
		}
	}
	assert(r == end(result));
}
```

## Zipper merge
Given two sorted sequences, merge them into one sorted sequence.

# Intervals
TODO
()
[]
(]
[)
```
template<typename T> struct IntervalEE { T p0, p1; };
template<typename T> struct IntervalII { T p0, p1; };
template<typename T> struct IntervalEI { T p0, p1; };
template<typename T> struct IntervalIE { T p0, p1; };
```
## Discrete vs continuous
## Tiling

## Overlap
## Intersect
## Union
## Difference
## Symmetric difference
## Contains
Given a point and an interval, return if the point is inside the interval.
Given a point and a set of intervals, return if the point is inside any interval.
## Covering
Given a set of points and a set of intervals, return if all points are inside any interval.
## Merging
Given a set of intervals, merge them if adjacent/overlapping.
Given two lists of sorted, non-adjacent/overlapping intervals, merge them into one list.

# Fixed point
TODO

# Floating point
IEEE754 standard
TODO

## Kahan summation
TODO

# Non modulo numbers
TODO

# Rational numbers
The rational numbers are defined as the set of all possible fractions. That is, one integer divided by another, where the denominator cannot be zero.
In mathematical set notation,
```
Q = { a / b, where a is an integer and b is an integer not equal to zero }
```
TODO

# Linked lists
Also known as singly-linked lists
Undirected lists
Doubly-linked lists
Xor double-links
TODO

## Internal indexing
Recall the [Internal indexing](#internal-indexing) method, where per-node values are stored alongside the `next` link. `{ starting node, node 1 value, node 1 next, node 2 value, node 2 next, ... }`. 

If we commit to using a discriminated union to pack multiple node types into one supertype (call it `T`), that looks like:
```
template<typename T> struct SingleLinkedNode {
	T value;
	SingleLinkedNode* next = nullptr;

	SingleLinkedNode(const T& value_) : value(value_) {}
	SingleLinkedNode(T&& value_) : value(std::move(value_)) {}
};
template<typename T> struct SingleLinkedList {
	SingleLinkedNode<T>* head = nullptr;
	SingleLinkedNode<T>* tail = nullptr;

	__forceinline bool empty() const { return !head; }
	__forceinline const T& front() const { assert(!empty()); return head->value; }
	__forceinline       T& front()       { assert(!empty()); return head->value; }
	__forceinline const T& back() const { assert(!empty()); return tail->value; }
	__forceinline       T& back()       { assert(!empty()); return tail->value; }

	void clear() {
		while (head) {
			auto* next = head->next;
			delete head;
			head = next;
		}
		head = tail = nullptr;
	}

	__forceinline void _push_back_node(SingleLinkedNode<T>* node) {
		node->next = nullptr;
		if (!head) {
			head = tail = node;
		}
		else {
			assert(!tail->next);
			tail->next = node;
			tail = node;
		}
	}
	void push_back(const T& value) {
		auto* node = new SingleLinkedNode<T>(value);
		_push_back_node(node);
	}
	void emplace_back(T&& value) {
		auto* node = new SingleLinkedNode<T>(std::move(value));
		_push_back_node(node);
	}

	__forceinline void _push_front_node(SingleLinkedNode<T>* node) {
		if (!head) {
			head = tail = node;
		}
		else {
			node->next = head;
			head = node;
		}
	}
	void push_front(const T& value) {
		auto* node = new SingleLinkedNode<T>(value);
		_push_front_node(node);
	}
	void emplace_front(T&& value) {
		auto* node = new SingleLinkedNode<T>(std::move(value));
		_push_front_node(node);
	}

	// NOTE: pop_back would require O(N) node traversal to find the second-to-last node.
	void pop_front() {
		assert(!empty());
		if (head == tail) {
			delete head;
			head = tail = nullptr;
		}
		else {
			auto* headNew = head->next;
			delete head;
			head = headNew;
		}
	}

	// Template to share code across iterator value_type=`T` and const_iterator value_type=`const T`
	template<typename TValue> struct base_iterator final {
		using difference_type = ptrdiff_t;
		using value_type = TValue;
		using reference_type = TValue&;

		SingleLinkedNode<T>* node = nullptr;

		base_iterator() = default;
		base_iterator(SingleLinkedNode<T>* node_) : node(node_) {}
		~base_iterator() noexcept = default;
		base_iterator(const base_iterator& o) { node = o.node; }
		base_iterator& operator=(const base_iterator& o) { node = o.node; }
		bool operator==(const base_iterator& o) const { return node == o.node; }
		reference_type operator*() const { assert(node); return node->value; }
		base_iterator& operator++() { // Prefix
			if (node) {
				node = node->next;
			}
			return *this;
		}
		base_iterator operator++(int) { // Postfix
			auto tmp = *this;
			++(*this);
			return tmp;
		}
	};
	using iterator = base_iterator<T>;
	using const_iterator = base_iterator<const T>;
	static_assert(forward_iterator<iterator>);
	static_assert(forward_iterator<const_iterator>);
	iterator begin() { return iterator(head); }
	iterator end() { return iterator(); }
	const_iterator begin() const { return const_iterator(head); }
	const_iterator end() const { return const_iterator(); }
};
static void TestSingleLinkedList() {
	SingleLinkedList<int> list;
	deque<int> reference;
	auto VerifyEqual = [](const SingleLinkedList<int>& list, const deque<int>& reference) {
		assert(equal(begin(list), end(list), begin(reference), end(reference)));
	};
	minstd_rand rng(0x1234);
	geometric_distribution<int> distGeo(0.01);
	uniform_int_distribution<int> distInt(1, 100);
	const function<void()> fns[] = {
		[&]() {
			const int v = distInt(rng);
			list.push_back(v);
			reference.push_back(v);
			VerifyEqual(list, reference);
		},
		[&]() {
			const int v = distInt(rng);
			int u = v;
			list.emplace_back(move(u));
			u = v;
			reference.emplace_back(move(u));
			VerifyEqual(list, reference);
		},
		[&]() {
			const int v = distInt(rng);
			list.push_front(v);
			reference.push_front(v);
			VerifyEqual(list, reference);
		},
		[&]() {
			const int v = distInt(rng);
			int u = v;
			list.emplace_front(move(u));
			u = v;
			reference.emplace_front(move(u));
			VerifyEqual(list, reference);
		},
		[&]() {
			assert(list.empty() == reference.empty());
			if (!list.empty()) {
				assert(list.front() == reference.front());
				assert(list.back() == reference.back());
			}
			list.pop_front();
			reference.pop_front();
			VerifyEqual(list, reference);
		},
		[&]() {
			auto itL = begin(list);
			auto itL1 = end(list);
			auto itR = begin(reference);
			auto itR1 = end(reference);
			while (itL != itL1) {
				const int v = distInt(rng);
				*itL = v;
				*itR = v;
				++itL;
				++itR;
			}
			assert(itL == itL1);
			assert(itR == itR1);
			VerifyEqual(list, reference);
		},
	};
	for (size_t i = 0; i < 1000; ++i) {
		const size_t ifn = distInt(rng) % size(fns);
		const size_t cfn = distGeo(rng);
		for (size_t j = 0; j < cfn; ++j) {
			fns[ifn]();
		}
	}
}
```
Note that `pop_back()` isn't easily supported. Consider a sequence of `pop_back()` calls; we'd need a way to traverse backwards in the list, but our edges are directed only one way. We'd have to rescan from the list head each time, which is generally unacceptable performance.

The common resolution is to add a backwards direction edge. So instead of just `next`, we also store `prev`, the link in the opposite direction. This is a so-called doubly-linked list.
```
template<typename T> struct DoubleLinkedNode {
	T value;
	DoubleLinkedNode<T>* prev = nullptr;
	DoubleLinkedNode<T>* next = nullptr;

	DoubleLinkedNode(const T& value_) : value(value_) {}
	DoubleLinkedNode(T&& value_) : value(std::move(value_)) {}
};
template<typename T> struct DoubleLinkedList {
	DoubleLinkedNode<T>* head = nullptr;
	DoubleLinkedNode<T>* tail = nullptr;

	__forceinline bool empty() const { return !head; }
	__forceinline const T& front() const { assert(!empty()); return head->value; }
	__forceinline       T& front()       { assert(!empty()); return head->value; }
	__forceinline const T& back() const { assert(!empty()); return tail->value; }
	__forceinline       T& back()       { assert(!empty()); return tail->value; }

	void clear() {
		while (head) {
			auto* next = head->next;
			delete head;
			head = next;
		}
		head = tail = nullptr;
	}
	__forceinline void _push_back_node(DoubleLinkedNode<T>* node) {
		if (!head) {
			node->prev = nullptr;
			node->next = nullptr;
			head = tail = node;
		}
		else {
			node->prev = tail;
			node->next = nullptr;
			assert(!tail->next);
			tail->next = node;
			tail = node;
		}
	}
	void push_back(const T& value) {
		auto* node = new DoubleLinkedNode<T>(value);
		_push_back_node(node);
	}
	void emplace_back(T&& value) {
		auto* node = new DoubleLinkedNode<T>(std::move(value));
		_push_back_node(node);
	}

	__forceinline void _push_front_node(DoubleLinkedNode<T>* node) {
		if (!head) {
			node->prev = nullptr;
			node->next = nullptr;
			head = tail = node;
		}
		else {
			node->prev = nullptr;
			node->next = head;
			head->prev = node;
			head = node;
		}
	}
	void push_front(const T& value) {
		auto* node = new DoubleLinkedNode<T>(value);
		_push_front_node(node);
	}
	void emplace_front(T&& value) {
		auto* node = new DoubleLinkedNode<T>(std::move(value));
		_push_front_node(node);
	}

	void pop_front() {
		assert(!empty());
		if (head == tail) {
			delete head;
			head = tail = nullptr;
		}
		else {
			auto* headNew = head->next;
			delete head;
			headNew->prev = nullptr;
			head = headNew;
		}
	}

	void pop_back() {
		assert(!empty());
		if (head == tail) {
			delete tail;
			head = tail = nullptr;
		}
		else {
			auto* tailNew = tail->prev;
			delete tail;
			tailNew->next = nullptr;
			tail = tailNew;
		}
	}

	// Template to share code across iterator value_type=`T` and const_iterator value_type=`const T`
	template<typename TValue> struct base_iterator final {
		using difference_type = ptrdiff_t;
		using value_type = TValue;
		using reference_type = TValue&;

		DoubleLinkedNode<T>* node = nullptr;

		base_iterator() = default;
		base_iterator(DoubleLinkedNode<T>* node_) : node(node_) {}
		~base_iterator() noexcept = default;
		base_iterator(const base_iterator& o) { node = o.node; }
		base_iterator& operator=(const base_iterator& o) { node = o.node; }
		bool operator==(const base_iterator& o) const { return node == o.node; }
		reference_type operator*() const { assert(node); return node->value; }
		base_iterator& operator++() { // Prefix
			if (node) {
				node = node->next;
			}
			return *this;
		}
		base_iterator operator++(int) { // Postfix
			auto tmp = *this;
			++(*this);
			return tmp;
		}
		base_iterator& operator--() { // Prefix
		if (node) {
			node = node->prev;
		}
		return *this;
	}
	base_iterator operator--(int) { // Postfix
		auto tmp = *this;
		--(*this);
		return tmp;
	}
	};
	using iterator = base_iterator<T>;
	using const_iterator = base_iterator<const T>;
	static_assert(bidirectional_iterator<iterator>);
	static_assert(bidirectional_iterator<const_iterator>);
	iterator begin() { return iterator(head); }
	iterator end() { return iterator(); }
	const_iterator begin() const { return const_iterator(head); }
	const_iterator end() const { return const_iterator(); }
};
static void TestDoubleLinkedList() {
	DoubleLinkedList<int> list;
	deque<int> reference;
	auto VerifyEqual = [](const DoubleLinkedList<int>& list, const deque<int>& reference) {
		assert(equal(begin(list), end(list), begin(reference), end(reference)));
	};
	minstd_rand rng(0x1234);
	geometric_distribution<int> distGeo(0.01);
	uniform_int_distribution<int> distInt(1, 100);
	const function<void()> fns[] = {
		[&]() {
			const int v = distInt(rng);
			list.push_back(v);
			reference.push_back(v);
			VerifyEqual(list, reference);
		},
		[&]() {
			const int v = distInt(rng);
			int u = v;
			list.emplace_back(move(u));
			u = v;
			reference.emplace_back(move(u));
			VerifyEqual(list, reference);
		},
		[&]() {
			const int v = distInt(rng);
			list.push_front(v);
			reference.push_front(v);
			VerifyEqual(list, reference);
		},
		[&]() {
			const int v = distInt(rng);
			int u = v;
			list.emplace_front(move(u));
			u = v;
			reference.emplace_front(move(u));
			VerifyEqual(list, reference);
		},
		[&]() {
			assert(list.empty() == reference.empty());
			if (!list.empty()) {
				assert(list.front() == reference.front());
				assert(list.back() == reference.back());
			}
			list.pop_front();
			reference.pop_front();
			VerifyEqual(list, reference);
		},
		[&]() {
			assert(list.empty() == reference.empty());
			if (!list.empty()) {
				assert(list.front() == reference.front());
				assert(list.back() == reference.back());
			}
			list.pop_back();
			reference.pop_back();
			VerifyEqual(list, reference);
		},
		[&]() {
			auto itL = begin(list);
			auto itL1 = end(list);
			auto itR = begin(reference);
			auto itR1 = end(reference);
			while (itL != itL1) {
				const int v = distInt(rng);
				*itL = v;
				*itR = v;
				++itL;
				++itR;
			}
			assert(itL == itL1);
			assert(itR == itR1);
			VerifyEqual(list, reference);
		},
	};
	for (size_t i = 0; i < 1000; ++i) {
		const size_t ifn = distInt(rng) % size(fns);
		const size_t cfn = distGeo(rng);
		for (size_t j = 0; j < cfn; ++j) {
			fns[ifn]();
		}
	}
}
```

## External indexing
Recall the [External indexing](#external-indexing) method, where we store per-node values separately from the links. I.e. `{ starting node, node 1 next, node 2 next, ..., node 1 value, node 2 value, ... }`

To allow for 


```
```

# Trees
Fundamental tree property:
Directed trees
Undirected trees
Parent pointer or branch stack
Unordered trees
Preorder
Inorder
Postorder
Levelorder
TODO

## Succinct static trees
The basic idea here is to define a recursive serialization of a tree: `<node> = '(' <node0> ... <nodeN> ')'`. I.e. define `(` and `)` as open/close tags denoting a tree node, and `<nodeI>` form the children of that node. Hence we can nest indefinitely, forming any arbitrary tree. If we wanted to store additional data per-node inline, we could serialize extra data into the open/close tag. Or, we could store a separate array of that data, in some order that we can correspond to this structural serialization. This lets us separate the structure of the tree and encode it succinctly. The key insight here is that we can use a `0` bit for the open tag, and a `1` bit for the close tag, so we only require a bitmap with two bits per node. E.g. to encode
```
    0
  1   2
     3 4
```
We can write
```
(()(()()))
0112334420
```
Where I've written the associated node underneath each tag. Converting the parentheses tags to bitmap form looks like:
```
(()(()()))
0010010111
```
Note that this is a preorder tree traversal to generate the open tags. The close tags are a simultaneous postorder traversal. So to serialize, the recursive version looks like:
```
template<typename T> struct Node { vector<Node*> children; T value; };
// Serialize the given `node` tree into `bitmap` (encoding tree structure) and `preorder` (encoding per-node data in preorder).
template<typename T> void Serialize(Node<T>* node, vector<bool>& bitmap, vector<T>& preorder) {
	if (!node) return;
	bitmap.push_back(false);
	preorder.push_back(node->value);
	for (const auto& child : node->children) {
		Serialize(child, bitmap, preorder);
	}
	bitmap.push_back(true);
}
```
To parse the bitmap, maintain a stack of nodes. On `0`, make a new child node of the current stack top and push it onto the stack. On `1`, finish the current stack top and pop it.
```
template<typename T> struct Node { vector<Node*> children; T value; };
// Deserialize the given `bitmap` (encoding tree structure) and `preorder` (encoding per-node data in preorder) into a preorder tree stored in `result` (root is `result[0]`).
template<typename T> void Deserialize(const vector<bool>& bitmap, span<T> preorder, vector<unique_ptr<Node>>& result) {
	assert(bitmap.size() % 2 == 0);
	assert(preorder.size() == bitmap.size() / 2);
	result.clear();
	if (bitmap.empty()) return;
	result.resize(bitmap.size() / 2);
	stack<Node*> s;
	size_t ipreorder = 0;
	for (const auto& b : bitmap) {
		if (b) {
			assert(ipreorder < preorder.size());
			auto node = make_unique<Node>(preorder[ipreorder]);
			if (!s.empty()) {
				auto* parent = s.top();
				parent->children.push_back(node.get());
			}
			s.push(node.get());
			result[ipreorder] = move(node);
			++ipreorder;
		}
		else {
			assert(!s.empty());
			s.pop();
		}
	}
	assert(s.empty());
}
```

## Left-child only binary tree encoding
The key idea here is to eliminate the `right` pointer, and instead infer that 
TODO

## Label isomorphic subtrees
The problem here is to assign unique IDs to unique subtrees. 

E.g. assign `0` to null, `1` to a leaf, `2` to a parent with one left child, `3` to a parent with one right child, `4` to a parent with two children, `5` to a grandparent with `2` as left child, `6` as grandparent with `2` as right child, ...

```
struct Node { Node* left; Node* right; };
void LabelIsomorphicSubtrees(Node* root, unordered_map<Node*, size_t>& result) {
	size_t uid = 1;
	unordered_map<pair<size_t, size_t>, size_t> dedupeTable;
	auto LabelRecur = [&](Node* t) -> size_t {
		if (!t) return 0;
		const size_t L = LabelRecur(t->left);
		const size_t R = LabelRecur(t->right);
		const auto LR = make_pair<size_t, size_t>(L,R);
		auto it = dedupeTable.find(LR);
		size_t id = 0;
		if (it != end(dedupeTable)) {
			id = *it;
		}
		else {
			id = uid;
			++uid;
			dedupeTable[LR] = id;
		}
		result[t] = id;
		return id;
	}
	result.emplace(nullptr, 0u);
	LabelRecur(root);
}
```

# Forests
Directed forests
Undirected forests
Useful for copy-on-write (aka COW)
TODO

# Directed Acyclic Graphs (aka DAGs)
TODO

# Graphs
TODO
Families of connectivity/edge implementations:
1. [Adjacency matrix](#adjacency-matrix)
2. [Adjacency lists](#adjacency-lists)
3. [Implicit/formulaic](#implicitformulaic)
4. 
Directed graphs
Undirected graphs

## Adjacency matrix
Index the nodes in the graph as `{ 0, ..., N-1 }`. Then define a matrix `E(i,j)` which stores information about the edge from node `i` to `j`. Note that `E` is a square matrix, with forward direction edges stored in the lower triangle `i < j`, reverse direction edges stored in the upper triangle `i > j`, and self-loop edges stored in the diagonal `i == j`.

If the graph is an undirected graph, note that the matrix would be symmetric, `E(i,j) == E(j,i)`, because there's no distinction between forward/reverse edges. So for storage efficientcy we can simplify this down to a triangular matrix, only storing `E(i,j)` for `i <= j`. Access becomes `E(min(i,j), max(i,j))`. For efficient 1D storage, see [Lower triangular, row-wise](#lower-triangular-row-wise).

If the graph is undirected and also doesn't allow self-loops, we can eliminate the diagonal as well. Effectively only storing `E(i,j)` for `i < j`. For efficient 1D storage, see [Lower triangular, excluding diagonal, row-wise](#lower-triangular-excluding-diagonal-row-wise)

## Adjacency lists
TODO

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

## Topological sort
TODO
	
## Minimum spanning tree
TODO

## Shortest path
TODO

## Minimum cut (aka maximum flow)
TODO

## Connected components
TODO

## Bipartite matching
TODO

# Hypergraphs (aka multigraphs)
The defining feature of graphs is that the edges are pairs of nodes: (source node, destination node). If the edges are directed, then the order matters. Otherwise the edges are undirected, and the order doesn't matter.

What makes hypergraphs different is that instead of edges being pairs, they are triples, quads, and so on. Limitations on the number of nodes in a hyperedge forms classes of hypergraphs. For instance, `k`-hypergraphs contain `k`-hyperedges that are tuples of `k` different nodes.

## Directed hyperedges
Direction is much more interesting for hypergraphs. Regular graph edges only have two permutations, `(A,B)` or `(B,A)`. On the other hand, 3-hyperedges have `P(3,3) = 3! = 6` permutations:
```
A,B,C
A,C,B
B,A,C
B,C,A
C,A,B
C,B,A
```
This gets even more dramatic for large `k` `k`-hyperedges. In a sense we're encoding directionality within the subset of nodes in the hyperedge with a specific permutation.

Also note that self-loops are more complex; in any of the permutations, we could have a node duplicated, triplicated, etc. all the way up to `(A,A,...,A)` that's all one node.

If you let `k` become unbounded and arbitrary, then notice how one hyperedge is effectively encoding a path in a graph. E.g. `(A,B,C)` hyperedge encodes `(A,B)`, `(B,C)` graph edges. So with a set of hyperedges, we can equivalently encode a graph. If you consider all possible subsets of hyperedges in a hypergraph, each of those defines a graph. With `E` hyperedges, there are `2^E` possible subsets and hence that many graphs encoded into the hypergraph. Hence why the name *hyper*-graph; it's an exponentially larger superset of graphs.

## Undirected hyperedges
Undirected hypergraphs are somewhat more common, which is where we ignore the ordering of edges in a hyperedge, and just treat it as a set. I.e. collapse all the above `(A,B,C)` permutations down to one, `(A,B,C)`. There's still an exponential number of hyperedges that could exist, but no longer the additional exponentiality of all permutations as well.

Note that undirected hypergraphs form a generalization of partitioning. With graph partitioning, you assign each node to some bucket, and the buckets form the partition. In undirected hypergraphs, we could represent that partition by saying we have a hyperedge for each bucket, and the nodes within each bucket form that hyperedge. The key partition property is that each node belongs to only one hyperedge. However, we could drop that property and thus allow assigning one node to multiple buckets, or to no bucket. I.e. we can use hyperedges to encode all possible buckets.

Adjacency in an undirected hypergraph is defined as: nodes within an undirected hyperedge are fully connected. That is, for `(A,B,C)`, the adjacency lists look like:
```
A: B,C
B: A,C
C: A,B
```
This is a significantly more efficient way of encoding fully connected subgraphs than traditional adjacency matrix or adjacency list representations. So if your graph can be mostly decomposed into fully connected subgraphs, using this undirected hyperedge model can significantly reduce memory usage. You can combine a traditional representation of graph edges alongside hyperedges to represent the fully connected subgraphs into a hybridized graph.

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
TODO

# Instructions
TODO

# Execution state
TODO

# Instruction parallelism (SIMD)
TODO

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
