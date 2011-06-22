
#ifndef __GLOBALARRAY_HPP__
#define __GLOBALARRAY_HPP__

#include <cstdint>

#include <memory>
#include <vector>
#include <stack>
#include <utility>

#include <MemoryDescriptor.hpp>

#ifndef DEBUG
#define DEBUG 0
#endif


template < typename MemoryDescriptor,
           typename Communicator >
class GlobalArray {
public:
  typedef uint64_t Index;
  typedef uint64_t Data;

private:
  Index total_size;
  int my_rank;
  int total_num_nodes;
  Index local_size;
  Index local_offset;
  Communicator * communicator;

public:
  std::auto_ptr< Data > array;
  typedef typename Communicator::InFlightRequest InFlightRequest;

public:

  bool isLocalIndex( Index index ) {
    return (local_offset <= index) && (index < local_offset + local_size);
  }

  GlobalArray ( Index total_size,
                int my_rank,
                int total_num_nodes,
                Communicator * communicator)
    : total_size(total_size)
    , my_rank(my_rank)
    , total_num_nodes(total_num_nodes)
    , local_size( total_size / total_num_nodes )
    , local_offset( my_rank * local_size )
    , communicator( communicator )
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

      communicator->blocking( descriptor, descriptor );
    }
  }

  InFlightRequest issueOp( MemoryDescriptor* descriptor ) {
    uint64_t index = descriptor->index;
    if ( isLocalIndex( index ) ) {
      Data* a = array.get();
      __builtin_prefetch( &a[ index - local_offset ], 0, 0 );
      return NULL;
    } else {
      descriptor->address = index % local_size * sizeof(Data);
      descriptor->node = index / local_size;
      std::cout << "Issuing communication for " << descriptor << std::endl;
      InFlightRequest request = communicator->issue( descriptor, descriptor );
      return request;
    }
  }

  bool testOp( MemoryDescriptor descriptor, InFlightRequest requests ) {
    return communicator->test( requests );
  }

  Data completeOp( MemoryDescriptor* descriptor, InFlightRequest requests ) {
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
      communicator->complete( requests );
    }
  }

  void blockingQuit() {
    MemoryDescriptor request_descriptor, response_descriptor;
    
    request_descriptor.type = MemoryDescriptor::Quit;
    request_descriptor.address = 0;
    
    for (int i = 0; i < total_num_nodes; ++i) {
      request_descriptor.node = i;
      communicator->blocking( &request_descriptor, &response_descriptor );
    }
  }

  Data* getBase() { return array.get(); }

};

#endif
