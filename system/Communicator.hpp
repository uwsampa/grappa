
#ifndef __COMMUNICATOR_HPP__
#define __COMMUNICATOR_HPP__

/// Definition of SoftXMT communication layer wrapper class.  This
/// class' functionality is implemented using the GASNet library.
///
/// There are three phases in this class' lifetime. 
///
/// -# after the constructor call, before the init call. No method
/// calls are allowed at this point.
///
/// -# after the init() call. Handlers can be registered, and node
/// parameters can be queried. Communication is not allowed at this
/// point.
///
/// -# after the activate() call. Communication is allowed, and node
/// parameters may be queried. No more handlers may be
/// registered. Applications spend most of their time in this phase.

#include <cassert>
#include <vector>
#include <iostream>
#include <ctime>

#include <glog/logging.h>

#include "common.hpp"

#include <gasnet.h>
#include "gasnet_helpers.h"

/// Common pointer type for active message handlers used by GASNet
/// library. Actual handlers may have arguments.
typedef void (*HandlerPointer)();    

/// Type for Node ID. 
typedef int16_t Node;

/// Maximum size of GASNet medium active message payload. 
///
/// Each GASNet medium message carries a bunch of overhead:
///  - a 4-byte immediate value in the infiniband message that t impact the MTU
///  - 4 bytes of flow control
///  - 4 bytes of payload size
///  - space for up to 16 4-byte arguments, which may be reused for
///    payload when not used for arguments
///
/// GASNETC_MAX_MEDIUM is the maximum payload size allowed for GASNet
/// medium active messages with up to 16 4-byte argments. For
/// Infiniband this will be nearly 4KB, Infiniband's maximum MTU. This
/// is true even if the network is configured with a smaller MTU.
//#define GASNET_NOARG_MAX_MEDIUM (GASNETC_MAX_MEDIUM + sizeof( int32_t ) * 16)

class Communicator;

/// Class for recording Communicator stats
class CommunicatorStatistics {
private:
  uint64_t messages_;
  uint64_t bytes_;
  uint64_t histogram_[16];
  timespec start_;
  std::string hist_labels[16];

  std::ostream& header( std::ostream& o ) {
    o << "CommunicatorStatistics, header, time, messages, bytes, messages_per_second, bytes_per_second";
    for (int i=0; i<16; i++) o << ", " << hist_labels[i];
    return o;
  }

  std::ostream& data( std::ostream& o, double time ) {
    o << "CommunicatorStatistics, data, " << time << ", ";
    double messages_per_second = messages_ / time;
    double bytes_per_second = bytes_ / time;
    o << messages_ << ", " 
      << bytes_ << ", "
      << messages_per_second << ", "
      << bytes_per_second;
    for( int i = 0; i < 16; ++i ) {
      o << ", " << histogram_[ i ];
    }
    return o;
  }
  
  std::ostream& as_map( std::ostream& o, double time) {
    double messages_per_second = messages_ / time;
    double bytes_per_second = bytes_ / time;
    o << "CommunicatorStatistics {"
      << "comm_time: " << time
      << ", messages: " << messages_
      << ", bytes: " << bytes_
      << ", messages_per_second: " << messages_per_second
      << ", bytes_per_second: " << bytes_per_second;
    for (int i=0; i<16; i++) o << ", " << hist_labels[i] << ": " << histogram_[i];
    o << " }";
    return o;
  }
  
  double time() {
    timespec end;
    clock_gettime( CLOCK_MONOTONIC, &end );
    return (end.tv_sec + end.tv_nsec * 0.000000001) - (start_.tv_sec + start_.tv_nsec * 0.000000001);
  }

public:
  CommunicatorStatistics()
    : messages_(0)
    , bytes_(0)
    , histogram_()
    , start_()
  { 
    reset();
    hist_labels[ 0] = "comm_0_to_255_bytes";
    hist_labels[ 1] = "comm_256_to_511_bytes";
    hist_labels[ 2] = "comm_512_to_767_bytes";
    hist_labels[ 3] = "comm_768_to_1023_bytes";
    hist_labels[ 4] = "comm_1024_to_1279_bytes";
    hist_labels[ 5] = "comm_1280_to_1535_bytes";
    hist_labels[ 6] = "comm_1536_to_1791_bytes";
    hist_labels[ 7] = "comm_1792_to_2047_bytes";
    hist_labels[ 8] = "comm_2048_to_2303_bytes";
    hist_labels[ 9] = "comm_2304_to_2559_bytes";
    hist_labels[10] = "comm_2560_to_2815_bytes";
    hist_labels[11] = "comm_2816_to_3071_bytes";
    hist_labels[12] = "comm_3072_to_3327_bytes";
    hist_labels[13] = "comm_3328_to_3583_bytes";
    hist_labels[14] = "comm_3584_to_3839_bytes";
    hist_labels[15] = "comm_3840_to_4095_bytes";
  }
  
  void reset() {
    messages_ = 0;
    bytes_ = 0;
    clock_gettime(CLOCK_MONOTONIC, &start_);
    for( int i = 0; i < 16; ++i ) {
      histogram_[i] = 0;
    }
  }

  void reset_clock() {
    clock_gettime(CLOCK_MONOTONIC, &start_);
  }

  void record_message( size_t bytes ) {
    messages_++;
    bytes_ += bytes;
    histogram_[ (bytes >> 8) & 0xf ]++;
  }
  void dump_csv() {
    header(LOG(INFO));
    data(LOG(INFO), time());
  }
  
  void dump() {
    as_map(std::cout, time());
    std::cout << std::endl;
  }
  
  void merge(CommunicatorStatistics * other) {
    messages_ += other->messages_;
    bytes_ += other->bytes_;
    for (int i=0; i<16; i++) histogram_[i] += other->histogram_[i];
    // pick earlier start time of the two
    start_ = (start_.tv_sec < other->start_.tv_sec && start_.tv_nsec < other->start_.tv_nsec) ? start_ : other->start_;
  }
};

/// Communication layer wrapper class.
class Communicator {
private:
  DISALLOW_COPY_AND_ASSIGN( Communicator );

  /// GASNet allows only a limited number of user-defined active message handlers. This is the first handler ID allowed.
  static const int initial_handler_index = 128;
  /// GASNet allows only a limited number of user-defined active message handlers. This is the last handler ID allowed.
  static const int maximum_handler_index = 255;

  /// used as intermediate storage for active message handlers during the registration process.
  std::vector< gasnet_handlerentry_t > handlers_;

  /// Are we in the phase that allows handler registration?
  bool registration_is_allowed_;

  /// Are we in the phase that allows communication?
  bool communication_is_allowed_;

public:

  /// Record statistics
  CommunicatorStatistics stats;
  
  /// Maximum size of GASNet medium active message payload. 
  ///
  /// Each GASNet medium message carries a bunch of overhead:
  ///  - a 4-byte immediate value in the infiniband message that t impact the MTU
  ///  - 4 bytes of flow control
  ///  - 4 bytes of payload size
  ///  - space for some number of 4-byte arguments, which may be
  ///    reused for payload when not used for arguments
  ///
  /// gasnet_AMMaxMedium() is the maximum payload size allowed for
  /// GASNet medium active messages with up to gasnet_AMMaxArgs()
  /// 4-byte argments. For Infiniband this will be nearly 4KB,
  /// Infiniband's maximum MTU. This is true even if the network is
  /// configured with a smaller MTU, so if your goal is to minimize
  /// network overhead and your network is configured with a smaller
  /// MTU, it may be best to send messages smaller than this.
  static const int maximum_message_payload_size = 
    gasnet_AMMaxMedium() +                  // worst-case maximum, plus
    sizeof( int32_t ) * gasnet_AMMaxArgs(); // the space that would be used for arguments

  /// construct
  Communicator( );

  void init( int * argc_p, char ** argv_p[] );

  /// Register an active message handler with the communication
  /// layer. This is a template method so the user doesn't have to
  /// cast the pointer to the handler pointer type.
  ///
  ///   @param hp Pointer to handler function. Method pointers are not supported.
  ///   @return Handler ID to be used in active message call.
  template< typename HP >
  inline int register_active_message_handler( HP hp ) {
    assert( registration_is_allowed_ );
    int current_handler_index = initial_handler_index + handlers_.size();
    assert( current_handler_index < maximum_handler_index );
    gasnet_handlerentry_t handler = { current_handler_index, 
                                      reinterpret_cast< HandlerPointer >( hp ) };
    handlers_.push_back( handler );
    return current_handler_index;
  }

  void activate();
  void finish( int retval = 0 );

  void dump_stats() { stats.dump(); }
  void merge_stats();
  void reset_stats() { stats.reset(); }

  /// Get id of this node
  inline Node mynode() const { 
    assert( registration_is_allowed_ || communication_is_allowed_ );
    return gasnet_mynode(); 
  }

  /// Get total count of nodes
  inline Node nodes() const { 
    assert( registration_is_allowed_ || communication_is_allowed_ );
    return gasnet_nodes(); 
  }

  /// Global (anonymous) barrier
  inline void barrier() {
    assert( communication_is_allowed_ );
    gasnet_barrier_notify( 0, GASNET_BARRIERFLAG_ANONYMOUS );
    GASNET_CHECK( gasnet_barrier_wait( 0, GASNET_BARRIERFLAG_ANONYMOUS ) );
  }

  /// Global (anonymous) two-phase barrier notify
  inline void barrier_notify() {
    assert( communication_is_allowed_ );
    gasnet_barrier_notify( 0, GASNET_BARRIERFLAG_ANONYMOUS );
  }
  
  /// Global (anonymous) two-phase barrier try
  inline bool barrier_try() {
    return GASNET_OK == gasnet_barrier_try( 0, GASNET_BARRIERFLAG_ANONYMOUS );
  }

  /// Send no-argment active message with payload
  inline void send( Node node, int handler, void * buf, size_t size ) { 
    assert( communication_is_allowed_ );
    assert( size < maximum_message_payload_size ); // make sure payload isn't too big
    stats.record_message( size );
    GASNET_CHECK( gasnet_AMRequestMedium0( node, handler, buf, size ) );
  }

  /// poll messaging layer
  inline void poll() { 
    assert( communication_is_allowed_ );
    GASNET_CHECK( gasnet_AMPoll() ); 
  }
      
};

extern Communicator global_communicator;

#endif
