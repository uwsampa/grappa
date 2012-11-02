
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "GlobalAllocator.hpp"


/// global GlobalAllocator pointer
GlobalAllocator * global_allocator = NULL;

/// dump 
std::ostream& operator<<( std::ostream& o, const GlobalAllocator& a ) {
  return a.dump( o );
}

/// global allocation communication
/// allocator runs on node 0 for now.
/// TODO: aggregate and/or coalesce requests or something

// struct GlobalAllocator_descriptor {
//   Thread * t;
//   GlobalAddress< void > address;
//   bool done;
// };

// static void GlobalAllocator_wait_on( GlobalAllocator_descriptor * descriptor ) {
//   while( !descriptor->done ) {
//     Grappa_suspend();
//   }
// }

// static void GlobalAllocator_wake( GlobalAllocator_descriptor * descriptor ) {
//   Grappa_wake( descriptor->t );
// }


// /// malloc

// struct GlobalAllocator_malloc_reply_args {
//   GlobalAddress< GlobalAllocator_descriptor > descriptor;
//   GlobalAddress< void > address;
// };

// struct GlobalAllocator_malloc_request_args {
//   GlobalAddress< GlobalAllocator_descriptor > descriptor;
//   size_t size_bytes;
// };

// void GlobalAllocator_malloc_reply_am( GlobalAllocator_malloc_reply_args * args, size_t size, 
//                                       void * payload, size_t payload_size ) {
//   (args->descriptor.pointer())->address = args->address;
//   (args->descriptor.pointer())->done = true;
//   GlobalAllocator_wakeup( args->descriptor->pointer() );
// }

// void GlobalAllocator_malloc_request_am( GlobalAllocator_malloc_request_args * args, size_t size, 
//                                         void * payload, size_t payload_size ) {
//   GlobalAllocator_malloc_reply_args reply_args;
//   reply_args.descriptor = args->descriptor;
//   reply_args.address = global_memory->get_allocator()->malloc( args->size_bytes );
//   Grappa_call_on( args->descriptor.node(), &GlobalAllocator_malloc_reply_am, &reply_args );
// }

/// Allocate size_bytes bytes from global heap.
GlobalAddress< void > Grappa_malloc( size_t size_bytes ) {
  // // ask node 0 to allocate memory
  // malloc_descriptor descriptor;
  // malloc_request_args args;
  // args.descriptor = make_global( &descriptor );
  // args.size_bytes = size_bytes;
  // Grappa_call_on( 0, &malloc_request_am, &args );
  // GlobalAllocator_wait_on( &descriptor );
  // return descriptor.address;
  return GlobalAllocator::remote_malloc( size_bytes );
}


/// free


// struct GlobalAllocator_free_reply_args {
//   GlobalAddress< GlobalAllocator_descriptor > descriptor;
// };

// struct GlobalAllocator_free_request_args {
//   GlobalAddress< GlobalAllocator_descriptor > descriptor;
//   GlobalAddress< void > address;
// };

// void GlobalAllocator_free_reply_am( GlobalAllocator_free_reply_args * args, size_t size, 
//                                       void * payload, size_t payload_size ) {
//   (args->descriptor.pointer())->done = true;
//   GlobalAllocator_wakeup( args->descriptor.pointer() );
// }

// void GlobalAllocator_free_request_am( GlobalAllocator_free_request_args * args, size_t size, 
//                                         void * payload, size_t payload_size ) {
//   GlobalAllocator_free_reply_args reply_args;
//   reply_args.descriptor = args->descriptor;
//   global_allocator->free( args->address );
//   Grappa_call_on( args->descriptor.node(), &GlobalAllocator_free_reply_am, &reply_args );
// }


/// Frees memory in global heap
void Grappa_free( GlobalAddress< void > address ) {
  // // ask node 0 to free memory
  // malloc_descriptor descriptor;
  // malloc_request_args args;
  // args.descriptor = make_global( &descriptor );
  // args.address = address;
  // Grappa_call_on( 0, &GlobalAllocator_free_request_am, &args );
  // GlobalAllocator_wait_on( &descriptor );
  GlobalAllocator::remote_free( address );
}


