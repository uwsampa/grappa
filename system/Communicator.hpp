
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
#include "Metrics.hpp"

#pragma GCC system_header
#include <gasnet.h>
#pragma GCC system_header
#include <gasnet_tools.h>
#include "gasnet_helpers.h"

#include "PerformanceTools.hpp"
#ifdef VTRACE
#include <vt_user.h>
#endif

GRAPPA_DECLARE_METRIC( SimpleMetric<uint64_t>, communicator_messages);
GRAPPA_DECLARE_METRIC( SimpleMetric<uint64_t>, communicator_bytes);

/// Common pointer type for active message handlers used by GASNet
/// library. Actual handlers may have arguments.
typedef void (*HandlerPointer)();    

/// Type for Core and Locale IDs. 
typedef int16_t Core;
typedef int16_t Locale;

const static int16_t MAX_CORES_PER_LOCALE = 64;

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
class CommunicatorMetrics {
private:
  Grappa::SimpleMetric<uint64_t> * histogram_[16];

public:
  CommunicatorMetrics();
  
  void reset_clock();

  // inlined
  void record_message( size_t bytes ) {
    communicator_messages++;
    communicator_bytes+=bytes;
#ifdef GASNET_CONDUIT_IBV
    if( bytes > 4095 ) bytes = 4095;
    (*(histogram_[ (bytes >> 8) & 0xf ]))++;
#endif
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
  CommunicatorMetrics stats;
  
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
  inline void send( Core node, int handler, void * buf, size_t size ) { 
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
  inline void send_immediate( Core node, int handler, void * buf, size_t size ) { 
    DCHECK_EQ( communication_is_allowed_, true );
    CHECK_LT( size, gasnetc_inline_limit ); // make sure payload isn't too big
    stats.record_message( size );
#ifdef VTRACE_FULL
    VT_COUNT_UNSIGNED_VAL( send_ev_vt, size );
#endif
    GASNET_CHECK( gasnet_AMRequestMedium0( node, handler, buf, size ) );
  }

  /// Send no-argment active message with payload via RDMA, blocking until sent.
  inline void send( Core node, int handler, void * buf, size_t size, void * dest_buf ) { 
    DCHECK_EQ( communication_is_allowed_, true );
    stats.record_message( size );
#ifdef VTRACE_FULL
    VT_COUNT_UNSIGNED_VAL( send_ev_vt, size );
#endif
    GASNET_CHECK( gasnet_AMRequestLong0( node, handler, buf, size, dest_buf ) );
  }

  /// Send no-argment active message with payload via RDMA asynchronously.
  inline void send_async( Core node, int handler, void * buf, size_t size, void * dest_buf ) { 
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
inline Core cores() { return global_communicator.cores(); }

/// What's my core ID in this job?
inline Core mycore() { return global_communicator.mycore(); }

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

/// @}

}

#endif
