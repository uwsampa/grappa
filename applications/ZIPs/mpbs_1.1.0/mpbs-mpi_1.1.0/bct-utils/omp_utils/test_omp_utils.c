#define _POSIX_SOURCE

#include "omp_utils.h"

#include <stdio.h>
#include <omp.h>
#include <stdint.h>


int main() {
  uint64_t buffer[1024*1024];
  ssize_t  written;
  size_t element_size = sizeof(uint64_t);
  size_t length = 1024*1024;

  int i;

  for( i = 0; i < 1024*1024; i++)
    buffer[i] = i;

  printf("Element size = %d, length = %d, total size = %d\n", element_size, length, element_size * length);
  FILE * fout = fopen("junk.out", "wb");

  written = write_threaded( fileno(fout), buffer, element_size, length, 0 );

  printf("Written: %d\n", written);

  return 0;
}
 
