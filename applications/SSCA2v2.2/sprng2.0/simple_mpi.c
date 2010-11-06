/*************************************************************************/
/*************************************************************************/
/* LINEAR CONGRUENTIAL RANDOM NUMBER GENERATION WITH PRIME ADDEND        */
/*                                                                       */ 
/* Author: Ashok Srinivasan,                                             */
/*            NCSA, University of Illinois, Urbana-Champaign             */
/* E-Mail: ashoks@ncsa.uiuc.edu                                          */
/*                                                                       */ 
/* Note: This generator is based on the Cray YMP compatible random number*/ 
/* generator for 32-bit IEEE machines by William Magro, Cornell Theory   */
/* Center                                                                */
/*                                                                       */
/* Disclaimer: NCSA expressly disclaims any and all warranties, expressed*/
/* or implied, concerning the enclosed software.  The intent in sharing  */
/* this software is to promote the productive interchange of ideas       */
/* throughout the research community. All software is furnished on an    */
/* "as is" basis. No further updates to this software should be          */
/* expected. Although this may occur, no commitment exists. The authors  */
/* certainly invite your comments as well as the reporting of any bugs.  */
/* NCSA cannot commit that any or all bugs will be fixed.                */
/*************************************************************************/
/*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include "sprng.h"

#ifndef ANSI_ARGS
#ifdef __STDC__
#define ANSI_ARGS(args) args
#else
#define ANSI_ARGS(args) ()
#endif
#endif

extern int *defaultgen;
int junkmpi;			/* pass useless pointer at times */


#ifdef SPRNG_MPI
#ifdef __STDC__
int *init_rng_simple_mpi(int rng_type, int seed,  int mult)
#else
int *init_rng_simple_mpi(rng_type,seed,mult)
int rng_type,mult,seed;
#endif
{
  int myid=0, nprocs=1, *temp;
  
  get_proc_info_mpi(&myid,&nprocs);

  temp = init_rng(rng_type,myid,nprocs,seed,mult);

  if(temp == NULL)
    return NULL;
  else
  {
    if(defaultgen != NULL)
      free_rng(defaultgen);
    defaultgen = temp;
    return &junkmpi;		/* return "garbage" value */
  }
} 


#ifdef __STDC__
int get_rn_int_simple_mpi(void)
#else
int get_rn_int_simple_mpi()
#endif
{
  if(defaultgen == NULL)
    if(init_rng_simple_mpi(DEFAULT_RNG_TYPE,0,0) == NULL)
      return -1.0;
  
  return get_rn_int(defaultgen);
} 


#ifdef __STDC__
float get_rn_flt_simple_mpi(void)
#else
float get_rn_flt_simple_mpi()
#endif
{
  if(defaultgen == NULL)
    if(init_rng_simple_mpi(DEFAULT_RNG_TYPE,0,0) == NULL)
      return -1.0;
  
  return get_rn_flt(defaultgen);
} 



#ifdef __STDC__
double get_rn_dbl_simple_mpi(void)
#else
double get_rn_dbl_simple_mpi()
#endif
{
  if(defaultgen == NULL)
    if(init_rng_simple_mpi(DEFAULT_RNG_TYPE,0,0) == NULL)
      return -1.0;
  
  return get_rn_dbl(defaultgen);
} 

#endif

