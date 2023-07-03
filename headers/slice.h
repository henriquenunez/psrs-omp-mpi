#ifndef _SLICE_
#define _SLICE_

void print_slice(Slice s);
void print_slice_rank(Slice s, int rank);
Slice split_data(Slice data_s, int i, int P);

#endif