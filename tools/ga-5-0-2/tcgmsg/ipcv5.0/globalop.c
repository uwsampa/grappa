#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include "typesf2c.h"
#include "srftoc.h"
#include "tcgmsgP.h"

#ifdef USE_VAMPIR
#   include "tcgmsg_vampir.h"
#endif

#define BUF_SIZE  10000
#define IBUF_SIZE (BUF_SIZE * sizeof(DoublePrecision)/sizeof(Integer)) 
DoublePrecision _gops_work[BUF_SIZE];

Integer one=1;

#define TCG_MAX(a,b) (((a) >= (b)) ? (a) : (b))
#define TCG_MIN(a,b) (((a) <= (b)) ? (a) : (b))
#define TCG_ABS(a)   (((a) >= 0) ? (a) : (-(a)))


void BRDCST_(Integer *type, void *buf, Integer *len, Integer *originator)
{
    Integer me=NODEID_(), nproc=NNODES_(), lenmes, from, root=0;
    Integer up, left, right;

#ifdef USE_VAMPIR
    vampir_begin(TCGMSG_BRDCST,__FILE__,__LINE__);
#endif

    /* determine location in the binary tree */
    up    = (me-1)/2;    if(up >= nproc)       up = -1;
    left  =  2* me + 1;  if(left >= nproc)   left = -1;
    right =  2* me + 2;  if(right >= nproc) right = -1;

    /*  originator sends data to root */
    if (*originator != root ){
        if(me == *originator) SND_(type, buf, len, &root, &one); 
        if(me == root) RCV_(type, buf, len, &lenmes, originator, &from, &one); 
    }

    if (me != root) RCV_(type, buf, len, &lenmes, &up, &from, &one);
    if (left > -1)  SND_(type, buf, len, &left, &one);
    if (right > -1) SND_(type, buf, len, &right, &one);

#ifdef USE_VAMPIR
    vampir_end(TCGMSG_BRDCST,__FILE__,__LINE__);
#endif
}


/**
 * implements x = op(x,work) for integer datatype
 *  x[n], work[n] -  arrays of n integers
 */ 
static void idoop(Integer n, char *op, Integer *x, Integer *work)
{
    if (strncmp(op,"+",1) == 0)
        while(n--)
            *x++ += *work++;
    else if (strncmp(op,"*",1) == 0)
        while(n--)
            *x++ *= *work++;
    else if (strncmp(op,"max",3) == 0)
        while(n--) {
            *x = TCG_MAX(*x, *work);
            x++; work++;
        }
    else if (strncmp(op,"min",3) == 0)
        while(n--) {
            *x = TCG_MIN(*x, *work);
            x++; work++;
        }
    else if (strncmp(op,"absmax",6) == 0)
        while(n--) {
            register Integer x1 = TCG_ABS(*x), x2 = TCG_ABS(*work);
            *x = TCG_MAX(x1, x2);
            x++; work++;
        }
    else if (strncmp(op,"absmin",6) == 0)
        while(n--) {
            register Integer x1 = TCG_ABS(*x), x2 = TCG_ABS(*work);
            *x = TCG_MIN(x1, x2);
            x++; work++;
        }
    else if (strncmp(op,"or",2) == 0)
        while(n--) {
            *x |= *work;
            x++; work++;
        }
    else
        Error("idoop: unknown operation requested", (long) n);
}


/**
 * implements x = op(x,work) for double datatype
 *  x[n], work[n] -  arrays of n doubles
 */ 
static void ddoop(Integer n, char *op, double *x, double *work)
{
    if (strncmp(op,"+",1) == 0)
        while(n--)
            *x++ += *work++;
    else if (strncmp(op,"*",1) == 0)
        while(n--)
            *x++ *= *work++;
    else if (strncmp(op,"max",3) == 0)
        while(n--) {
            *x = TCG_MAX(*x, *work);
            x++; work++;
        }
    else if (strncmp(op,"min",3) == 0)
        while(n--) {
            *x = TCG_MIN(*x, *work);
            x++; work++;
        }
    else if (strncmp(op,"absmax",6) == 0)
        while(n--) {
            register double x1 = TCG_ABS(*x), x2 = TCG_ABS(*work);
            *x = TCG_MAX(x1, x2);
            x++; work++;
        }
    else if (strncmp(op,"absmin",6) == 0)
        while(n--) {
            register double x1 = TCG_ABS(*x), x2 = TCG_ABS(*work);
            *x = TCG_MIN(x1, x2);
            x++; work++;
        }
    else
        Error("ddoop: unknown operation requested", (long) n);
}


void DGOP_(
        Integer *type, DoublePrecision *x, Integer *n, char *op, Integer oplen)
{
    Integer me=NODEID_(), nproc=NNODES_(), len, lenmes, from, root=0;
    DoublePrecision *work = _gops_work, *origx = x;
    Integer ndo, up, left, right, np=*n, orign = *n;

#ifdef USE_VAMPIR
    vampir_begin(TCGMSG_DGOP,__FILE__,__LINE__);
#endif

    /* determine location in the binary tree */
    up    = (me-1)/2;    if(up >= nproc)       up = -1;
    left  =  2* me + 1;  if(left >= nproc)   left = -1;
    right =  2* me + 2;  if(right >= nproc) right = -1;

    while ((ndo = (np <= BUF_SIZE) ? np : BUF_SIZE)) {
        len = lenmes = ndo*sizeof(DoublePrecision);

        if (left > -1) {
            RCV_(type, (char *) work, &len, &lenmes, &left, &from, &one);
            ddoop(ndo, op, x, work);
        }
        if (right > -1) {
            RCV_(type, (char *) work, &len, &lenmes, &right, &from, &one);
            ddoop(ndo, op, x, work);
        }
        if (me != root) SND_(type, x, &len, &up, &one); 

        np -=ndo;
        x  +=ndo;
    }

    /* Now, root broadcasts the result down the binary tree */
    len = orign*sizeof(DoublePrecision);
    BRDCST_(type, (char *) origx, &len, &root);

#ifdef USE_VAMPIR
    vampir_end(TCGMSG_DGOP,__FILE__,__LINE__);
#endif
}


void IGOP_(Integer *type, Integer *x, Integer *n, char *op, Integer oplen)
{
    Integer me=NODEID_(), nproc=NNODES_(), len, lenmes, from, root=0;
    Integer *work = (Integer*)_gops_work;
    Integer *origx = x;
    Integer ndo, up, left, right, np=*n, orign =*n;

#ifdef USE_VAMPIR
    vampir_begin(TCGMSG_IGOP,__FILE__,__LINE__);
#endif

    /* determine location in the binary tree */
    up    = (me-1)/2;    if(up >= nproc)       up = -1;
    left  =  2* me + 1;  if(left >= nproc)   left = -1;
    right =  2* me + 2;  if(right >= nproc) right = -1;

    while ((ndo = (np<=IBUF_SIZE) ? np : IBUF_SIZE)) {
        len = lenmes = ndo*sizeof(Integer);

        if (left > -1) {
            RCV_(type, (char *) work, &len, &lenmes, &left, &from, &one);
            idoop(ndo, op, x, work);
        }
        if (right > -1) {
            RCV_(type, (char *) work, &len, &lenmes, &right, &from, &one);
            idoop(ndo, op, x, work);
        }
        if (me != root) SND_(type, x, &len, &up, &one); 

        np -=ndo;
        x  +=ndo;
    }

    /* Now, root broadcasts the result down the binary tree */
    len = orign*sizeof(Integer);
    BRDCST_(type, (char *) origx, &len, &root);

#ifdef USE_VAMPIR
    vampir_end(TCGMSG_IGOP,__FILE__,__LINE__);
#endif
}
