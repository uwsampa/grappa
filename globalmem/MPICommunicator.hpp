
#ifndef __MPICOMMUNICATOR_HPP__
#define __MPICOMMUNICATOR_HPP__

#include <cstdint>
#include <mpi.h>

#include <memory>
#include <vector>
#include <stack>
#include <utility>

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef MPI_GLOBAL_ARRAY_ASSERTS
#define MPI_GLOBAL_ARRAY_ASSERTS 1
#endif

template < typename MemoryDescriptor >
class MPICommunicator {
private:
  int my_rank;
  int total_num_nodes;
  MPI_Comm request_communicator;
  MPI_Comm response_communicator;

  typedef std::pair< MPI_Request, MPI_Request > MPI_Requests; // hold request and response MPI_Request types
  typedef std::pair< int, MPI_Requests > Request;             // tag for messages
  std::vector< Request > request_vector;
  std::stack< Request* > request_pointers;

  const int max_outstanding_requests_log;
  const int max_outstanding_requests;

public:

  // used for non-blocking communications
  typedef Request * InFlightRequest;

  MPICommunicator( int my_rank, int total_num_nodes,
                   MPI_Comm request_communicator, MPI_Comm response_communicator ) 
    : my_rank( my_rank )
    , request_communicator(request_communicator)
    , response_communicator(response_communicator)
    , total_num_nodes( total_num_nodes )
    , max_outstanding_requests_log( 5 )
    , max_outstanding_requests( 1 << max_outstanding_requests_log )
    , request_vector( max_outstanding_requests )
    , request_pointers( )
  {
    // fill bag of requests
    for (int i = 0; i < max_outstanding_requests; ++i) {
      request_vector[i].first = i;
      request_pointers.push( &request_vector[i] );
    }

    // after filling, make sure bag has stuff in it.
    assert( ! request_pointers.empty() );
  }


  void blocking( MemoryDescriptor* request_descriptor, MemoryDescriptor* response_descriptor ) {
    assert( ! request_pointers.empty() );

    if (DEBUG) std::cout << "grabbing next request pair" << std::endl;
    if (DEBUG) std::cout << "value is " << request_pointers.top() << std::endl;
    Request* request = request_pointers.top();

    if (DEBUG) std::cout << "popping off stack" << std::endl;
    request_pointers.pop();

    MPI_Request& request_request = request->second.first;
    MPI_Request& response_request = request->second.second;

    int request_tag = request->first;
    int response_tag = request->first;

    int request_result = MPI_Isend( request_descriptor,
                                    sizeof(MemoryDescriptor),
                                    MPI_BYTE,
                                    request_descriptor->node,
                                    request_tag,
                                    request_communicator,
                                    &request_request );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( request_result == MPI_SUCCESS );

    int response_result = MPI_Irecv( response_descriptor, 
                                     sizeof( MemoryDescriptor ),
                                     MPI_BYTE,                   // TODO: MPI type instead of byte buffer?
                                     MPI_ANY_SOURCE,
                                     response_tag,
                                     response_communicator,
                                     &response_request );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( response_result == MPI_SUCCESS );

    MPI_Status request_status, response_status;

    request_result = MPI_Wait( &request_request, &request_status );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( request_result == MPI_SUCCESS );

    response_result = MPI_Wait( &response_request, &response_status );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( response_result == MPI_SUCCESS );

    request_pointers.push(request);
  }

  InFlightRequest issue( MemoryDescriptor* request_descriptor, MemoryDescriptor* response_descriptor ) {
    assert( ! request_pointers.empty() );

    if (DEBUG) std::cout << "grabbing next request pair" << std::endl;
    if (DEBUG) std::cout << "value is " << request_pointers.top() << std::endl;
    Request* request = request_pointers.top();

    if (DEBUG) std::cout << "popping off stack" << std::endl;
    request_pointers.pop();

    MPI_Request& request_request = request->second.first;
    MPI_Request& response_request = request->second.second;

    int request_tag = request->first;
    int response_tag = request->first;

    if (DEBUG) std::cout << "sending request" << std::endl;
    int request_result = MPI_Isend( request_descriptor,
                                    sizeof(MemoryDescriptor),
                                    MPI_BYTE,
                                    request_descriptor->node,
                                    request_tag,
                                    request_communicator,
                                    &request_request );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( request_result == MPI_SUCCESS );

    if (DEBUG) std::cout << "posting response" << std::endl;
    int result = MPI_Irecv( response_descriptor, 
                            sizeof( MemoryDescriptor ),
                            MPI_BYTE,                   // TODO: MPI type instead of byte buffer?
                            MPI_ANY_SOURCE,
                            response_tag,
                            response_communicator,
                            &response_request );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( request_result == MPI_SUCCESS );

    return request;
  }

  bool test( InFlightRequest request ) {
    MPI_Request& request_request = request->second.first;
    MPI_Request& response_request = request->second.second;

    MPI_Status request_status;
    int request_flag = 0;
    int request_result = MPI_Test( &request_request, &request_flag, &request_status );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( request_result == MPI_SUCCESS );

    MPI_Status response_status;
    int response_flag = 0;
    int response_result = MPI_Test( &response_request, &response_flag, &response_status );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( response_result == MPI_SUCCESS );

    return request_flag && response_flag;
  }

  void complete( InFlightRequest request ) {
    MPI_Request& request_request = request->second.first;
    MPI_Request& response_request = request->second.second;

    MPI_Status request_status;
    int request_result = MPI_Wait( &request_request, &request_status );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( request_result == MPI_SUCCESS );

    MPI_Status response_status;
    int response_result = MPI_Wait( &response_request, &response_status );
    if (MPI_GLOBAL_ARRAY_ASSERTS) assert( response_result == MPI_SUCCESS );

    request_pointers.push( request );
  }



};


#endif
