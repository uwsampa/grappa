#include "rand64.h"

#include <stdint.h>

/*
 * Random number generation
 */
#define _BR_RUNUP_      128L
#define _BR_LG_TABSZ_    7L
#define _BR_TABSZ_      (1L<<_BR_LG_TABSZ_)

typedef uint64_t uint64;
typedef  int64_t  int64;

typedef struct {
  uint64 hi, lo, ind;
  uint64 tab[_BR_TABSZ_];
} brand_t;

static brand_t p;

#define _BR_64STEP_(H,L,A,B) {\
  uint64 x;\
  x = H ^ (H << A) ^ (L >> (64-A));\
  H = L | (x >> (B-64));\
  L = x << (128 - B);\
}

#define _ZERO64       0uL
#define _maskl(x)     (((x) == 0) ? _ZERO64   : ((~_ZERO64) << (64-(x))))
#define _maskr(x)     (((x) == 0) ? _ZERO64   : ((~_ZERO64) >> (64-(x))))
#define _mask(x)      (((x) < 64) ? _maskl(x) : _maskr(2*64 - (x)))

uint64 rand64() {
  uint64 hi=p.hi, lo=p.lo, i=p.ind, ret;

  ret = p.tab[i];

  _BR_64STEP_(hi,lo,45,118);

  p.tab[i] = ret + hi;

  p.hi  = hi;
  p.lo  = lo;
  p.ind = hi & _maskr(_BR_LG_TABSZ_);

  return ret;
}

void srand64(uint64 val) {
  int64 i;
  uint64 hi, lo;

  hi = 0x9ccae22ed2c6e578uL ^ val;
  lo = 0xce4db5d70739bd22uL & _maskl(118-64);

  for (i = 0; i < 64; i++)
    _BR_64STEP_(hi,lo,33,118);

  for (i = 0; i < _BR_TABSZ_; i++) {
    _BR_64STEP_(hi,lo,33,118);
    p.tab[i] = hi;
  }
  p.ind = (uint64)(_BR_TABSZ_/2);
  p.hi  = hi;
  p.lo  = lo;

  for (i = 0; i < _BR_RUNUP_; i++)
    rand64();
}

// A random fraction
double rfraction(){

  return  ( (double)(rand64() & ((1L << 50) - 1)) ) /
    (double)((1L << 50) - 1);
}


#ifdef TEST_RAND64

#include <stdlib.h>
#include <stdio.h>


uint64 seeds [3] = { 1UL, 37UL, 12345UL };
uint64 good_vals[3][3] =
  {
    {  6155087932085668511UL, 17596454886979868140UL,  7449151104311482138UL },
    { 15409345016658701752UL, 11263177473774080416UL,  3588872263257954155UL },
    {  5764221613212592605UL,  7000661078794421758UL, 11703761339612756870UL  }
  };

int main(int argc, char** argv){
  brand_t   p;
  uint64 seed;
  uint64 rval;
  int i, j;
  int error = 0;

  for (i=0; i<3; i++){
    // Get a new seed and initialize the RNG
    srand64 (seeds[i]);

    for (j=0; j<3; j++){
      // Generate some random values with the seed
      rval = rand64();
      if ( rval != good_vals[i][j] ){
        fprintf(stderr, "ERROR: brand an incorrect value \n");
        fprintf(stderr, "good_vals[%d][%d] = %lu :  rval = %lu\n", i, j, good_vals[i][j], rval );
        error++;
      }
    }
  }
  if ( error )
    exit( EXIT_FAILURE );
  printf("PASSED        All values agree\n");

  return 0;
}

#endif


