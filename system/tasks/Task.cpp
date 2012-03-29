#include "Task.hpp"

// TODO replace with call to something like SoftXMT_currentThreadId
int cur_tid () {
    return 1;
}

#define MAXQUEUEDEPTH 500000

TaskManager::TaskManager (bool doSteal, Node localId, Node* neighbors, Node numLocalNodes, int chunkSize, int cbint) 
    : workDone( false )
    , doSteal( doSteal ), okToSteal( true ), mightBeWork ( true )
    , localId( localId ), neighbors( neighbors ), numLocalNodes( numLocalNodes )
    , chunkSize( chunkSize ), cbint( cbint ) 
    , privateQ( )
    , publicQ( MAXQUEUEDEPTH ) {
    
          // TODO the way this is being used, it might as well have a singleton
          StealQueue<Task>::registerAddress( &publicQ );
}
        

bool TaskManager::getWork ( Task* result, Scheduler* scheduler) {

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
            if ( publicQ.acquire(chunkSize) ) {
                mightBeWork = true;
                continue;
            }

            DVLOG(5) << cur_tid() << " okToSteal";

            // try to steal
            if (doSteal) {
                okToSteal = false;      // prevent running unassigned threads from trying to steal again
                DVLOG(5) << cur_tid() << " trying to steal";
                bool goodSteal = false;
                int victimId;

                /*          ss_setState(ss, SS_SEARCH);             */
                for (int i = 1; i < numLocalNodes && !goodSteal; i++) { // TODO permutation order
                    victimId = (localId + i) % numLocalNodes;
                    goodSteal = publicQ.steal_locally(neighbors[victimId], chunkSize);
                }

                if (goodSteal) {
                    DVLOG(5) << cur_tid() << " steal from rank" << victimId;
                    okToSteal = true; // release steal lock
                    mightBeWork = true; // now there is work so allow more threads to be scheduled
                    continue;
                } else {
                    DVLOG(5) << cur_tid() << " failed to steal";
                }

                /**TODO remote load balance**/

            } else {
                DVLOG(5) << cur_tid() << " !okToSteal";
            }
        }

//        DVLOG(5) << cur_tid() << "goes idle because sees no work (idle=" << scheduler->num_idle
//            << " idleReady="<<me->sched->idleReady <<")";

        if (!scheduler->thread_idle(num_workers)) {
         
            DVLOG(5) << cur_tid() << " saw all were idle so suggest barrier";
         
            // no work so suggest global termination barrier
            if (cbarrier_wait()) {
                DVLOG(5) << cur_tid() << " left barrier from finish";
                workDone = true;
            } else {
                DVLOG(5) << cur_tid() << " left barrier from cancel";
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

inline void TaskManager::spawnRemotePrivate( Node dest, void (*f)(void * arg), void * arg) {
    spawn_args args = {f, arg};
    SoftXMT_call_on( dest, &spawnRemotePrivate_am, &args );
}
////////////////
