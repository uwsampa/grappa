#pragma once
#ifndef __MESSAGE_BASE_IMPL_HPP__
#define __MESSAGE_BASE_IMPL_HPP__

#include <glog/logging.h>
#include "common.hpp"

#include "MessageBase.hpp"

#ifndef ENABLE_RDMA_AGGREGATOR
//#define LEGACY_SEND
#endif
//#define LEGACY_SEND

#include "RDMAAggregator.hpp"

namespace Grappa {
  
  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{

    inline void Grappa::impl::MessageBase::enqueue() {
      CHECK( !is_moved_ ) << "Shouldn't be sending a message that has been moved!"
                          << " Your compiler's return value optimization failed you here.";
      CHECK_NULL( next_ );
      CHECK_EQ( is_enqueued_, false ) << "Why are we enqueuing a message that's already enqueued?";
      CHECK_EQ( is_sent_, false ) << "Why are we enqueuing a message that's already sent?";
      is_enqueued_ = true;
      DVLOG(5) << this << " on " << global_scheduler.get_current_thread()
               << " enqueuing with is_enqueued_=" << is_enqueued_ << " and is_sent_= " << is_sent_;
      if( interesting() ) {
        LOG(INFO) << this << " on " << global_scheduler.get_current_thread()
                  << " enqueued";
      }
#ifndef LEGACY_SEND
      Grappa::impl::global_rdma_aggregator.enqueue( this );
#endif
#ifdef LEGACY_SEND
      legacy_send();
#endif
      // if( (get_pointer(&Grappa::impl::global_rdma_aggregator.cores_[c].messages_) != NULL)
      //     && (get_pointer(&Grappa::impl::global_rdma_aggregator.cores_[c].messages_) != this) ) {
      //CHECK_EQ( get_pointer(&Grappa::impl::global_rdma_aggregator.cores_[c].messages_), this );
      // }
    }

    inline void Grappa::impl::MessageBase::enqueue( Core c ) {
      destination_ = c;
      enqueue();
    }
    
    inline void Grappa::impl::MessageBase::send_immediate() {
      CHECK( !is_moved_ ) << "Shouldn't be sending a message that has been moved!"
                          << " Your compiler's return value optimization failed you here.";
      is_enqueued_ = true;
#ifndef LEGACY_SEND
      Grappa::impl::global_rdma_aggregator.send_immediate( this );
#endif
#ifdef LEGACY_SEND
      legacy_send();
#endif
    }

    inline void Grappa::impl::MessageBase::send_immediate( Core c ) {
      destination_ = c;
      send_immediate();
    }

    /// @}
  }
}

#endif
