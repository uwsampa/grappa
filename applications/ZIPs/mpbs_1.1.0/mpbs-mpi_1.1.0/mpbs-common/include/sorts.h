#ifndef __SORTS_H
#define __SORTS_H

#include "types.h"
#include "usrt.h"

/**
 * @file
 * 
 * @section DESCRIPTION
 * Collection of various 64 bit sorting algorithms.
 */

/**
 * Recursive implementation of the Quicksort algorithm for 64 bit integers. 
 * In-place sort.
 *
 * @param n Array length
 * @param a [in|out] Array to be sorted
 */
void qsortui64( uint64 n, uint64 *a );

/**
 * Selection sort on 64 bit integers.  In place sort.
 *
 * @param n Array length
 * @param a [in|out] Array to be sorted
 */
void ssortui64( uint64 n, uint64 *a );

/**
 * Insertion sort on 64 bit integers.  In place sort.
 *
 * @param n Array length
 * @param a [in|out] Array to be sorted
 */
void isortui64( uint64 n, uint64 *a );

/**
 * Bubble sort on 64 bit integers.  In place sort.
 *
 * @param n Array length
 * @param a [in|out] Array to be sorted
 */
void bsortui64( uint64 n, uint64 *a );

/**
 * Radix sort on 64 bit integers.  In place sort.
 *
 * @param n Array length
 * @param a [in|out] Array to be sorted
 */
void radixui64( uint32 r, uint64 n, uint64 *a );

#endif
