#include "StealQueue.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

int maxint(int x, int y) { return (x>y)?x:y; }

/* restore stack to empty state */
/* This version assumed to be called on a working copy to be written*/
void ss_mkEmpty(StealStack *s) {
  s->sharedStart = 0;
  s->local  = 0;
  s->top    = 0;
  s->workAvail = 0;
}

/* fatal error */
void ss_error(char *str) {
  printf("*** [Thread %i] %s\n",GET_THREAD_NUM, str);
  exit(4);
}

/* initialize the stack */
void ss_init(global_address/*<StealStack>*/ s, int nelts, int numNodes, int myNode) {
  StealStack sTemp;

  //int nbytes = nelts * sizeof(Node*);

  // allocate stack in shared addr space with affinity to calling thread
  // and record local addr for efficient access in sequel
  global_array* stackGA = ga_allocate(sizeof(Node_ptr), numNodes*nelts);
  ga_index(ga, myNode*nelts, &sTemp.stack_g);
  assert(gm_is_address_local(&sTemp.stack_g));
  sTemp.stack = (Node_ptr*) gm_local_gm_address_to_local_ptr(&sTemp.stack_g);   // assumes local pointer is contiguous
  
  if (sTemp.stack == NULL) {
    printf("Request for %d bytes for stealStack on thread %d failed\n",
           nbytes, GET_THREAD_NUM);
    ss_error("ss_init: unable to allocate space for stealstack");
  }

  global_array* stackLockGA = ga_allocate(sizeof(LOCK_T), numNodes);
  ga_index(ga, myNode, &sTemp.stackLock_g);
  assert(gm_is_address_local(&sTemp.stackLock_g));
  sTemp.stackLock = (LOCK_T*) gm_local_gm_address_to_local_ptr(&sTemp.stackLock_g);
  INIT_LOCK(sTemp.stackLock);
  
  sTemp.stackSize = nelts;
  sTemp.nNodes = 0;
  sTemp.maxStackDepth = 0;
  sTemp.maxTreeDepth = 0;
  sTemp.nLeaves = 0;
  sTemp.nAcquire = 0;
  sTemp.nRelease = 0;
  sTemp.nSteal = 0;
  sTemp.nFail = 0;
  sTemp.wakeups = 0;
  sTemp.falseWakeups = 0;
  sTemp.nNodes_last = 0;
  ss_mkEmpty(&sTemp);

  SET_LOCK(sTemp.stackLock);
  put_remote_address(s, &sTemp, 0, sizeof(StealStack));
  UNSET_LOCK(sTemp.stackLock);
}


/* local push */
/* This is always a local operation so pass to it a pointer-to-local
 * instead of pointer-to-shared */
void ss_push(StealStack *s, Node_ptr c) {
  if (s->top >= s->stackSize)
    ss_error("ss_push: overflow");
 
  s->stack[s->top] = c; 
  s->top++;
  s->nNodes++;
  s->maxStackDepth = maxint(s->top, s->maxStackDepth);
  s->maxTreeDepth = maxint(s->maxTreeDepth, c->height);
}


/* local top: get top element */ 
/* local-only operation */
Node * ss_top(StealStack *s) {
  Node *r;
  if (s->top <= s->local)
    ss_error("ss_top: empty local stack");
  r = s->stack[(s->top) - 1];
  
  return r;
}


/* local pop */
/* local-only operation */
void ss_pop(StealStack *s) {
  Node **r;
  if (s->top <= s->local)
    ss_error("ss_pop: empty local stack");
  s->top--;
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

int ss_steal_locally(StealStack* thief, int victim, int k) {

    /* TODO unify the ss_error checks for starting all stealing.
     * We may want to allow stealing onto non-empty stack, too. */

    SET_LOCK(stealStacks[victim].stackLock);

    int victimLocal = stealStacks[victim].local;
    int victimShared = stealStacks[victim].sharedStart;
    int victimWorkAvail = stealStacks[victim].workAvail;

    if (victimLocal - victimShared != victimWorkAvail)
        ss_error("handle steal request: stealStack invariant violated");

    int ok = victimWorkAvail >= k;
    if (ok) {
        /* reserve a chunk */
        stealStacks[victim].sharedStart =  victimShared + k;
        stealStacks[victim].workAvail = victimWorkAvail - k;
    }
    UNSET_LOCK(stealStacks[victim].stackLock);


    /* if k elts reserved, move them to local portion of our stack */
    if (ok) {
        SHARED_INDEF Node ** victimStackBase = stealStacks[victim].stack_g;
        SHARED_INDEF Node ** victimSharedStart = victimStackBase + victimShared;

        memcpy(&thief->stack[thief->top], victimSharedStart, k*sizeof(Node*));

        thief->nSteal++;
        thief->top += k;
        
        return 1;

    } else {
        thief->nFail++;
    }
    return 0;
}

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
//            SHARED_INDEF Node * victimStackBase = stealStack[victim]->stack_g;
//            SHARED_INDEF Node * victimSharedStart = victimStackBase + victimShared;
//
//            gasnet_AMReplyMedium2(token, REMOTE_STEAL_REPLY_HANDLER, victimStackBase, k*sizeof(Node), REMOTE_STEAL_STATUS_OK, thiefcore);
//            return;
//        } else {
//            continue;
//        }
//    }
//
//    gasnet_AMReplyMedium2(token, REMOTE_STEAL_REPLY_HANDLER, 0, 0, REMOTE_STEAL_STATUS_FAIL, thiefcore);
//    return;
//} 
