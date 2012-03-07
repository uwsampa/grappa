const thread* NULL_THREAD = NULL;

//cases:
//periodic yield: 
//  back of periodic queue; 
//  nextCoro swap
//  (if only one then eventually will run again)
//
//yield: 
//  back of readyQ
//  nextCoro swap
//  (if only one then will be chosen)
//
//suspend: 
//  no Q;
//  call nextCoro
//  (if only one then periodic tasks
//     should eventually cause it to
//     go on the ready queue and will be chosen)


// scheduling

thread* nextCoroutine () {
    
    while ( true ) {
        thread* result;

        // check for periodic tasks
        result = periodicQ.dequeue();
        if (result != NULL_THREAD) return result;

        // check ready tasks
        result = readyQ.dequeue();
        if (result != NULL_THREAD) return result;

        // check for new workers
        result = getWorker();
        if (result != NULL_THREAD) return result;

        // no coroutines can run, so handle
        DVLOG(5) << "scheduler: no coroutines can run";
        usleep(1);
    }
}






// task assignment


thread* getWorker () {
    if (tasks.available()) {
        // check the pool of unassigned coroutines
        thread* result = unassignedQ.dequeue();
        if (result != NULL_THREAD) return result;

        // possibly spawn more coroutines
        thread* result = maybeSpawnCoroutines();
        return result;
    } else {
        return NULL_THREAD;
    }
}

//
//tasks.available will be true even if the queues
//are empty if the flag=1 to indicate try to 
//acquire work




// worker loop
const Task* TASK_DONE = NULL;


void workerLoop ( thread* me, void* args) {

    while ( true ) {
        // block until receive work or termination reached
        Task* nextTask = tasks.getWork();
        if (nextTask==TASK_DONE) break; // quitting time

        nextTask.execute(); // FIXME: we want Task deallocation?
                                 // e.g. tasks.getWork().execIfValid() --> return condition

        SoftXMT_yield ( ); // yield to the scheduler
    }
}
// NOTES:
// "Task" can be a class that hold anything that
// could go in a task queue
//

class TaskManager {
    private:
        /* private queue */
        /* public queue */

        bool workDone;
        bool mightBeWork; // flag to enable wake up optimization
       
        bool doSteal;   // stealing on/off
        bool okToSteal; // steal lock
        
        // to support hierarchical dynamic load balancing
        Node localId;
        Node* neighbors;
        Node numLocalNodes;

        // steal parameters
        int chunksize;

    public:
        TaskManager (bool doSteal, Node localId, Node* neighbors, Node numLocalNodes) 
                : workDone ( false )
                , doSteal( doSteal )
                , okToSteal ( true )
                , mightBeWork ( true )
                , localId( localId )
                , neighbors ( neighbors )
                , numLocalNodes ( numLocalNodes ) { }

        /*TODO return value?*/ void spawnPublic(...)
        /*TODO return value?*/ void spawnPrivate(...)
        Task* getWork ( );

        bool available ( );
};

bool available ( ) {
    return  /*private !empty*/
            || /*public !empty */
            || mightBeWork;
}

Task* getWork ( ) {

    // break this loop under two conditions
    // 1. receive work from the work queues
    // 2. termination reached
    while ( !workDone ) { 

        Task* result = Tasks.getNext();  // check the private and public-local queues

        if (result != NULL) return result;

        // assume no work
        mightBeWork = false;

        if (doSteal) {
            // try to put some work back to local 
            if (ss_acquire(ss, chunkSize)) {
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

    return TASK_DONE;
}


