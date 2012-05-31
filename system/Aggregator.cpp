
#include <gflags/gflags.h>

#include "Aggregator.hpp"
#include "SoftXMT.hpp"
#include <csignal>


/// command line options for Aggregator
DEFINE_int64( aggregator_autoflush_ticks, 1000, "number of ticks to wait before autoflushing aggregated active messages");

/// global Aggregator pointer
Aggregator * global_aggregator = NULL;

#ifdef STL_DEBUG_ALLOCATOR
DEFINE_int64( aggregator_access_control_signal, SIGUSR2, "signal used to toggle aggregator queue access control");
bool aggregator_access_control_active = false;
static void aggregator_toggle_access_control_sighandler( int signum ) {
  aggregator_access_control_active = ~aggregator_access_control_active;
}

#endif

// Construct Aggregator. Takes a Communicator pointer to register active message handlers
Aggregator::Aggregator( Communicator * communicator ) 
  : communicator_( communicator )
  , max_nodes_( communicator->nodes() )
  , buffers_( max_nodes_ )
  , route_map_( max_nodes_ )
  , autoflush_ticks_( FLAGS_aggregator_autoflush_ticks )
  , previous_timestamp_( 0L )
  , least_recently_sent_( )
  , aggregator_deaggregate_am_handle_( communicator_->register_active_message_handler( &Aggregator_deaggregate_am ) )
  , stats()
{ 
#ifdef STL_DEBUG_ALLOCATOR
  struct sigaction access_control_toggle_sa;
  sigemptyset( &access_control_toggle_sa.sa_mask );
  access_control_toggle_sa.sa_flags = 0;
  access_control_toggle_sa.sa_handler = &aggregator_toggle_access_control_sighandler;
  CHECK_EQ( 0, sigaction( FLAGS_aggregator_access_control_signal, &access_control_toggle_sa, 0 ) ) 
    << "Aggregator access control signal handler installation failed.";
  if( aggregator_access_control_active ) STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadOnly);
#endif
  // initialize route map
  for( Node i = 0; i < max_nodes_; ++i ) {
    route_map_[i] = i;
  }
  assert( global_aggregator == NULL );
  global_aggregator = this;
}

Aggregator::~Aggregator() {
#ifdef STL_DEBUG_ALLOCATOR
  STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadWrite);
#endif
}

void Aggregator::deaggregate( ) {
  StateTimer::enterState_deaggregation();
  while( !received_AM_queue_.empty() ) {
    DVLOG(5) << "deaggregating";
    // TODO: too much copying
    ReceivedAM amp = received_AM_queue_.front();

#ifdef STL_DEBUG_ALLOCATOR
    if( aggregator_access_control_active ) STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadWrite);
#endif
    received_AM_queue_.pop();
#ifdef STL_DEBUG_ALLOCATOR
    if( aggregator_access_control_active ) STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadOnly);
#endif

    DVLOG(5) << "deaggregating message of size " << amp.size_;
    uintptr_t msg_base = reinterpret_cast< uintptr_t >( amp.buf_ );
    for( int i = 0; i < amp.size_; ) {
      AggregatorGenericCallHeader * header = reinterpret_cast< AggregatorGenericCallHeader * >( msg_base );
      AggregatorAMHandler fp = reinterpret_cast< AggregatorAMHandler >( header->function_pointer );
      void * args = reinterpret_cast< void * >( msg_base + 
                                                sizeof( AggregatorGenericCallHeader ) );
      void * payload = reinterpret_cast< void * >( msg_base +
                                                   sizeof( AggregatorGenericCallHeader ) +
                                                   header->args_size );
      
      if( header->destination == gasnet_mynode() ) { // for us?
          
          // trace fine-grain communication
#ifdef GRAPPA_TRACE
          {
              // TODO: good candidate for TAU_CONTEXT_EVENT
              int fn_p_tag = aggregator_trace_tag( fp );
              TAU_TRACE_RECVMSG(fn_p_tag, header->source, header->args_size + header->payload_size );
          }
#endif

        DVLOG(5) << "calling " << *header 
                << " with args " << args
                << " and payload " << payload;
        fp( args, header->args_size, payload, header->payload_size ); // execute
      } else { // not for us, so forward towards destination
        DVLOG(5) << "forwarding " << *header
                << " with args " << args
                << " and payload " << payload;
        SoftXMT_call_on( header->destination, fp, args, header->args_size, payload, header->payload_size );
      }
      i += sizeof( AggregatorGenericCallHeader ) + header->args_size + header->payload_size;
      msg_base += sizeof( AggregatorGenericCallHeader ) + header->args_size + header->payload_size;
    }
  }
}
  
  
void Aggregator::finish() {
#ifdef STL_DEBUG_ALLOCATOR
  LOG(INFO) << "Cleaning up access control....";
  STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadWrite);
#endif
}

void Aggregator_deaggregate_am( gasnet_token_t token, void * buf, size_t size ) {
  DVLOG(5) << "received message with size " << size;
  // TODO: too much copying
  Aggregator::ReceivedAM am( size, buf );
#ifdef STL_DEBUG_ALLOCATOR
  if( aggregator_access_control_active ) STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadWrite);
#endif
  global_aggregator->received_AM_queue_.push( am );
#ifdef STL_DEBUG_ALLOCATOR
  if( aggregator_access_control_active ) STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadOnly);
#endif

}
