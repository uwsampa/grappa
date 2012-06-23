#include "Task.hpp"
#include "../SoftXMT.hpp"
#include "../PerformanceTools.hpp"
#include "TaskingScheduler.hpp"

#define MAXQUEUEDEPTH 500000

TaskManager global_task_manager;

GRAPPA_DEFINE_EVENT_GROUP(task_manager);
//DEFINE_bool(TaskManager_events, true, "Enable tracing of events in TaskManager.");

TaskManager::TaskManager ( ) 
  : workDone( false )
  , all_terminate( false )
  , doSteal( false )
  , stealLock( true )
  , sharedMayHaveWork ( true )
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
}
        

bool TaskManager::getWork( Task * result ) {
    GRAPPA_FUNCTION_PROFILE( GRAPPA_TASK_GROUP );

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
        DVLOG(5) << "consuming local task";
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
    if ( doSteal ) {
        if ( stealLock ) {
    
            GRAPPA_PROFILE_CREATE( prof, "stealing", "(session)", GRAPPA_TASK_GROUP );
            GRAPPA_PROFILE_START( prof );
            
            // only one Thread is allowed to steal
            stealLock = false;

            VLOG(5) << CURRENT_THREAD << " trying to steal";
            int goodSteal = 0;
            Node victimId;

            for ( Node i = 1; 
                  i < numLocalNodes && !goodSteal && !(sharedMayHaveWork || publicHasEle() || privateHasEle() || workDone);
                  i++ ) { // TODO permutation order

                victimId = (localId + i) % numLocalNodes;

                goodSteal = publicQ.steal_locally(neighbors[victimId], chunkSize);
                
                if (goodSteal) { stats.record_successful_steal(); }
                else { stats.record_failed_steal(); }
            }

            // if finished because succeeded in stealing
            if ( goodSteal ) {
                VLOG(5) << CURRENT_THREAD << " steal " << goodSteal
                    << " from Node" << victimId;
                VLOG(5) << *this; 
                stats.record_successful_steal_session();

                // publicQ should have had some elements in it
                // at some point after successful steal
            } else {
                VLOG(5) << CURRENT_THREAD << " failed to steal";
                
                stats.record_failed_steal_session();

            }

            VLOG(5) << "left stealing loop with goodSteal=" << goodSteal
                    << " last victim=" << victimId
                    << " sharedMayHaveWork=" << sharedMayHaveWork
                    << " publicHasEle()=" << publicHasEle()
                    << " privateHasEle()=" << privateHasEle();
            stealLock = true; // release steal lock

            /**TODO remote load balance**/
        
            GRAPPA_PROFILE_STOP( prof );
        }
    }

    if ( !local_available() ) {
        GRAPPA_PROFILE_CREATE( prof, "worker idle", "(suspended)", GRAPPA_SUSPEND_GROUP ); 
        GRAPPA_PROFILE_START( prof );
        if ( !SoftXMT_thread_idle() ) {
            SoftXMT_yield();
        } else {
            DVLOG(5) << CURRENT_THREAD << " un-idled";
        }
        GRAPPA_PROFILE_STOP( prof );
    }
    
    return false;
}


std::ostream& operator<<( std::ostream& o, const TaskManager& tm ) {
    return tm.dump( o );
}

void TaskManager::signal_termination( ) {
    workDone = true;
    SoftXMT_signal_done();
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

