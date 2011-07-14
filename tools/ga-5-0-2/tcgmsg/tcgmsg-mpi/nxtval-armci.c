#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <mpi.h>

#if HAVE_MALLOC_H
#   include <malloc.h>
#endif

#include "armci.h"
#include "tcgmsgP.h"
#ifdef USE_VAMPIR
#   include "tcgmsg_vampir.h"
#endif

#define LEN 2
static Integer pnxtval_counter_val;
static Integer *pnxtval_counter=&pnxtval_counter_val;
#define INCR 1   /**< increment for NXTVAL */
#define BUSY -1L /**< indicates somebody else updating counter*/
#define NXTV_SERVER ((int)NNODES_() -1)


/**
 *  Get next value of shared counter.
 *  mproc > 0 ... returns requested value
 *  mproc < 0 ... server blocks until abs(mproc) processes are queued
 *                and returns junk
 *  mproc = 0 ... indicates to server that I am about to terminate
 */
Integer NXTVAL_(Integer *mproc)
{
    Integer local;
    int rc;
    int server = NXTV_SERVER;         /* id of server process */

#ifdef USE_VAMPIR
    vampir_begin(TCGMSG_NXTVAL,__FILE__,__LINE__);
#endif

    if (SR_parallel) {
        if (DEBUG_) {
            (void) printf(FMT_INT ": nxtval: mproc=" FMT_INT "\n",
                          NODEID_(), *mproc);
            (void) fflush(stdout);
        }

        if (*mproc < 0) {
            rc=MPI_Barrier(MPI_COMM_WORLD); 
            if(rc!=MPI_SUCCESS) {
                Error("nxtval: barrier failed",0);
            }

            /* reset the counter value to zero */
            if( NODEID_() == server) {
                *pnxtval_counter = 0;
            }

            rc=MPI_Barrier(MPI_COMM_WORLD); 
            if(rc!=MPI_SUCCESS) {
                Error("nxtval: barrier failed",0);
            }
        }
        if (*mproc > 0) {
            rc = ARMCI_Rmw(ARMCI_FETCH_AND_ADD_LONG,(int*)&local,(int*)pnxtval_counter,1,server);
        }
    } else {
        /* Not running in parallel ... just do a simulation */
        static int count = 0;
        if (*mproc == 1) {
            local = count++;
        } else if (*mproc == -1) {
            count = 0;
            local = 0;
        } else {
            Error("nxtval: sequential version with silly mproc ", (Integer) *mproc);
        }
    }

#ifdef USE_VAMPIR
    vampir_end(TCGMSG_NXTVAL,__FILE__,__LINE__);
#endif
    return local;
}


/**
 * initialization for nxtval -- called in PBEGIN
 */
void install_nxtval(int *argc, char **argv[])
{
    int rc;
    int me = (int)NODEID_(), bytes, server;

    void **ptr_ar;
    ptr_ar = (void **)malloc(sizeof(void *)*(int)NNODES_());
    if(!ptr_ar) {
        Error("malloc failed in install_nxtval", (Integer)NNODES_());  
    }

    ARMCI_Init_args(argc, argv);
    server = NXTV_SERVER;

    if(me== server) {
        bytes = sizeof(Integer);
    } else {
        bytes =0;
    }

    rc = ARMCI_Malloc(ptr_ar,bytes);
    if(rc) {
        Error("nxtv: armci_malloc failed",rc);
    }

    pnxtval_counter = (Integer*) ptr_ar[server];

    if(me==server) {
        *pnxtval_counter = (Integer)0;
    }

    free(ptr_ar);
    rc=MPI_Barrier(MPI_COMM_WORLD); 
    if(rc!=MPI_SUCCESS) {
        Error("init_nxtval: barrier failed",0);
    }
}


void finalize_nxtval()
{
/*
 * Cannot call ARMCI functions here as ARMCI might have been terminated
 * by now. NOTE: finalize_nxtval is called in pend(), which is called after
 * GA_Terminate/ARMCI_Finalize.
 */    
#if 0
    if(NODEID_() == NXTV_SERVER)ARMCI_Free(pnxtval_counter);
#endif
    ARMCI_Finalize();
}
