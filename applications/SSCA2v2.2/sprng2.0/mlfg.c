/*************************************************************************/
/*************************************************************************/
/*           Parallel Multiplicative Lagged Fibonacci Generator          */
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

/*             This is version 0.2, created 26 Dec 1997                  */




#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define NDEBUG
#include <assert.h>
#include "memory.h"
#include "sprng_interface.h"
#include "mlfg.h"
#include "int64.h"

#define init_rng mlfg_init_rng
#define get_rn_int mlfg_get_rn_int
#define get_rn_flt mlfg_get_rn_flt
#define get_rn_dbl mlfg_get_rn_dbl
#define spawn_rng mlfg_spawn_rng
#define get_seed_rng mlfg_get_seed_rng
#define free_rng mlfg_free_rng
#define pack_rng mlfg_pack_rng
#define unpack_rng mlfg_unpack_rng
#define print_rng mlfg_print_rng

#define MAX_STREAMS mlfg_MAX_STREAMS
#define NGENS mlfg_NGENS
#define valid mlfg_valid


#define VERSION "00"
/*** Name for Generator ***/
#define GENTYPE VERSION "Multiplicative Lagged Fibonacci Generator"

#define NPARAMS 11		/* number of valid parameters */
int MAX_STREAMS = 0x7fffffff;  /* Maximum number streams to init_sprng */

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

  uint64 *lags;
  uint64 *si;
  int hptr;          /* integer pointer into fill */
  int lval, kval, seed;
};

struct vstruct {
      int L;
      int K;
      int LSBS;     /* number of least significant bits that are 1 */
      int first;    /* the first seed whose LSB is 1 */
};

const struct vstruct valid[] = { {17,5,1,10}, {31,6,1,2},
{55,24,1,11}, {63,31,1,14}, {127,97,1,21}, {521,353,1,100},
{521,168,1,83}, {607,334,1,166}, {607,273,1,105}, {1279,418,1,208}, {1279,861,1,233} };


#define TWO_M52 2.2204460492503131e-16 /* 2^(-52) */
#define TWO_M64 5.4210108624275222e-20 /* 2^(-64) */
#define BITS 62			/* Initialization of ALFG part is m-2 bits */
#define MAX_BIT_INT (BITS-2)
#define RUNUP (2*BITS)		/* Do RUNUP iterations after initialization */
#define GS0 0x372f05ac

#ifdef LONG64
#define INT_MOD_MASK (MASK64>>(64-BITS))
#define INT_MASK (MASK64>>(64-BITS+1))
#define INTX2_MASK ((((uint64)1)<<MAX_BIT_INT)-1)
static const uint64 SEED_MASK=0x5a38;
#else
static const uint64 INT_MOD_MASK={0xffffffffU,0x3fffffffU};
static const uint64 INT_MASK={0xffffffffU,0x1fffffffU};
static const uint64 INTX2_MASK={0xffffffffU,0x0fffffffU};
static const uint64 SEED_MASK={0x5a38,0x0U};
#endif


int NGENS=0;		  /* number of random streams in current process */

/*************************************************************************/
/*************************************************************************/
/*            ROUTINES USED TO CREATE GENERATOR FILLS                    */
/*************************************************************************/
/*************************************************************************/

#ifdef __STDC__
static int bitcnt( int x)
#else
static int bitcnt(x)
int x;
#endif
{
  unsigned i=0,y;

  for (y=(unsigned)x; y; y &= (y-1) ) 
    i++;

  return(i);
}



static void advance_reg(int *reg_fill)
{
/*      the register steps according to the primitive polynomial         */
/*           (64,4,3,1,0); each call steps register 64 times             */
/*      we use two words to represent the register to allow for integer  */
/*           size of 32 bits                                             */

  const int mask = 0x1b;
  int adv_64[4][2];
  int i,new_fill[2];
  unsigned temp;

  adv_64[0][0] = 0xb0000000;
  adv_64[0][1] = 0x1b;
  adv_64[1][0] = 0x60000000;
  adv_64[1][1] = 0x2d;
  adv_64[2][0] = 0xc0000000;
  adv_64[2][1] = 0x5a;
  adv_64[3][0] = 0x80000000;
  adv_64[3][1] = 0xaf;
  new_fill[1] = new_fill[0] = 0;
  temp = mask<<27;

  for (i=27;i>=0;i--) 
  {
    new_fill[0] = (new_fill[0]<<1) | (1&bitcnt(reg_fill[0]&temp));
    new_fill[1] = (new_fill[1]<<1) | (1&bitcnt(reg_fill[1]&temp));
    temp >>= 1;
  }

  for (i=28;i<32;i++) 
  {
    temp = bitcnt(reg_fill[0]&(mask<<i));
    temp ^= bitcnt(reg_fill[1]&(mask>>(32-i)));
    new_fill[0] |= (1&temp)<<i;
    temp = bitcnt(reg_fill[0]&adv_64[i-28][0]);
    temp ^= bitcnt(reg_fill[1]&adv_64[i-28][1]);
    new_fill[1] |= (1&temp)<<i;
  }

  reg_fill[0] = new_fill[0];
  reg_fill[1] = new_fill[1];
}



static void get_fill(uint64 *n, uint64 *r, int param, unsigned seed)
{
  int i,j,k,temp[2], length;
  uint64 tempui;
  
  length = valid[param].L;
  
  /* Initialize the shift register with the node number XORed with seed    */
  temp[1] = highword(n[0]);
  temp[0] = lowword(n[0])^seed;
  if (!temp[0])
    temp[0] = GS0;


  advance_reg(temp); /* Advance the shift register some */
  advance_reg(temp);

  /* The first word of the RNG is defined by the LSBs of the node number   */
  and(n[0],INT_MASK,tempui);
  lshift(tempui,1,r[0]);
  
  /* The RNG is filled with the bits of the shift register, at each time   */
  /* shifted up to make room for the bits defining the canonical form;     */
  /* the node number is XORed into the fill to make the generators unique  */

  for (i=1;i<length-1;i++) 
  {
    advance_reg(temp);

    seti2(temp[0],temp[1],tempui);
    xor(tempui,n[i],tempui);
    and(tempui,INT_MASK,tempui);
    lshift(tempui,1,r[i]);
  }
  seti(0,r[length-1]);
/*      the canonical form for the LSB is instituted here                */
  k = valid[param].first + valid[param].LSBS;

  for (j=valid[param].first;j<k;j++)
    or(r[j],ONE,r[j]);

  return;
}


/* left shift array 'b' by one, and place result in array 'a' */

static void si_double(uint64 *a,  uint64 *b, int length)
{
  int i;
  uint64 mask1, temp1;
  
  lshift(ONE,MAX_BIT_INT,mask1);
  
  and(b[length-2],mask1,temp1)
  if (notzero(temp1))
    fprintf(stderr,"WARNING: si_double -- RNG has branched maximum number of times.\n\t Independence of generators no longer guaranteed\n");

  and(b[length-2],INTX2_MASK,temp1);
  lshift(temp1,1,a[length-2]);

  for (i=length-3;i>=0;i--) 
  {
    and(b[i],mask1,temp1)
    if(notzero(temp1)) 
      add(a[i+1],ONE,a[i+1]);

    and(b[i],INTX2_MASK,temp1);
    lshift(temp1,1,a[i]);
  }
}



static void pow3(uint64 n, uint64 *ui)		/* return 3^n (mod 2^BITS) */
{
  uint64 p, value, temp, bit, temp2, temp3;
  int exponent;
  
  set(n,p);
  seti(3,temp);
  seti(1,temp3);
  and(n,temp3,temp2);
  if(notzero(temp2))
    seti(3,value)
  else
    seti(1,value)
  seti(1,bit);
  
  for(exponent=2; exponent<64; exponent++)
  {
    multiply(temp,temp,temp);
    lshift(bit,1,bit);
    
    and(bit,n,temp2);
    if(notzero(temp2))
      multiply(value,temp,value);
  }
  
  and(value,MASK64,value);
  
  set(value,(*ui));
}


static void findseed(int sign, uint64 n, uint64 *ui)
{
  uint64 temp;
  
  pow3(n,&temp);
  
  if(sign&1)
    multiply(temp,MINUS1,temp);
  
  set(temp,(*ui));
}


#define  advance_state(genptr)  {int lval = genptr->lval, kval = genptr->kval;\
  int lptr;\
  genptr->hptr--;\
  if(genptr->hptr < 0)\
   genptr->hptr = lval-1;\
  lptr = genptr->hptr + kval;\
  if (lptr>=lval)\
   lptr -= lval;\
  multiply(genptr->lags[genptr->hptr],genptr->lags[lptr],genptr->lags[genptr->hptr]);}



static struct rngen **initialize(int rng_type, int ngen, int param, unsigned int seed, 
				 uint64 *nstart, unsigned int initseed)
{
  int i,j,k,l,m,*order, length;
  struct rngen **q;
  uint64 *nindex, temp1, mask;

  length = valid[param].L;
  
  order = (int *) mymalloc(ngen*sizeof(int));
  q = (struct rngen **) mymalloc(ngen*sizeof(struct rngen *));
  if (q == NULL || order == NULL) 
    return NULL;

  for (i=0;i<ngen;i++) 
  {
    q[i] = (struct rngen *) mymalloc(sizeof(struct rngen));
    if (q[i] == NULL) 
      return NULL;

    q[i]->rng_type = rng_type;
    q[i]->hptr = 0;   /* This is reset to lval-1 before first iteration */
    q[i]->si = (uint64 *) mymalloc((length-1)*sizeof(uint64));
    q[i]->lags = (uint64 *) mymalloc(length*sizeof(uint64));
    q[i]->lval = length;
    q[i]->kval = valid[param].K;
    q[i]->parameter = param;
    q[i]->seed = seed;
    q[i]->init_seed = initseed;
    q[i]->narrays=2;
    q[i]->gentype = GENTYPE;
    
    if (q[i]->lags == NULL || q[i]->si == NULL) 
      return NULL;
  }
/*      specify register fills and node number arrays                    */
/*      do fills in tree fashion so that all fills branch from index     */
/*           contained in nstart array                                   */
  q[0]->stream_number = lowword(nstart[0]);
  get_fill(nstart,q[0]->lags,param,seed);
  si_double(q[0]->si,nstart,length);

  set(ONE,mask);
  for(m=0; m<length; m++)
  {
    and(SEED_MASK,mask,temp1);
    if(notzero(temp1))
      findseed(1,q[0]->lags[m], &q[0]->lags[m]);
    else
      findseed(0,q[0]->lags[m], &q[0]->lags[m]);
    lshift(mask,1,mask);
  }
  
  add(q[0]->si[0],ONE,q[0]->si[0]);

  i = 1;
  order[0] = 0;
  if (ngen>1) 
    while (1) 
    {
      l = i;
      for (k=0;k<l;k++) 
      {
	nindex = q[order[k]]->si;
	q[i]->stream_number = lowword(nindex[0]);
	get_fill(nindex,q[i]->lags,param,seed);
	si_double(nindex,nindex, length);
	for (j=0;j<length-1;j++) 
	  set(nindex[j],q[i]->si[j]);

	set(ONE,mask);
	for(m=0; m<length; m++)
	{
	  and(SEED_MASK,mask,temp1);
	  if(notzero(temp1))
	    findseed(1,q[i]->lags[m], &q[i]->lags[m]);
	  else
	    findseed(0,q[i]->lags[m], &q[i]->lags[m]);
	  lshift(mask,1,mask);
	}
	
	add(q[i]->si[0],ONE,q[i]->si[0]);
	if (ngen == ++i) 
	  break;
      }
      
      if (ngen == i) 
	break;
                
      for (k=l-1;k>0;k--) 
      {
	order[2*k+1] = l+k;
	order[2*k] = order[k];
      }
      order[1] = l;
    }

  free(order);

  for (i=ngen-1;i>=0;i--) 
  {
    k = 0;
    for (j=1;j<length-1;j++)
      if (notzero(q[i]->si[j])) 
	k = 1;
    if (!k) 
      break;
    for (j=0;j<length*RUNUP;j++)
      advance_state(q[i]);
  }

  while (i>=0)
  {
    for (j=0;j<4*length;j++)
      advance_state(q[i]);
    i--;
  }   

  return q;
}



/* Initialize random number stream */

#ifdef __STDC__
int *init_rng(int rng_type, int gennum, int total_gen,  int seed, int param)
#else
int *init_rng(rng_type,gennum,total_gen,seed,param)
int rng_type,gennum,param,seed,total_gen;
#endif
{
/*      gives back one stream (node gennum) with updated spawning         */
/*      info; should be called total_gen times, with different value      */
/*      of gennum in [0,total_gen) each call                              */
  struct rngen **p=NULL, *genptr;
  uint64 *nstart=NULL,*si;
  int i, length, k;
  
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

  seed &= 0x7fffffff;		/* Only 31 LSB of seed considered */

  if (param < 0 || param >= NPARAMS)     /* check if parameter is valid */
  {
    fprintf(stderr,"WARNING - init_rng: parameter not valid. Using Default parameter.\n");
    param = 0;
  }
  
  length = valid[param].L; /* determine parameters   */
  k = valid[param].K;
  
/*      define the starting vector for the initial node                  */
  nstart = (uint64 *) mymalloc((length-1)*sizeof(uint64));
  if (nstart == NULL)
    return NULL;

  seti(gennum,nstart[0]);
  for (i=1;i<length-1;i++) 
    seti(0,nstart[i]);

  p = initialize(rng_type, 1,param,seed^GS0,nstart,seed);  /* create a generator  */
  if (p==NULL) 
    return NULL;
  else
  {
    genptr = p[0]; 
    free(p);
  }
  
/*      update si array to allow for future spawning of generators       */
  si = genptr->si;
  while (lowword(si[0]) < total_gen && !highword(si[0])) 
    si_double(si,si,length);

  NGENS++;
      
  
  genptr->rng_type = rng_type;
  genptr->stream_number = gennum;
  genptr->nstreams = total_gen;
  
  genptr->array_sizes = (int *) mymalloc(genptr->narrays*sizeof(int));
  genptr->arrays = (int **) mymalloc(genptr->narrays*sizeof(int *));
  if(genptr->array_sizes == NULL || genptr->arrays == NULL)
    return NULL;
  genptr->arrays[0] = (int *) genptr->lags;
  genptr->arrays[1] = (int *) genptr->si;
  genptr->array_sizes[0] = genptr->lval*sizeof(uint64)/sizeof(int);
  genptr->array_sizes[1] = (genptr->lval-1)*sizeof(uint64)/sizeof(int);
  
  return (int *) genptr;
} 





/* Returns a double precision random number */

#ifdef __STDC__
double get_rn_dbl(int *igenptr)
#else
double get_rn_dbl(igenptr)
int *igenptr;
#endif
{
  struct rngen *genptr = (struct rngen *) igenptr;

  advance_state(genptr);	
  /*printf("\t sprng: %lu\n", genptr->lags[genptr->hptr]);*/
#ifdef LONG64  
  return (genptr->lags[genptr->hptr]>>12)*TWO_M52;
#else
  return (genptr->lags[genptr->hptr][1])*TWO_M32 + 
    (genptr->lags[genptr->hptr][0])*TWO_M64;
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
  struct rngen *genptr = (struct rngen *) igenptr;

  advance_state(genptr);	

#ifdef LONG64
  return genptr->lags[genptr->hptr]>>33;
#else
  return genptr->lags[genptr->hptr][1]>>1;
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
  struct rngen **genptr, *temp = (struct rngen *) igenptr;
  int i;
  uint64 *p;
  
  if (nspawned <= 0) /* is nspawned valid ? */
  {
    nspawned = 1;
    fprintf(stderr,"WARNING - spawn_rng: nspawned <= 0. Default value of 1 used for nspawned\n");
  }
  
  p = temp->si;
  
  
  genptr = initialize(temp->rng_type, nspawned,temp->parameter,temp->seed,p,temp->init_seed);
  if(genptr == NULL)	   /* allocate memory for pointers to structures */
  {
    *newgens = NULL;
    return 0;
  }
  
  si_double(p,p,temp->lval);

  for(i=0; i<nspawned; i++)
  {
    genptr[i]->array_sizes = (int *) mymalloc(genptr[i]->narrays*sizeof(int));
    genptr[i]->arrays = (int **) mymalloc(genptr[i]->narrays*sizeof(int *));
    if(genptr[i]->array_sizes == NULL || genptr[i]->arrays == NULL)
      return 0;
    genptr[i]->arrays[0] = (int *) genptr[i]->lags;
    genptr[i]->arrays[1] = (int *) genptr[i]->si;
    genptr[i]->array_sizes[0] = genptr[i]->lval*sizeof(uint64)/sizeof(int);
    genptr[i]->array_sizes[1] = (genptr[i]->lval-1)*sizeof(uint64)/sizeof(int);
  }
  
  NGENS += nspawned;
      
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
  struct rngen *q;

  q = (struct rngen *) genptr;
  size = 4 + 24+16*q->lval + strlen(q->gentype)+1;
  
  initp = p = (unsigned char *) mymalloc(size); /* allocate memory */
  if(p == NULL)
  {
    *buffer = NULL;
    return 0;
  }
  
  p += store_int(q->rng_type,4,p);
  strcpy((char *) p,q->gentype);
  p += strlen(q->gentype)+1;
  p += store_int(q->stream_number,4,p);
  p += store_int(q->nstreams,4,p);
  p += store_int(q->init_seed,4,p);
  p += store_int(q->parameter,4,p);
  p += store_int(q->hptr,4,p);
  p += store_int(q->lval,4,p);
  p += store_int(q->kval,4,p);
  p += store_int(q->seed,4,p);
  p += store_uint64array(q->si,q->lval-1,p);
  p += store_uint64array(q->lags,q->lval,p);
  
  *buffer = (char *) initp;
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
  unsigned char *p;
  int i, found;

  q = (struct rngen *) mymalloc(sizeof(struct rngen));
  if(q == NULL)
    return NULL;

  p = (unsigned char *) packed;
  p += load_int(p,4,(unsigned int *)&q->rng_type);
  if(strcmp((char *) p,GENTYPE) != 0)
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
  p += load_int(p,4,(unsigned int *)&q->hptr);
  p += load_int(p,4,(unsigned int *)&q->lval);
  p += load_int(p,4,(unsigned int *)&q->kval);
  p += load_int(p,4,(unsigned int *)&q->seed);


/*      check values of parameters for consistency                       */
  for(i=found=0; i<NPARAMS; i++)
    if(q->lval==valid[i].L && q->kval==valid[i].K)
    {
      found = 1;
      break;
    }
  
  if(found == 0)
  {
    fprintf(stderr,"ERROR: Unpacked parameters are not acceptable.\n");
    return NULL;
  }

  q->narrays = 2;
  q->array_sizes = (int *) mymalloc(q->narrays*sizeof(int));
  q->arrays = (int **) mymalloc(q->narrays*sizeof(int *));
  if(q->array_sizes == NULL || q->arrays == NULL)
    return NULL;
  q->array_sizes[0] = q->lval*sizeof(uint64)/sizeof(int);
  q->array_sizes[1] = (q->lval-1)*sizeof(uint64)/sizeof(int);
  
  for(i=0; i<q->narrays; i++)
  {
    q->arrays[i] = (int *) mymalloc(q->array_sizes[i]*sizeof(int));
    if(q->arrays[i] == NULL)
      return NULL;
  }   
  q->lags = (uint64 *) q->arrays[0];
  q->si   = (uint64 *) q->arrays[1];
  p += load_uint64array(p,q->lval-1,q->si);
  p += load_uint64array(p,q->lval,q->lags);
  
    
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


