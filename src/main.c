/*
Parallel Sorting with Random Samples
Developed as the second assignment for the SSC0903 subject at ICMC-USP.
July 2023
*/

/* Trabalho 2 - Computacao de Alto Desempenho (SSC0903) */
/* Alunos:
* Carolina Mokarzel               N USP 
* Felipi Adenildo Soares Sousa    N USP 10438790
* Gustavo Romanini Gois Barco     N USP 10749202
* Henrique Hiram Libutti Nunez    N USP 11275300
* Joao Alexandro Ferraz           N USP 11800441
* Luiz Fernando Rissotto de Jesus N USP 11200268
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
#include "../headers/slice.h"
#include "../headers/mpi_routines.h"

// Global parameters
size_t P;
size_t N;

int *create_array(size_t arr_size) {
  int *arr = (int *) malloc(sizeof(int) * arr_size);
  
  unsigned int seed_arr[T];

  int i;

  // Seeds are necessary, because without them all the threads would generate
  // the same random numbers
  for(i = 0; i < T; i++) {
    seed_arr[i] = (unsigned int) rand() % 100; // Vector of seeds
  }

  // Generates pseudo random numbers using a seed for each thread
  #pragma omp parallel num_threads(T)
  {
    int thread_number = omp_get_thread_num();

    srand(seed_arr[thread_number]);
    
    #pragma omp for private(i)
    for(i = 0; i < arr_size; i++) {
      arr[i] = rand() % MAX_INT_VALUE_ARR;
    }
  }

  return arr;
}

int main(int argc, char* argv[])
{
  if (argc < 2) return -1; // return 1?

  // Initialize MPI
  MPI_Init(&argc, &argv);
  int _p;
  int _rank;

	MPI_Comm_size(MPI_COMM_WORLD, &_p);
  P = (size_t) _p;

	MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
	//MPI_Get_processor_name(processor_name, &namelen);
 
  N = atoi(argv[1]); // Array size
  
  // One buffer for each process
  // int **aggregated_buffers = (int **) malloc(P * sizeof(int *));

  if (_rank == 0)
  {
    // Create array
    // int *arr = create_array(N);
    // For debugging
    int arr[] = {15, 46, 48, 93, 39, 6, 72, 91, 14, 36, 69, 40, 89, 61, 97, 12, 21, 54, 53, 97, 84, 58, 32, 27, 33, 72, 20};
    Slice data_s = {arr, N};

    #pragma omp parallel num_threads(T) // Step (a) to (b)
    {
      int thread_num = omp_get_thread_num();
      int low = thread_num * (N / T);
      int high = (thread_num + 1) * (N / T) - 1;

      // Adjust the last thread's high index to include remaining elements
      if (thread_num == T - 1) {
        high += N % T;
      }

      size_t slice_size = high - low + 1;
      Slice thr_slice = {&arr[low], slice_size};

      slowsort(thr_slice);
    }
    
    // printf("Data after step 1:\n");
    // print_slice(data_s);

    P = 3; // REMOVER DEPOIS !! (DEBUGGING)

    int *regular_samples = (int *) malloc((P - 1) * sizeof(int));
    Slice regular_samples_s = {regular_samples, P - 1};

    // local_sort_and_sample(data_s, regular_samples_s, P);
    local_sort_and_sample(data_s, regular_samples, P);

    // printf("Samples: \n");
    // print_slice(regular_samples_s);

    /* --------------------------------- OK --------------------------------- */
    
    // Distribute samples with MPI
    //distribute_samples_and_slices(data_s, regular_samples_s, P);
    
    // all_to_all_main(data_s, regular_samples);
    
    // // The main process prints the sorted vector
    // print_slice(s);
    
    all_to_all_main(data_s, regular_samples_s, P, N, _rank);
  }
  else
  {
    all_to_all(P, N, _rank);
  }
  // else
  // {
  //   // Perform the all-to-all data exchange and merge.
  //   all_to_all();
  // }
  
  // // Finally, the main process needs to merge the data.
  
  // // Merge data again
  // parallel_merge(aggregated_buffers);

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

// #include "mpi_routines.c"
// #include "psrs.c"
// #include "slice.c"
