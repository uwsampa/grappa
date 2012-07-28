
/* CVS info   */
/* $Date: 2005/06/07 18:58:35 $     */
/* $Revision: 1.1 $ */
/* $RCSfile: EZmisc.c,v $  */
/* $Name:  $     */

/*
  EZpcall.c
  Date:       12/29/98
  
*/

#include "EZThreads.h"


int EZnThreads(WorkerArg_t *arg)
{
    return(arg->NThreads);
}

int EZthreadID(WorkerArg_t *arg)
{
    return(arg->ThreadID);
}

void *EZfnArg(WorkerArg_t *arg)
{
    return(arg->FnArgs);
}


int EZlockSL(WorkerArg_t *arg)
{
    int status;
    
    static char cvs_info[] = "BMARKGRP $Date: 2005/06/07 18:58:35 $ $Revision: 1.1 $ $RCSfile: EZmisc.c,v $ $Name:  $";

    status = pthread_mutex_lock(arg->Lock);
    EZerror(status, "EZlockSL:pthread_mutex_lock:");
    return(0);
}

int EZunlockSL(WorkerArg_t *arg)
{
    int status;
    status = pthread_mutex_unlock(arg->Lock);
    EZerror(status, "EZunlockSL:pthread_mutex_unlock:");
    return(0);
}

int EZfetchIncSC(WorkerArg_t *arg)
{
    return(EZfetchInc(arg->Counter));
}

int EZfetchAddSC(WorkerArg_t *arg, int addend)
{
    return(EZfetchAdd(arg->Counter, addend));
}

int EZsetAtomicCntSC(WorkerArg_t *arg, int value)
{
    return(EZsetAtomicCnt(arg->Counter, value));
}

int EZwaitBarSB(WorkerArg_t *arg)
{
    return(EZwaitBar(arg->Barrier));
}
