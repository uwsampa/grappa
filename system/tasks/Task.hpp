#ifndef TASK_HPP_
#define TASK_HPP_

#include <iostream>
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

class TaskStatistics {
    private:
        uint64_t single_steal_successes_;
        uint64_t single_steal_fails_;
        uint64_t session_steal_successes_;
        uint64_t session_steal_fails_;
        uint64_t acquire_successes_;
        uint64_t acquire_fails_;
        uint64_t releases_;

    public:
        TaskStatistics()
            : single_steal_successes_ (0)
            , single_steal_fails_ (0)
            , session_steal_successes_ (0)
            , session_steal_fails_ (0)
            , acquire_successes_ (0)
            , acquire_fails_ (0)
            , releases_ (0)
         { }

        void record_successful_steal_session() {
            session_steal_successes_++;
        }

        void record_failed_steal_session() {
            session_steal_fails_++;
        }

        void record_successful_steal() {
            single_steal_successes_++;
        }

        void record_failed_steal() {
            single_steal_fails_++;
        }

        void record_successful_acquire() {
            acquire_successes_++;
        }
        
        void record_failed_acquire() {
            acquire_fails_++;
        }

        void record_release() {
            releases_++;
        }

        void dump();
};



template < typename ArgsStruct >
static Task createTask( void (* fn_p)(ArgsStruct *), ArgsStruct * args, Node createdOn,
                        size_t args_size = sizeof( ArgsStruct )) {
    Task t( reinterpret_cast< void (*) (void*) >( fn_p ),
            static_cast< void *>( args ), args_size, createdOn );
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
        
        /// Flags to save whether a worker thinks there
        /// could be work or if other workers should not
        /// also try.
        bool sharedMayHaveWork;
        bool globalMayHaveWork;

        TaskStatistics stats;
       
        bool publicHasEle() const {
            return publicQ.localDepth() > 0;
        }

        bool privateHasEle() const {
            return !privateQ.empty();
        }
        
        // "queue" operations
        bool tryConsumeLocal( Task * result );
        bool tryConsumeShared( Task * result );
        bool waitConsumeAny( Task * result );
        
        
        std::ostream& dump( std::ostream& o ) const {
            return o << "TaskManager {" << std::endl
                << "  publicQ.local: " << publicQ.localDepth( ) << std::endl
                << "  publicQ.shared: " << publicQ.sharedDepth( ) << std::endl
                << "  privateQ: " << privateQ.size() << std::endl
                << "  work-may-be-available? " << available() << std::endl
                << "}";
        }



    public:
        TaskManager (bool doSteal, Node localId, Node* neighbors, Node numLocalNodes, int chunkSize, int cbint);

        bool isWorkDone() {
            return workDone;
        }
        
        /// Maybe release tasks to the shared portion of publicQ
        void releaseTasks() {
            if (doSteal) {
                if (publicQ.localDepth() > 2 * chunkSize) {
                    publicQ.release(chunkSize);
                    stats.record_release(); 

                    // set that there COULD be work in shared portion
                    // (not "is work" because may be stolen)
                    sharedMayHaveWork = true;

                    // This has significant overhead on clusters!
                    if (publicQ.get_nNodes() % cbint == 0) { // possible for cbint to get skipped if push multiple?
                        VLOG(5) << "canceling barrier";
                        cbarrier_cancel();
                    }

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

        bool available ( ) const;
        
        void dump_stats();
        void finish();


        friend std::ostream& operator<<( std::ostream& o, const TaskManager& tm );

};


inline bool TaskManager::available( ) const {
    VLOG(5) << " sharedMayHaveWork=" << sharedMayHaveWork
            << " publicHasEle()=" << publicHasEle()
            << " privateHasEle()=" << privateHasEle();
    return privateHasEle() 
           || publicHasEle()
           || (doSteal && (sharedMayHaveWork || (okToSteal && globalMayHaveWork)));
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
    privateQ.push_front( newtask );

    /* no notification necessary since
     * presence of a local spawn means
     * we are not in the cbarrier */
}

/// Should ONLY be called from the context of
/// an AM handler
template < typename ArgsStruct > 
inline void TaskManager::spawnRemotePrivate( void (*f)(ArgsStruct * arg), ArgsStruct * arg, Node from ) {
    Task newtask = createTask(f, arg, from);
    privateQ.push_front( newtask );

    cbarrier_cancel_local();
}


#endif
