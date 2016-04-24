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

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "common.hpp"
//#include "Metrics.hpp"

//#include "PerformanceTools.hpp"

#include <mpi.h>
#include <memory>
#include <deque>

#ifdef VTRACE
#include <vt_user.h>
#endif

// GRAPPA_DECLARE_METRIC( SimpleMetric<uint64_t>, communicator_messages);
// GRAPPA_DECLARE_METRIC( SimpleMetric<uint64_t>, communicator_bytes);

/// Type for Core and Locale IDs. 
typedef int16_t Core;
typedef int16_t Locale;

const static int16_t MAX_CORES_PER_LOCALE = 128;

struct CommunicatorContext {
  MPI_Request request;
  void * buf;
  int size;
  int reference_count;
  void (*callback)( CommunicatorContext * c, int source, int tag, int received_size );
  CommunicatorContext(): request(MPI_REQUEST_NULL), buf(NULL), size(0), reference_count(0), callback(NULL) {}
};

namespace Grappa {
namespace impl {

/// generic deserializer type
typedef void (*Deserializer)(char *, int, CommunicatorContext *);

template < typename F >
void immediate_deserializer( char * f, int size, CommunicatorContext * c ) {
  F * obj = reinterpret_cast< F * >( f );
  (*obj)();
  c->reference_count = 0;
}

template < typename F >
void immediate_deserializer_with_payload( char * f, int size, CommunicatorContext * c ) {
  F * obj = reinterpret_cast< F * >( f );
  char * buf = (char*) (obj+1);
  (*obj)( (void*) buf, (f+size) - buf  );
  c->reference_count = 0;
}

}
}


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

  CommunicatorContext * receives;
  int receive_head;      //< pointer to next context that may be done delivering and ready to repost
  int receive_dispatch;  //< pointer to next context that may be done receiving and ready to deliver
  int receive_tail;      //< pointer to next context that may be done delivering and ready to repost
  int receive_mask;

  CommunicatorContext * sends;
  int send_head;
  int send_tail;
  int send_mask;

  MPI_Request barrier_request;
  
  void process_received_buffers();
  void process_collectives();

  std::deque<CommunicatorContext*> external_sends;
  CommunicatorContext * collective_context;
  
public:
  MPI_Comm locale_comm; // locale-local communicator
  MPI_Comm grappa_comm; // grappa-specific communicator

  void garbage_collect();
  void repost_receive_buffers();


  /// Construct communicator. Must call init() before
  /// most methods may be called.
  Communicator( );

  /// Adjust parameters to attempt to make this component use a given amount of memory.
  ///
  /// @param target  memory footprint (in bytes) that this should try to fit in
  /// @return actual size estimate (may be larger than target)
  size_t adjust_footprint(size_t target);
  
  /// Estimate amount of memory the communicator will use when 'activate()' is called
  size_t estimate_footprint() const;
  
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

  inline bool send_context_available() const { return ((send_head + 1) & send_mask) != send_tail; }
  CommunicatorContext * try_get_send_context();


  void post_send( CommunicatorContext * c,
                  int dest,
                  size_t size,
                  int tag = 1 );
  
  void post_external_send( CommunicatorContext * c,
                           int dest,
                           size_t size,
                           int tag = 1 );
  
  void post_receive( CommunicatorContext * c );
  

  template< typename F >
  void send_immediate( int dest, F f ) {
    CommunicatorContext * c = NULL;
    while( NULL == (c = try_get_send_context()) ) {
      garbage_collect();
    }
          
    DVLOG(3) << "Sending immediate " << &f << " to " << dest << " with " << c;
    c->callback = NULL;
    char * buf = (char*) c->buf;

    CHECK_LE( sizeof(Grappa::impl::Deserializer) + sizeof(f), c->size )
      << "Immediate buffer size to small to contain "
      << sizeof(Grappa::impl::Deserializer) << "-byte deserializer + "
      << sizeof(f) << "-byte lambda";

    *((void**)buf) = (void*) Grappa::impl::immediate_deserializer<F>;
    buf += sizeof(Grappa::impl::Deserializer);
    
    memcpy( buf, &f, sizeof(f) );
    buf += sizeof(f);
    
    post_send( c, dest, (buf - ((char*) c->buf)) );
  }

  template< typename F >
  void send_immediate_with_payload( int dest, F f, void * payload, size_t payload_size ) {
    CommunicatorContext * c = NULL;
    while( NULL == (c = try_get_send_context()) ) {
      garbage_collect();
    }
          
    DVLOG(3) << "Sending immediate " << &f << " to " << dest << " with " << c;
    c->callback = NULL;
    char * buf = (char*) c->buf;

    CHECK_LE( sizeof(Grappa::impl::Deserializer) + sizeof(f) + payload_size, c->size )
      << "Immediate buffer size to small to contain "
      << sizeof(Grappa::impl::Deserializer) << "-byte deserializer + "
      << sizeof(f) << "-byte lambda + "
      << payload_size << " payload";

    *((void**)buf) = (void*) Grappa::impl::immediate_deserializer_with_payload<F>;
    buf += sizeof(Grappa::impl::Deserializer);
    
    memcpy( buf, &f, sizeof(f) );
    buf += sizeof(f);

    memcpy( buf, payload, payload_size );
    buf += payload_size;
    
    post_send( c, dest, (buf - ((char*) c->buf)) );
  }

  void poll( unsigned int max_receives = 0 );
  
  template< typename F >
  void with_request_do_blocking( F f );

  
  
  
  /// Global (anonymous) barrier (ALLNODES)
  inline void barrier() {
    MPI_CHECK( MPI_Barrier( grappa_comm ) );
  }

  template< typename T >
  inline void allreduce_inplace(T * p, MPI_Datatype type, MPI_Op op) {
    MPI_CHECK( MPI_Allreduce( MPI_IN_PLACE, p, 1, type, op, grappa_comm ) );
  }

  /// Global (anonymous) two-phase barrier notify (ALLNODES)
  inline void barrier_notify() {
    MPI_CHECK( MPI_Ibarrier( grappa_comm, &barrier_request ) );
  }
  
  /// Global (anonymous) two-phase barrier try (ALLNODES)
  inline bool barrier_try() {
    int flag;
    MPI_CHECK( MPI_Test( &barrier_request, &flag, MPI_STATUS_IGNORE ) );
    return flag;
  }

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

/// What name does MPI think this node has?
inline const char * hostname() { return global_communicator.hostname(); }

/// @}

}

#define MASTER_ONLY if (Grappa::mycore() == 0)

#endif
