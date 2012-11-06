#include "psort.h"
#include "hpc_output.h"
#include "shmem_utils.h"
#include "timer.h"
#include "types.h"
#include "utils.h"
#include "sorts.h"

/********************************************************************************
               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************
********************************************************************************/


#include <mpp/shmem.h> 
#include <string.h>

typedef struct {
   int l;
   int h;
} sbucket;

#define MOD(x, y)            ((x) % (y))


#define barrier_all()  shmem_barrier_all()


/********1*****|***2*********3*********4*********5*********6*********7**!

description:   Carries out a parallel radix sort based on the following:
               A. Sohn and Y. Kodama. Load Balanced Parallel Radix Sort.
               In Proceedings of the 12th ACM International Conference
               on Supercomputing, Melbourne, Australia, July 14-17, 1998.

               The load balancing aspect of this paper was not
               implemented.

 invoked by:   routine            description
               --------------     --------------------------------------
               rstrps64         - void (ioops.src)
                                  Reads in the 64bit bucket files.
                                  For a given bucket number, each
                                  process reads in a range of records
                                  across all files, where the range of
                                  records was calculated in the work()
                                  function.  A parallel sort is carried
                                  out and then the data is written to
                                  the output file.
    invokes:   routine            description
               ---------------    --------------------------------------
               ssortui32        - void (sorts.src)
                                  Selection sort is an in-place
                                  comparison sort.

 
  variables:   varible            description
               --------------     --------------------------------------
               bssize           - uint64
                                  The total size in bytes of a bucket
                                  stripe.

	       pos              - int
                                  Position counter to find most
                                  significant digit in npes-1.

               npbits           - int
                                  Most significant digit position
                                  in npes-1. 

               r                - int
                                  High end of the bucket range.

               rem              - int
                                  Remaining bucket range to be
                                  distributed across pes.

               cnt              - int
                                  Calculate minimum range for each
                                  pe/bucket.

               rshft            - int
                                  Number of bits each data element
                                  will have to be shifted to the right.

               c                - int
                                  Count of numbers that belong in
                                  specific bucket.

               ao               - int
                                  Global index into data array.

               dist             - int
                                  Process distance counter iused when
                                  getting or putting data from other
                                  pes.

               source           - int
                                  Process id from which data will sent.

               dest             - int
                                  Process id to which data will sent.

               sum              - int
                                  Running sum of number of elements
                                  that each processor will receive into
                                  its sdata array.

               maxnum           - int
                                  Largest number in the recvs array.

               soff             - int
                                  Index offset in send array.

               doff             - int
                                  Index offset in dest/map array.

               start_idx        - int
                                  Start record number in a bucket.

               stop_idx         - int
                                  Ending record number in a bucket.

               lb_per_pes       - int
                                  Minimum range of values for each
                                  bucket.

               t_per_pes        - int
                                  Load balanced range for each bucket. 

               sends            - int *
                                  Each process calculates how many in
                                  its data array have to be sent to
                                  each of the other processes.

                                  for PE j:
                                           0  (num to 0)
                                           1  (num to 1)
                                  sends =  2  (num to 2)
                                           3  (num to 3)
                                           .
                                           .
                                           j  (num to j(self))
                                           .
                                           .

               map              - int **
                                  npes * npes array each process allocates
                                  that stores what data elements all
                                  processes receive from all other
                                  processes.

               m                - uint64
                                  Mask bit used to find position in
                                  npes-1 number of the highest sig
                                  digit.

               rsv              - unit64
                                  Right bit shifted number for proper
                                  bucket assignment.

               sdata            - unit64 *
                                  Holds sorted data.

               sbuckets         - sbucket *
                                  This structure holds the lower and
                                  upper bucket ranges.
                                  
       lmod:   10/28/11

*********1*****|***2*********3*********4*********5*********6*********7**/
uint64 *psortui64( int npes, int id, int bbits, uint64 bssize, 
		   int *gmax, int num, uint64 *data, int *recvs,
		   char *pt_bsort )
{
   int i, j;
   int pos;
   int npbits;
   int r;
   int rem;
   int cnt;
   int rshft;
   int c;
   int ao;
   int dist;
   int source;
   int dest;
   int sum;
   int maxnum;
   int soff;
   int doff;

   int start_idx  = 0;
   int stop_idx   = 0;
   int lb_per_pes = 0;
   int t_per_pes  = 0;

   int *sends;
   int **map; 

   uint64 m, rsv;
   uint64 *sdata;

   sbucket  *sbuckets;

   /*
    *  execution timing
    */
   timer t;

   int pe_bits    = ceil(log2(npes));
   int pow_of_two = npes && !(npes & (npes - 1));
   
   /*
    * allocate data structures for sort
    */
   sbuckets = (sbucket *)malloc(   npes * sizeof( sbucket ) );
   sends    = (  int   *)shmalloc( npes * sizeof( int )     );

   map = (int **)malloc( npes * sizeof( int * ) );
   for( i = 0; i < npes; i++ )
      map[i] = (int *)malloc( npes * sizeof( int ) );

   printf_single(id, "    Bucketing data...\n" );
   
   timer_clear(&t);
   timer_start(&t);

   /*
    * seral sort local array 
    */
   if( !strcmp( pt_bsort, "insert" ) )
     isortui64( num, data );
   else if( !strcmp( pt_bsort, "select" ) )
     ssortui64( num, data );
   else if( !strcmp( pt_bsort, "bubble" ) )
     bsortui64( num, data );
   else if( !strcmp( pt_bsort, "qsort" ) )
     qsortui64( num, data );
   else if( !strcmp( pt_bsort, "radix"  ) )
     (void)usort( data, num, 0, (int64)(64-(bbits)-1) );
   
   timer_stop(&t);
   print_times_and_rate( id, &t, bssize, "\bB" );
   log_times(pt_log, id, "BUCKETING", &t, bssize);
   
   printf_single(id, "    Key exchange...\n" );
   
   /*
    * Begin timing the parallel sort.
    */
   timer_clear(&t);
   timer_start(&t);
  
   /*
    * find position of most significant bit in npes-1
    */
   pos = 8 * sizeof( int );
   m = 0x1U << ( pos - 1 );
   if( npes == 1 )
      npbits=0;
   else
   {
      while( !(m & (npes-1)) )
      {
         pos--;
         m >>= 1;
      }
      npbits=pos;
   }

   /*
    * calculate high end bucket ranges
    */
   if( npes == 1 )
      r=1;
   else
      r = (int)pow(2,npbits);

   /*
    * calculate minimum range and remainders for load balance
    * routine
    */
   rem = r % npes;
   cnt = r / npes;

   /*
    * bucket load balance algorithm
    */ 
   lb_per_pes = cnt + 1;
   for( i = 0; i < npes; i++ )
   {
      if( rem > 0 )
      {
         t_per_pes = lb_per_pes;
         rem--;
      }
      else
         t_per_pes = cnt;

      stop_idx = start_idx + t_per_pes - 1;

      sbuckets[i].l = start_idx;
      sbuckets[i].h = stop_idx;

      start_idx  += t_per_pes;
   }

   /*
    * calculate the number of bits each data element
    * will have to be shifted to the right
    */
   rshft = 8 * sizeof( uint64 ) - npbits;

   /*
    * determine which data elements fit into which buckets
    */ 
   c   = 0;
   ao  = 0;
   rsv = 0;
   for( i = 0; i < npes; i++ )
   {
      rsv = (data[ao]<<bbits)>>rshft;
      while( rsv >= sbuckets[i].l && rsv <= sbuckets[i].h && ao < num )
      {
         ao++;
         c++;
         rsv = (data[ao]<<bbits)>>rshft;
      }
      sends[i] = c;
      c = 0;
   }

   barrier_all();

   /*
    * each process must grab(get) other processors sends in order to
    * populate the local npes*npes map
    */
   for( dist = 1; dist <= npes - 1; dist++ )
   {
      source = MOD(npes + id - dist, npes);

      shmem_int_get( recvs, sends, npes, source );
      for( i = 0; i < npes; i++ ) {
	map[source][i] = recvs[i];
      }
   }

   barrier_all();

   /*
    * place my sends into my map array
    */
   for( i = 0; i < npes; i++ )
      map[id][i] = sends[i];

   /*
    * sum up number of elements that each
    * processor will receive into its sdata array
    */ 
   for( i = 0; i < npes; i++ )
   {
      sum = 0;
      for( j = 0; j < npes; j++ )
         sum += map[j][i];
      recvs[i] = sum;
   }
 
   /*
    * figure the largest number in the recvs array
    */
   maxnum = 0;
   for( i = 0; i < npes; i++ )
      if( recvs[i] >= maxnum )
         maxnum = recvs[i];

   *gmax = maxnum;

   /*
    * allocate symetric memory for data exchanges between
    * the processes
    */
   sdata = (uint64 *)shmalloc( maxnum * sizeof( uint64 ) );

   /*
    * check that sdata was allocated properly
    */
   //check_shptrs( id, npes, sdata, "sdata" );
   

   /*
    * each process must push its array data to other processes
    * sdata array at the appropriate offsets
    */
   for( dist = 1; dist <= npes - 1; dist++ )
   {
      dest = MOD(id + dist, npes);

      soff = 0;
      for( i = 0; i < dest; i++ )
         soff += sends[i];

      doff = 0;
      for( i = 0; i < id; i++ )
         doff += map[i][dest];

      shmem_put64( &sdata[doff], &data[soff], sends[dest], dest );
   }

   barrier_all();

   /*
    * put my data into sdata
    */
   soff = 0;
   for( i = 0; i < id; i++ )
      soff += sends[i];

   doff = 0;
   for( i = 0; i < id; i++ )
      doff += map[i][id];

   memcpy( &sdata[doff], &data[soff], sends[id] * sizeof(uint64_t) ); 

   /*
    * Parallel sort complete
    */
   timer_stop(&t);

   print_times_and_rate( id, &t, bssize, "\bB" );
   log_times(pt_log, id, "KEY EXCHANGE", &t, bssize);

   /*
    * seral sort local array; skip if 0 elements 
    */
   printf_single(id, "    Final sort...\n" );

   timer_clear(&t);
   timer_start(&t);

   if( recvs[id] != 0 )
   {
      if( !strcmp( pt_bsort, "insert" ) )
         isortui64( recvs[id], sdata );
      else if( !strcmp( pt_bsort, "select" ) )
         ssortui64( recvs[id], sdata );
      else if( !strcmp( pt_bsort, "bubble" ) )
         bsortui64( recvs[id], sdata );
      else if( !strcmp( pt_bsort, "qsort" ) )
         qsortui64( recvs[id], sdata );
      else if( !strcmp( pt_bsort, "radix"  ) ) {
	// Unlike the first serial sort, we know that the most significant 
	// bbits + pe_bits bits of the key are identical on this PE if there are
	// a power of two number of PEs.  We restrict usort to sort over this 
	// range of bits.
	if (pow_of_two)
	  (void)usort( sdata, recvs[id], 0, (int64)(64 - (bbits + pe_bits) - 1) );
	else
	  (void)usort( sdata, recvs[id], 0, (int64)(64 - (bbits) - 1) );
      }
   }

   timer_stop(&t);
   print_times_and_rate( id, &t, bssize, "\bB" );
   log_times(pt_log, id, "FINAL SORT", &t, bssize);

   barrier_all();

   for( i = 0; i < npes; i++) 
     free(map[i]);
   free( map );
   shfree( sends );
   free( sbuckets );

   return( sdata );
}
 
