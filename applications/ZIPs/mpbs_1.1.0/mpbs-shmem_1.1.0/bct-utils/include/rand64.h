#ifndef __RAND64_H
#define __RAND64_H

#include <stdint.h>

/**
 * @file
 *
 * @section DESCRIPTION
 * 64-bit random number generator.  This is an encapsulation of
 * the random number generator found in CPU Suite (brand).
 */

/**
 * Generate a 64 bit unsigned random integer.  
 *
 * @return A random 64 bit unsigned integer.
 */
uint64_t rand64();

/**
 * Seed the random number generator.
 * 
 * @param val  Seed value
 */
void     srand64(uint64_t val);

/**
 * Generate a random real number between 0 and 1.
 * 
 * @return A random fraction between 0 and 1.
 */
double   rfraction();

#endif
