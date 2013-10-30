
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __COMMUNICATOR_HPP__
#define __COMMUNICATOR_HPP__

/// Definition of Grappa communication layer wrapper class.  This
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

#include <glog/logging.h>

#include "common.hpp"

#include <gasnet.h>
#include <gasnet_tools.h>
#include "gasnet_helpers.h"

#include "PerformanceTools.hpp"
#ifdef VTRACE
#include <vt_user.h>
#endif

/// Common pointer type for active message handlers used by GASNet
/// library. Actual handlers may have arguments.
typedef void (*HandlerPointer)();    

/// Type for Core and Locale IDs. 
typedef int16_t Core;
typedef int16_t Locale;

/// @deprecated
typedef Core Node;

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


/// some variables from gasnet
extern size_t gasnetc_inline_limit;

/* from gasnet_internal.c:
 *   gasneti_nodemap_local_count = number of GASNet nodes collocated w/ gasneti_mynode
 *   gasneti_nodemap_local_rank  = rank of gasneti_mynode among gasneti_nodemap_local_count
 *   gasneti_nodemap_local[]     = array (length gasneti_nodemap_local_count) of local nodes
 *   gasneti_nodemap_global_count = number of unique values in the nodemap
 *   gasneti_nodemap_global_rank  = rank of gasneti_mynode among gasneti_nodemap_global_count
 * and constructs:
 *   gasneti_nodeinfo[] = array of length gasneti_nodes of supernode ids and mmap offsets
 */
extern gasnet_node_t *gasneti_nodemap;
extern gasnet_node_t *gasneti_nodemap_local;
extern gasnet_node_t gasneti_nodemap_local_count;
extern gasnet_node_t gasneti_nodemap_local_rank;
extern gasnet_node_t gasneti_nodemap_global_count;
extern gasnet_node_t gasneti_nodemap_global_rank;
extern gasnet_nodeinfo_t *gasneti_nodeinfo;


class Communicator;

/// Class for recording Communicator stats
class CommunicatorStatistics {
private:
  uint64_t messages_;
  uint64_t bytes_;
  uint64_t histogram_[16];
  double start_;

#ifdef VTRACE_SAMPLED
  unsigned communicator_vt_grp;
  unsigned messages_vt_ev;
  unsigned bytes_vt_ev;
  unsigned communicator_0_to_255_bytes_vt_ev;
  unsigned communicator_256_to_511_bytes_vt_ev;
  unsigned communicator_512_to_767_bytes_vt_ev;
  unsigned communicator_768_to_1023_bytes_vt_ev;
  unsigned communicator_1024_to_1279_bytes_vt_ev;
  unsigned communicator_1280_to_1535_bytes_vt_ev;
  unsigned communicator_1536_to_1791_bytes_vt_ev;
  unsigned communicator_1792_to_2047_bytes_vt_ev;
  unsigned communicator_2048_to_2303_bytes_vt_ev;
  unsigned communicator_2304_to_2559_bytes_vt_ev;
  unsigned communicator_2560_to_2815_bytes_vt_ev;
  unsigned communicator_2816_to_3071_bytes_vt_ev;
  unsigned communicator_3072_to_3327_bytes_vt_ev;
  unsigned communicator_3328_to_3583_bytes_vt_ev;
  unsigned communicator_3584_to_3839_bytes_vt_ev;
  unsigned communicator_3840_to_4095_bytes_vt_ev;
#endif

  static std::string hist_labels[16];

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
    o << "   \"CommunicatorStatistics\": {"
      << "\"comm_time\": " << time
      << ", \"messages\": " << messages_
      << ", \"bytes\": " << bytes_
      << ", \"messages_per_second\": " << messages_per_second
      << ", \"bytes_per_second\": " << bytes_per_second;
    for (int i=0; i<16; i++) o << ", " << hist_labels[i] << ": " << histogram_[i];
    o << " }";
    return o;
  }
  
  double time() {
    double end = Grappa_walltime();
    return end-start_;
  }

public:
  CommunicatorStatistics()
    : messages_(0)
    , bytes_(0)
    , histogram_()
    , start_()
#ifdef VTRACE_SAMPLED
    , communicator_vt_grp( VT_COUNT_GROUP_DEF( "Communicator" ) )
    , messages_vt_ev( VT_COUNT_DEF( "Total messages sent", "messages", VT_COUNT_TYPE_UNSIGNED, communicator_vt_grp ) )
    , bytes_vt_ev( VT_COUNT_DEF( "Total bytes sent", "bytes", VT_COUNT_TYPE_UNSIGNED, communicator_vt_grp ) )
    , communicator_0_to_255_bytes_vt_ev(     VT_COUNT_DEF(     "Raw 0 to 255 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_256_to_511_bytes_vt_ev(   VT_COUNT_DEF(   "Raw 256 to 511 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_512_to_767_bytes_vt_ev(   VT_COUNT_DEF(   "Raw 512 to 767 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_768_to_1023_bytes_vt_ev(  VT_COUNT_DEF(  "Raw 768 to 1023 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_1024_to_1279_bytes_vt_ev( VT_COUNT_DEF( "Raw 1024 to 1279 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_1280_to_1535_bytes_vt_ev( VT_COUNT_DEF( "Raw 1280 to 1535 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_1536_to_1791_bytes_vt_ev( VT_COUNT_DEF( "Raw 1536 to 1791 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_1792_to_2047_bytes_vt_ev( VT_COUNT_DEF( "Raw 1792 to 2047 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_2048_to_2303_bytes_vt_ev( VT_COUNT_DEF( "Raw 2048 to 2303 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_2304_to_2559_bytes_vt_ev( VT_COUNT_DEF( "Raw 2304 to 2559 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_2560_to_2815_bytes_vt_ev( VT_COUNT_DEF( "Raw 2560 to 2815 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_2816_to_3071_bytes_vt_ev( VT_COUNT_DEF( "Raw 2816 to 3071 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_3072_to_3327_bytes_vt_ev( VT_COUNT_DEF( "Raw 3072 to 3327 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_3328_to_3583_bytes_vt_ev( VT_COUNT_DEF( "Raw 3328 to 3583 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_3584_to_3839_bytes_vt_ev( VT_COUNT_DEF( "Raw 3584 to 3839 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
    , communicator_3840_to_4095_bytes_vt_ev( VT_COUNT_DEF( "Raw 3840 to 4095 bytes", "messages", VT_COUNT_TYPE_DOUBLE, communicator_vt_grp ) )
#endif
  { 
    reset();
  }
  
  void reset() {
    messages_ = 0;
    bytes_ = 0;
    start_ = Grappa_walltime();
    for( int i = 0; i < 16; ++i ) {
      histogram_[i] = 0;
    }
  }

  void reset_clock() {
    start_ = Grappa_walltime();
  }

  void record_message( size_t bytes ) {
    messages_++;
    bytes_ += bytes;
#ifdef GASNET_CONDUIT_IBV
    if( bytes > 4095 ) bytes = 4095;
    histogram_[ (bytes >> 8) & 0xf ]++;
#endif
  }

  void profiling_sample() {
#ifdef VTRACE_SAMPLED
#define calc_hist(bin,total) (total == 0) ? 0.0 : (double)bin/total
    VT_COUNT_UNSIGNED_VAL( messages_vt_ev, messages_ );
    VT_COUNT_UNSIGNED_VAL( bytes_vt_ev, bytes_ );
    VT_COUNT_DOUBLE_VAL( communicator_0_to_255_bytes_vt_ev,     calc_hist(histogram_[0] , messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_256_to_511_bytes_vt_ev,   calc_hist(histogram_[1] , messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_512_to_767_bytes_vt_ev,   calc_hist(histogram_[2] , messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_768_to_1023_bytes_vt_ev,  calc_hist(histogram_[3] , messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_1024_to_1279_bytes_vt_ev, calc_hist(histogram_[4] , messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_1280_to_1535_bytes_vt_ev, calc_hist(histogram_[5] , messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_1536_to_1791_bytes_vt_ev, calc_hist(histogram_[6] , messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_1792_to_2047_bytes_vt_ev, calc_hist(histogram_[7] , messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_2048_to_2303_bytes_vt_ev, calc_hist(histogram_[8] , messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_2304_to_2559_bytes_vt_ev, calc_hist(histogram_[9] , messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_2560_to_2815_bytes_vt_ev, calc_hist(histogram_[10], messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_2816_to_3071_bytes_vt_ev, calc_hist(histogram_[11], messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_3072_to_3327_bytes_vt_ev, calc_hist(histogram_[12], messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_3328_to_3583_bytes_vt_ev, calc_hist(histogram_[13], messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_3584_to_3839_bytes_vt_ev, calc_hist(histogram_[14], messages_) );
    VT_COUNT_DOUBLE_VAL( communicator_3840_to_4095_bytes_vt_ev, calc_hist(histogram_[15], messages_) );
#undef calc_hist
#endif
  }

  void dump_csv() {
    header(LOG(INFO));
    data(LOG(INFO), time());
  }
  
  void dump( std::ostream& o = std::cout, const char * terminator = "" ) {
    as_map(o, time());
    o << terminator << std::endl;
  }
  
  void merge(const CommunicatorStatistics * other) {
    messages_ += other->messages_;
    bytes_ += other->bytes_;
    for (int i=0; i<16; i++) histogram_[i] += other->histogram_[i];
    // pick earlier start time of the two
    start_ = MIN(start_, other->start_);
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

  /// store data about gasnet segments on all nodes
  std::vector< gasnet_seginfo_t > segments_;

  /// Are we in the phase that allows handler registration?
  bool registration_is_allowed_;

  /// Are we in the phase that allows communication?
  bool communication_is_allowed_;


  Core mycore_;
  Core cores_;
  Core mylocale_;
  Core locales_;
  Core locale_mycore_;
  Core locale_cores_;
  
  /// array of core-to-locale translations
  Core * locale_of_core_;


#ifdef VTRACE_FULL
  unsigned communicator_grp_vt;
  unsigned send_ev_vt;
#endif

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

  /// Construct communicator. Must call init() and activate() before
  /// most methods may be called.
  Communicator( );

  /// Begin setting up communicator. Must activate() before most
  /// methods can be called.
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
    gasnet_handlerentry_t handler = { static_cast< gasnet_handler_t >( current_handler_index ),
                                      reinterpret_cast< HandlerPointer >( hp ) };
    handlers_.push_back( handler );
    return current_handler_index;
  }

  /// Finish setting up communicator. After this, all communicator
  /// methods can be called.
  void activate();

  void finish( int retval = 0 );

  void dump_stats( std::ostream& o = std::cout, const char * terminator = "" ) { 
    stats.dump( o, terminator );
  }
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





  inline Core mycore() const { 
    return mycore_;
  }
  inline Core cores() const { 
    return cores_;
  }

  inline Locale mylocale() const { 
    return mylocale_;
  }

  inline Locale locales() const { 
    return locales_;
  }

  inline Core locale_cores() const { 
    return locale_cores_;
  }

  inline Core locale_mycore() const { 
    return locale_mycore_;
  }


  inline Locale locale_of( Core c ) const { 
    return locale_of_core_[c];
  }




  inline size_t inline_limit() const {
    return gasnetc_inline_limit;
  }

  /// Global (anonymous) barrier (ALLNODES)
  inline void barrier() {
    assert( communication_is_allowed_ );
    gasnet_barrier_notify( 0, GASNET_BARRIERFLAG_ANONYMOUS );
    GASNET_CHECK( gasnet_barrier_wait( 0, GASNET_BARRIERFLAG_ANONYMOUS ) );
  }

  /// Global (anonymous) two-phase barrier notify (ALLNODES)
  inline void barrier_notify() {
    assert( communication_is_allowed_ );
    gasnet_barrier_notify( 0, GASNET_BARRIERFLAG_ANONYMOUS );
  }
  
  /// Global (anonymous) two-phase barrier try (ALLNODES)
  inline bool barrier_try() {
    return GASNET_OK == gasnet_barrier_try( 0, GASNET_BARRIERFLAG_ANONYMOUS );
  }

  /// Send no-argment active message with payload
  inline void send( Node node, int handler, void * buf, size_t size ) { 
    assert( communication_is_allowed_ );
    assert( size < maximum_message_payload_size ); // make sure payload isn't too big
    stats.record_message( size );
#ifdef VTRACE_FULL
    VT_COUNT_UNSIGNED_VAL( send_ev_vt, size );
#endif
    GASNET_CHECK( gasnet_AMRequestMedium0( node, handler, buf, size ) );
  }

  /// Send no-argment active message with payload. This only allows
  /// messages will be immediately copied to the HCA.
  /// TODO: can we avoid the copy onto gasnet's buffer? This is so small it probably doesn't matter.
  inline void send_immediate( Node node, int handler, void * buf, size_t size ) { 
    DCHECK_EQ( communication_is_allowed_, true );
    CHECK_LT( size, gasnetc_inline_limit ); // make sure payload isn't too big
    stats.record_message( size );
#ifdef VTRACE_FULL
    VT_COUNT_UNSIGNED_VAL( send_ev_vt, size );
#endif
    GASNET_CHECK( gasnet_AMRequestMedium0( node, handler, buf, size ) );
  }

  /// Send no-argment active message with payload via RDMA, blocking until sent.
  inline void send( Node node, int handler, void * buf, size_t size, void * dest_buf ) { 
    DCHECK_EQ( communication_is_allowed_, true );
    stats.record_message( size );
#ifdef VTRACE_FULL
    VT_COUNT_UNSIGNED_VAL( send_ev_vt, size );
#endif
    GASNET_CHECK( gasnet_AMRequestLong0( node, handler, buf, size, dest_buf ) );
  }

  /// Send no-argment active message with payload via RDMA asynchronously.
  inline void send_async( Node node, int handler, void * buf, size_t size, void * dest_buf ) { 
    DCHECK_EQ( communication_is_allowed_, true );
    stats.record_message( size );
#ifdef VTRACE_FULL
    VT_COUNT_UNSIGNED_VAL( send_ev_vt, size );
#endif
    GASNET_CHECK( gasnet_AMRequestLongAsync0( node, handler, buf, size, dest_buf ) );
  }

  /// poll messaging layer
  inline void poll() { 
    GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
    assert( communication_is_allowed_ );
    GASNET_CHECK( gasnet_AMPoll() ); 
  }

  inline const char * hostname() {
    return gasnett_gethostname();
  }
      
};

extern Communicator global_communicator;

namespace Grappa {

/// @addtogroup Communication
/// @{

/// How many cores are there in this job?
inline Core cores() { return global_communicator.nodes(); }

/// What's my core ID in this job?
inline Core mycore() { return global_communicator.mynode(); }

/// How many cores are in my shared memory domain?
inline Core locale_cores() { return global_communicator.locale_cores(); }

/// What's my core ID within my shared memory domain?
inline Core locale_mycore() { return global_communicator.locale_mycore(); }

/// How many shared memory domains are in this job?
inline Locale locales() { return global_communicator.locales(); }

/// What's my shared memory domain ID within this job?
inline Locale mylocale() { return global_communicator.mylocale(); }

/// What shared memory domain does core c belong to?
inline Locale locale_of(Core c) { return global_communicator.locale_of(c); }

/// how big can inline messages be?
inline size_t inline_limit() { return global_communicator.inline_limit(); }

// /// @deprecated How many cores are in this job?
// inline Core nodes() { return global_communicator.supernodes(); }

// /// @deprecated What's my core ID within this job?
// inline Core mynode() { return global_communicator.mysupernode(); }


/// @}

}

#endif
