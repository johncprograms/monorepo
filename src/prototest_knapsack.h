// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

knapsack problem:
given a catalogue of items with given weight and price, and a maximum weight capacity of your knapsack,
pick the number of each item to steal which maximize price and fit in your knapsack.
simplified: just return the maximum price.

struct
item_t
{
  int weight; // assume non-negative
  int price; // assume non-negative
};

// The greedy algorithm is to compute ratio = price / weight, and then pick items in ratio order as they fit.
// That's not going to be optimal for huge ratio differences, where we really want something in between.
// This will only really be a problem when the knapsack is mostly-full.

// The full exponential search will be optimal, although potentially extremely slow.

int MaxValue(
  item_t* items, // array of length N, containing initialized item_t objects
  size_t N,
  int capacity) // capacity of the knapsack.
{
  assert(capacity > 0);

  int max_value = INT_MIN;
  for (size_t i = 0; i < N; ++i) {
    // if we pick items[i], then we have less capacity for other things.
    auto weight_i = items[i].weight;
    auto value_i = items[i].price;
    if (weight_i > capacity) {
      // wouldn't fit in the knapsack anymore.
      continue;
    }
    auto capacity_after_i = capacity - weight_i;
    auto value = value_i;
    if (capacity_after_i) {
      // Only look for more things if we have remaining capacity.
      // This slightly saves on function call overhead
      value += MaxValue(items, N, capacity_after_i);
    }
    if (value > max_value) {
      max_value = value;
    }
  }
  if (max_value == INT_MIN) {
    return 0;
  }
  return max_value;
}

// Note we could turn this into a dynamic programming memoizing solution, by solving
// for all possible capacity values. That's potentially silly if capacity is huge, but it'd be a bound.
// The idea being:
// Memoize MaxValue(capacity)
// We know that MaxValue(c) = max over i of ( value_i + MaxValue(c - weight_i) )
// And MaxValue(0) = 0 by convention.
// So we could build up from capacity=0 up to whatever capacity we want:

MaxValue[0] = 0;
for (size_t capacity = 1; capacity <= target_capacity; ++capacity) {
  int max_value = INT_MIN;
  for (size_t i = 0; i < N; ++i) {
    auto value_i = items[i].price;
    auto weight_i = items[i].weight;
    if (weight_i > capacity) {
      // doesn't fit.
      continue;
    }
    auto value = value_i + MaxValue[capacity - weight_i];
    if (value > max_value) {
      max_value = value;
    }
  }
  MaxValue[capacity] = max_value;
}
return MaxValue[target_capacity];

// Well, presumably you'd cache this memoization with the item list, and support variable-capacity queries.
// That's the whole point of memoization here, really.

#endif
