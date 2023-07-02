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