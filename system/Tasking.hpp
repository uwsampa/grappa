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
#include "tasks/TaskingScheduler.hpp"
#include "StateTimer.hpp"
#include "Communicator.hpp"

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


#define STATIC_ASSERT_SIZE_8(type) static_assert(sizeof(type) == 8, "Size of "#type" must be 8 bytes.")

DECLARE_uint64( num_starting_workers );


///
/// Task routines
///



namespace Grappa {

  /// @addtogroup Tasking
  /// @{

  namespace impl {

    /// Helper function to insert lambdas and functors in our task queues.
    template< typename T >
    static void task_functor_proxy( uint64_t a0, uint64_t a1, uint64_t a2 ) {
      uint64_t functor_storage[3] = { a0, a1, a2 };
      T * tp = reinterpret_cast< T * >( &functor_storage[0] );
      (*tp)();
    }

    /// Helper function to insert lambdas and functors in our task
    /// queues when they are larger than 24 bytes. This function takes
    /// ownership of the heap-allocated functor and deallocates it
    /// after it has run.
    template< typename T >
    static void task_heapfunctor_proxy( T * tp, T * unused1, T * unused2 ) {
      (*tp)();
      delete tp;
    }

  }

  /// Spawn a task visible to this Node only. The task is specified as
  /// a functor or lambda. If it is 24 bytes or less, it is copied
  /// directly into the task queue. If it is larger, a copy is
  /// allocated on the heap. This copy will be deallocated after the
  /// task completes.
  ///
  /// @tparam TF type of task functor
  ///
  /// @param func functor the new task.
  ///
  /// Example:
  /// @code
  ///   int data = 0;
  ///   Grappa::privateTask( [&data] { data++; } );
  /// @endcode
  template < typename TF >
  void privateTask( TF tf ) {
    if( sizeof( tf ) > 24 ) { // if it's too big to fit in a task queue entry
      DVLOG(4) << "Heap allocated task of size " << sizeof(tf);
      
      struct __attribute__((deprecated("heap allocating private task functor"))) Warning {};
      
      // heap-allocate copy of functor, passing ownership to spawned task
      TF * tp = new TF(tf);
      Grappa::impl::global_task_manager.spawnLocalPrivate( Grappa::impl::task_heapfunctor_proxy<TF>, tp, tp, tp );
    } else {
      /// Shove copy of functor into space used for task arguments.
      /// @TODO: misusing argument list. Is this okay?
      uint64_t * args = reinterpret_cast< uint64_t * >( &tf );
      /// // if not, substitute this:
      // uint64_t args[3] = { 0 };
      // TF * tfargs = reinterpret_cast< TF * >( &args[0] );
      // *tfargs = tf;
      DVLOG(5) << "Thread " << Grappa::impl::global_scheduler.get_current_thread() << " spawns private";
      Grappa::impl::global_task_manager.spawnLocalPrivate( Grappa::impl::task_functor_proxy<TF>, args[0], args[1], args[2] );
    }
  }
  
  template < typename TF >
  void publicTask( TF tf ) {
    // TODO: implement automatic heap allocation and caching to handle larger functors
    static_assert(sizeof(tf) <= 24,
        "Functor argument to publicTask too large to be automatically coerced.");
    
    DVLOG(5) << "Thread " << Grappa::impl::global_scheduler.get_current_thread() << " spawns public";
    
    uint64_t * args = reinterpret_cast< uint64_t * >( &tf );
    Grappa::impl::global_task_manager.spawnPublic(Grappa::impl::task_functor_proxy<TF>, args[0], args[1], args[2]);
  }

  /// @}

}


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
  DVLOG(5) << "Thread " << Grappa::impl::global_scheduler.get_current_thread() << " spawns private";
  Grappa::impl::global_task_manager.spawnLocalPrivate( fn_p, arg0, arg1, arg2 );
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
  DVLOG(5) << "Thread " << Grappa::impl::global_scheduler.get_current_thread() << " spawns public";
  Grappa::impl::global_task_manager.spawnPublic( fn_p, arg0, arg1, arg2 );
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
extern Thread * master_thread;
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

  if( global_communicator.mynode() == 0 ) {
    CHECK_EQ( Grappa::impl::global_scheduler.get_current_thread(), master_thread ); // this should only be run at the toplevel

    // create user_main as a private task
    Grappa_privateTask( &user_main_wrapper<A>, fp, args );
    DVLOG(5) << "Spawned user_main";
    
    // spawn 1 extra worker that will take user_main
    Grappa::impl::global_scheduler.createWorkers( 1 );
  }

  // spawn starting number of worker coroutines
  Grappa::impl::global_scheduler.createWorkers( FLAGS_num_starting_workers );
  Grappa::impl::global_scheduler.allow_active_workers(-1); // allow all workers to be active
  
  StateTimer::init();

  // start the scheduler
  Grappa::impl::global_scheduler.run( );

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
  Grappa::impl::global_task_manager.spawnRemotePrivate(args->fn_p, args->arg0, args->arg1, args->arg2 );
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

