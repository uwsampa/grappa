#include "bench.h"

#define NROWS	64L
#define ITER	1024L

#define MAX(A,B)     (((A)>(B)) ? (A) : (B))
#define MIN(A,B)     (((A)<(B)) ? (A) : (B))

/* CVS info                         */
/* $Date: 2010/04/02 20:46:16 $     */
/* $Revision: 1.4 $                 */
/* $RCSfile: xba.h,v $              */
/* $State: Stab $:                */

void xor_bit_arr	(uint64 * RESTRICT mat, brand_t *br,
			 uint64 * RESTRICT ans, int64 n);
