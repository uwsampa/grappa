
#ifndef __MESSAGE_BASE_IMPL_HPP__
#define __MESSAGE_BASE_IMPL_HPP__

#include "MessageBase.hpp"

#define LEGACY_SEND

//#ifndef LEGACY_SEND
#include "RDMAAggregator.hpp"
//#endif

namespace Grappa {
  
  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{

    inline void Grappa::impl::MessageBase::enqueue() {
      CHECK( !is_moved_ ) << "Shouldn't be sending a message that has been moved!"
                          << " Your compiler's return value optimization failed you here.";
#ifndef LEGACY_SEND
      Grappa::impl::global_rdma_aggregator.enqueue( this );
#endif
      is_enqueued_ = true;
#ifdef LEGACY_SEND
      legacy_send();
#endif
    }

      inline void Grappa::impl::MessageBase::enqueue( Core c ) {
      CHECK( !is_moved_ ) << "Shouldn't be sending a message that has been moved!"
                          << " Your compiler's return value optimization failed you here.";
      destination_ = c;
#ifndef LEGACY_SEND
      Grappa::impl::global_rdma_aggregator.enqueue( this );
#endif
      is_enqueued_ = true;
#ifdef LEGACY_SEND
      legacy_send();
#endif
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
      CHECK( !is_moved_ ) << "Shouldn't be sending a message that has been moved!"
                          << " Your compiler's return value optimization failed you here.";
      destination_ = c;
      is_enqueued_ = true;
#ifndef LEGACY_SEND
      Grappa::impl::global_rdma_aggregator.send_immediate( this );
#endif
#ifdef LEGACY_SEND
      legacy_send();
#endif
    }

    /// @}
  }
}

#endif // __MESSAGE_BASE_IMPL_HPP__
