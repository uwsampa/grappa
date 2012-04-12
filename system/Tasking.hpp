
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

///// Spawn a private task on another Node
//template < typename ArgsStruct >
//void SoftXMT_remotePrivateTask( Node target, void (*fn_p)(ArgsStruct * arg), ArgsStruct * arg)
//{
//    SoftXMT_call_on( target, &remote_task_spawn_am, arg );
//}
//

#endif
