
#include "GlobalAllocator.hpp"


// global GlobalAllocator pointer
GlobalAllocator * global_allocator = NULL;

std::ostream& operator<<( std::ostream& o, const GlobalAllocator& a ) {
  return a.dump( o );
}

/// global allocation communication
/// allocator runs on node 0 for now.
/// TODO: aggregate and/or coalesce requests or something

// struct GlobalAllocator_descriptor {
//   thread * t;
//   GlobalAddress< void > address;
//   bool done;
// };

// static void GlobalAllocator_wait_on( GlobalAllocator_descriptor * descriptor ) {
//   while( !descriptor->done ) {
//     SoftXMT_suspend();
//   }
// }

// static void GlobalAllocator_wake( GlobalAllocator_descriptor * descriptor ) {
//   SoftXMT_wake( descriptor->t );
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
//   SoftXMT_call_on( args->descriptor.node(), &GlobalAllocator_malloc_reply_am, &reply_args );
// }


GlobalAddress< void > SoftXMT_malloc( size_t size_bytes ) {
  // // ask node 0 to allocate memory
  // malloc_descriptor descriptor;
  // malloc_request_args args;
  // args.descriptor = make_global( &descriptor );
  // args.size_bytes = size_bytes;
  // SoftXMT_call_on( 0, &malloc_request_am, &args );
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
//   SoftXMT_call_on( args->descriptor.node(), &GlobalAllocator_free_reply_am, &reply_args );
// }


// TODO: should free block?
void SoftXMT_free( GlobalAddress< void > address ) {
  // // ask node 0 to free memory
  // malloc_descriptor descriptor;
  // malloc_request_args args;
  // args.descriptor = make_global( &descriptor );
  // args.address = address;
  // SoftXMT_call_on( 0, &GlobalAllocator_free_request_am, &args );
  // GlobalAllocator_wait_on( &descriptor );
  GlobalAllocator::remote_free( address );
}


