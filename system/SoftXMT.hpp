
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __SOFTXMT_HPP__
#define __SOFTXMT_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "Communicator.hpp"
#include "tasks/Thread.hpp"
#include "tasks/TaskingScheduler.hpp"
#include "PerformanceTools.hpp"

//#include <typeinfo>
//#include <cxxabi.h>

/// global scheduler instance
extern TaskingScheduler global_scheduler;

/// pointer to parent pthread
extern Thread * master_thread;

/// for convenience/brevity, define macro to get current thread/worker
/// pointer
#define CURRENT_THREAD global_scheduler.get_current_thread()

extern Node * node_neighbors;


void SoftXMT_init( int * argc_p, char ** argv_p[], size_t size = -1 );
void SoftXMT_activate();

bool SoftXMT_done();
void SoftXMT_finish( int retval );


///
/// Thread management routines
///

/// Yield to scheduler, placing current Thread on run queue.
static inline void SoftXMT_yield( )
{
  /*bool immed =*/ global_scheduler.thread_yield( );
}

/// Yield to scheduler, placing current Thread on periodic queue.
static inline void SoftXMT_yield_periodic( )
{
  /*bool immed =*/ global_scheduler.thread_yield_periodic( );
}

/// Yield to scheduler, suspending current Thread.
static inline void SoftXMT_suspend( )
{
  DVLOG(5) << "suspending Thread " << global_scheduler.get_current_thread() << "(# " << global_scheduler.get_current_thread()->id << ")";
  global_scheduler.thread_suspend( );
  //CHECK_EQ(retval, 0) << "Thread " << th1 << " suspension failed. Have the server threads exited?";
}

/// Wake a Thread by putting it on the run queue, leaving the current thread running.
static inline void SoftXMT_wake( Thread * t )
{
  DVLOG(5) << global_scheduler.get_current_thread()->id << " waking Thread " << t;
  global_scheduler.thread_wake( t );
}

/// Wake a Thread t by placing current thread on run queue and running t next.
static inline void SoftXMT_yield_wake( Thread * t )
{
  DVLOG(5) << "yielding Thread " << global_scheduler.get_current_thread() << " and waking thread " << t;
  global_scheduler.thread_yield_wake( t );
}

/// Wake a Thread t by suspending current thread and running t next.
static inline void SoftXMT_suspend_wake( Thread * t )
{
  DVLOG(5) << "suspending Thread " << global_scheduler.get_current_thread() << " and waking thread " << t;
  global_scheduler.thread_suspend_wake( t );
}

/// Place current thread on queue to be reused by tasking layer as a worker.
/// @deprecated should not be in the public API because it is a Thread-level not Task-level routine
static inline bool SoftXMT_thread_idle( )
{
  DVLOG(5) << "Thread " << global_scheduler.get_current_thread()->id << " going idle";
  return global_scheduler.thread_idle( );
}


///
/// Query job info
///

/// How many nodes are there?
static inline Node SoftXMT_nodes() {
  return global_communicator.nodes();
}

/// What is my node?
static inline Node SoftXMT_mynode() {
  return global_communicator.mynode();
}


///
/// Communication utilities
///

/// Enter global barrier.
static inline void SoftXMT_barrier() {
  global_communicator.barrier();
}

/// Enter global barrier that is poller safe
static inline void SoftXMT_barrier_commsafe() {
    global_communicator.barrier_notify();
    while (!global_communicator.barrier_try()) {
        SoftXMT_yield();
    }
}

void SoftXMT_barrier_suspending();




/// Spawn a user function. TODO: get return values working
/// TODO: remove Thread * arg
Thread * SoftXMT_spawn( void (* fn_p)(Thread *, void *), void * args );

template< typename T >
Thread * SoftXMT_template_spawn( void (* fn_p)(Thread *, T *), T * args )
{
  typedef void (* fn_t)(Thread *, void *);

  Thread * th = SoftXMT_spawn( (fn_t)fn_p, (void *)args );
  DVLOG(5) << "Spawned Thread " << th;
  return th;
}

/// Active message for spawning a Thread on a remote node (used by SoftXMT_remote_spawn())
template< typename T >
static void am_remote_spawn(T* args, size_t args_size, void* payload, size_t payload_size) {
  typedef void (*thread_fn)(Thread*,T*);
  void (*fn_p)(Thread*,T*) = *reinterpret_cast<thread_fn*>(payload);
  T* aa = new T;
  *aa = *args;
  SoftXMT_template_spawn(fn_p, aa);
}

/// Spawn a user Thread on a remote node. Copies the passed arguments 
/// to the remote node.
/// Note: the Thread function should take ownership of the arguments 
/// and clean them up at the end of the function call.
template< typename T >
void SoftXMT_remote_spawn( void (*fn_p)(Thread*,T*), const T* args, Node target) {
  // typedef void (*am_t)(T*,size_t,void*,size_t);
  // am_t a = &am_remote_spawn<T>;
  SoftXMT_call_on(target, SoftXMT_magic_identity_function(&am_remote_spawn<T>), args, sizeof(T), (void*)&fn_p, sizeof(fn_p));
  DVLOG(5) << "Sent AM to spawn Thread on Node " << target;
}
template< typename T >
void SoftXMT_remote_spawn( void (*fn_p)(Thread*,void*), const T* args, Node target) {
  // typedef void (*am_t)(T*,size_t,void*,size_t);
  // am_t a = &am_remote_spawn<T>;
  SoftXMT_call_on(target, SoftXMT_magic_identity_function(&am_remote_spawn<T>), args, sizeof(T), (void*)&fn_p, sizeof(fn_p));
  DVLOG(5) << "Sent AM to spawn Thread on Node " << target;
}

/// TODO: remove this
void SoftXMT_signal_done( );

/// User main signal tasks done
void SoftXMT_end_tasks( );

/// dump stats for this node
void SoftXMT_dump_stats();
/// dump stats for all nodes
void SoftXMT_dump_stats_all_nodes();

/// reset stats on this node
void SoftXMT_reset_stats();
/// reset stats on all nodes
void SoftXMT_reset_stats_all_nodes();

/// merge stats from all other nodes with this one and dump this node's stats
void SoftXMT_merge_and_dump_stats();

void SoftXMT_dump_task_series();





#include "Aggregator.hpp"

/// Poll SoftXMT aggregation and communication layers.
static inline void SoftXMT_poll()
{
  global_aggregator.poll();
}

/// Send waiting aggregated messages to a particular destination.
static inline void SoftXMT_flush( Node n )
{
  global_aggregator.flush( n );
}

/// Meant to be called when there's no other work to be done, calls poll, 
/// flushes any aggregator buffers with anything in them, and deaggregates.
static inline void SoftXMT_idle_flush_poll() {
  global_aggregator.idle_flush_poll();
}

void SoftXMT_take_profiling_sample();

#include "Addressing.hpp"
#include "Tasking.hpp"
#include "StateTimer.hpp"

#endif
