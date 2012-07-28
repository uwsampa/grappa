#ifndef __KERNELS_H
#define __KERNELS_H

/**
 * @file
 * 
 * @section DESCRIPTION
 * Set of small kernels designed to stress a particular operation.
 */

#include <stdint.h>
#include <stdio.h>

#include "timer.h"

/**
 * This kernel allocates two buffers and performs rounds consisting of an 
 * all-to-all type communication followed by integer multiplications on the 
 * buffers followed by a second all-to-all communication.  Each round involves
 * sending an increasing number of integers, starting with 1 and doubling each
 * round until the maximum buffer size is reached.
 * 
 * The exact implementation depends on whether UPC, MPI-1, MPI-2 or SHMEM is
 * being used.  MPI-1 and MPI-2 implementations are identical.  UPC uses 
 * the upc_all_exchange function and SHMEM uses the shmem_broadcast64 function.
 *
 * @param niter Number of iterations to perform (1 iteration is a full set 
 *              of rounds as described above).
 * @param npes  Number of PEs.
 * @param id    Rank of calling PE.
 * @param nelms Maximum number of 64-bit integers to use for each buffer.
 */  
void collective_compute_kernel(uint64_t niter, int npes, int id, uint64_t nelms);

/**
 * This kernel tests the speed of the mpz_powm() routine in the open source
 * GNU multiprecision arithmetic library.  This test is taken from the CPU 
 * suite module of the same name and is designed to primarily test the speed
 * of integer multiplies and adds for a processor.
 *
 * @param   niter Number of iterations to perform.
 * @param   *t    Address of the timer to be used.
 *
 * @returns Number of mpz_powm()'s performed during the time captured by t.
 */
uint64_t gmp_kernel   (uint64_t niter, timer *t);

/**
 * This kernel tests the speed of the _popcnt intrinsic.
 *
 * @param   niter Number of iterations to perform.
 * @param   *t    Address of the timer to be used.
 *
 * @returns Number of 64 bit popcnts's performed during the time captured by t.
 */
uint64_t popcnt_kernel(uint64_t niter, timer *t);

/**
 * This kernel repeatedly transposes a 64x64 bit matrix.  Taken directly from 
 * the CPU suite.
 *
 * @param   niter Number of iterations to perform.
 * @param   *t    Address of the timer to be used.
 *
 * @returns Number of transpositions performed during the time captured by t.
 */
uint64_t tilt_kernel  (uint64_t niter, timer *t);

/**
 * The kernel performs 64x64 bit-matrix multiplies with a fixed set of
 * 2**17 random input vectors and varying random matrices.  Each result
 * is reduced and accumulated into a 512-long table. The implementation comes
 * directly from the CPU suite.
 *
 * @param   niter Number of iterations to perform.
 * @param   *t    Address of the timer to be used.
 *
 * @returns Number of bit matrix multiplications performed during the time 
 *          captured by t.
 */
uint64_t bmm_kernel   (uint64_t niter, timer *t);

/**
 * This kernel computes the square of X MOD (2**16-1) for a series of
 * random X values.  It uses the SSE2 intrinsics with a 128-bit vector size.  
 * It is a vectorized direct port of the CPU suite module BS2.
 *
 * @param   niter  Number of iterations to perform.
 * @param   *t     Address of the timer to be used.
 *
 * @returns Number of bit serial squares performed during the time captured by 
 *          t.
 */
uint64_t mmbs2_kernel (uint64_t niter, timer *t);

#endif
