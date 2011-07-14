#if HAVE_CONFIG_H
#   include "config.h"
#endif

/*$Id: matrix.c,v 1.7.8.11 2007/12/18 18:49:36 d3g293 Exp $******************************************************
File: matrix.c 

Author: Limin Zhang, Ph.D.
        Mathematics Department
        Columbia Basin College
        Pasco, WA 99301
        Limin.Zhang@cbc2.org
 
Mentor: Jarek Naplocha, Ph.D.
        Environmental Molecular Science Laboratory
        Richland, WA 99352
 
Date: 2/28/2002
 
Purpose:
      matrix interfaces between TAO and
      global arrays.
**************************************************************/

#include "global.h"
#include "globalp.h"
#if HAVE_MATH_H
#   include <math.h>
#endif
#if HAVE_STDLIB_H
#   include <stdlib.h>
#endif
#if HAVE_STRING_H
#   include <string.h>
#endif
#include "message.h"

#define auxi_median(a,b,c,m)                         \
{                                                    \
  if ((c < a) && (a < b))        m = a;              \
  else if ((a <= c) && (c <= b)) m = c;              \
  else                           m = b;              \
}                                                    

#define median(a,b,c,m)                              \
{                                                    \
  if(a == b)     m = a;                              \
  else if(a < b) auxi_median(a, b, c, m)             \
  else           auxi_median(b, a, c, m)             \
}

#define auxi_median_dcpl(na, nb, nc, za, zb, zc, zm) \
{                                                    \
   if ((nc < na) && (na < nb))       zm = za;        \
  else if ((na <= nc) && (nc <= nb)) zm = zc;        \
  else                               zm = zb;        \
}

#define median_dcpl(na, nb, nc, za, zb, zc, zm)                    \
{                                                                  \
  if (na == nb)     zm = za;                                       \
  else if (na < nb) auxi_median_dcpl (na, nb, nc, za, zb, zc, zm)  \
  else              auxi_median_dcpl (nb, na, nc, zb, za, zc, zm)  \
}

#define auxi_median_scpl(na, nb, nc, ca, cb, cc, cm) \
{                                                    \
   if ((nc < na) && (na < nb))       cm = ca;        \
  else if ((na <= nc) && (nc <= nb)) cm = cc;        \
  else                               cm = cb;        \
}

#define median_scpl(na, nb, nc, ca, cb, cc, cm)                    \
{                                                                  \
  if (na == nb)     cm = ca;                                       \
  else if (na < nb) auxi_median_scpl (na, nb, nc, ca, cb, cc, cm)  \
  else              auxi_median_scpl (nb, na, nc, cb, ca, cc, cm)  \
}

/*\ Utility function to find median values of patch
\*/
void gai_median_patch_values(Integer type, Integer ndim, Integer *loA,
                              Integer *hiA, Integer *ldA, void *A_ptr,
                              void *B_ptr, void *C_ptr, void *M_ptr,
                              Integer offset)
{
  Integer bvalue[MAXDIM], bunit[MAXDIM], baseldA[MAXDIM];
  Integer idx, n1dim;
  Integer i, j;

  double na, nb, nc;            /*norm of a, norm of b, norm of c */
  int ia, ib, ic, im;
  float fa, fb, fc, fm;
  double da, db, dc, dm;
  long la, lb, lc, lm;
  DoubleComplex za, zb, zc, zm;
  SingleComplex ca, cb, cc, cm;

  /* number of n-element of the first dimension */
  n1dim = 1;
  for (i = 1; i < ndim; i++)
    n1dim *= (hiA[i] - loA[i] + 1);

  /* calculate the destination indices */
  bvalue[0] = 0;
  bvalue[1] = 0;
  bunit[0] = 1;
  bunit[1] = 1;
  /* baseldA[0] = ldA[0]
   * baseldA[1] = ldA[0] * ldA[1]
   * baseldA[2] = ldA[0] * ldA[1] * ldA[2] .....
   */
  baseldA[0] = ldA[0];
  baseldA[1] = baseldA[0] * ldA[1];
  for (i = 2; i < ndim; i++)
  {
    bvalue[i] = 0;
    bunit[i] = bunit[i - 1] * (hiA[i - 1] - loA[i - 1] + 1);
    baseldA[i] = baseldA[i - 1] * ldA[i];
  }
  switch(type) {
    case C_DBL:
      A_ptr = (void*)((double*)(A_ptr) + offset);
      B_ptr = (void*)((double*)(B_ptr) + offset);
      C_ptr = (void*)((double*)(C_ptr) + offset);
      M_ptr = (void*)((double*)(M_ptr) + offset);
      break;
    case C_INT:
      A_ptr = (void*)((int*)(A_ptr) + offset);
      B_ptr = (void*)((int*)(B_ptr) + offset);
      C_ptr = (void*)((int*)(C_ptr) + offset);
      M_ptr = (void*)((int*)(M_ptr) + offset);
      break;
    case C_DCPL:
      A_ptr = (void*)((DoubleComplex*)(A_ptr) + offset);
      B_ptr = (void*)((DoubleComplex*)(B_ptr) + offset);
      C_ptr = (void*)((DoubleComplex*)(C_ptr) + offset);
      M_ptr = (void*)((DoubleComplex*)(M_ptr) + offset);
      break;
    case C_SCPL:
      A_ptr = (void*)((SingleComplex*)(A_ptr) + offset);
      B_ptr = (void*)((SingleComplex*)(B_ptr) + offset);
      C_ptr = (void*)((SingleComplex*)(C_ptr) + offset);
      M_ptr = (void*)((SingleComplex*)(M_ptr) + offset);
      break;
    case C_FLOAT:
      A_ptr = (void*)((float*)(A_ptr) + offset);
      B_ptr = (void*)((float*)(B_ptr) + offset);
      C_ptr = (void*)((float*)(C_ptr) + offset);
      M_ptr = (void*)((float*)(M_ptr) + offset);
      break;
    case C_LONG:
      A_ptr = (void*)((long*)(A_ptr) + offset);
      B_ptr = (void*)((long*)(B_ptr) + offset);
      C_ptr = (void*)((long*)(C_ptr) + offset);
      M_ptr = (void*)((long*)(M_ptr) + offset);
      break;
    case C_LONGLONG:
      A_ptr = (void*)((long long*)(A_ptr) + offset);
      B_ptr = (void*)((long long*)(B_ptr) + offset);
      C_ptr = (void*)((long long*)(C_ptr) + offset);
      M_ptr = (void*)((long long*)(M_ptr) + offset);
      break;
    default:
      break;
  }

  /*compute elementwise median */

  switch (type) {

    case C_INT: 
      for (i = 0; i < n1dim; i++) {
        idx = 0;
        for (j = 1; j < ndim; j++) {
          idx += bvalue[j] * baseldA[j - 1];
          if (((i + 1) % bunit[j]) == 0)     bvalue[j]++;
          if (bvalue[j] > (hiA[j] - loA[j])) bvalue[j] = 0;
        }
        for (j = 0; j < (hiA[0] - loA[0] + 1); j++) {
          ia = ((int *) A_ptr)[idx + j];
          ib = ((int *) B_ptr)[idx + j];
          ic = ((int *) C_ptr)[idx + j];
          median(ia, ib, ic, im);
          ((int *) M_ptr)[idx + j] = im;
        }
      }
      break;
    case C_LONG: 
      for (i = 0; i < n1dim; i++) {
        idx = 0;
        for (j = 1; j < ndim; j++) {
          idx += bvalue[j] * baseldA[j - 1];
          if (((i + 1) % bunit[j]) == 0)     bvalue[j]++;
          if (bvalue[j] > (hiA[j] - loA[j])) bvalue[j] = 0;
        }
        for (j = 0; j < (hiA[0] - loA[0] + 1); j++) {
          la = ((long *) A_ptr)[idx + j];
          lb = ((long *) B_ptr)[idx + j];
          lc = ((long *) C_ptr)[idx + j];
          median(la, lb, lc, lm);
          ((long *) M_ptr)[idx + j] = lm;
        }
      }
      break;
    case C_FLOAT: 
      for (i = 0; i < n1dim; i++) {
        idx = 0;
        for (j = 1; j < ndim; j++) {
          idx += bvalue[j] * baseldA[j - 1];
          if (((i + 1) % bunit[j]) == 0)     bvalue[j]++;
          if (bvalue[j] > (hiA[j] - loA[j])) bvalue[j] = 0;
        }
        for (j = 0; j < (hiA[0] - loA[0] + 1); j++) {
          fa = ((float *) A_ptr)[idx + j];
          fb = ((float *) B_ptr)[idx + j];
          fc = ((float *) C_ptr)[idx + j];
          median(fa, fb, fc, fm);
          ((float *) M_ptr)[idx + j] = fm;
        }
      }
      break;
    case C_DBL: 
      for (i = 0; i < n1dim; i++) {
        idx = 0;
        for (j = 1; j < ndim; j++) {
          idx += bvalue[j] * baseldA[j - 1];
          if (((i + 1) % bunit[j]) == 0)     bvalue[j]++;
          if (bvalue[j] > (hiA[j] - loA[j])) bvalue[j] = 0;
        }
        for (j = 0; j < (hiA[0] - loA[0] + 1); j++) {
          da = ((double *) A_ptr)[idx + j];
          db = ((double *) B_ptr)[idx + j];
          dc = ((double *) C_ptr)[idx + j];
          median(da, db, dc, dm);
          ((double *) M_ptr)[idx + j] = dm;
        }
      }
      break;
    case C_DCPL:
      for (i = 0; i < n1dim; i++) {
        idx = 0;
        for (j = 1; j < ndim; j++) {
          idx += bvalue[j] * baseldA[j - 1];
          if (((i + 1) % bunit[j]) == 0)     bvalue[j]++;
          if (bvalue[j] > (hiA[j] - loA[j])) bvalue[j] = 0;
        }
        for (j = 0; j < (hiA[0] - loA[0] + 1); j++) {
          za = ((DoubleComplex *) A_ptr)[idx + j];
          zb = ((DoubleComplex *) B_ptr)[idx + j];
          zc = ((DoubleComplex *) C_ptr)[idx + j];
          na = sqrt ((za.real) * (za.real) + (za.imag) * (za.imag));
          nb = sqrt ((zb.real) * (zb.real) + (zb.imag) * (zb.imag));
          nc = sqrt ((zc.real) * (zc.real) + (zc.imag) * (zc.imag));
          median_dcpl(na, nb, nc, za, zb, zc, zm);
          ((DoubleComplex *) M_ptr)[idx + j] = zm;
        }
      }
      break;

    case C_SCPL:
      for (i = 0; i < n1dim; i++) {
        idx = 0;
        for (j = 1; j < ndim; j++) {
          idx += bvalue[j] * baseldA[j - 1];
          if (((i + 1) % bunit[j]) == 0)     bvalue[j]++;
          if (bvalue[j] > (hiA[j] - loA[j])) bvalue[j] = 0;
        }
        for (j = 0; j < (hiA[0] - loA[0] + 1); j++) {
          ca = ((SingleComplex *) A_ptr)[idx + j];
          cb = ((SingleComplex *) B_ptr)[idx + j];
          cc = ((SingleComplex *) C_ptr)[idx + j];
          na = sqrt ((ca.real) * (ca.real) + (ca.imag) * (ca.imag));
          nb = sqrt ((cb.real) * (cb.real) + (cb.imag) * (cb.imag));
          nc = sqrt ((cc.real) * (cc.real) + (cc.imag) * (cc.imag));
          median_scpl(na, nb, nc, ca, cb, cc, cm);
          ((SingleComplex *) M_ptr)[idx + j] = cm;
        }
      }
      break;
    default:
      gai_error("median: wrong data type", type);
  }
}

/*\ median routine
\*/
void FATR
ga_median_patch_ (g_a, alo, ahi, g_b, blo, bhi, g_c, clo, chi, g_m, mlo, mhi)
     Integer *g_a, *alo, *ahi;  /* patch of g_a */
     Integer *g_b, *blo, *bhi;  /* patch of g_b */
     Integer *g_c, *clo, *chi;  /* patch of g_c */
     Integer *g_m, *mlo, *mhi;  /* patch of g_m */
{
  Integer i, j;
  Integer atype, btype, andim, adims[MAXDIM], bndim, bdims[MAXDIM];
  Integer ctype, mtype, cndim, cdims[MAXDIM], mndim, mdims[MAXDIM];
  Integer loA[MAXDIM], hiA[MAXDIM], ldA[MAXDIM];
  Integer loB[MAXDIM], hiB[MAXDIM], ldB[MAXDIM];
  Integer loC[MAXDIM], hiC[MAXDIM], ldC[MAXDIM];
  Integer loM[MAXDIM], hiM[MAXDIM], ldM[MAXDIM];
  Integer g_A = *g_a, g_B = *g_b;
  Integer g_C = *g_c, g_M = *g_m;
  void *A_ptr, *B_ptr;
  void *C_ptr, *M_ptr;
  Integer offset;
  Integer atotal, btotal;
  Integer ctotal, mtotal;
  Integer me = ga_nodeid_ (), a_temp_created = 0, b_temp_created = 0, c_temp_created = 0;
  Integer type = GA_TYPE_GSM, compatible;
  char *tempname = "temp", transp = 'n';        /*no transpose */
  Integer num_blocks_a, num_blocks_b, num_blocks_c, num_blocks_m;
  int local_sync_begin,local_sync_end;

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  if(local_sync_begin)ga_sync_();

  GA_PUSH_NAME ("ga_median_patch_");

  nga_inquire_ (g_a, &atype, &andim, adims);
  nga_inquire_ (g_b, &btype, &bndim, bdims);
  nga_inquire_ (g_c, &ctype, &cndim, cdims);
  nga_inquire_ (g_m, &mtype, &mndim, mdims);

  /* I have to inquire the data type again since nga_inquire and
   * nga_inquire_internal_ treat data type differently */
  nga_inquire_internal_ (g_m, &type, &mndim, mdims);

  if (mtype != atype)
    gai_error(" ga_median_patch_:type mismatch ", 0L);
  if (mtype != btype)
    gai_error(" ga_median_patch_:type mismatch ", 0L);
  if (mtype != ctype)
    gai_error(" ga_median_patch_:type mismatch ", 0L);

  /* check if patch indices and g_a dims match */
  for (i = 0; i < andim; i++)
    if (alo[i] <= 0 || ahi[i] > adims[i])
    {
      gai_error("ga_median_patch_: g_a indices out of range ", *g_a);
    }

  for (i = 0; i < bndim; i++)
    if (blo[i] <= 0 || bhi[i] > bdims[i])
    {
      gai_error("ga_median_patch_:g_b indices out of range ", *g_b);
    }

  for (i = 0; i < cndim; i++)
    if (clo[i] <= 0 || chi[i] > cdims[i])
    {
      gai_error("ga_median_patch_:g_c indices out of range ", *g_c);
    }         
  for (i = 0; i < mndim; i++)
    if (mlo[i] <= 0 || mhi[i] > mdims[i])
    {
      gai_error("ga_median_patch_:g_m indices out of range ", *g_m);
    }

  /* check if numbers of elements in two patches match each other */

  atotal = 1;
  for (i = 0; i < andim; i++)
    atotal *= (ahi[i] - alo[i] + 1);

  btotal = 1;
  for (i = 0; i < bndim; i++)
    btotal *= (bhi[i] - blo[i] + 1);

  ctotal = 1;
  for (i = 0; i < cndim; i++)
    ctotal *= (chi[i] - clo[i] + 1);

  mtotal = 1;
  for (i = 0; i < mndim; i++)
    mtotal *= (mhi[i] - mlo[i] + 1);


  if (mtotal != atotal)
    gai_error("ga_median_patch_:  capacities of patches do not match ", 0L);

  if (mtotal != btotal)
    gai_error("ga_median_patch_:  capacities of patches do not match ", 0L);

  if (mtotal != ctotal)
    gai_error("ga_median_patch_:  capacities of patches do not match ", 0L);

  num_blocks_a = ga_total_blocks_(g_a);
  num_blocks_b = ga_total_blocks_(g_b);
  num_blocks_c = ga_total_blocks_(g_c);
  num_blocks_m = ga_total_blocks_(g_m);

  if (num_blocks_a < 0 && num_blocks_b < 0 &&
      num_blocks_c < 0 && num_blocks_m < 0) {
    /* find out coordinates of patches of g_A, g_B, g_C, and g_M that I own */
    nga_distribution_ (&g_A, &me, loA, hiA);
    nga_distribution_ (&g_B, &me, loB, hiB);
    nga_distribution_ (&g_C, &me, loC, hiC);
    nga_distribution_ (&g_M, &me, loM, hiM);

    if (!ngai_comp_patch (andim, loA, hiA, mndim, loM, hiM)) compatible = 1;
    else compatible = 0;
    gai_igop(GA_TYPE_GSM, &compatible, 1, "*");
    if (!compatible) {
      /* either patches or distributions do not match:
       *        - create a temp array that matches distribution of g_a
       *        - copy & reshape patch of g_b into g_B
       */
      if (!gai_duplicate(g_m, &g_A, tempname))
        gai_error("ga_median_patch_:duplicate failed", 0L);

      ngai_copy_patch(&transp, g_a, alo, ahi, &g_A, mlo, mhi);
      andim = mndim;
      a_temp_created = 1;
      nga_distribution_ (&g_A, &me, loA, hiA);
    }

    if (!ngai_comp_patch (bndim, loB, hiB, mndim, loM, hiM)) compatible = 1;
    else compatible = 0;
    gai_igop(GA_TYPE_GSM, &compatible, 1, "*");
    if (!compatible) {
      /* either patches or distributions do not match:
       *        - create a temp array that matches distribution of g_a
       *        - copy & reshape patch of g_c into g_C
       */
      if (!gai_duplicate(g_m, &g_B, tempname))
        gai_error("ga_median_patch_:duplicate failed", 0L);

      ngai_copy_patch(&transp, g_b, blo, bhi, &g_B, mlo, mhi);
      bndim = mndim;
      b_temp_created = 1;
      nga_distribution_ (&g_B, &me, loB, hiB);
    }

    if (!ngai_comp_patch (cndim, loC, hiC, mndim, loM, hiM)) compatible = 1;
    else compatible = 0;
    gai_igop(GA_TYPE_GSM, &compatible, 1, "*");
    if (!compatible) {
      /* either patches or distributions do not match:
       *        - create a temp array that matches distribution of g_a
       *        - copy & reshape patch of g_m into g_M
       */
      if (!gai_duplicate(g_m, &g_C, tempname))
        gai_error("ga_median_patch_:duplicate failed", 0L);

      /*no need to copy g_m since it is the output matrix. */
      cndim = mndim;
      c_temp_created = 1;
      ngai_copy_patch(&transp, g_c, clo, chi, &g_C, mlo, mhi);
      nga_distribution_ (&g_C, &me, loC, hiC);
    }


    if (!ngai_comp_patch (mndim, loM, hiM, andim, loA, hiA))
      gai_error(" patches mismatch ", 0);
    if (!ngai_comp_patch (mndim, loM, hiM, bndim, loB, hiB))
      gai_error(" patches mismatch ", 0);
    if (!ngai_comp_patch (mndim, loM, hiM, cndim, loC, hiC))
      gai_error(" patches mismatch ", 0);


    /* A[83:125,1:1]  <==> B[83:125] */
    if (mndim > andim)
      mndim = andim;            /* need more work */
    if (mndim > bndim)
      mndim = bndim;            /* need more work */
    if (mndim > cndim)
      mndim = cndim;            /* need more work */


    /*  determine subsets of my patches to access  */
    if (ngai_patch_intersect (mlo, mhi, loM, hiM, mndim)) {
      offset = 0;
      nga_access_ptr (&g_A, loM, hiM, &A_ptr, ldA);
      nga_access_ptr (&g_B, loM, hiM, &B_ptr, ldB);
      nga_access_ptr (&g_C, loM, hiM, &C_ptr, ldC);
      nga_access_ptr (&g_M, loM, hiM, &M_ptr, ldM);

      gai_median_patch_values(type, mndim, loM, hiM, ldM,
          A_ptr, B_ptr, C_ptr, M_ptr, offset);

      /* release access to the data */
      nga_release_ (&g_A, loM, hiM);
      nga_release_ (&g_B, loM, hiM);
      nga_release_ (&g_C, loM, hiM);
      nga_release_update_ (&g_M, loM, hiM);
    }
  } else {
    /* create copies of A, B, and C that are identically distributed
       as M */
    if (!gai_duplicate(g_m, &g_A, tempname))
      gai_error("ga_add_patch: dup failed", 0L);
    ngai_copy_patch(&transp, g_a, alo, ahi, &g_A, mlo, mhi);
    andim = mndim;
    a_temp_created = 1;

    if (!gai_duplicate(g_m, &g_B, tempname))
      gai_error("ga_add_patch: dup failed", 0L);
    ngai_copy_patch(&transp, g_b, blo, bhi, &g_B, mlo, mhi);
    bndim = mndim;
    b_temp_created = 1;

    if (!gai_duplicate(g_m, &g_C, tempname))
      gai_error("ga_add_patch: dup failed", 0L);
        ngai_copy_patch(&transp, g_c, clo, chi, &g_C, mlo, mhi);
    cndim = mndim;
    c_temp_created = 1;

    /* M is normally distributed so just get the mean using standard approach */
    if (num_blocks_m < 0) {
      offset = 0;
      nga_distribution_(g_m, &me, loM, hiM);
      if (ngai_patch_intersect (mlo, mhi, loM, hiM, mndim)) {
        nga_access_ptr (&g_A, loM, hiM, &A_ptr, ldA);
        nga_access_ptr (&g_B, loM, hiM, &B_ptr, ldB);
        nga_access_ptr (&g_C, loM, hiM, &C_ptr, ldC);
        nga_access_ptr (&g_M, loM, hiM, &M_ptr, ldM);

        gai_median_patch_values(type, mndim, loM, hiM, ldM,
            A_ptr, B_ptr, C_ptr, M_ptr, offset);

        /* release access to the data */
        nga_release_ (&g_A, loM, hiM);
        nga_release_ (&g_B, loM, hiM);
        nga_release_ (&g_C, loM, hiM);
        nga_release_update_ (&g_M, loM, hiM);
      }
    } else {
      Integer idx, lod[MAXDIM], hid[MAXDIM];
      Integer jtot, last, nproc;
      /* Simple block-cyclic data distribution */
      if (!ga_uses_proc_grid_(g_m)) {
        nproc = ga_nnodes_();
        for (idx = me; idx < num_blocks_m; idx += nproc) {
          nga_distribution_(g_m, &idx, loM, hiM);
          /* make temporary copies of loM and hiM since ngai_patch_intersect
             destroys original versions */
          for (j=0; j<mndim; j++) {
            lod[j] = loM[j];
            hid[j] = hiM[j];
          }
          if (ngai_patch_intersect(mlo, mhi, loM, hiM, mndim)) {
            nga_access_block_ptr(&g_A, &idx, &A_ptr, ldA);
            nga_access_block_ptr(&g_B, &idx, &B_ptr, ldB);
            nga_access_block_ptr(&g_C, &idx, &C_ptr, ldC);
            nga_access_block_ptr( g_m, &idx, &M_ptr, ldM);

            /* evaluate offsets for system */
            offset = 0;
            last = mndim - 1;
            jtot = 1;
            for (j=0; j<last; j++) {
              offset += (loM[j] - lod[j])*jtot;
              jtot *= ldM[j];
            }
            offset += (loM[last]-lod[last])*jtot;

            gai_median_patch_values(type, mndim, loM, hiM, ldM,
                A_ptr, B_ptr, C_ptr, M_ptr, offset);

            /* release access to the data */
            nga_release_block_ (&g_A, &idx);
            nga_release_block_ (&g_B, &idx);
            nga_release_block_ (&g_C, &idx);
            nga_release_update_block_ (&g_M, &idx);
          }
        }
      } else {
        /* Uses scalapack block-cyclic data distribution */
        Integer lod[MAXDIM], hid[MAXDIM], chk;
        Integer proc_index[MAXDIM], index[MAXDIM];
        Integer topology[MAXDIM];
        Integer blocks[MAXDIM], block_dims[MAXDIM];
        ga_get_proc_index_(g_m, &me, proc_index);
        ga_get_proc_index_(g_m, &me, index);
        ga_get_block_info_(g_m, blocks, block_dims);
        ga_get_proc_grid_(g_m, topology);

        while (index[mndim-1] < blocks[mndim-1]) {
          /* find bounding coordinates of block */
          chk = 1;
          for (i = 0; i < mndim; i++) {
            loM[i] = index[i]*block_dims[i]+1;
            hiM[i] = (index[i] + 1)*block_dims[i];
            if (hiM[i] > mdims[i]) hiM[i] = mdims[i];
            if (hiM[i] < loM[i]) chk = 0;
          }
          /* make temporary copies of loC and hiC since ngai_patch_intersect
             destroys original versions */
          for (j=0; j<mndim; j++) {
            lod[j] = loM[j];
            hid[j] = hiM[j];
          }
          if (ngai_patch_intersect(mlo, mhi, loM, hiM, mndim)) {
            nga_access_block_grid_ptr(&g_A, index, &A_ptr, ldA);
            nga_access_block_grid_ptr(&g_B, index, &B_ptr, ldB);
            nga_access_block_grid_ptr(&g_C, index, &C_ptr, ldC);
            nga_access_block_grid_ptr( g_m, index, &M_ptr, ldM);

            /* evaluate offsets for system */
            offset = 0;
            last = mndim - 1;
            jtot = 1;
            for (j=0; j<last; j++) {
              offset += (loM[j] - lod[j])*jtot;
              jtot *= ldM[j];
            }
            offset += (loM[last]-lod[last])*jtot;

            gai_median_patch_values(type, mndim, loM, hiM, ldM,
                A_ptr, B_ptr, C_ptr, M_ptr, offset);

            /* release access to the data */
            nga_release_block_grid_ (&g_A, index);
            nga_release_block_grid_ (&g_B, index);
            nga_release_block_grid_ (&g_C, index);
            nga_release_update_block_grid_ (&g_M, index);
          }

          /* increment index to get next block on processor */
          index[0] += topology[0];
          for (i = 0; i < mndim; i++) {
            if (index[i] >= blocks[i] && i<mndim-1) {
              index[i] = proc_index[i];
              index[i+1] += topology[i+1];
            }
          }
        }
      }
    }
  }
  if (a_temp_created)
    ga_destroy_ (&g_A);
  if (b_temp_created)
    ga_destroy_ (&g_B);
  if (c_temp_created)
    ga_destroy_ (&g_C);
  GA_POP_NAME;
  if(local_sync_end)ga_sync_();
}



void FATR
ga_median_ (Integer * g_a, Integer * g_b, Integer * g_c, Integer * g_m){


   Integer atype, andim;
   Integer alo[MAXDIM],ahi[MAXDIM];
   Integer btype, bndim;
   Integer blo[MAXDIM],bhi[MAXDIM];
   Integer ctype, cndim;
   Integer clo[MAXDIM],chi[MAXDIM];
   Integer mtype, mndim;
   Integer mlo[MAXDIM],mhi[MAXDIM];

    nga_inquire_internal_(g_a,  &atype, &andim, ahi);
    nga_inquire_internal_(g_b,  &btype, &bndim, bhi);
    nga_inquire_internal_(g_c,  &ctype, &cndim, chi);
    nga_inquire_internal_(g_m,  &mtype, &mndim, mhi);

    while(andim){
        alo[andim-1]=1;
        andim--;
    }

    while(bndim){
        blo[bndim-1]=1;
        bndim--;
    }

    while(cndim){
        clo[cndim-1]=1;
        cndim--;
    }

    while(mndim){
        mlo[mndim-1]=1;
        mndim--;
    }
    _ga_sync_begin = 1;
    ga_median_patch_(g_a, alo, ahi, g_b, blo, bhi, g_c, clo, chi, g_m, mlo, mhi);

}

void gai_norm_infinity_block(Integer *g_a, void *ptr,
                             Integer *lo, Integer *hi, Integer ld,
                             Integer type,
                             Integer ndim, Integer *dims, void *buf)
{
  Integer size, nelem, dim2;
  Integer iloA=0, ihiA=0, jloA=0, jhiA=0;
  Integer i, j;
  int *isum = NULL;
  long *lsum = NULL;
  double *dsum = NULL;
  float *fsum = NULL;
  DoubleComplex *zsum = NULL;
  SingleComplex *csum = NULL;

  if (ndim == 1)
    dim2 = 1;
  else if (ndim == 2)
    dim2 = dims[1];

  size = GAsizeof(type);
  nelem = dim2;

  switch (type)
  {
    case C_INT:
      isum = (int *) buf;
      break;
    case C_LONG:
      lsum = (long *) buf;
      break;
    case C_FLOAT:
      fsum = (float *) buf;
      break;
    case C_DBL:
      dsum = (double *) buf;
      break;
    case C_DCPL:
      zsum = (DoubleComplex *) buf;
      break;
    case C_SCPL:
      csum = (SingleComplex *) buf;
      break;
    default:
      gai_error("ga_norm_infinity_: wrong data type:", type);
  }

  if(ndim<=0)
    gai_error("ga_norm_infinity: wrong dimension", ndim);
  else if(ndim == 1){
    iloA=lo[0];
    ihiA=hi[0];
    jloA=1;
    jhiA=1;
  }
  else if(ndim == 2)
  {
    iloA=lo[0];
    ihiA=hi[0];
    jloA=lo[1];
    jhiA=hi[1];
  }
  else
    gai_error("ga_norm_infinity: wrong dimension", ndim);

  /* determine subset of my patch to access */
  if (ihiA > 0 && jhiA > 0)
  {
    /* lo[0] = iloA; */
    /* lo[1] = jloA; */
    /* hi[0] = ihiA; */
    /* hi[1] = jhiA; */

    switch (type)
    {
      int *pi;
      double *pd;
      long *pl;
      float *pf;
      DoubleComplex *pz;
      SingleComplex *pc;
      case C_INT:
      pi = (int *) ptr;
      for (i = 0; i < ihiA - iloA + 1; i++)
        for (j = 0; j < jhiA - jloA + 1; j++)
          isum[iloA + i - 1] += GA_ABS (pi[j * ld + i]);
      break;
      case C_LONG:
      pl = (long *) ptr;
      for (i = 0; i < ihiA - iloA + 1; i++)
        for (j = 0; j < jhiA - jloA + 1; j++)
          lsum[iloA + i - 1] += GA_ABS (pl[j * ld + i]);
      break;
      case C_DCPL:
      pz = (DoubleComplex *) ptr;
      for (i = 0; i < ihiA - iloA + 1; i++)
        for (j = 0; j < jhiA - jloA + 1; j++)
        {
          DoubleComplex zval = pz[j * ld + i];
          double temp =
            sqrt (zval.real * zval.real + zval.imag * zval.imag);
          (zsum[iloA + i - 1]).real += temp;
        }
      break;
      case C_SCPL:
      pc = (SingleComplex *) ptr;
      for (i = 0; i < ihiA - iloA + 1; i++)
        for (j = 0; j < jhiA - jloA + 1; j++)
        {
          SingleComplex cval = pc[j * ld + i];
          float  temp =
            sqrt (cval.real * cval.real + cval.imag * cval.imag);
          (csum[iloA + i - 1]).real += temp;
        }
      break;
      case C_FLOAT:
      pf = (float *) ptr;
      for (i = 0; i < ihiA - iloA + 1; i++)
        for (j = 0; j < jhiA - jloA + 1; j++)
          fsum[iloA + i - 1] += GA_ABS (pf[j * ld + i]);
      break;
      case C_DBL:
      pd = (double *) ptr;
      for (i = 0; i < ihiA - iloA + 1; i++)
        for (j = 0; j < jhiA - jloA + 1; j++)
          dsum[iloA + i - 1] += GA_ABS (pd[j * ld + i]);
      break;
      default:
      gai_error("gai_norm_infinity_block: wrong data type ", type);
    }
  }
}

void FATR
ga_norm_infinity_ (Integer * g_a, double *nm)
{
  Integer dim1, dim2, type, size, nelem;
  Integer me = ga_nodeid_ (), i, j, nproc = ga_nnodes_();
  Integer ndim, dims[MAXDIM], lo[2], hi[2], ld;
  Integer num_blocks_a;
  int local_sync_begin,local_sync_end;
  int imax, *isum = NULL;
  long lmax, *lsum = NULL;
  double dmax, zmax, *dsum = NULL;
  float fmax, cmax,*fsum = NULL;
  DoubleComplex *zsum = NULL;
  SingleComplex *csum = NULL;
  void *buf = NULL;                    /*temporary buffer */
  void *ptr = NULL;


  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  if(local_sync_begin)ga_sync_();

  gai_check_handle (g_a, "ga_norm_infinity_");
  GA_PUSH_NAME ("ga_norm_infinity_");

  /*  gai_inquire (g_a, &type, &dim1, &dim2); */
  nga_inquire_internal_ (g_a, &type, &ndim, dims);

  dim1 = dims[0];
  if(ndim<=0)
    gai_error("ga_norm_infinity: wrong dimension", ndim);
  else if(ndim == 1)
    dim2 = 1;
  else if(ndim==2)  
    dim2 = dims[1];
  else
    gai_error("ga_norm_infinity: wrong dimension", ndim);


  /*allocate a temporary buffer of size equal to the number of rows */
  size = GAsizeof (type);
  nelem = dim1;
  buf = malloc (nelem * size);

  if (buf == NULL)
    gai_error("ga_norm_infinity_: no more memory for the buffer.\n", 0);

  /*zero the buffer */
  memset (buf, 0, nelem * size);

  switch (type)
  {
    case C_INT:
      isum = (int *) buf;
      break;
    case C_LONG:
      lsum = (long *) buf;
      break;
    case C_FLOAT:
      fsum = (float *) buf;
      break;
    case C_DBL:
      dsum = (double *) buf;
      break;
    case C_DCPL:
      zsum = (DoubleComplex *) buf;
      break;
    case C_SCPL:
      csum = (SingleComplex *) buf;
      break;
    default:
      gai_error("ga_norm_infinity_: wrong data type:", type);
  }

  num_blocks_a = ga_total_blocks_(g_a);

  if (num_blocks_a < 0) {
    nga_distribution_(g_a, &me, lo, hi);
    nga_access_ptr(g_a, lo, hi, &ptr, &ld);
    gai_norm_infinity_block(g_a, ptr, lo, hi, ld, type, ndim, dims, buf);
    nga_release_update_(g_a, lo, hi);
  } else {
    Integer idx;
    /* Simple block-cyclic data distribution */
    if (!ga_uses_proc_grid_(g_a)) {
      for (idx = me; idx < num_blocks_a; idx += nproc) {
        nga_distribution_(g_a, &idx, lo, hi);
        nga_access_block_ptr(g_a, &idx, &ptr, &ld);
        gai_norm_infinity_block(g_a, ptr, lo, hi, ld, type, ndim, dims, buf);
        nga_release_update_block_(g_a, &idx);
      }
    } else {
      /* Uses scalapack block-cyclic data distribution */
      Integer chk;
      Integer proc_index[MAXDIM], index[MAXDIM];
      Integer topology[MAXDIM];
      Integer blocks[MAXDIM], block_dims[MAXDIM];
      ga_get_proc_index_(g_a, &me, proc_index);
      ga_get_proc_index_(g_a, &me, index);
      ga_get_block_info_(g_a, blocks, block_dims);
      ga_get_proc_grid_(g_a, topology);
      while (index[ndim-1] < blocks[ndim-1]) {
        /* find bounding coordinates of block */
        chk = 1;
        for (i = 0; i < ndim; i++) {
          lo[i] = index[i]*block_dims[i]+1;
          hi[i] = (index[i] + 1)*block_dims[i];
          if (hi[i] > dims[i]) hi[i] = dims[i];
          if (hi[i] < lo[i]) chk = 0;
        }
        if (chk) {
          nga_access_block_grid_ptr(g_a, index, &ptr, &ld);
          gai_norm_infinity_block(g_a, ptr, lo, hi, ld, type, ndim, dims, buf);
          nga_release_update_block_(g_a, index);
        }
        /* increment index to get next block on processor */
        index[0] += topology[0];
        for (i = 0; i < ndim; i++) {
          if (index[i] >= blocks[i] && i<ndim-1) {
            index[i] = proc_index[i];
            index[i+1] += topology[i+1];
          }
        }
      }
    }
  }


  /*calculate the global value buf[j] for each column */
  switch (type)
  {
    case C_INT:
      armci_msg_igop (isum, nelem, "+");
      break;
    case C_DBL:
      armci_msg_dgop (dsum, nelem, "+");
      break;
    case C_DCPL:
      armci_msg_dgop ((double *) zsum, 2 * nelem, "+");
      break;
    case C_FLOAT:
      armci_msg_fgop (fsum, nelem, "+");
      break;
    case C_SCPL:
      armci_msg_fgop ((float *) csum, 2 * nelem, "+");
      break;
    case C_LONG:
      armci_msg_lgop (lsum, nelem, "+");
      break;
    default:
      gai_error("ga_norm_infinity_: wrong data type ", type);
  }

  /*evaluate the norm infinity for the matrix g_a */
  switch (type)
  {
    case C_INT:
      imax = ((int *) buf)[0];
      for (j = 1; j < nelem; j++)
        if (imax < ((int *) buf)[j])
          imax = ((int *) buf)[j];
      *((double *) nm) = (double) imax;
      break;
    case C_LONG:
      lmax = ((long *) buf)[0];
      for (j = 1; j < nelem; j++)
        if (lmax < ((long *) buf)[j])
          lmax = ((long *) buf)[j];
      *((double *) nm) = (double) lmax;
      break;
    case C_FLOAT:
      fmax = ((float *) buf)[0];
      for (j = 1; j < nelem; j++)
        if (fmax < ((float *) buf)[j])
          fmax = ((float *) buf)[j];
      *((double *) nm) = (double) fmax;
      break;
    case C_DBL:
      dmax = ((double *) buf)[0];
      for (j = 1; j < nelem; j++)
        if (dmax < ((double *) buf)[j])
          dmax = ((double *) buf)[j];
      *((double *) nm) = dmax;
      break;
    case C_DCPL:
      zmax = (((DoubleComplex *) buf)[0]).real;
      for (j = 1; j < nelem; j++)
        if (zmax < (((DoubleComplex *) buf)[j]).real)
          zmax = (((DoubleComplex *) buf)[j]).real;
      *((double *) nm) = zmax;
      break;
    case C_SCPL:
      cmax = (((SingleComplex *) buf)[0]).real;
      for (j = 1; j < nelem; j++)
        if (cmax < (((SingleComplex *) buf)[j]).real)
          cmax = (((SingleComplex *) buf)[j]).real;
      *((double *) nm) = (double) cmax;
      break;
    default:
      gai_error("ga_norm_infinity_:wrong data type.", type);
  }

  /*free the memory allocated to buf */
  free (buf);
  buf = NULL;

  GA_POP_NAME;
  ga_sync_ ();
}

void gai_norm1_block(Integer *g_a, void *ptr,
                     Integer *lo, Integer *hi, Integer ld,
                     Integer type,
                     Integer ndim, Integer *dims, void *buf)
{
  Integer size, nelem, dim2;
  Integer iloA=0, ihiA=0, jloA=0, jhiA=0;
  Integer i, j;
  int *isum = NULL;
  long *lsum = NULL;
  double *dsum = NULL;
  float *fsum = NULL;
  DoubleComplex *zsum = NULL;
  SingleComplex *csum = NULL;

  if(ndim == 1) 
    dim2 = 1;
  else if(ndim == 2) 
    dim2 = dims[1];

  size = GAsizeof (type);
  nelem = dim2;

  switch (type)
  {
    case C_INT:
      isum = (int *) buf;
      break;
    case C_LONG:
      lsum = (long *) buf;
      break;
    case C_FLOAT:
      fsum = (float *) buf;
      break;
    case C_DBL:
      dsum = (double *) buf;
      break;
    case C_DCPL:
      zsum = (DoubleComplex *) buf;
      break;
    case C_SCPL:
      csum = (SingleComplex *) buf;
      break;
    default:
      gai_error("ga1_norm1_block: wrong data type:", type);
  }

  if(ndim<=0)
    gai_error("gai_norm1_block: wrong dimension", ndim);
  else if(ndim == 1) { 
    iloA=lo[0];
    ihiA=hi[0];
    jloA=1;
    jhiA=1;
  }
  else if(ndim == 2)
  {
    iloA=lo[0];
    ihiA=hi[0];
    jloA=lo[1];
    jhiA=hi[1];
  }
  else
    gai_error("gai_norm1_block: wrong dimension", ndim);

  /* determine subset of my patch to access */
  if (ihiA > 0 && jhiA > 0)
  {
    /* lo[0] = iloA; */
    /* lo[1] = jloA; */
    /* hi[0] = ihiA; */
    hi[1] = jhiA;

    switch (type)
    {
      int *pi;
      double *pd;
      long *pl;
      float *pf;
      DoubleComplex *pz;
      SingleComplex *pc;
      case C_INT:
      pi = (int *) ptr;
      for (j = 0; j < jhiA - jloA + 1; j++)
        for (i = 0; i < ihiA - iloA + 1; i++)
          isum[jloA + j - 1 ] += GA_ABS (pi[j * ld + i]);
      break;
      case C_LONG:
      pl = (long *) ptr;
      for (j = 0; j < jhiA - jloA + 1; j++)
        for (i = 0; i < ihiA - iloA + 1; i++)
          lsum[jloA + j  - 1] += GA_ABS (pl[j * ld + i]);
      break;
      case C_DCPL:
      pz = (DoubleComplex *) ptr;
      for (j = 0; j < jhiA - jloA + 1; j++)
        for (i = 0; i < ihiA - iloA + 1; i++)
        {
          DoubleComplex zval = pz[j * ld + i];
          double temp =
            sqrt (zval.real * zval.real + zval.imag * zval.imag);
          (zsum[jloA + j  - 1 ]).real += temp;
        }
      break;

      case C_SCPL:
      pc = (SingleComplex *) ptr;
      for (j = 0; j < jhiA - jloA + 1; j++)
        for (i = 0; i < ihiA - iloA + 1; i++)
        {
          SingleComplex cval = pc[j * ld + i];
          float temp =
            sqrt (cval.real * cval.real + cval.imag * cval.imag);
          (csum[jloA + j  - 1 ]).real += temp;
        }
      break;

      case C_FLOAT:
      pf = (float *) ptr;
      for (j = 0; j < jhiA - jloA + 1; j++)
        for (i = 0; i < ihiA - iloA + 1; i++)
          fsum[jloA + j  - 1 ] += GA_ABS (pf[j * ld + i]);
      break;
      case C_DBL:
      pd = (double *) ptr;
      for (j = 0; j < jhiA - jloA + 1; j++)
        for (i = 0; i < ihiA - iloA + 1; i++)
          dsum[jloA + j - 1 ] += GA_ABS (pd[j * ld + i]);
      break;
      default:
      gai_error("gai_norm1_block: wrong data type ", type);
    }
  }
}

void FATR
ga_norm1_ (Integer * g_a, double *nm)
{
  Integer dim1=0, dim2=0, type=0, size=0, nelem=0;
  Integer me = ga_nodeid_ (), i, j, nproc = ga_nnodes_();
  Integer ndim, dims[MAXDIM], lo[2], hi[2], ld; 
  Integer num_blocks_a;
  int local_sync_begin,local_sync_end;
  int imax, *isum = NULL;
  long lmax, *lsum = NULL;
  double dmax, zmax, *dsum = NULL;
  float fmax, cmax, *fsum = NULL;
  DoubleComplex *zsum = NULL;
  SingleComplex *csum = NULL;
  void *buf = NULL;                    /*temporary buffer */
  void *ptr = NULL;

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  if(local_sync_begin)ga_sync_();

  gai_check_handle (g_a, "ga_norm1_");
  GA_PUSH_NAME ("ga_norm1_");

  nga_inquire_internal_ (g_a, &type, &ndim, dims);


  dim1 = dims[0];
  if(ndim<=0)
    gai_error("ga_norm1: wrong dimension", ndim);
  else if(ndim == 1) 
    dim2 = 1;
  else if(ndim == 2) 
    dim2 = dims[1];
  else
    gai_error("ga_norm1: wrong dimension", ndim);
  /* gai_inquire (g_a, &type, &dim1, &dim2); */

  /*allocate a temporary buffer of size equal to the number of columns */
  size = GAsizeof (type);
  nelem = dim2;
  buf = malloc (nelem * size);

  if (buf == NULL)
    gai_error("ga_norm1: no more memory for the buffer.\n", 0);

  /*zero the buffer */
  memset (buf, 0, nelem * size);

  switch (type)
  {
    case C_INT:
      isum = (int *) buf;
      break;
    case C_LONG:
      lsum = (long *) buf;
      break;
    case C_FLOAT:
      fsum = (float *) buf;
      break;
    case C_DBL:
      dsum = (double *) buf;
      break;
    case C_DCPL:
      zsum = (DoubleComplex *) buf;
      break;
    case C_SCPL:
      csum = (SingleComplex *) buf;
      break;
    default:
      gai_error("ga_norm1_: wrong data type:", type);
  }

  num_blocks_a = ga_total_blocks_(g_a);

  if (num_blocks_a < 0) {
    nga_distribution_(g_a, &me, lo, hi);
    nga_access_ptr(g_a, lo, hi, &ptr, &ld);
    gai_norm1_block(g_a, ptr, lo, hi, ld, type, ndim, dims, buf);
    nga_release_update_(g_a, lo, hi);
  } else {
    Integer idx;
    /* Simple block-cyclic data distribution */
    if (!ga_uses_proc_grid_(g_a)) {
      for (idx = me; idx < num_blocks_a; idx += nproc) {
        nga_distribution_(g_a, &idx, lo, hi);
        nga_access_block_ptr(g_a, &idx, &ptr, &ld);
        gai_norm1_block(g_a, ptr, lo, hi, ld, type, ndim, dims, buf);
        nga_release_update_block_(g_a, &idx);
      }
    } else {
      /* Uses scalapack block-cyclic data distribution */
      Integer chk;
      Integer proc_index[MAXDIM], index[MAXDIM];
      Integer topology[MAXDIM];
      Integer blocks[MAXDIM], block_dims[MAXDIM];
      ga_get_proc_index_(g_a, &me, proc_index);
      ga_get_proc_index_(g_a, &me, index);
      ga_get_block_info_(g_a, blocks, block_dims);
      ga_get_proc_grid_(g_a, topology);
      while (index[ndim-1] < blocks[ndim-1]) {
        /* find bounding coordinates of block */
        chk = 1;
        for (i = 0; i < ndim; i++) {
          lo[i] = index[i]*block_dims[i]+1;
          hi[i] = (index[i] + 1)*block_dims[i];
          if (hi[i] > dims[i]) hi[i] = dims[i];
          if (hi[i] < lo[i]) chk = 0;
        }
        if (chk) {
          nga_access_block_grid_ptr(g_a, index, &ptr, &ld);
          gai_norm1_block(g_a, ptr, lo, hi, ld, type, ndim, dims, buf);
          nga_release_update_block_grid_(g_a, index);
        }
        /* increment index to get next block on processor */
        index[0] += topology[0];
        for (i = 0; i < ndim; i++) {
          if (index[i] >= blocks[i] && i<ndim-1) {
            index[i] = proc_index[i];
            index[i+1] += topology[i+1];
          }
        }
      }
    }
  }

  /*calculate the global value buf[j] for each column */
  switch (type)
  {
    case C_INT:
      armci_msg_igop (isum, nelem, "+");
      break;
    case C_DBL:
      armci_msg_dgop (dsum, nelem, "+");
      break;
    case C_DCPL:
      armci_msg_dgop ((double *) zsum, 2 * nelem, "+");
      break;
    case C_FLOAT:
      armci_msg_fgop (fsum, nelem, "+");
      break;
    case C_SCPL:
      armci_msg_fgop ((float *) csum, 2 * nelem, "+");
      break;
    case C_LONG:
      armci_msg_lgop (lsum, nelem, "+");
      break;
    default:
      gai_error("ga_norm1_: wrong data type ", type);
  }

  /*evaluate the norm1 for the matrix g_a */
  switch (type)
  {
    case C_INT:
      imax = ((int *) buf)[0];
      for (j = 1; j < nelem; j++)
        if (imax < ((int *) buf)[j])
          imax = ((int *) buf)[j];
      *((double *) nm) = (double) imax;
      break;
    case C_LONG:
      lmax = ((long *) buf)[0];
      for (j = 1; j < nelem; j++)
        if (lmax < ((long *) buf)[j])
          lmax = ((long *) buf)[j];
      *((double *) nm) = (double) lmax;
      break;
    case C_FLOAT:
      fmax = ((float *) buf)[0];
      for (j = 1; j < nelem; j++)
        if (fmax < ((float *) buf)[j])
          fmax = ((float *) buf)[j];
      *((double *) nm) = (double) fmax;
      break;
    case C_DBL:
      dmax = ((double *) buf)[0];
      for (j = 1; j < nelem; j++)
        if (dmax < ((double *) buf)[j])
          dmax = ((double *) buf)[j];
      *((double *) nm) = dmax;
      break;
    case C_DCPL:
      zmax = (((DoubleComplex *) buf)[0]).real;
      for (j = 1; j < nelem; j++)
        if (zmax < (((DoubleComplex *) buf)[j]).real)
          zmax = (((DoubleComplex *) buf)[j]).real;
      *((double *) nm) = zmax;
      break;
    case C_SCPL:
      cmax = (((SingleComplex *) buf)[0]).real;
      for (j = 1; j < nelem; j++)
        if (cmax < (((SingleComplex *) buf)[j]).real)
          cmax = (((SingleComplex *) buf)[j]).real;
      *((double *) nm) = (double) cmax;
      break;
    default:
      gai_error("ga_norm1_:wrong data type.", type);
  }


  /*free the memory allocated to buf */
  free (buf);
  buf = NULL;

  GA_POP_NAME;
  ga_sync_ ();
}

void gai_get_diagonal_block(Integer *g_a, void *ptr, Integer *g_v,
                            Integer *loA, Integer *hiA, Integer ld,
                            Integer type)
{
  Integer nelem, size;
  Integer vlo, vhi, iloA, ihiA, jloA, jhiA, lo[2], hi[2];
  Integer i;
  void *buf;
  int *ia;
  float *fa;
  double *da;
  long *la;
  DoubleComplex *dca;
  SingleComplex *fca;

  iloA = loA[0];
  ihiA = hiA[0];
  jloA = loA[1];
  jhiA = hiA[1];

  /* determine subset of my patch to access */
  if (iloA > 0)
  {
    lo[0] = GA_MAX (iloA, jloA);
    lo[1] = GA_MAX (iloA, jloA);
    hi[0] = GA_MIN (ihiA, jhiA);
    hi[1] = GA_MIN (ihiA, jhiA);



    if (hi[0] >= lo[0]) /*make sure the equality symbol is there!!! */
    {                   /* we got a block containing diagonal elements */

      /*allocate a buffer for the given vector g_v */
      size = GAsizeof (type);
      vlo = GA_MAX (iloA, jloA);
      vhi = GA_MIN (ihiA, jhiA);
      nelem = vhi - vlo + 1;
      buf = malloc (nelem * size);
      if (buf == NULL)
        gai_error
          ("ga_get_diag_:failed to allocate memory for the local buffer.",
           9999);

      /* get the vector from the global array g_a, put that in the the local memory buffer buf */
      switch (type)
      {
        case C_INT:
          ia = (int *) ptr;
          ia += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            ((int *) buf)[i] = *ia;
            ia += ld + 1;
          }
          break;
        case C_LONG:
          la = (long *) ptr;
          la += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            ((long *) buf)[i] = *la;
            la += ld + 1;
          }
          break;
        case C_FLOAT:
          fa = (float *) ptr;
          fa += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            ((float *) buf)[i] = *fa;
            fa += ld + 1;
          }
          break;
        case C_DBL:
          da = (double *) ptr;
          da += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            ((double *) buf)[i] = *da;
            da += ld + 1;
          }
          break;
        case C_DCPL:
          dca = (DoubleComplex *) ptr;
          dca += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            (((DoubleComplex *) buf)[i]).real = (*dca).real;
            (((DoubleComplex *) buf)[i]).imag = (*dca).imag;
            dca += ld + 1;
          }
          break;

        case C_SCPL:
          fca = (SingleComplex *) ptr;
          fca += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            (((SingleComplex *) buf)[i]).real = (*fca).real;
            (((SingleComplex *) buf)[i]).imag = (*fca).imag;
            fca += ld + 1;
          }
          break;

        default:
          gai_error("get_diagonal_zero: wrong data type:", type);
      }

      /* copy the local memory buffer buf to g_v */
      nga_put_ (g_v, &vlo, &vhi, buf, &vhi);

      /*free the memory */
      free (buf);
    }
  }
}

void FATR
ga_get_diag_ (Integer * g_a, Integer * g_v)
{
  Integer vndim, vdims, dim1, dim2, vtype, atype, type;
  Integer me = ga_nodeid_ (), i, nproc = ga_nnodes_();
  Integer andim, adims[2];
  Integer loA[2], hiA[2], ld;
  Integer num_blocks_a;
  int local_sync_begin,local_sync_end;
  void *ptr;

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  if(local_sync_begin)ga_sync_();

  gai_check_handle (g_a, "ga_get_diag_");
  gai_check_handle (g_v, "ga_get_diag_");
  GA_PUSH_NAME ("ga_get_diag_");

  gai_inquire (g_a, &type, &dim1, &dim2);

  /*Make sure to use nga_inquire to query for the data type since gai_inquire and nga_inquire treat data type differently */
  nga_inquire_ (g_a, &atype, &andim, adims);
  nga_inquire_ (g_v, &vtype, &vndim, &vdims);

  /* Perform some error checking */
  if (vndim != 1)
    gai_error("ga_get_diag: wrong dimension for g_v.", vndim);


  if (vdims != GA_MIN (dim1, dim2))
    gai_error
      ("ga_get_diag: The size of the first array's diagonal is greater than the size of the second array.",
       type);

  if (vtype != atype)
  {
    gai_error
      ("ga_get_diag: input global arrays do not have the same data type. Global array type =",
       atype);
  }

  num_blocks_a = ga_total_blocks_(g_a);

  if (num_blocks_a < 0) {
    nga_distribution_(g_a, &me, loA, hiA);
    nga_access_ptr(g_a, loA, hiA, &ptr, &ld);
    gai_get_diagonal_block(g_a, ptr, g_v, loA, hiA, ld, type);
    nga_release_update_(g_a, loA, hiA);
  } else {
    Integer idx;
    /* Simple block-cyclic data distribution */
    if (!ga_uses_proc_grid_(g_a)) {
      for (idx = me; idx < num_blocks_a; idx += nproc) {
        nga_distribution_(g_a, &idx, loA, hiA);
        nga_access_block_ptr(g_a, &idx, &ptr, &ld);
        gai_get_diagonal_block(g_a, ptr, g_v, loA, hiA, ld, type);
        nga_release_update_block_(g_a, &idx);
      }
    } else {
      /* Uses scalapack block-cyclic data distribution */
      Integer chk;
      Integer proc_index[MAXDIM], index[MAXDIM];
      Integer topology[MAXDIM];
      Integer blocks[MAXDIM], block_dims[MAXDIM];
      ga_get_proc_index_(g_a, &me, proc_index);
      ga_get_proc_index_(g_a, &me, index);
      ga_get_block_info_(g_a, blocks, block_dims);
      ga_get_proc_grid_(g_a, topology);
      while (index[andim-1] < blocks[andim-1]) {
        /* find bounding coordinates of block */
        chk = 1;
        for (i = 0; i < andim; i++) {
          loA[i] = index[i]*block_dims[i]+1;
          hiA[i] = (index[i] + 1)*block_dims[i];
          if (hiA[i] > adims[i]) hiA[i] = adims[i];
          if (hiA[i] < loA[i]) chk = 0;
        }
        if (chk) {
          nga_access_block_grid_ptr(g_a, index, &ptr, &ld);
          gai_get_diagonal_block(g_a, ptr, g_v, loA, hiA, ld, type);
          nga_release_update_block_grid_(g_a, index);
        }
        /* increment index to get next block on processor */
        index[0] += topology[0];
        for (i = 0; i < andim; i++) {
          if (index[i] >= blocks[i] && i<andim-1) {
            index[i] = proc_index[i];
            index[i+1] += topology[i+1];
          }
        }
      }
    }
  }

  GA_POP_NAME;
  if(local_sync_end)ga_sync_();
}


void gai_add_diagonal_block(Integer *g_a, void *ptr, Integer *g_v,
                            Integer *loA, Integer *hiA, Integer ld,
                            Integer type)
{
  Integer nelem, size;
  Integer vlo, vhi, iloA, ihiA, jloA, jhiA, lo[2], hi[2];
  Integer i;
  void *buf;
  int *ia;
  float *fa;
  double *da;
  long *la;
  DoubleComplex *dca;
  SingleComplex *fca;

  iloA = loA[0];
  ihiA = hiA[0];
  jloA = loA[1];
  jhiA = hiA[1];

  /* determine subset of my patch to access */
  if (iloA > 0)
  {
    lo[0] = GA_MAX (iloA, jloA);
    lo[1] = GA_MAX (iloA, jloA);
    hi[0] = GA_MIN (ihiA, jhiA);
    hi[1] = GA_MIN (ihiA, jhiA);



    if (hi[0] >= lo[0]) /*make sure the equality symbol is there!!! */
    {                   /* we got a block containing diagonal elements */

      /*allocate a buffer for the given vector g_v */
      size = GAsizeof (type);
      vlo = GA_MAX (iloA, jloA);
      vhi = GA_MIN (ihiA, jhiA);
      nelem = vhi - vlo + 1;
      buf = malloc (nelem * size);
      if (buf == NULL)
        gai_error
          ("ga_add_diagonal_:failed to allocate memory for the local buffer.",
           0);

      /* get the vector from the global array to the local memory buffer */
      nga_get_ (g_v, &vlo, &vhi, buf, &vhi);

      switch (type)
      {
        case C_INT:
          ia = (int *) ptr;
          ia += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            *ia += ((int *) buf)[i];
            ia += ld + 1;
          }
          break;
        case C_LONG:
          la = (long *) ptr;
          la += ld*(lo[1]-jloA) + lo[0]-iloA; 
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            *la += ((long *) buf)[i];
            la += ld + 1;
          }
          break;
        case C_FLOAT:
          fa = (float *) ptr;
          fa += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            *fa += ((float *) buf)[i];
            fa += ld + 1;
          }
          break;
        case C_DBL:
          da = (double *) ptr;
          da += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            *da += ((double *) buf)[i];
            da += ld + 1;
          }
          break;
        case C_DCPL:
          dca = (DoubleComplex *) ptr;
          dca += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            (*dca).real += (((DoubleComplex *) buf)[i]).real;
            (*dca).imag += (((DoubleComplex *) buf)[i]).imag;
            dca += ld + 1;
          }
          break;

        case C_SCPL:
          fca = (SingleComplex *) ptr;
          fca += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            (*fca).real += (((SingleComplex *) buf)[i]).real;
            (*fca).imag += (((SingleComplex *) buf)[i]).imag;
            fca += ld + 1;
          }
          break;

        default:
          gai_error("ga_add_diagonal_: wrong data type:", type);
      }

      /*free the memory */
      free (buf);
    }
  }
}

void FATR
ga_add_diagonal_ (Integer * g_a, Integer * g_v)
{
  Integer vndim, vdims, dim1, dim2, vtype, atype, type;
  Integer me = ga_nodeid_ (), i, nproc = ga_nnodes_();
  Integer andim, adims[2];
  Integer loA[2], hiA[2], ld;
  Integer num_blocks_a;
  int local_sync_begin,local_sync_end;
  void *ptr;

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  if(local_sync_begin)ga_sync_();

  gai_check_handle (g_a, "ga_add_diagonal_");
  gai_check_handle (g_v, "ga_add_diagonal_");
  GA_PUSH_NAME ("ga_add_diagonal_");

  gai_inquire (g_a, &type, &dim1, &dim2);


  /*Make sure to use nga_inquire to query for the data type since gai_inquire and nga_inquire treat data type differently */
  nga_inquire_ (g_a, &atype, &andim, adims);
  nga_inquire_ (g_v, &vtype, &vndim, &vdims);

  /* Perform some error checking */
  if (vndim != 1)
    gai_error("ga_add_diagonal: wrong dimension for g_v.", vndim);


  if (vdims != GA_MIN (dim1, dim2))
    gai_error
      ("ga_add_diagonal: The size of the first array's diagonal is greater than the size of the second array.",
       type);

  if (vtype != atype)
  {
    gai_error
      ("ga_add_diagonal: input global arrays do not have the same data type. Global array type =",
       atype);
  }

  num_blocks_a = ga_total_blocks_(g_a);

  if (num_blocks_a < 0) {
    nga_distribution_(g_a, &me, loA, hiA);
    nga_access_ptr(g_a, loA, hiA, &ptr, &ld);
    gai_add_diagonal_block(g_a, ptr, g_v, loA, hiA, ld, type);
  } else {
    Integer idx;
    /* Simple block-cyclic data distribution */
    if (!ga_uses_proc_grid_(g_a)) {
      for (idx = me; idx < num_blocks_a; idx += nproc) {
        nga_distribution_(g_a, &idx, loA, hiA);
        nga_access_block_ptr(g_a, &idx, &ptr, &ld);
        gai_add_diagonal_block(g_a, ptr, g_v, loA, hiA, ld, type);
        nga_release_update_block_(g_a, &idx);
      }
    } else {
      /* Uses scalapack block-cyclic data distribution */
      Integer chk;
      Integer proc_index[MAXDIM], index[MAXDIM];
      Integer topology[MAXDIM];
      Integer blocks[MAXDIM], block_dims[MAXDIM];
      ga_get_proc_index_(g_a, &me, proc_index);
      ga_get_proc_index_(g_a, &me, index);
      ga_get_block_info_(g_a, blocks, block_dims);
      ga_get_proc_grid_(g_a, topology);
      while (index[andim-1] < blocks[andim-1]) {
        /* find bounding coordinates of block */
        chk = 1;
        for (i = 0; i < andim; i++) {
          loA[i] = index[i]*block_dims[i]+1;
          hiA[i] = (index[i] + 1)*block_dims[i];
          if (hiA[i] > adims[i]) hiA[i] = adims[i];
          if (hiA[i] < loA[i]) chk = 0;
        }
        if (chk) {
          nga_access_block_grid_ptr(g_a, index, &ptr, &ld);
          gai_add_diagonal_block(g_a, ptr, g_v, loA, hiA, ld, type);
          nga_release_update_block_grid_(g_a, index);
        }
        /* increment index to get next block on processor */
        index[0] += topology[0];
        for (i = 0; i < andim; i++) {
          if (index[i] >= blocks[i] && i<andim-1) {
            index[i] = proc_index[i];
            index[i+1] += topology[i+1];
          }
        }
      }
    }
  }
  GA_POP_NAME;
  if(local_sync_end)ga_sync_();
}

void gai_set_diagonal_block(Integer *g_a, void *ptr, Integer *g_v, Integer *loA,
                            Integer *hiA, Integer ld, Integer type)
{
  Integer nelem, size;
  Integer vlo, vhi, iloA, ihiA, jloA, jhiA, lo[2], hi[2];
  Integer i;
  void *buf;
  int *ia;
  float *fa;
  double *da;
  long *la;
  DoubleComplex *dca;
  SingleComplex *fca;

  iloA = loA[0];
  ihiA = hiA[0];
  jloA = loA[1];
  jhiA = hiA[1];

  /* determine subset of my patch to access */
  if (iloA > 0)
  {
    lo[0] = GA_MAX (iloA, jloA);
    lo[1] = GA_MAX (iloA, jloA);
    hi[0] = GA_MIN (ihiA, jhiA);
    hi[1] = GA_MIN (ihiA, jhiA);



    if (hi[0] >= lo[0]) /*make sure the equality symbol is there!!! */
    {                   /* we got a block containing diagonal elements*/

      /*allocate a buffer for the given vector g_v */
      size = GAsizeof (type);
      vlo = GA_MAX (iloA, jloA);
      vhi = GA_MIN (ihiA, jhiA);
      nelem = vhi - vlo + 1;
      buf = malloc (nelem * size);
      if (buf == NULL)
        gai_error
          ("ga_set_diagonal_:failed to allocate memory for local buffer",0);

      /* get the vector from the global array to the local memory buffer */
      nga_get_ (g_v, &vlo, &vhi, buf, &vhi);

      switch (type)
      {
        case C_INT:
          ia = (int *) ptr;
          ia += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            *ia = ((int *) buf)[i];
            ia += ld + 1;
          }
          break;
        case C_LONG:
          la = (long *) ptr;
          la += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            *la = ((long *) buf)[i];
            la += ld + 1;
          }
          break;
        case C_FLOAT:
          fa = (float *) ptr;
          fa += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            *fa = ((float *) buf)[i];
            fa += ld + 1;
          }
          break;
        case C_DBL:
          da = (double *) ptr;
          da += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            *da = ((double *) buf)[i];
            da += ld + 1;
          }
          break;
        case C_DCPL:
          dca = (DoubleComplex *) ptr;
          dca += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            (*dca).real = (((DoubleComplex *) buf)[i]).real;
            (*dca).imag = (((DoubleComplex *) buf)[i]).imag;
            dca += ld + 1;
          }
          break;

        case C_SCPL:
          fca = (SingleComplex *) ptr;
          fca += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            (*fca).real = (((SingleComplex *) buf)[i]).real;
            (*fca).imag = (((SingleComplex *) buf)[i]).imag;
            fca += ld + 1;
          }
          break;

        default:
          gai_error("ga_set_diagonal_: wrong data type:", type);
      }

      /*free the memory */
      free (buf);

      /* release access to the data */
      ga_release_update_ (g_a, &iloA, &ihiA, &jloA, &jhiA);
    }
  }
}

void FATR
ga_set_diagonal_ (Integer * g_a, Integer * g_v)
{
  Integer vndim, vdims, dim1, dim2, vtype, atype, type;
  Integer me = ga_nodeid_ (), i, nproc = ga_nnodes_();
  Integer andim, adims[2];
  Integer loA[2], hiA[2], ld;
  Integer num_blocks_a;
  int local_sync_begin,local_sync_end;
  void *ptr;

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  if(local_sync_begin)ga_sync_();

  gai_check_handle (g_a, "ga_set_diagonal_");
  gai_check_handle (g_v, "ga_set_diagonal_");
  GA_PUSH_NAME ("ga_set_diagonal_");

  gai_inquire (g_a, &type, &dim1, &dim2);

  /*Make sure to use nga_inquire to query for the data type since gai_inquire and nga_inquire treat data type differently */
  nga_inquire_ (g_a, &atype, &andim, adims);
  nga_inquire_ (g_v, &vtype, &vndim, &vdims);

  /* Perform some error checking */
  if (vndim != 1)
    gai_error("ga_set_diagonal: wrong dimension for g_v.", vndim);


  if (vdims != GA_MIN (dim1, dim2))
    gai_error
      ("ga_set_diagonal: The size of the first array's diagonal is greater than the size of the second array.",
       type);

  if (vtype != atype)
  {
    gai_error
      ("ga_set_diagonal: input global arrays do not have the same data type. Global array type =",
       atype);
  }

  num_blocks_a = ga_total_blocks_(g_a);

  if (num_blocks_a < 0) {
    nga_distribution_(g_a, &me, loA, hiA);
    nga_access_ptr(g_a, loA, hiA, &ptr, &ld);
    gai_set_diagonal_block(g_a, ptr, g_v, loA, hiA, ld, type);
    nga_release_update_(g_a, loA, hiA);
  } else {
    Integer idx;
    /* Simple block-cyclic data distribution */
    if (!ga_uses_proc_grid_(g_a)) {
      for (idx = me; idx < num_blocks_a; idx += nproc) {
        nga_distribution_(g_a, &idx, loA, hiA);
        nga_access_block_ptr(g_a, &idx, &ptr, &ld);
        gai_set_diagonal_block(g_a, ptr, g_v, loA, hiA, ld, type);
        nga_release_update_block_(g_a, &idx);
      }
    } else {
      /* Uses scalapack block-cyclic data distribution */
      Integer chk;
      Integer proc_index[MAXDIM], index[MAXDIM];
      Integer topology[MAXDIM];
      Integer blocks[MAXDIM], block_dims[MAXDIM];
      ga_get_proc_index_(g_a, &me, proc_index);
      ga_get_proc_index_(g_a, &me, index);
      ga_get_block_info_(g_a, blocks, block_dims);
      ga_get_proc_grid_(g_a, topology);
      while (index[andim-1] < blocks[andim-1]) {
        /* find bounding coordinates of block */
        chk = 1;
        for (i = 0; i < andim; i++) {
          loA[i] = index[i]*block_dims[i]+1;
          hiA[i] = (index[i] + 1)*block_dims[i];
          if (hiA[i] > adims[i]) hiA[i] = adims[i];
          if (hiA[i] < loA[i]) chk = 0;
        }
        if (chk) {
          nga_access_block_grid_ptr(g_a, index, &ptr, &ld);
          gai_set_diagonal_block(g_a, ptr, g_v, loA, hiA, ld, type);
          nga_release_update_block_grid_(g_a, index);
        }
        /* increment index to get next block on processor */
        index[0] += topology[0];
        for (i = 0; i < andim; i++) {
          if (index[i] >= blocks[i] && i<andim-1) {
            index[i] = proc_index[i];
            index[i+1] += topology[i+1];
          }
        }
      }
    }

  }

  GA_POP_NAME;
  if(local_sync_begin)ga_sync_();
}

void gai_shift_diagonal_block(Integer *g_a, void *ptr, Integer *loA, Integer *hiA,
                              Integer ld, void *c, Integer type)
{
  Integer iloA, ihiA, jloA, jhiA, lo[2], hi[2];
  Integer i;
  int *ia;
  float *fa;
  double *da;
  long *la;
  DoubleComplex *dca;
  SingleComplex *fca;

  iloA = loA[0];
  ihiA = hiA[0];
  jloA = loA[1];
  jhiA = hiA[1];

  /* determine subset of my patch to access */
  if (iloA > 0)
  {
    lo[0] = GA_MAX (iloA, jloA);
    lo[1] = GA_MAX (iloA, jloA);
    hi[0] = GA_MIN (ihiA, jhiA);
    hi[1] = GA_MIN (ihiA, jhiA);
    if (hi[0] >= lo[0]) /*make sure the equality sign is there since it is the singleton case */
    {                   /* we got a block containing diagonal elements */

      switch (type)
      {
        case C_INT:
          ia = (int *) ptr;
          ia += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            *ia += *((int *) c);
            ia += ld + 1;
          }
          break;
        case C_LONG:
          la = (long *) ptr;
          la += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            *la += *((long *) c);
            la += ld + 1;
          }
          break;
        case C_FLOAT:
          fa = (float *) ptr;
          fa += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            *fa += *((float *) c);
            fa += ld + 1;
          }
          break;
        case C_DBL:
          da = (double *) ptr;
          da += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            *da += *((double *) c);
            da += ld + 1;
          }
          break;
        case C_DCPL:
          dca = (DoubleComplex *) ptr;
          dca += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            (*dca).real += (*((DoubleComplex *) c)).real;
            (*dca).imag += (*((DoubleComplex *) c)).imag;
            dca += ld + 1;
          }
          break;

        case C_SCPL:
          fca = (SingleComplex *) ptr;
          fca += ld*(lo[1]-jloA) + lo[0]-iloA;
          for (i = 0; i < hi[0] - lo[0] + 1; i++)
          {
            (*fca).real += (*((SingleComplex *) c)).real;
            (*fca).imag += (*((SingleComplex *) c)).imag;
            fca += ld + 1;
          }
          break;

        default:
          gai_error("ga_shift_diagonal_: wrong data type:", type);
      }
    }
  }
}

void FATR
ga_shift_diagonal_ (Integer * g_a, void *c)
{
  Integer loA[2], hiA[2], dim1, dim2, ld;
  Integer andim, adims[2], type, atype;
  Integer me = ga_nodeid_ (), i, nproc = ga_nnodes_();
  void *ptr;
  Integer num_blocks_a;
  int local_sync_begin,local_sync_end;

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  if(local_sync_begin)ga_sync_();

  gai_check_handle (g_a, "ga_shift_diagonal_");
  GA_PUSH_NAME ("ga_shift_diagonal_");

  nga_inquire_ (g_a, &atype, &andim, adims);
  if (andim != 2) 
    gai_error("Dimension must be 2 for shift diagonal operation",andim);

  /* gai_inquire and nga_inquire handle type differently */
  gai_inquire(g_a, &type, &dim1, &dim2);

  num_blocks_a = ga_total_blocks_(g_a);

  if (num_blocks_a < 0) {
    nga_distribution_(g_a, &me, loA, hiA);
    nga_access_ptr(g_a, loA, hiA, &ptr, &ld);
    gai_shift_diagonal_block(g_a, ptr, loA, hiA, ld, c, type);
    nga_release_update_(g_a, loA, hiA);
  } else {
    Integer idx;
    /* Simple block-cyclic data distribution */
    if (!ga_uses_proc_grid_(g_a)) {
      for (idx = me; idx < num_blocks_a; idx += nproc) {
        nga_distribution_(g_a, &idx, loA, hiA);
        nga_access_block_ptr(g_a, &idx, &ptr, &ld);
        gai_shift_diagonal_block(g_a, ptr, loA, hiA, ld, c, type);
        nga_release_update_block_(g_a, &idx);
      }
    } else {
      /* Uses scalapack block-cyclic data distribution */
      Integer chk;
      Integer proc_index[MAXDIM], index[MAXDIM];
      Integer topology[MAXDIM];
      Integer blocks[MAXDIM], block_dims[MAXDIM];
      ga_get_proc_index_(g_a, &me, proc_index);
      ga_get_proc_index_(g_a, &me, index);
      ga_get_block_info_(g_a, blocks, block_dims);
      ga_get_proc_grid_(g_a, topology);
      while (index[andim-1] < blocks[andim-1]) {
        /* find bounding coordinates of block */
        chk = 1;
        for (i = 0; i < andim; i++) {
          loA[i] = index[i]*block_dims[i]+1;
          hiA[i] = (index[i] + 1)*block_dims[i];
          if (hiA[i] > adims[i]) hiA[i] = adims[i];
          if (hiA[i] < loA[i]) chk = 0;
        }
        if (chk) {
          nga_access_block_grid_ptr(g_a, index, &ptr, &ld);
          gai_shift_diagonal_block(g_a, ptr, loA, hiA, ld, c, type);
          nga_release_update_block_grid_(g_a, index);
        }
        /* increment index to get next block on processor */
        index[0] += topology[0];
        for (i = 0; i < andim; i++) {
          if (index[i] >= blocks[i] && i<andim-1) {
            index[i] = proc_index[i];
            index[i+1] += topology[i+1];
          }
        }
      }
    }
  }

  GA_POP_NAME;
  if(local_sync_end)ga_sync_();
}

void gai_zero_diagonal_block(Integer *g_a, void *ptr, Integer *lo, Integer *hi,
                             Integer ld, Integer offset, Integer type)
{
  Integer i;
  int *ia;
  float *fa;
  double *da;
  long *la;
  DoubleComplex *dca;
  SingleComplex *fca;

  switch (type) {
    case C_INT:
      ia = (int *) ptr + offset;
      for (i = 0; i < hi[0] - lo[0] + 1; i++) {
        *ia = 0;
        ia += ld + 1;
      }
      break;
    case C_LONG:
      la = (long *) ptr + offset;
      for (i = 0; i < hi[0] - lo[0] + 1; i++) {
        *la = 0;
        la += ld + 1;
      }
      break;
    case C_FLOAT:
      fa = (float *) ptr + offset;
      for (i = 0; i < hi[0] - lo[0] + 1; i++) {
        *fa = 0.0;
        fa += ld + 1;
      }
      break;
    case C_DBL:
      da = (double *) ptr + offset;
      for (i = 0; i < hi[0] - lo[0] + 1; i++) {
        *da = 0.0;
        da += ld + 1;
      }
      break;
    case C_DCPL:
      dca = (DoubleComplex *) ptr + offset;
      for (i = 0; i < hi[0] - lo[0] + 1; i++) {
        (*dca).real = 0.0;
        (*dca).imag = 0.0;
        dca += ld + 1;
      }
      break;
    case C_SCPL:
      fca = (SingleComplex *) ptr + offset;
      for (i = 0; i < hi[0] - lo[0] + 1; i++) {
        (*fca).real = 0.0;
        (*fca).imag = 0.0;
        fca += ld + 1;
      }
      break;
    default:
      gai_error("set_diagonal_zero: wrong data type:", type);
  }
}

void FATR ga_zero_diagonal_(Integer * g_a)
{
  Integer dim1, dim2, type;
  Integer ld, lo[2], hi[2], loA[2], hiA[2];
  Integer me = ga_nodeid_ (), i, offset;
  void *ptr;
  Integer num_blocks_a;
  Integer atype, andim, adims[2];
  int local_sync_begin,local_sync_end;

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  if(local_sync_begin)ga_sync_();

  GA_PUSH_NAME ("ga_zero_diagonal_");

  nga_inquire_ (g_a, &atype, &andim, adims);
  /* gai_inquire and nga_inquire return different values for type */
  gai_inquire (g_a, &type, &dim1, &dim2);

  num_blocks_a = ga_total_blocks_(g_a);

  if (num_blocks_a < 0) {
    offset = 0;
    nga_distribution_ (g_a, &me, loA, hiA);
    /* determine subset of my patch to access */
    if (loA[0] > 0) {
      lo[0] = GA_MAX (loA[0], loA[1]);
      lo[1] = GA_MAX (loA[0], loA[1]);
      hi[0] = GA_MIN (hiA[0], hiA[1]);
      hi[1] = GA_MIN (hiA[0], hiA[1]);
      if (hi[0] >= lo[0]) {
                              /* we got a block containing diagonal elements */
        nga_access_ptr (g_a, lo, hi, &ptr, &ld);
        gai_zero_diagonal_block(g_a, ptr, lo, hi, ld, offset, type);
        /* release access to the data */
        nga_release_update_ (g_a, lo, hi);
      }
    }
  } else {
    Integer idx, lld[MAXDIM], offset;
    Integer jtot, last, j;
    Integer nproc = ga_nnodes_();
    /* Simple block-cyclic data distribution */
    if (!ga_uses_proc_grid_(g_a)) {
      for (idx = me; idx < num_blocks_a; idx += nproc) {
        nga_distribution_(g_a, &idx, loA, hiA);
        lo[0] = GA_MAX (loA[0], loA[1]);
        lo[1] = GA_MAX (loA[0], loA[1]);
        hi[0] = GA_MIN (hiA[0], hiA[1]);
        hi[1] = GA_MIN (hiA[0], hiA[1]);

        if (hi[0] >= lo[0]) {
          nga_access_block_ptr(g_a, &idx, &ptr, lld);
          /* evaluate offsets for system */
          offset = 0;
          last = andim - 1;
          jtot = 1;
          for (j=0; j<last; j++) {
            offset += (lo[j] - loA[j])*jtot;
            jtot *= lld[j];
          }
          offset += (lo[last]-loA[last])*jtot;
          gai_zero_diagonal_block(g_a, ptr, lo, hi, lld[0], offset, type);
          nga_release_update_block_(g_a, &idx);
        }
      }
    } else {
      /* Uses scalapack block-cyclic data distribution */
      Integer lld[MAXDIM], chk;
      Integer proc_index[MAXDIM], index[MAXDIM];
      Integer topology[MAXDIM];
      Integer blocks[MAXDIM], block_dims[MAXDIM];
      ga_get_proc_index_(g_a, &me, proc_index);
      ga_get_proc_index_(g_a, &me, index);
      ga_get_block_info_(g_a, blocks, block_dims);
      ga_get_proc_grid_(g_a, topology);

      while (index[andim-1] < blocks[andim-1]) {
        /* find bounding coordinates of block */
        chk = 1;
        for (i = 0; i < andim; i++) {
          loA[i] = index[i]*block_dims[i]+1;
          hiA[i] = (index[i] + 1)*block_dims[i];
          if (hiA[i] > adims[i]) hiA[i] = adims[i];
          if (hiA[i] < loA[i]) chk = 0;
        }
        lo[0] = GA_MAX (loA[0], loA[1]);
        lo[1] = GA_MAX (loA[0], loA[1]);
        hi[0] = GA_MIN (hiA[0], hiA[1]);
        hi[1] = GA_MIN (hiA[0], hiA[1]);

        if (hi[0] >= lo[0]) {
          nga_access_block_grid_ptr(g_a, index, &ptr, lld);
          /* evaluate offsets for system */
          offset = 0;
          last = andim - 1;
          jtot = 1;
          for (j=0; j<last; j++) {
            offset += (lo[j] - loA[j])*jtot;
            jtot *= lld[j];
          }
          offset += (lo[last]-loA[last])*jtot;
          gai_zero_diagonal_block(g_a, ptr, lo, hi, lld[0], offset, type);
          nga_release_update_block_grid_(g_a, index);
        }

        /* increment index to get next block on processor */
        index[0] += topology[0];
        for (i = 0; i < andim; i++) {
          if (index[i] >= blocks[i] && i<andim-1) {
            index[i] = proc_index[i];
            index[i+1] += topology[i+1];
          }
        }
      }
    }
  }
  GA_POP_NAME;
  if(local_sync_end)ga_sync_();
}

void gai_scale_row_values(Integer type, Integer *lo,
                          Integer *hi, Integer ld, void *ptr, Integer *g_v)
{
  Integer vlo, vhi, size;
  int *ia;
  float *fa;
  double *da;
  long *la;
  DoubleComplex *dca;
  SingleComplex *fca;
  void *buf;

  /* determine subset of my patch to access */
  if (lo[0] > 0) {

    if (hi[0] >= lo[0]) { /*make sure the equality symbol is there!!! */
                          /* we got a block containing diagonal elements */

      Integer myrows = hi[0] - lo[0] + 1;
      Integer i, j;
      /*number of rows on the patch is jhiA - jloA + 1 */
      vlo =lo[0] ;
      vhi = hi[0];

      /*allocate a buffer for the given vector g_v */
      size = GAsizeof (type);

      buf = malloc (myrows * size);
      if (buf == NULL)
        gai_error
          ("ga_scale_rows_:failed to allocate memory for the local buffer.",
           0);

      /* get the vector from the global array to the local memory buffer */
      nga_get_ (g_v, &vlo, &vhi, buf, &vhi);

      switch (type) {
        case C_INT:
          ia = (int *) ptr;
          for (i = 0; i < hi[0]-lo[0]+1; i++) /*for each row */
            for(j=0;j<hi[1]-lo[1]+1;j++) /*for each column*/
              ia[j*ld+i] *= ((int *) buf)[i];
          break;
        case C_LONG:
          la = (long *) ptr;
          for (i = 0; i < hi[0]-lo[0]+1; i++)
            for(j=0;j<hi[1]-lo[1]+1;j++)
              la[j*ld+i] *= ((long *) buf)[i];
          break;
        case C_FLOAT:
          fa = (float *) ptr;
          for (i = 0; i < hi[0]-lo[0]+1; i++)
            for(j=0;j<hi[1]-lo[1]+1;j++)
              fa[j*ld+i] *= ((float *) buf)[i];
          break;
        case C_DBL:
          da = (double *) ptr;
          for (i = 0; i < hi[0]-lo[0]+1; i++)
            for(j=0;j<hi[1]-lo[1]+1;j++)
              da[j*ld+i] *= ((double *) buf)[i];
          break;
        case C_DCPL:
          dca = (DoubleComplex *) ptr;
          for (i = 0; i < hi[0]-lo[0]+1; i++)
            for(j=0;j<hi[1]-lo[1]+1;j++) {
              (dca[j*ld+i]).real *= (((DoubleComplex *) buf)[i]).real;
              (dca[j*ld+i]).imag *= (((DoubleComplex *) buf)[i]).imag;
            }
          break;
        case C_SCPL:
          fca = (SingleComplex *) ptr;
          for (i = 0; i < hi[0]-lo[0]+1; i++)
            for(j=0;j<hi[1]-lo[1]+1;j++) {
              (fca[j*ld+i]).real *= (((SingleComplex *) buf)[i]).real;
              (fca[j*ld+i]).imag *= (((SingleComplex *) buf)[i]).imag;
            }
          break;
        default:
          gai_error("ga_scale_rows_: wrong data type:", type);
      }

      /*free the memory */
      free (buf);
    }
  }
}

void FATR ga_scale_rows_(Integer *g_a, Integer *g_v)
{
  Integer vndim, vdims, dim1, dim2, vtype, atype, type;
  Integer ld, lo[2], hi[2];
  Integer me = ga_nodeid_ (), i, chk;
  void *ptr;
  Integer andim, adims[2];
  Integer num_blocks_a;
  int local_sync_begin,local_sync_end;

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  if(local_sync_begin)ga_sync_();

  gai_check_handle (g_a, "ga_scale_rows_");
  gai_check_handle (g_v, "ga_scale_rows_");
  GA_PUSH_NAME ("ga_scale_rows_");

  gai_inquire (g_a, &type, &dim1, &dim2);

  /*Make sure to use nga_inquire to query for the data type since gai_inquire and nga_inquire treat data type differently */
  nga_inquire_ (g_a, &atype, &andim, adims);
  nga_inquire_ (g_v, &vtype, &vndim, &vdims);

  /* Perform some error checking */
  if (vndim != 1)
    gai_error("ga_scale_rows_: wrong dimension for g_v.", vndim);

  /*in internal functions, dim1 = number of rows of the matrix g_a*/
  /*in internal functions, dim2 = number of columns of the matrix g_a*/
  if (vdims != dim1)
    gai_error
      ("ga_scale_rows_: The size of the scalar array is not the same as the number of the rows of g_a.",
       vdims);

  if (vtype != atype)
  {
    gai_error
      ("ga_scale_rows_: input global arrays do not have the same data type. Global array type =",
       atype);
  }

  num_blocks_a = ga_total_blocks_(g_a);

  if (num_blocks_a < 0) {
    nga_distribution_ (g_a, &me, lo, hi);

    chk = 1;
    for (i=0; i<andim; i++) {
      if (lo[i]>hi[i]) chk = 0;
    }
    if (chk) {
      nga_access_ptr (g_a, lo, hi, &ptr, &ld);

      gai_scale_row_values(type, lo, hi, ld, ptr, g_v);

      /* release access to the data */
      nga_release_update_ (g_a, lo, hi);
    }
  } else {
    Integer nproc = ga_nnodes_();
    /* Simple block-cyclic data distribution */
    if (!ga_uses_proc_grid_(g_a)) {
      Integer idx;
      for (idx=me; idx<num_blocks_a; idx += nproc) {
        nga_distribution_(g_a, &idx, lo, hi);
        nga_access_block_ptr(g_a, &idx, &ptr, &ld);
        gai_scale_row_values(type, lo, hi, ld, ptr, g_v);
        nga_release_update_block_(g_a, &idx);
      }
    } else {
      Integer proc_index[MAXDIM], index[MAXDIM];
      Integer topology[MAXDIM];
      Integer blocks[MAXDIM], block_dims[MAXDIM];
      ga_get_proc_index_(g_a, &me, proc_index);
      ga_get_proc_index_(g_a, &me, index);
      ga_get_block_info_(g_a, blocks, block_dims);
      ga_get_proc_grid_(g_a, topology);

      while (index[andim-1] < blocks[andim-1]) {
        /* find bounding coordinates of block */
        chk = 1;
        for (i = 0; i < andim; i++) {
          lo[i] = index[i]*block_dims[i]+1;
          hi[i] = (index[i] + 1)*block_dims[i];
          if (hi[i] > adims[i]) hi[i] = adims[i];
          if (hi[i] < lo[i]) chk = 0;
        }
        if (chk) {
          nga_access_block_grid_ptr(g_a, index, &ptr, &ld);
          gai_scale_row_values(type, lo, hi, ld, ptr, g_v);
          nga_release_update_block_grid_(g_a, index);
        }
        /* increment index to get next block on processor */
        index[0] += topology[0];
        for (i = 0; i < andim; i++) {
          if (index[i] >= blocks[i] && i<andim-1) {
            index[i] = proc_index[i];
            index[i+1] += topology[i+1];
          }
        }
      }
    }
  }
  GA_POP_NAME;
  if(local_sync_end)ga_sync_();
}

void gai_scale_col_values(Integer type, Integer *lo,
                          Integer *hi, Integer ld, void *ptr, Integer *g_v)
{
  Integer vlo, vhi, size;
  void *buf;
  int *ia;
  float *fa;
  double *da;
  long *la;
  DoubleComplex *dca;
  SingleComplex *fca;

  /* determine subset of my patch to access */
  if (lo[1] > 0) {
    if (hi[0] >= lo[0]) {     /*make sure the equality symbol is there!!! */
                              /* we got a block containing diagonal elements*/

      Integer mycols = hi[1] - lo[1] + 1;
      Integer i, j;
      /*number of rows on the patch is jhiA - jloA + 1 */
      vlo =lo[1] ;
      vhi = hi[1];

      /*allocate a buffer for the given vector g_v */
      size = GAsizeof (type);

      buf = malloc (mycols * size);
      if (buf == NULL)
        gai_error
          ("ga_scale_cols_:failed to allocate memory for the local buffer.",
           0);

      /* get the vector from the global array to the local memory buffer */
      nga_get_ (g_v, &vlo, &vhi, buf, &vhi);

      switch (type) {
        case C_INT:
          ia = (int *) ptr;
          for(j=0;j<hi[1]-lo[1]+1;j++) /*for each column*/
            for (i = 0; i < hi[0]-lo[0]+1; i++) /*for each row */
              ia[j*ld+i] *= ((int *) buf)[j];
          break;
        case C_LONG:
          la = (long *) ptr;
          for(j=0;j<hi[1]-lo[1]+1;j++)
            for (i = 0; i < hi[0]-lo[0]+1; i++)
              la[j*ld+i] *= ((long *) buf)[j];
          break;
        case C_FLOAT:
          fa = (float *) ptr;
          for(j=0;j<hi[1]-lo[1]+1;j++)
            for (i = 0; i < hi[0]-lo[0]+1; i++)
              fa[j*ld+i] *= ((float *) buf)[j];
          break;
        case C_DBL:
          da = (double *) ptr;
          for(j=0;j<hi[1]-lo[1]+1;j++)
            for (i = 0; i < hi[0]-lo[0]+1; i++)
              da[j*ld+i] *= ((double *) buf)[j];
          break;
        case C_DCPL:
          dca = (DoubleComplex *) ptr;
          for(j=0;j<hi[1]-lo[1]+1;j++)
            for (i = 0; i < hi[0]-lo[0]+1; i++)
            {
              (dca[j*ld+i]).real *= (((DoubleComplex *) buf)[j]).real;
              (dca[j*ld+i]).imag *= (((DoubleComplex *) buf)[j]).imag;
            }
          break;
        case C_SCPL:
          fca = (SingleComplex *) ptr;
          for(j=0;j<hi[1]-lo[1]+1;j++)
            for (i = 0; i < hi[0]-lo[0]+1; i++)
            {
              (fca[j*ld+i]).real *= (((SingleComplex *) buf)[j]).real;
              (fca[j*ld+i]).imag *= (((SingleComplex *) buf)[j]).imag;
            }
          break;
        default:
          gai_error("ga_scale_cols_: wrong data type:", type);
      }
      /*free the memory */
      free (buf);
    }
  }
}

void FATR ga_scale_cols_(Integer *g_a, Integer *g_v)
{
  Integer vndim, vdims, dim1, dim2, vtype, atype, type;
  Integer ld, lo[2], hi[2];
  Integer me = ga_nodeid_ (), i;
  void *ptr;
  Integer andim, adims[2];
  int local_sync_begin,local_sync_end;
  Integer num_blocks_a;
  Integer chk;

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  if(local_sync_begin)ga_sync_();

  gai_check_handle (g_a, "ga_scale_cols_");
  gai_check_handle (g_v, "ga_scale_cols_");
  GA_PUSH_NAME ("ga_scale_cols_");

  gai_inquire (g_a, &type, &dim1, &dim2);

  /*Make sure to use nga_inquire to query for the data type since gai_inquire and nga_inquire treat data type differently */
  nga_inquire_ (g_a, &atype, &andim, adims);
  nga_inquire_ (g_v, &vtype, &vndim, &vdims);

  /* Perform some error checking */
  if (vndim != 1)
    gai_error("ga_scale_cols_: wrong dimension for g_v.", vndim);

  /*in internal functions, dim1 = number of rows of the matrix g_a*/
  /*in internal functions, dim2 = number of columns of the matrix g_a*/
  if (vdims != dim2)
    gai_error
      ("ga_scale_cols_: The size of the scalar array is not the same as the number of the rows of g_a.",
       vdims);

  if (vtype != atype)
    {
      gai_error
        ("ga_scale_cols_: input global arrays do not have the same data type. Global array type =",
         atype);
    }

  num_blocks_a = ga_total_blocks_(g_a);

  if (num_blocks_a < 0) {
    nga_distribution_ (g_a, &me, lo, hi);

    chk = 1;
    for (i=0; i<andim; i++) {
      if (lo[i]>hi[i]) chk = 0;
    }
    if (chk) {
      nga_access_ptr (g_a, lo, hi, &ptr, &ld);

      gai_scale_col_values(type, lo, hi, ld, ptr, g_v);

      /* release access to the data */
      nga_release_update_ (g_a, lo, hi);
    }
  } else {
    Integer nproc = ga_nnodes_();
    /* Simple block-cyclic data distribution */
    if (!ga_uses_proc_grid_(g_a)) {
      Integer idx;
      for (idx=me; idx<num_blocks_a; idx += nproc) {
        nga_distribution_(g_a, &idx, lo, hi);
        nga_access_block_ptr(g_a, &idx, &ptr, &ld);
        gai_scale_col_values(type, lo, hi, ld, ptr, g_v);
        nga_release_update_block_(g_a, &idx);
      }
    } else {
      Integer proc_index[MAXDIM], index[MAXDIM];
      Integer topology[MAXDIM];
      Integer blocks[MAXDIM], block_dims[MAXDIM];
      ga_get_proc_index_(g_a, &me, proc_index);
      ga_get_proc_index_(g_a, &me, index);
      ga_get_block_info_(g_a, blocks, block_dims);
      ga_get_proc_grid_(g_a, topology);

      while (index[andim-1] < blocks[andim-1]) {
        /* find bounding coordinates of block */
        chk = 1;
        for (i = 0; i < andim; i++) {
          lo[i] = index[i]*block_dims[i]+1;
          hi[i] = (index[i] + 1)*block_dims[i];
          if (hi[i] > adims[i]) hi[i] = adims[i];
          if (hi[i] < lo[i]) chk = 0;
        }
        if (chk) {
          nga_access_block_grid_ptr(g_a, index, &ptr, &ld);
          gai_scale_col_values(type, lo, hi, ld, ptr, g_v);
          nga_release_update_block_grid_(g_a, index);
        }
        /* increment index to get next block on processor */
        index[0] += topology[0];
        for (i = 0; i < andim; i++) {
          if (index[i] >= blocks[i] && i<andim-1) {
            index[i] = proc_index[i];
            index[i+1] += topology[i+1];
          }
        }
      }
    }
  }
  GA_POP_NAME;
  if(local_sync_end)ga_sync_();
}
