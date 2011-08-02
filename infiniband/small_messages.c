
#include <stdio.h>
#include <mpi.h>
#include <assert.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>

#include "debug.h"
#include "rdma.h"
#include "find_ib_interfaces.h"
#include "alloc.h"
#include "use_verbs.h"

int main( int argc, char * argv[] ) {

  MPI_Init( &argc, &argv );

  int rank = 0;
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );

  int size = 0;
  MPI_Comm_size( MPI_COMM_WORLD, &size );

  char node_name[MPI_MAX_PROCESSOR_NAME];
  int node_name_length;
  MPI_Get_processor_name( node_name, &node_name_length );

  LOG_INFO("%s: %s starting on node %d of %d.\n", node_name, argv[0], rank+1, size);

  const size_t total_alloc_size = 1 << 20;
  void * block = alloc_my_block( rank, size, total_alloc_size );

  //rdma_setup( node_name );

  //rdma_finalize( );

  ib_init( rank, size, node_name, block );
  
  ib_finalize( rank, size, node_name );

  MPI_Finalize();

  int result = free_my_block( rank, size, total_alloc_size, block );
  assert( 0 == result && "free failed" );

  return 0;
}
