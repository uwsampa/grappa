#if HAVE_CONFIG_H
#   include "config.h"
#endif

/** @file
 * This header file declares the public C API to tcgmsg.
 */

#if HAVE_STRING_H
#   include <string.h>
#endif
#if HAVE_STDLIB_H
#   include <stdlib.h>
#endif

#include "sndrcv.h"
#include "tcgmsg.h"
#include "typesf2c.h"

extern void tcgi_pfilecopy(Integer*,Integer*,char*);
extern void tcgi_pbegin(int argc, char **argv);
extern void tcgi_alt_pbegin(int *argc, char **argv[]);


void tcg_alt_pbegin(int *argc, char **argv[])
{
    tcgi_alt_pbegin(argc, argv);
}


void tcg_brdcst(long type, void *buf, long lenbuf, long originator)
{
    Integer atype = type;
    Integer alenbuf = lenbuf;
    Integer aoriginator = originator;

    BRDCST_(&atype, buf, &alenbuf, &aoriginator);
}


void tcg_dgop(long type, double *x, long n, char *op)
{
    Integer atype = type;
    Integer an = n;
    DoublePrecision *ax;
    int i;

    ax = (DoublePrecision*)malloc(n * sizeof(DoublePrecision));
    for (i=0; i<n; ++i) {
        ax[i] = (DoublePrecision)x[i];
    }
    DGOP_(&atype, ax, &an, op, strlen(op));
    for (i=0; i<n; ++i) {
        x[i] = (double)ax[i];
    }
    free(ax);
}


double tcg_drand48()
{
    return DRAND48_();
}


void tcg_igop(long type, long *x, long n, char *op)
{
    Integer atype = type;
    Integer an = n;
    Integer *ax;
    int i;

    ax = (Integer*)malloc(n * sizeof(Integer));
    for (i=0; i<n; ++i) {
        ax[i] = (Integer)x[i];
    }
    IGOP_(&atype, ax, &an, op, strlen(op));
    for (i=0; i<n; ++i) {
        x[i] = (long)ax[i];
    }
    free(ax);
}


void tcg_llog()
{
    LLOG_();
}


long tcg_mdtob(long n)
{
    Integer an = n;

    return MDTOB_(&an);
}


long tcg_mdtoi(long n)
{
    Integer an = n;

    return MDTOI_(&an);
}


long tcg_mitob(long n)
{
    Integer an = n;

    return MITOB_(&an);
}


long tcg_mitod(long n)
{
    Integer an = n;

    return MITOD_(&an);
}


long tcg_mtime()
{
    return MTIME_();
}


long tcg_niceftn(long ival)
{
    Integer aival = ival;

    return NICEFTN_(&aival);
}


long tcg_nnodes()
{
    return NNODES_();
}


long tcg_nodeid()
{
    return NODEID_();
}


long tcg_nxtval(long mproc)
{
    Integer amproc = mproc;

    return NXTVAL_(&amproc);
}


void tcg_error(char *msg, long code)
{
    Integer acode = code;

    Error(msg, acode);
}


void tcg_pbegin(int argc, char **argv)
{
    tcgi_pbegin(argc,argv);
}


void tcg_pbeginf()
{
    PBEGINF_();
}


void tcg_pbginf()
{
    PBGINF_();
}


void tcg_pend()
{
    PEND_();
}


void tcg_pfcopy(long type, long node0, char *fname)
{
    Integer atype = type;
    Integer anode0 = node0;

    tcgi_pfilecopy(&atype, &anode0, fname);
}


long tcg_probe(long type, long node)
{
    Integer atype = type;
    Integer anode = node;

    return PROBE_(&atype, &anode);
}


void tcg_rcv(long type, void *buf, long lenbuf, long *lenmes,
        long nodeselect, long *nodefrom, long sync)
{
    Integer atype = type;
    Integer alenbuf = lenbuf;
    Integer alenmes = *lenmes;
    Integer anodeselect = nodeselect;
    Integer anodefrom = *nodefrom;
    Integer async = sync;

    RCV_(&atype, buf, &alenbuf, &alenmes, &anodeselect, &anodefrom, &async);

    *lenmes = alenmes;
    *nodefrom = anodefrom;
}


void tcg_setdbg(long value)
{
    Integer avalue = value;
    SETDBG_(&avalue);
}


void tcg_snd(long type, void *buf, long lenbuf, long node, long sync)
{
    Integer atype = type;
    Integer alenbuf = lenbuf;
    Integer anode = node;
    Integer async = sync;

    SND_(&atype, buf, &alenbuf, &anode, &async);
}


void tcg_srand48(long seed)
{
    Integer aseed = seed;

    SRAND48_(&aseed);
}


void tcg_stats()
{
    STATS_();
}


void tcg_synch(long type)
{
    Integer atype = type;

    SYNCH_(&atype);
}


long tcg_tcgready()
{
    return TCGREADY_();
}


double tcg_time()
{
    return TCGTIME_();
}


void tcg_waitcom(long node)
{
    Integer anode = node;

    WAITCOM_(&anode);
}
