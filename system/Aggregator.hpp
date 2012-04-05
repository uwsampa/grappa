
#ifndef __AGGREGATOR_HPP__
#define __AGGREGATOR_HPP__



#include <vector>
#include <algorithm>
#include <queue>

#include <iostream>
#include <cassert>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <gasnet.h>

#include "common.hpp"
#include "gasnet_helpers.h"

#include "Communicator.hpp"
#include "Timestamp.hpp"

#include "MutableHeap.hpp"

/// Type of aggregated active message handler
typedef void (* AggregatorAMHandler)( void *, size_t, void *, size_t );

/// Active message handler called to deaggregate aggregated messages.
extern void Aggregator_deaggregate_am( gasnet_token_t token, void * buf, size_t size );

/// Header for aggregated active messages.
struct AggregatorGenericCallHeader {
  uintptr_t function_pointer;
  Node destination;
  uint16_t args_size;
  uint16_t payload_size;
};

static std::ostream& operator<<( std::ostream& o, const AggregatorGenericCallHeader& h ) {
  return o << "[f="           << (void*) h.function_pointer 
           << ",d=" << h.destination
           << ",a=" << h.args_size
           << ",p=" << h.payload_size
           << "(t=" << sizeof(h) + h.args_size + h.payload_size << ")"
           << "]";
}

/// Active message aggregation per-destination storage class.
template< const int max_size_ >
class AggregatorBuffer {
private:
public:
  int current_position_;
  char buffer_[ max_size_ ];

  AggregatorBuffer()
    : current_position_( 0 )
  { }

  /// use default copy constructor and assignment operator

  inline bool fits( size_t size ) const { 
    return (current_position_ + size) < max_size_; 
  }

  inline void insert( const void * data, size_t size ) {
    assert ( fits( size ) );
    memcpy( &buffer_[ current_position_ ], data, size );
    current_position_ += size;
  }

  inline void flush() {
    current_position_ = 0;
  }
};

/// Active message aggregation class.
class Aggregator {
private:
  DISALLOW_COPY_AND_ASSIGN( Aggregator );

  /// Pointer to communication layer.
  Communicator * communicator_;

  /// max node count. used to allocate buffers.
  const int max_nodes_;

  /// number of bytes in each aggregation buffer
  /// TODO: this should track the IB MTU
  static const int buffer_size_ = 4024;

  /// buffers holding aggregated messages. 
  std::vector< AggregatorBuffer< buffer_size_ > > buffers_;


  /// number of ticks to wait before automatically flushing a buffer.
  const int autoflush_ticks_;

  /// current timestamp for autoflusher.
  uint64_t previous_timestamp_;

  /// priority queue for autoflusher.
  MutableHeap< uint64_t, // priority (inverted timestamp)
               int       // key (target)
               > least_recently_sent_;

  /// handle for deaggregation active message
  const int aggregator_deaggregate_am_handle_;

  /// routing table for hierarchical aggregation
  std::vector< Node > route_map_;

  /// storage for deaggregation
  struct ReceivedAM {
    size_t size_;
    char buf_[buffer_size_];
    ReceivedAM( size_t size, void * buf ) 
      : size_( size )
      , buf_()
    { 
      assert( size < buffer_size_ );
      memcpy( buf_, buf, size );
    }
  };
  std::queue< ReceivedAM > received_AM_queue_;
  void deaggregate( );
  friend void Aggregator_deaggregate_am( gasnet_token_t token, void * buf, size_t size );

public:

  /// Construct Aggregator. Takes a Communicator pointer in order to
  /// register active message handlers
  explicit Aggregator( Communicator * communicator );
                                              
  /// route map lookup for hierarchical aggregation
  inline Node get_target_for_node( Node n ) const {
    return route_map_[ n ];
  }

  /// route map update for hierarchical aggregation
  inline void update_target_for_node( Node node, Node target ) {
    route_map_[ node ] = target;
  }

  /// send aggregated messages for node
  inline void flush( Node node ) {
    DVLOG(5) << "flushing node " << node;
    Node target = route_map_[ node ];
    communicator_->send( target, 
                         aggregator_deaggregate_am_handle_,
                         buffers_[ target ].buffer_,
                         buffers_[ target ].current_position_ );
    buffers_[ target ].flush();
    DVLOG(5) << "heap before flush:\n" << least_recently_sent_.toString( );
    least_recently_sent_.remove_key( target );
    DVLOG(5) << "heap after flush:\n" << least_recently_sent_.toString( );
  }

  /// get timestamp. we avoid calling rdtsc for performance
  inline uint64_t get_timestamp() {
    //return previous_timestamp_ + 1;
    return SoftXMT_get_timestamp();
   }

  inline uint64_t get_previous_timestamp() {
    return previous_timestamp_;
  }

  /// poll communicator. send any aggregated messages that have been sitting for too long
  inline void poll() {
    communicator_->poll();
    uint64_t ts = get_timestamp();
    // timestamp overflows are silently ignored. 
    // since it would take many many years to see one, I think that's okay for now.
    if( !least_recently_sent_.empty() ) {                                    // if messages are waiting, and
      if( -least_recently_sent_.top_priority() + autoflush_ticks_ < ts ) { // we've waited long enough,
        DVLOG(5) << "timeout for node " << least_recently_sent_.top_key() 
                 << ": inserted at " << -least_recently_sent_.top_priority()
                 << " autoflush_ticks_ " << autoflush_ticks_ 
                 << " (current ts " << ts << ")";
	flush( least_recently_sent_.top_key() );                   // send.
      }
    }
    previous_timestamp_ = ts;
    deaggregate();
  }

  inline const size_t max_size() const { return buffer_size_; }
  inline const size_t remaining_size( Node destination ) const { 
    Node target = get_target_for_node( destination );
    return buffer_size_ - buffers_[ target ].current_position_; 
  }

  inline void aggregate( Node destination, AggregatorAMHandler fn_p, 
                         const void * args, const size_t args_size,
                         const void * payload, const size_t payload_size ) {
    CHECK( destination < max_nodes_ ) << "destination:" << destination << " max_nodes_:" << max_nodes_;
    Node target = get_target_for_node( destination );
  
    // make sure arg struct and payload aren't too big.
    // in the future, this would lead us down a separate code path for large messages.
    // for now, fail.
    size_t total_call_size = payload_size + args_size + sizeof( AggregatorGenericCallHeader );
    DVLOG(5) << "aggregating " << total_call_size << " bytes to " 
             << destination << "(target " << target << ")";
    CHECK( total_call_size < buffer_size_ ) << "payload_size( " << payload_size << " )"
                                           << "+args_size( " << args_size << " )"
                                           << "+header_size( " << sizeof( AggregatorGenericCallHeader ) << " )"
                                           << "= " << total_call_size << " of max( " << buffer_size_ << " )";
  
    // does call fit in aggregation buffer?
    if( !( buffers_[ target ].fits( total_call_size ) ) ) {
      DVLOG(5) << "aggregating " << total_call_size << " bytes to " 
               << destination << "(target " << target 
               << "): didn't fit "
               << "(current buffer position " << buffers_[ target ].current_position_
               << ", next buffer position " << buffers_[ target ].current_position_ + total_call_size << ")";
      // doesn't fit, so flush before inserting
      flush( target );
      assert ( buffers_[ target ].fits( total_call_size ));
    }
  
    // now call must fit, so just insert it
    AggregatorGenericCallHeader header = { reinterpret_cast< intptr_t >( fn_p ),
                                           destination,
                                           args_size,
                                           payload_size };
    buffers_[ target ].insert( &header, sizeof( header ) );
    buffers_[ target ].insert( args, args_size );
    buffers_[ target ].insert( payload, payload_size );
    
    uint64_t ts = get_timestamp();
    least_recently_sent_.update_or_insert( target, -ts );
    previous_timestamp_ = ts;
    DVLOG(5) << "aggregated " << header;
  }

};

extern Aggregator * global_aggregator;

template< typename ArgsStruct >
inline void SoftXMT_call_on( Node destination, void (* fn_p)(ArgsStruct *, size_t, void *, size_t), 
                             const ArgsStruct * args, const size_t args_size = sizeof( ArgsStruct ),
                             const void * payload = NULL, const size_t payload_size = 0)
{
  assert( global_aggregator != NULL );
  global_aggregator->aggregate( destination, 
                                reinterpret_cast< AggregatorAMHandler >( fn_p ), 
                                static_cast< const void * >( args ), args_size,
                                static_cast< const void * >( payload ), payload_size );
}

template< typename ArgsStruct, typename PayloadType >
inline void SoftXMT_call_on_x( Node destination, void (* fn_p)(ArgsStruct *, size_t, PayloadType *, size_t), 
                               const ArgsStruct * args, const size_t args_size = sizeof( ArgsStruct ),
                               const PayloadType * payload = NULL, const size_t payload_size = 0)
{
  assert( global_aggregator != NULL );
  global_aggregator->aggregate( destination, 
                                reinterpret_cast< AggregatorAMHandler >( fn_p ), 
                                static_cast< const void * >( args ), args_size,
                                static_cast< const void * >( payload ), payload_size );
}


#endif
