
#ifdef HEAPCHECK
#include <gperftools/heap-checker.h>
#endif

#include "SoftXMT.hpp"
#include "GlobalMemory.hpp"
#include "tasks/TaskingScheduler.hpp"
#include "tasks/Task.hpp"


static Communicator * my_global_communicator = NULL;
static Aggregator * my_global_aggregator = NULL;

// This is the pointer for the generic memory pool.
// TODO: should granular memory pools be stored at this level?
static GlobalMemory * my_global_memory = NULL;

static thread * master_thread;
TaskingScheduler * my_global_scheduler;
static TaskManager * my_task_manager;

/// Flag to tell this node it's okay to exit.
bool SoftXMT_done_flag;

#ifdef HEAPCHECK
HeapLeakChecker * SoftXMT_heapchecker = 0;
#endif

static void poller( thread * me, void * args ) {
  while( !SoftXMT_done() ) {
    SoftXMT_poll();
    SoftXMT_yield();
  }
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

  // start threading layer
  master_thread = thread_init();
  my_task_manager = new TaskManager( false, 0, NULL, 1, 10, 1 ); //TODO: fill in steal options
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

/// Spawn and run user main function on node 0. Other nodes just run
/// existing threads (service threads) until they are given more to
/// do. TODO: get return values working TODO: remove thread * arg
int SoftXMT_run_user_main( void (* fn_p)(thread *, void *), void * args )
{
  if( SoftXMT_mynode() == 0 ) {
    CHECK( my_global_scheduler->get_current_thread() == master_thread ); // this should only be run at the toplevel
    thread * main = thread_spawn( my_global_scheduler->get_current_thread(), my_global_scheduler,
                                  fn_p, args );
    my_global_scheduler->ready( main );
    DVLOG(5) << "Spawned main thread " << main;
  }
  my_global_scheduler->run( );
}

/// Spawn a user function. TODO: get return values working
/// TODO: remove thread * arg
thread * SoftXMT_spawn( void (* fn_p)(thread *, void *), void * args )
{
  thread * th = thread_spawn( my_global_scheduler->get_current_thread(), my_global_scheduler, fn_p, args );
  my_global_scheduler->ready( th );
  DVLOG(5) << "Spawned thread " << th;
  return th;
}

/// Yield to scheduler, placing current thread on run queue.
void SoftXMT_yield( )
{
  bool immed = my_global_scheduler->thread_yield( ); 
  DVLOG(5) << "Thread " << my_global_scheduler->get_current_thread() << " yield to thread " << my_global_scheduler->get_current_thread() << (immed ? " (same thread)." : " (diff thread).");
}

/// Yield to scheduler, suspending current thread.
void SoftXMT_suspend( )
{
  DVLOG(5) << "suspending thread " << my_global_scheduler->get_current_thread();
  my_global_scheduler->thread_suspend( );
  //CHECK_EQ(retval, 0) << "Thread " << th1 << " suspension failed. Have the server threads exited?";
}

/// Wake a thread by putting it on the run queue, leaving the current thread running.
void SoftXMT_wake( thread * t )
{
  DVLOG(5) << "waking thread " << t;
  my_global_scheduler->thread_wake( t );
}

/// Wake a thread t by placing current thread on run queue and running t next.
void SoftXMT_yield_wake( thread * t )
{
  DVLOG(5) << "yielding thread " << my_global_scheduler->get_current_thread() << " and waking thread " << t;
  my_global_scheduler->thread_yield_wake( t );
}

/// Wake a thread t by suspending current thread and running t next.
void SoftXMT_suspend_wake( thread * t )
{
  DVLOG(5) << "suspending thread " << my_global_scheduler->get_current_thread() << " and waking thread " << t;
  my_global_scheduler->thread_suspend_wake( t );
}

/// Join on thread t
void SoftXMT_join( thread * t )
{
  DVLOG(5) << "thread " << my_global_scheduler->get_current_thread() << " joining on thread " << t;
  my_global_scheduler->thread_join( t );
}

///
/// Job exit routines
///

/// Check whether we are ready to exit.
bool SoftXMT_done() {
  return SoftXMT_done_flag;
}

/// Active message to tell this node it's okay to exit.
static void SoftXMT_mark_done_am( void * args, size_t args_size, void * payload, size_t payload_size ) {
  SoftXMT_done_flag = true;
}

/// Tell all nodes that we are ready to exit
void SoftXMT_signal_done() {
  if( !SoftXMT_done() ) {
    for( Node i = 0; i < SoftXMT_nodes(); ++i ) {
      SoftXMT_call_on( i, &SoftXMT_mark_done_am, (void *)NULL, 0 );
      SoftXMT_flush( i );
    }
  }
}

/// Finish the job. 
/// 
/// If we've already been notified that we can exit, enter global
/// barrier and then clean up. If we have not been notified, then
/// notify everyone else, enter the barrier, and then clean up.
void SoftXMT_finish( int retval )
{
  SoftXMT_signal_done();

  SoftXMT_barrier();

  DVLOG(1) << "Cleaning up SoftXMT library....";
  
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

