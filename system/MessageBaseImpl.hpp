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

#ifndef __MESSAGE_BASE_IMPL_HPP__
#define __MESSAGE_BASE_IMPL_HPP__

#include <glog/logging.h>
#include "common.hpp"

#include "MessageBase.hpp"

#include "RDMAAggregator.hpp"

namespace Grappa {
  
  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{

    inline void Grappa::impl::MessageBase::enqueue() {
      CHECK( !is_moved_ ) << "Shouldn't be sending a message that has been moved!"
                          << " Your compiler's return value optimization failed you here.";
      DCHECK_NULL( next_ );
      
      Grappa::impl::locale_shared_memory.validate_address( this );

      if( !is_enqueued_ ) source_ = global_communicator.mycore;
      //bool mine = source_ == Grappa::mycore();
      bool mine = !is_delivered_;
      DCHECK_EQ( mine && is_enqueued_, false ) << "Why are we enqueuing a message that's already enqueued?";
      DCHECK_EQ( mine && is_sent_, false ) << "Why are we enqueuing a message that's already sent?";

      is_enqueued_ = true;
      DVLOG(5) << this << " on " << Grappa::impl::global_scheduler.get_current_thread() << ": " << this->typestr()
               << " enqueuing to " << destination_ << " with is_enqueued_=" << is_enqueued_ << " and is_sent_= " << is_sent_;
      Grappa::impl::global_rdma_aggregator.enqueue( this, false );
    }

    inline void Grappa::impl::MessageBase::enqueue( Core c ) {
      this->destination_ = c;
      this->enqueue();
    }
    
    inline void Grappa::impl::MessageBase::locale_enqueue() {
      CHECK( !is_moved_ ) << "Shouldn't be sending a message that has been moved!"
                          << " Your compiler's return value optimization failed you here.";
      DCHECK_NULL( next_ );

      Grappa::impl::locale_shared_memory.validate_address( this );

      if( !is_enqueued_ ) source_ = global_communicator.mycore;
      //bool mine = source_ == Grappa::mycore();
      bool mine = !is_delivered_;
      DCHECK_EQ( mine && is_enqueued_, false ) << "Why are we enqueuing a message that's already enqueued?";
      DCHECK_EQ( mine && is_sent_, false ) << "Why are we enqueuing a message that's already sent?";

      is_enqueued_ = true;
      DVLOG(5) << this << " on " << Grappa::impl::global_scheduler.get_current_thread() << ": " << this->typestr()
               << " enqueuing to " << destination_ << " with is_enqueued_=" << is_enqueued_ << " and is_sent_= " << is_sent_;
      Grappa::impl::global_rdma_aggregator.enqueue( this, true );
    }

    inline void Grappa::impl::MessageBase::locale_enqueue( Core c ) {
      destination_ = c;
      locale_enqueue();
    }
    
    inline void Grappa::impl::MessageBase::send_immediate() {
      CHECK( !is_moved_ ) << "Shouldn't be sending a message that has been moved!"
                          << " Your compiler's return value optimization failed you here.";
      if( !is_enqueued_ ) source_ = global_communicator.mycore;
      is_enqueued_ = true;
      is_delivered_ = true;
      Grappa::impl::global_rdma_aggregator.send_immediate( this );
    }

    inline void Grappa::impl::MessageBase::send_immediate( Core c ) {
      destination_ = c;
      send_immediate();
    }

    /// @}
  }
}

#endif
