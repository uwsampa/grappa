
#include "MessageBase.hpp"
#include "MessageBaseImpl.hpp"

namespace Grappa {

  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{

    void legacy_send_message_am( char * buf, size_t size, void * payload, size_t payload_size ) {
      Grappa::impl::MessageBase::deserialize_and_call( static_cast< char * >( buf ) );
    }

    /// Block until message can be deallocated.
    void Grappa::impl::MessageBase::block_until_sent() {
      // if message has not been enqueued to be sent, do so.
      // if it has already been sent somehow, then don't worry about it.
      if( !is_sent_ && !is_enqueued_ ) {
        DVLOG(5) << this << " not sent, so enqueuing";
        enqueue();
      }
      // now block until message is sent
      while( !is_sent_ ) {
        DVLOG(5) << this << " blocking until sent";
        Grappa::wait( &cv_ );
        DVLOG(5) << this << " woken";
      }
    }

    /// @}

  }
}

