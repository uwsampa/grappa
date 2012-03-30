
#ifndef __SOFTXMT_HPP__
#define __SOFTXMT_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "Communicator.hpp"
#include "tasks/Thread.hpp"

//#include <typeinfo>
//#include <cxxabi.h>

extern TaskingScheduler * my_global_scheduler;
thread * TaskingScheduler::get_current_thread();
#define CURRENT_THREAD my_global_scheduler->get_current_thread()

void SoftXMT_init( int * argc_p, char ** argv_p[], size_t size = 4096 );
void SoftXMT_activate();

bool SoftXMT_done();
void SoftXMT_finish( int retval );

Node SoftXMT_nodes();
Node SoftXMT_mynode();

void SoftXMT_poll();
void SoftXMT_flush( Node n );

void SoftXMT_barrier();

void SoftXMT_barrier_commsafe();

/// Spawn and run user main function. TODO: get return values working
/// TODO: remove thread * arg
int SoftXMT_run_user_main( void (* fn_p)(thread *, void *), void * args );

/// Spawn a user function. TODO: get return values working
/// TODO: remove thread * arg
thread * SoftXMT_spawn( void (* fn_p)(thread *, void *), void * args );

template< typename T >
thread * SoftXMT_template_spawn( void (* fn_p)(thread *, T *), T * args )
{
  typedef void (* fn_t)(thread *, void *);

  thread * th = SoftXMT_spawn( (fn_t)fn_p, (void *)args );
  DVLOG(5) << "Spawned thread " << th;
  return th;
}

/// Active message for spawning a thread on a remote node (used by SoftXMT_remote_spawn())
template< typename T >
static void am_remote_spawn(T* args, size_t args_size, void* payload, size_t payload_size) {
  typedef void (*thread_fn)(thread*,T*);
  void (*fn_p)(thread*,T*) = *reinterpret_cast<thread_fn*>(payload);
  T* aa = new T;
  *aa = *args;
  SoftXMT_template_spawn(fn_p, aa);
}

/// Spawn a user thread on a remote node. Copies the passed arguments 
/// to the remote node.
/// Note: the thread function should take ownership of the arguments 
/// and clean them up at the end of the function call.
template< typename T >
void SoftXMT_remote_spawn( void (*fn_p)(thread*,T*), const T* args, Node target) {
  // typedef void (*am_t)(T*,size_t,void*,size_t);
  // am_t a = &am_remote_spawn<T>;
  SoftXMT_call_on(target, SoftXMT_magic_identity_function(&am_remote_spawn<T>), args, sizeof(T), (void*)&fn_p, sizeof(fn_p));
  DVLOG(5) << "Sent AM to spawn thread on Node " << target;
}
template< typename T >
void SoftXMT_remote_spawn( void (*fn_p)(thread*,void*), const T* args, Node target) {
  // typedef void (*am_t)(T*,size_t,void*,size_t);
  // am_t a = &am_remote_spawn<T>;
  SoftXMT_call_on(target, SoftXMT_magic_identity_function(&am_remote_spawn<T>), args, sizeof(T), (void*)&fn_p, sizeof(fn_p));
  DVLOG(5) << "Sent AM to spawn thread on Node " << target;
}


/// Yield to scheduler, placing current thread on run queue.
void SoftXMT_yield( );

/// Yield to scheduler, suspending current thread.
void SoftXMT_suspend( );

/// Wake a thread by putting it on the run queue, leaving the current thread running.
void SoftXMT_wake( thread * t );

/// Wake a thread t by placing current thread on run queue and running t next.
void SoftXMT_yield_wake( thread * t );

/// Wake a thread t by suspending current thread and running t next.
void SoftXMT_suspend_wake( thread * t );

/// Join on thread t
void SoftXMT_join( thread * t );

/// TODO: remove this
void SoftXMT_signal_done();


/// Memory management routines.

// // allocate a block of memory from generic pool
// GlobalAddress< void > SoftXMT_malloc( size_t size_bytes );

// // free a block of memory from generic pool
// void SoftXMT_free( GlobalAddress< void > pointer );

// /// allocate some number of some type of object
// template< typename T >
// GlobalAddress< T > SoftXMT_typed_malloc( size_t count );

#include "Aggregator.hpp"
#include "Addressing.hpp"

#endif
