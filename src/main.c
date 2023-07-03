/*
Parallel Sorting with Random Samples
Developed as the second assignment for the SSC0903 subject at ICMC-USP.
July 2023
*/

/* Trabalho 2 - Computacao de Alto Desempenho (SSC0903) */
/* Alunos:
* Carolina Mokarzel               N USP 11932247
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

#include <omp.h>
#include <mpi.h>

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
 
  N = atoi(argv[1]); // Array size

  if (_rank == 0)
  {
    int *arr = create_array(N); // Create array
    // For debugging
    //int arr[] = {15, 46, 48, 93, 39, 6, 72, 91, 14, 36, 69, 40, 89, 61, 97, 12, 21, 54, 53, 97, 84, 58, 32, 27, 33, 72, 20};
    
    Slice data_s = {arr, N};
    
    int *final_slice_ptr = (int *) calloc(N, sizeof(int));
    Slice final_slice = {final_slice_ptr, N};

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

    int *regular_samples = (int *) malloc((P - 1) * sizeof(int));
    Slice regular_samples_s = {regular_samples, P - 1};

    local_sort_and_sample(data_s, regular_samples, P);

    // printf("Samples: \n");
    // print_slice(regular_samples_s);
    
    Slice root_slice = all_to_all_main(data_s, regular_samples_s, P, N, _rank);

    size_t *slices_sizes = (size_t *) malloc(sizeof(size_t) * P);    
    int *slices_sizes_int = (int *) malloc(sizeof(int) * P);
    
    MPI_Gather(&root_slice.size, 1, MPI_UNSIGNED_LONG, slices_sizes, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    for(int i = 0; i < P; i++) slices_sizes_int[i] = slices_sizes[i];

    slices_sizes_int[0] = root_slice.size;
    //slices_sizes[0] = 0;

    // print slices_sizes
    // for(int i = 0; i < P; i++) printf("slices_sizes[%d]: %d\n", i, slices_sizes_int[i]);
    
    // Calculate displacements.
    int *displs = (int *) calloc(P, sizeof(int));
    displs[0] = 0;
    for(int i = 1; i < P; i++)
    {
      //if(i - 1 == 0) displs[i] += displs[i - 1] + root_slice.size;
      //else displs[i] += displs[i - 1] + slices_sizes[i - 1];
      displs[i] = displs[i - 1] + slices_sizes_int[i - 1];
      // printf("displs. %d + %d = %d\n", displs[i - 1], slices_sizes_int[i - 1], displs[i]);
    }

    // printf("displs:\n");
    // for(int i = 0; i < P; i++) printf("displs[%d]: %d\n", i, displs[i]);

    for(int i = 0; i < root_slice.size; i++) // Copying root data
      ((int *) final_slice.ptr)[i] = ((int *) root_slice.ptr)[i];

    MPI_Gatherv(root_slice.ptr, root_slice.size, MPI_INT, final_slice.ptr, slices_sizes_int, displs, MPI_INT, 0, MPI_COMM_WORLD);
    // print_slice(final_slice);
    
    #pragma omp parallel num_threads(T)
    {
      // it's simple to compute slices cus we know the sizes already...
      int thread_num = omp_get_thread_num();
      
      size_t buffer_offset = 0;
      for (int i = 0 ; i < thread_num ; i++)
      {
        buffer_offset += slices_sizes_int[i];
      }
      
      int* start_slice = ((int*)final_slice.ptr) + buffer_offset; // Compute the offset
      
      Slice thr_slice = {start_slice, slices_sizes_int[thread_num]};
      slowsort(thr_slice);
      
      // #pragma omp critical
      // {
      //   print_slice_rank(thr_slice, thread_num);
      // }
    }
    
    // print_slice_rank(final_slice, 0);
    print_slice(final_slice);
    free(arr);
  }
  else
  {
    all_to_all(P, N, _rank);
  }
  
  MPI_Finalize();
  
  return 0;
}
