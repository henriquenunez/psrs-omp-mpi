#ifndef _PSRS_
#define _PSRS_

void swap(int* a, int* b);
int partition(int arr[], int low, int high);
void _slowsort(int arr[], int low, int high);
void slowsort(Slice s);
void local_sort_and_sample(Slice s, int* samples_out, size_t P);
void parallel_merge(int** vecs, size_t P);

#endif