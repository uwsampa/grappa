#include <deque>
#include "StealQueue.hpp"
#include "gasnet_cbarrier.h"
#include "thread.h"

const thread* NULL_THREAD = NULL;


/// Task is a function pointer and pointer to arguments
/// Ideally Task would be interface that just declares execute and makeGlobal
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
// TODO: on steal of work, need to make sure args pointers are global or copy args struct



class TaskManager {
    private:
        std::deque<Task> privateQ; 
        StealQueue<Task>* publicQ;

        Scheduler* scheduler;

        bool workDone;
        bool mightBeWork; // flag to enable wake up optimization
       
        bool doSteal;   // stealing on/off
        bool okToSteal; // steal lock
        int cbint;      // how often to make local public work visible
        
        // to support hierarchical dynamic load balancing
        Node localId;
        Node* neighbors;
        Node numLocalNodes;

        uint64_t num_workers;

        // steal parameters
        int chunkSize;

        thread* maybeSpawnCoroutines( );

        bool publicHasEle() {
            return publicQ->local_depth > 0;
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
                *result = publicQ->top();
                publicQ->pop( );
                return true;
            } else {
                return false;
            }
        }

    public:
        TaskManager (Scheduler* scheduler, bool doSteal, Node localId, Node* neighbors, Node numLocalNodes, int chunkSize, int cbint) 
                : workDone( false )
                , doSteal( doSteal ), okToSteal( true ), mightBeWork ( true )
                , localId( localId ), neighbors( neighbors ), numLocalNodes( numLocalNodes )
                , chunkSize( chunkSize ), cbint( cbint ) 
                , scheduler( scheduler )
                , privateQ( )
                , publicQ( &my_steal_stack ) {
                   
                   work_args.tasks = this;
        }
        
        void createWorkers( uint64_t num );


        /// Possibly move work from local partition of public queue to global partition
        void releaseTasks() {
          if (doSteal) {
            if (publicQ->localDepth > 2 * chunkSize) {
              // Attribute this time to runtime overhead
        /*      ss_setState(ss, SS_OVH);                    */
              publicQ->release(chunkSize);
              // This has significant overhead on clusters!
              if (publicQ->get_nNodes() % cbint == 0) { // possible for cbint to get skipped if push multiple?
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
        
        static void spawnRemotePrivate( void (*f)(void * arg), void * arg);
};

inline void TaskManager::spawnPublic( void (*f)(void * arg), void * arg ) {
    Task newtask(f, arg);
    publicQ->push( newtask );
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
