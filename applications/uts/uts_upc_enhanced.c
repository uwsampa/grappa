/*
 *         ---- The Unbalanced Tree Search (UTS) Benchmark ----
 *  
 *  Copyright (c) 2010 See AUTHORS file for copyright holders
 *
 *  This file is part of the unbalanced tree search benchmark.  This
 *  project is licensed under the MIT Open Source license.  See the LICENSE
 *  file for copyright and licensing information.
 *
 *  UTS is a collaborative project between researchers at the University of
 *  Maryland, the University of North Carolina at Chapel Hill, and the Ohio
 *  State University.  See AUTHORS file for more information.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "uts.h"

/***********************************************************
 *                                                         *
 *  Compiler Type (these flags are set by at compile time) *
 *     (default) ANSI C compiler - sequential execution    *
 *     (__UPC__) UPC compiler                              *
 *                                                         *
 ***********************************************************/

/**** UPC Definitions ****/
#if defined(__UPC__)
#include <upc.h>
#define PARALLEL         1
#define COMPILER_TYPE    2
#define SHARED           shared
#define SHARED_INDEF     shared [0]
#define VOLATILE         strict
#define MAX_THREADS       (THREADS)
#define LOCK_T           upc_lock_t
#define GET_NUM_THREADS  (THREADS)
#define GET_THREAD_NUM   (MYTHREAD)
#define SET_LOCK(zlk)    upc_lock(zlk)
#define UNSET_LOCK(zlk)  upc_unlock(zlk)
#define INIT_LOCK(zlk)   zlk=upc_global_lock_alloc()
#define INIT_SINGLE_LOCK(zlk) zlk=upc_all_lock_alloc()
#define SMEMCPY          upc_memget
#define ALLOC            upc_alloc
#define BARRIER          upc_barrier;


/**** Default Sequential Definitions ****/
#else
#define PARALLEL         0
#define COMPILER_TYPE    0
#define SHARED
#define SHARED_INDEF
#define VOLATILE
#define MAX_THREADS 1
#define LOCK_T           void
#define GET_NUM_THREADS  1
#define GET_THREAD_NUM   0
#define SET_LOCK(zlk)    
#define UNSET_LOCK(zlk)  
#define INIT_LOCK(zlk) 
#define INIT_SINGLE_LOCK(zlk) 
#define SMEMCPY          memcpy
#define ALLOC            malloc
#define BARRIER           

#endif /* END Par. Model Definitions */


/***********************************************************
 *  Parallel execution parameters                          *
 ***********************************************************/

int doSteal   = PARALLEL; // 1 => use work stealing
int chunkSize = 20;       // number of nodes to move to/from shared area
int stealPollInterval = 20;  // number of nodes generated between poll for steal requests (set equal to chunk size)


#ifdef __BERKELEY_UPC__
/* BUPC nonblocking I/O Handles */
bupc_handle_t cb_handle       = BUPC_COMPLETE_HANDLE;
const int     local_cb_cancel = 1;
#endif


/***********************************************************
 *  Execution Tracing                                      *
 ***********************************************************/

#define SS_WORK    0   /* performing local work */
#define SS_SEARCH  1   /* searching for non-local work */
#define SS_IDLE    2   /* in termination detection */
#define SS_OVH     3
#define SS_WAIT    4   /* enqueued waiting for steal */
#define SS_NSTATES 5

/* session record for session visualization */
struct sessionRecord_t {
  double startTime, endTime;
};
typedef struct sessionRecord_t SessionRecord;

/* steal record for steal visualization */
struct stealRecord_t {
  long int nodeCount;           /* count nodes generated during the session */
  int victimThread;             /* thread from which we stole the work  */
};
typedef struct stealRecord_t StealRecord;

/* Store debugging and trace data */
struct metaData_t {
  SessionRecord sessionRecords[SS_NSTATES][20000];   /* session time records */
  StealRecord stealRecords[20000]; /* steal records */
};
typedef struct metaData_t MetaData;

/* holds text string for debugging info */
char debug_str[1000];


/***********************************************************
/* Values with special interpretation                      *
 ***********************************************************/

#define NOT_WORKING      -1  /* work_available */
#define REQ_AVAILABLE    -1  /* req_thread */
#define REQ_UNAVAILABLE  -2  /* req_thread */
#define WAITING_FOR_WORK -1  /* stolen_work_addr */
#define NONE_STOLEN      -2  /* stolen_work_addr */
#define NONE_WORKING     -2  /* result of probe sequence */
#define NO_WORK_AVAIL    -1  /* result of probe sequence */                               


/***********************************************************
 * StealStack types                                        *
 ***********************************************************/

#define MAXSTACKDEPTH 2000000

/* stack of nodes */
struct stealStack_t
{
  /* note next few fields are subject to concurrent access and should 
   * be separated to avoid false sharing in shared mem systems
   */
  long workAvail;     /* elements on local stack */
  int pad1[128];
  int req_thread;     /* in victim, id of queued thief */
  int pad2[128];
  long stolen_work_addr; /* in thief, location of granted steal */
  int pad3[128];
  long stolen_work_amt; /* in thief, number of nodes stolen */
  int pad4[128];
  LOCK_T * reqLock; /* lock to make steal requests exclusive */

  long stackSize;     /* total space avail (in number of elements) */
  long sharedStart;   /* index of start of shared portion of stack */
  long local;         /* index of start of local portion */
  long top;           /* index of stack top */
  long maxStackDepth;                      /* stack stats */ 
  long nNodes, maxTreeDepth;               /* tree stats  */
  long nLeaves;
  long nAcquire, nRelease, nSteal, nFail;  /* steal stats */
  long wakeups, falseWakeups, nNodes_last;
  double time[SS_NSTATES], timeLast;         /* perf measurements */
  int entries[SS_NSTATES], curState; 
  LOCK_T * stackLock; /* lock for manipulation of shared portion */
  Node * stack;       /* addr of actual stack of nodes in local addr space */
  SHARED_INDEF Node * stack_g; /* addr of same stack in global addr space */
#ifdef TRACE
  MetaData * md;        /* meta data used for debugging and tracing */
#endif
};
typedef struct stealStack_t StealStack;

typedef SHARED StealStack * SharedStealStackPtr;


/***********************************************************
 *  Global shared state                                    *
 ***********************************************************/

// shared access to each thread's stealStack
SHARED SharedStealStackPtr stealStack[MAX_THREADS];

// termination detection 
VOLATILE SHARED int cb_cancel;
VOLATILE SHARED int cb_count;
VOLATILE SHARED int cb_done;
LOCK_T * cb_lock;

SHARED double startTime[MAX_THREADS];


/***********************************************************
 *  UTS Implementation Hooks                               *
 ***********************************************************/

// Return a string describing this implementation
char * impl_getName() {
  char * name[] = {"Sequential C", "C/OpenMP", "UPC"};
  return name[COMPILER_TYPE];
}


// construct string with all parameter settings 
int impl_paramsToStr(char *strBuf, int ind) {
  ind += sprintf(strBuf+ind, "Execution strategy:  ");
  if (PARALLEL) {
    ind += sprintf(strBuf+ind, "Parallel search using %d threads\n", GET_NUM_THREADS);
    if (doSteal) {
      ind += sprintf(strBuf+ind, "   Load balance by work stealing, chunk size = %d nodes\n",chunkSize);
    }
    else
      ind += sprintf(strBuf+ind, "   No load balancing.\n");
  }
  else
    ind += sprintf(strBuf+ind, "Iterative sequential search\n");
      
  return ind;
}


int impl_parseParam(char *param, char *value) {
  int err = 0;  // Return 0 on a match, nonzero on an error

  switch (param[1]) {
#if (PARALLEL == 1)
    case 'c':
      chunkSize = atoi(value); 
      stealPollInterval = chunkSize; // Heuristic
      break;
    case 's':
      doSteal = atoi(value); 
      if (doSteal != 1 && doSteal != 0) 
	err = 1;
      break;
#endif /* PARALLEL */
    default:
      err = 1;
      break;
  }

  return err;
}

void impl_helpMessage() {
  if (PARALLEL) {
    printf("   -s  int   zero/nonzero to disable/enable work stealing\n");
    printf("   -c  int   chunksize for work stealing\n");
  } else {
    printf("   none.\n");
  }
}

void impl_abort(int err) {
#if defined(__UPC__)
  upc_global_exit(err);
#else
  exit(err);
#endif
}


/***********************************************************
 *                                                         *
 *  FUNCTIONS                                              *
 *                                                         *
 ***********************************************************/

/* 
 * StealStack
 *    Stack of nodes controlled by its owning thread,
 *    which may grant requests from other threads for
 *    work to be stolen.
 *
 *    * workAvail is the count of elements in the shared
 *      portion of the stack.  Idle threads read workAvail
 *      in a speculative fashion to minimize overhead to 
 *      working threads.
 *    * Elements can be stolen from the bottom of the stack
 *      The values are reserved by the victim thread, and
 *      later copied by the theif (currently space for
 *      reserved values is never reclaimed).
 *
 */

/* restore stack to empty state */
void ss_mkEmpty(StealStack *s) {
  SET_LOCK(s->stackLock);
  s->sharedStart = 0;
  s->local  = 0;
  s->top    = 0;
  s->workAvail = 0;
  UNSET_LOCK(s->stackLock);
  
  SET_LOCK(s->reqLock);
  s->req_thread = REQ_UNAVAILABLE;
  UNSET_LOCK(s->reqLock);
}

/* fatal error */
void ss_error(char *str) {
  printf("*** [Thread %i] %s\n",GET_THREAD_NUM, str);
  exit(4);
}

/* initialize the stack */
void ss_init(StealStack *s, int nelts) {
  int nbytes = nelts * sizeof(Node);

  if (debug & 1)
    printf("Thread %d intializing stealStack %p, sizeof(Node) = %X\n", 
           GET_THREAD_NUM, s, (int)(sizeof(Node)));

  // allocate stack in shared addr space with affinity to calling thread
  // and record local addr for efficient access in sequel
  s->stack_g = (SHARED_INDEF Node *) ALLOC (nbytes);
  s->stack = (Node *) s->stack_g;
#ifdef TRACE
  s->md = (MetaData *) ALLOC (sizeof(MetaData));
  if (s->md == NULL)
    ss_error("ss_init: out of memory");
#endif
  if (s->stack == NULL) {
    printf("Request for %d bytes for stealStack on thread %d failed\n",
           nbytes, GET_THREAD_NUM);
    ss_error("ss_init: unable to allocate space for stealstack");
  }
  INIT_LOCK(s->stackLock);
  INIT_LOCK(s->reqLock);
  if (debug & 1)
    printf("Thread %d init stackLock %p\n", GET_THREAD_NUM, (void *) s->stackLock);
  s->stackSize = nelts;
  s->nNodes = 0;
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
void ss_push(StealStack *s, Node *c) {
  if (s->top >= s->stackSize)
    ss_error("ss_push: overflow");
  if (debug & 1)
    printf("ss_push: Thread %d, posn %ld: node %s [%d]\n",
           GET_THREAD_NUM, s->top, rng_showstate(c->state.state, debug_str), c->height);
  memcpy(&(s->stack[s->top]), c, sizeof(Node));
  s->top++;
  s->nNodes++;
  s->maxStackDepth = max(s->top, s->maxStackDepth);
  s->maxTreeDepth = max(s->maxTreeDepth, c->height);
}

/* local top:  get local addr of node at top */ 
Node * ss_top(StealStack *s) {
  Node *r;
  if (s->top <= s->local)
    ss_error("ss_top: empty local stack");
  r = &(s->stack[(s->top) - 1]);
  if (debug & 1) 
    printf("ss_top: Thread %d, posn %ld: node %s [%d] nchild = %d\n",
           GET_THREAD_NUM, s->top - 1, rng_showstate(r->state.state, debug_str),
           r->height, r->numChildren);
  return r;
}

/* local pop */
void ss_pop(StealStack *s) {
  Node *r;
  if (s->top <= s->local)
    ss_error("ss_pop: empty local stack");
  s->top--;
  r = &(s->stack[s->top]);
  if (debug & 1)
    printf("ss_pop: Thread %d, posn %ld: node %s [%d] nchild = %d\n",
           GET_THREAD_NUM, s->top, rng_showstate(r->state.state, debug_str), 
           r->height, r->numChildren);
}
  
/* local top position:  stack index of top element */
int ss_topPosn(StealStack *s)
{
  if (s->top <= s->local)
    ss_error("ss_topPosn: empty local stack");
  return s->top - 1;
}

/* local depth */
int ss_localDepth(StealStack *s) {
  return (s->top - s->local);
}


/* steal k values from shared portion of victim thread's stealStack
 * onto local portion of current thread's stealStack.
 * return false if k vals are not avail in victim thread
 */
int ss_steal(StealStack *s, int victim, int k) {
  long stealIndex;
  int stealAmt;

  if (ss_localDepth(s) > 0)
    ss_error("ss_steal: thief attempts to steal onto non-empty stack");

  if (s->top + k >= s->stackSize)
    ss_error("ss_steal: steal will overflow thief's stack");
  
  // local spin waiting for victim to respond to steal request
  stealIndex = WAITING_FOR_WORK;
  while (stealIndex == WAITING_FOR_WORK) {
#ifdef __BERKELEY_UPC__
    bupc_poll();
#endif
    stealIndex = s->stolen_work_addr;      // <- add bupc_poll here
  }

  if (stealIndex >= 0) {
    // steal granted
    if (debug & 4){
      printf("Thread %d granted steal from thread %d: chunk size %d at offset %ld\n", 
	     GET_THREAD_NUM, victim, k, stealIndex);
    }

    stealAmt = s->stolen_work_amt;      // <- add bupc_poll here?
    
    SMEMCPY(&((s->stack)[s->top]), 
	    &(stealStack[victim]->stack_g)[stealIndex], 
	    stealAmt * sizeof(Node));
    s->nSteal += stealAmt;

    if (debug & 2) {
      int i;
      for (i = 0; i < stealAmt; i ++) {
        Node * r = &(s->stack[s->top + i]);
        printf("ss_steal:  Thread %2d posn %ld (steal #%ld) receives %s [%d] from thread %d location %ld\n",
               GET_THREAD_NUM, s->top + i, s->nSteal,
               rng_showstate(r->state.state, debug_str),
               r->height, victim,  
               stealIndex + i);
      }
    }
    s->top += stealAmt;
    
#ifdef TRACE
    /* update session record of theif */
    s->stealRecords[s->entries[SS_WORK]].victimThread = victim;
#endif

    return 1;
  }

  else {
    // denied request
    s->nFail++;
    if (debug & 4) {
      printf("Thread %d failed (reason %ld) to steal %d nodes from thread %d\n",
	     GET_THREAD_NUM, stealIndex, k, victim);
    }
    return 0;
  }
} 

/* search other threads for work to steal 
 * repeatedly queries other threads until
 *  (a) returns victim thread on which succesfully queued for work  OR
 *  (b) returns NONE_WORKING, could not observe any working threads
 */
int findwork(int k) {
  int i,v;
  int workProbe, workDetected, success;
  int req_thread;

  do {
    workDetected = 0;
    /* check all other threads */
    for (i = 1; i < GET_NUM_THREADS; i++) {
      v = (GET_THREAD_NUM + i) % GET_NUM_THREADS;
      workProbe = stealStack[v]->workAvail;
//printf("work probe at thread %d found work avail = %d at time %f\n", v, workProbe, wctime());

      if (workProbe != NOT_WORKING)
	workDetected = 1;

      if (workProbe >= 2 * k) {
	// try to queue for work
	SET_LOCK(stealStack[v]->reqLock);
//printf("requesting work at thread %d (got lock) at time %f\n", v, wctime());
	req_thread = stealStack[v]->req_thread;
	success = (stealStack[v]->req_thread == REQ_AVAILABLE);
	if (success) {
	  stealStack[v]->req_thread = GET_THREAD_NUM;
	}
	UNSET_LOCK(stealStack[v]->reqLock);

	if (success) {
	  if (debug & 4)
	    printf("Thread %d queued request at thread %d\n", GET_THREAD_NUM, v);
	  return v;
	}
        else
          if (debug & 4)
            printf("Thread %d unable to queue request at thread %d (req_thread == %d)\n", GET_THREAD_NUM, v, req_thread);
      }

    } /*for */

  } while (workDetected);

  return NONE_WORKING;
}


/**
 *  Tracing functions
 *   Track changes in the search state for offline analysis.
**/
void ss_initState(StealStack *s) {
  int i;
  s->timeLast = uts_wctime();
  for (i = 0; i < SS_NSTATES; i++) {
    s->time[i] = 0.0;
    s->entries[i] = 0;
  }
  s->curState = SS_WORK;
  if (debug & 8)
    printf("Thread %d start state %d (t = %f)\n", 
           GET_THREAD_NUM, s->curState, s->timeLast);
}

void ss_setState(StealStack *s, int state){
  double time;
  if (state < 0 || state >= SS_NSTATES)
    ss_error("ss_setState: thread state out of range");
  if (state == s->curState)
    return;
  time = uts_wctime();
  s->time[s->curState] +=  time - s->timeLast;

  #ifdef TRACE  
    /* close out last session record */
    s->md->sessionRecords[s->curState][s->entries[s->curState] - 1].endTime = time;
    if (s->curState == SS_WORK)
    {
       s->md->stealRecords[s->entries[SS_WORK] - 1].nodeCount = s->nNodes
           - s->md->stealRecords[s->entries[SS_WORK] - 1].nodeCount;
    }

    /* initialize new session record */
    s->md->sessionRecords[state][s->entries[state]].startTime = time;
    if (state == SS_WORK)
    {
       s->md->stealRecords[s->entries[SS_WORK]].nodeCount = s->nNodes;
    }
  #endif

  s->entries[state]++;
  s->timeLast = time;
  s->curState = state;

  if(debug & 8)
    printf("Thread %d enter state %d [#%d] (t = %f)\n",
           GET_THREAD_NUM, state, s->entries[state], time);
}

/*
 *	Tree Implementation      
 *
 */
void initNode(Node * child)
{
  child->type = -1;
  child->height = -1;
  child->numChildren = -1;    // not yet determined
}


void initRootNode(Node * root, int type)
{
  uts_initRoot(root, type);

  #ifdef TRACE
    stealStack[0]->md->stealRecords[0].victimThread = 0;  // first session is own "parent session"
  #endif
}

// forward decl
void checkSteal(StealStack *ss);

/* 
 * Generate all children of the parent
 *
 * details depend on tree type, node type and shape function
 *
 */
void genChildren(Node * parent, Node * child, StealStack * ss) {
  int parentHeight = parent->height;
  int numChildren, childType;

  numChildren = uts_numChildren(parent);
  childType   = uts_childType(parent);

  // record number of children in parent
  parent->numChildren = numChildren;
  if (debug & 2) {       
    printf("Gen:  Thread %d, posn %2d: node %s [%d] has %2d children\n",
           GET_THREAD_NUM, ss_topPosn(ss),
           rng_showstate(parent->state.state, debug_str), 
           parentHeight, numChildren);
  }
  
  // construct children and push onto stack
  if (numChildren > 0) {
    int i, j;
    child->type = childType;
    child->height = parentHeight + 1;

    for (i = 0; i < numChildren; i++) {
      for (j = 0; j < computeGranularity; j++) {
        // TBD:  add parent height to spawn
        // computeGranularity controls number of rng_spawn calls per node
        rng_spawn(parent->state.state, child->state.state, i);
      }
      ss_push(ss, child);

      // check for steal requests periodically
      if (ss->nNodes % stealPollInterval == 0)
	checkSteal(ss);
    }
  } else {
    ss->nLeaves++;
  }
}
   
 
/*
 *  Parallel tree traversal
 *
 */

// cancellable barrier

// initialize lock:  single thread under omp, all threads under upc
void cb_init(){
  INIT_SINGLE_LOCK(cb_lock);
  if (debug & 4)
    printf("Thread %d, cb lock at %p\n", GET_THREAD_NUM, (void *) cb_lock);

  // fixme: no need for all upc threads to repeat this
  SET_LOCK(cb_lock);
  cb_count = 0;
  cb_cancel = 0;
  cb_done = 0;
  UNSET_LOCK(cb_lock);
}

#define NO_TERM   0
#define TERM      1

int cbarrier_inc() {
  SET_LOCK(cb_lock);
  cb_count++;
  UNSET_LOCK(cb_lock);
  if (cb_count == GET_NUM_THREADS) {
    return TERM;
  }
  else {
    return NO_TERM;
  }
}

/* cbarrier_dec allows thread to exit barrier wait, but not after
 * barrier has been achieved
 */
int cbarrier_dec() {
  int status = TERM;

  SET_LOCK(cb_lock);
  if (cb_count < GET_NUM_THREADS) {
    status = NO_TERM;
    cb_count--;
  }
  UNSET_LOCK(cb_lock);

  return status;
}

int cbarrier_test() {
  return  (cb_count == GET_NUM_THREADS)? TERM : NO_TERM;
}

void checkSteal(StealStack *ss){
  long d, position;
  int stealAmt;
  int requestor;

  if (doSteal) {
    int d = ss_localDepth(ss);
    if (d > 2 * chunkSize) {
      // enough work to share
      requestor = ss->req_thread;
      if (requestor >= 0) {
        stealAmt = (d / 2 / chunkSize) * chunkSize;

        // make chunk(s) available
        position = ss->local;
        ss->local += stealAmt;
        ss->nRelease++;

        // advertise correct amount of work left locally
        ss->workAvail = d - stealAmt;
#ifdef __BERKELEY_UPC__
        bupc_poll();
#endif
        if (debug & 4){
          printf("Thread %d servicing request from thread %d: releasing chunk size %d at offset %ld\n",
                 GET_THREAD_NUM, ss->req_thread, chunkSize, position);
        }

        // re-open request opportunity
        ss->req_thread = REQ_AVAILABLE;

        // mark thief's stack to show amount of granted steal
        stealStack[requestor]->stolen_work_amt = stealAmt;
        // mark thief's stack to show location of granted steal
        stealStack[requestor]->stolen_work_addr = position;
#ifdef __BERKELEY_UPC__
        bupc_poll();
#endif

        return;
      }
    }
    // no thread waiting or insufficient work - advertise current workAvail
    ss->workAvail = d;
#ifdef __BERKELEY_UPC__
    bupc_poll();
#endif
  }

  return;
}

/* DFS of nodes on local stack
 * poll for steal request every stealPollInterval node generations
 * return when stack is empty
 */
void dfsWithPolling(StealStack *ss) {
  Node * parent;
  Node child;

  /* template for children */
  initNode(&child);

  /* dfs */
  while (ss_localDepth(ss) > 0) {

    /* examine node at stack top */
    parent = ss_top(ss);
    if (parent->numChildren < 0){
      // first time visited, construct children and place on stack
      genChildren(parent,&child,ss);

    }
    else {
      // second time visit, pop
      ss_pop(ss);
    }
  }

  /* local stack empty */
  return;
}

/*
 * true if neighboring thread to the right is working
 */
int detectWork() {
  int neighbor = (GET_THREAD_NUM + 1) % GET_NUM_THREADS;
  return (NOT_WORKING != stealStack[neighbor]->workAvail);
}

/* 
 * parallel search of UTS trees using work stealing 
 * 
 *   Note: tree size is measured by the number of
 *         push operations
 */
void parTreeSearch(StealStack *ss) {
  int victim, pendingReq, success;
  int status = NO_TERM;
  
  while (status != TERM) {

    switch (ss->curState) {

    case SS_WORK: 
      // set steal availability
      ss->workAvail = 0;
      ss->req_thread = REQ_AVAILABLE;

      // local dfs of tree nodes until exhausted
      dfsWithPolling(ss);

      if (doSteal) {
	// show this thread as not working
	SET_LOCK(ss->reqLock);
	ss->workAvail = NOT_WORKING;
	pendingReq = ss->req_thread;
	ss->req_thread = REQ_UNAVAILABLE;
	UNSET_LOCK(ss->reqLock);

	// fail any pending steal request
	if (pendingReq >= 0)
	  stealStack[pendingReq]->stolen_work_addr = NONE_STOLEN;

	// go to SEARCH state
	ss_setState(ss, SS_SEARCH);
      }
      else {
	// no load balancing, exit to barrier
	ss_setState(ss, SS_IDLE);
	status = TERM;
      }
      break;

    case SS_SEARCH:
      // reset notification field for steal requests
      SET_LOCK(ss->stackLock);
      ss->stolen_work_addr = WAITING_FOR_WORK;
      UNSET_LOCK(ss->stackLock);

      // find work
      victim = findwork(chunkSize);
      if (victim >= 0) 
	ss_setState(ss, SS_WAIT);
      else 
	ss_setState(ss, SS_IDLE);
      break;

    case SS_WAIT:
      // wait for notification from victim
      success = ss_steal(ss, victim, chunkSize);
      if (success)
	ss_setState(ss, SS_WORK);
      else
	ss_setState(ss, SS_SEARCH);
      break;

    case SS_IDLE:
      // termination detection
      status = cbarrier_inc();
      while (status != TERM) {
	    if (detectWork() && cbarrier_dec() != TERM) {
	      status = NO_TERM;
	      ss_setState(ss, SS_SEARCH);
	      break;
	    }
	    status = cbarrier_test();
      }
      break;

    default:
      ss_error("Thread in unknown state");
      status = TERM;
    }
  }
  /* tree search complete ! */
}

#ifdef TRACE
// print session records for each thread (used when trace is enabled)
void printSessionRecords()
{
  int i, j, k;
  double offset;

  for (i = 0; i < GET_NUM_THREADS; i++) {
    offset = startTime[i] - startTime[0];

    for (j = 0; j < SS_NSTATES; j++)
       for (k = 0; k < stealStack[i]->entries[j]; k++) {
          printf ("%d %d %f %f", i, j,
            stealStack[i]->md->sessionRecords[j][k].startTime - offset,
            stealStack[i]->md->sessionRecords[j][k].endTime - offset);
          if (j == SS_WORK)
            printf (" %d %ld",
              stealStack[i]->md->stealRecords[k].victimThread,
              stealStack[i]->md->stealRecords[k].nodeCount);
            printf ("\n");
     }
  }
}
#endif

// display search statistics
void showStats(double elapsedSecs) {
  int i;
  long tnodes = 0, tleaves = 0, trel = 0, tacq = 0, tsteal = 0, tfail= 0;
  long mdepth = 0, mheight = 0;
  double twork = 0.0, tsearch = 0.0, tidle = 0.0, tovh = 0.0, tcbovh = 0.0;

  // combine measurements from all threads
  for (i = 0; i < GET_NUM_THREADS; i++) {
    tnodes  += stealStack[i]->nNodes;
    tleaves += stealStack[i]->nLeaves;
    trel    += stealStack[i]->nRelease;
    tacq    += stealStack[i]->nAcquire;
    tsteal  += stealStack[i]->nSteal;
    tfail   += stealStack[i]->nFail;
    twork   += stealStack[i]->time[SS_WORK];
    tsearch += stealStack[i]->time[SS_SEARCH];
    tidle   += stealStack[i]->time[SS_IDLE];
    tovh    += stealStack[i]->time[SS_OVH];
    mdepth   = max(mdepth, stealStack[i]->maxStackDepth);
    mheight  = max(mheight, stealStack[i]->maxTreeDepth);
  }
    
  uts_showStats(GET_NUM_THREADS, chunkSize, elapsedSecs, tnodes, tleaves, mheight);

  if (verbose > 1) {
    if (doSteal) {
      printf("Failed steal operations = %ld, ", tfail);
    }

    printf("Max stealStack size = %ld\n", mdepth);
    printf("Avg time per thread: Work = %.6f, Search = %.6f, Idle = %.6f\n", (twork / GET_NUM_THREADS),
        (tsearch / GET_NUM_THREADS), (tidle / GET_NUM_THREADS));
    printf("                     Overhead = %6f\n\n", (tovh / GET_NUM_THREADS));
  }

  // per thread execution info
  if (verbose > 2) {
    for (i = 0; i < GET_NUM_THREADS; i++) {
      printf("** Thread %d\n", i);
      printf("  # nodes explored    = %ld\n", stealStack[i]->nNodes);
      printf("  # completed steals  = %ld\n", stealStack[i]->nSteal);
      printf("  # failed steals     = %ld\n", stealStack[i]->nFail);
      printf("  maximum stack depth = %ld\n", stealStack[i]->maxStackDepth);
      printf("  work time           = %.6f secs (%d sessions)\n",
             stealStack[i]->time[SS_WORK], stealStack[i]->entries[SS_WORK]);
      printf("  overhead time       = %.6f secs (%d sessions)\n",
             stealStack[i]->time[SS_OVH], stealStack[i]->entries[SS_OVH]);
      printf("  search time         = %.6f secs (%d sessions)\n",
             stealStack[i]->time[SS_SEARCH], stealStack[i]->entries[SS_SEARCH]);
      printf("  idle time           = %.6f secs (%d sessions)\n",
             stealStack[i]->time[SS_IDLE], stealStack[i]->entries[SS_IDLE]);
      printf("  wakeups             = %ld, false wakeups = %ld (%.2f%%)",
             stealStack[i]->wakeups, stealStack[i]->falseWakeups,
             (stealStack[i]->wakeups == 0) ? 0.00 : ((((double)stealStack[i]->falseWakeups)/stealStack[i]->wakeups)*100.0));
      printf("\n");
    }
  }

  #ifdef TRACE
    printSessionRecords();
  #endif
}


/*  Main() function for: Sequential, UPC
 *
 *  Notes on execution model:
 *     - under UPC, global vars are private unless explicitly shared
 *     - UPC is SPMD starting with main
 */
int main(int argc, char *argv[]) {
  Node root;

  /* determine benchmark parameters (all PEs) */
  uts_parseParams(argc, argv);

  /* cancellable barrier initialization */
  cb_init();

    double t1, t2, et;
    StealStack * ss;    

    /* show parameter settings */
    if (GET_THREAD_NUM == 0) {
      uts_printParams();
    }

    /* initialize stealstacks */
    stealStack[GET_THREAD_NUM] = (SHARED StealStack *) ALLOC (sizeof(StealStack));
    ss = (StealStack *) stealStack[GET_THREAD_NUM];	
    ss_init(ss, MAXSTACKDEPTH);
    
    /* initialize root node and push on thread 0 stack */
    if (GET_THREAD_NUM == 0) {
      initRootNode(&root, type);
      ss_push(ss, &root);
    }

    // line up for the start
    BARRIER
    
    /* time parallel search */
    ss_initState(ss);
    t1 = uts_wctime();
    parTreeSearch(ss);
    t2 = uts_wctime();
    et = t2 - t1;

#ifdef TRACE
    startTime[GET_THREAD_NUM] = t1;
    ss->md->sessionRecords[SS_IDLE][ss->entries[SS_IDLE] - 1].endTime = t2;
#endif

    BARRIER

    /* display results */
    if (GET_THREAD_NUM == 0) {
      showStats(et);
    } 

  return 0;
}
