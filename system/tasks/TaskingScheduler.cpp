
#include "TaskingScheduler.hpp"
#include "Task.hpp"

#include <gflags/gflags.h>

/// TODO: this should be based on some actual time-related metric so behavior is predictable across machines
DEFINE_int64( periodic_poll_ticks, 500, "number of ticks to wait before polling periodic queue");

TaskingScheduler::TaskingScheduler ( Thread * master, TaskManager * taskman ) 
    : readyQ ( )
    , periodicQ ( )
    , unassignedQ ( )
    , master ( master )
    , current_thread ( master )
    , nextId ( 1 )
    , num_idle ( 0 )
    , num_workers ( 0 )
    , task_manager ( taskman )
    , work_args( new task_worker_args( taskman, this ) )
    , previous_periodic_ts( 0 ) {

          periodctr = 0;/*XXX*/
}

void TaskingScheduler::run ( ) {
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
    
    Task nextTask;
   
    while ( true ) {
        // block until receive work or termination reached
        if (!tasks->getWork(&nextTask)) break; // quitting time

        nextTask.execute();

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
