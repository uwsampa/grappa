
#include "ib_utils.h"

#include "debug.h"

#include <mpi.h>

#include <infiniband/verbs.h>
#include <infiniband/arch.h>

void ib_global_init( int mpi_rank, int mpi_size, char mpi_node_name[], 
		     void * my_block, size_t my_block_size,
		     struct global_ib_context * context ) {

  // detect infiniband devices
  int num_devices = 0;
  struct ibv_device ** infiniband_devices;
  ASSERT_NZ( infiniband_devices = ibv_get_device_list( &num_devices ) );
  if ( 1 < num_devices ) {
    LOG_WARN( "%s/%d: %d infiniband devices detected; using first one\n", mpi_node_name, mpi_rank, num_devices );
  }

  // open device
  ASSERT_NZ( context->device = ibv_open_device( infiniband_devices[0] ) );

  // create protection domain
  ASSERT_NZ( context->protection_domain = ibv_alloc_pd( context->device ) );

  // register memory region
  ASSERT_NZ( context->memory_region = ibv_reg_mr( context->protection_domain, 
						  my_block, my_block_size, 
						  ( IBV_ACCESS_LOCAL_WRITE  | 
						    IBV_ACCESS_REMOTE_WRITE | 
						    IBV_ACCESS_REMOTE_READ  |
						    IBV_ACCESS_REMOTE_ATOMIC) ) );

  ibv_free_device_list( infiniband_devices );
}

void ib_nodes_init( int mpi_rank, int mpi_size, char mpi_node_name[], 
		    int physical_port,
		    int send_message_depth, int receive_message_depth, 
		    int send_rdma_depth, int receive_rdma_depth, 
		    int scatter_gather_element_count,
		    struct global_ib_context * global_context,
		    struct node_ib_context node_contexts[] ) {
  MPI_Barrier( MPI_COMM_WORLD );

  int i;
  for( i = 0; i < mpi_size; ++i ) {
    // create per-node completion queue
    ASSERT_NZ( node_contexts[i].completion_queue = ibv_create_cq( global_context->device, 
								  receive_message_depth+1, 
								  NULL,  // no user context
								  NULL,  // no completion channel 
								  0 ) ); // no completion channel vector

    // create send/receive queue
    struct ibv_qp_init_attr init_attributes = {
      .send_cq = node_contexts[i].completion_queue,
      .recv_cq = node_contexts[i].completion_queue,
      .cap = {
	.max_send_wr = send_message_depth,
	.max_recv_wr = receive_message_depth,
	.max_send_sge = scatter_gather_element_count,
	.max_recv_sge = scatter_gather_element_count,
      },
      .qp_type = IBV_QPT_RC, // use "reliable connections"
    };
    ASSERT_NZ( node_contexts[i].queue_pair = ibv_create_qp( global_context->protection_domain, &init_attributes ) );
  }

  //
  // exchange queue pair info through MPI
  //
  
  // get my LID 
  struct ibv_port_attr port_attributes;
  ASSERT_Z( ibv_query_port( global_context->device, physical_port, &port_attributes ) );
  
  // collect LIDs of all nodes so we can open connections
  uint16_t * lids = malloc( mpi_size * sizeof(uint16_t) );
  ASSERT_Z( MPI_Allgather(  &port_attributes.lid, 1, MPI_SHORT,
			    lids, 1, MPI_SHORT,
			    MPI_COMM_WORLD ) );

  // collect QP numbers from all nodes so we can open connections
  uint32_t * my_qp_nums = malloc( mpi_size * sizeof(uint32_t) );
  for( i = 0; i < mpi_size; ++i ) {
    my_qp_nums[i] = node_contexts[i].queue_pair->qp_num;
  }

  uint32_t * all_qp_nums = malloc( mpi_size * mpi_size * sizeof(uint32_t) );
  ASSERT_Z( MPI_Allgather(  my_qp_nums, mpi_size, MPI_UNSIGNED,
			    all_qp_nums, mpi_size, MPI_UNSIGNED,
			    MPI_COMM_WORLD ) );

  // collect remote keys
  uint32_t * rkeys = malloc( mpi_size * sizeof(uint32_t) );
  ASSERT_Z( MPI_Allgather(  &global_context->memory_region->rkey, 1, MPI_UNSIGNED,
			    rkeys, 1, MPI_UNSIGNED,
			    MPI_COMM_WORLD ) );
  
  // collect remote addresses
  uintptr_t * raddrs = malloc( mpi_size * sizeof(uint64_t) );
  ASSERT_Z( MPI_Allgather(  &global_context->memory_region->addr, 1, MPI_UNSIGNED_LONG,
			    raddrs, 1, MPI_UNSIGNED_LONG,
			    MPI_COMM_WORLD ) );
  
  if (0) {
    for( i = 0; i < mpi_size; ++i ) {
      LOG_INFO( "%s/%d knows about node %d's lid %.4x rkey %x raddr %lx\n", 
		mpi_node_name, mpi_rank, i, lids[i], rkeys[i], raddrs[i] );
      int j;
      for( j = 0; j < mpi_size; ++j ) {
	LOG_INFO( "%s/%d knows about node %d's node %d qp_num %.4x\n", 
		  mpi_node_name, mpi_rank, i, j, all_qp_nums[i * mpi_size + j] );
      }
    }
  }
  
  for( i = 0; i < mpi_size; ++i ) {

    node_contexts[i].remote_address = (void *) raddrs[i];
    node_contexts[i].remote_key = rkeys[i];

    // setting up a queue pair requires a number of state transitions. 
    // a queue pair starts in RESET, and must move through INIT, RTR (ready-to-receive), and RTS (ready-to-send)
    // receive work requests must be posted to the queue before we move to RTR and RTS, or else messages may be dropped.

    // move send/receive queue to INIT
    enum ibv_mtu mtu = IBV_MTU_1024;
    struct ibv_qp_attr attributes = {
      .qp_state = IBV_QPS_INIT,
      .qp_access_flags = (IBV_ACCESS_LOCAL_WRITE | 
			  IBV_ACCESS_REMOTE_WRITE | 
			  IBV_ACCESS_REMOTE_READ  |
			  IBV_ACCESS_REMOTE_ATOMIC),
      .pkey_index = 0,  // todo: ???

      // some network properties
      .port_num = physical_port,
      .path_mtu = mtu,

      // outstanding RDMA request limits (still subject to message limits)
      .max_rd_atomic = send_rdma_depth,
      .max_dest_rd_atomic = receive_rdma_depth,

      // timers and timeouts
      .min_rnr_timer = 12, // RNR NAK timer
      .timeout = 14, 
      .rnr_retry = 7,
      .retry_cnt = 7,

      .sq_psn = 123, // local packet sequence number
      .rq_psn = 123, // remote packet sequence number

      .dest_qp_num = all_qp_nums[ i * mpi_size + mpi_rank ],
      .ah_attr = {
	.is_global = 0,
	.dlid = lids[ i ],
	.sl = 0, // "default" service level 
	.src_path_bits = 0,
	.port_num = physical_port,
      }
    };
    ASSERT_Z( ibv_modify_qp( node_contexts[i].queue_pair, &attributes,
			     IBV_QP_STATE | 
			     IBV_QP_PKEY_INDEX | 
			     IBV_QP_PORT | 
			     IBV_QP_ACCESS_FLAGS ) );

    attributes.qp_state = IBV_QPS_RTR;
    ASSERT_Z( ibv_modify_qp( node_contexts[i].queue_pair, &attributes, 
			     IBV_QP_STATE | 
			     IBV_QP_AV |
			     IBV_QP_PATH_MTU | 
			     IBV_QP_DEST_QPN | 
			     IBV_QP_RQ_PSN | 
			     IBV_QP_MAX_DEST_RD_ATOMIC | 
			     IBV_QP_MIN_RNR_TIMER ) );

    attributes.qp_state = IBV_QPS_RTS;
    ASSERT_Z( ibv_modify_qp( node_contexts[i].queue_pair, &attributes, 
			     IBV_QP_STATE | 
			     IBV_QP_TIMEOUT |
			     IBV_QP_RETRY_CNT | 
			     IBV_QP_RNR_RETRY | 
			     IBV_QP_SQ_PSN | 
			     IBV_QP_MAX_QP_RD_ATOMIC ) );
  }

  free( lids );
  free( my_qp_nums );
  free( all_qp_nums );

  MPI_Barrier( MPI_COMM_WORLD );
}


void ib_nodes_finalize( int mpi_rank, int mpi_size, char mpi_node_name[], 
			struct global_ib_context * context,
			struct node_ib_context node_contexts[] ) {
  MPI_Barrier( MPI_COMM_WORLD );

  int i;
  for( i = 0; i < mpi_size; ++i ) {
    ASSERT_Z( ibv_destroy_qp( node_contexts[i].queue_pair ) );
    ASSERT_Z( ibv_destroy_cq( node_contexts[i].completion_queue ) );
  }

  MPI_Barrier( MPI_COMM_WORLD );
}

void ib_global_finalize( int mpi_rank, int mpi_size, char mpi_node_name[], 
			 struct global_ib_context * context ) {
  ASSERT_Z( ibv_dereg_mr( context->memory_region ) );
  ASSERT_Z( ibv_dealloc_pd( context->protection_domain ) );
  ASSERT_Z( ibv_close_device( context->device ) );
}

void ib_post_send( struct global_ib_context * global_context,
		   struct node_ib_context * node_context,
		   void * pointer, size_t size, uint64_t id ) {
  struct ibv_sge list = {
    .addr = (uintptr_t) pointer,
    .length = size,
    .lkey = global_context->memory_region->lkey,
  };

  struct ibv_send_wr work_request = {
    .wr_id = id,
    .next = NULL,
    .sg_list = &list,
    .num_sge = 1,
    .opcode = IBV_WR_SEND,
    .send_flags = IBV_SEND_SIGNALED,
  };

  struct ibv_send_wr  * bad_work_request = NULL;

  ASSERT_Z( ibv_post_send( node_context->queue_pair, &work_request, &bad_work_request ) );
  ASSERT_Z( bad_work_request );
}

void ib_post_receive( struct global_ib_context * global_context,
		      struct node_ib_context * node_context,
		      void * pointer, size_t size, uint64_t id ) {
  struct ibv_sge list = {
    .addr = (uintptr_t) pointer,
    .length = size,
    .lkey = global_context->memory_region->lkey,
  };

  struct ibv_recv_wr work_request = {
    .wr_id = id,
    .next = NULL,
    .sg_list = &list,
    .num_sge = 1,
  };

  struct ibv_recv_wr  * bad_work_request = NULL;

  ASSERT_Z( ibv_post_recv( node_context->queue_pair, &work_request, &bad_work_request ) );
  ASSERT_Z( bad_work_request );
}

void ib_post_bare_receive( struct global_ib_context * global_context,
			   struct node_ib_context * node_context,
			   uint64_t id ) {
  struct ibv_recv_wr work_request = {
    .wr_id = id,
    .next = NULL,
    .sg_list = NULL,
    .num_sge = 0,
  };

  struct ibv_recv_wr  * bad_work_request = NULL;

  ASSERT_Z( ibv_post_recv( node_context->queue_pair, &work_request, &bad_work_request ) );
  ASSERT_Z( bad_work_request );
}

void ib_complete( int mpi_rank, int mpi_size, char mpi_node_name[], 
		  struct global_ib_context * global_context,
		  struct node_ib_context * node_context,
		  int count ) {
  struct ibv_wc work_completions[2];
  int num_entries = 0;
  int i = 0;
  int tripcount = 0;
  
  while ( 0 < count ) {
    do {
      if ( !(tripcount & 0xfffffff) ) {
	LOG_INFO( "%s/%d: polling completion queue tripcount=%d count=%d\n",mpi_node_name, mpi_rank, tripcount, count );
      }
      ASSERT_NZ( 0 <= ( num_entries = ibv_poll_cq( node_context->completion_queue, 2, work_completions ) ) );
      ++tripcount;
    } while (num_entries < 1);
    for( i = 0; i < num_entries; ++i ) {
      LOG_INFO( "%s/%d: entry %d: found status %s (%d) for wr_id %d receive_count %d send_count %d iterations %d\n",
		mpi_node_name, mpi_rank, i,
		ibv_wc_status_str(work_completions[i].status),
		work_completions[i].status, (int) work_completions[i].wr_id,
		0, 0, 0);
      if ( work_completions[i].status != IBV_WC_SUCCESS ) {
	LOG_ERROR( "Failed status %s (%d) for wr_id %d\n",
		   ibv_wc_status_str(work_completions[i].status),
		   work_completions[i].status, (int) work_completions[i].wr_id );
	ASSERT_Z( "bad work completion status" );
      }
      --count;
    }
  }
}

void ib_post_write( struct global_ib_context * global_context,
		    struct node_ib_context * node_context,
		    void * pointer, void * remote_pointer, size_t size, uint64_t id ) {
  struct ibv_sge list = {
    .addr = (uintptr_t) pointer,
    .length = size,
    .lkey = global_context->memory_region->lkey,
  };

  struct ibv_send_wr work_request = {
    .wr_id = id,
    .next = NULL,
    .opcode = IBV_WR_RDMA_WRITE,
    .sg_list = &list,
    .num_sge = 1,
    .send_flags = IBV_SEND_SIGNALED,
    .wr.rdma.remote_addr = (intptr_t) remote_pointer,
    .wr.rdma.rkey = node_context->remote_key,
  };

  struct ibv_send_wr  * bad_work_request = NULL;

  ASSERT_Z( ibv_post_send( node_context->queue_pair, &work_request, &bad_work_request ) );
  ASSERT_Z( bad_work_request );
}

void ib_post_read( struct global_ib_context * global_context,
		   struct node_ib_context * node_context,
		   void * pointer, void * remote_pointer, size_t size, uint64_t id ) {
  struct ibv_sge list = {
    .addr = (uintptr_t) pointer,
    .length = size,
    .lkey = global_context->memory_region->lkey,
  };

  struct ibv_send_wr work_request = {
    .wr_id = id,
    .next = NULL,
    .opcode = IBV_WR_RDMA_READ,
    .sg_list = &list,
    .num_sge = 1,
    .send_flags = IBV_SEND_SIGNALED,
    .wr.rdma.remote_addr = (intptr_t) remote_pointer,
    .wr.rdma.rkey = node_context->remote_key,
  };

  struct ibv_send_wr  * bad_work_request = NULL;

  ASSERT_Z( ibv_post_send( node_context->queue_pair, &work_request, &bad_work_request ) );
  ASSERT_Z( bad_work_request );
}

void ib_post_fetch_and_add( struct global_ib_context * global_context,
			    struct node_ib_context * node_context,
			    void * pointer, void * remote_pointer, size_t size, uintptr_t add, uint64_t id ) {
  struct ibv_sge list = {
    .addr = (uintptr_t) pointer,
    .length = size,
    .lkey = global_context->memory_region->lkey,
  };

  struct ibv_send_wr work_request = {
    .wr_id = id,
    .opcode = IBV_WR_ATOMIC_FETCH_AND_ADD,
    //.opcode = IBV_WR_ATOMIC_CMP_AND_SWP,
    .sg_list = &list,
    .num_sge = 1,
    .send_flags = IBV_SEND_SIGNALED,
    .wr.atomic.remote_addr = (intptr_t) remote_pointer,
    .wr.atomic.compare_add = 123, //add,
    .wr.atomic.swap = 12345, //add,
    .wr.atomic.rkey = node_context->remote_key,
  };

  struct ibv_send_wr  * bad_work_request = NULL;

  ASSERT_Z( ibv_post_send( node_context->queue_pair, &work_request, &bad_work_request ) );
  ASSERT_Z( bad_work_request );
}
