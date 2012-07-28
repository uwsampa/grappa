#ifndef __CPU_SUITE_H
#define __CPU_SUITE_H

/**
 * @file
 *
 * Functions provided by the libcpu_suite.a library.
 */

#include <stdint.h>

/**
 * Performs the tilt kernel from CPU suite.
 *
 * @param p 64x64 bit matrix to be transposed.
 */
void tilt(uint64_t p[]);

/**
 * Performs the BMM kernel from CPU suite.
 *
 * @param vec Vector input.
 * @param cnt Result.
 * @param mat Random matrix.
 */
void bmm_update(uint64_t vec[], int32_t cnt[], uint64_t mat[]);

#endif
