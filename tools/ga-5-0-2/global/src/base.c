#if HAVE_CONFIG_H
#   include "config.h"
#endif

/* $Id: base.c,v 1.149.2.19 2007/12/18 18:42:20 d3g293 Exp $ */
/* 
 * module: base.c
 * author: Jarek Nieplocha
 * description: implements GA primitive operations --
 *              create (regular& irregular) and duplicate, destroy
 * 
 * DISCLAIMER
 *
 * This material was prepared as an account of work sponsored by an
 * agency of the United States Government.  Neither the United States
 * Government nor the United States Department of Energy, nor Battelle,
 * nor any of their employees, MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
 * ASSUMES ANY LEGAL LIABILITY OR RESPONSIBILITY FOR THE ACCURACY,
 * COMPLETENESS, OR USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT,
 * SOFTWARE, OR PROCESS DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT
 * INFRINGE PRIVATELY OWNED RIGHTS.
 *
 *
 * ACKNOWLEDGMENT
 *
 * This software and its documentation were produced with United States
 * Government support under Contract Number DE-AC06-76RLO-1830 awarded by
 * the United States Department of Energy.  The United States Government
 * retains a paid-up non-exclusive, irrevocable worldwide license to
 * reproduce, prepare derivative works, perform publicly and display
 * publicly by or for the US Government, including the right to
 * distribute to other US Government contractors.
 */

 
/*#define PERMUTE_PIDS */

#if HAVE_STDIO_H
#   include <stdio.h>
#endif
#if HAVE_STRING_H
#   include <string.h>
#endif
#if HAVE_STDLIB_H
#   include <stdlib.h>
#endif
#if HAVE_MATH_H
#   include <math.h>
#endif
#if HAVE_ASSERT_H
#   include <assert.h>
#endif

#include "farg.h"
#include "globalp.h"
#include "message.h"
#include "base.h"
#include "macdecls.h"
#include "armci.h"


#ifdef USE_VAMPIR
#include "ga_vampir.h"
#endif
#ifdef ENABLE_PROFILE
#include "ga_profile.h"
#endif
/*#define AVOID_MA_STORAGE 1*/ 
#define DEBUG 0
#define USE_MALLOC 1
#define INVALID_MA_HANDLE -1 
#define NEAR_INT(x) (x)< 0.0 ? ceil( (x) - 0.5) : floor((x) + 0.5)

#define MAPLEN  (GAnproc + MAXDIM)
#define FLEN        80              /* length of Fortran strings */

/*uncomment line below to verify consistency of MA in every sync */
/*#define CHECK_MA yes */

/*uncomment line below to verify if MA base address is alligned wrt datatype*/
#if !(defined(LINUX) || defined(CRAY) || defined(CYGWIN))
#define CHECK_MA_ALGN 1
#endif

/*uncomment line below to initialize arrays in ga_create/duplicate */
/*#define GA_CREATE_INDEF yes */

/*uncomment line below to introduce padding between shared memory regions 
  of a GA when the region spans in more than 1 process within SMP */
#define GA_ELEM_PADDING yes

global_array_t *_ga_main_data_structure;
global_array_t *GA;
proc_list_t *_proc_list_main_data_structure;
proc_list_t *PGRP_LIST;
static int GAinitialized = 0;
int _ga_sync_begin = 1;
int _ga_sync_end = 1;
int _max_global_array = MAX_ARRAYS;
Integer *GA_proclist;
int* GA_Proc_list = NULL;
int* GA_inv_Proc_list=NULL;
int GA_World_Proc_Group = -1;
int GA_Default_Proc_Group = -1;
int ga_armci_world_group=0;
int GA_Init_Proc_Group = -2;
Integer GA_Debug_flag = 0;
int *ProcPermList = NULL;

/* MA addressing */
DoubleComplex   *DCPL_MB;           /* double precision complex base address */
SingleComplex   *SCPL_MB;           /* single precision complex base address */
DoublePrecision *DBL_MB;            /* double precision base address */
Integer         *INT_MB;            /* integer base address */
float           *FLT_MB;            /* float base address */
int** GA_Update_Flags;
int* GA_Update_Signal;

typedef struct {
long id;
long type;
long size;
long dummy;
} getmem_t;

/* set total limit (bytes) for memory usage per processor to "unlimited" */
static Integer GA_total_memory = -1;
static Integer GA_memory_limited = 0;
struct ga_stat_t GAstat;
struct ga_bytes_t GAbytes ={0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.};
long   *GAstat_arr;
static Integer GA_memory_limit=0;
Integer GAme, GAnproc;
static Integer MPme;
Integer *mapALL;

#ifdef PERMUTE_PIDS
char** ptr_array;
#endif


char *GA_name_stack[NAME_STACK_LEN];  /* stack for storing names of GA ops */
int  GA_stack_size=0;

/* Function prototypes */
extern void gai_init_onesided();
int gai_getmem(char* name, char **ptr_arr, C_Long bytes, int type, long *id,
               int grp_id);
#ifdef ENABLE_CHECKPOINT
static int ga_group_is_for_ft=0;
int ga_spare_procs;
#endif

extern int _ga_initialize_args;


/*************************************************************************/

/*\ This macro computes index (place in ordered set) for the element
 *  identified by _subscript in ndim- dimensional array of dimensions _dim[]
 *  assume that first subscript component changes first
\*/
#define ga_ComputeIndexM(_index, _ndim, _subscript, _dims)                     \
{                                                                              \
  Integer  _i, _factor=1;                                                      \
  __CRAYX1_PRAGMA("_CRI novector");                                            \
  for(_i=0,*(_index)=0; _i<_ndim; _i++){                                       \
      *(_index) += _subscript[_i]*_factor;                                     \
      if(_i<_ndim-1)_factor *= _dims[_i];                                      \
  }                                                                            \
}


/*\ updates subscript corresponding to next element in a patch <lo[]:hi[]>
\*/
#define ga_UpdateSubscriptM(_ndim, _subscript, _lo, _hi, _dims)\
{                                                                              \
  Integer  _i;                                                                 \
  __CRAYX1_PRAGMA("_CRI novector");                                            \
  for(_i=0; _i<_ndim; _i++){                                                   \
       if(_subscript[_i] < _hi[_i]) { _subscript[_i]++; break;}                \
       _subscript[_i] = _lo[_i];                                               \
  }                                                                            \
}


/*\ Initialize n-dimensional loop by counting elements and setting subscript=lo
\*/
#define ga_InitLoopM(_elems, _ndim, _subscript, _lo, _hi, _dims)\
{                                                                              \
  Integer  _i;                                                                 \
  *_elems = 1;                                                                 \
  __CRAYX1_PRAGMA("_CRI novector");                                            \
  for(_i=0; _i<_ndim; _i++){                                                   \
       *_elems *= _hi[_i]-_lo[_i] +1;                                          \
       _subscript[_i] = _lo[_i];                                               \
  }                                                                            \
}


Integer GAsizeof(Integer type)    
{
  switch (type) {
     case C_DBL  : return (sizeof(double));
     case C_INT  : return (sizeof(int));
     case C_SCPL : return (sizeof(SingleComplex));
     case C_DCPL : return (sizeof(DoubleComplex));
     case C_FLOAT : return (sizeof(float));
     case C_LONG : return (sizeof(long));
     case C_LONGLONG : return (sizeof(long long));
          default   : return 0; 
  }
}


/*\ Register process list
 *  process list can be used to:
 *   1. permute process ids w.r.t. message-passing ids (set PERMUTE_PIDS), or
 *   2. change logical mapping of array blocks to processes
\*/
void ga_register_proclist_(Integer *list, Integer* np)
{
int i;

      GA_PUSH_NAME("ga_register_proclist");
      if( *np <0 || *np >GAnproc) gai_error("invalid number of processors",*np);
      if( *np <GAnproc) gai_error("Invalid number of processors",*np);

      GA_Proc_list = (int*)malloc((size_t)GAnproc * sizeof(int)*2);
      GA_inv_Proc_list = GA_Proc_list + *np;
      if(!GA_Proc_list) gai_error("could not allocate proclist",*np);

      for(i=0;i< (int)*np; i++){
          int p  = (int)list[i];
          if(p<0 || p>= GAnproc) gai_error("invalid list entry",p);
          GA_Proc_list[i] = p; 
          GA_inv_Proc_list[p]=i;
      }

      GA_POP_NAME;
}


void GA_Register_proclist(int *list, int np)
{
      int i;
      GA_PUSH_NAME("ga_register_proclist");
      if( np <0 || np >GAnproc) gai_error("invalid number of processors",np);
      if( np <GAnproc) gai_error("Invalid number of processors",np);

      GA_Proc_list = (int*)malloc((size_t)GAnproc * sizeof(int)*2);
      GA_inv_Proc_list = GA_Proc_list + np;
      if(!GA_Proc_list) gai_error("could not allocate proclist",np);

      for(i=0; i< np; i++){
          int p  = list[i];
          if(p<0 || p>= GAnproc) gai_error("invalid list entry",p);
          GA_Proc_list[i] = p;
          GA_inv_Proc_list[p]=i;
      }
      GA_POP_NAME;
}



/*\ FINAL CLEANUP of shmem when terminating
\*/
void ga_clean_resources()
{
    ARMCI_Cleanup();
}


/*\ CHECK GA HANDLE and if it's wrong TERMINATE
 *  Fortran version
\*/
void FATR ga_check_handle_(Integer *g_a, char *fstring, int slen)
{
    char  buf[FLEN];

    ga_f2cstring(fstring, slen, buf, FLEN);
    gai_check_handle(g_a, buf);
}


/*\ CHECK GA HANDLE and if it's wrong TERMINATE
 *  C version
\*/
void gai_check_handle(Integer *g_a,char * string)
{
  ga_check_handleM(g_a,string);
}


/*\ Get command line arguments if GA in initailzed thru' Fortran interface.
 *  Also, this args are required only for MPI-SPAWN networks
\*/
#ifdef MPI_SPAWN
int    gai_argc;
char **gai_argv;
void gai_get_cmd_args() 
{
    /* GA initialized from C programs should skip this */
    if(_ga_initialize_args) { 
        return; 
    } 
    gai_argv = malloc(sizeof(char*)*F2C_GETARG_ARGV_MAX);
    if (!gai_argv) {
        gai_error("gai_get_cmd_args:malloc gai_argv failed",0);
    }

    Integer argc = F2C_IARGC(); 
    Integer i; 
    Integer maxlen = F2C_GETARG_ARGLEN_MAX; 
    for (i=0; i<argc; i++) { 
        char arg[F2C_GETARG_ARGLEN_MAX]; 
        int len; 
        F2C_GETARG(&i, arg, len); 
        for(len = maxlen-2; len && (arg[len] == ' '); len--); 
        len++; 
        arg[len] = '\0'; /* insert string terminator */ 
        /*printf("%10s, len=%d\n", arg, len);  fflush(stdout);*/ 
        gai_argv[i] = strdup(arg); 
    } 

    gai_argc = argc; 
    _ga_argc = &gai_argc; 
    _ga_argv = &gai_argv; 
}
#endif


/*\ Initialize MA-like addressing:
 *  get addressees for the base arrays for double, complex and int types
\*/
static int ma_address_init=0;
void gai_ma_address_init()
{
#ifdef CHECK_MA_ALGN
Integer  off_dbl, off_int, off_dcpl, off_flt,off_scpl;
#endif
     ma_address_init=1;
     INT_MB = (Integer*)MA_get_mbase(MT_F_INT);
     DBL_MB = (DoublePrecision*)MA_get_mbase(MT_F_DBL);
     DCPL_MB= (DoubleComplex*)MA_get_mbase(MT_F_DCPL);
     SCPL_MB= (SingleComplex*)MA_get_mbase(MT_F_SCPL);
     FLT_MB = (float*)MA_get_mbase(MT_F_REAL);  
#   ifdef CHECK_MA_ALGN
        off_dbl = 0 != ((long)DBL_MB)%sizeof(DoublePrecision);
        off_int = 0 != ((long)INT_MB)%sizeof(Integer);
        off_dcpl= 0 != ((long)DCPL_MB)%sizeof(DoublePrecision);
        off_scpl= 0 != ((long)SCPL_MB)%sizeof(float);
        off_flt = 0 != ((long)FLT_MB)%sizeof(float);  
        if(off_dbl)
           gai_error("GA initialize: MA DBL_MB not alligned", (Integer)DBL_MB);

        if(off_int)
           gai_error("GA initialize: INT_MB not alligned", (Integer)INT_MB);

        if(off_dcpl)
          gai_error("GA initialize: DCPL_MB not alligned", (Integer)DCPL_MB);

        if(off_scpl)
          gai_error("GA initialize: SCPL_MB not alligned", (Integer)SCPL_MB);

        if(off_flt)
           gai_error("GA initialize: FLT_MB not alligned", (Integer)FLT_MB);   

#   endif

    if(DEBUG) printf("%d INT_MB=%p DBL_MB=%p DCPL_MB=%p FLT_MB=%p SCPL_MB=%p\n",
                     (int)GAme, INT_MB,DBL_MB, DCPL_MB, FLT_MB, SCPL_MB);
}



/*\ INITIALIZE GLOBAL ARRAY STRUCTURES
 *
 *  either ga_initialize_ltd or ga_initialize must be the first 
 *         GA routine called (except ga_uses_ma)
\*/
void FATR  ga_initialize_()
{
Integer  i, j,nproc, nnode, zero;
int bytes;

    if(GAinitialized) return;
#ifdef USE_VAMPIR
    vampir_init(NULL,NULL,__FILE__,__LINE__);
    ga_vampir_init(__FILE__,__LINE__);
    vampir_begin(GA_INITIALIZE,__FILE__,__LINE__);
#endif

#ifndef MPI_SPAWN
    ARMCI_Init(); /* initialize GA run-time library */
#else
    gai_get_cmd_args();    
    ARMCI_Init_args(_ga_argc, _ga_argv); /* initialize GA run-time library */
#endif
    
    GA_Default_Proc_Group = -1;
    /* zero in pointers in GA array */
    _ga_main_data_structure
       = (global_array_t *)malloc(sizeof(global_array_t)*MAX_ARRAYS);
    _proc_list_main_data_structure
       = (proc_list_t *)malloc(sizeof(proc_list_t)*MAX_ARRAYS);
    if(!_ga_main_data_structure)
       gai_error("ga_init:malloc ga failed",0);
    if(!_proc_list_main_data_structure)
       gai_error("ga_init:malloc proc_list failed",0);
    GA = _ga_main_data_structure;
    PGRP_LIST = _proc_list_main_data_structure;
    for(i=0;i<MAX_ARRAYS; i++) {
       GA[i].ptr  = (char**)0;
       GA[i].mapc = (C_Integer*)0;
       GA[i].rstrctd_list = (C_Integer*)0;
       GA[i].rank_rstrctd = (C_Integer*)0;
#ifdef ENABLE_CHECKPOINT
       GA[i].record_id = 0;
#endif
       PGRP_LIST[i].map_proc_list = (int*)0;
       PGRP_LIST[i].inv_map_proc_list = (int*)0;
       PGRP_LIST[i].actv = 0;
    }

    bzero(&GAstat,sizeof(GAstat));

    GAnproc = (Integer)armci_msg_nproc();

    /* Allocate arrays used by library */
    ProcListPerm = (int*)malloc(GAnproc*sizeof(int));
#ifdef PERMUTE_PIDS
    ptr_array = (char**)malloc(GAnproc*sizeof(char*));
#endif
    mapALL = (Integer*)malloc((GAnproc+MAXDIM-1)*sizeof(Integer*));

#ifdef PERMUTE_PIDS
    ga_sync_();
    ga_hook_();
    if(GA_Proc_list) GAme = (Integer)GA_Proc_list[ga_msg_nodeid_()];
    else
#endif

    GAme = (Integer)armci_msg_me();
    if(GAme<0 || GAme>GAnproc) 
       gai_error("ga_init:message-passing initialization problem: my ID=",GAme);

    MPme= (Integer)armci_msg_me();

    if(GA_Proc_list)
      fprintf(stderr,"permutation applied %d now becomes %d\n",(int)MPme,(int)GAme);

    GA_proclist = (Integer*)malloc((size_t)GAnproc*sizeof(Integer)); 
    if(!GA_proclist) gai_error("ga_init:malloc failed (proclist)",0);
    gai_init_onesided();

    /* set activity status for all arrays to inactive */
    for(i=0;i<_max_global_array;i++)GA[i].actv=0;
    for(i=0;i<_max_global_array;i++)GA[i].actv_handle=0;

    /* Create proc list for mirrored arrays */
    PGRP_LIST[0].map_proc_list = (int*)malloc(GAnproc*sizeof(int)*2);
    PGRP_LIST[0].inv_map_proc_list = PGRP_LIST[0].map_proc_list + GAnproc;
    for (i=0; i<GAnproc; i++) PGRP_LIST[0].map_proc_list[i] = -1;
    for (i=0; i<GAnproc; i++) PGRP_LIST[0].inv_map_proc_list[i] = -1;
    nnode = ga_cluster_nodeid_();
    nproc = ga_cluster_nprocs_((Integer*)&nnode);
    zero = 0;
    j = ga_cluster_procid_((Integer*)&nnode, (Integer*)&zero);
    PGRP_LIST[0].parent = -1;
    PGRP_LIST[0].actv = 1;
    PGRP_LIST[0].map_nproc = nproc;
    PGRP_LIST[0].mirrored = 1;
    for (i=0; i<nproc; i++) {
       PGRP_LIST[0].map_proc_list[i+j] = i;
       PGRP_LIST[0].inv_map_proc_list[i] = i+j;
    }

    /* assure that GA will not alocate more shared memory than specified */
    if(ARMCI_Uses_shm())
       if(GA_memory_limited) ARMCI_Set_shm_limit(GA_total_memory);

    /* Allocate memory for update flags and signal*/
    bytes = 2*MAXDIM*sizeof(int);
    GA_Update_Flags = (int**)malloc(GAnproc*sizeof(void*));
    if (!GA_Update_Flags)
      gai_error("ga_init: Failed to initialize GA_Update_Flags",(int)GAme);
    if (ARMCI_Malloc((void**)GA_Update_Flags, (armci_size_t) bytes))
      gai_error("ga_init:Failed to initialize memory for update flags",GAme);
    if(GA_Update_Flags[GAme]==NULL)gai_error("ga_init:ARMCIMalloc failed",GAme);

    bytes = sizeof(int);
    GA_Update_Signal = ARMCI_Malloc_local((armci_size_t) bytes);

    /* Zero update flags */
    for (i=0; i<2*MAXDIM; i++) GA_Update_Flags[GAme][i] = 0;

    /* set MA error function */
    MA_set_error_callback(ARMCI_Error);

    GAinitialized = 1;

#ifdef ENABLE_PROFILE 
    ga_profile_init();
#endif
#ifdef ENABLE_CHECKPOINT
    {
    Integer tmplist[1000];
    Integer tmpcount;
    tmpcount = GAnproc-ga_spare_procs;
    for(i=0;i<tmpcount;i++)
            tmplist[i]=i;
    ga_group_is_for_ft=1;
    GA_Default_Proc_Group = ga_pgroup_create_(tmplist,&tmpcount);
    ga_group_is_for_ft=0;
    if(GAme>=tmpcount)
      ga_irecover(0);
    printf("\n%d:here done with initialize\n",GAme);
                 
    }
#endif
    
#ifdef USE_VAMPIR
    vampir_end(GA_INITIALIZE,__FILE__,__LINE__);
#endif

}
#if ENABLE_CHECKPOINT
void set_ga_group_is_for_ft(int val)
{
    ga_group_is_for_ft = val;
}
#endif

/*\ IS MA USED FOR ALLOCATION OF GA MEMORY ?
\*/ 
logical FATR ga_uses_ma_()
{
#ifdef AVOID_MA_STORAGE
   return FALSE;
#else
   if(!GAinitialized) return FALSE;
   
   if(ARMCI_Uses_shm()) return FALSE;
   else return TRUE;
#endif
}


/*\ IS MEMORY LIMIT SET ?
\*/
logical FATR ga_memory_limited_()
{
   if(GA_memory_limited) return TRUE;
   else                  return FALSE;
}



/*\ RETURNS AMOUNT OF MEMORY on each processor IN ACTIVE GLOBAL ARRAYS 
\*/
Integer  FATR ga_inquire_memory_()
{
Integer i, sum=0;
    for(i=0; i<_max_global_array; i++) 
        if(GA[i].actv) sum += (Integer)GA[i].size; 
    return(sum);
}


/*\ RETURNS AMOUNT OF GA MEMORY AVAILABLE on calling processor 
\*/
Integer FATR ga_memory_avail_()
{
   if(!ga_uses_ma_()) return(GA_total_memory);
   else{
      Integer ma_limit = MA_inquire_avail(MT_F_BYTE);

      if ( GA_memory_limited ) return( GA_MIN(GA_total_memory, ma_limit) );
      else return( ma_limit );
   }
}



/*\ (re)set limit on GA memory usage
\*/
void FATR ga_set_memory_limit_(Integer *mem_limit)
{
     if(GA_memory_limited){

         /* if we had the limit set we need to adjust the amount available */
         if (*mem_limit>=0)
             /* adjust the current value by diff between old and new limit */
             GA_total_memory += (*mem_limit - GA_memory_limit);     
         else{

             /* negative values reset limit to "unlimited" */
             GA_memory_limited =  0;     
             GA_total_memory= -1;     
         }

     }else{
         
          GA_total_memory = GA_memory_limit  = *mem_limit;
          if(*mem_limit >= 0) GA_memory_limited = 1;
     }
}


/*\ INITIALIZE GLOBAL ARRAY STRUCTURES and SET LIMIT ON GA MEMORY USAGE
 *  
 *  the byte limit is per processor (even for shared memory)
 *  either ga_initialize_ltd or ga_initialize must be the first 
 *         GA routine called (except ga_uses_ma)
 *  ga_initialize_ltd is another version of ga_initialize 
 *         without memory control
 *  mem_limit < 0 means "memory unlimited"
\*/
void FATR  ga_initialize_ltd_(Integer *mem_limit)
{
#ifdef USE_VAMPIR
  vampir_init(NULL,NULL,__FILE__,__LINE__);
  ga_vampir_init(__FILE__,__LINE__);
  vampir_begin(GA_INITIALIZE_LTD,__FILE__,__LINE__);
#endif
  GA_total_memory =GA_memory_limit  = *mem_limit; 
  if(*mem_limit >= 0) GA_memory_limited = 1; 
  ga_initialize_();
#ifdef USE_VAMPIR
  vampir_end(GA_INITIALIZE_LTD,__FILE__,__LINE__);
#endif
}


  

#define gam_checktype(_type)\
       if(_type != C_DBL  && _type != C_INT &&  \
          _type != C_DCPL && _type != C_SCPL && _type != C_FLOAT && \
          _type != C_LONG &&_type != C_LONGLONG)\
         gai_error("ttype not yet supported ",  _type)

#define gam_checkdim(ndim, dims)\
{\
int _d;\
    if(ndim<1||ndim>MAXDIM) gai_error("unsupported number of dimensions",ndim);\
  __CRAYX1_PRAGMA("_CRI novector");                                         \
    for(_d=0; _d<ndim; _d++)\
         if(dims[_d]<1)gai_error("wrong dimension specified",dims[_d]);\
}

/*\ utility function to tell whether or not an array is mirrored
\*/
logical FATR ga_is_mirrored_(Integer *g_a)
{
  Integer ret = FALSE;
  Integer handle = GA_OFFSET + *g_a;
  Integer p_handle = (Integer)GA[handle].p_handle;
  if (p_handle >= 0) {
     if (PGRP_LIST[p_handle].mirrored) ret = TRUE;
  }
  return ret;
}


/*\ UTILITY FUNCTION TO LOCATE THE BOUNDING INDICES OF A CONTIGUOUS CHUNK OF
 *  SHARED MEMORY FOR A MIRRORED ARRAY
\*/
void ngai_get_first_last_indices( Integer *g_a)  /* array handle (input) */
{

  Integer  lo[MAXDIM], hi[MAXDIM];
  Integer  nelems, nnodes, inode, nproc;
  Integer  ifirst, ilast, nfirst, nlast, icnt, np;
  Integer  i, j, itmp, ndim, map_offset[MAXDIM];
  /* Integer  icheck; */
  Integer  index[MAXDIM], subscript[MAXDIM];
  Integer  handle = GA_OFFSET + *g_a;
  Integer  type, size=0, id, grp_id;
  int Save_default_group;
  char     *fptr, *lptr;

  /* find total number of elements */
  ndim = GA[handle].ndim;
  nelems = 1;
  for (i=0; i<ndim; i++) nelems *= GA[handle].dims[i];

  /* If array is mirrored, evaluate first and last indices */
  if (ga_is_mirrored_(g_a)) {
    /* If default group is not world group, change default group to world group
       temporarily */
    Save_default_group = GA_Default_Proc_Group;
    GA_Default_Proc_Group = -1;
    nnodes = ga_cluster_nnodes_();
    inode = ga_cluster_nodeid_();
    nproc = ga_cluster_nprocs_(&inode);
    grp_id = GA[handle].p_handle;
    ifirst = (Integer)((double)(inode*nelems)/((double)nnodes));
    if (inode != nnodes-1) {
      ilast = (Integer)((double)((inode+1)*nelems)/((double)nnodes))-1;
    } else {
      ilast = nelems-1;
    }
    /* ifirst and ilast correspond to offsets in shared memory. Find the
       actual indices of the data elements corresponding to these offsets.
       The map_offset array provides a convenient mechanism for extracting
       the first indices on each processor along each coordinate dimension
       from the mapc array. */
    for (i = 0; i<ndim; i++) {
      map_offset[i] = 0;
      for (j = 0; j<i; j++) {
        map_offset[i] += GA[handle].nblock[j];
      }
    }
    icnt = 0;
    nfirst = -1;
    nlast = -1;
    for (i = 0; i<nproc; i++) {
      /* find block indices corresponding to proc i */
      nga_proc_topology_(g_a, &i, index);
      nelems = 1;
      for (j = 0; j<ndim; j++) {
        if (index[j] < GA[handle].nblock[j]-1) {
          
          itmp = ((Integer)GA[handle].mapc[map_offset[j]+index[j]+1]
               - (Integer)GA[handle].mapc[map_offset[j]+index[j]]);
          nelems *= itmp;
        } else {
          itmp = ((Integer)GA[handle].dims[j]
               - (Integer)GA[handle].mapc[map_offset[j]+index[j]] + 1);
          nelems *= itmp;
        }
      }
      icnt += nelems;
      if (icnt-1 >= ifirst && nfirst < 0) {
        nfirst = i;
      }
      if (ilast <= icnt-1 && nfirst >= 0 && nlast < 0) {
        nlast = i;
      }
    }
    /* Adjust indices corresponding to start and end of block of
       shared memory so that it can be decomposed into large
       rectangular blocks of the global array. Start by
       adusting the lower index */
    icnt = 0;
    for (i = 0; i<nfirst; i++) {
      nga_distribution_(g_a, &i, lo, hi);
      nelems = 1;
      for (j = 0; j<ndim; j++) {
        if (hi[j] >= lo[j]) {
          nelems *= (hi[j] - lo[j] + 1);
        } else {
          nelems = 0;
        }
      }
      icnt += nelems;
    }
    /* calculate offset in local block of memory */
    ifirst = ifirst - icnt;
    /* find dimensions of data on block nfirst */
    np = nfirst;
    nga_distribution_(g_a, &np, lo, hi);
    nelems = 1;
    for (i=0; i<ndim-1; i++) {
      nelems *= (hi[i] - lo[i] + 1);
    }
    if (ifirst%nelems == 0) {
      ifirst = ifirst/nelems;
    } else {
      ifirst = (ifirst-ifirst%nelems)/nelems;
      ifirst++;
    }
    if (ifirst > GA[handle].dims[ndim-1]-1) ifirst=GA[handle].dims[ndim-1]-1;
    /* adjust value of ifirst */
    nga_proc_topology_(g_a, &nfirst, index);
    subscript[ndim-1] = ifirst;
    for (i=0; i<ndim-1; i++) {
      subscript[i] = 0;
    }
    /* Finally, evaluate absolute indices of first data point */
    for (i=0; i<ndim; i++) {
      GA[handle].first[i] = GA[handle].mapc[map_offset[i]+index[i]]
                          + (C_Integer)subscript[i];
    }
    /* adjust upper bound. If nlast = nfirst, just use old value of icnt */
    if (nlast > nfirst) {
      icnt = 0;
      for (i = 0; i<nlast; i++) {
        nga_distribution_(g_a, &i, lo, hi);
        nelems = 1;
        for (j = 0; j<ndim; j++) {
          if (hi[j] >= lo[j]) {
            nelems *= (hi[j] - lo[j] + 1);
          } else {
            nelems = 0;
          }
        }
        icnt += nelems;
      }
    }
    ilast = ilast - icnt;
    /* find dimensions of data on block nlast */
    np = nlast;
    nga_distribution_(g_a, &np, lo, hi);
    nelems = 1;
    for (i=0; i<ndim-1; i++) {
      nelems *= (hi[i] - lo[i] + 1);
    }
    ilast = (ilast-ilast%nelems)/nelems;
    /* adjust value of ilast */
    subscript[ndim-1] = ilast;
    for (i=0; i<ndim-1; i++) {
      subscript[i] = (hi[i] - lo[i]);
    }
    nga_proc_topology_(g_a, &nlast, index);
    /*
    icheck = 1;
    for (i=1; i<ndim; i++) {
      if (index[i] < GA[handle].nblock[i]-1) {
        itmp = (Integer)GA[handle].mapc[map_offset[i]+index[i]+1]
             - (Integer)GA[handle].mapc[map_offset[i]+index[i]];
      } else {
        itmp = (Integer)GA[handle].dims[i]
             - (Integer)GA[handle].mapc[map_offset[i]+index[i]] + 1;
      }
      if (subscript[i] < itmp-1) icheck = 0;
      subscript[i] = itmp-1;
    }
    if (!icheck) {
      subscript[0]--;
    } */
    /* Finally, evaluate absolute indices of last data point */
    for (i=0; i<ndim; i++) {
      GA[handle].last[i] = GA[handle].mapc[map_offset[i]+index[i]]
                          + (C_Integer)subscript[i];
      if (GA[handle].last[i] > GA[handle].dims[i]) {
        GA[handle].last[i] = GA[handle].dims[i];
      }
    }
    /* find length of shared memory segment owned by this node. Adjust
     * length, if necessary, to account for gaps in memory between
     * processors */
    type = GA[handle].type;
    switch(type) {
      case C_FLOAT: size = sizeof(float); break;
      case C_DBL: size = sizeof(double); break;
      case C_LONG: size = sizeof(long); break;
      case C_LONGLONG: size = sizeof(long long); break;
      case C_INT: size = sizeof(int); break;
      case C_SCPL: size = 2*sizeof(float); break;
      case C_DCPL: size = 2*sizeof(double); break;
      default: gai_error("type not supported",type);
    }
    for (i=0; i<ndim; i++) index[i] = (Integer)GA[handle].first[i];
    i = nga_locate_(g_a, index, &id);
    gam_Loc_ptr(id, handle, (Integer)GA[handle].first, &fptr);

    for (i=0; i<ndim; i++) index[i] = (Integer)GA[handle].last[i];
    i = nga_locate_(g_a, index, &id);
    gam_Loc_ptr(id, handle, (Integer)GA[handle].last, &lptr);

    GA[handle].shm_length = (C_Long)(lptr - fptr + size);
    GA_Default_Proc_Group = Save_default_group;
  } else {
    for (i=0; i<ndim; i++) {
      GA[handle].first[i] = 0;
      GA[handle].last[i] = -1;
      GA[handle].shm_length = -1;
    }
  }
}

/*\ print subscript of ndim dimensional array with two strings before and after
\*/
void gai_print_subscript(char *pre,int ndim, Integer subscript[], char* post)
{
        int i;

        printf("%s [",pre);
        for(i=0;i<ndim;i++){
                printf("%ld",(long)subscript[i]);
                if(i==ndim-1)printf("] %s",post);
                else printf(",");
        }
}

void gai_init_struct(int handle)
{
     if(!GA[handle].ptr){
        int len = (int)GAnproc;
        GA[handle].ptr = (char**)malloc(len*sizeof(char**));
     }
     if(!GA[handle].mapc){
        int len = (int)MAPLEN;
        GA[handle].mapc = (C_Integer*)malloc(len*sizeof(C_Integer*));
        GA[handle].mapc[0] = -1;
     }
     if(!GA[handle].ptr)gai_error("malloc failed: ptr:",0);
     if(!GA[handle].mapc)gai_error("malloc failed: mapc:",0);
     GA[handle].ndim = -1;
}

/*\ SIMPLE FUNCTION TO SET DEFAULT PROCESSOR GROUP
  \*/
void FATR ga_pgroup_set_default_(Integer *grp)
{
    int local_sync_begin,local_sync_end;
 
    local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
    _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous sync masking*/
 
    /* force a hang if default group is not being set correctly */
#if 0
    if (local_sync_begin || local_sync_end) ga_pgroup_sync_(grp);
#endif
    GA_Default_Proc_Group = (int)(*grp);

#ifdef MPI
    {
       ARMCI_Group parent_grp;
       if(GA_Default_Proc_Group > 0)
          parent_grp = PGRP_LIST[GA_Default_Proc_Group].group;
       else
          ARMCI_Group_get_world(&parent_grp);  
       ARMCI_Group_set_default(&parent_grp);
    }
#endif
}
 
Integer FATR ga_pgroup_create_(Integer *list, Integer *count)
{
    Integer pgrp_handle, i, j, nprocs, itmp;
    Integer parent;
    int tmp_count;
    Integer *tmp_list;
    int *tmp2_list;
#ifdef MPI
    ARMCI_Group *tmpgrp;
#endif
 
    GA_PUSH_NAME("ga_pgroup_create_");

    /* Allocate temporary arrays */
    tmp_list = (Integer*)malloc(GAnproc*sizeof(Integer));
    tmp2_list = (int*)malloc(GAnproc*sizeof(int));

    /*** Get next free process group handle ***/
    pgrp_handle =-1; i=0;
    do{
       if(!PGRP_LIST[i].actv) pgrp_handle=i;
       i++;
    }while(i<_max_global_array && pgrp_handle==-1);
    if( pgrp_handle == -1)
       gai_error(" Too many process groups ", (Integer)_max_global_array);
 
    /* Check list for validity (no duplicates and no out of range entries) */
    nprocs = GAnproc;
    for (i=0; i<*count; i++) {
       if (list[i] <0 || list[i] >= nprocs)
	  gai_error(" invalid element in list ", list[i]);
       for (j=i+1; j<*count; j++) {
	  if (list[i] == list[j])
	     gai_error(" Duplicate elements in list ", list[i]);
       }
    }
 
    /* Allocate memory for arrays containg processor maps and initialize
       values */
    PGRP_LIST[pgrp_handle].map_proc_list
       = (int*)malloc(GAnproc*sizeof(int)*2);
    PGRP_LIST[pgrp_handle].inv_map_proc_list
       = PGRP_LIST[pgrp_handle].map_proc_list + GAnproc;
    for (i=0; i<GAnproc; i++)
       PGRP_LIST[pgrp_handle].map_proc_list[i] = -1;
    for (i=0; i<GAnproc; i++)
       PGRP_LIST[pgrp_handle].inv_map_proc_list[i] = -1;
    
    for (i=0; i<*count; i++) {
       tmp2_list[i] = (int)list[i];
    }
    
    /* use a simple sort routine to reorder list into assending order */
    for (j=1; j<*count; j++) {
       itmp = tmp2_list[j];
       i = j-1;
       while(i>=0  && tmp2_list[i] > itmp) {
          tmp2_list[i+1] = tmp2_list[i];
          i--;
       }
       tmp2_list[i+1] = itmp;
    }
    
    /* Remap elements in list to absolute processor indices (if necessary)*/
    if (GA_Default_Proc_Group != -1) {
       parent = GA_Default_Proc_Group;
       for (i=0; i<*count; i++) {
          tmp_list[i] = (int)PGRP_LIST[parent].inv_map_proc_list[tmp2_list[i]];
       }
    } else {
       for (i=0; i<*count; i++) {
          tmp_list[i] = (int)tmp2_list[i];
       }
    }
    
    tmp_count = (int)(*count);
    /* Create proc list maps */
    for (i=0; i<*count; i++) {
       j = tmp_list[i];
       PGRP_LIST[pgrp_handle].map_proc_list[j] = i;
       PGRP_LIST[pgrp_handle].inv_map_proc_list[i] = j;
    }
    PGRP_LIST[pgrp_handle].actv = 1;
    PGRP_LIST[pgrp_handle].parent = GA_Default_Proc_Group;
    PGRP_LIST[pgrp_handle].mirrored = 0;
    PGRP_LIST[pgrp_handle].map_nproc = tmp_count;
#ifdef MPI
    tmpgrp = &PGRP_LIST[pgrp_handle].group;
#if ENABLE_CHECKPOINT
    if(ga_group_is_for_ft)
       tmpgrp = ARMCI_Get_ft_group();
    else
#endif
       ARMCI_Group_create(tmp_count, tmp2_list, &PGRP_LIST[pgrp_handle].group);
#endif

    /* Clean up temporary arrays */
    free(tmp_list);
    free(tmp2_list);

    GA_POP_NAME;
#ifdef MPI
    return pgrp_handle;
#else
    return ga_pgroup_get_default_();
#endif
}

/*\ FREE UP PROCESSOR GROUP HANDLE FOR REUSE
\*/
logical FATR ga_pgroup_destroy_(Integer *grp)
{
  logical ret = TRUE;
  Integer grp_id = *grp;

   GA_PUSH_NAME("ga_pgroup_destroy_");
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous sync masking*/

#ifdef MPI
       ARMCI_Group_free(&PGRP_LIST[grp_id].group);
#endif
  
  if (PGRP_LIST[grp_id].actv == 0) {
    ret = FALSE;
  }
  PGRP_LIST[grp_id].actv = 0;

  /* Deallocate memory for lists */
  free(PGRP_LIST[grp_id].map_proc_list);
  GA_POP_NAME;
  return ret;
}


/*\ SIMPLE FUNCTIONS TO RECOVER STANDARD PROCESSOR LISTS
\*/
Integer FATR ga_pgroup_get_default_()
{
  return GA_Default_Proc_Group;
}

Integer FATR ga_pgroup_get_mirror_()
{
  return 0;
}

Integer FATR ga_pgroup_get_world_()
{
  return -1;
}

Integer FATR ga_pgroup_split_(Integer *grp, Integer *grp_num)
{
  Integer nprocs, me, default_grp;
  Integer ratio, start, end, grp_size;
  Integer i, icnt;
  Integer *nodes;
  Integer grp_id, ret=-1;

  GA_PUSH_NAME("ga_pgroup_split_");
  /* Allocate temporary array */
  nodes = (Integer*)malloc(GAnproc*sizeof(Integer));

  if(*grp_num<0) gai_error("Invalid argument (number of groups < 0)",*grp_num);
  if(*grp_num==0) return *grp;
  
  default_grp = ga_pgroup_get_default_();
  ga_pgroup_set_default_(grp);
  
#if 0 /* This is wrong. Should split only default group and not world group */
  world_grp = ga_pgroup_get_world_();
  ga_pgroup_set_default_(&world_grp);
#endif
  nprocs = ga_nnodes_();
  me = ga_nodeid_();
  /* Figure out how big groups are */
  grp_size = nprocs/(*grp_num);
  if (nprocs > grp_size*(*grp_num)) grp_size++;
  /* Figure out what procs are in my group */
  ratio = me/grp_size;
  start = ratio*grp_size;
  end = (ratio+1)*grp_size-1;
  end = GA_MIN(end,nprocs-1);
  if (end<start)
    gai_error("Invalid proc range encountered",0);
  icnt = 0;
  for (i= 0; i<nprocs; i++) {
    if (icnt%grp_size == 0 && i>0) {
      grp_id = ga_pgroup_create_(nodes, &grp_size);
      if (i == end + 1) {
        ret = grp_id;
      }
      icnt = 0;
    }
    nodes[icnt] = i;
    icnt++;
  }
  grp_id = ga_pgroup_create_(nodes, &icnt);
  if (end == nprocs-1) {
    ret = grp_id;
  }
  ga_pgroup_set_default_(&default_grp);
  if(ret==-1) gai_error("ga_pgroup_split failed",ret);
  /* Free temporary array */
  GA_POP_NAME;
  free(nodes);
  return ret;
}

Integer FATR ga_pgroup_split_irreg_(Integer *grp, Integer *mycolor, Integer *key)
{
  Integer nprocs, me, default_grp, grp_id;
  Integer i, icnt=0;
  Integer *nodes, *color_arr;
  
  GA_PUSH_NAME("ga_pgroup_split_irreg_");
  
  /* Allocate temporary arrays */
  nodes = (Integer*)malloc(GAnproc*sizeof(Integer));
  color_arr = (Integer*)malloc(GAnproc*sizeof(Integer));

  if(*mycolor<0) gai_error("Invalid argument (color < 0)",*mycolor);

  default_grp = ga_pgroup_get_default_();
  ga_pgroup_set_default_(grp);
  nprocs = ga_nnodes_();
  me = ga_nodeid_();

  /* Figure out what procs are in my group */
  for(i=0; i<nprocs; i++) color_arr[i] = 0;
  color_arr[me] = *mycolor;
  gai_igop(GA_TYPE_GOP, color_arr, nprocs, "+");

  for (icnt=0, i=0; i<nprocs; i++) {
     if(color_arr[i] == *mycolor) {
        nodes[icnt] = i;
        icnt++;
     }
  }

  grp_id = ga_pgroup_create_(nodes, &icnt);

  ga_pgroup_set_default_(&default_grp);

  /* Free temporary arrays */
  free(nodes);
  free(color_arr);

  GA_POP_NAME;

  return grp_id;
}

#ifdef MPI
ARMCI_Group* ga_get_armci_group_(int grp_id)
{
  return &PGRP_LIST[grp_id].group;
}
#endif

/*\ Return a new global array handle
\*/
Integer ga_create_handle_()
{
  Integer ga_handle, i, g_a;
  /*** Get next free global array handle ***/
  GA_PUSH_NAME("ga_create_handle");
  ga_handle =-1; i=0;
  do{
      if(!GA[i].actv_handle) ga_handle=i;
      i++;
  }while(i<_max_global_array && ga_handle==-1);
  if( ga_handle == -1)
      gai_error(" too many arrays ", (Integer)_max_global_array);
  g_a = (Integer)ga_handle - GA_OFFSET;

  /*** fill in Global Info Record for g_a ***/
  gai_init_struct(ga_handle);
  GA[ga_handle].p_handle = GA_Init_Proc_Group;
  GA[ga_handle].ndim = -1;
  GA[ga_handle].name[0] = '\0';
  GA[ga_handle].mapc[0] = -1;
  GA[ga_handle].irreg = 0;
  GA[ga_handle].ghosts = 0;
  GA[ga_handle].corner_flag = -1;
  GA[ga_handle].cache = NULL;
  GA[ga_handle].block_flag = 0;
  GA[ga_handle].block_sl_flag = 0;
  GA[ga_handle].block_total = -1;
  GA[ga_handle].rstrctd_list = NULL;
  GA[ga_handle].rank_rstrctd = NULL;
  GA[ga_handle].num_rstrctd = 0;   /* This is also used as a flag for   */
                                   /* restricted arrays. If it is zero, */
                                   /* then array is not restricted.     */
  GA[ga_handle].actv_handle = 1;
  GA[ga_handle].has_data = 1;
  GA_POP_NAME;
  return g_a;
}

/*\ Set the dimensions and data type on a new global array
\*/
void FATR ga_set_data_(Integer *g_a, Integer *ndim, Integer *dims, Integer *type)
{
  Integer i;
  Integer ga_handle = *g_a + GA_OFFSET;
  GA_PUSH_NAME("ga_set_data");
  if (GA[ga_handle].actv == 1)
    gai_error("Cannot set data on array that has been allocated",0);
  gam_checkdim(*ndim, dims);
  gam_checktype(ga_type_f2c(*type));

  GA[ga_handle].type = ga_type_f2c((int)(*type));
  GA[ga_handle].elemsize = GAsizeofM(GA[ga_handle].type);

  for (i=0; i<*ndim; i++) {
    GA[ga_handle].dims[i] = (C_Integer)dims[i];
    GA[ga_handle].chunk[i] = 0;
    GA[ga_handle].width[i] = 0;
  }
  GA[ga_handle].ndim = (int)(*ndim);
  GA_POP_NAME;
}

/*\ Set the chunk array on a new global array
\*/
void FATR ga_set_chunk_(Integer *g_a, Integer *chunk)
{
  Integer i;
  Integer ga_handle = *g_a + GA_OFFSET;
  GA_PUSH_NAME("ga_set_chunk");
  if (GA[ga_handle].actv == 1)
    gai_error("Cannot set chunk on array that has been allocated",0);
  if (GA[ga_handle].ndim < 1)
    gai_error("Dimensions must be set before chunk array is specified",0);
  if (chunk) {
    for (i=0; i<GA[ga_handle].ndim; i++) {
      GA[ga_handle].chunk[i] = (C_Integer)chunk[i];
    }
  }
  GA_POP_NAME;
}

/*\ Set the array name on a new global array
\*/
void FATR gai_set_array_name(Integer g_a, char *array_name)
{
  Integer ga_handle = g_a + GA_OFFSET;
  GA_PUSH_NAME("ga_set_array_name");
  if (GA[ga_handle].actv == 1)
    gai_error("Cannot set array name on array that has been allocated",0);
  strcpy(GA[ga_handle].name, array_name);
  GA_POP_NAME;
}

/*\ Set the array name on a new global array
 *  Fortran version
\*/
void FATR ga_set_array_name_(Integer *g_a, char* array_name, int slen)
{
  char buf[FNAM];
  ga_f2cstring(array_name ,slen, buf, FNAM);

  gai_set_array_name(*g_a, buf);
}

/*\ Set the processor configuration on a new global array
\*/
void FATR ga_set_pgroup_(Integer *g_a, Integer *p_handle)
{
  Integer ga_handle = *g_a + GA_OFFSET;
  GA_PUSH_NAME("ga_set_pgroup");
  if (GA[ga_handle].actv == 1)
    gai_error("Cannot set processor configuration on array that has been allocated",0);
  if (*p_handle == GA_World_Proc_Group || PGRP_LIST[*p_handle].actv == 1) {
    GA[ga_handle].p_handle = (int) (*p_handle);
  } else {
    gai_error("Processor group does not exist",0);
  }
  GA_POP_NAME;
}

Integer FATR ga_get_pgroup_(Integer *g_a)
{
    Integer ga_handle = *g_a + GA_OFFSET;
    return (Integer)GA[ga_handle].p_handle;
}
 
Integer FATR ga_get_pgroup_size_(Integer *grp_id)
{
    int p_handle = (int)(*grp_id);
    if (p_handle > 0) {
       return (Integer)PGRP_LIST[p_handle].map_nproc;
    } else {
       return GAnproc;
    }
}

/*\ Add ghost cells to a new global array
\*/
void FATR ga_set_ghosts_(Integer *g_a, Integer *width)
{
  Integer i;
  Integer ga_handle = *g_a + GA_OFFSET;
  GA_PUSH_NAME("ga_set_ghosts");
  if (GA[ga_handle].actv == 1)
    gai_error("Cannot set ghost widths on array that has been allocated",0);
  if (GA[ga_handle].ndim < 1)
    gai_error("Dimensions must be set before array widths are specified",0);
  for (i=0; i<GA[ga_handle].ndim; i++) {
    if ((C_Integer)width[i] > GA[ga_handle].dims[i])
      gai_error("Boundary width must be <= corresponding dimension",i);
    if ((C_Integer)width[i] < 0)
      gai_error("Boundary width must be >= 0",i);
  }
  for (i=0; i<GA[ga_handle].ndim; i++) {
    GA[ga_handle].width[i] = (C_Integer)width[i];
    if (width[i] > 0) GA[ga_handle].ghosts = 1;
  }
  GA_POP_NAME;
}

/*\ Set irregular distribution in a new global array
\*/
void FATR ga_set_irreg_distr_(Integer *g_a, Integer *mapc, Integer *nblock)
{
  Integer i, j, ichk, maplen;
  Integer ga_handle = *g_a + GA_OFFSET;
  GA_PUSH_NAME("ga_set_irreg_distr");
  if (GA[ga_handle].actv == 1)
    gai_error("Cannot set irregular data distribution on array that has been allocated",0);
  if (GA[ga_handle].ndim < 1)
    gai_error("Dimensions must be set before irregular distribution is specified",0);
  for (i=0; i<GA[ga_handle].ndim; i++)
    if ((C_Integer)nblock[i] > GA[ga_handle].dims[i])
      gai_error("number of blocks must be <= corresponding dimension",i);
  /* Check to see that mapc array is sensible */
  maplen = 0;
  for (i=0; i<GA[ga_handle].ndim; i++) {
    ichk = mapc[maplen];
    if (ichk < 1 || ichk > GA[ga_handle].dims[i])
      gai_error("Mapc entry outside array dimension limits",ichk);
    maplen++;
    for (j=1; j<nblock[i]; j++) {
      if (mapc[maplen] < ichk) {
        gai_error("Mapc entries are not properly monotonic",ichk);
      }
      ichk = mapc[maplen];
      if (ichk < 1 || ichk > GA[ga_handle].dims[i])
        gai_error("Mapc entry outside array dimension limits",ichk);
      maplen++;
    }
  }

  maplen = 0;
  for (i=0; i<GA[ga_handle].ndim; i++) {
    maplen += nblock[i];
    GA[ga_handle].nblock[i] = (C_Integer)nblock[i];
  }
  for (i=0; i<maplen; i++) {
    GA[ga_handle].mapc[i] = (C_Integer)mapc[i];
  }
  GA[ga_handle].mapc[maplen] = -1;
  GA[ga_handle].irreg = 1;
  GA_POP_NAME;
}

/*\ Overide the irregular data distribution flag on a new global array
\*/
void FATR ga_set_irreg_flag_(Integer *g_a, logical *flag)
{
  Integer ga_handle = *g_a + GA_OFFSET;
  GA_PUSH_NAME("ga_set_irreg");
  GA[ga_handle].irreg = (int)(*flag);
  GA_POP_NAME;
}

/*\ Get dimension on a new global array
\*/
Integer FATR ga_get_dimension_(Integer *g_a)
{
  Integer ga_handle = *g_a + GA_OFFSET;
  return (Integer)GA[ga_handle].ndim;
} 

/*\ Use block-cyclic data distribution for array
\*/
void FATR ga_set_block_cyclic_(Integer *g_a, Integer *dims)
{
  Integer i, jsize;
  Integer ga_handle = *g_a + GA_OFFSET;
  GA_PUSH_NAME("ga_set_block_cyclic");
  if (GA[ga_handle].actv == 1)
    gai_error("Cannot set block-cyclic data distribution on array that has been allocated",0);
  if (!(GA[ga_handle].ndim > 0))
    gai_error("Cannot set block-cyclic data distribution if array size not set",0);
  if (GA[ga_handle].block_flag == 1)
    gai_error("Cannot reset block-cyclic data distribution on array that has been set",0);
  GA[ga_handle].block_flag = 1;
  GA[ga_handle].block_sl_flag = 0;
  /* evaluate number of blocks in each dimension */
  for (i=0; i<GA[ga_handle].ndim; i++) {
    if (dims[i] < 1)
      gai_error("Block dimensions must all be greater than zero",0);
    GA[ga_handle].block_dims[i] = dims[i];
    jsize = GA[ga_handle].dims[i]/dims[i];
    if (GA[ga_handle].dims[i]%dims[i] != 0) jsize++;
    GA[ga_handle].num_blocks[i] = jsize;
  }
  jsize = 1;
  for (i=0; i<GA[ga_handle].ndim; i++) {
    jsize *= GA[ga_handle].num_blocks[i];
  }
  GA[ga_handle].block_total = jsize;
  GA_POP_NAME;
}

/*\ Use block-cyclic data distribution with ScaLAPACK proc grid for array
\*/
void FATR ga_set_block_cyclic_proc_grid_(Integer *g_a, Integer *dims, Integer *proc_grid)
{
  Integer i, jsize, tot;
  Integer ga_handle = *g_a + GA_OFFSET;
  GA_PUSH_NAME("ga_set_block_cyclic_proc_grid");
  if (GA[ga_handle].actv == 1)
    gai_error("Cannot set block-cyclic data distribution on array that has been allocated",0);
  if (!(GA[ga_handle].ndim > 0))
    gai_error("Cannot set block-cyclic data distribution if array size not set",0);
  if (GA[ga_handle].block_flag == 1)
    gai_error("Cannot reset block-cyclic data distribution on array that has been set",0);
  GA[ga_handle].block_flag = 1;
  GA[ga_handle].block_sl_flag = 1;
  /* Check to make sure processor grid is compatible with total number of processors */
  tot = 1;
  for (i=0; i<GA[ga_handle].ndim; i++) {
    if (proc_grid[i] < 1)
      gai_error("Processor grid dimensions must all be greater than zero",0);
    GA[ga_handle].nblock[i] = proc_grid[i];
    tot *= proc_grid[i];
  }
  if (tot != GAnproc)
    gai_error("Number of processors in processor grid must equal available processors",0);
  /* evaluate number of blocks in each dimension */
  for (i=0; i<GA[ga_handle].ndim; i++) {
    if (dims[i] < 1)
      gai_error("Block dimensions must all be greater than zero",0);
    GA[ga_handle].block_dims[i] = dims[i];
    jsize = GA[ga_handle].dims[i]/dims[i];
    if (GA[ga_handle].dims[i]%dims[i] != 0) jsize++;
    GA[ga_handle].num_blocks[i] = jsize;
  }
  jsize = 1;
  for (i=0; i<GA[ga_handle].ndim; i++) {
    jsize *= GA[ga_handle].num_blocks[i];
  }
  GA[ga_handle].block_total = jsize;
  GA_POP_NAME;
}

/*\  Restrict processors that actually contain data in the global array
\*/
void FATR ga_set_restricted_(Integer *g_a, Integer *list, Integer *size)
{
  Integer i, ig, id=0, me, p_handle, has_data, nproc;
  Integer ga_handle = *g_a + GA_OFFSET;
  GA_PUSH_NAME("ga_set_restricted");
  GA[ga_handle].num_rstrctd = *size;
  GA[ga_handle].rstrctd_list = (Integer*)malloc((*size)*sizeof(Integer));
  GA[ga_handle].rank_rstrctd = (Integer*)malloc((GAnproc)*sizeof(Integer));
  p_handle = GA[ga_handle].p_handle;
  if (p_handle == -2) p_handle = ga_pgroup_get_default_();
  if (p_handle > 0) {
    me = PGRP_LIST[p_handle].map_proc_list[GAme];
    nproc = PGRP_LIST[p_handle].map_nproc;
  } else {
    me = GAme;
    nproc = GAnproc;
  }
  has_data = 0;

  for (i=0; i<GAnproc; i++) {
    GA[ga_handle].rank_rstrctd[i] = -1;
  }

  for (i=0; i<*size; i++) {
    GA[ga_handle].rstrctd_list[i] = list[i];
    /* check if processor is in list */
    if (me == list[i]) {
      has_data = 1;
      id = i;
    }
    /* check if processor is in group */
    if (list[i] < 0 || list[i] >= nproc)
      gai_error("Invalid processor in list",list[i]);
    ig = list[i];
    GA[ga_handle].rank_rstrctd[ig] = i;
  }
  GA[ga_handle].has_data = has_data;
  GA[ga_handle].rstrctd_id = id;
  GA_POP_NAME;
}

/*\  Restrict processors that actually contain data in the global array
 *   by specifying a range of processors
\*/
void FATR ga_set_restricted_range_(Integer *g_a, Integer *lo_proc, Integer *hi_proc)
{
  Integer i, ig, id=0, me, p_handle, has_data, icnt, nproc, size;
  Integer ga_handle = *g_a + GA_OFFSET;
  size = *hi_proc - *lo_proc + 1;
  GA_PUSH_NAME("ga_set_restricted_range");
  GA[ga_handle].num_rstrctd = size;
  GA[ga_handle].rstrctd_list = (Integer*)malloc((size)*sizeof(Integer));
  GA[ga_handle].rank_rstrctd = (Integer*)malloc((GAnproc)*sizeof(Integer));
  p_handle = GA[ga_handle].p_handle;
  if (p_handle == -2) p_handle = ga_pgroup_get_default_();
  if (p_handle > 0) {
    me = PGRP_LIST[p_handle].map_proc_list[GAme];
    nproc = PGRP_LIST[p_handle].map_nproc;
  } else {
    me = GAme;
    nproc = GAnproc;
  }
  has_data = 0;

  for (i=0; i<GAnproc; i++) {
    GA[ga_handle].rank_rstrctd[i] = -1;
  }

  icnt = 0;
  for (i=*lo_proc; i<=*hi_proc; i++) {
    GA[ga_handle].rstrctd_list[icnt] = i;
    /* check if processor is in list */
    if (me == i) {
      has_data = 1;
      id = icnt;
    }
    /* check if processor is in group */
    if (i < 0 || i >= nproc)
      gai_error("Invalid processor in list",i);
    ig = i;
    GA[ga_handle].rank_rstrctd[ig] = icnt;
    icnt++;
  }
  GA[ga_handle].has_data = has_data;
  GA[ga_handle].rstrctd_id = id;
  GA_POP_NAME;
}

/*\  Allocate memory and complete setup of global array
\*/
logical FATR ga_allocate_( Integer *g_a)
{

  Integer hi[MAXDIM];
  Integer ga_handle = *g_a + GA_OFFSET;
  Integer d, width[MAXDIM], ndim;
  Integer mem_size, nelem;
  Integer i, status, maplen=0, p_handle;
  Integer dims[MAXDIM], chunk[MAXDIM];
  Integer pe[MAXDIM], *pmap[MAXDIM], *map;
  Integer blk[MAXDIM];
  Integer grp_me=GAme, grp_nproc=GAnproc;
  Integer block_size = 0;
#ifdef USE_VAMPIR
  vampir_begin(GA_ALLOCATE,__FILE__,__LINE__);
#endif

  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous sync masking*/
  if (GA[ga_handle].ndim == -1)
    gai_error("Insufficient data to create global array",0);

  p_handle = (Integer)GA[ga_handle].p_handle;
  if (p_handle == (Integer)GA_Init_Proc_Group) {
    GA[ga_handle].p_handle = GA_Default_Proc_Group;
    p_handle = GA_Default_Proc_Group;
  }
  ga_pgroup_sync_(&p_handle);
  GA_PUSH_NAME("ga_allocate");

  if (p_handle > 0) {
     grp_nproc  = PGRP_LIST[p_handle].map_nproc;
     grp_me = PGRP_LIST[p_handle].map_proc_list[GAme];
  }
  
  if(!GAinitialized) gai_error("GA not initialized ", 0);
  if(!ma_address_init) gai_ma_address_init();

  ndim = GA[ga_handle].ndim;
  for (i=0; i<ndim; i++) width[i] = (C_Integer)GA[ga_handle].width[i];

  /* The data distribution has not been specified by the user. Create
     default distribution */
  if (GA[ga_handle].mapc[0] == -1 && GA[ga_handle].block_flag == 0) {
#define OLD_DISTRIBUTION 1
#if OLD_DISTRIBUTION
    extern void ddb_h2(Integer ndims, Integer dims[], Integer npes,
                    double threshold, Integer bias, Integer blk[],
                    Integer pedims[]);
#else
    extern void ddb(Integer ndims, Integer dims[], Integer npes,
                    Integer blk[], Integer pedims[]);
#endif

    for (d=0; d<ndim; d++) {
      dims[d] = (Integer)GA[ga_handle].dims[d];
      chunk[d] = (Integer)GA[ga_handle].chunk[d];
    }
    if(chunk && chunk[0]!=0) /* for either NULL or chunk[0]=0 compute all */
      for(d=0; d< ndim; d++) blk[d]=(Integer)GA_MIN(chunk[d],dims[d]);
    else
      for(d=0; d< ndim; d++) blk[d]=-1;

    /* eliminate dimensions =1 from ddb analysis */
    for(d=0; d<ndim; d++)if(dims[d]==1)blk[d]=1;
 
    if (GAme==0 && DEBUG )
      for (d=0;d<ndim;d++) fprintf(stderr,"b[%ld]=%ld\n",(long)d,(long)blk[d]);
    ga_pgroup_sync_(&p_handle);

    /* ddb(ndim, dims, GAnproc, blk, pe);*/
    if(p_handle == 0) /* for mirrored arrays */
#if OLD_DISTRIBUTION
       ddb_h2(ndim, dims, PGRP_LIST[p_handle].map_nproc,0.0,(Integer)0, blk, pe);
#else
       ddb(ndim, dims, PGRP_LIST[p_handle].map_nproc, blk, pe);
#endif
    else
       if (GA[ga_handle].num_rstrctd == 0) {
         /* Data is normally distributed on processors */
#if OLD_DISTRIBUTION
         ddb_h2(ndim, dims, grp_nproc,0.0,(Integer)0, blk, pe);
#else
         ddb(ndim, dims, grp_nproc, blk, pe);
#endif
       } else {
         /* Data is only distributed on subset of processors */
#if OLD_DISTRIBUTION
         ddb_h2(ndim, dims, GA[ga_handle].num_rstrctd, 0.0, (Integer)0, blk, pe);
#else
         ddb(ndim, dims, GA[ga_handle].num_rstrctd, blk, pe);
#endif
       }

    for(d=0, map=mapALL; d< ndim; d++){
      Integer nblock;
      Integer pcut; /* # procs that get full blk[] elements; the rest gets less*/
      int p;

      pmap[d] = map;

      /* RJH ... don't leave some nodes without data if possible
       but respect the users block size */
      
      if (chunk && chunk[d] > 1) {
        Integer ddim = ((dims[d]-1)/GA_MIN(chunk[d],dims[d]) + 1);
        pcut = (ddim -(blk[d]-1)*pe[d]) ;
      }
      else {
        pcut = (dims[d]-(blk[d]-1)*pe[d]) ;
      }

      for (nblock=i=p=0; (p<pe[d]) && (i<dims[d]); p++, nblock++) {
        Integer b = blk[d];
        if (p >= pcut)
          b = b-1;
        map[nblock] = i+1;
        if (chunk && chunk[d]>1) b *= GA_MIN(chunk[d],dims[d]);
        i += b;
      }

      pe[d] = GA_MIN(pe[d],nblock);
      map +=  pe[d]; 
    }
    if(GAme==0&& DEBUG){
      gai_print_subscript("pe ",(int)ndim, pe,"\n");
      gai_print_subscript("blocks ",(int)ndim, blk,"\n");
      printf("decomposition map\n");
      for(d=0; d< ndim; d++){
        printf("dim=%ld: ",(long)d); 
        for (i=0;i<pe[d];i++)printf("%ld ",(long)pmap[d][i]);
        printf("\n"); 
      }
      fflush(stdout);
    }
    maplen = 0;
    for( i = 0; i< ndim; i++){
      GA[ga_handle].nblock[i] = pe[i];
      maplen += pe[i];
    }
    for(i = 0; i< maplen; i++) {
      GA[ga_handle].mapc[i] = (C_Integer)mapALL[i];
    }
    GA[ga_handle].mapc[maplen] = -1;
  } else if (GA[ga_handle].block_flag == 1) {
    if (GA[ga_handle].block_sl_flag == 0) {
      /* Regular block-cyclic data distribution has been specified. Figure
         out how much memory is needed by each processor to store blocks */
      Integer nblocks = GA[ga_handle].block_total;
      Integer tsize, j;
      Integer lo[MAXDIM];
      block_size = 0;
      for (i=GAme; i<nblocks; i +=GAnproc) {
        ga_ownsM(ga_handle,i,lo,hi);
        tsize = 1;
        for (j=0; j<ndim; j++) {
          tsize *= (hi[j] - lo[j] + 1);
        }
        block_size += tsize;
      }
    } else {
      /* ScaLAPACK block-cyclic data distribution has been specified. Figure
         out how much memory is needed by each processor to store blocks */
      Integer j, jtot, skip, imin, imax;
      Integer index[MAXDIM];
      gam_find_proc_indices(ga_handle,GAme,index);
      block_size = 1;
      for (i=0; i<ndim; i++) {
        skip = GA[ga_handle].nblock[i];
        jtot = 0;
        for (j=index[i]; j<GA[ga_handle].num_blocks[i]; j += skip) {
          imin = j*GA[ga_handle].block_dims[i] + 1;
          imax = (j+1)*GA[ga_handle].block_dims[i];
          if (imax > GA[ga_handle].dims[i]) imax = GA[ga_handle].dims[i];
          jtot += (imax-imin+1);
        }
        block_size *= jtot;
      }
    }
  }

  GAstat.numcre ++;

  GA[ga_handle].actv = 1;
  /* If only one node is being used and array is mirrored,
   * set proc list to world group */
  if (ga_cluster_nnodes_() == 1 && GA[ga_handle].p_handle == 0) {
    GA[ga_handle].p_handle = ga_pgroup_get_world_();
  }

  /* Set remaining paramters and determine memory size if regular data
   * distribution is being used */
  if (GA[ga_handle].block_flag == 0) {
    /* set corner flag, if it has not already been set and set up message
       passing data */
    if (GA[ga_handle].corner_flag == -1) {
       i = 1;
    } else {
       i = GA[ga_handle].corner_flag;
    }
    ga_set_ghost_corner_flag_(g_a, &i);
 
    for( i = 0; i< ndim; i++){
       GA[ga_handle].scale[i] = (double)GA[ga_handle].nblock[i]
         / (double)GA[ga_handle].dims[i];
    }
    /*** determine which portion of the array I am supposed to hold ***/
    if (p_handle == 0) { /* for mirrored arrays */
       Integer me_local = (Integer)PGRP_LIST[p_handle].map_proc_list[GAme];
       nga_distribution_(g_a, &me_local, GA[ga_handle].lo, hi);
    } else {
       nga_distribution_(g_a, &grp_me, GA[ga_handle].lo, hi);
    }
    if (GA[ga_handle].num_rstrctd == 0 || GA[ga_handle].has_data == 1) {
      for( i = 0, nelem=1; i< ndim; i++){
        GA[ga_handle].chunk[i] = ((C_Integer)hi[i]-GA[ga_handle].lo[i]+1);
        nelem *= (hi[i]-(Integer)GA[ga_handle].lo[i]+1+2*width[i]);
      }
    } else {
      nelem = 0;
    }
    mem_size = nelem * GA[ga_handle].elemsize;
  } else {
    mem_size = block_size * GA[ga_handle].elemsize;
  }
  GA[ga_handle].id = INVALID_MA_HANDLE;
  GA[ga_handle].size = (C_Long)mem_size;
  /* if requested, enforce limits on memory consumption */
  if(GA_memory_limited) GA_total_memory -= mem_size;
  /* check if everybody has enough memory left */
  if(GA_memory_limited){
     status = (GA_total_memory >= 0) ? 1 : 0;
     if (p_handle > 0) {
        gai_pgroup_igop(p_handle,GA_TYPE_GSM, &status, 1, "*");
     } else {
        gai_igop(GA_TYPE_GSM, &status, 1, "*");
     }
  }else status = 1;

  if (status) {
    status = !gai_getmem(GA[ga_handle].name, GA[ga_handle].ptr,mem_size,
                             GA[ga_handle].type, &GA[ga_handle].id, p_handle);
  } else {
     GA[ga_handle].ptr[grp_me]=NULL;
  }

  if (GA[ga_handle].block_flag == 0) {
    /* Finish setting up information for ghost cell updates */
    if (GA[ga_handle].ghosts == 1) {
      if (!ga_set_ghost_info_(g_a))
        gai_error("Could not allocate update information for ghost cells",0);
    }
    /* If array is mirrored, evaluate first and last indices */
    /* ngai_get_first_last_indices(g_a); */
  }

  ga_pgroup_sync_(&p_handle);
  if (status) {
    GAstat.curmem += (long)GA[ga_handle].size;
    GAstat.maxmem  = (long)GA_MAX(GAstat.maxmem, GAstat.curmem);
    status = TRUE;
  } else {
    if(GA_memory_limited) GA_total_memory += mem_size;
    ga_destroy_(g_a);
    status = FALSE;
  }
  GA_POP_NAME;
#ifdef USE_VAMPIR
  vampir_end(GA_ALLOCATE,__FILE__,__LINE__);
#endif
  return status;
}

/*\ CREATE AN N-DIMENSIONAL GLOBAL ARRAY WITH GHOST CELLS
 *   -- IRREGULAR DISTRIBUTION
 *  This is the master routine. All other creation routines are derived
 *  from this one.
\*/
logical ngai_create_ghosts_irreg_config(
        Integer type,     /* MA type */ 
        Integer ndim,     /* number of dimensions */
        Integer dims[],   /* array of dimensions */
        Integer width[],  /* width of boundary cells for each dimension */
        char *array_name, /* array name */
        Integer map[],    /* decomposition map array */ 
        Integer nblock[], /* number of blocks for each dimension in map */
        Integer p_handle, /* processor list handle */
        Integer *g_a)     /* array handle (output) */
{
  logical status;
#ifdef USE_VAMPIR
  vampir_begin(NGA_CREATE_GHOSTS_IRREG_CONFIG,__FILE__,__LINE__);
#endif

  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous sync masking*/
  ga_sync_();
  GA_PUSH_NAME("ngai_create_ghosts_irreg_config");

  *g_a = ga_create_handle_();
  ga_set_data_(g_a,&ndim,dims,&type);
  ga_set_ghosts_(g_a,width);
  gai_set_array_name(*g_a,array_name);
  ga_set_irreg_distr_(g_a,map,nblock);
  ga_set_pgroup_(g_a,&p_handle);
  status = ga_allocate_(g_a);

  GA_POP_NAME;
#ifdef USE_VAMPIR
  vampir_end(NGA_CREATE_IRREG_CONFIG,__FILE__,__LINE__);
#endif
  return status;
}

logical ngai_create_ghosts_irreg(
        Integer type,     /* MA type */ 
        Integer ndim,     /* number of dimensions */
        Integer dims[],   /* array of dimensions */
        Integer width[],  /* width of boundary cells for each dimension */
        char *array_name, /* array name */
        Integer map[],    /* decomposition map array */ 
        Integer nblock[], /* number of blocks for each dimension in map */
        Integer *g_a)     /* array handle (output) */
{
   Integer p_handle = ga_pgroup_get_default_();
   return ngai_create_ghosts_irreg_config(type, ndim, dims, width,
                array_name, map, nblock, p_handle, g_a);
}


/*\ CREATE AN N-DIMENSIONAL GLOBAL ARRAY
 *  Allow machine to choose location of array boundaries on individual
 *  processors.
\*/
logical ngai_create_config(Integer type,
                         Integer ndim,
                         Integer dims[],
                         char* array_name,
                         Integer *chunk,
                         Integer p_handle,
                         Integer *g_a)
{
  logical status;
  GA_PUSH_NAME("ngai_create_config");
  *g_a = ga_create_handle_();
  ga_set_data_(g_a,&ndim,dims,&type);
  gai_set_array_name(*g_a,array_name);
  ga_set_chunk_(g_a,chunk);
  ga_set_pgroup_(g_a,&p_handle);
  status = ga_allocate_(g_a);
  GA_POP_NAME;
  return status;
}


logical ngai_create(Integer type,
                   Integer ndim,
                   Integer dims[],
                   char* array_name,
                   Integer *chunk,
                   Integer *g_a)
{
  Integer p_handle = ga_pgroup_get_default_();
  return ngai_create_config(type, ndim, dims, array_name, chunk, p_handle, g_a);
}


/*\ CREATE AN N-DIMENSIONAL GLOBAL ARRAY WITH GHOST CELLS
 *  Allow machine to choose location of array boundaries on individual
 *  processors.
\*/
logical ngai_create_ghosts_config(Integer type,
                   Integer ndim,
                   Integer dims[],
                   Integer width[],
                   char* array_name,
                   Integer chunk[],
                   Integer p_handle,
                   Integer *g_a)
{
  logical status;
  GA_PUSH_NAME("nga_create_ghosts_config");
  *g_a = ga_create_handle_();
  ga_set_data_(g_a,&ndim,dims,&type);
  ga_set_ghosts_(g_a,width);
  gai_set_array_name(*g_a,array_name);
  ga_set_chunk_(g_a,chunk);
  ga_set_pgroup_(g_a,&p_handle);
  status = ga_allocate_(g_a);
  GA_POP_NAME;
  return status;
}

logical ngai_create_ghosts(Integer type,
                   Integer ndim,
                   Integer dims[],
                   Integer width[],
                   char* array_name,
                   Integer chunk[],
                   Integer *g_a)
{
  Integer p_handle = ga_pgroup_get_default_();
  return ngai_create_ghosts_config(type, ndim, dims, width, array_name,
                  chunk, p_handle, g_a);
}

/*\ CREATE A 2-DIMENSIONAL GLOBAL ARRAY
 *  Allow machine to choose location of array boundaries on individual
 *  processors.
\*/
logical gai_create(type, dim1, dim2, array_name, chunk1, chunk2, g_a)
     Integer *type, *dim1, *dim2, *chunk1, *chunk2, *g_a;
     char *array_name;
{
Integer ndim=2, dims[2], chunk[2];
logical status;
#ifdef  OLD_DEFAULT_BLK
#define BLK_THR 1
#else
#define BLK_THR 0
#endif
 
    dims[0]=*dim1;
    dims[1]=*dim2;

    /*block size of 1 is troublesome, old ga treated it as "use default" */
    /* for backward compatibility we use old convention */
    chunk[0] = (*chunk1 ==BLK_THR)? -1: *chunk1;
    chunk[1] = (*chunk2 ==BLK_THR)? -1: *chunk2;

    status = ngai_create(*type, ndim,  dims, array_name, chunk, g_a);

    return status;
}


/*\ CREATE A GLOBAL ARRAY -- IRREGULAR DISTRIBUTION -- PROCESSOR CONFIGURATION
 *  User can specify location of array boundaries on individual
 *  processors and the processor configuration.
\*/
logical ngai_create_irreg_config(
        Integer type,     /* MA type */ 
        Integer ndim,     /* number of dimensions */
        Integer dims[],   /* array of dimensions */
        char *array_name, /* array name */
        Integer map[],    /* decomposition map array */ 
        Integer nblock[], /* number of blocks for each dimension in map */
        Integer p_handle, /* processor list hande */
        Integer *g_a)     /* array handle (output) */
{
Integer  d,width[MAXDIM];
logical status;

      for (d=0; d<ndim; d++) width[d] = 0;
      status = ngai_create_ghosts_irreg_config(type, ndim, dims, width,
          array_name, map, nblock, p_handle, g_a);

      return status;
}


/*\ CREATE A GLOBAL ARRAY -- IRREGULAR DISTRIBUTION
 *  User can specify location of array boundaries on individual
 *  processors.
\*/
logical ngai_create_irreg(
        Integer type,     /* MA type */ 
        Integer ndim,     /* number of dimensions */
        Integer dims[],   /* array of dimensions */
        char *array_name, /* array name */
        Integer map[],    /* decomposition map array */ 
        Integer nblock[], /* number of blocks for each dimension in map */
        Integer *g_a)     /* array handle (output) */
{

Integer  d,width[MAXDIM];
logical status;

      for (d=0; d<ndim; d++) width[d] = 0;
      status = ngai_create_ghosts_irreg(type, ndim, dims, width,
          array_name, map, nblock, g_a);

      return status;
}


/*\ CREATE A 2-DIMENSIONAL GLOBAL ARRAY -- IRREGULAR DISTRIBUTION
 *  User can specify location of array boundaries on individual
 *  processors.
 *  array_name    - a unique character string [input]
 *  type          - MA type [input]
 *  dim1/2        - array(dim1,dim2) as in FORTRAN [input]
 *  nblock1       - no. of blocks first dimension is divided into [input]
 *  nblock2       - no. of blocks second dimension is divided into [input]
 *  map1          - no. ilo in each block [input]
 *  map2          - no. jlo in each block [input]
 *  g_a           - Integer handle for future references [output]
\*/
logical gai_create_irreg(Integer *type, Integer *dim1, Integer *dim2,
        char *array_name, Integer *map1, Integer *nblock1, Integer *map2,
        Integer *nblock2, Integer *g_a)
{
Integer  ndim, dims[MAXDIM], width[MAXDIM], nblock[MAXDIM], *map;
Integer  i,ctype;
logical status;
 
      ctype = ga_type_f2c((int)(*type));  
      if(ctype != C_DBL  && ctype != C_INT &&  
         ctype != C_DCPL && ctype != C_SCPL && ctype != C_FLOAT  &&
         ctype != C_LONG && ctype != C_LONGLONG)
         gai_error("gai_create_irreg: type not yet supported ",  *type);
      else if( *dim1 <= 0 )
         gai_error("gai_create_irreg: array dimension1 invalid ",  *dim1);
      else if( *dim2 <= 0)
         gai_error("gai_create_irreg: array dimension2 invalid ",  *dim2);
      else if(*nblock1 <= 0)
         gai_error("gai_create_irreg: nblock1 <=0  ",  *nblock1);
      else if(*nblock2 <= 0)
         gai_error("gai_create_irreg: nblock2 <=0  ",  *nblock2);
      else if(*nblock1 * *nblock2 > GAnproc)
         gai_error("gai_create_irreg: too many blocks ",*nblock1 * *nblock2);

      if(GAme==0&& DEBUG){
        fprintf(stderr," array:%d map1:\n", (int)*g_a);
        for (i=0;i<*nblock1;i++)fprintf(stderr," %ld |",(long)map1[i]);
        fprintf(stderr," \n array:%d map2:\n",(int) *g_a);
        for (i=0;i<*nblock2;i++)fprintf(stderr," %ld |",(long)map2[i]);
        fprintf(stderr,"\n\n");
      }
      ndim = 2;
      dims[0] = *dim1;
      dims[1] = *dim2;
      width[0] = 0;
      width[1] = 0;
      nblock[0] = *nblock1;
      nblock[1] = *nblock2;
      map = mapALL;
      for(i=0;i< *nblock1; i++) map[i] = map1[i];
      for(i=0;i< *nblock2; i++) map[i+ *nblock1] = map2[i];
      status = ngai_create_ghosts_irreg(*type, ndim, dims, width,
          array_name, mapALL, nblock, g_a);
 
      return status;

}

/*\ CREATE AN N-DIMENSIONAL GLOBAL ARRAY WITH GHOST CELLS
 * -- IRREGULAR DISTRIBUTION -- PROCESSOR CONFIGURATION
 *  Fortran version
\*/
#if F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS
logical FATR nga_create_ghosts_irreg_config_(Integer *type,
    Integer *ndim, Integer *dims, Integer width[], char* array_name,
    Integer map[], Integer block[], Integer *p_handle, Integer *g_a,
    int slen)
#else
logical FATR nga_create_ghosts_irreg_config_(Integer *type,
    Integer *ndim, Integer *dims, Integer width[], char* array_name,
    int slen, Integer map[], Integer block[],
    Integer *p_handle, Integer *g_a)
#endif
{
char buf[FNAM];
Integer st; 
      ga_f2cstring(array_name ,slen, buf, FNAM);
  
      _ga_irreg_flag = 1; /* set this flag=1, to indicate array is irregular */
      st = ngai_create_ghosts_irreg_config(*type, *ndim,  dims, width, buf, 
					  map, block, *p_handle, g_a);
      _ga_irreg_flag = 0; /* unset it, after creating array */ 
      return st;
}

/*\ CREATE AN N-DIMENSIONAL GLOBAL ARRAY WITH GHOST CELLS
 * -- IRREGULAR DISTRIBUTION
 *  Fortran version
\*/
#if F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS
logical FATR nga_create_ghosts_irreg_(Integer *type, Integer *ndim,
    Integer *dims, Integer width[], char* array_name, Integer map[],
    Integer block[], Integer *g_a, int slen)
#else
logical FATR nga_create_ghosts_irreg_(Integer *type, Integer *ndim,
    Integer *dims, Integer width[], char* array_name, int slen,
    Integer map[], Integer block[], Integer *g_a)
#endif
{
char buf[FNAM];
Integer st;
      ga_f2cstring(array_name ,slen, buf, FNAM);
      
      _ga_irreg_flag = 1; /* set this flag=1, to indicate array is irregular */
      st = ngai_create_ghosts_irreg(*type, *ndim,  dims, width, buf, map,
				   block, g_a);
      _ga_irreg_flag = 0; /* unset it, after creating array */
      return st;
}

/*\ CREATE A 2-DIMENSIONAL GLOBAL ARRAY
 *  Fortran version
\*/
#if F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS
logical FATR ga_create_(type, dim1, dim2, array_name, chunk1, chunk2, g_a, slen)
#else
logical FATR ga_create_(type, dim1, dim2, array_name, slen, chunk1, chunk2, g_a)
#endif
     Integer *type, *dim1, *dim2, *chunk1, *chunk2, *g_a;
     int slen;
     char* array_name;
{
char buf[FNAM];
      ga_f2cstring(array_name ,slen, buf, FNAM);

  return(gai_create(type, dim1, dim2, buf, chunk1, chunk2, g_a));
}

/*\ CREATE AN N-DIMENSIONAL GLOBAL ARRAY -- PROCESSOR CONFIGURATION
 *  Fortran version
\*/
#if F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS
logical FATR nga_create_config_(Integer *type, Integer *ndim,
                   Integer *dims, char* array_name, Integer *chunk,
                   Integer *p_handle, Integer *g_a, int slen)
#else
logical FATR nga_create_config_(Integer *type, Integer *ndim,
                   Integer *dims, char* array_name, int slen,
                   Integer *chunk, Integer *p_handle, Integer *g_a)
#endif
{
char buf[FNAM];
      ga_f2cstring(array_name ,slen, buf, FNAM);

  return (ngai_create_config(*type, *ndim,  dims, buf, chunk, *p_handle, g_a));
}

/*\ CREATE AN N-DIMENSIONAL GLOBAL ARRAY
 *  Fortran version
\*/
#if F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS
logical FATR nga_create_(Integer *type, Integer *ndim, Integer *dims,
                   char* array_name, Integer *chunk, Integer *g_a, int slen)
#else
logical FATR nga_create_(Integer *type, Integer *ndim, Integer *dims,
                   char* array_name, int slen, Integer *chunk, Integer *g_a)
#endif
{
char buf[FNAM];
      ga_f2cstring(array_name ,slen, buf, FNAM);

  return (ngai_create(*type, *ndim,  dims, buf, chunk, g_a));
}

/*\ CREATE AN N-DIMENSIONAL GLOBAL ARRAY WITH GHOST CELLS -- PROCESSOR
 *  CONFIGURATION
 *  Fortran version
\*/
#if F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS
logical FATR nga_create_ghosts_config_(Integer *type, Integer *ndim,
                   Integer *dims, Integer *width, char* array_name,
                   Integer *chunk, Integer *p_handle, Integer *g_a,
                   int slen)
#else
logical FATR nga_create_ghosts_config_(Integer *type, Integer *ndim,
                   Integer *dims, Integer *width, char* array_name,
                   int slen,
                   Integer *chunk, Integer *p_handle, Integer *g_a)
#endif
{
char buf[FNAM];
      ga_f2cstring(array_name ,slen, buf, FNAM);

  return (ngai_create_ghosts_config(*type, *ndim,  dims, width, buf, chunk,
                   *p_handle, g_a));
}

/*\ CREATE AN N-DIMENSIONAL GLOBAL ARRAY WITH GHOST CELLS
 *  Fortran version
\*/
#if F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS
logical FATR nga_create_ghosts_(Integer *type, Integer *ndim, Integer *dims,
        Integer *width, char* array_name, Integer *chunk, Integer *g_a,
        int slen)
#else
logical FATR nga_create_ghosts_(Integer *type, Integer *ndim, Integer *dims,
        Integer *width, char* array_name, int slen,
        Integer *chunk, Integer *g_a)
#endif
{
char buf[FNAM];
      ga_f2cstring(array_name ,slen, buf, FNAM);

  return (ngai_create_ghosts(*type, *ndim,  dims, width, buf, chunk, g_a));
}

/*\ CREATE A 2-DIMENSIONAL GLOBAL ARRAY -- IRREGULAR DISTRIBUTION
 *  Fortran version
\*/
#if F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS
logical FATR ga_create_irreg_(
        Integer *type, Integer *dim1, Integer *dim2, char *array_name, Integer *map1, Integer *nblock1, Integer *map2, Integer *nblock2, Integer *g_a, int slen)
#else
logical FATR ga_create_irreg_(
        Integer *type, Integer *dim1, Integer *dim2, char *array_name, int slen, Integer *map1, Integer *nblock1, Integer *map2, Integer *nblock2, Integer *g_a)
#endif
{
char buf[FNAM];
Integer st;
      ga_f2cstring(array_name ,slen, buf, FNAM);
      _ga_irreg_flag = 1; /* set this flag=1, to indicate array is irregular*/
      st = gai_create_irreg(type, dim1, dim2, buf, map1, nblock1,
			   map2, nblock2, g_a);
      _ga_irreg_flag = 0; /* unset it, after creating array */ 
      return st;
}

/*\ CREATE AN N-DIMENSIONAL GLOBAL ARRAY -- IRREGULAR DISTRIBUTION
 *  -- PROCESSOR DISTRIBUTION
 *  Fortran version
\*/
#if F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS
logical FATR nga_create_irreg_config_(Integer *type, Integer *ndim,
                 Integer *dims, char* array_name, Integer map[],
                 Integer block[], Integer *p_handle, Integer *g_a,
                 int slen)
#else
logical FATR nga_create_irreg_config_(Integer *type, Integer *ndim,
                 Integer *dims, char* array_name, int slen, Integer map[],
                 Integer block[], Integer *p_handle, Integer *g_a)
#endif
{
char buf[FNAM];
Integer st;
      ga_f2cstring(array_name ,slen, buf, FNAM);

      _ga_irreg_flag = 1; /* set this flag=1, to indicate array is irregular*/
      st = ngai_create_irreg_config(*type, *ndim,  dims, buf, map, block,
				   *p_handle, g_a);
      _ga_irreg_flag = 0; /* unset it, after creating array */ 
      return st;
}

/*\ CREATE AN N-DIMENSIONAL GLOBAL ARRAY -- IRREGULAR DISTRIBUTION
 *  Fortran version
\*/
#if F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS
logical FATR nga_create_irreg_(Integer *type, Integer *ndim, Integer *dims,
                 char* array_name, Integer map[], Integer block[],
                 Integer *g_a, int slen)
#else
logical FATR nga_create_irreg_(Integer *type, Integer *ndim, Integer *dims,
                 char* array_name, int slen,
                 Integer map[], Integer block[], Integer *g_a)
#endif
{
char buf[FNAM];
Integer st;
      ga_f2cstring(array_name ,slen, buf, FNAM);
      
      _ga_irreg_flag = 1; /* set this flag=1, to indicate array is irregular */
      st = ngai_create_irreg(*type, *ndim,  dims, buf, map, block, g_a);
      _ga_irreg_flag = 0; /* unset it, after creating array */
      return st;
}

/*\ get memory alligned w.r.t. MA base
 *  required on Linux as g77 ignores natural data alignment in common blocks
\*/ 
int gai_get_shmem(char **ptr_arr, C_Long bytes, int type, long *adj,
		  int grp_id)
{
int status=0;
#ifndef _CHECK_MA_ALGN
char *base;
long diff, item_size;  
Integer *adjust;
int i, nproc,grp_me=GAme;

    if (grp_id > 0) {
       nproc  = PGRP_LIST[grp_id].map_nproc;
       grp_me = PGRP_LIST[grp_id].map_proc_list[GAme];
    }
    else
       nproc = GAnproc; 
 
    /* need to enforce proper, natural allignment (on size boundary)  */
    switch (ga_type_c2f(type)){
      case MT_F_DBL:   base =  (char *) DBL_MB; break;
      case MT_F_INT:   base =  (char *) INT_MB; break;
      case MT_F_DCPL:  base =  (char *) DCPL_MB; break;
      case MT_F_SCPL:  base =  (char *) SCPL_MB; break;
      case MT_F_REAL:  base =  (char *) FLT_MB; break;  
      default:        base = (char*)0;
    }

    item_size = GAsizeofM(type);
#   ifdef GA_ELEM_PADDING
       bytes += (C_Long)item_size; 
#   endif

#endif

    *adj = 0;
#ifdef PERMUTE_PIDS
    if(GA_Proc_list){
       bzero(ptr_array,nproc*sizeof(char*));
       /* use ARMCI_Malloc_group for groups if proc group is not world group
	  or mirror group */
#  ifdef MPI
       if (grp_id > 0)
	  status = ARMCI_Malloc_group((void**)ptr_array, bytes,
				      &PGRP_LIST[grp_id].group);
       else
#  endif
	  status = ARMCI_Malloc((void**)ptr_array, bytes);
       if(bytes!=0 && ptr_array[grp_me]==NULL) 
	  gai_error("gai_get_shmem: ARMCI Malloc failed", GAme);
       for(i=0;i<nproc;i++)ptr_arr[i] = ptr_array[GA_inv_Proc_list[i]];
    }else
#endif
       
    /* use ARMCI_Malloc_group for groups if proc group is not world group
       or mirror group */
#ifdef MPI
    if (grp_id > 0) {
       status = ARMCI_Malloc_group((void**)ptr_arr, (armci_size_t)bytes,
				   &PGRP_LIST[grp_id].group);
    } else
#endif
      status = ARMCI_Malloc((void**)ptr_arr, (armci_size_t)bytes);

    if(bytes!=0 && ptr_arr[grp_me]==NULL) 
       gai_error("gai_get_shmem: ARMCI Malloc failed", GAme);
    if(status) return status;

#ifndef _CHECK_MA_ALGN

    /* adjust all addresses if they are not alligned on corresponding nodes*/

    /* we need storage for GAnproc*sizeof(Integer) */
    /* JAD -- fixed bug where _ga_map was reused before gai_getmem was done
     * with it. Now malloc/free needed memory. */
    adjust = (Integer*)malloc(GAnproc*sizeof(Integer));

    diff = (GA_ABS( base - (char *) ptr_arr[grp_me])) % item_size; 
    for(i=0;i<nproc;i++)adjust[i]=0;
    adjust[grp_me] = (diff > 0) ? item_size - diff : 0;
    *adj = adjust[grp_me];

    if (grp_id > 0)
       gai_pgroup_igop(grp_id,GA_TYPE_GSM, adjust, nproc, "+");
    else
       gai_igop(GA_TYPE_GSM, adjust, nproc, "+");
    
    for(i=0;i<nproc;i++){
       ptr_arr[i] = adjust[i] + (char*)ptr_arr[i];
    }
    free(adjust);

#endif
    return status;
}

int gai_uses_shm(int grp_id) 
{
#ifdef MPI
    if(grp_id > 0) return ARMCI_Uses_shm_grp(&PGRP_LIST[grp_id].group);
    else
#endif
      return ARMCI_Uses_shm();
}

int gai_getmem(char* name, char **ptr_arr, C_Long bytes, int type, long *id,
	       int grp_id)
{
#ifdef AVOID_MA_STORAGE
   return gai_get_shmem(ptr_arr, bytes, type, id, grp_id);
#else
Integer handle = INVALID_MA_HANDLE, index;
Integer nproc=GAnproc, grp_me=GAme, item_size = GAsizeofM(type);
C_Long nelem;
char *ptr = (char*)0;

   if (grp_id > 0) {
     nproc  = PGRP_LIST[grp_id].map_nproc;
     grp_me = PGRP_LIST[grp_id].map_proc_list[GAme];
   }
 
   if(gai_uses_shm(grp_id)) return gai_get_shmem(ptr_arr, bytes, type, id, grp_id);
   else{
     nelem = bytes/((C_Long)item_size) + 1;
     if(bytes)
        if(MA_alloc_get(type, nelem, name, &handle, &index)){
                MA_get_pointer(handle, &ptr);}
     *id   = (long)handle;

     /* 
            printf("bytes=%d ptr=%ld index=%d\n",bytes, ptr,index);
            fflush(stdout);
     */

     bzero((char*)ptr_arr,(int)nproc*sizeof(char*));
     ptr_arr[grp_me] = ptr;

#   ifndef _CHECK_MA_ALGN /* align */
     {
        long diff, adjust;  
        diff = ((unsigned long)ptr_arr[grp_me]) % item_size; 
        adjust = (diff > 0) ? item_size - diff : 0;
        ptr_arr[grp_me] = adjust + (char*)ptr_arr[grp_me];
     }
#   endif
     
#   ifdef MPI
     if (grp_id > 0) {
        armci_exchange_address_grp((void**)ptr_arr,(int)nproc,
                                   &PGRP_LIST[grp_id].group);
     } else
#   endif
        armci_exchange_address((void**)ptr_arr,(int)nproc);
     if(bytes && !ptr) return 1; 
     else return 0;
   }
#endif /* AVOID_MA_STORAGE */
}


/*\ externalized version of gai_getmem to facilitate two-step array creation
\*/
void *GA_Getmem(int type, int nelem, int grp_id)
{
char **ptr_arr=(char**)0;
int  rc,i;
long id;
int bytes;
int extra=sizeof(getmem_t)+GAnproc*sizeof(char*);
char *myptr;
Integer status;
     type = ga_type_f2c(type);	
     bytes = nelem *  GAsizeofM(type);
     if(GA_memory_limited){
         GA_total_memory -= bytes+extra;
         status = (GA_total_memory >= 0) ? 1 : 0;
         gai_igop(GA_TYPE_GSM, &status, 1, "*");
         if(!status)GA_total_memory +=bytes+extra;
     }else status = 1;

     ptr_arr=(char**)_ga_map; /* need memory GAnproc*sizeof(char**) */
     rc= gai_getmem("ga_getmem", ptr_arr,(Integer)bytes+extra, type, &id, grp_id);
     if(rc)gai_error("ga_getmem: failed to allocate memory",bytes+extra);

     myptr = ptr_arr[GAme];  

     /* make sure that remote memory addresses point to user memory */
     for(i=0; i<GAnproc; i++)ptr_arr[i] += extra;

#ifndef AVOID_MA_STORAGE
     if(ARMCI_Uses_shm()) 
#endif
        id += extra; /* id is used to store offset */

     /* stuff the type and id info at the beginning */
     ((getmem_t*)myptr)->id = id;
     ((getmem_t*)myptr)->type = type;
     ((getmem_t*)myptr)->size = bytes+extra;

     /* add ptr info */
     memcpy(myptr+sizeof(getmem_t),ptr_arr,(size_t)GAnproc*sizeof(char**));

     return (void*)(myptr+extra);
}


void GA_Freemem(void *ptr)
{
int extra = sizeof(getmem_t)+GAnproc*sizeof(char*); 
getmem_t *info = (getmem_t *)((char*)ptr - extra);
char **ptr_arr = (char**)(info+1);

#ifndef AVOID_MA_STORAGE
    if(ARMCI_Uses_shm()){
#endif
      /* make sure that we free original (before address alignment) pointer */
      ARMCI_Free(ptr_arr[GAme] - info->id);
#ifndef AVOID_MA_STORAGE
    }else{
      if(info->id != INVALID_MA_HANDLE) MA_free_heap(info->id);
    }
#endif

    if(GA_memory_limited) GA_total_memory += info->size;
}

/*\ RETURN COORDINATES OF A GA PATCH ASSOCIATED WITH PROCESSOR proc
\*/
void FATR nga_distribution_(Integer *g_a, Integer *proc, Integer *lo, Integer *
hi)
{
Integer ga_handle, lproc;

   ga_check_handleM(g_a, "nga_distribution");
   ga_handle = (GA_OFFSET + *g_a);

   lproc = *proc;
   if (GA[ga_handle].num_rstrctd > 0) {
     lproc = GA[ga_handle].rank_rstrctd[lproc];
   }
   if (GA[ga_handle].block_flag == 0) {
     ga_ownsM(ga_handle, lproc, lo, hi);
   } else {
     C_Integer index[MAXDIM];
     int ndim = GA[ga_handle].ndim;
     int i;
     gam_find_block_indices(ga_handle,lproc,index);
     for (i=0; i<ndim; i++) {
       lo[i] = index[i]*GA[ga_handle].block_dims[i] + 1;
       hi[i] = (index[i]+1)*GA[ga_handle].block_dims[i];
       if (hi[i] > GA[ga_handle].dims[i]) hi[i] = GA[ga_handle].dims[i];
     }
   }
}

void ngai_distribution(Integer *g_a, Integer *proc, Integer *lo, Integer *hi)
{
    nga_distribution_(g_a, proc, lo, hi);
}

/*\ Check to see if array has ghost cells.
\*/
logical FATR ga_has_ghosts_(Integer* g_a)
{
      int h_a = (int)*g_a + GA_OFFSET;
      return GA[h_a].ghosts;
}

Integer FATR ga_ndim_(Integer *g_a)
{
      ga_check_handleM(g_a,"ga_ndim");       
      return GA[*g_a +GA_OFFSET].ndim;
}
 


/*\ DUPLICATE A GLOBAL ARRAY
 *  -- new array g_b will have properties of g_a
 * array_name    - a character string [input]
 * g_a           - Integer handle for reference array [input]
 * g_b           - Integer handle for new array [output]
\*/
logical gai_duplicate(Integer *g_a, Integer *g_b, char* array_name)
{
  char     **save_ptr;
  C_Long  mem_size, mem_size_proc;
  Integer  i, ga_handle, status;
  C_Integer  *save_mapc;
  int local_sync_begin,local_sync_end;
  Integer grp_id, grp_me=GAme, grp_nproc=GAnproc;

#ifdef USE_VAMPIR
  vampir_begin(GA_DUPLICATE,__FILE__,__LINE__);
#endif

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
  grp_id = ga_get_pgroup_(g_a);
  if(local_sync_begin)ga_pgroup_sync_(&grp_id);

  if (grp_id > 0) {
    grp_nproc  = PGRP_LIST[grp_id].map_nproc;
    grp_me = PGRP_LIST[grp_id].map_proc_list[GAme];
  }

  GAstat.numcre ++; 

  ga_check_handleM(g_a,"gai_duplicate");       

  /* find a free global_array handle for g_b */
  ga_handle =-1; i=0;
  do{
    if(!GA[i].actv_handle) ga_handle=i;
    i++;
  }while(i<_max_global_array && ga_handle==-1);
  if( ga_handle == -1)
    gai_error("gai_duplicate: too many arrays", (Integer)_max_global_array);
  *g_b = (Integer)ga_handle - GA_OFFSET;
   GA[ga_handle].actv_handle = 1;

  gai_init_struct(ga_handle);

  /*** copy content of the data structure ***/
  save_ptr = GA[ga_handle].ptr;
  save_mapc = GA[ga_handle].mapc;
  GA[ga_handle] = GA[GA_OFFSET + *g_a];
  strcpy(GA[ga_handle].name, array_name);
  GA[ga_handle].ptr = save_ptr;
  GA[ga_handle].mapc = save_mapc;
  for(i=0;i<MAPLEN; i++)GA[ga_handle].mapc[i] = GA[GA_OFFSET+ *g_a].mapc[i];

  /*** copy info for restricted arrays, if relevant ***/
  if (GA[GA_OFFSET + *g_a].num_rstrctd > 0) {
    GA[ga_handle].num_rstrctd = GA[GA_OFFSET + *g_a].num_rstrctd;
    ga_set_restricted_(g_a, GA[GA_OFFSET + *g_a].rstrctd_list,
        &GA[GA_OFFSET + *g_a].num_rstrctd);
  }

  /*** Memory Allocation & Initialization of GA Addressing Space ***/
  mem_size = mem_size_proc = GA[ga_handle].size; 
  GA[ga_handle].id = INVALID_MA_HANDLE;
  /* if requested, enforce limits on memory consumption */
  if(GA_memory_limited) GA_total_memory -= mem_size_proc;

  /* check if everybody has enough memory left */
  if(GA_memory_limited){
    status = (GA_total_memory >= 0) ? 1 : 0;
    if (grp_id > 0) {
      gai_pgroup_igop((int)grp_id,GA_TYPE_GSM, &status, 1, "*");
      status = (Integer)status;
    } else {
      gai_igop(GA_TYPE_GSM, &status, 1, "*");
    }
  }else status = 1;

  if(status)
  {
    status = !gai_getmem(array_name, GA[ga_handle].ptr,mem_size,
        (int)GA[ga_handle].type, &GA[ga_handle].id,
        (int)grp_id);
}
  else{
    GA[ga_handle].ptr[grp_me]=NULL;
  }

  if(local_sync_end)ga_pgroup_sync_(&grp_id);

#     ifdef GA_CREATE_INDEF
  /* This code is incorrect. It needs to fixed if INDEF is ever used */
  if(status){
    Integer one = 1; 
    Integer dim1 =(Integer)GA[ga_handle].dims[1], dim2=(Integer)GA[ga_handle].dims[2];
    if(GAme==0)fprintf(stderr,"duplicate:initializing GA array%ld\n",*g_b);
    if(GA[ga_handle].type == C_DBL) {
      double bad = (double) DBL_MAX;
      ga_fill_patch_(g_b, &one, &dim1, &one, &dim2,  &bad);
    } else if (GA[ga_handle].type == C_INT) {
      int bad = (int) INT_MAX;
      ga_fill_patch_(g_b, &one, &dim1, &one, &dim2,  &bad);
    } else if (GA[ga_handle].type == C_LONG) {
      long bad = LONG_MAX;
      ga_fill_patch_(g_b, &one, &dim1, &one, &dim2,  &bad);
    } else if (GA[ga_handle].type == C_LONGLONG) {
      long long bad = LONG_MAX;
      ga_fill_patch_(g_b, &one, &dim1, &one, &dim2,  &bad);
    } else if (GA[ga_handle].type == C_DCPL) { 
      DoubleComplex bad = {DBL_MAX, DBL_MAX};
      ga_fill_patch_(g_b, &one, &dim1, &one, &dim2,  &bad);
    } else if (GA[ga_handle].type == C_SCPL) { 
      SingleComplex bad = {FLT_MAX, FLT_MAX};
      ga_fill_patch_(g_b, &one, &dim1, &one, &dim2,  &bad);
    } else if (GA[ga_handle].type == C_FLOAT) {
      float bad = FLT_MAX;
      ga_fill_patch_(g_b, &one, &dim1, &one, &dim2,  &bad);   
    } else {
      gai_error("gai_duplicate: type not supported ",GA[ga_handle].type);
    }
  }
#     endif

#ifdef USE_VAMPIR
  vampir_end(GA_DUPLICATE,__FILE__,__LINE__);
#endif

  if(status){
    GAstat.curmem += (long)GA[ga_handle].size;
    GAstat.maxmem  = (long)GA_MAX(GAstat.maxmem, GAstat.curmem);
    return(TRUE);
  }else{ 
    if (GA_memory_limited) GA_total_memory += mem_size_proc;
    ga_destroy_(g_b);
    return(FALSE);
  }
}

/*\ DUPLICATE A GLOBAL ARRAY -- memory comes from user
 *  -- new array g_b will have properties of g_a
\*/
int GA_Assemble_duplicate(int g_a, char* array_name, void* ptr)
{
char     **save_ptr;
int      i, ga_handle;
C_Integer      *save_mapc;
int extra = sizeof(getmem_t)+GAnproc*sizeof(char*);
getmem_t *info = (getmem_t *)((char*)ptr - extra);
char **ptr_arr = (char**)(info+1);
int g_b;


      ga_sync_();

      GAstat.numcre ++;

      ga_check_handleM(&g_a,"ga_assemble_duplicate");

      /* find a free global_array handle for g_b */
      ga_handle =-1; i=0;
      do{
        if(!GA[i].actv_handle) ga_handle=i;
        i++;
      }while(i<_max_global_array && ga_handle==-1);
      if( ga_handle == -1)
          gai_error("ga_assemble_duplicate: too many arrays ", 
                                           (Integer)_max_global_array);
      g_b = ga_handle - GA_OFFSET;

      gai_init_struct(ga_handle);
      GA[ga_handle].actv_handle = 1;

      /*** copy content of the data structure ***/
      save_ptr = GA[ga_handle].ptr;
      save_mapc = GA[ga_handle].mapc;
      GA[ga_handle] = GA[GA_OFFSET + g_a];
      strcpy(GA[ga_handle].name, array_name);
      GA[ga_handle].ptr = save_ptr;
      GA[ga_handle].mapc = save_mapc;
      for(i=0;i<MAPLEN; i++)GA[ga_handle].mapc[i] = GA[GA_OFFSET+ g_a].mapc[i];

      /* get ptrs and datatype from user memory */
      gam_checktype(ga_type_f2c(info->type));
      GA[ga_handle].type = ga_type_f2c(info->type);
      GA[ga_handle].size = (C_Long)info->size;
      GA[ga_handle].id = info->id;
      memcpy(GA[ga_handle].ptr,ptr_arr,(size_t)GAnproc*sizeof(char**));

      GAstat.curmem += (long)GA[ga_handle].size;
      GAstat.maxmem  = (long)GA_MAX(GAstat.maxmem, GAstat.curmem);

      ga_sync_();

      return(g_b);
}


/*\ DUPLICATE A GLOBAL ARRAY
 *  Fortran version
\*/
logical FATR ga_duplicate_(
        Integer *g_a, Integer *g_b, char *array_name, int slen)
{
    char buf[FNAM];

    ga_f2cstring(array_name ,slen, buf, FNAM);
    return(gai_duplicate(g_a, g_b, buf));
}



/*\ DESTROY A GLOBAL ARRAY
\*/
logical FATR ga_destroy_(Integer *g_a)
{
Integer ga_handle = GA_OFFSET + *g_a, grp_id, grp_me=GAme;
int local_sync_begin;

#ifdef USE_VAMPIR
    vampir_begin(GA_DESTROY,__FILE__,__LINE__);
#endif

    local_sync_begin = _ga_sync_begin; 
    _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
    grp_id = (Integer)GA[ga_handle].p_handle;
    if(local_sync_begin)ga_pgroup_sync_(&grp_id);

    if (grp_id > 0) grp_me = PGRP_LIST[grp_id].map_proc_list[GAme];
    else grp_me=GAme;
    
    GAstat.numdes ++; /*regardless of array status we count this call */
    /* fails if handle is out of range or array not active */
    if(ga_handle < 0 || ga_handle >= _max_global_array){
#ifdef USE_VAMPIR
       vampir_end(GA_DESTROY,__FILE__,__LINE__);
#endif
       return FALSE;
    }
    if(GA[ga_handle].actv==0){
#ifdef USE_VAMPIR
       vampir_end(GA_DESTROY,__FILE__,__LINE__);
#endif
       return FALSE;
    }
    if (GA[ga_handle].cache)
      free(GA[ga_handle].cache);
    GA[ga_handle].cache = NULL;
    GA[ga_handle].actv = 0;     
    GA[ga_handle].actv_handle = 0;     

    if (GA[ga_handle].num_rstrctd > 0) {
      GA[ga_handle].num_rstrctd = 0;
      if (GA[ga_handle].rstrctd_list)
        free(GA[ga_handle].rstrctd_list);
      GA[ga_handle].rstrctd_list = NULL;

      if (GA[ga_handle].rank_rstrctd)
        free(GA[ga_handle].rank_rstrctd);
      GA[ga_handle].rank_rstrctd = NULL;
    }

    if(GA[ga_handle].ptr[grp_me]==NULL){
#ifdef USE_VAMPIR
       vampir_end(GA_DESTROY,__FILE__,__LINE__);
#endif
       return TRUE;
    } 
#ifndef AVOID_MA_STORAGE
    if(gai_uses_shm((int)grp_id)){
#endif
      /* make sure that we free original (before address allignment) pointer */
#ifdef MPI
      if (grp_id > 0){
	 ARMCI_Free_group(GA[ga_handle].ptr[grp_me] - GA[ga_handle].id,
			  &PGRP_LIST[grp_id].group);
      }
      else
#endif
	 ARMCI_Free(GA[ga_handle].ptr[GAme] - GA[ga_handle].id);
#ifndef AVOID_MA_STORAGE
    }else{
      if(GA[ga_handle].id != INVALID_MA_HANDLE) MA_free_heap(GA[ga_handle].id);
    }
#endif

    if(GA_memory_limited) GA_total_memory += GA[ga_handle].size;
    GAstat.curmem -= GA[ga_handle].size;

#ifdef USE_VAMPIR
    vampir_end(GA_DESTROY,__FILE__,__LINE__);
#endif

    return(TRUE);
}

    
     
/*\ TERMINATE GLOBAL ARRAY STRUCTURES
 *
 *  all GA arrays are destroyed & shared memory is dealocated
 *  GA routines (except for ga_initialize) should not be called thereafter 
\*/
void FATR  ga_terminate_() 
{
Integer i, handle;

    _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
    if(!GAinitialized) return;

#ifdef USE_VAMPIR
    vampir_begin(GA_TERMINATE,__FILE__,__LINE__);
#endif
#ifdef ENABLE_PROFILE 
    ga_profile_terminate();
#endif
    for (i=0;i<_max_global_array;i++){
          handle = i - GA_OFFSET ;
          if(GA[i].actv) ga_destroy_(&handle);
          if(GA[i].ptr) free(GA[i].ptr);
          if(GA[i].mapc) free(GA[i].mapc);
    }
    ga_sync_();

    GA_total_memory = -1; /* restore "unlimited" memory usage status */
    GA_memory_limited = 0;
    free(_ga_map);
    free(GA_proclist);
    free(ProcListPerm);
#ifdef PERMUTE_PIDS
    free(ptr_array);
#endif
    free(mapALL);
    ARMCI_Free_local(GA_Update_Signal);

    ga_sync_();
    ARMCI_Finalize();
    GAinitialized = 0;

#ifdef USE_VAMPIR
    vampir_end(GA_TERMINATE,__FILE__,__LINE__);
    vampir_finalize(__FILE__,__LINE__);
#endif
}   

    
/*\ IS ARRAY ACTIVE/INACTIVE
\*/ 
Integer FATR ga_verify_handle_(g_a)
     Integer *g_a;
{
  return (Integer)
    ((*g_a + GA_OFFSET>= 0) && (*g_a + GA_OFFSET< _max_global_array) && 
             GA[GA_OFFSET + (*g_a)].actv);
}
 


/*\fill array with random values in [0,val)
\*/
void FATR ga_randomize_(Integer *g_a, void* val)
{
  int i,handle=GA_OFFSET + (int)*g_a;
  char *ptr;
  int local_sync_begin,local_sync_end;
  C_Long elems;
  Integer grp_id;
  Integer num_blocks;

#ifdef GA_USE_VAMPIR
  vampir_begin(GA_RANDOMIZE,__FILE__,__LINE__);
#endif

  GA_PUSH_NAME("ga_randomize");

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous sync masking*/
  grp_id = ga_get_pgroup_(g_a);
  if(local_sync_begin)ga_pgroup_sync_(&grp_id);


  ga_check_handleM(g_a, "ga_randomize");
  gam_checktype(GA[handle].type);
  elems = GA[handle].size/((C_Long)GA[handle].elemsize);
  num_blocks = GA[handle].block_total;

  if (num_blocks < 0) {
    /* Bruce..Please CHECK if this is correct */
    if (grp_id >= 0){  
      Integer grp_me = PGRP_LIST[GA[handle].p_handle].map_proc_list[GAme];
      ptr = GA[handle].ptr[grp_me];
    }
    else  ptr = GA[handle].ptr[GAme];

    switch (GA[handle].type){
/*
      case C_DCPL: 
        for(i=0; i<elems;i++)((DoubleComplex*)ptr)[i]=*(DoubleComplex*) rand();
        break;
      case C_SCPL: 
        for(i=0; i<elems;i++)((SingleComplex*)ptr)[i]=*(SingleComplex*)val;
        break;
*/
      case C_DBL:  
        for(i=0; i<elems;i++)((double*)ptr)[i]=*(double*) val * ((double)rand())/RAND_MAX;
        break;
      case C_INT:  
        for(i=0; i<elems;i++)((int*)ptr)[i]=*(int*) val * ((int)rand())/RAND_MAX;
        break;
      case C_FLOAT:
        for(i=0; i<elems;i++)((float*)ptr)[i]=*(float*) val * ((float)rand())/RAND_MAX;
        break;     
      case C_LONG:
        for(i=0; i<elems;i++)((long*)ptr)[i]=*(long*) val * ((long)rand())/RAND_MAX;
        break;
      case C_LONGLONG:
        for(i=0; i<elems;i++)((long long*)ptr)[i]=*( long long*) val * ((long long)rand())/RAND_MAX;
        break;
      default:
        gai_error("type not supported",GA[handle].type);
    }
  } else {
    Integer I_elems = (Integer)elems;
    nga_access_block_segment_ptr(g_a,&GAme,&ptr,&I_elems);
    elems = (C_Long)I_elems;
    switch (GA[handle].type){
/*
      case C_DCPL: 
        for(i=0; i<elems;i++)((DoubleComplex*)ptr)[i]=*(DoubleComplex*)val;
        break;
      case C_SCPL: 
        for(i=0; i<elems;i++)((SingleComplex*)ptr)[i]=*(SingleComplex*)val;
        break;
*/
      case C_DBL:  
        for(i=0; i<elems;i++)((double*)ptr)[i]=*(double*)val * ((double)rand())/RAND_MAX;
        break;
      case C_INT:  
        for(i=0; i<elems;i++)((int*)ptr)[i]=*(int*)val * ((int)rand())/RAND_MAX;
        break;
      case C_FLOAT:
        for(i=0; i<elems;i++)((float*)ptr)[i]=*(float*)val * ((float)rand())/RAND_MAX;
        break;     
      case C_LONG:
        for(i=0; i<elems;i++)((long*)ptr)[i]=*(long*)val * ((long)rand())/RAND_MAX;
        break;
      case C_LONGLONG:
        for(i=0; i<elems;i++)((long long*)ptr)[i]=*(long long*)val * ((long long)rand())/RAND_MAX;
        break;
      default:
        gai_error("type not supported",GA[handle].type);
    }
    nga_release_block_segment_(g_a,&GAme);
  }

  if(local_sync_end)ga_pgroup_sync_(&grp_id);

  GA_POP_NAME;

#ifdef GA_USE_VAMPIR
  vampir_end(GA_RANDOMIZE,__FILE__,__LINE__);
#endif
}

/*\fill array with value
\*/
void FATR ga_fill_(Integer *g_a, void* val)
{
  int i,handle=GA_OFFSET + (int)*g_a;
  char *ptr;
  int local_sync_begin,local_sync_end;
  C_Long elems;
  Integer grp_id;
  Integer num_blocks;

#ifdef USE_VAMPIR
  vampir_begin(GA_FILL,__FILE__,__LINE__);
#endif

  GA_PUSH_NAME("ga_fill");

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous sync masking*/
  grp_id = ga_get_pgroup_(g_a);
  if(local_sync_begin)ga_pgroup_sync_(&grp_id);


  ga_check_handleM(g_a, "ga_fill");
  gam_checktype(GA[handle].type);
  elems = GA[handle].size/((C_Long)GA[handle].elemsize);
  num_blocks = GA[handle].block_total;

  if (num_blocks < 0) {
    /* Bruce..Please CHECK if this is correct */
    if (grp_id >= 0){  
      Integer grp_me = PGRP_LIST[GA[handle].p_handle].map_proc_list[GAme];
      ptr = GA[handle].ptr[grp_me];
    }
    else  ptr = GA[handle].ptr[GAme];

    switch (GA[handle].type){
      case C_DCPL: 
        for(i=0; i<elems;i++)((DoubleComplex*)ptr)[i]=*(DoubleComplex*)val;
        break;
      case C_SCPL: 
        for(i=0; i<elems;i++)((SingleComplex*)ptr)[i]=*(SingleComplex*)val;
        break;
      case C_DBL:  
        for(i=0; i<elems;i++)((double*)ptr)[i]=*(double*)val;
        break;
      case C_INT:  
        for(i=0; i<elems;i++)((int*)ptr)[i]=*(int*)val;
        break;
      case C_FLOAT:
        for(i=0; i<elems;i++)((float*)ptr)[i]=*(float*)val;
        break;     
      case C_LONG:
        for(i=0; i<elems;i++)((long*)ptr)[i]=*(long*)val;
        break;
      case C_LONGLONG:
        for(i=0; i<elems;i++)((long long*)ptr)[i]=*( long long*)val;
        break;
      default:
        gai_error("type not supported",GA[handle].type);
    }
  } else {
    Integer I_elems = (Integer)elems;
    nga_access_block_segment_ptr(g_a,&GAme,&ptr,&I_elems);
    elems = (C_Long)I_elems;
    switch (GA[handle].type){
      case C_DCPL: 
        for(i=0; i<elems;i++)((DoubleComplex*)ptr)[i]=*(DoubleComplex*)val;
        break;
      case C_SCPL: 
        for(i=0; i<elems;i++)((SingleComplex*)ptr)[i]=*(SingleComplex*)val;
        break;
      case C_DBL:  
        for(i=0; i<elems;i++)((double*)ptr)[i]=*(double*)val;
        break;
      case C_INT:  
        for(i=0; i<elems;i++)((int*)ptr)[i]=*(int*)val;
        break;
      case C_FLOAT:
        for(i=0; i<elems;i++)((float*)ptr)[i]=*(float*)val;
        break;     
      case C_LONG:
        for(i=0; i<elems;i++)((long*)ptr)[i]=*(long*)val;
        break;
      case C_LONGLONG:
        for(i=0; i<elems;i++)((long long*)ptr)[i]=*(long long*)val;
        break;
      default:
        gai_error("type not supported",GA[handle].type);
    }
    nga_release_block_segment_(g_a,&GAme);
  }

  if(local_sync_end)ga_pgroup_sync_(&grp_id);

  GA_POP_NAME;

#ifdef USE_VAMPIR
  vampir_end(GA_FILL,__FILE__,__LINE__);
#endif
}

/* (old?) Fortran interface to ga_fill_ */

void FATR ga_cfill_(Integer *g_a, SingleComplex *val)
{
    ga_fill_(g_a, val);
}


void FATR ga_dfill_(Integer *g_a, DoublePrecision *val)
{
    ga_fill_(g_a, val);
}


void FATR ga_ifill_(Integer *g_a, Integer *val)
{
    ga_fill_(g_a, val);
}


void FATR ga_sfill_(Integer *g_a, Real *val)
{
    ga_fill_(g_a, val);
}


void FATR ga_zfill_(Integer *g_a, DoubleComplex *val)
{
    ga_fill_(g_a, val);
}


/*\ INQUIRE POPERTIES OF A GLOBAL ARRAY
 *   Fortran version for internal global array functions
\*/
void FATR ga_inquire_internal_(Integer* g_a, Integer* type, Integer* dim1, Integer* dim2)
{
    gai_inquire(g_a, type, dim1, dim2);
}


/*\ INQUIRE POPERTIES OF A GLOBAL ARRAY
 *  Fortran version
\*/ 
void FATR ga_inquire_(Integer* g_a, Integer* type, Integer* dim1,Integer* dim2)
{
    gai_inquire(g_a, type, dim1, dim2);
    *type = (Integer)ga_type_c2f(*type);
}


/*\ INQUIRE POPERTIES OF A GLOBAL ARRAY
 *  C version
\*/
void gai_inquire(Integer* g_a, Integer* type, Integer* dim1, Integer* dim2)
{
Integer ndim = ga_ndim_(g_a);

   if(ndim != 2)
      gai_error("gai_inquire: 2D API cannot be used for array dimension",ndim);

   *type       = GA[GA_OFFSET + *g_a].type;
   *dim1       = (Integer)GA[GA_OFFSET + *g_a].dims[0];
   *dim2       = (Integer)GA[GA_OFFSET + *g_a].dims[1];
}


/*\ INQUIRE POPERTIES OF A GLOBAL ARRAY
 *  Fortran version
\*/
void FATR nga_inquire_(Integer *g_a, Integer *type, Integer *ndim, Integer *dims)
{
    ngai_inquire(g_a, type, ndim, dims);
    *type = (Integer)ga_type_c2f(*type);
}


/*\ INQUIRE POPERTIES OF A GLOBAL ARRAY
 *  C version
\*/
void ngai_inquire(Integer *g_a, Integer *type, Integer *ndim, Integer *dims)
{
Integer handle = GA_OFFSET + *g_a,i;
   ga_check_handleM(g_a, "nga_inquire");
   *type       = GA[handle].type;
   *ndim       = GA[handle].ndim;
   for(i=0;i<*ndim;i++)dims[i]=(Integer)GA[handle].dims[i];
}


/*\ INQUIRE POPERTIES OF A GLOBAL ARRAY
 *  Fortran version for internal global array routines
\*/
void FATR nga_inquire_internal_(Integer *g_a, Integer *type, Integer *ndim,Integer *dims)
{
    ngai_inquire(g_a, type, ndim, dims);
}


/*\ RETURN A POINTER TO LOCAL DATA FOR BLOCK-CYCLIC DISTRIBUTION AND
 *  RETURN THE SIZE OF THE DATA BLOCK
\*/
void nga_inquire_block_internal(Integer* g_a, Integer proc, Integer *size, void* ptr)
{
  Integer  handle = GA_OFFSET + *g_a;

  ga_check_handleM(g_a, "nga_inquire_block_internal");
  ptr = GA[handle].ptr[proc];
  
}


/*\ INQUIRE NAME OF A GLOBAL ARRAY
 *  Fortran version
\*/
void FATR ga_inquire_name_(Integer *g_a, char *array_name, int len)
{
   ga_c2fstring(GA[GA_OFFSET + *g_a].name, array_name, len);
}


/*\ INQUIRE NAME OF A GLOBAL ARRAY
 *  C version
\*/
void gai_inquire_name(Integer *g_a, char **array_name)
{ 
   ga_check_handleM(g_a, "gai_inquire_name");
   *array_name = GA[GA_OFFSET + *g_a].name;
}


/*\ RETURN PROCESSOR COORDINATES
\*/
void FATR nga_proc_topology_(Integer* g_a, Integer* proc, Integer* subscript)
{
Integer d, index, ndim, ga_handle = GA_OFFSET + *g_a;

   ga_check_handleM(g_a, "nga_proc_topology");
   ndim = GA[ga_handle].ndim;

   index = GA_Proc_list ? GA_Proc_list[*proc]: *proc;

   for(d=0; d<ndim; d++){
       subscript[d] = index% GA[ga_handle].nblock[d];
       index  /= GA[ga_handle].nblock[d];  
   }
}

/*\ RETURN DIMENSIONS OF PROCESSOR GRID
\*/
void FATR ga_get_proc_grid_(Integer *g_a, Integer *dims)
{
  Integer i, ndim, ga_handle = GA_OFFSET + *g_a;
  ga_check_handleM(g_a, "ga_get_proc_grid");
  ndim = GA[ga_handle].ndim;
  for (i=0; i<ndim; i++) {
    dims[i] = GA[ga_handle].nblock[i];
  }
}

void gai_get_proc_from_block_index_(Integer *g_a, Integer *index, Integer *proc)
{
  Integer ga_handle = GA_OFFSET + *g_a;
  Integer ndim = GA[ga_handle].ndim;
  Integer i, ld;
  if (ga_uses_proc_grid_(g_a)) {
    int *proc_grid = GA[ga_handle].nblock;
    Integer proc_id[MAXDIM];
    for (i=0; i<ndim; i++) {
      proc_id[i] = index[i]%proc_grid[i];
    }
    ld = 1;
    *proc = index[0];
    for (i=1; i<ndim; i++) {
      ld *= proc_grid[i];
      *proc *= ld;
      *proc += index[i];
    }
  } else {
    C_Integer *block_grid = GA[ga_handle].num_blocks;
    *proc = index[ndim-1];
    ld = 1;
    for (i=ndim-2; i >= 0; i--) {
       ld *= block_grid[i];
       *proc *= ld;
       *proc += index[i];
    }
    *proc = *proc%ga_nnodes_();
  }
}


#define findblock(map_ij,n,scale,elem, block)\
{\
int candidate, found, b; \
C_Integer *map= (map_ij);\
\
    candidate = (int)(scale*(elem));\
    found = 0;\
    if(map[candidate] <= (elem)){ /* search downward */\
         b= candidate;\
         while(b<(n)-1){ \
            found = (map[b+1]>(elem));\
            if(found)break;\
            b++;\
         } \
    }else{ /* search upward */\
         b= candidate-1;\
         while(b>=0){\
            found = (map[b]<=(elem));\
            if(found)break;\
            b--;\
         }\
    }\
    if(!found)b=(n)-1;\
    *(block) = b;\
}



/*\ LOCATE THE OWNER OF SPECIFIED ELEMENT OF A GLOBAL ARRAY
\*/
logical FATR nga_locate_(Integer *g_a, Integer* subscript, Integer* owner)
{
Integer d, proc, dpos, ndim, ga_handle = GA_OFFSET + *g_a, proc_s[MAXDIM];
int use_blocks;

   ga_check_handleM(g_a, "nga_locate");
   ndim = GA[ga_handle].ndim;
   use_blocks = GA[ga_handle].block_flag;

   if (!use_blocks) {
     for(d=0, *owner=-1; d< ndim; d++) 
       if(subscript[d]< 1 || subscript[d]>GA[ga_handle].dims[d]) return FALSE;

     for(d = 0, dpos = 0; d< ndim; d++){
       findblock(GA[ga_handle].mapc + dpos, GA[ga_handle].nblock[d],
           GA[ga_handle].scale[d], subscript[d], &proc_s[d]);
       dpos += GA[ga_handle].nblock[d];
     }

     ga_ComputeIndexM(&proc, ndim, proc_s, GA[ga_handle].nblock); 

     *owner = GA_Proc_list ? GA_Proc_list[proc]: proc;
     if (GA[ga_handle].num_rstrctd > 0) {
       *owner = GA[ga_handle].rstrctd_list[*owner];
     }
   } else {
     if (GA[ga_handle].block_sl_flag == 0) {
       Integer i, j, chk, lo[MAXDIM], hi[MAXDIM];
       Integer num_blocks = GA[ga_handle].block_total;
       for (i=0; i< num_blocks; i++) {
         nga_distribution_(g_a, &i, lo, hi);
         chk = 1;
         for (j=0; j<ndim; j++) {
           if (subscript[j]<lo[j] || subscript[j] > hi[j]) chk = 0;
         }
         if (chk) {
           *owner = i;
           break;
         }
       }
     } else {
       Integer index[MAXDIM];
       Integer i;
       for (i=0; i<ndim; i++) {
         index[i] = (subscript[i]-1)/GA[ga_handle].block_dims[i];
       }
       gam_find_block_from_indices(ga_handle, i, index);
       *owner = i;
     }
   }
   
   return TRUE;
}


/*\
 * RETURN HOW MANY PROCESSORS/OWNERS THERE ARE FOR THE SPECIFIED PATCH OF A
 * GLOBAL ARRAY
\*/
logical FATR nga_locate_nnodes_( Integer *g_a,
                                 Integer *lo,
                                 Integer *hi,
                                 Integer *np)
/*    g_a      [input]  global array handle
      lo       [input]  lower indices of patch in global array
      hi       [input]  upper indices of patch in global array
      np       [output] total number of processors containing a portion
                        of the patch

      For a block cyclic data distribution, this function returns a list of
      blocks that cover the region along with the lower and upper indices of
      each block.
*/
{
  int  procT[MAXDIM], procB[MAXDIM], proc_subscript[MAXDIM];
  Integer  proc, /*owner,*/ i, ga_handle;
  Integer  d, dpos, ndim, elems, p_handle, use_blocks;

  ga_check_handleM(g_a, "nga_locate_nnodes");

  ga_handle = GA_OFFSET + *g_a;
#ifdef __crayx1
#pragma _CRI novector
#endif
  for(d = 0; d< GA[ga_handle].ndim; d++)
    if((lo[d]<1 || hi[d]>GA[ga_handle].dims[d]) ||(lo[d]>hi[d]))return FALSE;

  ndim = GA[ga_handle].ndim;
  use_blocks = GA[ga_handle].block_flag;

  if (!use_blocks) {
    /* find "processor coordinates" for the top left corner and store them
     * in ProcT */
#ifdef __crayx1
#pragma _CRI novector
#endif
    for(d = 0, dpos = 0; d< GA[ga_handle].ndim; d++){
      findblock(GA[ga_handle].mapc + dpos, GA[ga_handle].nblock[d], 
          GA[ga_handle].scale[d], lo[d], &procT[d]);
      dpos += GA[ga_handle].nblock[d];
    }

    /* find "processor coordinates" for the right bottom corner and store
     * them in procB */
#ifdef __crayx1
#pragma _CRI novector
#endif
    for(d = 0, dpos = 0; d< GA[ga_handle].ndim; d++){
      findblock(GA[ga_handle].mapc + dpos, GA[ga_handle].nblock[d], 
          GA[ga_handle].scale[d], hi[d], &procB[d]);
      dpos += GA[ga_handle].nblock[d];
    }

    *np = 0;

    /* Find total number of processors containing data and return the
     * result in elems. Also find the lowest "processor coordinates" of the
     * processor block containing data and return these in proc_subscript.
     */
    ga_InitLoopM(&elems, ndim, proc_subscript, procT,procB,GA[ga_handle].nblock);

    p_handle = (Integer)GA[ga_handle].p_handle;
    for(i= 0; i< elems; i++){ 
      Integer _lo[MAXDIM], _hi[MAXDIM];
      /*Integer  offset;*/

      /* convert i to owner processor id using the current values in
         proc_subscript */
      ga_ComputeIndexM(&proc, ndim, proc_subscript, GA[ga_handle].nblock); 
      /* get range of global array indices that are owned by owner */
      ga_ownsM(ga_handle, proc, _lo, _hi);

#if 0
      offset = *np *(ndim*2); /* location in map to put patch range */

#ifdef __crayx1
#pragma _CRI novector
#endif
      for(d = 0; d< ndim; d++)
        map[d + offset ] = lo[d] < _lo[d] ? _lo[d] : lo[d];
#ifdef __crayx1
#pragma _CRI novector
#endif
      for(d = 0; d< ndim; d++)
        map[ndim + d + offset ] = hi[d] > _hi[d] ? _hi[d] : hi[d];

      owner = proc;
      if (GA[ga_handle].num_rstrctd == 0) {
        proclist[i] = owner;
      } else {
        proclist[i] = GA[ga_handle].rstrctd_list[owner];
      }
#endif
      /* Update to proc_subscript so that it corresponds to the next
       * processor in the block of processors containing the patch */
      ga_UpdateSubscriptM(ndim,proc_subscript,procT,procB,GA[ga_handle].nblock);
      (*np)++;
    }
  } else {
    Integer nblocks = GA[ga_handle].block_total;
    Integer chk, j, tlo[MAXDIM], thi[MAXDIM], cnt;
    /*Integer offset;*/
    cnt = 0;
    for (i=0; i<nblocks; i++) {
      /* check to see if this block overlaps with requested block
       * defined by lo and hi */
      chk = 1;
      /* get limits on block i */
      nga_distribution_(g_a,&i,tlo,thi);
      for (j=0; j<ndim && chk==1; j++) {
        /* check to see if at least one end point of the interval
         * represented by blo and bhi falls in the interval
         * represented by lo and hi */
        if (!((tlo[j] >= lo[j] && tlo[j] <= hi[j]) ||
              (thi[j] >= lo[j] && thi[j] <= hi[j]))) {
          chk = 0;
        }
      }
      /* store blocks that overlap request region in proclist */
      if (chk) {
#if 0
        proclist[cnt] = i;
#endif
        cnt++;
      }
    }
    *np = cnt;

#if 0
    /* fill map array with block coordinates */
    for (i=0; i<cnt; i++) {
      offset = i*2*ndim;
      j = proclist[i];
      nga_distribution_(g_a,&j,tlo,thi);
      for (j=0; j<ndim; j++) {
        map[offset + j] = lo[j] < tlo[j] ? tlo[j] : lo[j];
        map[offset + ndim + j] = hi[j] > thi[j] ? thi[j] : hi[j];
      }
    }
#endif
  }
  return(TRUE);
}
#ifdef __crayx1
#pragma _CRI inline nga_locate_nnodes_
#endif


/*\ LOCATE PROCESSORS/OWNERS OF THE SPECIFIED PATCH OF A GLOBAL ARRAY
\*/
logical FATR nga_locate_region_( Integer *g_a,
                                 Integer *lo,
                                 Integer *hi,
                                 Integer *map,
                                 Integer *proclist,
                                 Integer *np)
/*    g_a      [input]  global array handle
      lo       [input]  lower indices of patch in global array
      hi       [input]  upper indices of patch in global array
      map      [output] list of lower and upper indices for portion of
                        patch that exists on each processor containing a
                        portion of the patch. The map is constructed so
                        that for a D dimensional global array, the first
                        D elements are the lower indices on the first
                        processor in proclist, the next D elements are
                        the upper indices of the first processor in
                        proclist, the next D elements are the lower
                        indices for the second processor in proclist, and
                        so on.
      proclist [output] list of processors containing some portion of the
                        patch
      np       [output] total number of processors containing a portion
                        of the patch

      For a block cyclic data distribution, this function returns a list of
      blocks that cover the region along with the lower and upper indices of
      each block.
*/
{
  int  procT[MAXDIM], procB[MAXDIM], proc_subscript[MAXDIM];
  Integer  proc, owner, i, ga_handle;
  Integer  d, dpos, ndim, elems, p_handle, use_blocks;

  ga_check_handleM(g_a, "nga_locate_region");

  ga_handle = GA_OFFSET + *g_a;
#ifdef __crayx1
#pragma _CRI novector
#endif
  for(d = 0; d< GA[ga_handle].ndim; d++)
    if((lo[d]<1 || hi[d]>GA[ga_handle].dims[d]) ||(lo[d]>hi[d]))return FALSE;

  ndim = GA[ga_handle].ndim;
  use_blocks = GA[ga_handle].block_flag;

  if (!use_blocks) {
    /* find "processor coordinates" for the top left corner and store them
     * in ProcT */
#ifdef __crayx1
#pragma _CRI novector
#endif
    for(d = 0, dpos = 0; d< GA[ga_handle].ndim; d++){
      findblock(GA[ga_handle].mapc + dpos, GA[ga_handle].nblock[d], 
          GA[ga_handle].scale[d], lo[d], &procT[d]);
      dpos += GA[ga_handle].nblock[d];
    }

    /* find "processor coordinates" for the right bottom corner and store
     * them in procB */
#ifdef __crayx1
#pragma _CRI novector
#endif
    for(d = 0, dpos = 0; d< GA[ga_handle].ndim; d++){
      findblock(GA[ga_handle].mapc + dpos, GA[ga_handle].nblock[d], 
          GA[ga_handle].scale[d], hi[d], &procB[d]);
      dpos += GA[ga_handle].nblock[d];
    }

    *np = 0;

    /* Find total number of processors containing data and return the
     * result in elems. Also find the lowest "processor coordinates" of the
     * processor block containing data and return these in proc_subscript.
     */
    ga_InitLoopM(&elems, ndim, proc_subscript, procT,procB,GA[ga_handle].nblock);

    p_handle = (Integer)GA[ga_handle].p_handle;
    for(i= 0; i< elems; i++){ 
      Integer _lo[MAXDIM], _hi[MAXDIM];
      Integer  offset;

      /* convert i to owner processor id using the current values in
         proc_subscript */
      ga_ComputeIndexM(&proc, ndim, proc_subscript, GA[ga_handle].nblock); 
      /* get range of global array indices that are owned by owner */
      ga_ownsM(ga_handle, proc, _lo, _hi);

      offset = *np *(ndim*2); /* location in map to put patch range */

#ifdef __crayx1
#pragma _CRI novector
#endif
      for(d = 0; d< ndim; d++)
        map[d + offset ] = lo[d] < _lo[d] ? _lo[d] : lo[d];
#ifdef __crayx1
#pragma _CRI novector
#endif
      for(d = 0; d< ndim; d++)
        map[ndim + d + offset ] = hi[d] > _hi[d] ? _hi[d] : hi[d];

      owner = proc;
      if (GA[ga_handle].num_rstrctd == 0) {
        proclist[i] = owner;
      } else {
        proclist[i] = GA[ga_handle].rstrctd_list[owner];
      }
      /* Update to proc_subscript so that it corresponds to the next
       * processor in the block of processors containing the patch */
      ga_UpdateSubscriptM(ndim,proc_subscript,procT,procB,GA[ga_handle].nblock);
      (*np)++;
    }
  } else {
    Integer nblocks = GA[ga_handle].block_total;
    Integer chk, j, tlo[MAXDIM], thi[MAXDIM], cnt;
    Integer offset;
    cnt = 0;
    for (i=0; i<nblocks; i++) {
      /* check to see if this block overlaps with requested block
       * defined by lo and hi */
      chk = 1;
      /* get limits on block i */
      nga_distribution_(g_a,&i,tlo,thi);
      for (j=0; j<ndim && chk==1; j++) {
        /* check to see if at least one end point of the interval
         * represented by blo and bhi falls in the interval
         * represented by lo and hi */
        if (!((tlo[j] >= lo[j] && tlo[j] <= hi[j]) ||
              (thi[j] >= lo[j] && thi[j] <= hi[j]))) {
          chk = 0;
        }
      }
      /* store blocks that overlap request region in proclist */
      if (chk) {
        proclist[cnt] = i;
        cnt++;
      }
    }
    *np = cnt;

    /* fill map array with block coordinates */
    for (i=0; i<cnt; i++) {
      offset = i*2*ndim;
      j = proclist[i];
      nga_distribution_(g_a,&j,tlo,thi);
      for (j=0; j<ndim; j++) {
        map[offset + j] = lo[j] < tlo[j] ? tlo[j] : lo[j];
        map[offset + ndim + j] = hi[j] > thi[j] ? thi[j] : hi[j];
      }
    }
  }
  return(TRUE);
}
#ifdef __crayx1
#pragma _CRI inline nga_locate_region_
#endif


/*\ returns in nblock array the number of blocks each dimension is divided to
\*/
void GA_Nblock(int g_a, int *nblock)
{
int ga_handle = GA_OFFSET + g_a;
int i, n;

     ga_check_handleM(&g_a, "GA_Nblock");

     n = GA[ga_handle].ndim;

#ifdef USE_FAPI 
     for(i=0; i<n; i++) nblock[i] = GA[ga_handle].nblock[i];
#else
     for(i=0; i<n; i++) nblock[n-i-1] = GA[ga_handle].nblock[i];
#endif
     
}
     

void FATR ga_nblock_(Integer *g_a, Integer *nblock)
{
Integer ga_handle = GA_OFFSET + *g_a;
int i, n;

     ga_check_handleM(g_a, "ga_nblock");

     n = GA[ga_handle].ndim;

     for(i=0; i<n; i++) nblock[i] = (Integer)GA[ga_handle].nblock[i];
}


Integer FATR ga_nodeid_()
{
    if (GA_Default_Proc_Group > 0) {
       return (Integer)PGRP_LIST[GA_Default_Proc_Group].map_proc_list[GAme];
    } else {
       return ((Integer)GAme);
    }
}

Integer FATR ga_pgroup_nodeid_(Integer *grp)
{
    if (*grp >= 0) {
       return (Integer)PGRP_LIST[(int)(*grp)].map_proc_list[GAme];
    } else {
       return GAme;
    }
}


Integer FATR ga_nnodes_()
{
    if (GA_Default_Proc_Group > 0) {
       return (Integer)PGRP_LIST[GA_Default_Proc_Group].map_nproc;
    } else {
       return ((Integer)GAnproc);
    }
}

Integer FATR ga_pgroup_nnodes_(Integer *grp)
{
    if(*grp >=0 )
       return (Integer)PGRP_LIST[(int)(*grp)].map_nproc;
    else
       return ((Integer)GAnproc);
}


/*\ COMPARE DISTRIBUTIONS of two global arrays
\*/
logical FATR ga_compare_distr_(Integer *g_a, Integer *g_b)
{
int h_a =(int)*g_a + GA_OFFSET;
int h_b =(int)*g_b + GA_OFFSET;
int i;

   _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
   GA_PUSH_NAME("ga_compare_distr");
   ga_check_handleM(g_a, "distribution a");
   ga_check_handleM(g_b, "distribution b");
   
   GA_POP_NAME;

   if(GA[h_a].ndim != GA[h_b].ndim) return FALSE; 

   for(i=0; i <GA[h_a].ndim; i++)
       if(GA[h_a].dims[i] != GA[h_b].dims[i]) return FALSE;

   if (GA[h_a].block_flag != GA[h_b].block_flag) return FALSE;
   if (GA[h_a].block_sl_flag != GA[h_b].block_sl_flag) return FALSE;
   if (!GA[h_a].block_flag) {
     for(i=0; i <MAPLEN; i++){
       if(GA[h_a].mapc[i] != GA[h_b].mapc[i]) return FALSE;
       if(GA[h_a].mapc[i] == -1) break;
     }
   } else {
     for (i=0; i<GA[h_a].ndim; i++) {
       if (GA[h_a].block_dims[i] != GA[h_b].block_dims[i]) return FALSE;
     }
     for (i=0; i<GA[h_a].ndim; i++) {
       if (GA[h_a].nblock[i] != GA[h_b].nblock[i]) return FALSE;
     }
   }
   if (GA[h_a].num_rstrctd == GA[h_b].num_rstrctd) {
     if (GA[h_a].num_rstrctd > 0) {
       for (i=0; i<GA[h_a].num_rstrctd; i++) {
         if (GA[h_a].rstrctd_list[i] != GA[h_b].rstrctd_list[i]) return FALSE;
       }
     }
   } else {
     return FALSE;
   }
   return TRUE;
}


static int num_mutexes=0;
static int chunk_mutex;

logical FATR ga_create_mutexes_(Integer *num)
{
int myshare;

#ifdef USE_VAMPIR
   vampir_begin(GA_CREATE_MUTEXES,__FILE__,__LINE__);
#endif
   _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
   if (*num <= 0 || *num > MAX_MUTEXES) return(FALSE);
   if(num_mutexes) gai_error("mutexes already created",num_mutexes);

   num_mutexes= (int)*num;

   if(GAnproc == 1){
#ifdef USE_VAMPIR
      vampir_end(GA_CREATE_MUTEXES,__FILE__,__LINE__);
#endif
      return(TRUE);
   }
   chunk_mutex = (int)((*num + GAnproc-1)/GAnproc);
   if(GAme * chunk_mutex >= *num)myshare =0;
   else myshare=chunk_mutex;

   /* need work here to use permutation */
   if(ARMCI_Create_mutexes(myshare)){
#ifdef USE_VAMPIR
      vampir_end(GA_CREATE_MUTEXES,__FILE__,__LINE__);
#endif
      return FALSE;
   }
#ifdef USE_VAMPIR
   vampir_end(GA_CREATE_MUTEXES,__FILE__,__LINE__);
#endif
   return TRUE;
}


void FATR ga_lock_(Integer *mutex)
{
int m,p;

   if(GAnproc == 1) return;
   if(num_mutexes< *mutex)gai_error("invalid mutex",*mutex);

#ifdef USE_VAMPIR
   vampir_begin(GA_LOCK,__FILE__,__LINE__);
#endif

   p = num_mutexes/chunk_mutex -1;
   m = num_mutexes%chunk_mutex;

#ifdef PERMUTE_PIDS
    if(GA_Proc_list) p = GA_inv_Proc_list[p];
#endif

   ARMCI_Lock(m,p);
 
#ifdef USE_VAMPIR
   vampir_end(GA_LOCK,__FILE__,__LINE__);
#endif
}


void FATR ga_unlock_(Integer *mutex)
{
int m,p;

   if(GAnproc == 1) return;
   if(num_mutexes< *mutex)gai_error("invalid mutex",*mutex);
   
#ifdef USE_VAMPIR
   vampir_begin(GA_UNLOCK,__FILE__,__LINE__);
#endif

   p = num_mutexes/chunk_mutex -1;
   m = num_mutexes%chunk_mutex;

#ifdef PERMUTE_PIDS
    if(GA_Proc_list) p = GA_inv_Proc_list[p];
#endif

   ARMCI_Unlock(m,p);

#ifdef USE_VAMPIR
   vampir_end(GA_UNLOCK,__FILE__,__LINE__);
#endif
}              
   

logical FATR ga_destroy_mutexes_()
{
   _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
   if(num_mutexes<1) gai_error("mutexes destroyed",0);

#ifdef USE_VAMPIR
   vampir_begin(GA_DESTROY_MUTEXES,__FILE__,__LINE__);
#endif

   num_mutexes= 0;
   if(GAnproc == 1){
#ifdef USE_VAMPIR
      vampir_end(GA_DESTROY_MUTEXES,__FILE__,__LINE__);
#endif
      return TRUE;
   }
   if(ARMCI_Destroy_mutexes()){
#ifdef USE_VAMPIR
      vampir_end(GA_DESTROY_MUTEXES,__FILE__,__LINE__);
#endif
      return FALSE;
   }
#ifdef USE_VAMPIR
   vampir_end(GA_DESTROY_MUTEXES,__FILE__,__LINE__);
#endif
   return TRUE;
}


/*\ return list of message-passing process ids for GA process ids
\*/
void FATR ga_list_nodeid_(list, num_procs)
     Integer *list, *num_procs;
{
Integer proc;
   for( proc = 0; proc < *num_procs; proc++)

#ifdef PERMUTE_PIDS
       if(GA_Proc_list) list[proc] = GA_inv_Proc_list[proc]; 
       else
#endif
       list[proc]=proc;
}


/*************************************************************************/

logical FATR ga_locate_region_(g_a, ilo, ihi, jlo, jhi, mapl, np )
        Integer *g_a, *ilo, *jlo, *ihi, *jhi, mapl[][5], *np;
{
   logical status = FALSE;
   Integer lo[2], hi[2], p;
   if (!GA[GA_OFFSET+(*g_a)].block_flag) {
     lo[0]=*ilo; lo[1]=*jlo;
     hi[0]=*ihi; hi[1]=*jhi;

     status = nga_locate_region_(g_a,lo,hi,_ga_map, GA_proclist, np);

     /* need to swap elements (ilo,jlo,ihi,jhi) -> (ilo,ihi,jlo,jhi) */
     for(p = 0; p< *np; p++){
       mapl[p][0] = _ga_map[4*p];
       mapl[p][1] = _ga_map[4*p + 2];
       mapl[p][2] = _ga_map[4*p + 1];
       mapl[p][3] = _ga_map[4*p + 3];
       mapl[p][4] = GA_proclist[p];
     } 
   } else {
     gai_error("Must call nga_locate_region on block-cyclic data distribution",0);
   }

   return status;
}



/*\ LOCATE THE OWNER OF THE (i,j) ELEMENT OF A GLOBAL ARRAY
\*/
logical FATR ga_locate_(g_a, i, j, owner)
        Integer *g_a, *i, *j, *owner;
{
Integer subscript[2];
  
    subscript[0]=*i; subscript[1]=*j;

    return nga_locate_(g_a, subscript, owner);
}


/*\ RETURN COORDINATES OF A 2-D GA PATCH ASSOCIATED WITH PROCESSOR proc
\*/
void FATR  ga_distribution_(g_a, proc, ilo, ihi, jlo, jhi)
   Integer *g_a, *ilo, *ihi, *jlo, *jhi, *proc;
{
Integer lo[2], hi[2];
Integer ndim = ga_ndim_(g_a);

   if(ndim != 2)
      gai_error("ga_distribution:2D API cannot be used for dimension",ndim);

   nga_distribution_(g_a, proc, lo, hi);
   *ilo = lo[0]; *ihi=hi[0];
   *jlo = lo[1]; *jhi=hi[1]; 
}


/*\ RETURN COORDINATES OF ARRAY BLOCK HELD BY A PROCESSOR
\*/
void FATR ga_proc_topology_(g_a, proc, pr, pc)
   Integer *g_a, *proc, *pr, *pc;
{
Integer subscript[2];
   nga_proc_topology_(g_a, proc,subscript);
   *pr = subscript[0]; 
   *pc = subscript[1]; 
}


/*\ returns true/false depending on validity of the handle
\*/
logical FATR ga_valid_handle_(Integer *g_a)
{
   if(GA_OFFSET+ (*g_a) < 0 || GA_OFFSET+(*g_a) >= _max_global_array ||
      ! (GA[GA_OFFSET+(*g_a)].actv) ) return FALSE;
   else return TRUE;
}

int gai_getval(int *ptr) { return *ptr;}

/*\ A function that helps user avoid syncs that he thinks are unnecessary
    inside a collective call.
\*/

/*
       Mask flags have to be reset in every collective call. Even if that
       collective call doesnt do any sync at all.
       If masking only the beginning sync is possible, make sure to
       clear even the _sync_end mask to avoid a mask intended for this
       collective_function_call to be carried to next collective_function_call
       or to a collective function called by this function.
       Similarly, make sure to use two copy mask values to local variables
       and reset the global mask variables to avoid carring the mask to a
       collective call inside the current collective call.
*/
void FATR ga_mask_sync_(Integer *begin, Integer *end)
{
  if (*begin) _ga_sync_begin = 1;
  else _ga_sync_begin = 0;

  if (*end) _ga_sync_end = 1;
  else _ga_sync_end = 0;
}

/*\ merge all copies of a mirrored array by adding them together
\*/
void FATR ga_merge_mirrored_(Integer *g_a)
{
  Integer handle = GA_OFFSET + *g_a;
  Integer inode, nprocs, nnodes, zero, zproc, nblocks;
  int *blocks;
  C_Integer  *map, *dims, *width;
  Integer i, j, index[MAXDIM], itmp, ndim;
  Integer nelem, count, type, atype=ARMCI_INT;
  char *zptr=NULL, *bptr=NULL, *nptr=NULL;
  Integer bytes, total;
  int local_sync_begin, local_sync_end;

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end = 1; /*remove any previous masking */
  if (local_sync_begin) ga_sync_();
  /* don't perform update if node is not mirrored */
  if (!ga_is_mirrored_(g_a)) return;
  GA_PUSH_NAME("ga_merge_mirrored");

  inode = ga_cluster_nodeid_();
  nnodes = ga_cluster_nnodes_(); 
  nprocs = ga_cluster_nprocs_(&inode);
  zero = 0;

  zproc = ga_cluster_procid_(&inode, &zero);
  zptr = GA[handle].ptr[zproc];
  map = GA[handle].mapc;
  blocks = GA[handle].nblock;
  dims = GA[handle].dims;
  width = GA[handle].width;
  type = GA[handle].type;
  ndim = GA[handle].ndim;

  /* Check whether or not all nodes contain the same number
     of processors. */
  if (nnodes*nprocs == ga_nnodes_())  {
    /* check to see if there is any buffer space between the data
       associated with each processor that needs to be zeroed out
       before performing the merge */
    if (zproc == GAme) {
      /* the use of nblocks instead of nprocs is designed to support a peculiar
         coding style in which the dimensions of the block array are all set to
         1 and all the data is restricted to the master processor on the node */
      nblocks = 1;
      for (i=0; i<ndim; i++) {
        nblocks *= blocks[i];
      }
      for (i=0; i<nblocks; i++) {
        /* Find out from mapc data how many elements are supposed to be located
           on this processor. Start by converting processor number to indices */
        itmp = i;
        for (j=0; j<ndim; j++) {
          index[j] = itmp%(Integer)blocks[j];
          itmp = (itmp - index[j])/(Integer)blocks[j];
        }

        nelem = 1;
        count = 0;
        for (j=0; j<ndim; j++) {
          if (index[j] < (Integer)blocks[j]-1) {
            nelem *= (Integer)(map[index[j]+1+count] - map[index[j]+count]
                   + 2*width[j]);
          } else {
            nelem *= (Integer)(dims[j] - map[index[j]+count] + 1 + 2*width[j]);
          }
          count += (Integer)blocks[j];
        }
        /* We now have the total number of elements located on this processor.
           Find out if the location of the end of this data set matches the
           origin of the data on the next processor. If not, then zero data in
           the gap. */
        nelem *= GAsizeof(type);
        bptr = GA[handle].ptr[ga_cluster_procid_(&inode, &i)];
        bptr += nelem;
        if (i<nblocks-1) {
          j = i+1;
          nptr = GA[handle].ptr[ga_cluster_procid_(&inode, &j)];
          if (bptr != nptr) {
            bytes = (long)nptr - (long)bptr;
            /* BJP printf("p[%d] Gap on proc %d is %d\n",GAme,i,bytes); */
            bzero(bptr, bytes);
          }
        }
      }
      /* find total number of bytes containing global array */
      total = (long)bptr - (long)zptr;
      total /= GAsizeof(type);
      /*convert from C data type to ARMCI type */
      switch(type) {
        case C_FLOAT: atype=ARMCI_FLOAT; break;
        case C_DBL: atype=ARMCI_DOUBLE; break;
        case C_LONG: atype=ARMCI_LONG; break;
        case C_INT: atype=ARMCI_INT; break;
        case C_DCPL: atype=ARMCI_DOUBLE; break;
        case C_SCPL: atype=ARMCI_FLOAT; break;
        default: gai_error("type not supported",type);
      }
      /* now that gap data has been zeroed, do a global sum on data */
      armci_msg_gop_scope(SCOPE_MASTERS, zptr, total, "+", atype);
    } 
  } else {
    Integer _ga_tmp;
    Integer lo[MAXDIM], hi[MAXDIM], ld[MAXDIM];
    Integer idims[MAXDIM], iwidth[MAXDIM], ichunk[MAXDIM];
    int chk = 1;
    void *ptr_a;
    void *one = NULL;
    double d_one = 1.0;
    int i_one = 1;
    float f_one = 1.0;
    long l_one = 1;
    double c_one[2];
    float cf_one[2];
    c_one[0] = 1.0;
    c_one[1] = 0.0;

    cf_one[0] = 1.0;
    cf_one[1] = 0.0;

    /* choose one as scaling factor in accumulate */
    switch (type) {
      case C_FLOAT: one = &f_one; break;
      case C_DBL: one = &d_one; break;
      case C_LONG: one = &l_one; break;
      case C_INT: one = &i_one; break;
      case C_DCPL: one = &c_one; break;
      case C_SCPL: one = &cf_one; break;
      default: gai_error("type not supported",type);
    }
    
  /* Nodes contain a mixed number of processors. Create a temporary GA to
     complete merge operation. */
    count = 0;
    for (i=0; i<ndim; i++) {
      idims[i] = (Integer)dims[i];
      iwidth[i] = (Integer)width[i];
      ichunk[i] = 0;
    }
    if (!ngai_create_ghosts(type, ndim, idims,
        iwidth, "temporary", ichunk, &_ga_tmp)) 
      gai_error("Unable to create work array for merge",GAme);
    ga_zero_(&_ga_tmp);
    /* Find data on this processor and accumulate in temporary global array */
    inode = GAme - zproc;
    nga_distribution_(g_a,&inode,lo,hi);

    /* Check to make sure processor has data */
    chk = 1;
    for (i=0; i<ndim; i++) {
      if (hi[i]<lo[i]) {
        chk = 0;
      }
    }
    if (chk) {
      nga_access_ptr(g_a, lo, hi, &ptr_a, ld);
      nga_acc_(&_ga_tmp, lo, hi, ptr_a, ld, one);
    }
    /* copy data back to original global array */
    ga_sync_();
    if (chk) {
      nga_get_(&_ga_tmp, lo, hi, ptr_a, ld);
    }
    ga_destroy_(&_ga_tmp);
  }
  if (local_sync_end) ga_sync_();
  GA_POP_NAME;
}

/*\ merge all copies of a  patch of a mirrored array into a patch in a
 *  distributed array
\*/
void FATR nga_merge_distr_patch_(Integer *g_a, Integer *alo, Integer *ahi,
                                 Integer *g_b, Integer *blo, Integer *bhi)
/*    Integer *g_a  handle to mirrored array
      Integer *alo  indices of lower corner of mirrored array patch
      Integer *ahi  indices of upper corner of mirrored array patch
      Integer *g_b  handle to distributed array
      Integer *blo  indices of lower corner of distributed array patch
      Integer *bhi  indices of upper corner of distributed array patch
*/
{
  Integer local_sync_begin, local_sync_end;
  Integer a_handle, b_handle, adim, bdim;
  Integer mlo[MAXDIM], mhi[MAXDIM], mld[MAXDIM];
  Integer dlo[MAXDIM], dhi[MAXDIM];
  char trans[2];
  double d_one;
  Integer type, i_one;
  double z_one[2];
  float  c_one[2];
  float f_one;
  long l_one;
  void *src_data_ptr;
  void *one = NULL;
  Integer i, idim, intersect, p_handle;

  GA_PUSH_NAME("nga_merge_distr_patch");
  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end = 1; /*remove any previous masking */
  if (local_sync_begin) ga_sync_();
  gai_check_handle(g_a, "nga_merge_distr_patch");
  gai_check_handle(g_b, "nga_merge_distr_patch");

  /* check to make sure that both patches lie within global arrays and
     that patches are the same dimensions */
  a_handle = GA_OFFSET + *g_a;
  b_handle = GA_OFFSET + *g_b;

  if (!ga_is_mirrored_(g_a)) {
    if (ga_cluster_nnodes_() > 1) {
      gai_error("Handle to a non-mirrored array passed",0);
    } else {
      trans[0] = 'N';
      trans[1] = '\0';
      ngai_copy_patch(trans, g_a, alo, ahi, g_b, blo, bhi);
      return;
    }
  }

  if (ga_is_mirrored_(g_b) && ga_cluster_nnodes_())
    gai_error("Distributed array is mirrored",0);

  adim = GA[a_handle].ndim;
  bdim = GA[b_handle].ndim;

  p_handle = GA[a_handle].p_handle;

  if (adim != bdim)
    gai_error("Global arrays must have same dimension",0);

  type = GA[a_handle].type;
  if (type != GA[b_handle].type)
    gai_error("Global arrays must be of same type",0);

  for (i=0; i<adim; i++) {
    idim = (Integer)GA[a_handle].dims[i];
    if (alo[i] < 0 || alo[i] >= idim || ahi[i] < 0 || ahi[i] >= idim ||
        alo[i] > ahi[i])
      gai_error("Invalid patch index on mirrored GA",0);
  }
  for (i=0; i<bdim; i++) {
    idim = GA[b_handle].dims[i];
    if (blo[i] < 0 || blo[i] >= idim || bhi[i] < 0 || bhi[i] >= idim ||
        blo[i] > bhi[i])
      gai_error("Invalid patch index on distributed GA",0);
  }
  for (i=0; i<bdim; i++) {
    idim = (Integer)GA[b_handle].dims[i];
    if (ahi[i] - alo[i] != bhi[i] - blo[i])
      gai_error("Patch dimensions do not match for index ",i);
  }
  nga_zero_patch_(g_b, blo, bhi);

  /* Find coordinates of mirrored array patch that I own */
  i = PGRP_LIST[p_handle].map_proc_list[GAme];
  nga_distribution_(g_a, &i, mlo, mhi);
  /* Check to see if mirrored array patch intersects my portion of
     mirrored array */
  intersect = 1;
  for (i=0; i<adim; i++) {
    if (mhi[i] < alo[i]) intersect = 0;
    if (mlo[i] > ahi[i]) intersect = 0;
  }
  if (intersect) {
    /* get portion of mirrored array patch that actually resides on this
       processor */
    for (i=0; i<adim; i++) {
      mlo[i] = GA_MAX(alo[i],mlo[i]);
      mhi[i] = GA_MIN(ahi[i],mhi[i]);
    }

    /* get pointer to locally held distribution */
    nga_access_ptr(g_a, mlo, mhi, &src_data_ptr, mld);

    /* find indices in distributed array corresponding to this patch */
    for (i=0; i<adim; i++) {
      dlo[i] = blo[i] + mlo[i]-alo[i];
      dhi[i] = blo[i] + mhi[i]-alo[i];
    }

    /* perform accumulate */
    if (type == C_DBL) {
      d_one = 1.0;
      one = &d_one;
    } else if (type == C_DCPL) {
      z_one[0] = 1.0;
      z_one[1] = 0.0;
      one = &z_one;
    } else if (type == C_SCPL) {
      c_one[0] = 1.0;
      c_one[1] = 0.0;
      one = &c_one;
    } else if (type == C_FLOAT) {
      f_one = 1.0;
      one = &f_one;
    } else if (type == C_INT) {
      i_one = 1;
      one = &i_one;
    } else if (type == C_LONG) {
      l_one = 1;
      one = &l_one;
    } else {
      gai_error("Type not supported",type);
    }
    nga_acc_(g_b, dlo, dhi, src_data_ptr, mld, one);
  }
  if (local_sync_end) ga_sync_();
  GA_POP_NAME;
}

/*\ get number of distinct patches corresponding to a contiguous shared
 *  memory segment
\*/
Integer FATR ga_num_mirrored_seg_(Integer *g_a)
{
  Integer handle = *g_a + GA_OFFSET;
  Integer i, j, ndim, map_offset[MAXDIM];
  int *nblock;
  C_Integer *first, *last;
  Integer lower[MAXDIM], upper[MAXDIM];
  Integer istart, nproc, inode;
  Integer ret = 0, icheck;

  if (!ga_is_mirrored_(g_a)) return ret;
  GA_PUSH_NAME("ga_num_mirrored_seg");
  ndim = GA[handle].ndim;
  first = GA[handle].first;
  last = GA[handle].last;
  nblock = GA[handle].nblock;
  for (i=0; i<ndim; i++) {
    map_offset[i] = 0;
    for (j=0; j<i; j++) {
      map_offset[i] += nblock[j];
    }
  }
  inode = ga_cluster_nodeid_();
  nproc = ga_cluster_nprocs_(&inode);
  /* loop over all data blocks on this node to find out how many
   * separate data blocks correspond to this segment of shared
   * memory */
  istart = 0;
  for (i=0; i<nproc; i++) {
    nga_distribution_(g_a,&i,lower,upper);
    icheck = 0;
    /* see if processor corresponds to block of array data
     * that contains start of shared memory segment */
    if (!istart) {
      icheck = 1;
      for (j=0; j<ndim; j++) {
        if (first[j] < (C_Integer)lower[j] || first[j] > (C_Integer)upper[j]) {
          icheck = 0;
          break;
        }
      }
    }
    if (icheck && !istart) {
      istart = 1;
    }
    icheck = 1;
    for (j=0; j<ndim; j++) {
      if (last[j] < (C_Integer)lower[j] || last[j] > (C_Integer)upper[j]) {
        icheck = 0;
        break;
      }
    }
    if (istart) ret++;
    if (istart && icheck) {
      GA_POP_NAME;
      return ret;
    }
  }
  GA_POP_NAME;
  return ret;
}

/*\ Get patch corresponding to one of the blocks of data
 *  identified using ga_num_mirrored_seg_
\*/
void FATR ga_get_mirrored_block_(Integer *g_a,
                               Integer *npatch,
                               Integer *lo,
                               Integer *hi)
{
  Integer handle = *g_a + GA_OFFSET;
  Integer i, j, ndim, map_offset[MAXDIM];
  C_Integer *first, *last;
  int *nblock;
  Integer lower[MAXDIM], upper[MAXDIM];
  Integer istart, nproc, inode;
  Integer ipatch, tpatch, icheck;

  /* Assume fortran indexing for npatch */
  tpatch = *npatch - 1;

  if (!ga_is_mirrored_(g_a)) {
    for (j=0; j<GA[handle].ndim; j++) {
      lo[j] = 0;
      hi[j] = -1;
    }
    return;
  }
  GA_PUSH_NAME("ga_get_mirrored_block");
  ndim = GA[handle].ndim;
  first = GA[handle].first;
  last = GA[handle].last;
  nblock = GA[handle].nblock;
  for (i=0; i<ndim; i++) {
    map_offset[i] = 0;
    for (j=0; j<i; j++) {
      map_offset[i] += nblock[j];
    }
  }
  inode = ga_cluster_nodeid_();
  nproc = ga_cluster_nprocs_(&inode);
  /* loop over all data blocks on this node to find out how many
   * separate data blocks correspond to this segment of shared
   * memory */
  ipatch = 0;
  istart = 0;
  for (i=0; i<nproc; i++) {
    nga_distribution_(g_a,&i,lower,upper);
    icheck = 0;
    /* see if processor corresponds to block of array data
     * that contains start of shared memory segment */
    if (!istart) {
      icheck = 1;
      for (j=0; j<ndim; j++) {
        if (first[j] < (C_Integer)lower[j] || first[j] > (C_Integer)upper[j]) {
          icheck = 0;
          break;
        }
      }
    }
    if (icheck && !istart) {
      istart = 1;
    }
    icheck = 1;
    for (j=0; j<ndim; j++) {
      if (last[j] < (C_Integer)lower[j] || last[j] > (C_Integer)upper[j]) {
        icheck = 0;
        break;
      }
    }
    if (istart && ipatch == tpatch) {
      if (!icheck) {
        if (ipatch == 0) {
          for (j=0; j<ndim; j++) {
            lo[j] = (Integer)first[j];
            hi[j] = upper[j];
          }
        } else {
          for (j=0; j<ndim; j++) {
            lo[j] = lower[j];
            hi[j] = upper[j];
          }
        }
      } else {
        if (ipatch == 0) {
          for (j=0; j<ndim; j++) {
            lo[j] = (Integer)first[j];
            hi[j] = (Integer)last[j];
          }
        } else {
          for (j=0; j<ndim; j++) {
            lo[j] = lower[j];
            hi[j] = (Integer)last[j];
          }
        }
      }
      GA_POP_NAME;
      return;
    }
    if (istart) ipatch++;
  }
  for (j=0; j<ndim; j++) {
    lo[j] = 0;
    hi[j] = -1;
  }
  GA_POP_NAME;
  return;
}

/*\ do a fast merge of all copies of a mirrored array only passing
 *  around non-zero data
\*/
void FATR ga_fast_merge_mirrored_(Integer *g_a)
{
  Integer handle = GA_OFFSET + *g_a;
  Integer inode, new_inode, nprocs, nnodes, new_nnodes, zero, zproc;
  C_Integer *map, *dims, *width;
  int *blocks;
  Integer i, j, index[MAXDIM], itmp, ndim;
  Integer nelem, count, type;
  int slength, rlength, nsize;
  char  *bptr, *nptr, *fptr;
  Integer bytes;
  Integer ilast,inext,id;
  int Save_default_group;
  int local_sync_begin, local_sync_end;

  /* declarations for message exchanges */
  int next_node,next;
  int armci_tag = 88000;
  char *dstn,*srcp;
  int next_nodel=0;
  int dummy=1, LnB, powof2nodes;
  int groupA, groupB, sizeB;
  void armci_util_wait_int(volatile int *,int,int);

  local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
  _ga_sync_begin = 1; _ga_sync_end = 1; /*remove any previous masking */
  if (local_sync_begin) ga_sync_();
  /* don't perform update if node is not mirrored */
  if (!ga_is_mirrored_(g_a)) return;
  
  /* If default group is not world group, change default group to world group
     temporarily */
  Save_default_group = GA_Default_Proc_Group;
  GA_Default_Proc_Group = -1;

  GA_PUSH_NAME("ga_fast_merge_mirrored");

  inode = ga_cluster_nodeid_();
  /* BJP printf("p[%d] inode: %d\n",GAme,inode); */
  nnodes = ga_cluster_nnodes_(); 
  nprocs = ga_cluster_nprocs_(&inode);
  zero = 0;

  powof2nodes=1;
  LnB = floor(log(nnodes)/log(2))+1;
  if(pow(2,LnB-1)<nnodes){powof2nodes=0;}
  /* Partition nodes into groups A and B. Group A contains a power of 2
   * nodes, group B contains the remainder */
  if (powof2nodes) {
    groupA = 1;
    groupB = 0;
    sizeB = 0;
    new_nnodes = nnodes;
  } else {  
    new_nnodes = pow(2,LnB-1);
    sizeB = nnodes-new_nnodes;
    if (inode<2*sizeB) {
      if (inode%2 == 0) {
        groupA = 1;
        groupB = 0;
      } else {
        groupA = 0;
        groupB = 1;
      }
    } else {
      groupA = 1;
      groupB = 0;
    }
  }
  /*if (groupA) printf("p[%d] Group A\n",GAme);
  if (groupB) printf("p[%d] Group B\n",GAme);*/

  zproc = ga_cluster_procid_(&inode, &zero);
  map = GA[handle].mapc;
  blocks = GA[handle].nblock;
  dims = GA[handle].dims;
  width = GA[handle].width;
  type = GA[handle].type;
  ndim = GA[handle].ndim;

  /* Check whether or not all nodes contain the same number
     of processors. */
  if (nnodes*nprocs == ga_nnodes_())  {
    /* check to see if there is any buffer space between the data
       associated with each processor that needs to be zeroed out
       before performing the merge */
    if (zproc == GAme) {
      nsize = 0;
      for (i=0; i<nprocs; i++) {
        /* Find out from mapc data how many elements are supposed to be located
           on this processor. Start by converting processor number to indices */
        itmp = i;
        for (j=0; j<ndim; j++) {
          index[j] = itmp%(Integer)blocks[j];
          itmp = (itmp - index[j])/(Integer)blocks[j];
        }

        nelem = 1;
        count = 0;
        for (j=0; j<ndim; j++) {
          if (index[j] < (Integer)blocks[j]-1) {
            nelem *= (Integer)(map[index[j]+1+count] - map[index[j]+count]
                   + 2*width[j]);
          } else {
            nelem *= (Integer)(dims[j] - map[index[j]+count] + 1 + 2*width[j]);
          }
          count += (Integer)blocks[j];
        }
        /* We now have the total number of elements located on this processor.
           Find out if the location of the end of this data set matches the
           origin of the data on the next processor. If not, then zero data in
           the gap. */
        nelem *= GAsizeof(type);
        nsize += (int)nelem;
        bptr = GA[handle].ptr[ga_cluster_procid_(&inode, &i)];
        bptr += nelem;
        if (i<nprocs-1) {
          j = i+1;
          nptr = GA[handle].ptr[ga_cluster_procid_(&inode, &j)];
          if (bptr != nptr) {
            bytes = (long)nptr - (long)bptr;
            nsize += (int)bytes;
            bzero(bptr, bytes);
          }
        }
      }
      /* The gaps have now been zeroed out. Begin exchange of data */
      /* This algorith is based on the armci_msg_barrier code */
      /* Locate pointers to beginning and end of non-zero data */
      for (i=0;i<ndim;i++) index[i] = (Integer)GA[handle].first[i];
      i = nga_locate_(g_a, index, &id);
      gam_Loc_ptr(id, handle, GA[handle].first, &fptr);
      for (i=0;i<ndim;i++) index[i] = (Integer)GA[handle].last[i];
      slength = (int)GA[handle].shm_length;
      if(nnodes>1){
        if(!powof2nodes && inode < 2*sizeB && groupA) {
          ilast = inode + 1;
          next_nodel = ga_cluster_procid_(&ilast, &zero);
        } else if (groupB) {
          ilast = inode - 1;
          next_nodel = ga_cluster_procid_(&ilast, &zero);
        }
        ilast = ((int)pow(2,(LnB-1)))^inode;
        /*printf("p[%d] Value of next nodel: %d\n",GAme,next_nodel);*/
        /*three step exchange if num of nodes is not pow of 2*/
        /*divide _nodes_ into two sets, first set "pow2" will have a power of 
         *two nodes, the second set "not-pow2" will have the remaining.
         *Each node in the not-pow2 set will have a pair node in the pow2 set.
         *Step-1:each node in pow2 set with a pair in not-pow2 set first recvs 
         *      :a message from its pair in not-pow2. 
         *step-2:All nodes in pow2 do a Rercusive Doubling based Pairwise exng.
         *step-3:Each node in pow2 with a pair in not-pow2 snds msg to its 
         *      :pair node.
         *if num of nodes a pow of 2, only step 2 executed
         */
        if(/*ilast>inode &&*/ groupA){ /*the pow2 set of procs*/
          /* Use actual index of processor you are recieving from in group B
           * and perform first exchange (for non-power of 2) */
          if(!powof2nodes && inode < 2*sizeB){ /*step 1*/
            dstn = (char *)&rlength;
            armci_msg_rcv(armci_tag, dstn,4,NULL,next_nodel);
            if (GAme > next_nodel) {
              dstn = fptr - rlength;
            } else {
              dstn = fptr + slength;
            }
            armci_msg_rcv(armci_tag, dstn,rlength,NULL,next_nodel);
            if (GAme > next_nodel)
              fptr -= rlength;
            slength += rlength;
          }
          /* Assign inode = new_inode */
          if (inode < 2*sizeB) {
            new_inode  = inode/2;
          } else {
            new_inode  = inode - sizeB;
          }
          /*LnB=1;*/ /*BJP*/
          for(i=0;i<LnB-1;i++){ /*step 2*/
            next=((int)pow(2,i))^new_inode;
            if(next>=0 && next<new_nnodes){
              /* Translate back from relative_next_node to actual_next_node */
              if (next < sizeB)
                inext = (Integer)2*next;
              else
                inext = (Integer)(next+sizeB);
              next_node = ga_cluster_procid_(&inext, &zero);
              srcp = (char *)&slength;
              dstn = (char *)&rlength;
              if(next_node > GAme){
                armci_msg_snd(armci_tag, srcp,4,next_node);
                armci_msg_rcv(armci_tag, dstn,4,NULL,next_node);
              }
              else{
                /*would we gain anything by doing a snd,rcv instead of rcv,snd*/
                armci_msg_rcv(armci_tag, dstn,4,NULL,next_node);
                armci_msg_snd(armci_tag, srcp,4,next_node);
              }
              srcp = fptr;
              if (GAme > next_node) {
                dstn = fptr - rlength;
              } else {
                dstn = fptr + slength;
              }
              /* Translate back from relative_next_node to actual_next_node */
              if(next_node > GAme){
                armci_msg_snd(armci_tag, srcp,slength,next_node);
                armci_msg_rcv(armci_tag, dstn,rlength,NULL,next_node);
              }
              else{
                /*would we gain anything by doing a snd,rcv instead of rcv,snd*/
                armci_msg_rcv(armci_tag, dstn,rlength,NULL,next_node);
                armci_msg_snd(armci_tag, srcp,slength,next_node);
              }
              if (GAme > next_node)
                fptr -= rlength;
              slength += rlength;
            }
          }
              /* Use actual index of processor that you already recieved from
               * and that you will be sending to in group B*/
          if(!powof2nodes && inode < 2*sizeB){ /*step 3*/
            srcp = GA[handle].ptr[GAme];
            armci_msg_snd(armci_tag, srcp,nsize,next_nodel);
          }
        }
        else if (groupB) {
          /* Send data from group B to group A and then wait to
           * recieve data from group A to group B */
          if(!powof2nodes){
            /* printf("p[%d] Sending (1) data to %d\n",GAme,next_nodel); */
            srcp = (char *)&slength;
            armci_msg_snd(armci_tag, srcp,4,next_nodel);
            srcp = fptr;
            armci_msg_snd(armci_tag, srcp,slength,next_nodel);
            dstn = GA[handle].ptr[GAme];
            rlength = nsize;
            armci_msg_rcv(armci_tag, dstn,rlength,NULL,next_nodel);
            /*printf("p[%d] Recieved (2) data from %d\n",GAme,next_nodel);*/
          }
        }
      }
      /*printf("p[%d] About to execute armci_msg_gop_scope\n",GAme);*/
      armci_msg_gop_scope(SCOPE_NODE,&dummy,1,"+",ARMCI_INT);
    } else {
      /*printf("p[%d] About to execute armci_msg_gop_scope\n",GAme);*/
      armci_msg_gop_scope(SCOPE_NODE,&dummy,1,"+",ARMCI_INT);
    }
    /*printf("p[%d] Executed armci_msg_gop_scope\n",GAme);*/
  } else {
    ga_merge_mirrored_(g_a);
  }

  GA_Default_Proc_Group = Save_default_group;
  if (local_sync_end) ga_sync_();
  GA_POP_NAME;
}

/*\ RETURN THE TOTAL NUMBER OF BLOCKS IN REGION (IF ANY)
\*/
Integer FATR nga_locate_num_blocks_(Integer *g_a, Integer *lo, Integer *hi)
{
  Integer ga_handle = GA_OFFSET + *g_a;
  Integer ndim = GA[ga_handle].ndim;
  Integer ret = -1, d;
  Integer cnt;

  GA_PUSH_NAME("nga_locate_num_blocks");
  for(d = 0; d< GA[ga_handle].ndim; d++)
    if((lo[d]<1 || hi[d]>GA[ga_handle].dims[d]) ||(lo[d]>hi[d]))
      gai_error("Requested region out of bounds",0);

  if (GA[ga_handle].block_flag) {
    Integer nblocks = GA[ga_handle].block_total;
    Integer chk, i, j, tlo[MAXDIM], thi[MAXDIM];
    cnt = 0;
    for (i=0; i<nblocks; i++) {
      /* check to see if this block overlaps with requested block
       * defined by lo and hi */
      chk = 1;
      /* get limits on block i */
      nga_distribution_(g_a,&i,tlo,thi);
      for (j=0; j<ndim && chk==1; j++) {
        /* check to see if at least one end point of the interval
         * represented by blo and bhi falls in the interval
         * represented by lo and hi */
        if (!((tlo[j] >= lo[j] && tlo[j] <= hi[j]) ||
              (thi[j] >= lo[j] && thi[j] <= hi[j]))) {
          chk = 0;
        }
      }

      if (chk) {
        cnt++;
      }
    }
    ret = cnt;
  }

  GA_POP_NAME;
  return ret;
}

/*\ RETURN THE TOTAL NUMBER OF BLOCKS IN REGION (IF ANY)
\*/
Integer FATR ga_total_blocks_(Integer *g_a)
{
  Integer ga_handle = GA_OFFSET + *g_a;
  return GA[ga_handle].block_total;
}

/*\ RETURN TRUE IF GA USES SCALAPACK DATA DISTRIBUTION
\*/
logical FATR ga_uses_proc_grid_(Integer *g_a)
{
  Integer ga_handle = GA_OFFSET + *g_a;
  return (logical)GA[ga_handle].block_sl_flag;
}

/*\ RETURN THE INDEX OF PROCESSOR BASED ON THE BLOCK PARTITION ASSOCIATED
 *  WITH A PARTICULAR GLOBAL ARRAY
\*/
void FATR ga_get_proc_index_(Integer *g_a, Integer *iproc, Integer *index)
{
  Integer ga_handle = GA_OFFSET + *g_a;
  Integer proc = *iproc;
  if (!GA[ga_handle].block_sl_flag)
    gai_error("Global array does not use ScaLAPACK data distribution",0);
  gam_find_proc_indices(ga_handle, proc, index);
  return;
}

/*\ RETURN PROC GRID DIMENSIONS AND BLOCK DIMENSIONS FOR A PARTICULAR
 *  GLOBAL ARRAY
\*/
void FATR ga_get_block_info_(Integer *g_a, Integer *num_blocks, Integer *block_dims)
{
  Integer ga_handle = GA_OFFSET + *g_a;
  Integer i, ndim;
  ndim = GA[ga_handle].ndim; 
  if (GA[ga_handle].block_sl_flag) {
    for (i=0; i<ndim; i++) {
      num_blocks[i] = GA[ga_handle].num_blocks[i];
      block_dims[i] = GA[ga_handle].block_dims[i];
    }
  } else {
    Integer dim, bsize;
    for (i=0; i<ndim; i++) {
      dim = GA[ga_handle].dims[i];
      bsize = GA[ga_handle].block_dims[i];
      if (bsize > 0) {
        if (dim%bsize == 0) {
          num_blocks[i] = dim/bsize;
        } else {
          num_blocks[i] = dim/bsize+1;
        }
      } else {
        num_blocks[i] = 0;
      }
      block_dims[i] = GA[ga_handle].block_dims[i];
    }
  }
  return;
}

void FATR ga_set_debug_(logical *flag)
{
  GA_Debug_flag = (Integer)(*flag);
}

logical FATR ga_get_debug_()
{
  return (logical)GA_Debug_flag;
}

#ifdef ENABLE_CHECKPOINT
void FATR ga_checkpoint_arrays_(Integer *gas,int *num)
{
   int ga = *(gas+0);
   int hdl = GA_OFFSET + ga;
   printf("\n%d:in checkpoint %d %d %d\n",GAme,ga,*(gas+1),*num);fflush(stdout);
   if(GA[hdl].record_id==0)
     ga_icheckpoint_init(gas,*num);
   ga_icheckpoint(gas,*num);
}


int ga_recover_arrays(Integer *gas, int num)
{
    int i;
    for(i=0;i<num;i++){
       int ga = *(gas+i);
       int hdl = GA_OFFSET + ga;
       if(GA[hdl].record_id!=0)
         ga_irecover(ga);
    }
}
#endif

Integer FATR ga_pgroup_absolute_id_(Integer *grp, Integer *pid) 
{
#ifdef MPI
  if(*grp == GA_World_Proc_Group) /*a.k.a -1*/
    return *pid;
  else
    return ARMCI_Absolute_id(&PGRP_LIST[*grp].group, *pid);
#else
  gai_error("ga_pgroup_absolute_id(): Defined only when using MPI groups",0);
  return -1;
#endif
}
