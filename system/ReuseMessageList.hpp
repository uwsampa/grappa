////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////
#pragma once 

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
