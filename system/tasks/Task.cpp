#include "Task.hpp"
#include "../SoftXMT.hpp"


#define MAXQUEUEDEPTH 500000



TaskManager::TaskManager (bool doSteal, Node localId, Node * neighbors, Node numLocalNodes, int chunkSize, int cbint) 
    : workDone( false )
    , doSteal( doSteal ), okToSteal( true )
    , sharedMayHaveWork ( true )
    , globalMayHaveWork ( true )
    , localId( localId ), neighbors( neighbors ), numLocalNodes( numLocalNodes )
    , chunkSize( chunkSize ), cbint( cbint ) 
    , privateQ( )
    , publicQ( MAXQUEUEDEPTH ) 
    , stats( ) {
    
          // TODO the way this is being used, it might as well have a singleton
          StealQueue<Task>::registerAddress( &publicQ );
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
        return true;
    } else if ( publicHasEle() ) {
        *result = publicQ.peek();
        publicQ.pop( );
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
        if ( okToSteal ) {
            // only one Thread is allowed to steal
            okToSteal = false;

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
            okToSteal = true; // release steal lock

            /**TODO remote load balance**/
        }
    }

    if ( !available() ) {
        if ( !SoftXMT_thread_idle() ) {
            VLOG(5) << CURRENT_THREAD << " saw all were idle so suggest barrier";

            // no work so suggest global termination barrier
            CHECK( !workDone ) << "perhaps there is a stray unidled thread problem?";
            int finished_barrier = cbarrier_wait();
            if (finished_barrier) {
                VLOG(5) << CURRENT_THREAD << " left barrier from finish";
                workDone = true;
                SoftXMT_signal_done( ); // terminate auto communications
                //TODO: if we do this on just node0 can we ensure there is not a race between
                //      cbarrier_exit_am and the softxmt_mark_done_am?
            } else {
                VLOG(5) << CURRENT_THREAD << " left barrier from cancel";
                globalMayHaveWork = true;   // work is available so allow unassigned threads to be scheduled
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
    dump_stats();
}

void TaskManager::dump_stats() {
    stats.dump();
}

#include "DictOut.hpp"
void TaskStatistics::dump() {
    DictOut dout;
    DICT_ADD(dout, session_steal_successes_);
    DICT_ADD(dout, session_steal_fails_);
    DICT_ADD(dout, single_steal_successes_);
    DICT_ADD(dout, single_steal_fails_);
    DICT_ADD(dout, acquire_successes_);
    DICT_ADD(dout, acquire_fails_);
    DICT_ADD(dout, releases_);

    LOG(INFO) << dout.toString();
}
