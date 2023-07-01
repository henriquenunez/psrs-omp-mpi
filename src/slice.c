#include <stdio.h>

#include "../headers/defines.h"

void print_slice(Slice s) {
  for (int i = 0; i < s.size; i++) {
    printf("%d ", ((int*)s.ptr)[i]);
  }
  printf("\n");
}