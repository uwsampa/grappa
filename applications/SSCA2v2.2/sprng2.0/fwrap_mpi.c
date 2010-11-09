
#include "fwrap.h"
#include "sprng_interface.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/************************************************************************/
/************************************************************************/
/*									*/
/*	This package of C wrappers is intended to be called from a 	*/
/*	FORTRAN program. The main purpose of the package is to mediate	*/
/*	between the call-by-address and call-by-value conventions in	*/
/*	the two languages. In most cases, the arguments of the C	*/
/*	routines and the wrappers are the same. There are two		*/
/*	exceptions to this. The trivial exception is that the C number	*/
/*	scheme of 0 thru N-1 is automatically converted to the FORTRAN	*/
/*	scheme of 1 thru N, so when referring to a particular generator	*/
/*	the FORTRAN user should number as is natural to that language.	*/
/*									*/
/*									*/
/*	The wrappers should be treated as FORTRAN function calls.	*/
/*									*/
/************************************************************************/
/************************************************************************/


#ifdef __STDC__
int FNAMEOF_fseed_mpi(void)
#else
int FNAMEOF_fseed_mpi()
#endif
{
#ifdef SPRNG_MPI
  return make_new_seed_mpi();
#else
  return -1;
#endif
}

#ifdef SPRNG_MPI

#ifdef __STDC__
int * FNAMEOF_finit_rng_simmpi(int *rng_type, int *seed, int *mult)
#else
int * FNAMEOF_finit_rng_simmpi(rng_type,seed,mult)
int *rng_type,*mult,*seed;
#endif
{
	return init_rng_simple_mpi(*rng_type,*seed, *mult);
}

#ifdef __STDC__
int FNAMEOF_fget_rn_int_simmpi(void)
#else
int FNAMEOF_fget_rn_int_simmpi()
#endif
{
  return get_rn_int_simple_mpi();
}


#ifdef __STDC__
float FNAMEOF_fget_rn_flt_simmpi(void)
#else
float FNAMEOF_fget_rn_flt_simmpi()
#endif
{
  return get_rn_flt_simple_mpi();
}



#ifdef __STDC__
double FNAMEOF_fget_rn_dbl_simmpi(void)
#else
double FNAMEOF_fget_rn_dbl_simmpi()
#endif
{
  return get_rn_dbl_simple_mpi();
}
#endif

