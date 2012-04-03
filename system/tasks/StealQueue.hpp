
#ifndef STEAL_QUEUE_HPP
#define STEAL_QUEUE_HPP

#include <boost/cstdint.hpp>
#include <glog/logging.h>   

// TODO remove locks
#include <gasnet.h>
#define INIT_LOCK gasnet_hsl_init
#define SET_LOCK gasnet_hsl_lock
#define UNSET_LOCK gasnet_hsl_unlock
#define LOCK_T gasnet_hsl_t
#define LOCK_INITIALIZER GASNET_HSL_INITIALIZER

#define SS_NSTATES 1

struct workStealRequest_args;
struct workStealReply_args;

/// Type for Node ID. 
typedef int16_t Node;


template <class T>
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
        LOCK_T* stackLock; 
        LOCK_T* stackLock_g;
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

                stackLock = (LOCK_T*)malloc(sizeof(LOCK_T));
                INIT_LOCK(stackLock);

                mkEmpty();
            }
        
        void mkEmpty(); 
        void push( T c); 
        T peek( ); 
        void pop( ); 
        uint64_t topPosn( );
        uint64_t localDepth( ); 
        void release( int k ); 
        int acquire( int k ); 
        int steal_locally( Node victim, int chunkSize ); 
        void setState( int state );
        
        uint64_t get_nNodes( ) {
            return nNodes;
        }
        
        /// register local address of remote steal queues
        static void registerAddress( StealQueue<T> * addr );

};


static int maxint(int x, int y) { return (x>y)?x:y; }

/// Push onto top of local stack
template <class T>
inline void StealQueue<T>::push( T c ) {
  CHECK( top < stackSize ) << "push: overflow (top:" << top << " stackSize:" << stackSize << ")";

  stack[top] = c; 
  top++;
  nNodes++;
  maxStackDepth = maxint(top, maxStackDepth);
  //s->maxTreeDepth = maxint(s->maxTreeDepth, c->height); //XXX dont want to deref c here (expensive for just a bookkeeping operation
}

/// get top element
template <class T>
inline T StealQueue<T>::peek( ) {
    CHECK(top > local) << "peek: empty local stack";
    return stack[top-1];
}

/// local pop
template <class T>
inline void StealQueue<T>::pop( ) {
  CHECK(top > local) << "pop: empty local stack";
  top--;
  nVisited++;
}


/// local depth
template <class T>
inline uint64_t StealQueue<T>::localDepth() {
  return (top - local);
}


template <class T>
void StealQueue<T>::setState( int state ) { return; }


/// Initialize the dedicated queue for T
template <class T>
StealQueue<T>* StealQueue<T>::staticQueueAddress = NULL;

template <class T>
void StealQueue<T>::registerAddress( StealQueue<T> * addr ) {
    staticQueueAddress = addr;
}

/// set queue to empty
template <class T>
void StealQueue<T>::mkEmpty( ) {
    SET_LOCK(stackLock);
    sharedStart = 0;
    local  = 0;
    top    = 0;
    workAvail = 0;
    UNSET_LOCK(stackLock);
}

/// local top position:  stack index of top element
template <class T>
uint64_t StealQueue<T>::topPosn()
{
  CHECK ( top > local ) << "ss_topPosn: empty local stack";
  return top - 1;
}


/// release k values from bottom of local stack
template <class T>
void StealQueue<T>::release( int k ) {
  CHECK(top - local >= k) << "ss_release:  do not have k vals to release";
 
  // the above check is not time-of-check-to-use bug, because top/local guarenteed
  // not to change. We just need to update them
  SET_LOCK(stackLock);
  local += k;
  workAvail += k;
  nRelease++;

  UNSET_LOCK(stackLock);
}


/// move k values from top of shared stack into local stack
/// return false if k vals are not avail on shared stack
template <class T>
int StealQueue<T>::acquire( int k ) {
  int avail;
  SET_LOCK(stackLock);
  avail = local - sharedStart;
  if (avail >= k) {
    local -= k;
    workAvail -= k;
    nAcquire++;
  }
  UNSET_LOCK(stackLock);
  return (avail >= k);
}


//////////////////////////////////////////////////
// Work stealing
/////////////////////////////////////////////////

/* TODO remove the locks */

struct workStealRequest_args {
    int k;
    Node from;
};

struct workStealReply_args {
    int k;
};

static int local_steal_amount;
static LOCK_T lsa_lock = LOCK_INITIALIZER;

template <class T>
void StealQueue<T>::workStealReply_am( workStealReply_args * args,  size_t size, void * payload, size_t payload_size ) {
    T* stolen_work = static_cast<T*>( payload );
    int k = args->k;
    
    StealQueue<T>* thiefStack = StealQueue<T>::staticQueueAddress;

    if (k > 0) {
        SET_LOCK(&lsa_lock);
        memcpy(&thiefStack->stack[thiefStack->top], stolen_work, payload_size);
        local_steal_amount = k;
        UNSET_LOCK(&lsa_lock);
    } else {
        thiefStack->nFail++;
        SET_LOCK(&lsa_lock);
        local_steal_amount = 0;
        UNSET_LOCK(&lsa_lock);
    }
    
}

#include "../SoftXMT.hpp" // TODO: SoftXMT_comm only

template <class T>
void StealQueue<T>::workStealRequest_am(workStealRequest_args * args, size_t size, void * payload, size_t payload_size) {
    int k = args->k;

    StealQueue<T>* victimStack = StealQueue<T>::staticQueueAddress;
   
    SET_LOCK(victimStack->stackLock);
    
    int victimLocal = victimStack->local;
    int victimShared = victimStack->sharedStart;
    int victimWorkAvail = victimStack->workAvail;
    
    CHECK( victimLocal - victimShared == victimWorkAvail ) << "handle steal request: stealStack invariant violated";
    
    int ok = victimWorkAvail >= k;
    if (ok) {
        /* reserve a chunk */
        victimStack->sharedStart =  victimShared + k;
        victimStack->workAvail = victimWorkAvail - k;
    }
    UNSET_LOCK(victimStack->stackLock);
    
    /* if k elts reserved, move them to local portion of our stack */
    if (ok) {
        T* victimStackBase = victimStack->stack;
        T* victimSharedStart = victimStackBase + victimShared;
   
        workStealReply_args reply_args = { k };
        SoftXMT_call_on( args->from, &StealQueue<T>::workStealReply_am, 
                         &reply_args, sizeof(workStealReply_args), 
                         victimSharedStart, k*sizeof( T ));
    } else {
        workStealReply_args reply_args = { 0 };
        SoftXMT_call_on( args->from, &StealQueue<T>::workStealReply_am, &reply_args );
    }

}

template <class T>
int StealQueue<T>::steal_locally( Node victim, int k ) {

    SET_LOCK(&lsa_lock);
    local_steal_amount = -1;
    UNSET_LOCK(&lsa_lock);

    workStealRequest_args req_args = { k, SoftXMT_mynode() };
    SoftXMT_call_on( victim, &StealQueue<T>::workStealRequest_am, &req_args );

    // steal is blocking
    // TODO: use suspend-wake mechanism
    while (true) {
        SET_LOCK(&lsa_lock);
        if (local_steal_amount != -1) {
            break;
        }
        UNSET_LOCK(&lsa_lock);
        SoftXMT_yield();
    }
    
    if (local_steal_amount > 0) {
        nSteal++;
        top += local_steal_amount;
    }
    UNSET_LOCK(&lsa_lock);

    return local_steal_amount;
}
/////////////////////////////////////////////////////////







///////////////////////////////////////////////////
// Pushing work
//////////////////////////////////////////////////
//
// 
/*
struct pushWorkRequest_args {
    int amount_pushed; 
};

void pushWorkRequest_am( pushWorkRequest_args * args, size_t size, void * payload, size_t payload_size ) {
    Node_ptr* received_work = (Node_ptr*) payload;
    int amount_pushed = args->amount_pushed;
    
    SET_LOCK(&lsa_lock);
    memcpy(&myStealStack.stack[myStealStack.top], received_work, payload_size);
    myStealStack.top += amount_pushed;
    UNSET_LOCK(&lsa_lock);
}

/// Push work offline to a remote node. 
/// Not concurrency-safe because pushes to local portion of queue,
/// which is not lock protected for pops.
void StealQueue::pushRemote(Node destnode, Node_ptr* work, int k) {
    pushWorkRequest_args pargs = { k };
    SoftXMT_call_on( destnode, &pushWorkRequest_am, &pargs, sizeof(pushWorkRequest_args), work, sizeof(Node_ptr)*k);
}
*/
////////////////////////////////////////////////////


#endif
