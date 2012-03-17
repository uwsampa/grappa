#include "Task.hpp"

struct task_worker_args {
    TaskManager * tasks;
};

void workerLoop ( thread* me, void* args ) {
    TaskManager* tasks = ((task_worker_args*)args)->tasks;

    while ( true ) {
        // block until receive work or termination reached
        Task nextTask;
        if (!tasks.getWork(&nextTask)) break; // quitting time

        nextTask.execute();

        thread_yield( me ); // yield to the scheduler
    }
}

/// create worker threads for executing tasks
void TaskManager::createWorkers( uint64_t num ) {
    num_workers += num;
    for (int i=0; i<num; i++) {
        thread* t = thread_spawn( current_thread, scheduler, workerLoop, &work_args);
        unassignedQ.enqueue( t );
    }
}

#define BASIC_MAX_WORKERS 1
thread* TaskManager::maybeSpawnCoroutines( ) {
    if ( num_workers < BASIC_MAX_WORKERS ) {
       num_workers++;
       return thread_spawn( current_thread, scheduler, workerLoop, &work_args ); // current thread will be coro parent; is this okay?
    } else {
        // might have another way to spawn
        return NULL_THREAD;
    }
}

bool TaskManager::getWork ( Task* result ) {

    // break this loop under two conditions
    // 1. receive work from the work queues
    // 2. termination reached
    while ( !workDone ) { 

        // check the private and public-local queues
        if ( getNextInQueues(result) ) {
            return true;
        }

        // assume no work
        mightBeWork = false;

        if (doSteal) {
            // try to put some work back to local 
            if ( publicQ->acquire(chunkSize) ) {
                mightBeWork = true;
                continue;
            }

            VLOG(5) << me->id << " okToSteal";

            // try to steal
            if (doSteal) {
                okToSteal = false;      // prevent running unassigned threads from trying to steal again
                VLOG(5) << me->id << " trying to steal";
                bool goodSteal = false;
                int victimId;

                /*          ss_setState(ss, SS_SEARCH);             */
                for (int i = 1; i < numLocalNodes && !goodSteal; i++) { // TODO permutation order
                    victimId = (localId + i) % numLocalNodes;
                    goodSteal = ss_steal_locally(ss, neighbors[victimId], chunkSize);
                }

                if (goodSteal) {
                    VLOG(5) << me->id << " steal from rank" << victimId;
                    okToSteal = true; // release steal lock
                    mightBeWork = true; // now there is work so allow more threads to be scheduled
                    continue;
                } else {
                    VLOG(5) << me->id << " failed to steal";
                }

                /**TODO remote load balance**/

            } else {
                VLOG(5) << me->id << " !okToSteal";
            }
        }

        VLOG(5) << me->id << "goes idle because sees no work (idle=" <<me->sched->num_idle
            << " idleReady="<<me->sched->idleReady <<")";

        if (!thread_idle(me, num_workers)) {
         
            VLOG(5) << me->id << " saw all were idle so suggest barrier";
         
            // no work so suggest global termination barrier
            if (cbarrier_wait()) {
                VLOG(5) << me->id << " left barrier from finish";
                workDone = true;
            } else {
                VLOG(5) << me->id << " left barrier from cancel";
                mightBeWork = true;   // work is available so allow unassigned threads to be scheduled
                okToSteal = true;        // work is available so allow steal attempts
            }
        }
    }

    return false;
}


///////////////////
//Remote Spawn
////////////////

static void spawnRemotePrivate_am( spawn_args * args, size_t size, void * payload, size_t payload_size ) {
  /**  my_task_manager->spawnPrivate( args->f, args->arg ); **/
}

inline static void TaskManager::spawnRemotePrivate( Node dest, void (*f)(void * arg), void * arg) {
    spawn_args args = {f, arg};
    SoftXMT_call_on( dest, &spawnRemotePrivate_am, &args );
}
////////////////
