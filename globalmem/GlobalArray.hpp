
#ifndef __GLOBALARRAY_HPP__
#define __GLOBALARRAY_HPP__

#include <cstdint>

#include <memory>
#include <vector>
#include <stack>
#include <utility>

#include "MemoryDescriptor.hpp"

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
  Index local_uint64_offset;
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

  uint64_t getGlobalOffsetFromGlobalAddress( Cell* ga ) {
    my_intptr_t global_offset = (reinterpret_cast< my_uintptr_t >( ga ) << 16) >> 16;
    return global_offset;
  }

  uint64_t getLocalOffsetFromGlobalAddress( Cell* ga ) {
    uint64_t global_offset = getGlobalOffsetFromGlobalAddress(ga);
    uint64_t local_offset = global_offset % local_size;
    return local_offset;
  }

  uint64_t getNodeFromGlobalOffset( uint64_t global_offset ) {
    uint64_t node_id = global_offset / local_size;
    return node_id;
  }

  uint64_t getNodeFromGlobalAddress( uint64_t ga ) {
    return getNodeFromGlobalAddress( reinterpret_cast< Cell * >( ga ) );
  }

  uint64_t getNodeFromGlobalAddress( Cell* ga ) {
    uint64_t global_offset = getGlobalOffsetFromGlobalAddress(ga);
    return getNodeFromGlobalOffset(global_offset);
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

  bool isLocalGlobalAddress( const uint64_t p ) {
    return isLocalGlobalAddress( reinterpret_cast< Cell * >( p ) );
  }

  bool isLocalGlobalAddress( const Cell * p ) {
    my_intptr_t int64_offset = ((reinterpret_cast<my_intptr_t>(p) << 16) >> 16) >> 3;
    my_uintptr_t uint64_offset = static_cast<my_uintptr_t>(int64_offset);
    return (local_uint64_offset <= uint64_offset) && (uint64_offset < local_uint64_offset + (local_size * sizeof(Data) >> 3) );
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
    , local_uint64_offset( my_rank * local_size * sizeof(Data) / sizeof(uint64_t) )
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

  ~GlobalArray() {
    if (DEBUG) {
      for(unsigned i = 0; i < local_size; ++i) {
	std::cout << "array_id " << array_id 
		  << " index " << local_offset + i 
		  << " address " << getGlobalAddressForIndex(local_offset + i) 
		  << ": " << array.get()[i] 
		  << std::endl;
      }
    }
  }

  void blockingOp( MemoryDescriptor* descriptor ) {
    //uint64_t index = descriptor->index;
    int64_t uint64_offset = descriptor->address << 16 >> 16 >> 3;
    uint64_t* array_as_uint64_ptr = reinterpret_cast< uint64_t* >( array.get() );
    if (DEBUG) std::cout << "Global arrray blocking op: " << *descriptor << std::endl;
    if ( isLocalGlobalAddress( descriptor->address ) ) {
      switch (descriptor->type) {
      case MemoryDescriptor::Read: 
	//descriptor->data = a[ index - local_offset ];
        descriptor->data = *( array_as_uint64_ptr + (uint64_offset - local_uint64_offset) );
        break;
      case MemoryDescriptor::Write:
        //a[ index - local_offset ] = descriptor->data;
	*( array_as_uint64_ptr + (uint64_offset - local_uint64_offset) ) = descriptor->data;
        break;
      default:
        assert(false);
      }
    } else {
      descriptor->node = getNodeFromGlobalAddress( descriptor->address );

      communicator->blocking( descriptor, descriptor );
    }
  }

  InFlightRequest issueOp( MemoryDescriptor* descriptor ) {
    //uint64_t index = descriptor->index;
    int64_t uint64_offset = descriptor->address << 16 >> 16 >> 3;
    uint64_t* array_as_uint64_ptr = reinterpret_cast< uint64_t* >( array.get() );
    if ( isLocalGlobalAddress( descriptor->address ) ) {
      //__builtin_prefetch( &a[ index - local_offset ], 0, 0 );
      __builtin_prefetch( array_as_uint64_ptr + uint64_offset - local_uint64_offset, 0, 0 );
      return NULL;
    } else {
      descriptor->node = getNodeFromGlobalAddress( descriptor->address );
      std::cout << "Issuing communication for " << descriptor << std::endl;
      InFlightRequest request = communicator->issue( descriptor, descriptor );
      return request;
    }
  }

  bool testOp( MemoryDescriptor descriptor, InFlightRequest requests ) {
    return communicator->test( requests );
  }

  Data completeOp( MemoryDescriptor* descriptor, InFlightRequest requests ) {
    //uint64_t index = descriptor->index;
    int64_t uint64_offset = descriptor->address << 16 >> 16 >> 3;
    uint64_t* array_as_uint64_ptr = reinterpret_cast< uint64_t* >( array.get() );
    if ( requests == NULL ) {
      switch (descriptor->type) {
      case MemoryDescriptor::Read: 
        //descriptor->data = a[ index - local_offset ];
        descriptor->data = *( array_as_uint64_ptr + (uint64_offset - local_uint64_offset) );
        break;
      case MemoryDescriptor::Write:
        //a[ index - local_offset ] = descriptor->data;
	*( array_as_uint64_ptr + (uint64_offset - local_uint64_offset) ) = descriptor->data;
        break;
      default:
        assert(false);
      }
    } else {
      communicator->complete( requests );
    }
    return descriptor->data;
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
