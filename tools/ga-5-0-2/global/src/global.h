/** @file global.h
 *
 * This is a private header file which defines all Fortran functions.
 * The names of these functions should mirror those found in global.h.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include "typesf2c.h"
#include "c.names.h"

#if SIZEOF_VOIDP==8
typedef long AccessIndex;
#else
typedef int AccessIndex;
#endif
  
/* for brevity */
#define EXT  extern
#define DCPL DoubleComplex
#define SCPL SingleComplex
#define DBL  DoublePrecision
#define REAL Real
#define FLT  float
#define INT  Integer
#define ACCESS_IDX AccessIndex
#define LOG  Logical

#ifdef __cplusplus
extern "C" {
#endif

#if F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS

EXT void FATR ga_cgemm_(char *transa, char *transb, INT *m, INT *n, INT *k, void *alpha, INT *g_a, INT *g_b, void *beta, INT *g_c, int talen, int tblen);
EXT void FATR ga_copy_patch_(char *trans, INT *g_a, INT *ailo, INT *aihi, INT *ajlo, INT *ajhi, INT *g_b, INT *bilo, INT *bihi, INT *bjlo, INT *bjhi, int len);
EXT LOG  FATR ga_create_(INT *type, INT *dim1, INT *dim2, char *array_name, INT *chunk1, INT *chunk2, INT *g_a, int slen);
EXT LOG  FATR ga_create_irreg_(INT *type, INT *dim1, INT *dim2, char *array_name, INT *map1, INT *nblock1, INT *map2, INT *nblock2, INT *g_a, int slen);
EXT DBL  FATR ga_ddot_patch_(INT *, char*, INT *, INT *, INT *, INT *, INT *, char*, INT *, INT *, INT *, INT *, int, int);
EXT void FATR ga_dgemm_(char *transa, char *transb, INT *m, INT *n, INT *k, void *alpha, INT *g_a, INT *g_b, void *beta, INT *g_c, int talen, int tblen);
EXT void FATR ga_error_(char *string, INT *icode, int slen);
EXT INT  FATR ga_idot_patch_(INT *, char*, INT *, INT *, INT *, INT *, INT *, char*, INT *, INT *, INT *, INT *, int, int);
EXT void FATR ga_lu_solve_seq_(char *trans, INT *g_a, INT *g_b, int len);
EXT void FATR ga_matmul_patch_(char *transa, char *transb, DoublePrecision *alpha, DoublePrecision *beta, INT *g_a, INT *ailo, INT *aihi, INT *ajlo, INT *ajhi, INT *g_b, INT *bilo, INT *bihi, INT *bjlo, INT *bjhi, INT *g_c, INT *cilo, INT *cihi, INT *cjlo, INT *cjhi, int alen, int blen);
EXT REAL FATR ga_sdot_patch_(INT *, char*, INT *, INT *, INT *, INT *, INT *, char*, INT *, INT *, INT *, INT *, int, int);
EXT void FATR ga_sgemm_(char *transa, char *transb, INT *m, INT *n, INT *k, void *alpha, INT *g_a, INT *g_b, void *beta, INT *g_c, int talen, int tblen);
EXT void FATR ga_zgemm_(char *transa, char *transb, INT *m, INT *n, INT *k, void *alpha, INT *g_a, INT *g_b, void *beta, INT *g_c, int talen, int tblen);
EXT void FATR gai_cdot_patch_(INT *, char*, INT *, INT *, INT *, INT *, INT *, char*, INT *, INT *, INT *, INT *, SCPL *ret, int, int);
EXT void FATR gai_zdot_patch_(INT *, char*, INT *, INT *, INT *, INT *, INT *, char*, INT *, INT *, INT *, INT *, DCPL *ret, int, int);
EXT void FATR nga_copy_patch_(char *trans, INT *g_a, INT *alo, INT *ahi, INT *g_b, INT *blo, INT *bhi, int len);
EXT LOG  FATR nga_create_(INT *type, INT *ndim, INT *dims, char* array_name, INT *chunk, INT *g_a, int slen);
EXT LOG  FATR nga_create_config_(INT *type, INT *ndim, INT *dims, char* array_name, INT *chunk, INT *p_handle, INT *g_a, int slen);
EXT LOG  FATR nga_create_ghosts_(INT *type, INT *ndim, INT *dims, INT *width, char* array_name, INT *chunk, INT *g_a, int slen);
EXT LOG  FATR nga_create_ghosts_config_(INT *type, INT *ndim, INT *dims, INT *width, char* array_name, INT *chunk, INT *p_handle, INT *g_a, int slen);
EXT LOG  FATR nga_create_ghosts_irreg_config_(INT *type, INT *ndim, INT *dims, INT width[], char* array_name, INT map[], INT block[], INT *p_handle, INT *g_a, int slen);
EXT LOG  FATR nga_create_ghosts_irreg_config_(INT *type, INT *ndim, INT *dims, INT width[], char* array_name, INT map[], INT block[], INT *p_handle, INT *g_a, int slen);
EXT LOG  FATR nga_create_irreg_(INT *type, INT *ndim, INT *dims, char* array_name, INT map[], INT block[], INT *g_a, int slen);
EXT LOG  FATR nga_create_irreg_config_(INT *type, INT *ndim, INT *dims, char* array_name, INT map[], INT block[], INT *p_handle, INT *g_a, int slen);
EXT DBL  FATR nga_ddot_patch_(INT *g_a, char *t_a, INT *alo, INT *ahi, INT *g_b, char *t_b, INT *blo, INT *bhi, int alen, int blen);
EXT INT  FATR nga_idot_patch_(INT *g_a, char *t_a, INT *alo, INT *ahi, INT *g_b, char *t_b, INT *blo, INT *bhi, int alen, int blen);
EXT void FATR nga_matmul_patch_(char *transa, char *transb, void *alpha, void *beta, INT *g_a, INT alo[], INT ahi[], INT *g_b, INT blo[], INT bhi[], INT *g_c, INT clo[], INT chi[], int alen, int blen);
EXT REAL FATR nga_sdot_patch_(INT *g_a, char *t_a, INT *alo, INT *ahi, INT *g_b, char *t_b, INT *blo, INT *bhi, int alen, int blen);
EXT void FATR nga_select_elem_(INT *g_a, char* op, void* val, INT *subscript, int len);
EXT void FATR ngai_cdot_patch_(INT *g_a, char *t_a, INT *alo, INT *ahi, INT *g_b, char *t_b, INT *blo, INT *bhi, SCPL* ret, int alen, int blen);
EXT void FATR ngai_zdot_patch_(INT *g_a, char *t_a, INT *alo, INT *ahi, INT *g_b, char *t_b, INT *blo, INT *bhi, DCPL *ret, int alen, int blen);

#else /* F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS */

EXT void FATR ga_cgemm_(char *transa, int talen, char *transb, int tblen, INT *m, INT *n, INT *k, void *alpha, INT *g_a, INT *g_b, void *beta, INT *g_c);
EXT void FATR ga_copy_patch_(char *trans, int len, INT *g_a, INT *ailo, INT *aihi, INT *ajlo, INT *ajhi, INT *g_b, INT *bilo, INT *bihi, INT *bjlo, INT *bjhi);
EXT LOG  FATR ga_create_(INT *type, INT *dim1, INT *dim2, char *array_name, int slen, INT *chunk1, INT *chunk2, INT *g_a);
EXT LOG  FATR ga_create_irreg_(INT *type, INT *dim1, INT *dim2, char *array_name, int slen, INT *map1, INT *nblock1, INT *map2, INT *nblock2, INT *g_a);
EXT DBL  FATR ga_ddot_patch_(INT *, char*, int, INT *, INT *, INT *, INT *, INT *, char*, int, INT *, INT *, INT *, INT *);
EXT void FATR ga_dgemm_(char *transa, int talen, char *transb, int tblen, INT *m, INT *n, INT *k, void *alpha, INT *g_a, INT *g_b, void *beta, INT *g_c);
EXT void FATR ga_error_(char *string, int slen, INT *icode);
EXT INT  FATR ga_idot_patch_(INT *, char*, int, INT *, INT *, INT *, INT *, INT *, char*, int, INT *, INT *, INT *, INT *);
EXT void FATR ga_lu_solve_seq_(char *trans, int len, INT *g_a, INT *g_b);
EXT void FATR ga_matmul_patch_(char *transa, int alen, char *transb, int blen, DoublePrecision *alpha, DoublePrecision *beta, INT *g_a, INT *ailo, INT *aihi, INT *ajlo, INT *ajhi, INT *g_b, INT *bilo, INT *bihi, INT *bjlo, INT *bjhi, INT *g_c, INT *cilo, INT *cihi, INT *cjlo, INT *cjhi);
EXT REAL FATR ga_sdot_patch_(INT *, char*, int, INT *, INT *, INT *, INT *, INT *, char*, int, INT *, INT *, INT *, INT *);
EXT void FATR ga_sgemm_(char *transa, int talen, char *transb, int tblen, INT *m, INT *n, INT *k, void *alpha, INT *g_a, INT *g_b, void *beta, INT *g_c);
EXT void FATR ga_zgemm_(char *transa, int talen, char *transb, int tblen, INT *m, INT *n, INT *k, void *alpha, INT *g_a, INT *g_b, void *beta, INT *g_c);
EXT void FATR gai_cdot_patch_(INT *, char*, int, INT *, INT *, INT *, INT *, INT *, char*, int, INT *, INT *, INT *, INT *, SCPL *ret);
EXT void FATR gai_zdot_patch_(INT *, char*, int, INT *, INT *, INT *, INT *, INT *, char*, int, INT *, INT *, INT *, INT *, DCPL *ret);
EXT void FATR nga_copy_patch_(char *trans, int len, INT *g_a, INT *alo, INT *ahi, INT *g_b, INT *blo, INT *bhi);
EXT LOG  FATR nga_create_(INT *type, INT *ndim, INT *dims, char* array_name, int slen, INT *chunk, INT *g_a);
EXT LOG  FATR nga_create_config_(INT *type, INT *ndim, INT *dims, char* array_name, int slen, INT *chunk, INT *p_handle, INT *g_a);
EXT LOG  FATR nga_create_ghosts_(INT *type, INT *ndim, INT *dims, INT *width, char* array_name, int slen, INT *chunk, INT *g_a);
EXT LOG  FATR nga_create_ghosts_config_(INT *type, INT *ndim, INT *dims, INT *width, char* array_name, int slen, INT *chunk, INT *p_handle, INT *g_a);
EXT LOG  FATR nga_create_ghosts_irreg_config_(INT *type, INT *ndim, INT *dims, INT width[], char* array_name, int slen, INT map[], INT block[], INT *p_handle, INT *g_a);
EXT LOG  FATR nga_create_ghosts_irreg_config_(INT *type, INT *ndim, INT *dims, INT width[], char* array_name, int slen, INT map[], INT block[], INT *p_handle, INT *g_a);
EXT LOG  FATR nga_create_irreg_(INT *type, INT *ndim, INT *dims, char* array_name, int slen, INT map[], INT block[], INT *g_a);
EXT LOG  FATR nga_create_irreg_config_(INT *type, INT *ndim, INT *dims, char* array_name, int slen, INT map[], INT block[], INT *p_handle, INT *g_a);
EXT DBL  FATR nga_ddot_patch_(INT *g_a, char *t_a, int alen, INT *alo, INT *ahi, INT *g_b, char *t_b, int blen, INT *blo, INT *bhi);
EXT INT  FATR nga_idot_patch_(INT *g_a, char *t_a, int alen, INT *alo, INT *ahi, INT *g_b, char *t_b, int blen, INT *blo, INT *bhi);
EXT void FATR nga_matmul_patch_(char *transa, int alen, char *transb, int blen, void *alpha, void *beta, INT *g_a, INT alo[], INT ahi[], INT *g_b, INT blo[], INT bhi[], INT *g_c, INT clo[], INT chi[]);
EXT REAL FATR nga_sdot_patch_(INT *g_a, char *t_a, int alen, INT *alo, INT *ahi, INT *g_b, char *t_b, int blen, INT *blo, INT *bhi);
EXT void FATR nga_select_elem_(INT *g_a, char* op, int len, void* val, INT *subscript);
EXT void FATR ngai_cdot_patch_(INT *g_a, char *t_a, int alen, INT *alo, INT *ahi, INT *g_b, char *t_b, int blen, INT *blo, INT *bhi, SCPL *ret);
EXT void FATR ngai_zdot_patch_(INT *g_a, char *t_a, int alen, INT *alo, INT *ahi, INT *g_b, char *t_b, int blen, INT *blo, INT *bhi, DCPL *ret);

#endif /* F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS */

EXT void FATR ga_abs_value_(INT *);
EXT void FATR ga_abs_value_patch_(INT *, INT *, INT *);
EXT void FATR ga_acc_(INT*, INT*, INT*, INT*, INT*, void*, INT*, void* );
EXT void FATR ga_access_(INT*, INT*, INT*, INT*, INT*, ACCESS_IDX*, INT* );
EXT void FATR ga_add_(void *, INT *, void *, INT *, INT *);
EXT void FATR ga_add_constant_(INT *g_a, void *);
EXT void FATR ga_add_constant_patch_(INT *, INT *, INT *, void *);
EXT void FATR ga_add_diagonal_(INT *g_a, INT *g_v);
EXT void FATR ga_add_patch_(void *, INT *, INT *, INT *, INT *, INT *, void *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *  );
EXT LOG  FATR ga_allocate_(INT *g_a);
EXT void FATR ga_brdcst_(INT*, void*, INT*, INT* );
EXT void FATR ga_cadd_(SCPL*, INT*, SCPL*, INT*, INT*);
EXT void FATR ga_cadd_patch_(SCPL *, INT *, INT *, INT *, INT *, INT *, SCPL *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *);
EXT void FATR ga_cfill_(INT *, SCPL *);
EXT void FATR ga_cfill_patch_(INT *, INT *, INT *, INT *, INT *, SCPL *);
EXT void FATR ga_cgop_(INT*, SCPL*, INT*, char*, int);
EXT void FATR ga_check_handle_(INT*, char*, int);
EXT INT  FATR ga_cluster_nnodes_( void);
EXT INT  FATR ga_cluster_nodeid_( void);
EXT INT  FATR ga_cluster_nprocs_(INT*);
EXT INT  FATR ga_cluster_proc_nodeid_(INT*);
EXT INT  FATR ga_cluster_procid_(INT*, INT*);
EXT LOG  FATR ga_compare_distr_(INT*, INT* );
EXT void FATR ga_copy_(INT *, INT *);
EXT INT  FATR ga_create_handle_();
EXT LOG  FATR ga_create_mutexes_(INT*);
EXT void FATR ga_cscal_(INT *, SCPL *);
EXT void FATR ga_cscal_patch_(INT *, INT *, INT *, INT *, INT *, SCPL *);
EXT void FATR ga_dadd_(DBL*, INT*, DBL*, INT*, INT*);
EXT void FATR ga_dadd_patch_(DBL *, INT *, INT *, INT *, INT *, INT *, DBL *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *);
EXT DBL  FATR ga_ddot_(INT *, INT *);
EXT LOG  FATR ga_destroy_(INT* );
EXT LOG  FATR ga_destroy_mutexes_(void);
EXT void FATR ga_dfill_(INT *, DBL *);
EXT void FATR ga_dfill_patch_(INT *, INT *, INT *, INT *, INT *, DBL *);
EXT void FATR ga_dgop_(INT*, DBL*, INT*, char*, int);
EXT void FATR ga_diag_(INT *, INT *, INT *, DBL *);
EXT void FATR ga_diag_reuse_(INT*, INT *, INT *, INT *, DBL *);
EXT void FATR ga_diag_seq_(INT *, INT *, INT *, DBL *);
EXT void FATR ga_diag_std_(INT *, INT *, DBL *);
EXT void FATR ga_diag_std_seq_(INT *, INT *, DBL *);
EXT void FATR ga_distribution_(INT*, INT*, INT*, INT*, INT*, INT* );
EXT void FATR ga_dscal_(INT *, DBL *);
EXT void FATR ga_dscal_patch_(INT *, INT *, INT *, INT *, INT *, DBL *);
EXT LOG  FATR ga_duplicate_(INT*, INT*, char*, int);
EXT void FATR ga_elem_divide_(INT *g_a, INT *g_b, INT *g_c);
EXT void FATR ga_elem_divide_patch_(INT *g_a,INT *alo,INT *ahi, INT *g_b,INT *blo,INT *bhi, INT *g_c,INT *clo,INT *chi);
EXT void FATR ga_elem_maximum_(INT *g_a, INT *g_b, INT *g_c);
EXT void FATR ga_elem_maximum_patch_(INT *g_a,INT *alo,INT *ahi, INT *g_b,INT *blo,INT *bhi, INT *g_c,INT *clo,INT *chi);
EXT void FATR ga_elem_minimum_(INT *g_a, INT *g_b, INT *g_c);
EXT void FATR ga_elem_minimum_patch_(INT *g_a,INT *alo,INT *ahi, INT *g_b,INT *blo,INT *bhi, INT *g_c,INT *clo,INT *chi);
EXT void FATR ga_elem_multiply_(INT *g_a, INT *g_b, INT *g_c);
EXT void FATR ga_elem_multiply_patch_(INT *g_a,INT *alo,INT *ahi, INT *g_b,INT *blo,INT *bhi, INT *g_c,INT *clo,INT *chi);
EXT void FATR ga_fast_merge_mirrored_(INT *g_a);
EXT void FATR ga_fence_( void);
EXT void FATR ga_fill_(INT *, void *);
EXT void FATR ga_fill_patch_(INT *, INT *, INT *, INT *, INT *, void *);
EXT void FATR ga_gather_(INT*, void*, INT*, INT*, INT* );
EXT void FATR ga_get_(INT*, INT*, INT*, INT*, INT*, void*, INT* );
EXT void FATR ga_get_block_info_( INT *g_a, INT *num_blocks, INT *block_dims );
EXT LOG  FATR ga_get_debug_();
EXT void FATR ga_get_diag_(INT *g_a, INT *g_v);
EXT INT  FATR ga_get_dimension_(INT *g_a);
EXT void FATR ga_get_mirrored_block_(INT *g_a, INT *npatch, INT *lo,  INT *hi);
EXT INT  FATR ga_get_pgroup_(INT *g_a);
EXT INT  FATR ga_get_pgroup_size_(INT *grp_id);
EXT void FATR ga_get_proc_grid_(INT* g_a, INT* dims);
EXT void FATR ga_get_proc_index_( INT *g_a, INT *iproc, INT *index );
EXT void FATR ga_ghost_barrier_(void);
EXT void FATR ga_gop_(INT*, void*, INT*, char*, int);   
EXT LOG  FATR ga_has_ghosts_(INT *g_a);
EXT void FATR ga_iadd_(INT*, INT*, INT*, INT*, INT*);
EXT void FATR ga_iadd_patch_(INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *);
EXT INT  FATR ga_idot_(INT *, INT *);
EXT void FATR ga_ifill_(INT *, INT *);
EXT void FATR ga_ifill_patch_(INT *, INT *, INT *, INT *, INT *, INT *);
EXT void FATR ga_igop_(INT*, INT*, INT*, char*, int);
EXT void FATR ga_init_fence_( void);
EXT void FATR ga_initialize_( void);
EXT void FATR ga_initialize_ltd_( INT* );
EXT void FATR ga_inquire_(INT*, INT*, INT*, INT* );
EXT void FATR ga_inquire_internal_(INT*, INT*, INT*, INT* );
EXT INT  FATR ga_inquire_memory_(void);
EXT void FATR ga_inquire_name_(INT*, char*, int);
EXT LOG  FATR ga_is_mirrored_(INT *g_a);
EXT void FATR ga_iscal_(INT *, INT *);
EXT void FATR ga_iscal_patch_(INT *, INT *, INT *, INT *, INT *, INT *);
EXT void FATR ga_list_data_servers_(INT* );
EXT void FATR ga_list_nodeid_(INT*, INT* );
EXT INT  FATR ga_llt_solve_(INT *, INT *);
EXT LOG  FATR ga_locate_(INT*, INT*, INT*, INT* );
EXT LOG  FATR ga_locate_region_(INT*, INT*, INT*, INT*, INT*, INT map[][5], INT* );
EXT void FATR ga_lock_(INT* );
EXT void FATR ga_lu_solve_(char *, INT *, INT *);
EXT void FATR ga_lu_solve_alt_(INT *, INT *, INT *);
EXT void FATR ga_mask_sync_(INT *begin, INT *end);
EXT void FATR ga_median_(INT *g_a, INT *g_b, INT *g_c, INT *g_m);
EXT void FATR ga_median_patch_(INT *g_a, INT *alo, INT *ahi, INT *g_b, INT *blo, INT *bhi, INT *g_c, INT *clo, INT *chi, INT *g_m, INT *mlo, INT *mhi);
EXT INT  FATR ga_memory_avail_(void);
EXT LOG  FATR ga_memory_limited_( void);
EXT void FATR ga_merge_mirrored_(INT *g_a);
EXT void FATR ga_nbacc_(INT*, INT*, INT*, INT*, INT*, void*, INT*, void* ,INT *);
EXT void FATR ga_nbget_(INT*, INT*, INT*, INT*, INT*, void*, INT*, INT* );
EXT void FATR ga_nbput_(INT*, INT*, INT*, INT*, INT*, void*, INT*, INT* );
EXT INT  FATR ga_nbtest_(INT*);
EXT void FATR ga_nbwait_(INT*);
EXT INT  FATR ga_ndim_(INT *g_a);
EXT INT  FATR ga_nnodes_(void);
EXT INT  FATR ga_nodeid_(void);
EXT void FATR ga_norm1_(INT *g_a, double *nm);
EXT void FATR ga_norm_infinity_(INT *g_a, double *nm);
EXT void FATR ga_num_data_servers_(INT* );
EXT INT  FATR ga_num_mirrored_seg_(INT *g_a);
EXT void FATR ga_pack_(INT* g_a, INT* g_b, INT* g_sbit, INT* lo, INT* hi, INT* icount);
EXT void FATR ga_patch_enum_(INT* g_a, INT* lo, INT* hi, void* start, void* stride);
EXT INT  FATR ga_pgroup_absolute_id_(INT *grp, INT *pid);
EXT void FATR ga_pgroup_brdcst_(INT*, INT*, void*, INT*, INT* );
EXT void FATR ga_pgroup_cgop_(INT*, INT*, SCPL*, INT*, char*, int);
EXT INT  FATR ga_pgroup_create_(INT *list, INT *count);
EXT INT  FATR ga_pgroup_destroy_(INT *grp);
EXT void FATR ga_pgroup_dgop_(INT*, INT*, DBL*, INT*, char*, int);
EXT INT  FATR ga_pgroup_get_default_();
EXT INT  FATR ga_pgroup_get_mirror_();
EXT INT  FATR ga_pgroup_get_world_();
EXT void FATR ga_pgroup_igop_(INT*, INT*, INT*, INT*, char*, int);
EXT INT  FATR ga_pgroup_nnodes_(INT *grp_id);
EXT INT  FATR ga_pgroup_nodeid_(INT *grp_id);
EXT void FATR ga_pgroup_set_default_(INT *grp);
EXT void FATR ga_pgroup_sgop_(INT*, INT*, REAL*, INT*, char*, int);
EXT INT  FATR ga_pgroup_split_( INT *grp, INT *num_group);
EXT INT  FATR ga_pgroup_split_irreg_(INT *grp, INT *mycolor, INT *key);
EXT void FATR ga_pgroup_sync_(INT*);
EXT void FATR ga_pgroup_zgop_(INT*, INT*, DCPL*, INT*, char*, int);
EXT void FATR ga_print_(INT *);
EXT void FATR ga_print_distribution_(INT *g_a);
EXT void FATR ga_print_patch_(INT *, INT *, INT *, INT *, INT *, INT *);
EXT void FATR ga_print_stats_();
EXT void FATR ga_proc_topology_(INT *g_a, INT *proc, INT *pr, INT *pc);
EXT void FATR ga_put_(INT*, INT*, INT*, INT*, INT*, void*, INT* );
EXT void FATR ga_randomize_(INT*, void*);
EXT INT  FATR ga_read_inc_(INT*, INT*, INT*, INT* );
EXT void FATR ga_recip_(INT *g_a);
EXT void FATR ga_recip_patch_(INT *g_a, INT *lo, INT *hi);
EXT void FATR ga_release_(INT*, INT*, INT*, INT*, INT*);
EXT void FATR ga_release_update_(INT*, INT*, INT*, INT*, INT* );
EXT void FATR ga_sadd_(REAL*, INT*, REAL*, INT*, INT*);
EXT void FATR ga_sadd_patch_(REAL *, INT *, INT *, INT *, INT *, INT *, REAL *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *);
EXT void FATR ga_scale_(INT *, void *);
EXT void FATR ga_scale_cols_(INT *g_a, INT *g_v);
EXT void FATR ga_scale_patch_(INT *, INT *, INT *, INT *, INT *, void *);
EXT void FATR ga_scale_rows_(INT *g_a, INT *g_v);
EXT void FATR ga_scan_add_(INT* g_a, INT* g_b, INT* g_sbit, INT* lo, INT* hi, INT *excl);
EXT void FATR ga_scan_copy_(INT* g_a, INT* g_b, INT* g_sbit, INT* lo, INT* hi);
EXT void FATR ga_scatter_(INT*, void*, INT*, INT*, INT*);
EXT REAL FATR ga_sdot_(INT *, INT *);            
EXT void FATR ga_set_array_name_(INT *g_a, char *array_name, int slen);
EXT void FATR ga_set_block_cyclic_(INT *g_a, INT *dims);
EXT void FATR ga_set_block_cyclic_proc_grid_(INT *g_a, INT block[], INT proc_grid[]);
EXT void FATR ga_set_chunk_(INT *g_a, INT *chunk);
EXT void FATR ga_set_data_(INT *g_a, INT *ndim, INT *dims, INT *type);
EXT void FATR ga_set_debug_( LOG *flag );
EXT void FATR ga_set_diagonal_(INT *g_a, INT *g_v);
EXT void FATR ga_set_ghost_corner_flag_(INT *g_a, LOG *flag);
EXT LOG  FATR ga_set_ghost_info_(INT *g_a);
EXT void FATR ga_set_ghosts_(INT *g_a, INT *width);
EXT void FATR ga_set_irreg_distr_(INT *g_a, INT *map, INT *nblock);
EXT void FATR ga_set_irreg_flag_(INT *g_a, LOG *flag);
EXT void FATR ga_set_memory_limit_(INT *mem_limit);
EXT void FATR ga_set_pgroup_(INT *g_a, INT *p_handle);
EXT void FATR ga_set_restricted_(INT *g_a, INT list[], INT *size);
EXT void FATR ga_set_restricted_range_(INT *g_a, INT *lo_proc, INT *hi_proc);
EXT LOG  FATR ga_set_update4_info_(INT *g_a);
EXT LOG  FATR ga_set_update5_info_(INT *g_a);
EXT void FATR ga_sfill_(INT *, REAL *);
EXT void FATR ga_sfill_patch_(INT *, INT *, INT *, INT *, INT *, REAL *);
EXT void FATR ga_sgop_(INT*, REAL*, INT*, char*, int);
EXT void FATR ga_shift_diagonal_(INT *g_a, void *c);
EXT INT  FATR ga_solve_(INT *, INT *);
EXT void FATR ga_sort_permut_(INT* g_a, INT* index, INT* i, INT* j, INT* nv);
EXT INT  FATR ga_spd_invert_(INT *);
EXT void FATR ga_sscal_(INT *, REAL *);
EXT void FATR ga_sscal_patch_(INT *, INT *, INT *, INT *, INT *, REAL *);
EXT void FATR ga_step_bound_info_(INT *g_xx, INT *g_vv, INT *g_xxll, INT *g_xxuu, void *boundmin, void *wolfemin, void *boundmax);
EXT void FATR ga_step_bound_info_patch_(INT *g_xx, INT *xxlo,INT *xxhi, INT *g_vv, INT *vvlo, INT *vvhi, INT *g_xxll, INT *xxlllo, INT *xxllhi, INT *g_xxuu, INT *xxuulo, INT *xxuuhi, void *boundmin, void *wolfemin, void *boundmax);
EXT void FATR ga_step_max_(INT *g_a, INT *g_b, void *step);
EXT void FATR ga_step_max_patch_(INT *g_a, INT *alo, INT *ahi, INT *g_b,  INT *blo, INT *bhi, void *step);
EXT void FATR ga_summarize_(LOG*);
EXT void FATR ga_symmetrize_(INT *); 
EXT void FATR ga_sync_( void);
EXT void FATR ga_terminate_( void);
EXT INT  FATR ga_total_blocks_( INT *g_a );
EXT void FATR ga_transpose_(INT *, INT *);
EXT void FATR ga_unlock_(INT* );
EXT void FATR ga_unpack_(INT* g_a, INT* g_b, INT* g_sbit, INT* lo, INT* hi, INT* icount);
EXT void FATR ga_update1_ghosts_(INT *g_a);
EXT LOG  FATR ga_update2_ghosts_(INT *g_a);
EXT LOG  FATR ga_update3_ghosts_(INT *g_a);
EXT LOG  FATR ga_update4_ghosts_(INT *g_a);
EXT LOG  FATR ga_update5_ghosts_(INT *g_a);
EXT LOG  FATR ga_update6_ghosts_(INT *g_a);
EXT LOG  FATR ga_update7_ghosts_(INT *g_a);
EXT void FATR ga_update_ghosts_(INT *g_a);
EXT LOG  FATR ga_uses_ma_( void);
EXT LOG  FATR ga_uses_proc_grid_( INT *g_a );
EXT LOG  FATR ga_valid_handle_(INT *g_a);
EXT INT  FATR ga_verify_handle_(INT* );
EXT DBL  FATR ga_wtime_(void);
EXT void FATR ga_zadd_(DCPL*, INT*, DCPL*, INT*, INT*);
EXT void FATR ga_zadd_patch_(DCPL *, INT *, INT *, INT *, INT *, INT *, DCPL *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *, INT *);
EXT void FATR ga_zero_(INT *);
EXT void FATR ga_zero_diagonal_(INT *g_a);
EXT void FATR ga_zfill_(INT *, DCPL *);
EXT void FATR ga_zfill_patch_(INT *, INT *, INT *, INT *, INT *, DCPL *);
EXT void FATR ga_zgop_(INT*, DCPL*, INT*, char*, int);
EXT void FATR ga_zscal_(INT *, DCPL *);
EXT void FATR ga_zscal_patch_(INT *, INT *, INT *, INT *, INT *, DCPL *);
EXT void FATR gai_cdot_(INT *, INT *, SCPL*);
EXT void FATR gai_zdot_(INT *, INT *, DCPL*);
EXT void FATR nga_acc_(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld, void *alpha);
EXT void FATR nga_access_(INT* g_a, INT lo[], INT hi[], ACCESS_IDX* index, INT ld[]);
EXT void FATR nga_access_block_(INT* g_a, INT* idx, ACCESS_IDX* index, INT ld[]);
EXT void FATR nga_access_block_grid_(INT* g_a, INT* subscript, ACCESS_IDX *index, INT *ld);
EXT void FATR nga_access_block_segment_(INT* g_a, INT *proc, ACCESS_IDX *index, INT *len);
EXT void FATR nga_access_ghost_element_(INT* g_a, ACCESS_IDX* index, INT subscript[], INT ld[]);
EXT void FATR nga_access_ghosts_(INT* g_a, INT dims[], ACCESS_IDX* index, INT ld[]);
EXT void FATR nga_add_patch_(void *alpha, INT *g_a, INT *alo, INT *ahi, void *beta, INT *g_b, INT *blo, INT *bhi, INT *g_c, INT *clo, INT *chi);
EXT void FATR nga_distribution_(INT *g_a, INT *proc, INT *lo, INT *hi);
EXT void FATR nga_fill_patch_(INT *g_a, INT *lo, INT *hi, void *val);
EXT void FATR nga_gather_(INT *g_a, void* v, INT subscr[], INT *nv);
EXT void FATR nga_get_(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld);
EXT void FATR nga_get_ghost_block_(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld);
EXT void FATR nga_inquire_(INT *g_a, INT *type, INT *ndim, INT *dims); 
EXT void FATR nga_inquire_internal_(INT *g_a, INT *type, INT *ndim, INT *dims);
EXT LOG  FATR nga_locate_(INT *g_a, INT* subscr, INT* owner);
EXT INT  FATR nga_locate_num_blocks_(INT *g_a, INT *lo, INT *hi);
EXT LOG  FATR nga_locate_nnodes_(INT *g_a, INT *lo, INT *hi, INT *np);
EXT LOG  FATR nga_locate_region_(INT *g_a, INT *lo, INT *hi, INT *map, INT *proclist, INT *np);
EXT void FATR nga_merge_distr_patch_(INT *g_a, INT *alo, INT *ahi, INT *g_b, INT *blo, INT *bhi);
EXT void FATR nga_merge_distr_patch_(INT *g_a, INT *alo, INT *ahi, INT *g_b, INT *blo, INT *bhi);
EXT void FATR nga_nbacc_(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld, void *alpha,INT *nbhdl);
EXT void FATR nga_nbget_(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld, INT *nbhdl);
EXT void FATR nga_nbget_ghost_dir_(INT *g_a, INT *mask, INT *nbhandle);
EXT void FATR nga_nbput_(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld, INT *nbhdl);
EXT INT  FATR nga_nbtest_(INT *nbhdl);
EXT void FATR nga_nbwait_(INT *nbhdl);
EXT void FATR nga_periodic_acc_(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld, void *alpha);
EXT void FATR nga_periodic_get_(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld);
EXT void FATR nga_periodic_put_(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld);
EXT void FATR nga_print_patch_(INT *g_a, INT *lo, INT *hi, INT *pretty);
EXT void FATR nga_proc_topology_(INT* g_a, INT* proc, INT* subscr);
EXT void FATR nga_put_(INT *g_a, INT *lo, INT *hi, void *buf, INT *ld);
EXT INT  FATR nga_read_inc_(INT* g_a,INT* subscr,INT* inc);
EXT void FATR nga_release_(INT *g_a, INT *lo, INT *hi);
EXT void FATR nga_release_block_(INT *g_a, INT *iblock);
EXT void FATR nga_release_block_grid_(INT *g_a, INT *index);
EXT void FATR nga_release_block_segment_(INT *g_a, INT *iproc);
EXT void FATR nga_release_ghost_element_(INT *g_a, INT *index);
EXT void FATR nga_release_ghosts_(INT* g_a);
EXT void FATR nga_release_update_(INT *g_a, INT *lo, INT *hi);
EXT void FATR nga_release_update_block_(INT *g_a, INT *iblock);
EXT void FATR nga_release_update_block_grid_(INT *g_a, INT *index);
EXT void FATR nga_release_update_block_segment_(INT *g_a, INT *iproc);
EXT void FATR nga_release_update_ghost_element_(INT *g_a, INT *index);
EXT void FATR nga_release_update_ghosts_(INT *g_a);
EXT void FATR nga_scale_patch_(INT *g_a, INT *lo, INT *hi, void *alpha);
EXT void FATR nga_scatter_(INT *g_a, void* v, INT subscr[], INT *nv);
EXT void FATR nga_scatter_acc_(INT *g_a, void* v, INT subscr[], INT *nv, void *alpha);
EXT void FATR nga_strided_acc_(INT *g_a, INT *lo, INT *hi, INT *skip, void *buf, INT *ld, void *alpha);
EXT void FATR nga_strided_get_(INT *g_a, INT *lo, INT *hi, INT *skip, void *buf, INT *ld);
EXT void FATR nga_strided_put_(INT *g_a, INT *lo, INT *hi, INT *skip, void *buf, INT *ld);
EXT LOG  FATR nga_update_ghost_dir_(INT *g_a, INT *idim, INT *idir, LOG *flag);
EXT void FATR nga_zero_patch_(INT *g_a, INT *lo, INT *hi);

#ifdef __cplusplus
}
#endif

EXT DCPL *DCPL_MB;
EXT SCPL *SCPL_MB;
EXT DBL  *DBL_MB;
EXT FLT  *FLT_MB;
EXT INT  *INT_MB;

#undef EXT
#undef DCPL
#undef SCPL
#undef DBL
#undef REAL
#undef FLT
#undef INT
#undef LOG
#undef ACCESS_IDX

#endif /* GLOBAL_H */
