
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __GLOBAL_ALLOCATOR_HPP__
#define __GLOBAL_ALLOCATOR_HPP__

#include <iostream>
#include <glog/logging.h>

#include <boost/scoped_ptr.hpp>


#include "Grappa.hpp"
#include "Allocator.hpp"


class GlobalAllocator;
extern GlobalAllocator * global_allocator;

/// Global memory allocator
class GlobalAllocator {
private:
  boost::scoped_ptr< Allocator > a_p_;



  //
  // communication
  // 

  struct Descriptor {
    Thread * t;
    GlobalAddress< void > address;
    size_t size;
    bool done;
  };

  static void wait_on( Descriptor * d ) {
    while( !d->done ) {
      Grappa_suspend();
    }
  }

  static void wake( Descriptor * d ) {
    Grappa_wake( d->t );
  }

  // Handler for remote malloc reply
  static void malloc_reply_am( GlobalAddress< Descriptor > * d_p, size_t size, 
                        GlobalAddress< void > * address_p, size_t payload_size ) {
    DVLOG(5) << "got malloc response with descriptor " << d_p->pointer() << " pointer " << address_p->pointer();
    d_p->pointer()->address = *address_p;
    d_p->pointer()->done = true;
    wake( d_p->pointer() );
  }

  /// Handler for remote malloc request
  static void malloc_request_am( GlobalAddress< Descriptor > * d_p, size_t size, 
                                 size_t * size_p, size_t payload_size ) {
    DVLOG(5) << "got malloc request for descriptor " << d_p->pointer() << " size " << *size_p;
    GlobalAddress< void > a = global_allocator->local_malloc( *size_p );
    DVLOG(5) << "malloc returning pointer " << a.pointer();
    Grappa_call_on_x( d_p->node(), &malloc_reply_am, 
                       d_p, size,
                       &a, sizeof( a ) );
  }

  // Handler for remote free reply
  static void free_reply_am( GlobalAddress< Descriptor > * d_p, size_t size, 
                             void * payload, size_t payload_size ) {
    d_p->pointer()->done = true;
    wake( d_p->pointer() );
  }

  // Handler for remote free request
  static void free_request_am( GlobalAddress< Descriptor > * d_p, size_t size, 
                               GlobalAddress< void > * address_p, size_t payload_size ) {
    DVLOG(5) << "got free request for descriptor " << d_p->pointer();
    global_allocator->local_free( *address_p );
    Grappa_call_on_x( d_p->node(), &free_reply_am, d_p );
  }


  /// allocate some number of bytes from local heap
  /// (should be called only on node responsible for allocator)
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

  // release data at pointer in local heap
  /// (should be called only on node responsible for allocator)
  void local_free( GlobalAddress< void > address ) {
    void * va = reinterpret_cast< void * >( address.raw_bits() );
    a_p_->free( va );
  }


public:
  /// Construct global allocator. Allocates no storage, just controls
  /// ownership of memory region.
  ///   @param base base address of region to allocate from
  ///   @param size number of bytes available for allocation
  GlobalAllocator( GlobalAddress< void > base, size_t size )
    : a_p_( 0 == Grappa_mynode()  // node 0 does all allocation for now
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

  /// delegate malloc
  static GlobalAddress< void > remote_malloc( size_t size_bytes ) {
    // ask node 0 to allocate memory
    Descriptor descriptor;
    descriptor.t = CURRENT_THREAD;
    descriptor.done = false;
    GlobalAddress< Descriptor > global_descriptor = make_global( &descriptor );
    Grappa_call_on_x( 0, &malloc_request_am, 
                       &global_descriptor, sizeof( global_descriptor ),
                       &size_bytes, sizeof(size_bytes) );
    wait_on( &descriptor );
    return descriptor.address;
  }

  /// delegate free
  /// TODO: should free block?
  static void remote_free( GlobalAddress< void > address ) {
    // ask node 0 to free memory
    Descriptor descriptor;
    descriptor.t = CURRENT_THREAD;
    descriptor.done = false;
    GlobalAddress< Descriptor > global_descriptor = make_global( &descriptor );
    Grappa_call_on_x( 0, &free_request_am, 
                       &global_descriptor, sizeof( global_descriptor ),
                       &address, sizeof( address ) );
    wait_on( &descriptor );
  }

  //
  // debugging
  //

  /// human-readable allocator state
  std::ostream& dump( std::ostream& o ) const {
    if( a_p_ ) {
      return o << "{GlobalAllocator: " << a_p_->dump( o ) << "}";
    } else {
      return o << "{delegated GlobalAllocator}";
    }
  }

  /// Number of bytes available for allocation;
  size_t total_bytes() const { return a_p_->total_bytes(); }
  /// Number of bytes allocated
  size_t total_bytes_in_use() const { return a_p_->total_bytes_in_use(); }

};

std::ostream& operator<<( std::ostream& o, const GlobalAllocator& a );


extern GlobalAllocator * global_allocator;

GlobalAddress< void > Grappa_malloc( size_t size_bytes );

void Grappa_free( GlobalAddress< void > address );

/// Allocate count T's worth of bytes from global heap.
template< typename T >
GlobalAddress< T > Grappa_typed_malloc( size_t count ) {
  return Grappa_malloc( sizeof( T ) * count );
}


#endif
