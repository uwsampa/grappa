/*
 
    This is a check routine for Benchmark #8, part b, large sparse
    matrices.  It will verify that the Lg Path Probability is 
    correct for the path given in B.
 
    Parameters:
 
      Provided by the calling routine:
        N   =  The size of the A matrices
        K   =  The number of A matrices
        L   =  Number of non-zeros in each row and column of each A
               matrix
        T   =  The length of D, and one less than the length of B
        A   =  K long array of N by N matrices, packed into N by L
              arrays.
        IA  =  N by L array of indices for the packing of A
        B   =  T+1 long array containing the best path      
        D   =  T long array of integers between 1 and K used to
               select the appropriate A matrix.
        Z   =  The probability of this path
        ZZ  =  Expected probability of path
 
      Returned by this routine:
 
        IER  =  Error code
 
*/
#include "s8s.h"

void c8s(int n,
	int k,
	int l,
	int t,
	double a[NMATRICES][NONZERO][MATRIXSIZE],
	long ia[NONZERO][MATRIXSIZE],
	long b[DLEN+1],
	long d[DLEN],
	double z,
	double *zz,
	int *ier)

{
  int i,j;
  double fabs();

#define DELTA .00002	/* An allowable difference in score */
      *ier = 0;

  /*  First check the score */
      *zz = 0;

  for (i=0;i<t;i++)
    {
      for (j=0;j<l;j++)
        if (ia[j][b[i+1]] == b[i])
          {
            *zz = *zz + a[d[i]][j][b[i+1]];
            break;
          }
      if (j == l)
        {
          *ier = 1;
          return;
        }
    }

    /* Check for error in Path Probability */
    if (fabs(z / *zz - (double) 1.0) > DELTA) 
      {
        *ier = 2;
        return;
      }

    /*  Path Probability ok */
  return;
}
