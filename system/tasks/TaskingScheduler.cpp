
#include "TaskingScheduler.hpp"
#include "Task.hpp"

#include <gflags/gflags.h>
#include "../PerformanceTools.hpp"

GRAPPA_DEFINE_EVENT_GROUP(scheduler);

/// TODO: this should be based on some actual time-related metric so behavior is predictable across machines
DEFINE_int64( periodic_poll_ticks, 500, "number of ticks to wait before polling periodic queue");

DEFINE_bool(flush_on_idle, true, "have tasking layer flush all aggregations if it has nothing better to do");

// for debugging
//int64_t num_active_tasks;
//int64_t task_calls;
//int64_t task_log_index;
//short active_task_log[1<<20];
//
//int64_t max_active;
//double  avg_active;
TaskingScheduler global_scheduler;

TaskingScheduler::TaskingScheduler ( )
    : readyQ ( )
    , periodicQ ( )
    , unassignedQ ( )
    , master ( NULL )
    , current_thread ( NULL )
    , nextId ( 1 )
    , num_idle ( 0 )
    , num_workers ( 0 )
    , task_manager ( NULL )
    , work_args( NULL )
    , previous_periodic_ts( 0 ) 
    , stats( this )
{ }

void TaskingScheduler::init ( Thread * master_arg, TaskManager * taskman ) {
  master = master_arg;
  current_thread = master;
  task_manager = taskman;
  work_args = new task_worker_args( taskman, this );
}

void TaskingScheduler::run ( ) {
    StateTimer::setThreadState( StateTimer::SCHEDULER );
    StateTimer::enterState_scheduler();
    while (thread_wait( NULL ) != NULL) { } // nothing
}

void TaskingScheduler::thread_join( Thread * wait_on ) {
    while ( !wait_on->done ) {
        wait_on->joinqueue.enqueue( current_thread );
        thread_suspend( );
    }
}


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


/// create worker threads for executing tasks
void TaskingScheduler::createWorkers( uint64_t num ) {
    num_workers += num;
    VLOG(5) << "spawning " << num << " workers; now there are " << num_workers;
    for (int i=0; i<num; i++) {
        Thread * t = thread_spawn( current_thread, this, workerLoop, work_args);
        unassigned( t );
    }
    num_idle += num;
}

#define BASIC_MAX_WORKERS 2
Thread * TaskingScheduler::maybeSpawnCoroutines( ) {
    if ( num_workers < BASIC_MAX_WORKERS ) {
       num_workers += 1;
       VLOG(5) << "spawning another worker; now there are " << num_workers;
       return thread_spawn( current_thread, this, workerLoop, work_args ); // current Thread will be coro parent; is this okay?
    } else {
        // might have another way to spawn
        return NULL;
    }
}

void TaskingScheduler::onWorkerStart( ) {
    num_idle--;
}

bool TaskingScheduler::queuesFinished( ) {
    return periodicQ.empty() && task_manager->isWorkDone();
}

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

void TaskingScheduler::TaskingSchedulerStatistics::profiling_sample() {
#ifdef VTRACE_SAMPLED
    VT_COUNT_UNSIGNED_VAL( active_tasks_out_ev_vt, sched->num_active_tasks);
    VT_COUNT_UNSIGNED_VAL(num_idle_out_ev_vt, sched->num_idle);
    VT_COUNT_UNSIGNED_VAL(readyQ_size_ev_vt, sched->readyQ.length());
#endif
}

void TaskingScheduler::TaskingSchedulerStatistics::merge(TaskingSchedulerStatistics * other) {
  task_calls += other->task_calls;
  merged++;
  max_active = (int64_t)inc_avg((double)max_active, merged, (double)other->max_active);
  avg_active = inc_avg(avg_active, merged, other->avg_active);
  avg_ready = inc_avg(avg_ready, merged, other->avg_ready);
}

void TaskingScheduler::TaskingSchedulerStatistics::merge_am(TaskingScheduler::TaskingSchedulerStatistics * other, size_t sz, void* payload, size_t psz) {
  global_scheduler.stats.merge(other);
}



