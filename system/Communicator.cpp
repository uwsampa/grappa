
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <cassert>

#include <gflags/gflags.h>

#include <mpi.h>

#ifdef HEAPCHECK_ENABLE
#include <gperftools/heap-checker.h>
extern HeapLeakChecker * Grappa_heapchecker;
#endif

#include "Communicator.hpp"

/// Global communicator instance
Communicator global_communicator;
  
/// declare labels for histogram
std::string CommunicatorStatistics::hist_labels[16] = {
    "\"comm_0_to_255_bytes\"",
    "\"comm_256_to_511_bytes\"",
    "\"comm_512_to_767_bytes\"",
    "\"comm_768_to_1023_bytes\"",
    "\"comm_1024_to_1279_bytes\"",
    "\"comm_1280_to_1535_bytes\"",
    "\"comm_1536_to_1791_bytes\"",
    "\"comm_1792_to_2047_bytes\"",
    "\"comm_2048_to_2303_bytes\"",
    "\"comm_2304_to_2559_bytes\"",
    "\"comm_2560_to_2815_bytes\"",
    "\"comm_2816_to_3071_bytes\"",
    "\"comm_3072_to_3327_bytes\"",
    "\"comm_3328_to_3583_bytes\"",
    "\"comm_3584_to_3839_bytes\"",
    "\"comm_3840_to_4095_bytes\"" };

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

  DVLOG(2) << " mycore_ " << mycore_ 
           << " cores_ " << cores_
           << " mylocale_ " << mylocale_ 
           << " locales_ " << locales_ 
           << " locale_mycore_ " << locale_mycore_ 
           << " locale_cores_ " << locale_cores_
           << " pid " << getpid();

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
  DVLOG(3) << "Entering activation barrier";
  barrier();
  DVLOG(3) << "Leaving activation barrier";
}

/// tear down communication layer.
void Communicator::finish(int retval) {
  communication_is_allowed_ = false;
  // Don't call gasnet_exit, since it screws up VampirTrace
  //gasnet_exit( retval );
  // Instead, call MPI_finalize();
  // MPI_Finalize();
}

