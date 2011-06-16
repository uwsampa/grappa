
#include <iostream>
#include <cassert>
#include <cstdint>

#include <mpi.h>

#define DEBUG 0

#include <MPIGlobalArray.hpp>
#include <MPIWorker.hpp>


int main( int argc, char* argv[] ) {
  MPI_Init( &argc, &argv );

  int my_rank = -1;
  int total_num_nodes = -1;

  MPI_Comm_rank( MPI_COMM_WORLD, &my_rank );
  MPI_Comm_size( MPI_COMM_WORLD, &total_num_nodes );

  std::cout << "Process " << my_rank+1 << " of " << total_num_nodes << std::endl;

  {
    const int total_size = 128;
    MPIGlobalArray arr( total_size, my_rank, total_num_nodes );
    MPIWorker worker( arr.getBase() );

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
            MPIGlobalArray::MPI_Requests* requests = arr.issueOp( &md );

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
