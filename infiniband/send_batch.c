
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

#include "send_batch.h"

#include <infiniband/verbs.h>
#include <infiniband/arch.h>



inline void send_push( int data, struct batched_sender * bs ) {
  int next = bs->send_stack_index + 1;
  ASSERT_NZ( next < bs->outstanding );
  bs->send_stack[next] = data;
  bs->send_stack_index = next;
}

inline int send_get( struct batched_sender * bs ) {
  ASSERT_NZ( 0 <= bs->send_stack_index );
  return bs->send_stack[ bs->send_stack_index ];
}

inline int send_check( struct batched_sender * bs ) {
  return 0 <= bs->send_stack_index ;
}

inline void send_pop( struct batched_sender * bs ) {
  int next = bs->send_stack_index - 1;
  ASSERT_NZ( -1 <= next );
  bs->send_stack_index = next;
}

inline void recv_push( int data, struct batched_sender * bs ) {
  int next = bs->recv_stack_index + 1;
  ASSERT_NZ( next < bs->outstanding );
  bs->recv_stack[next] = data;
  bs->recv_stack_index = next;
}

inline int recv_get( struct batched_sender * bs ) {
  ASSERT_NZ( 0 <= bs->recv_stack_index );
  return bs->recv_stack[ bs->recv_stack_index ];
}

inline void recv_pop( struct batched_sender * bs ) {
  int next = bs->recv_stack_index - 1;
  ASSERT_NZ( -1 <= next );
  bs->recv_stack_index = next;
}

struct batched_sender * construct_batched_sender( char * mpi_node_name, int mpi_rank,
						  struct options * opt, struct ibv_mr * mr, 
						  struct ibv_qp * qp, struct ibv_cq * cq ) {
  struct batched_sender * bs = malloc( sizeof( struct batched_sender ) );
  bs->mpi_node_name = mpi_node_name;
  bs->mpi_rank = mpi_rank;
  bs->batch_size = opt->batch_size;
  bs->outstanding = opt->outstanding;
  bs->qp = qp;
  bs->cq = cq;
  bs->current_batch_size = 0;
  bs->current_outstanding_recvs = 0;
  bs->current_wr = NULL;

  bs->send_sges = malloc( opt->outstanding * sizeof( struct ibv_sge ) );
  bs->send_wrs = malloc( opt->outstanding * sizeof( struct ibv_send_wr ) );
  bs->send_stack = malloc( opt->outstanding * sizeof( int ) );
  bs->send_stack_index = -1;

  bs->recv_sges = malloc( opt->outstanding * sizeof( struct ibv_sge ) );
  bs->recv_wrs = malloc( opt->outstanding * sizeof( struct ibv_recv_wr ) );
  bs->recv_stack = malloc( opt->outstanding * sizeof( int ) );
  bs->recv_stack_index = -1;

  int i;
  for( i = 0; i < opt->outstanding; ++i ) {
    bs->send_sges[i].lkey = mr->lkey;
    bs->send_sges[i].addr = 0;
    bs->send_sges[i].length = 0;
    bs->send_wrs[i].wr_id = i;
    bs->send_wrs[i].next = NULL;
    bs->send_wrs[i].sg_list = &bs->send_sges[i];
    bs->send_wrs[i].num_sge = 1;
    bs->send_wrs[i].opcode = IBV_WR_SEND;
    bs->send_wrs[i].send_flags = IBV_SEND_INLINE | IBV_SEND_SIGNALED;
    //bs->send_wrs[i].send_flags = IBV_SEND_SIGNALED;
    bs->send_wrs[i].imm_data = 0;
    send_push( i, bs );

    bs->recv_sges[i].lkey = mr->lkey;
    bs->recv_sges[i].addr = mr->addr;
    bs->recv_sges[i].length = mr->length;
    bs->recv_wrs[i].wr_id = i;
    bs->recv_wrs[i].next = i + 1 == opt->outstanding ? NULL : &bs->recv_wrs[ (i + 1) % opt->outstanding ];
    bs->recv_wrs[i].sg_list = &bs->recv_sges[i];
    bs->recv_wrs[i].num_sge = 1;
    //    recv_push( i, bs );
  }
  return bs;
}

void teardown_batched_sender( struct batched_sender * bs ) {
  free( bs->send_sges );
  free( bs->send_wrs );
  free( bs->send_stack );
  free( bs->recv_sges );
  free( bs->recv_wrs );
  free( bs->recv_stack );
  free( bs );
}

inline int batched_send( struct batched_sender * bs, 
	      char * local_address, int len ) {
  int avail = send_check( bs );
  if( 0 == avail ) {
    batched_send_poll( bs );
    return 0;
  } else {
    struct ibv_send_wr * this_wr = &bs->send_wrs[ send_get( bs ) ];
    send_pop( bs );

    this_wr->sg_list->addr = (uintptr_t) local_address;
    this_wr->sg_list->length = len;
    this_wr->next = bs->current_wr;
    bs->current_wr = this_wr;
    bs->current_batch_size++;
    //if( bs->current_batch_size >= bs->batch_size ) batched_flush( bs );
    if( bs->current_batch_size >= bs->batch_size ) batched_send_poll( bs );
    //batched_send_poll( bs );
    return 1;
  }
}

inline int batched_flush( struct batched_sender * bs ) {
  struct ibv_send_wr * bad_send_wr = NULL;
  ASSERT_Z( ibv_post_send( bs->qp, bs->current_wr, &bad_send_wr ) );
  ASSERT_Z( bad_send_wr );
  bs->current_wr = NULL;
  int count = bs->current_batch_size;
  bs->current_batch_size = 0;
  return count;
}

inline int batched_send_poll( struct batched_sender * bs ) {
  int count = 0;
  int i;

  // see if we have stuff to send
  if( bs->current_batch_size >= bs->batch_size ) {
    //LOG_INFO( "%s/%d: sending %d work requests\n", bs->mpi_node_name, bs->mpi_rank, bs->current_batch_size );
    batched_flush( bs );
  }

  struct ibv_recv_wr * recycled_recv_wrs = NULL;

  // see if we have stuff to receive
  struct ibv_wc work_completions[bs->batch_size];
  int num_completion_entries = ibv_poll_cq( bs->cq, 
					    bs->batch_size, 
					    &work_completions[0] );
  for( i = 0; i < num_completion_entries; ++i ) {
    if ( work_completions[i].status != IBV_WC_SUCCESS ) {
      LOG_ERROR( "%s/%d: Failed status %s (%d) for wr_id %d\n", bs->mpi_node_name, bs->mpi_rank,
		 ibv_wc_status_str(work_completions[i].status),
		 work_completions[i].status, (int) work_completions[i].wr_id );
    }
    ASSERT_NZ( work_completions[i].status == IBV_WC_SUCCESS );
    if( work_completions[i].opcode & IBV_WC_RECV ) { // if it's some kind of receive
      // grab pointer to work request
      struct ibv_recv_wr * received_wr = &bs->recv_wrs[ work_completions[i].wr_id ];
      // add to list of recycled work requests
      received_wr->next = recycled_recv_wrs;
      recycled_recv_wrs = received_wr;
      // // checksum something....
      // bs->checksum += *( (char*) received_wr->addr ) + (bs->checksum >> 7);
      ++count;
    } else {
      send_push( work_completions[i].wr_id, bs );
    }
  }

  if (recycled_recv_wrs) {
    //LOG_INFO( "%s/%d: recycling received work requests \n", bs->mpi_node_name, bs->mpi_rank );
    struct ibv_recv_wr  * bad_work_request = NULL;
    int result = ibv_post_recv( bs->qp, recycled_recv_wrs, &bad_work_request );
    if (result) {
      LOG_ERROR(__FILE__ ":%d: error: ibv_post_recv failed (returned %d, with bad wr %p next %p).\n", __LINE__-2, result, bad_work_request, bad_work_request->next);
    }
    //ASSERT_Z( ibv_post_recv( bs->qp, recycled_recv_wrs, &bad_work_request ) );
    //ASSERT_Z( bad_work_request );
  }

  return count;
}

void batched_recv_arm( struct batched_sender * bs ) {
  //int i;
  struct ibv_recv_wr  * bad_work_request = NULL;
  //bs->recv_stack_index = -1;
  //ASSERT_Z( ibv_post_recv( bs->qp, &bs->recv_wrs[ bs->recv_stack[0] ], &bad_work_request ) );
  ASSERT_Z( ibv_post_recv( bs->qp, &bs->recv_wrs[0], &bad_work_request ) );
  ASSERT_Z( bad_work_request );
}

 /* void batched_recv_poll( struct batched_sender * bs ) { */
 /*  // see if we have stuff to receive */
 /*  struct ibv_wc work_completions[opt->batch_size]; */
 /*  int num_completion_entries = ibv_poll_cq( bs->cq,  */
 /* 					    opt->batch_size,  */
 /* 					    &work_completions[0] ); */
 /*  for( i = 0; i < num_completion_entries; ++i ) { */
 /*    ASSERT_NZ( work_completions[i].status == IBV_WC_SUCCESS ); */
 /*     send_push( work_completions[i].wr_id, bs ); */
 /*  } */
 /* } */
