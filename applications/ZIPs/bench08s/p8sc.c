#define restrict

/*

   Benchmark #8 -- Dynamic Program
   Large sparse matrices

   Parameters:

   Provided by the calling routine:
     A  =  K-long array of N by N matrices, packed into N by L arrays
     IA =  N by L array of indices for the packing of A
     D  =  T-long array of integers between 1 and K used to
           select the appropriate A matrix
     N  =  The size of the A matrices
     K  =  The number of A matrices
     L  =  Number of non-zeros in each row and column of each A matrix
     T  =  The length of D, and one less than the length of B
     D2 =  T = D1*D2, with neither of these equal to 1.

   Returned by this routine:
     B  =  T+1 long array containing the best path.
     Z  =  The log probability of this path

cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc

  Basic Algorithm:

  The input matrices are assumed to be compressed along their
  columns.  Thus there is an array containing the nonzero elements,
  compressed into L rows, and a same shaped array containing the 
  row-numbers of these elements.

  The basic algorithm is then straight-forward:

  For i = 1,2,..,N  Y[0](i) = 1.0
  For t = 1,2,...,T
      k = D(t)
      for i = 1,...,N
          Y[t](i) = A[k](i,1)*Y[t-1](IA[k](i,1))
          P[t](i) = IA[k](i,1)
      next i
      for j = 2,...,L
          for i = 1,...,N
              if A[k](i,j)*Y[t-1](IA[k](i,j)) > Y[t](i)
                  Y[t](i) = A[k](i,j)*Y[t-1](IA[k](i,j))
                  P[t](i) = IA[k](i,j)
              end if
          next i
      next j
  next t
  B(T) = 1
  YY = Y[T](1)
  For i = 2,...,N
      if Y[T](i) > YY
          YY = Y[T](i)
          B(T) = i
      end if
  Next i
  For t = T-1,T-2,...,1,0   set B(t) = P[t+1](B(t+1))

  Modifications to the Basic Algorithm:

  The algorithm would be as given in the Basic Algorithm above
  except for two things:  the numbers tend to underflow, and 
  the amount of storage required for all the Y and P arrays 
  would overflow memory on any existing computer.

  To solve the underflow problem, note that the i which maximizes
       Y[t](j) = max Y[t-1](i)*A[k](i,j)
  also maximizes 
       Log (Y[t](j)) = max Log (Y[t-1](i)) + Log (A[k](i,j))
  The matrices are non-negative.  Thus, all work will be done
  in the log domain.  (Zeros in the matrices, if any, will be replaced 
  with very small values.)

  To solve the memory overflow problem, there are three options.  One
  could 
     a) store most of the Y and P arrays on secondary storage; 
     b) compress the data so it does fit; or 
     c) store only a portion of the data, from which the rest can be 
       recomputed fairly quickly.  
  It is this last option that is implemented here.

  Pick two numbers, D1 and D2, such that D1*D2=T.  Then two passes can
  be made through the T steps.  The first saves the Y arrays for every
  D1'th step.  The second pass then takes the saved steps, starting
  with the last and working down, regenerating the P arrays for all the
  steps, and then generating the B array working backwards through
  the P's.

  The memory required for this solution is proportional to D1+D2, as
  opposed to D1*D2(=T).  Obviously, memory used will be minimal if
  D1=D2=SQRT(T).  For the parameter of T = 2000, this should fit on
  most modern high performance computers.  And this is what is
  implemented here.  As now coded, it is assumed that D1 * D2 = T, and
  that neither of these are equal to 1. 

  Note that the work goes up as the number of passes.  This work is
  needed to recompute the Y arrays.  Thus, this requires roughly  
  twice the work.

*/
#include <math.h>
#include <float.h>
#include "s8s.h" 
#include <unistd.h>
#include <time.h>
#include <stdio.h>

#define min(A,B) ((A) < (B) ? (A) : (B))
#define max(A,B) ((A) < (B) ? (B) : (A))
double y[2][MATRIXSIZE];	/* The vectors formed after each step        */
double yim[PART][MATRIXSIZE];   /* The intermediate values of Y being stored
				   at the end of each of the subdivisions of
                                   the first and second pass.  The first D3
                                   vectors are used for the first pass, and
                                   the remaining D2 for the second pass.     */
int pp[MATRIXSIZE*MD1];		/* The storage space for the D1 P vectors
                                   needed during each of the subdivisions of
                                   the last pass.                            */



void p8s(double a[NMATRICES][NONZERO][MATRIXSIZE],
         long ia[NONZERO][MATRIXSIZE],
         long d[DLEN],
         long b[DLEN+1],
         int n,
         int k,
         int l,
         int t,
         int d2,
         double *z)

/*
void p8s(double (* restrict a)[NONZERO][MATRIXSIZE],
         long (* restrict ia)[MATRIXSIZE],
         long (* restrict d),
         long (* restrict b),
         int n,
         int k,
         int l,
         int t,
         int d2,
         double *z)
*/
{
  int start;	/* offset of starting point of this subdivision of steps */
  int first;	/* Flag which is initialized to zero, indicating that the
                   first time FWBW is called, B(T) is to be calculated.
                   Thereafter, FIRST is set to 1			*/
  int d1;
  extern double y[2][MATRIXSIZE];
  extern double yim[PART][MATRIXSIZE];
  void g8s(),fw(),fwbw();
  int i,i2;
  double x0, y0;
  double cputime(), wall();

  d1 = t/d2;
  if ((d1 == 1) || (d2 == 1))
    {
      printf(" In p8s, both d1 and d2 must be greater than 1\n");
      printf(" d1 = %d, d2 = %d, t = %d\n", d1,d2,t);
      exit(-1);
    }

  x0 = -cputime();
  y0 = -wall();

  g8s(a,n,k,l);
 

  x0 += cputime();
  y0 += wall();

  /*printf("Time to convert matrices to logs\n");
  printf("CPU      = %12.4f\n",x0);
  printf("WallCloc = %12.4f\n",y0);*/

  /* Initialize Y[1] to be all log(1.0). */
  for(i=0; i<n; i++)
    {
      yim[0][i] = 0;
    }

  /* Begin the first pass */

  x0 = -cputime();
  y0 = -wall();



  /* Calculate the D2-1 intermediate values spaced by D1 steps.*/
  for (i2=1;i2<d2;i2++)
    {
      fw(ia,a,y,&(d[d1*(i2-1)]),&(yim[i2-1][0]),&(yim[i2][0]),n,k,d1,l);
    }

  x0 += cputime();
  y0 += wall();

  /*printf("Time for first pass\n");
  printf("CPU      = %12.4f\n",x0);
  printf("WallCloc = %12.4f\n",y0);*/


  /* Begin the second pass */
  first = 0;


  x0 = -cputime();
  y0 = -wall();

  for(i2=d2;i2; i2--)
    {
      start = d1 * i2-1;
      /*
       * Calculate the I2'th of D2 groups of subdivisions of D1 steps,
       * saving the P vector for each step, and backtrack computing B.
       */
      fwbw(ia,a,y,&(d[d1*(i2-1)]),pp,&(b[d1*(i2-1)]),&(yim[i2-1][0]),n,k,d1,l,first,z);
      /*
       * After the first call to FWBW, B(T) doesn't need to be calculated
       * anymore.
       */
      first = 1;
    }

  x0 += cputime();
  y0 += wall();

  /*printf("Time for second pass\n");
  printf("CPU      = %12.4f\n",x0);
  printf("WallCloc = %12.4f\n",y0);*/

  return;

}

void fw(long ia[NONZERO][MATRIXSIZE],
        double a[NMATRICES][NONZERO][MATRIXSIZE],
        double y[2][MATRIXSIZE],
        long d[DLEN],
        double in[],
        double out[],
        int n,
        int k,
        int t,
        int l)
/*
void fw(long (* restrict ia)[MATRIXSIZE],
        double (* restrict a)[NONZERO][MATRIXSIZE],
        double (* restrict y)[MATRIXSIZE],
        long * restrict d,
        double * restrict in,
        double out[],
        int n,
        int k,
        int t,
        int l)
*/
{
  int i,j,kk,ibuf, ibuf2;
  double temp;
  int zero=0;
  double cputime(), wall();
/*  void FLOWMARK(); */
  /* Do the first iteration from array IN to Y(,0) */
#pragma _CRI ivdep
  for(i = 0; i<n; i++) y[0][i] = in[ia[0][i]] + a[d[0]][0][i];

#pragma _CRI ivdep
  for(j = 1;j<l;j++)
#pragma _CRI ivdep
   for(i = 0;i<n; i++)
    {
      temp = in[ia[j][i]] + a[d[0]][j][i];
      y[0][i] = max(y[0][i], temp);
    }
  ibuf = 1;
  ibuf2 = 0;

  /* Do all but the last step, going between Y(,1) and Y(,0) */
  for(kk = 1; kk<t-1; kk++)
    {
/*      FLOWMARK("loop1"); */
#pragma _CRI ivdep 
      for(i=0;i<n;i++) y[ibuf][i] = a[d[kk]][0][i] + y[ibuf2][ia[0][i]];
      zero=0;
/*      FLOWMARK(&zero); */
      for(j = 1;j<l;j++)
        {
      
/*      FLOWMARK("loop2"); */
#pragma _CRI ivdep
        for(i = 0;i<n;i++)
          {
            temp = a[d[kk]][j][i] + y[ibuf2][ia[j][i]];
            y[ibuf][i] = max(y[ibuf][i],temp);

            /*if(kk >31 && kk < 42) */
            /*printf("%8.4f\n", y[ibuf2][ia[j][i]]); */
            /*printf("%d %d %d %d %d %8.4f %d\n",kk, i,j,d[kk],ia[j][i], y[ibuf2][ia[j][i]], ibuf2); */

          }
      zero=0;
/*      FLOWMARK(&zero); */
        }

      ibuf2 = ibuf;
      ibuf = 1-ibuf;
    }
  /*
   * On the last step, go between Y(,IBUF2) and OUT.
   */
#pragma _CRI ivdep 
  for(i=0;i<n;i++) out[i] = a[d[t-1]][0][i] + y[ibuf2][ia[0][i]];
  for(j=1;j<l;j++)
#pragma _CRI ivdep
    for (i=0;i<n;i++)
      {
        temp = a[d[t-1]][j][i] + y[ibuf2][ia[j][i]];
        out[i] = max(out[i],temp);
      }
    
}
/*
 * Subroutine to do the pass 4,  including backtracking.  This
 * routine needs to calculate and store the P vectors in order to
 * backtrack.  However, it does not need to store any Y vectors.
 */
void fwbw(long ia[NONZERO][MATRIXSIZE],
          double a[NMATRICES][NONZERO][MATRIXSIZE],
          double y[2][MATRIXSIZE],
          long d[DLEN],
          int pp[DLEN][MATRIXSIZE],
          long b[DLEN+1],
          double *in,
          int n,
          int k,
          int t,
          int l,
          int first,
          double *z)
{
  int i,j,kk,ii,ibuf,ibuf2;
  int zero=0;
  double temp;
  double cputime(), wall();
/*  void FLOWMARK(); */
  /* Do the first iteration from array IN to Y(,0) */
#pragma _CRI ivdep
  for(i=0;i<n;i++)
    {
      y[0][i] = in[ia[0][i]] + a[d[0]][0][i];
      pp[0][i] = ia[0][i];
    }
  for(j=1;j<l;j++)
#pragma _CRI ivdep
    for(i=0;i<n;i++)
      {
        temp = in[ia[j][i]] + a[d[0]][j][i];
        if (temp > y[0][i])
          {
            y[0][i] = temp;
            pp[0][i] = ia[j][i];
          }
      }
  ibuf = 1;
  ibuf2 = 0;

  /* Do the remaining steps, going between Y(,1) and Y(,0) */
  for(kk=1;kk<t;kk++)
    {
/*      FLOWMARK("loop3"); */
#pragma _CRI ivdep
      for(i=0;i<n;i++)
        {
          pp[kk][i] = ia[0][i];
          y[ibuf][i] = a[d[kk]][0][i] + y[ibuf2][ia[0][i]];
        }
      zero=0;
/*      FLOWMARK(&zero); */
      for(j = 1;j<l;j++){
/*      FLOWMARK("loop4"); */
#pragma _CRI ivdep
        for(i=0;i<n;i++)
          {
            temp = a[d[kk]][j][i] + y[ibuf2][ia[j][i]];
            if (temp > y[ibuf][i])
              {
                y[ibuf][i] = temp;
                pp[kk][i] = ia[j][i];
              }
          }
       zero=0;
/*      FLOWMARK(&zero); */
      }
       ibuf2 = ibuf;
       ibuf = 1 - ibuf;
    }
  /*
   * If these are the last steps, compute B(T) by taking the maximum
   * value in Y(T) and storing its position
   */
  if (!first)
    {
      temp = y[ibuf2][0];
      ii = 0;
      for(i = 1;i<n;i++)
        if (y[ibuf2][i] > temp)
          {
            temp = y[ibuf2][i];
            ii = i;
          }
      b[t] = ii;
      *z = temp;
    }
  /*
   * Backtrack over all the steps computed in this subroutine
   */
  for(i=t-1;i>-1;i--)
    b[i] = pp[i][b[i+1]];
}

/*
 * Subroutine to convert A matrices to logs, to eliminate the need to rescale.
 */

void g8s( double a[NMATRICES][NONZERO][MATRIXSIZE],
          int n,
          int k,
          int l)
{
 double fpmin = 1.17549435e-38f;
 int i,j,kk;

/*
#ifdef FPMIN
 fpmin=FPMIN;
#endif
#ifdef FLT_MIN
 fpmin = FLT_MIN;
#endif
*/
/* 
 * Scale each matrix
 */
 for(i=0;i<k;i++)
   for(j=0;j<l;j++)
     for(kk=0;kk<n;kk++)
       {
       if (a[i][j][kk] < fpmin) a[i][j][kk] = fpmin;
       a[i][j][kk] = log(a[i][j][kk]);
       } 
}

