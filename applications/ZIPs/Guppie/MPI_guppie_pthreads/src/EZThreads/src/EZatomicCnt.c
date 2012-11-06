/* CVS info   */
/* $Date: 2005/06/07 18:58:35 $     */
/* $Revision: 1.1 $ */
/* $RCSfile: EZatomicCnt.c,v $  */
/* $Name:  $     */
/*
  EZatomicCnt.c
  Date:       12/29/98
  
*/

#include "EZThreads.h"

int EZfetchInc(AtomicCnt_t *cnt)
{
    int prevValue;
    int status;
    
    static char cvs_info[] = "BMARKGRP $Date: 2005/06/07 18:58:35 $ $Revision: 1.1 $ $RCSfile: EZatomicCnt.c,v $ $Name:  $";


    status = pthread_mutex_lock(&(cnt->Lock));
    EZerror(status, "EZfetchInc:pthread_mutex_lock:");

    prevValue = cnt->Counter;
    cnt->Counter++;

    status = pthread_mutex_unlock(&(cnt->Lock));
    EZerror(status, "EZfetchInc:pthread_mutex_unlock:");

    return(prevValue);
}

int EZfetchAdd(AtomicCnt_t *cnt, int addend)
{
    int prevValue;
    int status;

    status = pthread_mutex_lock(&(cnt->Lock));
    EZerror(status, "EZfetchInc:pthread_mutex_lock:");

    prevValue = cnt->Counter;
    cnt->Counter += addend;

    status = pthread_mutex_unlock(&(cnt->Lock));
    EZerror(status, "EZfetchInc:pthread_mutex_unlock:");

    return(prevValue);
}


int EZallocAtomicCnt(AtomicCnt_t **cnt)
{
    AtomicCnt_t *ret;
    int status;
    
    ret = (AtomicCnt_t *) malloc(sizeof(AtomicCnt_t));
    if(ret == NULL)
    {
	perror("EZallocAtomicCnt:malloc:");
	fflush(stderr);
	exit(1);
    }

    ret->Counter = 0;
    status = pthread_mutex_init(&(ret->Lock), NULL);
    EZerror(status, "EZallocAtomicCnt:pthread_mutex_init:");

    *cnt = ret;

    return(0);
}

int EZsetAtomicCnt(AtomicCnt_t *cnt, int value)
{
    int oldValue = cnt->Counter;
    
    cnt->Counter = value;
    return(oldValue);
}

int EZdestroyAtomicCnt(AtomicCnt_t *cnt)
{
    pthread_mutex_destroy(&(cnt->Lock));
    free(cnt);
    return(0);
}

