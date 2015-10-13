////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef STL_DEBUG_ALLOCATOR
#include "../tools/stl-debug-allocator/archive/Source/includes.h"
#include "../tools/stl-debug-allocator/archive/Source/allocator.h"
#endif

#include <vector>
#include <algorithm>
#include <queue>

#include <iostream>
#include <cassert>

#include <type_traits>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "common.hpp"

#include "Communicator.hpp"
#include "Timestamp.hpp"

#include "StateTimer.hpp"
#include "MetricsTools.hpp"
#include "Metrics.hpp"

#ifdef VTRACE
#include <vt_user.h>
#endif
#include "PerformanceTools.hpp"

/// macro to compute a mpi-like tag for communication tracing
//#define aggregator_trace_tag(data) (int) (0x7FFF & reinterpret_cast<intptr_t>(data))
#define aggregator_trace_tag(data) (0xffffffff & reinterpret_cast<intptr_t>(data))

/// number of ticks to wait before automatically flushing a buffer.
DECLARE_int64( aggregator_autoflush_ticks );
DECLARE_int64( aggregator_max_flush );
DECLARE_bool( aggregator_enable );
DECLARE_bool(flush_on_idle );

/// Type of aggregated active message handler
typedef void (* AggregatorAMHandler)( void *, size_t, void *, size_t );

extern void Aggregator_deaggregate_am( void * buf, size_t size );

extern bool aggregator_access_control_active;

/// Least recently used queue for tracking when to send buffered active messages
class LRQueue {
private:
  struct LREntry {
    LREntry * older_;
    LREntry * newer_;
    int key_;
    uint64_t priority_;
    LREntry() 
      : older_(NULL)
      , newer_(NULL)
      , key_( 0 )
      , priority_( 0 ) 
    { }
  };

  LREntry * entries_;

  LREntry oldest_;
  LREntry newest_;

  int size_;

public:
  LRQueue( ) 
    : entries_( NULL )
    , oldest_( )
    , newest_( )
    , size_( 0 )
  { 
    oldest_.newer_ = &newest_;
    oldest_.key_ = -1;
    oldest_.priority_ = 0;

    newest_.older_ = &oldest_;
    newest_.key_ = -1;
    newest_.priority_ = 0;
  }

  ~LRQueue() {
    delete [] entries_;
  }

  void resize( int size ) {
    CHECK( entries_ == NULL ) << "may only resize once";
    entries_ = new LREntry[ size ];
    for( int i = size_; i < size; ++i ) {
      entries_[i].key_ = i;
      entries_[i].priority_ = 0;
    }
    size_ = size;
  }

  inline void update_or_insert( int key, uint64_t priority ) {
    // NB: update has no effect.
    // TODO: change method name

    //DCHECK( key < size_ ) << "key too big";

    // are we not already in the queue?
    if( entries_[key].older_ == NULL ) {
      //DCHECK( entries_[key].newer_ == NULL ) << "if older is NULL, newer must be NULL";

      // insert at newer end
      entries_[key].newer_ = &newest_;
      entries_[key].older_ = newest_.older_; 
      newest_.older_ = &entries_[key];

      // add back link
      entries_[key].older_->newer_ = &entries_[key];

      // keeping this is the whole point!
      entries_[key].priority_ = priority;
    } 
  }

  int top_key() {
    return oldest_.newer_->key_;
  }

  uint64_t top_priority() {
    return oldest_.newer_->priority_;
  }

  void remove_key( int key ) {
    //DCHECK( key < size_ ) << "key too big";
    if( entries_[key].older_ != NULL ) {
      //DCHECK( entries_[key].newer_ != NULL ) << "nullness should match";
      entries_[key].older_->newer_ = entries_[key].newer_;
      entries_[key].newer_->older_ = entries_[key].older_;
      entries_[key].older_ = NULL;
      entries_[key].newer_ = NULL;
    }
  }
  
  bool empty() {
    return oldest_.newer_ == &newest_;
  }
  
};


GRAPPA_DECLARE_METRIC( SimpleMetric<uint64_t>, aggregator_messages_aggregated_);
GRAPPA_DECLARE_METRIC( SimpleMetric<uint64_t>, aggregator_bytes_aggregated_);
GRAPPA_DECLARE_METRIC( SimpleMetric<uint64_t>, aggregator_messages_deaggregated_);
GRAPPA_DECLARE_METRIC( SimpleMetric<uint64_t>, aggregator_bytes_deaggregated_);
GRAPPA_DECLARE_METRIC( SimpleMetric<uint64_t>, aggregator_bundles_received_);

/// stats class for aggregator
class AggregatorMetrics {
private:
  Grappa::SimpleMetric<uint64_t> * histogram_[16];
    
public:

  AggregatorMetrics();

  void record_poll();

  void record_flush( Grappa::Timestamp oldest_ts, Grappa::Timestamp newest_ts );
  void record_idle_flush();
  void record_multiflush();
  void record_timeout();
  void record_idle_poll( bool useful );
  void record_capacity_flush();

  void record_aggregation( size_t bytes ) {
    aggregator_messages_aggregated_++;
    aggregator_bytes_aggregated_ += bytes;
    (*(histogram_[ (bytes >> 8) & 0xf ]))++;
  }

  void record_deaggregation( size_t bytes ) {
    ++aggregator_messages_deaggregated_;
    aggregator_bytes_deaggregated_ += bytes;
  }

  void record_forward( size_t bytes );

  void record_receive_bundle( size_t bytes );

};

/// Header for aggregated active messages.
struct AggregatorGenericCallHeader {
  uintptr_t function_pointer;
  Core destination;
  uint16_t args_size;
  uint16_t payload_size;
#ifdef GRAPPA_TRACE
// TODO: really don't want this transmitted even in tracing
  Core source;
#endif
#ifdef VTRACE_FULL
// TODO: really don't want this transmitted even in tracing
  uint64_t tag;
#endif
};

/// dump human-readable aggregated active message header
static std::ostream& operator<<( std::ostream& o, const AggregatorGenericCallHeader& h ) {
  return o << "[f="           << (void*) h.function_pointer 
           << ",d=" << h.destination
           << ",a=" << h.args_size
           << ",p=" << h.payload_size
           << "(t=" << sizeof(h) + h.args_size + h.payload_size << ")"
           << "]";
}

/// Active message aggregation per-destination storage class.
  /// use default copy constructor and assignment operator
template< const int max_size_ >
class AggregatorBuffer {
private:
public:
  Grappa::Timestamp oldest_ts_;
  Grappa::Timestamp newest_ts_;
  int current_position_;
  char buffer_[ max_size_ ];

  AggregatorBuffer()
    : current_position_( 0 )
  { }

  /// does a message of size fit in this buffer?
  inline bool fits( size_t size ) const { 
    return (current_position_ + size) < max_size_; 
  }

  /// insert data into buffer. assumes it fits.
  inline void insert( const void * data, size_t size ) {
    newest_ts_ = Grappa::timestamp();
    if( current_position_ == 0 ) oldest_ts_ = newest_ts_;
    DCHECK ( fits( size ) );
    memcpy( &buffer_[ current_position_ ], data, size );
    current_position_ += size;
  }

  // reset buffer
  inline void flush() {
    current_position_ = 0;
  }
};

template <typename ArgStruct>
size_t Grappa_sizeof_message( const ArgStruct * args, const size_t args_size = sizeof( ArgStruct ),
                             const void * payload = NULL, const size_t payload_size = 0) {
  return payload_size + args_size + sizeof( AggregatorGenericCallHeader );
}


/// Active message aggregation class.
class Aggregator {
private:
  DISALLOW_COPY_AND_ASSIGN( Aggregator );

  /// max node count. used to allocate buffers.
  int max_nodes_;

  /// number of bytes in each aggregation buffer
  /// TODO: this should track the IB MTU
  static const unsigned int buffer_size_ = 4096 - 72;

  /// buffer for sending non-aggregated messages
  char raw_send_buffer_[ buffer_size_ ];

  /// buffers holding aggregated messages. 
  std::vector< AggregatorBuffer< buffer_size_ > > buffers_;

  /// current timestamp for autoflusher.
  uint64_t previous_timestamp_;

  /// queue for autoflusher.
  LRQueue least_recently_sent_;

  /// handle for deaggregation active message
  int aggregator_deaggregate_am_handle_;

  /// routing table for hierarchical aggregation
  std::vector< Core > route_map_;

#ifdef VTRACE_FULL
  /// Vampir trace message metadata
  uint64_t tag_;
  /// Vampir trace message metadata
  unsigned vt_agg_commid_;
#endif

  /// storage for deaggregation
  struct ReceivedAM {
    size_t size_;
    char buf_[buffer_size_];
    ReceivedAM( size_t size, void * buf ) 
      : size_( size )
      , buf_()
    { 
      DCHECK( size < buffer_size_ );
      memcpy( buf_, buf, size );
    }
  };

#ifdef STL_DEBUG_ALLOCATOR
  /// Storage for received pre-deaggregation active messages
  std::queue< ReceivedAM, std::deque< ReceivedAM, STLMemDebug::Allocator< ReceivedAM > > > received_AM_queue_;
#else
  /// Storage for received pre-deaggregation active messages
  std::queue< ReceivedAM > received_AM_queue_;
#endif

  /// Deaggregated buffered active messages
  void deaggregate( );
  friend void Aggregator_deaggregate_am( void * buf, size_t size );

public:
  /// statistics
  AggregatorMetrics stats;  

  /// Construct Aggregator.
  Aggregator( );

  /// Initialize aggregator. 
  void init();

  /// Tear down aggegator.
  ~Aggregator();

  /// Clean up aggregator before destruction.
  void finish();

  /// route map lookup for hierarchical aggregation
  inline Core get_target_for_node( Core n ) const {
    return route_map_[ n ];
  }

  /// route map update for hierarchical aggregation
  inline void update_target_for_node( Core node, Core target ) {
    route_map_[ node ] = target;
  }

  /// send aggregated messages for node
  inline void flush( Core node ) {
    GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
    DVLOG(5) << "flushing node " << node;
    Core target = route_map_[ node ];
    stats.record_flush( buffers_[ target ].oldest_ts_, buffers_[ target ].newest_ts_ );
    size_t size = buffers_[ target ].current_position_;
    global_communicator.send_immediate_with_payload( target, [] (void * buf, int size) {
        Aggregator_deaggregate_am( buf, size );
      }, buffers_[ target ].buffer_, size );
    buffers_[ target ].flush();
    //DVLOG(5) << "heap before flush:\n" << least_recently_sent_.toString( );
    least_recently_sent_.remove_key( target );
    //DVLOG(5) << "heap after flush:\n" << least_recently_sent_.toString( );
  }
  
  /// poll and optionally flush on idle
  inline bool idle_flush_poll() {
    GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
#ifdef VTRACE_FULL
    VT_TRACER("idle_flush_poll");
#endif
    StateTimer::enterState_communication();
    if( FLAGS_flush_on_idle ) {
      while ( !least_recently_sent_.empty() ) {
        stats.record_idle_flush();
        DVLOG(5) << "idle flush Core " << least_recently_sent_.top_key();
        flush(least_recently_sent_.top_key());
      }
    }
    bool useful = poll(); 
    stats.record_idle_poll(useful);
    return useful;
  }
  

  /// get timestamp. we avoid calling rdtsc for performance
  inline uint64_t get_timestamp() {
    //return previous_timestamp_ + 1;
    Grappa::tick();
    return Grappa::timestamp();
   }

  /// get delayed timestamp. 
  /// TODO: remove. unused.
  inline uint64_t get_previous_timestamp() {
    return previous_timestamp_;
  }

  /// poll communicator. send any aggregated messages that have been sitting for too long
  inline bool poll() {
    GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
#ifdef VTRACE_FULL
    VT_TRACER("poll");
#endif
    stats.record_poll();

    uint64_t beforePoll = aggregator_bundles_received_; 
    global_communicator.poll();
    uint64_t afterPoll = aggregator_bundles_received_;
    bool pollUseful = afterPoll > beforePoll;


    uint64_t ts = get_timestamp();

    uint64_t beforeDeaggregate = aggregator_messages_deaggregated_;
    deaggregate();
    uint64_t afterDeaggregate = aggregator_messages_deaggregated_;
    bool deagUseful = afterDeaggregate > beforeDeaggregate;
    
    // timestamp overflows are silently ignored. 
    // since it would take many many years to see one, I think that's okay for now.
    int num_flushes = 0;
    while( !least_recently_sent_.empty() &&
	   ((-least_recently_sent_.top_priority() + FLAGS_aggregator_autoflush_ticks) < ts) &&
	   ((FLAGS_aggregator_max_flush == 0) || (num_flushes < FLAGS_aggregator_max_flush)) ) {
      stats.record_timeout();
      DVLOG(5) << "timeout for node " << least_recently_sent_.top_key()
	       << ": inserted at " << -least_recently_sent_.top_priority()
	       << " autoflush_ticks " << FLAGS_aggregator_autoflush_ticks
	       << " (current ts " << ts << ")";
      flush( least_recently_sent_.top_key() );                   // send.
      ++num_flushes;
    }
    if( num_flushes > 0 ) stats.record_multiflush();
    bool flushUseful = num_flushes > 0;
    previous_timestamp_ = ts;

    return flushUseful || deagUseful || pollUseful;
  }

  /// what's the largest message we can aggregate?
  inline const size_t max_size() const { return buffer_size_; }

  /// how much space is available for aggregation to this destination?
  inline const size_t remaining_size( Core destination ) const { 
    Core target = get_target_for_node( destination );
    return buffer_size_ - buffers_[ target ].current_position_; 
  }

  /// Aggregate a message. Do not call this directly; instead, call
  /// Grappa_call_on().
  ///
  ///  @param destination core that will receive this message
  ///  @param fn_p function pointer of handler to run on reception
  ///  @param args pointer to arg struct
  ///  @param args_size size in bytes of arg struct
  ///  @param payload pointer to payload buffer
  ///  @param payload_size size in bytes of payload buffer
  inline void aggregate( Core destination, AggregatorAMHandler fn_p,
                         const void * args, const size_t args_size,
                         const void * payload, const size_t payload_size ) {
    GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
#ifdef VTRACE_FULL
    VT_TRACER("aggregate");
#endif
    CHECK( destination < max_nodes_ ) << "destination:" << destination << " max_nodes_:" << max_nodes_;
    Core target = get_target_for_node( destination );
    CHECK( target < max_nodes_ ) << "target:" << target << " max_nodes_:" << max_nodes_;

    // make sure arg struct and payload aren't too big.
    // in the future, this would lead us down a separate code path for large messages.
    // for now, fail.
    size_t total_call_size = Grappa_sizeof_message( args, args_size, payload, payload_size );
    DVLOG(5) << "aggregating " << total_call_size << " bytes to " 
             << destination << "(target " << target << ")";
    CHECK( total_call_size < buffer_size_ ) << "payload_size( " << payload_size << " )"
                                           << "+args_size( " << args_size << " )"
                                           << "+header_size( " << sizeof( AggregatorGenericCallHeader ) << " )"
                                           << "= " << total_call_size << " of max( " << buffer_size_ << " )";

    AggregatorGenericCallHeader header = { reinterpret_cast< uintptr_t >( fn_p ),
					   destination,
					   static_cast<uint16_t>(args_size),
					   static_cast<uint16_t>(payload_size)
#ifdef GRAPPA_TRACE
					   , global_communicator.mycore()
#endif
#ifdef VTRACE_FULL
					   , tag_
#endif
      };

    if( FLAGS_aggregator_enable ) {
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
	DCHECK( buffers_[ target ].fits( total_call_size ));
      }

      // now call must fit, so just insert it
      buffers_[ target ].insert( &header, sizeof( header ) );
      buffers_[ target ].insert( args, args_size );
      buffers_[ target ].insert( payload, payload_size );
      stats.record_aggregation( total_call_size );

    } else {

      // aggregator is disabled; just send message
      char * buf = raw_send_buffer_;
      memcpy( buf, &header, sizeof( header ) );
      buf += sizeof( header );
      memcpy( buf, args, args_size );
      buf += args_size;
      memcpy( buf, payload, payload_size );
      global_communicator.send_immediate_with_payload( target, [] (void * buf, int size) {
          Aggregator_deaggregate_am( buf, size );
        }, raw_send_buffer_, total_call_size );
    }

    // trace fine-grain communication
    if (FLAGS_record_grappa_events) {
      // TODO: good candidate for TAU_CONTEXT_EVENT
#ifdef GRAPPA_TRACE
      int fn_p_tag = aggregator_trace_tag( fn_p );
      TAU_TRACE_SENDMSG(fn_p_tag, destination, args_size + payload_size );
      // TODO: maybe add named communicators for separate function calls?
#endif
#ifdef VTRACE_FULL
      VT_SEND( vt_agg_commid_, tag_, total_call_size );
#endif
  }

  uint64_t ts = get_timestamp();
  least_recently_sent_.update_or_insert( target, -ts );
  previous_timestamp_ = ts;
#ifdef VTRACE_FULL
  tag_ += global_communicator.mycore();
#endif
  DVLOG(5) << "aggregated " << header;
}

};


extern Aggregator global_aggregator;

// TODO: fix this so it works
// log deprecated call sites 
#define Grappa_call_onx( ... )                                           \
  do {                                                                  \
    LOG(WARNING) << "Using old aggregator bypass, which adds additional blocking"; \
    Grappa_call_on_m( __VA_ARGS__ );                                    \
  } while(0)

#define Grappa_call_on_xx( ... )                                         \
  do {                                                                  \
    LOG(WARNING) << "Using old aggregator bypass, which adds additional blocking"; \
    Grappa_call_on_x_m( __VA_ARGS__ );                                  \
  } while(0)


#ifdef ENABLE_RDMA_AGGREGATOR
#include "Message.hpp"
#endif



/// Aggregate a message.
///
///  @param destination core that will receive this message
///  @param fn_p function pointer of handler to run on reception
///  @param args pointer to arg struct
///  @param args_size size in bytes of arg struct
///  @param payload pointer to payload buffer
///  @param payload_size size in bytes of payload buffer
template< typename ArgsStruct >
inline void Grappa_call_on( Core destination, void (* fn_p)(ArgsStruct *, size_t, void *, size_t), 
                              const ArgsStruct * args, const size_t args_size = sizeof( ArgsStruct ),
                              const void * payload = NULL, const size_t payload_size = 0)
{
  LOG(ERROR) << "Do not call this function!";
  exit(1);
  StateTimer::start_communication();
#if defined(OLD_MESSAGES_NEW_AGGREGATOR) && defined(ENABLE_RDMA_AGGREGATOR)
  struct __attribute__((deprecated("Using old aggregator bypass"))) Warning {};
  CHECK_EQ( sizeof(ArgsStruct), args_size ) << "must add special-case for nonstandard ArgsStruct usage";
  typedef typename std::remove_const<ArgsStruct>::type NonConstArgsStruct;
  ArgsStruct& a = *(const_cast<NonConstArgsStruct*>(args)); // HACK to work around some const disagreements at call sites
  if( NULL == payload ) {
    auto m = Grappa::send_message( destination, [a,fn_p] () mutable {
        fn_p( &a, sizeof(a), NULL, 0 );
      });
  } else {
    auto m = Grappa::send_message( destination, [a,fn_p] (void * payload, size_t size) mutable {
        fn_p( &a, sizeof(a), payload, size );
      }, const_cast<void*>(payload), payload_size );
  }
#else
  global_aggregator.aggregate( destination,
                               reinterpret_cast< AggregatorAMHandler >( fn_p ),
                               static_cast< const void * >( args ), args_size,
                               static_cast< const void * >( payload ), payload_size );
#endif
  StateTimer::stop_communication();
}


/// Aggregate a message. Same as Grappa_call_on(), but with a
/// different payload type.
/// TODO: deprecated. remove this.
template< typename ArgsStruct, typename PayloadType >
inline void Grappa_call_on_x( Core destination, void (* fn_p)(ArgsStruct *, size_t, PayloadType *, size_t), 
                               const ArgsStruct * args, const size_t args_size = sizeof( ArgsStruct ),
                               const PayloadType * payload = NULL, const size_t payload_size = 0)
{
  LOG(ERROR) << "Do not call this function!";
  exit(1);
  StateTimer::start_communication();
#if defined(OLD_MESSAGES_NEW_AGGREGATOR) && defined(ENABLE_RDMA_AGGREGATOR)
  struct __attribute__((deprecated("Using old aggregator bypass, which adds additional blocking"))) Warning {};
  //LOG(WARNING) << "Using old aggregator bypass, which adds additional blocking";
  CHECK_EQ( sizeof(ArgsStruct), args_size ) << "must add special-case for nonstandard ArgsStruct usage";
  ArgsStruct& a = *(std::remove_const<ArgsStruct>(args));
  if( NULL == payload ) {
    auto m = Grappa::send_message( destination, [a,fn_p] {
        fn_p( &a, sizeof(a), NULL, 0 );
      });
  } else {
    auto m = Grappa::send_message( destination, [a,fn_p] (void * void_payload, size_t size) {
        PayloadType * p = reinterpret_cast< PayloadType * >( void_payload );
        size_t psize = size / sizeof(PayloadType);
        fn_p( &a, sizeof(a), p, psize );
      }, static_cast< void * >( payload ), payload_size );
  }
#else
  global_aggregator.aggregate( destination,
                               reinterpret_cast< AggregatorAMHandler >( fn_p ),
                               static_cast< const void * >( args ), args_size,
                               static_cast< const void * >( payload ), payload_size );
#endif
  StateTimer::stop_communication();
}

namespace Grappa {
  
  namespace impl {

    /// Poll Grappa aggregation and communication layers.
    static inline void poll()
    {
      global_aggregator.poll();
    #ifdef ENABLE_RDMA_AGGREGATOR
      Grappa::impl::global_rdma_aggregator.poll();
    #endif
    }

    /// Send waiting aggregated messages to a particular destination.
    static inline void flush( Core n )
    {
      global_aggregator.flush( n );
    }

    /// Meant to be called when there's no other work to be done, calls poll, 
    /// flushes any aggregator buffers with anything in them, and deaggregates.
    static inline void idle_flush_poll() {
      global_aggregator.idle_flush_poll();
    }
    
  }
}


