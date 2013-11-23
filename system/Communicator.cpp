
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

// histogram buckets
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_0_to_255_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_256_to_511_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_512_to_767_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_768_to_1023_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_1024_to_1279_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_1280_to_1535_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_1536_to_1791_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_1792_to_2047_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_2048_to_2303_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_2304_to_2559_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_2560_to_2815_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_2816_to_3071_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_3072_to_3327_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_3328_to_3583_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_3584_to_3839_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, comm_3840_to_4095_bytes, 0);

// other metrics
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, communicator_messages, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<uint64_t>, communicator_bytes, 0);
GRAPPA_DEFINE_STAT( SimpleStatistic<double>, communicator_start_time, []() {
    // initialization value
    return Grappa::walltime();
    });
GRAPPA_DEFINE_STAT( CallbackStatistic<double>, communicator_end_time, []() {
    // sampling value
    return Grappa::walltime();
    });

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


void finalise_mpi(void)
{
   int already_finalised;

   MPI_Finalized(&already_finalised);
   if (!already_finalised)
      MPI_Finalize();
}

/// tear down communication layer.
void Communicator::finish(int retval) {
  communication_is_allowed_ = false;
  // Don't call gasnet_exit, since it screws up VampirTrace
  //gasnet_exit( retval );
  // Instead, call MPI_finalize();
  // TODO: when we call this here, boost test crashes for some reason. why?
  //MPI_Finalize();
  // Instead, call it in an atexit handler.
  atexit(finalise_mpi);
}


CommunicatorStatistics::CommunicatorStatistics() 
    : histogram_()
  { 
    histogram_[0] = &comm_0_to_255_bytes;
    histogram_[1] = &comm_256_to_511_bytes;
    histogram_[2] = &comm_512_to_767_bytes;
    histogram_[3] = &comm_768_to_1023_bytes;
    histogram_[4] = &comm_1024_to_1279_bytes;
    histogram_[5] = &comm_1280_to_1535_bytes;
    histogram_[6] = &comm_1536_to_1791_bytes;
    histogram_[7] = &comm_1792_to_2047_bytes;
    histogram_[8] = &comm_2048_to_2303_bytes;
    histogram_[9] = &comm_2304_to_2559_bytes;
    histogram_[10] = &comm_2560_to_2815_bytes;
    histogram_[11] = &comm_2816_to_3071_bytes;
    histogram_[12] = &comm_3072_to_3327_bytes;
    histogram_[13] = &comm_3328_to_3583_bytes;
    histogram_[14] = &comm_3584_to_3839_bytes;
    histogram_[15] = &comm_3840_to_4095_bytes;
  }

void CommunicatorStatistics::reset_clock() {
  communicator_start_time.reset();
}
  
