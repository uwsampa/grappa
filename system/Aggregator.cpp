
#include <gflags/gflags.h>

#include "Aggregator.hpp"

/// command line options for Aggregator
DEFINE_int64( aggregator_autoflush_ticks, 1000, "number of ticks to wait before autoflushing aggregated active messages");

/// global Aggregator pointer
Aggregator * global_aggregator = NULL;


// Construct Aggregator. Takes a Communicator pointer to register active message handlers
Aggregator::Aggregator( Communicator * communicator ) 
  : communicator_( communicator ),
    max_nodes_( communicator->nodes() ),
    buffers_( max_nodes_ ),
    route_map_( max_nodes_ ),
    autoflush_ticks_( FLAGS_aggregator_autoflush_ticks ),
    previous_timestamp_( 0L ),
    least_recently_sent_( ),
    aggregator_deaggregate_am_handle_( communicator_->register_active_message_handler( &Aggregator_deaggregate_am ) )
{ 
  // initialize route map
  for( Node i = 0; i < max_nodes_; ++i ) {
    route_map_[i] = i;
  }
  assert( global_aggregator == NULL );
  global_aggregator = this;
}


void Aggregator_deaggregate_am( gasnet_token_t token, void * buf, size_t size ) {
  for( int i = 0; i < size; ) {
    AggregatorGenericCallHeader * header = static_cast< AggregatorGenericCallHeader * >( buf );
    AggregatorAMHandler fp = reinterpret_cast< AggregatorAMHandler >( header->function_pointer );
    void * args = reinterpret_cast< void * >( reinterpret_cast< uintptr_t >( buf ) + 
                                              sizeof( AggregatorGenericCallHeader ) );
    void * payload = reinterpret_cast< void * >( reinterpret_cast< uintptr_t >( buf ) + 
                                                 sizeof( AggregatorGenericCallHeader ) +
                                                 header->args_size );

    if( header->destination == gasnet_mynode() ) { // for us?
      fp( args, header->args_size, payload, header->payload_size ); // execute
    } else { // not for us, so forward towards destination
      call_on( header->destination, fp, args, header->args_size, payload, header->payload_size );
    }
    i += sizeof( AggregatorGenericCallHeader ) + header->args_size + header->payload_size;
  }
}



