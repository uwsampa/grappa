
#ifndef __TASKING_HPP__
#define __TASKING_HPP__

//#include "SoftXMT.hpp"
#include "tasks/Task.hpp"
#include "StateTimer.hpp"

#include <boost/type_traits/remove_pointer.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/static_assert.hpp>

#ifdef GRAPPA_TRACE
#include <TAU.h>
#endif

#ifdef VTRACE
#include <vt_user.h>
#endif

#define STATIC_ASSERT_SIZE_8( type ) BOOST_STATIC_ASSERT( sizeof(type) == 8 )

extern TaskManager global_task_manager;

DECLARE_uint64( num_starting_workers );

void SoftXMT_take_profiling_sample();


///
/// Task routines
///

/// Spawn a task visible to this Node only
template < typename T, typename S, typename R >
void SoftXMT_privateTask( void (*fn_p)(T,S,R), T arg0, S arg1, R arg2 ) {
  STATIC_ASSERT_SIZE_8( T );
  STATIC_ASSERT_SIZE_8( S );
  STATIC_ASSERT_SIZE_8( R );
  DVLOG(5) << "Thread " << global_scheduler.get_current_thread() << " spawns private";
  global_task_manager.spawnLocalPrivate( fn_p, arg0, arg1, arg2 );
}

template < typename T, typename S >
void SoftXMT_privateTask( void (*fn_p)(T, S), T arg, S shared_arg) 
{
  SoftXMT_privateTask(reinterpret_cast<void (*)(T,S,void*)>(fn_p), arg, shared_arg, (void*)NULL);
}

template < typename T >
inline void SoftXMT_privateTask( void (*fn_p)(T), T arg) {
  SoftXMT_privateTask(reinterpret_cast<void (*)(T,void*)>(fn_p), arg, (void*)NULL);
}

/// Spawn a task visible to other Nodes
template < typename T, typename S, typename R >
void SoftXMT_publicTask( void (*fn_p)(T, S, R), T arg0, S arg1, R arg2) 
{
  STATIC_ASSERT_SIZE_8( T );
  STATIC_ASSERT_SIZE_8( S );
  STATIC_ASSERT_SIZE_8( R );
  DVLOG(5) << "Thread " << global_scheduler.get_current_thread() << " spawns public";
  global_task_manager.spawnPublic( fn_p, arg0, arg1, arg2 );
}

template < typename T, typename S >
void SoftXMT_publicTask( void (*fn_p)(T, S), T arg, S shared_arg) 
{
  SoftXMT_publicTask(reinterpret_cast<void (*)(T,S,void*)>(fn_p), arg, shared_arg, (void*)NULL);
}

template < typename T >
void SoftXMT_publicTask( void (*fn_p)(T), T arg) {
  SoftXMT_publicTask(reinterpret_cast<void (*)(T,void*)>(fn_p), arg, (void*)NULL);
}

// wrapper to make user_main terminate the tasking layer
void SoftXMT_end_tasks();
template < typename T >
void user_main_wrapper( void (*fp)(T), T args ) {
    fp( args );
    SoftXMT_end_tasks();
}

/// Spawn and run user main function on node 0. Other nodes just run
/// existing threads (service threads) until they are given more to
/// do. TODO: get return values working
//template < typename T, void (*F)(T) >
template < typename T >
int SoftXMT_run_user_main( void (*fp)(T), T args )
{
  STATIC_ASSERT_SIZE_8( T );
  
#ifdef GRAPPA_TRACE  
  TAU_PROFILE("run_user_main()", "(user code entry)", TAU_USER);
#endif
#ifdef VTRACE
  VT_TRACER("run_user_main()");
#endif
#ifdef VTRACE_SAMPLED
  // this doesn't really add anything to the profiled trace
  //SoftXMT_take_profiling_sample();
#endif

  if( SoftXMT_mynode() == 0 ) {
    CHECK_EQ( CURRENT_THREAD, master_thread ); // this should only be run at the toplevel

    // create user_main as a private task
    SoftXMT_privateTask( &user_main_wrapper<T>, fp, args );
    DVLOG(5) << "Spawned user_main";
    
    // spawn 1 extra worker that will take user_main
    global_scheduler.createWorkers( 1 );
  }

  // spawn starting number of worker coroutines
  global_scheduler.createWorkers( FLAGS_num_starting_workers );
  
  StateTimer::init();

  // start the scheduler
  global_scheduler.run( );

#ifdef VTRACE_SAMPLED
  // this doesn't really add anything to the profiled trace
  //SoftXMT_take_profiling_sample();
#endif

  return 0;
}


/// remote task spawn
template< typename T, typename S, typename R >
struct remote_task_spawn_args {
  void (*fn_p)(T, S, R);
  T arg0;
  S arg1;
  R arg2;
};

template< typename T, typename S, typename R >
static void remote_task_spawn_am( remote_task_spawn_args<T,S,R> * args, size_t args_size, void* payload, size_t payload_size) {
   global_task_manager.spawnRemotePrivate(args->fn_p, args->arg0, args->arg1, args->arg2 );
}

/// Spawn a private task on another Node
template< typename T, typename S, typename R >
void SoftXMT_remote_privateTask( void (*fn_p)(T,S,R), T arg0, S arg1, R arg2, Node target) {
  STATIC_ASSERT_SIZE_8( T );
  STATIC_ASSERT_SIZE_8( S );
  STATIC_ASSERT_SIZE_8( R );

  remote_task_spawn_args<T,S,R> spawn_args = { fn_p, arg0, arg1, arg2 };
  SoftXMT_call_on( target, SoftXMT_magic_identity_function(&remote_task_spawn_am<T,S,R>), &spawn_args );
  DVLOG(5) << "Sent AM to spawn private task on Node " << target;
}

template< typename T, typename S >
void SoftXMT_remote_privateTask( void (*fn_p)(T,S), T args, S shared_arg, Node target) {
  SoftXMT_remote_privateTask(reinterpret_cast<void (*)(T,S,void*)>(fn_p), args, shared_arg, (void*)NULL, target);
}

template< typename T >
void SoftXMT_remote_privateTask( void (*fn_p)(T), T args, Node target) {
  SoftXMT_remote_privateTask(reinterpret_cast<void (*)(T,void*)>(fn_p), args, (void*)NULL, target);
}

#endif

