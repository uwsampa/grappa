/*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "memory.h"
#include "sprng_interface.h"

#ifndef ANSI_ARGS
#ifdef __STDC__
#define ANSI_ARGS(args) args
#else
#define ANSI_ARGS(args) ()
#endif
#endif


int *defaultgen=NULL;
int junk;			/* pass useless pointer at times */

#ifdef __STDC__
int *init_rng_simple(int rng_type, int seed,  int mult)
#else
int *init_rng_simple(rng_type,seed,mult)
int rng_type,mult,seed;
#endif
{
  int myid=0, nprocs=1, *temp;
  
  temp = init_rng(rng_type,myid,nprocs,seed,mult);

  if(temp == NULL)
    return NULL;
  else
  {
    if(defaultgen != NULL)
      free_rng(defaultgen);
    defaultgen = temp;
    return &junk;		/* return "garbage" value */
  }
} 





#ifdef __STDC__
int get_rn_int_simple(void)
#else
int get_rn_int_simple()
#endif
{
  if(defaultgen == NULL)
    if(init_rng_simple(DEFAULT_RNG_TYPE,0,0) == NULL)
      return -1.0;
  
  return get_rn_int(defaultgen);
} 




#ifdef __STDC__
float get_rn_flt_simple(void)
#else
float get_rn_flt_simple()
#endif
{
  if(defaultgen == NULL)
    if(init_rng_simple(DEFAULT_RNG_TYPE,0,0) == NULL)
      return -1.0;
  
  return get_rn_flt(defaultgen);
} 


#ifdef __STDC__
double get_rn_dbl_simple(void)
#else
double get_rn_dbl_simple()
#endif
{
  if(defaultgen == NULL)
    if(init_rng_simple(DEFAULT_RNG_TYPE,0,0) == NULL)
      return -1.0;
  
  return get_rn_dbl(defaultgen);
} 





#ifdef __STDC__
int pack_rng_simple(char **buffer)
#else
int pack_rng_simple(buffer)
char **buffer;
#endif
{
  if(defaultgen == NULL)
    return 0;
  
  return pack_rng(defaultgen,buffer);
}



#ifdef __STDC__
int *unpack_rng_simple( char *packed)
#else
int *unpack_rng_simple(packed)
char *packed;
#endif
{
  int *temp;
  
  temp = unpack_rng(packed);
  
  if(temp == NULL)
    return NULL;
  else
  {
    if(defaultgen != NULL)
      free_rng(defaultgen);
    defaultgen = temp;
    return &junk;		/* return "garbage" value */
  }
}

      

#ifdef __STDC__
int print_rng_simple(void)
#else
int print_rng_simple()
#endif
{
  if(defaultgen == NULL)
  {
    fprintf(stderr,"WARNING: No generator initialized so far\n");
    return 0;
  }
  
  return print_rng(defaultgen);
}

