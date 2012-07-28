/* CVS info                         */
/* $Date: 2010/04/02 20:39:36 $     */
/* $Revision: 1.5 $                 */
/* $RCSfile: sc.c,v $               */
/* $State: Stab $:                */

/*@unused@*/
static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:39:36 $ $Revision: 1.5 $ $RCSfile: sc.c,v $ $Name:  $";

#include "bench.h"

/* Log base 2 of data array size */
/* 2^8 = 256,   2^17 = 131,072 2^25 = 33,554,432 */

#ifndef LG
#define LG                            25
#endif

#define MAX_ITER                      17

/* Array size in words */
#define MEMSZ                    (1<<LG)

//    SC_SMALL(a,8,8,v);
//    Stores v into random blocks of 4 consecutive words.
//    A block is at a random offset of 0-255, 2**8 -1
//    8 blocks per macro use.


#define SC_SMALL(X,N,L,VAL) {\
  uint64 x=(X);\
  int64 i;\
  for (i = 0; i < 64/L; i++) {\
    int64 a = x&_maskr(N);  x>>=N;\
    data[a+0] = VAL;\
    data[a+1] = VAL;\
    data[a+2] = VAL;\
    data[a+3] = VAL;\
  }\
}
//    SC_LARGE(a,16,16,v);
//    Stores 4 64-bit words
//    A word is at a random offset of 0-65535, 2**16 -1


//    SC_LARGE(a,LG,LG,v);
//    Sores 2 64-bit words
//    A word is at a random offset of 0-33,554,432 i.e. 2**25 -1

#define SC_LARGE(X,N,L,VAL) {\
  uint64 x=(X);\
  int64 i;\
  for (i = 0; i < 64/L; i++) {\
    data[x&_maskr(N)] = VAL;  x>>=N;\
  }\
}


static uint64 data[MEMSZ + 64];

static uint64 init (brand_t* br);
static uint64 gs_8 (brand_t* br);
static uint64 gs_16 (brand_t* br);
static uint64 gs_LG (brand_t* br);


uint64 gs (int64 op, brand_t*  br)

{
  uint64 checksum;

  switch (op){
  case 1:
    checksum = init( br);
    break;
  case 2:
    checksum = gs_8( br);
    break;
  case 3:
    checksum = gs_16( br);
    break;
  case 4:
    checksum = gs_LG( br);;
    break;
  }
  return checksum;
}

/**********************************************************\
*  Do lots of random stores in a small, 256, table.        *
*  Make the ratio of stores per brand function call large  *
*  so most of the recorded time measures memory stores.    *
\**********************************************************/

static uint64 gs_8 (brand_t* br)

{
  int64 i;
  uint64 a, b, c, v;

  /* This loop does 4 load/mac_iter * 8 mac_iter/macro * 24 macro/gs_iter =
     196 load/gs_iter. loads = load/gs_iter * 256 gs_iter/outer_iter = 50176 */
  for (i = 0; i < (1<<8); i++) {
    a = brand(br);
    b = brand(br);
    c = brand(br);
    v = (a|~b) ^ ~c;

    SC_SMALL(a,8,8,v);
    SC_SMALL(b,8,8,v);
    SC_SMALL(c,8,8,v);
    SC_SMALL(a^b,8,8,v);
    SC_SMALL(a^c,8,8,v);
    SC_SMALL(b^c,8,8,v);
    SC_SMALL((a|b)^c,8,8,v);
    SC_SMALL((a|c)^b,8,8,v);
    SC_SMALL((b|c)^a,8,8,v);
    SC_SMALL((a&~b)^c,8,8,v);
    SC_SMALL((a&~c)^b,8,8,v);
    SC_SMALL((b&~c)^a,8,8,v);

    a = (a <<  4) | (a >> 60);
    b = (b << 12) | (b >> 52);
    c = (c << 20) | (c >> 44);

    SC_SMALL(a,8,8,v);
    SC_SMALL(b,8,8,v);
    SC_SMALL(c,8,8,v);
    SC_SMALL(a^b,8,8,v);
    SC_SMALL(a^c,8,8,v);
    SC_SMALL(b^c,8,8,v);
    SC_SMALL((a|b)^c,8,8,v);
    SC_SMALL((a|c)^b,8,8,v);
    SC_SMALL((b|c)^a,8,8,v);
    SC_SMALL((a&~b)^c,8,8,v);
    SC_SMALL((a&~c)^b,8,8,v);
    SC_SMALL((b&~c)^a,8,8,v);
  }
  return data[brand(br)&0xff];
}

/**********************************************************\
*  Do lots of random stores in a 65,536 long table.        *
*  Make the ratio of stores per brand function call large  *
*  so most of the recorded time measures memory stores.    *
\**********************************************************/

static uint64 gs_16 (brand_t* br)

{
  int64 i;
  uint64 a, b, c, v;

  for (i = 0; i < (1<<16); i++) {
    a = brand(br);
    b = brand(br);
    c = brand(br);
    v = (a|~b) ^ ~c;

    SC_LARGE(a,16,16,v);
    SC_LARGE(b,16,16,v);
    SC_LARGE(c,16,16,v);
    SC_LARGE(a^b,16,16,v);
    SC_LARGE(a^c,16,16,v);
    SC_LARGE(b^c,16,16,v);
    SC_LARGE((a|b)^c,16,16,v);
    SC_LARGE((a|c)^b,16,16,v);
    SC_LARGE((b|c)^a,16,16,v);
    SC_LARGE((a&~b)^c,16,16,v);
    SC_LARGE((a&~c)^b,16,16,v);
    SC_LARGE((b&~c)^a,16,16,v);

    a = (a <<  4) | (a >> 60);
    b = (b << 12) | (b >> 52);
    c = (c << 20) | (c >> 44);

    SC_LARGE(a,16,16,v);
    SC_LARGE(b,16,16,v);
    SC_LARGE(c,16,16,v);
    SC_LARGE(a^b,16,16,v);
    SC_LARGE(a^c,16,16,v);
    SC_LARGE(b^c,16,16,v);
    SC_LARGE((a|b)^c,16,16,v);
    SC_LARGE((a|c)^b,16,16,v);
    SC_LARGE((b|c)^a,16,16,v);
    SC_LARGE((a&~b)^c,16,16,v);
    SC_LARGE((a&~c)^b,16,16,v);
    SC_LARGE((b&~c)^a,16,16,v);
  }
  return data[brand(br)&0xffff];
}

static uint64 gs_LG (brand_t* br)

{
  int64 i;
  uint64 a, b, c, v;

  for (i = 0; i < (1<<MAX_ITER); i++) {
    a = brand(br);
    b = brand(br);
    c = brand(br);
    v = (a|~b) ^ ~c;

    SC_LARGE(a,LG,LG,v);
    SC_LARGE(b,LG,LG,v);
    SC_LARGE(c,LG,LG,v);
    SC_LARGE(a^b,LG,LG,v);
    SC_LARGE(a^c,LG,LG,v);
    SC_LARGE(b^c,LG,LG,v);
    SC_LARGE((a|b)^c,LG,LG,v);
    SC_LARGE((a|c)^b,LG,LG,v);
    SC_LARGE((b|c)^a,LG,LG,v);
    SC_LARGE((a&~b)^c,LG,LG,v);
    SC_LARGE((a&~c)^b,LG,LG,v);
    SC_LARGE((b&~c)^a,LG,LG,v);

    a = (a <<  4) | (a >> 60);
    b = (b << 12) | (b >> 52);
    c = (c << 20) | (c >> 44);

    SC_LARGE(a,LG,LG,v);
    SC_LARGE(b,LG,LG,v);
    SC_LARGE(c,LG,LG,v);
    SC_LARGE(a^b,LG,LG,v);
    SC_LARGE(a^c,LG,LG,v);
    SC_LARGE(b^c,LG,LG,v);
    SC_LARGE((a|b)^c,LG,LG,v);
    SC_LARGE((a|c)^b,LG,LG,v);
    SC_LARGE((b|c)^a,LG,LG,v);
    SC_LARGE((a&~b)^c,LG,LG,v);
    SC_LARGE((a&~c)^b,LG,LG,v);
    SC_LARGE((b&~c)^a,LG,LG,v);
  }
  return data[brand(br)&_maskr(LG)];
}

static uint64 init (brand_t* br)

{
  int64 i;
  uint64 sum = 0;

  for(i=0; i< (1<<LG)+64; i++){
    data[i] = brand(br);
    sum ^= data[i];
  }
  return sum;
}
