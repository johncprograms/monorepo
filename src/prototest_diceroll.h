// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

dice roll problem:
given a 5-sided die, simulate a 7-sided die.

// given:
int rand5();

int ChangeRadix(symbol* values, size_t N, int in_radix, int out_radix)
{

}

int rand7()
{
  // 5 and 7 are relatively prime ( both are primes ), so presumably we'll want 7*5=35 as a conversion.
  // if we roll the 5-die 7 times, can we then interpret that as 5 rolls of a 7-die?
  // the pmf of each die is very simple:
  // pmf5(x) = 1/5, for x in { 0, 1, 2, 3, 4 }
  // pmf7(x) = 1/7, for x in { 0, 1, 2, 3, 4, 5, 6 }
  // can we just sum the values of the 7 5-die rolls?
  // the LLN says summations of RVs converge on a gaussian distribution, so likely not.
  // yeah, the variance characteristics change; rolling all 1s is more unlikely than all middle values.
  // so summation is out; what about bit concatenation? that should prevent the mixing of addition.
  // problem is: 5 and 7 aren't powers of 2.
  // so can we use custom radix systems instead of 2? maybe base-35 is convertible to both 5 and 7?
  // (r, x0, x1, ..., xN-1) = r^0 x0 + r^1 x1 + ... + r^(N-1) xN-1
  // oh right, the radix systems are just encodings; the underlying integers are still integers.
  // well i guess it would help to make bit concatenation and slicing work, right?
  // i think actually no, we'd want gcd(5,7) as the radix to do nice bit concat/slicing.
  // we can't break apart a base-35 symbol into anything smaller, right? or can we?
  // yeah we can. so i think this approach would work.

  // what about the inverse CDF approach?
  // cmf7(x) = {
  //   1/7 for x == 0
  //   2/7 for x == 1
  //   3/7 for x == 2
  //   4/7 for x == 3
  //   5/7 for x == 4
  //   6/7 for x == 5
  //   7/7 for x == 6

  // inverse-cmf7(p) = {
  //   0 for p <= 1/7
  //   1 for p <= 2/7
  //   2 for p <= 3/7
  //   3 for p <= 4/7
  //   4 for p <= 5/7
  //   5 for p <= 6/7
  //   6 for p <= 7/7

  // If we can generate appropriate uniformly-distributed p values, then inverse-cmf7(p) is the 7-die roll result.
  // This doesn't really solve this problem directly, since the p value is the hard part.

  // Well so we could just discard bits, right?
  // For efficiency, that might be fine.
  // E.g. we could get 8 uniformly random bits with: rand5() & 0b11 << 4 | rand5 & 0b11
  // We really only need 3 uniformly random bits, and rand5 generates at least 2.
  //
  //   int upper_bit = rand5() & 0b1;
  //   int lower_bits = rand5() & 0b11;
  //   int rand8 = upper_bit << 2 | lower_bits;
  //
  // Actually, darn. The lower bits aren't uniformly random.
  // E.g. 4 = 0b100, so the lower bits 0b00 are more common in 5-die roll results.
  // So discarding bits is out, unless we do it in a uniform/unbiased way. E.g. retry to ignore 4.
  // Well that would work.
  //

  int rand7;
  for (;;) {

    int unbiased_rand4;
    for (;;) {
      unbiased_rand4 = rand5();
      if (unbiased_rand4 != 4) break;
    }
    int unbiased_rand2;
    for (;;) {
      unbiased_rand2 = rand5();
      if (unbiased_rand2 != 4) break;
    }
    // Discard the 2's place bit.
    // TODO: This is wasteful; it'd be better to save that for subsequent rand7 invocations.
    unbiased_rand2 &= 0b1;

    int rand8 = unbiased_rand2 << 2 | unbiased_rand4;

    // Now how do we get from rand8 to rand7?
    // We can't force 7=0b111 into anything in [0, 6], since that would make things biased.
    // So we do the same retry strategy, I suppose.
    if (rand8 == 7) continue;

    rand7 = rand8;
    break;
  }

  return rand7;
}

#endif
