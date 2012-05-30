
#ifndef STEAL_QUEUE_HPP
#define STEAL_QUEUE_HPP

#include <iostream>
#include <glog/logging.h>   
#include <stdlib.h>

// profiling/tracing
#include "../PerformanceTools.hpp"

GRAPPA_DECLARE_EVENT_GROUP(scheduler);

#define SS_NSTATES 1

struct workStealRequest_args;
struct workStealReply_args;

/// Type for Node ID. 
#include <boost/cstdint.hpp>
typedef int16_t Node;

/// Forward declare for steal_locally
class Thread;


template <typename T>
class StealQueue {
    private:
        uint64_t stackSize;     /* total space avail (in number of elements) */
        uint64_t workAvail;     /* elements available for stealing */
        uint64_t sharedStart;   /* index of start of shared portion of stack */
        uint64_t local;         /* index of start of local portion */
        uint64_t top;           /* index of stack top */
        uint64_t maxStackDepth;                      /* stack stats */ 
        uint64_t nNodes, maxTreeDepth, nVisited, nLeaves;        /* tree stats: (num pushed, max depth, num popped, leaves)  */
        uint64_t nAcquire, nRelease, nSteal, nFail;  /* steal stats */
        uint64_t wakeups, falseWakeups, nNodes_last;
        double time[SS_NSTATES], timeLast;
        /* perf measurements */ 
        int entries[SS_NSTATES], curState; 
        /* lock for manipulation of shared portion */ 
        T* stack;       /* addr of actual
                                  stack of nodes
                                  in local addr
                                  space */
        T* stack_g; /* addr of same
                              stack in global
                              addr space */

        static StealQueue<T>* staticQueueAddress;        
        
        static void workStealReply_am( workStealReply_args * args,  size_t size, void * payload, size_t payload_size );
        static void workStealRequest_am( workStealRequest_args * args, size_t size, void * payload, size_t payload_size );
        
        std::ostream& dump ( std::ostream& o ) const {
            return o << "StealQueue[localDepth=" << localDepth()
                     << "; sharedDepth=" << sharedDepth()
                     << "; indices(top= " << top 
                                   << " local=" << local 
                                   << " sharedStart=" << sharedStart << ")"
                     << "; stackSize=" << stackSize << "]";
        }
    
    public:
        StealQueue( uint64_t numEle ) 
            : stackSize( numEle )
            , maxStackDepth( 0 )
            , nNodes( 0 ), maxTreeDepth( 0 ), nVisited( 0 ), nLeaves( 0 )
            , nAcquire( 0 ), nRelease( 0 ), nSteal( 0 ), nFail( 0 )
            , wakeups( 0 ), falseWakeups( 0 ), nNodes_last( 0 ) {

                uint64_t nbytes = numEle * sizeof(T);

                // allocate stack in shared addr space with affinity to calling thread
                // and record local addr for efficient access in sequel
                stack_g = static_cast<T*>( malloc( nbytes ) );
                stack = stack_g;

                CHECK( stack!= NULL ) << "Request for " << nbytes << " bytes for stealStack failed";

                mkEmpty();
            }
        
        void mkEmpty(); 
        void push( T c); 
        T peek( ); 
        void pop( ); 
        uint64_t topPosn( ) const;
        uint64_t localDepth( ) const; 
        uint64_t sharedDepth( ) const;
        void release( int k ); 
        int acquire( int k ); 
        int steal_locally( Node victim, int chunkSize, Thread * current ); 
        void setState( int state );
        
        uint64_t get_nNodes( ) {
            return nNodes;
        }
        
        /// register local address of remote steal queues
        static void registerAddress( StealQueue<T> * addr );

        template< typename U >
        friend std::ostream& operator<<( std::ostream& o, const StealQueue<U>& sq );
};


static int maxint(int x, int y) { return (x>y)?x:y; }

/// Push onto top of local stack
template <typename T>
inline void StealQueue<T>::push( T c ) {
  CHECK( top < stackSize ) << "push: overflow (top:" << top << " stackSize:" << stackSize << ")";

  stack[top] = c; 
  top++;
  nNodes++;
  maxStackDepth = maxint(top, maxStackDepth);
  //s->maxTreeDepth = maxint(s->maxTreeDepth, c->height); //XXX dont want to deref c here (expensive for just a bookkeeping operation
}

/// get top element
template <typename T>
inline T StealQueue<T>::peek( ) {
    CHECK(top > local) << "peek: empty local stack";
    return stack[top-1];
}

/// local pop
template <typename T>
inline void StealQueue<T>::pop( ) {
  CHECK(top > local) << "pop: empty local stack";
  
  // 0 out the popped element (to detect errors)
  memset( &stack[top-1], 0, sizeof(T) );
  
  top--;
  nVisited++;
}


/// local depth
template <typename T>
inline uint64_t StealQueue<T>::localDepth() const {
  return (top - local);
}


template <typename T>
void StealQueue<T>::setState( int state ) { return; }


/// Initialize the dedicated queue for T
template <typename T>
StealQueue<T>* StealQueue<T>::staticQueueAddress = NULL;

template <typename T>
void StealQueue<T>::registerAddress( StealQueue<T> * addr ) {
    staticQueueAddress = addr;
}

/// set queue to empty
template <typename T>
void StealQueue<T>::mkEmpty( ) {
    sharedStart = 0;
    local  = 0;
    top    = 0;
    workAvail = 0;
}

/// local top position:  stack index of top element
template <typename T>
uint64_t StealQueue<T>::topPosn() const
{
  CHECK ( top > local ) << "ss_topPosn: empty local stack";
  return top - 1;
}


/// release k values from bottom of local stack
template <typename T>
void StealQueue<T>::release( int k ) {
  CHECK(top - local >= k) << "ss_release:  do not have k vals to release";
 
  // the above check is not time-of-check-to-use bug, because top/local guarenteed
  // not to change. We just need to update them
  VLOG(4) << "(before)release: k=" << k << " local size=" << top-local << " sharedSize=" << workAvail;
  local += k;
  workAvail += k;
  VLOG(4) << "(after)release: k=" << k << " local size=" << top-local << " sharedSize=" << workAvail;
  nRelease++;

}


/// move k values from top of shared stack into local stack
/// return false if k vals are not avail on shared stack
template <typename T>
int StealQueue<T>::acquire( int k ) {
  int avail;
  avail = local - sharedStart;
  VLOG(4) << "(before)acquire: k=" << k << " avail=" << avail << " acquire?=" << (avail>=k);
  if (avail >= k) {
    local -= k;
    workAvail -= k;
    VLOG(4) << "(after)acquire: local" << local << " workAvail=" << workAvail;
    nAcquire++;
  }
  return (avail >= k);
}

template <typename T>
uint64_t StealQueue<T>::sharedDepth( ) const {
    return local - sharedStart;
}


//////////////////////////////////////////////////
// Work stealing
/////////////////////////////////////////////////

//#include "../SoftXMT.hpp" 
#include <Communicator.hpp>
#include <tasks/TaskingScheduler.hpp>

extern TaskingScheduler global_scheduler;
// void SoftXMT_suspend();
// void SoftXMT_wake( Thread * );
// Node SoftXMT_mynode();

struct workStealRequest_args {
    int k;
    Node from;
};

struct workStealReply_args {
    int k;
};

static int local_steal_amount;
static Thread * steal_waiter = NULL;

template <typename T>
void StealQueue<T>::workStealReply_am( workStealReply_args * args,  size_t size, void * payload, size_t payload_size ) {
    CHECK ( local_steal_amount == -1 ) << "local_steal_amount=" << local_steal_amount << " when steal reply arrives";

    T * stolen_work = static_cast<T*>( payload );
    int k = args->k;
    
    StealQueue<T>* thiefStack = StealQueue<T>::staticQueueAddress;

    if (k > 0) {
        GRAPPA_EVENT(steal_success_ev, "Steal success", 1, scheduler, k);
        //TAU_REGISTER_EVENT(steal_success_ev, "Steal success");
        //TAU_EVENT(steal_success_ev, k);
        
        memcpy(&thiefStack->stack[thiefStack->top], stolen_work, payload_size);
        local_steal_amount = k;
        
        thiefStack->top += local_steal_amount;
        thiefStack->nSteal++;
    } else {
        local_steal_amount = 0;
        
        thiefStack->nFail++;
    }

    if ( steal_waiter != NULL ) {
      //SoftXMT_wake( steal_waiter );
      global_scheduler.thread_wake( steal_waiter );
        steal_waiter = NULL;
    }
}



template <typename T>
void StealQueue<T>::workStealRequest_am(workStealRequest_args * args, size_t size, void * payload, size_t payload_size) {
    int k = args->k;

    StealQueue<T>* victimStack = StealQueue<T>::staticQueueAddress;
   
    int victimLocal = victimStack->local;
    int victimShared = victimStack->sharedStart;
    int victimWorkAvail = victimStack->workAvail;
    
    CHECK( victimLocal - victimShared == victimWorkAvail ) << "handle steal request: stealStack invariant violated";
    
    int ok = victimWorkAvail >= k;
    VLOG(4) << "Victim (Node " << global_communicator.mynode() << ") victimWorkAvail=" << victimWorkAvail << " k=" << k;
    if (ok) {
        /* reserve a chunk */
        victimStack->sharedStart =  victimShared + k;
        victimStack->workAvail = victimWorkAvail - k;
    }
    
    /* if k elts reserved, move them to local portion of our stack */
    if (ok) {
        GRAPPA_EVENT(steal_victim_ev, "Steal victim", 1, scheduler, k);
        //TAU_REGISTER_EVENT(steal_victim_ev, "Steal victim");
        //TAU_EVENT(steal_victim_ev, k);

        T* victimStackBase = victimStack->stack;
        T* victimSharedStart = victimStackBase + victimShared;
   
        workStealReply_args reply_args = { k };
        SoftXMT_call_on( args->from, &StealQueue<T>::workStealReply_am, 
                         &reply_args, sizeof(workStealReply_args), 
                         victimSharedStart, k*sizeof( T ));
        
        // 0 out the stolen stuff (to detect errors)
        memset( victimSharedStart, 0, k*sizeof( T ) );
    } else {
        workStealReply_args reply_args = { 0 };
        SoftXMT_call_on( args->from, &StealQueue<T>::workStealReply_am, &reply_args );
    }

}

#include "Thread.hpp"
template <typename T>
int StealQueue<T>::steal_locally( Node victim, int k, Thread * current ) {

    local_steal_amount = -1;

    workStealRequest_args req_args = { k, global_communicator.mynode() };
    SoftXMT_call_on( victim, &StealQueue<T>::workStealRequest_am, &req_args );

    void * stealprof;
    TAU_PROFILER_CREATE( stealprof, "steal_locally", "(suspended)", TAU_USER2);

    // steal is blocking
    // TODO: use suspend-wake mechanism
    while ( local_steal_amount == -1 ) {
        steal_waiter = current;
        
        TAU_PROFILER_START_TASK( stealprof, current->tau_taskid );
	    
        global_scheduler.thread_suspend();
        //SoftXMT_suspend();
        
        TAU_PROFILER_STOP_TASK( stealprof, current->tau_taskid );

        SoftXMT_suspend();
    }

    return local_steal_amount;
}
/////////////////////////////////////////////////////////

template <typename T>
std::ostream& operator<<( std::ostream& o, const StealQueue<T>& sq ) {
    return sq.dump( o );
}

#endif
