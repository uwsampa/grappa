////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

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

#include "PerformanceTools.hpp"

#include <mpi.h>
#include <memory>
#include <stack>

#ifdef VTRACE
#include <vt_user.h>
#endif

// GRAPPA_DECLARE_METRIC( SimpleMetric<uint64_t>, communicator_messages);
// GRAPPA_DECLARE_METRIC( SimpleMetric<uint64_t>, communicator_bytes);

#define MPI_CHECK( block )                              \
  do {                                                  \
    PCHECK( (block) == 0 ) << "MPI call failed";        \
  } while(0)


/// Type for Core and Locale IDs. 
typedef int16_t Core;
typedef int16_t Locale;

const static int16_t MAX_CORES_PER_LOCALE = 64;

struct Context {
  MPI_Request request;
  void * buf;
  void (*callback)( int source, int tag, void * buf, size_t size );
  Context(): request(MPI_REQUEST_NULL), buf(NULL), callback(NULL) {}
};


/// Communication layer
class Communicator {
private:
  DISALLOW_COPY_AND_ASSIGN( Communicator );



  

  Core mycore_;
  Core cores_;
  Core mylocale_;
  Core locales_;
  Core locale_mycore_;
  Core locale_cores_;
  
  /// array of core-to-locale translations
  std::unique_ptr< Locale[] > locale_of_core_;

#ifdef VTRACE_FULL
  unsigned communicator_grp_vt;
  unsigned send_ev_vt;
#endif

  std::unique_ptr< Context[] > receives;
  int receive_head;
  int receive_tail;

  std::unique_ptr< Context[] > sends;
  std::stack< Context * > available_sends;

    MPI_Request barrier_request;

public:

  /// Construct communicator. Must call init() before
  /// most methods may be called.
  Communicator( );

  /// Begin setting up communicator.
  void init( int * argc_p, char ** argv_p[] );

  /// Finish setting up communicator. After this, all communicator
  /// methods can be called.
  void activate();

  /// Tear down communicator.
  void finish( int retval = 0 );

  /// Public versions of job geometry
  const Core & mycore;
  const Core & cores;
  const Core & mylocale;
  const Core & locales;
  const Core & locale_mycore;
  const Core & locale_cores;

  /// What locale is responsible for this core?
  inline Locale locale_of( Core c ) const { 
    return locale_of_core_[c];
  }

    const char * hostname();

  Context * get_context();

  void post_send( int dest,
                  void * buf, size_t size,
                  void (*callback)( int source, int tag, void * buf, size_t size ),
                  int tag = 0 );
  void post_receive( void * buf, size_t size,
                     void (*callback)( int source, int tag, void * buf, size_t size ),
                     int tag = MPI_ANY_TAG, int source = MPI_ANY_SOURCE );
  void garbage_collect();
  void poll( unsigned int max_receives = -1 );


  
  /// Global (anonymous) barrier (ALLNODES)
  inline void barrier() {
    MPI_CHECK( MPI_Barrier( MPI_COMM_WORLD ) );
  }

  /// Global (anonymous) two-phase barrier notify (ALLNODES)
  inline void barrier_notify() {
    MPI_CHECK( MPI_Ibarrier( MPI_COMM_WORLD, &barrier_request ) );
  }
  
  /// Global (anonymous) two-phase barrier try (ALLNODES)
  inline bool barrier_try() {
    int flag;
    MPI_CHECK( MPI_Test( &barrier_request, &flag, MPI_STATUS_IGNORE ) );
    return flag;
  }

  void post_receive( void * buf, size_t size, void (*callback)(void), int tag = MPI_ANY_TAG );


  
//   /// Send no-argment active message with payload
//   inline void send( Core node, int handler, void * buf, size_t size ) { 
//     assert( communication_is_allowed_ );
//     assert( size < maximum_message_payload_size ); // make sure payload isn't too big
//     stats.record_message( size );
// #ifdef VTRACE_FULL
//     VT_COUNT_UNSIGNED_VAL( send_ev_vt, size );
// #endif
//     GASNET_CHECK( gasnet_AMRequestMedium0( node, handler, buf, size ) );
//   }

//   /// Send no-argment active message with payload. This only allows
//   /// messages will be immediately copied to the HCA.
//   /// TODO: can we avoid the copy onto gasnet's buffer? This is so small it probably doesn't matter.
//   inline void send_immediate( Core node, int handler, void * buf, size_t size ) { 
//     DCHECK_EQ( communication_is_allowed_, true );
//     CHECK_LT( size, gasnetc_inline_limit ); // make sure payload isn't too big
//     stats.record_message( size );
// #ifdef VTRACE_FULL
//     VT_COUNT_UNSIGNED_VAL( send_ev_vt, size );
// #endif
//     GASNET_CHECK( gasnet_AMRequestMedium0( node, handler, buf, size ) );
//   }

//   /// Send no-argment active message with payload via RDMA, blocking until sent.
//   inline void send( Core node, int handler, void * buf, size_t size, void * dest_buf ) { 
//     DCHECK_EQ( communication_is_allowed_, true );
//     stats.record_message( size );
// #ifdef VTRACE_FULL
//     VT_COUNT_UNSIGNED_VAL( send_ev_vt, size );
// #endif
//     GASNET_CHECK( gasnet_AMRequestLong0( node, handler, buf, size, dest_buf ) );
//   }

//   /// Send no-argment active message with payload via RDMA asynchronously.
//   inline void send_async( Core node, int handler, void * buf, size_t size, void * dest_buf ) { 
//     DCHECK_EQ( communication_is_allowed_, true );
//     stats.record_message( size );
// #ifdef VTRACE_FULL
//     VT_COUNT_UNSIGNED_VAL( send_ev_vt, size );
// #endif
//     GASNET_CHECK( gasnet_AMRequestLongAsync0( node, handler, buf, size, dest_buf ) );
//   }

//   /// poll messaging layer
//   inline void poll() { 
//     //
//   }

};

extern Communicator global_communicator;

namespace Grappa {

/// @addtogroup Communication
/// @{

/// How many cores are there in this job?
inline const Core cores() { return global_communicator.cores; }

/// What's my core ID in this job?
inline const Core mycore() { return global_communicator.mycore; }

/// How many cores are in my shared memory domain?
inline const Core locale_cores() { return global_communicator.locale_cores; }

/// What's my core ID within my shared memory domain?
inline const Core locale_mycore() { return global_communicator.locale_mycore; }

/// How many shared memory domains are in this job?
inline const Locale locales() { return global_communicator.locales; }

/// What's my shared memory domain ID within this job?
inline const Locale mylocale() { return global_communicator.mylocale; }

/// What shared memory domain does core c belong to?
inline const Locale locale_of(Core c) { return global_communicator.locale_of(c); }

/// @}

}

#endif
