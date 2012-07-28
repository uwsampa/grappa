#ifndef __PSORT_H
#define __PSORT_H

/**
 * @file
 * 
 * @section DESCRIPTION
 * External interface for parallel sorting functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

/**
 * Externally defined log file that can be used for logging timing info.
 */
extern FILE *pt_log;

/**
 * Carries out a parallel radix sort based on the following:
 *   A. Sohn and Y. Kodama. Load Balanced Parallel Radix Sort.
 *   In Proceedings of the 12th ACM International Conference
 *   on Supercomputing, Melbourne, Australia, July 14-17, 1998.
 *
 * @param npes   Number of PEs.
 * @param id     Rank of calling PE.
 * @param bbits  The number of high order bits in each record that are allocated
 *               to the bucket number.
 * @param bssize The total size in bytes of a bucket stripe.
 * @param *gmax  [out] Holds the maximum number of sorted array elements.
 * @param num    Number of records this PE is responsible for.
 * @param *data  [in] Array to be sorted.
 * @param *recvs [out] Array indicating how many elements a given PE will obtain
 *               from other PEs by bucket id.  Holds the sums of the number of 
 *               elements each PE has in its sorted array.
 * @param *pt_bsort [in] Name of the serial sort to use.
 * 
 * @returns A dynamically allocated array of length recvs that contains this
 *          PE's portion of the sorted data.  Must be freed by the calling 
 *          function.
 */ 
uint64_t * psortui64( int npes, int id, int bbits, uint64_t bssize, int *gmax, 
		      int num, uint64_t *data, int *recvs, char *pt_bsort );

uint64_t *hybrid_parallel_quicksort(int npes, int id, int bbits,
				    int len, uint64_t *data_out, uint64_t *recvs,
				    uint64_t stripe_size);

#endif
