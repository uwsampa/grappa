
#include "MessageBase.hpp"

namespace Grappa {

  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{

    void legacy_send_message_am( char * buf, size_t size, void * payload, size_t payload_size ) {
      Grappa::impl::MessageBase::deserialize_and_call( static_cast< char * >( buf ) );
    }

    /// @}

  }
}

