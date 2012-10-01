
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

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

#include "StateTimer.hpp"
#include "PerformanceTools.hpp"


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

/// Type of aggregated active message handler
typedef void (* AggregatorAMHandler)( void *, size_t, void *, size_t );

extern void Aggregator_deaggregate_am( gasnet_token_t token, void * buf, size_t size );

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


/// stats class for aggregator
class AggregatorStatistics {
private:
  uint64_t messages_aggregated_;
  uint64_t bytes_aggregated_;
  uint64_t messages_deaggregated_;
  uint64_t bytes_deaggregated_;
  uint64_t messages_forwarded_;
  uint64_t bytes_forwarded_;
  uint64_t newest_wait_ticks_;
  uint64_t oldest_wait_ticks_;
  uint64_t polls_;
  uint64_t flushes_;
  uint64_t multiflushes_;
  uint64_t timeouts_;
  uint64_t idle_flushes_;
  uint64_t capacity_flushes_;
  uint64_t histogram_[16];
  double start_;

#ifdef VTRACE_SAMPLED
  unsigned aggregator_vt_grp;
  unsigned messages_aggregated_vt_ev;
  unsigned bytes_aggregated_vt_ev;
  unsigned messages_deaggregated_vt_ev;
  unsigned bytes_deaggregated_vt_ev;
  unsigned messages_forwarded_vt_ev;
  unsigned bytes_forwarded_vt_ev;
  unsigned newest_wait_ticks_vt_ev;
  unsigned oldest_wait_ticks_vt_ev;
  unsigned average_wait_vt_ev;
  unsigned polls_vt_ev;
  unsigned flushes_vt_ev;
  unsigned multiflushes_vt_ev;
  unsigned timeouts_vt_ev;
  unsigned idle_flushes_vt_ev;
  unsigned capacity_flushes_vt_ev;
  unsigned aggregator_0_to_255_bytes_vt_ev;
  unsigned aggregator_256_to_511_bytes_vt_ev;
  unsigned aggregator_512_to_767_bytes_vt_ev;
  unsigned aggregator_768_to_1023_bytes_vt_ev;
  unsigned aggregator_1024_to_1279_bytes_vt_ev;
  unsigned aggregator_1280_to_1535_bytes_vt_ev;
  unsigned aggregator_1536_to_1791_bytes_vt_ev;
  unsigned aggregator_1792_to_2047_bytes_vt_ev;
  unsigned aggregator_2048_to_2303_bytes_vt_ev;
  unsigned aggregator_2304_to_2559_bytes_vt_ev;
  unsigned aggregator_2560_to_2815_bytes_vt_ev;
  unsigned aggregator_2816_to_3071_bytes_vt_ev;
  unsigned aggregator_3072_to_3327_bytes_vt_ev;
  unsigned aggregator_3328_to_3583_bytes_vt_ev;
  unsigned aggregator_3584_to_3839_bytes_vt_ev;
  unsigned aggregator_3840_to_4095_bytes_vt_ev;
#endif


  std::string hist_labels[16];
  
  /// dump csv stats header 
  /// TODO: remove. unused
  std::ostream& header( std::ostream& o ) {
    o << "AggregatorStatistics, header, time, messages_aggregated, bytes_aggregated, messages_aggregated_per_second, bytes_aggregated_per_second, flushes, timeouts";
    for (int i=0; i<16; i++) o << ", " << hist_labels[i];
    return o;
  }

  /// dump csv stats data
  /// TODO: remove. unused
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
  
  /// dump stats as a ruby map
  std::ostream& as_map( std::ostream& o, double time) {
    double messages_aggregated_per_second = messages_aggregated_ / time;
    double bytes_aggregated_per_second = bytes_aggregated_ / time;
    
    o << "AggregatorStatistics {" ;
    o << "time_aggregated: " << time << ", "
      << "messages_aggregated: " << messages_aggregated_ << ", "
      << "bytes_aggregated: " << bytes_aggregated_ << ", "
      << "messages_aggregated_per_second: " << messages_aggregated_per_second << ", "
      << "bytes_aggregated_per_second: " << bytes_aggregated_per_second << ", "
      << "newest_wait_ticks: " << newest_wait_ticks_ << ", "
      << "oldest_wait_ticks: " << oldest_wait_ticks_ << ", "
      << "average_wait_time: " << (double) (oldest_wait_ticks_ + newest_wait_ticks_) / (2 * flushes_) << ", "
      << "polls: " << polls_ << ", "
      << "flushes: " << flushes_ << ", "
      << "multiflushes: " << multiflushes_ << ", "
      << "timeouts: " << timeouts_ << ", "
      << "idle_flushes: " << idle_flushes_ << ", "
      << "capacity_flushes: " << capacity_flushes_;
    for (int i=0; i<16; i++) {
      o << ", " << hist_labels[i] << ": " << histogram_[i];
    }
    o << " }";
    return o;
  }
  
  /// number of ticks since start_
  double time() {
    double end = SoftXMT_walltime();
    return end-start_;
  }

public:
  AggregatorStatistics()
    : histogram_()
    , start_()
#ifdef VTRACE_SAMPLED
    , aggregator_vt_grp( VT_COUNT_GROUP_DEF( "Aggregator" ) )
    , messages_aggregated_vt_ev( VT_COUNT_DEF( "Total messages aggregated", "messages", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , bytes_aggregated_vt_ev( VT_COUNT_DEF( "Total bytes aggregated", "bytes", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , messages_deaggregated_vt_ev( VT_COUNT_DEF( "Total messages deaggregated", "messages", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , bytes_deaggregated_vt_ev( VT_COUNT_DEF( "Total bytes deaggregated", "bytes", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , messages_forwarded_vt_ev( VT_COUNT_DEF( "Total messages forwarded", "messages", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , bytes_forwarded_vt_ev( VT_COUNT_DEF( "Total bytes forwarded", "bytes", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , newest_wait_ticks_vt_ev( VT_COUNT_DEF( "Aggregator newest wait ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , oldest_wait_ticks_vt_ev( VT_COUNT_DEF( "Aggregator oldest wait ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , average_wait_vt_ev( VT_COUNT_DEF( "Aggregator average wait time", "ticks", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , polls_vt_ev( VT_COUNT_DEF( "Aggregator polls", "polls", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , flushes_vt_ev( VT_COUNT_DEF( "Flushes", "flushes", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , multiflushes_vt_ev( VT_COUNT_DEF( "Nonzero timeout loops", "multiflushes", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , timeouts_vt_ev( VT_COUNT_DEF( "Timeouts", "timeouts", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , idle_flushes_vt_ev( VT_COUNT_DEF( "Idle flushes", "flushes", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , capacity_flushes_vt_ev( VT_COUNT_DEF( "Capacity flushes", "flushes", VT_COUNT_TYPE_UNSIGNED, aggregator_vt_grp ) )
    , aggregator_0_to_255_bytes_vt_ev(     VT_COUNT_DEF(     "Aggregated 0 to 255 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_256_to_511_bytes_vt_ev(   VT_COUNT_DEF(   "Aggregated 256 to 511 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_512_to_767_bytes_vt_ev(   VT_COUNT_DEF(   "Aggregated 512 to 767 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_768_to_1023_bytes_vt_ev(  VT_COUNT_DEF(  "Aggregated 768 to 1023 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_1024_to_1279_bytes_vt_ev( VT_COUNT_DEF( "Aggregated 1024 to 1279 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_1280_to_1535_bytes_vt_ev( VT_COUNT_DEF( "Aggregated 1280 to 1535 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_1536_to_1791_bytes_vt_ev( VT_COUNT_DEF( "Aggregated 1536 to 1791 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_1792_to_2047_bytes_vt_ev( VT_COUNT_DEF( "Aggregated 1792 to 2047 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_2048_to_2303_bytes_vt_ev( VT_COUNT_DEF( "Aggregated 2048 to 2303 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_2304_to_2559_bytes_vt_ev( VT_COUNT_DEF( "Aggregated 2304 to 2559 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_2560_to_2815_bytes_vt_ev( VT_COUNT_DEF( "Aggregated 2560 to 2815 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_2816_to_3071_bytes_vt_ev( VT_COUNT_DEF( "Aggregated 2816 to 3071 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_3072_to_3327_bytes_vt_ev( VT_COUNT_DEF( "Aggregated 3072 to 3327 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_3328_to_3583_bytes_vt_ev( VT_COUNT_DEF( "Aggregated 3328 to 3583 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_3584_to_3839_bytes_vt_ev( VT_COUNT_DEF( "Aggregated 3584 to 3839 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
    , aggregator_3840_to_4095_bytes_vt_ev( VT_COUNT_DEF( "Aggregated 3840 to 4095 bytes", "messages", VT_COUNT_TYPE_DOUBLE, aggregator_vt_grp ) )
#endif
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
    messages_deaggregated_ = 0;
    bytes_deaggregated_ = 0;
    messages_forwarded_ = 0;
    bytes_forwarded_ = 0;
    newest_wait_ticks_ = 0;
    oldest_wait_ticks_ = 0;
    polls_ = 0;
    flushes_ = 0;
    multiflushes_ = 0;
    timeouts_ = 0;
    idle_flushes_ = 0;
    capacity_flushes_ = 0;
    start_ = SoftXMT_walltime();
    for( int i = 0; i < 16; ++i ) {
      histogram_[i] = 0;
    }
  }

  void record_poll() {
    polls_++;
  }

  void record_flush( SoftXMT_Timestamp oldest_ts, SoftXMT_Timestamp newest_ts ) {
    SoftXMT_Timestamp ts = SoftXMT_get_timestamp();
    oldest_wait_ticks_ += ts - oldest_ts;
    newest_wait_ticks_ += ts - newest_ts;
    flushes_++;
  }

  void record_multiflush() {
    multiflushes_++;
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
#ifdef GASNET_CONDUIT_IBV
    histogram_[ (bytes >> 8) & 0xf ]++;
#endif
  }

  void record_deaggregation( size_t bytes ) {
    ++messages_deaggregated_;
    bytes_deaggregated_ += bytes;
  }

  void record_forward( size_t bytes ) {
    ++messages_forwarded_;
    bytes_forwarded_ += bytes;
  }

  void profiling_sample() {
#ifdef VTRACE_SAMPLED
    VT_COUNT_UNSIGNED_VAL( messages_aggregated_vt_ev, messages_aggregated_ );
    VT_COUNT_UNSIGNED_VAL( bytes_aggregated_vt_ev, bytes_aggregated_ );
    VT_COUNT_UNSIGNED_VAL( messages_deaggregated_vt_ev, messages_deaggregated_ );
    VT_COUNT_UNSIGNED_VAL( bytes_deaggregated_vt_ev, bytes_deaggregated_ );
    VT_COUNT_UNSIGNED_VAL( messages_forwarded_vt_ev, messages_forwarded_ );
    VT_COUNT_UNSIGNED_VAL( bytes_forwarded_vt_ev, bytes_forwarded_ );
    VT_COUNT_UNSIGNED_VAL( newest_wait_ticks_vt_ev, newest_wait_ticks_ );
    VT_COUNT_UNSIGNED_VAL( oldest_wait_ticks_vt_ev, oldest_wait_ticks_ );
    VT_COUNT_DOUBLE_VAL( average_wait_vt_ev, (double) (oldest_wait_ticks_ + newest_wait_ticks_) / (2 * flushes_) );
    VT_COUNT_UNSIGNED_VAL( polls_vt_ev, polls_ );
    VT_COUNT_UNSIGNED_VAL( flushes_vt_ev, flushes_ );
    VT_COUNT_UNSIGNED_VAL( multiflushes_vt_ev, multiflushes_ );
    VT_COUNT_UNSIGNED_VAL( timeouts_vt_ev, timeouts_ );
    VT_COUNT_UNSIGNED_VAL( idle_flushes_vt_ev, idle_flushes_ );
    VT_COUNT_UNSIGNED_VAL( capacity_flushes_vt_ev, capacity_flushes_ );
    
#define calc_hist(bin,total) (total == 0) ? 0.0 : (double)bin/total

    VT_COUNT_DOUBLE_VAL( aggregator_0_to_255_bytes_vt_ev,     calc_hist(histogram_[0] , messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_256_to_511_bytes_vt_ev,   calc_hist(histogram_[1] , messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_512_to_767_bytes_vt_ev,   calc_hist(histogram_[2] , messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_768_to_1023_bytes_vt_ev,  calc_hist(histogram_[3] , messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_1024_to_1279_bytes_vt_ev, calc_hist(histogram_[4] , messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_1280_to_1535_bytes_vt_ev, calc_hist(histogram_[5] , messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_1536_to_1791_bytes_vt_ev, calc_hist(histogram_[6] , messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_1792_to_2047_bytes_vt_ev, calc_hist(histogram_[7] , messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_2048_to_2303_bytes_vt_ev, calc_hist(histogram_[8] , messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_2304_to_2559_bytes_vt_ev, calc_hist(histogram_[9] , messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_2560_to_2815_bytes_vt_ev, calc_hist(histogram_[10], messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_2816_to_3071_bytes_vt_ev, calc_hist(histogram_[11], messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_3072_to_3327_bytes_vt_ev, calc_hist(histogram_[12], messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_3328_to_3583_bytes_vt_ev, calc_hist(histogram_[13], messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_3584_to_3839_bytes_vt_ev, calc_hist(histogram_[14], messages_aggregated_) );
    VT_COUNT_DOUBLE_VAL( aggregator_3840_to_4095_bytes_vt_ev, calc_hist(histogram_[15], messages_aggregated_) );
#undef calc_hist
#endif
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
    messages_deaggregated_ += other->messages_deaggregated_;
    bytes_deaggregated_ += other->bytes_deaggregated_;
    messages_forwarded_ += other->messages_forwarded_;
    bytes_forwarded_ += other->bytes_forwarded_;
    newest_wait_ticks_ += other->newest_wait_ticks_;
    oldest_wait_ticks_ += other->oldest_wait_ticks_;
    polls_ += other->polls_;
    flushes_ += other->flushes_;
    multiflushes_ += other->multiflushes_;
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
  SoftXMT_Timestamp oldest_ts_;
  SoftXMT_Timestamp newest_ts_;
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
    newest_ts_ = SoftXMT_get_timestamp();
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

/// Active message aggregation class.
class Aggregator {
private:
  DISALLOW_COPY_AND_ASSIGN( Aggregator );

  /// max node count. used to allocate buffers.
  int max_nodes_;

#ifdef GASNET_CONDUIT_IBV
  /// number of bytes in each aggregation buffer
  /// TODO: this should track the IB MTU
  static const unsigned int buffer_size_ = 4096 - 72;
#endif
#ifdef GASNET_CONDUIT_UDP
  /// number of bytes in each aggregation buffer
  static const unsigned int buffer_size_ = 512 - 72;
#endif
#ifdef GASNET_CONDUIT_MPI
  /// number of bytes in each aggregation buffer
  static const unsigned int buffer_size_ = 65000 - 72;
#endif

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
  std::vector< Node > route_map_;

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
  /// Storage for received pre-deaggregation GASNet active messages
  std::queue< ReceivedAM, std::deque< ReceivedAM, STLMemDebug::Allocator< ReceivedAM > > > received_AM_queue_;
#else
  /// Storage for received pre-deaggregation GASNet active messages
  std::queue< ReceivedAM > received_AM_queue_;
#endif

  /// Deaggregated buffered active messages
  void deaggregate( );
  friend void Aggregator_deaggregate_am( gasnet_token_t token, void * buf, size_t size );

public:
  /// statistics
  AggregatorStatistics stats;  

  /// Construct Aggregator.
  Aggregator( );

  /// Initialize aggregator. 
  void init();

  /// Tear down aggegator.
  ~Aggregator();

  /// Clean up aggregator before destruction.
  void finish();

  /// Dump aggregator stats
  void dump_stats() { stats.dump_as_map(); }
  /// Merge aggregator stats with stats from another core
  void merge_stats();
  /// Reset aggregator stats
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
    Node target = route_map_[ node ];
    stats.record_flush( buffers_[ target ].oldest_ts_, buffers_[ target ].newest_ts_ );
    global_communicator.send( target,
                              aggregator_deaggregate_am_handle_,
                              buffers_[ target ].buffer_,
                              buffers_[ target ].current_position_ );
    buffers_[ target ].flush();
    //DVLOG(5) << "heap before flush:\n" << least_recently_sent_.toString( );
    least_recently_sent_.remove_key( target );
    //DVLOG(5) << "heap after flush:\n" << least_recently_sent_.toString( );
  }
  
  /// poll and optionally flush on idle
  inline void idle_flush_poll() {
    GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
#ifdef VTRACE_FULL
    VT_TRACER("idle_flush_poll");
#endif
    StateTimer::enterState_communication();
    //global_communicator.poll();
    //while ( !least_recently_sent_.empty() ) {
      //stats.record_idle_flush();
      //DVLOG(5) << "idle flush Node " << least_recently_sent_.top_key();
      //flush(least_recently_sent_.top_key());
    //}
    //deaggregate();
    poll();
  }
  

  /// get timestamp. we avoid calling rdtsc for performance
  inline uint64_t get_timestamp() {
    //return previous_timestamp_ + 1;
    SoftXMT_tick();
    return SoftXMT_get_timestamp();
   }

  /// get delayed timestamp. 
  /// TODO: remove. unused.
  inline uint64_t get_previous_timestamp() {
    return previous_timestamp_;
  }

  /// poll communicator. send any aggregated messages that have been sitting for too long.
  inline void poll() {
    GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
#ifdef VTRACE_FULL
    VT_TRACER("poll");
#endif
    stats.record_poll();
    global_communicator.poll();
    uint64_t ts = get_timestamp();
    deaggregate();
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
    previous_timestamp_ = ts;
  }

  /// what's the largest message we can aggregate?
  inline const size_t max_size() const { return buffer_size_; }

  /// how much space is available for aggregation to this destination?
  inline const size_t remaining_size( Node destination ) const { 
    Node target = get_target_for_node( destination );
    return buffer_size_ - buffers_[ target ].current_position_; 
  }

  /// Aggregate a message. Do not call this directly; instead, call
  /// SoftXMT_call_on().
  ///
  ///  @param destination core that will receive this message
  ///  @param fn_p function pointer of handler to run on reception
  ///  @param args pointer to arg struct
  ///  @param args_size size in bytes of arg struct
  ///  @param payload pointer to payload buffer
  ///  @param payload_size size in bytes of payload buffer
  inline void aggregate( Node destination, AggregatorAMHandler fn_p,
                         const void * args, const size_t args_size,
                         const void * payload, const size_t payload_size ) {
    GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
#ifdef VTRACE_FULL
    VT_TRACER("aggregate");
#endif
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

    AggregatorGenericCallHeader header = { reinterpret_cast< intptr_t >( fn_p ),
					   destination,
					   args_size,
					   payload_size
#ifdef GRAPPA_TRACE
					   , global_communicator.mynode()
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
      global_communicator.send( target,
				aggregator_deaggregate_am_handle_,
				raw_send_buffer_,
				total_call_size );
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
  tag_ += global_communicator.mynode();
#endif
  DVLOG(5) << "aggregated " << header;
}

};


extern Aggregator global_aggregator;


/// Aggregate a message.
///
///  @param destination core that will receive this message
///  @param fn_p function pointer of handler to run on reception
///  @param args pointer to arg struct
///  @param args_size size in bytes of arg struct
///  @param payload pointer to payload buffer
///  @param payload_size size in bytes of payload buffer
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

/// Aggregate a message. Same as SoftXMT_call_on(), but with a
/// different payload type.
/// TODO: deprecated. remove this.
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
