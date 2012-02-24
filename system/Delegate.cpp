

#include "Delegate.hpp"

#include <cassert>
#include <gflags/gflags.h>
#include <glog/logging.h>


struct memory_descriptor {
  thread * t;
  GlobalAddress<int64_t> address;
  int64_t data;
  bool done;
};

/////////////////////////////////////////////////////////////////////////////

static inline void Delegate_wait( memory_descriptor * md ) {
  while( !md->done ) {
    SoftXMT_suspend();
  }
}

static inline void Delegate_wakeup( memory_descriptor * md ) {
  SoftXMT_wake( md->t );
}

/////////////////////////////////////////////////////////////////////////////

struct memory_write_reply_args {
  GlobalAddress<memory_descriptor> descriptor;
};

void memory_write_reply_am( memory_write_reply_args * args, size_t size, void * payload, size_t payload_size ) {
  (args->descriptor.pointer())->done = true;
  Delegate_wakeup( args->descriptor.pointer() );
}

struct memory_write_request_args {
  GlobalAddress<memory_descriptor> descriptor;
  GlobalAddress<int64_t> address;
};


void memory_write_request_am( memory_write_request_args * args, size_t size, void * payload, size_t payload_size ) {
  assert( payload_size == sizeof(int64_t) );
  *(args->address.pointer()) = *(static_cast<int64_t*>(payload));
  memory_write_reply_args reply_args;
  reply_args.descriptor = args->descriptor;
  SoftXMT_call_on( args->descriptor.node(), &memory_write_reply_am, &reply_args );
}

void SoftXMT_delegate_write_word( GlobalAddress<int64_t> address, int64_t data ) {
  memory_descriptor md;
  md.address = address;
  md.data = data;
  md.done = false;
  md.t = get_current_thread();
  memory_write_request_args args;
  args.descriptor = make_global(&md);
  args.address = address;
  SoftXMT_call_on( address.node(), &memory_write_request_am, 
                   &args, sizeof(args), 
                   &data, sizeof(data) );
  Delegate_wait( &md );
}

/////////////////////////////////////////////////////////////////////////////

struct memory_read_reply_args {
  GlobalAddress<memory_descriptor> descriptor;
};

void memory_read_reply_am( memory_read_reply_args * args, size_t size, void * payload, size_t payload_size ) {
  assert( payload_size == sizeof(int64_t ) );
  args->descriptor.pointer()->data = *(static_cast<int64_t*>(payload));
  args->descriptor.pointer()->done = true;
  Delegate_wakeup( args->descriptor.pointer() );
}

struct memory_read_request_args {
  GlobalAddress<memory_descriptor> descriptor;
  GlobalAddress<int64_t> address;
};


void memory_read_request_am( memory_read_request_args * args, size_t size, void * payload, size_t payload_size ) {
  int64_t data = *(args->address.pointer());
  memory_read_reply_args reply_args;
  reply_args.descriptor = args->descriptor;
  SoftXMT_call_on( args->descriptor.node(), &memory_read_reply_am, 
                   &reply_args, sizeof(reply_args), 
                   &data, sizeof(data) );
}

int64_t SoftXMT_delegate_read_word( GlobalAddress<int64_t> address ) {
  memory_descriptor md;
  md.address = address;
  md.data = 0;
  md.done = false;
  md.t = get_current_thread();
  memory_read_request_args args;
  args.descriptor = make_global(&md);
  args.address = address;
  SoftXMT_call_on( address.node(), &memory_read_request_am, &args );
  Delegate_wait( &md );
  return md.data;
}


/////////////////////////////////////////////////////////////////////////////


struct memory_fetch_add_reply_args {
  GlobalAddress<memory_descriptor> descriptor;
};

struct memory_fetch_add_request_args {
  GlobalAddress<memory_descriptor> descriptor;
  GlobalAddress<int64_t> address;
};

void memory_fetch_add_reply_am( memory_fetch_add_reply_args * args, size_t size, void * payload, size_t payload_size ) {
  assert( payload_size == sizeof(int64_t) );
  args->descriptor.pointer()->data = *(static_cast<int64_t*>(payload));
  args->descriptor.pointer()->done = true;
  Delegate_wakeup( args->descriptor.pointer() );
}

/// runs on server side to fetch data 
void memory_fetch_add_request_am( memory_fetch_add_request_args * args, size_t size, void * payload, size_t payload_size ) {
  assert( payload_size == sizeof(int64_t) );
  int64_t data = *(args->address.pointer()); // fetch
  *(args->address.pointer()) += *(static_cast<int64_t*>(payload)); // increment
  memory_fetch_add_reply_args reply_args;
  reply_args.descriptor = args->descriptor;

  SoftXMT_call_on( args->descriptor.node(), &memory_fetch_add_reply_am, 
                   &reply_args, sizeof(reply_args), 
                   &data, sizeof(data) );
}

int64_t SoftXMT_delegate_fetch_and_add_word( GlobalAddress<int64_t> address, int64_t data ) {

  // set up descriptor
  memory_descriptor md;
  md.address = address;
  md.data = data;
  md.done = false;
  md.t = get_current_thread();

  // set up args for request
  memory_fetch_add_request_args args;
  args.descriptor = make_global(&md);
  args.address = address;

  // make request
  SoftXMT_call_on( address.node(), &memory_fetch_add_request_am, 
                   &args, sizeof(args), 
                   &data, sizeof(data) );

  // wait for response
  Delegate_wait( &md );
  
  return md.data;
}
