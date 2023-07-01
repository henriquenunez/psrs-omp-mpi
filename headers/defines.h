#ifndef _DEFINES_
#define _DEFINES_

#include <stdio.h>

#define T 8

typedef struct _Slice
{
  void* ptr;
  size_t size;
} Slice;

#endif