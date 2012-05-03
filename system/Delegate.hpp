
#ifndef __DELEGATE_HPP__
#define __DELEGATE_HPP__

#include "SoftXMT.hpp"

void SoftXMT_delegate_write_word( GlobalAddress<int64_t> address, int64_t data );

int64_t SoftXMT_delegate_read_word( GlobalAddress<int64_t> address );

int64_t SoftXMT_delegate_fetch_and_add_word( GlobalAddress<int64_t> address, int64_t data );

bool SoftXMT_delegate_compare_and_swap_word(GlobalAddress<int64_t> address, int64_t cmpval, int64_t newval);

template< typename T >
struct memory_desc;

template< typename T >
struct read_reply_args {
  GlobalAddress< memory_desc<T> > descriptor;
};

template< typename T >
static void read_reply_am( read_reply_args<T> * args, size_t size, void * payload, size_t payload_size ) {
  DCHECK( payload_size == sizeof(T) );
  *(args->descriptor.pointer()->data) = *(static_cast<T*>(payload));
  args->descriptor.pointer()->done = true;
  if( args->descriptor.pointer()->t != NULL ) {
    SoftXMT_wake( args->descriptor.pointer()->t );
  }
}

template< typename T >
struct memory_desc {
  Thread * t;
  GlobalAddress<T> address;
  T* data;
  bool done;
};

template< typename T >
struct read_request_args {
  GlobalAddress< memory_desc<T> > descriptor;
  GlobalAddress<T> address;
};

template< typename T >
static void read_request_am( read_request_args<T> * args, size_t size, void * payload, size_t payload_size ) {
  T data = *(args->address.pointer());
  read_reply_args<T> reply_args;
  reply_args.descriptor = args->descriptor;
  if( args->descriptor.node() == SoftXMT_mynode() ) {
    read_reply_am( &reply_args, sizeof( reply_args ), &data, sizeof(data) );
  } else {
    SoftXMT_call_on( args->descriptor.node(), &read_reply_am<T>, 
		     &reply_args, sizeof(reply_args), 
		     &data, sizeof(data) );
  }
}

template< typename T >
void SoftXMT_delegate_read( GlobalAddress<T> address, T * buf) {
  memory_desc<T> md;
  md.address = address;
  md.done = false;
  md.data = buf;
  md.t = NULL;
  read_request_args<T> args;
  args.descriptor = make_global(&md);
  args.address = address;
  if( address.node() == SoftXMT_mynode() ) {
    read_request_am( &args, sizeof( args ), NULL, 0 );
  } else {
    SoftXMT_call_on( address.node(), &read_request_am<T>, &args );
  }
  while (!md.done) {
    md.t = CURRENT_THREAD;
    SoftXMT_suspend();
    md.t = NULL;
  }
}

#endif
