/*************************************************************************/
/*************************************************************************/
/*               Parallel 48 bit Linear Congruential Generator           */
/*                                                                       */ 
/* Author: Ashok Srinivasan,                                             */
/*            NCSA, University of Illinois, Urbana-Champaign             */
/* E-Mail: ashoks@ncsa.uiuc.edu                                          */
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

/*             This is version 0.2, created 13 April 1998                */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "memory.h"
#include "primes_32.h"
#include "sprng_interface.h"
#include "lcg.h"
#include <limits.h>
#define NDEBUG
#include <assert.h>
#include "store.h"

#define init_rng lcg_init_rng
#define get_rn_int lcg_get_rn_int
#define get_rn_flt lcg_get_rn_flt
#define get_rn_dbl lcg_get_rn_dbl
#define spawn_rng lcg_spawn_rng
#define get_seed_rng lcg_get_seed_rng
#define free_rng lcg_free_rng
#define print_rng lcg_print_rng
#define pack_rng lcg_pack_rng
#define unpack_rng lcg_unpack_rng

#define MAX_STREAMS lcg_MAX_STREAMS
#define NGENS lcg_NGENS


#define VERSION "00"
#define GENTYPE  VERSION "48 bit Linear Congruential Generator with Prime Addend"

#if LONG_MAX > 2147483647L	/* 32 bit integer */
#if LONG_MAX > 35184372088831L	/* 46 bit integer */
#if LONG_MAX >= 9223372036854775807L /* 64 bit integer */
#define LONG_SPRNG
#define LONG64 long
#define store_long64 store_long
#define store_long64array store_longarray
#define load_long64 load_long
#define load_long64array load_longarray

#define INIT_SEED 0x2bc68cfe166dL
#define MSB_SET 0x3ff0000000000000L
#define LSB48 0xffffffffffffL
#define AN1 0xdadf0ac00001L
#define AN2 0xfefd7a400001L
#define AN3 0x6417b5c00001L
#define AN4 0xcf9f72c00001L
#define AN5 0xbdf07b400001L
#define AN6 0xf33747c00001L
#define AN7 0xcbe632c00001L
#define PMULT1 0xa42c22700000L
#define PMULT2 0xfa858cb00000L
#define PMULT3 0xd0c4ef00000L
#define PMULT4 0xc3cc8e300000L
#define PMULT5 0x11bdbe700000L
#define PMULT6 0xb0f0e9f00000L
#define PMULT7 0x6407de700000L
#define MULT1 0x2875a2e7b175L	/* 44485709377909  */
#define MULT2 0x5deece66dL	/* 1575931494      */
#define MULT3 0x3eac44605265L	/* 68909602460261  */
#define MULT4 0x275b38eb4bbdL	/* 4327250451645   */
#define MULT5 0x1ee1429cc9f5L	/* 33952834046453  */
#define MULT6 0x739a9cb08605L	/* 127107890972165 */
#define MULT7 0x3228d7cc25f5L	/* 55151000561141  */
#endif
#endif
#endif

#if !defined(LONG_SPRNG) &&  defined(_LONG_LONG)
#define LONG64 long long
#define store_long64 store_longlong
#define store_long64array store_longlongarray
#define load_long64 load_longlong
#define load_long64array load_longlongarray

#define INIT_SEED 0x2bc68cfe166dLL
#define MSB_SET 0x3ff0000000000000LL
#define LSB48 0xffffffffffffLL
#define AN1 0xdadf0ac00001LL
#define AN2 0xfefd7a400001LL
#define AN3 0x6417b5c00001LL
#define AN4 0xcf9f72c00001LL
#define AN5 0xbdf07b400001LL
#define AN6 0xf33747c00001LL
#define AN7 0xcbe632c00001LL
#define PMULT1 0xa42c22700000LL
#define PMULT2 0xfa858cb00000LL
#define PMULT3 0xd0c4ef00000LL
#define PMULT4 0x11bdbe700000LL
#define PMULT5 0xc3cc8e300000LL
#define PMULT6 0xb0f0e9f00000LL
#define PMULT7 0x6407de700000LL
#define MULT1 0x2875a2e7b175LL
#define MULT2 0x5deece66dLL
#define MULT3 0x3eac44605265LL
#define MULT4 0x1ee1429cc9f5LL
#define MULT5 0x275b38eb4bbdLL
#define MULT6 0x739a9cb08605LL
#define MULT7 0x3228d7cc25f5LL
#endif

#define TWO_M24 5.96046447753906234e-8
#define TWO_M48 3.5527136788005008323e-15

#include "multiply.h"

#ifdef LittleEndian
#define MSB 1
#else
#define MSB 0
#endif
#define LSB (1-MSB)

#define LCGRUNUP 29

int MAX_STREAMS = 1<<19;

#ifndef TOOMANY
#define TOOMANY "generator has branched maximum number of times;\nindependence of streams cannot be guranteed\n"
#endif

#ifdef LONG64
struct rngen
{
  int rng_type;
  unsigned LONG64 seed;
  int init_seed;
  int prime;
  int prime_position;
  int prime_next;
  char *gentype;
  int parameter;
  unsigned LONG64 multiplier;
};

unsigned LONG64 mults[] = {MULT1,MULT2,MULT3,MULT4,MULT5,MULT6,MULT7};
unsigned LONG64 multiplier=0;

#else
struct rngen
{
  int rng_type;
  int seed[2];
  int init_seed;
  int prime;
  int prime_position;
  int prime_next;
  char *gentype;
  int parameter;
  int *multiplier;
};

int mults[][4] = {{0x175,0xe7b,0x5a2,0x287},{0x66d,0xece,0x5de,0x000},
		  {0x265,0x605,0xc44,0x3ea},{0x9f5,0x9cc,0x142,0x1ee},
		  {0xbbd,0xeb4,0xb38,0x275},{0x605,0xb08,0xa9c,0x739},
		  {0x5f5,0xcc2,0x8d7,0x322}};
int *multiplier=NULL;
#endif

#define NPARAMS 7

int NGENS=0;

void plus ANSI_ARGS((int *a, int *b, int *c));
void mult ANSI_ARGS((int *a, int *b, int *c, int size));
void advance_seed ANSI_ARGS((struct rngen *gen));
double get_rn_dbl ANSI_ARGS((int *gen));

#ifdef __STDC__
int bit_reverse(int n)
#else
int bit_reverse(n)
int n;
#endif
{
  int i=31, rev=0;
  
  for(i=30; i>=0; i--)
  {
    rev |= (n&1)<<i;
    n >>= 1;
  }
  
  return rev;
}


#ifdef __STDC__
void errprint(char *level, char *routine, char *error)
#else
void errprint(level, routine, error)
char *level,*routine,*error;
#endif
{
#ifdef CRAY
#pragma _CRI guard 63
#endif
      fprintf(stderr,"%s from %s: %s\n",level,routine,error);
#ifdef CRAY
#pragma _CRI endguard 63
#endif
}


#ifdef __STDC__
int strxncmp(char *s1, char *s2, int n)
#else
int strxncmp(s1, s2, n)
char *s1, *s2;
int n;
#endif
{
  int i;
  
  for(i=0; i<n; i++)
    if(s1[i] != s2[i])
      return s1[i]-s2[i];
  
  return 0;			/* First n characters of strings are equal. */
}



#ifdef __STDC__
int *init_rng(int rng_type, int gennum,  int total_gen,  int seed, int mult)
#else
int *init_rng(rng_type,gennum,total_gen,seed,mult)
int rng_type,gennum,mult,seed,total_gen;
#endif
{
/*      gives back one generator (node gennum) with updated spawning     */
/*      info; should be called total_gen times, with different value     */
/*      of gennum in [0,total_gen) each call                             */
  struct rngen *genptr;
  int i;
  
  if (total_gen <= 0) /* check if total_gen is valid */
  {
    total_gen = 1;
    errprint("WARNING","init_rng","Total_gen <= 0. Default value of 1 used for total_gen");
  }

  if (gennum >= MAX_STREAMS) /* check if gen_num is valid    */
    fprintf(stderr,"WARNING - init_rng: gennum: %d > maximum number of independent streams: %d\n\tIndependence of streams cannot be guranteed.\n",
	    gennum, MAX_STREAMS); 

  if (gennum < 0 || gennum >= total_gen) /* check if gen_num is valid */
  {
    errprint("ERROR","init_rng","gennum out of range. "); 
    return (int *) NULL;
  }

  seed &= 0x7fffffff;		/* Only 31 LSB of seed considered */
  
  if (mult < 0 || mult >= NPARAMS) 
  {
    errprint("WARNING","init_rng","multiplier not valid. Using Default param");
    mult = 0;
  }

#ifdef LONG64
  if(multiplier == 0)
    multiplier = mults[mult];
  /*  else
    if(multiplier != mults[mult]) 
      fprintf(stderr,"WARNING: init_rng_d: Proposed multiplier does not agree with current multiplier.\n\t Independence of streams is not guaranteed\n");*/
#else
  if(multiplier == NULL)
    multiplier = mults[mult];
  /*else
    if(strxncmp((char *) multiplier,(char *) mults[mult],4*sizeof(int)) != 0) 
      fprintf(stderr,"WARNING: init_rng_d: Proposed multiplier does not agree with current multiplier.\n\t Independence of streams is not guaranteed\n");*/
#endif
    
  genptr = (struct rngen *) mymalloc(1*sizeof(struct rngen));
  if(genptr == NULL)
    return NULL;
  
  genptr->rng_type = rng_type;
  genptr->gentype = GENTYPE;
  genptr->init_seed = seed;
  getprime_32(1, &(genptr->prime), gennum);
  genptr->prime_position = gennum;
  genptr->prime_next = total_gen;
  genptr->parameter = mult;
  
#ifdef LONG64
  genptr->seed = INIT_SEED;	/* initialize generator */
  genptr->seed ^= ((unsigned LONG64) seed)<<16;	
  genptr->multiplier = mults[mult];
  if (genptr->prime == 0) 
    genptr->seed |= 1;
#else
  genptr->seed[1] = 16651885^((seed<<16)&(0xff0000));/* initialize generator */
  genptr->seed[0] = 2868876^((seed>>8)&(0xffffff));
  genptr->multiplier = mults[mult];
  if (genptr->prime == 0) 
    genptr->seed[1] |= 1;
#endif

  for(i=0; i<LCGRUNUP*(genptr->prime_position); i++)
    get_rn_dbl( (int *) genptr);

  NGENS++;
  
  return (int *) genptr;
} 




/*  On machines with 32 bit integers, */
/*  the Cray's 48 bit integer math is duplicated by breaking the problem into*/
/*  steps.  The algorithm is linear congruential.  M is the multiplier and S*/
/*   is the current seed. The 31 High order bits out of the 48 bits are 
     returned*/
#ifdef __STDC__
int get_rn_int(int *igenptr)
#else
int get_rn_int(igenptr)
int *igenptr;
#endif
{
  struct rngen *genptr = (struct rngen *) igenptr;

#ifdef LONG64
    multiply(genptr);
  
    return ((unsigned LONG64) genptr->seed) >> 17;
#else
    int s[4], res[4];
  multiply(genptr,genptr->multiplier,s,res);


    return (genptr->seed[0]<<7) | ((unsigned int) genptr->seed[1] >> 17) ;
#endif
  
  
} 

#ifdef __STDC__
float get_rn_flt(int *igenptr)
#else
float get_rn_flt(igenptr)
int *igenptr;
#endif
{
    return (float) get_rn_dbl(igenptr);
} /* get_rn_float */


#ifdef __STDC__
double get_rn_dbl(int *igenptr)
#else
double get_rn_dbl(igenptr)
int *igenptr;
#endif
{
    struct rngen *genptr = (struct rngen *) igenptr;

#ifdef LONG64
    double temp[1];
    unsigned LONG64 *ltemp;
    
    temp[0] = 0.0;
    multiply(genptr);
    /* Add defined(O2K) || defined(SGI) if optimization level is -O2 
       or lower */
#if defined(CONVEX) || defined(GENERIC)
    ltemp = (unsigned LONG64 *) temp;
    *ltemp = (genptr->seed<<4) | MSB_SET;
  
    return temp[0] - (double) 1.0;
#else
    return genptr->seed*3.5527136788005008e-15;
#endif

#else

    static double equiv[1];
#define iran ((int *)equiv)
#define ran (equiv)

    int expo, s[4], res[4];
    

    multiply(genptr,genptr->multiplier,s,res);
    
#if defined(HP) || defined(SUN) || defined(SOLARIS) || defined(GENERIC)
    expo = 1072693248;

/*IEEE mantissa is 52 bits.  We have only 48 bits, so we shift our result 4*/
/*  bits left.  32-(24+4) = 4 bits are still blank in the lower word, so we*/
/*  grab the low 4 bits of seedhi to fill these. */
    iran[LSB] = genptr->seed[1] << 4 | genptr->seed[0] << 28;

/* place the remaining (24-4)=20 bits of seedhi in bits 20-0 of ran. */
/* Expo occupies bits 30-20.  Bit 31 (sign) is always zero. */
    iran[MSB] = expo | genptr->seed[0] >> 4;

    return (*ran - (double) 1.0);
#else
    return genptr->seed[0]*TWO_M24 + genptr->seed[1]*TWO_M48;
#endif  
  
#undef ran
#undef iran
#endif
} /* get_rn_dbl */



/*************************************************************************/
/*************************************************************************/
/*                  SPAWN_RNG: spawns new generators                     */
/*************************************************************************/
/*************************************************************************/

#ifdef __STDC__
int spawn_rng(int *igenptr,  int nspawned, int ***newgens, int checkid)
#else
int spawn_rng(igenptr,nspawned, newgens, checkid)
int *igenptr,nspawned, ***newgens, checkid;
#endif
{
  struct rngen **genptr, *tempptr = (struct rngen *) igenptr;
  int i, j;
  
  if (nspawned <= 0) /* check if nspawned is valid */
  {
    nspawned = 1;
    errprint("WARNING","spawn_rng","nspawned <= 0. Default value of 1 used for nspawned");
  }

  genptr = (struct rngen **) mymalloc(nspawned*sizeof(struct rngen *));
  if(genptr == NULL)
  {
    *newgens = NULL;
    return 0;
  }
  
  for(i=0; i<nspawned; i++)
  {
    genptr[i] = (struct rngen *) mymalloc(sizeof(struct rngen));
    if(genptr[i] == NULL)
    {
      nspawned = i;
      break;
    }
    
    genptr[i]->init_seed = tempptr->init_seed;
    genptr[i]->prime_position = tempptr->prime_position + 
      tempptr->prime_next*(i+1);
    if(genptr[i]->prime_position > MAXPRIMEOFFSET)
    {
      fprintf(stderr,"WARNING - spawn_rng: gennum: %d > maximum number of independent streams: %d\n\tIndependence of streams cannot be guranteed.\n",
	      genptr[i]->prime_position, MAX_STREAMS); 
      genptr[i]->prime_position %= MAXPRIMEOFFSET;
    }
    
    genptr[i]->prime_next = (nspawned+1)*tempptr->prime_next;
    getprime_32(1, &(genptr[i]->prime), genptr[i]->prime_position);
    genptr[i]->multiplier = tempptr->multiplier;
    genptr[i]->parameter = tempptr->parameter;
    genptr[i]->gentype = tempptr->gentype;
    genptr[i]->rng_type = tempptr->rng_type;
    
#ifdef LONG64
    genptr[i]->seed = INIT_SEED;	/* initialize generator */
    genptr[i]->seed ^= ((unsigned LONG64) tempptr->init_seed)<<16;	

    if (genptr[i]->prime == 0) 
      genptr[i]->seed |= 1;
#else
    genptr[i]->seed[1] = 16651885^((tempptr->init_seed<<16)&(0xff0000));
    genptr[i]->seed[0] = 2868876^((tempptr->init_seed>>8)&(0xffffff));
    if (genptr[i]->prime == 0) 
      genptr[i]->seed[1] |= 1;
#endif

    if(genptr[i]->prime_position > MAXPRIMEOFFSET)
      advance_seed(genptr[i]); /* advance lcg 10^6 steps from initial seed */

    for(j=0; j<LCGRUNUP*(genptr[i]->prime_position); j++)
      get_rn_dbl( (int *) genptr[i]);
  }
  tempptr->prime_next = (nspawned+1)*tempptr->prime_next;
    
  NGENS += nspawned;
    
  *newgens = (int **) genptr;
  
  if(checkid != 0)
    for(i=0; i<nspawned; i++)
      if(addID(( int *) genptr[i]) == NULL)
	return i;
  
  return nspawned;
}



/*Compute a + b. a and b are positive 4 digit integers */
/* in base 2^12, modulo 2^48 */
#ifdef __STDC__
void plus(int *a, int *b, int *result) 
#else
void plus(a,b,result)
int *a, *b, *result;
#endif
{
  int temp[5];
  int i;
  
  temp[4] = 0;
  
  for(i=0; i<4; i++)
    temp[i] = a[i] + b[i];
  
  for(i=1; i<5; i++)
  {
    temp[i] += temp[i-1]>>12;
    temp[i-1] &= 4095;
  }
  
  for(i=0; i<4 ; i++)
    result[i] = temp[i];
}



/*multiply two 4 digit numbers in base 2^12 and return 'size' lowest order */
/* digits*/
#ifdef __STDC__
void mult(int *a, int *b, int *c, int size)
#else
void mult(a,b,c,size)
int *a, *b, *c, size;
#endif
{
  int temp[8];
  int i, j;
  
  for(i=0; i<8; i++)
    temp[i] = 0;
  
  for(i=0; i<4; i++)
    for(j=0; j<4; j++)
      temp[i+j] += a[i]*b[j];
  
  for(i=1; i<8; i++)
  {
    temp[i] += temp[i-1]>>12;
    temp[i-1] &= 4095;
  }
  
  for(i=0; i<size && i<8 ; i++)
    c[i] = temp[i];
}


#ifdef __STDC__
void advance_seed(struct rngen *gen)
#else
void advance_seed(gen)
struct rngen *gen;
#endif
{
#ifdef LONG64
  int i, found;
  unsigned LONG64 an, pmult;
  
  for(i=0, found=0; i<NPARAMS; i++)
    if (gen->multiplier == mults[i])
    {
      found = 1;
      break;
    }
  if(found == 0)
  {
    fprintf(stderr,"WARNING: advance_seed: multiplier not acceptable.\n");
    return ;
  }

  /* a^n, n = 10^6 and pmult =  (a^n-1)/(a-1), n = 10^6 */
  switch(i)
  {
  case 0 :
    an = AN1;
    pmult = PMULT1;
    break;
  case 1 :
    an = AN2;
    pmult = PMULT2;
    break;
  case 2 :
    an = AN3;
    pmult = PMULT3;
    break;
  case 3 :
    an = AN4;
    pmult = PMULT4;
    break;
  case 4 :
    an = AN5;
    pmult = PMULT5;
    break;
  case 5 :
    an = AN6;
    pmult = PMULT6;
    break;
  case 6 :
    an = AN7;
    pmult = PMULT7;
    break;
  default:
    fprintf(stderr,"WARNING: advance_seed parameters for multiplier %d not available\n", i);
    return;
  }
  
  gen->seed = gen->seed*an + pmult*gen->prime;
  gen->seed &= LSB48;
  
#else
  int an[4], pmult[4], x0, x1, temp[4],temp2[4], i, found;
	
  for(i=0, found=0; i<NPARAMS; i++)
    if (strxncmp((char *) gen->multiplier, (char *) (mults[i]), 4*sizeof(int)) 
	== 0)
    {
      found = 1;
      break;
    }
  if(found == 0)
  {
    fprintf(stderr,"WARNING: advance_seed: multiplier not acceptable.\n");
    return ;
  }

  /* a^n, n = 10^6 and pmult =  (a^n-1)/(a-1), n = 10^6 */
  switch(i)
  {
  case 0 :
    an[0] = 0x001; an[1] = 0xc00; an[2] = 0xf0a; an[3] = 0xdad;
    pmult[0] = 0x000; pmult[1] = 0x700; pmult[2] = 0xc22; pmult[3] = 0xa42;
    break;
  case 1 :
    an[0] = 0x001; an[1] = 0x400; an[2] = 0xd7a; an[3] = 0xfef;
    pmult[0] = 0x000; pmult[1] = 0xb00; pmult[2] = 0x58c; pmult[3] = 0xfa8;
    break;
  case 2 :
    an[0] = 0x001; an[1] = 0xc00; an[2] = 0x7b5; an[3] = 0x641;
    pmult[0] = 0x000; pmult[1] = 0xf00; pmult[2] = 0xc4e; pmult[3] = 0x0d0;
    break;
  case 3 :
    an[0] = 0x001; an[1] = 0xc00; an[2] = 0xf72; an[3] = 0xcf9;
    pmult[0] = 0x000; pmult[1] = 0x700; pmult[2] = 0xdbe; pmult[3] = 0x11b;
    break;
  case 4 :
    an[0] = 0x001; an[1] = 0x400; an[2] = 0x07b; an[3] = 0xbdf;
    pmult[0] = 0x000; pmult[1] = 0x300; pmult[2] = 0xc8e; pmult[3] = 0xc3c;
    break;
  case 5 :
    an[0] = 0x001; an[1] = 0xc00; an[2] = 0x747; an[3] = 0xf33;
    pmult[0] = 0x000; pmult[1] = 0xf00; pmult[2] = 0x0e9; pmult[3] = 0xb0f;
    break;
  case 6 :
    an[0] = 0x001; an[1] = 0xc00; an[2] = 0x632; an[3] = 0xcbe;
    pmult[0] = 0x000; pmult[1] = 0x700; pmult[2] = 0x7de; pmult[3] = 0x640;
    break;
  default:
    fprintf(stderr,"WARNING: advance_seed parameters for multiplier %d not available\n", i);
    return;
  }
  
  x0 = gen->seed[0]; x1 = gen->seed[1];
  
  temp[0] = x1&4095; temp[1] = (x1>>12)&4095; temp[2] = x0&4095; /* seed */
  temp[3] = (x0>>12)&4095;

  temp2[0] = gen->prime%(1<<12); temp2[1] = (gen->prime>>12)%(1<<12);
  temp2[2] = (gen->prime>>24)%(1<<12); temp2[3] = 0;	/* prime */

  mult(temp,an,temp,4);
  mult(temp2,pmult,temp2,4);
	
  plus(temp,temp2,temp);
	
  gen->seed[1] = (temp[1]<<12) + temp[0];
  gen->seed[0] = (temp[3]<<12) + temp[2];
#endif
}


#ifdef __STDC__
int free_rng(int *genptr)
#else
int free_rng(genptr)
int *genptr;
#endif
{
  struct rngen *q;

  q = (struct rngen *) genptr;
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
  unsigned int size, m[2], i;
  struct rngen *q;
  
  q = (struct rngen *) genptr;
  size = 6*4 /*sizeof(int)*/ + 2*8/*sizeof(unsigned LONG64)*/ 
    + strlen(q->gentype)+1;
  /* The new load/store routines make using sizeof unnecessary. Infact, */
  /* using sizeof could be erroneous. */
  initp = p = (unsigned char *) mymalloc(size);
  if(p == NULL)
  {
    *buffer = NULL;
    return 0;
  }
  

  p += store_int(q->rng_type,4,p);
  strcpy((char *)p,q->gentype);
  p += strlen(q->gentype)+1;
#ifdef LONG64
  p += store_long64(q->seed,8,p);
  p += store_int(q->init_seed,4,p);
  p += store_int(q->prime,4,p);
  p += store_int(q->prime_position,4,p);
  p += store_int(q->prime_next,4,p);
  p += store_int(q->parameter,4,p);
  p += store_long64(q->multiplier,8,p);
#else
  m[0] = q->seed[0]>>8;m[1] = q->seed[0]<<24 | q->seed[1];
  p += store_intarray(m,2,4,p);
  p += store_int(q->init_seed,4,p);
  p += store_int(q->prime,4,p);
  p += store_int(q->prime_position,4,p);
  p += store_int(q->prime_next,4,p);
  p += store_int(q->parameter,4,p);
  /* The following is done since the multiplier is stored in */
  /* pieces of 12 bits each                               */
  m[1] = q->multiplier[2]&0xff; m[1] <<= 24; 
  m[1] |= q->multiplier[1]<<12; m[1] |= q->multiplier[0];
  m[0] = (q->multiplier[3]<<4) | (q->multiplier[2]>>8);
  
  p += store_intarray(m,2,4,p);
#endif     
  *buffer =  (char *) initp;

  assert(p-initp == size);
  
  return p-initp;
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
int *unpack_rng( char *packed)
#else
int *unpack_rng(packed)
char *packed;
#endif
{
  struct rngen *q;
  unsigned char *p;
  unsigned int m[4], m2[2], i;

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

#ifdef LONG64
  p += load_long64(p,8,&q->seed);
  p += load_int(p,4,(unsigned int *)&q->init_seed);
  p += load_int(p,4,(unsigned int *)&q->prime);
  p += load_int(p,4,(unsigned int *)&q->prime_position);
  p += load_int(p,4,(unsigned int *)&q->prime_next);
  p += load_int(p,4,(unsigned int *)&q->parameter);
  p += load_long64(p,8,&q->multiplier);

#else
  p += load_intarray(p,2,4,m2);
  q->seed[1] = m2[1]&0xffffff; q->seed[0] = m2[1]>>24 | m2[0]<<8;
  p += load_int(p,4,(unsigned int *)&q->init_seed);
  p += load_int(p,4,(unsigned int *)&q->prime);
  p += load_int(p,4,(unsigned int *)&q->prime_position);
  p += load_int(p,4,(unsigned int *)&q->prime_next);
  p += load_int(p,4,(unsigned int *)&q->parameter);
  p += load_intarray(p,2,4,m2);
  /* The following is done since the multiplier is stored in */
  /* pieces of 12 bits each                               */
  m[0] = m2[1]&0xfff; m[1] = (m2[1]&0xfff000)>>12;
  m[2] = (m2[1]&0xff000000)>>24 | (m2[0]&0xf)<<8;
  m[3] = (m2[0]&0xfff0)>>4;
  
#endif


  if(q->parameter < 0 || q->parameter >= NPARAMS)
  {
    fprintf(stderr,"ERROR: Unpacked parameters not acceptable.\n");
    free(q);
    return NULL;
  }
  
  q->multiplier = mults[q->parameter];
  
  NGENS++;
  
  return (int *) q;
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
  printf("\n \tseed = %d, stream_number = %d\tparameter = %d\n\n", gen->init_seed, gen->prime_position, gen->parameter);

  return 1;
}


