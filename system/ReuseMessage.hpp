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
