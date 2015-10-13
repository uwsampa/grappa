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
#include "Metrics.hpp"




namespace Grappa {

/// @addtogroup Communication
/// @{

/// message to be used in message pools. adds itself to a list after being sent
template< typename T >
class ReuseMessage : public Grappa::Message<T> {
public:
  Grappa::impl::ReuseList< ReuseMessage > * list_;
  virtual ReuseMessage * get_next() { 
    // we know this is safe since the list only holds this message type.
    return static_cast<ReuseMessage*>(this->next_); 
  }
  virtual void set_next( ReuseMessage * next ) { this->next_ = next; }
protected:
  virtual void mark_sent() {
    DVLOG(5) << __func__ << ": " << this << " Marking sent with is_enqueued_=" << this->is_enqueued_ 
             << " is_delivered_=" << this->is_delivered_ 
             << " is_sent_=" << this->is_sent_;

    Grappa::Message<T>::mark_sent();

    if( Grappa::mycore() == this->source_ ) {
      CHECK_EQ( Grappa::mycore(), this->source_ );
      DVLOG(5) << __func__ << ": " << this << " Pushing onto list";
      list_->push(this);
    }
  }
} __attribute__((aligned(64)));

/// @}

}
