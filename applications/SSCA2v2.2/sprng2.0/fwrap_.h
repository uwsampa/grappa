#include "fwrap.h"
/************************************************************************/
/************************************************************************/
/*									*/
/*	This package of C wrappers is intended to be called from a 	*/
/*	FORTRAN program. The main purpose of the package is to mediate	*/
/*	between the call-by-address and call-by-value conventions in	*/
/*	the two languages. In most cases, the arguments of the C	*/
/*	routines and the wrappers are the same.                         */
/*									*/
/*	The wrappers should be treated as FORTRAN function calls.	*/
/*									*/
/* Note: This code is a header file to facilitte inlining.              */
/************************************************************************/
/************************************************************************/


#ifdef __STDC__
int FNAMEOF_fget_seed_rng(int **genptr)
#else
int FNAMEOF_fget_seed_rng(genptr)
int **genptr; 
#endif
{
  return get_seed_rng(*genptr);
}



#ifdef __STDC__
int FNAMEOF_ffree_rng_ptr(int **genptr)
#else
int FNAMEOF_ffree_rng_ptr(genptr)
int **genptr; 
#endif
{
  if (deleteID(*genptr) == NULL)
    return -1;

  return free_rng(*genptr);
}

#ifdef __STDC__
int FNAMEOF_ffree_rng(int **genptr)
#else
int FNAMEOF_ffree_rng(genptr)
int **genptr; 
#endif
{
  return free_rng(*genptr);
}


#ifdef __STDC__
int FNAMEOF_fmake_new_seed(void)
#else
int FNAMEOF_fmake_new_seed()
#endif
{
  return make_new_seed();
}

#ifdef __STDC__
int * FNAMEOF_finit_rng_sim(int *rng_type, int *seed,  int *mult)
#else
int * FNAMEOF_finit_rng_sim(rng_type, seed,mult)
int *rng_type, *mult,*seed;
#endif
{
	return init_rng_simple(*rng_type, *seed, *mult);
}



#ifdef __STDC__
int * FNAMEOF_finit_rng(int *rng_type, int *gennum, int *total_gen, int *seed,
			 int *length)
#else
int * FNAMEOF_finit_rng(rng_type, gennum, total_gen, seed, length)
int *rng_type, *gennum, *length, *seed, *total_gen;
#endif
{
	return init_rng(*rng_type, *gennum, *total_gen, *seed, *length);
}

#ifdef __STDC__
int * FNAMEOF_finit_rng_ptr(int *rng_type, int *gennum,  int *total_gen,
			     int *seed,  int *length)
#else
int * FNAMEOF_finit_rng_ptr(rng_type, gennum, total_gen, seed, length)
int *rng_type, *gennum, *length, *seed, *total_gen;
#endif
{
  int *tmpGen;
  tmpGen = init_rng(*rng_type, *gennum, *total_gen, *seed, *length);
  addID(tmpGen);
  return tmpGen;
}

#ifdef __STDC__
int FNAMEOF_fspawn_rng(int **genptr,  int *nspawned, int **newGen,  int checkid)
#else
int FNAMEOF_fspawn_rng(genptr, nspawned, newGen, checkid)
int **genptr, *nspawned, **newGen, checkid;
#endif
{
  int i, **tmpGen, n;

  n =  spawn_rng(*genptr, *nspawned, &tmpGen, checkid);
  for (i=0; i< n; i++)
    newGen[i] = tmpGen[i];
  if(n != 0)
    free( tmpGen); 

  return n;
}


#ifdef __STDC__
int FNAMEOF_fspawn_rng_ptr(int **genptr,  int *nspawned, int **newGen, 
			    int checkid)
#else
int FNAMEOF_fspawn_rng_ptr(genptr, nspawned, newGen, checkid)
int **genptr, *nspawned, **newGen, checkid;
#endif
{
  int i, **tmpGen, n;

  if (checkID(*genptr) == NULL)
    return 0;

  n =  spawn_rng(*genptr, *nspawned, &tmpGen, checkid);
  for (i=0; i< n; i++)
    newGen[i] = tmpGen[i];
  if(n != 0)
    free( tmpGen); 

  return n;
}

#ifdef __STDC__
int FNAMEOF_fget_rn_int_sim(void)
#else
int FNAMEOF_fget_rn_int_sim()
#endif
{
  return get_rn_int_simple();
}



#ifdef __STDC__
int FNAMEOF_fget_rn_int(int **genptr)
#else
int FNAMEOF_fget_rn_int(genptr)
int **genptr;
#endif
{
	return get_rn_int(*genptr);
}

#ifdef __STDC__
int FNAMEOF_fget_rn_int_ptr(int **genptr)
#else
int FNAMEOF_fget_rn_int_ptr(genptr)
int **genptr;
#endif
{
  if (checkID(*genptr)==NULL) 
    return -1;

  return get_rn_int(*genptr);
}



#ifdef __STDC__
float FNAMEOF_fget_rn_flt_sim(void)
#else
float FNAMEOF_fget_rn_flt_sim()
#endif
{
  return get_rn_flt_simple();
}



#ifdef __STDC__
float FNAMEOF_fget_rn_flt(int **genptr)
#else
float FNAMEOF_fget_rn_flt(genptr)
int **genptr;
#endif
{
	return get_rn_flt(*genptr);
}

#ifdef __STDC__
float FNAMEOF_fget_rn_flt_ptr(int **genptr)
#else
float FNAMEOF_fget_rn_flt_ptr(genptr)
int **genptr;
#endif
{
  if (checkID(*genptr)==NULL) 
    return -1.0;

  return get_rn_flt(*genptr);
}


#ifdef __STDC__
double FNAMEOF_fget_rn_dbl_sim(void)
#else
double FNAMEOF_fget_rn_dbl_sim()
#endif
{
  return get_rn_dbl_simple();
}




#ifdef __STDC__
double FNAMEOF_fget_rn_dbl(int **genptr)
#else
double FNAMEOF_fget_rn_dbl(genptr)
int **genptr;
#endif
{
	return get_rn_dbl(*genptr);
}

#ifdef __STDC__
double FNAMEOF_fget_rn_dbl_ptr(int **genptr)
#else
double FNAMEOF_fget_rn_dbl_ptr(genptr)
int **genptr;
#endif
{
  if ( checkID(*genptr)==NULL) 
    return -1.0;

  return get_rn_dbl(*genptr);
}


#ifdef __STDC__
int FNAMEOF_fpack_rng( int **genptr, char *buffer)
#else
int FNAMEOF_fpack_rng(genptr, buffer)
int **genptr;
char *buffer;
#endif
{
  int size;
  char *temp;

  size = pack_rng(*genptr, &temp);
  if(temp != NULL)
  {
    memcpy(buffer,temp,size);
    free(temp);
  }
  
  return size;
}

#ifdef __STDC__
int FNAMEOF_fpack_rng_ptr( int **genptr, char *buffer)
#else
int FNAMEOF_fpack_rng_ptr(genptr, buffer)
int **genptr;
char *buffer;
#endif
{
  int size;
  char *temp;

  if( checkID(*genptr)==NULL)
    return 0;

  size = pack_rng(*genptr, &temp);
  if(temp != NULL)
  {
    memcpy(buffer,temp,size);
    free(temp);
  }


  return size;
}


#ifdef __STDC__
int FNAMEOF_fpack_rng_simple(char *buffer)
#else
int FNAMEOF_fpack_rng_simple(buffer)
char *buffer;
#endif
{
  int size;
  char *temp;
	
  size = pack_rng_simple(&temp);
  if(temp != NULL)
  {
    memcpy(buffer,temp,size);
    free(temp);
  }
  
  return size;
}


#ifdef __STDC__
int * FNAMEOF_funpack_rng( char *buffer)
#else
int * FNAMEOF_funpack_rng(buffer)
char *buffer;
#endif
{
  return    unpack_rng(buffer);
}

#ifdef __STDC__
int * FNAMEOF_funpack_rng_ptr( char *buffer)
#else
int * FNAMEOF_funpack_rng_ptr(buffer)
char *buffer;
#endif
{
  int *tmpGen;
  tmpGen = unpack_rng(buffer);
  addID(tmpGen);
  return tmpGen;
}


#ifdef __STDC__
int * FNAMEOF_funpack_rng_simple( char *buffer)
#else
int * FNAMEOF_funpack_rng_simple(buffer)
char *buffer;
#endif
{
  return unpack_rng_simple(buffer);
}

/* 11/15/96 J.J.: split into two cases: general print_rng, and print_rng_ptr
#ifdef __STDC__
int FNAMEOF_fprint_rng( int **genptr,int checkid)
#else
int FNAMEOF_fprint_rng(genptr, checkid)
int **genptr, checkid;
#endif
{
  if(checkid != 0)
    if(checkID(*genptr) == NULL)
      return 0;
  
  return print_rng(*genptr);
}
*/


#ifdef __STDC__
int FNAMEOF_fprint_rng( int **genptr)
#else
int FNAMEOF_fprint_rng(genptr)
int **genptr;
#endif
{
  return print_rng(*genptr);
}


#ifdef __STDC__
int FNAMEOF_fprint_rng_simple(void)
#else
int FNAMEOF_fprint_rng_simple()
#endif
{
  return print_rng_simple();
}

#ifdef __STDC__
int FNAMEOF_fprint_rng_ptr( int **genptr)
#else
int FNAMEOF_fprint_rng_ptr(genptr)
int **genptr;
#endif
{
  if(checkID(*genptr) == NULL)
    return 0;
  
  return print_rng(*genptr);
}

