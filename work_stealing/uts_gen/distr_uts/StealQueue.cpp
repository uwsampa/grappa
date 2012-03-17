#include "StealQueue.hpp"
#include <boost/type_traits/is_same.hpp>
       

/// global task queue 
#define MAXSTACKDEPTH 500000 
#include "Task.hpp"
StealQueue<Task> my_steal_stack(MAXSTACKDEPTH);


        
template <class T>
void StealQueue<T>::registerAddress( StealQueue<T> * addr ) {
    CHECK( !boost::is_same<T, Task>::value ) << "[T = Task] instantiation need not call this (uses global &my_steal_stack)";
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
  if (top <= local)
    ss_error("ss_topPosn: empty local stack");
  return top - 1;
}


/// release k values from bottom of local stack
template <class T>
void StealQueue<T>::release( int k ) {
  SET_LOCK(stackLock);
  if (top - local >= k) {
    local += k;
    workAvail += k;
    nRelease++;
  }
  else
    ss_error("ss_release:  do not have k vals to release");
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
static void StealQueue<T>::workStealReply_am( workStealReply_args * args,  size_t size, void * payload, size_t payload_size ) {
    T* stolen_work = static_cast<T>( payload );
    int k = args->k;
    
    // if using StealQueue<Task> use the global task steal stack address
    StealQueue<T>* thiefStack = (boost::is_same<T, Task>::value) ? &my_steal_stack : args->thief_local;

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
static void StealQueue<T>::workStealRequest_am(workStealRequest_args<T> * args, size_t size, void * payload, size_t payload_size) {
    int k = args->k;

    // if using StealQueue<Task> use the global task steal stack address
    StealQueue<T>* victimStack = (boost::is_same<T, Task>::value) ? &my_steal_stack : args->victim_local;
   
    SET_LOCK(victimStack->stackLock);
    
    int victimLocal = victimStack->local;
    int victimShared = victimStack->sharedStart;
    int victimWorkAvail = victimStack->workAvail;
    
    if (victimLocal - victimShared != victimWorkAvail)
        ss_error("handle steal request: stealStack invariant violated");
    
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
   
        workStealReply_args reply_args = { k, args->victim_ocal };
        SoftXMT_call_on( args->from, &workStealReply_am<T>, 
                         &reply_args, sizeof(workStealReply_args), 
                         victimSharedStart, k*sizeof( T ));
    } else {
        workStealReply_args reply_args = { 0, args->victim_local };
        SoftXMT_call_on( args->from, &workStealReply_am<T>, &reply_args );
    }

}

template <class T>
int StealQueue<T>::steal_locally( Node victim, int k ) {

    SET_LOCK(&lsa_lock);
    local_steal_amount = -1;
    UNSET_LOCK(&lsa_lock);

    workStealRequest_args req_args = { k, SoftXMT_mynode(), staticQueueAddress };
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
