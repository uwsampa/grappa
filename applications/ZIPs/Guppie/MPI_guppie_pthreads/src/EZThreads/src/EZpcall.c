/* CVS info   */
/* $Date: 2005/06/07 18:58:35 $     */
/* $Revision: 1.1 $ */
/* $RCSfile: EZpcall.c,v $  */
/* $Name:  $     */
/*
  EZpcall.c
  Date:       12/28/98
  
*/

#include "EZThreads.h"

/* simple N-way parallel call, these might be nestable in the future */
int EZpcall(void * (*function)(void *),
	    void *fnArgs,
	    int nThreads)
{
    int i;
    int status;
    
    static char cvs_info[] = "BMARKGRP $Date: 2005/06/07 18:58:35 $ $Revision: 1.1 $ $RCSfile: EZpcall.c,v $ $Name:  $";


    WorkerArg_t *aWorkerArg;
    void *pWorkerRet;
    pthread_t *aThreads;
    pthread_mutex_t sharedLock = PTHREAD_MUTEX_INITIALIZER;
    AtomicCnt_t *sharedCounter;
    Bar_t *sharedBarrier;
    
    aWorkerArg = (WorkerArg_t *) malloc(sizeof(WorkerArg_t) * nThreads);
    if(aWorkerArg == NULL)
    {
	perror("EZpcall:malloc aWorkerArg:");
	fflush(stderr);
	exit(1);
    }

    aThreads = (pthread_t *) malloc(sizeof(pthread_t) * nThreads);
    if(aThreads == NULL)
    {
	perror("EZpcall:malloc aThreads:");
	fflush(stderr);
	exit(1);
    }

    /* allocate the shared atomic counter */
    EZallocAtomicCnt(&sharedCounter);
    
    /* allocate the shared barrier */
    EZallocBar(&sharedBarrier, nThreads);

    /* create threads */
    for(i = 0; i < nThreads; i++)
    {
	aWorkerArg[i].ThreadID = i;
	aWorkerArg[i].NThreads = nThreads;
	aWorkerArg[i].FnArgs = fnArgs;
	aWorkerArg[i].Lock = &sharedLock;
	aWorkerArg[i].Counter = sharedCounter;
	aWorkerArg[i].Barrier = sharedBarrier;
	
	status = pthread_create(&(aThreads[i]),
				NULL,
				function,
				(void *) &(aWorkerArg[i]));
	EZerror(status,"EZpcall:pthread_create:");
    }

    /* wait for thread completion */
    for(i = 0; i < nThreads; i++)
    {
	status = pthread_join(aThreads[i], (void **) &(pWorkerRet));
	EZerror(status, "main:pthread_join:");
    }

    /* deallocate the shared atomic counter */
    EZdestroyAtomicCnt(sharedCounter);

    /* deallocate the shared barrier */
    EZdestroyBar(sharedBarrier);

    free(aWorkerArg);
    free(aThreads);
    
    return(0);
}

