
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <hugetlbfs.h>

#include <mpi.h>

#include <infiniband/verbs.h>
#include <infiniband/arch.h>

#include "debug.h"

#include "use_verbs.h"

enum {
  RECV_WRID = 1,
  SEND_WRID = 2,
};

struct context {
  struct ibv_context * context;
  int port;
  struct ibv_comp_channel * channel;
  struct ibv_pd * protection_domain;
  struct ibv_mr * memory_region;
  struct ibv_cq * completion_queue;
  struct ibv_qp * queue_pair;
  void * buffer;
  size_t size;
  int receive_depth;
  int pending;
  struct ibv_port_attr port_attributes;
};

void ib_init( int mpi_rank, int mpi_size, const char * node_name, void * block ) {
  
  int result = 0;

  int num_devices = 0;
  struct ibv_device ** devices = ibv_get_device_list( &num_devices );
  assert( 0 != num_devices && "no ib devices found" );
  assert( NULL != devices && "ib device list empty" );

  printf( "%s: found %d ib devices\n", node_name, num_devices );

  uint64_t my_first_guid = 0;
  int i;
  for( i = 0; i < num_devices; ++i ) {
    uint64_t guid = (uint64_t) ntohll( ibv_get_device_guid( devices[i] ) );
    if (!my_first_guid) my_first_guid = guid;
    LOG_INFO("%s: device %d: %s with guid %.16lx\n", node_name, i,  
	     ibv_get_device_name( devices[i] ), 
	     guid );
  }

  // collect GUIDs of all nodes so we can open connections
  uint64_t * guids = malloc( mpi_size * sizeof(uint64_t) );
  result = MPI_Allgather(  &my_first_guid, 1, MPI_UNSIGNED_LONG_LONG, 
			   guids, 1, MPI_UNSIGNED_LONG_LONG, 
			   MPI_COMM_WORLD );
  assert( 0 == result && "allgather GUID communication failed" );

  for( i = 0; i < mpi_size; ++i ) {
    LOG_INFO( "%s knows about node %d's guid %.16lx\n", node_name, i, guids[i] );
  }



  struct context ctx;


  long int hugepagesize = 0;
  uint64_t hugecount = gethugepagesizes( &hugepagesize, 1 );
  assert( 1 == hugecount && "too many/few huge page sizes" );
  assert( (1 << 30) == hugepagesize && "wrong default page size" );

  

  //const int buffer_size = 1 << 16;
  size_t buffer_size = (1LL << 30) * 6;

  ctx.size = buffer_size;

  ctx.receive_depth = 500;

  //const int page_size = 4096;
  //ctx.buffer = memalign( page_size, buffer_size );
  ctx.buffer = get_huge_pages( buffer_size, GHP_DEFAULT );
  

  
  assert( NULL != ctx.buffer && "work buffer allocation failed" );

  memset( ctx.buffer, 0, buffer_size );

  ctx.context = ibv_open_device( devices[ 0 ] );
  assert( NULL != ctx.context && "couldn't get device context" );

/*   ctx.channel = ibv_create_comp_channel( ctx.context ); */
/*   assert( NULL != ctx.channel && "couldn't get completion channel" ); */
  ctx.channel = NULL; // unused for now
  
  ctx.protection_domain = ibv_alloc_pd( ctx.context );
  assert( NULL != ctx.protection_domain && "couldn't alloc protection domain" );

  ctx.memory_region = ibv_reg_mr( ctx.protection_domain, ctx.buffer, buffer_size, IBV_ACCESS_LOCAL_WRITE );
  assert( NULL != ctx.memory_region && "couldn't register memory region" );

  ctx.completion_queue = ibv_create_cq( ctx.context, ctx.receive_depth+1, NULL, ctx.channel, 0 );
  assert( NULL != ctx.completion_queue && "couldn't create completion queue" );

  struct ibv_qp_init_attr init_attr = {
    .send_cq = ctx.completion_queue,
    .recv_cq = ctx.completion_queue,
    .cap = {
      .max_send_wr = 1,
      .max_recv_wr = ctx.receive_depth,
      .max_send_sge = 1,
      .max_recv_sge = 1
    },
    .qp_type = IBV_QPT_RC
  };
  ctx.queue_pair = ibv_create_qp( ctx.protection_domain, &init_attr );
  assert( NULL != ctx.queue_pair && "couldn't create queue pair" );

  // move queue pair from RESET to INIT
  ctx.port = 1;
  {
    struct ibv_qp_attr attr = {
      .qp_state = IBV_QPS_INIT,
      .pkey_index = 0,
      .port_num = ctx.port, // I think this is the physical port
      .qp_access_flags = 0
    };
    
    result = ibv_modify_qp( ctx.queue_pair, &attr, 
			    IBV_QP_STATE | 
			    IBV_QP_PKEY_INDEX | 
			    IBV_QP_PORT | 
			    IBV_QP_ACCESS_FLAGS );
    assert( 0 == result && "couldn't modify queue pair to INIT" );
  }

  // post receive work requests
  int num_outstanding_receives = ctx.receive_depth;
  {
    struct ibv_sge list = {
      .addr = (uintptr_t) ctx.buffer,
      .length = (1 << 29), //ctx.size,
      .lkey = ctx.memory_region->lkey
    };
    struct ibv_recv_wr wr = {
      .wr_id = RECV_WRID,
      .sg_list = &list,
      .num_sge = 1
    };
    struct ibv_recv_wr * bad_wr;
    for (i = 0; i < num_outstanding_receives; ++i ) {
      result = ibv_post_recv( ctx.queue_pair, &wr, &bad_wr );
      assert( 0 == result && "couldn't post recieve WR" );
    }
  }

  

  // collect LIDs of all nodes so we can open connections
  result = ibv_query_port( ctx.context, ctx.port, &ctx.port_attributes );
  assert( 0 == result && "couldn't query port attributes" );
  //  assert( IBV_LINK_LAYER_INFINIBAND == ctx.port_attributes.link_layer && "this isn't an infiniband card?");
  assert( 0 != ctx.port_attributes.lid && "didn't get a LID?");

  uint16_t * lids = malloc( mpi_size * sizeof(uint16_t) );
  result = MPI_Allgather(  &ctx.port_attributes.lid, 1, MPI_SHORT,
			   lids, 1, MPI_SHORT,
			   MPI_COMM_WORLD );
  assert( 0 == result && "allgather LID communication failed" );

  for( i = 0; i < mpi_size; ++i ) {
    LOG_INFO( "%s knows about node %d's lid %.4x\n", node_name, i, lids[i] );
  }
 
  // collect QP numbers from all nodes so we can open connections
  uint32_t * qp_nums = malloc( mpi_size * sizeof(uint32_t) );
  result = MPI_Allgather(  &ctx.queue_pair->qp_num, 1, MPI_UNSIGNED,
			   qp_nums, 1, MPI_UNSIGNED,
			   MPI_COMM_WORLD );
  assert( 0 == result && "allgather qp_num communication failed" );

  for( i = 0; i < mpi_size; ++i ) {
    LOG_INFO( "%s knows about node %d's qp_num %.4x\n", node_name, i, qp_nums[i] );
  }

  // generate PSNs for all nodes
  uint32_t my_psn = lrand48() & 0xffffff;
  uint32_t * psns = malloc( mpi_size * sizeof(uint32_t) );
  result = MPI_Allgather(  &my_psn, 1, MPI_UNSIGNED,
			   psns, 1, MPI_UNSIGNED,
			   MPI_COMM_WORLD );
  assert( 0 == result && "allgather psns communication failed" );

  for( i = 0; i < mpi_size; ++i ) {
    LOG_INFO( "%s knows about node %d's psn %.4x\n", node_name, i, psns[i] );
  }

  //resibv_query_gid(ctx->context, ib_port, gidx, &my_dest.gid)) {
  union ibv_gid my_gid;
  result = ibv_query_gid( ctx.context, ctx.port, 0, &my_gid );
  assert( 0 == result && "gid read failed" );

  // collect GUIDs of all nodes so we can open connections
  union ibv_gid * gids = malloc( mpi_size * sizeof(union ibv_gid) );
  result = MPI_Allgather(  &my_gid, sizeof(union ibv_gid), MPI_CHAR, 
			   gids, sizeof(union ibv_gid), MPI_CHAR, 
			   MPI_COMM_WORLD );
  assert( 0 == result && "allgather GID communication failed" );

  for( i = 0; i < mpi_size; ++i ) {
    char my_gid_str[33];
    inet_ntop(AF_INET6, &gids[i], my_gid_str, sizeof my_gid_str);

    LOG_INFO( "%s knows about node %d's gid %s\n", node_name, i, my_gid_str );
  }


  uint32_t dest_rank = (mpi_rank + 1) % mpi_size;

  char my_gid_str[33];
  inet_ntop(AF_INET6, &gids[ mpi_rank  ], my_gid_str, sizeof my_gid_str);
  char dest_gid_str[33];
  inet_ntop(AF_INET6, &gids[ dest_rank ], dest_gid_str, sizeof dest_gid_str);

  printf("  local address:  LID 0x%04x, QPN 0x%06x, PSN 0x%06x, GID %s\n",
	 lids[ mpi_rank ], qp_nums[ mpi_rank ], psns[ mpi_rank ], my_gid_str);
  printf(" remote address:  LID 0x%04x, QPN 0x%06x, PSN 0x%06x, GID %s\n",
	 lids[ dest_rank ], qp_nums[ dest_rank ], psns[ dest_rank ], dest_gid_str);

  int is_server = mpi_rank % 2;
  if (is_server) {
    printf("%s is a server.\n", node_name);
  } else {
    printf("%s is a client.\n", node_name);
  }

  //if (!is_server) {
    enum ibv_mtu		 mtu = IBV_MTU_1024;

    struct ibv_qp_attr attr = {
      .qp_state               = IBV_QPS_RTR,
      .path_mtu               = mtu,
      .dest_qp_num            = qp_nums[ dest_rank ],
      .rq_psn                 = psns[ dest_rank ],
      .max_dest_rd_atomic     = 1,
      .min_rnr_timer          = 12,
      .ah_attr                = {
	.is_global      = 0,
	.dlid           = lids[ dest_rank ],
	.sl             = 0,
	.src_path_bits  = 0,
	.port_num       = ctx.port
      }
    };
    result = ibv_modify_qp( ctx.queue_pair, &attr, 
			    IBV_QP_STATE | 
			    IBV_QP_AV |
			    IBV_QP_PATH_MTU | 
			    IBV_QP_DEST_QPN | 
			    IBV_QP_RQ_PSN | 
			    IBV_QP_MAX_DEST_RD_ATOMIC | 
			    IBV_QP_MIN_RNR_TIMER );
    assert( 0 == result && "couldn't modify queue pair to RTR" );
  
    attr.qp_state = IBV_QPS_RTS;
    attr.timeout = 14;
    attr.retry_cnt = 7;
    attr.rnr_retry = 7;
    attr.sq_psn = psns[ mpi_rank ];
    attr.max_rd_atomic = 1;
    result = ibv_modify_qp( ctx.queue_pair, &attr, 
			    IBV_QP_STATE | 
			    IBV_QP_TIMEOUT |
			    IBV_QP_RETRY_CNT | 
			    IBV_QP_RNR_RETRY | 
			    IBV_QP_SQ_PSN | 
			    IBV_QP_MAX_QP_RD_ATOMIC );
    assert( 0 == result && "couldn't modify queue pair to RTS" );
    //  }
  
  ctx.pending = RECV_WRID;

  if (!is_server) {
    struct ibv_sge list = {
      .addr = (uintptr_t) ctx.buffer,
      .length = (1 << 29), //ctx.size,
      .lkey = ctx.memory_region->lkey
    };
    struct ibv_send_wr work_request = {
      .wr_id = SEND_WRID,
      .sg_list = &list,
      .num_sge = 1,
      .opcode = IBV_WR_SEND,
      .send_flags = IBV_SEND_SIGNALED,
    };
    struct ibv_send_wr *bad_wr;

    result = ibv_post_send( ctx.queue_pair, &work_request, &bad_wr);
    assert( 0 == result && "send failed" );
    ctx.pending |= SEND_WRID;
    //ctx.pending = SEND_WRID | RECV_WRID;
  }


  struct timeval start, end;

  if (gettimeofday(&start, NULL)) {
    perror("gettimeofday");
    assert(0);
  }


  int receive_count = 0;
  int send_count = 0;
  int iterations = 100;

  while ( receive_count < iterations && send_count < iterations ) {
    
    struct ibv_wc work_completions[2];
    int num_entries = 0;
    
    do {
      num_entries = ibv_poll_cq( ctx.completion_queue, 2, work_completions );
      assert( num_entries >= 0 && "CQ poll failed" );
    } while ( num_entries < 1 );
    
    for( i = 0; i < num_entries; ++i ) {
/*       printf("%s: %d: found status %s (%d) for wr_id %d receive_count %d send_count %d iterations %d\n",  */
/* 	     node_name, i, */
/* 	      ibv_wc_status_str(work_completions[i].status), */
/* 	     work_completions[i].status, (int) work_completions[i].wr_id, */
/* 	     receive_count, send_count, iterations); */
      if ( work_completions[i].status != IBV_WC_SUCCESS ) {
	fprintf(stderr, "Failed status %s (%d) for wr_id %d\n",
		ibv_wc_status_str(work_completions[i].status),
		work_completions[i].status, (int) work_completions[i].wr_id);
	assert( 0 && "bad work completion status" );
      }
      switch ( (int) work_completions[i].wr_id ) {
      case SEND_WRID:
	++send_count;
	break;
      case RECV_WRID:
	// stuff
	if (--num_outstanding_receives <= 1) {
	  struct ibv_sge list = {
	    .addr = (uintptr_t) ctx.buffer,
	    .length = (1 << 29), //ctx.size,
	    .lkey = ctx.memory_region->lkey
	  };
	  struct ibv_recv_wr wr = {
	    .wr_id = RECV_WRID,
	    .sg_list = &list,
	    .num_sge = 1
	  };
	  struct ibv_recv_wr * bad_wr;
	  int j;
	  for (j = 0; j < ctx.receive_depth - num_outstanding_receives; ++j ) {
	    result = ibv_post_recv( ctx.queue_pair, &wr, &bad_wr );
	    assert( 0 == result && "couldn't post recieve WR" );
	  }
	  num_outstanding_receives += ctx.receive_depth - num_outstanding_receives;
	}
	++receive_count;
	break;
      default:
	fprintf(stderr, "Completion for unknown wr_id %d\n",
		(int) work_completions[i].wr_id);
	assert( 0 && "completion for unknown wr_id" );
      }

      ctx.pending &= ~(int) work_completions[i].wr_id;
      if ( send_count < iterations && !ctx.pending ) {
	struct ibv_sge list = {
	  .addr = (uintptr_t) ctx.buffer,
	  .length = (1 << 29), //ctx.size,
	  .lkey = ctx.memory_region->lkey
	};
	struct ibv_send_wr work_request = {
	  .wr_id = SEND_WRID,
	  .sg_list = &list,
	  .num_sge = 1,
	  .opcode = IBV_WR_SEND,
	  .send_flags = IBV_SEND_SIGNALED,
	};
	struct ibv_send_wr *bad_wr;
	
	result = ibv_post_send( ctx.queue_pair, &work_request, &bad_wr);
	assert( 0 == result && "send failed" );
	//ctx.pending |= SEND_WRID;
	ctx.pending = SEND_WRID | RECV_WRID;
      }
    }
  }


  if (gettimeofday(&end, NULL)) {
    perror("gettimeofday");
    assert(0);
  }
  
  {
    float usec = (end.tv_sec - start.tv_sec) * 1000000 +
      (end.tv_usec - start.tv_usec);
    //long long bytes = (long long) ctx.size * iterations * 2;
    long long bytes = (long long) (1 << 29) * iterations * 2;
    
    printf("%lld bytes in %.2f seconds = %.2f Mbit/sec\n",
	   bytes, usec / 1000000., bytes * 8. / usec);
    printf("%d iters in %.2f seconds = %.2f usec/iter = %.2f Miter/s\n",
	   iterations, usec / 1000000., usec / iterations, (double) iterations / usec);
    
  }


  int num_cq_events = 0;
  ibv_ack_cq_events( ctx.completion_queue, num_cq_events);

  result = ibv_destroy_qp( ctx.queue_pair );
  assert( 0 == result && "couldn't destroy queue pair" );

  result = ibv_destroy_cq( ctx.completion_queue );
  assert( 0 == result && "couldn't destroy completion queue" );

  result = ibv_dereg_mr( ctx.memory_region );
  assert( 0 == result && "couldn't deregister memory region" );

  result = ibv_dealloc_pd( ctx.protection_domain );
  assert( 0 == result && "couldn't deallocate protection domain" );

  if ( ctx.channel ) { 
    result = ibv_destroy_comp_channel( ctx.channel );
    assert( 0 == result && "couldn't destroy completion channel" );
  }

  result = ibv_close_device( ctx.context );
  assert( 0 == result && "couldn't close device" );

  //free( ctx.buffer );
  free_huge_pages( ctx.buffer );

  ibv_free_device_list(devices);
  free(lids);
  free(guids);
}

void ib_finalize() {
  ;
}
