#define _XOPEN_SOURCE 500

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <omp.h>

ssize_t write_threaded( int fd, const void *buffer, 
			size_t element_size, 
			size_t length,  
			int max_threads ) {
  int      i;
  ssize_t  global_size_written;
  ssize_t *thread_sizes_written;
  ssize_t  size_written = 0;

  off_t    current_pos = lseek(fd, 0, SEEK_CUR);
  
  if( current_pos < 0 ) {
    perror("lseek failure in write_threaded.");
    return 0;
  }

  /*
   * Distribute the array equally among the threads and write to disk.
   */
  if( !max_threads ) {
    max_threads = omp_get_max_threads();
  }

  thread_sizes_written = malloc( max_threads * sizeof(ssize_t) );

#pragma omp parallel num_threads(max_threads) 
  { 
    int    id            = omp_get_thread_num();
    size_t thread_length = length / max_threads;
    size_t rem           = length - thread_length * max_threads;
    int    index         = id * thread_length;

    /*
     * Distribute any extra elements to the low numbered threads.
     */
    if( id >= rem ) {
      index += rem;
    }
    else {
      thread_length++;
      index += id;
    }

    thread_sizes_written[id] = pwrite(fd, 
				      buffer + element_size * index, 
				      thread_length * element_size, 
				      current_pos + index * element_size );

  }//end omp parallel
  
  /*
   * After the write, advance the file pointer by the buffer amount.
   */
  for( i = 0; i < max_threads; i++) {
    size_written += thread_sizes_written[i];
  }
  free(thread_sizes_written);
  current_pos = lseek(fd, size_written, SEEK_CUR);
  
  if( current_pos < 0 ) {
    perror("lseek failure in write_threaded.");
    return 0;
  }

  return size_written;
}
