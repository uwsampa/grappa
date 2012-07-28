/**************************************************************\
*   These constants are used for checks on results correctness *
* and a partial check of the run time reasonableness. They are *
* most useful during acceptance testing when the output is     *
* is shortened to a single line.                               *
*   The numbers in constant names reflect the arguments and    *
* the number of iterations.                                    *
*   During acceptance testing the times can be change at       *
* compile time. This may be helpful in finding non-uniform run *
* times.                                                       *
\**************************************************************/
#ifndef __BM_CKS_TIMES
#  define __BM_CKS_TIMES

#  ifndef TOO_FAST
#    define TOO_FAST        (1)
#  endif

#  ifndef VALID_CKSM_GT_8
#     define  VALID_CKSM_GT_8     (0x5af344f737e3fbbe)
#  endif
#  ifndef VALID_CKSM_GT_16
#     define  VALID_CKSM_GT_16    (0x5f1dec4cdca9fe22)
#  endif
#  ifndef VALID_CKSM_GT_LG
#     define  VALID_CKSM_GT_LG    (0x918eee0f80bd58b4)
#  endif

#  ifndef VALID_CKSM_SC_8
#     define  VALID_CKSM_SC_8     (0xe04b4741d63d9108)
#  endif
#  ifndef VALID_CKSM_SC_16
#     define  VALID_CKSM_SC_16    (0xe9d41154e5be15c0)
#  endif
#  ifndef VALID_CKSM_SC_LG
#     define  VALID_CKSM_SC_LG    (0x3f6be27a586e4589)
#  endif

#endif
