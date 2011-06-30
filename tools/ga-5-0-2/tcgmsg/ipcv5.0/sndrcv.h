/** @file
 * This header file declares stubs and show prototypes of the 
 * public sndrcv calls
 *
 * srftoc.h contains macros which define the names of c routines
 * accessible from FORTRAN and vice versa
 */
#ifndef SNDRCV_H_
#define SNDRCV_H_

#include "typesf2c.h"
#include "msgtypesc.h"
#include "srftoc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* for brevity */
#define DBL DoublePrecision
#define INT Integer

extern void BRDCST_(INT *type, void *buf, INT *lenbuf, INT *originator);
extern void DGOP_(INT *type, DBL *x, INT *n, char *op, INT oplen);
extern DBL  DRAND48_();
extern void IGOP_(INT *type, INT *x, INT *n, char *op, INT oplen);
extern void LLOG_();
extern INT  MDTOB_(INT *n);
extern INT  MDTOI_(INT *n);
extern INT  MITOB_(INT *n);
extern INT  MITOD_(INT *n);
extern INT  MTIME_();
extern INT  NICEFTN_(INT *ival);
extern INT  NNODES_();
extern INT  NODEID_();
extern INT  NXTVAL_(INT *mproc);
extern void PARERR_(INT *code);
extern void PBEGINF_();
extern void PBGINF_();
extern void PEND_();
extern void PFCOPY_(INT *type, INT *node0, char *fname, int len);
extern INT  PROBE_(INT *type, INT *node);
extern void RCV_(INT *type, void *buf, INT *lenbuf, INT *lenmes, INT *nodeselect, INT * nodefrom, INT *sync);
extern void SETDBG_(INT *value);
extern void SND_(INT *type, void *buf, INT *lenbuf, INT *node, INT *sync);
extern void SRAND48_(INT *seed);
extern void STATS_();
extern void SYNCH_(INT *type);
extern INT  TCGREADY_();
extern DBL  TCGTIME_();
extern void WAITCOM_(INT *node);

#undef DBL
#undef INT

#ifdef __cplusplus
}
#endif

extern void Error(const char *string, Integer integer);
extern void tcgi_pbegin(int argc, char **argv);

#endif /* SNDRCV_H_ */
