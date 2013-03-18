#pragma once 

// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "Message.hpp"
#include "ReuseList.hpp"
#include "Statistics.hpp"




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

    if( Grappa::mycore() == this->source_ ) {
      CHECK_EQ( Grappa::mycore(), this->source_ );
      list_->push(this);
    }

    Grappa::Message<T>::mark_sent();
  }
};

/// @}

}
