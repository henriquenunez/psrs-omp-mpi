#ifndef _MPI_ROUTINES_
#define _MPI_ROUTINES_

#include "defines.h"

Slice _all_to_all(Slice local_data_s, Slice pivots_s, int P, int rank);
Slice all_to_all_main(Slice data, Slice samples, size_t P, size_t N, int rank);
void all_to_all(size_t P, size_t N, int rank);
void distribute_samples_and_slices(Slice data, Slice regular_samples, size_t P);

#endif