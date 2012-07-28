/* CVS info   */
/* $Date: 2005/06/07 18:58:35 $     */
/* $Revision: 1.1 $ */
/* $RCSfile: EZbarrier.c,v $  */
/* $Name:  $     */
/*
  EZbarrier.c
  Date:       12/29/98
  
*/

#include "EZThreads.h"

int EZwaitBar(Bar_t *bar)
{
    int status;
    int release = FALSE;
    
    static char cvs_info[] = "BMARKGRP $Date: 2005/06/07 18:58:35 $ $Revision: 1.1 $ $RCSfile: EZbarrier.c,v $ $Name:  $";


    status = pthread_mutex_lock(&(bar->Lock));
    EZerror(status, "EZwaitBar:pthread_mutex_lock:");

    /* if previous call still releasing */
    while(bar->Releasing)
    {
	pthread_cond_wait(&(bar->Cond), &(bar->Lock));
	EZerror(status, "EZwaitBar:pthread_cond_wait:");
    }
    
    bar->NSleepers++;
    if(bar->NSleepers == bar->Count)
    {
	release = TRUE;
	bar->Releasing = TRUE;
    }
    else
    {
	while(!bar->Releasing)
	{
	    pthread_cond_wait(&(bar->Cond), &(bar->Lock));
	    EZerror(status, "EZwaitBar:pthread_cond_wait:");
	}
    }

    bar->NSleepers--;
    if(bar->NSleepers == 0)
    {
	bar->Releasing = FALSE;
	release = TRUE;
    }

    status = pthread_mutex_unlock(&(bar->Lock));
    EZerror(status, "EZwaitBar:pthread_mutex_unlock:");

    if(release)
    {
	status = pthread_cond_broadcast(&(bar->Cond));
	EZerror(status, "EZwaitBar:pthread_cond_broadcast:");
    }

    return(0);
}

int EZallocBar(Bar_t **bar, int nThreads)
{
    Bar_t *ret;
    int status;
    
    ret = (Bar_t *) malloc(sizeof(Bar_t));
    if(ret == NULL)
    {
	perror("EZallocBar:malloc:");
	fflush(stderr);
	exit(1);
    }

    ret->NSleepers = 0;
    ret->Count = nThreads;
    ret->Releasing = FALSE;
    status = pthread_mutex_init(&(ret->Lock), NULL);
    EZerror(status, "EZallocBar:pthread_mutex_init:");
    status = pthread_cond_init(&(ret->Cond), NULL);
    EZerror(status, "EZallocBar:pthread_cond_init:");

    *bar = ret;
    
    return(0);
}

int EZdestroyBar(Bar_t *bar)
{
    int status;
    
    status = pthread_mutex_destroy(&(bar->Lock));
    EZerror(status, "EZdestroyBar:pthread_mutex_destroy:");
    status = pthread_cond_destroy(&(bar->Cond));
    EZerror(status, "EZdestroyBar:pthread_cond_destroy:");
    free(bar);
    
    return(0);
}

