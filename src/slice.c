#include <stdio.h>

#include "../headers/defines.h"
#include "../headers/slice.h"

void print_slice(Slice s) {
  int i;
  for(i = 0; i < s.size - 1; i++) {
    printf("%d, ", ((int *) s.ptr)[i]);
  }

  printf("%d.", ((int *) s.ptr)[i]);
}

void print_slice_rank(Slice s, int rank) {
  printf("[%d] ", rank);
  for (int i = 0; i < s.size; i++) {
    printf("%d ", ((int *) s.ptr)[i]);
  }

  printf("\n");
}

Slice split_data(Slice data_s, int i, int P)
{
  int N = data_s.size;
  int thread_num = i;
  
  int low = thread_num * (N / P);
  int high = (thread_num + 1) * (N / P) - 1;

  // Adjust the last thread's high index to include remaining elements
  if (thread_num == P - 1) {
    high += N % P;
  }

  size_t slice_size = high - low + 1;
  Slice thr_slice = {&((int *) data_s.ptr)[low], slice_size};

  return thr_slice;
}