
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __GRAPPA_HPP__
#define __GRAPPA_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "Communicator.hpp"
#include "Worker.hpp"
#include "tasks/TaskingScheduler.hpp"
#include "PerformanceTools.hpp"

//#include <typeinfo>
//#include <cxxabi.h>

#define GRAPPA_TEST_ARGS &(boost::unit_test::framework::master_test_suite().argc), &(boost::unit_test::framework::master_test_suite().argv) 

namespace Grappa {

extern double tick_rate;

namespace impl {

/// global scheduler instance
extern TaskingScheduler global_scheduler;

/// called on failures to backtrace and pause for debugger
extern void failure_function();

}

/// Initialize Grappa. Call in SPMD context before running Grappa
/// code. Running Grappa code before calling init() is illegal.
void init( int * argc_p, char ** argv_p[], size_t size = -1 );

/// Clean up Grappa. Call in SPMD context after all Grappa code
/// finishes. Running Grappa code after calling finalize() is illegal.
int finalize();


/// pointer to parent pthread
extern Worker * master_thread;

}

/// for convenience/brevity, define macro to get current thread/worker
/// pointer
#define CURRENT_THREAD Grappa::impl::global_scheduler.get_current_thread()

extern Core * node_neighbors;

void Grappa_init( int * argc_p, char ** argv_p[], size_t size = -1 );

void Grappa_activate();

bool Grappa_done();
int Grappa_finish( int retval );


///
/// Worker management routines
///

/// Yield to scheduler, placing current Worker on run queue.
static inline void Grappa_yield( )
{
  /*bool immed =*/ Grappa::impl::global_scheduler.thread_yield( );
}

/// Yield to scheduler, placing current Worker on periodic queue.
static inline void Grappa_yield_periodic( )
{
  /*bool immed =*/ Grappa::impl::global_scheduler.thread_yield_periodic( );
}

/// Yield to scheduler, suspending current Worker.
static inline void Grappa_suspend( )
{
  DVLOG(5) << "suspending Worker " << Grappa::impl::global_scheduler.get_current_thread() << "(# " << Grappa::impl::global_scheduler.get_current_thread()->id << ")";
  Grappa::impl::global_scheduler.thread_suspend( );
  //CHECK_EQ(retval, 0) << "Worker " << th1 << " suspension failed. Have the server threads exited?";
}

/// Wake a Worker by putting it on the run queue, leaving the current thread running.
static inline void Grappa_wake( Grappa::Worker * t )
{
  DVLOG(5) << Grappa::impl::global_scheduler.get_current_thread()->id << " waking Worker " << t;
  Grappa::impl::global_scheduler.thread_wake( t );
}

/// Wake a Worker t by placing current thread on run queue and running t next.
static inline void Grappa_yield_wake( Grappa::Worker * t )
{
  DVLOG(5) << "yielding Worker " << Grappa::impl::global_scheduler.get_current_thread() << " and waking thread " << t;
  Grappa::impl::global_scheduler.thread_yield_wake( t );
}

/// Wake a Worker t by suspending current thread and running t next.
static inline void Grappa_suspend_wake( Grappa::Worker * t )
{
  DVLOG(5) << "suspending Worker " << Grappa::impl::global_scheduler.get_current_thread() << " and waking thread " << t;
  Grappa::impl::global_scheduler.thread_suspend_wake( t );
}

/// Place current thread on queue to be reused by tasking layer as a worker.
/// @deprecated should not be in the public API because it is a Worker-level not Task-level routine
static inline bool Grappa_thread_idle( )
{
  DVLOG(5) << "Worker " << Grappa::impl::global_scheduler.get_current_thread()->id << " going idle";
  return Grappa::impl::global_scheduler.thread_idle( );
}


/// TODO: remove this
void Grappa_signal_done( );

/// User main signal tasks done
void Grappa_end_tasks( );

/// Check initialized global queue
bool Grappa_global_queue_isInit( );

#include "Aggregator.hpp"

/// Poll Grappa aggregation and communication layers.
static inline void Grappa_poll()
{
  global_aggregator.poll();
#ifdef ENABLE_RDMA_AGGREGATOR
  Grappa::impl::global_rdma_aggregator.poll();
#endif
}

/// Send waiting aggregated messages to a particular destination.
static inline void Grappa_flush( Core n )
{
  global_aggregator.flush( n );
}

/// Meant to be called when there's no other work to be done, calls poll, 
/// flushes any aggregator buffers with anything in them, and deaggregates.
static inline void Grappa_idle_flush_poll() {
  global_aggregator.idle_flush_poll();
}

#include "Addressing.hpp"
#include "Tasking.hpp"
#include "StateTimer.hpp"

#endif
