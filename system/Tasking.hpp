#include "SoftXMT.hpp"
#include "tasks/Task.hpp"

extern TaskManager * my_task_manager;

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



///// Spawn a private task on another Node
//template < typename ArgsStruct >
//void SoftXMT_remotePrivateTask( Node target, void (*fn_p)(ArgsStruct * arg), ArgsStruct * arg)
//{
//    SoftXMT_call_on( target, &remote_task_spawn_am, arg );
//}
//
