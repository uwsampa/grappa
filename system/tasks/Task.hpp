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
        bool mightBeWork; // flag to enable wake up optimization
       
        bool inCBarrier;

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
        template < typename ArgsStruct > 
        void spawnPublic( void (*f)(ArgsStruct * arg), ArgsStruct * arg);
        
        /*TODO return value?*/ 
        template < typename ArgsStruct > 
        void spawnPrivate( void (*f)(ArgsStruct * arg), ArgsStruct * arg);
        
        bool getWork ( Task* result );

        bool available ( );


        static void spawnRemotePrivate( Node dest, void (*f)(void * arg), void * arg);
};





inline bool TaskManager::available ( ) {
    return  privateHasEle()
            || publicHasEle()
            || mightBeWork;
}


Node SoftXMT_mynode();
template < typename ArgsStruct > 
inline void TaskManager::spawnPublic( void (*f)(ArgsStruct * arg), ArgsStruct * arg ) {
    Task newtask = createTask(f, arg, SoftXMT_mynode());
    publicQ.push( newtask );
    releaseTasks();
}

template < typename ArgsStruct > 
inline void TaskManager::spawnPrivate( void (*f)(ArgsStruct * arg), ArgsStruct * arg ) {
    Task newtask = createTask(f, arg, SoftXMT_mynode());
    privateQ.push_front( newtask );

    // set to make sure that any alive worker looking for tasks knows not to idle
    mightBeWork = true;
    
    // TODO: only need to check this if spawnPrivate called from AM
    // ie write a separate remote spawn
    
    // goal is just to notify the barrier that mynode is not in the barrier
    // any longer, so just call cancel if mynode is in the barrier
    if ( inCBarrier ) { 
        cbarrier_cancel();
        //TODO more efficient local cancel since we produced only private work
    }

}



// Remote spawning
struct spawn_args {
    void (*f)(void * arg);
    void * arg;
};



#endif
