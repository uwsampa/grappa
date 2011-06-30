#if HAVE_CONFIG_H
#   include "config.h"
#endif

#if HAVE_STRING_H
#   include "string.h"
#endif

/* $Id: collect.c,v 1.23.2.5 2007-08-03 19:52:28 manoj Exp $ */
#include "typesf2c.h"
#include "globalp.h"
#include "global.h"
#include "message.h"
#include "base.h"

/* can handle ga_brdcst/igop/dgop via ARMCI or native message-passing library
 * uncomment line below to use the ARMCI version */
#ifndef NEC
#define  ARMCI_COLLECTIVES 
#endif

#ifdef MPI
#  include <mpi.h>
#else
#  include <tcgmsg.h>
#endif


void ga_msg_brdcst(Integer type, void *buffer, Integer len, Integer root)
{
#ifdef ARMCI_COLLECTIVES
    int p_grp = (int)ga_pgroup_get_default_();
    if (p_grp > 0) {
#   ifdef MPI
        int aroot = PGRP_LIST[p_grp].inv_map_proc_list[root];
        armci_msg_group_bcast_scope(SCOPE_ALL,buffer, (int)len, aroot,(&(PGRP_LIST[p_grp].group)));
#   endif
    } else {
        armci_msg_bcast(buffer, (int)len, (int)root);
    }
#else
#   ifdef MPI
    MPI_Bcast(buffer, (int)len, MPI_CHAR, (int)root, MPI_COMM_WORLD);
#   else
    tcg_brdcst(type, buffer, len, root);
#   endif
#endif
}


/*\ BROADCAST
\*/
void FATR ga_brdcst_(
        Integer *type, void *buf, Integer *len, Integer *originator)
{
    _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
    ga_msg_brdcst(*type,buf,*len,*originator);
}


void FATR ga_pgroup_brdcst_(Integer *grp_id, Integer *type, void *buf, Integer *len, Integer *originator)
{
    int p_grp = (int)*grp_id;
    _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
    if (p_grp > 0) {
#ifdef MPI
       int aroot = PGRP_LIST[p_grp].inv_map_proc_list[*originator];
       armci_msg_group_bcast_scope(SCOPE_ALL,buf,(int)*len,aroot,(&(PGRP_LIST[p_grp].group)));
#endif
    } else {
       int aroot = (int)*originator;
       armci_msg_bcast(buf, (int)*len, (int)aroot);
    }
}


#ifdef MPI
void ga_mpi_communicator(GA_COMM)
MPI_Comm *GA_COMM;
{
       *GA_COMM = MPI_COMM_WORLD;
}
#endif


void ga_msg_sync_()
{
#ifdef MPI
    int p_grp = (int)ga_pgroup_get_default_(); 
    if(p_grp>0)
       armci_msg_group_barrier(&(PGRP_LIST[p_grp].group));
    else
       armci_msg_barrier();
#else
#  ifdef LAPI
     armci_msg_barrier();
#  else
     tcg_synch(GA_TYPE_SYN);
#  endif
#endif
}


void ga_msg_pgroup_sync_(Integer *grp_id)
{
    int p_grp = (int)(*grp_id);
    if(p_grp>0) {
#     ifdef MPI       
        armci_msg_group_barrier(&(PGRP_LIST[p_grp].group));
#     else
        gai_error("ga_msg_pgroup_sync not implemented",0);
#     endif
    }
    else {
#     if defined(MPI) || defined(LAPI)
       armci_msg_barrier();
#     else
       tcg_synch(GA_TYPE_SYN);
#    endif
    }
}


void gai_pgroup_gop(Integer p_grp, Integer type, void *x, Integer n, char *op)
{
    _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
    if (p_grp > 0) {
#if defined(ARMCI_COLLECTIVES) && defined(MPI)
        int group = (int)p_grp;
        switch (type){
            case C_INT:
                armci_msg_group_igop((int*)x, n, op, (&(PGRP_LIST[group].group)));
                break;
            case C_LONG:
                armci_msg_group_lgop((long*)x, n, op, (&(PGRP_LIST[group].group)));
                break;
            case C_LONGLONG:
                armci_msg_group_llgop((long long*)x, n, op, (&(PGRP_LIST[group].group)));
                break;
            case C_FLOAT:
                armci_msg_group_fgop((float*)x, n, op, (&(PGRP_LIST[group].group)));
                break;
            case C_DBL:
                armci_msg_group_dgop((double*)x, n, op, (&(PGRP_LIST[group].group)));
                break;
            case C_SCPL:
                armci_msg_group_fgop((float*)x, 2*n, op, (&(PGRP_LIST[group].group)));
                break;
            case C_DCPL:
                armci_msg_group_dgop((double*)x, 2*n, op, (&(PGRP_LIST[group].group)));
                break;
            default: gai_error(" wrong data type ",type);
        }
#else
        gai_error("Groups not implemented for system",0);
#endif
    } else {
        gai_gop(type, x, n, op);
    }
}


void gai_gop(Integer type, void *x, Integer n, char *op)
{
    Integer p_grp = ga_pgroup_get_default_();

    _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
    if (p_grp > 0) {
        gai_pgroup_gop(p_grp, type, x, n, op);
    } else {
#if defined(ARMCI_COLLECTIVES) || defined(MPI)
        switch (type){
            case C_INT:
                armci_msg_igop((int*)x, n, op);
                break;
            case C_LONG:
                armci_msg_lgop((long*)x, n, op);
                break;
            case C_LONGLONG:
                armci_msg_llgop((long long*)x, n, op);
                break;
            case C_FLOAT:
                armci_msg_fgop((float*)x, n, op);
                break;
            case C_DBL:
                armci_msg_dgop((double*)x, n, op);
                break;
            case C_SCPL:
                armci_msg_fgop((float*)x, 2*n, op);
                break;
            case C_DCPL:
                armci_msg_dgop((double*)x, 2*n, op);
                break;
            default:
                gai_error(" wrong data type ",type);
        }
#else
        switch (type){
            case C_INT:
                gai_error("Operation not defined for system",0);
                break;
            case C_LONG:
                tcg_igop(GA_TYPE_GOP, x, n, op);
                break;
            case C_LONGLONG:
                gai_error("Operation not defined for system",0);
                break;
            case C_FLOAT:
                gai_error("Operation not defined for system",0);
                break;
            case C_DBL:
                tcg_dgop(GA_TYPE_GOP, x, n, op);
                break;
            case C_SCPL:
                gai_error("Operation not defined for system",0);
                break;
            case C_DCPL:
                gai_error("Operation not defined for system",0);
                break;
            default:
                gai_error(" wrong data type ",type);
        }
#endif
    }
}

/***************************************************************************
 * GLOBAL OPERATIONS -- C type safe for convenience
 **************************************************************************/

void gac_igop(int *x, Integer n, char *op)
{ gai_gop(C_INT, x, n, op); }

void gac_lgop(long *x, Integer n, char *op)
{ gai_gop(C_LONG, x, n, op); }

void gac_llgop(long long *x, Integer n, char *op)
{ gai_gop(C_LONGLONG, x, n, op); }

void gac_fgop(float *x, Integer n, char *op)
{ gai_gop(C_FLOAT, x, n, op); }

void gac_dgop(double *x, Integer n, char *op)
{ gai_gop(C_DBL, x, n, op); }

void gac_cgop(SingleComplex *x, Integer n, char *op)
{ gai_gop(C_SCPL, x, n, op); }

void gac_zgop(DoubleComplex *x, Integer n, char *op)
{ gai_gop(C_DCPL, x, n, op); }

/***************************************************************************
 * GLOBAL OPERATIONS GROUP -- Fortran
 **************************************************************************/

void ga_pgroup_igop_(Integer *grp, Integer *type, Integer *x, Integer *n, char *op, int len)
{ gai_pgroup_gop(*grp, ga_type_f2c(MT_F_INT), x, *n, op); }

void ga_pgroup_sgop_(Integer *grp, Integer *type, Real *x, Integer *n, char *op, int len)
{ gai_pgroup_gop(*grp, ga_type_f2c(MT_F_REAL), x, *n, op); }

void ga_pgroup_dgop_(Integer *grp, Integer *type, DoublePrecision *x, Integer *n, char *op, int len)
{ gai_pgroup_gop(*grp, ga_type_f2c(MT_F_DBL), x, *n, op); }

void ga_pgroup_cgop_(Integer *grp, Integer *type, SingleComplex *x, Integer *n, char *op, int len)
{ gai_pgroup_gop(*grp, ga_type_f2c(MT_F_SCPL), x, *n, op); }

void ga_pgroup_zgop_(Integer *grp, Integer *type, DoubleComplex *x, Integer *n, char *op, int len)
{ gai_pgroup_gop(*grp, ga_type_f2c(MT_F_DCPL), x, *n, op); }


/***************************************************************************
 * GLOBAL OPERATIONS -- Fortran
 **************************************************************************/

void ga_gop_(Integer *type, void *x, Integer *n, char *op, int len)
{ gai_gop(ga_type_f2c(*type), x, *n, op); }

void ga_igop_(Integer *type, Integer *x, Integer *n, char *op, int len)
{
#if defined(ARMCI_COLLECTIVES) || defined(MPI)
    gai_gop(ga_type_f2c(MT_F_INT), x, *n, op);
#else
    IGOP_(type, x, n, op, len);
#endif
}

void ga_sgop_(Integer *type, Real *x, Integer *n, char *op, int len)
{ gai_gop(ga_type_f2c(MT_F_REAL), x, *n, op); }

void ga_dgop_(Integer *type, DoublePrecision *x, Integer *n, char *op, int len)
{
#if defined(ARMCI_COLLECTIVES) || defined(MPI)
    gai_gop(ga_type_f2c(MT_F_DBL), x, *n, op);
#else
    DGOP_(type, x, n, op, len);
#endif
}

void ga_cgop_(Integer *type, SingleComplex *x, Integer *n, char *op, int len)
{ gai_gop(ga_type_f2c(MT_F_SCPL), x, *n, op); }


void ga_zgop_(Integer *type, DoubleComplex *x, Integer *n, char *op, int len)
{
#if defined(ARMCI_COLLECTIVES) || defined(MPI)
    gai_gop(ga_type_f2c(MT_F_DCPL), x, *n, op);
#else
    Integer 2n = 2*(*n);
    DGOP_(type, x, &2n, op, len);
#endif
}

/***************************************************************************
 * GLOBAL OPERATIONS -- Fortran (old internal interfaces -- deprecated)
 **************************************************************************/

void gai_igop(Integer type, Integer *x, Integer n, char *op)
{ ga_igop_(&type, x, &n, op, strlen(op)); }

void gai_sgop(Integer type, Real *x, Integer n, char *op)
{ ga_sgop_(&type, x, &n, op, strlen(op)); }

void gai_dgop(Integer type, DoublePrecision *x, Integer n, char *op)
{ ga_dgop_(&type, x, &n, op, strlen(op)); }

void gai_cgop(Integer type, SingleComplex *x, Integer n, char *op)
{ ga_cgop_(&type, x, &n, op, strlen(op)); }

void gai_zgop(Integer type, DoubleComplex *x, Integer n, char *op)
{ ga_zgop_(&type, x, &n, op, strlen(op)); }

void gai_pgroup_igop(Integer p_grp, Integer type, Integer *x, Integer n, char *op)
{ ga_pgroup_igop_(&p_grp, &type, x, &n, op, strlen(op)); }

void gai_pgroup_sgop(Integer p_grp, Integer type, Real *x, Integer n, char *op)
{ ga_pgroup_sgop_(&p_grp, &type, x, &n, op, strlen(op)); }

void gai_pgroup_dgop(Integer p_grp, Integer type, DoublePrecision *x, Integer n, char *op)
{ ga_pgroup_dgop_(&p_grp, &type, x, &n, op, strlen(op)); }

void gai_pgroup_cgop(Integer p_grp, Integer type, SingleComplex *x, Integer n, char *op)
{ ga_pgroup_cgop_(&p_grp, &type, x, &n, op, strlen(op)); }

void gai_pgroup_zgop(Integer p_grp, Integer type, DoubleComplex *x, Integer n, char *op)
{ ga_pgroup_zgop_(&p_grp, &type, x, &n, op, strlen(op)); }
