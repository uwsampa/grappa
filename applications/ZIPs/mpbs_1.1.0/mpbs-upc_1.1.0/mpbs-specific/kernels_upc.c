#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <upc.h>
#include <upc_collective.h>

void collective_compute_kernel(uint64_t niter, int npes, int id, uint64_t nelms) {
  shared uint64_t *src;
  shared uint64_t *dst;

  shared [] uint64_t *mysrc;
  shared [] uint64_t *mydst;

  uint64_t n, i, s;

  src = upc_all_alloc(npes * npes, (nelms / npes) * sizeof(uint64_t));
  dst = upc_all_alloc(npes * npes, (nelms / npes) * sizeof(uint64_t));
  mysrc = (shared [] uint64_t *)&src[id];
  mydst = (shared [] uint64_t *)&dst[id];

  for( n = 0; n < niter; n++) {
    for( s = 1; s < (nelms/npes); s *= 2) {
      upc_all_exchange(src, dst, s * sizeof(uint64_t), 
		       UPC_IN_ALLSYNC | UPC_OUT_ALLSYNC);
    }
    
    for( i = 0; i < nelms; i++) 
      dst[i] *= src[i]; 

    upc_barrier;
    
    for( s = 1; s < (nelms/npes); s *= 2) {  
      upc_all_exchange(src, dst, s * sizeof(uint64_t), 
		       UPC_IN_ALLSYNC | UPC_OUT_ALLSYNC);
    }
  }

  if(id == 0) {
    upc_free(src);
    upc_free(dst);
  }
}
