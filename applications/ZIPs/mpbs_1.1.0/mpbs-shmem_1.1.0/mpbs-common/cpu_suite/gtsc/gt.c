/* CVS info                         */
/* $Date: 2010/04/02 20:39:24 $     */
/* $Revision: 1.5 $                 */
/* $RCSfile: gt.c,v $               */
/* $State: Stab $:                */

/*@unused@*/
static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:39:24 $ $Revision: 1.5 $ $RCSfile: gt.c,v $ $Name:  $";

#include "bench.h"

/* Log base 2 of data array size */

#ifndef LG
#define LG                            25
#endif

#define MAX_ITER                      17

/* Array size in words */
#define MEMSZ                    (1<<LG)

//    UPD_SMALL(a,8,8);
//    Loads random blocks of 4 consecutive words.
//    A block is at a random offset of 0-255, 2**8 -1
//    8 blocks per macro use.


#define UPD_SMALL(X,N,L) {\
  uint64 x=(X);\
  int64 i;\
  for (i = 0; i < 64/L; i++) {\
    int64 a = x&_maskr(N);  x>>=N;\
    sum ^= data[a+0];\
    sum ^= data[a+1];\
    sum ^= data[a+2];\
    sum ^= data[a+3];\
  }\
}

//    UPD_LARGE(a,16,16);
//    Loads 4 64-bit words
//    A word is at a random offset of 0-65535, 2**16 -1

//    UPD_LARGE(a,25,25);
//    Loads 2 64-bit words
//    A word is at a random offset of 0-33,554,432 i.e. 2**25 -1

#define UPD_LARGE(X,N,L) {\
  uint64 x=(X);\
  int64 i;\
  for (i = 0; i < 64/L; i++) {\
    sum ^= data[x&_maskr(N)];  x>>=N;\
  }\
}

static uint64 data[MEMSZ + 64];

uint64 gs (int64 op, brand_t* br);

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
*  Do lots of random lookups in a small, 256, table.       *
*  Make the ratio of lookups per brand function call large *
*  so most of the recorded time measures memory loads.     *
\**********************************************************/
static uint64 gs_8 (brand_t* br)

{
  int64 i;
  uint64 a, b, c;
  uint64 sum = 0;

  /* This loop does 4 load/mac_iter * 8 mac_iter/macro * 24 macro/gs_iter =
     196 load/gs_iter. loads = load/gs_iter * 256 gs_iter/outer_iter = 50176 */
  for (i = 0; i < (1<<8); i++) {
    a = brand(br);
    b = brand(br);
    c = brand(br);

    UPD_SMALL(a,8,8);
    UPD_SMALL(b,8,8);
    UPD_SMALL(c,8,8);
    UPD_SMALL(a^b,8,8);
    UPD_SMALL(a^c,8,8);
    UPD_SMALL(b^c,8,8);
    UPD_SMALL((a|b)^c,8,8);
    UPD_SMALL((a|c)^b,8,8);
    UPD_SMALL((b|c)^a,8,8);
    UPD_SMALL((a&~b)^c,8,8);
    UPD_SMALL((a&~c)^b,8,8);
    UPD_SMALL((b&~c)^a,8,8);

    a = (a <<  4) | (a >> 60);
    b = (b << 12) | (b >> 52);
    c = (c << 20) | (c >> 44);

    UPD_SMALL(a,8,8);
    UPD_SMALL(b,8,8);
    UPD_SMALL(c,8,8);
    UPD_SMALL(a^b,8,8);
    UPD_SMALL(a^c,8,8);
    UPD_SMALL(b^c,8,8);
    UPD_SMALL((a|b)^c,8,8);
    UPD_SMALL((a|c)^b,8,8);
    UPD_SMALL((b|c)^a,8,8);
    UPD_SMALL((a&~b)^c,8,8);
    UPD_SMALL((a&~c)^b,8,8);
    UPD_SMALL((b&~c)^a,8,8);
  }
  return sum;
}

/**********************************************************\
*  Do lots of random lookups in a 65,536 long table.       *
*  Make the ratio of lookups per brand function call large *
*  so most of the recorded time measures memory loads.     *
\**********************************************************/
static uint64 gs_16 (brand_t* br)

{
  int64 i;
  uint64 a, b, c;
  uint64 sum = 0;

  /* This loop does 1 load/mac_iter * 64/16 mac_iter/macro * 24 macro/gs_iter =
     96 load/gs_iter. loads = load/gs_iter * 2^^16  gs_iter/outer_iter =
     6,291,456.
  */
  for (i = 0; i < (1<<16); i++) {
    a = brand(br);
    b = brand(br);
    c = brand(br);

    UPD_LARGE(a,16,16);
    UPD_LARGE(b,16,16);
    UPD_LARGE(c,16,16);
    UPD_LARGE(a^b,16,16);
    UPD_LARGE(a^c,16,16);
    UPD_LARGE(b^c,16,16);
    UPD_LARGE((a|b)^c,16,16);
    UPD_LARGE((a|c)^b,16,16);
    UPD_LARGE((b|c)^a,16,16);
    UPD_LARGE((a&~b)^c,16,16);
    UPD_LARGE((a&~c)^b,16,16);
    UPD_LARGE((b&~c)^a,16,16);

    a = (a <<  4) | (a >> 60);
    b = (b << 12) | (b >> 52);
    c = (c << 20) | (c >> 44);

    UPD_LARGE(a,16,16);
    UPD_LARGE(b,16,16);
    UPD_LARGE(c,16,16);
    UPD_LARGE(a^b,16,16);
    UPD_LARGE(a^c,16,16);
    UPD_LARGE(b^c,16,16);
    UPD_LARGE((a|b)^c,16,16);
    UPD_LARGE((a|c)^b,16,16);
    UPD_LARGE((b|c)^a,16,16);
    UPD_LARGE((a&~b)^c,16,16);
    UPD_LARGE((a&~c)^b,16,16);
    UPD_LARGE((b&~c)^a,16,16);
  }
  return sum;
}

static uint64 gs_LG (brand_t* br)

{
  int64 i;
  uint64 a, b, c;
  uint64 sum = 0;

  /* This loop does 1 load/mac_iter * 64/25 mac_iter/macro * 24 macro/gs_iter =
     48 load/gs_iter. loads = load/gs_iter * 2^^MAX_ITER  gs_iter/outer_iter =
     6,291,456 loads/call. (with MAX_ITER = 17).
  */
  for (i = 0; i < (1<<MAX_ITER); i++) {
    a = brand(br);
    b = brand(br);
    c = brand(br);

    UPD_LARGE(a,LG,LG);
    UPD_LARGE(b,LG,LG);
    UPD_LARGE(c,LG,LG);
    UPD_LARGE(a^b,LG,LG);
    UPD_LARGE(a^c,LG,LG);
    UPD_LARGE(b^c,LG,LG);
    UPD_LARGE((a|b)^c,LG,LG);
    UPD_LARGE((a|c)^b,LG,LG);
    UPD_LARGE((b|c)^a,LG,LG);
    UPD_LARGE((a&~b)^c,LG,LG);
    UPD_LARGE((a&~c)^b,LG,LG);
    UPD_LARGE((b&~c)^a,LG,LG);

    a = (a <<  4) | (a >> 60);
    b = (b << 12) | (b >> 52);
    c = (c << 20) | (c >> 44);

    UPD_LARGE(a,LG,LG);
    UPD_LARGE(b,LG,LG);
    UPD_LARGE(c,LG,LG);
    UPD_LARGE(a^b,LG,LG);
    UPD_LARGE(a^c,LG,LG);
    UPD_LARGE(b^c,LG,LG);
    UPD_LARGE((a|b)^c,LG,LG);
    UPD_LARGE((a|c)^b,LG,LG);
    UPD_LARGE((b|c)^a,LG,LG);
    UPD_LARGE((a&~b)^c,LG,LG);
    UPD_LARGE((a&~c)^b,LG,LG);
    UPD_LARGE((b&~c)^a,LG,LG);
  }
  return sum;
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
