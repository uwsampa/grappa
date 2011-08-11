
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
  
  attributes->qp_state = IBV_QPS_RTS;
  ASSERT_Z( ibv_modify_qp( queue_pair, attributes, 
			   IBV_QP_STATE | 
			   IBV_QP_TIMEOUT |
			   IBV_QP_RETRY_CNT | 
			   IBV_QP_RNR_RETRY | 
			   IBV_QP_SQ_PSN | 
			   IBV_QP_MAX_QP_RD_ATOMIC ) );
}



int main( int argc, char * argv[] ) {
  int i;
  
  ASSERT_NZ( sizeof( size_t ) == 8 && "is this not a 64-bit machine?" );
  
  softxmt_mpi_init( &argc, &argv, &mpi_rank, &mpi_size, mpi_node_name );

  struct options opt = parse_options( &argc, &argv );
  
  LOG_INFO( "%s/%d: %s starting on node %d of %d.\n", mpi_node_name, mpi_rank, argv[0], mpi_rank+1, mpi_size );

  // detect infiniband devices
  int num_devices = 0;
  struct ibv_device ** infiniband_devices = get_devices( &num_devices );

  struct ibv_context * context = get_context( infiniband_devices[0] );

  int num_ports = get_port_count( context );
  LOG_INFO( "%s/%d: found %d ports\n", mpi_node_name, mpi_rank, num_ports );
  int physical_port = 1;

  struct ibv_pd * protection_domain = get_protection_domain( context );

  void * foo = malloc( 1024 );
  struct ibv_mr * memory_region = get_memory_region( protection_domain, foo, 1024 );

  int depth = 128;
  struct ibv_cq * completion_queue = get_completion_queue( context, depth+1 );

  int scatter_gather_element_count = 1;
  struct ibv_qp_init_attr init_attributes = {
    .send_cq = completion_queue,
    .recv_cq = completion_queue,
    .cap = {
      .max_send_wr = depth,
      .max_recv_wr = depth,
      .max_send_sge = scatter_gather_element_count,
      .max_recv_sge = scatter_gather_element_count,
    },
    .qp_type = IBV_QPT_RC, // use "reliable connections"
    .sq_sig_all = 0, // only issue send completions if requested
  };
  struct ibv_qp * queue_pair = get_queue_pair( protection_domain, &init_attributes );

  // get my LID 
  struct ibv_port_attr port_attributes;
  ASSERT_Z( ibv_query_port( context, physical_port, &port_attributes ) );
  uint16_t my_lid = port_attributes.lid;
  uint16_t remote_lid = port_attributes.lid;

  // get my qp_num
  uint32_t my_qp_num = queue_pair->qp_num;
  uint32_t remote_qp_num = queue_pair->qp_num;

  // get my key
  uint32_t my_rkey = memory_region->rkey;
  uint32_t remote_rkey = memory_region->rkey;

  // get remote address
  uintptr_t my_raddr = (uintptr_t) memory_region->addr;
  uintptr_t remote_raddr = (uintptr_t) memory_region->addr;
  
  int remote_physical_port = physical_port;

  int my_rdma_depth = 16;
  int remote_rdma_depth = 16;

  enum ibv_mtu mtu = IBV_MTU_1024;
  struct ibv_qp_attr attributes = {
    .qp_access_flags = (IBV_ACCESS_LOCAL_WRITE | 
			IBV_ACCESS_REMOTE_WRITE | 
			IBV_ACCESS_REMOTE_READ  |
			IBV_ACCESS_REMOTE_ATOMIC),
    .pkey_index = 0,  // todo: ???
    
    // some network properties
    .port_num = physical_port,
    .path_mtu = mtu,
    
    // outstanding RDMA request limits (still subject to message limits)
    .max_rd_atomic = my_rdma_depth,
    .max_dest_rd_atomic = remote_rdma_depth,
    
    // timers and timeouts
    .min_rnr_timer = 12, // RNR NAK timer
    .timeout = 14, 
    .rnr_retry = 7,
    .retry_cnt = 7,
    
    .sq_psn = 123, // local packet sequence number
    .rq_psn = 123, // remote packet sequence number
    
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


  int pairs		= mpi_size / 2;
  int is_sender		= mpi_rank < pairs;
  int sender		= is_sender ? mpi_rank : mpi_rank - pairs;
  int is_receiver	= mpi_rank >= pairs;
  int receiver		= is_receiver ? mpi_rank : mpi_rank + pairs;


  uint64_t * send_word = (uint64_t *) memory_region->addr;
  uint64_t * recv_word = (uint64_t *) memory_region->addr + 8;

  {
    *send_word = 12345;
    
    struct ibv_sge send_sge = {
      .addr = (uintptr_t) recv_word,
      .length = 8,
      .lkey = memory_region->lkey,
    };
    struct ibv_send_wr send_wr = {
      .wr_id = 1,
      .next = NULL,
      .sg_list = &send_sge,
      .num_sge = 1,
      .opcode = IBV_WR_RDMA_READ,
      .send_flags = 0,
      .imm_data = 0,
      .wr.rdma.remote_addr = (uintptr_t) send_word,
      .wr.rdma.rkey = remote_rkey,
    };
    struct ibv_send_wr * bad_send_wr = NULL;
    ASSERT_Z( ibv_post_send( queue_pair, &send_wr, &bad_send_wr ) );
    ASSERT_Z( bad_send_wr );
    
    ASSERT_NZ( *recv_word == 12345 );
  }


  {
    *send_word = 23456;
    
    struct ibv_sge send_sge = {
      .addr = (uintptr_t) send_word,
      .length = 8,
      .lkey = memory_region->lkey,
    };
    struct ibv_send_wr send_wr = {
      .wr_id = 1,
      .next = NULL,
      .sg_list = &send_sge,
      .num_sge = 1,
      .opcode = IBV_WR_RDMA_WRITE,
      .send_flags = IBV_SEND_INLINE,
      .imm_data = 0,
      .wr.rdma.remote_addr = (uintptr_t) recv_word,
      .wr.rdma.rkey = remote_rkey,
    };
    struct ibv_send_wr * bad_send_wr = NULL;
    ASSERT_Z( ibv_post_send( queue_pair, &send_wr, &bad_send_wr ) );
    ASSERT_Z( bad_send_wr );
    
    ASSERT_NZ( *recv_word == 23456 );
  }

  {
    *send_word = 98765;

    struct ibv_sge recv_sge = {
      .addr = (uintptr_t) recv_word,
      .length = 8,
      .lkey = memory_region->lkey,
    };
    struct ibv_recv_wr recv_wr = {
      .wr_id = 2,
      .next = NULL,
      .sg_list = &recv_sge,
      .num_sge = 1,
    };
    struct ibv_recv_wr  * bad_recv_wr = NULL;
    ASSERT_Z( ibv_post_recv( queue_pair, &recv_wr, &bad_recv_wr ) );
    ASSERT_Z( bad_recv_wr );

    struct ibv_sge send_sge = {
      .addr = (uintptr_t) send_word,
      .length = 8,
      .lkey = memory_region->lkey,
    };
    struct ibv_send_wr send_wr = {
      .wr_id = 1,
      .next = NULL,
      .sg_list = &send_sge,
      .num_sge = 1,
      .opcode = IBV_WR_SEND,
      .send_flags = IBV_SEND_INLINE,
      .imm_data = 0,
    };
    struct ibv_send_wr * bad_send_wr = NULL;
    ASSERT_Z( ibv_post_send( queue_pair, &send_wr, &bad_send_wr ) );
    ASSERT_Z( bad_send_wr );

    int received = 0;
    while ( received < 1 ) {
      struct ibv_wc work_completions[1];
      int num_completion_entries = ibv_poll_cq( completion_queue, 
						1, 
						&work_completions[0] );
      for( i = 0; i < num_completion_entries; ++i ) {
	ASSERT_NZ( work_completions[i].status == IBV_WC_SUCCESS );
	++received;
      }
    }

    ASSERT_NZ( *recv_word == 98765 );
  }
  
  free_queue_pair( queue_pair );

  free_completion_queue( completion_queue );

  free_memory_region( memory_region );

  free_protection_domain( protection_domain );

  free_context( context );

  free_devices( infiniband_devices );


  LOG_INFO( "%s/%d: done.\n", mpi_node_name, mpi_rank );
  return 0;
}
