
#ifndef __GLOBAL_ALLOCATOR_HPP__
#define __GLOBAL_ALLOCATOR_HPP__

#include <iostream>
#include <glog/logging.h>

#include <boost/scoped_ptr.hpp>


#include "SoftXMT.hpp"
#include "Allocator.hpp"


class GlobalAllocator;
extern GlobalAllocator * global_allocator;

class GlobalAllocator {
private:
  boost::scoped_ptr< Allocator > a_p_;



  //
  // communication
  // 

  struct Descriptor {
    thread * t;
    GlobalAddress< void > address;
    size_t size;
    bool done;
  };

  static void wait_on( Descriptor * d ) {
    while( !d->done ) {
      SoftXMT_suspend();
    }
  }

  static void wake( Descriptor * d ) {
    SoftXMT_wake( d->t );
  }

  // remote malloc
  static void malloc_reply_am( GlobalAddress< Descriptor > * d_p, size_t size, 
                        GlobalAddress< void > * address_p, size_t payload_size ) {
    DVLOG(5) << "got malloc response with descriptor " << d_p->pointer() << " pointer " << address_p->pointer();
    d_p->pointer()->address = *address_p;
    d_p->pointer()->done = true;
    wake( d_p->pointer() );
  }

  static void malloc_request_am( GlobalAddress< Descriptor > * d_p, size_t size, 
                                 size_t * size_p, size_t payload_size ) {
    DVLOG(5) << "got malloc request for descriptor " << d_p->pointer() << " size " << *size_p;
    GlobalAddress< void > a = global_allocator->local_malloc( *size_p );
    DVLOG(5) << "malloc returning pointer " << a.pointer();
    SoftXMT_call_on_x( d_p->node(), &malloc_reply_am, 
                       d_p, size,
                       &a, sizeof( a ) );
  }

  // remote free
  static void free_reply_am( GlobalAddress< Descriptor > * d_p, size_t size, 
                             void * payload, size_t payload_size ) {
    d_p->pointer()->done = true;
    wake( d_p->pointer() );
  }

  static void free_request_am( GlobalAddress< Descriptor > * d_p, size_t size, 
                               GlobalAddress< void > * address_p, size_t payload_size ) {
    DVLOG(5) << "got free request for descriptor " << d_p->pointer();
    global_allocator->local_free( *address_p );
    SoftXMT_call_on_x( d_p->node(), &free_reply_am, d_p );
  }


  // allocate some number of bytes
  GlobalAddress< void > local_malloc( size_t size ) {
    intptr_t address = reinterpret_cast< intptr_t >( a_p_->malloc( size ) );
    GlobalAddress< void > ga = GlobalAddress< void >::Raw( address );
    return ga;
  }

  // // allocate some number of some type
  // template< typename T >
  // GlobalAddress< T > local_malloc( size_t count ) {
  //   intptr_t address = reinterpret_cast< intptr_t >( a->malloc( sizeof( T ) * count ) );
  //   GlobalAddress< T > ga = GlobalAddress< T >::Raw( address );
  //   return ga;
  // }

  // release data at pointer
  void local_free( GlobalAddress< void > address ) {
    void * va = reinterpret_cast< void * >( address.raw_bits() );
    a_p_->free( va );
  }


public:
  GlobalAllocator( GlobalAddress< void > base, size_t size )
    : a_p_( 0 == SoftXMT_mynode()  // node 0 does all allocation for now
            ? new Allocator( base, size )
            : NULL )
  { 
    // TODO: this won't work with pools....
    assert( !global_allocator );
    global_allocator = this;
  }

  //
  // basic operations
  //

  static GlobalAddress< void > remote_malloc( size_t size_bytes ) {
    // ask node 0 to allocate memory
    Descriptor descriptor;
    descriptor.t = current_thread;
    descriptor.done = false;
    GlobalAddress< Descriptor > global_descriptor = make_global( &descriptor );
    SoftXMT_call_on_x( 0, &malloc_request_am, 
                       &global_descriptor, sizeof( global_descriptor ),
                       &size_bytes, sizeof(size_bytes) );
    wait_on( &descriptor );
    return descriptor.address;
  }

  // TODO: should free block?
  static void remote_free( GlobalAddress< void > address ) {
    // ask node 0 to free memory
    Descriptor descriptor;
    descriptor.t = current_thread;
    descriptor.done = false;
    GlobalAddress< Descriptor > global_descriptor = make_global( &descriptor );
    SoftXMT_call_on_x( 0, &free_request_am, 
                       &global_descriptor, sizeof( global_descriptor ),
                       &address, sizeof( address ) );
    wait_on( &descriptor );
  }

  //
  // debugging
  //

  std::ostream& dump( std::ostream& o ) const {
    if( a_p_ ) {
      return o << "{GlobalAllocator: " << a_p_->dump( o ) << "}";
    } else {
      return o << "{delegated GlobalAllocator}";
    }
  }

  size_t total_bytes() const { return a_p_->total_bytes(); }
  size_t total_bytes_in_use() const { return a_p_->total_bytes_in_use(); }

};

std::ostream& operator<<( std::ostream& o, const GlobalAllocator& a );


extern GlobalAllocator * global_allocator;

GlobalAddress< void > SoftXMT_malloc( size_t size_bytes );

void SoftXMT_free( GlobalAddress< void > address );

template< typename T >
GlobalAddress< T > SoftXMT_typed_malloc( size_t count ) {
  return SoftXMT_malloc( sizeof( T ) * count );
}


#endif
