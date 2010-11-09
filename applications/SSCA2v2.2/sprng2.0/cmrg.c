/*************************************************************************/
/*************************************************************************/
/*               Parallel Combined Multiple Recursive Generator          */
/*                                                                       */ 
/* Author: Ashok Srinivasan,                                             */
/*            NCSA, University of Illinois, Urbana-Champaign             */
/* E-Mail: ashoks@ncsa.uiuc.edu                                          */
/*                                                                       */ 
/* Note: This generator combines the 64 bit LCG (lcg64) with a 32 bit    */
/*         multiple recursive generator.                                 */
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

/*             This is version 1.0, created 25 May 1998                */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#define NDEBUG
#include <assert.h>
#include "memory.h"
#include "sprng_interface.h"
#include "cmrg.h"
#include "primes_64.h"
#include "store.h"


#define init_rng cmrg_init_rng
#define get_rn_int cmrg_get_rn_int
#define get_rn_flt cmrg_get_rn_flt
#define get_rn_dbl cmrg_get_rn_dbl
#define spawn_rng cmrg_spawn_rng
#define get_seed_rng cmrg_get_seed_rng
#define free_rng cmrg_free_rng
#define pack_rng cmrg_pack_rng
#define unpack_rng cmrg_unpack_rng
#define print_rng cmrg_print_rng

#define MAX_STREAMS cmrg_MAX_STREAMS
#define NGENS cmrg_NGENS
#define PARAMLIST cmrg_PARAMLIST



#define VERSION "00"
/*** Name for Generator ***/
#define GENTYPE  VERSION "Combined multiple recursive generator"

int MAX_STREAMS = (146138719);  /*** Maximum number of independent streams ***/
#define NPARAMS 3		/*** number of valid parameters ***/

#if LONG_MAX > 2147483647L 
#if LONG_MAX > 35184372088831L 
#if LONG_MAX >= 9223372036854775807L 
#define LONG_SPRNG
#define LONG64 long		/* 64 bit long */
#define store_long64 store_long
#define store_long64array store_longarray
#define load_long64 load_long
#define load_long64array load_longarray
#endif
#endif
#endif

#if !defined(LONG_SPRNG) &&  defined(_LONG_LONG)
#define LONG64 long long
#define store_long64 store_longlong
#define store_long64array store_longlongarray
#define load_long64 load_longlong
#define load_long64array load_longlongarray
#endif

unsigned int PARAMLIST[NPARAMS][2] = {{0x87b0b0fdU, 0x27bb2ee6U}, 
				      {0xe78b6955U,0x2c6fe96eU},
				      {0x31a53f85U,0x369dea0fU}};

/*** Change this to the type of generator you are implementing ***/
struct rngen
{
  int rng_type;
  char *gentype;
  int stream_number;
  int nstreams;
  int init_seed;
  int parameter;
  int narrays;
  int *array_sizes;
  int **arrays;
  int spawn_offset;
  /*** declare other variables here ***/
  unsigned int prime;
#ifdef LONG64			/* 64 bit integer types */
  unsigned LONG64 state, multiplier;
  unsigned LONG64 s0, s1, s2, s3, s4;
#else  /* No 64 bit type available, so use array of floats */
  double state[3], multiplier[3];/* simulate 64 bit arithmetic */
  int s0, s1, s2, s3, s4;
#endif
};

/*************************************************************************/
/* You should not need to look at the next few lines!                    */


#define INIT_SEED1 0x2bc6ffffU
#define INIT_SEED0 0x8cfe166dU

#define TWO_M22  2.384185791015625e-07 /* 2^(-22) */
#define TWO_P22  4194304  /* 2^(22) */
#define TWO_M20  9.5367431640625e-07 /* 2^(-20) */
#define TWO_P20  1048576 /* 2^(20) */
#define TWO_M42  2.273736754432321e-13 /* 2^(-42) */

#define TWO_M64 5.4210108624275222e-20 /* 2^(-64) */

/*                                                                      */
/************************************************************************/

int NGENS=0;		  /* number of random streams in current process */





/* Initialize random number stream */

#ifdef __STDC__
int *init_rng(int rng_type, int gennum, int total_gen,  int seed, 
	       int param)
#else
int *init_rng(rng_type,gennum,total_gen,seed,param)
int rng_type,gennum,param,seed,total_gen;
#endif
{
/*      gives back one stream (node gennum) with updated spawning         */
/*      info; should be called total_gen times, with different value      */
/*      of gennum in [0,total_gen) each call                              */
  struct rngen *genptr;
  int i;
  double tempdbl;
  
  if (total_gen <= 0) /* Is total_gen valid ? */
  {
    total_gen = 1;
    fprintf(stderr,"WARNING - init_rng: Total_gen <= 0. Default value of 1 used for total_gen\n");
  }

  if (gennum >= MAX_STREAMS) /* check if gen_num is valid    */
    fprintf(stderr,"WARNING - init_rng: gennum: %d > maximum number of independent streams: %d\n\tIndependence of streams cannot be guranteed.\n",
	    gennum, MAX_STREAMS); 

  if (gennum < 0 || gennum >= total_gen) /* check if gen_num is valid    */
  {
    fprintf(stderr,"ERROR - init_rng: gennum %d out of range [%d,%d).\n",
	    gennum, 0, total_gen); 
    return (int *) NULL;
  }

  if (param < 0 || param >= NPARAMS)     /* check if parameter is valid */
  {
    fprintf(stderr,"WARNING - init_rng: parameter not valid. Using Default parameter.\n");
    param = 0;
  }
  
  genptr = (struct rngen *) mymalloc(1*sizeof(struct rngen)); 
  if(genptr == NULL)	 /* check if memory allocated for data structure */
    return NULL;
  
  /* Initiallize data structure variables */
  genptr->rng_type = rng_type;
  genptr->gentype = GENTYPE;
  genptr->stream_number = gennum;
  genptr->nstreams = total_gen;
  genptr->init_seed = seed & 0x7fffffff;  /* Only 31 LSB of seed considered */
  genptr->parameter = param;
  genptr->spawn_offset = total_gen;
  
  /*** Change the next line depending on your generators data needs ***/
  genptr->narrays = 0;		/* number of arrays needed by your generator */

  if(genptr->narrays > 0)
  {
    genptr->array_sizes = (int *) mymalloc(genptr->narrays*sizeof(int));
    genptr->arrays = (int **) mymalloc(genptr->narrays*sizeof(int *));
    if(genptr->array_sizes == NULL || genptr->arrays == NULL)
      return NULL;
  }
  else
  {
    genptr->array_sizes = NULL;
    genptr->arrays = NULL;
  }
  
  /*** Change the next line depending on your generators data needs ***/
  		/* initiallize ...array_sizes to the sizes of the arrays */


  for(i=0; i<genptr->narrays; i++)
  {
    genptr->arrays[i] = (int *) mymalloc(genptr->array_sizes[i]*sizeof(int)); 
    if(genptr->arrays[i] == NULL)  /* check if memory allocated for data structure */
      return NULL;
  }
  
  /*** Add initialization statements for your data in the arrays and other 
    variables you have defined ***/
  getprime_64(1,&genptr->prime,gennum);
#ifdef LONG64
  genptr->multiplier = ((unsigned LONG64) PARAMLIST[param][1])<<32 |
    ((unsigned LONG64) PARAMLIST[param][0]);
  genptr->state = ( ((unsigned LONG64) INIT_SEED1)<<32 | INIT_SEED0)
      ^(((unsigned LONG64) seed<<33)|gennum);
#else
  genptr->multiplier[0] = (double) (PARAMLIST[param][0]&0x3fffff);
  genptr->multiplier[1] = (double) 
    (PARAMLIST[param][0]>>22 | (PARAMLIST[param][1]&0xfff)<<10);
  genptr->multiplier[2] = (double) (PARAMLIST[param][1]>>12);
  genptr->state[0] = (double) ((INIT_SEED0^gennum)&0x3fffff);
  genptr->state[1] = (double) 
    ((INIT_SEED0^gennum)>>22 | ((INIT_SEED1 ^ (unsigned)seed<<1)&0xfff)<<10);
  genptr->state[2] = (double) ((INIT_SEED1 ^ (unsigned)seed<<1)>>12);
#endif  
  genptr->s0 = genptr->s1 = genptr->s2 = genptr->s3 = genptr->s4 = /*0xfffffff*/ 0x1;

  for(i=0; i<127*genptr->stream_number; i++)
    tempdbl = get_rn_dbl((int *) genptr); 
  
  NGENS++;			/* NGENS = # of streams */
  
  return (int *) genptr;
} 


#ifdef LONG64
#define advance_state(genptr)   genptr->state = genptr->state*genptr->multiplier + genptr->prime;
#else
#define advance_state(genptr)  {double t0, t1, t2, t3, st0, st1, st2;\
	      t0 = genptr->state[0]*genptr->multiplier[0] + genptr->prime;\
              t1 = (double) (int) (t0*TWO_M22); \
              st0 = t0 - TWO_P22*t1; \
              assert( (int) st0 == st0); \
              t1 += genptr->state[1]*genptr->multiplier[0] + \
                 genptr->state[0]*genptr->multiplier[1]; \
              t2 = (double) (int) (t1*TWO_M22); \
              st1 = t1 - TWO_P22*t2; \
              assert( (int) st1 == st1); \
              t2 += genptr->state[2]*genptr->multiplier[0] + \
                    genptr->state[1]*genptr->multiplier[1] + \
                    genptr->state[0]*genptr->multiplier[2];\
              t3 = (double) (int) (t2*TWO_M20); \
              st2 = t2 - TWO_P20*t3; \
              assert( (int) st2 == st2); \
              genptr->state[0] = st0; \
              genptr->state[1] = st1; \
              genptr->state[2] = st2;}
#endif

#define m1 2147483647
#define a0 107374182
#define a4 104480
#define q0 20
#define q4 20554
#define r0 7
#define r4 1727
#ifdef LONG64
#define advance_cmrg(genptr)  {unsigned LONG64 p;\
      advance_state(genptr);\
      p = a0*genptr->s0 + a4*genptr->s4;\
      p = (p>>31) + (p&0x7fffffff);\
      if(p&0x80000000) p = (p+1)&0x7fffffff;\
      genptr->s4=genptr->s3; genptr->s3=genptr->s2;genptr->s2=genptr->s1;genptr->s1=genptr->s0;genptr->s0=p;}
#else
#define advance_cmrg(genptr)  {int h, p0, p4;\
      advance_state(genptr);\
      h=genptr->s4/q4;p4=a4*(genptr->s4-h*q4)-h*r4;\
      genptr->s4=genptr->s3;genptr->s3=genptr->s2;genptr->s2=genptr->s1;genptr->s1=genptr->s0;\
      h=genptr->s0/q0; p0=a0*(genptr->s0-h*q0)-h*r0;\
      if(p0<0) p0+=m1; if(p4>0)p4-=m1;\
      genptr->s0=p0+p4; if(genptr->s0<0) genptr->s0+=m1;}
#endif

/* Returns a double precision random number */

#ifdef __STDC__
double get_rn_dbl(int *igenptr)
#else
double get_rn_dbl(igenptr)
int *igenptr;
#endif
{
  struct rngen *genptr = (struct rngen *) igenptr;
 
#ifdef LONG64
#ifdef _LONG_LONG
#define EXPO 0x3ff0000000000000ULL
#define MULT_MASK 0xffffffffffffffffULL

#else
#define EXPO 0x3ff0000000000000UL
#define MULT_MASK 0xffffffffffffffffUL
#endif
  static double dtemp[1] = {0.0};

  advance_cmrg(genptr);

#if defined(CONVEX) || defined(O2K) || defined(SGI) || defined(GENERIC)
  *((unsigned LONG64 *) dtemp) = (((genptr->state+(genptr->s0<<32))&MULT_MASK)>>12) | EXPO;
  return *dtemp - (double) 1.0;
#else
  return ((genptr->state+(genptr->s0<<32))&MULT_MASK)*TWO_M64;
#endif
  

#else  /* 32 bit machine */
#define EXPO 0x3ff00000
#ifdef LittleEndian
#define MSB 1
#else
#define MSB 0
#endif
#define LSB (1-MSB)
  double ans;
  unsigned int ist0, ist1, ist2, tempi, tempi2;
  static double temp[1] = {0.0};

  advance_cmrg(genptr);
  ist0 = genptr->state[0];
  ist1 = genptr->state[1];
  ist2 = genptr->state[2];
  tempi2 = ist2<<12 | ist1>>10;
  tempi  = (genptr->s0+tempi2)&0xffffffff; /* add modulo 2^64 */
  ist1 = (ist1&0x3ff) | (tempi&0xfff)<<10;
  ist2 = tempi>>12;

#if defined(HP) || defined(SUN) || defined(SOLARIS) || defined(GENERIC)
/*IEEE mantissa is 52 bits. */
    ((unsigned int *)temp)[LSB] = (ist1<<10 | ist0>>12);
    ((unsigned int *)temp)[MSB] = EXPO | ist2;

  return *temp - (double) 1.0;
#else
  return ist2*TWO_M20 + ist1*TWO_M42 + ist0*TWO_M64;
#endif

#endif		
} 



/* Return a random integer */

#ifdef __STDC__
int get_rn_int(int *igenptr)
#else
int get_rn_int(igenptr)
int *igenptr;
#endif
{
#ifdef LONG64
  struct rngen *genptr = (struct rngen *) igenptr;
  advance_cmrg(genptr);
  return ((genptr->state+(genptr->s0<<32))&MULT_MASK)>>33;
#else
  return (int) (get_rn_dbl(igenptr)*0x80000000U);
#endif

} 



/* Return a single precision random number */

#ifdef __STDC__
float get_rn_flt(int *igenptr)
#else
float get_rn_flt(igenptr)
int *igenptr;
#endif
{
  /* If you have a more efficient way of computing the random integer,
     then please replace the statement below with your scheme.        */

    return (float) get_rn_dbl(igenptr);
}

#undef m1
#undef a0
#undef a4
#undef q0
#undef q4
#undef r0
#undef r4




/*************************************************************************/
/*************************************************************************/
/*                  SPAWN_RNG: spawns new generators                     */
/*************************************************************************/
/*************************************************************************/

#ifdef __STDC__
int spawn_rng(int *igenptr, int nspawned, int ***newgens, int checkid)
#else
int spawn_rng(igenptr,nspawned, newgens, checkid)
int *igenptr,nspawned, ***newgens, checkid;
#endif
{
  struct rngen **genptr, *tempptr = (struct rngen *) igenptr;
  int i, j;
  
  if (nspawned <= 0) /* is nspawned valid ? */
  {
    nspawned = 1;
    fprintf(stderr,"WARNING - spawn_rng: nspawned <= 0. Default value of 1 used for nspawned\n");
  }
  
  genptr = (struct rngen **) mymalloc(nspawned*sizeof(struct rngen *));
  if(genptr == NULL)	   /* allocate memory for pointers to structures */
  {
    *newgens = NULL;
    return 0;
  }
  
  for(i=0; i<nspawned; i++)	/* create nspawned new streams */
  {
    int seed, gennum;
    
    gennum = tempptr->stream_number + tempptr->spawn_offset*(i+1);
  
    if(gennum > MAX_STREAMS)   /* change seed to avoid repeating sequence */
      seed = (tempptr->init_seed)^gennum; 
    else
      seed = tempptr->init_seed;
    
    /* Initialize a stream. This stream has incorrect spawning information.
       But we will correct it below. */

    genptr[i] = (struct rngen *) 
      init_rng(tempptr->rng_type,gennum, gennum+1, seed, tempptr->parameter);
    
  
    if(genptr[i] == NULL)	/* Was generator initiallized? */
    {
      nspawned = i;
      break;
    }
    genptr[i]->spawn_offset = (nspawned+1)*tempptr->spawn_offset;
  }
  
  tempptr->spawn_offset *= (nspawned+1);
  

  *newgens = (int **) genptr;
  
  
  if(checkid != 0)
    for(i=0; i<nspawned; i++)
      if(addID(( int *) genptr[i]) == NULL)
	return i;
  
  return nspawned;
}


/* Free memory allocated for data structure associated with stream */

#ifdef __STDC__
int free_rng(int *genptr)
#else
int free_rng(genptr)
int *genptr;
#endif
{
  struct rngen *q;
  int i;
  
  q = (struct rngen *) genptr;
  assert(q != NULL);
  
  for(i=0; i<q->narrays; i++)
    free(q->arrays[i]);

  if(q->narrays > 0)
  {
    free(q->array_sizes);
    free(q->arrays);
  }
  
  free(q);

  NGENS--;
  return NGENS;
}


#ifdef __STDC__
int pack_rng( int *genptr, char **buffer)
#else
int pack_rng(genptr,buffer)
int *genptr;
char **buffer;
#endif
{
  unsigned char *p, *initp;
  int size, i;
  unsigned int temp, m[2];
  struct rngen *q;

  q = (struct rngen *) genptr;
  size = 4 + 64 + strlen(q->gentype)+1;
  
  initp = p = (unsigned char *) mymalloc(size); /* allocate memory */
  /* The new load/store routines make using sizeof unnecessary. Infact, */
  /* using sizeof could be erroneous. */
  if(p == NULL)
  {
    *buffer = NULL;
    return 0;
  }
  
  
  p += store_int(q->rng_type,4,p);
  strcpy((char *)p,q->gentype);
  p += strlen(q->gentype)+1;
  p += store_int(q->stream_number,4,p);
  p += store_int(q->nstreams,4,p);
  p += store_int(q->init_seed,4,p);
  p += store_int(q->parameter,4,p);
  p += store_int(q->narrays,4,p);
  p += store_int(q->spawn_offset,4,p);
  p += store_int(q->prime,4,p);
#ifdef LONG64			/* 64 bit integer types */
  p += store_long64(q->state,8,p);
  p += store_long64(q->multiplier,8,p);
  p += store_long64(q->s0,4,p);
  p += store_long64(q->s1,4,p);
  p += store_long64(q->s2,4,p);
  p += store_long64(q->s3,4,p);
  p += store_long64(q->s4,4,p);
#else  /* No 64 bit type available */
  m[0] = q->state[2]; temp = q->state[1];m[0]=(m[0]<<12)|(temp>>10);
  m[1] = q->state[1]; temp = q->state[0];m[1]=(m[1]<<22)|(temp);
  p += store_intarray(m,2,4,p);
  m[0] = q->multiplier[2]; temp = q->multiplier[1];m[0]=(m[0]<<12)|(temp>>10);
  m[1] = q->multiplier[1]; temp = q->multiplier[0];m[1]=(m[1]<<22)|(temp);
  p += store_intarray(m,2,4,p);
  p += store_int(*(unsigned int *)&q->s0,4,p);
  p += store_int(*(unsigned int *)&q->s1,4,p);
  p += store_int(*(unsigned int *)&q->s2,4,p);
  p += store_int(*(unsigned int *)&q->s3,4,p);
  p += store_int(*(unsigned int *)&q->s4,4,p);
#endif
  
  
  *buffer =  (char *) initp;
  assert(p-initp == size);
  return p-initp;
}



#ifdef __STDC__
int *unpack_rng( char *packed)
#else
int *unpack_rng(packed)
char *packed;
#endif
{
  struct rngen *q;
  unsigned int i, m[2];
  unsigned char *p;

  p = (unsigned char *) packed;
  q = (struct rngen *) mymalloc(sizeof(struct rngen));
  if(q == NULL)
    return NULL;

  p += load_int(p,4,(unsigned int *)&q->rng_type);
  if(strcmp((char *)p,GENTYPE) != 0)
  {
    fprintf(stderr,"ERROR: Unpacked ' %.24s ' instead of ' %s '\n",  
	    p, GENTYPE); 
    return NULL; 
  }
  else
    q->gentype = GENTYPE;
  p += strlen(q->gentype)+1;

  p += load_int(p,4,(unsigned int *)&q->stream_number);
  p += load_int(p,4,(unsigned int *)&q->nstreams);
  p += load_int(p,4,(unsigned int *)&q->init_seed);
  p += load_int(p,4,(unsigned int *)&q->parameter);
  p += load_int(p,4,(unsigned int *)&q->narrays);
  p += load_int(p,4,(unsigned int *)&q->spawn_offset);
  p += load_int(p,4,&q->prime);
#ifdef LONG64			/* 64 bit integer types */
  p += load_long64(p,8,&q->state);
  p += load_long64(p,8,&q->multiplier);
  p += load_long64(p,4,&q->s0);
  p += load_long64(p,4,&q->s1);
  p += load_long64(p,4,&q->s2);
  p += load_long64(p,4,&q->s3);
  p += load_long64(p,4,&q->s4);
#else  /* No 64 bit type available */
  p += load_intarray(p,2,4,m);
  q->state[0] = (double) (m[1]&0x3fffff);
  q->state[1] = (double) ((m[1]>>22) | (m[0]&0xfff)<<10);
  q->state[2] = (double) (m[0]>>12);
  p += load_intarray(p,2,4,m);
  q->multiplier[0] = (double) (m[1]&0x3fffff);
  q->multiplier[1] = (double) ((m[1]>>22) | (m[0]&0xfff)<<10);
  q->multiplier[2] = (double) (m[0]>>12);
  p += load_int(p,4,(unsigned int *)&q->s0);
  p += load_int(p,4,(unsigned int *)&q->s1);
  p += load_int(p,4,(unsigned int *)&q->s2);
  p += load_int(p,4,(unsigned int *)&q->s3);
  p += load_int(p,4,(unsigned int *)&q->s4);
#endif

    
  q->array_sizes = NULL;
  q->arrays = NULL;
      
  NGENS++;
  
  return (int *) q;
}

      

#ifdef __STDC__
int get_seed_rng(int *gen)
#else
int get_seed_rng(gen)
int *gen;
#endif
{
  return ((struct rngen *) gen)->init_seed;
}



#ifdef __STDC__
int print_rng( int *igen)
#else
int print_rng(igen)
int *igen;
#endif
{
  struct rngen *gen;
  
  printf("\n%s\n", GENTYPE+2);
  
  gen = (struct rngen *) igen;
  printf("\n \tseed = %d, stream_number = %d\tparameter = %d\n\n", gen->init_seed, gen->stream_number, gen->parameter);

  return 1;
}


