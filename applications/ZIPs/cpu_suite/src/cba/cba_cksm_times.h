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

#  ifndef TOO_FAST_64_64_7173289
#    define TOO_FAST_64_64_7173289         (1)
#  endif

#  ifndef TOO_FAST_145_2368_202323
#    define TOO_FAST_145_2368_202323       (1)
#  endif

#  ifndef TOO_FAST_231_128000_3400
#    define TOO_FAST_231_128000_3400       (1)
#  endif





#endif
