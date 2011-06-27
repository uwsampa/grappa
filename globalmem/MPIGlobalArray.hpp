
#ifndef __MPIGLOBALARRAY_HPP__
#define __MPIGLOBALARRAY_HPP__

#include <cstdint>
#include <mpi.h>

#include <memory>
#include <vector>
#include <stack>
#include <utility>

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

  MPI_Comm request_communicator;
  MPI_Comm response_communicator;

  Index local_size;
  Index local_offset;

public:
  std::auto_ptr< Data > array;

  int current_tag;
  const int max_tag;

  typedef std::pair< MPI_Request, MPI_Request > MPI_Requests;
  std::vector< MPI_Requests > request_vector;
  std::stack< MPI_Requests* > request_pointers;

  

  void blocking_communication( MemoryDescriptor* request_descriptor, MemoryDescriptor* response_descriptor ) {
    MPI_Request request_request;
    int request_result = MPI_Isend( request_descriptor,
                                    sizeof(MemoryDescriptor),
                                    MPI_BYTE,
                                    request_descriptor->node,
                                    0,
                                    request_communicator,
                                    &request_request );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( request_result == 0 );

    MPI_Request response_request;
    int result = MPI_Irecv( response_descriptor, 
                            sizeof( MemoryDescriptor ),
                            MPI_BYTE,                   // TODO: MPI type instead of byte buffer?
                            MPI_ANY_SOURCE,
                            0,
                            response_communicator,
                            &response_request );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( request_result == 0 );

    MPI_Status request_status;
    MPI_Wait( &request_request, &request_status );

    MPI_Status response_status;
    MPI_Wait( &response_request, &response_status );
  }

  MPI_Requests* issue_communication( MemoryDescriptor* request_descriptor, MemoryDescriptor* response_descriptor ) {
    std::cout << "grabbing next request pair" << std::endl;
    std::cout << "value is " << request_pointers.top() << std::endl;
    MPI_Requests* request = request_pointers.top();

    std::cout << "popping off stack" << std::endl;
    request_pointers.pop();

    MPI_Request& request_request = request->first;
    MPI_Request& response_request = request->second;

    std::cout << "sending request" << std::endl;
    int request_result = MPI_Isend( request_descriptor,
                                    sizeof(MemoryDescriptor),
                                    MPI_BYTE,
                                    request_descriptor->node,
                                    0,
                                    request_communicator,
                                    &request_request );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( request_result == 0 );

    std::cout << "posting response" << std::endl;
    int result = MPI_Irecv( response_descriptor, 
                            sizeof( MemoryDescriptor ),
                            MPI_BYTE,                   // TODO: MPI type instead of byte buffer?
                            MPI_ANY_SOURCE,
                            0,
                            response_communicator,
                            &response_request );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( request_result == 0 );

    return request;
  }

  bool test_communication( MPI_Requests* request ) {
    MPI_Request& request_request = request->first;
    MPI_Request& response_request = request->second;

    MPI_Status request_status;
    int request_flag = 0;
    MPI_Test( &request_request, &request_flag, &request_status );

    MPI_Status response_status;
    int response_flag = 0;
    MPI_Test( &response_request, &response_flag, &response_status );

    return request_flag && response_flag;
  }

  void complete_communication( MPI_Requests* request ) {
    MPI_Request& request_request = request->first;
    MPI_Request& response_request = request->second;

    MPI_Status request_status;
    MPI_Wait( &request_request, &request_status );

    MPI_Status response_status;
    MPI_Wait( &response_request, &response_status );

    request_pointers.push(request);
  }

public:

  bool isLocalIndex( Index index ) {
    return (local_offset <= index) && (index < local_offset + local_size);
  }

  MPIGlobalArray ( Index total_size,
                   int my_rank,
                   int total_num_nodes,
                   MPI_Comm request_communicator,
                   MPI_Comm response_communicator )
    : total_size(total_size)
    , my_rank(my_rank)
    , request_communicator(request_communicator)
    , response_communicator(response_communicator)
    , total_num_nodes(total_num_nodes)
    , local_size( total_size / total_num_nodes )
    , local_offset( my_rank * local_size )
    , array( new Data [ local_size ] )
    , current_tag( 0 )
    , max_tag( 1 << 10 )
    , request_vector( max_tag )
    , request_pointers()
      //    , request_pointers(request_vector.begin(), request_vector.end())
  { 
    if (DEBUG) {
      std::cout << "Created global array chunk with" 
        << " total_size " << total_size
        << " local_size " << local_size
        << " local_offset " << local_offset
        << std::endl;
    }

    for (int i = 0; i < max_tag; ++i) {
      request_pointers.push( &request_vector[i] );
    }

    assert( ! request_pointers.empty() );
  }

  Data blockingGet( Index index ) {
    if ( isLocalIndex( index ) ) {
      Data* a = array.get();
      return a[ index - local_offset ];
    } else {
      MemoryDescriptor request_descriptor, response_descriptor;
      
      request_descriptor.index = index;
      request_descriptor.type = MemoryDescriptor::Read;
      request_descriptor.address = index % local_size * sizeof(Data);
      request_descriptor.node = index / local_size;

      blocking_communication( &request_descriptor, &response_descriptor );

      return response_descriptor.data;
    }
  }

  void blockingPut( Index index, Data data ) {
    if ( isLocalIndex( index ) ) {
      Data* a = array.get();
      a[ index - local_offset ] = data;
    } else {
      MemoryDescriptor request_descriptor, response_descriptor;
      
      request_descriptor.index = index;
      request_descriptor.type = MemoryDescriptor::Write;
      request_descriptor.address = index % local_size * sizeof(Data);
      request_descriptor.node = index / local_size;
      request_descriptor.data = data;
      
      blocking_communication( &request_descriptor, &response_descriptor );
    }
  }

  void blockingOp( MemoryDescriptor* descriptor ) {
    uint64_t index = descriptor->index;
    if (DEBUG) std::cout << "Blocking op: " << descriptor << std::endl;
    if ( isLocalIndex( index ) ) {
      Data* a = array.get();
      switch (descriptor->type) {
      case MemoryDescriptor::Read: 
        descriptor->data = a[ index - local_offset ];
        break;
      case MemoryDescriptor::Write:
        a[ index - local_offset ] = descriptor->data;
        break;
      default:
        assert(false);
      }
    } else {
      descriptor->address = index % local_size * sizeof(Data);
      descriptor->node = index / local_size;

      blocking_communication( descriptor, descriptor );
    }
  }

   MPI_Requests* issueOp( MemoryDescriptor* descriptor ) {
    uint64_t index = descriptor->index;
    if ( isLocalIndex( index ) ) {
      Data* a = array.get();
      __builtin_prefetch( &a[ index - local_offset ], 0, 0 );
      return NULL;
    } else {
      descriptor->address = index % local_size * sizeof(Data);
      descriptor->node = index / local_size;
      std::cout << "Issuing communication for " << descriptor << std::endl;
      return issue_communication( descriptor, descriptor );
    }
  }

  bool testOp( MemoryDescriptor descriptor, MPI_Requests* requests ) {
    return test_communication( requests );
  }

  Data completeOp( MemoryDescriptor* descriptor, MPI_Requests* requests ) {
    uint64_t index = descriptor->index;
    if ( requests == NULL ) {
      Data* a = array.get();
      switch (descriptor->type) {
      case MemoryDescriptor::Read: 
        descriptor->data = a[ index - local_offset ];
        break;
      case MemoryDescriptor::Write:
        a[ index - local_offset ] = descriptor->data;
        break;
      default:
        assert(false);
      }
    } else {
      complete_communication( requests );
    }
  }

  /*
  void issuePut( Index index, Data data ) {
    if ( isLocal( index ) ) {
      Data* a = array.get();
      a[ index - local_offset ] = data;
    } else {
      MemoryDescriptor request_descriptor, response_descriptor;
      
      request_descriptor.type = MemoryDescriptor::Write;
      request_descriptor.address = index % local_size * sizeof(Data);
      request_descriptor.node = index / local_size;
      request_descriptor.data = data;
      
      blocking_communication( request_descriptor, response_descriptor );
    }
  }

  void completePut( Index index, Data data ) {
    if ( isLocal( index ) ) {
      Data* a = array.get();
      a[ index - local_offset ] = data;
    } else {
      MemoryDescriptor request_descriptor, response_descriptor;
      
      request_descriptor.type = MemoryDescriptor::Write;
      request_descriptor.address = index % local_size * sizeof(Data);
      request_descriptor.node = index / local_size;
      request_descriptor.data = data;
      
      blocking_communication( request_descriptor, response_descriptor );
    }
  }
  */


  void blockingQuit() {
    MemoryDescriptor request_descriptor, response_descriptor;
    
    request_descriptor.type = MemoryDescriptor::Quit;
    request_descriptor.address = 0;
    
    for (int i = 0; i < total_num_nodes; ++i) {
      request_descriptor.node = i;
      blocking_communication( &request_descriptor, &response_descriptor );
    }
  }

  Data* getBase() { return array.get(); }

};

#endif
