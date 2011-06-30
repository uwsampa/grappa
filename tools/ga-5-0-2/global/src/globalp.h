#ifndef _GLOBALP_H_
#define _GLOBALP_H_

#if HAVE_CONFIG_H
#  include "config.h"
#endif

#if HAVE_STDIO_H
#   include <stdio.h>
#endif

#include "gaconfig.h"
#include "global.h"

#ifdef __crayx1
#undef CRAY
#endif

#ifdef FALSE
#undef FALSE
#endif
#ifdef TRUE
#undef TRUE
#endif
#ifdef CRAY_YMP
#define FALSE _btol(0)
#define TRUE  _btol(1)
#else
#define FALSE (logical) 0
#define TRUE  (logical) 1
#endif

#if HAVE_WINDOWS_H
#   include <windows.h>
#   define sleep(x) Sleep(1000*(x))
#endif
#include "macdecls.h"

#define GA_OFFSET   1000           /* offset for handle numbering */

/* types/tags of messages used internally by GA */
#define     GA_TYPE_SYN   GA_MSG_OFFSET + 1
#define     GA_TYPE_GSM   GA_MSG_OFFSET + 5
#define     GA_TYPE_GOP   GA_MSG_OFFSET + 15
#define     GA_TYPE_BRD   GA_MSG_OFFSET + 16

/* GA operation ids */
#define     GA_OP_GET 1          /* Get                         */
#define     GA_OP_END 2          /* Terminate                   */
#define     GA_OP_CRE 3          /* Create                      */
#define     GA_OP_PUT 4          /* Put                         */
#define     GA_OP_ACC 5          /* Accumulate                  */
#define     GA_OP_DES 6          /* Destroy                     */
#define     GA_OP_DUP 7          /* Duplicate                   */
#define     GA_OP_ZER 8          /* Zero                        */
#define     GA_OP_DDT 9          /* dot product                 */
#define     GA_OP_SCT 10         /* scatter                     */
#define     GA_OP_GAT 11         /* gather                      */
#define     GA_OP_RDI 15         /* Integer read and increment  */
#define     GA_OP_ACK 16         /* acknowledgment              */
#define     GA_OP_LCK 17         /* acquire lock                */
#define     GA_OP_UNL 18         /* release lock                */


#ifdef ENABLE_TRACE
  static Integer     op_code;
#endif


#define GA_MAX(a,b) (((a) >= (b)) ? (a) : (b))
#define GA_MIN(a,b) (((a) <= (b)) ? (a) : (b))
#define GA_ABS(a)   (((a) >= 0) ? (a) : (-(a)))

#define GAsizeofM(type)  ( (type)==C_DBL? sizeof(double): \
                           (type)==C_INT? sizeof(int): \
                           (type)==C_DCPL? sizeof(DoubleComplex): \
                           (type)==C_SCPL? sizeof(SingleComplex): \
                           (type)==C_LONG? sizeof(long): \
                           (type)==C_LONGLONG? sizeof(long long): \
                           (type)==C_FLOAT? sizeof(float):0)

#define NAME_STACK_LEN 10
#define PAGE_SIZE  4096

struct ga_stat_t {
         long   numcre; 
         long   numdes;
         long   numget;
         long   numput;
         long   numacc;
         long   numsca;
         long   numgat;
         long   numrdi;
         long   numser;
         long   curmem; 
         long   maxmem; 
         long   numget_procs;
         long   numput_procs;
         long   numacc_procs;
         long   numsca_procs;
         long   numgat_procs;
};

struct ga_bytes_t{ 
         double acctot;
         double accloc;
         double gettot;
         double getloc;
         double puttot;
         double putloc;
         double rditot;
         double rdiloc;
         double gattot;
         double gatloc;
         double scatot;
         double scaloc;
};

#define STAT_AR_SZ sizeof(ga_stat_t)/sizeof(long)

extern long *GAstat_arr;  
extern struct ga_stat_t GAstat;
extern struct ga_bytes_t GAbytes;
extern char *GA_name_stack[NAME_STACK_LEN];    /* stack for names of GA ops */ 
extern int GA_stack_size;
extern int _ga_sync_begin;
extern int _ga_sync_end;
extern int *_ga_argc;
extern char ***_ga_argv;

#define  GA_PUSH_NAME(name) (GA_name_stack[GA_stack_size++] = (name)) 
#define  GA_POP_NAME        (GA_stack_size--)

/* periodic operations */
#define PERIODIC_GET 1
#define PERIODIC_PUT 2
#define PERIODIC_ACC 3

#define FLUSH_CACHE
#ifdef  CRAY_T3D
#       define ALLIGN_SIZE      32
#else
#       define ALLIGN_SIZE      128
#endif

#define allign__(n, SIZE) (((n)%SIZE) ? (n)+SIZE - (n)%SIZE: (n))
#define allign_size(n) allign__((long)(n), ALLIGN_SIZE)
#define allign_page(n) allign__((long)(n), PAGE_SIZE)

/* for brevity */
#define EXT  extern
#define DBL  DoublePrecision
#define FLT  float
#define INT  Integer
#define LOG  Logical
#define SCPL SingleComplex
#define DCPL DoubleComplex
#define REAL Real

EXT INT   GAsizeof(INT type);

EXT void  ga_clean_resources( void);
EXT void  ga_free(void *ptr);
EXT void  ga_init_nbhandle(INT *nbhandle);
EXT void* ga_malloc(INT nelem, int type, char *name);
EXT void  ga_matmul(char *transa, char *transb, void *alpha, void *beta, INT *g_a, INT *ailo, INT *aihi, INT *ajlo, INT *ajhi, INT *g_b, INT *bilo, INT *bihi, INT *bjlo, INT *bjhi, INT *g_c, INT *cilo, INT *cihi, INT *cjlo, INT *cjhi);
EXT void  ga_matmul_mirrored(char *transa, char *transb, void *alpha, void *beta, INT *g_a, INT *ailo, INT *aihi, INT *ajlo, INT *ajhi, INT *g_b, INT *bilo, INT *bihi, INT *bjlo, INT *bjhi, INT *g_c, INT *cilo, INT *cihi, INT *cjlo, INT *cjhi);
EXT void  ga_msg_brdcst(INT type, void *buffer, INT len, INT root);
EXT void  ga_print_file(FILE *, INT *);
EXT int   ga_type_c2f(int type);
EXT int   ga_type_f2c(int type);
EXT short ga_usesMA;

EXT void  gai_gop(INT type, void *x, INT n, char *op);

EXT void  gac_igop(int *x, Integer n, char *op);
EXT void  gac_lgop(long *x, Integer n, char *op);
EXT void  gac_llgop(long long *x, Integer n, char *op);
EXT void  gac_fgop(float *x, Integer n, char *op);
EXT void  gac_dgop(double *x, Integer n, char *op);
EXT void  gac_cgop(SingleComplex *x, Integer n, char *op);
EXT void  gac_zgop(DoubleComplex *x, Integer n, char *op);

EXT void  gai_igop(INT type, INT  *x, INT n, char *op);
EXT void  gai_sgop(INT type, REAL *x, INT n, char *op);
EXT void  gai_dgop(INT type, DBL  *x, INT n, char *op);
EXT void  gai_cgop(INT type, SCPL *x, INT n, char *op);
EXT void  gai_zgop(INT type, DCPL *x, INT n, char *op);

EXT void  gai_pgroup_gop(INT p_grp, INT type, void *x, INT n, char *op);

EXT void  gai_pgroup_igop(INT p_grp, INT type, INT  *x, INT n, char *op);
EXT void  gai_pgroup_sgop(INT p_grp, INT type, REAL *x, INT n, char *op);
EXT void  gai_pgroup_dgop(INT p_grp, INT type, DBL  *x, INT n, char *op);
EXT void  gai_pgroup_cgop(INT p_grp, INT type, SCPL *x, INT n, char *op);
EXT void  gai_pgroup_zgop(INT p_grp, INT type, DCPL *x, INT n, char *op);

EXT void  gai_check_handle(INT *, char *);
EXT void  gai_copy_patch(char *trans, INT *g_a, INT *ailo, INT *aihi, INT *ajlo, INT *ajhi, INT *g_b, INT *bilo, INT *bihi, INT *bjlo, INT *bjhi);
EXT LOG   gai_create(INT *type, INT *dim1, INT *dim2, char *array_name, INT *chunk1, INT *chunk2, INT *g_a);
EXT LOG   gai_create_irreg(INT *type, INT *dim1, INT *dim2, char *array_name, INT *map1, INT *nblock1, INT *map2, INT *nblock2, INT *g_a);
EXT void  gai_dot(int Type, INT *g_a, INT *g_b, void *value);
EXT LOG   gai_duplicate(INT *g_a, INT *g_b, char* array_name);
EXT void  gai_error(char *string, INT icode);
EXT int   gai_getval(int *ptr);
EXT void  gai_inquire(INT* g_a, INT* type, INT* dim1, INT* dim2);
EXT void  gai_inquire_name(INT *g_a, char **array_name);
EXT void  gai_lu_solve_seq(char *trans, INT *g_a, INT *g_b);
EXT void  gai_matmul_patch(char *transa, char *transb, void *alpha, void *beta, INT *g_a,INT *ailo,INT *aihi,INT *ajlo,INT *ajhi, INT *g_b,INT *bilo,INT *bihi,INT *bjlo,INT *bjhi, INT *g_c,INT *cilo,INT *cihi,INT *cjlo,INT *cjhi);
EXT INT   gai_memory_avail(INT datatype);
EXT void  gai_print_distribution(int fstyle, INT g_a);
EXT void  gai_print_subscript(char *pre,int ndim, INT subscript[], char* post);
EXT void  gai_set_array_name(INT g_a, char *array_name);

EXT void  nga_acc_common(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld, void *alpha, INT *nbhandle);
EXT void  nga_access_block_grid_ptr(INT* g_a, INT *index, void* ptr, INT *ld);
EXT void  nga_access_block_ptr(INT* g_a, INT *idx, void* ptr, INT *ld);
EXT void  nga_access_block_segment_ptr(INT* g_a, INT *proc, void* ptr, INT *len);
EXT void  nga_access_ghost_ptr(INT* g_a, INT dims[], void* ptr, INT ld[]);
EXT void  nga_access_ghost_element_ptr(INT* g_a, void *ptr, INT subscript[], INT ld[]);
EXT void  nga_access_ptr(INT* g_a, INT lo[], INT hi[], void* ptr, INT ld[]);
EXT void  nga_get_common(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld, INT *nbhandle);
EXT void  nga_put_common(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld, INT *nbhandle);
EXT int   nga_test_internal(INT *nbhandle);
EXT int   nga_wait_internal(INT *nbhandle);

EXT LOG   ngai_comp_patch(INT andim, INT *alo, INT *ahi, INT bndim, INT *blo, INT *bhi);
EXT void  ngai_copy_patch(char *trans, INT *g_a, INT *alo, INT *ahi, INT *g_b, INT *blo, INT *bhi);
EXT LOG   ngai_create_config(INT type, INT ndim, INT dims[], char* array_name, INT chunk[], INT p_handle, INT *g_a);
EXT LOG   ngai_create_ghosts_config(INT type, INT ndim, INT dims[], INT width[], char* array_name, INT chunk[], INT p_handle, INT *g_a);
EXT LOG   ngai_create_ghosts(INT type, INT ndim, INT dims[], INT width[], char* array_name, INT chunk[], INT *g_a);
EXT LOG   ngai_create_ghosts_irreg_config(INT type, INT ndim, INT dims[], INT width[], char *array_name, INT map[], INT nblock[], INT p_handle, INT *g_a);
EXT LOG   ngai_create_ghosts_irreg(INT type, INT ndim, INT dims[], INT width[], char *array_name, INT map[], INT nblock[], INT *g_a);
EXT LOG   ngai_create(INT type, INT ndim, INT dims[], char* array_name, INT *chunk, INT *g_a);
EXT LOG   ngai_create_irreg_config(INT type, INT ndim, INT dims[], char *array_name, INT map[], INT nblock[], INT p_handle, INT *g_a);
EXT LOG   ngai_create_irreg(INT type, INT ndim, INT dims[], char *array_name, INT map[], INT nblock[], INT *g_a);
EXT void  ngai_dest_indices(INT ndims, INT *los, INT *blos, INT *dimss, INT ndimd, INT *lod, INT *blod, INT *dimsd);
EXT void  ngai_dot_patch(INT *g_a, char *t_a, INT *alo, INT *ahi, INT *g_b, char *t_b, INT *blo, INT *bhi, void *retval);
EXT void  ngai_inquire(INT *g_a, INT *type, INT *ndim, INT *dims);
EXT void  ngai_matmul_patch(char *transa, char *transb, void *alpha, void *beta, INT *g_a, INT alo[], INT ahi[], INT *g_b, INT blo[], INT bhi[], INT *g_c, INT clo[], INT chi[]);
EXT LOG   ngai_patch_intersect(INT *lo, INT *hi, INT *lop, INT *hip, INT ndim);
EXT void  ngai_periodic_(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld, void *alpha, INT op_code);
EXT LOG   ngai_test_shape(INT *alo, INT *ahi, INT *blo, INT *bhi, INT andim, INT bndim);

EXT void  xb_dgemm (char *transa, char *transb, int *M, int *N, int *K, double *alpha,const double *a,int *p_lda,const double *b, int *p_ldb, double *beta, double *c, int *p_ldc);
EXT void  xb_sgemm (char *transa, char *transb, int *M, int *N, int *K, FLT *alpha, const FLT *a, int *p_lda,const FLT *b, int *p_ldb, FLT *beta, FLT *c, int *p_ldc);
EXT void  xb_zgemm (char * transa, char *transb, int *M, int *N, int *K, const void *alpha,const void *a,int *p_lda,const void *b, int *p_ldb, const void *beta, void *c, int *p_ldc);

EXT DBL   gai_ddot_patch(INT *g_a, char *t_a, INT *ailo, INT *aihi, INT *ajlo, INT *ajhi, INT *g_b, char *t_b, INT *bilo, INT *bihi, INT *bjlo, INT *bjhi);
EXT INT   gai_idot_patch(INT *g_a, char *t_a, INT *ailo, INT *aihi, INT *ajlo, INT *ajhi, INT *g_b, char *t_b, INT *bilo, INT *bihi, INT *bjlo, INT *bjhi);
EXT REAL  gai_sdot_patch(INT *g_a, char *t_a, INT *ailo, INT *aihi, INT *ajlo, INT *ajhi, INT *g_b, char *t_b, INT *bilo, INT *bihi, INT *bjlo, INT *bjhi);

EXT DBL   ngai_ddot_patch(INT *g_a, char *t_a, INT *alo, INT *ahi, INT *g_b, char *t_b, INT *blo, INT *bhi);
EXT INT   ngai_idot_patch(INT *g_a, char *t_a, INT *alo, INT *ahi, INT *g_b, char *t_b, INT *blo, INT *bhi);
EXT REAL  ngai_sdot_patch(INT *g_a, char *t_a, INT *alo, INT *ahi, INT *g_b, char *t_b, INT *blo, INT *bhi);

#undef EXT
#undef DBL
#undef FLT
#undef INT
#undef LOG
#undef SCPL
#undef DCPL
#undef REAL

#endif /* _GLOBALP_H_ */
