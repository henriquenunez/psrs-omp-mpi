#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <mpi.h>

#include <omp.h>

#include "../headers/defines.h"

Slice split_data(Slice data_s, int i, int Total)
{
  int N = data_s.size;
  int thread_num = i;
  
  //#pragma omp parallel num_threads(T) // Step (a) to (b)
  //int thread_num = omp_get_thread_num();
  int low = thread_num * (N / Total);
  int high = (thread_num + 1) * (N / Total) - 1;

  // Adjust the last thread's high index to include remaining elements
  if (thread_num == T - 1) {
    high += N % Total;
  }

  size_t slice_size = high - low + 1;
  Slice thr_slice = {&((int*)data_s.ptr)[low], slice_size};

  return thr_slice;
}

int* index_samples(Slice data_s, Slice samples)
{ 
  int* samples_to_index = (int*) malloc(samples.size * sizeof(int));

  // Index the samples wrt to the data
  int i = 0;
  for (int j = 0 ; j < data_s.size ; j++)
  {
    //printf("Is %d < %d?\n", ((int*)samples.ptr)[i], ((int*)data_s.ptr)[j]);
    if (((int*)samples.ptr)[i] < ((int*)data_s.ptr)[j])
    {
      //printf("yes!\n");
      samples_to_index[i] = j;
      i++;
    }
  }
  
  return samples_to_index;
}

void all_to_all_main(Slice data_s, Slice samples, size_t P, size_t N, int rank)
{
  // If we are running it in the main process, the array is already there.
  /*1- Send samples to non 0 ranks
    2- Send the size of the data
    3- Send the actual data
    4- Act like a normal rank??*/
 
  printf("[%d] allocating pivots\n", rank);
  int* pivots = (int*) malloc((P-1) * sizeof(int));
  
  printf("[%d] memcpying pivots\n", rank);
  memcpy(pivots, samples.ptr, (P-1) * sizeof(int));
  
  // Broadcast the data from the root process to all other processes
  printf("[%d] broadcasting pivots\n", rank);
  MPI_Bcast(pivots, P-1, MPI_INT, 0, MPI_COMM_WORLD);
  
  print_slice_rank((Slice){pivots, P-1}, rank);
  
  printf("[%d] spreading data\n", rank);
  for (int i = 1 ; i < P ; i++) // Skip 0, ofc
  {
    // Compute the data slice for each process
    Slice p_slice = split_data(data_s, i, P);
    printf("[%d] sending data size (%zu) to %d\n", rank, p_slice.size, i);
    MPI_Send(&(p_slice.size), 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD);
    printf("[%d] sending data to %d\n", rank, i);
    MPI_Send(p_slice.ptr, p_slice.size, MPI_INT, i, 0, MPI_COMM_WORLD);
  }
}

void all_to_all(size_t P, size_t N, int rank)
{
  printf("[%d] allocating pivots\n", rank);
  int* pivots = (int*) malloc((P-1) * sizeof(int));
  
  // Broadcast the data from the root process to all other processes
  printf("[%d] receiving broadcasted pivots\n", rank);
  MPI_Bcast(pivots, P-1, MPI_INT, 0, MPI_COMM_WORLD);
  
  Slice pivots_s = {pivots, P-1};
  print_slice_rank(pivots_s, rank);
  
  // Receive the data size from the main process
  int data_size;
  MPI_Recv(&data_size, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  printf("[%d] received data size (%d)\n", rank, data_size);
  
  // Now, receive the actual data
  int* data_ptr = (int*) malloc(data_size * sizeof(int));
  MPI_Recv(data_ptr, data_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  printf("[%d] received full data\n", rank);
  
  Slice local_data_s = {data_ptr, data_size};
  
  print_slice_rank(local_data_s, rank);
  
  int* samples_to_index = index_samples(local_data_s, pivots_s);
  printf("[%d] indexed samples\n", rank);
  print_slice_rank((Slice){samples_to_index, pivots_s.size}, rank);
  
  // Send data to the respective processes
  size_t start, stop;
  for (int j = 0 ; j < P ; j++)
  {
    if (j == rank) continue;
    if (j == 0)
    {
      start = 0;
      stop = samples_to_index[j];
    }
    else if (j == P-1)
    {
      start = samples_to_index[j-1];
      stop = local_data_s.size;
    }
    else
    {
      start = samples_to_index[j-1];
      stop = samples_to_index[j];      
    }
    
    Slice s = {local_data_s.ptr + sizeof(int) * start, stop - start};
    printf("[%d] slice to be sent to %d [%zu:%zu]\n", rank, j, start, stop);
    print_slice_rank(s, rank);
    
    // TODO: actually send and receive the slices. Find a way to collect the slices from the other processes.
  }
  
  free(samples_to_index);
  free(data_ptr);
  free(pivots);
  /*

  // Receive data from MPI
  int error_code;
  int* samples = (int *) malloc((P - 1) * sizeof(int));
  int* samples_to_index = (int *) malloc((P - 1) * sizeof(int));
  size_t data_size;
  MPI_Status status;
  
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
  */
}

/*
This function is always run by rank 0.
*/
void distribute_samples_and_slices(Slice data, Slice regular_samples, size_t P)
{
  /*
  size_t per_thread_len = (int) ceil(data.len / (float)P);

  // Crazy MPI calls here to select the slices
  for (int r = 1 ; r < P ; r++) // We skip rank 0 since it is the main process' rank
  {
    size_t start_idx = r * per_thread_len;
    // size_t stop_idx = min(start_idx + per_thread_len, s.b);
    size_t stop_idx = fmin(start_idx + per_thread_len, start_idx + (s.size - 1));
    
    // Sending both arrays
    MPI_Send(regular_samples.ptr, regular_samples.len, MPI_INT, r, 1, MPI_COMM_WORLD);
    MPI_Send(data.ptr + start * sizeof(MPI_INT), stop_idx - start_idx, MPI_INT, r, 1, MPI_COMM_WORLD);
  }
  */
}