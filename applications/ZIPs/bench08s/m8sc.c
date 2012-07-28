/*

   Benchmark #8 -- Dynamic Program

   Large sparse matrices (Part b)

   Assumption: All values in the A matrices are nonnegative
               (Since these are state transition probabilities, this
                is a reasonable assumption.)

  The Problem:

  Given: A[1],A[2],...A[K],  N x N floating point matrices
          D, an 8-bit integer vector of length T where
              1 <= D(t) <= K for t = 1,2,...T

  Calculate: Y[0],Y[1],...Y[T] 64-bit floating point vectors of length N
            P[1],P[2],...P[T] 32-bit integer vectors of length N
            B, a T+1 long 32-bit integer vector where
               1 <= B(t) <= N for t = 0,1,2,...T
  according to the following algorithm:

     Y[0](i) = 1 for i = 1,2,...N
     For t = 1,2,...,T
         k = D(t)
         For j = 1,2,...,N
             Y[t](j) = max Y[t-1](i)*A[k](i,j)
                        i
             P[t](j) = any i which maximized Y[t](j)
         Next j
     Next t

     Let Z = max Y[T](i), and let I be any i which maximized Z.
              i

     Set B(T) = I
     For t = T-1,T-2,...,1,0
         Set B(t) = P[t+1](B(t+1))
     Next t

  Output Z and B(t) for t = 0,1,2,...T.

  Parameters:    N = 10**6   K = 2   T = 2000

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc

   Main Program for the large, sparse matrix version of Benchmark #8

   Call time parameters:

     N    =  Size of the A matrices
                Maximum = Default = 10**6

     K    =  Number of A matrices
                Maximum = 5
                Default = 2

     L    =  The number of non-zero entries per row and column.
                Maximum = 5
                Default = 4

     T    =  Length of D array
                Maximum = Default = 2000

     D2   =  Partitions of T.  D1 and D2 are greater than 1, and
             their product is T.  (D1 is computed from D2)
                Maximum = Default = 50

************************************************************************* */
#include "s8s.h"
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>



/* Define these matrices global to keep the stack from getting too large. */

  double a[NMATRICES][NONZERO][MATRIXSIZE];
  long ia[NONZERO][MATRIXSIZE];
  long d[DLEN];			/* Which matrix to use in each step */
  long b[DLEN+1];		/* Best path */

int main( int argc, char *argv[] ) 
{

  int ier;			/* Error flag */
  int n = -1;			/* The size of the A matrices */
  int k = -1;			/* The number of A matrices */
  int l = -1;			/* The number of non-zero entries */
  int d1, d2=-1, t=-1;                /* d1 * d2 = t */
  double z,zz;
  int i,j;
  double cputime(), wall();
  double setcputime;               /* CPU time for setup */
  double nsetcputime;               /* CPU time for setup */
  double setrealtime;              /* Real time for setup */
  double runcputime;               /* CPU time for benchmark */
  double runrealtime;              /* Real time for benchmark */
  double ncputime;


  /* The array to hold the matrices, and the sparse index */
  extern double a[NMATRICES][NONZERO][MATRIXSIZE];
  extern long ia[NONZERO][MATRIXSIZE];
  extern long d[DLEN];			/* Which matrix to use in each step */
  extern long b[DLEN+1];			/* Best path */
  int iii,ii;

  /* Get the input parameters */

  /* Get the input parameters */
  if (argc >1) n = atoi(argv[1]);
  if (argc >2) k = atoi(argv[2]);
  if (argc >3) l = atoi(argv[3]);
  if (argc >4) t = atoi(argv[4]);
  if (argc >5) d2 = atoi(argv[5]);

  /* Get N, the matrix size */
  if (n <= 0) n = MATRIXSIZE;
  if (n > MATRIXSIZE)
    {
      printf("Maximum allowed value for n is %d\n",MATRIXSIZE);
      exit(-1);
    }
  /* Get K, the number of matrices */
  if (k <= 0) k = NMATRICES;
  if (k > NMATRICES)
    {
      printf("Maximum allowed value for k is %d\n",NMATRICES);
      exit(-1);
    }
  /* Get l, the number of non-zero entries per row and column */
  if (l <= 0) l = NONZERO;
  if (l > NONZERO)
    {
      printf("Maximum allowed value for l is %d\n",NONZERO);
      exit(-1);
    }
  /* Get t, the length of the D array */
  if (t <= 0) t = DLEN;
  if (t > DLEN)
    {
      printf("Maximum allowed value for t is %d\n",DLEN);
      exit(-1);
    }
  /* Get d2,  the partitions of T */
  if (d2 <= 0) d2 = PART;
  if (d2 > PART)
    {
      printf("Maximum allowed value for d2 is %d\n",PART);
      exit(-1);
    }
    
    d1 = t/d2;
  if (d1 > PART)
    {
      printf("Maximum allowed value for d1 is %d\n",PART);
      exit(-1);
    }
  d1 = d1 * d2;
  if (d1 != t) 
    {
      printf ("d2 does not divide t; t = %d, d2 = %d\n",t,d2);
      exit(-1);
    }

  /* Initialize the random # generator */
  i = irand(-999081);


  /* Begin timing here */
  setcputime = cputime();
  setcputime = -cputime();
  
  setrealtime = -wall();

  /* Initialize timing variables */

  s8s(n,k,l,t,d,a,ia);

  /* End timed setup */
  
  setcputime += cputime(); 
  setrealtime += wall();

/*
   for(iii = 0;iii<2;iii++)
   for(ii = 0;ii<4;ii++)
   for(i=0; i<1000;i++) {
      printf(" %8.4f\n",a[iii][ii][i]);
   }
  for (i = 0; i<DLEN;i++) printf("%d\n",d[i]);
  for (i=0;i<4;i++)
    for (ii = 0;ii<1000;ii++)
      printf("%d\n",ia[i][ii]);
*/

  /* Begin timing here */
  runcputime = -cputime();
  runrealtime = -wall();




  /* Do the real work */
  p8s(a,ia,d,b,n,k,l,t,d2,&z); 

  /* End timed setup */
  runcputime += cputime();
  runrealtime += wall();


  /* Check the results */
  c8s(n,k,l,t,a,ia,b,d,z,&zz,&ier); 

  /* Output results */
  r8s(n,k,l,t,d2,z,zz,b,setcputime,setrealtime,runcputime,runrealtime,ier);
exit(1);
}
