#ifndef TASK_HPP_
#define TASK_HPP_

#include <deque>
#include "StealQueue.hpp"
#include "gasnet_cbarrier.hpp"
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
        Task (void (* fn_p)(void *), void * args, const size_t args_size, Node createdOn) 
            : fn_p ( fn_p )
            , args ( args )
            , args_size ( args_size )
            , home ( createdOn ){}

        void execute ( );

        template < typename ArgsStruct >
        static Task createTask( void (* fn_p)(ArgStruct *), const ArgsStruct * args, 
                const size_t args_size = sizeof( ArgsStruct ), Node createdOn) {
            Task t( static_cast< void (*) (void*) >( fn_p )
                    static_cast< void *>( args ), args_size, createdOn );
            return t;
        }
};

// TODO: on steal of work, need to make sure args pointers are global or copy args struct


class TaskManager {
    private:
        std::deque<Task> privateQ; 
        StealQueue<Task> publicQ;

        bool workDone;
        bool mightBeWork; // flag to enable wake up optimization
       
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

        bool getNextInQueues(Task * result) {
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

    public:
        TaskManager (bool doSteal, Node localId, Node* neighbors, Node numLocalNodes, int chunkSize, int cbint);

        bool isWorkDone() {
            return workDone;
        }

        /// Possibly move work from local partition of public queue to global partition
        void releaseTasks() {
          if (doSteal) {
            if (publicQ.localDepth() > 2 * chunkSize) {
              // Attribute this time to runtime overhead
        /*      ss_setState(ss, SS_OVH);                    */
              publicQ.release(chunkSize);
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
        void spawnPublic( void (*f)(void * arg), void * arg);
        
        /*TODO return value?*/ 
        void spawnPrivate( void (*f)(void * arg), void * arg);
        
        bool getWork ( Task* result );

        bool available ( );


        static void spawnRemotePrivate( Node dest, void (*f)(void * arg), void * arg);
};



inline void TaskManager::spawnPublic( void (*f)(void * arg), void * arg ) {
    Task newtask(f, arg);
    publicQ.push( newtask );
    releaseTasks();
}

inline void TaskManager::spawnPrivate( void (*f)(void * arg), void * arg ) {
    Task newtask(f, arg);
    privateQ.push_front( newtask );
}



inline bool TaskManager::available ( ) {
    return  privateHasEle()
            || publicHasEle()
            || mightBeWork;
}



// Remote spawning
struct spawn_args {
    void (*f)(void * arg);
    void * arg;
};


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


#include "../SoftXMT.hpp"

// XXX not templatized
void Task::execute( ) {
    if ( home != SoftXMT_mynode() ) {
        Incoherent<ArgStruct>::RO cached_args( GlobalAddress<ArgStruct>::TwoDimensional(args, home) );
        rem_args.block_until_acquired();
        fn_p( cached_args );
        rem_args.block_until_released();
    } else {
        fn_p( args );
    }
}


#endif
