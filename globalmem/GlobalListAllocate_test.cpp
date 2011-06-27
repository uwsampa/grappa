
#include <iostream>
#include <cassert>
#include <cstdint>

#include <mpi.h>

#define DEBUG 1

#include <MPIWorker.hpp>
#include <MPICommunicator.hpp>
#include <GlobalArray.hpp>
#include <GlobalListAllocate.hpp>


int main( int argc, char* argv[] ) {
  MPI_Init( &argc, &argv );

  int my_rank = -1;
  int total_num_nodes = -1;

  MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );
  MPI_Comm_size( MPI_COMM_WORLD, &total_num_nodes );


  MPI_Comm request_communicator, response_communicator;
  MPI_Comm_dup( MPI_COMM_WORLD, &request_communicator );
  MPI_Comm_dup( MPI_COMM_WORLD, &response_communicator );

  int rank1, rank2;
  MPI_Comm_rank( request_communicator, &rank1 );
  MPI_Comm_rank( response_communicator, &rank2 );

  std::cout << "Process " << my_rank+1 
            << " (dup1:" << rank1+1 << " dup2:" << rank2+1 << ")"
            << " of " << total_num_nodes << std::endl;

  {
    const int total_size = 128;

    MPICommunicator< MemoryDescriptor > communicator( my_rank, total_num_nodes, request_communicator, response_communicator );
    typedef GlobalArray< MemoryDescriptor, MPICommunicator< MemoryDescriptor > > MPIGlobalArray;

    MPIGlobalArray arr( total_size, my_rank, total_num_nodes, &communicator );

    MPIWorker<> worker( request_communicator, response_communicator );
    int array_id = worker.register_array( arr.getBase() );
    assert( array_id == 0 );

    int result = MPI_Barrier( MPI_COMM_WORLD );
    assert( result == 0 );

    if (my_rank == 0) {
      
#pragma omp parallel for num_threads(2)
      for( int thread = 0; thread < 2; ++thread) {
        
        if (thread == 0) {
          
          std::cout << "Initializing array" << std::endl;



          for( int i = 0; i < total_size; ++i ) {
            //arr.blockingPut( i, i );
            MemoryDescriptor md;
            md.type = MemoryDescriptor::Write;
            md.index = i;
            md.data = i;
            //arr.blockingOp( &md );

            std::cout << "issuing store to " << i << std::endl;
            MPIGlobalArray::InFlightRequest requests = arr.issueOp( &md );

            std::cout << "completing store to " << i << std::endl;
            arr.completeOp( &md, requests );

          }

          std::cout << "Initialization complete" << std::endl;          

          for( int i = 0; i < total_size / total_num_nodes; ++i ) {
            std::cout << i << ": " << (arr.array.get())[i] << std::endl;
          }

          std::cout << "Checking array" << std::endl;
          for( int i = 0; i < total_size; ++i ) {
            MemoryDescriptor md;
            md.type = MemoryDescriptor::Read;
            md.index = i;
            arr.blockingOp( &md );
            uint64_t data = md.data;
            //uint64_t data = arr.blockingGet( i );
            if ( i != data ) {
              std::cout << "Error at " << i << ": expected " << i << ", got " << data << std::endl;
            } else {
              std::cout << "Success at " << i << ": expected " << i << ", got " << data << std::endl;
            }
            assert( i == data );
          }

          std::cout << "All results check out." << std::endl;

          arr.blockingQuit();
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
