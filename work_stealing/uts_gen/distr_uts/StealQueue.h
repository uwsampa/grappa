
#ifdef __cplusplus
extern "C" {
#endif

#ifndef STEAL_QUEUE_H
#define STEAL_QUEUE_H

#include "uts.h"
#include "SoftXMT.hpp"

#define SHARED_INDEF /*nothing*/

#define GASNET_LOCKS 1
#define OPENMP_LOCKS 0

/** OpenMP defs **/
#if OPENMP_LOCKS
#include <omp.h>
#define LOCK_T           omp_lock_t
#define SET_LOCK(zlk)    omp_set_lock(zlk)
#define UNSET_LOCK(zlk)  omp_unset_lock(zlk)
#define INIT_LOCK(zlk)   zlk=omp_global_lock_alloc()
#define INIT_SINGLE_LOCK(zlk) zlk=omp_global_lock_alloc()
omp_lock_t * omp_global_lock_alloc();

#elif GASNET_LOCKS
    #include <gasnet.h>
    #define INIT_LOCK gasnet_hsl_init
    #define SET_LOCK gasnet_hsl_lock
    #define UNSET_LOCK gasnet_hsl_unlock
    #define LOCK_T gasnet_hsl_t
    #define LOCK_INITIALIZER GASNET_HSL_INITIALIZER
    int SET_COND(LOCK_T*);
    int UNSET_COND(LOCK_T*);
#endif


void ss_pushRemote(Node destnode, Node_ptr* work, int k);


//unused
#define SS_NSTATES 1   

int maxint(int x, int y); 

/* stack of nodes */
struct stealStack_t
{
  int stackSize;     /* total space avail (in number of elements) */
  int workAvail;     /* elements available for stealing */
  int sharedStart;   /* index of start of shared portion of stack */
  int local;         /* index of start of local portion */
  int top;           /* index of stack top */
  int maxStackDepth;                      /* stack stats */ 
  int nNodes, maxTreeDepth, nVisited;        /* tree stats: (num pushed, max depth, num popped)  */
  int nLeaves;
  int nAcquire, nRelease, nSteal, nFail;  /* steal stats */
  int wakeups, falseWakeups, nNodes_last;
  double time[SS_NSTATES], timeLast;
  /* perf measurements */ 
  int entries[SS_NSTATES], curState; 
  LOCK_T* stackLock; 
  LOCK_T* stackLock_g;
  /* lock for manipulation of shared portion */ 
  Node_ptr* stack;       /* addr of actual
                               stack of nodes
                               in local addr
                               space */
  Node_ptr* stack_g; /* addr of same
                             stack in global
                             addr space */

}; 
typedef struct stealStack_t StealStack;



void ss_mkEmpty(StealStack *s); 
void ss_error(char *str); 
void ss_init(StealStack *s, int nelts); 
void ss_push(StealStack *s, Node_ptr c); 
Node_ptr ss_top(StealStack *s); 
void ss_pop(StealStack *s); 
int ss_topPosn(StealStack *s);
int ss_localDepth(StealStack *s); 
void ss_release(StealStack *s, int k); 
int ss_acquire(StealStack *s, int k); 
int ss_steal_locally(StealStack* thief_id, int victim, int chunkSize); 
int ss_remote_steal(StealStack *s, int thiefCore, int victimNode, int k); 
void ss_setState(StealStack *s, int state);

#define MAX_NUM_THREADS 12 
#define MAXSTACKDEPTH 500000 
extern StealStack myStealStack;




#endif


#ifdef __cplusplus 
} 
#endif
