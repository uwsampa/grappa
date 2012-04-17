#include "Task.hpp"
#include "../Cache.hpp"
#include "../SoftXMT.hpp"


#define MAXQUEUEDEPTH 500000

void Task::execute( ) {
    if ( home != SoftXMT_mynode() ) {
        char argbuf[args_size];
        Incoherent<char>::RO cached_args( GlobalAddress<char>::TwoDimensional(static_cast<char*>(args), home), args_size, argbuf );
        cached_args.block_until_acquired();
        fn_p( &argbuf );
        cached_args.block_until_released();
    } else {
        CHECK( fn_p!=NULL ) << "fn_p=" << (void*)fn_p << "\nargs=" << (void*)args << "\nhome=" << home;
        CHECK( args!=NULL ) << "fn_p=" << (void*)fn_p << "\nargs=" << (void*)args << "\nhome=" << home;
        fn_p( args );
    }
}


TaskManager::TaskManager (bool doSteal, Node localId, Node * neighbors, Node numLocalNodes, int chunkSize, int cbint) 
    : workDone( false )
    , doSteal( doSteal ), okToSteal( true ), 
    , sharedMayHaveWork ( true )
    , globalMayHaveWork ( true )
    , localId( localId ), neighbors( neighbors ), numLocalNodes( numLocalNodes )
    , chunkSize( chunkSize ), cbint( cbint ) 
    , privateQ( )
    , publicQ( MAXQUEUEDEPTH ) {
    
          // TODO the way this is being used, it might as well have a singleton
          StealQueue<Task>::registerAddress( &publicQ );
          cbarrier_init( SoftXMT_nodes(), SoftXMT_mynode() );
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
bool tryConsumeLocal( Task * result ) {
    if ( privateHasEle() ) {
        *result = privateQ.front();
        privateQ.pop_front();
        return true;
    } else if ( publicQHasEle() ) {
        *result = publicQ.peek();
        publicQ.pop( );
        return true;
    } else {
        return false;
    }
}

bool tryConsumeShared( Task * result ) {
    if ( doSteal ) {
        if ( publicQ.aquire( chunkSize ) ) {
            CHECK( publicQHasEle() );
            *result = publicQ.peek();
            publicQ.pop( );
            return true;
        }
    }
     
    sharedMayHaveWork = false;   
    return false;
}

/// Only returns when there is work or when
/// the system has no more work.
bool waitConsumeAny( Task * result ) {
    if ( doSteal ) {
        if ( okToSteal ) {
            // only one Thread is allowed to steal
            okToSteal = false;

            DVLOG(5) << CURRENT_THREAD << " trying to steal";
            bool goodSteal = false;
            Node victimId;

            /*          ss_setState(ss, SS_SEARCH);             */
            for ( Node i = 1; 
                  i < numLocalNodes && !goodSteal && !queues_mightBeWork; 
                  i++ ) { // TODO permutation order

                victimId = (localId + i) % numLocalNodes;
                goodSteal = publicQ.steal_locally(neighbors[victimId], chunkSize);
            }

            // if finished because succeeded in stealing
            if ( goodSteal ) {
                VLOG(5) << CURRENT_THREAD << " steal " << goodSteal
                    << " from Node" << victimId;

                // publicQ should have had some elements in it
                // at some point after successful steal
            } else {
                VLOG(5) << CURRENT_THREAD << " failed to steal";

                // mark that we already tried to steal from other queues
                globalMayHaveWork = false;
            }


            okToSteal = true; // release steal lock

            /**TODO remote load balance**/
        }
    }

    if ( !available() ) {
        if ( !SoftXMT_thread_idle() ) {
            VLOG(5) << CURRENT_THREAD << " saw all were idle so suggest barrier";

            // no work so suggest global termination barrier
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

