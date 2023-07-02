// Copyright (c) John A. Carlos Jr., all rights reserved.

#if 0

"increasing subarray" is a span of an array where elements in the span are strictly increasing in value.
1. write a program to find the length of the longest such span, given an integer array.
2. write a program to return the longest such span.
2. write a program to return the number of such longest spans, as well as the span length they all have.
E.g. 
A = { 1, 3, 4, 2, 7, 5, 6, 9, 8 }
      >.....<  >..<  >.....<  =

// Finds the length of the longest increasing subsequence, starting at index 0.
// We'll want a higher level iteration to find the multiple subsequences.
size_t
problem1_index0(int* data, size_t data_len)
{
  if (!data_len) return 0;
  int start = data[0];
  int previous = start;
  for (size_t i = 1; i < data_len; ++i) {
    int subsequent = data[i];
    if (previous >= subsequent) {
      return i;
    }
    // Advance the trailing iterator.
    previous = subsequent;
  }
  return data_len;
}

size_t
problem1(int* data, size_t data_len)
{
  size_t max_subsequence_len = 0;
  while (data_len) {
    size_t subsequence_len = problem1_index0(data, data_len);
    assert(subsequence_len);
    assert(subsequence_len <= data_len);
    data += subsequence_len;
    data_len -= subsequence_len;
    max_subsequence_len = max(max_subsequence_len, subsequence_len);
  }
  return max_subsequence_len;
}

void
problem2(int* data, size_t data_len, size_t* result_start_index, size_t* result_len)
{
  assert(result_start_index);
  assert(result_len);
  *result_start_index = 0;
  *result_len = 0;
  auto original_data = data;
  size_t max_subsequence_len = 0;
  while (data_len) {
    size_t subsequence_len = problem1_index0(data, data_len);
    assert(subsequence_len);
    assert(subsequence_len <= data_len);
    data += subsequence_len;
    data_len -= subsequence_len;
    // Note we'll just return the first one, hence strictly less-than here.
    if (max_subsequence_len < subsequence_len) {
      *result_start_index = data - original_data;
      *result_len = subsequence_len;
    }
  }
}

void
problem3(int* data, size_t data_len, size_t* result_len, size_t* num_maxes)
{
  assert(result_start_index);
  assert(result_len);
  *result_len = 0;
  *num_maxes = 0;
  size_t max_subsequence_len = 0;
  while (data_len) {
    size_t subsequence_len = problem1_index0(data, data_len);
    assert(subsequence_len);
    assert(subsequence_len <= data_len);
    data += subsequence_len;
    data_len -= subsequence_len;
    if (max_subsequence_len < subsequence_len) {
      *num_maxes = 1;
      *result_len = subsequence_len;
    } else if (max_subsequence_len == subsequence_len) {
      *num_maxes = *num_maxes + 1;
    }
  }
}

#endif
