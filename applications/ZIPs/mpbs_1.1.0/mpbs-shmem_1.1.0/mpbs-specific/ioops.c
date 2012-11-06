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

#include <mpp/shmem.h>

#define barrier_all()  shmem_barrier_all()

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
               main             - int (mpbs.src)
                                  This is the main driving program for
                                  the MPBS system.

    invokes:   routine            description
               ---------------    --------------------------------------
               mkfname          - char *  (utils.src)
                                  Creates a file name based on based on
                                  range given and returns pointer to
                                  file name string.

               rand64           - uint64 (utils.src)
                                  Returns a 64bit unsigned integer.
                                  Since there is no native rand()
                                  function for producing 64bit integers,
                                  64bit integers will be produced by
                                  calling two 32bit integers and
                                  shifting one of them into the high
                                  32bits of a 64bit integer.  The other
                                  32bit integer is then placed in the
                                  low 32bits of the 64bit integer.

               malloc           - void *
                                  C library function that allocates u
                                  bytes of memory.  Returns a pointer
                                  to the beginning of the allocated
                                  space.

               printf           - int
                                  C library function that prints text
                                  to standard output.

               perror           - void
                                  C library function that prints to
                                  stderr the string representation of
                                  the current error described by errno
                                  when passed the string of the library
                                  function.

               exit             - void
                                  C library function that exits the
                                  program.

               log2             - double
                                  C math library function that returns
                                  the base 2 logarithm of arg.

               sizeof           - int
                                  C library operator that returns
                                  the size of its operand in bytes.

               srand            - void
                                  C library function that seeds for a new
                                  sequence of pseudo-random integers.

               time             - time_t
                                  C library function that returns the
                                  time since the Epoch measured in
                                  seconds.

               fopen            - FILE *
                                  C library function that opens a file
                                  given by pathname specified by
                                  argument.

               fseek            - int
                                  C library function that sets the file
                                  position indicator for the stream
                                  pointed to by file pointer.
                                  The new position, measured in bytes,
                                  is obtained by adding offset bytes to
                                  the position specified by SEEK_SET,
                                  meaning the offset is relative to the
                                  start of the file.

               fwrite           - size_t
                                  C library function that writes nmemb
                                  elements of data, each size bytes long,
                                  to the stream pointed to by stream,
                                  obtaining them from the location given
                                  by ptr.

               fclose           - int
                                  C library function that closes a
                                  given file object.

               return           - void
                                  C library function that returns
                                  control to calling function.

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

     author:   RLB(rlbell5@nsa)

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


void read_files_striped64( void )
/********1*****|***2*********3*********4*********5*********6*********7**!

               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************

source file:   ioops.src

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
               shmem_barrier_all- void
                                  SHMEM library function that performs
                                  a barrier operation including all PEs.

               get_times        - times
                                  Returns clock ticks and wall clock
                                  time in type times to calling function.

               mkfname          - char *  (utils.src)
                                  This subroutine reads in process's
                                  record stripe which are all of the
                                  records striped across all files for a
                                  given bucket.

               tsum             - void
                                  Prints timing summary information for
                                  various sections of execution to
                                  standard output and mpbs log file.

               psortui64        - uint64 *
                                  Returns memory location of parallel
                                  bucket sorted data for each pe.

               shmem_get64      - void
                                  SHMEM library function that provides  a
                                  high-performance method for copying a
                                  contiguous data object from a different
                                  PE to a contiguous data object on a the
                                  local PE.  The routines return after the
                                  data has been delivered to the target
                                  array on the local PE.

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

               start_rdstp      - times
                                  Wall clock & clock tick times.
                                  This is the start time for reading one
                                  bucket stripe.

               start_wrtstp     - times
                                  Wall clock & clock tick times.
                                  This is the start time for writing one
                                  bucket stripe.

               start_sort       - times
                                  Wall clock & clock tick times.
                                  This is the start time for sorting one
                                  bucket stripe.

       lmod:   11/04/11 by RLB

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
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
   
   uint64 *sstripe   = NULL;
   uint64 *stripe    = NULL;

   FILE *pt_bf;
   FILE *pt_sf;

   /*
    *  execution timing
    */
   timer t;

   /*
    * Used to label each line in the log file
    */ 
   char description[80];

   /*
    * calculate bucket stripe size to pass to parallel sort
    * for sorting rate calculation
    */
   bssize = nf * bsize; 

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
      printf( "rstrps64: stripe vector mallocation failure %p!\n", stripe );
      exit( 1 );
   }
   almem = npes*sizeof(int);
   //gma += almem;      /* add to global memory total */
   recvs = (int *)malloc( almem );
   if( !recvs )
   {
      perror( "malloc" );
      printf( "rstrps64: recvs vector mallocation failure %p!\n", recvs );
      exit( 1 );
   }
   maxnum = (int *)malloc( nb * sizeof(int) ); 
   barrier_all();
   
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
      * loop over files
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
       fread( &stripe[m], rs, stripe_blk, pt_bf );
       fclose( pt_bf );
       m += stripe_blk;
     } /* end of file loop */
     m = 0;

     /*
      * timing summary
      */
     timer_stop(&t);
     print_times_and_rate( id, &t, bsize * nf, "\bB" );
     sprintf(description, "READ B %lu", i);
     log_times(pt_log, id, description, &t, bsize * nf);

     execute_presort_kernels();

     printf_single(id, " sorting...\n");

     barrier_all();
     
     timer_clear(&t);
     timer_start(&t);
     
     /*
      * sort my stripe
      */
     sstripe=psortui64( npes, id, bbits, bssize, &maxnum[i], num_recs,
			stripe, recvs, pt_bsort );
     
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
      
     barrier_all();
     
     /*
      * sum up number of stripes this process will have
      * to skip by summing up the number of records from
      * all lower order pes
      */     
     for( k = 0; k < id; k++ ) {
       shmem_get64( &skip, &num_recs, 1, k );
       sum += skip;
     }

     barrier_all();
     
     /*
      * write my records to global file
      */
#ifdef _USE_O_DIRECT
     int fd;
     if( 0 > (fd = open(pt_ofpath, O_WRONLY | O_DIRECT )))
#else
     if( !( pt_sf = fopen( pt_ofpath, "rb+" ) ) )
#endif
       {
	 perror( "fopen" );
	 exit( 1 );
       }
     
     /*
      * timing and logging
      */
     printf_single(id, " writing...\n" );
     
     /*
      * write my records to specific
      * place in global sorted file
      */
      seekto = (uint64)( i*bsize*nf + sum*rs );

      timer_clear(&t);
      timer_start(&t);

      fseek( pt_sf, seekto, SEEK_SET );
      fwrite( sstripe, rs*num_recs, 1, pt_sf );
      fclose( pt_sf );

      /*
       * timing and logging summary
       */
      timer_stop(&t);
      print_times_and_rate( id, &t, bsize*nf, "\bB" );
      log_times(pt_log, id, "WRITE OUTPUT", &t, bsize * nf);
      
      /*
       * free memory for parallel sort and synchronize
       * all pes after each bucket is processed
       */
      shfree( sstripe );
      
      barrier_all();
      
   } /* end bucket loop */

   maxn = 0;
   for( i = 0; i < nb; i++ ) {
     if( maxnum[i] >= maxn )
       maxn = maxnum[i];
   }

   almem = maxn * sizeof( uint64 );

   free(recvs);
   free(maxnum);
   free(stripe);

   return;
}


void read_files_striped128( void )
{
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
                                  This is the main driving program for
                                  the MPBS system.

    invokes:   routine            description
               ---------------    --------------------------------------
               shmem_barrier_all- void
                                  SHMEM library function that performs
                                  a barrier operation including all PEs.

               mkfname          - char *  (utils.src)
                                  This subroutine reads in process's
                                  record stripe which are all of the
                                  records striped across all files for a
                                  given bucket.

               psortui64        - uint64 *
                                  Returns memory location of parallel
                                  bucket sorted data for each pe.

               shmem_get64      - void
                                  SHMEM library function that provides  a
                                  high-performance method for copying a
                                  contiguous data object from a different
                                  PE to a contiguous data object on a the
                                  local PE.  The routines return after the
                                  data has been delivered to the target
                                  array on the local PE.

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

       lmod:   11/07/11

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   char *pt_fname;

   int i, j, k;
   int bbits;
   int maxn;
   int *recvs;
   int *maxnum;

   uint64 almem = 0;

   uint64 m;
   uint64 bssize     = 0;
   uint64 file_blk   = 0;
   uint64 seekto     = 0;
   uint64 skip       = 0;
   uint64 sum        = 0;
   uint64 *buckets   = NULL;
   uint64 *sbuckets  = NULL;

   FILE *pt_bf;
   FILE *pt_sf;

   /*
    *  execution timing
    */
   timer t;


   /*
    * Used to label each line in the log file.
    */ 
   char description[80];

   printf_single(id, "Opening common file\n");
   
   barrier_all();
   
   timer_clear(&t);
   timer_start(&t);
   
   if( !( pt_sf = fopen( pt_ofpath, "rb+" ) ) ) {
     perror( "fopen" );
     exit( 1 );
   }
   
   barrier_all();
   
   timer_stop(&t);
   print_times_and_rate(id, &t, nf * npes, "fopen");
   sprintf(description, "FOPEN");
   log_times(pt_log, id, description, &t, nf * npes);

   /*
    * calculate bucket stripe size to pass to parallel sort
    * for sorting rate calculation
    */
   bssize = nf * bsize;

   /*
    * allocate memory for buckets
    */
   file_blk = file_stop - file_start + 1; 
   almem = file_blk * rb * rs;   
   buckets = (uint64 *)malloc( almem );
   if( !buckets )
   {
      perror( "malloc" );
      printf( "rbyfls64: buckets vector mallocation failure %p!\n", buckets );
      exit( 1 );
   }
   almem = npes*sizeof( int );
   recvs = (int *)malloc( almem );
   if( !recvs )
   {
      perror( "malloc" );
      printf( "rbyfls64: recvs vector mallocation failure %p!\n", recvs );
      exit( 1 );
   }
   maxnum = (int *)malloc( nb * sizeof(int) ); 

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

   printf_single(id, "All processes reading files.\n" );

   /*
    * loop over buckets
    */
   m = 0;
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
     
     for( j = file_start; j <= file_stop; j++ ) {
       pt_fname = mkfname( j );
       if( !( pt_bf = fopen( pt_fname, "r" ) ) ) {
	 perror( "fopen" );
	 exit( 1 );
       }
       
       fseek( pt_bf, i*bsize, SEEK_SET ); 
       fread( &buckets[m], rs, rb, pt_bf );
       fclose( pt_bf );
       free(pt_fname);
       m += rb;    
     } /* end of file loop */
     m = 0;

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
     sbuckets = psortui64(npes, id, bbits, bssize, &maxnum[i],	
			  num_recs, buckets, recvs, pt_bsort);

     /*
      * timing summary
      */
     timer_stop(&t);
     printf_single(id, "  total\b\b\b\b\b" );
     print_times_and_rate( id, &t, bsize * nf, "\bB" );
     log_times(pt_log, id, "TOTAL SORT", &t, bsize * nf);

     execute_postsort_kernels();

     /*
      * set number of elements to
      * new value from psortui64
      */
     num_recs = recvs[id];
     
     barrier_all();

     /*
      * sum up number of stripes this process will have
      * to skip by summing up the number of records from
      * all lower order pes
      */
     for( k = 0; k < id; k++ ) {
       shmem_get64( &skip, &num_recs, 1, k );
       sum += skip;
     }
     
     barrier_all();
     
     printf_single(id, " writing...\n" );
     
     timer_clear(&t);
     timer_start(&t);

     /*
      * write my records to specific
      * place in global sorted file
      */
     seekto = (uint64)( i*bsize*nf + sum*rs );
     fseek( pt_sf, seekto, SEEK_SET );
     fwrite( sbuckets, rs*num_recs, 1, pt_sf );
     
     /*
      * timing and logging summary
      */
     barrier_all();
     timer_stop(&t);
     print_times_and_rate( id, &t, bsize * nf, "\bB" );
     log_times(pt_log, id, "WRITE OUTPUT", &t, bsize * nf);
     
     /*
      * free memory for parallel sort and synchronize
      * all pes after each bucket is processed
      */
     shfree( sbuckets );

   } /* end bucket loop */

   maxn = 0;
   for( i = 0; i < nb; i++ )
     if( maxnum[i] >= maxn )
       maxn = maxnum[i];
   
   almem = maxn * sizeof( uint64 );

   fclose( pt_sf );

   free(recvs);
   free(maxnum);
   free(buckets);

   return;
}


void read_files_exclusive128( void )
{
   printf( "found rbyfls 128bit funct\n" );
   return;
}
