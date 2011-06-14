
#include <iostream>

#include <cstdint>

#include <mpi.h>

#include <MPIReceiveWorker.hpp>

int main( int argc, char* argv[] ) {
  
  MPI_Init( &argc, &argv );

  {
    uint64_t data[1024] = {0};
    data[3] = 0x12345;

    MPIReceiveWorker mrw( (uint64_t) data );
    mrw.tick();
    mrw.tick();

    MemoryDescriptor md1, md2;
    md1.address = (&data[3] - &data[0]) * sizeof(uint64_t);

    std::cout << "Ready. Posting request for address " << (void*) &data[3] << "." << std::endl;
    MPI_Request req1, req2;
    MPI_Isend( &md1, 
               sizeof( MemoryDescriptor ),
               MPI_BYTE,
               0,
               1,
               MPI_COMM_WORLD,
               &req1 );

    std::cout << "Tick" << std::endl;
    mrw.tick();
    std::cout << "Tick" << std::endl;
    mrw.tick();
    std::cout << "Tick" << std::endl;
    mrw.tick();
    std::cout << "Tick" << std::endl;
    mrw.tick();
    std::cout << "Tick" << std::endl;
    mrw.tick();

    MPI_Irecv( &md2,
               sizeof( MemoryDescriptor ),
               MPI_BYTE,
               MPI_ANY_SOURCE,
               2,
               MPI_COMM_WORLD,
               &req2 );

    std::cout << "Checking results" << std::endl;
    MPI_Status stat1, stat2;
    int flag1, flag2;

    //MPI_Wait( &req2, &stat2 );

    MPI_Test( &req1, &flag1, &stat1 );
    MPI_Test( &req2, &flag2, &stat2 );

    assert( flag1 );
    assert( flag2 );
    assert( md2.data == 0x12345 );

    std::cout << "Validated!" << std::endl;
  }

  MPI_Finalize();

  return 0;
}
