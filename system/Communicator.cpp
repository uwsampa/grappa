
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <cassert>

#include <gflags/gflags.h>

#ifdef HEAPCHECK_ENABLE
#include <gperftools/heap-checker.h>
extern HeapLeakChecker * Grappa_heapchecker;
#endif

#include "Communicator.hpp"

/// Global communicator instance
Communicator global_communicator;

/// Construct communicator
Communicator::Communicator( )
  : handlers_()
  , registration_is_allowed_( false )
  , communication_is_allowed_( false )
  , mycore_( -1 )
  , mylocale_( -1 )
  , locale_mycore_( -1 )
  , cores_( -1 )
  , locales_( -1 )
  , locale_cores_( -1 )
  , locale_of_core_(NULL)
#ifdef VTRACE_FULL
  , communicator_grp_vt( VT_COUNT_GROUP_DEF( "Communicator" ) )
  , send_ev_vt( VT_COUNT_DEF( "Sends", "bytes", VT_COUNT_TYPE_UNSIGNED, communicator_grp_vt ) )
#endif
  , stats()
{ }


/// initialize communication layer. After this call, node parameters
/// may be queried and handlers may be registered, but no
/// communication is allowed.
void Communicator::init( int * argc_p, char ** argv_p[] ) {
#ifdef HEAPCHECK_ENABLE
  {
    HeapLeakChecker::Disabler disable_leak_checks_here;
#endif
  GASNET_CHECK( gasnet_init( argc_p, argv_p ) ); 
#ifdef HEAPCHECK_ENABLE
  }
#endif
  // make sure the Node type is big enough for our system
  assert( static_cast< int64_t >( gasnet_nodes() ) <= (1L << sizeof(Node) * 8) && 
          "Node type is too small for number of nodes in job" );

  // initialize job geometry
  mycore_ = gasnet_mynode();
  cores_ = gasnet_nodes();

  mylocale_ = gasneti_nodeinfo[mycore_].supernode;
  locales_ = gasnet_nodes() / gasneti_nodemap_local_count;
  locale_mycore_ = gasneti_nodemap_local_rank;
  locale_cores_ = gasneti_nodemap_local_count;

  // allocate and initialize core-to-locale translation
  locale_of_core_ = new Locale[ cores_ ];
  for( int i = 0; i < cores_; ++i ) {
    locale_of_core_[i] = gasneti_nodeinfo[i].supernode;
  }

  LOG(INFO) << " mycore_ " << mycore_ 
            << " cores_ " << cores_
            << " mylocale_ " << mylocale_ 
            << " locales_ " << locales_ 
            << " locale_mycore_ " << locale_mycore_ 
            << " locale_cores_ " << locale_cores_;

  registration_is_allowed_ = true;
}



#define ONE_MEGA (1024 * 1024)
#define SHARED_PROCESS_MEMORY_SIZE  (0 * ONE_MEGA)
#define SHARED_PROCESS_MEMORY_OFFSET (0 * ONE_MEGA)
/// activate communication layer. finishes registering handlers and
/// any shared memory segment. After this call, network communication
/// is allowed, but no more handler registrations are allowed.
void Communicator::activate() {
    assert( registration_is_allowed_ );
  GASNET_CHECK( gasnet_attach( &handlers_[0], handlers_.size(), // install active message handlers
                               SHARED_PROCESS_MEMORY_SIZE,
                               SHARED_PROCESS_MEMORY_OFFSET ) );
  stats.reset_clock();
  registration_is_allowed_ = false;
  communication_is_allowed_ = true;
  LOG(INFO) << "Entering activation barrier";
  barrier();
  LOG(INFO) << "Leaving activation barrier";
}

/// tear down communication layer.
void Communicator::finish(int retval) {
  communication_is_allowed_ = false;
  // TODO: for now, don't call gasnet exit. should we in future?
  //gasnet_exit( retval );
}

