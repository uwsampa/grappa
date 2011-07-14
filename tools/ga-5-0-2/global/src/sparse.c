#if HAVE_CONFIG_H
#   include "config.h"
#endif

#if HAVE_STDIO_H
#   include <stdio.h>
#endif
#if HAVE_STDLIB_H
#   include <stdlib.h>
#endif
#if HAVE_STRINGS_H
#   include <strings.h>
#endif
#include "global.h"
#include "globalp.h"
#include "macdecls.h"
#include "message.h"


/* abstract operations, 'regular' (reg) and 'complex' (cpl) */
#define assign_reg(a,b) (a) = (b)
#define assign_cpl(a,b) (a).real = (b).real; \
                        (a).imag = (b).imag
#define assign_zero_reg(a) (a) = 0
#define assign_zero_cpl(a) (a).real = 0; \
                           (a).imag = 0
#define assign_add_reg(a,b,c) (a) = ((b) + (c))
#define assign_add_cpl(a,b,c) (a).real = ((b).real + (c).real); \
                              (a).imag = ((b).imag + (c).imag)
#define assign_mul_constant_reg(a,b,c) (a) = ((b) * (c))
#define assign_mul_constant_cpl(a,b,c) (a).real = ((b) * (c).real); \
                                       (a).imag = ((b) * (c).imag)
#define add_assign_reg(a,b) (a) += (b)
#define add_assign_cpl(a,b) (a).real += (b).real; \
                            (a).imag += (b).imag
#define neq_zero_reg(a) (0 != (a))
#define neq_zero_cpl(a) (0 != (a).real || 0 != (a).imag)
#define eq_zero_reg(a) (0 == (a))
#define eq_zero_cpl(a) (0 == (a).real && 0 == (a).imag)


/*\ sets values for specified array elements by enumerating with stride
\*/
void FATR ga_patch_enum_(Integer* g_a, Integer* lo, Integer* hi, 
                         void* start, void* stride)
{
Integer dims[1],lop,hip;
Integer ndim, type, me, off;
register Integer i;
register Integer nelem;

   ga_sync_();
   me = ga_nodeid_();

   gai_check_handle(g_a, "ga_patch_enum");

   ndim = ga_ndim_(g_a);
   if (ndim > 1) gai_error("ga_patch_enum:applicable to 1-dim arrays",ndim);

   nga_inquire_internal_(g_a, &type, &ndim, dims);
   nga_distribution_(g_a, &me, &lop, &hip);

   if ( lop > 0 ){ /* we get 0 if no elements stored on this process */

      /* take product of patch owned and specified by the user */ 
      if(*hi <lop || hip <*lo); /* we got no elements to update */
      else{
        void *ptr;
        Integer ld;
        nelem = hip-lop+1;

        if(lop < *lo)lop = *lo;
        if(hip > *hi)hip = *hi;
        off = lop - *lo;
        nga_access_ptr(g_a, &lop, &hip, &ptr, &ld);
        
        switch (type) {
#define ga_patch_enum_case(MT,T,AT)                                         \
            case MT:                                                        \
                {                                                           \
                    T *aptr = (T*)ptr;                                      \
                    T astart = *((T*)start);                                \
                    T astride = *((T*)stride);                              \
                    for (i=0; i<nelem; i++) {                               \
                        T offset;                                           \
                        assign_mul_constant_##AT(offset,off+i,astride);     \
                        assign_add_##AT(aptr[i],astart,offset);             \
                    }                                                       \
                    break;                                                  \
                }
            ga_patch_enum_case(C_INT,int,reg)
            ga_patch_enum_case(C_LONG,long,reg)
            ga_patch_enum_case(C_LONGLONG,long long,reg)
            ga_patch_enum_case(C_FLOAT,float,reg)
            ga_patch_enum_case(C_DBL,double,reg)
            ga_patch_enum_case(C_SCPL,SingleComplex,cpl)
            ga_patch_enum_case(C_DCPL,DoubleComplex,cpl)
#undef ga_patch_enum_cpl
            default: gai_error("ga_patch_enum:wrong data type ",type);
        }

        nga_release_update_(g_a, &lop, &hip);
      }
   }
   
   ga_sync_();
}



void FATR ga_scan_copy_(Integer* g_src, Integer* g_dst, Integer* g_msk,
                        Integer* lo, Integer* hi)
{
    long *lim=NULL;
    Integer i, nproc, me, elems, ioff, lop, hip, ndim, dims, ld;
    Integer type_src, type_dst, type_msk, combined_type;
    void *ptr_src=NULL;
    void *ptr_dst=NULL;
    void *ptr_msk=NULL;

    nproc = ga_nnodes_();
    me = ga_nodeid_();

    gai_check_handle(g_src, "ga_scan_copy 1");
    gai_check_handle(g_dst, "ga_scan_copy 2");
    gai_check_handle(g_msk, "ga_scan_copy 3");

    if(!ga_compare_distr_(g_src, g_msk))
        gai_error("ga_scan_copy: different distribution src",0);
    if(!ga_compare_distr_(g_dst, g_msk))
        gai_error("ga_scan_copy: different distribution dst",0);

    nga_inquire_internal_(g_src, &type_src, &ndim, &dims);
    nga_inquire_internal_(g_dst, &type_dst, &ndim, &dims);
    nga_inquire_internal_(g_msk, &type_msk, &ndim, &dims);
    if(ndim>1)gai_error("ga_scan_copy: applicable to 1-dim arrays",ndim);
    if(g_src == g_dst) {
        gai_error("ga_scan_copy: src and dst must be different arrays", 0);
    }
    if(type_src != type_dst) {
        gai_error("ga_scan_copy: src and dst arrays must be same type", 0);
    }

    ga_sync_();

    nga_distribution_(g_msk, &me, &lop, &hip);

    /* create arrays to hold last bit set on a given process */
    lim = (long *) ga_malloc(nproc, MT_C_LONGINT, "ga scan buf");
    bzero(lim,sizeof(long)*nproc);
    lim[me] = -1;

    /* find last bit set on given process (store as global index) */
    if ( lop > 0 ){ /* we get 0 if no elements stored on this process */ 
        elems = hip - lop + 1;
        nga_access_ptr(g_msk, &lop, &hip, &ptr_msk, &ld);
        switch (type_msk) {
#define ga_scan_copy_case(MT,T,AT)                              \
            case MT:                                            \
                {                                               \
                    T * restrict buf = (T*)ptr_msk;             \
                    for(i=0; i<elems; i++) {                    \
                        if (neq_zero_##AT(buf[i])) {            \
                            ioff = i + lop;                     \
                            if (ioff >= *lo && ioff <= *hi) {   \
                                lim[me]= ioff;                  \
                            }                                   \
                        }                                       \
                    }                                           \
                    break;                                      \
                }
            ga_scan_copy_case(C_INT,int,reg)
            ga_scan_copy_case(C_LONG,long,reg)
            ga_scan_copy_case(C_LONGLONG,long long,reg)
            ga_scan_copy_case(C_FLOAT,float,reg)
            ga_scan_copy_case(C_DBL,double,reg)
            ga_scan_copy_case(C_SCPL,SingleComplex,cpl)
            ga_scan_copy_case(C_DCPL,DoubleComplex,cpl)
#undef ga_scan_copy_case
        }
        nga_release_(g_msk, &lop, &hip);
    }
    gai_gop(C_LONG,lim,nproc,"+");

    if(*hi <lop || hip <*lo) {
        /* we have no elements to update */
    }
    else {
        Integer rmt_idx, start, stop;

        if (lop < *lo) {
            start = *lo - lop;
        } else {
            start = 0;
        }
        if (hip > *hi) {
            stop = *hi-lop+1;
        } else {
            stop = hip-lop+1;
        }

        /* If start bit is not first local bit, find last remote bit.
         * We must scan the entire lim to find the next-highest index.
         * Otherwise, this algorithm won't work with restricted arrays? */
        rmt_idx = -1;
        for (i=0; i<nproc; i++) {
            if (-1 != lim[i] && lim[i] > rmt_idx && lim[i] < lop) {
                rmt_idx = lim[i];
            }
        }

        nga_access_ptr(g_src, &lop, &hip, &ptr_src, &ld);
        nga_access_ptr(g_dst, &lop, &hip, &ptr_dst, &ld);
        nga_access_ptr(g_msk, &lop, &hip, &ptr_msk, &ld);
        combined_type = MT_NUMTYPES*type_src + type_msk;
        switch (combined_type) {
#define ga_scan_copy_case(MT,T,AT,MT_MSK,T_MSK,AT_MSK)                      \
            case (MT_NUMTYPES*MT) + MT_MSK:                                 \
                {                                                           \
                    T * restrict src = (T*)ptr_src;                         \
                    T * restrict dst = (T*)ptr_dst;                         \
                    T_MSK * restrict msk = (T_MSK*)ptr_msk;                 \
                    T last_val;                                             \
                    assign_zero_##AT(last_val);                             \
                    if (-1 != rmt_idx) {                                    \
                        nga_get_(g_src, &rmt_idx, &rmt_idx, &last_val, &ld);\
                    }                                                       \
                    for (i=start; i<stop; i++) {                            \
                        if (neq_zero_##AT_MSK(msk[i])) {                    \
                            assign_##AT(last_val, src[i]);                  \
                        }                                                   \
                        assign_##AT(dst[i], last_val);                      \
                    }                                                       \
                    break;                                                  \
                }
            ga_scan_copy_case(C_INT,int,reg,           C_INT,int,reg)
            ga_scan_copy_case(C_LONG,long,reg,         C_INT,int,reg)
            ga_scan_copy_case(C_LONGLONG,long long,reg,C_INT,int,reg)
            ga_scan_copy_case(C_FLOAT,float,reg,       C_INT,int,reg)
            ga_scan_copy_case(C_DBL,double,reg,        C_INT,int,reg)
            ga_scan_copy_case(C_SCPL,SingleComplex,cpl,C_INT,int,reg)
            ga_scan_copy_case(C_DCPL,DoubleComplex,cpl,C_INT,int,reg)
            ga_scan_copy_case(C_INT,int,reg,           C_LONG,long,reg)
            ga_scan_copy_case(C_LONG,long,reg,         C_LONG,long,reg)
            ga_scan_copy_case(C_LONGLONG,long long,reg,C_LONG,long,reg)
            ga_scan_copy_case(C_FLOAT,float,reg,       C_LONG,long,reg)
            ga_scan_copy_case(C_DBL,double,reg,        C_LONG,long,reg)
            ga_scan_copy_case(C_SCPL,SingleComplex,cpl,C_LONG,long,reg)
            ga_scan_copy_case(C_DCPL,DoubleComplex,cpl,C_LONG,long,reg)
            ga_scan_copy_case(C_INT,int,reg,           C_LONGLONG,long long,reg)
            ga_scan_copy_case(C_LONG,long,reg,         C_LONGLONG,long long,reg)
            ga_scan_copy_case(C_LONGLONG,long long,reg,C_LONGLONG,long long,reg)
            ga_scan_copy_case(C_FLOAT,float,reg,       C_LONGLONG,long long,reg)
            ga_scan_copy_case(C_DBL,double,reg,        C_LONGLONG,long long,reg)
            ga_scan_copy_case(C_SCPL,SingleComplex,cpl,C_LONGLONG,long long,reg)
            ga_scan_copy_case(C_DCPL,DoubleComplex,cpl,C_LONGLONG,long long,reg)
            ga_scan_copy_case(C_INT,int,reg,           C_FLOAT,float,reg)
            ga_scan_copy_case(C_LONG,long,reg,         C_FLOAT,float,reg)
            ga_scan_copy_case(C_LONGLONG,long long,reg,C_FLOAT,float,reg)
            ga_scan_copy_case(C_FLOAT,float,reg,       C_FLOAT,float,reg)
            ga_scan_copy_case(C_DBL,double,reg,        C_FLOAT,float,reg)
            ga_scan_copy_case(C_SCPL,SingleComplex,cpl,C_FLOAT,float,reg)
            ga_scan_copy_case(C_DCPL,DoubleComplex,cpl,C_FLOAT,float,reg)
            ga_scan_copy_case(C_INT,int,reg,           C_DBL,double,reg)
            ga_scan_copy_case(C_LONG,long,reg,         C_DBL,double,reg)
            ga_scan_copy_case(C_LONGLONG,long long,reg,C_DBL,double,reg)
            ga_scan_copy_case(C_FLOAT,float,reg,       C_DBL,double,reg)
            ga_scan_copy_case(C_DBL,double,reg,        C_DBL,double,reg)
            ga_scan_copy_case(C_SCPL,SingleComplex,cpl,C_DBL,double,reg)
            ga_scan_copy_case(C_DCPL,DoubleComplex,cpl,C_DBL,double,reg)
            ga_scan_copy_case(C_INT,int,reg,           C_SCPL,SingleComplex,cpl)
            ga_scan_copy_case(C_LONG,long,reg,         C_SCPL,SingleComplex,cpl)
            ga_scan_copy_case(C_LONGLONG,long long,reg,C_SCPL,SingleComplex,cpl)
            ga_scan_copy_case(C_FLOAT,float,reg,       C_SCPL,SingleComplex,cpl)
            ga_scan_copy_case(C_DBL,double,reg,        C_SCPL,SingleComplex,cpl)
            ga_scan_copy_case(C_SCPL,SingleComplex,cpl,C_SCPL,SingleComplex,cpl)
            ga_scan_copy_case(C_DCPL,DoubleComplex,cpl,C_SCPL,SingleComplex,cpl)
            ga_scan_copy_case(C_INT,int,reg,           C_DCPL,DoubleComplex,cpl)
            ga_scan_copy_case(C_LONG,long,reg,         C_DCPL,DoubleComplex,cpl)
            ga_scan_copy_case(C_LONGLONG,long long,reg,C_DCPL,DoubleComplex,cpl)
            ga_scan_copy_case(C_FLOAT,float,reg,       C_DCPL,DoubleComplex,cpl)
            ga_scan_copy_case(C_DBL,double,reg,        C_DCPL,DoubleComplex,cpl)
            ga_scan_copy_case(C_SCPL,SingleComplex,cpl,C_DCPL,DoubleComplex,cpl)
            ga_scan_copy_case(C_DCPL,DoubleComplex,cpl,C_DCPL,DoubleComplex,cpl)
#undef ga_scan_copy_case
            default: gai_error("ga_scan_copy:wrong data type",combined_type);
        }
        /* release local access to arrays */
        nga_release_(g_src, &lop, &hip);
        nga_release_(g_msk, &lop, &hip);
        nga_release_update_(g_dst, &lop, &hip);
    }

    ga_sync_();
    ga_free(lim);
}


void FATR ga_scan_add_(Integer* g_src, Integer* g_dst, Integer* g_msk,
                       Integer* lo, Integer* hi, Integer* excl)
{
    long *lim=NULL;
    Integer i, nproc, me, elems, ioff, lop, hip, ndim, dims, ld;
    Integer type_src, type_dst, type_msk, combined_type;
    void *ptr_src=NULL;
    void *ptr_dst=NULL;
    void *ptr_msk=NULL;

    nproc = ga_nnodes_();
    me = ga_nodeid_();

    gai_check_handle(g_src, "ga_scan_add 1");
    gai_check_handle(g_dst, "ga_scan_add 2");
    gai_check_handle(g_msk, "ga_scan_add 3");

    if(!ga_compare_distr_(g_src, g_msk))
        gai_error("ga_scan_add: different distribution src",0);
    if(!ga_compare_distr_(g_dst, g_msk))
        gai_error("ga_scan_add: different distribution dst",0);

    nga_inquire_internal_(g_src, &type_src, &ndim, &dims);
    nga_inquire_internal_(g_dst, &type_dst, &ndim, &dims);
    nga_inquire_internal_(g_msk, &type_msk, &ndim, &dims);
    if(ndim>1)gai_error("ga_scan_add: applicable to 1-dim arrays",ndim);
    if(g_src == g_dst) {
        gai_error("ga_scan_add: src and dst must be different arrays", 0);
    }
    if(type_src != type_dst) {
        gai_error("ga_scan_add: src and dst arrays must be same type", 0);
    }

    ga_sync_();

    nga_distribution_(g_msk, &me, &lop, &hip);

    /* create arrays to hold last bit set on a given process */
    lim = (long *) ga_malloc(nproc, MT_C_LONGINT, "ga scan buf");
    bzero(lim,sizeof(long)*nproc);
    lim[me] = -1;

    /* find last bit set on given process (store as global index) */
    if ( lop > 0 ){ /* we get 0 if no elements stored on this process */ 
        elems = hip - lop + 1;
        nga_access_ptr(g_msk, &lop, &hip, &ptr_msk, &ld);
        switch (type_msk) {
#define ga_scan_add_case(MT,T,AT)                               \
            case MT:                                            \
                {                                               \
                    T * restrict buf = (T*)ptr_msk;             \
                    for(i=0; i<elems; i++) {                    \
                        if (neq_zero_##AT(buf[i])) {            \
                            ioff = i + lop;                     \
                            if (ioff >= *lo && ioff <= *hi) {   \
                                lim[me]= ioff;                  \
                            }                                   \
                        }                                       \
                    }                                           \
                    break;                                      \
                }
            ga_scan_add_case(C_INT,int,reg)
            ga_scan_add_case(C_LONG,long,reg)
            ga_scan_add_case(C_LONGLONG,long long,reg)
            ga_scan_add_case(C_FLOAT,float,reg)
            ga_scan_add_case(C_DBL,double,reg)
            ga_scan_add_case(C_SCPL,SingleComplex,cpl)
            ga_scan_add_case(C_DCPL,DoubleComplex,cpl)
#undef ga_scan_add_case
        }
        nga_release_(g_msk, &lop, &hip);
    }
    gai_gop(C_LONG,lim,nproc,"+");

    if(*hi <lop || hip <*lo) {
        /* we have no elements to update, but there are sync's in the else */
        ga_sync_();
        ga_sync_();
    }
    else {
        Integer rmt_idx, start, stop;

        /* find the nearest set bit on another process */
        rmt_idx = -1;
        for (i=0; i<nproc; i++) {
            if (-1 != lim[i] && lim[i] > rmt_idx && lim[i] < lop) {
                rmt_idx = lim[i];
            }
        }

        if (lop < *lo) {
            start = *lo - lop;
        } else {
            start = 0;
        }
        if (hip > *hi) {
            stop = *hi-lop+1;
        } else {
            stop = hip-lop+1;
        }

        /* first, perform local scan add */
        nga_access_ptr(g_src, &lop, &hip, &ptr_src, &ld);
        nga_access_ptr(g_dst, &lop, &hip, &ptr_dst, &ld);
        nga_access_ptr(g_msk, &lop, &hip, &ptr_msk, &ld);
        combined_type = MT_NUMTYPES*type_src + type_msk;
        switch (combined_type) {
#define ga_scan_add_case(MT,T,AT,MT_MSK,T_MSK,AT_MSK)                       \
            case (MT_NUMTYPES*MT) + MT_MSK:                                 \
                {                                                           \
                    T * restrict src = (T*)ptr_src;                         \
                    T * restrict dst = (T*)ptr_dst;                         \
                    T_MSK * restrict msk = (T_MSK*)ptr_msk;                 \
                    int found_bit = 0;                                      \
                    /* find first set bit on first segment */               \
                    if (lop <= *lo) {                                       \
                        while (eq_zero_##AT_MSK(msk[start]) && start<stop) {\
                            start++;                                        \
                        }                                                   \
                    }                                                       \
                    /* set first index then use for subsequent indices */   \
                    if (start < stop) {                                     \
                        i = start;                                          \
                        if (neq_zero_##AT_MSK(msk[i])) {                    \
                            found_bit = 1;                                  \
                            if (*excl != 0) {                               \
                                assign_zero_##AT(dst[i]);                   \
                            } else {                                        \
                                assign_##AT(dst[i], src[i]);                \
                            }                                               \
                        } else if (-1 != rmt_idx) {                         \
                            found_bit = 1;                                  \
                            if (*excl != 0) {                               \
                                Integer loc = i+lop - 1;                    \
                                if (loc > 0) {                              \
                                    nga_get_(g_src, &loc, &loc, &dst[i], NULL);\
                                }                                           \
                            } else {                                        \
                                assign_##AT(dst[i], src[i]);                \
                            }                                               \
                        }                                                   \
                    }                                                       \
                    if (*excl != 0) {                                       \
                        for (i=start+1; i<stop; i++) {                      \
                            if (neq_zero_##AT_MSK(msk[i])) {                \
                                assign_zero_##AT(dst[i]);                   \
                                found_bit = 1;                              \
                            } else if (1 == found_bit) {                    \
                                assign_add_##AT(dst[i], dst[i-1], src[i-1]);\
                            }                                               \
                        }                                                   \
                    } else {                                                \
                        for (i=start+1; i<stop; i++) {                      \
                            if (neq_zero_##AT_MSK(msk[i])) {                \
                                assign_##AT(dst[i], src[i]);                \
                                found_bit = 1;                              \
                            } else if (1 == found_bit) {                    \
                                assign_add_##AT(dst[i], dst[i-1], src[i]);  \
                            }                                               \
                        }                                                   \
                    }                                                       \
                    ga_sync_();                                             \
                    /* lastly, reconcile segment boundaries on other procs */\
                    if (eq_zero_##AT_MSK(msk[start]) && -1 != rmt_idx) {    \
                        Integer np, *map, *proclist, *subs, rmt_hi;         \
                        T *v, sum;                                          \
                        rmt_hi = lop-1;                                     \
                        nga_locate_nnodes_(g_dst, &rmt_idx, &rmt_hi, &np);  \
                        map = ga_malloc(4*np, MT_F_INT, "ga scan add locate");\
                        v = ga_malloc(np, MT, "ga scan add gather values"); \
                        proclist = map+(2*np);                              \
                        subs = map+(3*np);                                  \
                        nga_locate_region_(g_dst, &rmt_idx, &rmt_hi, map, proclist, &np);\
                        for (i=0; i<np; i++) {                              \
                            subs[i] = map[i*2+1];                           \
                        }                                                   \
                        nga_gather_(g_dst, v, subs, &np);                   \
                        ga_sync_();                                         \
                        assign_zero_##AT(sum);                              \
                        for (i=0; i<np; i++) {                              \
                            add_assign_##AT(sum, v[i]);                     \
                        }                                                   \
                        for (i=start; i<stop; i++) {                        \
                            if (eq_zero_##AT_MSK(msk[i])) {                 \
                                add_assign_##AT(dst[i], sum);               \
                            } else {                                        \
                                break;                                      \
                            }                                               \
                        }                                                   \
                        ga_free(v);                                         \
                        ga_free(map);                                       \
                    } else {                                                \
                        ga_sync_();                                         \
                    }                                                       \
                    break;                                                  \
                }
            ga_scan_add_case(C_INT,int,reg,           C_INT,int,reg)
            ga_scan_add_case(C_LONG,long,reg,         C_INT,int,reg)
            ga_scan_add_case(C_LONGLONG,long long,reg,C_INT,int,reg)
            ga_scan_add_case(C_FLOAT,float,reg,       C_INT,int,reg)
            ga_scan_add_case(C_DBL,double,reg,        C_INT,int,reg)
            ga_scan_add_case(C_SCPL,SingleComplex,cpl,C_INT,int,reg)
            ga_scan_add_case(C_DCPL,DoubleComplex,cpl,C_INT,int,reg)
            ga_scan_add_case(C_INT,int,reg,           C_LONG,long,reg)
            ga_scan_add_case(C_LONG,long,reg,         C_LONG,long,reg)
            ga_scan_add_case(C_LONGLONG,long long,reg,C_LONG,long,reg)
            ga_scan_add_case(C_FLOAT,float,reg,       C_LONG,long,reg)
            ga_scan_add_case(C_DBL,double,reg,        C_LONG,long,reg)
            ga_scan_add_case(C_SCPL,SingleComplex,cpl,C_LONG,long,reg)
            ga_scan_add_case(C_DCPL,DoubleComplex,cpl,C_LONG,long,reg)
            ga_scan_add_case(C_INT,int,reg,           C_LONGLONG,long long,reg)
            ga_scan_add_case(C_LONG,long,reg,         C_LONGLONG,long long,reg)
            ga_scan_add_case(C_LONGLONG,long long,reg,C_LONGLONG,long long,reg)
            ga_scan_add_case(C_FLOAT,float,reg,       C_LONGLONG,long long,reg)
            ga_scan_add_case(C_DBL,double,reg,        C_LONGLONG,long long,reg)
            ga_scan_add_case(C_SCPL,SingleComplex,cpl,C_LONGLONG,long long,reg)
            ga_scan_add_case(C_DCPL,DoubleComplex,cpl,C_LONGLONG,long long,reg)
            ga_scan_add_case(C_INT,int,reg,           C_FLOAT,float,reg)
            ga_scan_add_case(C_LONG,long,reg,         C_FLOAT,float,reg)
            ga_scan_add_case(C_LONGLONG,long long,reg,C_FLOAT,float,reg)
            ga_scan_add_case(C_FLOAT,float,reg,       C_FLOAT,float,reg)
            ga_scan_add_case(C_DBL,double,reg,        C_FLOAT,float,reg)
            ga_scan_add_case(C_SCPL,SingleComplex,cpl,C_FLOAT,float,reg)
            ga_scan_add_case(C_DCPL,DoubleComplex,cpl,C_FLOAT,float,reg)
            ga_scan_add_case(C_INT,int,reg,           C_DBL,double,reg)
            ga_scan_add_case(C_LONG,long,reg,         C_DBL,double,reg)
            ga_scan_add_case(C_LONGLONG,long long,reg,C_DBL,double,reg)
            ga_scan_add_case(C_FLOAT,float,reg,       C_DBL,double,reg)
            ga_scan_add_case(C_DBL,double,reg,        C_DBL,double,reg)
            ga_scan_add_case(C_SCPL,SingleComplex,cpl,C_DBL,double,reg)
            ga_scan_add_case(C_DCPL,DoubleComplex,cpl,C_DBL,double,reg)
            ga_scan_add_case(C_INT,int,reg,           C_SCPL,SingleComplex,cpl)
            ga_scan_add_case(C_LONG,long,reg,         C_SCPL,SingleComplex,cpl)
            ga_scan_add_case(C_LONGLONG,long long,reg,C_SCPL,SingleComplex,cpl)
            ga_scan_add_case(C_FLOAT,float,reg,       C_SCPL,SingleComplex,cpl)
            ga_scan_add_case(C_DBL,double,reg,        C_SCPL,SingleComplex,cpl)
            ga_scan_add_case(C_SCPL,SingleComplex,cpl,C_SCPL,SingleComplex,cpl)
            ga_scan_add_case(C_DCPL,DoubleComplex,cpl,C_SCPL,SingleComplex,cpl)
            ga_scan_add_case(C_INT,int,reg,           C_DCPL,DoubleComplex,cpl)
            ga_scan_add_case(C_LONG,long,reg,         C_DCPL,DoubleComplex,cpl)
            ga_scan_add_case(C_LONGLONG,long long,reg,C_DCPL,DoubleComplex,cpl)
            ga_scan_add_case(C_FLOAT,float,reg,       C_DCPL,DoubleComplex,cpl)
            ga_scan_add_case(C_DBL,double,reg,        C_DCPL,DoubleComplex,cpl)
            ga_scan_add_case(C_SCPL,SingleComplex,cpl,C_DCPL,DoubleComplex,cpl)
            ga_scan_add_case(C_DCPL,DoubleComplex,cpl,C_DCPL,DoubleComplex,cpl)
#undef ga_scan_add_case
            default: gai_error("ga_scan_add:wrong data type",combined_type);
        }
        /* release local access to arrays */
        nga_release_(g_src, &lop, &hip);
        nga_release_(g_msk, &lop, &hip);
        nga_release_update_(g_dst, &lop, &hip);
    }

    ga_sync_();
    ga_free(lim);
}


static void sga_pack(Integer first, long lim, Integer elems,
                     Integer type_src, Integer type_msk,
                     void *ptr_src, void *ptr_dst, void *ptr_msk)
{
    Integer combined_type, i, pck_idx=0;
    combined_type = MT_NUMTYPES*type_src + type_msk;
    switch (combined_type) {
#define ga_pack_case(MT,T,AT,MT_MSK,T_MSK,AT_MSK)                       \
        case (MT_NUMTYPES*MT) + MT_MSK:                                 \
            {                                                           \
                T * restrict pck = (T*)ptr_dst;                         \
                T * restrict src = (T*)ptr_src;                         \
                T_MSK * restrict msk = (T_MSK*)ptr_msk;                 \
                for (i=first; i<elems&&pck_idx<lim; i++) {              \
                    if (neq_zero_##AT_MSK(msk[i])) {                    \
                        assign_##AT(pck[pck_idx], src[i]);              \
                        ++pck_idx;                                      \
                    }                                                   \
                }                                                       \
                break;                                                  \
            }
        ga_pack_case(C_INT,int,reg,           C_INT,int,reg)
        ga_pack_case(C_LONG,long,reg,         C_INT,int,reg)
        ga_pack_case(C_LONGLONG,long long,reg,C_INT,int,reg)
        ga_pack_case(C_FLOAT,float,reg,       C_INT,int,reg)
        ga_pack_case(C_DBL,double,reg,        C_INT,int,reg)
        ga_pack_case(C_SCPL,SingleComplex,cpl,C_INT,int,reg)
        ga_pack_case(C_DCPL,DoubleComplex,cpl,C_INT,int,reg)
        ga_pack_case(C_INT,int,reg,           C_LONG,long,reg)
        ga_pack_case(C_LONG,long,reg,         C_LONG,long,reg)
        ga_pack_case(C_LONGLONG,long long,reg,C_LONG,long,reg)
        ga_pack_case(C_FLOAT,float,reg,       C_LONG,long,reg)
        ga_pack_case(C_DBL,double,reg,        C_LONG,long,reg)
        ga_pack_case(C_SCPL,SingleComplex,cpl,C_LONG,long,reg)
        ga_pack_case(C_DCPL,DoubleComplex,cpl,C_LONG,long,reg)
        ga_pack_case(C_INT,int,reg,           C_LONGLONG,long long,reg)
        ga_pack_case(C_LONG,long,reg,         C_LONGLONG,long long,reg)
        ga_pack_case(C_LONGLONG,long long,reg,C_LONGLONG,long long,reg)
        ga_pack_case(C_FLOAT,float,reg,       C_LONGLONG,long long,reg)
        ga_pack_case(C_DBL,double,reg,        C_LONGLONG,long long,reg)
        ga_pack_case(C_SCPL,SingleComplex,cpl,C_LONGLONG,long long,reg)
        ga_pack_case(C_DCPL,DoubleComplex,cpl,C_LONGLONG,long long,reg)
        ga_pack_case(C_INT,int,reg,           C_FLOAT,float,reg)
        ga_pack_case(C_LONG,long,reg,         C_FLOAT,float,reg)
        ga_pack_case(C_LONGLONG,long long,reg,C_FLOAT,float,reg)
        ga_pack_case(C_FLOAT,float,reg,       C_FLOAT,float,reg)
        ga_pack_case(C_DBL,double,reg,        C_FLOAT,float,reg)
        ga_pack_case(C_SCPL,SingleComplex,cpl,C_FLOAT,float,reg)
        ga_pack_case(C_DCPL,DoubleComplex,cpl,C_FLOAT,float,reg)
        ga_pack_case(C_INT,int,reg,           C_DBL,double,reg)
        ga_pack_case(C_LONG,long,reg,         C_DBL,double,reg)
        ga_pack_case(C_LONGLONG,long long,reg,C_DBL,double,reg)
        ga_pack_case(C_FLOAT,float,reg,       C_DBL,double,reg)
        ga_pack_case(C_DBL,double,reg,        C_DBL,double,reg)
        ga_pack_case(C_SCPL,SingleComplex,cpl,C_DBL,double,reg)
        ga_pack_case(C_DCPL,DoubleComplex,cpl,C_DBL,double,reg)
        ga_pack_case(C_INT,int,reg,           C_SCPL,SingleComplex,cpl)
        ga_pack_case(C_LONG,long,reg,         C_SCPL,SingleComplex,cpl)
        ga_pack_case(C_LONGLONG,long long,reg,C_SCPL,SingleComplex,cpl)
        ga_pack_case(C_FLOAT,float,reg,       C_SCPL,SingleComplex,cpl)
        ga_pack_case(C_DBL,double,reg,        C_SCPL,SingleComplex,cpl)
        ga_pack_case(C_SCPL,SingleComplex,cpl,C_SCPL,SingleComplex,cpl)
        ga_pack_case(C_DCPL,DoubleComplex,cpl,C_SCPL,SingleComplex,cpl)
        ga_pack_case(C_INT,int,reg,           C_DCPL,DoubleComplex,cpl)
        ga_pack_case(C_LONG,long,reg,         C_DCPL,DoubleComplex,cpl)
        ga_pack_case(C_LONGLONG,long long,reg,C_DCPL,DoubleComplex,cpl)
        ga_pack_case(C_FLOAT,float,reg,       C_DCPL,DoubleComplex,cpl)
        ga_pack_case(C_DBL,double,reg,        C_DCPL,DoubleComplex,cpl)
        ga_pack_case(C_SCPL,SingleComplex,cpl,C_DCPL,DoubleComplex,cpl)
        ga_pack_case(C_DCPL,DoubleComplex,cpl,C_DCPL,DoubleComplex,cpl)
    }
#undef ga_pack_case
}


static void sga_unpack(Integer first, long lim, Integer elems,
                       Integer type_src, Integer type_msk,
                       void *ptr_src, void *ptr_dst, void *ptr_msk)
{
    Integer combined_type, i, pck_idx=0;
    combined_type = MT_NUMTYPES*type_src + type_msk;
    switch (combined_type) {
#define ga_unpack_case(MT,T,AT,MT_MSK,T_MSK,AT_MSK)                     \
        case (MT_NUMTYPES*MT) + MT_MSK:                                 \
            {                                                           \
                T * restrict pck = (T*)ptr_src;                         \
                T * restrict dst = (T*)ptr_dst;                         \
                T_MSK * restrict msk = (T_MSK*)ptr_msk;                 \
                for (i=first; i<elems&&pck_idx<lim; i++) {              \
                    if (neq_zero_##AT_MSK(msk[i])) {                    \
                        assign_##AT(dst[i], pck[pck_idx]);              \
                        ++pck_idx;                                      \
                    }                                                   \
                }                                                       \
                break;                                                  \
            }
        ga_unpack_case(C_INT,int,reg,           C_INT,int,reg)
        ga_unpack_case(C_LONG,long,reg,         C_INT,int,reg)
        ga_unpack_case(C_LONGLONG,long long,reg,C_INT,int,reg)
        ga_unpack_case(C_FLOAT,float,reg,       C_INT,int,reg)
        ga_unpack_case(C_DBL,double,reg,        C_INT,int,reg)
        ga_unpack_case(C_SCPL,SingleComplex,cpl,C_INT,int,reg)
        ga_unpack_case(C_DCPL,DoubleComplex,cpl,C_INT,int,reg)
        ga_unpack_case(C_INT,int,reg,           C_LONG,long,reg)
        ga_unpack_case(C_LONG,long,reg,         C_LONG,long,reg)
        ga_unpack_case(C_LONGLONG,long long,reg,C_LONG,long,reg)
        ga_unpack_case(C_FLOAT,float,reg,       C_LONG,long,reg)
        ga_unpack_case(C_DBL,double,reg,        C_LONG,long,reg)
        ga_unpack_case(C_SCPL,SingleComplex,cpl,C_LONG,long,reg)
        ga_unpack_case(C_DCPL,DoubleComplex,cpl,C_LONG,long,reg)
        ga_unpack_case(C_INT,int,reg,           C_LONGLONG,long long,reg)
        ga_unpack_case(C_LONG,long,reg,         C_LONGLONG,long long,reg)
        ga_unpack_case(C_LONGLONG,long long,reg,C_LONGLONG,long long,reg)
        ga_unpack_case(C_FLOAT,float,reg,       C_LONGLONG,long long,reg)
        ga_unpack_case(C_DBL,double,reg,        C_LONGLONG,long long,reg)
        ga_unpack_case(C_SCPL,SingleComplex,cpl,C_LONGLONG,long long,reg)
        ga_unpack_case(C_DCPL,DoubleComplex,cpl,C_LONGLONG,long long,reg)
        ga_unpack_case(C_INT,int,reg,           C_FLOAT,float,reg)
        ga_unpack_case(C_LONG,long,reg,         C_FLOAT,float,reg)
        ga_unpack_case(C_LONGLONG,long long,reg,C_FLOAT,float,reg)
        ga_unpack_case(C_FLOAT,float,reg,       C_FLOAT,float,reg)
        ga_unpack_case(C_DBL,double,reg,        C_FLOAT,float,reg)
        ga_unpack_case(C_SCPL,SingleComplex,cpl,C_FLOAT,float,reg)
        ga_unpack_case(C_DCPL,DoubleComplex,cpl,C_FLOAT,float,reg)
        ga_unpack_case(C_INT,int,reg,           C_DBL,double,reg)
        ga_unpack_case(C_LONG,long,reg,         C_DBL,double,reg)
        ga_unpack_case(C_LONGLONG,long long,reg,C_DBL,double,reg)
        ga_unpack_case(C_FLOAT,float,reg,       C_DBL,double,reg)
        ga_unpack_case(C_DBL,double,reg,        C_DBL,double,reg)
        ga_unpack_case(C_SCPL,SingleComplex,cpl,C_DBL,double,reg)
        ga_unpack_case(C_DCPL,DoubleComplex,cpl,C_DBL,double,reg)
        ga_unpack_case(C_INT,int,reg,           C_SCPL,SingleComplex,cpl)
        ga_unpack_case(C_LONG,long,reg,         C_SCPL,SingleComplex,cpl)
        ga_unpack_case(C_LONGLONG,long long,reg,C_SCPL,SingleComplex,cpl)
        ga_unpack_case(C_FLOAT,float,reg,       C_SCPL,SingleComplex,cpl)
        ga_unpack_case(C_DBL,double,reg,        C_SCPL,SingleComplex,cpl)
        ga_unpack_case(C_SCPL,SingleComplex,cpl,C_SCPL,SingleComplex,cpl)
        ga_unpack_case(C_DCPL,DoubleComplex,cpl,C_SCPL,SingleComplex,cpl)
        ga_unpack_case(C_INT,int,reg,           C_DCPL,DoubleComplex,cpl)
        ga_unpack_case(C_LONG,long,reg,         C_DCPL,DoubleComplex,cpl)
        ga_unpack_case(C_LONGLONG,long long,reg,C_DCPL,DoubleComplex,cpl)
        ga_unpack_case(C_FLOAT,float,reg,       C_DCPL,DoubleComplex,cpl)
        ga_unpack_case(C_DBL,double,reg,        C_DCPL,DoubleComplex,cpl)
        ga_unpack_case(C_SCPL,SingleComplex,cpl,C_DCPL,DoubleComplex,cpl)
        ga_unpack_case(C_DCPL,DoubleComplex,cpl,C_DCPL,DoubleComplex,cpl)
    }
#undef ga_unpack_case
}


static void sga_pack_unpack(Integer* g_src, Integer* g_dst, Integer* g_msk,
                            Integer* lo, Integer* hi, Integer* icount,
                            Integer pack)
{
    void *ptr;
    long *lim=NULL;
    Integer nproc, me, i, myplace, np=0, first=-1;
    Integer lop, hip, dims;
    Integer ndim_src, ndim_dst, ndim_msk;
    Integer type_src, type_dst, type_msk;

    nproc = ga_nnodes_();
    me = ga_nodeid_();

    gai_check_handle(g_src, "ga_pack src");
    gai_check_handle(g_dst, "ga_pack dst");
    gai_check_handle(g_msk, "ga_pack msk");
    nga_inquire_internal_(g_src, &type_src, &ndim_src, &dims);
    nga_inquire_internal_(g_dst, &type_dst, &ndim_dst, &dims);
    nga_inquire_internal_(g_msk, &type_msk, &ndim_msk, &dims);
    if (1 != ndim_src) {
        gai_error("ga_pack: supports 1-dim arrays only: src", ndim_src);
    }
    if (1 != ndim_dst) {
        gai_error("ga_pack: supports 1-dim arrays only: dst", ndim_dst);
    }
    if (1 != ndim_msk) {
        gai_error("ga_pack: supports 1-dim arrays only: msk", ndim_msk);
    }
    if (type_src != type_dst) {
        gai_error("ga_pack: src and dst must be same type", 0);
    }
    if(!ga_compare_distr_(g_src, g_msk)) {
        gai_error("ga_pack: src and msk distributions differ",0);
    }

    ga_sync_();

    lim = (long*) ga_malloc(nproc, MT_C_LONGINT, "ga_pack lim buf");
    bzero(lim,sizeof(long)*nproc);
    nga_distribution_(g_msk, &me, &lop, &hip);

    /* how many elements do we have to copy? */
    if ( lop > 0 ){ /* we get 0 if no elements stored on this process */
        Integer lop_1 = lop-1;
        /* adjust the range of elements to be within <lo,hi> */
        if(lop < *lo) lop = *lo;
        if(hip > *hi) hip = *hi;
        if(*hi <lop || hip <*lo); /* we have no elements to update */
        else{
            Integer elems, ONE=1;
            nga_locate_nnodes_(g_msk, &ONE, &lop_1, &np);
            nga_access_ptr(g_msk, &lop, &hip, &ptr, &elems);
            elems = hip-lop+1;
            switch (type_msk) {
#define ga_pack_case(MT,T,AT) case MT:                                      \
                {                                                           \
                    T * restrict aptr = (T*)ptr;                            \
                    for (i=0; i<elems; i++) {                               \
                        if (neq_zero_##AT(aptr[i])) {                       \
                            if (first<0) first=i;                           \
                            lim[np]++;                                      \
                        }                                                   \
                    }                                                       \
                    break;                                                  \
                }
                ga_pack_case(C_INT,int,reg)
                ga_pack_case(C_LONG,long,reg)
                ga_pack_case(C_LONGLONG,long long,reg)
                ga_pack_case(C_FLOAT,float,reg)
                ga_pack_case(C_DBL,double,reg)
                ga_pack_case(C_SCPL,SingleComplex,cpl)
                ga_pack_case(C_DCPL,DoubleComplex,cpl)
            }
#undef ga_pack_case
        }
    }

    /* find number of elements everybody else is contributing */
    gac_lgop(lim, nproc, "+");

    for(i= myplace= *icount= 0; i<nproc; i++){
        if(i<np) myplace += lim[i];
        *icount += lim[i];
    }

    if(*hi<lop || hip<*lo || lim[np]==0 ); /* we have no elements to update */
    else{
        Integer ignore;
        void *buf=NULL, *msk=NULL;
        buf = ga_malloc(lim[np], type_dst, "ga pack buf");
        nga_access_ptr(g_msk, &lop, &hip, &msk, &ignore);
        if (0 != pack) {
            void *src=NULL;
            Integer dst_lo=myplace+1, dst_hi=myplace+lim[np];
            nga_access_ptr(g_src, &lop, &hip, &src, &ignore);
            sga_pack(first, lim[np], hip-lop+1,
                     type_src, type_msk, src, buf, msk);
            nga_put_(g_dst, &dst_lo, &dst_hi, buf, &ignore);
            nga_release_(g_src, &lop, &hip);
        } else {
            void *dst=NULL;
            Integer src_lo=myplace+1, src_hi=myplace+lim[np];
            nga_access_ptr(g_dst, &lop, &hip, &dst, &ignore);
            nga_get_(g_src, &src_lo, &src_hi, buf, &ignore);
            sga_unpack(first, lim[np], hip-lop+1,
                       type_src, type_msk, buf, dst, msk);
            nga_release_(g_dst, &lop, &hip);
        }
        ga_free(buf);
    }
    ga_free(lim);
    ga_sync_();
}


void FATR ga_pack_(Integer* g_src, Integer* g_dst, Integer* g_msk,
                   Integer* lo, Integer* hi, Integer* icount)
{
     sga_pack_unpack(g_src, g_dst, g_msk, lo, hi, icount, 1);
}


void FATR ga_unpack_(Integer* g_src, Integer* g_dst, Integer* g_msk,
                     Integer* lo, Integer* hi, Integer* icount)
{
     sga_pack_unpack(g_src, g_dst, g_msk, lo, hi, icount, 0);
}


#define NWORK 2000
int workR[NWORK], workL[NWORK];

/*\ compute offset for each of n bins for the given processor to contribute its
 *  elements, number of which for each bin is specified in x
\*/ 
void gai_bin_offset(int scope, int *x, int n, int *offset)
{
int root, up, left, right;
int len, lenmes, tag=32100, i, me=armci_msg_me();

    if(!x)gai_error("gai_bin_offset: NULL pointer", n);
    if(n>NWORK)gai_error("gai_bin_offset: >NWORK", n);
    len = sizeof(int)*n;

    armci_msg_bintree(scope, &root, &up, &left, &right);

    /* up-tree phase: collect number of elements */
    if (left > -1) armci_msg_rcv(tag, workL, len, &lenmes, left);
    if (right > -1) armci_msg_rcv(tag, workR, len, &lenmes, right);

    /* add number of elements in each bin */
    if((right > -1) && left>-1) for(i=0;i<n;i++)workL[i] += workR[i] +x[i];
    else if(left > -1) for(i=0;i<n;i++)workL[i] += x[i];
    else for(i=0;i<n;i++)workL[i] = x[i]; 

    /* now, workL on root contains the number of elements in each bin*/
         
    if (me != root && up!=-1) armci_msg_snd(tag, workL, len, up);

    /* down-tree: compute offset subtracting elements for self and right leaf*/
    if (me != root && up!=-1){
             armci_msg_rcv(tag, workL, len, &lenmes, up);
    }
    for(i=0; i<n; i++) offset[i] = workL[i]-x[i];

    if (right > -1) armci_msg_snd(tag, offset, len, right);
    if (left > -1) {
            /* we saved num elems for right subtree to adjust offset for left*/
            for(i=0; i<n; i++) workR[i] = offset[i] -workR[i]; 
            armci_msg_snd(tag, workR, len, left);
    }
/*    printf("%d:left=%d right=%d up=%d root=%d off=%d\n",me,left, right,up,root,offset[0]);
    fflush(stdout);
*/
}

static 
Integer gai_match_bin2proc(Integer blo, Integer bhi, Integer plo, Integer phi)
{
int rc=0;
       if(blo == plo) rc=1;
       if(bhi == phi) rc+=2; 
       return rc; /* 1 - first 2-last 3-last+first */
}


logical FATR ga_create_bin_range_(Integer *g_bin, Integer *g_cnt, Integer *g_off, Integer *g_range)
{
Integer type, ndim, nbin, lobin, hibin, me=ga_nodeid_(),crap;
Integer dims[2], nproc=ga_nnodes_(),chunk[2];

    nga_inquire_internal_(g_bin, &type, &ndim, &nbin);
    if(ndim !=1) gai_error("ga_bin_index: 1-dim array required",ndim);
    if(type!= C_INT && type!=C_LONG && type!=C_LONGLONG)
       gai_error("ga_bin_index: not integer type",type);

    chunk[0]=dims[0]=2; dims[1]=nproc; chunk[1]=1;
    if(!ngai_create(MT_F_INT, 2, dims, "bin_proc",chunk,g_range)) return FALSE;

    nga_distribution_(g_off,&me, &lobin,&hibin);

    if(lobin>0){ /* enter this block when we have data */
      Integer first_proc, last_proc, p;
      Integer first_off, last_off;
      Integer *myoff, bin;

      /* get offset values stored on my processor to first and last bin */
      nga_access_ptr(g_off, &lobin, &hibin, &myoff, &crap);
      first_off = myoff[0]; last_off = myoff[hibin-lobin];
/*
      nga_get_(g_off,&lobin,&lobin,&first_off,&lo);
      nga_get_(g_off,&hibin,&hibin,&last_off,&hi);
*/

      /* since offset starts at 0, add 1 to get index to g_bin */
      first_off++; last_off++;

      /* find processors on which these bins are located */
      if(!nga_locate_(g_bin, &first_off, &first_proc))
          gai_error("ga_bin_sorter: failed to locate region f",first_off);
      if(!nga_locate_(g_bin, &last_off, &last_proc))
          gai_error("ga_bin_sorter: failed to locate region l",last_off);

      /* inspect range of indices to bin elements stored on these processors */
      for(p=first_proc, bin=lobin; p<= last_proc; p++){
          Integer lo, hi, buf[2], off, cnt; 
          buf[0] =-1; buf[1]=-1;

          nga_distribution_(g_bin,&p,&lo,&hi);

          for(/* start from current bin */; bin<= hibin; bin++, myoff++){ 
              Integer blo,bhi,stat;

              blo = *myoff +1;
              if(bin == hibin){
                 nga_get_(g_cnt, &hibin, &hibin, &cnt, &hibin); /* local */
                 bhi = blo + cnt-1; 
              }else
                 bhi = myoff[1]; 

              stat= gai_match_bin2proc(blo, bhi, lo, hi);

              switch (stat) {
              case 0:  /* bin in a middle */ break;
              case 1:  /* first bin on that processor */
                       buf[0] =bin; break;
              case 2:  /* last bin on that processor */
                       buf[1] =bin; break;
              case 3:  /* first and last bin on that processor */
                       buf[0] =bin; buf[1] =bin; break;
              }

              if(stat>1)break; /* found last bin on that processor */
          }
          
          /* set range of bins on processor p */
          cnt =0; off=1;
          if(buf[0]!=-1){cnt=1; off=0;} 
          if(buf[1]!=-1)cnt++; 
          if(cnt){
                 Integer p1 = p+1;
                 lo = 1+off; hi = lo+cnt-1;
                 ga_put_(g_range,&lo,&hi,&p1, &p1, buf+off, &cnt);
          }
      }
   }
/*
   ga_print_(g_range);
   ga_print_(g_bin);
   ga_print_distribution_(g_bin);
*/
   return TRUE;
}


void FATR ga_bin_sorter_(Integer *g_bin, Integer *g_cnt, Integer *g_off)
{
extern void gai_hsort(Integer *list, int n);
Integer nbin,totbin,type,ndim,lo,hi,me=ga_nodeid_(),crap;
Integer g_range;

    if(FALSE==ga_create_bin_range_(g_bin, g_cnt, g_off, &g_range))
        gai_error("ga_bin_sorter: failed to create temp bin range array",0); 

    nga_inquire_internal_(g_bin, &type, &ndim, &totbin);
    if(ndim !=1) gai_error("ga_bin_sorter: 1-dim array required",ndim);
     
    nga_distribution_(g_bin, &me, &lo, &hi);
    if (lo > 0 ){ /* we get 0 if no elements stored on this process */
        Integer bin_range[2], rlo[2],rhi[2];
        Integer *bin_cnt, *ptr, i;

        /* get and inspect range of bins stored on current processor */
        rlo[0] = 1; rlo[1]= me+1; rhi[0]=2; rhi[1]=rlo[1];
        nga_get_(&g_range, rlo, rhi, bin_range, rhi); /* local */
        nbin = bin_range[1]-bin_range[0]+1;
        if(nbin<1 || nbin> totbin || nbin>(hi-lo+1))
           gai_error("ga_bin_sorter:bad nbin",nbin);

        /* get count of elements in each bin stored on this task */
        if(!(bin_cnt = (Integer*)malloc(nbin*sizeof(Integer))))
           gai_error("ga_bin_sorter:memory allocation failed",nbin);
        nga_get_(g_cnt,bin_range,bin_range+1,bin_cnt,&nbin);

        /* get access to local bin elements */
        nga_access_ptr(g_bin, &lo, &hi, &ptr, &crap);
        
        for(i=0;i<nbin;i++){ 
            int elems =(int) bin_cnt[i];
            gai_hsort(ptr, elems);
            ptr+=elems;
        }
        nga_release_update_(g_bin, &lo, &hi);             
    }

    ga_sync_();
}


/*\ note that subs values must be sorted; bins numbered from 1
\*/
void FATR ga_bin_index_(Integer *g_bin, Integer *g_cnt, Integer *g_off, 
                   Integer *values, Integer *subs, Integer *n, Integer *sortit)
{
int i, my_nbin=0;
int *all_bin_contrib, *offset;
Integer type, ndim, nbin;

    nga_inquire_internal_(g_bin, &type, &ndim, &nbin);
    if(ndim !=1) gai_error("ga_bin_index: 1-dim array required",ndim);
    if(type!= C_INT && type!=C_LONG && type!=C_LONGLONG)
       gai_error("ga_bin_index: not integer type",type);

    all_bin_contrib = (int*)calloc(nbin,sizeof(int));
    if(!all_bin_contrib)gai_error("ga_binning:calloc failed",nbin);
    offset = (int*)malloc(nbin*sizeof(int));
    if(!offset)gai_error("ga_binning:malloc failed",nbin);

    /* count how many elements go to each bin */
    for(i=0; i< *n; i++){
       int selected = subs[i];
       if(selected <1 || selected> nbin) gai_error("wrong bin",selected);

       if(all_bin_contrib[selected-1] ==0) my_nbin++; /* new bin found */
       all_bin_contrib[selected-1]++;
    }

    /* process bins in chunks to match available buffer space */
    for(i=0; i<nbin; i+=NWORK){
        int cnbin = ((i+NWORK)<nbin) ? NWORK: nbin -i;
        gai_bin_offset(SCOPE_ALL, all_bin_contrib+i, cnbin, offset+i);
    }

    for(i=0; i< *n; ){
       Integer lo, hi;
       Integer selected = subs[i];
       int elems = all_bin_contrib[selected-1];

       nga_get_(g_off,&selected,&selected, &lo, &selected);
       lo += offset[selected-1]+1;
       hi = lo + elems -1;
/*
       printf("%d: elems=%d lo=%d sel=%d off=%d contrib=%d nbin=%d\n",ga_nodeid_(), elems, lo, selected,offset[selected-1],all_bin_contrib[0],nbin);
*/
       if(lo > nbin) {
	      printf("Writing off end of bins array: index=%d elems=%d lo=%ld hi=%ld values=%ld nbin=%ld\n",
                i,elems,(long)lo,(long)hi,(long)values+i,(long)nbin);
         break;   
       }else{
          nga_put_(g_bin, &lo, &hi, values+i, &selected); 
       }
       i+=elems;
    }
    
    free(offset);
    free(all_bin_contrib);

    if(*sortit)ga_bin_sorter_(g_bin, g_cnt, g_off);
    else ga_sync_();
}

