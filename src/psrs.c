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

// Global parameters
size_t P;
size_t N;

#define T 8

typedef struct _Slice
{
  void* ptr;
  size_t size;
} Slice;

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
  _slowsort((int*) s.ptr, 0, s.len - 1);
}

void print_slice(Slice s) {
  for (int i = 0; i < s.size; i++) {
    printf("%d ", ((int*)s.ptr)[i]);
  }
  printf("\n");
}

// NOTE: samples_out must be P * P long
void local_sort_and_sample(Slice s, int* samples_out)
{
	// Divide array by the number of threads
	int* data = (int*) s.ptr;
	size_t len = s.size;
	size_t per_thread_len = (int) ceil(len / (float)P);
	
	int* samples = (int*) malloc(P * P * sizeof(int));
	
	// Parallel region with shared array
  #pragma omp parallel num_threads(T) shared(data)
  {
    int tid = omp_get_thread_num(); // Starts at 0
		size_t start_idx = tid * per_thread_len;
		size_t stop_idx = min(start_idx + per_thread_len, s.b);
		
		// Perform the slowsort in the local data.
		// slowsort(data, );
		
		// Sample the vector
    // TODO: Consider P instead of T (check if it is right)
		for (int i = 0 ; i < P ; i++)
		{
			size_t sample_index = i * (len / (P * P));
			samples[tid * P + P] = data[sample_index];
		}
		
		// We can iterate over the vector now
    //for (size_t i = start_idx; i < start_idx + per_thread_len; i += 1) {
    //}
  }
	
	// Sort the samples
  Slice sample_slice = {samples, samples_size};
	slowsort(sample_slice);
		
	// Resample the samples
	for (int i = 1 ; i <= P-1 ; i++)
	{
		size_t sample_index = i * P + (P / 2) - 1;
		samples_out[i] = samples[sample_index];
	}

	free(samples);
}

/*
This function is always run by rank 0.
*/
void distribute_samples_and_slices(Slice data, Slice regular_samples)
{
  size_t per_thread_len = (int) ceil(data.len / (float)P);

  // Crazy MPI calls here to select the slices
  for (int r = 1 ; r < P ; r++) // We skip rank 0 since it is the main process' rank
  {
    size_t start_idx = r * per_thread_len;
    size_t stop_idx = min(start_idx + per_thread_len, s.b);
    
    // Sending both arrays
    MPI_Send(regular_samples.ptr, regular_samples.len, MPI_INT, r, 1, MPI_COMM_WORLD);
    MPI_Send(data.ptr + start * sizeof(MPI_INT), stop_idx - start_idx, MPI_INT, r, 1, MPI_COMM_WORLD);
  }
}

void all_to_all_main(Slice data, Slice samples)
{
  // If we are running it in the main process, the array is already there.
}

void all_to_all()
{
  // Receive data from MPI
  int* samples = (int*) malloc((P-1) * sizeof(int));
  int* samples_to_index = (int*) malloc((P-1) * sizeof(int));
  size_t data_size;
  
  // TODO: error check
  // Receive samples from rank 0;
  MPI_Recv(samples, (P-1), MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
  
  // First receive the size of the data being received
  MPI_Recv(&data_size, 1, MPI_UINT, 0, 1, MPI_COMM_WORLD, &status);
  void* data = malloc(data_size * sizeof(int));
  // Then actually receive the data
  MPI_Recv(data, data_size, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
 
  // Index the samples wrt to the data
  int i = 0;
  for (int j = 0 ; j < N ; j++)
  {
    if (samples[i] > data[j])
    {
      samples_to_index[i] = j;
      i++;
    }
  }
  
  int* incoming_buf = malloc(N * sizeof(int));
  int* aggregate_buf = malloc(3 * N * sizeof(int));
  
  // Send data to the respective processes
  size_t start, stop;
  for (int j = 0 ; j < P ; j++)
  {
    if (j == 0)
    {
      start = 0;
      stop = samples_to_index[j];
    }
    else if (j == P-1)
    {
      start = samples_to_index[j];
      stop = N;
    }
    else
    {
      start = samples_to_index[j-1];
      stop = samples_to_index[j];      
    }
    
    // TODO: check
    // Must be an asyncronous send, because we need to send and receive at the same time (sort of)
    size_t sent_data_size = stop-start;
    MPI_Send(&sent_data_size, 1, MPI_INT, j, 1, MPI_COMM_WORLD);
    MPI_Send(data + start * sizeof(MPI_INT), stop - start, MPI_INT, j, 1, MPI_COMM_WORLD);
    
    // Receive
    size_t incoming_buf_size;
    MPI_Recv(&incoming_buf_size, 1, MPI_INT, j, 1, MPI_COMM_WORLD, &status);
    MPI_Recv(incoming_buf, N, MPI_INT, j, 1, MPI_COMM_WORLD, &status);
    
    memcpy(aggregate_buf + pointer_offset, incoming_buf, incoming_buf_size);
    pointer_offset += incoming_buf_size;
    
    // TODO: check if the message was correctly sent/received.
  }
  
  // Send data back to main
  MPI_Send(aggregate_buf, aggregate_buf_size, MPI_INT, 0, 1, MPI_COMM_WORLD);
  
  free(samples);
  free(samples_to_index);
  free(data);
  free(aggregate_buf);
  free(incoming_buf);
}

/*
This function merges the data that was sent back from the multiple processes.
Uses openmp and quicksort to acvhieve the job.
*/
void parallel_merge(int** vecs) // TODO: replace with slice
{
  #pragma omp parallel num_threads(T)
  {
      // For each MPI process (same as the number of vectors to merge),
      // create a task that will merge that vector.
      for (int i = 0 ; i < P ; i++)
      {
        vecs[0]
      }
  }
}

int main(int argc, char* argv[])
{
  // Initialize MPI
  MPI_Init(&argc, &argv);
  int _p;
	MPI_Comm_size(MPI_COMM_WORLD, &_p);
  P = (size_t) _p;
	MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
	//MPI_Get_processor_name(processor_name, &namelen);
 
  if (argc < 2) return -1;
  N = atoi(argv[1]);
  
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
    distribute_samples_and_slices(data_s, regular_samples_s);
    
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
