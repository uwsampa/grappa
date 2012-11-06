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

#  ifndef VALID_CKSM_GMP_8192
#     define VALID_CKSM_GMP_8192      (0x3238330d01030c04)
#  endif
#  ifndef VALID_CKSM_GMP_4096
#     define VALID_CKSM_GMP_4096      (0x3b31370a0907050f)
#  endif
#  ifndef VALID_CKSM_GMP_2048
#     define VALID_CKSM_GMP_2048      (0x7070931363d3532)
#  endif
#  ifndef VALID_CKSM_GMP_1024
#     define VALID_CKSM_GMP_1024      (0x3a3f3335383d3534)
#  endif
#  ifndef VALID_CKSM_GMP_512
#     define VALID_CKSM_GMP_512       (0x0a03080c0f080134)
#  endif
#  ifndef VALID_CKSM_GMP_256
#     define VALID_CKSM_GMP_256       (0x3339070b0f0c000c)
#  endif
#  ifndef VALID_CKSM_GMP_128
#     define VALID_CKSM_GMP_128       (0x0e0b0a063b3c363e)
#  endif


#endif
