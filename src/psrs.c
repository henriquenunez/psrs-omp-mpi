#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <omp.h>

#include "../headers/defines.h"
#include "../headers/psrs.h"
#include "../headers/slice.h"

void swap(int* a, int* b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

int partition(int arr[], int low, int high) {
  int pivot = arr[high];
  int i = (low - 1);

  for (int j = low; j <= high - 1; j++) {
    if (arr[j] < pivot) {
      i++;
      swap(&arr[i], &arr[j]);
    }
  }

  swap(&arr[i + 1], &arr[high]);
  return (i + 1);
}

void _slowsort(int arr[], int low, int high) {
  if (low < high) {
    int pi = partition(arr, low, high);
    _slowsort(arr, low, pi - 1);
    _slowsort(arr, pi + 1, high);
  }
}

void slowsort(Slice s) {
  _slowsort((int *) s.ptr, 0, s.size - 1);
}

// NOTE: samples_out must be P * P long
void local_sort_and_sample(Slice s, int *samples_out, size_t P)
{
  size_t samples_size = P * P;

	// Divide array by the number of threads
	int *data = (int *) s.ptr;
	size_t len = s.size;
	size_t per_thread_len = (int) ceil(len / (float) P);
	
	int* samples = (int *) malloc(samples_size * sizeof(int));
	
	// Parallel region with shared array
  // #pragma omp parallel num_threads(P) shared(data, samples)
  #pragma omp parallel num_threads(T) shared(data, samples)
  {
    int tid = omp_get_thread_num(); // Starts at 0
    size_t start_idx = tid * per_thread_len;
    size_t stop_idx = ((tid + 1) * per_thread_len) - 1;

    // Adjust the last thread's stop index to include remaining elements
    if (tid == T - 1) {
      stop_idx += samples_size % T;
    }

    int j = tid * P;
    // Step: n/p**2 (size of the array / number of processes ** 2)
    for(int i = start_idx; i < stop_idx; i += len / (P * P))
    {
      samples[j++] = data[i];
    }
  }

	// Sort the samples
  Slice sample_slice = {samples, samples_size};
	slowsort(sample_slice);
  // print_slice(sample_slice);

  int i;
	// Resample the samples
	for (i = 1 ; i <= P - 1 ; i++)
	{
		size_t sample_index = (i * P) + (P / 2) - 1;
		samples_out[i-1] = samples[sample_index];
	}

	free(samples);
}
