#include "defs.h"

/* Global variables */
INT_T SCALE;
LONG_T N;
LONG_T M;

/* R-MAT parameters */
DOUBLE_T A;
DOUBLE_T B;
DOUBLE_T C;
DOUBLE_T D;

WEIGHT_T MaxIntWeight;
INT_T SubGraphPathLength;
INT_T K4approx;

void init(int SCALE) {

	/* Binary Scaling Heuristic */
	// SCALE 

    N = (1<<SCALE);
#ifndef VERIFYK4
    M = 8*N;
#else
    M = 4*N;
#endif

    /* R-MAT parameters */
    A = 0.55;
    B = 0.1;
    C = B;
    D = 0.25;

    MaxIntWeight = (1<<SCALE);
    SubGraphPathLength = 3;
#ifndef VERIFYK4    
    if (SCALE < 10)
        K4approx = SCALE;
    else 
        K4approx = 10;
#else
    K4approx = SCALE;
#endif
}
