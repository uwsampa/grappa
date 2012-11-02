
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <signal.h>

#ifdef HEAPCHECK
#include <gperftools/heap-checker.h>
#endif

#include "Grappa.hpp"
#include "GlobalMemory.hpp"
#include "tasks/Task.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"
#include "PerformanceTools.hpp"
#include "tasks/GlobalQueue.hpp"
#include "Collective.hpp"
#include "StatisticsTools.hpp"
#include "tasks/StealQueue.hpp"
#include "tasks/GlobalQueue.hpp"

#include <fstream>

#ifndef SHMMAX
#error "no SHMMAX defined for this system -- look it up with the command: `sysctl -A | grep shm`"
#endif

#ifdef VTRACE
#include <vt_user.h>
#endif

// command line arguments
DEFINE_uint64( num_starting_workers, 4, "Number of starting workers in task-executer pool" );
DEFINE_bool( set_affinity, false, "Set processor affinity based on local rank" );
DEFINE_string( stats_blob_filename, "stats.json", "Stats blob filename" );

DECLARE_bool( global_queue );

static Thread * barrier_thread = NULL;

Thread * master_thread;
static Thread * user_main_thr;

/// Flag to tell this node it's okay to exit.
bool Grappa_done_flag;

double tick_rate = 0.0;
static int jobid = 0;
static const char * nodelist_str = NULL;

Node * node_neighbors;

#ifdef HEAPCHECK
HeapLeakChecker * Grappa_heapchecker = 0;
#endif

/// Sample all stats for VampirTrace
void Grappa_take_profiling_sample() {
  global_aggregator.stats.profiling_sample();
  global_communicator.stats.profiling_sample();
  global_task_manager.stats.profiling_sample();
  global_scheduler.stats.profiling_sample();
  delegate_stats.profiling_sample();
  cache_stats.profiling_sample();
  incoherent_acquirer_stats.profiling_sample();
  incoherent_releaser_stats.profiling_sample();
  steal_queue_stats.profiling_sample();
  global_queue_stats.profiling_sample();

  // print user-registered stats
  Grappa_profiling_sample_user();
}

/// Body of the polling thread.
static void poller( Thread * me, void * args ) {
  StateTimer::setThreadState( StateTimer::COMMUNICATION );
  StateTimer::enterState_communication();
  while( !Grappa_done() ) {
    global_scheduler.stats.sample();
    global_task_manager.stats.sample();

    Grappa_poll();
    if (barrier_thread) {
      if (global_communicator.barrier_try()) {
        Grappa_wake(barrier_thread);
        barrier_thread = NULL;
      }
    }
    Grappa_yield_periodic();
  }
  VLOG(5) << "polling Thread exiting";
}

/// handler for dumping stats on a signal
static int stats_dump_signal = SIGUSR2;
static void stats_dump_sighandler( int signum ) {
  // TODO: make this set a flag and have scheduler check and dump.
  Grappa_dump_stats();

  // instantaneous state
  LOG(INFO) << global_scheduler;
  LOG(INFO) << global_task_manager;
}

/// handler to redirect SIGABRT override to activate a GASNet backtrace
static void sigabrt_sighandler( int signum ) {
  raise( SIGUSR1 );
}

DECLARE_bool( global_memory_use_hugepages );

/// Initialize Grappa components. We are not ready to run until the
/// user calls Grappa_activate().
void Grappa_init( int * argc_p, char ** argv_p[], size_t global_memory_size_bytes)
{
  // std::cerr << "Argc is " << *argc_p << std::endl;
  // for( int i = 0; i < *argc_p; ++i ) {
  //   std::cerr << "Arg " << i << ": " << (*argv_p)[i] << std::endl;
  // }
  // help generate unique profile filename
  Grappa_set_profiler_argv0( (*argv_p)[0] );

  // parse command line flags
  google::ParseCommandLineFlags(argc_p, argv_p, true);

  // activate logging
  google::InitGoogleLogging( *argv_p[0] );
  google::InstallFailureSignalHandler( );

  DVLOG(1) << "Initializing Grappa library....";
#ifdef HEAPCHECK
  Grappa_heapchecker = new HeapLeakChecker("Grappa");
#endif

  // how fast do we tick?
  Grappa_tick();
  Grappa_tick();
  Grappa_Timestamp start_ts = Grappa_get_timestamp();
  double start = Grappa_walltime();
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
  sigabrt_sa.sa_handler = &sigabrt_sighandler;
  CHECK_EQ( 0, sigaction( SIGABRT, &sigabrt_sa, 0 ) ) << "SIGABRT signal handler installation failed.";

  // initialize Tau profiling groups
  generate_profile_groups();

  // initializes system_wide global_communicator
  global_communicator.init( argc_p, argv_p );

  //  initializes system_wide global_aggregator
  global_aggregator.init();

  // set CPU affinity if requested
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


  // by default, will allocate as much shared memory as it is
  // possible to evenly split among the processors on a node
  if (global_memory_size_bytes == -1) {
    // TODO: this should be a long literal
    int64_t shmmax_gb = SHMMAX; // make sure it's a long literal
    // seems to work better with salloc
    char * nnodes_str = getenv("SLURM_JOB_NUM_NODES");
    // if not, try the one that srun sets
    if( NULL == nnodes_str ) nnodes_str = getenv("SLURM_NNODES");
    int64_t nnode = atoi(nnodes_str);
    int64_t ppn = atoi(getenv("SLURM_NTASKS_PER_NODE"));
    int64_t bytes_per_proc = SHMMAX / ppn;
    // round down to page size so we don't ask for too much?
    bytes_per_proc &= ~( (1L << 12) - 1 );

    // be aware of hugepages
    // Each core should ask for a multiple of 1GB hugepages
    // and the whole node should ask for no more than the total pages available
    if ( FLAGS_global_memory_use_hugepages ) {
      int64_t pages_per_proc = bytes_per_proc / (1L << 30);
      int64_t new_bpp = pages_per_proc * (1L << 30);
      VLOG_IF(1, bytes_per_proc != new_bpp) << "With ppn=" << ppn << ", can only allocate " 
                                            << pages_per_proc*ppn << " / " << SHMMAX / (1L << 30) << " 1GB huge pages per node";
      bytes_per_proc = new_bpp;
    }

    int64_t bytes = nnode * ppn * bytes_per_proc;
    int64_t bytes_per_node = ppn * bytes_per_proc;
    DVLOG(2) << "bpp = " << bytes_per_proc << ", bytes = " << bytes << ", bytes_per_node = " << bytes_per_node << ", SHMMAX = " << SHMMAX;
    VLOG(1) << "nnode: " << nnode << ", ppn: " << ppn << ", iBs/node: " << log2((double)bytes_per_node) << ", total_iBs: " << log2((double)bytes);
    global_memory_size_bytes = bytes;
  }

  VLOG(1) << "global_memory_size_bytes = " << global_memory_size_bytes;

  // initializes system_wide global_memory pointer
  global_memory = new GlobalMemory( global_memory_size_bytes );

  Grappa_done_flag = false;

  // process command line args for Tau
  //TAU_INIT( argc_p, argv_p );
#ifdef GRAPPA_TRACE
  TAU_PROFILE_SET_NODE(Grappa_mynode());
#endif

  //TODO: options for local stealing
  node_neighbors = new Node[Grappa_nodes()];
  for ( Node nod=0; nod < Grappa_nodes(); nod++ ) {
    node_neighbors[nod] = nod;
  }

  // start threading layer
  master_thread = thread_init();
  VLOG(1) << "Initializing tasking layer."
           << " num_starting_workers=" << FLAGS_num_starting_workers;
  global_task_manager.init( Grappa_mynode(), node_neighbors, Grappa_nodes() ); //TODO: options for local stealing
  global_scheduler.init( master_thread, &global_task_manager );
  global_scheduler.periodic( thread_spawn( master_thread, &global_scheduler, &poller, NULL ) );

  // collect some stats on this job
  Grappa_tick();
  Grappa_Timestamp end_ts = Grappa_get_timestamp();
  double end = Grappa_walltime();
  tick_rate = (double) (end_ts - start_ts) / (end-start);

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
  Grappa_barrier();
}

/// Split-phase barrier. (ALLNODES)
void Grappa_barrier_suspending() {
  global_communicator.barrier_notify();
  barrier_thread = CURRENT_THREAD;
  Grappa_suspend();
}


///
/// Thread management routines
///

/// Spawn a user function. TODO: get return values working
/// TODO: remove Thread * arg
inline Thread * Grappa_spawn( void (* fn_p)(Thread *, void *), void * args )
{
  Thread * th = thread_spawn( global_scheduler.get_current_thread(), &global_scheduler, fn_p, args );
  global_scheduler.ready( th );
  DVLOG(5) << "Spawned Thread " << th;
  return th;
}


static bool global_queue_initialized = false;
// fork-join function for Grappa_initialize_global_queue
LOOP_FUNCTION( initialize_global_queue_func, nid ) {
  GlobalQueue<Task>::global_queue.init();
  global_queue_initialized = true;
}

/// Initialize global queue for load balancing.
/// Must be called in user_main
void Grappa_global_queue_initialize() {
  if ( global_task_manager.global_queue_on() ) {
    initialize_global_queue_func f;
    fork_join_custom( &f );
  }
}

bool Grappa_global_queue_isInit() {
  return global_queue_initialized;
}


///
/// Job exit routines
///

/// Check whether we are ready to exit.
bool Grappa_done() {
  return Grappa_done_flag;
}

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
  CHECK( Grappa_mynode() == 0 );
  for ( Node n = 1; n < Grappa_nodes(); n++ ) {
      int ignore = 0;
      Grappa_call_on( n, &signal_task_termination_am, &ignore );
      Grappa_flush( n );
  }
  signal_task_termination_am( NULL, 0, NULL, 0 );
}


///// Active message to tell this node it's okay to exit.
//static void Grappa_mark_done_am( void * args, size_t args_size, void * payload, size_t payload_size ) {
//  VLOG(5) << "mark done";
//  Grappa_done_flag = true;
//}

/// Tell all nodes that we are ready to exit.
/// This will terminate the automatic portions of the communication layer
void Grappa_signal_done ( ) { 
    VLOG(5) << "mark done";
    Grappa_done_flag = true;
}


/// Dump the state of all flags
void dump_flags( std::ostream& o, const char * delimiter ) {
  typedef std::vector< google::CommandLineFlagInfo > FlagVec;
  FlagVec flags;
  google::GetAllFlags( &flags );
  o << "  \"Flags\": { ";
  for( FlagVec::iterator i = flags.begin(); i != flags.end(); ++i ) {
    o << "\"FLAGS_" << i->name << "\": \"" << i->current_value << "\"";
    if( i+1 != flags.end() ) o << ", ";
  }
  o << " }" << delimiter << std::endl;
}


/// Reset stats son this node
void Grappa_reset_stats() {
  global_aggregator.reset_stats();
  global_communicator.reset_stats();
  global_scheduler.reset_stats();
  global_task_manager.reset_stats();
  delegate_stats.reset();
  cache_stats.reset();
  incoherent_acquirer_stats.reset();
  incoherent_releaser_stats.reset();
  steal_queue_stats.reset();
  global_queue_stats.reset();
 
  Grappa_reset_user_stats(); 
}

/// Functor to reset stats on all nodes
LOOP_FUNCTION(reset_stats_func,nid) {
  Grappa_reset_stats();
}
/// Reset stats on all nodes
void Grappa_reset_stats_all_nodes() {
  reset_stats_func f;
  fork_join_custom(&f);
}


/// Dump statistics
void Grappa_dump_stats( std::ostream& oo ) {
  std::ostringstream o;
  o << "STATS{\n";
  o << "   \"GrappaStats\": { \"tick_rate\": " << tick_rate
    << ", \"job_id\": " << jobid
    << ", \"nodelist\": \"" << nodelist_str << "\""
    << " },\n";
  global_aggregator.dump_stats( o, "," );
  global_communicator.dump_stats( o, "," );
  global_task_manager.dump_stats( o, "," );
  global_scheduler.dump_stats( o, "," );
  delegate_stats.dump( o, "," );
  cache_stats.dump( o, "," );
  incoherent_acquirer_stats.dump( o, "," );
  incoherent_releaser_stats.dump( o, "," );
  steal_queue_stats.dump( o, "," );
  global_queue_stats.dump( o, "," );
  dump_flags( o, "," );
  Grappa_dump_user_stats( o, "" ); // TODO: user stats are NOT merged
  o << "}STATS";
  oo << o.str();
}

/// Dump stats blob
void Grappa_dump_stats_blob() {
  std::ofstream o( FLAGS_stats_blob_filename.c_str(), std::ios::out );
  Grappa_dump_stats( o );
}
 

/// Functor to dump stats on all nodes
LOOP_FUNCTION(dump_stats_func,nid) {
  Grappa_dump_stats();
}
/// Dump stats on all nodes
void Grappa_dump_stats_all_nodes() {
  dump_stats_func f;
  fork_join_custom(&f);
}


///
/// Statistics reduction
///
#define STAT_REDUCE(statType, stat) (statType) Grappa_allreduce_noinit<statType, stat_reduce<statType> >( (stat) )
#define STAT_FUNC( name, statType, stat ) \
  LOOP_FUNCTOR( name, nid, ((statType*, resultAddress)) ) { \
    statType result = STAT_REDUCE( statType, (stat) ); \
    if ( nid == 0 ) { \
      *resultAddress = result; \
    } \
  }
#define STAT_FORK_AND_DUMP(name, statType, o) { \
  statType result; \
  name f; \
  f.resultAddress = &result; \
  fork_join_custom(&f); \
  result.dump( (o), "," ); \
}

// define the forkjoin calls to Grappa_allreduce
STAT_FUNC(schedulerstat_func, TaskingScheduler::TaskingSchedulerStatistics, global_scheduler.stats );
STAT_FUNC(aggregatorstat_func, AggregatorStatistics, global_aggregator.stats );
STAT_FUNC(commstat_func, CommunicatorStatistics, global_communicator.stats );
STAT_FUNC(taskmanagerstat_func, TaskManager::TaskStatistics, global_task_manager.stats );
STAT_FUNC(stealstat_func, StealStatistics, steal_queue_stats );
STAT_FUNC(delegatestat_func, DelegateStatistics, delegate_stats );
STAT_FUNC(cachestat_func, CacheStatistics, cache_stats );
STAT_FUNC(incoherentacq_func, IAStatistics, incoherent_acquirer_stats );
STAT_FUNC(incoherentrel_func, IRStatistics, incoherent_releaser_stats );
STAT_FUNC(globalqueuestat_func, GlobalQueueStatistics, global_queue_stats );

// call the forkjoin functions to reduce and print each statistic object
static void reduce_stats_and_dump( std::ostream& oo ) {
  CHECK( Grappa_mynode() == 0 );
 
  std::ostringstream o;
  o << "STATS{\n";
  o << "   \"GrappaStats\": { \"tick_rate\": " << tick_rate
    << ", \"job_id\": " << jobid
    << ", \"nodelist\": \"" << nodelist_str << "\""
    << " },\n";
  STAT_FORK_AND_DUMP(aggregatorstat_func, AggregatorStatistics, o)
  STAT_FORK_AND_DUMP(commstat_func, CommunicatorStatistics, o)
  STAT_FORK_AND_DUMP(taskmanagerstat_func, TaskManager::TaskStatistics, o)
  STAT_FORK_AND_DUMP(schedulerstat_func, TaskingScheduler::TaskingSchedulerStatistics, o)
  STAT_FORK_AND_DUMP(stealstat_func, StealStatistics, o)
  STAT_FORK_AND_DUMP(delegatestat_func, DelegateStatistics, o) 
  STAT_FORK_AND_DUMP(cachestat_func, CacheStatistics, o)
  STAT_FORK_AND_DUMP(incoherentacq_func, IAStatistics, o)
  STAT_FORK_AND_DUMP(incoherentrel_func, IRStatistics, o)
  STAT_FORK_AND_DUMP(globalqueuestat_func, GlobalQueueStatistics, o)
  dump_flags( o, "," );
  Grappa_dump_user_stats( o, "" );  // TODO: user stats are NOT merged
  o << "}STATS";
  oo << o.str();
}

void Grappa_merge_and_dump_stats( std::ostream& oo ) {
  reduce_stats_and_dump( oo );
}




void Grappa_dump_task_series() {
	global_scheduler.stats.print_active_task_log();
}


/// Finish the job. 
/// 
/// If we've already been notified that we can exit, enter global
/// barrier and then clean up. If we have not been notified, then
/// notify everyone else, enter the barrier, and then clean up.
void Grappa_finish( int retval )
{
  Grappa_signal_done(); // this may be overkill (just set done bit?)

  //TAU_PROFILE_EXIT("Tau_profile_exit called");
  Grappa_barrier();

  DVLOG(1) << "Cleaning up Grappa library....";

  StateTimer::finish();

  global_task_manager.finish();
  global_aggregator.finish();
  global_communicator.finish( retval );
 
//  Grappa_dump_stats();

  // probably never get here (depending on communication layer)

  destroy_thread( master_thread );

  if (global_memory) delete global_memory;

#ifdef HEAPCHECK
  assert( Grappa_heapchecker->NoLeaks() );
#endif
  
}
