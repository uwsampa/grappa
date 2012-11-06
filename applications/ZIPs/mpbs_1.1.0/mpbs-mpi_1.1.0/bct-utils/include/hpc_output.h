#ifndef __HPC_OUTPUT_H
#define __HPC_OUTPUT_H

/**
 * @file
 *
 * @section DESCRIPTION   
 * Printing functions for use in programming HPC systems.
 */

#include <stdio.h>
#include <stdarg.h>

/**
 * Rank 0 process prints to screen and simultaneously to a log file.
 *
 * @param rank Rank of the calling process or thread.
 * @param fp   File pointer to write to.
 * @param fmt  Format string to print.
 * @param ...  Substitutions.
 *
 * @return Total number of substitutions made by both print statements.
 */
int lprintf_single( int rank, FILE *fp, char *fmt, ... );

/**
 * Rank 0 process prints to a stdio stream.
 * 
 * @param rank Rank of the calling process or thread.
 * @param fp   Open file pointer to write to.
 * @param fmt  Format string to print.
 * @param ...  Substitutions.
 *
 * @return Total number of substitutions made.
 */ 
int fprintf_single( int rank, FILE *fp, char *fmt, ... );

/**
 * Rank 0 process prints to screen.
 * 
 * @param rank Rank of the calling process or thread.
 * @param fmt  Format string to print.
 * @param ...  Substitutions.
 *
 * @return Total number of substitutions made.
 */ 
int  printf_single( int rank, char *fmt, ... );

#endif
