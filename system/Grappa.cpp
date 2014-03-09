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

#include <signal.h>

#ifdef HEAPCHECK_ENABLE
#include <gperftools/heap-checker.h>
#endif

#include "GlobalMemory.hpp"
#include "tasks/Task.hpp"
#include "Cache.hpp"
#include "PerformanceTools.hpp"

#include "Collective.hpp"
#include "MetricsTools.hpp"
#include "tasks/StealQueue.hpp"

// #include "tasks/GlobalQueue.hpp"

#include "FileIO.hpp"

#include "RDMAAggregator.hpp"
#include "Barrier.hpp"
#include "LocaleSharedMemory.hpp"
#include "SharedMessagePool.hpp"
#include "Metrics.hpp"

#include <fstream>

#include "Grappa.hpp"

#ifndef SHMMAX
#error "no SHMMAX defined for this system -- look it up with the command: `sysctl -A | grep shm`"
#endif

#ifdef VTRACE
#include <vt_user.h>
#endif

// command line arguments
DEFINE_uint64( num_starting_workers, 512, "Number of starting workers in task-executer pool" );
DEFINE_bool( set_affinity, false, "Set processor affinity based on local rank" );
DEFINE_string( stats_blob_filename, "stats.json", "Stats blob filename" );
DEFINE_bool( stats_blob_enable, true, "Enable stats dumping" );

DEFINE_uint64( io_blocks_per_node, 4, "Maximum number of asynchronous IO operations to issue concurrently per node.");
DEFINE_uint64( io_blocksize_mb, 4, "Size of each asynchronous IO operation's buffer." );

DECLARE_int64( locale_shared_size );
DECLARE_double( global_heap_fraction );

using namespace Grappa::impl;
using namespace Grappa::Metrics;
using namespace Grappa;

/// Flag to tell this node it's okay to exit.
bool Grappa_done_flag;

static int jobid = 0;
static const char * nodelist_str = NULL;

Core * node_neighbors;

#ifdef HEAPCHECK_ENABLE
HeapLeakChecker * Grappa_heapchecker = 0;
#endif

namespace Grappa {
  
  double tick_rate = 0.0;
  
  static Worker * barrier_thread = NULL;

  Worker * master_thread;
  static Worker * user_main_thr;
  
  // defined here so FileIO.hpp doesn't need a .cpp
  IODescriptor * aio_completed_stack;
  
  namespace impl {

    int64_t global_memory_size_bytes = 0;
    int64_t global_bytes_per_core = 0;
    int64_t global_bytes_per_locale = 0;

    /// Tell all nodes that we are ready to exit.
    /// This will terminate the automatic portions of the communication layer
    void signal_done() { 
      VLOG(5) << "mark done";
      Grappa_done_flag = true;
    }
  }

  
}

/// Check whether we are ready to exit.
bool Grappa_done() {
  return Grappa_done_flag;
}

/// Body of the polling thread.
static void poller( Worker * me, void * args ) {
  StateTimer::setThreadState( StateTimer::COMMUNICATION );
  StateTimer::enterState_communication();
  while( !Grappa_done() ) {
    global_scheduler.stats.sample();

    Grappa::impl::poll();
    
    // poll global barrier
    Grappa::impl::barrier_poll();

    // check async. io completions
    if (aio_completed_stack) {
      // atomically grab the stack, replacing it with an empty stack again
      IODescriptor * desc = __sync_lock_test_and_set(&aio_completed_stack, NULL);

      while (desc != NULL) {
        desc->handle_completion();
        IODescriptor * temp = desc->nextCompleted;
        desc->nextCompleted = NULL;
        desc = temp;
      }
    }

    Grappa::yield_periodic();
  }
  // cleanup stragglers on readyQ since I should be last to run;
  // no one else matters.
  // Tasks on task queues would be a programmer error
  global_scheduler.shutdown_readyQ();
  VLOG(5) << "polling Worker exiting";

  // master will be scheduled upon exit of poller thread
}

/// handler to redirect SIGABRT override to activate a GASNet backtrace
static void gasnet_pause_sighandler( int signum ) {
  raise( SIGUSR1 );
}

// from google
namespace google {
typedef void (*override_handler_t)(int);
extern void OverrideDefaultSignalHandler( override_handler_t handler );
extern void DumpStackTrace();
}

/// handler for dumping stats on a signal
static int stats_dump_signal = SIGUSR2;
static void stats_dump_sighandler( int signum ) {
  google::DumpStackTrace();

  Grappa::Metrics::print( LOG(INFO), registered_stats(), "" );

  global_rdma_aggregator.dump_counts();

  // instantaneous state
  LOG(INFO) << global_scheduler;
  LOG(INFO) << global_task_manager;
}

// function to call when google logging library detect a failure
namespace Grappa {
  namespace impl {
    
    bool freeze_on_error = false;
    
    /// called on failures to backtrace and pause for debugger
    void failure_function(int ignore = 0) {
      google::FlushLogFiles(google::GLOG_INFO);
      // google::DumpStackTrace();
      if (freeze_on_error) gasnett_freezeForDebuggerErr();
      gasnet_exit(1);
    }
    
  }
}

DECLARE_bool( global_memory_use_hugepages );

/// Initialize Grappa components. We are not ready to run until the
/// user calls Grappa_activate().
void Grappa_init( int * argc_p, char ** argv_p[], int64_t global_memory_size_bytes)
{
  // std::cerr << "Argc is " << *argc_p << std::endl;
  // for( int i = 0; i < *argc_p; ++i ) {
  //   std::cerr << "Arg " << i << ": " << (*argv_p)[i] << std::endl;
  // }

  // make sure gasnet is ready to backtrace
  // gasnett_backtrace_init( (*argv_p)[0] );

  // help generate unique profile filename
  Grappa::impl::set_exe_name( (*argv_p)[0] );

  // parse command line flags
  google::ParseCommandLineFlags(argc_p, argv_p, true);

  // activate logging
  google::InitGoogleLogging( *argv_p[0] );
  
  char * env_freeze = getenv("GASNET_FREEZE_ON_ERROR");
  if (env_freeze && strncmp(env_freeze,"1",1) == 0) freeze_on_error = true;
  
  google::OverrideDefaultSignalHandler( &Grappa::impl::failure_function );

  DVLOG(1) << "Initializing Grappa library....";
#ifdef HEAPCHECK_ENABLE
  VLOG(1) << "heap check enabled";
  Grappa_heapchecker = new HeapLeakChecker("Grappa");
#endif
  
  char * mem_reg_disabled = getenv("MV2_USE_LAZY_MEM_UNREGISTER");
  if (mem_reg_disabled && strncmp(mem_reg_disabled,"0",1) == 0) {
    VLOG(2) << "memory registration disabled";
  }

  // how fast do we tick?
  Grappa::force_tick();
  Grappa::force_tick();
  Grappa::Timestamp start_ts = Grappa::timestamp();
  double start = Grappa::walltime();
  // now go do other stuff for a while
  
  // set up stats dump signal handler
  struct sigaction stats_dump_sa;
  sigemptyset( &stats_dump_sa.sa_mask );
  stats_dump_sa.sa_flags = 0;
  stats_dump_sa.sa_handler = &stats_dump_sighandler;
  CHECK_EQ( 0, sigaction( stats_dump_signal, &stats_dump_sa, 0 ) ) << "Stats dump signal handler installation failed.";
  struct sigaction sigabrt_sa;
  sigemptyset( &sigabrt_sa.sa_mask );
  sigabrt_sa.sa_flags = 0;
  sigabrt_sa.sa_handler = &gasnet_pause_sighandler;
  // CHECK_EQ( 0, sigaction( SIGABRT, &sigabrt_sa, 0 ) ) << "SIGABRT signal handler installation failed.";

  // Asynchronous IO
  // initialize completed stack
  aio_completed_stack = NULL;

  // handler
#ifdef AIO_SIGNAL
  struct sigaction aio_sa;
  aio_sa.sa_flags = SA_RESTART | SA_SIGINFO;
  aio_sa.sa_sigaction = Grappa::impl::handle_async_io;
  if (sigaction(AIO_SIGNAL, &aio_sa, NULL) == -1) {
    fprintf(stderr, "Error setting up async io signal handler.\n");
    exit(1);
  }
#endif

  // initializes system_wide global_communicator
  global_communicator.init( argc_p, argv_p );
  
  CHECK( global_communicator.locale_cores() <= MAX_CORES_PER_LOCALE );
  
  //  initializes system_wide global_aggregator
  global_aggregator.init();

  // set CPU affinity if requested
#ifdef CPU_SET
  if( FLAGS_set_affinity ) {
    char * localid_str = getenv("SLURM_LOCALID");
    if( NULL != localid_str ) {
      int localid = atoi( localid_str );
      cpu_set_t mask;
      CPU_ZERO( &mask );
      CPU_SET( localid, &mask );
      sched_setaffinity( 0, sizeof(mask), &mask );
    }
  }
#endif

  // initialize node shared memory
  locale_shared_memory.init();

  // by default, will allocate as much shared memory as it is
  // possible to evenly split among the processors on a node
  if (global_memory_size_bytes == -1) {

    // Decide how much memory we should allocate for global shared heap.
    // TODO: this should be a long literal
    double shmmax_fraction = static_cast< double >( SHMMAX ) * FLAGS_global_heap_fraction;
    int64_t shmmax_adjusted_floor = static_cast< int64_t >( shmmax_fraction );

    int64_t nnode = global_communicator.locales();
    int64_t ppn = global_communicator.locale_cores();
    
    int64_t bytes_per_proc = shmmax_adjusted_floor / ppn;
    // round down to page size so we don't ask for too much?
    bytes_per_proc &= ~( (1L << 12) - 1 );
    
    // be aware of hugepages
    // Each core should ask for a multiple of 1GB hugepages
    // and the whole node should ask for no more than the total pages available
    if ( FLAGS_global_memory_use_hugepages ) {
      int64_t pages_per_proc = bytes_per_proc / (1L << 30);
      int64_t new_bpp = pages_per_proc * (1L << 30);
      if (new_bpp == 0) {
        VLOG(1) << "Allocating 1GB per core anyway.";
        new_bpp = 1L << 30;
      }
      VLOG_IF(1, bytes_per_proc != new_bpp) << "With ppn=" << ppn << ", can only allocate " 
                                            << pages_per_proc*ppn << " / " << SHMMAX / (1L << 30) << " 1GB huge pages per node";
      bytes_per_proc = new_bpp;
    }

    int64_t bytes = nnode * ppn * bytes_per_proc;
    int64_t bytes_per_node = ppn * bytes_per_proc;
    DVLOG(2) << "bpp = " << bytes_per_proc << ", bytes = " << bytes << ", bytes_per_node = " << bytes_per_node
             << ", SHMMAX = " << SHMMAX << ", shmmax_adjusted_floor = " << shmmax_adjusted_floor;
    VLOG(1) << "nnode: " << nnode << ", ppn: " << ppn << ", iBs/node: " << log2((double)bytes_per_node) << ", total_iBs: " << log2((double)bytes);
    global_memory_size_bytes = bytes;

    Grappa::impl::global_memory_size_bytes = global_memory_size_bytes;
    Grappa::impl::global_bytes_per_core = bytes_per_proc;
    Grappa::impl::global_bytes_per_locale = bytes_per_node;
  } else {
    Grappa::impl::global_memory_size_bytes = global_memory_size_bytes;
    Grappa::impl::global_bytes_per_core = global_memory_size_bytes / cores();
    Grappa::impl::global_bytes_per_locale = global_memory_size_bytes / locales();
  }
  
  VLOG(1) << "global_memory_size_bytes = " << Grappa::impl::global_memory_size_bytes;
  VLOG(1) << "global_bytes_per_core = " << Grappa::impl::global_bytes_per_core;
  VLOG(1) << "global_bytes_per_locale = " << Grappa::impl::global_bytes_per_locale;

  Grappa_done_flag = false;

  // process command line args for Tau
  //TAU_INIT( argc_p, argv_p );
#ifdef GRAPPA_TRACE
  TAU_PROFILE_SET_NODE(Grappa::mycore());
#endif

  //TODO: options for local stealing
  node_neighbors = new Core[Grappa::cores()];
  for ( Core nod=0; nod < Grappa::cores(); nod++ ) {
    node_neighbors[nod] = nod;
  }
  
  // start threading layer
  master_thread = convert_to_master();
  VLOG(1) << "Initializing tasking layer."
           << " num_starting_workers=" << FLAGS_num_starting_workers;
  global_task_manager.init( Grappa::mycore(), node_neighbors, Grappa::cores() ); //TODO: options for local stealing
  global_scheduler.init( master_thread, &global_task_manager );
  
  // start RDMA Aggregator *after* threading layer
  global_rdma_aggregator.init();
  
  // collect some stats on this job
  Grappa::force_tick();
  Grappa::force_tick();
  Grappa::Timestamp end_ts = Grappa::timestamp();
  double end = Grappa::walltime();
  Grappa::tick_rate = (double) (end_ts - start_ts) / (end-start);

  char * jobid_str = getenv("SLURM_JOB_ID");
  jobid = jobid_str ? atoi(jobid_str) : 0;
  nodelist_str = getenv("SLURM_NODELIST");
  if( NULL == nodelist_str ) nodelist_str = "undefined";
}


/// Activate Grappa network layer and enter barrier. After this,
/// arbitrary communication is allowed.
void Grappa_activate() 
{
  DVLOG(1) << "Activating Grappa library....";
  global_communicator.activate();
  google::InstallFailureSignalHandler();
  
  locale_shared_memory.activate();
  global_task_manager.activate();
  Grappa::comm_barrier();

  // initializes system_wide global_memory pointer
  global_memory = new GlobalMemory( Grappa::impl::global_memory_size_bytes );

  // fire up polling thread
  global_scheduler.periodic( impl::worker_spawn( master_thread, &global_scheduler, &poller, NULL ) );


  global_rdma_aggregator.activate();
  
  Grappa::init_shared_pool(); // (must be after locale-heap is initialized in RDMAAggregator)
  
  if (Grappa::mycore() == 0) {
    size_t stack_sz = FLAGS_stack_size * FLAGS_num_starting_workers;
    double stack_sz_gb = static_cast<double>(stack_sz) / (1L<<30);
    double gheap_sz_gb = static_cast<double>(global_bytes_per_core) / (1L<<30);
    size_t shpool_sz = FLAGS_shared_pool_max * FLAGS_shared_pool_size;
    double shpool_sz_gb = static_cast<double>(shpool_sz) / (1L<<30);
    size_t free_sz = Grappa::impl::locale_shared_memory.get_free_memory() / Grappa::locale_cores();
    double free_sz_gb = static_cast<double>(free_sz) / (1L<<30);
    VLOG(1) << "\n-------------------------\nShared memory breakdown:\n  global heap: " << global_bytes_per_core << " (" << gheap_sz_gb << " GB)\n  stacks: " << stack_sz << " (" << stack_sz_gb << " GB)\n  rdma_aggregator: ??\n  shared_message_pool: " << shpool_sz << " (" << shpool_sz_gb << " GB)\n  free:  " << free_sz << " (" << free_sz_gb << " GB)\n-------------------------";
  }
  
  Grappa::comm_barrier();
}


static bool global_queue_initialized = false;

/// Initialize global queue for load balancing.
/// Must be called in user_main
void Grappa_global_queue_initialize() {
  // if ( global_task_manager.global_queue_on() ) {
  //   on_all_cores([]{
  //     GlobalQueue<Task>::global_queue.init();
  //     global_queue_initialized = true;
  //   });
  // }
}

bool Grappa_global_queue_isInit() {
  return global_queue_initialized;
}


///
/// Job exit routines
///

//// termination fork join declaration
//LOOP_FUNCTION(signal_task_termination_func,nid) {
//    global_task_manager.signal_termination();
//}
static void signal_task_termination_am( int * ignore, size_t isize, void * payload, size_t payload_size ) {
    global_task_manager.signal_termination();
}

/// User main done
void Grappa_end_tasks() {
  // send task termination signal
  CHECK( Grappa::mycore() == 0 );
  for ( Core n = 1; n < Grappa::cores(); n++ ) {
      int ignore = 0;
      Grappa_call_on( n, &signal_task_termination_am, &ignore );
      Grappa::flush( n );
  }
  signal_task_termination_am( NULL, 0, NULL, 0 );
}


///// Active message to tell this node it's okay to exit.
//static void Grappa_mark_done_am( void * args, size_t args_size, void * payload, size_t payload_size ) {
//  VLOG(5) << "mark done";
//  Grappa_done_flag = true;
//}

/// Finish the job. 
/// 
/// If we've already been notified that we can exit, enter global
/// barrier and then clean up. If we have not been notified, then
/// notify everyone else, enter the barrier, and then clean up.
int Grappa_finish( int retval )
{
  Grappa::impl::signal_done(); // this may be overkill (just set done bit?)

  //TAU_PROFILE_EXIT("Tau_profile_exit called");
  Grappa::comm_barrier();

  DVLOG(1) << "Cleaning up Grappa library....";

  StateTimer::finish();

  global_task_manager.finish();
  global_aggregator.finish();

  if (global_memory) delete global_memory;
  locale_shared_memory.finish();

  global_communicator.finish( retval );
 
//  Grappa_dump_stats();

  // probably never get here (depending on communication layer)

  destroy_thread( master_thread );

#ifdef HEAPCHECK_ENABLE
  assert( Grappa_heapchecker->NoLeaks() );
#endif

  return retval;
}

namespace Grappa {

  void init( int * argc_p, char ** argv_p[], int64_t size ) {
    Grappa_init( argc_p, argv_p, size );
    Grappa_activate();
  }

  int finalize() {
    return Grappa_finish(0);
  }

} // namespace Grappa
