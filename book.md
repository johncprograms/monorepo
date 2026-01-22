
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
  - [And](#and)
  - [Or](#or)
  - [Xor](#xor)
  - [Not (aka flip)](#not-aka-flip)
  - [Set](#set)
  - [Clear (or reset)](#clear-or-reset)
  - [Conditional not](#conditional-not)
  - [Conditional set](#conditional-set)
  - [Conditional clear](#conditional-clear)
  - [Population count](#population-count)
  - [Shift Left Logical 0:](#shift-left-logical-0)
  - [Shift Right Logical 0:](#shift-right-logical-0)
  - [Shift Left Logical 1:](#shift-left-logical-1)
  - [Shift Right Logical 1:](#shift-right-logical-1)
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
  - [Arithmetic Shift:](#arithmetic-shift)
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
  - [1D from 2D](#1d-from-2d)
    - [Lower triangular, row-wise](#lower-triangular-row-wise)
    - [Upper triangular, col-wise](#upper-triangular-col-wise)
    - [Upper triangular, row-wise](#upper-triangular-row-wise)
    - [Lower triangular, col-wise](#lower-triangular-col-wise)
    - [Lower triangular, excluding diagonal, row-wise](#lower-triangular-excluding-diagonal-row-wise)
    - [Upper triangular, excluding diagonal, col-wise](#upper-triangular-excluding-diagonal-col-wise)
    - [Upper triangular, excluding diagonal, row-wise](#upper-triangular-excluding-diagonal-row-wise)
- [SCRATCH](#scratch)




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

## And
```
and(0,0) = 0
and(0,1) = 0
and(1,0) = 0
and(1,1) = 1
result = a & b
```
## Or
```
or(0,0) = 0
or(0,1) = 1
or(1,0) = 1
or(1,1) = 1
result = a | b
```
## Xor
```
xor(0,0) = 0
xor(0,1) = 1
xor(1,0) = 1
xor(1,1) = 0
result = a ^ b
```
## Not (aka flip)
```
not(0) = 1
not(1) = 0
result = ~b
```
## Set
By 'set' most programmers mean 'set to 1' in a base 2 context. This is a trivial set to 1.
```
result = 1
```
## Clear (or reset)
By 'clear' or 'reset', most programmers mean 'set to 0' in a base 2 context. This is a trivial set to 0.
```
result = 0
```
## Conditional not
`cnot(b,value)` means: if value is true, flip b. Otherwise return b.
```
cnot(0,0) = 0
cnot(0,1) = 1
cnot(1,0) = 1
cnot(1,1) = 0
```
Note this is the same as `xor(b,value)`. So you can write this in code as:
```
result = b ^ value
```
## Conditional set
`cset(b,value)` means: If value is 0, we want to leave b alone. Else value is 1, we're setting b to 1.
```
result = b | value
```
## Conditional clear
`cclear(b,value)` means: If value is 0, we want to leave b alone. Else value is 1, we're setting b to 0.
```
result = b & ~value
```

## Population count
Counts the number of bits set to 1 in the bitmap.
```
result = popcount(b)
```
## Shift Left Logical 0:
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
a << b
```
## Shift Right Logical 0:
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
a >> b
```
## Shift Left Logical 1:
Same concept as for Left Logical 0, but shifts in 1 bits instead of 0 bits.
```
shiftLeftLogical1(11110000,0) =     1111
shiftLeftLogical1(11110000,1) =    11111
shiftLeftLogical1(11110000,2) =   111111
shiftLeftLogical1(11110000,3) =  1111111
shiftLeftLogical1(11110000,4) = 11111111
shiftLeftLogical1(11110000,5) = 11111111
shiftLeftLogical1(11110000,6) = 11111111
shiftLeftLogical1(11110000,7) = 11111111
shiftLeftLogical1(11110000,8) = 11111111
```
Generally there's no intrinsic for this.
```
TODO
```
## Shift Right Logical 1:
Same concept as for Right Logical 0, but shifts in 1 bits instead of 0 bits.
```
shiftRightLogical1(11110000,0) = 11110000
shiftRightLogical1(11110000,1) = 11111000
shiftRightLogical1(11110000,2) = 11111100
shiftRightLogical1(11110000,3) = 11111110
shiftRightLogical1(11110000,4) = 11111111
shiftRightLogical1(11110000,5) = 11111111
shiftRightLogical1(11110000,6) = 11111111
shiftRightLogical1(11110000,7) = 11111111
shiftRightLogical1(11110000,8) = 11111111
```
Generally there's no intrinsic for this.
```
TODO
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
result = rotl(b,count)
```
For uint64, it would look like:
```
uint64_t rotl(uint64_t b, uint64_t count) {
    count = count % 64;
    return count == 0 ? b : (b << count) | (b >> (64 - count));
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
		result = rotr(b,count)
```
For uint64, it would look like:
```
uint64_t rotr(uint64_t b, uint64_t count) {
    count = count % 64;
    return count == 0 ? b : (b >> count) | (b << (64 - count));
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

## Arithmetic Shift:
TODO

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
## 1D from 2D
### Lower triangular, row-wise
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
### Upper triangular, col-wise
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
### Upper triangular, row-wise
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

### Lower triangular, col-wise
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

### Lower triangular, excluding diagonal, row-wise
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

### Upper triangular, excluding diagonal, col-wise
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

### Upper triangular, excluding diagonal, row-wise
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






# SCRATCH


The pattern here is trickier to define. It's easiest to conceptualize as a sequence of mirrorings. Start with [Lower triangular, row-wise](#lower-triangular-row-wise)
```
0
1 2
3 4 5
6 7 8 9
```
```
index(x,y) = y(y+1)/2 + x
```
Mirror in the x dimension.
```
    0
    2 1
5 4 3
9 8 7 6
```
```
index1(x,y) = index(N-1-x, y)
= y(y+1)/2 + N-1-x
```
Mirror in the y dimension.
```
9 8 7 6
5 4 3
    2 1
    0
```
```
index2(x,y) = index1(x, N-1-y)
= (N-1-y)((N-1-y)+1)/2 + N-1-x
```
And finally, mirror the 1D index itself to reverse the linear order and get to the desired order.
```
0 1 2 3
4 5 6
    7 8
    9
```
```
index3(x,y) = N(N+1)/2 - 1 - index2(x, y)
```

```
{ 0 4 7 9 }
{ 9-9 9-5 9-2 9-0 }


index(x,y) = y(y+1)/2 + x
mirror x and y:
index1(x,y) = index(N-1-x, N-1-y)
= (N-1-y)(N-y)/2 + (N-1-x)
= ((N-1) - y)(N - y)/2 + (N-1) - x
= (N-1)N/2 - (N-1)y/2 - Ny/2 + y^2/2 + (N-1) - x

Reverse the linear index too:
= N(N+1)/2 - 1 - (N-1)N/2 + (N-1)y/2 + Ny/2 - y^2/2 - (N-1) + x
= ((N+1)-(N-1))N/2 - 1 + (N-1+N)y/2 - y^2/2 - (N-1) + x
= (N+1-N+1)N/2 - 1 + (2N-1)y/2 - y^2/2 - N + 1 + x
= (2N-1)y/2 - y^2/2 + x
= (2N-1-y)y/2 + x
```
