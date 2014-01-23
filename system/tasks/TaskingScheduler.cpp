////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////
#include "TaskingScheduler.hpp"
#include "Task.hpp"

#include <gflags/gflags.h>
#include "../PerformanceTools.hpp"

/// TODO: this should be based on some actual time-related metric so behavior is predictable across machines
DEFINE_int64( periodic_poll_ticks, 20000, "number of ticks to wait before polling periodic queue");

DEFINE_bool(poll_on_idle, true, "have tasking layer poll aggregator if it has nothing better to do");

DEFINE_int64( stats_blob_ticks, 300000000000L, "number of ticks to wait before dumping stats blob");

DEFINE_uint64( readyq_prefetch_distance, 4, "How far ahead in the ready queue to prefetch contexts" );

GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, scheduler_context_switches, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, scheduler_count, 0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, scheduler_samples, 0);

// set in sample()
GRAPPA_DEFINE_METRIC( SummarizingMetric<uint64_t>, active_tasks_sampled, 0);
GRAPPA_DEFINE_METRIC( SummarizingMetric<uint64_t>, ready_tasks_sampled, 0);
GRAPPA_DEFINE_METRIC( SummarizingMetric<uint64_t>, idle_workers_sampled, 0);

// ticks
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, scheduler_polling_thread_ticks, 0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, scheduler_ready_thread_ticks, 0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, scheduler_idle_thread_ticks, 0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, scheduler_idle_useful_thread_ticks, 0);


namespace Grappa {

  namespace impl {

/// global TaskingScheduler for this Core
TaskingScheduler global_scheduler;

/// Create uninitialized TaskingScheduler.
/// init() must subsequently be called before fully initialized.
  TaskingScheduler::TaskingScheduler ( )
  : readyQ ( )
  , periodicQ ( )
  , unassignedQ ( )
  , master ( NULL )
  , current_thread ( NULL )
  , nextId ( 1 )
  , num_idle ( 0 )
  , num_active_tasks( 0 )
  , task_manager ( NULL )
  , num_workers ( 0 )
  , work_args( NULL )
  , previous_periodic_ts( 0 ) 
  , in_no_switch_region_( false )
  , prev_ts( 0 )
  , prev_stats_blob_ts( 0 )
    , stats( this )
{ 
  Grappa::tick();
  prev_ts = Grappa::timestamp();
}

/// Initialize with references to master Worker and a TaskManager.
void TaskingScheduler::init ( Worker * master_arg, TaskManager * taskman ) {
  master = master_arg;
  readyQ.init( FLAGS_readyq_prefetch_distance );
  current_thread = master;
  task_manager = taskman;
  work_args = new task_worker_args( taskman, this );
}

/// Give control to the scheduler until task layer
/// says that it is finished.
void TaskingScheduler::run ( ) {
  StateTimer::setThreadState( StateTimer::SCHEDULER );
  StateTimer::enterState_scheduler();
  while (thread_wait( NULL ) != NULL) { } // nothing
}

/// Schedule Threads from the scheduler until one e
/// If <result> non-NULL, store the Worker's exit value there.
/// This routine is only to be called if the current Worker
/// is the master Worker (the one returned by convert_to_master()).
/// @return the exited Worker, or NULL if scheduler is done
Worker * TaskingScheduler::thread_wait( void **result ) {
  CHECK( current_thread == master ) << "only meant to be called by system Worker";

  Worker * next = nextCoroutine( false );
  if (next == NULL) {
    // no user threads remain
    return NULL;
  } else {
    current_thread = next;

    Worker * died = (Worker *) thread_context_switch( master, next, NULL);

    // At the moment, we only return from a Worker in the case of death.

    if (result != NULL) {
      void *retval = (void *)died->next;
      *result = retval;
    }
    return died;
  }
}

/// Get an idle worker Worker that can be paired with a Task.
///
/// Threads returned by this method are specifically running workerLoop().
Worker * TaskingScheduler::getWorker () {
  if (task_manager->available()) {
    // check the pool of unassigned coroutines
    Worker * result = unassignedQ.dequeue();
    if (result != NULL) return result;

    // possibly spawn more coroutines
    result = maybeSpawnCoroutines();
    VLOG_IF(3, result==NULL) << "run out of coroutines";
    return result;
  } else {
    return NULL;
  }
}

/// Worker Worker routine.
/// This is the routine every worker thread runs.
/// The worker thread continuously asks for Tasks and executes them.
///
/// Note that worker threads are NOT guarenteed to ever call Worker.exit()
/// before the program ends.
void workerLoop ( Worker * me, void* args ) {
  task_worker_args* wargs = (task_worker_args*) args;
  TaskManager* tasks = wargs->tasks;
  TaskingScheduler * sched = wargs->scheduler; 

  sched->onWorkerStart();

  StateTimer::setThreadState( StateTimer::FINDWORK );
  StateTimer::enterState_findwork();

  Task nextTask;

  while ( true ) {
    // block until receive work or termination reached
    if (!tasks->getWork(&nextTask)) break; // quitting time

    sched->num_active_tasks++;
    StateTimer::setThreadState( StateTimer::USER );
    StateTimer::enterState_user();
    {
      GRAPPA_PROFILE( exectimer, "user_execution", "", GRAPPA_USER_GROUP );
      nextTask.execute();
    }
    StateTimer::setThreadState( StateTimer::FINDWORK );
    sched->num_active_tasks--;

    sched->thread_yield( ); // yield to the scheduler
  }
}


/// create worker Threads for executing Tasks
///
/// @param num how many workers to create
void TaskingScheduler::createWorkers( uint64_t num ) {
  num_workers += num;
  VLOG(5) << "spawning " << num << " workers; now there are " << num_workers;
  for (uint64_t i=0; i<num; i++) {
    // spawn a new worker Worker
    Worker * t = impl::worker_spawn( current_thread, this, workerLoop, work_args);

    // place the Worker in the pool of idle workers
    unassigned( t );
  }
  num_idle += num;
}

#define BASIC_MAX_WORKERS 2
/// Give the scheduler a chance to spawn more worker Threads,
/// based on some heuristics.
Worker * TaskingScheduler::maybeSpawnCoroutines( ) {
  // currently only spawn a worker if there are less than some threshold
  if ( num_workers < BASIC_MAX_WORKERS ) {
    num_workers += 1;
    VLOG(5) << "spawning another worker; now there are " << num_workers;
    return impl::worker_spawn( current_thread, this, workerLoop, work_args ); // current Worker will be coro parent; is this okay?
  } else {
    // might have another way to spawn
    return NULL;
  }
}

/// Callback for when a worker Worker is run for the first time.
void TaskingScheduler::onWorkerStart( ) {
  // all worker Threads start in the idle worker pool (unassigned)
  // so decrement num_idle when it starts.
  num_idle--;
}

/// Are there anymore threads to run?
bool TaskingScheduler::queuesFinished( ) {
  // there are no more threads to run if the periodicQueue is empty
  // and the TaskManager has terminated
  return periodicQ.empty() && task_manager->isWorkDone();
}

/// Print representation of TaskingScheduler
std::ostream& operator<<( std::ostream& o, const TaskingScheduler& ts ) {
  return ts.dump( o );
}




/*
 * TaskingSchedulerMetrics
 *
 */

TaskingScheduler::TaskingSchedulerMetrics::TaskingSchedulerMetrics() { active_task_log = new short[16]; }  
        
TaskingScheduler::TaskingSchedulerMetrics::TaskingSchedulerMetrics( TaskingScheduler * scheduler )
  : sched( scheduler ) 
  , state_timers()
    , prev_state( StateIdle )

{
  state_timers[ StatePoll ]  = &scheduler_polling_thread_ticks;
  state_timers[ StateReady ] = &scheduler_ready_thread_ticks;
  state_timers[ StateIdle ]  = &scheduler_idle_thread_ticks;
  state_timers[ StateIdleUseful ] = &scheduler_idle_useful_thread_ticks;

  active_task_log = new short[1L<<20];
  reset();
}
        
TaskingScheduler::TaskingSchedulerMetrics::~TaskingSchedulerMetrics() {
// XXX: not copy-safe, so pointer can be invalid
/* delete[] active_task_log; */
}
        
void TaskingScheduler::TaskingSchedulerMetrics::reset() {
  task_log_index = 0;
}
        
void TaskingScheduler::TaskingSchedulerMetrics::print_active_task_log() {
#ifdef DEBUG
  if (task_log_index == 0) return;

  std::stringstream ss;
  for (int64_t i=0; i<task_log_index; i++) ss << active_task_log[i] << " ";
  LOG(INFO) << "Active tasks log: " << ss.str();
#endif
}

void TaskingScheduler::TaskingSchedulerMetrics::sample() {
  scheduler_samples++;
  active_tasks_sampled += sched->num_active_tasks;
  ready_tasks_sampled += sched->readyQ.length();
  idle_workers_sampled += sched->num_idle;

#ifdef DEBUG  
  if ((scheduler_samples.value() % 1024) == 0) {
    active_task_log[task_log_index++] = sched->num_active_tasks;
  }
#endif

}


} // impl
} // Grappa
