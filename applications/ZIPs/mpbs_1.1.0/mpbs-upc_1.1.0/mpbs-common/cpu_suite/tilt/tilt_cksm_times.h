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

#  ifndef TOO_FAST_119554831
#    define TOO_FAST_119554831         (1)
#  endif

#  ifndef TOO_FAST_F_119554831
#    define TOO_FAST_F_119554831       (1)
#  endif

#  ifdef CSEED
#    define VALID_CKSM1_CSEED_119554831         (0x0f6e7fa7d0dff0afd)
#    define VALID_CKSM2_CSEED_119554831         (0x0d053290c9d4c983f)
#  endif

#  ifdef FSEED
#    define VALID_CKSM1_FSEED_119554831         (0x03d175d94b98c8721)
#    define VALID_CKSM2_FSEED_119554831         (0x08a373795016e3f63)
#  endif

#endif
