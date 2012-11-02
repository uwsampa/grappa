// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef TASKING_HPP
#define TASKING_HPP

// This file contains the Grappa task spawning API

//#include "Grappa.hpp"
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

void Grappa_take_profiling_sample();


///
/// Task routines
///

/// Spawn a task visible to this Node only
///
/// @tparam A0 type of first task argument
/// @tparam A1 type of second task argument
/// @tparam A2 type of third task argument
///
/// @param fn_p function pointer for the new task
/// @param arg0 first task argument
/// @param arg1 second task argument
/// @param arg2 third task argument
template < typename A0, typename A1, typename A2 >
void Grappa_privateTask( void (*fn_p)(A0,A1,A2), A0 arg0, A1 arg1, A2 arg2 ) {
  STATIC_ASSERT_SIZE_8( A0 );
  STATIC_ASSERT_SIZE_8( A1 );
  STATIC_ASSERT_SIZE_8( A2 );
  DVLOG(5) << "Thread " << global_scheduler.get_current_thread() << " spawns private";
  global_task_manager.spawnLocalPrivate( fn_p, arg0, arg1, arg2 );
}

/// Spawn a task visible to this Node only
///
/// @tparam A0 type of first task argument
/// @tparam A1 type of second task argument
///
/// @param fn_p function pointer for the new task
/// @param arg0 first task argument
/// @param arg1 second task argument
template < typename A0, typename A1 >
void Grappa_privateTask( void (*fn_p)(A0, A1), A0 arg, A1 shared_arg) 
{
  Grappa_privateTask(reinterpret_cast<void (*)(A0,A1,void*)>(fn_p), arg, shared_arg, (void*)NULL);
}

/// Spawn a task visible to this Node only
///
/// @tparam A0 type of first task argument
///
/// @param fn_p function pointer for the new task
/// @param arg0 first task argument
template < typename T >
inline void Grappa_privateTask( void (*fn_p)(T), T arg) {
  Grappa_privateTask(reinterpret_cast<void (*)(T,void*)>(fn_p), arg, (void*)NULL);
}

/// Spawn a task to the global task pool.
/// That is, it can potentially be executed on any Node.
///
/// @tparam A0 type of first task argument
/// @tparam A1 type of second task argument
/// @tparam A2 type of third task argument
///
/// @param fn_p function pointer for the new task
/// @param arg0 first task argument
/// @param arg1 second task argument
/// @param arg2 third task argument
template < typename A0, typename A1, typename A2 >
void Grappa_publicTask( void (*fn_p)(A0, A1, A2), A0 arg0, A1 arg1, A2 arg2) 
{
  STATIC_ASSERT_SIZE_8( A0 );
  STATIC_ASSERT_SIZE_8( A1 );
  STATIC_ASSERT_SIZE_8( A2 );
  DVLOG(5) << "Thread " << global_scheduler.get_current_thread() << " spawns public";
  global_task_manager.spawnPublic( fn_p, arg0, arg1, arg2 );
}

/// Spawn a task to the global task pool.
/// That is, it can potentially be executed by any Node.
///
/// @tparam A0 type of first task argument
/// @tparam A1 type of second task argument
///
/// @param fn_p function pointer for the new task
/// @param arg0 first task argument
/// @param arg1 second task argument
template < typename A0, typename A1 >
void Grappa_publicTask( void (*fn_p)(A0, A1), A0 arg, A1 shared_arg) 
{
  Grappa_publicTask(reinterpret_cast<void (*)(A0,A1,void*)>(fn_p), arg, shared_arg, (void*)NULL);
}

/// Spawn a task to the global task pool.
/// That is, it can potentially be executed by any Node.
///
/// @tparam A0 type of first task argument
///
/// @param fn_p function pointer for the new task
/// @param arg0 first task argument
template < typename A0 >
void Grappa_publicTask( void (*fn_p)(A0), A0 arg) {
  Grappa_publicTask(reinterpret_cast<void (*)(A0,void*)>(fn_p), arg, (void*)NULL);
}

// initialize global queue if used
void Grappa_global_queue_initialize();
  
// forward declaration needed for below wrapper
void Grappa_end_tasks();

/// Wrapper to make user_main terminate the tasking layer
/// after it is done
template < typename T >
static void user_main_wrapper( void (*fp)(T), T args ) {
    Grappa_global_queue_initialize();
    fp( args );
    Grappa_end_tasks();
}

/// Spawn and run user main function on node 0. Other nodes just run
/// existing threads (service threads) until they are given more to
/// do. TODO: return values
///
/// ALLNODES
///
/// @tparam A type of task argument
///
/// @param fn_p function pointer for the user main task
/// @param args task argument
///
/// @return 0 if completed without errors
/// TODO: error return values?
template < typename A >
int Grappa_run_user_main( void (*fp)(A), A args )
{
  STATIC_ASSERT_SIZE_8( A );
  
#ifdef GRAPPA_TRACE  
  TAU_PROFILE("run_user_main()", "(user code entry)", TAU_USER);
#endif
#ifdef VTRACE
  VT_TRACER("run_user_main()");
#endif
#ifdef VTRACE_SAMPLED
  // this doesn't really add anything to the profiled trace
  //Grappa_take_profiling_sample();
#endif

  if( Grappa_mynode() == 0 ) {
    CHECK_EQ( CURRENT_THREAD, master_thread ); // this should only be run at the toplevel

    // create user_main as a private task
    Grappa_privateTask( &user_main_wrapper<A>, fp, args );
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
  //Grappa_take_profiling_sample();
#endif

  return 0;
}


/// remote task spawn arguments
template< typename A0, typename A1, typename A2 >
struct remote_task_spawn_args {
  void (*fn_p)(A0, A1, A2);
  A0 arg0;
  A1 arg1;
  A2 arg2;
};

/// Grappa Active message for 
/// void Grappa_remote_privateTask( void (*fn_p)(A0,A1,A2), A0, A1, A2, Node)
///
/// @tparam A0 type of first task argument
/// @tparam A1 type of second task argument
/// @tparam A2 type of third task argument
template< typename A0, typename A1, typename A2 >
static void remote_task_spawn_am( remote_task_spawn_args<A0,A1,A2> * args, size_t args_size, void* payload, size_t payload_size) {
   global_task_manager.spawnRemotePrivate(args->fn_p, args->arg0, args->arg1, args->arg2 );
}

/// Spawn a private task on another Node
///
/// @tparam A0 type of first task argument
/// @tparam A1 type of second task argument
/// @tparam A2 type of third task argument
///
/// @param f function pointer for the new task
/// @param arg0 first task argument
/// @param arg1 second task argument
/// @param arg2 third task argument
/// @param target Node to spawn the task on
template< typename A0, typename A1, typename A2 >
void Grappa_remote_privateTask( void (*fn_p)(A0,A1,A2), A0 arg0, A1 arg1, A2 arg2, Node target) {
  STATIC_ASSERT_SIZE_8( A0 );
  STATIC_ASSERT_SIZE_8( A1 );
  STATIC_ASSERT_SIZE_8( A2 );

  remote_task_spawn_args<A0,A1,A2> spawn_args = { fn_p, arg0, arg1, arg2 };
  Grappa_call_on( target, Grappa_magic_identity_function(&remote_task_spawn_am<A0,A1,A2>), &spawn_args );
  DVLOG(5) << "Sent AM to spawn private task on Node " << target;
}

/// Spawn a private task on another Node
///
/// @tparam A0 type of first task argument
/// @tparam A1 type of second task argument
///
/// @param fn_p function pointer for the new task
/// @param args first task argument
/// @param shared_arg second task argument
/// @param target Node to spawn the task on
template< typename A0, typename A1 >
void Grappa_remote_privateTask( void (*fn_p)(A0,A1), A0 args, A1 shared_arg, Node target) {
  Grappa_remote_privateTask(reinterpret_cast<void (*)(A0,A1,void*)>(fn_p), args, shared_arg, (void*)NULL, target);
}

/// Spawn a private task on another Node
///
/// @tparam A type of first task argument
///
/// @param fn_p function pointer for the new task
/// @param args first task argument
/// @param target Node to spawn the task on
template< typename A >
void Grappa_remote_privateTask( void (*fn_p)(A), A args, Node target) {
  Grappa_remote_privateTask(reinterpret_cast<void (*)(A,void*)>(fn_p), args, (void*)NULL, target);
}

#endif // TASKING_HPP

