#define min(A,B) ((A) < (B) ? (A) : (B))
#ifdef TEST
/* TEST PROGRAM TO CALL IRAND */
/******************************/
#define ISEED -1431
main()
{
unsigned int i,j,ib,k,irand();
long ia[5000];
void vrand();
  i = irand(ISEED);
  ib = irand(0);
  printf("0   %o\n", i);
  printf("1   %o\n", ib);

  for (j=0; j<20; j++)
    {
      for (k=0; k< 100; k++) vrand(0, ia, 5000);
      printf("%d   %o\n", j+2, ia[4999]);
    }
}

#endif

/*                                                                  
  This random number generator is as described on page 26         
  (Algorithm A) of "The Art of Computer Programming, Vol 2",      
                    by D.  E. Knuth.                              
                                                                  
  This algorithm uses a linear congruential generator to          
  fill a 55 long register with 32-bit numbers.  The generator     
  steps according to the rule X_N = X_N-24 + X_N-55 mod 2^32.     
  This has a cycle length of (2^55 - 1)*(2^31) or about 2^86,     
  when at least one of the initial 55 fill numbers is odd.        
  Since the largest run of even numbers in the initial fill       
  generator is 28, this condition is guaranteed.  Random          
  numbers are then successive entries in the continued sequence.  
                                                                  
  ISEED < 0 initialize the generator to iseed                     
                                                                  
  IRAND  the returned random deviate between 0 and 2^32           
                                                                  
  Subroutine VRAND(SEED,ARRAY,LENGTH)                             
  is a companion routine which does the same function, except     
  returns a vector of results of length N; regardless of         
  which is called, the same results will be returned.             
                                                                  
  April 19, 1993                                                  
*/

#define JQ 127773
#define JA 16807
#define JR 2836
#define JM 0x7fffffff
#define I32M1 0xffffffff
#define JSEED0 967560787
#define ILEN 1024

unsigned long ix[ILEN];			/* The random register */
int iflag = 0;
int ipoint;

unsigned int irand(int iseed)
{
  int jseed, jh, jl;

extern unsigned long ix[ILEN];                  /* The random register */
extern int iflag;
extern int ipoint;
int i;
/* printf("iflag = %d, ipoint = %d\n",iflag, ipoint); */
  if ((iseed < 0) ||  !iflag )
    {
      iflag = -1;
      jseed = -iseed;
      if ( jseed >= JM ||  jseed <= 0) jseed = JSEED0;
      for (i = 0; i < 55; i++)
        {
          jh = jseed/JQ;
          jl = jseed - (jh*JQ);
          jseed = JA*jl - JR*jh;
          if(jseed < 0) jseed = jseed + JM;
          ix[i] = jseed;
         }
      for (i = 0; i < 55; i++)
        {
          jh = jseed/JQ;
          jl = jseed - (jh*JQ);
          jseed = JA*jl - JR*jh;
          if (jseed < 0) jseed = jseed + JM;
          ix[i] = jseed ^ (ix[i] << 1);
        }
        /*
         * Run up generator
         */
        for (i = 55; i<ILEN; i++)
          {
            ix[i] = I32M1 & (ix[i-24] + ix[i-55]);
          }
        ipoint = 257;
      }
    if (ipoint > ILEN)		/* Have we used all the available numbers? */
      {

        for (i= 0; i<24; i++)
          {
            ix[i] = I32M1 & (ix[i + ILEN -24] + ix[i+ILEN - 55]);
          }
        for (i = 24; i<55; i++)
          {
            ix[i] = I32M1 & (ix[i-24] + ix[i+ILEN - 55]);
          }
        for (i = 55; i<ILEN; i++)
          {
            ix[i] = I32M1 & (ix[i-24] + ix[i - 55]);
          }
        ipoint = 1;
      }
    ipoint++;
    return ix[ipoint-2];
}
void vrand(long iseed, long iray[], int n)
{

int jseed, jh, jl;
void ranrun();
extern unsigned long ix[ILEN];                  /* The random register */
extern int iflag;
extern int ipoint;
int i, ii, jj;
  if ((iseed < 0) ||  !iflag )
    {
      iflag = -1;
      jseed = -iseed;
      if ( jseed >= JM ||  jseed <= 0) jseed = JSEED0;
      for (i = 0; i < 55; i++)
        {
          jh = jseed/JQ;
          jl = jseed - (jh*JQ);
          jseed = JA*jl - JR*jh;
          if(jseed < 0) jseed = jseed + JM;
          ix[i] = jseed;
        }
      for (i = 0; i < 55; i++)
        {
          jh = jseed/JQ;
          jl = jseed - (jh*JQ);
          jseed = JA*jl - JR*jh;
          if (jseed < 0) jseed = jseed + JM;
          ix[i] = jseed ^ (ix[i] << 1);
        }
      /*
       * Run up generator
       */
      for (i = 55; i<ILEN; i++)
        {
          ix[i] = I32M1 & (ix[i-24] + ix[i-55]);
        }
      ipoint = 257;
    }
  ii = 0;
  while (ii != n)
    {
      if (ipoint > ILEN)
        {
          ranrun();
          ipoint = 1;
        }
      /*
       * How many can we move?
       */
      jj = min(ILEN+1-ipoint, n - ii);
#pragma _CRI ivdep
      for(i=0; i<jj; i++) iray[ii+i] = ix[ipoint+i -1];
      ipoint = ipoint + jj;
      ii = ii + jj;
    }
  return;     
}

/*
 *    This routine generates the next ILEN entries in array ix.
 */
void ranrun()
{
  extern unsigned long ix[ILEN];                  /* The random register */
  extern int iflag;
  extern int ipoint;
  int i;
  for(i=0; i<24; i++) ix[i] = I32M1 & (ix[i+ILEN -24] + ix[i+ILEN - 55]);
  for(i=24; i<55; i++) ix[i] = I32M1 & (ix[i-24] + ix[i+ILEN -55]);
  for(i=55; i<ILEN; i++) ix[i] = I32M1 & (ix[i-24] + ix[i -55]);
  return;
}



#ifdef FORTRAN
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C                                                                      C
C                   Random Number Generator                            C
C                                                                      C
C  This program uses a 55 long shift register containing 30 bit        C
C  numbers as the random number generator.                             C
C                                                                      C
C  This is a program to generate a few specific values in the random   C
C  stream, to be used in comparisons with other versions of this       C
C  program.                                                            C
C                                                                      C
C  Aug 19, 1991                                                        C
C                                                                      C
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
c
c      REAL SECOND, BEF
c
c      DIMENSION IA(5000)
c      PARAMETER (ISEED = -1431)
c
c
cc  Initialize random number generator and run it up 1 000 000 ticks
c      BEF = SECOND()
c      I = IRAND(ISEED)
c
c      IB = IRAND(0)
c      PRINT 5, 1, IB
c 5    FORMAT(I5, 3X, O11)
c
c      DO 30 J=1,20
c
c         DO 20 K=1,100
c            CALL VRAND(0,IA,5000)
c 20      CONTINUE
c         PRINT 5, J+1,IA(5000)
c
c 30   CONTINUE
c
c      BEF = SECOND() - BEF
c      PRINT 35, BEF
c 35   FORMAT('Time = ',F12.5)
c
c      STOP
c      END
c
c
c
c
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c                                                                      c
c  This random number generator is as described on page 26             c
c  (Algorithm A) of "The Art of Computer Programming, Vol 2",          c
c                    by D.  E. Knuth.                                  c
c                                                                      c
c  This algorithm uses a linear congruential generator to              c
c  fill a 55 long register with 32-bit numbers.  The generator         c
c  steps according to the rule X_N = X_N-24 + X_N-55 mod 2^32.         c
c  This has a cycle length of (2^55 - 1)*(2^31) or about 2^86,         c
c  when at least one of the initial 55 fill numbers is odd.            c
c  Since the largest run of even numbers in the initial fill           c
c  generator is 28, this condition is guaranteed.  Random              c
c  numbers are then successive entries in the continued sequence.      c
c                                                                      c
c  ISEED < 0 initialize the generator to iseed                         c
c                                                                      c
c  IRAND  the returned random deviate between 0 and 2^32               c
c                                                                      c
c  Subroutine VRAND(SEED,ARRAY,LENGTH)                                 c
c  is a companion routine which does the same function, except         c
c  returns a vector of results of length N; regardless of             c
c  which is called, the same results will be returned.                 c
c                                                                      c
c  April 19, 1993                                                      c
c                                                                      c
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
      FUNCTION IRAND(ISEED)
c
      PARAMETER(JQ = 127773, JA = 16807, JR = 2836, JM = Z'7FFFFFFF')
      PARAMETER(JSEED0 = 967560787, ILEN = 1024, I32M1 = Z'FFFFFFFF')
c
c  The random register
      DIMENSION IX(ILEN)
c
c  For the linear congruential generator
      DATA IFLAG /0/
      COMMON /IRAN/ IFLAG,IPOINT,IX
c
c  Initialize the array
c
      IF((ISEED .LT. 0) .OR. (IFLAG .EQ. 0)) THEN
         IFLAG = -1
         JSEED = -ISEED
         IF((JSEED.GE.JM).OR.(JSEED.LE.0)) JSEED = JSEED0
         DO  I=1,55
            JH    = JSEED/JQ
            JL    = JSEED - (JH*(JQ))
            JSEED = JA*JL - JR*JH
            IF(JSEED.LT.0) JSEED = JSEED + JM
            IX(I) = JSEED
         ENDDO
         DO  I=1,55
            JH    = JSEED/JQ
            JL    = JSEED - (JH*(JQ))
            JSEED = JA*JL - JR*JH
            IF(JSEED.LT.0) JSEED = JSEED + JM
            IX(I) = IEOR(JSEED,ISHFT(IX(I),1))
         ENDDO
c
c  Run up generator
         DO 20 I=56, ILEN
            IX(I) = IAND(I32M1, IX(I-24) + IX(I-55))
 20      CONTINUE
c
         IPOINT = 257
      ENDIF
c
c  Have we used all of the available numbers?
      IF(IPOINT .GT. ILEN) THEN
c
c  Run up generator
         DO 30 I=1, 24
            IX(I) = AND(I32M1, IX(I+ILEN - 24) + IX(I+ILEN - 55))
 30      CONTINUE
c
         DO 40 I=25, 55
            IX(I) = IAND(I32M1, IX(I-24) + IX(I+ILEN - 55))
 40      CONTINUE
c
         DO 50 I= 56, ILEN
            IX(I) = IAND(I32M1, IX(I-24) + IX(I-55))
 50      CONTINUE
c
         IPOINT = 1
      ENDIF
c
      IRAND = IX(IPOINT)
      IPOINT = IPOINT + 1
c
      RETURN
      END
c
c
c
c
c
      SUBROUTINE VRAND(ISEED, IRAY, N)
c
      PARAMETER(JQ = 127773, JA = 16807, JR = 2836, JM = Z'7FFFFFFF')
      PARAMETER(JSEED0 = 967560787, ILEN = 1024, I32M1 = Z'FFFFFFFF')
c
c  The random register
      DIMENSION IX(ILEN), IRAY(1)
c
c  For the linear congruential generator
      DATA IFLAG /0/
c
      COMMON /IRAN/ IFLAG,IPOINT,IX
c
c
c  Initialize the array
      IF((ISEED .LT. 0) .OR. (IFLAG .EQ. 0)) THEN
         IFLAG = -1
         JSEED = -ISEED
         IF((JSEED.GE.JM).OR.(JSEED.LE.0)) JSEED = JSEED0
         DO  I=1,55
            JH    = JSEED/JQ
            JL    = JSEED - (JH*(JQ))
            JSEED = JA*JL - JR*JH
            IF(JSEED.LT.0) JSEED = JSEED + JM
            IX(I) = JSEED
         ENDDO
         DO  I=1,55
            JH    = JSEED/JQ
            JL    = JSEED - (JH*(JQ))
            JSEED = JA*JL - JR*JH
            IF(JSEED.LT.0) JSEED = JSEED + JM
            IX(I) = IEOR(JSEED,ISHFT(IX(I),1))
         ENDDO
c
c  Run up generator
         DO 20 I=56, ILEN
            IX(I) = IAND(I32M1, IX(I-24) + IX(I-55))
 20      CONTINUE
c
         IPOINT = 257
      ENDIF
c
      II = 0
c
 100  CONTINUE
      IF(IPOINT .GT. ILEN) THEN
         CALL RANRUN
         IPOINT = 1
      ENDIF
c
c  How many can we move?
      JJ = MIN(ILEN + 1 - IPOINT, N - II)
c
      DO 30 I=1, JJ
         IRAY(II+I) = IX(IPOINT + I -1)
 30   CONTINUE
c
      IPOINT = IPOINT + JJ
      II = II + JJ
c
c  Are we done?
      IF(II .EQ. N) RETURN
c
      GOTO 100
      END
c
c
c
c
c
      SUBROUTINE RANRUN
c
c     This routine generates the next ILEN entries in array IX
c
      PARAMETER (ILEN = 1024, I32M1 = Z'FFFFFFFF')
c
c  The random register
      DIMENSION IX(ILEN)
      COMMON /IRAN/ IFLAG,IPOINT,IX
c
      DO 30 I=1, 24
         IX(I) = IAND(I32M1, IX(I+ILEN - 24) + IX(I+ILEN - 55))
 30   CONTINUE
c
      DO 40 I=25, 55
         IX(I) = IAND(I32M1, IX(I-24) + IX(I+ILEN - 55))
 40   CONTINUE
c
      DO 50 I= 56, ILEN
         IX(I) = IAND(I32M1, IX(I-24) + IX(I-55))
 50   CONTINUE
c
      RETURN
      END
#endif
