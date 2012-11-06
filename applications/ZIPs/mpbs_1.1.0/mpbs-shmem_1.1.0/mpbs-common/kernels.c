#include "hpc_output.h"
#include "cpu_suite.h"
#include "mmbs2.h"
#include "rand64.h"
#include "timer.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <gmp.h>

#if defined (__INTEL_COMPILER)
#include <nmmintrin.h>
#define _popcnt(x) _mm_popcnt_u64(x);
#elif defined(_CRAYC)
#include <intrinsics.h>
#elif defined(__PGI) // doesn't compile
#include <intrin.h>
#elif defined(__GNUC__) //pathscale as well
#define _popcnt(x) __builtin_popcount(x);
#endif 

#if defined( MPI ) || defined( MPI2 )
#include <mpi.h>
#endif

#if defined( SHMEM ) 
#include <mpp/shmem.h>
#endif


uint64_t tilt_kernel(uint64_t niter, timer *t) {
  static uint64_t arr[64];
  int i, j;

  // Force n odd.
  niter |= 1;

  for(j = 0; j < 64; j++)
      arr[j] = rand64();

  timer_start(t);
  for(i = 0; i < niter; i++) {    
    tilt(arr);
  }
  timer_stop(t);

  return niter;
}

uint64_t bmm_kernel(uint64_t niter, timer *t) {
  // 1L << 17 comes from CPU suite VLEN
  // 512 comes from CLEN
  static uint64_t vec[(1L<<17)], mat[64];
  static int32_t cnt[512];

  int i, j, k;

  /*
   * Timed like CPU suite, but the timing really shouldn't include
   * the random number generation.
   */

  timer_start(t);
  for( i = 0; i < niter; i++ ) {
    // Each BMM kernel from CPU suite is run for 55179 iterations.
    for( j = 0; j < 55179; j++ ) {
      for( k = 0; k < 64; k++ ) 
	mat[k] = rand64();
      bmm_update(vec, cnt, mat);
    }
  }
  timer_stop(t);
  return niter * 55179 * (1L <<17);
}

// nbits should be a power of 2.
uint64_t gmp_kernel(uint64_t niter, timer *t) {
  mpz_t base, exp, mod, ans, sum;
  gmp_randstate_t rand_state;

  int i = 0;

  uint64_t nbits = 512;
  
  gmp_randinit    (rand_state, GMP_RAND_ALG_LC, 128UL);
  gmp_randseed_ui (rand_state, rand());

  mpz_init(base);
  mpz_init(exp);
  mpz_init(mod);
  mpz_init(ans);
  mpz_init(sum);
  
  timer_start(t);
  for (i = 0; i < niter; i++) {
    mpz_urandomb (base, rand_state, nbits);
    mpz_urandomb (exp,  rand_state, nbits);
    mpz_urandomb (mod,  rand_state, nbits);
    
    /* force odd modulus */
    mpz_setbit (mod, 0);

    /* ans <- base ^ exp (Mod mod) */
    mpz_powm (ans, base, exp, mod);
    /* sum <- sum + ans */
    mpz_add  (sum, sum, ans);
  }
  timer_stop(t);
  
  return niter;
}

#ifndef _CRAYC
#ifdef __SSE2__
uint64_t mmbs2_kernel(uint64_t niter, timer *t) {
  vec_t x;
  int k, i, j;

  static vec_t in[TLEN+PAD] __attribute__ ((aligned (ALIGN)));
  static vec_t out[TLEN] __attribute__ ((aligned (ALIGN)));

  timer_start(t);
  for (k = 0; k < niter; k++) { 
    x = vec_set64_1(rand64()); 

    for (i = 0; i < TLEN; i++) 
      in[i] = vec_xor(in[i], x); 
    
    for (j = 0; j < (TOT/NWORDS); j++) 
      mmbs2(&in[j*VLEN], &out[j*VLEN]); 
  } 
  timer_stop(t);

  return niter * TOT * (sizeof(vec_t)/8) * 64;
}
#else
uint64_t mmbs2_kernel(uint64_t niter, timer *t) {
  printf("WARNING: SSE2 instruction set not available, MMBS2 kernel is a NOOP\n");
  return 0;
}
#endif
#else
uint64_t mmbs2_kernel(uint64_t niter, timer *t) {
  printf("WARNING: MMBS2 kernel is not supported when compiling with craycc.\n");
  return 0;
}
#endif

// The Cray compiler is smart enough to recognize that the popcnt loop can be
// optimized down significantly, so turn off optimization for this function.  
// GCC does not optimize the double loop, even at -O3.
#ifdef _CRAYC
#pragma _CRI noopt
#endif
// Each iteration does 1<<14 popcounts
uint64_t popcnt_kernel(uint64_t niter, timer *t) {
  static uint64_t cnt; 
  
  uint64_t arr[1<<14];
  uint64_t len = 1<<14;

  int n = 0;
  int i = 0;
  for( i = 0; i < len; i++)
    arr[i] = rand64();
  
  timer_start(t);
  for(n = 0; n < niter; n++) {
    for( i = 0; i < len; i++)
      cnt += _popcnt(arr[i]);
  }
  timer_stop(t);
  
  return (1L<<14) * niter; 
}
#ifdef _CRAYC
#pragma _CRI opt
#endif


/* #if defined( MPI ) || defined( MPI2 ) */
/* void collective_compute_kernel(uint64_t niter, int npes, int id, uint64_t nelms) {  */
/*   uint64_t *src = malloc(nelms * sizeof(uint64_t)); */
/*   uint64_t *dst = malloc(nelms * sizeof(uint64_t)); */
/*   uint64_t n, i, s; */

/*   for( i = 0; i < nelms; i++) */
/*     src[i] = rand64(); */

/*   for( n = 0; n < niter; n++ ) { */
/*     for( s = 1; s < (nelms/npes); s *= 2) { */
/*       MPI_Alltoall(src, s, MPI_UNSIGNED_LONG,  */
/* 		   dst, s, MPI_UNSIGNED_LONG,  */
/* 		   MPI_COMM_WORLD); */
/*     } */

/*     for( i = 0; i < nelms; i++) */
/*       dst[i] *= src[i]; */
   
/*     for( s = 1; s < (nelms/npes); s *= 2) { */
/*       MPI_Alltoall(dst, s, MPI_UNSIGNED_LONG,  */
/* 		   src, s, MPI_UNSIGNED_LONG,  */
/* 		   MPI_COMM_WORLD); */
/*     } */
/*   } */
/*   free(src); */
/*   free(dst); */
/* } */
/* #endif */

/* #ifdef SHMEM */
/* void collective_compute_kernel(uint64_t niter, int npes, int id, uint64_t nelms) { */
/*   uint64_t *src = shmalloc(nelms * sizeof(uint64_t)); */
/*   uint64_t *dst = shmalloc(nelms * sizeof(uint64_t)); */
  
/*   static long pSync[_SHMEM_BCAST_SYNC_SIZE]; */
  
/*   uint64_t n, i, s, j; */

/*   for( i = 0; i < _SHMEM_BCAST_SYNC_SIZE; i++)  */
/*     pSync[i] = _SHMEM_SYNC_VALUE; */

/*   for( i = 0; i < nelms; i++) */
/*     src[i] = rand64(); */
  
/*   // Complete initialization of synchronization array. */
/*   shmem_barrier_all(); */

/*   for( n = 0; n < niter; n++) { */
/*     for( s = 1; s < (nelms/npes); s *= 2) { */
/*       for( j = 0; j < npes; j++) { */
/* 	shmem_broadcast64(&dst[j * nelms/npes], &src[j * nelms/npes],  */
/* 			  s, j, 0, 0, npes, pSync); */
/* 	shmem_barrier_all(); */
/*       } */
/*     } */

/*     for( i = 0; i < nelms; i++)  */
/*       dst[i] *= src[i];  */

/*     shmem_barrier_all(); */

/*     for( s = 1; s < (nelms/npes); s *= 2) { */
/*       for( j = 0; j < npes; j++) { */
/* 	shmem_broadcast64(&src[j * nelms/npes], &dst[j * nelms/npes],  */
/* 			  s, j, 0, 0, npes, pSync); */
/* 	shmem_barrier_all(); */
/*       } */
/*     } */
/*   } */
  
/*   shfree(src); */
/*   shfree(dst); */
/* } */
/* #endif */



/* #ifdef MPI2 */
/* void collective_compute_kernel(int npes, int id, void *a, uint64_t nbytes) { */
/*   // Simulate source and destination buffers for each process then  */
/*   // create a netstress type workload.  */
/*   uint64_t nelms = nbytes / sizeof(uint64_t); */
  
/*   uint64_t *src = (uint64_t *)a; */
/*   uint64_t *dst = &(((uint64_t *)a)[nelms/npes]); */

/*   uint64_t n, i, j; */
/*   uint64_t min_size = 1; */
/*   uint64_t max_size = nelms/npes; */

/*   // Prevent the compiler from optimizing this out. */
/*   static uint64_t cnt; */

/*   MPI_Win srcwin; */
/*   MPI_Win dstwin; */
  
/*   MPI_Win_create(src, nelms * sizeof( uint64_t ), sizeof( uint64_t ), MPI_INFO_NULL,  */
/* 		 MPI_COMM_WORLD, &srcwin); */
/*   MPI_Win_create(dst, nelms * sizeof( uint64_t ), sizeof( uint64_t ), MPI_INFO_NULL,  */
/* 		 MPI_COMM_WORLD, &dstwin); */
 
/*   for( n = min_size; n < max_size; n *= 2 ) { */
/*     for( j = 0; j < npes; j++) { */
/*       MPI_Win_fence(0, srcwin); */
/*       MPI_Get(&dst[j*n], n, MPI_UNSIGNED_LONG, j,  */
/* 	      0, n, MPI_UNSIGNED_LONG, srcwin); */
/*       MPI_Win_fence(0, srcwin); */

/*       MPI_Win_fence(0, dstwin); */
/*       MPI_Put(src, n, MPI_UNSIGNED_LONG, j, */
/* 	      id*n, n, MPI_UNSIGNED_LONG, dstwin); */
/*       MPI_Win_fence(0, dstwin); */
/*     } */
      
/*     // Do popcounts on the whole array to keep the cpu busy. */
/*     for( i = 0; i < nelms - 3; i++) */
/*       cnt += _popcnt( src[i] * src[i + 1] + src[i + 2] ); */

/*   } */
/*   MPI_Win_free(&srcwin); */
/*   MPI_Win_free(&dstwin); */
/* } */
/* #endif */
