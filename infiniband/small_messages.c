
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mpi.h>

#include "debug.h"

#include "alloc.h"

#include "mpi_utils.h"
#include "ib_utils.h"


int main( int argc, char * argv[] ) {

  ASSERT_NZ( sizeof( size_t ) == 8 && "is this not a 64-bit machine?" );
  
  int mpi_rank = 0;
  int mpi_size = 0;
  char mpi_node_name[MPI_MAX_PROCESSOR_NAME];
  softxmt_mpi_init( &argc, &argv, &mpi_rank, &mpi_size, mpi_node_name );

  //  ASSERT_NZ( mpi_size == 2 && "this benchmark currently requires exactly two MPI processes" );
  
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
		 10, 10, 
		 10, 10, 
		 1,
		 &global_context,
		 &node_contexts[0] );

  uint64_t * send_buffer_base = (uint64_t *) (my_block); // + sizeof( uint64_t ) * mpi_size );
  uint64_t * recv_buffer_base = (uint64_t *) (my_block + sizeof( uint64_t ) * mpi_size);
  uint64_t * rdma_buffer_base = (uint64_t *) (my_block + sizeof( uint64_t ) * mpi_size * 2);
  uint64_t * atomic_buffer_base = (uint64_t *) (my_block + sizeof( uint64_t ) * mpi_size * 3);
  uint64_t * fetch_buffer_base = (uint64_t *) (my_block + sizeof( uint64_t ) * mpi_size * 4);

  LOG_INFO( "%s/%d: initialized.\n", mpi_node_name, mpi_rank );
  
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
