
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

typedef uint64_t my_uintptr_t;
typedef int64_t my_intptr_t;

template < typename DataType,
           typename MemoryDescriptor,
           typename Communicator >
class GlobalArray {
public:
  typedef my_uintptr_t Index;
  typedef DataType Data;

  struct Cell; // opaque type for returned pointers

private:
  Index total_size;
  int my_rank;
  int total_num_nodes;
  uint64_t array_id;
  Index local_size;
  Index local_offset;
  Communicator * communicator;

public:
  std::auto_ptr< Data > array;
  typedef typename Communicator::InFlightRequest InFlightRequest;

public:

  // each array has an id, to allow it to be identified 
  void setArrayId( int array_id ) {
    this->array_id = array_id;
  }

  /*
      descriptor->address = index % local_size * sizeof(Data);
      descriptor->node = index / local_size;
  */


  my_uintptr_t getGlobalOffsetForIndex( Index i ) { 
    return i * sizeof(Data); 
  }

  // in this version, global addresses have two components:
  //   array id
  //   byte offset
  Cell * getGlobalAddressForIndex( Index i ) { 
    my_uintptr_t address = (this->array_id << 48) | getGlobalOffsetForIndex( i );
    return reinterpret_cast< Cell * >( address );
  }

  uint64_t getArrayIdFromGlobalAddress( Cell* ga ) {
    my_uintptr_t id = reinterpret_cast< my_uintptr_t >( ga );
    return id >> 48; // zero-extend
  }

  uint64_t getLocalOffsetFromGlobalAddress( Cell* ga ) {
    my_intptr_t global_offset = (reinterpret_cast< my_uintptr_t >( ga ) << 16) >> 16;
  }

  Index getIndexForAddress( const Cell * p ) { return -1; }

  Cell * geLocalForIndex( Index i ) {
    my_uintptr_t local_address = i % local_size * sizeof(Data);
    //uint64_t base = static_cast< uint64_t >( this->array.get() );
    //return static_cast< Cell * >( i * sizeof(Data) -  base );
  }

  int getNodeForIndex( const Index i ) { 
    return i / local_size;
  }

  int getNodeForAddress( const Cell * p ) { return -1; }



  bool isIndexLocal( const Index i ) { return false; }
  bool isAddressLocal( const Index i ) { return false; }
  

  // Data operator[]( int index ) {
  //   return 12345;
    
  // }

  // Data* operator&( const GlobalArray* g ) {
  //   return NULL;
    
  // }

  bool isLocalIndex( Index index ) {
    return (local_offset <= index) && (index < local_offset + local_size);
  }


  // local ops
  Data* getBase() { return array.get(); }
  



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

};

#endif
