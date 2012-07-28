#ifndef __USRT_H
#define __USRT_H

#include <stdint.h>
/**
 * @file
 * Functions provided by libusrt.a
 */

/**
 * High performance radix sort.  Can sort over a subset of the bits if desired.
 *
 * @param *a [in|out] Array to be sorted.
 * @param n Length of a.
 * @param x Low bit.
 * @param y High bit.
 */ 
void usort( uint64_t *a, int64_t n, int64_t x, int64_t y);

#endif
