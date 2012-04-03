#include "TaskingScheduler.hpp"
#include "Task.hpp"

TaskingScheduler::TaskingScheduler ( thread * master, TaskManager * taskman ) 
    : readyQ ( )
    , periodicQ ( )
    , unassignedQ ( )
    , master ( master )
    , current_thread ( master )
    , nextId ( 1 )
    , num_idle ( 0 )
    , num_workers ( 0 )
    , task_manager ( taskman )
    , work_args( new task_worker_args( taskman, this ) ) { 

          periodctr = 0;/*XXX*/
}

void TaskingScheduler::run ( ) {
    while (thread_wait( NULL ) != NULL) { } // nothing
}

void TaskingScheduler::thread_join( thread* wait_on ) {
    while ( !wait_on->done ) {
        wait_on->joinqueue.enqueue( current_thread );
        thread_suspend( );
    }
}


thread * TaskingScheduler::thread_wait( void **result ) {
    CHECK( current_thread == master ) << "only meant to be called by system thread";

    thread * next = nextCoroutine( false );
    if (next == NULL) {
        // no user threads remain
        return NULL;
    } else {
        current_thread = next;

        thread * died = (thread *) thread_context_switch( master, next, NULL);

        // At the moment, we only return from a thread in the case of death.

        if (result != NULL) {
            void *retval = (void *)died->next;
            *result = retval;
        }
        return died;
    }
}


thread* TaskingScheduler::getWorker () {
    if (task_manager->available()) {
        // check the pool of unassigned coroutines
        thread* result = unassignedQ.dequeue();
        if (result != NULL) return result;

        // possibly spawn more coroutines
        result = maybeSpawnCoroutines();
        return result;
    } else {
        return NULL;
    }
}


void workerLoop ( thread* me, void* args ) {
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
        thread* t = thread_spawn( current_thread, this, workerLoop, work_args);
        unassigned( t );
    }
    num_idle += num;
}

#define BASIC_MAX_WORKERS 2
thread* TaskingScheduler::maybeSpawnCoroutines( ) {
    if ( num_workers < BASIC_MAX_WORKERS ) {
       num_workers += 1;
       VLOG(5) << "spawning another worker; now there are " << num_workers;
       return thread_spawn( current_thread, this, workerLoop, work_args ); // current thread will be coro parent; is this okay?
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
