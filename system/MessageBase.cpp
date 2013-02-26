
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
      DVLOG(5) << this << " on " << global_scheduler.get_current_thread()
               << " maybe blocking until sent";
      // if enqueued, block until message is sent
      while( is_enqueued_ && !is_sent_ ) {
        DVLOG(5) << this << " on " << global_scheduler.get_current_thread()
                 << " actually blocking until sent";
        Grappa::wait( &cv_ );
        DVLOG(5) << this << " on " << global_scheduler.get_current_thread()
                 << " woken after blocking until sent";
      }
      DVLOG(5) << this << " on " << global_scheduler.get_current_thread()
               << " continuing after blocking until sent";
    }

    /// @}

  }
}

