// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "TaskingScheduler.hpp"
#include "Task.hpp"

#include <gflags/gflags.h>
#include "../PerformanceTools.hpp"

GRAPPA_DEFINE_EVENT_GROUP(scheduler);

/// TODO: this should be based on some actual time-related metric so behavior is predictable across machines
//DEFINE_int64( periodic_poll_ticks, 500, "number of ticks to wait before polling periodic queue");

DEFINE_bool(poll_on_idle, true, "have tasking layer poll aggregator if it has nothing better to do");

DEFINE_int64( stats_blob_ticks, 3000000000L, "number of ticks to wait before dumping stats blob");


namespace Grappa {

  namespace impl {

/// global TaskingScheduler for this Node
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
  , prev_ts( 0 )
  , prev_stats_blob_ts( 0 )
    , stats( this )
{ 
  Grappa_tick();
  prev_ts = Grappa_get_timestamp();
}

/// Initialize with references to master Thread and a TaskManager.
void TaskingScheduler::init ( Thread * master_arg, TaskManager * taskman ) {
  master = master_arg;
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

/// Join on a Thread. 
/// This is a low level synchronization mechanism specifically for Threads
void TaskingScheduler::thread_join( Thread * wait_on ) {
  while ( !wait_on->done ) {
    wait_on->joinqueue.enqueue( current_thread );
    thread_suspend( );
  }
}

/// Schedule Threads from the scheduler until one e
/// If <result> non-NULL, store the Thread's exit value there.
/// This routine is only to be called if the current Thread
/// is the master Thread (the one returned by thread_init()).
/// @return the exited Thread, or NULL if scheduler is done
Thread * TaskingScheduler::thread_wait( void **result ) {
  CHECK( current_thread == master ) << "only meant to be called by system Thread";

  Thread * next = nextCoroutine( false );
  if (next == NULL) {
    // no user threads remain
    return NULL;
  } else {
    current_thread = next;

    Thread * died = (Thread *) thread_context_switch( master, next, NULL);

    // At the moment, we only return from a Thread in the case of death.

    if (result != NULL) {
      void *retval = (void *)died->next;
      *result = retval;
    }
    return died;
  }
}

/// Get an idle worker Thread that can be paired with a Task.
///
/// Threads returned by this method are specifically running workerLoop().
Thread * TaskingScheduler::getWorker () {
  if (task_manager->available()) {
    // check the pool of unassigned coroutines
    Thread * result = unassignedQ.dequeue();
    if (result != NULL) return result;

    // possibly spawn more coroutines
    result = maybeSpawnCoroutines();
    VLOG_IF(3, result==NULL) << "run out of coroutines";
    return result;
  } else {
    return NULL;
  }
}

/// Worker Thread routine.
/// This is the routine every worker thread runs.
/// The worker thread continuously asks for Tasks and executes them.
///
/// Note that worker threads are NOT guarenteed to ever call Thread.exit()
/// before the program ends.
void workerLoop ( Thread * me, void* args ) {
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
    // spawn a new worker Thread
    Thread * t = thread_spawn( current_thread, this, workerLoop, work_args);

    // place the Thread in the pool of idle workers
    unassigned( t );
  }
  num_idle += num;
}

#define BASIC_MAX_WORKERS 2
/// Give the scheduler a chance to spawn more worker Threads,
/// based on some heuristics.
Thread * TaskingScheduler::maybeSpawnCoroutines( ) {
  // currently only spawn a worker if there are less than some threshold
  if ( num_workers < BASIC_MAX_WORKERS ) {
    num_workers += 1;
    VLOG(5) << "spawning another worker; now there are " << num_workers;
    return thread_spawn( current_thread, this, workerLoop, work_args ); // current Thread will be coro parent; is this okay?
  } else {
    // might have another way to spawn
    return NULL;
  }
}

/// Callback for when a worker Thread is run for the first time.
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


void TaskingScheduler::TaskingSchedulerStatistics::sample() {
  task_calls++;
  if (sched->num_active_tasks > max_active) max_active = sched->num_active_tasks;
  avg_active = inc_avg(avg_active, task_calls, sched->num_active_tasks);
  avg_ready = inc_avg(avg_ready, task_calls, sched->readyQ.length());
#ifdef DEBUG  
  if ((task_calls % 1024) == 0) {
    active_task_log[task_log_index++] = sched->num_active_tasks;
  }
#endif

  GRAPPA_EVENT(active_tasks_out_ev, "Active tasks sample", SAMPLE_RATE, scheduler, sched->num_active_tasks);
  GRAPPA_EVENT(num_idle_out_event, "Idle workers sample", SAMPLE_RATE, scheduler, sched->num_idle);
  GRAPPA_EVENT(readyQ_size_ev, "readyQ size", SAMPLE_RATE, scheduler, sched->readyQ.length());

#ifdef VTRACE
  //VT_COUNT_UNSIGNED_VAL( active_tasks_out_ev_vt, sched->num_active_tasks);
  //VT_COUNT_UNSIGNED_VAL(num_idle_out_ev_vt, sched->num_idle);
  //VT_COUNT_UNSIGNED_VAL(readyQ_size_ev_vt, sched->readyQ.length());
#endif

  //#ifdef GRAPPA_TRACE
  //    if ((task_calls % 1) == 0) {
  //        TAU_REGISTER_EVENT(active_tasks_out_event, "Active tasks sample");
  //        TAU_REGISTER_EVENT(num_idle_out_event, "Idle workers sample");
  //
  //        TAU_EVENT(active_tasks_out_event, sched->num_active_tasks);
  //        TAU_EVENT(num_idle_out_event, sched->num_idle);
  //
  //    }
  //#endif
}

/// Take a profiling sample of scheduler stats and state.
void TaskingScheduler::TaskingSchedulerStatistics::profiling_sample() {
#ifdef VTRACE_SAMPLED
  VT_COUNT_UNSIGNED_VAL( active_tasks_out_ev_vt, sched->num_active_tasks);
  VT_COUNT_UNSIGNED_VAL(num_idle_out_ev_vt, sched->num_idle);
  VT_COUNT_UNSIGNED_VAL(readyQ_size_ev_vt, sched->readyQ.length());
#endif
}

/// Merge other statistics into this one.
void TaskingScheduler::TaskingSchedulerStatistics::merge(const TaskingSchedulerStatistics * other) {
  task_calls += other->task_calls;
  // if *this has not been merged with others, then copy int timers to double timers
  if (merged==1) {
    for (int i=StatePoll; i<StateLast; i++) state_timers_d[i] = state_timers[i];
  }
  // if *other has not been merged with others, then use its int timers (double are invalid);
  // otherwise use its double timers (int are unmerged)
  if ( other->merged == 1 ) {
    for (int i=StatePoll; i<StateLast; i++) state_timers_d[i] += other->state_timers[i];
  } else {
    for (int i=StatePoll; i<StateLast; i++) state_timers_d[i] += other->state_timers_d[i];
  }
  scheduler_count += other->scheduler_count;

  merged+=other->merged;
  max_active = (int64_t)inc_avg((double)max_active, merged, (double)other->max_active);
  avg_active = inc_avg(avg_active, merged, other->avg_active);
  avg_ready = inc_avg(avg_ready, merged, other->avg_ready);
}

} // impl
} // Grappa
