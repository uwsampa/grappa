
#include "SoftXMT.hpp"

static Communicator * my_global_communicator;
static Aggregator * my_global_aggregator;

static thread * master_thread;
static scheduler * sched;

/// Flag to tell this node it's okay to exit.
bool SoftXMT_done_flag;



static void poller( thread * me, void * args ) {
  while( !SoftXMT_done() ) {
    SoftXMT_poll();
    SoftXMT_yield();
  }
}

/// Initialize SoftXMT components. We are not ready to run until the
/// user calls SoftXMT_activate().
void SoftXMT_init( int * argc_p, char ** argv_p[] )
{
  // also initializes system_wide global_communicator pointer
  my_global_communicator = new Communicator();
  my_global_communicator->init( argc_p, argv_p );

  // also initializes system_wide global_aggregator pointer
  my_global_aggregator = new Aggregator( my_global_communicator );

  SoftXMT_done_flag = false;

  master_thread = thread_init();
  sched = create_scheduler( master_thread );
  thread_spawn( master_thread, sched, &poller, NULL );
}


/// Activate SoftXMT network layer and enter barrier. After this,
/// arbitrary communication is allowed.
void SoftXMT_activate() 
{
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
    assert( current_thread == master_thread ); // this should only be run at the toplevel
    thread * main = thread_spawn( current_thread, sched,
                                  fn_p, args );
  }
  run_all( sched );
}

/// Spawn a user function. TODO: get return values working
/// TODO: remove thread * arg
thread * SoftXMT_spawn( void (* fn_p)(thread *, void *), void * args )
{
  return thread_spawn( current_thread, sched, fn_p, args );
}

/// Yield to scheduler, placing current thread on run queue.
void SoftXMT_yield( )
{
  thread_yield( current_thread );
}

/// Yield to scheduler, suspending current thread.
void SoftXMT_suspend( )
{
  thread_suspend( current_thread );
}

/// Wake a thread by putting it on the run queue, leaving the current thread running.
void SoftXMT_wake( thread * t )
{
  thread_wake( t );
}

/// Wake a thread t by placing current thread on run queue and running t next.
void SoftXMT_yield_wake( thread * t )
{
  thread_yield_wake( current_thread, t );
}

/// Wake a thread t by suspending current thread and running t next.
void SoftXMT_suspend_wake( thread * t )
{
  thread_suspend_wake( current_thread, t );
}

/// Join on thread t
void SoftXMT_join( thread * t )
{
  thread_join( current_thread, t );
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
  
  my_global_communicator->finish( retval );
  
  // probably never get here (depending on communication layer

  destroy_scheduler( sched );
  destroy_thread( master_thread );

  delete my_global_aggregator;
  delete my_global_communicator;
}

