#if HAVE_CONFIG_H
#   include "config.h"
#endif

/* $Id: fence.c,v 1.25.4.6 2007-08-30 19:17:02 manoj Exp $ */
#include "armcip.h"
#include "armci.h"
#include "copy.h"
#include <stdio.h>
#if defined(PVM)
#   include <pvm3.h>
#elif defined(TCGMSG)
#   include <sndrcv.h>
#elif defined(BGML)
#   include "bgml.h"
#else
#   include <mpi.h>
#endif

char *_armci_fence_arr;

#ifdef GA_USE_VAMPIR
#include "armci_vampir.h"
#endif
#ifdef ARMCI_PROFILE
#include "armci_profile.h"
#endif
void armci_init_fence()
{
     _armci_fence_arr=calloc(armci_nproc,1);
     if(!_armci_fence_arr)
         armci_die("armci_init_fence: calloc failed",0);
}

void ARMCI_DoFence(int proc)
{
int i;
  if(!SAMECLUSNODE(proc) && (armci_nclus >1)){
  int cluster = armci_clus_id(proc);
    armci_rem_ack(cluster);
  }
}

void ARMCI_Fence(int proc)
{
int i;

#ifdef ARMCI_PROFILE
 if (!SAMECLUSNODE(proc))
 armci_profile_start(ARMCI_PROF_FENCE);
#endif
#if defined(DATA_SERVER) && !(defined(GM) && defined(ACK_FENCE))
//   printf("%d [cp] fence_arr(%d)=%d\n",armci_me,proc,FENCE_ARR(proc));
     if(FENCE_ARR(proc) && (armci_nclus >1)){

           int cluster = armci_clus_id(proc);
           int master=armci_clus_info[cluster].master;

           armci_rem_ack(cluster);

           /* one ack per cluster node suffices */
           /* note, in multi-threaded case it will only clear for current thread */
           bzero(&FENCE_ARR(master),armci_clus_info[cluster].nslave);
     }
#elif defined(BGML)
     BGML_WaitProc(proc);
     MEM_FENCE;
#else
     FENCE_NODE(proc);
     MEM_FENCE;
#endif
#ifdef ARMCI_PROFILE
 if (!SAMECLUSNODE(proc))
 armci_profile_stop(ARMCI_PROF_FENCE);
#endif
}


/*
    portals developers' note:
    armci fence is not guaranteed to be correct unless PUT_START events are captured
    PUT_ENDs do NOT guarantee order; only PUT_STARTs
*/
void ARMCI_AllFence()
{
#ifdef ARMCI_PROFILE
     armci_profile_start(ARMCI_PROF_ALLFENCE);
#endif
#if defined(CLUSTER)
     { int p; for(p=0;p<armci_nproc;p++)ARMCI_Fence(p); }
#endif
#ifdef ARMCI_PROFILE
     armci_profile_stop(ARMCI_PROF_ALLFENCE);
#endif
     MEM_FENCE;
}

void ARMCI_Barrier()
{
    if(armci_nproc==1)return;
#ifdef ARMCI_PROFILE
    armci_profile_start(ARMCI_PROF_BARRIER);
#endif
    ARMCI_AllFence();
#  ifdef MPI
    MPI_Barrier(MPI_COMM_WORLD);
#  else
    {
       long type=ARMCI_TAG;
       SYNCH_(&type);
    }
#  endif
    MEM_FENCE;
#ifdef ARMCI_PROFILE
    armci_profile_stop(ARMCI_PROF_BARRIER);
#endif

}
