#ifndef TASK_HPP_
#define TASK_HPP_

#include <deque>
#include "StealQueue.hpp"
#include "cbarrier.hpp"
#include "Thread.hpp"

//thread* const NULL_THREAD = NULL;

typedef int16_t Node;

/// Task is a function pointer and pointer to arguments
/// Ideally Task would be interface that just declares execute and makeGlobal
class Task {

    private:
        void (* fn_p)(void *);
        void * args;
        size_t args_size;
        Node home;
    
    public:
        Task () {}
        Task (void (* fn_p)(void *), void * args, size_t args_size, Node createdOn) 
            : fn_p ( fn_p )
            , args ( args )
            , args_size ( args_size )
            , home ( createdOn ){}

        void execute ( );
        
};


template < typename ArgsStruct >
static Task createTask( void (* fn_p)(ArgsStruct *), ArgsStruct * args, Node createdOn,
                        size_t args_size = sizeof( ArgsStruct )) {
    Task t( reinterpret_cast< void (*) (void*) >( fn_p ),
            static_cast< void *>( args ), args_size, createdOn );
    return t;
}

// TODO: on steal of work, need to make sure args pointers are global or copy args struct


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
                        VLOG(5) << "canceling barrier"
                            cbarrier_cancel();
                    }

                    /*      ss_setState(ss, SS_WORK);                   */
                }
            }
        }

        /*TODO return value?*/
        template < typename ArgsStruct > 
        void spawnPublic( void (*f)(ArgsStruct * arg), ArgsStruct * arg);
        
        /*TODO return value?*/ 
        template < typename ArgsStruct > 
        void spawnLocalPrivate( void (*f)(ArgsStruct * arg), ArgsStruct * arg);
        
        /*TODO return value?*/ 
        template < typename ArgsStruct > 
        void spawnRemotePrivate( void (*f)(ArgsStruct * arg), ArgsStruct * arg, Node from);
        
        bool getWork ( Task * result );

        bool available ( );


};


inline bool TaskManager::available( ) {
    return sharedMayHaveWork ||
           globalMayHaveWork ||
           privateQHasEle()  ||
           publicQHasEle();
}


Node SoftXMT_mynode();
template < typename ArgsStruct > 
inline void TaskManager::spawnPublic( void (*f)(ArgsStruct * arg), ArgsStruct * arg ) {
    Task newtask = createTask(f, arg, SoftXMT_mynode());
    publicQ.push( newtask );
    releaseTasks();
}

/// Should NOT be called from the context of
/// an AM handler
template < typename ArgsStruct > 
inline void TaskManager::spawnLocalPrivate( void (*f)(ArgsStruct * arg), ArgsStruct * arg ) {

    Task newtask = createTask(f, arg, SoftXMT_mynode());
    privateQ.push_front();

    /* no notification necessary since
     * presence of a local spawn means
     * we are not in the cbarrier */
}

/// Should ONLY be called from the context of
/// an AM handler
template < typename ArgsStruct > 
inline void TaskManager::spawnRemotePrivate( void (*f)(ArgsStruct * arg), ArgsStruct * arg, Node from ) {
    Task newtask = createTask(f, arg, from);
    privateQ.push_front();

    cbarrier_cancel_local();
}


#endif
