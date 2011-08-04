
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <mpi.h>
#include <hugetlbfs.h>

#include "debug.h"

#include "alloc.h"

#include "mpi_utils.h"
#include "ib_utils.h"
#include "options.h"

#include <infiniband/verbs.h>
#include <infiniband/arch.h>

int main( int argc, char * argv[] ) {

  ASSERT_NZ( sizeof( size_t ) == 8 && "is this not a 64-bit machine?" );
  
  int mpi_rank = 0;
  int mpi_size = 0;
  char mpi_node_name[MPI_MAX_PROCESSOR_NAME];
  softxmt_mpi_init( &argc, &argv, &mpi_rank, &mpi_size, mpi_node_name );

  struct options opt = parse_options( &argc, &argv );
  
  LOG_INFO( "%s/%d: %s starting on node %d of %d.\n", mpi_node_name, mpi_rank, argv[0], mpi_rank+1, mpi_size );

  //size_t total_alloc_size = ( 1ll << 33 ) + ( 1ll << 32 ); // todo: parameterize
  size_t total_alloc_size = ( 1ll << 32 ); // todo: parameterize
  size_t my_alloc_size = 0;
  void * my_block = alloc_my_block( mpi_rank, mpi_size, total_alloc_size, &my_alloc_size );

  LOG_INFO( "%s/%d: allocated %lu of %lu bytes.\n", mpi_node_name, mpi_rank, my_alloc_size, total_alloc_size );

  struct global_ib_context global_context;
  struct node_ib_context node_contexts[ mpi_size ];
  ib_global_init( mpi_rank, mpi_size, mpi_node_name, 
		  my_block, my_alloc_size,
		  &global_context );

  ib_nodes_init( mpi_rank, mpi_size, mpi_node_name, 
		 1,
		 opt.outstanding, opt.outstanding, 
		 opt.outstanding, opt.rdma_outstanding, 
		 1,
		 &global_context,
		 &node_contexts[0] );

  LOG_INFO( "%s/%d: initialized.\n", mpi_node_name, mpi_rank );


  // RDMA read and write
  if (opt.rdma_read || opt.rdma_write) {
    struct timespec start;
    struct timespec end;
    int64_t i;
    int j;


    ASSERT_NZ( 2 == mpi_size && "this test requires 2 MPI processes" );
    int sender = 0;
    int receiver = 1;
    int is_sender  = sender == mpi_rank;
    int is_receiver = receiver == mpi_rank;
  
    int outstanding = 0;
    int sent = 0;
    int completed = 0;

    //while( opt.count * opt.payload_size > my_alloc_size / 2 ) opt.count /= 2;
    size_t max_count = (my_alloc_size / 2) / opt.payload_size;
    if (opt.count > max_count) opt.count = max_count;
    size_t my_send_buffer_count = opt.count;

    char * my_send_buffers = my_block;
    char * my_recv_buffers = my_block + opt.count * opt.payload_size;
    uint64_t send_sum = 0;
    uint64_t recv_sum = 0;

    struct ibv_sge * sges = malloc( my_send_buffer_count * sizeof( struct ibv_sge ) );
    assert( NULL != sges && "couldn't allocate sges" );
    LOG_INFO( "%s/%d: need %lu bytes for %lu wrs.\n", mpi_node_name, mpi_rank, my_send_buffer_count * sizeof( struct ibv_send_wr ), my_send_buffer_count );
    struct ibv_send_wr * wrs = malloc( my_send_buffer_count * sizeof( struct ibv_send_wr ) );
    assert( NULL != wrs && "couldn't allocate wrs" );

    for( i = 0; i < my_send_buffer_count; ++i ) {
      send_sum += my_send_buffers[i] = i;
      my_recv_buffers[i] = 0;
    }

    if (is_sender) {
      #pragma omp parallel for 
      for( i = 0; i < my_send_buffer_count; ++i ) {
    	sges[i].addr = opt.rdma_write ? (uintptr_t) &my_send_buffers[i] : (uintptr_t) &my_recv_buffers[i];
    	sges[i].length = opt.payload_size;
    	sges[i].lkey = global_context.memory_region->lkey;
    	wrs[i].wr_id = i;
    	wrs[i].next = (((i + 1) % opt.batch_size) == 0) || ((i + 1) == my_send_buffer_count) ? NULL : &wrs[i+1];
    	wrs[i].sg_list = &sges[i];
    	wrs[i].num_sge = 1;
    	wrs[i].opcode = opt.rdma_write ? IBV_WR_RDMA_WRITE : IBV_WR_RDMA_READ;
    	wrs[i].send_flags = i + 1 == my_send_buffer_count ? IBV_SEND_SIGNALED | IBV_SEND_FENCE : 0;
    	wrs[i].imm_data = 1; //hton( i );
    	wrs[i].wr.rdma.remote_addr = opt.rdma_write ? (uintptr_t) &my_recv_buffers[i] : (uintptr_t) &my_send_buffers[i];
    	wrs[i].wr.rdma.rkey = node_contexts[ receiver ].remote_key;
      }

    }

    LOG_INFO( "%s/%d: ready.\n", mpi_node_name, mpi_rank );

    MPI_Barrier( MPI_COMM_WORLD );
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (is_sender) {
      
      #pragma omp parallel for num_threads(1)
      for( i = 0; i < my_send_buffer_count; i += opt.batch_size ) {
	struct ibv_send_wr  * bad_work_request = NULL;
	int result = ibv_post_send( node_contexts[ receiver ].queue_pair, &wrs[i], &bad_work_request );
	ASSERT_Z( result && "send failed");
	ASSERT_Z( bad_work_request );
      }

      LOG_INFO( "%s/%d: everything sent. waiting for completion.\n", mpi_node_name, mpi_rank );      

      if (1) {
	while ( completed < 1 ) {
	  struct ibv_wc work_completions[ opt.batch_size ];
	  int num_completion_entries = ibv_poll_cq( node_contexts[ receiver ].completion_queue, 
						    opt.batch_size, work_completions );
	  for( i = 0; i < num_completion_entries; ++i ) {
	    ++completed;
	    //--outstanding;
	    if (DEBUG)
	      printf("%s: %d: found status %s (%d) for wr_id %d sent %d completed %d outstanding %d\n",
		     mpi_node_name, i,
		     ibv_wc_status_str(work_completions[i].status),
		     work_completions[i].status, (int) work_completions[i].wr_id,
		     sent, completed, outstanding);
	    if ( work_completions[i].status != IBV_WC_SUCCESS ) {
	      fprintf(stderr, "Failed status %s (%d) for wr_id %d\n",
		      ibv_wc_status_str(work_completions[i].status),
		      work_completions[i].status, (int) work_completions[i].wr_id);
	      assert( 0 && "bad work completion status" );
	    }
	  }
	}
      }

    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    MPI_Barrier( MPI_COMM_WORLD );

    sent = opt.count;
    completed = opt.count;

    for( i = 0; i < my_send_buffer_count; ++i ) {
      recv_sum += my_recv_buffers[i];
    }

    LOG_INFO( "%s/%d: send_sum %lu recv_sum %lu done.\n", mpi_node_name, mpi_rank, send_sum, recv_sum );

    free( sges );
    free( wrs );

    uint64_t start_ns = ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);
    uint64_t end_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec);

    if (opt.rdma_read && is_sender) ASSERT_NZ( send_sum == recv_sum );
    if (opt.rdma_write && is_receiver) ASSERT_NZ( send_sum == recv_sum );
      

    if (is_sender) {
      //ASSERT_NZ( sent == opt.count );
      //ASSERT_NZ( completed == opt.count );
      double time_ns = end_ns - start_ns;
      double time = time_ns / 1.0e9;
      double rate = (double) completed / time;
      double bw = (double) completed * opt.payload_size / time;
      LOG_INFO( "%s/%d: sent %d messages of size %u bytes in %f seconds: %f Mmsg/s, %f MB/s.\n", 
		mpi_node_name, mpi_rank,
		opt.count, opt.payload_size, time, rate / 1.0e6, bw / 1.0e6);
      LOG_INFO( "data, "
		"cores, "
		"threads_per_core, "
		"payload_size_log, "
		"payload_size, "
		"count, "
		"outstanding, "
		"rdma_outstanding, "
		"batch_size, "
		"messages, "
		"rdma_read, "
		"rdma_write, "
		"fetch_and_add, "
		"time, Mmsg/s, MB/s\n");
      LOG_INFO( "data, "
		"%d, %d, %u, %u, %u, %d, %d, %d, %d, %d, %d, %d, %d, "
		"%f, %f, %f\n", 
		opt.cores,
		opt.threads_per_core,
		opt.payload_size_log,
		opt.payload_size,
		opt.count,
		opt.outstanding,
		opt.rdma_outstanding,
		opt.batch_size,
		opt.messages,
		opt.rdma_read,
		opt.rdma_write,
		opt.fetch_and_add,
		time, rate / 1.0e6, bw / 1.0e6);
    }
  }




  if (0) {
    
    struct timespec start;
    struct timespec end;
    int i;
    int j;


    ASSERT_NZ( 2 == mpi_size && "this test requires 2 MPI processes" );
    int sender = 0;
    int receiver = 1;
    int is_sender  = sender == mpi_rank;
    int is_receiver = receiver == mpi_rank;
    
    int outstanding = 0;
    int sent = 0;
    int completed = 0;

    if ( is_receiver ) {
      for( i = 0; i < opt.outstanding; ++i ) {
	ib_post_receive( &global_context, &node_contexts[ sender ], 
			 my_block, opt.payload_size, 
			 1307 );
      }
      outstanding = opt.outstanding;
      LOG_INFO( "%s/%d: posted initial receives.\n", mpi_node_name, mpi_rank );
    }


    MPI_Barrier( MPI_COMM_WORLD );
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    struct ibv_wc work_completions[ opt.batch_size ];
    int num_completion_entries = 0;

    if (is_sender) {
      while (completed < opt.count) {
	// post sends
	while (outstanding < opt.outstanding && sent < opt.count) {
	  for( i = 0; i < opt.batch_size && sent < opt.count; ++i ) {
	    if (DEBUG)
	      LOG_INFO( "%s/%d: posting send.\n", mpi_node_name, mpi_rank );
	    ib_post_send( &global_context, &node_contexts[ receiver ],
			  my_block, opt.payload_size, 
			  5397 );
	    sent += 1;
	    outstanding += 1;
	  }
	}
	
	// complete sends
	num_completion_entries = ibv_poll_cq( node_contexts[ receiver ].completion_queue, 
					      opt.batch_size, work_completions );
	for( i = 0; i < num_completion_entries; ++i ) {
	  ++completed;
	  --outstanding;
	  if (DEBUG)
	    printf("%s: %d: found status %s (%d) for wr_id %d sent %d completed %d outstanding %d\n",
		   mpi_node_name, i,
		   ibv_wc_status_str(work_completions[i].status),
		   work_completions[i].status, (int) work_completions[i].wr_id,
		   sent, completed, outstanding);
	  if ( work_completions[i].status != IBV_WC_SUCCESS ) {
	    fprintf(stderr, "Failed status %s (%d) for wr_id %d\n",
		    ibv_wc_status_str(work_completions[i].status),
		    work_completions[i].status, (int) work_completions[i].wr_id);
	    assert( 0 && "bad work completion status" );
	  }
	}
      }
    } else {
      while (completed < opt.count) {
	// complete receives
	num_completion_entries = ibv_poll_cq( node_contexts[ sender ].completion_queue, 
					      opt.batch_size, work_completions );
	for( i = 0; i < num_completion_entries; ++i ) {
	  ++completed;
	  --outstanding;
	  if (DEBUG) 
	    printf("%s: %d: found status %s (%d) for wr_id %d sent %d completed %d outstanding %d\n",
		   mpi_node_name, i,
		   ibv_wc_status_str(work_completions[i].status),
		   work_completions[i].status, (int) work_completions[i].wr_id,
		   sent, completed, outstanding);
	  if ( work_completions[i].status != IBV_WC_SUCCESS ) {
	    fprintf(stderr, "Failed status %s (%d) for wr_id %d\n",
		    ibv_wc_status_str(work_completions[i].status),
		    work_completions[i].status, (int) work_completions[i].wr_id);
	    assert( 0 && "bad work completion status" );
	  }
	}

	for( i = 0; i < opt.outstanding - outstanding; ++i ) {
	  ib_post_receive( &global_context, &node_contexts[ sender ], 
			   my_block, opt.payload_size, 
			   1307 );
	  ++outstanding;
	}
      }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    MPI_Barrier( MPI_COMM_WORLD );

    uint64_t start_ns = ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);
    uint64_t end_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec);

    if (is_sender) {
      ASSERT_NZ( sent == opt.count );
      double time_ns = end_ns - start_ns;
      double time = time_ns / 1.0e9;
      double rate = (double) opt.count / time;
      double bw = rate * opt.payload_size;
      LOG_INFO( "%s/%d: sent %d messages of size %d bytes in %f seconds: %f Mmsg/s, %f MB/s.\n", 
		mpi_node_name, mpi_rank,
		opt.count, opt.payload_size, time, rate / 1.0e6, bw / 1.0e6);
      LOG_INFO( "data, batch size, outstanding, message count, payload size, time, Mmsg/s, MB/s\n");
      LOG_INFO( "data, %d, %d, %d, %d, %f, %f, %f\n", 
		opt.batch_size, opt.outstanding, opt.count, opt.payload_size, time, rate / 1.0e6, bw / 1.0e6);
    }

  }

  if (0) {
    uint64_t * send_buffer_base = (uint64_t *) (my_block); // + sizeof( uint64_t ) * mpi_size );
    uint64_t * recv_buffer_base = (uint64_t *) (my_block + sizeof( uint64_t ) * mpi_size);
    uint64_t * rdma_buffer_base = (uint64_t *) (my_block + sizeof( uint64_t ) * mpi_size * 2);
    uint64_t * atomic_buffer_base = (uint64_t *) (my_block + sizeof( uint64_t ) * mpi_size * 3);
    uint64_t * fetch_buffer_base = (uint64_t *) (my_block + sizeof( uint64_t ) * mpi_size * 4);

  
    int i;
    for( i = 0; i < mpi_size; ++i ) {
      //ib_post_receive( &global_context, &node_contexts[ i ], recv_buffer_base + i, sizeof(uint64_t), 123 + i );
    }
  
    LOG_INFO( "%s/%d: receives posted.\n", mpi_node_name, mpi_rank );

    MPI_Barrier( MPI_COMM_WORLD );

    *send_buffer_base = mpi_rank + 1000;

    /* *(fetch_buffer_base + mpi_rank) = 0; */
    /* *(atomic_buffer_base + mpi_rank) = mpi_rank + 2000; */
    *(fetch_buffer_base + 0) = 0;
    *(fetch_buffer_base + 1) = 0;
    *(atomic_buffer_base + 0) = 0;
    *(atomic_buffer_base + 1) = 0;

    for( i = 0; i < mpi_size; ++i ) {
      if (0) ib_post_send( &global_context, &node_contexts[ i ], send_buffer_base, sizeof(uint64_t), 456 + i);
      if (1) ib_post_read( &global_context, &node_contexts[ i ], rdma_buffer_base + i, 
			   node_contexts[i].remote_address,  // send buffer address
			   //send_buffer_base,
			   sizeof(uint64_t), 789 + i );
      if (1) ib_post_write( &global_context, &node_contexts[ i ], send_buffer_base,
      			    //node_contexts[i].remote_address,
      			    recv_buffer_base + mpi_rank,
      			    sizeof(uint64_t), 890 + i );
      if (1) ib_post_fetch_and_add( &global_context, &node_contexts[ i ], fetch_buffer_base + i,
				    //node_contexts[i].remote_address,
				    atomic_buffer_base + i, 
				    sizeof(uint64_t), 1, 901 + i );
    }


    LOG_INFO( "%s/%d: sends posted.\n", mpi_node_name, mpi_rank );

    for( i = 0; i < mpi_size; ++i ) {
      ib_complete( mpi_rank, mpi_size, mpi_node_name, 
		   &global_context, &node_contexts[ i ], 3 );
    }

    for( i = 0; i < mpi_size; ++i ) {
      LOG_INFO( "%s/%d: got %lu from %d (rdma %lu, atomic %lu %lu).\n", mpi_node_name, mpi_rank, *(recv_buffer_base + i), i, *(rdma_buffer_base + i), *(fetch_buffer_base + i), *(atomic_buffer_base + i) );
    }
  }


  LOG_INFO( "%s/%d: completed. Going to finalize!\n", mpi_node_name, mpi_rank );

  ib_nodes_finalize( mpi_rank, mpi_size, mpi_node_name, 
		     &global_context,
		     node_contexts );

  ib_global_finalize( mpi_rank, mpi_size, mpi_node_name, 
		      &global_context );

  free_my_block( my_block );

  LOG_INFO( "%s/%d: done.\n", mpi_node_name, mpi_rank );
  return 0;
}
