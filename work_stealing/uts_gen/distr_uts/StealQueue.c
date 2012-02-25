#include "StealQueue.h"
#include "SoftXMT.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

//XXX
#define debug 0
#define REMOTE_REQUEST_HANDLER 0
#define REMOTE_STEAL_STATUS_OK 0

// XXX fill in
#define GET_THREAD_NUM -1


#if OPENMP_LOCKS
/** Lock init **/
// OpenMP helper function to match UPC lock allocation semantics
omp_lock_t * omp_global_lock_alloc() {
  omp_lock_t *lock = (omp_lock_t *) malloc(sizeof(omp_lock_t) + 128);
  omp_init_lock(lock);
  return lock;
}
/**/
#endif


StealStack myStealStack; 


int maxint(int x, int y) { return (x>y)?x:y; }

/* restore stack to empty state */
/* This version assumed to be called on a working copy to be written*/
void ss_mkEmpty(StealStack *s) {
  SET_LOCK(s->stackLock);
  s->sharedStart = 0;
  s->local  = 0;
  s->top    = 0;
  s->workAvail = 0;
  UNSET_LOCK(s->stackLock);
}

/* fatal error */
void ss_error(char *str) {
  printf("*** [Thread %i] %s\n",GET_THREAD_NUM, str);
  exit(4);
}

/* initialize the stack */
void ss_init(StealStack* s, int nelts) {
  int nbytes = nelts * sizeof(Node_ptr);

  // allocate stack in shared addr space with affinity to calling thread
  // and record local addr for efficient access in sequel
  s->stack_g = (Node_ptr*) malloc (nbytes);
  s->stack = (Node_ptr*) s->stack_g;
  
  if (s->stack == NULL) {
    printf("Request for %d bytes for stealStack on thread %d failed\n",
           nbytes, GET_THREAD_NUM);
    ss_error("ss_init: unable to allocate space for stealstack");
  }

  s->stackLock = (LOCK_T*)malloc(sizeof(LOCK_T));
  INIT_LOCK(s->stackLock);
  
  s->stackSize = nelts;
  s->nNodes = 0;
  s->nVisited = 0;
  s->maxStackDepth = 0;
  s->maxTreeDepth = 0;
  s->nLeaves = 0;
  s->nAcquire = 0;
  s->nRelease = 0;
  s->nSteal = 0;
  s->nFail = 0;
  s->wakeups = 0;
  s->falseWakeups = 0;
  s->nNodes_last = 0;
  ss_mkEmpty(s);
}


/* local push */
/* This is always a local operation so pass to it a pointer-to-local
 * instead of pointer-to-shared */
void ss_push(StealStack *s, Node_ptr c) {
  if (s->top >= s->stackSize) {
      char buf[2048];
      sprintf(buf, "ss_push: overflow (top:%d stackSize:%d)\n", s->top, s->stackSize);
      ss_error(buf);
  }
  s->stack[s->top] = c; 
  s->top++;
  s->nNodes++;
  s->maxStackDepth = maxint(s->top, s->maxStackDepth);
  //s->maxTreeDepth = maxint(s->maxTreeDepth, c->height); //XXX dont want to deref c here (expensive for just a bookkeeping operation
}


/* local top: get top element */ 
/* local-only operation */
Node_ptr ss_top(StealStack *s) {
  Node_ptr r;
  if (s->top <= s->local)
    ss_error("ss_top: empty local stack");
  r = s->stack[(s->top) - 1];
  
  return r;
}


/* local pop */
/* local-only operation */
void ss_pop(StealStack *s) {
  Node_ptr* r;
  if (s->top <= s->local)
    ss_error("ss_pop: empty local stack");
  s->top--;
  s->nVisited++;
  r = &(s->stack[s->top]);
}


/* local top position:  stack index of top element */
/* local-only operation */
int ss_topPosn(StealStack *s)
{
  if (s->top <= s->local)
    ss_error("ss_topPosn: empty local stack");
  return s->top - 1;
}


/* local depth */
/* local-only operation */
int ss_localDepth(StealStack *s) {
  return (s->top - s->local);
}



/* release k values from bottom of local stack */
void ss_release(StealStack *s, int k) {
  SET_LOCK(s->stackLock);
  if (s->top - s->local >= k) {
    s->local += k;
    s->workAvail += k;
    s->nRelease++;
  }
  else
    ss_error("ss_release:  do not have k vals to release");
  UNSET_LOCK(s->stackLock);
}


/* move k values from top of shared stack into local stack
 * return false if k vals are not avail on shared stack
 */
int ss_acquire(StealStack *s, int k) {
  int avail;
  SET_LOCK(s->stackLock);
  avail = s->local - s->sharedStart;
  if (avail >= k) {
    s->local -= k;
    s->workAvail -= k;
    s->nAcquire++;
  }
  UNSET_LOCK(s->stackLock);
  return (avail >= k);
}

void ss_setState(StealStack* s, int state) { return; }

int local_steal_amount;
LOCK_T lsa_lock = LOCK_INITIALIZER;
//hack functions to protect GASNET_BLOCKUNTIL cond expression in a lock
int SET_COND(LOCK_T* lock) {
    SET_LOCK(lock);
    return 1;
}
int UNSET_COND(LOCK_T* lock) {
    UNSET_LOCK(lock);
    return 0;
}

///////////////////////////////////////////////////
// Pushing work
//////////////////////////////////////////////////
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

void ss_pushRemote(Node destnode, Node_ptr* work, int k) {
    pushWorkRequest_args pargs = { k };
    SoftXMT_call_on( destnode, &pushWorkRequest_am, &pargs, sizeof(pushWorkRequest_args), work, sizeof(Node_ptr)*k);
}
////////////////////////////////////////////////////

/////////////////////////////////////////////
// Work stealing
/////////////////////////////////////////////
struct workStealRequest_args {
    int k;
    Node from;
};

struct workStealReply_args {
    int k;
};

void workStealRequest_am(workStealRequest_args * args, size_t size, void * payload, size_t payload_size);
void workStealReply_am( workStealReply_args * args,  size_t size, void * payload, size_t payload_size );

int ss_steal_locally(StealStack* thief, int victim, int k) {
    assert(thief == &myStealStack);//TODO just remove the parameter

    SET_LOCK(&lsa_lock);
    local_steal_amount = -1;
    UNSET_LOCK(&lsa_lock);

    workStealRequest_args req_args = { k, SoftXMT_mynode() };
    SoftXMT_call_on( victim, &workStealRequest_am, &req_args );

    // steal is blocking
    // TODO: change to blocking thread?
    while (true) {
        SET_LOCK(&lsa_lock);
        if (local_steal_amount != -1) {
            break;
        }
        UNSET_LOCK(&lsa_lock);
        SoftXMT_yield();
    }
    
    if (local_steal_amount > 0) {
        myStealStack.nSteal++;
        myStealStack.top += local_steal_amount;
    }
    UNSET_LOCK(&lsa_lock);

    return local_steal_amount;
}



void workStealRequest_am(workStealRequest_args * args, size_t size, void * payload, size_t payload_size) {
    int k = args->k;

    StealStack* victimStack = &myStealStack;
   
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
   
        workStealReply_args reply_args = { k };
        SoftXMT_call_on( args->from, &workStealReply_am, 
                         &reply_args, sizeof(workStealReply_args), 
                         victimSharedStart, k*sizeof(Node_ptr));
    } else {
        workStealReply_args reply_args = { 0 };
        SoftXMT_call_on( args->from, &workStealReply_am, &reply_args );
    }

}

void workStealReply_am( workStealReply_args * args,  size_t size, void * payload, size_t payload_size ) {
    Node_ptr* stolen_work = (Node_ptr*) payload;
    int k = args->k;

    if (k > 0) {
        SET_LOCK(&lsa_lock);
        memcpy(&myStealStack.stack[myStealStack.top], stolen_work, payload_size);
        local_steal_amount = k;
        UNSET_LOCK(&lsa_lock);
    } else {
        myStealStack.nFail++;
        SET_LOCK(&lsa_lock);
        local_steal_amount = 0;
        UNSET_LOCK(&lsa_lock);
    }
    
}
//////////////////////////////////////////////////////////////////////

//TODO: should global steal go into a node-level queue or into the core's queue? 
//core's queue:
//  + the core won't have to try restealing
//  - require locking if asynch?
//  impl: might have the core go into gasnet_blockuntil, except for green threads need to allow
//        the other threads to work on their work. But perhaps a sched should not let global
//        steal be required unless whole core out of work
//node queue:
//  + allows decouple of core after initiate a global steal; a core can pull off this queue when it wants to
//  - extra queue to get through


/* initiate asynchronous global steal k values from shared portion of victim thread's stealStack
 * onto local portion of current thread's stealStack.
 * return false if k vals are not avail in victim thread
 */
//int ss_remote_steal(StealStack *s, int thiefCore, int victimNode, int k) {
//  
//  if (s->sharedStart != s->top)
//    ss_error("ss_steal: thief attempts to steal onto non-empty stack");
//
//  if (s->top + k >= s->stackSize)
//    ss_error("ss_steal: steal will overflow thief's stack");
//  
//  /* lock victim stack and try to reserve k elts */
//  if (debug & 32)
//    printf("Thread %d wants    SS %d\n", GET_THREAD_NUM, victimNode);
// 
// 
//  /** send the am request **/
//  gasnet_AMRequestShort1(victimNode, REMOTE_REQUEST_HANDLER, thiefCore);
//
//}


// assumes only one remote steal for this thief's stack is handled at one time; 
// also that other queue ops are suspended until this is done
//      -this is not acceptable, since we'd like the queue to be worked on asynchronous to a remote steal
//      -could compromise by not allowing release/acquire to be called until the remote steal is done
//void remote_steal_reply_handler(gasnet_token_t token, void* buf, size_t nbytes, gasnet_handlerarg_t a0, gasnet_handlerarg_t a1) {
//    int thiefcore = (int) a0;
//    int status = (int) a1;
//
//    if (status == REMOTE_STEAL_STATUS_OK) {
//        StealQueue* thiefStack = stealStack[thiefcore];
//        SMEMCPY(&thiefStack->stack[thiefStack->top], buf, nbytes);
//        thiefStack->nRemSteals++; 
//        thiefStack->top += k;
//    }
//}

//void remote_steal_handler(gasnet_token_t token, gasnet_handlerarg_t a0) {
//    int thiefcore = (int) a0;
//
//    for (permutation of vic cores ==> victim) {
//        SET_LOCK(stealStack[victim]->stackLock);
//
//        int victimLocal = stealStack[victim]->local;
//        int victimShared = stealStack[victim]->sharedStart;
//        int victimWorkAvail = stealStack[victim]->workAvail;
//
//        if (victimLocal - victimShared != victimWorkAvail)
//            ss_error("handle steal request: stealStack invariant violated");
//
//        int ok = victimWorkAvail >= k;
//        if (ok) {
//            /* reserve a chunk */
//            stealStack[victim]->sharedStart =  victimShared + k;
//            stealStack[victim]->workAvail = victimWorkAvail - k;
//        }
//        UNSET_LOCK(stealStack[victim]->stackLock);
//
//
//        /* if k elts reserved, move them to local portion of our stack */
//        if (ok) {
//            SHARED_INDEF TreeNode * victimStackBase = stealStack[victim]->stack_g;
//            SHARED_INDEF TreeNode * victimSharedStart = victimStackBase + victimShared;
//
//            gasnet_AMReplyMedium2(token, REMOTE_STEAL_REPLY_HANDLER, victimStackBase, k*sizeof(TreeNode), REMOTE_STEAL_STATUS_OK, thiefcore);
//            return;
//        } else {
//            continue;
//        }
//    }
//
//    gasnet_AMReplyMedium2(token, REMOTE_STEAL_REPLY_HANDLER, 0, 0, REMOTE_STEAL_STATUS_FAIL, thiefcore);
//    return;
//} 
