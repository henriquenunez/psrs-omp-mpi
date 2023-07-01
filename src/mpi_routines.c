#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <mpi.h>

#include <omp.h>


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
This function is always run by rank 0.
*/
void distribute_samples_and_slices(Slice data, Slice regular_samples, size_t P)
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