
#include <iostream>
#include <cassert>
#include <cstdint>

#include <mpi.h>

#define DEBUG 1

#include "MPIWorker.hpp"
#include "MPICommunicator.hpp"
#include "GlobalArray.hpp"

#include "ListNode.hpp"
#include "GlobalListAllocate.hpp"


int main( int argc, char* argv[] ) {
  MPI_Init( &argc, &argv );

  int my_rank = -1;
  int total_num_nodes = -1;

  MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );
  MPI_Comm_size( MPI_COMM_WORLD, &total_num_nodes );


  MPI_Comm request_communicator, response_communicator;
  MPI_Comm_dup( MPI_COMM_WORLD, &request_communicator );
  MPI_Comm_dup( MPI_COMM_WORLD, &response_communicator );


  {
    const int total_size = 16;
    const int total_num_threads = total_num_nodes;
    const int total_num_lists = 4 * total_num_threads;

    MPICommunicator< MemoryDescriptor > communicator( my_rank, total_num_nodes, request_communicator, response_communicator );
    typedef GlobalArray< ListNode, MemoryDescriptor, MPICommunicator< MemoryDescriptor > > ListNodeGlobalArray;
    typedef GlobalArray< ListNodeGlobalArray::Cell * , MemoryDescriptor, MPICommunicator< MemoryDescriptor > > BasesGlobalArray;

    MPIWorker<> worker( request_communicator, response_communicator );


    ListNodeGlobalArray list_nodes( total_size, my_rank, total_num_nodes, &communicator );
    int list_nodes_array_id = worker.register_array( reinterpret_cast< uint64_t * >( list_nodes.getBase() ) );
    list_nodes.setArrayId( list_nodes_array_id );

    BasesGlobalArray bases( total_num_lists, my_rank, total_num_nodes, &communicator );
    int bases_array_id = worker.register_array( reinterpret_cast< uint64_t * >( bases.getBase() ) );
    bases.setArrayId( bases_array_id );

    int result = MPI_Barrier( MPI_COMM_WORLD );
    assert( result == 0 );

    if (my_rank == 0) {
      
#pragma omp parallel for num_threads(2)
      for( int thread = 0; thread < 2; ++thread) {
        
        if (thread == 0) { // worker thread
          
          std::cout << "Initializing array" << std::endl;

	  globalListAllocate( total_size, total_num_threads, total_num_lists, &list_nodes, &bases );
	  //globalListAllocate< ListNodeGlobalArray, BasesGlobalArray>( total_size, total_num_nodes, &list_nodes, &bases );


          //   MemoryDescriptor md;
          //   md.type = MemoryDescriptor::Write;
          //   md.index = i;
          //   md.data = i;
          //   //arr.blockingOp( &md );

          //   std::cout << "issuing store to " << i << std::endl;
          //   MPIGlobalArray::InFlightRequest requests = arr.issueOp( &md );

          //   std::cout << "completing store to " << i << std::endl;
          //   arr.completeOp( &md, requests );

          // }

          // std::cout << "Initialization complete" << std::endl;          

          // for( int i = 0; i < total_size / total_num_nodes; ++i ) {
          //   std::cout << i << ": " << (arr.array.get())[i] << std::endl;
          // }

          // std::cout << "Checking array" << std::endl;
          // for( int i = 0; i < total_size; ++i ) {
          //   MemoryDescriptor md;
          //   md.type = MemoryDescriptor::Read;
          //   md.index = i;
          //   arr.blockingOp( &md );
          //   uint64_t data = md.data;
          //   //uint64_t data = arr.blockingGet( i );
          //   if ( i != data ) {
          //     std::cout << "Error at " << i << ": expected " << i << ", got " << data << std::endl;
          //   } else {
          //     std::cout << "Success at " << i << ": expected " << i << ", got " << data << std::endl;
          //   }
          //   assert( i == data );
          // }

          // std::cout << "All results check out." << std::endl;

          list_nodes.blockingQuit();
          std::cout << "Quit sent." << std::endl;
          
        } else {
          bool active = true;
          while (active) {
            active = worker.tick();
            //sleep(1);
          }
        }
      }
    } else { // other ranks
      bool active = true;
      while (active) {
        active = worker.tick();
        //sleep(1);
      }
    }
  }

  int result = MPI_Barrier( MPI_COMM_WORLD );
  assert( result == 0 );

  std::cout << "Done." << std::endl;

  MPI_Finalize();
  return 0;
}
