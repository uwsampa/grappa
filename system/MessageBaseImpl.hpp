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
