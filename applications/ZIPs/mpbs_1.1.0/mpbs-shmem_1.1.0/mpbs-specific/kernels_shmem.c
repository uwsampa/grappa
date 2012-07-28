#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <mpp/shmem.h>

#include "rand64.h"

void collective_compute_kernel(uint64_t niter, int npes, int id, uint64_t nelms) {
  uint64_t *src = shmalloc(nelms * sizeof(uint64_t));
  uint64_t *dst = shmalloc(nelms * sizeof(uint64_t));
  
  static long pSync[_SHMEM_BCAST_SYNC_SIZE];
  
  uint64_t n, i, s, j;

  for( i = 0; i < _SHMEM_BCAST_SYNC_SIZE; i++) 
    pSync[i] = _SHMEM_SYNC_VALUE;

  for( i = 0; i < nelms; i++)
    src[i] = rand64();
  
  // Complete initialization of synchronization array.
  shmem_barrier_all();

  for( n = 0; n < niter; n++) {
    for( s = 1; s < (nelms/npes); s *= 2) {
      for( j = 0; j < npes; j++) {
	shmem_broadcast64(&dst[j * nelms/npes], &src[j * nelms/npes], 
			  s, j, 0, 0, npes, pSync);
	shmem_barrier_all();
      }
    }

    for( i = 0; i < nelms; i++) 
      dst[i] *= src[i]; 

    shmem_barrier_all();

    for( s = 1; s < (nelms/npes); s *= 2) {
      for( j = 0; j < npes; j++) {
	shmem_broadcast64(&src[j * nelms/npes], &dst[j * nelms/npes], 
			  s, j, 0, 0, npes, pSync);
	shmem_barrier_all();
      }
    }
  }
  
  shfree(src);
  shfree(dst);
}
