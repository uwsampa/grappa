
#ifdef HEAPCHECK
#include <gperftools/heap-checker.h>
#endif

#include "SoftXMT.hpp"
#include "GlobalMemory.hpp"
#include "tasks/Task.hpp"

// command line arguments
DEFINE_bool( steal, true, "Allow work-stealing between public task queues");
DEFINE_int32( chunk_size, 10, "Amount of work to publish or steal in multiples of" );
DEFINE_int32( cancel_interval, 1, "Interval for notifying others of new work" );
DEFINE_uint64( num_starting_workers, 4, "Number of starting workers in task-executer pool" );

static Communicator * my_global_communicator = NULL;
static Aggregator * my_global_aggregator = NULL;

// This is the pointer for the generic memory pool.
// TODO: should granular memory pools be stored at this level?
static GlobalMemory * my_global_memory = NULL;

Thread * master_thread;
static Thread * user_main_thr;
TaskingScheduler * my_global_scheduler;
TaskManager * my_task_manager;

/// Flag to tell this node it's okay to exit.
bool SoftXMT_done_flag;

#ifdef HEAPCHECK
HeapLeakChecker * SoftXMT_heapchecker = 0;
#endif

static void poller( Thread * me, void * args ) {
  while( !SoftXMT_done() ) {
    SoftXMT_poll();
    SoftXMT_yield_periodic();
  }
  VLOG(5) << "polling Thread exiting";
}

/// Initialize SoftXMT components. We are not ready to run until the
/// user calls SoftXMT_activate().
void SoftXMT_init( int * argc_p, char ** argv_p[], size_t global_memory_size_bytes )
{

  // parse command line flags
  google::ParseCommandLineFlags(argc_p, argv_p, true);

  // activate logging
  google::InitGoogleLogging( *argv_p[0] );
  google::InstallFailureSignalHandler( );

  DVLOG(1) << "Initializing SoftXMT library....";
#ifdef HEAPCHECK
  SoftXMT_heapchecker = new HeapLeakChecker("SoftXMT");
#endif

  // also initializes system_wide global_communicator pointer
  my_global_communicator = new Communicator();
  my_global_communicator->init( argc_p, argv_p );

  // also initializes system_wide global_aggregator pointer
  my_global_aggregator = new Aggregator( my_global_communicator );

  // also initializes system_wide global_memory pointer
  my_global_memory = new GlobalMemory( global_memory_size_bytes );

  SoftXMT_done_flag = false;

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
  my_task_manager = new TaskManager( FLAGS_steal, SoftXMT_mynode(), neighbors, SoftXMT_nodes(), FLAGS_chunk_size, FLAGS_cancel_interval ); //TODO: options for local stealing
  my_global_scheduler = new TaskingScheduler( master_thread, my_task_manager );
  my_global_scheduler->periodic( thread_spawn( master_thread, my_global_scheduler, &poller, NULL ) );
}


/// Activate SoftXMT network layer and enter barrier. After this,
/// arbitrary communication is allowed.
void SoftXMT_activate() 
{
  DVLOG(1) << "Activating SoftXMT library....";
  my_global_communicator->activate();
  SoftXMT_barrier();
}


///
/// Query job info
///


/// How many nodes are there?
Node SoftXMT_nodes() {
  return my_global_communicator->nodes();
}

/// What is my node?
Node SoftXMT_mynode() {
  return my_global_communicator->mynode();
}

///
/// Communication utilities
///

/// Enter global barrier.
void SoftXMT_barrier() {
  my_global_communicator->barrier();
}

/// Enter global barrier that is poller safe
void SoftXMT_barrier_commsafe() {
    my_global_communicator->barrier_notify();
    while (!my_global_communicator->barrier_try()) {
        SoftXMT_yield();
    }
}

/// Poll SoftXMT aggregation and communication layers.
void SoftXMT_poll()
{
  my_global_aggregator->poll();
}

/// Send waiting aggregated messages to a particular destination.
void SoftXMT_flush( Node n )
{
  my_global_aggregator->flush( n );
}


///
/// Thread management routines
///

/// Spawn a user function. TODO: get return values working
/// TODO: remove Thread * arg
Thread * SoftXMT_spawn( void (* fn_p)(Thread *, void *), void * args )
{
  Thread * th = thread_spawn( my_global_scheduler->get_current_thread(), my_global_scheduler, fn_p, args );
  my_global_scheduler->ready( th );
  DVLOG(5) << "Spawned Thread " << th;
  return th;
}

/// Yield to scheduler, placing current Thread on run queue.
void SoftXMT_yield( )
{
  bool immed = my_global_scheduler->thread_yield( ); 
}

/// Yield to scheduler, placing current Thread on periodic queue.
void SoftXMT_yield_periodic( )
{
  bool immed = my_global_scheduler->thread_yield_periodic( );
}

/// Yield to scheduler, suspending current Thread.
void SoftXMT_suspend( )
{
  DVLOG(5) << "suspending Thread " << my_global_scheduler->get_current_thread() << "(# " << my_global_scheduler->get_current_thread()->id << ")";
  my_global_scheduler->thread_suspend( );
  //CHECK_EQ(retval, 0) << "Thread " << th1 << " suspension failed. Have the server threads exited?";
}

/// Wake a Thread by putting it on the run queue, leaving the current thread running.
void SoftXMT_wake( Thread * t )
{
  DVLOG(5) << my_global_scheduler->get_current_thread()->id << " waking Thread " << t;
  my_global_scheduler->thread_wake( t );
}

/// Wake a Thread t by placing current thread on run queue and running t next.
void SoftXMT_yield_wake( Thread * t )
{
  DVLOG(5) << "yielding Thread " << my_global_scheduler->get_current_thread() << " and waking thread " << t;
  my_global_scheduler->thread_yield_wake( t );
}

/// Wake a Thread t by suspending current thread and running t next.
void SoftXMT_suspend_wake( Thread * t )
{
  DVLOG(5) << "suspending Thread " << my_global_scheduler->get_current_thread() << " and waking thread " << t;
  my_global_scheduler->thread_suspend_wake( t );
}

/// Join on Thread t
void SoftXMT_join( Thread * t )
{
  DVLOG(5) << "Thread " << my_global_scheduler->get_current_thread() << " joining on thread " << t;
  my_global_scheduler->thread_join( t );
}

bool SoftXMT_thread_idle( ) 
{
  DVLOG(5) << "Thread " << my_global_scheduler->get_current_thread()->id << " going idle";
  return my_global_scheduler->thread_idle( );
}



///
/// Job exit routines
///

/// Check whether we are ready to exit.
bool SoftXMT_done() {
  return SoftXMT_done_flag;
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

/// Dump statistics
void SoftXMT_dump_stats() {
  my_global_aggregator->dump_stats();
  my_global_communicator->dump_stats();
}

/// Finish the job. 
/// 
/// If we've already been notified that we can exit, enter global
/// barrier and then clean up. If we have not been notified, then
/// notify everyone else, enter the barrier, and then clean up.
void SoftXMT_finish( int retval )
{
  SoftXMT_signal_done(); // this may be overkill (just set done bit?)

  SoftXMT_barrier();

  DVLOG(1) << "Cleaning up SoftXMT library....";

  my_global_aggregator->finish();
  my_global_communicator->finish( retval );
  
  // probably never get here (depending on communication layer)

  delete my_global_scheduler;
  destroy_thread( master_thread );

  if (my_global_memory) delete my_global_memory;
  if (my_global_aggregator) delete my_global_aggregator;
  if (my_global_communicator) delete my_global_communicator;

#ifdef HEAPCHECK
  assert( SoftXMT_heapchecker->NoLeaks() );
#endif
}

