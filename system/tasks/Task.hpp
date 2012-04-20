#ifndef TASK_HPP_
#define TASK_HPP_

#include <deque>
#include "StealQueue.hpp"
#include "cbarrier.hpp"
#include "Thread.hpp"

typedef int16_t Node;

/// Task is a function pointer and pointer to arguments
/// Ideally Task would be interface that just declares execute and makeGlobal
class Task {

    private:
        void (* fn_p)(void *);
        void * arg;
    
    public:
        Task () {}
        Task (void (* fn_p)(void *), void * arg) 
            : fn_p ( fn_p )
            , arg ( arg ) { }

        void execute( ) {
            CHECK( fn_p!=NULL ) << "fn_p=" << (void*)fn_p << "\nargs=" << (void*)arg;
            fn_p( arg );
        }
};



template < typename T >
static Task createTask( void (* fn_p)(T *), T * args ) {
    Task t( reinterpret_cast< void (*) (void*) >( fn_p ),
            reinterpret_cast< void *>( *args ) );
    return t;
}


class TaskManager {
    private:
        std::deque<Task> privateQ; 
        StealQueue<Task> publicQ;

        bool workDone;
       
        bool doSteal;   // stealing on/off
        bool okToSteal; // steal lock
        int cbint;      // how often to make local public work visible

        // to support hierarchical dynamic load balancing
        Node localId;
        Node* neighbors;
        Node numLocalNodes;

        // steal parameters
        int chunkSize;

        bool publicHasEle() {
            return publicQ.localDepth() > 0;
        }

        bool privateHasEle() {
            return !privateQ.empty();
        }

        /// Flags to save whether a worker thinks there
        /// could be work or if other workers should not
        /// also try.
        bool sharedMayHaveWork;
        bool globalMayHaveWork;
        
        // "queue" operations
        bool tryConsumeLocal( Task * result );
        bool tryConsumeShared( Task * result );
        bool waitConsumeAny( Task * result );


    public:
        TaskManager (bool doSteal, Node localId, Node* neighbors, Node numLocalNodes, int chunkSize, int cbint);

        bool isWorkDone() {
            return workDone;
        }
        
        /// Maybe release tasks to the shared portion of publicQ
        void releaseTasks() {
            if (doSteal) {
                if (publicQ.localDepth() > 2 * chunkSize) {
                    // Attribute this time to runtime overhead
                    /*      ss_setState(ss, SS_OVH);                    */
                    publicQ.release(chunkSize);

                    // set that there COULD be work in shared portion
                    // (not "is work" because may be stolen)
                    sharedMayHaveWork = true;

                    // This has significant overhead on clusters!
                    if (publicQ.get_nNodes() % cbint == 0) { // possible for cbint to get skipped if push multiple?
                        /*        ss_setState(ss, SS_CBOVH);                */
                        VLOG(5) << "canceling barrier";
                        cbarrier_cancel();
                    }

                    /*      ss_setState(ss, SS_WORK);                   */
                }
            }
        }

        /*TODO return value?*/
        template < typename T > 
        void spawnPublic( void (*f)(T *), T * arg);
        
        /*TODO return value?*/ 
        template < typename T > 
        void spawnLocalPrivate( void (*f)(T *), T * arg);
        
        /*TODO return value?*/ 
        template < typename T > 
        void spawnRemotePrivate( void (*f)(T *), T * arg);
        
        bool getWork ( Task * result );

        bool available ( );


};


inline bool TaskManager::available( ) {
    VLOG(5) << " sharedMayHaveWork=" << sharedMayHaveWork
            << " publicHasEle()=" << publicHasEle()
            << " privateHasEle()=" << privateHasEle();
    return privateHasEle() 
           || publicHasEle()
           || (doSteal && (sharedMayHaveWork || (okToSteal && globalMayHaveWork)));
}


Node SoftXMT_mynode();
template < typename T > 
inline void TaskManager::spawnPublic( void (*f)(T *), T * arg ) {
    Task newtask = createTask(f, arg);
    publicQ.push( newtask );
    releaseTasks();
}

/// Should NOT be called from the context of
/// an AM handler
template < typename T > 
inline void TaskManager::spawnLocalPrivate( void (*f)(T *), T * arg ) {

    Task newtask = createTask(f, arg);
    privateQ.push_front( newtask );

    /* no notification necessary since
     * presence of a local spawn means
     * we are not in the cbarrier */
}

/// Should ONLY be called from the context of
/// an AM handler
template < typename T > 
inline void TaskManager::spawnRemotePrivate( void (*f)(T *), T * arg ) {
    Task newtask = createTask(f, arg);
    privateQ.push_front( newtask );

    cbarrier_cancel_local();
}


#endif
