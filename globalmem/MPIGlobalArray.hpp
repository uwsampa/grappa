
#ifndef __MPIGLOBALARRAY_HPP__
#define __MPIGLOBALARRAY_HPP__

#include <cstdint>
#include <mpi.h>

#include <memory>

#include <MemoryDescriptor.hpp>

#ifndef DEBUG
#define DEBUG 0
#endif

#define MPI_GLOBAL_ARRAY_ASSERTS 1


class MPIGlobalArray {
public:
  typedef uint64_t Index;
  typedef uint64_t Data;
private:
  Index total_size;
  int my_rank;
  int total_num_nodes;
  Index local_size;
  Index local_offset;
  std::auto_ptr< Data > array;


  void blocking_communication( MemoryDescriptor& request_descriptor, MemoryDescriptor& response_descriptor ) {
    MPI_Status request_status;
    MPI_Request request_request;
      
    int request_result = MPI_Isend( &request_descriptor,
                                    sizeof(MemoryDescriptor),
                                    MPI_BYTE,
                                    request_descriptor.source_node,
                                    MemoryDescriptor::RMA_Request,
                                    MPI_COMM_WORLD,
                                    &request_request );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( request_result == 0 );

    MPI_Status response_status;
    MPI_Request response_request;

    int result = MPI_Irecv( &response_descriptor, 
                            sizeof( MemoryDescriptor ),
                            MPI_BYTE,                   // TODO: MPI type instead of byte buffer?
                            MPI_ANY_SOURCE,
                            MemoryDescriptor::RMA_Response,
                            MPI_COMM_WORLD,    // TODO: maybe change to RMA-specific communicator later?
                            &response_request );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( request_result == 0 );

    MPI_Wait( &request_request, &request_status );
    MPI_Wait( &response_request, &response_status );
  }

public:

  bool isLocal( Index index ) {
    return (local_offset <= index) && (index < local_offset + local_size);
  }

  MPIGlobalArray ( Index total_size,
                   int my_rank,
                   int total_num_nodes )
    : total_size(total_size)
    , my_rank(my_rank)
    , total_num_nodes(total_num_nodes)
    , local_size( total_size / total_num_nodes )
    , local_offset( my_rank * local_size )
    , array( new Data [ local_size ] )
  { 
    if (DEBUG) {
      std::cout << "Created global array chunk with" 
        << " total_size " << total_size
        << " local_size " << local_size
        << " local_offset " << local_offset
        << std::endl;
    }
  }

  Data blockingGet( Index index ) {
    if ( isLocal( index ) ) {
      Data* a = array.get();
      return a[ index - local_offset ];
    } else {
      MemoryDescriptor request_descriptor, response_descriptor;
      
      request_descriptor.type = MemoryDescriptor::Read;
      request_descriptor.address = index % local_size * sizeof(Data);
      request_descriptor.source_node = index / local_size;

      blocking_communication( request_descriptor, response_descriptor );

      return response_descriptor.data;
    }
  }

  void blockingPut( Index index, Data data ) {
    if ( isLocal( index ) ) {
      Data* a = array.get();
      a[ index - local_offset ] = data;
    } else {
      MemoryDescriptor request_descriptor, response_descriptor;
      
      request_descriptor.type = MemoryDescriptor::Write;
      request_descriptor.address = index % local_size * sizeof(Data);
      request_descriptor.source_node = index / local_size;
      request_descriptor.data = data;
      
      blocking_communication( request_descriptor, response_descriptor );
    }
  }

  void blockingQuit() {
    MemoryDescriptor request_descriptor, response_descriptor;
    
    request_descriptor.type = MemoryDescriptor::Quit;
    request_descriptor.address = 0;
    
    for (int i = 0; i < total_num_nodes; ++i) {
      request_descriptor.source_node = i;
      blocking_communication( request_descriptor, response_descriptor );
    }
  }

  Data* getBase() { return array.get(); }

};

#endif
