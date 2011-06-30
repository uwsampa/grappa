#if HAVE_CONFIG_H
#   include "config.h"
#endif

/* $Id: ga_vampir.c,v 1.11.2.1 2006-12-22 13:05:21 manoj Exp $ */
#include "ga_vampir.h"

void ga_vampir_init() {
    vampir_symdef(GA_ACC,                    "GA_Acc",                     "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(GA_ACCESS,                 "GA_Access",                  "GA",__FILE__,__LINE__);
    vampir_symdef(GA_ADD,                    "GA_Add",                     "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_ADD_PATCH,              "GA_Add_patch",               "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_BRDCST,                 "GA_Brdcst",                  "GA_COLLECTIVE",__FILE__,__LINE__);
    vampir_symdef(GA_CHECK_HANDLE,           "GA_Check_handle",            "GA",__FILE__,__LINE__);
    vampir_symdef(GA_COMPARE_DISTR,          "GA_Compare_distr",           "GA",__FILE__,__LINE__);
    vampir_symdef(GA_COPY,                   "GA_Copy",                    "GA",__FILE__,__LINE__);
    vampir_symdef(GA_COPY_PATCH,             "GA_Copy_patch",              "GA",__FILE__,__LINE__);
    vampir_symdef(GA_CREATE,                 "GA_Create",                  "GA",__FILE__,__LINE__);
    vampir_symdef(GA_CREATE_IRREG,           "GA_Create_irreg",            "GA",__FILE__,__LINE__);
    vampir_symdef(GA_CREATE_MUTEXES,         "GA_Create_mutexes",          "GA",__FILE__,__LINE__);
    vampir_symdef(GA_DDOT,                   "GA_Ddot",                    "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_DDOT_PATCH,             "GA_Ddot_patch",              "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_DESTROY,                "GA_Destroy",                 "GA",__FILE__,__LINE__);
    vampir_symdef(GA_DESTROY_MUTEXES,        "GA_Destroy_mutexes",         "GA",__FILE__,__LINE__);
    vampir_symdef(GA_DGOP,                   "GA_Dgop",                    "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_GOP,                    "GA_Gop",                     "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_DISTRIBUTION,           "GA_Distribution",            "GA",__FILE__,__LINE__);
    vampir_symdef(GA_DOT,                    "GA_Dot",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_DOT_PATCH,              "GA_Dot_patch",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_DUPLICATE,              "GA_Duplicate",               "GA",__FILE__,__LINE__);
    vampir_symdef(GA_ERROR,                  "GA_Error",                   "GA",__FILE__,__LINE__);
    vampir_symdef(GA_FENCE,                  "GA_Fence",                   "GA",__FILE__,__LINE__);
    vampir_symdef(GA_FILL,                   "GA_Fill",                    "GA",__FILE__,__LINE__);
    vampir_symdef(GA_RANDOMIZE,              "GA_Randomize",               "GA",__FILE__,__LINE__);
    vampir_symdef(GA_FILL_PATCH,             "GA_Fill_patch",              "GA",__FILE__,__LINE__);
    vampir_symdef(GA_GATHER,                 "GA_Gather",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(GA_GET,                    "GA_Get",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(GA_IGOP,                   "GA_Igop",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_INITIALIZE,             "GA_Initialize",              "GA",__FILE__,__LINE__);
    vampir_symdef(GA_INITIALIZE_LTD,         "GA_Initialize_ltd",          "GA",__FILE__,__LINE__);
    vampir_symdef(GA_INIT_FENCE,             "GA_Init_fence",              "GA",__FILE__,__LINE__);
    vampir_symdef(GA_INQUIRE,                "GA_Inquire",                 "GA",__FILE__,__LINE__);
    vampir_symdef(GA_INQUIRE_MEMORY,         "GA_Inquire_memory",          "GA",__FILE__,__LINE__);
    vampir_symdef(GA_INQUIRE_NAME,           "GA_Inquire_name",            "GA",__FILE__,__LINE__);
    vampir_symdef(GA_LIST_NODEID,            "GA_List_nodeid",             "GA",__FILE__,__LINE__);
    vampir_symdef(GA_LOCATE,                 "GA_Locate",                  "GA",__FILE__,__LINE__);
    vampir_symdef(GA_LOCATE_REGION,          "GA_Locate_region",           "GA",__FILE__,__LINE__);
    vampir_symdef(GA_LOCK,                   "GA_Lock",                    "GA",__FILE__,__LINE__);
    vampir_symdef(GA_MATMUL_PATCH,           "GA_Matmul_patch",            "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_MATMUL,                 "GA_Matmul",                  "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(NGA_MATMUL_PATCH,          "NGA_Matmul_patch",           "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_MEMORY_AVAIL,           "GA_Memory_avail",            "GA",__FILE__,__LINE__);
    vampir_symdef(GA_MEMORY_LIMITED,         "GA_Memory_limited",          "GA",__FILE__,__LINE__);
    vampir_symdef(GA_MPI_COMMUNICATOR,       "GA_Mpi_communicator",        "GA",__FILE__,__LINE__);
    vampir_symdef(GA_NBLOCK,                 "GA_Nblock",                  "GA",__FILE__,__LINE__);
    vampir_symdef(GA_NDIM,                   "GA_Ndim",                    "GA",__FILE__,__LINE__);
    vampir_symdef(GA_NNODES,                 "GA_Nnodes",                  "GA",__FILE__,__LINE__);
    vampir_symdef(GA_NODEID,                 "GA_Nodeid",                  "GA",__FILE__,__LINE__);
    vampir_symdef(GA_PERIODIC_ACC,           "GA_Periodic_acc",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(GA_PERIODIC_GET,           "GA_Periodic_get",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(GA_PERIODIC_PUT,           "GA_Periodic_put",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(GA_PRINT,                  "GA_Print",                   "GA",__FILE__,__LINE__);
    vampir_symdef(GA_PRINT_DISTRIBUTION,     "GA_Print_distribution",      "GA",__FILE__,__LINE__);
    vampir_symdef(GA_PRINT_PATCH,            "GA_Print_patch",             "GA",__FILE__,__LINE__);
    vampir_symdef(GA_PRINT_STATS,            "GA_Print_stats",             "GA",__FILE__,__LINE__);
    vampir_symdef(GA_PROC_TOPOLOGY,          "GA_Proc_topology",           "GA",__FILE__,__LINE__);
    vampir_symdef(GA_PUT,                    "GA_Put",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(GA_READ_INC,               "GA_Read_inc",                "GA",__FILE__,__LINE__);
    vampir_symdef(GA_RELEASE,                "GA_Release",                 "GA",__FILE__,__LINE__);
    vampir_symdef(GA_RELEASE_UPDATE,         "GA_Release_update",          "GA",__FILE__,__LINE__);
    vampir_symdef(GA_SCALE,                  "GA_Scale",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_SCALE_PATCH,            "GA_Scale_patch",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_SCATTER,                "GA_Scatter",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(GA_SELECT_ELEM,            "GA_Select_elem",             "GA",__FILE__,__LINE__);
    vampir_symdef(GA_SET_MEMORY_LIMIT,       "GA_Set_memory_limit",        "GA",__FILE__,__LINE__);
    vampir_symdef(GA_SPD_INVERT,             "GA_Spd_invert",              "GA",__FILE__,__LINE__);
    vampir_symdef(GA_SUMMARIZE,              "GA_Summarize",               "GA",__FILE__,__LINE__);
    vampir_symdef(GA_SYNC,                   "GA_Sync",
        "GA_COLLECTIVE",__FILE__,__LINE__);
    vampir_symdef(GA_PGROUP_SYNC,            "GA_Pgroup_sync",
        "GA_COLLECTIVE",__FILE__,__LINE__);
    vampir_symdef(GA_TERMINATE,              "GA_Terminate",               "GA",__FILE__,__LINE__);
    vampir_symdef(GA_TRANSPOSE,              "GA_Transpose",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_UNLOCK,                 "GA_Unlock",                  "GA",__FILE__,__LINE__);
    vampir_symdef(GA_USES_MA,                "GA_Uses_ma",                 "GA",__FILE__,__LINE__);
    vampir_symdef(GA_ZDOT,                   "GA_Zdot",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_ZDOT_PATCH,             "GA_Zdot_patch",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_CDOT,                   "GA_Cdot",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_CDOT_PATCH,             "GA_Cdot_patch",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_ZERO,                   "GA_Zero",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(GA_ZERO_PATCH,             "GA_Zero_patch",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(NGA_ACC,                   "NGA_Acc",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(NGA_ACCESS,                "NGA_Access",                 "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_ADD_PATCH,             "NGA_Add_patch",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(NGA_COPY_PATCH,            "NGA_Copy_patch",             "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_CREATE,                "NGA_Create",                 "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_CREATE_IRREG,          "NGA_Create_irreg",           "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_CREATE_IRREG_CONFIG,          "NGA_Create_irreg_config",           "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_DDOT_PATCH,            "NGA_Ddot_patch",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(NGA_DISTRIBUTION,          "NGA_Distribution",           "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_FILL_PATCH,            "NGA_Fill_patch",             "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_GATHER,                "NGA_Gather",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(NGA_GET,                   "NGA_Get",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(NGA_INQUIRE,               "NGA_Inquire",                "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_LOCATE,                "NGA_Locate",                 "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_LOCATE_REGION,         "NGA_Locate_region",          "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_PERIODIC_ACC,          "NGA_Periodic_acc",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(NGA_PERIODIC_GET,          "NGA_Periodic_get",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(NGA_PERIODIC_PUT,          "NGA_Periodic_put",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(NGA_PUT,                   "NGA_Put",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(NGA_READ_INC,              "NGA_Read_inc",               "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_RELEASE,               "NGA_Release",                "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_RELEASE_UPDATE,        "NGA_Release_update",         "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_SCALE_PATCH,           "NGA_Scale_patch",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(NGA_SCATTER,               "NGA_Scatter",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(NGA_SELECT_ELEM,           "NGA_Select_elem",            "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_ZDOT_PATCH,            "NGA_Zdot_patch",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(NGA_CDOT_PATCH,            "NGA_Cdot_patch",
        "GA_MATRIX",__FILE__,__LINE__);
    vampir_symdef(NGA_ZERO_PATCH,            "NGA_Zero_patch",
        "GA_MATRIX",__FILE__,__LINE__);

    vampir_symdef(GA_CHOLESKY,               "GA_Cholesky",                "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(VT_GA_DGEMM,               "GA_Dgemm",                   "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(VT_GA_SGEMM,               "GA_Sgemm",                   "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(VT_GA_ZGEMM,               "GA_Zgemm",                   "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(VT_GA_CGEMM,               "GA_Cgemm",                   "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_DGEMM_SEQ,              "GA_Dgemm_seq",               "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_DIAG_STD,               "GA_Diag_std",                "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_DIAG,                   "GA_Diag",                    "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_DIAG_REUSE,             "GA_Diag_reuse",              "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_DIAG_SEQ,               "GA_Diag_seq",                "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_DIAG_STD_SEQ,           "GA_Diag_std_seq",            "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_LU_SOLVE,               "GA_LU_Solve",                "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_LU_SOLVE_ALT,           "GA_LU_Solve_alt",            "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_LU_SOLVE_SEQ,           "GA_LU_Solve_seq",            "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_SYMMETRIZE,             "GA_Symmetrize",              "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_LLT_I,                  "GA_llt_i",                   "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_LLT_S,                  "GA_llt_s",                   "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_LLT_SOLVE,              "GA_llt_solve",               "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_SOLVE,                  "GA_Solve",                   "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_SPD_INVERT,             "GA_SPD_Invert",              "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_SYMUL,                  "GA_SymUL",                   "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(GA_ZEROUL,                 "GA_ZeroUL",                  "GA_linalg",__FILE__,__LINE__);
    vampir_symdef(NGA_CREATE_GHOSTS_IRREG_CONFIG, "NGA_Create_ghosts_irreg_config",           "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_CREATE_IRREG_CONFIG,   "NGA_Create_irreg_config",    "GA",__FILE__,__LINE__);
    vampir_symdef(NGA_NBPUT,                 "NGA_NbPut",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(NGA_STRIDED_PUT,           "NGA_Strided_put",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(NGA_STRIDED_GET,           "NGA_Strided_get",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(NGA_STRIDED_ACC,           "NGA_Strided_acc",
        "GA_ONESIDED",__FILE__,__LINE__);
    vampir_symdef(GA_ALLOCATE,               "GA_ALLOCATE",                "GA",__FILE__,__LINE__);
}
