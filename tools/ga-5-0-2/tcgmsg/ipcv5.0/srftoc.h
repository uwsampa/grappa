/** @file
 * This header file provides definitions for c for the names of the
 * c message passing routines accessible from FORTRAN. It need not
 * be included directly in user c code, assuming that sndrcv.h has already.
 *
 * It is needed as the FORTRAN naming convention varies between machines
 * and it is the FORTRAN interface that is portable, not the c interface.
 * However by coding with the macro defnition names c portability is
 * ensured.
 *
 * On most system V machines (at least Cray and Ardent) FORTRAN uppercases
 * the names of all FORTRAN externals. On most BSD machines (at least
 * Sun, Alliant, Encore, Balance) FORTRAN appends an underbar (_).
 * Here the uppercase c routine name with an underbar is used as a
 * macro name appropriately defined for each machine. Also the BSD naming
 * is assumed by default.
 * Note that pbegin and pfilecopy are only called from c.
 */
#ifndef SRFTOC_H_
#define SRFTOC_H_

#define BRDCST_     F77_FUNC(brdcst,BRDCST)
#define DGOP_       F77_FUNC(dgop,DGOP)
#define DRAND48_    F77_FUNC(drand48,DRAND48)
#define IGOP_       F77_FUNC(igop,IGOP)
#define LLOG_       F77_FUNC(llog,LLOG)
#define MDTOB_      F77_FUNC(mdtob,MDTOB)
#define MDTOI_      F77_FUNC(mdtoi,MDTOI)
#define MITOB_      F77_FUNC(mitob,MITOB)
#define MITOD_      F77_FUNC(mitod,MITOD)
#define MTIME_      F77_FUNC(mtime,MTIME)
#define NICEFTN_    F77_FUNC(niceftn,NICEFTN)
#define NNODES_     F77_FUNC(nnodes,NNODES)
#define NODEID_     F77_FUNC(nodeid,NODEID)
#define NXTVAL_     F77_FUNC(nxtval,NXTVAL)
#define PARERR_     F77_FUNC(parerr,PARERR)
#define PBEGINF_    F77_FUNC(pbeginf,PBEGINF)
#define PBGINF_     F77_FUNC(pbginf,PBGINF)
#define PEND_       F77_FUNC(pend,PEND)
#define PFCOPY_     F77_FUNC(pfcopy,PFCOPY)
#define PROBE_      F77_FUNC(probe,PROBE)
#define RCV_        F77_FUNC(rcv,RCV)
#define SETDBG_     F77_FUNC(setdbg,SETDBG)
#define SND_        F77_FUNC(snd,SND)
#define SRAND48_    F77_FUNC(srand48,SRAND48)
#define STATS_      F77_FUNC(stats,STATS)
#define SYNCH_      F77_FUNC(synch,SYNCH)
#define TCGREADY_   F77_FUNC(tcgready,TCGREADY)
#define TCGTIME_    F77_FUNC(tcgtime,TCGTIME)
#define WAITCOM_    F77_FUNC(waitcom,WAITCOM)

#endif /* SRFTOC_H_ */
