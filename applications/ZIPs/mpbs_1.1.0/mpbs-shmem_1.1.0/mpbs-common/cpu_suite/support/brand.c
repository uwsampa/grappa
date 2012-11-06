#include "brand.h"

uint64 brand (brand_t *p) {
  uint64 hi=p->hi, lo=p->lo, i=p->ind, ret;

  ret = p->tab[i];

  _BR_64STEP_(hi,lo,45,118);

  p->tab[i] = ret + hi;

  p->hi  = hi;
  p->lo  = lo;
  p->ind = hi & _maskr(_BR_LG_TABSZ_);

  return ret;
}

void brand_init (brand_t *p, uint64 val)

{
  int64 i;
  uint64 hi, lo;

  hi = 0x9ccae22ed2c6e578uL ^ val;
  lo = 0xce4db5d70739bd22uL & _maskl(118-64);

  for (i = 0; i < 64; i++)
    _BR_64STEP_(hi,lo,33,118);

  for (i = 0; i < _BR_TABSZ_; i++) {
    _BR_64STEP_(hi,lo,33,118);
    p->tab[i] = hi;
  }
  p->ind = (uint64)(_BR_TABSZ_/2);
  p->hi  = hi;
  p->lo  = lo;

  for (i = 0; i < _BR_RUNUP_; i++)
    (void)brand(p);
}

// A random fraction
double rfraction(brand_t *p){

  return  ( (double)(brand(p) & ((1L << 50) - 1)) ) /
    (double)((1L << 50) - 1);
}


#ifdef TEST_BRAND

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
    brand_init (&p, seeds[i] );

    for (j=0; j<3; j++){
      // Generate some random values with the seed
      rval = brand (&p);
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




/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make brand.o"
End:
*/


