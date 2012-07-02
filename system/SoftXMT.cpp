
#include <signal.h>

#ifdef HEAPCHECK
#include <gperftools/heap-checker.h>
#endif

#include "SoftXMT.hpp"
#include "GlobalMemory.hpp"
#include "tasks/Task.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"
#include "PerformanceTools.hpp"


#ifdef VTRACE
#include <vt_user.h>
#endif

// command line arguments
DEFINE_bool( steal, true, "Allow work-stealing between public task queues");
DEFINE_int32( chunk_size, 10, "Amount of work to publish or steal in multiples of" );
DEFINE_int32( cancel_interval, 1, "Interval for notifying others of new work" );
DEFINE_uint64( num_starting_workers, 4, "Number of starting workers in task-executer pool" );

static Thread * barrier_thread = NULL;

Thread * master_thread;
static Thread * user_main_thr;

/// Flag to tell this node it's okay to exit.
bool SoftXMT_done_flag;

#ifdef HEAPCHECK
HeapLeakChecker * SoftXMT_heapchecker = 0;
#endif

void SoftXMT_take_profiling_sample() {
  global_aggregator.stats.profiling_sample();
  global_communicator.stats.profiling_sample();
  global_task_manager.stats.profiling_sample();
  global_scheduler.stats.profiling_sample();
  delegate_stats.profiling_sample();
  cache_stats.profiling_sample();

  // print user-registered stats
  SoftXMT_profiling_sample_user();
}

static void poller( Thread * me, void * args ) {
  StateTimer::setThreadState( StateTimer::COMMUNICATION );
  StateTimer::enterState_communication();
  while( !SoftXMT_done() ) {
    global_scheduler.stats.sample();
    global_task_manager.stats.sample();

    SoftXMT_poll();
    if (barrier_thread) {
      if (global_communicator.barrier_try()) {
        SoftXMT_wake(barrier_thread);
        barrier_thread = NULL;
      }
    }
    SoftXMT_yield_periodic();
  }
  VLOG(5) << "polling Thread exiting";
}

// handler for dumping stats on a signal
static int stats_dump_signal = SIGUSR2;
static void stats_dump_sighandler( int signum ) {
  // TODO: make this set a flag and have scheduler check and dump.
  SoftXMT_dump_stats();

  // instantaneous state
  LOG(INFO) << global_scheduler;
  LOG(INFO) << global_task_manager;
}

// handler for SIGABRT override
static void sigabrt_sighandler( int signum ) {
  raise( SIGUSR1 );
}

/// Initialize SoftXMT components. We are not ready to run until the
/// user calls SoftXMT_activate().
void SoftXMT_init( int * argc_p, char ** argv_p[], size_t global_memory_size_bytes )
{

  // help generate unique profile filename
  SoftXMT_set_profiler_argv0( (*argv_p)[0] );

  // parse command line flags
  google::ParseCommandLineFlags(argc_p, argv_p, true);

  // activate logging
  google::InitGoogleLogging( *argv_p[0] );
  google::InstallFailureSignalHandler( );

  DVLOG(1) << "Initializing SoftXMT library....";
#ifdef HEAPCHECK
  SoftXMT_heapchecker = new HeapLeakChecker("SoftXMT");
#endif

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

  // initializes system_wide global_memory pointer
  global_memory = new GlobalMemory( global_memory_size_bytes );

  SoftXMT_done_flag = false;

  // process command line args for Tau
  //TAU_INIT( argc_p, argv_p );
  TAU_PROFILE_SET_NODE(SoftXMT_mynode());

  //TODO: options for local stealing
  Node * neighbors = new Node[SoftXMT_nodes()];
  for ( Node nod=0; nod < SoftXMT_nodes(); nod++ ) {
    neighbors[nod] = nod;
  }

  // start threading layer
  master_thread = thread_init();
  VLOG(1) << "Initializing tasking layer."
           << " steal=" << FLAGS_steal
           << " num_starting_workers=" << FLAGS_num_starting_workers
           << " chunk_size=" << FLAGS_chunk_size
           << " cbint=" << FLAGS_cancel_interval;
  global_task_manager.init( FLAGS_steal, SoftXMT_mynode(), neighbors, SoftXMT_nodes(), FLAGS_chunk_size, FLAGS_cancel_interval ); //TODO: options for local stealing
  global_scheduler.init( master_thread, &global_task_manager );
  global_scheduler.periodic( thread_spawn( master_thread, &global_scheduler, &poller, NULL ) );
}


/// Activate SoftXMT network layer and enter barrier. After this,
/// arbitrary communication is allowed.
void SoftXMT_activate() 
{
  DVLOG(1) << "Activating SoftXMT library....";
  global_communicator.activate();
  SoftXMT_barrier();
}

void SoftXMT_barrier_suspending() {
  global_communicator.barrier_notify();
  barrier_thread = CURRENT_THREAD;
  SoftXMT_suspend();
}


///
/// Thread management routines
///

/// Spawn a user function. TODO: get return values working
/// TODO: remove Thread * arg
inline Thread * SoftXMT_spawn( void (* fn_p)(Thread *, void *), void * args )
{
  Thread * th = thread_spawn( global_scheduler.get_current_thread(), &global_scheduler, fn_p, args );
  global_scheduler.ready( th );
  DVLOG(5) << "Spawned Thread " << th;
  return th;
}

///
/// Job exit routines
///

/// Check whether we are ready to exit.
bool SoftXMT_done() {
  return SoftXMT_done_flag;
}

//// termination fork join declaration
//LOOP_FUNCTION(signal_task_termination_func,nid) {
//    global_task_manager.signal_termination();
//}
static void signal_task_termination_am( int * ignore, size_t isize, void * payload, size_t payload_size ) {
    global_task_manager.signal_termination();
}

/// User main done
void SoftXMT_end_tasks() {
  // send task termination signal
  CHECK( SoftXMT_mynode() == 0 );
  for ( Node n = 1; n < SoftXMT_nodes(); n++ ) {
      int ignore = 0;
      SoftXMT_call_on( n, &signal_task_termination_am, &ignore );
      SoftXMT_flush( n );
  }
  signal_task_termination_am( NULL, 0, NULL, 0 );
}


///// Active message to tell this node it's okay to exit.
//static void SoftXMT_mark_done_am( void * args, size_t args_size, void * payload, size_t payload_size ) {
//  VLOG(5) << "mark done";
//  SoftXMT_done_flag = true;
//}

/// Tell all nodes that we are ready to exit
/// This will terminate the automatic portions of the communication layer
void SoftXMT_signal_done ( ) { 
    VLOG(5) << "mark done";
    SoftXMT_done_flag = true;
}

void SoftXMT_reset_stats() {
  global_aggregator.reset_stats();
  global_communicator.reset_stats();
  global_scheduler.reset_stats();
  delegate_stats.reset();
  cache_stats.reset();
 
  SoftXMT_reset_user_stats(); 
}

LOOP_FUNCTION(reset_stats_func,nid) {
  SoftXMT_reset_stats();
}
void SoftXMT_reset_stats_all_nodes() {
  reset_stats_func f;
  fork_join_custom(&f);
}


/// Dump statistics
void SoftXMT_dump_stats() {
  global_aggregator.dump_stats();
  global_communicator.dump_stats();
  global_task_manager.dump_stats();
  global_scheduler.dump_stats();
  delegate_stats.dump();
  cache_stats.dump();
}

LOOP_FUNCTION(dump_stats_func,nid) {
  SoftXMT_dump_stats();
}
void SoftXMT_dump_stats_all_nodes() {
  dump_stats_func f;
  fork_join_custom(&f);
}

static void merge_stats_task(int64_t target) {
  SoftXMT_call_on(target, &CommunicatorStatistics::merge_am, &global_communicator.stats);
  SoftXMT_call_on(target, &AggregatorStatistics::merge_am, &global_aggregator.stats);
  SoftXMT_call_on(target, &TaskingScheduler::TaskingSchedulerStatistics::merge_am, &global_scheduler.stats);
  SoftXMT_call_on(target, &TaskManager::TaskStatistics::merge_am, &global_task_manager.stats);
  SoftXMT_call_on(target, &DelegateStatistics::merge_am, &delegate_stats);
  SoftXMT_call_on(target, &CacheStatistics::merge_am, &cache_stats);
}


void SoftXMT_merge_and_dump_stats() {
  Node me = SoftXMT_mynode();
  for (Node n=0; n<SoftXMT_nodes(); n++) {
    if (n != me) {
      SoftXMT_remote_privateTask(&merge_stats_task, (int64_t)me, n);
    }
  }
  
  SoftXMT_dump_stats();
}

void SoftXMT_dump_task_series() {
	global_scheduler.stats.print_active_task_log();
}


/// Finish the job. 
/// 
/// If we've already been notified that we can exit, enter global
/// barrier and then clean up. If we have not been notified, then
/// notify everyone else, enter the barrier, and then clean up.
void SoftXMT_finish( int retval )
{
  SoftXMT_signal_done(); // this may be overkill (just set done bit?)

  //TAU_PROFILE_EXIT("Tau_profile_exit called");
  SoftXMT_barrier();

  DVLOG(1) << "Cleaning up SoftXMT library....";

  StateTimer::finish();

  global_task_manager.finish();
  global_aggregator.finish();
  global_communicator.finish( retval );
 
//  SoftXMT_dump_stats();

  // probably never get here (depending on communication layer)

  destroy_thread( master_thread );

  if (global_memory) delete global_memory;

#ifdef HEAPCHECK
  assert( SoftXMT_heapchecker->NoLeaks() );
#endif
  
}
