#if HAVE_CONFIG_H
#   include "config.h"
#endif

/* $Id: gpctest.c,v 1.2.4.1 2007-06-13 00:44:01 vinod Exp $ */
#include <stdio.h>
#include <stdlib.h>

/*#define RMW*/
#  include <unistd.h>

/* ARMCI is impartial to message-passing libs - we handle them with MP macros */
#ifdef TCGMSG
#   include <sndrcv.h>
    long tcg_tag =30000;
#   define MP_BARRIER()      SYNCH_(&tcg_tag)
#   define MP_INIT(arc,argv) PBEGIN_((argc),(argv))
#   define MP_FINALIZE()     PEND_()
#   define MP_MYID(pid)      *(pid)   = (int)NODEID_()
#   define MP_PROCS(pproc)   *(pproc) = (int)NNODES_()
#else
#   include <mpi.h>
#   define MP_BARRIER()      MPI_Barrier(MPI_COMM_WORLD)
#   define MP_FINALIZE()     MPI_Finalize()
#   define MP_INIT(arc,argv) MPI_Init(&(argc),&(argv))
#   define MP_MYID(pid)      MPI_Comm_rank(MPI_COMM_WORLD, (pid))
#   define MP_PROCS(pproc)   MPI_Comm_size(MPI_COMM_WORLD, (pproc));
#endif

#include "armci.h"
#include "gpc.h"


#define MAXPROC 128
# define ELEMS 200
#define LOOP 100


/***************************** global data *******************/
int me, nproc;
void* work[MAXPROC]; /* work array for propagating addresses */

int hswap=0;
void gpc_swap(int *loc, int *rem, int p)
{
gpc_hdl_t nbh;
char rheader[100];
int hlen, rhlen, rhsize;
int rdsize;
void *header=rem;
extern int hswap;

    hlen=sizeof(header);
    bzero(rheader,100);
    rhlen = hlen;

    ARMCI_Gpc_init_handle(&nbh);

    if(ARMCI_Gpc_exec(hswap,p, &header, hlen, loc, sizeof(int), rheader, rhlen,
                      loc, sizeof(int), NULL/*&nbh*/))
       fprintf(stderr,"ARMCI_Gpc_exec failed\n");

    /*ARMCI_Gpc_wait(&nbh);*/
}


int gpc_swap_handler(int to, int from, void *hdr,   int hlen,
		      void *data,  int dlen,
		      void *rhdr,  int rhlen, int *rhsize,
		      void *rdata, int rdlen, int *rdsize,
		      int rtype)
{
int *rem;
int tmp_swap;

#ifdef DEBUG_
     printf("executing swap handler from=%d to=%d\n"); fflush(stdout);
#endif
     
     rem = (int*)ARMCI_Gpc_translate(*(void**)hdr,to,from);
     ARMCI_Gpc_lock(to);
     tmp_swap = *rem;
     *rem = *(int*)data;
     ARMCI_Gpc_unlock(to);
     *(int*)rdata = tmp_swap;
     *(int*)rhdr  = tmp_swap; /* 2nd copy just for debug purposes */
     *rhsize = sizeof(void *);
     *rdsize = sizeof(int);

     return GPC_DONE;
}


#define LOCKED -1
void test_swap()
{
    int rc, bytes, i, val, whatever=-8999;
    int *arr[MAXPROC];

    /* shared variable is located on processor 0 */
    bytes = me == 0 ? sizeof(int) : 0;


    rc = ARMCI_Malloc((void**)arr,bytes);
    if(rc!=0)
      ARMCI_Error("test_swap: ARMCI_Malloc failed", 0);
    MP_BARRIER();

    hswap = ARMCI_Gpc_register(gpc_swap_handler);

    if(me == 0) *arr[0] = 0;  /* initialization */

    MP_BARRIER();
    for(i = 0; i< LOOP; i++){
      val = LOCKED;
      do{

#ifdef RMW
        rc = ARMCI_Rmw(ARMCI_SWAP, &val, arr[0], whatever, 0);
        if(rc != 0)
	  ARMCI_Error("test_swap: ARMCI_Rmw failed", 0);
#else
        gpc_swap(&val, arr[0], 0);
#endif

      }while (val == LOCKED); 
      val++;

#ifdef RMW
      rc = ARMCI_Rmw(ARMCI_SWAP, &val, arr[0], whatever, 0);
      if(rc != 0)
        ARMCI_Error("test_swap: ARMCI_Malloc failed", 0);
	    
#else
      gpc_swap(&val, arr[0], 0);
#endif

    }

    ARMCI_AllFence();
    MP_BARRIER();

    if(me == 0){
       printf("The final value is %d, should be %d.\n\n",*arr[0],LOOP*nproc); 
       fflush(stdout);
       if( *arr[0] != LOOP*nproc) ARMCI_Error("failed ...",*arr[0]);
    }

    ARMCI_Free(arr[me]);
}




/* we need to rename main if linking with frt compiler */
#ifdef FUJITSU_FRT
#define main MAIN__
#endif

int main(int argc, char* argv[])
{
    int ndim;

    MP_INIT(argc, argv);
    MP_PROCS(&nproc);
    MP_MYID(&me);

    
    if(nproc>MAXPROC && me==0)
       ARMCI_Error("Test works for up to %d processors\n",MAXPROC);

    if(me==0){
      printf("ARMCI Global Procedure Call test program (%d processes)\n",nproc); 
      fflush(stdout);
      sleep(1);
    }
    
    ARMCI_Init();


        if(me==0){
#ifdef RMW
           printf("\nTesting atomic swap using ARMCI_Rmw\n");
#else
           printf("\nTesting atomic swap using GPC\n");
#endif
           fflush(stdout);
        }

    MP_BARRIER();

        test_swap();
        ARMCI_AllFence();
        MP_BARRIER();


    ARMCI_Finalize();
    MP_FINALIZE();
    return(0);
}
