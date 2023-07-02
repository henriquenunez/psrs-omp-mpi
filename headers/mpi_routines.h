#ifndef _MPI_ROUTINES_
#define _MPI_ROUTINES_

#include "defines.h"

void all_to_all_main(Slice data, Slice samples);
void all_to_all(size_t P, size_t N);
void distribute_samples_and_slices(Slice data, Slice regular_samples, size_t P);

#endif