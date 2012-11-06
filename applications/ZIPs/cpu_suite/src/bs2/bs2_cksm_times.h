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

#  ifndef TOO_FAST_257027
#    define TOO_FAST_257027      (1)
#  endif

#define VALID_CKSM_257027      (0x022670d92d9352d65)

#endif
