
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
#include "tasks/GlobalQueue.hpp"

#ifndef SHMMAX
#error "no SHMMAX defined for this system -- look it up with the command: `sysctl -A | grep shm`"
#endif

#ifdef VTRACE
#include <vt_user.h>
#endif

// command line arguments
DEFINE_bool( steal, true, "Allow work-stealing between public task queues");
DEFINE_int32( chunk_size, 10, "Amount of work to publish or steal in multiples of" );
DEFINE_int32( cancel_interval, 1, "Interval for notifying others of new work" );
DEFINE_uint64( num_starting_workers, 4, "Number of starting workers in task-executer pool" );

DECLARE_bool( global_queue );

static Thread * barrier_thread = NULL;

Thread * master_thread;
static Thread * user_main_thr;

/// Flag to tell this node it's okay to exit.
bool SoftXMT_done_flag;

double tick_rate = 0.0;

Node * node_neighbors;

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

DECLARE_bool( global_memory_use_hugepages );

/// Initialize SoftXMT components. We are not ready to run until the
/// user calls SoftXMT_activate().
void SoftXMT_init( int * argc_p, char ** argv_p[], size_t global_memory_size_bytes)
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

  // how fast do we tick?
  SoftXMT_tick();
  SoftXMT_tick();
  SoftXMT_Timestamp start_ts = SoftXMT_get_timestamp();
  double start = SoftXMT_walltime();
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

  // by default, will allocate as much shared memory as it is
  // possible to evenly split among the processors on a node
  if (global_memory_size_bytes == -1) {
    int64_t nnode = atoi(getenv("SLURM_NNODES"));
    int64_t ppn = atoi(getenv("SLURM_NTASKS_PER_NODE"));
    int64_t bytes_per_proc = SHMMAX / ppn;
    
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

  SoftXMT_done_flag = false;

  // process command line args for Tau
  //TAU_INIT( argc_p, argv_p );
#ifdef GRAPPA_TRACE
  TAU_PROFILE_SET_NODE(SoftXMT_mynode());
#endif

  //TODO: options for local stealing
  node_neighbors = new Node[SoftXMT_nodes()];
  for ( Node nod=0; nod < SoftXMT_nodes(); nod++ ) {
    node_neighbors[nod] = nod;
  }

  // start threading layer
  master_thread = thread_init();
  VLOG(1) << "Initializing tasking layer."
           << " steal=" << FLAGS_steal
           << " num_starting_workers=" << FLAGS_num_starting_workers
           << " chunk_size=" << FLAGS_chunk_size
           << " cbint=" << FLAGS_cancel_interval;
  global_task_manager.init( FLAGS_steal, SoftXMT_mynode(), node_neighbors, SoftXMT_nodes(), FLAGS_chunk_size, FLAGS_cancel_interval ); //TODO: options for local stealing
  global_scheduler.init( master_thread, &global_task_manager );
  global_scheduler.periodic( thread_spawn( master_thread, &global_scheduler, &poller, NULL ) );

  SoftXMT_tick();
  SoftXMT_Timestamp end_ts = SoftXMT_get_timestamp();
  double end = SoftXMT_walltime();
  tick_rate = (double) (end_ts - start_ts) / (end-start);
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


static bool global_queue_initialized = false;
// fork-join function for SoftXMT_initialize_global_queue
LOOP_FUNCTION( initialize_global_queue_func, nid ) {
  GlobalQueue<Task>::global_queue.init();
  global_queue_initialized = true;
}

/// Initialize global queue for load balancing.
/// Must be called in user_main
void SoftXMT_global_queue_initialize() {
  if ( FLAGS_global_queue ) {
    initialize_global_queue_func f;
    fork_join_custom( &f );
  }
}

bool SoftXMT_global_queue_isInit() {
  return global_queue_initialized;
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
  global_task_manager.reset_stats();
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
  std::cout << "SoftXMTStats { tick_rate: " << tick_rate << " }" << std::endl;
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

// XXX: yield based synchro
uint64_t merge_reply_count;
#define NUM_STATS_MERGE 6
static void merge_stats_task(int64_t target) {
  if ( target != SoftXMT_mynode() ) {
    SoftXMT_call_on(target, &CommunicatorStatistics::merge_am, &global_communicator.stats);
    SoftXMT_call_on(target, &AggregatorStatistics::merge_am, &global_aggregator.stats);
    SoftXMT_call_on(target, &TaskingScheduler::TaskingSchedulerStatistics::merge_am, &global_scheduler.stats);
    SoftXMT_call_on(target, &TaskManager::TaskStatistics::merge_am, &global_task_manager.stats);
    SoftXMT_call_on(target, &DelegateStatistics::merge_am, &delegate_stats);
    SoftXMT_call_on(target, &CacheStatistics::merge_am, &cache_stats);
  }
}

LOOP_FUNCTOR(merge_stats_task_func,nid, ((Node,target)) ) {
  merge_stats_task( target );
}

void SoftXMT_merge_and_dump_stats() {
  merge_reply_count = 0;
  merge_stats_task_func f;
  f.target = SoftXMT_mynode();
  fork_join_custom(&f);

  // wait for all merges to happen
  while( merge_reply_count < (SoftXMT_nodes()-1)*NUM_STATS_MERGE ) {
    SoftXMT_yield();
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
