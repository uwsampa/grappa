
#include "MessageBase.hpp"
#include "RDMAAggregator.hpp"


inline void Grappa::impl::MessageBase::enqueue() {
  CHECK( !is_moved_ ) << "Shouldn't be sending a message that has been moved!"
		      << " Your compiler's return value optimization failed you here.";
  Grappa::impl::global_rdma_aggregator.enqueue( this );
  is_enqueued_ = true;
  // legacy_send();
}

inline void Grappa::impl::MessageBase::send_immediate() {
  CHECK( !is_moved_ ) << "Shouldn't be sending a message that has been moved!"
		      << " Your compiler's return value optimization failed you here.";
  is_enqueued_ = true;
  Grappa::impl::global_rdma_aggregator.send_immediate( this );
}

