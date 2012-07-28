#include <math.h>
#include "s8s.h"
#define MASK 0x7fffffff
s8s(int n, int k, int l, int t, long d[], double a[NMATRICES][NONZERO][MATRIXSIZE], long ia[NONZERO][MATRIXSIZE])
{
  unsigned int i,j,m,jj,kk,mm, irand(),ii;
  double tmp;
  long itemp;

  /*
   * Generate the A arrays with entries between 0 and 1
   */

   for (i=0; i<k; i++)
     {
       for (j=0; j<l; j++)
         {
           vrand(0,ia, n);
#pragma _CRI ivdep
           for (kk = 0; kk<n; kk++)
             {
               a[i][j][kk] = (MASK & ia[0][kk]) * pow(0.5,31.0);
               /*printf("a(%d,%d,%d) = %f\n", i,j,kk,a[i][j][kk]);
               printf("ia(0,%d) = %o\n",kk,ia[0][kk]); */
             }
           vrand(0, ia, n);
#pragma _CRI ivdep
           for (kk = 0; kk<n; kk++)
             {
               a[i][j][kk] = a[i][j][kk] + (MASK & ia[0][kk]) * pow(0.5,62.0);
               /*printf("a(%d,%d,%d) = %f\n", i,j,kk,a[i][j][kk]); */
             }
         }
       for(kk = 0; kk<n; kk++)
         {
           tmp = a[i][0][kk];
#pragma _CRI ivdep
           for (j=1; j<l; j++)
             {
               /*printf("a(%d,%d,%d) = %f \n", i,j,kk,a[i][j][kk]);  */
               tmp = tmp + a[i][j][kk];
             }
#pragma _CRI ivdep
           for (j=0; j<l; j++)
            {
              /*printf("a(%d,%d,%d) = %f ", i,j,kk,a[i][j][kk]);*/  
              a[i][j][kk] = a[i][j][kk]/tmp;
              /*printf("scaled a(%d,%d,%d) = %f\n", i,j,kk,a[i][j][kk]);  */
            }
         }
     }

   /*
    *  Generate the array of indices of the non-zero entries in A
    */
   for (j=0; j<l; j++)
     for (m = 0; m<n; m++) ia[j][m] = m;
   
   /*
    *  Randomly permute the array IA
    */
L100:
   /*printf("At 100\n"); */
   for (i=0; i<5; i++)
     for (j=0; j<l; j++)
       for (m = 0; m<n; m++)
         {
           jj = irand(0);
           mm = irand(0);
          /* printf("jj = %u, mm = %u\n",jj,mm); */
           jj = (jj % l);
           mm = (mm % n);
           /*printf("mod jj = %u, mm = %u\n",jj,mm); */
           itemp = ia[j][m];
           ia[j][m] = ia[jj][mm];
           ia[jj][mm] = itemp;
         }
/*
  for (i=0;i<4;i++)
    for (ii = 0;ii<1000;ii++)
      printf("%d\n",ia[i][ii]);
*/

   /*
    * Make final adjustments to prevent repeats in the index in a column
    */
   for (m=0; m<n; m++)
     {
L140:
     /* printf("at 140\n"); */
     for (j=1; j<l; j++)
       for (jj = 0; jj<j; jj++)
         {
           if (ia[j][m] == ia[jj][m])
             {
               if (m != n)
                 {
                   itemp = ia[j][m];
                   ia[j][m] = ia[j][m+1];
                   ia[j][m+1] = itemp;
                   goto L140;
                  }
                else
                  { goto L100; }
             }
         }
     }
   /*
    * Generate the D array
    */
   vrand(0,d,t);
   for (i=0; i<t; i++)
      d[i] = (d[i] % k);
}

#ifdef FORTRAN
      SUBROUTINE S8S(N,K,L,T,D,A,IA)

c
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
c   This subroutine generates the random data for the large sparse
c   matrix version of Benchmark #8.
c
c   Parameters:
c
c    Provided by calling routine:
c      N   =  The size of the A matrices
c      K   =  The number of A matrices
c      L   =  The number of non-zero rows and columns in each A matrix
c      T   =  The length of the D array
c
c    Returned by this routine:
c      D   =  An array of length T, holding integers mod K
c      A   =  K real NxN matrices of positive real numbers
c      IA  =  An integer array of indices of the non-zero entries in A
c
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c      
      INTEGER T
      REAL A(N,L,K)
      INTEGER IA(N,L)
      INTEGER D(T)
      PARAMETER (MASK=Z'7FFFFFFF')

c  Generate the A arrays with entries between 0 and 1
c
      DO 70 I=1,K
        DO 30 J=1,L
          CALL VRAND(0,IA(1,1),N)
          DO 10 KK = 1,N
            A(KK,J,I) = IAND(MASK,IA(KK,1))*0.5**31
  10      CONTINUE
          CALL VRAND(0,IA(1,1),N)
          DO 20 KK=1,N
c The following line is modified:
c           A(KK,J,I) = IAND(MASK,IA(KK,1))*0.5**62
            A(KK,J,I) = A(KK,J,I) + IAND(MASK,IA(KK,1))*0.5**62
  20      CONTINUE
  30    CONTINUE
c
        DO 60 KK = 1,N
          TMP = A(KK,1,I)
          DO 40 J=2,L
            TMP = TMP + A(KK,J,I)
  40      CONTINUE
          DO 50 J=1,L
            A(KK,J,I) = A(KK,J,I)/TMP
  50      CONTINUE
  60    CONTINUE
  70  CONTINUE
c
c  Generate the array of indices of the non-zero entries in A
      DO 90 J = 1,L
        DO 80 M = 1,N
          IA(M,J) = M
  80    CONTINUE
  90  CONTINUE
c      
c  Randomly permute the array IA
 100   CONTINUE
      DO 130 I=1, 5
        DO 120 J=1,L
          DO 110 M = 1,N
            JJ = MOD(IRAND(0), L)+1
            MM = MOD(IRAND(0), N)+1
            ITEMP = IA(M,J)
            IA(M,J) = IA(MM,JJ)
            IA(MM,JJ) = ITEMP
 110      CONTINUE
 120    CONTINUE
 130  CONTINUE
c
c  Make final adjustments to prevent repeats in the index in a column 
      DO 170 M=1,N
 140     CONTINUE
        DO 160 J=2,L
          DO 150 JJ=1,J-1
            IF(IA(M,J) .EQ. IA(M,JJ)) THEN
              IF(M .NE. N) THEN
                ITEMP = IA(M,J)
                IA(M,J) = IA(M+1,J)
                IA(M+1,J) = ITEMP
                GOTO 140
              ELSE 
                GOTO 100
              ENDIF
            ENDIF
 150      CONTINUE
 160    CONTINUE
 170  CONTINUE
c
c  Generate the D array
      CALL VRAND(0,D,T)
      DO 180 I = 1,T
        D(I) = MOD(D(I),K) + 1
 180  CONTINUE
c
      RETURN
      END
#endif
