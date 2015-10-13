////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
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
