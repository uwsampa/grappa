#if HAVE_CONFIG_H
#   include "config.h"
#endif


/**
 * Symmetrizes matrix A:  A := .5 * (A+A`)
 * diag(A) remains unchanged
 * 
 */

#include "global.h"
#include "globalp.h"
#include "macdecls.h"

void FATR 
gai_add(Integer *lo, Integer *hi, void *a, void *b, DoublePrecision alpha,
	Integer type, Integer nelem, Integer ndim) {

  Integer i, j, m=0;
  Integer nrow, ncol, indexA=0, indexB=0;
  DoublePrecision *A = (DoublePrecision *)a, *B = (DoublePrecision*)b;
  Integer offset1=1, offset2=1;

  nrow = hi[ndim-2] - lo[ndim-2] + 1;
  ncol = hi[ndim-1] - lo[ndim-1] + 1;
  
  for(i=0; i<ndim-2; i++) {
    offset1 *= hi[i] - lo[i] + 1;
    offset2 *= hi[i] - lo[i] + 1;
  }
  offset1 *= nrow;
  
  for(j=0; j<offset2; ++j,indexA=j,indexB=j,m=0) {
    for(i=0; i<nrow*ncol; i++, indexA += offset1, indexB += offset2) {
      if(indexA >= nelem) indexA = j + ++m*offset2;
      A[indexA] = alpha *(A[indexA] + B[indexB]);
    }
  }
}

void FATR 
ga_symmetrize_(Integer *g_a) {
  
  DoublePrecision alpha = 0.5;
  Integer i, me = ga_nodeid_();
  Integer alo[GA_MAX_DIM], ahi[GA_MAX_DIM], lda[GA_MAX_DIM], nelem=1;
  Integer blo[GA_MAX_DIM], bhi[GA_MAX_DIM], ldb[GA_MAX_DIM];
  Integer ndim, dims[GA_MAX_DIM], type;
  Logical have_data;
  Integer g_b; /* temporary global array (b = A') */
  Integer num_blocks_a;
  void *a_ptr=NULL, *b_ptr=NULL;
  int local_sync_begin,local_sync_end;
  char *tempB = "A_transpose";

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  if(local_sync_begin)ga_sync_();

  GA_PUSH_NAME("ga_symmetrize");
  
  num_blocks_a = ga_total_blocks_(g_a);

  nga_inquire_internal_(g_a, &type, &ndim, dims);

  if (type != C_DBL)
    gai_error("ga_symmetrize: only implemented for double precision",0);

  if (num_blocks_a < 0) {

    if (dims[ndim-1] != dims[ndim-2]) 
      gai_error("ga_sym: can only sym square matrix", 0L);

    /* Find the local distribution */
    nga_distribution_(g_a, &me, alo, ahi);


    have_data = ahi[0]>0;
    for(i=1; i<ndim; i++) have_data = have_data && ahi[i]>0;

    if(have_data) {
      nga_access_ptr(g_a, alo, ahi, &a_ptr, lda); 

      for(i=0; i<ndim; i++) nelem *= ahi[i]-alo[i] +1;
      b_ptr = (void *) ga_malloc(nelem, MT_F_DBL, "v");

      for(i=0; i<ndim-2; i++) {bhi[i]=ahi[i]; blo[i]=alo[i]; }

      /* switch rows and cols */
      blo[ndim-1]=alo[ndim-2];
      bhi[ndim-1]=ahi[ndim-2];
      blo[ndim-2]=alo[ndim-1];
      bhi[ndim-2]=ahi[ndim-1];

      for (i=0; i < ndim-1; i++) 
        ldb[i] = bhi[i] - blo[i] + 1; 
      nga_get_(g_a, blo, bhi, b_ptr, ldb);
    }
    ga_sync_(); 

    if(have_data) {
      gai_add(alo, ahi, a_ptr, b_ptr, alpha, type, nelem, ndim);
      nga_release_update_(g_a, alo, ahi);
      ga_free(b_ptr);
    }
  } else {
    /* For block-cyclic data, probably most efficient solution is to
       create duplicate copy, transpose it and add the results together */
    DoublePrecision half = 0.5;
    if (!gai_duplicate(g_a, &g_b, tempB))
      gai_error("ga_symmetrize: duplicate failed", 0L);
    ga_transpose_(g_a, &g_b);
    ga_add_(&half, g_a, &half, &g_b, g_a);
    ga_destroy_(&g_b);
  }
  GA_POP_NAME;
  if(local_sync_end)ga_sync_();
}
