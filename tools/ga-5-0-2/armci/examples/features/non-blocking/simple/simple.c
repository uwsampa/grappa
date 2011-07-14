#if HAVE_CONFIG_H
#   include "config.h"
#endif

/* $Id$ */

#if HAVE_STDIO_H
#   include <stdio.h>
#endif
#if HAVE_STDLIB_H
#   include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#   include <unistd.h>
#endif

#include "mp3.h"
#include "armci.h"

int me,nprocs;
int LOOP=10;
int main(int argc, char **argv)
{
int i;
double **myptrs;
double t0,t1,tnbget=0,tnbwait=0,t2=0;
    MP_INIT(argc,argv);
    MP_PROCS(&nprocs);
    if (nprocs==1)
    {
        fprintf(stderr,"You must use more than 1 process for this test.  Exiting gently.");
        return 0;
    }
    MP_MYID(&me);
    myptrs = (double **)malloc(sizeof(double *)*nprocs);
    ARMCI_Init();
    ARMCI_Malloc((void **)myptrs,LOOP*sizeof(double)); 
    MP_BARRIER();
    if(me==0){
       for(i=0;i<10;i++){
         ARMCI_Get(myptrs[me]+i,myptrs[me+1]+i,sizeof(double),me+1);
       }
       t0 = MP_TIMER(); 
       for(i=0;i<LOOP;i++){
         ARMCI_Get(myptrs[me]+i,myptrs[me+1]+i,sizeof(double),me+1);
       }
       t1 = MP_TIMER(); 
       printf("\nGet Latency=%lf\n",1e6*(t1-t0)/LOOP);fflush(stdout);
       t1=t0=0;
       for(i=0;i<LOOP;i++){
         armci_hdl_t nbh;
         ARMCI_INIT_HANDLE(&nbh);
         t0 = MP_TIMER(); 
         ARMCI_NbGet(myptrs[me]+i,myptrs[me+1]+i,sizeof(double),me+1,&nbh);
         t1 = MP_TIMER(); 
         ARMCI_Wait(&nbh);
         t2 = MP_TIMER();
         tnbget+=(t1-t0);
         tnbwait+=(t2-t1);
       }
       printf("\nNb Get Latency=%lf Nb Wait=%lf\n",1e6*tnbget/LOOP,1e6*tnbwait/LOOP);fflush(stdout);
    }
    else
      sleep(1);
    MP_BARRIER();
    ARMCI_Finalize();
    MP_FINALIZE();
    return 0;
}
