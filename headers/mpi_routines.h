#ifndef _MPI_ROUTINES_
#define _MPI_ROUTINES_

Range get_range(Slice local_data_s, int* samples_to_index, int j, int P);
int *index_samples(Slice data_s, Slice samples);
Slice _all_to_all(Slice local_data_s, Slice pivots_s, int P, int rank);
Slice all_to_all_main(Slice data, Slice samples, size_t P, size_t N, int rank);
void all_to_all(size_t P, size_t N, int rank);

#endif