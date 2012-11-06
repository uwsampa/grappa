#ifndef __BRAND_H
#define __BRAND_H

#include "local_types.h"


/* some masking macros */

#define _ZERO64       0uL
#define _maskl(x)     (((x) == 0) ? _ZERO64   : ((~_ZERO64) << (64-(x))))
#define _maskr(x)     (((x) == 0) ? _ZERO64   : ((~_ZERO64) >> (64-(x))))
#define _mask(x)      (((x) < 64) ? _maskl(x) : _maskr(2*64 - (x)))

/* PRNG */

#define _BR_RUNUP_      128L
#define _BR_LG_TABSZ_    7L
#define _BR_TABSZ_      (1L<<_BR_LG_TABSZ_)

typedef struct {
  uint64 hi, lo, ind;
  uint64 tab[_BR_TABSZ_];
} brand_t;

#define _BR_64STEP_(H,L,A,B) {\
  uint64 x;\
  x = H ^ (H << A) ^ (L >> (64-A));\
  H = L | (x >> (B-64));\
  L = x << (128 - B);\
}

uint64 brand (brand_t *p);

void brand_init (brand_t *p, uint64 val);

double rfraction(brand_t *p);
#endif



/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make brand.o"
End:
*/
