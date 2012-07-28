#include "psort.h"
#include "hpc_output.h"
#include "timer.h"
#include "types.h"
#include "utils.h"
#include "sorts.h"

#include <mpi.h>
#include <assert.h>

#if defined( MPI2 )

typedef struct {
   int l;
   int h;
} sbucket;

#define MOD(x, y)            ((x) % (y))


#define barrier_all()  MPI_Barrier(MPI_COMM_WORLD)

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

 invoked by:   routine            description
               --------------     --------------------------------------
               rstrps64         - void (ioops.c)
                                  Reads in the 64bit bucket files.
                                  For a given bucket number, each
                                  process reads in a range of records
                                  across all files, where the range of
                                  records was calculated in the work()
                                  function.  A parallel sort is carried
                                  out and then the data is written to
                                  the output file.

  variables:   varible            description
               --------------     --------------------------------------
               bssize           - uint64
                                  The total size in bytes of a bucket
                                  stripe.

               i, j             - int
                                  Loop variable for looping for number
                                  of processes.

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

               m                - uint64
                                  Mask bit used to find position in
                                  npes-1 number of the highest sig
                                  digit.

               rsv              - unit64
                                  Right bit shifted number for proper
                                  bucket assignment.

               sbuckets         - sbucket *
                                  This structure holds the lower and
                                  upper bucket ranges.
                                  
       lmod:   07/12/11

*********1*****|***2*********3*********4*********5*********6*********7**/
uint64 * psortui64( int npes, int id, int bbits, uint64 bssize,
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

  /* 
   * npes length array of integers.  sends[i] = j indicates that this PE needs
   * to send j elements to PE i.
   */   
  int *sends;

  /*
   * map is an npes x npes 2D matrix where row i is the sends array from PE i.
   * This map should be identical on every PE.  The sum of column j gives the 
   * number of elements that PE j will receive.
   */
  int **map; 
 
  MPI_Win sends_win;
  MPI_Win sorted_data_win;

  uint64 m, rsv;
  uint64 *sorted_data;

  sbucket  *sbuckets;

  /*
   *  execution timing
   */
  timer t;

  int pe_bits    = ceil(log2(npes));
  int pow_of_two = npes && !(npes & (npes - 1));

  /*
   * allocate data structures for sort and make sends array available
   * for MPI-2 RMA.
   */
  sbuckets = (sbucket *)malloc( npes * sizeof( sbucket ) );
  sends    =     (int *)malloc( npes * sizeof( int ) );

  MPI_Win_create(sends, npes * sizeof( int ), sizeof( int ), MPI_INFO_NULL, 
		 MPI_COMM_WORLD, &sends_win);


  map      =    (int **)malloc( npes * sizeof( int * ) );
  for( i = 0; i < npes; i++ )
    map[i] = (int *)malloc( npes * sizeof( int ) );

  printf_single(id, "    Bucketing data...\n" );
   
  /*
   * seral sort local array 
   */
  timer_clear(&t);
  timer_start(&t);

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

      MPI_Win_fence(0, sends_win);
      MPI_Get(map[source], npes, MPI_INT, source, 0, npes, MPI_INT, sends_win);
      MPI_Win_fence(0, sends_win);
    }
  barrier_all();

  /*
   * place my sends into my map array
   */
  for( i = 0; i < npes; i++ )
    map[id][i] = sends[i];

  /*
   * sum up number of elements that each
   * processor will receive into its sorted_data array
   */ 
  for( i = 0; i < npes; i++ ) {
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

  sorted_data = (uint64 *)malloc( recvs[id] * sizeof( uint64 ) );

  MPI_Win_create(sorted_data, recvs[id] * sizeof(uint64), sizeof(uint64),
		 MPI_INFO_NULL, MPI_COMM_WORLD, &sorted_data_win);

  barrier_all();

  /*
   * each process must push its array data to other processes
   * sorted_data array at the appropriate offsets
   */
  for( dist = 1; dist <= npes - 1; dist++ ) {
      dest = MOD(id + dist, npes);

      soff = 0;
      for( i = 0; i < dest; i++ )
	soff += sends[i];

      doff = 0;
      for( i = 0; i < id; i++ )
	doff += map[i][dest];

      MPI_Win_fence(0, sorted_data_win); 
      MPI_Put(&data[soff], sends[dest], MPI_LONG, dest,
	      doff, sends[dest], MPI_LONG, sorted_data_win);
      MPI_Win_fence(0, sorted_data_win);
    }

  barrier_all();

  /*
   * put my data into sorted_data
   */
  soff = 0;
  for( i = 0; i < id; i++ )
    soff += sends[i];

  doff = 0;
  for( i = 0; i < id; i++ )
    doff += map[i][id];

  memcpy( &sorted_data[doff], &data[soff], sends[id] * sizeof(uint64_t) ); 

  //for( i = 0; i < sends[id]; i++ )
  //  sorted_data[doff+i] = data[soff+i]; 

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
	isortui64( recvs[id], sorted_data );
      else if( !strcmp( pt_bsort, "select" ) )
	ssortui64( recvs[id], sorted_data );
      else if( !strcmp( pt_bsort, "bubble" ) )
	bsortui64( recvs[id], sorted_data );
      else if( !strcmp( pt_bsort, "qsort" ) )
	qsortui64( recvs[id], sorted_data );
      else if( !strcmp( pt_bsort, "radix"  ) ) {
	// Unlike the first serial sort, we know that the most significant 
	// bbits + pe_bits bits of the key are identical on this PE if there are
	// a power of two number of PEs.  We restrict usort to sort over this 
	// range of bits.
	if (pow_of_two)
	  (void)usort( sorted_data, recvs[id], 0, (int64)(64 - (bbits + pe_bits) - 1) );
	else
	  (void)usort( sorted_data, recvs[id], 0, (int64)(64 - (bbits) - 1) );
      }
    }
  timer_stop(&t);
  print_times_and_rate( id, &t, bssize, "\bB" );
  log_times(pt_log, id, "FINAL SORT", &t, bssize);

  barrier_all();

  for( i = 0; i < npes; i++) 
    free(map[i]);
  free( map );
  MPI_Win_free(&sorted_data_win);
  MPI_Win_free(&sends_win);
  free( sends );
  free( sbuckets );

  return( sorted_data );
}

/*
 * Bucket the data by performing a parallel quicksort on it.
 */
void bucketize_data( uint64_t n, uint64_t *a ) {
  qsortui64( n, a );
}

/*
 * Locate the boundary between a given bucket in the data array.  Should only 
 * be called by find_displacement_boundary.  This function is tail recursive.
 * Recursion stops when the length of the array can fit in a single 4K page.
 */
uint64_t binary_boundary_search( uint64_t *data, int bucket, 
				 uint64_t low, uint64_t hi, int shift_amount, 
				 int bbits ) {
  uint64_t middle   = low + ((hi - low) / 2);
  uint64_t b_left   = (data[middle-1] <<bbits) >> shift_amount;
  uint64_t b_right  = (data[middle+1] <<bbits) >> shift_amount;
  uint64_t b_middle = (data[middle]   <<bbits) >> shift_amount;
  
  if( (hi - low) < 512 ) {
    uint64_t i;
    for( i = low; i < hi; i++ ) {
      uint64_t b = (data[i] << bbits) >> shift_amount;
      if( b == bucket )
	return i;
    }
  }

  if( b_left != bucket && b_middle == bucket )
    return middle;
  else if( b_middle != bucket && b_right == bucket )
    return middle + 1;

  /*
   * All buckets are the same, so we either recurse right or we recurse left.
   */
  else {
    if( b_right < bucket ) {
      return binary_boundary_search( data, bucket, middle+1, hi, shift_amount, bbits );
    }
    else {
      return binary_boundary_search( data, bucket, low, middle-1, shift_amount, bbits );
    }
  }
}

/*
 * In the array data, this function will calculate the index where the given
 * bucket starts.  This is a wrapper function for the recursive 
 * binary_boundary_search call.
 */ 
uint64_t find_bucket_boundary( uint64_t *data, uint64_t len,  
			       int bucket, int npes, int bbits ) {
  if(bucket == 0)
    return 0;

  uint32_t shift_amount = 64 - ceil(log2(npes));

  return binary_boundary_search( data, bucket, 0, len-1, shift_amount, bbits );
}

uint64_t *hybrid_parallel_quicksort(int npes, int id, int bbits,
				    int len, uint64_t *data_out, uint64_t *recvs,
				    uint64_t stripe_size) {
  int bucket;
  
  uint64_t i = 0;
  
  MPI_Status status;

  uint64_t count_incoming = 0;

  int *send_counts;
  int *send_displacements;
  int *recv_counts;
  int *recv_displacements;

  /*
   * in and out here refer to the global communication step, NOT the function.
   * data_out is the data this process will eventually be sending out to other
   * PEs (but which comes into the function.)  data_in is the buffer to hold 
   * the sorted data as it comes in from other PEs.  We return this buffer.
   */
  static uint64_t *data_in;  

  timer t_bucketize;
  timer t_communicate;
  timer t_quicksort;

  timer_clear(&t_bucketize);
  timer_clear(&t_communicate);
  timer_clear(&t_quicksort);

  send_counts        = calloc(npes, sizeof(int));
  send_displacements = calloc(npes, sizeof(int));
  recv_counts        = calloc(npes, sizeof(int));
  recv_displacements = calloc(npes, sizeof(int));

  printf_single(id, "  Bucketing data...\n");

  timer_start(&t_bucketize);

  bucketize_data(len, data_out);
  
  /*
   * Calculate the starting offset of the data I'll send to each PE.
   * send_displacement[i] = x indicates that the data for PE i starts at
   * data_out[x].  This is parallelizable, but probably only beneficial if 
   * the number of nodes is huge. Each binary search is O(lg len), but the
   * memory access pattern doesn't make efficient use of the cache.
   */
  send_displacements[0] = 0;
#pragma omp parallel for
  for(bucket = 1; bucket < npes; bucket++) 
  {
    send_displacements[bucket] = find_bucket_boundary( data_out, len, 
						       bucket, npes, bbits);
    assert(send_displacements[bucket] >= 0);
  }

  send_counts[npes - 1] = len - send_displacements[npes - 1];
  for(bucket = 0; bucket < npes - 1; bucket++) 
  {
    send_counts[bucket] = send_displacements[bucket + 1] - send_displacements[bucket];
  }
    
  /*
   * Send out the information on who is getting how many keys from each PE.
   */
  MPI_Alltoall(send_counts, 1, MPI_INT, 
	       recv_counts, 1, MPI_INT, MPI_COMM_WORLD);

  /*
   * Calculate the displacements to put the recieved data based on the number 
   * of elements coming from each PE and calculate the memory this PE will need
   * to allocate.
   */  
  recv_displacements[0] = 0;
  for(i = 1; i < npes; i++) {
    recv_displacements[i] = recv_counts[i - 1] + recv_displacements[i - 1];
    assert(recv_displacements[i] >= 0);
  }

  count_incoming = recv_counts[0];
  for(i = 1; i < npes; i++) {
    count_incoming += recv_counts[i];
  }
  
  /*
   * Allocate array to place the incoming sorted buckets from each PE.
   */
  data_in = realloc(data_in, count_incoming * sizeof(uint64_t));
  if (!data_in) {
    perror("malloc failure");
  }

  timer_stop(&t_bucketize);
  print_times_and_rate(id, &t_bucketize, stripe_size, "\bB");
  log_times(pt_log, id, "BUCKETING", &t_bucketize, stripe_size);

  /*
   * The above malloc call will conflate with the key measurement time below
   * if there isn't a barrier here to make sure everyone finishes before process
   * 0 starts it's timer.
   */
  MPI_Barrier(MPI_COMM_WORLD);

  printf_single(id, "  Key exchange...\n");
  timer_start(&t_communicate);
   
  MPI_Alltoallv(data_out, send_counts, send_displacements, MPI_UNSIGNED_LONG,
		data_in,  recv_counts, recv_displacements, MPI_UNSIGNED_LONG,
		MPI_COMM_WORLD); 

  timer_stop(&t_communicate);
  print_times_and_rate(id, &t_communicate, 
		       2 * len * (npes - 1) * sizeof(uint64_t), "\bB (bandwidth)");
  log_times(pt_log, id, "KEY EXCHANGE", &t_bucketize, stripe_size);


  /* 
   * Each PE now has npes sorted lists that we merge with a final sort.
   */
  printf_single(id, "  Local quicksort...\n");
  timer_start(&t_quicksort);

  if(count_incoming != 0) {
    qsortui64(count_incoming, data_in);
  }

  timer_stop(&t_quicksort);
  print_times_and_rate(id, &t_quicksort, stripe_size, "\bB");
  log_times(pt_log, id, "FINAL SORT", &t_bucketize, stripe_size);
  
  free(send_counts);
  free(send_displacements);
  free(recv_counts);
  free(recv_displacements);

  *recvs = count_incoming;

  return data_in;
}
  


#endif 
