
#ifndef __AGGREGATOR_HPP__
#define __AGGREGATOR_HPP__


#ifdef STL_DEBUG_ALLOCATOR
#include "../tools/stl-debug-allocator/archive/Source/includes.h"
#include "../tools/stl-debug-allocator/archive/Source/allocator.h"
#endif

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

#include "StateTimer.hpp"
#include "PerformanceTools.hpp"


#include <TAU.h>
// function to compute a mpi-like tag for communication tracing
//#define aggregator_trace_tag(data) (int) (0x7FFF & reinterpret_cast<intptr_t>(data))
#define aggregator_trace_tag(data) (0xffffffff & reinterpret_cast<intptr_t>(data))

/// number of ticks to wait before automatically flushing a buffer.
DECLARE_int64( aggregator_autoflush_ticks );

/// Type of aggregated active message handler
typedef void (* AggregatorAMHandler)( void *, size_t, void *, size_t );

/// Active message handler called to deaggregate aggregated messages.
extern void Aggregator_deaggregate_am( gasnet_token_t token, void * buf, size_t size );

extern bool aggregator_access_control_active;

class AggregatorStatistics {
private:
  uint64_t messages_aggregated_;
  uint64_t bytes_aggregated_;
  uint64_t flushes_;
  uint64_t timeouts_;
  uint64_t idle_flushes_;
  uint64_t capacity_flushes_;
  uint64_t histogram_[16];
  timespec start_;

  std::string hist_labels[16];
  
  std::ostream& header( std::ostream& o ) {
    o << "AggregatorStatistics, header, time, messages_aggregated, bytes_aggregated, messages_aggregated_per_second, bytes_aggregated_per_second, flushes, timeouts";
    for (int i=0; i<16; i++) o << ", " << hist_labels[i];
    return o;
  }

  std::ostream& data( std::ostream& o, double time) {
    o << "AggregatorStatistics, data, " << time << ", ";
    double messages_aggregated_per_second = messages_aggregated_ / time;
    double bytes_aggregated_per_second = bytes_aggregated_ / time;
    o << messages_aggregated_ << ", " 
      << bytes_aggregated_ << ", "
      << messages_aggregated_per_second << ", "
      << bytes_aggregated_per_second << ", ";
    o << flushes_ << ", " << timeouts_;
    for( int i = 0; i < 16; ++i ) {
      o << ", " << histogram_[ i ];
    }
    return o;
  }
  
  std::ostream& as_map( std::ostream& o, double time) {
    double messages_aggregated_per_second = messages_aggregated_ / time;
    double bytes_aggregated_per_second = bytes_aggregated_ / time;
    
    o << "AggregatorStatistics {" ;
    o << "time_aggregated: " << time << ", "
      << "messages_aggregated: " << messages_aggregated_ << ", "
      << "bytes_aggregated: " << bytes_aggregated_ << ", "
      << "messages_aggregated_per_second: " << messages_aggregated_per_second << ", "
      << "bytes_aggregated_per_second: " << bytes_aggregated_per_second << ", "
      << "flushes: " << flushes_ << ", "
      << "timeouts: " << timeouts_ << ", "
      << "idle_flushes: " << idle_flushes_ << ", "
      << "capacity_flushes: " << capacity_flushes_;
    for (int i=0; i<16; i++) {
      o << ", " << hist_labels[i] << ": " << histogram_[i];
    }
    o << " }";
    return o;
  }
  
  double time() {
    timespec end;
    clock_gettime( CLOCK_MONOTONIC, &end );
    return (end.tv_sec + end.tv_nsec * 0.000000001) - (start_.tv_sec + start_.tv_nsec * 0.000000001);
  }

public:
  AggregatorStatistics()
    : histogram_()
    , start_()
  {
    reset();
    hist_labels[ 0] = "aggregator_0_to_255_bytes";
    hist_labels[ 1] = "aggregator_256_to_511_bytes";
    hist_labels[ 2] = "aggregator_512_to_767_bytes";
    hist_labels[ 3] = "aggregator_768_to_1023_bytes";
    hist_labels[ 4] = "aggregator_1024_to_1279_bytes";
    hist_labels[ 5] = "aggregator_1280_to_1535_bytes";
    hist_labels[ 6] = "aggregator_1536_to_1791_bytes";
    hist_labels[ 7] = "aggregator_1792_to_2047_bytes";
    hist_labels[ 8] = "aggregator_2048_to_2303_bytes";
    hist_labels[ 9] = "aggregator_2304_to_2559_bytes";
    hist_labels[10] = "aggregator_2560_to_2815_bytes";
    hist_labels[11] = "aggregator_2816_to_3071_bytes";
    hist_labels[12] = "aggregator_3072_to_3327_bytes";
    hist_labels[13] = "aggregator_3328_to_3583_bytes";
    hist_labels[14] = "aggregator_3584_to_3839_bytes";
    hist_labels[15] = "aggregator_3840_to_4095_bytes";
  }
  
  void reset() {
    messages_aggregated_ = 0;
    bytes_aggregated_ = 0;
    flushes_ = 0;
    timeouts_ = 0;
    idle_flushes_ = 0;
    capacity_flushes_ = 0;
    clock_gettime(CLOCK_MONOTONIC, &start_);
    for( int i = 0; i < 16; ++i ) {
      histogram_[i] = 0;
    }
  }

  void record_flush() { 
    flushes_++;
  }

  void record_timeout() {
    timeouts_++;
  }

  void record_idle_flush() {
    idle_flushes_++;
  }
  
  void record_capacity_flush() {
    capacity_flushes_++;
  }
  
  void record_aggregation( size_t bytes ) {
    messages_aggregated_++;
    bytes_aggregated_ += bytes;
    histogram_[ (bytes >> 8) & 0xf ]++;
  }
  void dump() {
    header( LOG(INFO) );
    data( LOG(INFO), time() );
  }
  void dump_as_map() {
    as_map( std::cout, time() );
    std::cout << std::endl;
  }
  
  void merge(AggregatorStatistics * other) {
    messages_aggregated_ += other->messages_aggregated_;
    bytes_aggregated_ += other->bytes_aggregated_;
    flushes_ += other->flushes_;
    timeouts_ += other->timeouts_;
    idle_flushes_ += other->idle_flushes_;
    capacity_flushes_ += other->capacity_flushes_;
    for( int i = 0; i < 16; ++i ) {
      histogram_[i] += other->histogram_[i];
    }
  }

  static void merge_am(AggregatorStatistics * other, size_t sz, void* payload, size_t psz);
};

/// Header for aggregated active messages.
struct AggregatorGenericCallHeader {
  uintptr_t function_pointer;
  Node destination;
  uint16_t args_size;
  uint16_t payload_size;
#ifdef GRAPPA_TRACE
// TODO: really don't want this transmitted even in tracing
  Node source;
#endif
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

  /// max node count. used to allocate buffers.
  int max_nodes_;

  /// number of bytes in each aggregation buffer
  /// TODO: this should track the IB MTU
  static const int buffer_size_ = 4024;

  /// buffers holding aggregated messages. 
  std::vector< AggregatorBuffer< buffer_size_ > > buffers_;

  /// current timestamp for autoflusher.
  uint64_t previous_timestamp_;

  /// priority queue for autoflusher.
  MutableHeap< uint64_t, // priority (inverted timestamp)
               int       // key (target)
               > least_recently_sent_;

  /// handle for deaggregation active message
  int aggregator_deaggregate_am_handle_;

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

#ifdef STL_DEBUG_ALLOCATOR
  std::queue< ReceivedAM, std::deque< ReceivedAM, STLMemDebug::Allocator< ReceivedAM > > > received_AM_queue_;
#else
  std::queue< ReceivedAM > received_AM_queue_;
#endif

  void deaggregate( );
  friend void Aggregator_deaggregate_am( gasnet_token_t token, void * buf, size_t size );

public:
  /// statistics
  AggregatorStatistics stats;  

  /// Construct Aggregator.
  Aggregator( );

  void init();

  ~Aggregator();

  void finish();

  void dump_stats() { stats.dump_as_map(); }
  void merge_stats();
  void reset_stats() { stats.reset(); }

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
    GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
    DVLOG(5) << "flushing node " << node;
    stats.record_flush();
    Node target = route_map_[ node ];
    global_communicator.send( target,
                              aggregator_deaggregate_am_handle_,
                              buffers_[ target ].buffer_,
                              buffers_[ target ].current_position_ );
    buffers_[ target ].flush();
    DVLOG(5) << "heap before flush:\n" << least_recently_sent_.toString( );
    least_recently_sent_.remove_key( target );
    DVLOG(5) << "heap after flush:\n" << least_recently_sent_.toString( );
  }
  
  inline void idle_flush_poll() {
    GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
    StateTimer::enterState_communication();
    global_communicator.poll();
    while ( !least_recently_sent_.empty() ) {
      stats.record_idle_flush();
      DVLOG(5) << "idle flush Node " << least_recently_sent_.top_key();
      flush(least_recently_sent_.top_key());
    }
    deaggregate();
  }
  

  /// get timestamp. we avoid calling rdtsc for performance
  inline uint64_t get_timestamp() {
    //return previous_timestamp_ + 1;
    SoftXMT_tick();
    return SoftXMT_get_timestamp();
   }

  inline uint64_t get_previous_timestamp() {
    return previous_timestamp_;
  }

  /// poll communicator. send any aggregated messages that have been sitting for too long
  inline void poll() {
    GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
    global_communicator.poll();
    uint64_t ts = get_timestamp();
    // timestamp overflows are silently ignored. 
    // since it would take many many years to see one, I think that's okay for now.
    if( !least_recently_sent_.empty() ) {                                    // if messages are waiting, and
      if( -least_recently_sent_.top_priority() + FLAGS_aggregator_autoflush_ticks < ts ) { // we've waited long enough,
        stats.record_timeout();
        DVLOG(5) << "timeout for node " << least_recently_sent_.top_key() 
                 << ": inserted at " << -least_recently_sent_.top_priority()
                 << " autoflush_ticks " << FLAGS_aggregator_autoflush_ticks
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
    GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
    CHECK( destination < max_nodes_ ) << "destination:" << destination << " max_nodes_:" << max_nodes_;
    Node target = get_target_for_node( destination );
    CHECK( target < max_nodes_ ) << "target:" << target << " max_nodes_:" << max_nodes_;

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
      stats.record_capacity_flush();
      flush( target );
      assert ( buffers_[ target ].fits( total_call_size ));
    }
  
    // now call must fit, so just insert it
    AggregatorGenericCallHeader header = { reinterpret_cast< intptr_t >( fn_p ),
                                           destination,
                                           args_size,
                                           payload_size
#ifdef GRAPPA_TRACE
                                          , global_communicator.mynode()
#endif
   
             };
   
    buffers_[ target ].insert( &header, sizeof( header ) );
    buffers_[ target ].insert( args, args_size );
    buffers_[ target ].insert( payload, payload_size );
    stats.record_aggregation( total_call_size );
  
    // trace fine-grain communication
  {
    // TODO: good candidate for TAU_CONTEXT_EVENT
      int fn_p_tag = aggregator_trace_tag( fn_p );
      TAU_TRACE_SENDMSG(fn_p_tag, destination, args_size + payload_size );
  }

  uint64_t ts = get_timestamp();
  least_recently_sent_.update_or_insert( target, -ts );
  previous_timestamp_ = ts;
  DVLOG(5) << "aggregated " << header;
}

};


extern Aggregator global_aggregator;



template< typename ArgsStruct >
inline void SoftXMT_call_on( Node destination, void (* fn_p)(ArgsStruct *, size_t, void *, size_t), 
                             const ArgsStruct * args, const size_t args_size = sizeof( ArgsStruct ),
                             const void * payload = NULL, const size_t payload_size = 0)
{
  StateTimer::start_communication();
  global_aggregator.aggregate( destination,
                               reinterpret_cast< AggregatorAMHandler >( fn_p ),
                               static_cast< const void * >( args ), args_size,
                               static_cast< const void * >( payload ), payload_size );
  StateTimer::stop_communication();
}


template< typename ArgsStruct, typename PayloadType >
inline void SoftXMT_call_on_x( Node destination, void (* fn_p)(ArgsStruct *, size_t, PayloadType *, size_t), 
                               const ArgsStruct * args, const size_t args_size = sizeof( ArgsStruct ),
                               const PayloadType * payload = NULL, const size_t payload_size = 0)
{
  StateTimer::start_communication();
  global_aggregator.aggregate( destination,
                               reinterpret_cast< AggregatorAMHandler >( fn_p ),
                               static_cast< const void * >( args ), args_size,
                               static_cast< const void * >( payload ), payload_size );
  StateTimer::stop_communication();
}


#endif
