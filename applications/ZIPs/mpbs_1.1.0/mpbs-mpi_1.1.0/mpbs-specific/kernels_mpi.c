#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <mpi.h>

#include "rand64.h"

void collective_compute_kernel(uint64_t niter, int npes, int id, uint64_t nelms) { 
  uint64_t *src = malloc(nelms * sizeof(uint64_t));
  uint64_t *dst = malloc(nelms * sizeof(uint64_t));
  uint64_t n, i, s;

  for( i = 0; i < nelms; i++)
    src[i] = rand64();

  for( n = 0; n < niter; n++ ) {
    for( s = 1; s < (nelms/npes); s *= 2) {
      MPI_Alltoall(src, s, MPI_UNSIGNED_LONG, 
		   dst, s, MPI_UNSIGNED_LONG, 
		   MPI_COMM_WORLD);
    }

    for( i = 0; i < nelms; i++)
      dst[i] *= src[i];
   
    for( s = 1; s < (nelms/npes); s *= 2) {
      MPI_Alltoall(dst, s, MPI_UNSIGNED_LONG, 
		   src, s, MPI_UNSIGNED_LONG, 
		   MPI_COMM_WORLD);
    }
  }
  free(src);
  free(dst);
}
