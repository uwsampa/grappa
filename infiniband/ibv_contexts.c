
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <mpi.h>
//#include <hugetlbfs.h>

#include "debug.h"

#include "alloc.h"

#include "mpi_utils.h"
#include "ib_utils.h"
#include "options.h"
#include "send_batch.h"

#include <infiniband/verbs.h>
#include <infiniband/arch.h>


int mpi_rank = 0;
int mpi_size = 0;
char mpi_node_name[MPI_MAX_PROCESSOR_NAME];

struct ibv_device ** get_devices( int * num_devices ) {
  struct ibv_device ** devices;
  ASSERT_NZ( devices = ibv_get_device_list( num_devices ) );
  if ( 1 < *num_devices ) {
    LOG_WARN( "%s/%d: %d infiniband devices detected; using first one\n", mpi_node_name, mpi_rank, *num_devices );
  }
  return devices;
}

void free_devices( struct ibv_device ** devices ) {
  ibv_free_device_list( devices );
}

struct ibv_context * get_context( struct ibv_device * device ) {
  struct ibv_context * context;
  ASSERT_NZ( context = ibv_open_device( device ) );
  return context;
}

void free_context( struct ibv_context * context ) {
  ASSERT_Z( ibv_close_device( context ) );
}

int get_port_count( struct ibv_context * context ) {
  struct ibv_device_attr device_attrs;
  ASSERT_Z( ibv_query_device( context, &device_attrs ) );
  return device_attrs.phys_port_cnt;
}

struct ibv_pd * get_protection_domain( struct ibv_context * context ) {
  struct ibv_pd * pd;
  ASSERT_NZ( pd = ibv_alloc_pd( context ) );
  return pd;
}

void free_protection_domain( struct ibv_pd * pd ) {
  ASSERT_Z( ibv_dealloc_pd( pd ) );
}

struct ibv_mr * get_memory_region( struct ibv_pd * pd, void * block, size_t size ) {
  struct ibv_mr * mr;
  ASSERT_NZ( mr = ibv_reg_mr( pd, block, size, 
			      ( IBV_ACCESS_LOCAL_WRITE  | 
				IBV_ACCESS_REMOTE_WRITE | 
				IBV_ACCESS_REMOTE_READ  |
				IBV_ACCESS_REMOTE_ATOMIC) ) );
  return mr;
}

void free_memory_region( struct ibv_mr * mr ) {
  ASSERT_Z( ibv_dereg_mr( mr ) );
}


struct ibv_cq * get_completion_queue( struct ibv_context * context, int depth ) {
  struct ibv_cq * cq;
  ASSERT_NZ( cq = ibv_create_cq( context, 
				 depth, 
				 NULL,  // no user context
				 NULL,  // no completion channel 
				 0 ) ); // no completion channel vector
  return cq;
}

void free_completion_queue( struct ibv_cq * cq ) {
  ASSERT_Z( ibv_destroy_cq( cq ) );
}

struct ibv_qp * get_queue_pair( struct ibv_pd * protection_domain, struct ibv_qp_init_attr * init_attributes ) {
  struct ibv_qp * qp;
  ASSERT_NZ( qp = ibv_create_qp( protection_domain, init_attributes ) );
  return qp;
}

void free_queue_pair( struct ibv_qp * qp ) {
  ASSERT_Z( ibv_destroy_qp( qp ) );
}

void activate_queue_pair( struct ibv_qp * queue_pair, struct ibv_qp_attr * attributes ) {
  attributes->qp_state = IBV_QPS_INIT;
  ASSERT_Z( ibv_modify_qp( queue_pair, attributes,
			   IBV_QP_STATE | 
			   IBV_QP_PKEY_INDEX | 
			   IBV_QP_PORT | 
			   IBV_QP_ACCESS_FLAGS ) );
  
  attributes->qp_state = IBV_QPS_RTR;
  ASSERT_Z( ibv_modify_qp( queue_pair, attributes, 
			   IBV_QP_STATE | 
			   IBV_QP_AV |
			   IBV_QP_PATH_MTU | 
			   IBV_QP_DEST_QPN | 
			   IBV_QP_RQ_PSN | 
			   IBV_QP_MAX_DEST_RD_ATOMIC | 
			   IBV_QP_MIN_RNR_TIMER ) );
			   //			   IBV_QP_MAX_DEST_RD_ATOMIC | 
			   //IBV_QP_MIN_RNR_TIMER ) );
  //0 ) );
  
  attributes->qp_state = IBV_QPS_RTS;
  ASSERT_Z( ibv_modify_qp( queue_pair, attributes, 
			   IBV_QP_STATE | 
			   IBV_QP_TIMEOUT |
			   IBV_QP_RETRY_CNT | 
			   IBV_QP_RNR_RETRY | 
			   //IBV_QP_TIMEOUT |
			   //IBV_QP_RETRY_CNT | 
			   //IBV_QP_RNR_RETRY | 
			   IBV_QP_SQ_PSN | 
			   IBV_QP_MAX_QP_RD_ATOMIC ) );
//0 ) );
}



int main( int argc, char * argv[] ) {
  int i;
  struct timespec start;
  struct timespec end;
  uint64_t start_ns;
  uint64_t end_ns;
  double runtime;
  double rate;
  double bandwidth;

  ASSERT_NZ( sizeof( size_t ) == 8 && "is this not a 64-bit machine?" );
  
  softxmt_mpi_init( &argc, &argv, &mpi_rank, &mpi_size, mpi_node_name );
  ASSERT_NZ( mpi_size == 2 && "Two nodes are required." );

  struct options opt = parse_options( &argc, &argv );
  
  LOG_INFO( "%s/%d: %s starting on node %d of %d.\n", mpi_node_name, mpi_rank, argv[0], mpi_rank+1, mpi_size );

  // detect infiniband devices
  int num_devices = 0;
  struct ibv_device ** infiniband_devices = get_devices( &num_devices );

  struct ibv_context * context = get_context( infiniband_devices[0] );
  struct ibv_context * context2 = get_context( infiniband_devices[0] );

  int num_ports = get_port_count( context );
  LOG_INFO( "%s/%d: found %d ports\n", mpi_node_name, mpi_rank, num_ports );
  int physical_port = 1;

  struct ibv_pd * protection_domain = get_protection_domain( context );
  struct ibv_pd * protection_domain2 = get_protection_domain( context2 );

  //void * foo = malloc( 1024 );
//  void * foo = get_hugepage_region( opt.payload_size * 2, GHR_DEFAULT );
  void * foo = malloc( opt.payload_size * 2 );
  struct ibv_mr * memory_region = get_memory_region( protection_domain, foo, opt.payload_size * 2 );
  struct ibv_mr * memory_region2 = get_memory_region( protection_domain2, foo, opt.payload_size * 2 );

  int depth = opt.outstanding * 2; //opt.count; //128;
  struct ibv_cq * completion_queue = get_completion_queue( context, depth+1 );
  struct ibv_cq * completion_queue2 = get_completion_queue( context2, depth+1 );

  int scatter_gather_element_count = 1;
  struct ibv_qp_init_attr init_attributes = {
    .send_cq = completion_queue,
    .recv_cq = completion_queue,
    .cap = {
      .max_send_wr = opt.outstanding,
      .max_recv_wr = opt.outstanding,
      .max_send_sge = scatter_gather_element_count,
      .max_recv_sge = scatter_gather_element_count,
    },
    .qp_type = IBV_QPT_RC, // use "reliable connections"
    //.qp_type = IBV_QPT_UC,
    .sq_sig_all = 0, // only issue send completions if requested
  };
  struct ibv_qp * queue_pair = get_queue_pair( protection_domain, &init_attributes );

  init_attributes.send_cq = completion_queue2;
  init_attributes.recv_cq = completion_queue2;
  struct ibv_qp * queue_pair2 = get_queue_pair( protection_domain2, &init_attributes );

  // get my LID 
  struct ibv_port_attr port_attributes;
  ASSERT_Z( ibv_query_port( context, physical_port, &port_attributes ) );
  uint16_t my_lid = port_attributes.lid;
  uint16_t lids[2];
  ASSERT_Z( MPI_Allgather(  &my_lid, 1, MPI_SHORT,
			    lids, 1, MPI_SHORT,
			    MPI_COMM_WORLD ) );
  uint16_t remote_lid = lids[ 1 - mpi_rank ];

  // get my qp_num
  uint32_t my_qp_num = queue_pair->qp_num;
  uint32_t qp_nums[2];
  ASSERT_Z( MPI_Allgather(  &my_qp_num, 1, MPI_UNSIGNED,
			    qp_nums, 1, MPI_UNSIGNED,
			    MPI_COMM_WORLD ) );
  uint32_t remote_qp_num = qp_nums[ 1 - mpi_rank ];

  // get my qp_num
  uint32_t my_qp_num2 = queue_pair2->qp_num;
  uint32_t qp_nums2[2];
  ASSERT_Z( MPI_Allgather(  &my_qp_num2, 1, MPI_UNSIGNED,
			    qp_nums2, 1, MPI_UNSIGNED,
			    MPI_COMM_WORLD ) );
  uint32_t remote_qp_num2 = qp_nums2[ 1 - mpi_rank ];

  // get my key
  uint32_t my_rkey = memory_region->rkey;
  uint32_t rkeys[2];
  ASSERT_Z( MPI_Allgather(  &my_rkey, 1, MPI_UNSIGNED,
			    rkeys, 1, MPI_UNSIGNED,
			    MPI_COMM_WORLD ) );
  uint32_t remote_rkey = rkeys[ 1 - mpi_rank ];

  // get my key
  uint32_t my_rkey2 = memory_region2->rkey;
  uint32_t rkeys2[2];
  ASSERT_Z( MPI_Allgather(  &my_rkey2, 1, MPI_UNSIGNED,
			    rkeys2, 1, MPI_UNSIGNED,
			    MPI_COMM_WORLD ) );
  uint32_t remote_rkey2 = rkeys2[ 1 - mpi_rank ];

  // get remote address
  uintptr_t my_raddr = (uintptr_t) memory_region->addr;
  uintptr_t raddrs[2];
  ASSERT_Z( MPI_Allgather(  &my_raddr, 1, MPI_UNSIGNED_LONG,
			    raddrs, 1, MPI_UNSIGNED_LONG,
			    MPI_COMM_WORLD ) );
  uintptr_t remote_raddr = raddrs[ 1 - mpi_rank ];
  LOG_INFO( "%s/%d: my address %p, remote address %p\n", mpi_node_name, mpi_rank, (void*) my_raddr, (void*) remote_raddr );
  //ASSERT_NZ( my_raddr == remote_raddr && "addresses must match" );
  
  uint32_t my_physical_port = physical_port;
  uint32_t physical_ports[2];
  ASSERT_Z( MPI_Allgather(  &my_physical_port, 1, MPI_UNSIGNED,
			    physical_ports, 1, MPI_UNSIGNED,
			    MPI_COMM_WORLD ) );
  int remote_physical_port = physical_ports[ 1 - mpi_rank ];

  srand( (unsigned) time( NULL ) );  
  uint32_t my_sequence_number = rand() % 0xfff;
  uint32_t sequence_numbers[2];
  ASSERT_Z( MPI_Allgather(  &my_sequence_number, 1, MPI_UNSIGNED,
			    sequence_numbers, 1, MPI_UNSIGNED,
			    MPI_COMM_WORLD ) );
  int remote_sequence_number = sequence_numbers[ 1 - mpi_rank ];

  int my_rdma_depth = 1;
  int remote_rdma_depth = 1;

  enum ibv_mtu mtu = IBV_MTU_4096;
  struct ibv_qp_attr attributes = {
    .qp_access_flags = (IBV_ACCESS_LOCAL_WRITE | 
			IBV_ACCESS_REMOTE_WRITE | 
			IBV_ACCESS_REMOTE_READ |
			IBV_ACCESS_REMOTE_ATOMIC),
    .pkey_index = 0,  // todo: ???
    
    // some network properties
    .port_num = my_physical_port,
    .path_mtu = mtu,
    
    // outstanding RDMA request limits (still subject to message limits)
    .max_rd_atomic = my_rdma_depth,
    .max_dest_rd_atomic = remote_rdma_depth,
    
    // timers and timeouts
    .min_rnr_timer = 12, // RNR NAK timer
    .timeout = 14, 
    .rnr_retry = 7,
    .retry_cnt = 7,
    
    .sq_psn = my_sequence_number, // local packet sequence number
    .rq_psn = remote_sequence_number, // remote packet sequence number
    
    .dest_qp_num = remote_qp_num,
    .ah_attr = {
      .is_global = 0,
      .dlid = remote_lid,
      .sl = 0, // "default" service level 
      .src_path_bits = 0,
      .port_num = remote_physical_port,
    }
  };
  activate_queue_pair( queue_pair, &attributes );

  attributes.dest_qp_num = remote_qp_num2;
  activate_queue_pair( queue_pair2, &attributes );


  struct batched_sender * bs1 = construct_batched_sender( mpi_node_name, mpi_rank, &opt, memory_region, queue_pair, completion_queue );
  batched_recv_arm( bs1 );
							 
  MPI_Barrier( MPI_COMM_WORLD );

  struct batched_sender * bs2 = construct_batched_sender( mpi_node_name, mpi_rank, &opt, memory_region2, queue_pair2, completion_queue2 );
  batched_recv_arm( bs2 );
							 
  MPI_Barrier( MPI_COMM_WORLD );

  clock_gettime(CLOCK_MONOTONIC, &start);
  if( mpi_rank == 0 ) { // sender
    for( i = 0; i < opt.count; ) {
      int this_iter = batched_send( bs1, memory_region->addr, opt.payload_size );
      //this_iter += batched_send( bs2, memory_region2->addr, opt.payload_size );
      i += this_iter;
    }
    batched_flush( bs1 );
    batched_flush( bs2 );
  } else {
    int received = 0;
    while( received < opt.count ) {
      int this_iter = batched_send_poll( bs1 );
      this_iter += batched_send_poll( bs2 );
      received += this_iter;
      //if (this_iter) LOG_INFO(__FILE__ ":%d: received %d.\n", __LINE__-2, received );
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  MPI_Barrier( MPI_COMM_WORLD );

  //if (mpi_rank == 0) {
    start_ns = ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);
    end_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec);
    runtime = ( (double) end_ns - (double) start_ns ) / 1000000000.0;
    rate = (double) opt.count / runtime;
    bandwidth = (double) opt.payload_size * rate;
    LOG_INFO( "%s/%d: %d messages in %f seconds = %f messages per second = %f bytes per second\n", mpi_node_name, mpi_rank, opt.count, runtime, rate, bandwidth );
    //  }

  teardown_batched_sender( bs1 );
  teardown_batched_sender( bs2 );

  /*
  int pairs		= mpi_size / 2;
  int is_sender		= mpi_rank < pairs;
  int sender		= is_sender ? mpi_rank : mpi_rank - pairs;
  int is_receiver	= mpi_rank >= pairs;
  int receiver		= is_receiver ? mpi_rank : mpi_rank + pairs;


  uint64_t * send_word = (uint64_t *) memory_region->addr;
  uint64_t * recv_word = (uint64_t *) memory_region->addr + 1;
  //LOG_INFO( "%s/%d: my send address %p, my receive address %p\n", mpi_node_name, mpi_rank, send_word, recv_word );

  MPI_Barrier( MPI_COMM_WORLD );

  if (0) {
    *send_word = is_sender ? 1234 : 12345;
    *recv_word = 0;
    //LOG_INFO( "%s/%d: my send val %d, my receive val %d\n", mpi_node_name, mpi_rank, *send_word, *recv_word );

    MPI_Barrier( MPI_COMM_WORLD );

    struct ibv_sge send_sge = {
      .addr = (uintptr_t) recv_word,
      .length = 8,
      .lkey = memory_region->lkey,
    };
    

    struct ibv_send_wr send_wr[opt.count];

    //for( i = 0; i < opt.count; ++i ) {
    for( i = 0; i < opt.batch_size; ++i ) {
      send_wr[i].wr_id = i;
      send_wr[i].next = (i + 1) % opt.batch_size == 0 ? NULL : &send_wr[ (i + 1) % opt.batch_size ];
      send_wr[i].sg_list = &send_sge;
      send_wr[i].num_sge = 1;
      send_wr[i].opcode = IBV_WR_RDMA_READ;
      send_wr[i].send_flags = 0;
      send_wr[i].imm_data = 0;
      send_wr[i].wr.rdma.remote_addr = (uintptr_t) remote_raddr;
      send_wr[i].wr.rdma.rkey = remote_rkey;
    }

    if (is_sender) {
      clock_gettime(CLOCK_MONOTONIC, &start);
      for( i = 0; i < opt.count; i += opt.batch_size ) {
	struct ibv_send_wr * bad_send_wr = NULL;
	ASSERT_Z( ibv_post_send( queue_pair, &send_wr[i], &bad_send_wr ) );
	ASSERT_Z( bad_send_wr );
      }
      clock_gettime(CLOCK_MONOTONIC, &end);
      sleep(1);
    }

    MPI_Barrier( MPI_COMM_WORLD );
    //LOG_INFO( "%s/%d: my send val %d, my receive val %d\n", mpi_node_name, mpi_rank, *send_word, *recv_word );
    if (is_sender) {
      ASSERT_NZ( *recv_word == 12345 );
      start_ns = ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);
      end_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec);
      runtime = ( (double) end_ns - (double) start_ns ) / 1000000000.0;
      rate = (double) opt.count / runtime;
      LOG_INFO( "%s/%d: RDMA_READ %d messages in %f seconds = %f messages per second\n", mpi_node_name, mpi_rank, opt.count, runtime, rate );
    }
  }

    

  MPI_Barrier( MPI_COMM_WORLD );

  if (0) {
    *send_word = 23456;

    struct ibv_sge send_sge = {
      .addr = (uintptr_t) send_word,
      .length = 8,
      .lkey = memory_region->lkey,
    };

    struct ibv_send_wr send_wr[ opt.count ];
    for( i = 0; i < opt.count; ++i ) {
      send_wr[i].wr_id = 1;
      send_wr[i].next = (i + 1) % opt.batch_size == 0 ? NULL : &send_wr[ (i + 1) % opt.count ];
      send_wr[i].sg_list = &send_sge;
      send_wr[i].num_sge = 1;
      send_wr[i].opcode = IBV_WR_RDMA_WRITE;
      send_wr[i].send_flags = IBV_SEND_INLINE;
      send_wr[i].imm_data = 0;
      send_wr[i].wr.rdma.remote_addr = (uintptr_t) remote_raddr + 8;
      send_wr[i].wr.rdma.rkey = remote_rkey;
    }

    if (is_sender) {
      clock_gettime(CLOCK_MONOTONIC, &start);
      for( i = 0; i < opt.count; i += opt.batch_size ) {
	struct ibv_send_wr * bad_send_wr = NULL;
	ASSERT_Z( ibv_post_send( queue_pair, &send_wr[i], &bad_send_wr ) );
	ASSERT_Z( bad_send_wr );
      }
      clock_gettime(CLOCK_MONOTONIC, &end);
      sleep(1);
    } 

    MPI_Barrier( MPI_COMM_WORLD );

    if (is_receiver) {
      ASSERT_NZ( *recv_word == 23456 );
    }

    if (is_sender) {
      start_ns = ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);
      end_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec);
      runtime = ( (double) end_ns - (double) start_ns ) / 1000000000.0;
      rate = (double) opt.count / runtime;
      LOG_INFO( "%s/%d: RDMA_WRITE: %d messages in %f seconds = %f messages per second\n", mpi_node_name, mpi_rank, opt.count, runtime, rate );
    }
  }


  MPI_Barrier( MPI_COMM_WORLD );

  {
    *send_word = 98765;

    struct ibv_sge recv_sge = {
      .addr = (uintptr_t) recv_word,
      .length = 8,
      .lkey = memory_region->lkey,
    };

    struct ibv_recv_wr recv_wr[ opt.count ];
    for( i = 0; i < opt.count; ++i ) {
      recv_wr[i].wr_id = 2;
      recv_wr[i].next = (i + 1) % opt.batch_size == 0 ? NULL : &recv_wr[ (i + 1) % opt.count ];
      recv_wr[i].sg_list = &recv_sge;
      recv_wr[i].num_sge = 1;
    }
    if (is_receiver) {
      for( i = 0; i < opt.count; i += opt.batch_size ) {
	struct ibv_recv_wr  * bad_recv_wr = NULL;
	ASSERT_Z( ibv_post_recv( queue_pair, &recv_wr[i], &bad_recv_wr ) );
	ASSERT_Z( bad_recv_wr );
      }
    }

    MPI_Barrier( MPI_COMM_WORLD );

    struct ibv_sge send_sge = {
      .addr = (uintptr_t) send_word,
      .length = 8,
      .lkey = memory_region->lkey,
    };

    struct ibv_send_wr send_wr[ opt.count ];
    for( i = 0; i < opt.count; ++i ) {
      send_wr[i].wr_id = 1;
      send_wr[i].next = (i + 1) % opt.batch_size == 0 ? NULL : &send_wr[ (i + 1) % opt.count ];
      send_wr[i].sg_list = &send_sge;
      send_wr[i].num_sge = 1;
      send_wr[i].opcode = IBV_WR_SEND;
      send_wr[i].send_flags = IBV_SEND_INLINE;
      send_wr[i].imm_data = 0;
    }
    if (is_sender) {
      clock_gettime(CLOCK_MONOTONIC, &start);
      for( i = 0; i < opt.count; i += opt.batch_size ) {
	struct ibv_send_wr * bad_send_wr = NULL;
	ASSERT_Z( ibv_post_send( queue_pair, &send_wr[i], &bad_send_wr ) );
	ASSERT_Z( bad_send_wr );
      }
      clock_gettime(CLOCK_MONOTONIC, &end);
    }
    
    if (is_receiver) {
      clock_gettime(CLOCK_MONOTONIC, &start);
      int received = 0;
      while ( received < opt.count ) {
	struct ibv_wc work_completions[opt.batch_size];
	int num_completion_entries = ibv_poll_cq( completion_queue, 
						  opt.batch_size, 
						  &work_completions[0] );
	for( i = 0; i < num_completion_entries; ++i ) {
	  ASSERT_NZ( work_completions[i].status == IBV_WC_SUCCESS );
	  ++received;
	}
      }
      clock_gettime(CLOCK_MONOTONIC, &end);
    }

    //LOG_INFO( "%s/%d: sent %d, received %d\n", mpi_node_name, mpi_rank, *send_word, *recv_word );
    if (is_receiver) {
      ASSERT_NZ( *recv_word == 98765 );
    }
  
    start_ns = ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);
    end_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec);
    runtime = ( (double) end_ns - (double) start_ns ) / 1000000000.0;
    rate = (double) opt.count / runtime;
    LOG_INFO( "%s/%d: SEND_RECV %d messages in %f seconds = %f messages per second\n", mpi_node_name, mpi_rank, opt.count, runtime, rate );
  }

  */

  MPI_Barrier( MPI_COMM_WORLD );

  free_queue_pair( queue_pair );
  free_queue_pair( queue_pair2 );

  free_completion_queue( completion_queue );
  free_completion_queue( completion_queue2 );

  free_memory_region( memory_region );
  free_memory_region( memory_region2 );

  free_protection_domain( protection_domain );
  free_protection_domain( protection_domain2 );

  free_context( context );
  free_context( context2 );

  free_devices( infiniband_devices );


  LOG_INFO( "%s/%d: done.\n", mpi_node_name, mpi_rank );
  return 0;
}
