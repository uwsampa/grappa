
#ifndef __TASKING_HPP__
#define __TASKING_HPP__

//#include "SoftXMT.hpp"
#include "tasks/Task.hpp"

#include <boost/static_assert.hpp>
#define STATIC_ASSERT_SIZE_8( type ) BOOST_STATIC_ASSERT( sizeof(type) )

extern TaskManager * my_task_manager;

DECLARE_uint64( num_starting_workers );



///
/// Task routines
///


/// Spawn a task visible to this Node only
template < typename T >
void SoftXMT_privateTask( void (*fn_p)(T *), T arg) 
{
    STATIC_ASSERT_SIZE_8( T );
    DVLOG(5) << "Thread " << my_global_scheduler->get_current_thread() << " spawns private";
    my_task_manager->spawnLocalPrivate( fn_p, &arg );
}

/// Spawn a task visible to other Nodes
template < typename T >
void SoftXMT_publicTask( void (*fn_p)(T *), T arg) 
{
    STATIC_ASSERT_SIZE_8( T );
    DVLOG(5) << "Thread " << my_global_scheduler->get_current_thread() << " spawns public";
    my_task_manager->spawnPublic( fn_p, &arg );
}

/// Spawn and run user main function on node 0. Other nodes just run
/// existing threads (service threads) until they are given more to
/// do. TODO: get return values working
template < typename T >
int SoftXMT_run_user_main( void (* fn_p)(T *), T args )
{
  STATIC_ASSERT_SIZE_8( T );
  
  if( SoftXMT_mynode() == 0 ) {
    CHECK( CURRENT_THREAD == master_thread ); // this should only be run at the toplevel

    // create user_main as a private task
    SoftXMT_privateTask( fn_p, &args );
    DVLOG(5) << "Spawned user_main";
    
    // spawn 1 extra worker that will take user_main
    my_global_scheduler->createWorkers( 1 );
  }

  // spawn starting number of worker coroutines
  my_global_scheduler->createWorkers( FLAGS_num_starting_workers );

  // start the scheduler
  my_global_scheduler->run( );
}

/// remote task spawn
template< typename T >
struct remote_task_spawn_args {
    void (*fn_p)(T *);
    T userArgs;
};

template< typename T >
static void remote_task_spawn_am( remote_task_spawn_args<T> * args, size_t args_size, void* payload, size_t payload_size) {
  my_task_manager->spawnRemotePrivate(args->fn_p, &(args->userArgs) );
}

/// Spawn a private task on another Node
template< typename T >
void SoftXMT_remote_privateTask( void (*fn_p)(T*), T args, Node target) {
  STATIC_ASSERT_SIZE_8( T );
  
  remote_task_spawn_args<T> spawn_args = { fn_p, args };
  SoftXMT_call_on( target, SoftXMT_magic_identity_function(&remote_task_spawn_am<T>), &spawn_args );
  DVLOG(5) << "Sent AM to spawn private task on Node " << target;
}

#endif

