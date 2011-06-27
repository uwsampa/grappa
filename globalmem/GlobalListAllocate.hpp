
#ifndef __GLOBALLISTALLOCATE_HPP__
#define __GLOBALLISTALLOCATE_HPP__

#include <vector>

#include "MemoryDescriptor.hpp"
#include "ListNode.hpp"

#ifndef DEBUG
#define DEBUG 0
#endif

// initialize linked lists. (serial)
//node_t** allocate_heap(uint64_t size, int num_threads, int num_lists) {
template < typename ListNodeGlobalArray, typename BasesGlobalArray > 
void globalListAllocate(uint64_t size, int num_threads, int num_lists_per_thread, ListNodeGlobalArray* array, BasesGlobalArray* bases) {
  std::cout << "Initializing footprint of size " << size << " * " << sizeof(ListNode) << " = " << size * sizeof(ListNode) << " bytes...." << std::endl;

  MemoryDescriptor md;
  std::vector< typename ListNodeGlobalArray::Cell * > locs( size );

  for( uint64_t i = 0; i < size; ++i ) {

    // set node ID
    md.type = MemoryDescriptor::Write;
    md.address = reinterpret_cast<uint64_t>( array->getGlobalAddressForIndex( i ) );
    md.data = i;
    array->blockingOp( &md );

    // // now set node next pointer (to id, for now)
    // md.address += offsetof(ListNode, next);
    // array->blockingOp( &md );
  }

  for( uint64_t i = size-1; i != 0; --i) {
    uint64_t j = random() % i;
    
    // node_t temp = nodes[i];
    md.type = MemoryDescriptor::Read;
    md.address = reinterpret_cast<uint64_t>( array->getGlobalAddressForIndex( i ) );
    array->blockingOp( &md );
    uint64_t temp_id = md.data;


    // nodes[i] = nodes[j];
    md.type = MemoryDescriptor::Read;
    md.address = reinterpret_cast<uint64_t>( array->getGlobalAddressForIndex( j ) );
    array->blockingOp( &md );
    md.type = MemoryDescriptor::Write;
    md.address = reinterpret_cast<uint64_t>( array->getGlobalAddressForIndex( i ) );
    array->blockingOp( &md );

//     nodes[j] = temp;
    md.type = MemoryDescriptor::Write;
    md.address = reinterpret_cast<uint64_t>( array->getGlobalAddressForIndex( j ) );
    md.data = temp_id;
    array->blockingOp( &md );
  }


  for( uint64_t i = 0; i < size; ++i ) {
    md.type = MemoryDescriptor::Read;
    md.address = reinterpret_cast<uint64_t>( array->getGlobalAddressForIndex( i ) );
    array->blockingOp( &md );
    uint64_t ith_id = md.data;
    locs[ ith_id ] = reinterpret_cast<typename ListNodeGlobalArray::Cell *>(md.address);
  }

  for( uint64_t i = 0; i < size; ++i) {
    md.type = MemoryDescriptor::Read;
    md.address = reinterpret_cast<uint64_t>( array->getGlobalAddressForIndex( i ) );
    array->blockingOp( &md );
    uint64_t id = i;
    //typename ListNodeGlobalArray::Cell * current = locs[i];
    int64_t thread_size = size / num_lists_per_thread;
    int64_t base = id / thread_size;
    uint64_t nextid = id + 1;
    uint64_t wrapped_nextid = (nextid % thread_size) + (base * thread_size) % size;

    md.type = MemoryDescriptor::Write;
    md.address = reinterpret_cast<uint64_t>( locs[ i ] ) + offsetof( ListNode, next );
    md.data = reinterpret_cast<uint64_t>( locs[ wrapped_nextid ] );
    array->blockingOp( &md );
  }

   // grab initial pointers for each list
  for( uint64_t i = 0; i < static_cast<uint64_t>(num_threads * num_lists_per_thread); ++i) {
    md.type = MemoryDescriptor::Write;
    md.data = reinterpret_cast<uint64_t>( locs[i * size / (num_threads * num_lists_per_thread)] );
    md.address = reinterpret_cast<uint64_t>( bases->getGlobalAddressForIndex( i ) );
    bases->blockingOp( &md );
  }

  if (DEBUG) {
    for( uint64_t i = 0; i < size; ++i) {
      std::cout << i << ": " << locs[i] << std::endl;
    }
  }

//   //print(nodes, size);
  
//   free(locs);
//   return bases;
}

#endif
