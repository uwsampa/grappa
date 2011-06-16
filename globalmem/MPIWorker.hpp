#ifndef __MPI_WORKER_HPP__
#define __MPI_WORKER_HPP__

#include <cassert>

#include <mpi.h>

#include <MemoryDescriptor.hpp>

#ifndef DEBUG
#define DEBUG 0
#endif

#define MPI_WORKER_ASSERTS 0
#define MPI_WORKER_DEBUG DEBUG
#define MPI_WORKER_EXTRA_DEBUG 0

class MPIWorker {
private:
  enum State { Initial = 0, Waiting, Processing, Replying };

  State state;

  MemoryDescriptor receive_descriptor;
  MPI_Status receive_status;
  MPI_Request receive_request;

  MemoryDescriptor send_descriptor;
  MPI_Status send_status;
  MPI_Request send_request;

  uint64_t* base;

  bool active;

  int issueReceive() {
    if (MPI_WORKER_DEBUG) std::cout << "Receive worker posting receive...." << std::endl;
    int result = MPI_Irecv( &receive_descriptor, 
                            sizeof( MemoryDescriptor ),
                            MPI_BYTE,                   // TODO: MPI type instead of byte buffer?
                            MPI_ANY_SOURCE,
                            MemoryDescriptor::RMA_Request, 
                            MPI_COMM_WORLD,    // TODO: maybe change to RMA-specific communicator later?
                            &receive_request );
    if (MPI_WORKER_ASSERTS) assert( result == 0 );
    return result;
  }

  int tryCompleteReceive() {
    if (MPI_WORKER_EXTRA_DEBUG) std::cout << "Receive worker trying to complete receive...." << std::endl;
    int receive_flag = 0;
    int result = MPI_Test( &receive_request,
                           &receive_flag,
                           &receive_status );
    if (MPI_WORKER_ASSERTS) assert( result == 0 );
    if (0 != receive_flag) { // we've received something
    if (MPI_WORKER_DEBUG) std::cout << "Receive worker receiving...." << std::endl;
      send_descriptor = receive_descriptor; // make a copy of what we received
      send_descriptor.node = receive_status.MPI_SOURCE;      // grab source and put in descriptor
      issueReceive();                       // re-arm receive
      uint64_t addr = (send_descriptor.address & 0xffffffffffffLL) + ((uint64_t) base);
      __builtin_prefetch( (void*) addr, 0, 0 ); 
    }

    return receive_flag;
  }

  int trySend() {
    if (MPI_WORKER_DEBUG) std::cout << "Receive worker trying to send...." << std::endl;
    int send_flag = 0;
    int result = MPI_Test( &send_request,
                           &send_flag,
                           &send_status );
    if (MPI_WORKER_ASSERTS) assert( result == 0 );
    if (MPI_WORKER_DEBUG) std::cout << "Receive worker send flag " << send_flag << " send_status " << send_status.MPI_ERROR << "...." << std::endl;
    if (1 == send_flag) {
      if (MPI_WORKER_DEBUG) std::cout << "Receive worker sending...." << std::endl;
      uint64_t addr = (send_descriptor.address & 0xffffffffffffLL) + ((uint64_t) base);
      if (MPI_WORKER_DEBUG) std::cout << "Receive worker accessing address " << (void*) addr << "...." << std::endl;

      switch (send_descriptor.type) {
      case MemoryDescriptor::Write:
        *( (uint64_t*) addr ) = send_descriptor.data;
        if (MPI_WORKER_DEBUG) std::cout << "Receive worker stored data " << send_descriptor.data << "...." << std::endl;
        break;
      case MemoryDescriptor::Read:
        send_descriptor.data = *( (uint64_t*) addr );
        if (MPI_WORKER_DEBUG) std::cout << "Receive worker got data " << (void*) send_descriptor.data << "...." << std::endl;
        break;
      case MemoryDescriptor::Quit:
        if (MPI_WORKER_DEBUG) std::cout << "Receive worker got quit request...." << std::endl;
        break;
      default:
        assert(false);
      }

      if (MPI_WORKER_DEBUG) std::cout << "Receive worker sending...." << std::endl;
      result = MPI_Isend( &send_descriptor,
                          sizeof(MemoryDescriptor),
                          MPI_BYTE,
                          send_descriptor.node,
                          MemoryDescriptor::RMA_Response,
                          MPI_COMM_WORLD,
                          &send_request );
      if (MPI_WORKER_ASSERTS) assert( result == 0 );
    }

    if (send_descriptor.type == MemoryDescriptor::Quit) {
      active = false;
    }
    
    return send_flag;
  }


public:
  MPIWorker(uint64_t* base) 
    : state(Initial)
    , receive_descriptor()
    , receive_status()
    , receive_request(MPI_REQUEST_NULL)
    , send_descriptor()
    , send_status()
    , send_request(MPI_REQUEST_NULL)
    , base(base)
    , active(true)
  { 
    if (MPI_WORKER_DEBUG) std::cout << "Receive worker initializing...." << std::endl;
    issueReceive();
    state = Waiting;
  }
  
  bool tick() {
    if (MPI_WORKER_EXTRA_DEBUG) std::cout << "Receive worker in state " << state << "...." << std::endl;
    switch (state) {
    case Initial:
      issueReceive();
      state = Waiting;
      break;
    case Waiting:
      if (tryCompleteReceive()) {
        state = Processing;
      }
      break;
    case Processing:
      if (trySend()) {
        state = Waiting;
      }
      break;
    default:
      assert(false);
    }
    return active;
  }

};


#endif
