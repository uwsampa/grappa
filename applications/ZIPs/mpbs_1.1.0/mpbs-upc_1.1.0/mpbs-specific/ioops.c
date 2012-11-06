#include "ioops.h"
#include "utils.h"
#include "kernels.h"
#include "rand64.h"
#include "hpc_output.h"
#include "timer.h"
#include "psort.h"

#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <upc.h>
#include <upc_collective.h>
#include <upc_io.h>


#define barrier_all()  upc_barrier

static uint64 num_recs         = 0; //< Records per bucket this PE is responsible for.

void wrt_blk( int m, int bc, int bs, int mbs, uint64 bsize,
	      uint32 *a32, uint64 *a64, FILE *pt_bf )
{
   int i;

   uint64 seekto;

   if( a32 == NULL )
   {
      seekto = (uint64)( (bc-1)*bs*sizeof(uint64) + m*bsize );
      fseek( pt_bf, seekto, SEEK_SET );
      fwrite( a64, mbs*sizeof(uint64), 1, pt_bf );
   }
   else
   {
      seekto = (uint64)( (bc-1)*bs*sizeof(uint32) + m*bsize );
      fseek( pt_bf, seekto, SEEK_SET );
      fwrite( a32, mbs*sizeof(uint32), 1, pt_bf );
   }
   return;
}

void open_file_group(FILE *stream_ptrs[], int start, int stop) {
  int j = 0;
  int num_files = stop - start;

  /*
   * Open a list of bucket files.
   */
  for( j = 0; j <= num_files; j++ ) {
    char *pt_fname = mkfname( j + file_start );
    if( !( stream_ptrs[j] = fopen( pt_fname, "r" ) ) ) {
      perror( "fopen" );
      exit( 1 );
    }
      
    free(pt_fname);
  }
}

void execute_presort_kernels() {
  /*
   * Collective communication/computation kernel
   */
  if(kiters) {
    timer t;
    timer_clear(&t);

    printf_single(id, " collective/compute kernel...\n");
    
    timer_start(&t);
    collective_compute_kernel(kiters, npes, id, kbufsize);
    timer_stop(&t);
    if(id == 0)
      printf("\tCPU: %-11.4lf Wall: %-11.4lf Rate: N/A\n", 
	     t.accum_cpus, t.accum_wall);

    log_times(pt_log, id, "NETSTRESS", &t, 0); 
  }

  /*
   * POPCNT
   * This kernel is called before the sort since it allocates a large
   * number of integers on the stack.
   */
  if(popcntiters) {
    timer t;
    timer_clear(&t);
    uint64_t nops;
    printf_single(id, " POPCNT kernel...\n");
    nops = popcnt_kernel(popcntiters, &t);
    print_times_and_rate(id, &t, nops, "popcnts");
    
    log_times(pt_log, id, "POPCNT", &t, nops); 
  }
}


// Subroutine to execute a series of compute kernels if desired.
void execute_postsort_kernels() {
  /*
   * TILT
   */
  if(tiltiters) {
    timer t;
    timer_clear(&t);
    uint64_t nops;
    printf_single(id, " TILT kernel...\n");
    nops = tilt_kernel( tiltiters, &t );
    print_times_and_rate(id, &t, nops, "tilts");

    log_times(pt_log, id, "TILT", &t, nops); 
  }
  
  /*
   * BMM
   */
  if(bmmiters) {
    timer t;
    timer_clear(&t);
    uint64_t nops;
    printf_single(id, " BMM kernel...\n");
    nops = bmm_kernel( bmmiters, &t );
    print_times_and_rate(id, &t, nops, "bit matrix multiplies");

    log_times(pt_log, id, "BMM", &t, nops); 
  }
  
  /*
   * GMP
   */
  if(gmpiters) {
    timer t;
    timer_clear(&t);
    uint64_t nops;
    printf_single(id, " GMP kernel...\n");
    nops = gmp_kernel( gmpiters, &t );
    print_times_and_rate(id, &t, nops, "mpz_powm's");

    log_times(pt_log, id, "GMP", &t, nops); 
  } 

  /*
   * MMBS2
   */
  if(mmbs2iters) {
    timer t;
    timer_clear(&t);
    uint64_t nops;
    printf_single(id, " MMBS2 kernel...\n");
    nops = mmbs2_kernel( mmbs2iters, &t );
    print_times_and_rate(id, &t, nops, "bit serial squares");

    log_times(pt_log, id, "MMBS2", &t, nops); 
  } 
}


void make_files_sequential_exclusive64( void )
/********1*****|***2*********3*********4*********5*********6*********7**!

               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************

source file:   ioops.src

description:   This function creates the 64bit bucket files based on
               the index range calculated in the work() function.

 invoked by:   routine            description
               --------------     --------------------------------------
               main             - int (mpbs.src)
                                  This is the main driving program for
                                  the MPBS system. 

    invokes:   routine            description
               ---------------    --------------------------------------
               mkfname          - char *
                                  Creates a file name based on based on
                                  range given and returns pointer to
                                  file name string.

               rand64           - uint64
                                  Returns a 64bit unsigned integer.  Since
                                  there is no native rand() function for
                                  producing 64bit integers, 64bit integers
                                  will be produced by calling two 32bit
                                  integers and shifting one of them into
                                  the high 32bits of a 64bit integer.
                                  The other 32bit integer is then placed
                                  in the low 32bits of the 64bit integer.

  variables:   varible            description
               --------------     --------------------------------------
               pt_fname         - char *
                                  Pointer to each file name string.

               i                - int
                                  Variable for looping over files.

               base             - int
                                  Number of bits representing number.

               shft_bits        - int
                                  Number of high order bits representing
                                  bucket number.

               walle            - double
                                  Elapsed wall time in seconds.

               almem            - uint64
                                  Local memory allocation counter.

               j,k              - uint64
                                  Variables for looping over buckets and
                                  records, respectively.

               abucket          - uint64 *
                                  A bucket of 64bit numbers.

               num              - uint64
                                  Temporary holding place for number
                                  that has been manipulated to store
                                  bucket number in shft_bits high
                                  orders bits.

               m                - uint64
                                  The bucket number shifted up to the
                                  shft_bits high order bit positions.

               start_mkbuck     - times
                                  Wall clock & clock tick times.
                                  This is the start time for making one
                                  bucket of a file.

               start_wrtbuck    - times
                                  Wall clock & clock tick times.
                                  This is the start time for writing one
                                  bucket of a file.

               pt_bf            - FILE *
                                  Pointer to each open file.

       lmod:   10/05/11

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   char *pt_fname;

   int i;
   int base;
   int shft_bits;

   uint64 almem = 0;
   uint64 j, k;
   uint64 *abucket;
   uint64 num, m;

   /*
    * Used to label each line in the log file
    */ 
   char description[80];

   /*
    *  execution timing
    */
   timer t;

   FILE *pt_bf;

   /* calc mask bits */
   shft_bits = log2( nb );
   base = 8*sizeof( uint64 );

   /* make a bucket */
   almem = rb * sizeof( uint64 );
   abucket = (uint64 *)malloc( almem );
   if( !abucket )
      perror( "malloc" );

   /*
    * add to global memory total
    */
   //gma += almem;

   /*
    * loop over all files for which this process is responsible
    */
   srand64( time(NULL)*(id) );

   printf_single(id, "pe %d\n", id);

   for( i = file_start; i <= file_stop; i++ ) {
     printf_single(id, "file %d\n", i);

     pt_fname = mkfname( i );
     if( !( pt_bf = fopen( pt_fname, "w" ) ) ) {
       perror( "fopen" );
       exit( 1 );
     }
     
     /*
      * loop over buckets
      */
     for( j = 0L; j < nb; j++ ) {
       m = j;
       m <<= (base - shft_bits);
       
       printf_single(id, "bucket %3lu:\n", j);
       printf_single(id, "  filling...");

       /*
	* fill the bucket by looping over records
	*/
       timer_start(&t);
       for( k = 0L; k < rb; k++ ){
	 num = (uint64)rand64();
	 num <<= shft_bits;
	 num >>= shft_bits;
	 num += m;
	 abucket[k] = num;
       }
       timer_stop(&t);
       
       print_times_and_rate( id, &t, bsize * npes, "\bB" );
       sprintf(description, "FILL F %lu B %lu", i, j);
       log_times(pt_log, id, description, &t, bsize * npes);
       
       printf_single(id, "  writing..." );

       timer_clear(&t);
       timer_start(&t);

       /*
	* write bucket to file
	*/
       fwrite( abucket, bsize, 1, pt_bf );
       timer_stop(&t);

       print_times_and_rate( id, &t, bsize * npes, "\bB" );
       sprintf(description, "WRITE F %lu B %lu", i, j);
       log_times(pt_log, id, description, &t, bsize * npes);

       timer_clear(&t);
     }
     fclose( pt_bf );
     free( pt_fname );
   }

   /*
    * free bucket after all files written
    */
   free( abucket );
   return;
}

void make_files_sequential_exclusive128( void )
{
   printf( "found 128bit funct\n" );
   return;
}

void make_files_random_exclusive64( void )
/********1*****|***2*********3*********4*********5*********6*********7**!

               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************

source file:   ioops.src

description:   This function creates the 64bit bucket files based on
               the index range calculated in the work() function.
               The files are created by randomly filling buckets.

 invoked by:   routine            description
               --------------     --------------------------------------
               main             - int
                                  This is the main driving program for
                                  the MPBS system.

    invokes:   routine            description
               ---------------    --------------------------------------
	       mkfname          - char *
                                  Creates a file name based on based on
                                  range given and returns pointer to
                                  file name string.

               brand64          - inline uint64
                                  Returns a 64bit unsigned integer with
                                  the high order bits set to a random
                                  bucket number.  Since there is no
                                  native rand() function for producing
                                  64bit integers, 64bit integers will be
                                  produced by calling two 32bit integers
                                  and shifting one of them into the high
                                  32bits of a 64bit integer.  The other
                                  32bit integer is then placed in the low
                                  32bits of the 64bit integer.

               wrt_blk          - void
                                  Writes bucket records to file in
                                  blocks.

               get_done         - int
                                  Determines if all blocks have been
                                  filled.  Returns 1 if all full.

  variables:   varible            description
               --------------     --------------------------------------
               pt_fname         - char *
                                  Pointer to each file name string.

               i, j             - int
                                  Variable for looping over files and
                                  buckets respectively.

               blk_size         - int
                                  Number of records that are written
                                  to file per pass.

               num_blks         - int
                                  Number of blk_size blocks in a bucket.

               rem_recs         - int
                                  Number of records beyond num_blks *
                                  blk_size.

               done             - int
                                  Logical used to set for all buckets
                                  being full.

               base             - int
                                  Number of bits representing number.

               shft_bits        - int
                                  Number of high order bits representing
                                  bucket number.

               num              - uint64
                                  Record w/ bucket number in shft_bits
                                  high orders bits.

               m                - uint32
                                  Bucket number.

               buckets          - record *
                                  Bucket accounting system used to track
                                  the filling process and which buckets
                                  are full.

               pt_bf            - FILE *
                                  Bucket file.

       lmod:   12/01/11

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declarations
    */
   char *pt_fname;

   int i;
   int j;
   int k;
   int blk_size=4096;
   int num_blks;                /* number of blocks  */
   int rem_recs;                /* remaining records */
   int done = 0;                /* all buckets full  */
   int base;
   int shft_bits;
   int m = 0;

   uint64 n64 = 0UL;
   uint64 num = 0UL;
   uint64 almem = 0;

   typedef struct {
      unsigned int rc;          /* record count in a block              */
      unsigned int bd;          /* bucket done flag: 0=not done, 1=done */
      unsigned int bc;          /* block count                          */
      unsigned int ov;          /* overflow flag/state: 0=not, 1=over   */
      unsigned int bs;          /* block size                           */
      uint64 *bucket;           /* number array                         */
   } record;

   /* declare bucket accounting records */
   record *buckets;

   FILE *pt_bf;

   /*
    * Used to label each line in the log file
    */ 
   char description[80];

   /*
    *  execution timing
    */
   timer t;

   /*
    * calc mask bits for shifting off number of high order
    * bucket bits
    */
   shft_bits = log2( nb );
   base = 8*sizeof( uint64 );

   /*
    * must check if rb is greater than bs
    */
   if( rb < blk_size )
      blk_size = rb;

   /*
    * calculate whole number of record blocks
    * and number of overflow records
    */
   num_blks = rb / blk_size;
   rem_recs = rb % blk_size;

   /*
    * allocate buckets array
    */
   almem = nb * sizeof( record );
   buckets = (record *)malloc( almem );
   //gma += almem;
   if( !buckets )
   {
      perror( "malloc" );
      exit( 1 );
   }
   almem = nb * blk_size * sizeof( uint64 );
   //gma += almem;
   for( j = 0; j < nb; j++ )
   {
      buckets[j].bucket=(uint64 *)malloc( blk_size * sizeof( uint64 ) );
      if( !buckets[j].bucket )
      {
         perror( "malloc" );
         exit( 1 );
      }
   }

   /*
    * loop over all files for which this process is responsible
    */
   for( i = file_start; i <= file_stop; i++ )
   {
      printf_single(id, " file %d\n", i);
      /*
       * must reset done status
       * for next file
       */
      done = 0;

      /*
       * re-initialize bucket accounting
       */
      for( j = 0; j < nb; j++ )
      {
         buckets[j].rc=0;          /* record counter */
         buckets[j].bd=0;          /* bucket done    */
         buckets[j].bc=0;          /* block count    */
         buckets[j].ov=0;          /* overflow state */
         buckets[j].bs=blk_size;   /* block size     */
      }

      /*
       * make file name and create file
       */
      pt_fname = mkfname( i );
      if( !( pt_bf = fopen( pt_fname, "w" ) ) )
      {
         perror( "fopen" );
         exit( 1 );
      }

      /*
       * seed random number generator
       */
      srand64( time( NULL )*(id) );

      /*
       * loop until all buckets are full/done
       */
      timer_start(&t);
      while( !done )
      {
         /*
          * grab pseudo-random number
          */
         num = (uint64)rand64();

         num <<= shft_bits;
         num >>= shft_bits;

         /*
          * generate bucket number
          */
         m = (int)(rand64()%nb);
         n64 = (uint64)m; 

         /*
          * add in bucket number of ho bits
          */
         num += n64 << (base - shft_bits);

         /*
          * bucket m done?
          */
         if( !buckets[m].bd )
         {
            /*
             * m not full then put number in bucket
             */
            buckets[m].bucket[buckets[m].rc++] = num;

            /*
             * must check if record block is full
             */
            if( buckets[m].rc == buckets[m].bs )
            {
               /*
                * block is full then must be
                * written to file
                */
               buckets[m].bc++;               /* increment block count */
               buckets[m].rc=0;               /* reset record count    */

               /*
                * write block
                */
               wrt_blk( m, buckets[m].bc, blk_size, buckets[m].bs, bsize,
                           NULL, buckets[m].bucket, pt_bf );

               /*
                * check if bucket is full
                */
               if( buckets[m].ov || ( !rem_recs && buckets[m].bc == num_blks ) )
                  buckets[m].bd = 1;
            }
            /*
             * check if remainder records are being written
             */
            if( buckets[m].bc == num_blks && !buckets[m].ov )
            {
               buckets[m].bs = rem_recs;
               buckets[m].ov = 1;
            }
         }

         /*
          * check all buckets for done-ness
          */
         //done = get_done( nb, buckets );
         done = 1;
         for( k = 0; k < nb; k++ )
         {
            if( buckets[k].bd == 0 )
               done = 0;
         }
      }
      fclose( pt_bf );
      free( pt_fname );

      timer_stop(&t);
      
      print_times_and_rate( id, &t, bsize * nb * npes, "\bB" );
      sprintf(description, "WRITE F %lu", i);
      log_times(pt_log, id, description, &t, bsize * nb * npes );

      timer_clear(&t);
   }

   /*
    * de-allocate buckets array
    */
   for( j = 0; j < nb; j++ )
      free( buckets[j].bucket );
   free( buckets );

   return;
}

void make_files_random_exclusive128( void )
{
   printf( "found mkfrnd 128bit funct\n" );
   return;
}


void make_files_sequential_striped64( void )
/********1*****|***2*********3*********4*********5*********6*********7**!

               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************

source file:   ioops.src

description:   This function writes the 64bit bucket files.  For a
               given bucket number, each process writes a range of
               records across all files, where the range of records
               was calculated in the work() function.

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

               For example, pe 1 would write records 2 thru 3 across
               all files.  pe 1's stripe would look like:

               [02,03,12,13,22,23,32,33]

 invoked by:   routine            description
               --------------     --------------------------------------
               main             - int (mpbs.c)
                                  This is the main driving program for
                                  the MPBS system.

    invokes:   routine            description
               ---------------    --------------------------------------
               mkfname          - char * 
                                  Creates a file name based on based on
                                  range given and returns pointer to
                                  file name string.

               rand64           - uint64 
                                  Returns a 64bit unsigned integer.
                                  Since there is no native rand()
                                  function for producing 64bit integers,
                                  64bit integers will be produced by
                                  calling two 32bit integers and
                                  shifting one of them into the high
                                  32bits of a 64bit integer.  The other
                                  32bit integer is then placed in the
                                  low 32bits of the 64bit integer.

  variables:   varible            description
               --------------     --------------------------------------
               pt_fname         - char *
                                  Pointer to each file name string.

               i                - int
                                  Loop over buckets.

               j                - int
                                  Loop over files.

               k                - int
                                  Loop over records in a stripe.

               shft_bits        - int
                                  Number of high order bits representing
                                  bucket number.

               base             - int
                                  Number of bits representing a number.

               m                - uint64
                                  Stripe index number for writing
                                  stripe block to each file.

               num              - uint64
                                  Record w/ bucket number in shft_bits
                                  high orders bits.

               bid64            - uint64
                                  Bucket id derived from i;

               almem            - uint64
                                  Local memory allocation counter.

               stripe_blk       - uint64
                                  Number of records per bucket that
                                  an process will handle.

               seekto           - uint64
                                  Number of bytes to skip over in
                                  a file for reading or writing.

               skip             - uint64
                                  Number of records from a given
                                  process for each bucket.  Used to
                                  calculate a sum total of records
                                  over which to skip.

               pt_bf            - FILE *
                                  Pointer to open data file.

       lmod:   12/05/11

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   char *pt_fname;
   char description[80];   /* Used to label each line in the log file */

   int i, j, k;
   int shft_bits;
   int base;

   uint64 m          = 0;
   uint64 num        = 0;
   uint64 bid64      = 0;
   uint64 almem      = 0;
   uint64 stripe_blk = 0;
   uint64 seekto     = 0;
   uint64 skip       = 0;
   uint64 *stripe    = NULL;

   FILE *pt_bf;

   /*
    *  execution timing
    */
   timer t;

   /* function prototypes */
   char   *mkfname( int );
   uint64 rand64( void );

   /*
    * allocate memory for stripe
    */
   stripe_blk = bstripe_stop - bstripe_start + 1;
   almem = nf*stripe_blk*rs;
   //gma += almem;      /* add to global memory total */
   stripe = (uint64 *)malloc( almem );
   if( !stripe )
   {
      perror( "malloc" );
      printf( "mkfstp64: stripe vector mallocation failure %u!\n", stripe );
      exit( 1 );
   }

   /*
    * calc mask bits
    */
   shft_bits = log2( nb );
   base = 8*sizeof( uint64 );

   /*
    * number of records this pe will be responsible for
    * each bucket stripe
    */
   num_recs = stripe_blk * nf;

   /*
    * initialize random number generator
    */
   srand64( time( NULL )*(id) );

   /*
    * loop over buckets
    */
   m = 0;
   for( i = 0; i < nb; i++ )
   {
      printf_single(id, "bucket %3lu\n", i );
      printf_single(id, " writing...\n");

      timer_clear(&t);
      timer_start(&t);

      bid64   = (uint64)i;            /* convert to u64 number           */
      bid64 <<= (base - shft_bits);   /* shift to HO bits                */

      /*
       * fill stripe with pseudo-random numbers
       * adjusted for bucket number
       */
      for( k = 0; k < num_recs; k++ )
      {
         num   = rand64();       /* get a pseudo-random number      */
         num <<= shft_bits;      /* shift-off number of bucket bits */
         num >>= shft_bits;      /* shift back bits-now all zeros   */
         num  += bid64;          /* add bucket number to HO bits    */
         stripe[k] = num;        /* put number in stripe            */
      }
      barrier_all();

      /*
       * loop over files
       */
      for( j = 0; j < nf; j++ )
      {
         pt_fname = mkfname( j );
         if( !( pt_bf = fopen( pt_fname, "r+" ) ) )
         {
            perror( "fopen" );
            exit( 1 );
         }

         /*
          * read and store records in stripe blocks
          */
         seekto = (uint64)( i*bsize + bstripe_start*rs );
         fseek( pt_bf, seekto, SEEK_SET );
         fwrite( &stripe[m], rs, stripe_blk, pt_bf );
         fclose( pt_bf );
         m += stripe_blk;
      } /* end of file loop */
      m = 0;

     /*
      * timing summary
      */
     timer_stop(&t);

     print_times_and_rate( id, &t, bsize * nf, "\bB" );
     sprintf(description, "WRITE B %lu", i);
     log_times(pt_log, id, description, &t, bsize * nf);

     timer_clear(&t);

     barrier_all();
   }
   return;
}


void make_files_sequential_striped128( void )
{
   printf( "found mkfstp 128bit funct\n" );
   return;
}


void read_files_striped64()
/********1*****|***2*********3*********4*********5*********6*********7**!

               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************

description:   This function reads in the 64bit bucket files.  For a
               given bucket number, each process reads in a range of
               records across all files, where the range of records
               was calculated in the work() function.  A parallel sort
               is carried out and then the data is written to the
               output file.

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

               For example, pe 1 would read in records 2 thru 3 across
               all files.  pe 1's stripe would look like:

               [02,03,12,13,22,23,32,33]

 invoked by:   routine            description
               --------------     --------------------------------------
               main             - int (mpbs.c)
                                  This is the main driving program for
                                  the MPBS system.

    invokes:   routine            description
               ---------------    --------------------------------------
               mkfname          - char *  (utils.src)
                                  This subroutine reads in process's
                                  record stripe which are all of the
                                  records striped across all files for a
                                  given bucket.

               psortui64        - uint64 *
                                  Returns memory location of parallel
                                  bucket sorted data for each pe.

  variables:   varible            description
               --------------     --------------------------------------
               k                - int
                                  Loops over processes.

               bbits            - int
                                  The number of high order bits in the
                                  record number allocated to the bucket
                                  number.

               maxn             - int
                                  Maximum number of sstripe elements
                                  across all pes for a given bucket.

               recvs            - int *
                                  Array indicating how many elements
                                  a given pe will obtain from other
                                  pes by bucket id(array index number).
                                  Used to fill map array.  Also used
                                  to hold sums of the number of elements
                                  each pe has for its sstripe array. 

               maxnum           - int *
                                  Array holds the maximum number of
                                  sorted array elements for each bucket.

               almem            - uint64
                                  Local memory allocation counter.

               i                - int
                                  Variable for looping over files.

               j                - uint64
                                  Variable for looping over files.

               m                - uint64
                                  The bucket number shifted up to the
                                  shft_bits high order bit positions.

               bssize           - uint64
                                  The total size in bytes of a bucket
                                  stripe.

               stripe_blk       - uint64
                                  Number of records per bucket that
                                  an process will handle.

               seekto           - uint64
                                  Number of bytes to skip over in
                                  a file for reading or writing.

               skip             - uint64
                                  Number of records from a given
                                  process for each bucket.  Used to
                                  calculate a sum total of records
                                  over which to skip.

               sum              - uint64
                                  Sum total of records over which to
                                  skip for each process in a given
                                  bucket.

               stripe           - uint64 *
                                  Allocated for each process to read
                                  in its stripe of records.

               sstripe          - uint64 *
                                  Allocated for each process to
                                  store its sorted data. 

               pt_fname         - char *
                                  Pointer to each file name string.

               pt_bf            - FILE *
                                  Pointer to open data file.

               pt_sf            - FILE *
                                  Pointer to file of sorted data.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
  char *pt_fname;

  int k;
  int bbits;
  int maxn;
  int *recvs;
  int *maxnum;
  
  uint64 almem = 0;
  
  uint64 i, j, m;
  uint64 bssize     = 0;
  uint64 stripe_blk = 0;
  uint64 seekto     = 0;
  uint64 skip       = 0;
  uint64 sum        = 0;

  /*
   * Used to label each line in the log file
   */ 
  char description[80];  

  shared uint64 * sh_sorted_stripe = NULL; // Sorted data, affinity to this PE.
  shared uint64 * sh_stripe        = NULL; // Unsorted data
  uint64 * p_sorted_stripe         = NULL; // Private alias to sorted_stripe.    
  uint64 * p_stripe                = NULL;

  shared uint64 * sh_recvs;
  
  FILE *pt_bf;
  FILE *pt_sf;
  
  upc_file_t *upc_pt_sf;
  
  /* 
   * Open the global file.  Craycc fails to compile the following line.
   */
#ifndef _CRAYC
  upc_hint_t hint_list = {"access_style", "write_mostly,sequential"};
#endif
  if( !( upc_pt_sf = upc_all_fopen( pt_ofpath, UPC_RDWR | UPC_INDIVIDUAL_FP, 0, NULL))) 
    //if( !( pt_sf = fopen( pt_ofpath, "rb+" ) ) )
    {
      perror( "fopen" );
      exit( 1 );
    }

  /*
   *  execution timing
   */
  timer t;
  

  /*
   * calculate bucket stripe size to pass to parallel sort
   * for sorting rate calculation
   */
  bssize = nf * bsize; 
  
  /*
   * allocate memory for stripe
   */
  stripe_blk = bstripe_stop - bstripe_start + 1;
  almem = nf * stripe_blk * rs;
   
  sh_stripe     = (shared uint64 *)upc_all_alloc( THREADS, almem );
  if( !sh_stripe ) {
    perror( "malloc" );
    exit( 1 );
  }
  p_stripe = (uint64 *)sh_stripe;
  
  recvs         = malloc( THREADS * sizeof(int) );
  if( !recvs ) {
    perror( "malloc" );
    exit( 1 );
  }

  maxnum        = (int *)malloc( nb * sizeof(int) ); 
  if( !maxnum ) {
    perror( "malloc" );
    exit( 1 );
  }
  
  sh_recvs      = (shared uint64 *)upc_all_alloc( THREADS, sizeof(uint64) );
  if( !sh_recvs ) {
    perror( "upc_all_alloc" );
    exit( 1 );
  }
  
  upc_barrier;
  
  /*
   * number of records this pe will be responsible for
   * each bucket
   */
  num_recs = stripe_blk * nf;
  
  /*
   * number of bucket bits that will have to
   * be shifted off in the sort routine
   */
  bbits = log2( nb );
  
  printf_single(id, "All processes reading stripes.\n");
  
  /*
   * loop over buckets
   */
  m = 0;
  for( i = 0; i < nb; i++ ) {
    printf_single(id, "bucket %3lu\n", i );
    printf_single(id, " reading...\n");
    
    timer_clear(&t);
    timer_start(&t);
    
    /*
     * reset sum to zero for each bucket
     */
    sum = 0;
    
    /*
     * reset number of records for file reads
     */
    num_recs = stripe_blk * nf;
    
    /*
     * loop over ALL files and load this thread's portion
     * of the bucket stripe.
     */
    for( j = 0; j < nf; j++ ) {
      pt_fname = mkfname( j );
      if( !( pt_bf = fopen( pt_fname, "r" ) ) ) {
	perror( "fopen" );
	exit( 1 );
      }
      
      /*
       * read and store records in stripe blocks
       */
      seekto = (uint64)( i*bsize + bstripe_start*rs );
      fseek( pt_bf, seekto, SEEK_SET );
      fread( &p_stripe[m], rs, stripe_blk, pt_bf );
      fclose( pt_bf );
      m += stripe_blk;
      free(pt_fname);
    } 
    m = 0;
    
    timer_stop(&t);
    print_times_and_rate( id, &t, bsize * nf, "\bB" );
    sprintf(description, "READ B %lu", i);
    log_times(pt_log, id, description, &t, bsize * nf);
    
    execute_presort_kernels();
    
    printf_single(id, " sorting...\n");
    
    upc_barrier;
    
    timer_clear(&t);
    timer_start(&t);
    
    /*
     * sort my stripe
     */
    sh_sorted_stripe=psortui64( npes, id, bbits, bssize, &maxnum[i], num_recs,
				p_stripe, sh_recvs, pt_bsort );
    
    /*
     * timing summary
     */
    timer_stop(&t);
    printf_single(id, "  total\b\b\b\b\b" );
    print_times_and_rate( id, &t, bsize*nf, "\bB" );
    log_times(pt_log, id, "TOTAL SORT", &t, bsize * nf);
    
    execute_postsort_kernels();
    
    /*
     * set number of elements to
     * new value from psort
     */
    num_recs = recvs[id];
    
    /* shared memory for an array to be used in a prefix sum
     * calculation for each PE to determine how much of the output array
     * it is receiving from the other PEs.
     */
    sh_recvs[id] = num_recs;
    
    upc_barrier;
    
    /*
     * sum up number of stripes this process will have
     * to skip by summing up the number of records from
     * all lower order pes.  This clearly limits scalability
     * and should be replaced by a proper prefix sum.
     * TODO: Replace with a prefix sum.
     */
    for( k = 0; k < id; k++ ) {
      sum += sh_recvs[k];
    }
    
    upc_barrier;
    
    /*
     * timing and logging
     */
    printf_single(id, " writing...\n" );
    
    /*
     * write my records to specific
     * place in global sorted file
     */
    seekto = (uint64)( i*bsize*nf + sum*rs );
    
    /*
     * Create a private alias to the sorted data for use by the C library.
     */
    p_sorted_stripe = (uint64 *)sh_sorted_stripe;
    
    timer_clear(&t);
    timer_start(&t);
    
    /*      fseek( pt_sf, seekto, SEEK_SET ); */
    /*      fwrite( p_sorted_stripe, rs*num_recs, 1, pt_sf ); */
    /*      fclose( pt_sf ); */
    ssize_t bytes_written;
    upc_off_t offset;
    
    offset = upc_all_fseek( upc_pt_sf, seekto, UPC_SEEK_SET );
    if( offset < 0 ) {
      perror("ERROR: upc_all_fseek failure");
      printf("seekto is %lu\n", seekto);
    }
    
    bytes_written = upc_all_fwrite_local( upc_pt_sf, p_sorted_stripe, rs, num_recs, 
					  UPC_IN_NOSYNC | UPC_OUT_NOSYNC );     
    if( bytes_written != rs * num_recs ) {
      if( bytes_written < 0) {
	perror("ERROR: upc_all_fwrite failure");
      }
      printf("ERROR (%d): Asked for write of %lu bytes and got %d instead\n", id, rs*num_recs, bytes_written);
    }
    
    /*
     * timing and logging summary
     */
    timer_stop(&t);
    print_times_and_rate( id, &t, bsize*nf, "\bB" );
    log_times(pt_log, id, "WRITE OUTPUT", &t, bsize * nf);
    
    /*
     * free memory allocated during the parallel sort and synchronize
     * all pes after each bucket is processed
     */
    if( id == 0 )
      upc_free( sh_sorted_stripe );
    
    upc_barrier;
  } 
  
  upc_all_fclose( upc_pt_sf );
  
  if( id == 0 ) {
    upc_free(sh_stripe);
    upc_free(sh_recvs);
  }
  
  free(recvs);
  free(maxnum);
  
  return;
}


void read_files_striped128( void ) {
  printf( "found rstrps 128bit funct\n" );
  return;
}

void read_files_exclusive64( void )
/********1*****|***2*********3*********4*********5*********6*********7**!

               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************

source file:   ioops.src

description:   This function reads in the 64bit bucket files.  For a
               given bucket number, each process reads in the range of
               files that it created, where the range of files 
               was calculated in the work() function.  A parallel sort
               is carried out and then the data is written to the
               output file.

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

               Each pe reads in the buckets across the files that
               it created.

               pe0s reads would be:
               pe0->[00,01,02,03,04,05,10,11,12,13,14,15,20,21,22,23,24,25]

 invoked by:   routine            description
               --------------     --------------------------------------
               main             - int
           
    invokes:   routine            description
               ---------------    --------------------------------------
               mkfname          - char *  (utils.src)
                                  This subroutine reads in process's
                                  record stripe which are all of the
                                  records striped across all files for a
                                  given bucket.

               psortui64        - uint64 *
                                  Returns memory location of parallel
                                  bucket sorted data for each pe.

  variables:   varible            description
               --------------     --------------------------------------
               i,j,k            - int
                                  Variables for looping over buckets,
                                  files and pes, respectively.

               bbits            - int
                                  The number of high order bits in the
                                  record number allocated to the bucket
                                  number.

               maxn             - int
                                  Maximum number of sstripe elements
                                  across all pes for a given bucket.

               recvs            - int *
                                  Array indicating how many elements
                                  a given pe will obtain from other
                                  pes by bucket id(array index number).
                                  Used to fill map array.  Also used
                                  to hold sums of the number of elements
                                  each pe has for its sstripe array. 

               maxnum           - int *
                                  Array holds the maximum number of
                                  sorted array elements for each bucket.

               almem            - uint64
                                  Local memory allocation counter.

               m                - uint64
                                  Counts records in data array for
                                  for proper placement of reads.

               bssize           - uint64
                                  The total size in bytes of a bucket
                                  stripe.

               file_blk         - uint64
                                  Number of file that an process
                                  will handle.

               seekto           - uint64
                                  Number of bytes to skip over in
                                  a file for reading or writing.

               skip             - uint64
                                  Number of records from a given
                                  process for each bucket.  Used to
                                  calculate a sum total of records
                                  over which to skip.

               sum              - uint64
                                  Sum total of records over which to
                                  skip for each process in a given
                                  bucket.

               buckets          - uint64 *
                                  Allocated for each process to read
                                  in its file_blk*rb records.

               sbuckets         - uint64 *
                                  Allocated for each process to
                                  store its sorted data. 

               pt_fname         - char *
                                  Pointer to each file name string.

               pt_bf            - FILE *
                                  Pointer to open data file.

               pt_sf            - FILE *
                                  Pointer to file of sorted data.

               start_rdfls      - times
                                  Wall clock & clock tick times.
                                  This is the start time for reading a
                                  bucket across my files.

               start_wrtfls     - times
                                  Wall clock & clock tick times.
                                  This is the start time for writing a
                                  sorted bucket to output file.

               start_sort       - times
                                  Wall clock & clock tick times.
                                  This is the start time for sorting one
                                  bucket block.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
  char *pt_fname;

  int i, j, k;
  int bbits;
  int maxn;
  //int *recvs;
  int *maxnum;

  uint64 m;
  uint64 bssize     = 0;
  uint64 file_blk   = 0;
  uint64 seekto     = 0;
  uint64 skip       = 0;
  uint64 sum        = 0;

  /*
   * Used to label each line in the log file
   */
  char description[80];


  shared uint64 *sh_buckets        = NULL;
  shared uint64 *sh_sorted_buckets = NULL; // Sorted data
  uint64 *p_buckets                = NULL;
  uint64 * p_sorted_buckets        = NULL; // Private alias to sorted_buckets

  shared uint64 * sh_recvs;
  shared uint64 * sh_prefix_sum;

  FILE *pt_bf;
  FILE *pt_sf;
  FILE **stream_ptrs;

  upc_file_t *upc_pt_sorted_file;

  /*
   *  execution timing
   */
  timer t;
   
  /* 
   * Open the global file.  Craycc fails to compile the following line.
   */
#ifndef _CRAYC
  upc_hint_t hint_list = {"access_style", "write_mostly,sequential"};
#endif
  if( !( upc_pt_sorted_file = upc_all_fopen( pt_ofpath, UPC_RDWR | UPC_INDIVIDUAL_FP, 0, NULL))) {
    //if( !( pt_sf = fopen( pt_ofpath, "rb+" ) ) )  
    perror( "fopen" );
    exit( 1 );
  }

  /*
   * calculate bucket stripe size to pass to parallel sort
   * for sorting rate calculation
   */
  bssize = nf * bsize;

  /*
   * allocate memory for buckets
   */
  file_blk = file_stop - file_start + 1;

  sh_buckets     = (shared uint64 *)upc_all_alloc(THREADS, file_blk * rb * rs);
  if( !sh_buckets ) {
    perror( "malloc" );
    exit( 1 );
  }

  p_buckets = (uint64 *)sh_buckets;

  /* recvs          = (int *)malloc( npes * sizeof( int ) ); */
/*   if( !recvs ) { */
/*     perror( "malloc" ); */
/*     exit( 1 ); */
/*   } */

  maxnum         = (int *)malloc( nb * sizeof(int) );
  if( !maxnum ) {
    perror( "malloc" );
    exit(1);
  }
     
  sh_recvs       = (shared uint64 *)upc_all_alloc( THREADS, sizeof(uint64) );
  if( !sh_recvs ) {
    perror( "sh_recvs malloc " );
    exit(1);
  }

  sh_prefix_sum  = (shared uint64 *)upc_all_alloc( THREADS, sizeof(uint64) );
  if( !sh_prefix_sum ) {
    perror( "upc_all_alloc" );
    exit( 1 );
  }
  stream_ptrs = malloc( file_blk * sizeof( FILE * ));
  barrier_all();

  /*
   * number of records this pe will be responsible for
   * each bucket
   */
  num_recs = file_blk * rb;

  /*
   * number of bucket bits that will have to
   * be shifted off in the sort routine
   */
  bbits = log2( nb );

  printf_single(id, "Opening files...\n" );

  timer_clear(&t);
  timer_start(&t);

  open_file_group(stream_ptrs, file_start, file_stop);

  barrier_all();

  timer_stop(&t);
  print_times_and_rate(id, &t, nf * npes, "fopen");
  sprintf(description, "FOPEN");
  log_times(pt_log, id, description, &t, nf * npes);

  /*
   * loop over buckets
   */
  for( i = 0; i < nb; i++ ) {
    printf_single(id, "bucket %3d\n", i );
    printf_single(id, " reading...\n" );
     
    timer_clear(&t);
    timer_start(&t);
     
    /*
     * reset sum to zero for each bucket
     */
    sum = 0;
     
    /*
     * reset number of records for file reads
     */
    num_recs = file_blk * rb;
     
    /*
     * Loop over the subset of files I'm responsible for
     * and load each bucket in it's entirety.
     */
    for( j = 0; j < file_blk; j++ ) {  
      fseek( stream_ptrs[j], i*bsize, SEEK_SET );
      fread( &p_buckets[j*rb], rs, rb, stream_ptrs[j] );
    } /* end of file loop */
    

    /*
     * timing summary
     */
    timer_stop(&t);
    print_times_and_rate( id, &t, bsize * nf, "\bB" );
    sprintf(description, "READ B %lu", i);
    log_times(pt_log, id, description, &t, bsize * nf);

    execute_presort_kernels();

    printf_single(id, " sorting...\n" );
          
    barrier_all();

    timer_clear(&t);
    timer_start(&t);
     
    /*
     * sort my buckets
     */
    sh_sorted_buckets = psortui64(npes, id, bbits, bssize, &maxnum[i],	
				  num_recs, p_buckets, sh_recvs, pt_bsort);

    timer_stop(&t);
    printf_single(id, "  total\b\b\b\b\b" );
    print_times_and_rate( id, &t, bsize * nf, "\bB" );
    log_times(pt_log, id, "TOTAL SORT", &t, bsize * nf);

    execute_postsort_kernels();

    /*
     * set number of elements to
     * new value from psortui64
     */
    //num_recs = recvs[id];
    num_recs = sh_recvs[id];

    /* shared memory for an array to be used in a prefix sum
     * calculation for each PE to determine how much of the output array
     * it is receiving from the other PEs.
     */
    //sh_recvs[id] = num_recs;
     
    //barrier_all(); // To allow sh_recvs to fully initialize.

    /*
     * sum up number of stripes this process will have
     * to skip by summing up the number of records from
     * all lower order pes.  
     * TODO: Replace with a prefix sum.
     */
    upc_all_prefix_reduceUL( sh_prefix_sum, sh_recvs, UPC_ADD, THREADS, 1, NULL, UPC_IN_ALLSYNC | UPC_OUT_ALLSYNC);
/*     for( k = 0; k < id; k++ ) { */
/*       sum += sh_recvs[k]; */
/*     } */
     
    barrier_all();
     
    printf_single(id, " writing...\n" );
     
    /*
     * Write my records to specific place in global sorted file.
     */
    //   seekto = (uint64)( i*bsize*nf + sum*rs );
    if( id == 0 )
      seekto = (uint64)(i * bsize * nf );
    else
      seekto = (uint64)(i * bsize * nf + sh_prefix_sum[id-1] * rs);
    /*
     * Create a private alias for use by the C library functions.
     */
    p_sorted_buckets = (uint64 *)sh_sorted_buckets;
     
    timer_clear(&t);
    timer_start(&t);

    ssize_t bytes_written;
    upc_off_t offset;
     
    offset = upc_all_fseek( upc_pt_sorted_file, seekto, UPC_SEEK_SET );

    if( offset < 0 ) {
      perror("ERROR: upc_all_fseek failure");
      printf("seekto is %lu\n", seekto);
    }
 
    upc_barrier;

    bytes_written = upc_all_fwrite_local( upc_pt_sorted_file, p_sorted_buckets, rs, num_recs, 
					  UPC_IN_NOSYNC | UPC_OUT_NOSYNC );   
  
    if( bytes_written != rs * num_recs ) {
      if( bytes_written < 0) {
	perror("ERROR: upc_all_fwrite failure");
      }
      printf("ERROR (%d): Asked for write of %lu bytes and got %lu instead\n", 
	     id, rs*num_recs, bytes_written);
    }


    /*
     * timing and logging summary
     */
    timer_stop(&t);
    print_times_and_rate( id, &t, bsize * nf, "\bB" );
    log_times(pt_log, id, "WRITE OUTPUT", &t, bsize * nf);
     
    /*
     * free memory for parallel sort and synchronize
     * all pes after each bucket is processed
     */
    if( id == 0 )
      upc_free( sh_sorted_buckets );
     
    barrier_all();
  } /* end bucket loop */
   
  upc_all_fclose( upc_pt_sorted_file );
   
  if( id == 0 ) {
    upc_free(sh_recvs);
    upc_free(sh_buckets);
    upc_free(sh_prefix_sum);
  }

  //free(recvs);
  free(maxnum);
  free(stream_ptrs);

  return;
}


void read_files_exclusive128( void ) {
  printf( "read_files_exclusive128 stub\n" );
  return;
}
