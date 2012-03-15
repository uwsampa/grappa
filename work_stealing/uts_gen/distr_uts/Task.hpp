#include <deque>
#include "StealQueue.h"
#include "gasnet_cbarrier.h"


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





// task assignment

#define BASIC_MAX_WORKERS 1
thread* maybeSpawnCoroutines( ) {
    if ( num_workers < BASIC_MAX_WORKERS ) {
       num_workers++;
       return thread_spawn( current_thread, scheduler, workerLoop, NULL ); // current thread will be coro parent; is this okay?
    } else {
        // might have another way to spawn
        return NULL_THREAD;
    }
}

thread* getWorker () {
    if (taskman.available()) {
        // check the pool of unassigned coroutines
        thread* result = unassignedQ.dequeue();
        if (result != NULL_THREAD) return result;

        // possibly spawn more coroutines
        thread* result = taskman.maybeSpawnCoroutines();
        return result;
    } else {
        return NULL_THREAD;
    }
}


// worker loop
class Task {

    private:
        void (*func)(void*);
        void* args;
    
    public:
        Task (void (*f)(void*), void* args) 
            : func ( f )
            , args ( args ) 
            , valid ( valid ) {}

        void execute ( ) {
            func (args);
        }

};



void workerLoop ( thread* me, void* args) {

    while ( true ) {
        // block until receive work or termination reached
        Task nextTask;
        if (!tasks.getWork(&nextTask)) break; // quitting time

        nextTask.execute();

        thread_yield( me ); // yield to the scheduler
    }
}
// NOTES:
// "Task" can be a class that hold anything that
// could go in a task queue
//

class TaskManager {
    private:
        std::deque<Task> privateQ; 
        StealStack publicQ;

        Scheduler* scheduler;

        bool workDone;
        bool mightBeWork; // flag to enable wake up optimization
       
        bool doSteal;   // stealing on/off
        bool okToSteal; // steal lock
        
        // to support hierarchical dynamic load balancing
        Node localId;
        Node* neighbors;
        Node numLocalNodes;

        uint64_t num_workers;

        // steal parameters
        int chunkSize;

        bool publicHasEle() {
            return ss_local_depth( &publicQ ) > 0;
        }

        bool privateHasEle() {
            return !privateQ.empty();
        }

        bool getNextInQueues(Task * result) {
            if ( privateHasEle() ) {
                *result = privateQ.front();
                privateQ.pop_front();
                return true;
            } else if ( publicHasEle() ) {
                *result = ss_top( &publicQ );
                ss_pop( &publicQ );
                return true;
            } else {
                return false;
            }
        }

    public:
        TaskManager (Scheduler* scheduler, bool doSteal, Node localId, Node* neighbors, Node numLocalNodes, int chunkSize) 
                : workDone ( false )
                , doSteal( doSteal )
                , okToSteal ( true )
                , mightBeWork ( true )
                , localId( localId )
                , neighbors ( neighbors )
                , numLocalNodes ( numLocalNodes )
                , chunkSize( chunkSize ) 
                , scheduler( scheduler )
                , privateQ( ) {
                    
                   ss_init( &publicQ, MAXSTACKDEPTH ); 
        }

        /// create worker threads for executing tasks
        void createWorkers( uint64_t num ) {
            num_workers += num;
            for (int i=0; i<num; i++) {
                thread* t = thread_spawn( current_thread, scheduler, workerLoop, NULL);
                unassignedQ.enqueue( t );
            }
        }

        /// Possibly move work from local partition of public queue to global partition
        void releaseTasks() {
          if (doSteal) {
            if (ss_localDepth(ss) > 2 * chunkSize) {
              // Attribute this time to runtime overhead
        /*      ss_setState(ss, SS_OVH);                    */
              ss_release(&publicQ, chunkSize);
              // This has significant overhead on clusters!
              if (publicQ.nNodes % cbint == 0) { // possible for cbint to get skipped if push multiple?
        /*        ss_setState(ss, SS_CBOVH);                */
                VLOG(5) << "canceling barrier";
                cbarrier_cancel();
              }

        /*      ss_setState(ss, SS_WORK);                   */
            }
          }
        }

        /*TODO return value?*/ 
        void spawnPublic( void (*f)(void * arg), void * arg);
        
        /*TODO return value?*/ 
        void spawnPrivate( void (*f)(void * arg), void * arg);
        
        bool getWork ( Task* result );

        bool available ( );
};

void TaskManager::spawnPublic( void (*f)(void * arg), void * arg ) {
    Task newtask(f, arg);
    ss_push( &publicQ, newtask );
    releaseTasks();
}

void TaskManager::spawnPrivate( void (*f)(void * arg), void * arg ) {
    Task newtask(f, arg);
    privateQ.push_front( newtask );
}

bool TaskManager::available ( ) {
    return  privateHasEle()
            || publicHasEle()
            || mightBeWork;
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
            if (ss_acquire( &publicQ, chunkSize )) {
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


