#ifndef _INT64_H_
#define _INT64_H_

#include <limits.h>
#include "store.h"

#ifndef __STDC__
#define const			/* old C does not have 'const' qualifier */
#endif


#if LONG_MAX > 2147483647L 
#if LONG_MAX > 35184372088831L 
#if LONG_MAX >= 9223372036854775807L 
#define LONG_SPRNG
#define LONG64 long		/* 64 bit long */
#endif
#endif
#endif

#if !defined(LONG_SPRNG) &&  defined(_LONG_LONG)
#define LONG64 long long
#endif


#ifdef LONG64

typedef unsigned LONG64 uint64;

#ifdef LONG_SPRNG
#define MINUS1 0xffffffffffffffffUL		/* -1 (mod 2^(BITS-2)) */
#define ONE    0x1UL
#define MASK64 0xffffffffffffffffUL
#else
#define MINUS1 0xffffffffffffffffULL		/* -1 (mod 2^(BITS-2)) */
#define ONE    0x1ULL
#define MASK64 0xffffffffffffffffULL
#endif

#define multiply(a,b,c) {c = (a)*(b); c &= MASK64;}
#define add(a,b,c)      {c = (a)+(b); c &= MASK64;}
#define decrement(a,c)  {c = (a)-1; c &= MASK64;}
#define and(a,b,c)      {c = (a)&(b);}
#define or(a,b,c)       {c = (a)|(b);}
#define xor(a,b,c)      {c = (a)^(b);}
#define notzero(a)      (a==0?0:1)
#define lshift(a,b,c)   {c = (a)<<(b); c &= MASK64;} /* b is an int */
#define rshift(a,b,c)   {c = (a)>>(b); c &= MASK64;} /* b is an int */
#define highword(a)     ((unsigned int)((a)>>32))
#define lowword(a)      ((unsigned int)((a)&0xffffffff))
#define set(a,b)        {b = (a)&MASK64;}
#define seti(a,b)       {b = (a)&MASK64;} /* b is an int */
#define seti2(a,b,c)    {c = (b); c <<= 32; c |= (a); c &= MASK64;}/*a,b=+int*/

#else   /* Simulate 64 bit arithmetic on 32 bit integers */

typedef unsigned int uint64[2];

static const uint64 MASK64={0xffffffffU,0xffffffffU};
static const uint64 MINUS1={0xffffffffU,0xffffffffU}; /* -1 (mod 2^(BITS-2)) */
static uint64 ONE={0x1U,0x0U}; 
#define TWO_M32 2.3283064365386962e-10 /* 2^(-32) */

#define and(a,b,c)      {c[0] = a[0]&b[0]; c[1] = a[1]&b[1];}
#define or(a,b,c)       {c[0] = a[0]|b[0]; c[1] = a[1]|b[1];}
#define xor(a,b,c)      {c[0] = a[0]^b[0]; c[1] = a[1]^b[1];}
#define notzero(a)      ((a[0]==0 && a[1]==0)?0:1)
#define multiply(a,b,c) {c[1] = a[0]*b[1]+a[1]*b[0];\
                           c[1] += (unsigned int) (((double)a[0]*(double)b[0])\
                                                                *TWO_M32);\
                           c[0] = a[0]*b[0]; and(c,MASK64,c);}
#define add(a,b,c)      {unsigned int t = a[0]+b[0]; \
                           c[1] = a[1]+b[1]+(t<a[0]);c[0]=t;\
                           and(c,MASK64,c);}
#define decrement(a,c)  {if(a[0]==0){c[1]=a[1]-1;c[0]=0xffffffff;} \
                            else c[0] = a[0]-1; and(c,MASK64,c);}

static void lshift(uint64 a,int b,uint64 c)   
{
  if(b<32)
  {c[1] = (a[1]<<b)|(a[0]>>(32-b)); c[0] = a[0]<<(b);}
  else {c[1]=a[0]<<(b-32);c[0]=0;} 
  and(c,MASK64,c);
} 

static void rshift(uint64 a,int b,uint64 c)   
{
  if(b<32)
  {c[0] = (a[0]>>b)|(a[1]<<(32-b));c[1] = a[1]>>(b);}
  else {c[0]=a[1]>>(b-32);c[1]=0;} 
  and(c,MASK64,c);
} 

#define highword(a)     ((a)[1])
#define lowword(a)      ((a)[0])
#define set(a,b)        {b[0] = a[0];b[1]=a[1];and(b,MASK64,b);}
#define seti(a,b)       {b[0] = a;b[1]=0;} /* b is an int */
#define seti2(a,b,c)    {c[1] = b; c[0] = a; and(c,MASK64,c);}/*a,b = +ve int*/

#endif  /* LONG64 or 32 bit */  


static int store_uint64(uint64 l, unsigned char *c)
{
  int i;
  unsigned int m[2];
  
  m[0] = highword(l);
  m[1] = lowword(l);
  
  c += store_intarray(m,2,4,c);

  return 8;		/* return number of chars filled */
}


static int store_uint64array(uint64 *l, int n, unsigned char *c)
{
  int i;
  
  for(i=0; i<n; i++)
    c += store_uint64(l[i],c);

  return 8*n;
}


int load_uint64(unsigned char *c, uint64 *l)
{
  int i;
  unsigned int m[2];
  
  c += load_intarray(c,2,4,m);
  seti2(m[1],m[0],(*l));
  
  return 8;
}


int load_uint64array(unsigned char *c, int n, uint64 *l)
{
  int i;
  
  for(i=0; i<n; i++)
    c += load_uint64(c,l+i);

  return 8*n;
}



#endif /* _INT64_H_ */
