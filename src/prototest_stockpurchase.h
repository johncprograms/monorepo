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

auto max_i = 0;
for i in [0, prices.len - 1)
  auto max_j = prices[i];
  for j in [i + 1, prices.len)
    max_j = max(max_j, prices[j])
  max_i = max(max_i, max_j)
return max_i;

max_j[i] = max prices[j] over j in [i + 1, prices.len)
max_i = max max_j[i] over i in [0, prices.len)



Given
daymax[t]
daymin[t]

buy(t) = daymax[t]; // pessimistic assumption
sell(t) = daymin[t]; // pessimistic assumption
profit(t0,t1) = sell(t1) - buy(t0) // assume t0 < t1, sell after buy.

// That's how you calculate profits on known t0, t1.
// How do you predict the optimal t0,t1 ?
// If you predict them separately, does that lose information? Should we predict both at once?
// If not, you'd have to predict t0, and then t1 given the chosen t0.
// That's basically the above problem structure. I wonder if we can do better.
// You could trivially invert: predict t1, and then t0 given that t1. Kind of silly, but okay.
// 
// Maybe we could do historical analysis of optimums like this, and see what comes out?
// I'm guessing there would be a lot of 'buy at really old t0, sell at now', because of a few things:
// - stocks that are live now are very likely to have risen (inflation, especially over decades)
// - high inflation now means many stocks are at all-time highs.
// So probably this kind of analysis is most interesting for stocks that crashed and died.
//
// I think more interesting would be a fixed investment cap assignment optimization.
// E.g. given X dollars, which stocks are rising the most right now, so you can hold those?
// There's a risk problem there; stocks rising crazy fast are likely active scams.
// You don't want to be caught holding the bag. So we'd want some constraints on the optimization.
// E.g. max derivative, min volume, min company metrics, min company lifetime, etc.
//
// Since it's discrete functions, we'd want the discrete derivative of every stock at all time.
// d/dt S(t) = approximately ( S(t) - S(t - 1) ) / 1
// Or some similar difference equation to approximate the derivative.
//
// The sloppiness / rounding / averaging here affects our timescales. 
// Choosing a timescale for the differencing here is interesting. Too small is noisy; too large will miss details.
// This is likely a kind of hyper-parameter we'd want to tune.
//
// Given the discrete derivatives of all stocks we'd consider, the stock to hold at any given time t is:
// argmax d/dt s(t) over s in S.
// Note this changes over time, and at every change-over point, you'd want to sell the old stock and buy the new one.
// The act of buying/selling has consequences on the price, so we'd want to factor in things like the pessimistic assumption:
// 'Buy at the daymax, Sell at the daymin'. Probably we could be more accurate than that.
// Plus there's the risk / likelihood of even being able to make the sell and the buy successfully (and at the evaluated price).
// 
// Note there's also a different risk, which is that once you buy, there's risk in holding against your choice (sell might not happen for a long time, if ever).
// That spans two of these change-over points, so that's a different kind of analysis.
// There's also likely reasonable decisions to let that investment sit for a while until it's easier to sell (if that's likely).
//
// IDEA: compute a bunch of timescales at once for the derivatives.
// d/dt s at timescale k can be defined as: D(s,k) = ( s(t) - s(t - k) ) / k
// or a similar difference equation.
// Then we could, instead of picking the maximum D(s,1), pick the maximum linear combination of D(s,k), where the coefficients
// are hyper-parameters that we tune. (Or maybe parameters based on other metrics, e.g. macroeconomic)
// I'd expect something like an exponential falloff here. At least a power law falloff (more recent is a higher coefficient).
// 
// Some critiques of this strategy:
// 1. We're relying on past behavior to predict the future. That's not really true in theory, maybe at any timescale.
//    Except maybe to the extent that other folks trading in the market behave that way.
// 2. Holding a single max stock is risky. Probably we'd want to mitigate by diversifying across the next best N optimal stocks. Probably power law / exponential.
// 


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
