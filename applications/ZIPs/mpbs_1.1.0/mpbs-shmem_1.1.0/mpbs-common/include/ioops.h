#ifndef __IOOPS_H
#define __IOOPS_H

/**
 * @file 
 *
 * Main IO operation functions for MPBS.  Each function has a 32- and 64-bit
 * version.
 */

#include <stdio.h>
#include <stdlib.h>

#include "types.h"

extern char *pt_bsort;
extern char *pt_ofpath;
extern int id;
extern int npes;

extern uint64 tiltiters;
extern uint64 bmmiters;
extern uint64 gmpiters;
extern uint64 popcntiters;
extern uint64 kiters;
extern uint64 kbufsize;
extern uint64 mmbs2iters;

extern uint64 rs;
extern uint64 rb;
extern uint64 nb;
extern uint64 nf;

extern uint64 bsize;

extern uint64 file_start;
extern uint64 file_stop;
extern uint64 bstripe_start;
extern uint64 bstripe_stop;

extern FILE *pt_log;

/**
 * Write files from top to bottom.
 *
 * These functions write the bucket files by having each PE write (exclusively)
 * to a set of files equally distributed among all PEs.  PE 0 creates file 0,
 * PE 1 creates file 1 and so on.
 */
void make_files_sequential_exclusive64( );
void make_files_sequential_exclusive128( );

/**
 * Write files by randomly filling buckets.
 *
 * These functions write the bucket files by generating random numbers to  
 * fill up a block of memory.  Once a block is full it is flushed to disk.
 * The files are written in block sized chunks, but the order in which 
 * the blocks are written is completely random.
 *
 * @see work()
 */
void make_files_random_exclusive64( );
void make_files_random_exclusive128( );

/**
 * Write files by stripes.
 *
 * These functions write the bucket files.  For a given bucket number, 
 * each process writes a range of records across all files, where the 
 * range of records was calculated in the work() function.
@verbatim
               XY=a record
               where X=file id and Y=record number
            ___|------|------|------|------|____
              0|  00  |  10  |  20  |  30  | pe 0
              1|  01  |  11  |  21  |  31  |____
              2|  02  |  12  |  22  |  32  | pe 1 <-- a stripe
     bucket j 3|  03  |  13  |  23  |  33  |____
              4|  04  |  14  |  24  |  34  | pe 2
              5|  05  |  15  |  25  |  35  |____
            ---|------|------|------|------|
               |   f  |   f  |   f  |   f  |
                   i      i      i      i
                   l      l      l      l
                   e      e      e      e
                   0      1      2      3
@endverbatim
 * For example, pe 1 would write records 2 thru 3 across all files.  
 * PE 1's stripe would look like:
 *
 *             [02,03,12,13,22,23,32,33]
 *
 * @see work()
 */
void make_files_sequential_striped64( );
void make_files_sequential_striped128( );

/**
 * Read files by stripes.
 *
 * This function reads in the bucket files created by the mkfiles family
 * of functions.  For a given bucket number, each process reads in a range of
 * records across all files (where the range of records was calculated in the
 * work() function).  A parallel sort is carried out and then the data is 
 * written to the output file.
 @verbatim
 
               XY=a record
               where X=file id and Y=record number
            ___|------|------|------|------|____
              0|  00  |  10  |  20  |  30  | pe 0
              1|  01  |  11  |  21  |  31  |____
              2|  02  |  12  |  22  |  32  | pe 1 <-- a stripe
     bucket j 3|  03  |  13  |  23  |  33  |____
              4|  04  |  14  |  24  |  34  | pe 2
              5|  05  |  15  |  25  |  35  |____
            ---|------|------|------|------|
               |   f  |   f  |   f  |   f  |
                   i      i      i      i
                   l      l      l      l 
                   e      e      e      e
                   0      1      2      3
 @endverbatim
 * For example, pe 1 would read in records 2 thru 3 across all files.  PE 1's
 * stripe would look like: [02,03,12,13,22,23,32,33]
 *
 * @see work()
 */
void read_files_striped64( );

/**
 * Stub function for future expandability.  Currently a noop.
 */
void read_files_striped128( );

/**
 * Reads files top to bottom.
 *
 * This function reads in the bucket files created by the mkfiles family of 
 * functions.  For a given bucket number, each process reads in the range of
 * files that it created (where the range of files was calculated in the work()
 * function).  A parallel sort is carried out and then the data is written to 
 * the output file.
 @verbatim
              XY=a record
              where X=file id and Y=record number
           ___|------|------|------|------|------|
             0|  00  |  10  |  20  |  30  |  40  |
             1|  01  |  11  |  21  |  31  |  41  |
             2|  02  |  12  |  22  |  32  |  42  |
    bucket j 3|  03  |  13  |  23  |  33  |  43  |
             4|  04  |  14  |  24  |  34  |  44  |
             5|  05  |  15  |  25  |  35  |  45  |
           ---|------|------|------|------|------|
              |   f  |   f  |   f  |   f  |   f
                  i      i      i      i      i
                  l      l      l      l      l
                  e      e      e      e      e
                  0      1      2      3      4
               |                  |             |
               |--------|---------|------|------|
                        |                |
                        |                |
                       pe0              pe1
 @endverbatim
 * Each pe reads in the buckets across the files that it created.  PE 0's reads
 * would be: [00,01,02,03,04,05,10,11,12,13,14,15,20,21,22,23,24,25]
 * @see work()
 */
void read_files_exclusive64( );

/**
 * Stub function for future expandability.  Currently a noop.
 */
void read_files_exclusive128( );

#endif
