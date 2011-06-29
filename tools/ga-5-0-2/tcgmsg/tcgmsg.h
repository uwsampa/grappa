/** @file
 * This header file declares the public C API to tcgmsg.
 */
#ifndef TCGMSG_H_
#define TCGMSG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* for brevity */
#define DBL double
#define INT long

extern void tcg_alt_pbegin();
extern void tcg_brdcst(INT type, void *buf, INT lenbuf, INT originator);
extern void tcg_dgop(INT type, DBL *x, INT n, char *op);
extern DBL  tcg_drand48();
extern void tcg_error(char *msg, INT code);
extern void tcg_igop(INT type, INT *x, INT n, char *op);
extern void tcg_llog();
extern INT  tcg_mdtob(INT n);
extern INT  tcg_mdtoi(INT n);
extern INT  tcg_mitob(INT n);
extern INT  tcg_mitod(INT n);
extern INT  tcg_mtime();
extern INT  tcg_niceftn(INT ival);
extern INT  tcg_nnodes();
extern INT  tcg_nodeid();
extern INT  tcg_nxtval(INT mproc);
extern void tcg_parerr(INT code);
extern void tcg_pbegin(int argc, char **argv);
extern void tcg_pbeginf();
extern void tcg_pbginf();
extern void tcg_pend();
extern void tcg_pfcopy(INT type, INT node0, char *fname);
extern INT  tcg_probe(INT type, INT node);
extern void tcg_rcv(INT type, void *buf, INT lenbuf, INT *lenmes, INT nodeselect, INT *nodefrom, INT sync);
extern void tcg_setdbg(INT value);
extern void tcg_snd(INT type, void *buf, INT lenbuf, INT node, INT sync);
extern void tcg_srand48(INT seed);
extern void tcg_stats();
extern void tcg_synch(INT type);
extern INT  tcg_tcgready();
extern DBL  tcg_time();
extern void tcg_waitcom(INT node);

#undef DBL
#undef INT

#ifdef __cplusplus
}
#endif

#endif /* TCGMSG_H_ */
