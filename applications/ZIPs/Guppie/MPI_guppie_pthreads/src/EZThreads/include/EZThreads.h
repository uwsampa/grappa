/* CVS info   */
/* $Date: 2005/06/07 18:58:11 $     */
/* $Revision: 1.1 $ */
/* $RCSfile: EZThreads.h,v $  */
/* $Name:  $     */

/*
  EZThreads.h include file
  Date:       12/29/98
  
*/

/* check to see if EZThreads.h already included */
#ifndef _EZTHREADS_H
#define _EZTHREADS_H 1

/* define some stuff */
#define _POSIX_C_SOURCE 199506L
#define _REENTRANT 1

/* include some stuff */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* define some more stuff */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* typedef some stuff */

typedef struct atomic_counter_s
{
    int Counter;
    pthread_mutex_t Lock;
}
AtomicCnt_t;


typedef struct barrier_s
{
    int Count;
    int NSleepers;
    int Releasing;
    pthread_cond_t Cond;
    pthread_mutex_t Lock;
}
Bar_t;


typedef struct worker_arg_s
{
    int ThreadID;
    int NThreads;
    void *FnArgs;
    pthread_mutex_t *Lock;
    AtomicCnt_t *Counter;
    Bar_t *Barrier;
}
WorkerArg_t;

#define EZerror(status,str) {if((status) != 0) {perror(str); fflush(stderr); exit(1);}}

/* function prototypes */

/* simple N-way parallel call, these might be nestable in the future */
int EZpcall(void * (*function)(void *), void *fnArgs, int nThreads);

/* accessor functions for WorkerArg_t stuff */
int EZnThreads(WorkerArg_t *arg);
int EZthreadID(WorkerArg_t *arg);
void *EZfnArg(WorkerArg_t *arg);
int EZlockSL(WorkerArg_t *arg);
int EZunlockSL(WorkerArg_t *arg);
int EZfetchIncSC(WorkerArg_t *arg);
int EZfetchAddSC(WorkerArg_t *arg, int addend);
int EZsetAtomicCntSC(WorkerArg_t *arg, int value);
int EZwaitBarSB(WorkerArg_t *arg);

/* simple barrier */
int EZallocBar(Bar_t **bar, int nThreads);
int EZwaitBar(Bar_t *bar);
int EZdestroyBar(Bar_t *bar);

/* simple atomic counter */
int EZfetchInc(AtomicCnt_t *cnt);
int EZfetchAdd(AtomicCnt_t *cnt, int addend);
int EZallocAtomicCnt(AtomicCnt_t **cnt);
int EZsetAtomicCnt(AtomicCnt_t *cnt, int value);
int EZdestroyAtomicCnt(AtomicCnt_t *cnt);

/* endif EZThreads.h */
#endif







