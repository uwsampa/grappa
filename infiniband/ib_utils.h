
#ifndef __IB_UTILS_H__
#define __IB_UTILS_H__

#include <infiniband/verbs.h>

struct global_ib_context {
  struct ibv_context * device;
  struct ibv_pd * protection_domain;
  struct ibv_mr * memory_region;
};

struct node_ib_context {
  struct ibv_cq * completion_queue;
  struct ibv_qp * queue_pair;
  void * remote_address;
  uint32_t remote_key;
};

void ib_global_init( int mpi_rank, int mpi_size, char mpi_node_name[], 
		     void * my_block, size_t my_block_size,
		     struct global_ib_context * context );

void ib_nodes_init( int mpi_rank, int mpi_size, char mpi_node_name[], 
		    int physical_port,
		    int send_message_depth, int receive_message_depth, 
		    int send_rdma_depth, int receive_rdma_depth, 
		    int scatter_gather_element_count,
		    struct global_ib_context * global_context,
		    struct node_ib_context node_contexts[] );

void ib_nodes_finalize( int mpi_rank, int mpi_size, char mpi_node_name[], 
			struct global_ib_context * context, 
			struct node_ib_context node_contexts[] );

void ib_global_finalize( int mpi_rank, int mpi_size, char mpi_node_name[], 
			 struct global_ib_context * context );

inline void ib_post_send( struct global_ib_context * global_context,
			  struct node_ib_context * node_context,
			  void * pointer, size_t size, uint64_t id );

inline void ib_post_receive( struct global_ib_context * global_context,
			     struct node_ib_context * node_context,
			     void * pointer, size_t size, uint64_t id );

inline void ib_post_read( struct global_ib_context * global_context,
			  struct node_ib_context * node_context,
			  void * pointer, void * remote_pointer, size_t size, 
			  uint64_t id );

inline void ib_post_write( struct global_ib_context * global_context,
			   struct node_ib_context * node_context,
			   void * pointer, void * remote_pointer, size_t size, 
			   uint64_t id );

inline void ib_post_fetch_and_add( struct global_ib_context * global_context,
				   struct node_ib_context * node_context,
				   void * pointer, void * remote_pointer, size_t size, 
				   uintptr_t add, uint64_t id );

void ib_complete( int mpi_rank, int mpi_size, char mpi_node_name[], 
		  struct global_ib_context * global_context,
		  struct node_ib_context * node_context,
		  int count );
#endif
