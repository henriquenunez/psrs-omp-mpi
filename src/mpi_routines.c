#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include <omp.h>
#include <mpi.h>

#include "../headers/defines.h"
#include "../headers/slice.h"

// Gets the range of a slice
Range get_range(Slice local_data_s, int* samples_to_index, int j, int P)
{
  int start, stop;
  
  if (j == 0)
  {
    start = 0;
    stop = samples_to_index[j];
  }
  else if (j == P - 1)
  {
    start = samples_to_index[j - 1];
    stop = local_data_s.size;
  }
  else
  {
    start = samples_to_index[j - 1];
    stop = samples_to_index[j];      
  }
  
  return (Range){start, stop};
}

int *index_samples(Slice data_s, Slice samples)
{ 
  int* samples_to_index = (int*) malloc(samples.size * sizeof(int));

  // Index the samples wrt to the data
  int i = 0;
  for (int j = 0 ;j < data_s.size; j++)
  {
    if (((int*)samples.ptr)[i] < ((int*)data_s.ptr)[j])
    {
      samples_to_index[i] = j;
      i++;
    }
  }
  
  return samples_to_index;
}

Slice _all_to_all(Slice local_data_s, Slice pivots_s, int P, int rank)
{
  // Index each sample with the local data
  int* samples_to_index = index_samples(local_data_s, pivots_s);
  // printf("[%d] indexed samples\n", rank);
  // print_slice_rank((Slice){samples_to_index, pivots_s.size}, rank);
  
  bool first_pass = true;
  Slice *slices_ptr = malloc(sizeof(Slice) * P); // Slice pointer w/ P positions
  for (int j = rank; ; j++) {
    // printf("[%d] j = %d\n", rank, j);
    j = j % P;

    int send_to = (j + 1) % P;
    int recv_from = (j - 1 + P) % P;

    // printf("[%d] j = %d send_to = %d recv_from = %d\n", rank, j, send_to, recv_from);

    if (first_pass)
      first_pass = false;
    else if (j == rank) {
      // printf("[%d] finished sending\n", rank);
      break;
    }

    Range send_range = get_range(local_data_s, samples_to_index, send_to, P);
    int start = send_range.start;
    int stop = send_range.stop;
    Slice s = {local_data_s.ptr + sizeof(int) * start, stop - start};
    size_t slice_size = s.size * sizeof(int); // Calculate the slice size in bytes

    // if(rank != send_to)
    //   printf("[%d] slice to be sent to %d [%d:%d]\n", rank, send_to, start, stop);
    // else
    if(rank == send_to)
    {
      // printf("[%d] my slice [%d:%d]\n", rank, start, stop);
      slices_ptr[rank] = s;
    }
    // print_slice_rank(s, rank);

    MPI_Request size_send_request;
    // Send the size of the slice to the send_to rank
    if(send_to != rank) // Avoid sending to itself
    {
      MPI_Isend(&slice_size, 1, MPI_UNSIGNED_LONG, send_to, 0, MPI_COMM_WORLD, &size_send_request);
      // printf("[%d] (async) sending slice size to %d\n", rank, send_to);
    }

    size_t recv_size;
    MPI_Request size_recv_request;
    // Receive the size of the slice from the recv_from rank
    if(recv_from != rank) { // Avoid receiving from itself
      MPI_Irecv(&recv_size, 1, MPI_UNSIGNED_LONG, recv_from, 0, MPI_COMM_WORLD, &size_recv_request);
      // printf("[%d] (async) receiving slice size from %d\n", rank, recv_from);
    }

    // Wait for the size communication to complete
    if(rank != send_to)
    {
      MPI_Wait(&size_send_request, MPI_STATUS_IGNORE);
      // printf("[%d] sent slice size [%zu]B to %d\n", rank, slice_size, send_to);
    }

    if(rank != recv_from) {
      MPI_Wait(&size_recv_request, MPI_STATUS_IGNORE);
      // printf("[%d] received slice size [%zu]B from %d\n", rank, recv_size, recv_from);
    }

    // Allocate memory for the received slice
    int* recv_slice = malloc(recv_size);
    if (recv_slice == NULL) printf("[%d] error in malloc! Abort!", rank);

    MPI_Request data_send_request;
    if(rank != send_to) // Avoid sending to itself
    {
      // Send the slice to the send_to rank
      MPI_Isend(s.ptr, s.size, MPI_INT, send_to, 0, MPI_COMM_WORLD, &data_send_request);
      // printf("[%d] (async) sending slice to %d\n", rank, send_to);
    }

    MPI_Request data_recv_request;
    if(rank != recv_from) // Avoid receiving from itself
    {
      // Receive the slice from the recv_from rank
      MPI_Irecv(recv_slice, recv_size, MPI_INT, recv_from, 0, MPI_COMM_WORLD, &data_recv_request);
      // printf("[%d] (async) receiving slice from %d\n", rank, recv_from);
    }

    if(rank != send_to)
    {
      // Wait for the data communication to complete
      MPI_Wait(&data_send_request, MPI_STATUS_IGNORE);
      // printf("[%d] sent slice to %d\n", rank, send_to);
    }

    if(rank != recv_from)
    {
      MPI_Wait(&data_recv_request, MPI_STATUS_IGNORE);
      // printf("[%d] received slice from %d\n", rank, recv_from);
    }

    if(rank != recv_from) {
      Slice recv_s = {recv_slice, recv_size / sizeof(int)};
      // print_slice_rank(recv_s, rank);
      slices_ptr[recv_from] = recv_s;
    }
  }

  Slice new_slice;
  new_slice.size = 0;
  for(int i = 0; i < P; i++) new_slice.size += slices_ptr[i].size;

  // printf("[%d] My final slice size: %zu\n", rank, new_slice.size);

  new_slice.ptr = (void *) malloc(sizeof(int) * new_slice.size);

  int idx = 0;
  for(int i = 0; i < P; i++)
  {
    for(int j = 0; j < slices_ptr[i].size; j++)
    {
      ((int *) new_slice.ptr)[idx++] = ((int *) slices_ptr[i].ptr)[j];
    }
  }

  // printf("[%d] My final slice:\n", rank);
  // print_slice(new_slice);
  
  // printf("[%d] exitting all to all\n", rank);
  free(samples_to_index);

  if(rank != 0)
  {
    size_t *recvbuf_sizet = (size_t *) malloc(sizeof(size_t) * P);

    // Gather the sizes
    MPI_Gather(&new_slice.size, 1, MPI_UNSIGNED_LONG, recvbuf_sizet, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    // Gather the data
    MPI_Gatherv((int *) new_slice.ptr, new_slice.size, MPI_INT, NULL, NULL, NULL, MPI_INT, 0, MPI_COMM_WORLD);
  }

  return new_slice;
}

Slice all_to_all_main(Slice data_s, Slice pivots_s, size_t P, size_t N, int rank)
{
  // If we are running it in the main process, the array is already there.
  /*1- Send samples (pivots) to non 0 ranks
    2- Send the size of the data
    3- Send the actual data
    4- Act like a normal rank */
 
  // printf("[%d] allocating pivots\n", rank);
  int *pivots = (int *) malloc((P - 1) * sizeof(int));
  
  // printf("[%d] memcpying pivots\n", rank);
  memcpy(pivots, pivots_s.ptr, (P - 1) * sizeof(int));
  
  // Broadcast the data from the root process to all other processes
  // printf("[%d] broadcasting pivots\n", rank);
  MPI_Bcast(pivots, P - 1, MPI_INT, 0, MPI_COMM_WORLD);
  
  // print_slice_rank((Slice){pivots, P - 1}, rank);
  
  Slice local_data_s;
  
  // printf("[%d] spreading data\n", rank);
  for (int i = 0 ; i < P ; i++)
  {
    // Compute the data slice for each process
    Slice p_slice = split_data(data_s, i, P);
    if (i == 0)
    {
      local_data_s = p_slice;
      continue;
    }
    
    // printf("[%d] sending data size (%zu) to %d\n", rank, p_slice.size, i);
    MPI_Send(&(p_slice.size), 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD);
    // printf("[%d] sending data to %d\n", rank, i);
    MPI_Send(p_slice.ptr, p_slice.size, MPI_INT, i, 0, MPI_COMM_WORLD);
  }
  
  // Now, behave as a normal node
  // (Slice local_data_s, Slice pivots_s, int P, int rank)
  return _all_to_all(local_data_s, pivots_s, P, rank);
}

void all_to_all(size_t P, size_t N, int rank)
{
  // printf("[%d] allocating pivots\n", rank);
  int *pivots = (int *) malloc((P - 1) * sizeof(int));
  
  // Broadcast the data from the root process to all other processes
  // printf("[%d] receiving broadcasted pivots\n", rank);
  MPI_Bcast(pivots, P - 1, MPI_INT, 0, MPI_COMM_WORLD);
  
  Slice pivots_s = {pivots, P - 1};
  // print_slice_rank(pivots_s, rank);
  
  // Receive the data size from the main process
  int data_size;
  MPI_Recv(&data_size, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  // printf("[%d] received data size (%d)\n", rank, data_size);
  
  // Now, receive the actual data
  int *data_ptr = (int *) malloc(data_size * sizeof(int));
  MPI_Recv(data_ptr, data_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  // printf("[%d] received full data\n", rank);
  Slice local_data_s = {data_ptr, data_size};
  // print_slice_rank(local_data_s, rank);
  
  _all_to_all(local_data_s, pivots_s, P, rank);
  
  free(data_ptr);
  free(pivots);
}
