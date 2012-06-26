
#include <gflags/gflags.h>

#include "Aggregator.hpp"
#include "SoftXMT.hpp"
#include <csignal>

#include "PerformanceTools.hpp"

/// command line options for Aggregator
DEFINE_int64( aggregator_autoflush_ticks, 1000, "number of ticks to wait before autoflushing aggregated active messages");

/// global Aggregator instance
Aggregator global_aggregator;

#ifdef STL_DEBUG_ALLOCATOR
DEFINE_int64( aggregator_access_control_signal, SIGUSR2, "signal used to toggle aggregator queue access control");
bool aggregator_access_control_active = false;
static void aggregator_toggle_access_control_sighandler( int signum ) {
  aggregator_access_control_active = ~aggregator_access_control_active;
}

#endif

// Construct Aggregator.
Aggregator::Aggregator( ) 
  : max_nodes_( -1 )
  , buffers_( )
  , route_map_( )
  , previous_timestamp_( 0L )
  , least_recently_sent_( )
  , aggregator_deaggregate_am_handle_( -1 )
#ifdef VTRACE_FULL
  , tag_( -1 )
  , vt_agg_commid_( VT_COMM_DEF( "Aggregator" ) )
#endif
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
}

void Aggregator_deaggregate_am( gasnet_token_t token, void * buf, size_t size );

void Aggregator::init() {
  max_nodes_ = global_communicator.nodes();
  aggregator_deaggregate_am_handle_ = global_communicator.register_active_message_handler( &Aggregator_deaggregate_am );
  buffers_.resize( max_nodes_ - buffers_.size() );
  route_map_.resize( max_nodes_ - route_map_.size() );
  // initialize route map
  for( Node i = 0; i < max_nodes_; ++i ) {
    route_map_[i] = i;
  }
#ifdef VTRACE_FULL
  tag_ = global_communicator.mynode();
#endif
}

Aggregator::~Aggregator() {
#ifdef STL_DEBUG_ALLOCATOR
  STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadWrite);
#endif
}

void Aggregator::deaggregate( ) {
  GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
#ifdef VTRACE_FULL
  VT_TRACER("deaggregate");
#endif
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
          
	stats.record_deaggregation( sizeof( AggregatorGenericCallHeader ) + header->args_size + header->payload_size );
          // trace fine-grain communication
#ifdef GRAPPA_TRACE
          if (FLAGS_record_grappa_events) {
              // TODO: good candidate for TAU_CONTEXT_EVENT
              int fn_p_tag = aggregator_trace_tag( fp );
              TAU_TRACE_RECVMSG(fn_p_tag, header->source, header->args_size + header->payload_size );
          }
#endif

#ifdef VTRACE_FULL
	  {
	    VT_RECV( vt_agg_commid_, header->tag, sizeof( AggregatorGenericCallHeader ) + header->args_size + header->payload_size );
	  }
#endif


        DVLOG(5) << "calling " << *header 
                << " with args " << args
                << " and payload " << payload;
      
        {   
            GRAPPA_PROFILE( deag_func_timer, "deaggregate execution", "", GRAPPA_USERAM_GROUP );
            fp( args, header->args_size, payload, header->payload_size ); // execute
        } 
      } else { // not for us, so forward towards destination
        DVLOG(5) << "forwarding " << *header
                << " with args " << args
                << " and payload " << payload;
	stats.record_forward( sizeof( AggregatorGenericCallHeader ) + header->args_size + header->payload_size );
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
  GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
#ifdef VTRACE_FULL
  VT_TRACER("deaggregate AM");
#endif

  DVLOG(5) << "received message with size " << size;
  // TODO: too much copying
  Aggregator::ReceivedAM am( size, buf );
#ifdef STL_DEBUG_ALLOCATOR
  if( aggregator_access_control_active ) STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadWrite);
#endif
  global_aggregator.received_AM_queue_.push( am );
#ifdef STL_DEBUG_ALLOCATOR
  if( aggregator_access_control_active ) STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadOnly);
#endif

}

void AggregatorStatistics::merge_am(AggregatorStatistics * other, size_t sz, void* payload, size_t psz) {
  global_aggregator.stats.merge(other);
}
