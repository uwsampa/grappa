
#include "MessageBase.hpp"
#include "MessageBaseImpl.hpp"

#include "ConditionVariable.hpp"

namespace Grappa {

  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{

    void Grappa::impl::MessageBase::legacy_send_message_am( char * buf, size_t size, void * payload, size_t payload_size ) {
      Grappa::impl::MessageBase::deserialize_and_call( static_cast< char * >( buf ) );
    }

    /// Block until message can be deallocated.
    void Grappa::impl::MessageBase::block_until_sent() {
      // if enqueued, block until message is sent
      while( is_enqueued_ && !is_sent_ ) {
        DVLOG(5) << this << " blocking until sent";
        // LOG(INFO) << this << " on " << global_scheduler.get_current_thread()
        //           << " blocking until sent with is_enqueued_=" << is_enqueued_ << " and is_sent_=" << is_sent_
        //           << "; existing: " << (void*) cv_.waiters_;
        //Grappa::wait( &cv_, &mutex_ );
        Grappa::wait( &cv_ );
        // LOG(INFO) << this << " on " << global_scheduler.get_current_thread()
        //           << " woke with is_enqueued_=" << is_enqueued_ << " and is_sent_=" << is_sent_
        //           << "; existing: " << (void*) cv_.waiters_;
        DVLOG(5) << this << " woken";
        if( last_woken_ == global_scheduler.get_current_thread() ) last_woken_ = NULL;
      }
    }

    /// @}

  }
}

