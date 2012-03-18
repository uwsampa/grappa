#include "StealQueue.hpp"
#include <glog/logging.h>   

/// Initialize the dedicated queue for T
template <class T>
StealQueue<T>::staticQueueAddress = NULL;

template <class T>
void StealQueue<T>::registerAddress( StealQueue<T> * addr ) {
    staticQueueAddress = addr;
}


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

int local_steal_amount;
LOCK_T lsa_lock = LOCK_INITIALIZER;


template <class T>
void StealQueue<T>::workStealReply_am( workStealReply_args * args,  size_t size, void * payload, size_t payload_size ) {
    T* stolen_work = static_cast<T*>( payload );
    int k = args->k;
    
    StealQueue<T>* thiefStack = StealQueue<T>::staticQueueAddress;

    if (k > 0) {
        SET_LOCK(&lsa_lock);
        memcpy(thiefStack->stack[thiefStack->top], stolen_work, payload_size);
        local_steal_amount = k;
        UNSET_LOCK(&lsa_lock);
    } else {
        thiefStack->nFail++;
        SET_LOCK(&lsa_lock);
        local_steal_amount = 0;
        UNSET_LOCK(&lsa_lock);
    }
    
}

template <class T>
void StealQueue<T>::workStealRequest_am(workStealRequest_args<T> * args, size_t size, void * payload, size_t payload_size) {
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
        Node_ptr* victimStackBase = victimStack->stack;
        Node_ptr* victimSharedStart = victimStackBase + victimShared;
   
        workStealReply_args<T> reply_args = { k };
        SoftXMT_call_on( args->from, &workStealReply_am<T>, 
                         &reply_args, sizeof(workStealReply_args), 
                         victimSharedStart, k*sizeof( T ));
    } else {
        workStealReply_args<T> reply_args = { 0 };
        SoftXMT_call_on( args->from, &workStealReply_am<T>, &reply_args );
    }

}

template <class T>
int StealQueue<T>::steal_locally( Node victim, int k ) {

    SET_LOCK(&lsa_lock);
    local_steal_amount = -1;
    UNSET_LOCK(&lsa_lock);

    workStealRequest_args<T> req_args = { k, SoftXMT_mynode() };
    SoftXMT_call_on( victim, &workStealRequest_am<T>, &req_args );

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
