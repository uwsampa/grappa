
#include "options.h"

#include <infiniband/verbs.h>
#include <infiniband/arch.h>


struct batched_sender {
  struct ibv_qp * qp;
  struct ibv_cq * cq;
  struct ibv_sge * send_sges;
  struct ibv_send_wr * send_wrs;
  int * send_stack;
  int send_stack_index;
  uint64_t send_checksum;

  struct ibv_sge * recv_sges;
  struct ibv_recv_wr * recv_wrs;
  int * recv_stack;
  int recv_stack_index;
  uint64_t recv_checksum;

  int batch_size;
  int outstanding;
  int current_batch_size;
  int current_outstanding_recvs;
  struct ibv_send_wr * current_wr;

  char * mpi_node_name;
  int mpi_rank;
};

struct batched_sender * construct_batched_sender( char * mpi_node_name, int mpi_rank,
						  struct options * opt, struct ibv_mr * mr, 
						  struct ibv_qp * qp, struct ibv_cq * cq );

void teardown_batched_sender( struct batched_sender * bs );

inline int batched_send( struct batched_sender * bs, 
			 char * local_address, int len );

inline int batched_flush( struct batched_sender * bs );

inline int batched_send_poll( struct batched_sender * bs );

void batched_recv_arm( struct batched_sender * bs );
