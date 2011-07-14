
#define DEBUG 1

#include <iostream>

#include <cstdint>

#include <mpi.h>

#include "MPIWorker.hpp"


int main( int argc, char* argv[] ) {
  
  MPI_Init( &argc, &argv );

  {
    uint64_t data[1024] = {0};
    data[3] = 0x12345;
    data[7] = 0x54321;

    MPI_Comm request_communicator, response_communicator;
    MPI_Comm_dup( MPI_COMM_WORLD, &request_communicator );
    MPI_Comm_dup( MPI_COMM_WORLD, &response_communicator );
    MPIWorker<> mrw( request_communicator, response_communicator );
    int array_id = mrw.register_array( data );
    assert( array_id == 0 );
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
               0,
               request_communicator,
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
               0,
               response_communicator,
               &req2 );

    std::cout << "Checking results" << std::endl;
    MPI_Status stat1, stat2;
    int flag1, flag2;

    MPI_Wait( &req1, &stat1 );
    std::cout << "Message sent" << std::endl;

    MPI_Wait( &req2, &stat2 );
    std::cout << "Message received" << std::endl;

    MPI_Test( &req1, &flag1, &stat1 );
    MPI_Test( &req2, &flag2, &stat2 );

    assert( flag1 );
    assert( flag2 );
    assert( md2.data == 0x12345 );

    std::cout << "Validated 1!" << std::endl;


    md1.address = ((char*)&data[7]) - ((char*)&data[0]);

    std::cout << "Ready. Posting request for address " << (void*) &data[7] << "." << std::endl;
    //    MPI_Request req1, req2;
    MPI_Isend( &md1, 
               sizeof( MemoryDescriptor ),
               MPI_BYTE,
               0,
               0,
               request_communicator,
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
               0,
               response_communicator,
               &req2 );

    std::cout << "Checking results" << std::endl;
    //    MPI_Status stat1, stat2;
    //    int flag1, flag2;

    //MPI_Wait( &req2, &stat2 );

    MPI_Test( &req1, &flag1, &stat1 );
    MPI_Test( &req2, &flag2, &stat2 );

    assert( flag1 );
    assert( flag2 );
    assert( md2.data == 0x54321 );

    std::cout << "Validated 2!" << std::endl;

    std::cout << "Passed." << std::endl;

  }

  MPI_Finalize();

  return 0;
}
