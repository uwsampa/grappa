#include "psort.h"
#include "hpc_output.h"
#include "timer.h"
#include "types.h"
#include "utils.h"
#include "sorts.h"

#include <upc.h> 
#include <upc_collective.h> 
#include <string.h>

typedef struct {
   int l;
   int h;
} sbucket;

#define MOD(x, y)            ((x) % (y))



/********1*****|***2*********3*********4*********5*********6*********7**!

               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************

description:   Carries out a parallel radix sort based on the following:
               A. Sohn and Y. Kodama. Load Balanced Parallel Radix Sort.
               In Proceedings of the 12th ACM International Conference
               on Supercomputing, Melbourne, Australia, July 14-17, 1998.

               The load balancing aspect of this paper where not
               implemented.

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

*********1*****|***2*********3*********4*********5*********6*********7**/
shared uint64 *psortui64( int npes, int id, int bbits, uint64 bssize,
			  int *gmax, int num, uint64 *data, 
			  shared uint64 *sh_recvs,
			  char *pt_bsort )
{
  /*
   * NOTE: THREADS and npes are interchangeable in this file.
   */
  
  int i, j;
  int pos;
  int npbits;
  int r;
  int rem;
  int cnt;
  int rshft;
  int dist;
  int source;
  int dest;
  int sum;
  int soff;
  int doff;
  
  int start_idx  = 0;
  int stop_idx   = 0;
  int lb_per_pes = 0;
  int t_per_pes  = 0;
  
  uint64 m;
  
  int *sends;
  
  shared int * map; 
  shared uint64 *sorted_data;
  static shared uint64 max_len;
  
  int * p_map;
  
  sbucket  *sbuckets;
  
  /*
   * Private alias to the sorted data with affinity to my PE.
   */
  uint64 * p_sorted_data;
  
  /*
   *  execution timing
   */
  timer t;

  int pe_bits    = ceil(log2(npes));
  int pow_of_two = npes && !(npes & (npes - 1));
  
  sbuckets = (sbucket *)malloc( npes * sizeof( sbucket ) );   
  if( !sbuckets ) {
    perror( "malloc" );
    exit( 1 );
  }
  
  sends    =     (int *)malloc( npes * sizeof( int ) );
  if( !sends ) {
    perror( "malloc" );
    exit( 1 );
  }
  
  /* 
   * npes * npes 2D array with PE affinity along the columns
   */
  map      = (shared int *)upc_all_alloc(npes, npes * sizeof(int));
  if( !map ) {
    perror( "upc_all_alloc" );
    exit(1);
  }
  
  /*
   * Create a local alias to my column in the map matrix.
   */
  p_map    = (int *)&map[id];
  
  printf_single(id, "   Bucketing...\n" );
  
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
    (void)usort( data, num, 0, (int64)(64-bbits-1) );
  
  /*
   * Find position of most significant bit in npes-1
   */
  pos = 8 * sizeof( int );
  m = 0x1U << ( pos - 1 );
  if( npes == 1 ) {
    npbits=0;
  }
  else {
    while( !(m & (npes-1)) ) {
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
  for( i = 0; i < npes; i++ ) {
    if( rem > 0 ) {
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
   * Calculate the number of keys this thread will need to send to each
   * of the other threads.  sends[i] = x means this thread will send
   * x keys to thread i.
   */ 
  j  = 0;
  for( i = 0; i < npes; i++ ) {
    int c      = 0;
    uint64 rsv = (data[j]<<bbits)>>rshft;
    
    /*
     * Count the number of elements in this bucket.
     */
    while( rsv >= sbuckets[i].l && rsv <= sbuckets[i].h && j < num ) {
      j++;
      c++;
      rsv = (data[j]<<bbits)>>rshft;
    }
    sends[i] = c;
    c = 0;
  }
  
  /*
   * Initialize the map in shared memory.  Because affinity is on the columns, each
   * row will cause communication between threads.
   */
  for( i = 0; i < npes; i++ ) 
    map[id * npes + i] = sends[i];
    
  upc_barrier;

  /*
   * The sum of each column in the map is the number of items that each thread
   * will have at the end of the sort.  map has column affinity, so we have 
   * each thread calculate its own value, iterating over its portion of the map.
   */   
  upc_forall( i = 0; i < npes; i++; i) {
    sum = 0;
    for( j = 0; j < npes; j++ )
      sum += p_map[j];
    sh_recvs[i] = sum;
  }
   
  /*
   * Find the largest value in the sh_recvs array.
   */
  upc_all_reduceUL(&max_len, sh_recvs, UPC_MAX, THREADS, 1, NULL, 
		   UPC_IN_ALLSYNC | UPC_OUT_ALLSYNC); 

  /*
   * Pass the maximum value back to the calling function.
   */
  *gmax = max_len;
 
  /*
   * Allocate memory for data exchanges between the processes and take a 
   * local alias to the data local to my PE for use in the sorting routine.
   */
  sorted_data = (shared uint64 *)upc_all_alloc(THREADS, max_len * sizeof(uint64));
  p_sorted_data = (uint64 *)sorted_data;

  upc_barrier;

  timer_stop(&t);
  print_times_and_rate( id, &t, bssize, "\bB" );
  log_times(pt_log, id, "BUCKETIZE", &t, bssize);
   
  printf_single(id, "   Key exchange...\n" );

  /*
   * Begin timing the parallel sort.
   */
  timer_clear(&t);
  timer_start(&t);

  /*
   * Each PE will put the keys its not responsible for into the shared memory 
   * of the PE that is responsible for it.  A prefix sum on the map column of
   * the destination thread up to the row of the sending thread id tells the 
   * sender where to place the data on the destination thread.
   */
  for( dist = 1; dist <= npes - 1; dist++ ) {
    /*
     * This will point to the contiguous chunk of the shared space that is 
     * resident on the destination (target) PE.  It's the portion of 
     * shared_data that's local to the target PE.  dest and target here mean
     * the same thing.
     */
    shared [] uint64 * target_sorted_data;

    dest = MOD(id + dist, npes);
     
    soff = 0;  
    for( i = 0; i < dest; i++ )
      soff += sends[i];
     
    /*
     * p_map wouldn't work here since we need the *target* threads map column.
     * TODO: create a partial prefix sum map that calculates these ahead of time.
     * Would minimize iteration over a remote shared location, which is
     * obviously slower.
     */
    doff = 0;
    for( i = 0; i < id; i++ ) {
      doff += map[i*npes + dest];
    }
     
    target_sorted_data = (shared [] uint64_t *)&sorted_data[dest]; 
    upc_memput( &target_sorted_data[doff], &data[soff], sends[dest] * sizeof( uint64 ) );
  }
   
  /*
   * Transfer my local data into the shared space (using local pointers).
   */
  soff = 0;
  for( i = 0; i < id; i++ )
    soff += sends[i];

  doff = 0;
  for( i = 0; i < id; i++ ) {
    doff += p_map[i];
  }

  memcpy( &p_sorted_data[doff], &data[soff], sends[id] * sizeof(uint64_t) ); 

  /*
   * Key exchange complete.
   */
  timer_stop(&t);
  print_times_and_rate( id, &t, bssize, "\bB" );
  log_times(pt_log, id, "KEY EXCHANGE", &t, bssize);
   
  upc_barrier;

  /*
   * Each PE now has npes sorted lists, so we do one final sort on each PE
   * to put each key in its final place.
   */
  printf_single(id, "   Final sort...\n" );

  timer_clear(&t);
  timer_start(&t);

  if( sh_recvs[id] != 0 ) {
    if( !strcmp( pt_bsort, "insert" ) )
      isortui64( sh_recvs[id], p_sorted_data );
    else if( !strcmp( pt_bsort, "select" ) )
      ssortui64( sh_recvs[id], p_sorted_data );
    else if( !strcmp( pt_bsort, "bubble" ) )
      bsortui64( sh_recvs[id], p_sorted_data );
    else if( !strcmp( pt_bsort, "qsort" ) )
      qsortui64( sh_recvs[id], p_sorted_data );
    else if( !strcmp( pt_bsort, "radix"  ) ) {
      if (pow_of_two)
	(void)usort( p_sorted_data, sh_recvs[id], 0, (int64)(64 - (bbits + pe_bits) - 1) );
      else
	(void)usort( p_sorted_data, sh_recvs[id], 0, (int64)(64 - (bbits) - 1) );
    }
  }
  
  timer_stop(&t);
  print_times_and_rate( id, &t, bssize, "\bB" );
  log_times(pt_log, id, "FINAL SORT", &t, bssize);

  upc_barrier;
  
  if(id == 0){
    upc_free( map );
  }

  free( sends );
  free( sbuckets );

  return(sorted_data);
}

