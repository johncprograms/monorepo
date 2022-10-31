// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

stock purchase problem:
given a time-indexed stock price array,
find the maximum profit of a single purchase/sale.
extended problem:
find the best buy and sell time to maximize profit of a single purchase/sale.

E.g.
prices = { 10, 7, 5, 8, 11, 9 }
you should return 6, which is 11 - 5.

int Problem(
  int* prices, // array of length N
  int* buffer, // array of length N
  size_t N)
{
  assert(N);
  if (N == 1) {
    return 0;
  }

  //
  // We'll use the buffer array to pre-compute the maximums over [i+1, N).
  // That is,
  //   buffer[i] = max( prices[j] ) for j in [i+1, N).
  //
  buffer[N-1] = prices[N-1];
  for (size_t ri = 0; ri < N - 1; ++ri) {
    auto idx_previously_calculated = N - 1 - ri;
    auto idx_next_calculated = idx_previously_calculated - 1;
    buffer[idx_next_calculated] = max(prices[idx_next_calculated], buffer[idx_previously_calculated]);
  }
//
//    // then find the max profit given that assumption:
//    int max_profit = MIN_INT;
//    for (size_t j = i + 1; j < N; ++j) {
//      int sell_price = prices[j];
//      auto profit = sell_price - buy_price;
//      if (profit > max_profit) {
//        max_profit = profit;
//      }
//    }

  int total_max_profit = MIN_INT;
  for (size_t i = 0; i < N - 1; ++i) {
    // say we buy at time i.
    int buy_price = prices[i];
    int max_sell_price = buffer[i];
    auto max_profit = max_sell_price - buy_price;
    if (max_profit > total_max_profit) {
      total_max_profit = max_profit;
    }
  }
  assert(total_max_profit != MIN_INT);
  return total_max_profit;
}

#endif
