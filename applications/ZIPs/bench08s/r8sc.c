/*
     This is an output routine for Benchmark 8, part b, large
     sparse matrices.

     This routine will need to be modified to contain the particular
     configuration information requested.

     Parameters:

       Provided by calling routine:
         N     =  Size of A matrices
         K     =  Number of A matrices.
         L     =  Number of non-zeros in each row and column of each A
                  matrix
         T     =  Length of B array -1
         D2    =  Used to determine partitions of T
         Z     =  Probability of best path
         ZZ    =  Expected path probability
         B     =  Sequences of steps on a maximal path
         CSET  =  CPU time required to do the setup     
         WSET  =  Wall clock time required to do the setup
         CRUN  =  CPU time required to do the benchmark
         WRUN  =  Wall clock time required to do the benchmark
         IER   =  Error flag

*/
#include <time.h>
#include "s8s.h"
#ifndef _SYS_TIME_H
#include <sys/time.h>
#endif
void r8s(int n,
        int k,
        int l,
        int t,
        int d2,
        double z,
        double zz,
        long b[DLEN+1],
        double cset,
        double wset,
        double crun,
        double wrun,
        int ier)

{
struct timeval tp;
struct timezone tzp;
int ret,i,d1,j,jj;
char *cret;
  

  printf("Benchmark #8s -- Dynamic Program \n Large Sparse Matrices\n");
  ret = gettimeofday(&tp,&tzp);
  if (ret)
    {
      printf(" getttimeofday returned %d\n",ret);
      exit(-1);
    }
  cret = ctime(&tp.tv_sec);
  printf("Date:    %s\n\n",cret);
  d1 = t/d2;
  printf("Matrix Size = %d\n",n);
  printf("Entries per column   =  %d\n",l);
  printf("Number of matrices   =  %d\n",k);
  printf("Length of solution   =  %d\n",t);
  printf("Partition sizes     = %d   %d\n\n",d1, d2);

  printf("Time for set up: \n");
  printf("time: Setup CPU    = %12.4f  seconds\n",cset);
  printf("time: Setup Wall   = %12.4f  seconds\n",wset);
  
  printf("Time to run: \n");
  printf("time: Run CPU    = %12.4f  seconds\n",crun);
  printf("time: Run Wall   = %12.4f  seconds\n\n",wrun);

  printf("Lg probability of path = \n");
  printf("data:                      %16.4f\n\n",z);
  printf("A few of the steps are:\n");
  printf("Steps 0 through 19\n");
  printf("data: ");
  for(i=0;i<10;i++) printf("%8d",b[i]+1);
  printf("\n");
  printf("data: ");
  for(i=10;i<20;i++) printf("%8d",b[i]+1);
  printf("\n\n");
  j = t/2;
  jj = j+19;
  printf("Steps %d through %d\n",j,jj);
  printf("data: ");
  for(i=j;i<jj-9;i++) printf("%8d",b[i]+1);
  printf("\n");
  printf("data: ");
  for(i=jj-9;i<jj+1;i++) printf("%8d",b[i]+1);
  printf("\n\n");

  j = t-19;
  jj = t;
  printf("Steps %d through %d\n",j,jj);
  printf("data: ");
  for(i=j;i<jj-9;i++) printf("%8d",b[i]+1);
  printf("\n");
  printf("data: ");
  for(i=jj-9;i<jj+1;i++) printf("%8d",b[i]+1);
  printf("\n\n");
}
/* 
      PRINT 55,(B(I),I=0,19)
  55  FORMAT('Steps 0 through 19',/,10I8,/,10I8,/)
      J=T/2
      JJ=J+19
      PRINT 65,J,JJ,(B(I),I=J,JJ)
  65  FORMAT('Steps ',I7,' through ',I7,/,10I8,/,10I8,/)
      J=T-19
      JJ=T
      PRINT 65,J,JJ,(B(I),I=J,JJ)
c
      IF (IER .EQ. 1) THEN
         PRINT 75, I
  75     FORMAT(//,' *** Error in described path *** ',
     $             '   Invalid transition on step ',I5,//)
         RETURN
      ENDIF
c
      IF(IER .EQ. 2) THEN
         PRINT 85, Z, ZZ, ABS(Z/ZZ-1)
  85     FORMAT(//,' *** Error *** ',
     $         ' Path probability = ',E16.6,/,
     $         'Expected ',E16.6,'  Difference = ',E16.6,//)
         RETURN
      ENDIF
c
      PRINT 87
  87  FORMAT(//,'*** NO ERRORS DETECTED ***',//)
      RETURN
      END
*/
