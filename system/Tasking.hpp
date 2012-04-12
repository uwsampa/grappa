
#ifndef __TASKING_HPP__
#define __TASKING_HPP__

//#include "SoftXMT.hpp"
#include "tasks/Task.hpp"

extern TaskManager * my_task_manager;

DECLARE_uint64( num_starting_workers );

///
/// Task routines
///


/// Spawn a task visible to this Node only
template < typename ArgsStruct >
void SoftXMT_privateTask( void (*fn_p)(ArgsStruct * arg), ArgsStruct * arg) 
{
    DVLOG(5) << "Thread " << my_global_scheduler->get_current_thread() << " spawns private";
    my_task_manager->spawnPrivate( fn_p, arg );
}

/// Spawn a task visible to other Nodes
template < typename ArgsStruct >
void SoftXMT_publicTask( void (*fn_p)(ArgsStruct * arg), ArgsStruct * arg) 
{
    DVLOG(5) << "Thread " << my_global_scheduler->get_current_thread() << " spawns public";
    my_task_manager->spawnPublic( fn_p, arg );
}

/// Spawn and run user main function on node 0. Other nodes just run
/// existing threads (service threads) until they are given more to
/// do. TODO: get return values working
template < typename ArgsStruct >
int SoftXMT_run_user_main( void (* fn_p)(ArgsStruct *), ArgsStruct * args )
{
  if( SoftXMT_mynode() == 0 ) {
    CHECK( CURRENT_THREAD == master_thread ); // this should only be run at the toplevel

    // create user_main as a private task
    SoftXMT_privateTask( fn_p, args );
    DVLOG(5) << "Spawned user_main";
    
    // spawn 1 extra worker that will take user_main
    my_global_scheduler->createWorkers( 1 );
  }

  // spawn starting number of worker coroutines
  my_global_scheduler->createWorkers( FLAGS_num_starting_workers );

  // start the scheduler
  my_global_scheduler->run( );
}

template< typename T >
static void remote_task_spawn_am(T* args, size_t args_size, void* payload, size_t payload_size) {
  typedef void (*task_fn)(T*);
  void (*fn_p)(T*) = *reinterpret_cast<task_fn*>(payload);
  T* aa = new T;
  *aa = *args;
  SoftXMT_privateTask(fn_p, aa);
}

/// Spawn a private task on another Node
template< typename T >
void SoftXMT_remote_privateTask( void (*fn_p)(T*), const T* args, Node target) {
  // typedef void (*am_t)(T*,size_t,void*,size_t);
  // am_t a = &am_remote_spawn<T>;
  SoftXMT_call_on(target, SoftXMT_magic_identity_function(&remote_task_spawn_am<T>), args, sizeof(T), (void*)&fn_p, sizeof(fn_p));
  DVLOG(5) << "Sent AM to spawn private task on Node " << target;
}

#endif

