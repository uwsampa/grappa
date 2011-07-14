#if HAVE_CONFIG_H
#   include "config.h"
#endif

/*************************************************************************\
 Purpose:   File global.nalg.c contains a set of linear algebra routines 
            that operate on n-dim global arrays in the SPMD mode. 
 
 Date: 10.22.98
 Author: Jarek Nieplocha
\************************************************************************/

 
#if HAVE_STDIO_H
#   include <stdio.h>
#endif
#include "message.h"
#include "global.h"
#include "globalp.h"
#include "armci.h"

#ifdef USE_VAMPIR
#include "ga_vampir.h"
#endif

/* work arrays used in all routines */
static Integer dims[MAXDIM], ld[MAXDIM-1];
static Integer lo[MAXDIM],hi[MAXDIM];
static Integer one_arr[MAXDIM]={1,1,1,1,1,1,1};

#define GET_ELEMS(ndim,lo,hi,ld,pelems){\
int _i;\
      for(_i=0, *pelems = hi[ndim-1]-lo[ndim-1]+1; _i< ndim-1;_i++) {\
         if(ld[_i] != (hi[_i]-lo[_i]+1)) gai_error("layout problem",_i);\
         *pelems *= hi[_i]-lo[_i]+1;\
      }\
}

#define GET_ELEMS_W_GHOSTS(ndim,lo,hi,ld,pelems){\
int _i;\
      for(_i=0, *pelems = hi[ndim-1]-lo[ndim-1]+1; _i< ndim-1;_i++) {\
         if(ld[_i] < (hi[_i]-lo[_i]+1))\
           gai_error("layout problem with ghosts",_i);\
         *pelems *= hi[_i]-lo[_i]+1;\
      }\
}


void FATR ga_zero_(Integer *g_a)
{
  Integer ndim, type, me, elems, p_handle;
  Integer num_blocks;
  void *ptr;
  register Integer i;
  int local_sync_begin,local_sync_end;

#ifdef USE_VAMPIR
  vampir_begin(GA_ZERO,__FILE__,__LINE__);
#endif

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  p_handle = ga_get_pgroup_(g_a);

  if(local_sync_begin) ga_pgroup_sync_(&p_handle);

  me = ga_pgroup_nodeid_(&p_handle);

  gai_check_handle(g_a, "ga_zero");
  GA_PUSH_NAME("ga_zero");

  num_blocks = ga_total_blocks_(g_a);

  nga_inquire_internal_(g_a, &type, &ndim, dims);
  if (num_blocks < 0) {
    nga_distribution_(g_a, &me, lo, hi);

    if ( lo[0]> 0 ){ /* base index is 1: we get 0 if no elements stored on p */

      if (ga_has_ghosts_(g_a)) {
        nga_zero_patch_(g_a,lo,hi);
#ifdef USE_VAMPIR
        vampir_end(GA_ZERO,__FILE__,__LINE__);
#endif
        return;
      }
      nga_access_ptr(g_a, lo, hi, &ptr, ld);
      GET_ELEMS(ndim,lo,hi,ld,&elems);

      switch (type){
        int *ia;
        double *da;
        float *fa;
        long *la;
        long long *lla;
        case C_INT:
        ia = (int*)ptr;
        for(i=0;i<elems;i++) ia[i]  = 0;
        break;
        case C_DCPL:
        elems *=2;
        case C_DBL:
        da = (double*)ptr;
        for(i=0;i<elems;i++) da[i] = 0;
        break;
        case C_SCPL:
        elems *=2;
        case C_FLOAT:
        fa = (float*)ptr;
        for(i=0;i<elems;i++) fa[i]  = 0;
        break;
        case C_LONG:
        la = (long*)ptr;
        for(i=0;i<elems;i++) la[i]  = 0;
        break;                                 
        case C_LONGLONG:
        lla = (long long*)ptr;
        for(i=0;i<elems;i++) lla[i]  = 0;
        break;                                 
        default: gai_error(" wrong data type ",type);
      }

      /* release access to the data */
      nga_release_update_(g_a, lo, hi);
    } 
  } else {
    nga_access_block_segment_ptr(g_a, &me, &ptr, &elems);
    switch (type){
      int *ia;
      double *da;
      float *fa;
      long *la;
      long long *lla;
      case C_INT:
        ia = (int*)ptr;
        for(i=0;i<elems;i++) ia[i]  = 0;
        break;
      case C_DCPL:
        elems *=2;
      case C_DBL:
        da = (double*)ptr;
        for(i=0;i<elems;i++) da[i] = 0;
        break;
      case C_SCPL:
        elems *=2;
      case C_FLOAT:
        fa = (float*)ptr;
        for(i=0;i<elems;i++) fa[i]  = 0;
        break;
      case C_LONG:
      la = (long*)ptr;
      for(i=0;i<elems;i++) la[i]  = 0;
      break;                                 
      case C_LONGLONG:
      lla = (long long*)ptr;
      for(i=0;i<elems;i++) lla[i]  = 0;
      break;                                 
      default: gai_error(" wrong data type ",type);
    }

    /* release access to the data */
    nga_release_update_block_segment_(g_a, &me);
  }
  if(local_sync_end)ga_pgroup_sync_(&p_handle);
  GA_POP_NAME;
#ifdef USE_VAMPIR
  vampir_end(GA_ZERO,__FILE__,__LINE__);
#endif
}



/*\ COPY ONE GLOBAL ARRAY INTO ANOTHER
\*/
void FATR ga_copy_old(Integer *g_a, Integer *g_b)
{
Integer  ndim, ndimb, type, typeb, me, elems=0, elemsb=0;
Integer dimsb[MAXDIM];
void *ptr_a, *ptr_b;

   me = ga_nodeid_();

   GA_PUSH_NAME("ga_copy");

   if(*g_a == *g_b) gai_error("arrays have to be different ", 0L);

   nga_inquire_internal_(g_a,  &type, &ndim, dims);
   nga_inquire_internal_(g_b,  &typeb, &ndimb, dimsb);

   if(type != typeb) gai_error("types not the same", *g_b);

   if(!ga_compare_distr_(g_a,g_b))

      ngai_copy_patch("n",g_a, one_arr, dims, g_b, one_arr, dimsb);

   else {

     ga_sync_();

     nga_distribution_(g_a, &me, lo, hi);
     if(lo[0]>0){
        nga_access_ptr(g_a, lo, hi, &ptr_a, ld);
        if (ga_has_ghosts_(g_a)) {
          GET_ELEMS_W_GHOSTS(ndim,lo,hi,ld,&elems);
        } else {
          GET_ELEMS(ndim,lo,hi,ld,&elems);
        }
     }

     nga_distribution_(g_b, &me, lo, hi);
     if(lo[0]>0){
        nga_access_ptr(g_b, lo, hi, &ptr_b, ld);
        if (ga_has_ghosts_(g_b)) {
          GET_ELEMS_W_GHOSTS(ndim,lo,hi,ld,&elems);
        } else {
          GET_ELEMS(ndim,lo,hi,ld,&elems);
        }
     }
  
     if(elems!= elemsb)gai_error("inconsistent number of elements",elems-elemsb);

     if(elems>0){
        ARMCI_Copy(ptr_a, ptr_b, (int)elems*GAsizeofM(type));
        nga_release_(g_a,lo,hi);
        nga_release_(g_b,lo,hi);
     }

     ga_sync_();
   }

   GA_POP_NAME;
}



/*\ COPY ONE GLOBAL ARRAY INTO ANOTHER
\*/
void FATR ga_copy_(Integer *g_a, Integer *g_b)
{
Integer  ndim, ndimb, type, typeb, me_a, me_b;
Integer dimsb[MAXDIM],i;
Integer nseg;
Integer a_grp, b_grp, anproc, bnproc;
Integer num_blocks_a, num_blocks_b;
Integer blocks[MAXDIM], block_dims[MAXDIM];
void *ptr_a, *ptr_b;
int local_sync_begin,local_sync_end,use_put;

#ifdef USE_VAMPIR
   vampir_begin(GA_COPY,__FILE__,__LINE__);
#endif
   GA_PUSH_NAME("ga_copy");

   local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
   _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
   a_grp = ga_get_pgroup_(g_a);
   b_grp = ga_get_pgroup_(g_b);
   me_a = ga_pgroup_nodeid_(&a_grp);
   me_b = ga_pgroup_nodeid_(&b_grp);
   anproc = ga_get_pgroup_size_(&a_grp);
   bnproc = ga_get_pgroup_size_(&b_grp);
   num_blocks_a = ga_total_blocks_(g_a);
   num_blocks_b = ga_total_blocks_(g_b);
   if (anproc <= bnproc) {
     use_put = 1;
   } else {
     use_put = 0;
   }
   /*if (a_grp != b_grp)
     gai_error("Both arrays must be defined on same group",0L); */
   if(local_sync_begin) {
     if (anproc <= bnproc) {
       ga_pgroup_sync_(&a_grp);
     } else if (a_grp == ga_pgroup_get_world_() &&
                b_grp == ga_pgroup_get_world_()) {
       ga_sync_();
     } else {
       ga_pgroup_sync_(&b_grp);
     }
   }

   if(*g_a == *g_b) gai_error("arrays have to be different ", 0L);

   nga_inquire_internal_(g_a,  &type, &ndim, dims);
   nga_inquire_internal_(g_b,  &typeb, &ndimb, dimsb);

   if(type != typeb) gai_error("types not the same", *g_b);
   if(ndim != ndimb) gai_error("dimensions not the same", ndimb);

   for(i=0; i< ndim; i++)if(dims[i]!=dimsb[i]) 
                          gai_error("dimensions not the same",i);

   if ((ga_is_mirrored_(g_a) && ga_is_mirrored_(g_b)) ||
       (!ga_is_mirrored_(g_a) && !ga_is_mirrored_(g_b))) {
     /* Both global arrays are mirrored or both global arrays are not mirrored.
        Copy operation is straightforward */

     if (use_put) {
       if (num_blocks_a < 0) {
         nga_distribution_(g_a, &me_a, lo, hi);
         if(lo[0]>0){
           nga_access_ptr(g_a, lo, hi, &ptr_a, ld);
           nga_put_(g_b, lo, hi, ptr_a, ld);
         }
       } else {
         if (!ga_uses_proc_grid_(g_a)) {
           for (i=me_a; i<num_blocks_a; i += anproc) {
             nga_distribution_(g_a, &i, lo, hi);
             if (lo[0]>0) {
               nga_access_block_ptr(g_a, &i, &ptr_a, ld);
               nga_put_(g_b, lo, hi, ptr_a, ld);
             }
           }
         } else {
           Integer proc_index[MAXDIM], index[MAXDIM];
           Integer topology[MAXDIM], chk;
           ga_get_proc_index_(g_a, &me_a, proc_index);
           ga_get_proc_index_(g_a, &me_a, index);
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
               nga_access_block_grid_ptr(g_a, index, &ptr_a, ld);
               nga_put_(g_b, lo, hi, ptr_a, ld);
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
     } else {
       if (num_blocks_b < 0) {
         nga_distribution_(g_b, &me_b, lo, hi);
         if(lo[0]>0){
           nga_access_ptr(g_b, lo, hi, &ptr_b, ld);
           nga_get_(g_a, lo, hi, ptr_b, ld);
         }
       } else {
         if (!ga_uses_proc_grid_(g_a)) {
           for (i=me_b; i<num_blocks_b; i += bnproc) {
             nga_distribution_(g_b, &i, lo, hi);
             if (lo[0]>0) {
               nga_access_block_ptr(g_b, &i, &ptr_b, ld);
               nga_get_(g_a, lo, hi, ptr_b, ld);
             }
           }
         } else {
           Integer proc_index[MAXDIM], index[MAXDIM];
           Integer topology[MAXDIM], chk;
           ga_get_proc_index_(g_b, &me_b, proc_index);
           ga_get_proc_index_(g_b, &me_b, index);
           ga_get_block_info_(g_b, blocks, block_dims);
           ga_get_proc_grid_(g_b, topology);
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
               nga_access_block_grid_ptr(g_b, index, &ptr_b, ld);
               nga_get_(g_a, lo, hi, ptr_b, ld);
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
     }
   } else {
     /* One global array is mirrored and the other is not */
     if (ga_is_mirrored_(g_a)) {
       /* Source array is mirrored and destination
          array is distributed. Assume source array is consistent */
       nga_distribution_(g_b, &me_b, lo, hi);
       if (lo[0]>0) {
         nga_access_ptr(g_b, lo, hi, &ptr_b, ld);
         nga_get_(g_a, lo, hi, ptr_b, ld);
       } 
     } else {
       /* source array is distributed and destination
          array is mirrored */
       ga_zero_(g_b);
       if (ndim == 1) {
         nseg = ga_num_mirrored_seg_(g_b);
         for (i=0; i< nseg; i++) {
           ga_get_mirrored_block_(g_b, &i, lo, hi);
           nga_access_ptr(g_b, lo, hi, &ptr_b, ld);
           nga_get_(g_a, lo, hi, ptr_b, ld);
         }
         ga_fast_merge_mirrored_(g_b);
       } else {
         nga_distribution_(g_a, &me_a, lo, hi);
         if (lo[0] > 0) {
           nga_access_ptr(g_a, lo, hi, &ptr_a, ld);
           nga_put_(g_b, lo, hi, ptr_a, ld);
         }
         ga_merge_mirrored_(g_b);
       }
     }
   }

   if(local_sync_end) {
     if (anproc <= bnproc) {
       ga_pgroup_sync_(&a_grp);
     } else if (a_grp == ga_pgroup_get_world_() &&
                b_grp == ga_pgroup_get_world_()) {
       ga_sync_();
     } else {
       ga_pgroup_sync_(&b_grp);
     }
   }
   GA_POP_NAME;
#ifdef USE_VAMPIR
   vampir_end(GA_COPY,__FILE__,__LINE__);
#endif
}



/*\ internal version of dot product
\*/
void gai_dot(int Type, Integer *g_a, Integer *g_b, void *value)
{
Integer  ndim=0, type=0, atype=0, me=0, elems=0, elemsb=0;
register Integer i=0;
int isum=0;
long lsum=0;
long long llsum=0;
DoubleComplex zsum ={0.,0.};
SingleComplex csum ={0.,0.};
float fsum=0.0;
void *ptr_a=NULL, *ptr_b=NULL;
int alen=0;
Integer a_grp=0, b_grp=0;
Integer num_blocks_a=0, num_blocks_b=0;

Integer andim, adims[MAXDIM];
Integer bndim, bdims[MAXDIM];

   _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/

   GA_PUSH_NAME("ga_dot");
   a_grp = ga_get_pgroup_(g_a);
   b_grp = ga_get_pgroup_(g_b);
   if (a_grp != b_grp)
     gai_error("Both arrays must be defined on same group",0L);
   me = ga_pgroup_nodeid_(&a_grp);

   /* Check to see if either GA is block cyclic distributed */
   num_blocks_a = ga_total_blocks_(g_a);
   num_blocks_b = ga_total_blocks_(g_b);
   if (num_blocks_a >= 0 || num_blocks_b >= 0) {
     nga_inquire_internal_(g_a, &type, &andim, adims);
     nga_inquire_internal_(g_b, &type, &bndim, bdims);
     ngai_dot_patch(g_a, "n", one_arr, adims, g_b, "n", one_arr, bdims,
         value);
     GA_POP_NAME;
     return;
   }

   if(ga_compare_distr_(g_a,g_b) == FALSE ||
      ga_has_ghosts_(g_a) || ga_has_ghosts_(g_b)) {
       /* distributions not identical */
       nga_inquire_internal_(g_a, &type, &andim, adims);
       nga_inquire_internal_(g_b, &type, &bndim, bdims);

       ngai_dot_patch(g_a, "n", one_arr, adims, g_b, "n", one_arr, bdims,
                      value);
       
       GA_POP_NAME;
       return;
   }
   
   ga_pgroup_sync_(&a_grp);
   nga_inquire_internal_(g_a,  &type, &ndim, dims);
   if(type != Type) gai_error("type not correct", *g_a);
   nga_distribution_(g_a, &me, lo, hi);
   if(lo[0]>0){
      nga_access_ptr(g_a, lo, hi, &ptr_a, ld);
      if (ga_has_ghosts_(g_a)) {
        GET_ELEMS_W_GHOSTS(ndim,lo,hi,ld,&elems);
      } else {
        GET_ELEMS(ndim,lo,hi,ld,&elems);
      }
   }

   if(*g_a == *g_b){
     elemsb = elems;
     ptr_b = ptr_a;
   }else {  
     nga_inquire_internal_(g_b,  &type, &ndim, dims);
     if(type != Type) gai_error("type not correct", *g_b);
     nga_distribution_(g_b, &me, lo, hi);
     if(lo[0]>0){
        nga_access_ptr(g_b, lo, hi, &ptr_b, ld);
        if (ga_has_ghosts_(g_b)) {
          GET_ELEMS_W_GHOSTS(ndim,lo,hi,ld,&elemsb);
        } else {
          GET_ELEMS(ndim,lo,hi,ld,&elemsb);
        }
     }
   }

   if(elems!= elemsb)gai_error("inconsistent number of elements",elems-elemsb); 


      /* compute "local" contribution to the dot product */
      switch (type){
	int *ia, *ib;
	double *da,*db;
        float *fa, *fb;
        long *la,*lb;
        long long *lla,*llb;
        case C_INT:
           ia = (int*)ptr_a;
           ib = (int*)ptr_b;
           for(i=0;i<elems;i++) 
                 isum += ia[i]  * ib[i];
           *(int*)value = isum; 
           type = C_INT;
           alen = 1;
           break;

        case C_DCPL:
           for(i=0;i<elems;i++){
               DoubleComplex a = ((DoubleComplex*)ptr_a)[i];
               DoubleComplex b = ((DoubleComplex*)ptr_b)[i];
               zsum.real += a.real*b.real  - b.imag * a.imag;
               zsum.imag += a.imag*b.real  + b.imag * a.real;
           }
           *(DoubleComplex*)value = zsum; 
           type = C_DCPL;
           alen = 2;
           break;

        case C_SCPL:
           for(i=0;i<elems;i++){
               SingleComplex a = ((SingleComplex*)ptr_a)[i];
               SingleComplex b = ((SingleComplex*)ptr_b)[i];
               csum.real += a.real*b.real  - b.imag * a.imag;
               csum.imag += a.imag*b.real  + b.imag * a.real;
           }
           *(SingleComplex*)value = csum; 
           type = C_SCPL;
           alen = 2;
           break;

        case C_DBL:
           da = (double*)ptr_a;
           db = (double*)ptr_b;
           for(i=0;i<elems;i++) 
                 zsum.real += da[i]  * db[i];
           *(double*)value = zsum.real; 
           type = C_DBL;
           alen = 1;
           break;
        case C_FLOAT:
           fa = (float*)ptr_a;
           fb = (float*)ptr_b;
           for(i=0;i<elems;i++)
                 fsum += fa[i]  * fb[i];
           *(float*)value = fsum;
           type = C_FLOAT;
           alen = 1;
           break;         
        case C_LONG:
           la = (long*)ptr_a;
           lb = (long*)ptr_b;
           for(i=0;i<elems;i++)
		lsum += la[i]  * lb[i];
           *(long*)value = lsum;
           type = C_LONG;
           alen = 1;
           break;               
        case C_LONGLONG:
           lla = (long long*)ptr_a;
           llb = (long long*)ptr_b;
           for(i=0;i<elems;i++)
		llsum += lla[i]  * llb[i];
           *(long long*)value = llsum;
           type = C_LONGLONG;
           alen = 1;
           break;               
        default: gai_error(" wrong data type ",type);
      }
   
      /* release access to the data */
      if(elems>0){
         nga_release_(g_a, lo, hi);
         if(*g_a != *g_b)nga_release_(g_b, lo, hi);
      }

    /*convert from C data type to ARMCI type */
    switch(type) {
      case C_FLOAT: atype=ARMCI_FLOAT; break;
      case C_DBL: atype=ARMCI_DOUBLE; break;
      case C_INT: atype=ARMCI_INT; break;
      case C_LONG: atype=ARMCI_LONG; break;
      case C_LONGLONG: atype=ARMCI_LONG_LONG; break;
      case C_DCPL: atype=ARMCI_DOUBLE; break;
      case C_SCPL: atype=ARMCI_FLOAT; break;
      default: gai_error("gai_dot: type not supported",type);
    }

   if (ga_is_mirrored_(g_a) && ga_is_mirrored_(g_b)) {
     armci_msg_gop_scope(SCOPE_NODE,value,alen,"+",atype);
   } else {
#ifdef MPI
     extern ARMCI_Group* ga_get_armci_group_(int);
#endif
     if (a_grp == -1) {
       armci_msg_gop_scope(SCOPE_ALL,value,alen,"+",atype);
#ifdef MPI
     } else {
       armci_msg_group_gop_scope(SCOPE_ALL,value,alen,"+",atype,
           ga_get_armci_group_((int)a_grp));
#endif
     }
   }
    
   GA_POP_NAME;

}


Integer FATR ga_idot_(g_a, g_b)
        Integer *g_a, *g_b;
{
Integer sum;

#ifdef USE_VAMPIR
        vampir_begin(GA_IDOT,__FILE__,__LINE__);
#endif

        gai_dot(ga_type_f2c(MT_F_INT), g_a, g_b, &sum);

#ifdef USE_VAMPIR
        vampir_end(GA_IDOT,__FILE__,__LINE__);
#endif

        return sum;
}


DoublePrecision FATR ga_ddot_(g_a, g_b)
        Integer *g_a, *g_b;
{
DoublePrecision sum;

#ifdef USE_VAMPIR
        vampir_begin(GA_DDOT,__FILE__,__LINE__);
#endif

        gai_dot(ga_type_f2c(MT_F_DBL), g_a, g_b, &sum);

#ifdef USE_VAMPIR
        vampir_end(GA_DDOT,__FILE__,__LINE__);
#endif

        return sum;
}


Real FATR ga_sdot_(g_a, g_b)
        Integer *g_a, *g_b;
{
Real sum;

#ifdef USE_VAMPIR
        vampir_begin(GA_SDOT,__FILE__,__LINE__);
#endif

        gai_dot(ga_type_f2c(MT_F_REAL), g_a, g_b, &sum);

#ifdef USE_VAMPIR
        vampir_end(GA_SDOT,__FILE__,__LINE__);
#endif

        return sum;
}            


void gai_zdot_(Integer *g_a, Integer *g_b, DoubleComplex *sum)
{
#ifdef USE_VAMPIR
        vampir_begin(GA_ZDOT,__FILE__,__LINE__);
#endif

        gai_dot(ga_type_f2c(MT_F_DCPL), g_a, g_b, sum);

#ifdef USE_VAMPIR
        vampir_end(GA_ZDOT,__FILE__,__LINE__);
#endif
}


void gai_cdot_(Integer *g_a, Integer *g_b, SingleComplex *sum)
{
#ifdef USE_VAMPIR
        vampir_begin(GA_CDOT,__FILE__,__LINE__);
#endif

        gai_dot(ga_type_f2c(MT_F_SCPL), g_a, g_b, sum);

#ifdef USE_VAMPIR
        vampir_end(GA_CDOT,__FILE__,__LINE__);
#endif
}


void FATR ga_scale_(Integer *g_a, void* alpha)
{
  Integer ndim, type, me, elems, grp_id;
  register Integer i;
  Integer num_blocks;
  void *ptr;
  int local_sync_begin,local_sync_end;

#ifdef USE_VAMPIR
  vampir_begin(GA_SCALE,__FILE__,__LINE__);
#endif

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  grp_id = ga_get_pgroup_(g_a);
  if(local_sync_begin)ga_pgroup_sync_(&grp_id);

  me = ga_pgroup_nodeid_(&grp_id);

  gai_check_handle(g_a, "ga_scale");
  GA_PUSH_NAME("ga_scale");
  num_blocks = ga_total_blocks_(g_a);

  nga_inquire_internal_(g_a, &type, &ndim, dims);
  if (num_blocks < 0) {
    nga_distribution_(g_a, &me, lo, hi);
    if (ga_has_ghosts_(g_a)) {
      nga_scale_patch_(g_a, lo, hi, alpha);
#ifdef USE_VAMPIR
      vampir_end(GA_SCALE,__FILE__,__LINE__);
#endif
      return;
    }

    if ( lo[0]> 0 ){ /* base index is 1: we get 0 if no elements stored on p */

      nga_access_ptr(g_a, lo, hi, &ptr, ld);
      GET_ELEMS(ndim,lo,hi,ld,&elems);

      switch (type){
        int *ia;
        double *da;
        DoubleComplex *ca, scale;
        SingleComplex *cfa, cfscale;
        long *la;
        long long *lla;
        float *fa;
        case C_INT:
        ia = (int*)ptr;
        for(i=0;i<elems;i++) ia[i]  *= *(int*)alpha;
        break;
        case C_LONG:
        la = (long*)ptr;
        for(i=0;i<elems;i++) la[i]  *= *(long*)alpha;
        break;
        case C_LONGLONG:
        lla = (long long*)ptr;
        for(i=0;i<elems;i++) lla[i]  *= *(long long*)alpha;
        break;
        case C_DCPL:
        ca = (DoubleComplex*)ptr;
        scale= *(DoubleComplex*)alpha;
        for(i=0;i<elems;i++){
          DoubleComplex val = ca[i]; 
          ca[i].real = scale.real*val.real  - val.imag * scale.imag;
          ca[i].imag = scale.imag*val.real  + val.imag * scale.real;
        }
        break;
        case C_SCPL:
        cfa = (SingleComplex*)ptr;
        cfscale= *(SingleComplex*)alpha;
        for(i=0;i<elems;i++){
          SingleComplex val = cfa[i]; 
          cfa[i].real = cfscale.real*val.real  - val.imag * cfscale.imag;
          cfa[i].imag = cfscale.imag*val.real  + val.imag * cfscale.real;
        }
        break;
        case C_DBL:
        da = (double*)ptr;
        for(i=0;i<elems;i++) da[i] *= *(double*)alpha;
        break;
        case C_FLOAT:
        fa = (float*)ptr;
        for(i=0;i<elems;i++) fa[i]  *= *(float*)alpha;
        break;       
        default: gai_error(" wrong data type ",type);
      }

      /* release access to the data */
      nga_release_update_(g_a, lo, hi);
    }
  } else {
    nga_access_block_segment_ptr(g_a, &me, &ptr, &elems);
    switch (type){
      int *ia;
      double *da;
      DoubleComplex *ca, scale;
      SingleComplex *cfa, cfscale;
      long *la;
      long long *lla;
      float *fa;
      case C_INT:
      ia = (int*)ptr;
      for(i=0;i<elems;i++) ia[i]  *= *(int*)alpha;
      break;
      case C_LONG:
      la = (long*)ptr;
      for(i=0;i<elems;i++) la[i]  *= *(long*)alpha;
      break;
      case C_LONGLONG:
      lla = (long long*)ptr;
      for(i=0;i<elems;i++) lla[i]  *= *(long long*)alpha;
      break;
      case C_DCPL:
      ca = (DoubleComplex*)ptr;
      scale= *(DoubleComplex*)alpha;
      for(i=0;i<elems;i++){
        DoubleComplex val = ca[i]; 
        ca[i].real = scale.real*val.real  - val.imag * scale.imag;
        ca[i].imag = scale.imag*val.real  + val.imag * scale.real;
      }
      break;
      case C_SCPL:
      cfa = (SingleComplex*)ptr;
      cfscale= *(SingleComplex*)alpha;
      for(i=0;i<elems;i++){
        SingleComplex val = cfa[i]; 
        cfa[i].real = cfscale.real*val.real  - val.imag * cfscale.imag;
        cfa[i].imag = cfscale.imag*val.real  + val.imag * cfscale.real;
      }
      break;
      case C_DBL:
      da = (double*)ptr;
      for(i=0;i<elems;i++) da[i] *= *(double*)alpha;
      break;
      case C_FLOAT:
      fa = (float*)ptr;
      for(i=0;i<elems;i++) fa[i]  *= *(float*)alpha;
      break;       
      default: gai_error(" wrong data type ",type);
    }
    /* release access to the data */
    nga_release_update_block_segment_(g_a, &me);
  }
  GA_POP_NAME;
  if(local_sync_end)ga_pgroup_sync_(&grp_id); 
#ifdef USE_VAMPIR
  vampir_end(GA_SCALE,__FILE__,__LINE__);
#endif
}

/* (old?) Fortran interface to ga_scale_ */

void FATR ga_cscal_(Integer *g_a, SingleComplex *alpha)
{
    ga_scale_(g_a, alpha);
}


void FATR ga_dscal_(Integer *g_a, DoublePrecision *alpha)
{
    ga_scale_(g_a, alpha);
}


void FATR ga_iscal_(Integer *g_a, Integer *alpha)
{
    ga_scale_(g_a, alpha);
}


void FATR ga_sscal_(Integer *g_a, Real *alpha)
{
    ga_scale_(g_a, alpha);
}


void FATR ga_zscal_(Integer *g_a, DoubleComplex *alpha)
{
    ga_scale_(g_a, alpha);
}


void FATR ga_add_(void *alpha, Integer* g_a, 
                  void* beta, Integer* g_b, Integer* g_c)
{
Integer  ndim, type, typeC, me, elems=0, elemsb=0, elemsa=0;
register Integer i;
void *ptr_a, *ptr_b, *ptr_c;
Integer a_grp, b_grp, c_grp;
int local_sync_begin,local_sync_end;

 Integer andim, adims[MAXDIM];
 Integer bndim, bdims[MAXDIM];
 Integer cndim, cdims[MAXDIM];
 
#ifdef USE_VAMPIR
   vampir_begin(GA_ADD,__FILE__,__LINE__);
#endif

   local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
   _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/


   GA_PUSH_NAME("ga_add");
   a_grp = ga_get_pgroup_(g_a);
   b_grp = ga_get_pgroup_(g_b);
   c_grp = ga_get_pgroup_(g_c);
   if (a_grp != b_grp || b_grp != c_grp)
     gai_error("All three arrays must be on same group for ga_add",0L);

   me = ga_pgroup_nodeid_(&a_grp);
   if((ga_compare_distr_(g_a,g_b) == FALSE) ||
      (ga_compare_distr_(g_a,g_c) == FALSE) ||
       ga_has_ghosts_(g_a) || ga_has_ghosts_(g_b) || ga_has_ghosts_(g_c) ||
       ga_total_blocks_(g_a) > 0 || ga_total_blocks_(g_b) > 0 ||
       ga_total_blocks_(g_c) > 0) {
       /* distributions not identical */
       nga_inquire_internal_(g_a, &type, &andim, adims);
       nga_inquire_internal_(g_b, &type, &bndim, bdims);
       nga_inquire_internal_(g_b, &type, &cndim, cdims);

       nga_add_patch_(alpha, g_a, one_arr, adims, beta, g_b, one_arr, bdims,
                      g_c, one_arr, cdims);
       
       GA_POP_NAME;
#ifdef USE_VAMPIR
       vampir_end(GA_ADD,__FILE__,__LINE__);
#endif
       return;
   }

   ga_pgroup_sync_(&a_grp);
   nga_inquire_internal_(g_c,  &typeC, &ndim, dims);
   nga_distribution_(g_c, &me, lo, hi);
   if (  lo[0]>0 ){
     nga_access_ptr(g_c, lo, hi, &ptr_c, ld);
     GET_ELEMS(ndim,lo,hi,ld,&elems);
   }

   if(*g_a == *g_c){
     ptr_a  = ptr_c;
     elemsa = elems;
   }else { 
     nga_inquire_internal_(g_a,  &type, &ndim, dims);
     if(type != typeC) gai_error("types not consistent", *g_a);
     nga_distribution_(g_a, &me, lo, hi);
     if (  lo[0]>0 ){
       nga_access_ptr(g_a, lo, hi, &ptr_a, ld);
       GET_ELEMS(ndim,lo,hi,ld,&elemsa);
     }
   }

   if(*g_b == *g_c){
     ptr_b  = ptr_c;
     elemsb = elems;
   }else {
     nga_inquire_internal_(g_b,  &type, &ndim, dims);
     if(type != typeC) gai_error("types not consistent", *g_b);
     nga_distribution_(g_b, &me, lo, hi);
     if (  lo[0]>0 ){
       nga_access_ptr(g_b, lo, hi, &ptr_b, ld);
       GET_ELEMS(ndim,lo,hi,ld,&elemsb);
     }
   }

   if(elems!= elemsb)gai_error("inconsistent number of elements a",elems-elemsb);
   if(elems!= elemsa)gai_error("inconsistent number of elements b",elems-elemsa);

   if (  lo[0]>0 ){

       /* operation on the "local" piece of data */
       switch(type){
         int *ia, *ib, *ic;
         double *da,*db,*dc;
         float *fa, *fb, *fc;
         long *la,*lb,*lc;
         long long *lla,*llb,*llc;
         case C_DBL:
                  da = (double*)ptr_a;
                  db = (double*)ptr_b;
                  dc = (double*)ptr_c;
                  for(i=0; i<elems; i++)
                      dc[i] = *(double*)alpha *da[i] +
                              *(double*)beta * db[i];
              break;
         case C_DCPL:
                  for(i=0; i<elems; i++){
                     DoubleComplex a = ((DoubleComplex*)ptr_a)[i];
                     DoubleComplex b = ((DoubleComplex*)ptr_b)[i];
                     DoubleComplex *ac = (DoubleComplex*)ptr_c;
                     DoubleComplex x= *(DoubleComplex*)alpha;
                     DoubleComplex y= *(DoubleComplex*)beta;
                     /* c = x*a + y*b */
                     ac[i].real = x.real*a.real - 
                              x.imag*a.imag + y.real*b.real - y.imag*b.imag;
                     ac[i].imag = x.real*a.imag + 
                              x.imag*a.real + y.real*b.imag + y.imag*b.real;
                  }
              break;
         case C_SCPL:
                  for(i=0; i<elems; i++){
                     SingleComplex a = ((SingleComplex*)ptr_a)[i];
                     SingleComplex b = ((SingleComplex*)ptr_b)[i];
                     SingleComplex *ac = (SingleComplex*)ptr_c;
                     SingleComplex x= *(SingleComplex*)alpha;
                     SingleComplex y= *(SingleComplex*)beta;
                     /* c = x*a + y*b */
                     ac[i].real = x.real*a.real - 
                              x.imag*a.imag + y.real*b.real - y.imag*b.imag;
                     ac[i].imag = x.real*a.imag + 
                              x.imag*a.real + y.real*b.imag + y.imag*b.real;
                  }
              break;
         case C_FLOAT:
                  fa = (float*)ptr_a;
                  fb = (float*)ptr_b;
                  fc = (float*)ptr_c;
                  for(i=0; i<elems; i++)
                      fc[i] = *(float*)alpha *fa[i] + *(float*)beta *fb[i]; 
              break;
         case C_INT:
                  ia = (int*)ptr_a;
                  ib = (int*)ptr_b;
                  ic = (int*)ptr_c;
                  for(i=0; i<elems; i++) 
                      ic[i] = *(int*)alpha *ia[i] + *(int*)beta *ib[i];
              break;    
         case C_LONG:
                  la = (long*)ptr_a;
		  lb = (long*)ptr_b;
		  lc = (long*)ptr_c;
                  for(i=0; i<elems; i++)
                      lc[i] = *(long*)alpha *la[i] + *(long*)beta *lb[i];
              break;
         case C_LONGLONG:
                  lla = (long long*)ptr_a;
		  llb = (long long*)ptr_b;
		  llc = (long long*)ptr_c;
                  for(i=0; i<elems; i++)
                      llc[i] = ( *(long long*)alpha *lla[i] +
                                 *(long long*)beta  * llb[i] );
                
       }

       /* release access to the data */
       nga_release_update_(g_c, lo, hi);
       if(*g_c != *g_a)nga_release_(g_a, lo, hi);
       if(*g_c != *g_b)nga_release_(g_b, lo, hi);
   }


   GA_POP_NAME;
   if(local_sync_end)ga_pgroup_sync_(&a_grp);
#ifdef USE_VAMPIR
   vampir_end(GA_ADD,__FILE__,__LINE__);
#endif
}

/* (old?) Fortran interface to ga_add */

void FATR ga_cadd_(SingleComplex *alpha, Integer *g_a, SingleComplex *beta,
        Integer *g_b, Integer *g_c)
{
    ga_add_(alpha, g_a, beta, g_b, g_c);
}


void FATR ga_dadd_(DoublePrecision *alpha, Integer *g_a, DoublePrecision *beta,
        Integer *g_b, Integer *g_c)
{
    ga_add_(alpha, g_a, beta, g_b, g_c);
}


void FATR ga_iadd_(Integer *alpha, Integer *g_a, Integer *beta,
        Integer *g_b, Integer *g_c)
{
    ga_add_(alpha, g_a, beta, g_b, g_c);
}


void FATR ga_sadd_(Real *alpha, Integer *g_a, Real *beta,
        Integer *g_b, Integer *g_c)
{
    ga_add_(alpha, g_a, beta, g_b, g_c);
}


void FATR ga_zadd_(DoubleComplex *alpha, Integer *g_a, DoubleComplex *beta,
        Integer *g_b, Integer *g_c)
{
    ga_add_(alpha, g_a, beta, g_b, g_c);
}


static 
void gai_local_transpose(Integer type, char *ptra, Integer n, Integer stride, char *ptrb)
{
int i;
    switch(type){

       case C_INT:
            for(i = 0; i< n; i++, ptrb+= stride) 
               *(int*)ptrb= ((int*)ptra)[i];
            break;
       case C_DCPL:
            for(i = 0; i< n; i++, ptrb+= stride) 
               *(DoubleComplex*)ptrb= ((DoubleComplex*)ptra)[i];
            break;
       case C_SCPL:
            for(i = 0; i< n; i++, ptrb+= stride) 
               *(SingleComplex*)ptrb= ((SingleComplex*)ptra)[i];
            break;
       case C_DBL:
            for(i = 0; i< n; i++, ptrb+= stride) 
               *(double*)ptrb= ((double*)ptra)[i];
            break;
       case C_FLOAT:
            for(i = 0; i< n; i++, ptrb+= stride)
               *(float*)ptrb= ((float*)ptra)[i];
            break;      
       case C_LONG:
            for(i = 0; i< n; i++, ptrb+= stride)
               *(long*)ptrb= ((long*)ptra)[i];
            break;                                 
       case C_LONGLONG:
            for(i = 0; i< n; i++, ptrb+= stride)
               *(long long*)ptrb= ((long long*)ptra)[i];
            break;                                 
       default: gai_error("bad type:",type);
    }
}


void FATR ga_transpose_(Integer *g_a, Integer *g_b)
{
Integer me = ga_nodeid_();
Integer nproc = ga_nnodes_(); 
Integer atype, btype, andim, adims[MAXDIM], bndim, bdims[MAXDIM];
Integer lo[2],hi[2];
int local_sync_begin,local_sync_end;
Integer num_blocks_a;
char *ptr_tmp, *ptr_a;

#ifdef USE_VAMPIR
    vampir_begin(GA_TRANSPOSE,__FILE__,__LINE__);
#endif

    GA_PUSH_NAME("ga_transpose");
    
    local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
    _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
    if(local_sync_begin)ga_sync_();

    if(*g_a == *g_b) gai_error("arrays have to be different ", 0L);

    nga_inquire_internal_(g_a, &atype, &andim, adims);
    nga_inquire_internal_(g_b, &btype, &bndim, bdims);

    if(bndim != 2 || andim != 2) gai_error("dimension must be 2",0);
    if(atype != btype ) gai_error("array type mismatch ", 0L);

    num_blocks_a = ga_total_blocks_(g_a);

    if (num_blocks_a < 0) {
      nga_distribution_(g_a, &me, lo, hi);

      if(lo[0]>0){
        Integer nelem, lob[2], hib[2], nrow, ncol;
        int i, size=GAsizeofM(atype);

        nrow   = hi[0] -lo[0]+1;
        ncol   = hi[1] -lo[1]+1; 
        nelem  = nrow*ncol;
        lob[0] = lo[1]; lob[1] = lo[0];
        hib[0] = hi[1]; hib[1] = hi[0];

        /* allocate memory for transposing elements locally */
        ptr_tmp = (char *) ga_malloc(nelem, atype, "transpose_tmp");

        /* get access to local data */
        nga_access_ptr(g_a, lo, hi, &ptr_a, ld);

        for(i = 0; i < ncol; i++){
          char *ptr = ptr_tmp + i*size;

          gai_local_transpose(atype, ptr_a, nrow, ncol*size, ptr);
          ptr_a += ld[0]*size;
        }

        nga_release_(g_a, lo, hi); 

        nga_put_(g_b, lob, hib, ptr_tmp ,&ncol);

        ga_free(ptr_tmp);
      }
    } else {
      Integer idx;
      Integer blocks[MAXDIM], block_dims[MAXDIM];
      Integer nelem, lob[2], hib[2], nrow, ncol;
      int i, size=GAsizeofM(atype);

      /* allocate memory for transposing elements locally */
      ga_get_block_info_(g_a, blocks, block_dims);

      /* Simple block-cyclic data distribution */
      nelem = block_dims[0]*block_dims[1];
      ptr_tmp = (char *) ga_malloc(nelem, atype, "transpose_tmp");
      if (!ga_uses_proc_grid_(g_a)) {
        for (idx = me; idx < num_blocks_a; idx += nproc) {
          nga_distribution_(g_a, &idx, lo, hi);
          nga_access_block_ptr(g_a, &idx, &ptr_a, ld);

          nrow   = hi[0] -lo[0]+1;
          ncol   = hi[1] -lo[1]+1; 
          nelem  = nrow*ncol;
          lob[0] = lo[1]; lob[1] = lo[0];
          hib[0] = hi[1]; hib[1] = hi[0];
          for(i = 0; i < ncol; i++){
            char *ptr = ptr_tmp + i*size;

            gai_local_transpose(atype, ptr_a, nrow, ncol*size, ptr);
            ptr_a += ld[0]*size;
          }
          nga_put_(g_b, lob, hib, ptr_tmp ,&ncol);

          nga_release_update_block_(g_a, &idx);
        }
      } else {
        /* Uses scalapack block-cyclic data distribution */
        Integer chk;
        Integer proc_index[MAXDIM], index[MAXDIM];
        Integer topology[MAXDIM], ichk;

        ga_get_proc_index_(g_a, &me, proc_index);
        ga_get_proc_index_(g_a, &me, index);
        ga_get_block_info_(g_a, blocks, block_dims);
        ga_get_proc_grid_(g_a, topology);
        /* Verify that processor has data */
        ichk = 1;
        for (i=0; i<andim; i++) {
          if (index[i]<0 || index[i] >= blocks[i]) {
            ichk = 0;
          }
        }

        if (ichk) {
          nga_access_block_grid_ptr(g_a, index, &ptr_a, ld);
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
              nga_access_block_grid_ptr(g_a, index, &ptr_a, ld);
              nrow   = hi[0] -lo[0]+1;
              ncol   = hi[1] -lo[1]+1; 
              nelem  = nrow*ncol;
              lob[0] = lo[1]; lob[1] = lo[0];
              hib[0] = hi[1]; hib[1] = hi[0];
              for(i = 0; i < ncol; i++){
                char *ptr = ptr_tmp + i*size;
                gai_local_transpose(atype, ptr_a, nrow, block_dims[0]*size, ptr);
                ptr_a += ld[0]*size;
              }
              nga_put_(g_b, lob, hib, ptr_tmp ,&block_dims[0]);
              nga_release_update_block_(g_a, index);
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
      ga_free(ptr_tmp);
    }

    if(local_sync_end)ga_sync_();
    GA_POP_NAME;

#ifdef USE_VAMPIR
    vampir_end(GA_TRANSPOSE,__FILE__,__LINE__);
#endif

}
