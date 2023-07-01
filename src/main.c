/*
Parallel Sorting with Random Samples
Developed as the second assignment for the SSC0903 subject at ICMC-USP.
July 2023
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <mpi.h>

#include <omp.h>

#include "../headers/defines.h"
#include "../headers/psrs.h"

// Global parameters
size_t P;
size_t N;

int main(int argc, char* argv[])
{
  if (argc < 2) return -1;
  // Initialize MPI
  MPI_Init(&argc, &argv);
  int _p, _rank;
	MPI_Comm_size(MPI_COMM_WORLD, &_p);
  P = (size_t) _p;
	MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
	//MPI_Get_processor_name(processor_name, &namelen);
 
  N = atoi(argv[1]);
  
  // One buffer for each process
  int** aggregated_buffers = (int**) malloc(P * sizeof(int*));
 
  if (_rank == 0)
  {
    // Create array
    int* arr = create_array(N);
    Slice data_s = {arr, N};
    int* regular_samples = (int*) malloc(P-1);
    Slice regular_samples_s = {regular_samples, P-1}

    local_sort_and_sample(data_s, regular_samples_s);

    // Distribute samples with MPI
    distribute_samples_and_slices(data_s, regular_samples_s, P);
    
    all_to_all_main(data_s, regular_samples);
    
    // The main process prints the sorted vector
    print_slice(s);
  }
  else
  {
    // Perform the all-to-all data exchange and merge.
    all_to_all();
  }
  
  // Finally, the main process needs to merge the data.
  
  // Merge data again
  parallel_merge(aggregated_buffers);

  MPI_Finalize();
  
  return 0;
}

/*
int test_slowsort() {
  int arr[] = { 7, 2, 1, 6, 8, 5, 3, 4 };
  int size = sizeof(arr) / sizeof(arr[0]);
  Slice s = {arr, size};
  printf("Original array: ");
  print_slice(s);

  slowsort(arr, 0, size - 1);

  printf("Sorted array: ");
  print_slice(s);

  return 0;
}
*/
