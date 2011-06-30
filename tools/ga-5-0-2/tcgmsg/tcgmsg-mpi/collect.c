#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <mpi.h>

#include "tcgmsgP.h"

#ifdef USE_VAMPIR
#   include "tcgmsg_vampir.h"
#endif

/* size of internal buffer for global ops */
#define DGOP_BUF_SIZE 65536 
#define IGOP_BUF_SIZE (sizeof(DoublePrecision)/sizeof(Integer))*DGOP_BUF_SIZE 

static DoublePrecision gop_work[DGOP_BUF_SIZE]; /**< global ops buffer */

/**
 * global operations -- integer version 
 */
void IGOP_(Integer *ptype, Integer *x, Integer *pn, char *op, Integer oplen)
{
    Integer *work  = (Integer *) gop_work;
    Integer nleft  = *pn;
    Integer buflen = TCG_MIN(nleft,IGOP_BUF_SIZE); /**< Try to get even sized buffers */
    Integer nbuf   = (nleft-1) / buflen + 1;
    Integer n;

#ifdef USE_VAMPIR
    vampir_begin(TCGMSG_IGOP,__FILE__,__LINE__);
#endif

/* #ifdef ARMCI */
    if(!_tcg_initialized){
        TCGMSG_Comm = MPI_COMM_WORLD;
        _tcg_initialized = 1;
    }
/* #endif */

    buflen = (nleft-1) / nbuf + 1;

    if (strncmp(op,"abs",3) == 0) {
        n = *pn;
        while(n--) {
            x[n] = TCG_ABS(x[n]);
        }
    }

    while (nleft) {
        int root = 0; 
        int ierr = MPI_SUCCESS;
        int ndo = TCG_MIN(nleft, buflen);

        if (strncmp(op,"+",1) == 0) {
            ierr = MPI_Reduce(x, work, ndo, TCG_INT, MPI_SUM, root, TCGMSG_Comm);
        } else if (strncmp(op,"*",1) == 0) {
            ierr = MPI_Reduce(x, work, ndo, TCG_INT, MPI_PROD, root, TCGMSG_Comm);
        } else if (strncmp(op,"max",3) == 0 || strncmp(op,"absmax",6) == 0) {
            ierr = MPI_Reduce(x, work, ndo, TCG_INT, MPI_MAX, root, TCGMSG_Comm);
        }
        else if (strncmp(op,"min",3) == 0 || strncmp(op,"absmin",6) == 0) {
            ierr = MPI_Reduce(x, work, ndo, TCG_INT, MPI_MIN, root, TCGMSG_Comm);
        }
        else if (strncmp(op,"or",2) == 0) {
            ierr = MPI_Reduce(x, work, ndo, TCG_INT, MPI_BOR, root, TCGMSG_Comm);
        } else {
            Error("IGOP: unknown operation requested", (Integer) *pn);
        }
        tcgmsg_test_statusM("IGOP: MPI_Reduce:", ierr  );

        ierr   = MPI_Bcast(work, ndo, TCG_INT, root, TCGMSG_Comm);
        tcgmsg_test_statusM("IGOP: MPI_Bcast:", ierr);

        n = ndo;
        while(n--) {
            x[n] = work[n];
        }

        nleft -= ndo; x+= ndo;
    }
#ifdef USE_VAMPIR
    vampir_end(TCGMSG_IGOP,__FILE__,__LINE__);
#endif
}



/**
 * global operations -- DoublePrecision version 
 */
void DGOP_(Integer *ptype, DoublePrecision *x, Integer *pn, char *op, Integer oplen)
{
    DoublePrecision *work=  gop_work;
    Integer nleft  = *pn;
    Integer buflen = TCG_MIN(nleft,DGOP_BUF_SIZE); /**< Try to get even sized buffers */
    Integer nbuf   = (nleft-1) / buflen + 1;
    Integer n;

#ifdef USE_VAMPIR
    vampir_begin(TCGMSG_DGOP,__FILE__,__LINE__);
#endif
    buflen = (nleft-1) / nbuf + 1;

    if (strncmp(op,"abs",3) == 0) {
        n = *pn;
        while(n--) {
            x[n] = TCG_ABS(x[n]);
        }
    }

    while (nleft) {
        int root = 0; 
        int ierr = MPI_SUCCESS;
        int ndo = TCG_MIN(nleft, buflen);

        if (strncmp(op,"+",1) == 0) {
            ierr = MPI_Reduce(x, work, ndo, TCG_DBL, MPI_SUM, root, TCGMSG_Comm);
        } else if (strncmp(op,"*",1) == 0) {
            ierr = MPI_Reduce(x, work, ndo, TCG_DBL, MPI_PROD, root, TCGMSG_Comm);
        } else if (strncmp(op,"max",3) == 0 || strncmp(op,"absmax",6) == 0) {
            ierr = MPI_Reduce(x, work, ndo, TCG_DBL, MPI_MAX, root, TCGMSG_Comm);
        } else if (strncmp(op,"min",3) == 0 || strncmp(op,"absmin",6) == 0) {
            ierr = MPI_Reduce(x, work, ndo, TCG_DBL, MPI_MIN, root, TCGMSG_Comm);
        } else {
            Error("DGOP: unknown operation requested", (Integer) *pn);
        }
        tcgmsg_test_statusM("DGOP: MPI_Reduce:", ierr  );

        ierr   = MPI_Bcast(work, ndo, TCG_DBL, root, TCGMSG_Comm);
        tcgmsg_test_statusM("DGOP: MPI_Bcast:", ierr  );

        n = ndo;
        while(n--) {
            x[n] = work[n];
        }

        nleft -= ndo; x+= ndo;
    }
#ifdef USE_VAMPIR
    vampir_end(TCGMSG_DGOP,__FILE__,__LINE__);
#endif
}


/**
 * Synchronize processes
 */
void SYNCH_(Integer *type)
{
/* #ifdef ARMCI */
    if(!_tcg_initialized){
        TCGMSG_Comm = MPI_COMM_WORLD;
        _tcg_initialized = 1;
    }
/* #endif */
    MPI_Barrier(TCGMSG_Comm);
}



/**
 * broadcast buffer to all other processes from process originator
 */
void BRDCST_(Integer *type, char *buf, Integer *lenbuf, Integer *originator)
{
    /*  hope that MPI int is large enough to store value in lenbuf */
    int count = (int)*lenbuf, root = (int)*originator;

    MPI_Bcast(buf, count, MPI_CHAR, root, TCGMSG_Comm);
}
