
#ifndef STEAL_QUEUE_HPP
#define STEAL_QUEUE_HPP

#include "uts.h"
#include "SoftXMT.hpp"

template <class T>
struct workStealRequest_args {
    int k;
    Node from;
    StealQueue<T>* victim_local;
};

template <class T>
struct workStealReply_args {
    int k;
    StealQueue<T>* thief_local;
};

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

        StealQueue<T>* staticQueueAddress;        
        
        static void workStealReply_am( workStealReply_args<T> * args,  size_t size, void * payload, size_t payload_size );
        static void workStealRequest_am( workStealRequest_args<T> * args, size_t size, void * payload, size_t payload_size );
    
    public:
        StealQueue( uint64_t numEle ) 
            : stackSize( numEle )
            , maxStackDepth( 0 )
            , nNodes( 0 ), maxTreedepth( 0 ), nVisited( 0 ), nLeaves( 0 )
            , nAcquire( 0 ), nRelease( 0 ), nSteal( 0 ), nFail( 0 )
            , wakeups( 0 ), falseWakeups( 0 ), nNodes_last( 0 ),
            , staticQueueAddress( NULL ) {

                uint64_t nbytes = nelts * sizeof(T);

                // allocate stack in shared addr space with affinity to calling thread
                // and record local addr for efficient access in sequel
                stack_g = static_cast<T*>( malloc( nbytes ) );
                stack = stack_g;

                if (stack == NULL) {
                    LOG(FATAL) << "Request for " << nbytes <<< " bytes for stealStack on thread " << SoftXMT_mynode() << " failed";
                    ss_error("unable to allocate space for stealstack");
                }

                stackLock = (LOCK_T*)malloc(sizeof(LOCK_T));
                INIT_LOCK(stackLock);

                mkEmpty();
            }
        
        void mkEmpty(); 
        void push( T c); 
        T top( ); 
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
        void registerAddress( StealQueue<T> * addr );

};

        
void ss_error( char *str ) {
    CHECK(false) << str;
}




/// Push onto top of local stack
template <class T>
inline void StealQueue<T>::push( T c ) {
  if (top >= stackSize) {
      char buf[2048];
      sprintf(buf, "ss_push: overflow (top:%d stackSize:%d)\n", s->top, s->stackSize);
      ss_error(buf);
  }
  stack[top] = c; 
  top++;
  nNodes++;
  maxStackDepth = maxint(top, maxStackDepth);
  //s->maxTreeDepth = maxint(s->maxTreeDepth, c->height); //XXX dont want to deref c here (expensive for just a bookkeeping operation
}


/// local pop
template <class T>
inline void StealQueue<T>::pop( ) {
  T* r;
  if (top <= local)
    ss_error("ss_pop: empty local stack");
  top--;
  nVisited++;
  r = &(stack[top]);
}


/// local depth
template <class T>
inline int StealQueue<T>::localDepth() {
  return (top - local);
}


template <class T>
void StealQueue<T>::ss_setState( int state ) { return; }


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


extern StealQueue<Task> my_steal_stack;

#endif
