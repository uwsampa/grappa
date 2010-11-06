#ifndef _fwrap_h
#define _fwrap_h

/************************************************************************/
/************************************************************************/
/*      Inter-language Naming Convention Problem Solution               */
/*                                                                      */
/*      Note that with different compilers you may find that            */
/*      the linker fails to find certain modules due to the naming      */
/*      conventions implicit in particular compilers.  Here the         */
/*      solution was to look at the object code produced by the FORTRAN */
/*      compiler and modify this wrapper code so that the C routines    */
/*      compiled with the same routine names as produced in the FORTRAN */
/*      program.                                                        */
/*                                                                      */
/************************************************************************/
/************************************************************************/


/*
Turn funcName (which must be all lower-case) into something callable from
FORTRAN, typically by appending one or more underscores.
*/
#if defined(Add__)
#define FORTRAN_CALLABLE(funcName) funcName ## __
#elif defined(NoChange)
#define FORTRAN_CALLABLE(funcName) funcName 
#elif defined(Add_)
#define FORTRAN_CALLABLE(funcName) funcName ## _
#endif

#ifdef UpCase
#define FNAMEOF_finit_rng FINIT_RNG
#define FNAMEOF_fspawn_rng FSPAWN_RNG
#define FNAMEOF_fget_rn_int FGET_RN_INT
#define FNAMEOF_fget_rn_flt FGET_RN_FLT
#define FNAMEOF_fget_rn_dbl FGET_RN_DBL
#define FNAMEOF_fmake_new_seed FMAKE_NEW_SEED
#define FNAMEOF_fseed_mpi FSEED_MPI
#define FNAMEOF_ffree_rng FFREE_RNG
#define FNAMEOF_fget_seed_rng FGET_SEED_RNG
#define FNAMEOF_fpack_rng FPACK_RNG
#define FNAMEOF_funpack_rng FUNPACK_RNG
#define FNAMEOF_fprint_rng FPRINT_RNG

#define FNAMEOF_finit_rng_sim FINIT_RNG_SIM
#define FNAMEOF_fget_rn_int_sim FGET_RN_INT_SIM
#define FNAMEOF_fget_rn_flt_sim FGET_RN_FLT_SIM
#define FNAMEOF_fget_rn_dbl_sim FGET_RN_DBL_SIM
#define FNAMEOF_finit_rng_simmpi FINIT_RNG_SIMMPI
#define FNAMEOF_fget_rn_int_simmpi FGET_RN_INT_SIMMPI
#define FNAMEOF_fget_rn_flt_simmpi FGET_RN_FLT_SIMMPI
#define FNAMEOF_fget_rn_dbl_simmpi FGET_RN_DBL_SIMMPI
#define FNAMEOF_fpack_rng_simple FPACK_RNG_SIMPLE
#define FNAMEOF_funpack_rng_simple FUNPACK_RNG_SIMPLE
#define FNAMEOF_fprint_rng_simple FPRINT_RNG_SIMPLE

#define FNAMEOF_finit_rng_ptr FINIT_RNG_PTR
#define FNAMEOF_fget_rn_int_ptr FGET_RN_INT_PTR
#define FNAMEOF_fget_rn_flt_ptr FGET_RN_FLT_PTR
#define FNAMEOF_fget_rn_dbl_ptr FGET_RN_DBL_PTR
#define FNAMEOF_fpack_rng_ptr FPACK_RNG_PTR
#define FNAMEOF_funpack_rng_ptr FUNPACK_RNG_PTR
#define FNAMEOF_fprint_rng_ptr FPRINT_RNG_PTR
#define FNAMEOF_ffree_rng_ptr FFREE_RNG_PTR
#define FNAMEOF_fspawn_rng_ptr FSPAWN_RNG_PTR

#define FNAMEOF_fcpu_t FCPU_T

#else

#define FNAMEOF_ffree_rng FORTRAN_CALLABLE(ffree_rng) 
#define FNAMEOF_fmake_new_seed FORTRAN_CALLABLE(fmake_new_seed) 
#define FNAMEOF_fseed_mpi FORTRAN_CALLABLE(fseed_mpi) 
#define FNAMEOF_finit_rng FORTRAN_CALLABLE(finit_rng)
#define FNAMEOF_fspawn_rng FORTRAN_CALLABLE(fspawn_rng)
#define FNAMEOF_fget_rn_int FORTRAN_CALLABLE(fget_rn_int)
#define FNAMEOF_fget_rn_flt FORTRAN_CALLABLE(fget_rn_flt)
#define FNAMEOF_fget_rn_dbl FORTRAN_CALLABLE(fget_rn_dbl)
#define FNAMEOF_fget_seed_rng FORTRAN_CALLABLE(fget_seed_rng)
#define FNAMEOF_fpack_rng FORTRAN_CALLABLE(fpack_rng)
#define FNAMEOF_funpack_rng FORTRAN_CALLABLE(funpack_rng)
#define FNAMEOF_fprint_rng FORTRAN_CALLABLE(fprint_rng)

#define FNAMEOF_finit_rng_sim FORTRAN_CALLABLE(finit_rng_sim)
#define FNAMEOF_fget_rn_int_sim FORTRAN_CALLABLE(fget_rn_int_sim)
#define FNAMEOF_fget_rn_flt_sim FORTRAN_CALLABLE(fget_rn_flt_sim)
#define FNAMEOF_fget_rn_dbl_sim FORTRAN_CALLABLE(fget_rn_dbl_sim)
#define FNAMEOF_finit_rng_simmpi FORTRAN_CALLABLE(finit_rng_simmpi)
#define FNAMEOF_fget_rn_int_simmpi FORTRAN_CALLABLE(fget_rn_int_simmpi)
#define FNAMEOF_fget_rn_flt_simmpi FORTRAN_CALLABLE(fget_rn_flt_simmpi)
#define FNAMEOF_fget_rn_dbl_simmpi FORTRAN_CALLABLE(fget_rn_dbl_simmpi)
#define FNAMEOF_fpack_rng_simple FORTRAN_CALLABLE(fpack_rng_simple)
#define FNAMEOF_funpack_rng_simple FORTRAN_CALLABLE(funpack_rng_simple)
#define FNAMEOF_fprint_rng_simple FORTRAN_CALLABLE(fprint_rng_simple)

#define FNAMEOF_finit_rng_ptr FORTRAN_CALLABLE(finit_rng_ptr)
#define FNAMEOF_fget_rn_int_ptr FORTRAN_CALLABLE(fget_rn_int_ptr)
#define FNAMEOF_fget_rn_flt_ptr FORTRAN_CALLABLE(fget_rn_flt_ptr)
#define FNAMEOF_fget_rn_dbl_ptr FORTRAN_CALLABLE(fget_rn_dbl_ptr)
#define FNAMEOF_fpack_rng_ptr FORTRAN_CALLABLE(fpack_rng_ptr)
#define FNAMEOF_funpack_rng_ptr FORTRAN_CALLABLE(funpack_rng_ptr)
#define FNAMEOF_fprint_rng_ptr FORTRAN_CALLABLE(fprint_rng_ptr)
#define FNAMEOF_ffree_rng_ptr FORTRAN_CALLABLE(ffree_rng_ptr)
#define FNAMEOF_fspawn_rng_ptr FORTRAN_CALLABLE(fspawn_rng_ptr)

#define FNAMEOF_fcpu_t FORTRAN_CALLABLE(fcpu_t)

#endif

#endif
