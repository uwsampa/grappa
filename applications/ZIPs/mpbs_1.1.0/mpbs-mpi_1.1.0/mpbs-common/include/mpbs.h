/**
 * @file 
 * Main module for the MPBS benchmark.
 *
 * @section BENCHMARK DESCRIPTION
 * This MPBS benchmark produces a large number of files, and then performs a 
 * two-stage sort on the contents of those files so that a single, large sorted
 * file is produced.  The benchmark will be executed in two phases:
 *
 * -# File production.
 * 
 * -# File content sort.
 *
 * Each file is structured as a fixed, numbered sequence of buckets where each
 * bucket consists of a fixed number of records. Each record is a 4, 8 or 16 
 * byte unsigned integer.  In Phase 1, each file is produced by sequentially 
 * filling each bucket with integer records. The integer is set so that the
 * most significant bits reflect the bucket number.  A file is complete when 
 * all buckets have been filled and written to disk.  In Phase 2, the records 
 * are sorted for a given bucket number across all of the generated files, and
 * then written to a single file. This is done for each bucket.  This single 
 * file will contain all of the data from the Phase 1 files, completely sorted.
 *
 * @section DESCRIPTION
 * This module provides the main routine and the subroutines for handling
 * command line options, distributing the problem amongst the PEs, and printing
 * out the benchmark header and footer information.
 */

#include <stdio.h>

#include "types.h"
#include "timer.h"

char *pt_ofpath         = NULL; ///< Output file path.
char *pt_bsort          = NULL; ///< Bucket sort method.  Default is "none".

int id                  = 0;    ///< This PE's rank.
int npes                = 0;    ///< Number of PEs.

uint64 tiltiters        = 0;    ///< Number of iterations to run the TILT kernel.
uint64 bmmiters         = 0;    ///< Number of iterations to run the BMM kernel.
uint64 gmpiters         = 0;    ///< Number of iterations to run the GMP kernel.
uint64 popcntiters      = 0;    ///< Number of iterations to run the POPCNT kernel.
uint64 kiters           = 0;    ///< Number of iterations to run the NETSTRESS kernel.
uint64 kbufsize         = 0;    ///< Power of 2 size for the NETSTRESS buffers.
uint64 mmbs2iters       = 0;    ///< Number of iterations to run the MMBS2 kernel.

uint64 rs               = 0;    ///< Record size. 4 or 8 (default 8).
uint64 rb               = 0;    ///< Number of records per bucket (default 64).
uint64 nb               = 0;    ///< Number of buckets per file (default 4).
uint64 nf               = 0;    ///< Number of files total (default 1 per PE).

uint64 bsize            = 0;    ///< Bucket size in bytes.  bs = rs * rb


uint64 file_start       = 0;    ///< First file index this PE is responsible for.
uint64 file_stop        = 0;    ///< Last file index this PE is responsible for.
uint64 bstripe_start    = 0;    ///< Record stripe start index.
uint64 bstripe_stop     = 0;    ///< Record stripe stop index.

FILE *pt_log            = NULL; ///< Stream pointer for the log file.

/**
 * Handles the command line options.
 *
 * Responsible for setting the following global variables: 
 * - tiltiters
 * - bmmiters
 * - gmpiters
 * - popcntiters
 * - kiters
 * - kbufsize
 * - mmbs2iters
 * - rs
 * - rb
 * - nb
 * - nf
 * - pt_idir
 * - pt_odir
 * - pt_ofpath
 * - pt_bsort
 */
int        cmdline( int, char ** );

/**
 * Prints out the header information.
 */
void       header( uint32 ); 

/**
 * Determines how the work is distributed to the PEs.
 *
 * Responsible for setting the following global variables:
 * - file_start
 * - file_stop
 * - bstripe_start
 * - bstripe_stop
 */
void       work( );

/**
 * Prints usage information.
 */
void       help();

/**
 * Prints end of run timing information.
 */
void       trailer( timer * );



int        cmdline_common( int na, char *args[] );
void       help_common();
void       header_common( uint32 np );
void       trailer_common( timer *t );

/**
 * Prints basic system information.
 */
void       print_sys_stats( );

int        cmdline_specific( int na, char *args[] );
void       help_specific();
void       header_specific( uint32 np );
void       trailer_specific( timer *t );

