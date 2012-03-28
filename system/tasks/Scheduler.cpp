#include "Scheduler.hpp"
#include "Task.hpp"


void Scheduler::run ( ) {
    while (thread_wait( NULL ) != NULL) { } // nothing
}

void Scheduler::thread_join( thread* wait_on ) {
    if ( !wait_on->done ) {
        wait_on->joinqueue.enqueue( current_thread );
        thread_suspend( );
    }
}


thread * Scheduler::thread_wait( void **result ) {
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


inline thread* Scheduler::getWorker () {
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
    Scheduler * sched = wargs->scheduler;

    Task nextTask;
   
    while ( true ) {
        // block until receive work or termination reached
        if (!tasks->getWork(&nextTask)) break; // quitting time

        nextTask.execute();

        sched->thread_yield( ); // yield to the scheduler
    }
}


/// create worker threads for executing tasks
void Scheduler::createWorkers( uint64_t num ) {
    task_manager->addNumWorkers( num );
    for (int i=0; i<num; i++) {
        thread* t = thread_spawn( current_thread, this, workerLoop, &work_args);
        unassigned( t );
    }
}

#define BASIC_MAX_WORKERS 1
thread* Scheduler::maybeSpawnCoroutines( ) {
    if ( task_manager->getNumWorkers() < BASIC_MAX_WORKERS ) {
       task_manager->addNumWorkers( 1 );
       return thread_spawn( current_thread, this, workerLoop, &work_args ); // current thread will be coro parent; is this okay?
    } else {
        // might have another way to spawn
        return NULL;
    }
}
