#pragma once 

// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "Message.hpp"
#include "ReuseList.hpp"
#include "ReuseMessage.hpp"
#include "Metrics.hpp"

GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, reuse_message_list_ops, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, reuse_message_list_blocked_ops, 0 );



namespace Grappa {

/// @addtogroup Communication
/// @{

/// Reuse list of messages
template< typename T >
class ReuseMessageList : public impl::ReuseList< ReuseMessage< T > > {
private:
  ReuseMessage< T > * messages_;
  size_t outstanding_;

public:
  ReuseMessageList( size_t outstanding ) 
    : impl::ReuseList< ReuseMessage< T > >()
    , messages_( NULL )
    , outstanding_( outstanding )
  { }

  // nothing
  void init() {}

  // initialize list in locale shared memory
  void activate() {
    void * p = Grappa::impl::locale_shared_memory.allocate( sizeof(ReuseMessage<T>) * outstanding_ );
    messages_ = new (p) ReuseMessage<T>[ outstanding_ ];
    for( int i = 0; i < outstanding_; ++i ) {
      messages_[i].list_ = this;
      this->push( &messages_[i] );
    }
  }

  void finish() {
    while( !(this->empty()) ) {
      this->block_until_pop();
    }
    Grappa::impl::locale_shared_memory.deallocate( messages_ );
  }

  template< typename F >
  void with_message( F f ) {
    reuse_message_list_ops++;
    if( this->count() == 0 ) { reuse_message_list_blocked_ops++; }

    ReuseMessage< T > * m = this->block_until_pop();

    m->reset();

    f(m);
  }
  
};

/// @}

}
