#ifndef _DEFINES_
#define _DEFINES_

#include <stdio.h>

// #define T 8
#define T 3
#define MAX_INT_VALUE_ARR 10000

typedef struct _Slice
{
  void* ptr;
  size_t size;
} Slice;

typedef struct _Range
{
  int start, stop;
} Range;

#endif