#include "Task.hpp"
#include "../SoftXMT.hpp"
#include "../PerformanceTools.hpp"

#define MAXQUEUEDEPTH 500000

TaskManager global_task_manager;

GRAPPA_DEFINE_EVENT_GROUP(task_manager);
//DEFINE_bool(TaskManager_events, true, "Enable tracing of events in TaskManager.");

TaskManager::TaskManager ( ) 
  : workDone( false )
  , doSteal( false )
  , stealLock( true )
  , sharedMayHaveWork ( true )
  , globalMayHaveWork ( true )
  , privateQ( )
  , publicQ( MAXQUEUEDEPTH ) 
  , stats( this )
{
  StealQueue<Task>::registerAddress( &publicQ );
}


void TaskManager::init (bool doSteal_arg, Node localId_arg, Node * neighbors_arg, Node numLocalNodes_arg, int chunkSize_arg, int cbint_arg) {
  doSteal = doSteal_arg;
  localId = localId_arg;
  neighbors = neighbors_arg;
  numLocalNodes = numLocalNodes_arg;
  chunkSize = chunkSize_arg;
  cbint = cbint_arg;
  cbarrier_init( SoftXMT_nodes() );
}
        

bool TaskManager::getWork( Task * result ) {

    while ( !workDone ) {
        if ( tryConsumeLocal( result ) ) {
            return true;
        }

        if ( tryConsumeShared( result ) ) {
            return true;
        }

        if ( waitConsumeAny( result ) ) {
            return true;
        }
    }

    return false;

}


/// "work queue" operations
bool TaskManager::tryConsumeLocal( Task * result ) {
    if ( privateHasEle() ) {
        *result = privateQ.front();
        privateQ.pop_front();
        stats.record_private_task_dequeue();
        return true;
    } else if ( publicHasEle() ) {
        *result = publicQ.peek();
        publicQ.pop( );
        stats.record_public_task_dequeue();
        return true;
    } else {
        return false;
    }
}

bool TaskManager::tryConsumeShared( Task * result ) {
    if ( doSteal ) {
        if ( publicQ.acquire( chunkSize ) ) {
            stats.record_successful_acquire();
            CHECK( publicHasEle() ) << "successful acquire must produce local work";
        
            *result = publicQ.peek();
            publicQ.pop( );
            stats.record_public_task_dequeue();
            return true;
        } else {
            stats.record_failed_acquire();
        }
    }
     
    sharedMayHaveWork = false;   
    return false;
}

/// Only returns when there is work or when
/// the system has no more work.
bool TaskManager::waitConsumeAny( Task * result ) {
    if ( doSteal && globalMayHaveWork ) {
        if ( stealLock ) {
            // only one Thread is allowed to steal
            stealLock = false;

            VLOG(5) << CURRENT_THREAD << " trying to steal";
            bool goodSteal = false;
            Node victimId;

            for ( Node i = 1; 
                  i < numLocalNodes && !goodSteal && !(sharedMayHaveWork || publicHasEle() || privateHasEle());
                  i++ ) { // TODO permutation order

                victimId = (localId + i) % numLocalNodes;
                goodSteal = publicQ.steal_locally(neighbors[victimId], chunkSize, CURRENT_THREAD);
                
                if (goodSteal) { stats.record_successful_steal(); }
                else { stats.record_failed_steal(); }
            }

            // if finished because succeeded in stealing
            if ( goodSteal ) {
                VLOG(5) << CURRENT_THREAD << " steal " << goodSteal
                    << " from Node" << victimId;
                
                stats.record_successful_steal_session();

                // publicQ should have had some elements in it
                // at some point after successful steal
            } else {
                VLOG(5) << CURRENT_THREAD << " failed to steal";
                
                stats.record_failed_steal_session();

                // mark that we already tried to steal from other queues
                globalMayHaveWork = false;
            }

            VLOG(5) << "left stealing loop with goodSteal=" << goodSteal
                    << " last victim=" << victimId
                    << " sharedMayHaveWork=" << sharedMayHaveWork
                    << " publicHasEle()=" << publicHasEle()
                    << " privateHasEle()=" << privateHasEle();
            stealLock = true; // release steal lock

            /**TODO remote load balance**/
        }
    }

    if ( !available() ) {
        if ( !SoftXMT_thread_idle() ) {
            // no work so suggest global termination barrier
            
            VLOG(5) << CURRENT_THREAD << " saw all were idle so suggest barrier";

            CHECK( !workDone ) << "perhaps there is a stray unidled thread problem?";

            cb_cause finished_barrier = cbarrier_wait();
            switch( finished_barrier ) {
                case CB_Cause_Done:
                    VLOG(5) << CURRENT_THREAD << " left barrier from finish";
                    workDone = true;
                    SoftXMT_signal_done( ); // terminate auto communications
                    break;
                case CB_Cause_Cancel:
                    VLOG(5) << CURRENT_THREAD << " left barrier from cancel";
                    globalMayHaveWork = true;   // work is available so allow unassigned threads to be scheduled
                    // we rely on the invariant that if globalMayHaveWork=true then the cbarrier
                    // is NOT counting mynode. This way a barrier finish cannot occur without mynode,
                    // and so mynode can safely steal and receive steal replies without 
                    // communication layer terminating.
                    break;
                case CB_Cause_Local:
                    VLOG(5) << CURRENT_THREAD << " left barrier from local cancel";

                    // we rely on the invariant that mynode will not try to steal again
                    // until it enters the cbarrier and receives a CB_Cause_Cancel.
                    // Thus, the only communication mynode will then depend on until the next
                    // time it enters the cbarrier is communication initiated by user code, 
                    // which is guarenteed to be okay by the NoUnsyncedTasks requirement.

                    break;

                default:
                    CHECK ( false ) << "invalid cbarrier exit cause " << finished_barrier;
            }
        } else {
            DVLOG(5) << CURRENT_THREAD << " un-idled";
        }
    }

    return false;
}

std::ostream& operator<<( std::ostream& o, const TaskManager& tm ) {
    return tm.dump( o );
}

void TaskManager::finish() {
}

///
/// Stats
///

void TaskManager::dump_stats() {
    stats.dump();
}

#include "DictOut.hpp"
void TaskManager::TaskStatistics::dump() {
    DictOut dout;
    DICT_ADD(dout, session_steal_successes_);
    DICT_ADD(dout, session_steal_fails_);
    DICT_ADD(dout, single_steal_successes_);
    DICT_ADD(dout, single_steal_fails_);
    DICT_ADD(dout, acquire_successes_);
    DICT_ADD(dout, acquire_fails_);
    DICT_ADD(dout, releases_);
    DICT_ADD(dout, public_tasks_dequeued_);
    DICT_ADD(dout, private_tasks_dequeued_);

    std::cout << "TaskStatistics " << dout.toString() << std::endl;
}

void TaskManager::TaskStatistics::sample() {
  GRAPPA_EVENT(privateQ_size_ev,       "privateQ size sample",  SAMPLE_RATE, task_manager, tm->privateQ.size());
  GRAPPA_EVENT(publicQ_local_size_ev,  "publicQ.local sample",  SAMPLE_RATE, task_manager, tm->publicQ.localDepth());
  GRAPPA_EVENT(publicQ_shared_size_ev, "publicQ.shared sample", SAMPLE_RATE, task_manager, tm->publicQ.sharedDepth());

#ifdef VTRACE
  //VT_COUNT_UNSIGNED_VAL( privateQ_size_vt_ev, tm->privateQ.size() );
  //VT_COUNT_UNSIGNED_VAL( publicQ_local_size_vt_ev, tm->publicQ.localDepth() );
  //VT_COUNT_UNSIGNED_VAL( publicQ_shared_size_vt_ev, tm->publicQ.sharedDepth() );
#endif
//    sample_calls++;
//    /* todo: avgs */
//
//#ifdef GRAPPA_TRACE
//    if ((sample_calls % 1) == 0) {
//      TAU_REGISTER_EVENT(privateQ_size_ev, "privateQ size sample");
//      TAU_REGISTER_EVENT(publicQ_local_size_ev, "publicQ.local sample");
//      TAU_REGISTER_EVENT(publicQ_shared_size_ev, "publicQ.shared sample");
//      
//      TAU_EVENT(privateQ_size_ev, tm->privateQ.size());
//      TAU_EVENT(publicQ_local_size_ev, tm->publicQ.localDepth());
//      TAU_EVENT(publicQ_shared_size_ev, tm->publicQ.sharedDepth());
//    }
//#endif
}


void TaskManager::TaskStatistics::merge(TaskManager::TaskStatistics * other) {
  session_steal_successes_ += other->session_steal_successes_;
  session_steal_fails_ += other->session_steal_fails_;
  single_steal_successes_ += other->single_steal_successes_;
  single_steal_fails_ += other->single_steal_fails_;
  acquire_successes_ += other->acquire_successes_;
  acquire_fails_ += other->acquire_fails_;
  releases_ += other->releases_;
  public_tasks_dequeued_ += other->public_tasks_dequeued_;
  private_tasks_dequeued_ += other->private_tasks_dequeued_;
}

void TaskManager::TaskStatistics::merge_am(TaskManager::TaskStatistics * other, size_t sz, void* payload, size_t psz) {
  global_task_manager.stats.merge(other);
}

