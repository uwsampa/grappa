#include <sys/time.h>
#include <sys/resource.h>

#define SW_POPCNT	0
#define SW_LEADZ	0

/*************************************************************************
*
*     Following are software versions of the leading zero and popcount
*     functions needed for those architectures without these features.
*     It also contains versions of the timing routines.  The timing
*     routines most likely will need to be modified for each architecture.
*
*   Contents:
*       initpc  -  Popcount table initialization
*       popcnt  -  Popcount function
*       initlz  -  Leading zero table initialization
*       leadz   -  Leading zero function
*       wall    -  Wall Clock timer function
*       cputime -  CPU timer function
*
*************************************************************************/

#if SW_POPCNT

static int pops[65336];

void initpc()
{
/***************************************************************************
 *  For machines without popcnt hardware, define SW_POPCNT 1 at top of file.
 *  This routine initializes a 2^16 array to contain the popcount
 *  of each index.  Then the popcount of a 64 bit number can be 
 *  performed with four table look-ups.
 **************************************************************************/

   int i, j;
   int pops2[256];
   int pops3[16] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4}; /* 4-bit indices */

/* Generate array POPS2 to contain the popcount of 8 bit indices */
   for (i=0; i<16; i++)
   for (j=0; j<16; j++)
   {
      pops2[i*16+j] = pops3[i] + pops3[j];
   }

/* Finally, generate array POPS to contain the popcount of 16 bit indices */
   for (i=0; i<255; i++)
   for (j=0; j<255; j++)
   {
      pops[i*256+j] = pops2[i] + pops2[j];
   }
}

int popcnt( long numb )
{
/*  This routine uses the array initialized in INITPC to find the 
 *  popcount of a 64-bit number.
 */
   return
	pops [ 0xFFFF &  numb        ] +
	pops [ 0xFFFF & (numb >> 16) ] +
	pops [ 0xFFFF & (numb >> 32) ] +
	pops [ 0xFFFF & (numb >> 48) ];
}

#endif  /* #if SW_POPCNT */


#if SW_LEADZ

/************************************************************************
 * For machines without leadz hardware, define SW_LEADZ 1 at top of file.
 * 'initlz' initializes a 2^16 table to contain the position of
 * the leading zero of each index.
 * 'leadz' uses the table to handle 64-bit ints.
 *************************************************************************/

static int lzcnt[65536];
static long two48, two32, two16, mask16;

void initlz()
{
   int i, j;

   lzcnt[0] = 16;
   lzcnt[1] = 15;
   for (i=2; i<16; i++)
   {
      for ( j = 1 << (i-1); j < ((1 << i)); j++ )
      {
         lzcnt[j] = 16 - i;
      }
   }

   two48 = 1L << 48;
   two32 = 1L << 32;
   two16 = 1L << 16;
   mask16 = ( 1L << 16 ) - 1;
}

int leadz( long ival )
{
   if ( ival >= two48 )
      return      lzcnt [ (ival >> 48) & mask16 ];

   else
   if ( ival >= two32 )
      return 16 + lzcnt [ (ival >> 32) & mask16 ];

   else
   if ( ival >  mask16 )
      return 32 + lzcnt [ (ival >> 16) & mask16 ];

   else
      return 48 + lzcnt [ ival & mask16 ];
}

#endif  /* #if SW_LEADZ */



/*************************************************************************
 *
 *  wall() computes the amount of wall clock time that has elapsed.
 *
 *  Make suitable changes to provide access to wall clock time
 *  on architecture being used.
 *
 *************************************************************************/

#include <time.h>
#include <sys/time.h>
#include <unistd.h>
struct timeval tim;
struct timezone tz;

double wall()
{
   gettimeofday(&tim, &tz);
   return (double)tim.tv_sec + (double)(tim.tv_usec*0.000001);
}

char * date(void)
{
   gettimeofday(&tim, &tz);
   return ctime(&tim.tv_sec);
}


                              
/*************************************************************************
 *
 *  Computes the amount of cpu time that has elapsed.
 *
 *  Make suitable changes to provide access to cpu time on
 *  architecture being used.
 *
 *************************************************************************/
#ifndef __alpha
double cputime()
{
    return ( 0.000001 * (float)clock() );
}
#endif
#ifdef __alpha
double cputime()
{
    long microseconds;
    struct rusage resources;

    getrusage(RUSAGE_SELF, &resources);
    microseconds = (resources.ru_utime.tv_sec + resources.ru_stime.tv_sec)
                    * 1000000
                    + resources.ru_utime.tv_usec + resources.ru_stime.tv_usec;

    return  ( 0.000001 * microseconds);
}
#endif
