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

#  ifndef TOO_FAST_16_11587
#    define TOO_FAST_16_11587    (1)
#  endif

#  ifndef TOO_FAST_25_9
#    define TOO_FAST_25_9        (1)
#  endif


#define VALID_CKSM_16_11587   (0x0000d5e84e6128702a5d);
#define VALID_CKSM_25_9       (0x0000e3d42eeaf4074788);

#endif
