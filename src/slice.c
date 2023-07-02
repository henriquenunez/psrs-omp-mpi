#include <stdio.h>

#include "../headers/defines.h"
#include "../headers/slice.h"

void print_slice(Slice s) {
  for (int i = 0; i < s.size; i++) {
    printf("%d ", ((int *) s.ptr)[i]);
  }

  printf("\n");
}

void print_slice_rank(Slice s, int rank) {
  printf("[%d] ", rank);
  for (int i = 0; i < s.size; i++) {
    printf("%d ", ((int *) s.ptr)[i]);
  }

  printf("\n");
}

Slice split_data(Slice data_s, int i, int Total)
{
  int N = data_s.size;
  int thread_num = i;
  
  //#pragma omp parallel num_threads(T) // Step (a) to (b)
  //int thread_num = omp_get_thread_num();
  int low = thread_num * (N / Total);
  int high = (thread_num + 1) * (N / Total) - 1;

  // Adjust the last thread's high index to include remaining elements
  if (thread_num == T - 1) {
    high += N % Total;
  }

  size_t slice_size = high - low + 1;
  Slice thr_slice = {&((int *) data_s.ptr)[low], slice_size};

  return thr_slice;
}