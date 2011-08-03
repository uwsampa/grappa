
#include "debug.h"


#include <mpi.h>

void softxmt_mpi_finalize() {
  ASSERT_Z( MPI_Finalize() );
}

void softxmt_mpi_init( int * argc_p, char ** argv_p[], int * rank_p, int * size_p, char node_name[] ) {
  ASSERT_Z( MPI_Init( argc_p, argv_p ) );
  ASSERT_Z( MPI_Comm_rank( MPI_COMM_WORLD, rank_p ) );
  ASSERT_Z( MPI_Comm_size( MPI_COMM_WORLD, size_p ) );
  int node_name_length;
  ASSERT_Z( MPI_Get_processor_name( node_name, &node_name_length ) );
  ASSERT_Z( atexit( &softxmt_mpi_finalize ) );
}
