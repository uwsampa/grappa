#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <mpi.h>

#include "tcgmsgP.h"
#ifdef USE_VAMPIR
#   include "tcgmsg_vampir.h"
#endif

#define LEN 2
Integer nxtval_counter=0;
#define INCR 1   /**< increment for NXTVAL */
#define BUSY -1L /**< indicates somebody else updating counter*/


void NextValueServer()
{
    Integer  cnt     = 0;       /**< actual counter */
    Integer  ndone   = 0;       /**< no. finished for this loop */
    int  type    = TYPE_NXTVAL; /**< message type */
    Integer  buf[LEN];          /**< buffer to get values */
    Integer  mproc;             /**< no. of processes running loop */
    Integer  nval;              /**< no. of values requested */
    int  done_list[MAX_PROCESS];/**< list of processes finished with this loop */
    int  lenmes, nodefrom;
    int  node;
    Integer  ntermin=0;
    MPI_Status status;

    while (1) {

        /* Wait for input from any node */

        MPI_Recv(buf, LEN, MPI_LONG, MPI_ANY_SOURCE, type, MPI_COMM_WORLD, &status); 
        MPI_Get_count(&status, MPI_LONG, &lenmes);
        nodefrom = status.MPI_SOURCE;

        if (lenmes != LEN) {
            Error("NextValueServer: lenmes != LEN", (Integer) lenmes);
            return;   /* Never actually gets here as does Integer jump */
        }

        mproc = buf[0];
        nval = buf[1];
        if (DEBUG_) {
            (void) printf("NVS: from=%ld, mproc=%ld, ndone=%ld\n",
                          nodefrom, mproc, ndone);
        }

        if (mproc == 0) {

            /* Sending process is about to terminate. Send reply and disable
             * sending to him. If all processes have finished return.
             *
             * All processes block on waiting for message
             * from nxtval server before terminating. nxtval only lets
             * everyone go when all have registered termination.
             */

            if (++ntermin == NNODES_()) {
                for (node=0; node<NNODES_(); node++) {
                    MPI_Send(&cnt, 1, MPI_LONG,  node, type, MPI_COMM_WORLD); 
                }
                MPI_Barrier(MPI_COMM_WORLD);
                MPI_Finalize();
                exit(0);
            }

        }
        else if (mproc > 0) {

            /* This is what we are here for */

            MPI_Send(&cnt, 1, MPI_LONG,  nodefrom, type, MPI_COMM_WORLD); 
            cnt += nval;
        }
        else if (mproc < 0) {

            /* This process has finished the loop. Wait until all mproc
               processes have finished before releasing it */

            done_list[ndone++] = nodefrom;

            if (ndone == -mproc) {
                while (ndone--) {
                    nodefrom = done_list[ndone];
                    MPI_Send(&cnt, 1, MPI_LONG,  nodefrom, type, MPI_COMM_WORLD); 
                }
                cnt = 0;
                ndone = 0;
            }
        }
    }
}


/**
 * Get next value of shared counter.
 * mproc > 0 ... returns requested value
 * mproc < 0 ... server blocks until abs(mproc) processes are queued
 *               and returns junk
 * mproc = 0 ... indicates to server that I am about to terminate
 */
Integer NXTVAL_(Integer *mproc)
{
    Integer  buf[2];
    MPI_Status status;
    int  type = TYPE_NXTVAL;
    Integer local;

#ifdef NXTVAL_SERVER
    int server = (int)NNODES_();    /**< id of server process */
#else
    int server = (int)NNODES_() -1; /**< id of server process */
#endif

#ifdef USE_VAMPIR
    vampir_begin(TCGMSG_NXTVAL,__FILE__,__LINE__);
#endif

    if (SR_parallel) {
        buf[0] = *mproc;
        buf[1] = INCR;

        if (DEBUG_) {
            (void) printf("%2ld: nxtval: mproc=%ld\n",NODEID_(), *mproc);
            (void) fflush(stdout);
        }

#ifdef NXTVAL_SERVER
        MPI_Send(buf, LEN, MPI_LONG,  server, type, MPI_COMM_WORLD); 
        MPI_Recv(buf, 1,   MPI_LONG,  server, type, MPI_COMM_WORLD, &status); 
        return buf[0];
#endif
    } else {
        /* Not running in parallel ... just do a simulation */
        static int count = 0;
        if (*mproc == 1) {
            local = count++;
        }
        else if (*mproc == -1) {
            count = 0;
            local = 0;
        }
        else {
            Error("nxtval: sequential version with silly mproc ",
                    (Integer) *mproc);
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
    int numprocs, myid;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

#ifdef NXTVAL_SERVER
    /* in this mode one process is hidden from the application */
    if(SR_parallel && myid == numprocs -1) {
#   ifndef QUIET
        printf("TCGMSG-MPI info: excluding one process for nxtval server\n");
        fflush(stdout);
#   endif /* QUIET */
        NextValueServer();
    }
#else
#   error Do not know how to implement nxtval !
#endif
}


void finalize_nxtval()
{
}
