


#include <gflags/gflags.h>


#include "SoftXMT.hpp"

typedef void * Address;

int address_to_node( int64_t * address ) {
  return 1;
}

struct memory_descriptor {
  thread * t;
  Address address;
  int64_t data;
  bool done;
};

/////////////////////////////////////////////////////////////////////////////

static inline void Delegate_wait( memory_descriptor * md ) {
  while( !md->done ) {
    //SoftXMT_poll();
    //SoftXMT_yield();
    //SoftXMT_suspend();
    //assert( get_current_thread()->sched != NULL );
    thread_suspend( get_current_thread() );
  }
}

static inline void Delegate_wakeup( memory_descriptor * md ) {
  thread_wake( md->t );
}

/////////////////////////////////////////////////////////////////////////////

struct memory_write_reply_args {
  memory_descriptor * descriptor;
};

void memory_write_reply_am( memory_write_reply_args * args, size_t size, void * payload, size_t payload_size ) {
  args->descriptor->done = true;
  Delegate_wakeup( args->descriptor );
}

struct memory_write_request_args {
  Node source_node;
  memory_descriptor * source_descriptor;
  Address address;
  int64_t data;
};


void memory_write_request_am( memory_write_request_args * args, size_t size, void * payload, size_t payload_size ) {
  *((int64_t*)args->address) = args->data;
  memory_write_reply_args reply_args;
  reply_args.descriptor = args->source_descriptor;
  SoftXMT_call_on( args->source_node, &memory_write_reply_am, &reply_args );
}

void SoftXMT_delegate_write_word( int64_t * address, int64_t data ) {
  memory_descriptor md;
  md.address = address;
  md.data = data;
  md.done = false;
  md.t = get_current_thread();
  memory_write_request_args args;
  args.source_node = global_communicator->mynode();
  args.source_descriptor = &md;
  args.address = address;
  args.data = data;
  SoftXMT_call_on( address_to_node( address ), &memory_write_request_am, &args );
  Delegate_wait( &md );
}

/////////////////////////////////////////////////////////////////////////////

struct memory_read_reply_args {
  memory_descriptor * descriptor;
  int64_t data;
};

void memory_read_reply_am( memory_read_reply_args * args, size_t size, void * payload, size_t payload_size ) {
  args->descriptor->data = args->data;
  args->descriptor->done = true;
  Delegate_wakeup( args->descriptor );
}

struct memory_read_request_args {
  Node source_node;
  memory_descriptor * source_descriptor;
  Address address;
};


void memory_read_request_am( memory_read_request_args * args, size_t size, void * payload, size_t payload_size ) {
  int64_t data = *((int64_t*)args->address);
  memory_read_reply_args reply_args;
  reply_args.descriptor = args->source_descriptor;
  reply_args.data = data;
 // std::cout << "node1 got request with descr" << reply_args.descriptor << std::endl;
  SoftXMT_call_on( args->source_node, &memory_read_reply_am, &reply_args );
}

int64_t SoftXMT_delegate_read_word( int64_t * address ) {
  memory_descriptor md;
 // std::cout << current_thread->id << " descr address is " << &md << std::endl;
  md.address = address;
  md.data = 0;
  md.done = false;
  md.t = get_current_thread();
  memory_read_request_args args;
  args.source_node = global_communicator->mynode();
  args.source_descriptor = &md;
  args.address = address;
  SoftXMT_call_on( address_to_node( address ), &memory_read_request_am, &args );
  Delegate_wait( &md );
  return md.data;
}


/////////////////////////////////////////////////////////////////////////////


struct memory_fetch_add_reply_args {
  memory_descriptor * descriptor;
  int64_t data;
};

struct memory_fetch_add_request_args {
  Node source_node;
  memory_descriptor * source_descriptor;
  Address address;
  int64_t data;
};

void memory_fetch_add_reply_am( memory_fetch_add_reply_args * args, size_t size, void * payload, size_t payload_size ) {
  args->descriptor->data = args->data;
  args->descriptor->done = true;
  Delegate_wakeup( args->descriptor );
}

/// runs on server side to fetch data 
void memory_fetch_add_request_am( memory_fetch_add_request_args * args, size_t size, void * payload, size_t payload_size ) {
  int data = *((int64_t*)args->address); // fetch
  *((int64_t*)args->address) += args->data; // increment
  memory_fetch_add_reply_args reply_args;
  reply_args.descriptor = args->source_descriptor;
  reply_args.data = data;

  SoftXMT_call_on( args->source_node, &memory_fetch_add_reply_am, &reply_args );
}

int64_t SoftXMT_delegate_fetch_and_add_word( int64_t * address, int64_t data ) {

  // set up descriptor
  memory_descriptor md;
  md.address = address;
  md.data = data;
  md.done = false;
  md.t = get_current_thread();

  // set up args for request
  memory_fetch_add_request_args args;
  args.source_node = global_communicator->mynode();
  args.source_descriptor = &md;
  args.address = address;
  args.data = data;

  // make request
  SoftXMT_call_on( address_to_node( address ), &memory_fetch_add_request_am, &args );

  // wait for response
  Delegate_wait( &md );
  
  return md.data;
}
