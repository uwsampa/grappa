
#ifndef __SOFTXMT_HPP__
#define __SOFTXMT_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "Communicator.hpp"
#include "thread.h"

void SoftXMT_init( int * argc_p, char ** argv_p[] );
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

#include "Aggregator.hpp"
#include "Addressing.hpp"

#endif
