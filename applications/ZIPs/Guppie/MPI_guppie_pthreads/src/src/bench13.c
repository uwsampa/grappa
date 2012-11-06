
/* CVS info   */
/* $Date: 2005/06/07 18:59:10 $     */
/* $Revision: 1.1 $ */
/* $RCSfile: bench13.c,v $  */
/* $Name:  $     */

/* 
   Purpose:	
	Random table updates on a very large table
	upto half of global memory
*/

/*
  -how to compile in general: cc -D_REENTRANT -o bench13 bench13.c -lpthread libEZThreads.a
  -you might need more options to tell the compiler to load
  reentrant versions of the standard C libraries
  -or maybe you might need something like -D_REENTRANT on the compile-line  
  -on the DEC you need the -pthread option to do this
  
  -compile DEC: cc -pthread -o bench13 bench13.c libEZThreads.a
  -compile HP:  cc +DD64 -D_POSIX_C_SOURCE=199506L -o bench13 bench13.c -lpthread libEZThreads.a
  -compile Sun: cc -mt -xO3 -xarch=v9 -o bench13 bench13.c -lpthread libEZThreads.a

*/

#include "EZThreads.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/times.h>
#include <errno.h>
#include <mpi.h>

/* Types used by program (should be 64 bits) */
typedef unsigned long uint64;
typedef long int64;

/* Macros for timing */
struct tms t;
#define WSEC() (times(&t) / (double)sysconf(_SC_CLK_TCK))
#define CPUSEC() (clock() / (double)CLOCKS_PER_SEC)
#define RTSEC() (clock() / (double)CLOCKS_PER_SEC)

/* Random number generator */
#define POLY 0x0000000000000007UL
#define PERIOD 1317624576693539401L

/* Log size of main table (suggested: half of global memory) */
#define LTABSIZE 25L
#define TABSIZE (1L << LTABSIZE)

/* Number of updates to table (suggested: 4x number of table entries) */
#define NUPDATE (4L * TABSIZE)

#define VLEN 128

/* Allocate main table (in global memory) */
uint64 *table;
int64 tabsize;
int64 nfailures;
int64 nupdate;
double icputime;               /* CPU time to init table */
double is;
double cputime;               /* CPU time to update table */
double s;

/* arg struct for EZpcall() */
typedef struct myarg_s
{
    int64 ltabsize;
    int64 tabsize;
    int64 nupdate;
}
MyArg_t;


/*
  Utility routine to start random number generator at Nth step
*/
uint64 startr(int64 n)
{
    int i, j;
    uint64 m2[64];
    uint64 temp, ran;
    
    static char cvs_info[] = "BMARKGRP $Date: 2005/06/07 18:59:10 $ $Revision: 1.1 $ $RCSfile: bench13.c,v $ $Name:  $";


    while (n < 0) n += PERIOD;
    while (n > PERIOD) n -= PERIOD;
    if (n == 0) return 0x1;

    temp = 0x1;
    for (i=0; i<64; i++)
    {
	m2[i] = temp;
	temp = (temp << 1) ^ ((int64) temp < 0 ? POLY : 0);
	temp = (temp << 1) ^ ((int64) temp < 0 ? POLY : 0);
    }
    
    for (i=62; i>=0; i--)
	if ((n >> i) & 1)
	    break;

    ran = 0x2;
    while (i > 0)
    {
	temp = 0;
	for (j=0; j<64; j++)
	    if ((ran >> j) & 1)
		temp ^= m2[j];
	ran = temp;
	i -= 1;
	if ((n >> i) & 1)
	    ran = (ran << 1) ^ ((int64) ran < 0 ? POLY : 0);
    }
  
    return ran;
}    

/********************************
 divide up total size (loop iters or space amount) in a blocked way
*********************************/
void Block(int mype,
           int npes,
           int64 totalsize,
           int64 *start,
           int64 *stop,
           int64 *size)
{
    int64 div;
    int64 rem;

    div = totalsize / npes;
    rem = totalsize % npes;

    if (mype < rem)
    {
        *start = mype * (div + 1);
        *stop   = *start + div;
        *size  = div + 1;
    }
    else
    {
        *start = mype * div + rem;
        *stop  = *start + div - 1;
        *size  = div;
    }
}

/*
  update_table_1 -- single-threaded update_table
*/
void update_table_1(MyArg_t *myArgs)
{
    int64 i;
    int64 j;
    uint64 ran[VLEN];              /* Current random numbers */
    uint64 temp;
    int64 start, stop, size;
    int64 ltabsize;
    
    ltabsize = myArgs->ltabsize;
    tabsize = myArgs->tabsize;
    nupdate = myArgs->nupdate;
    
    /* Begin timing here */
    icputime = -CPUSEC();
    is = -WSEC();
    
    for (i = 0; i < tabsize; i++)
	table[i] = i;

    /* End timing here */
    icputime += CPUSEC();
    is += WSEC();

    /* Begin timing here */
    cputime = -CPUSEC();
    s = -WSEC();

    for (j=0; j<128; j++)
	ran[j] = startr(j * (nupdate/128));
    for (i=0; i<nupdate/128; i++)
    {
#pragma ivdep
	for (j=0; j<128; j++)
	{
	    ran[j] = (ran[j] << 1) ^ ((int64) ran[j] < 0 ? POLY : 0);
	    table[ran[j] & (tabsize-1)] ^= ran[j];
	}
    }
    
    /* End timed section */
    cputime += CPUSEC();
    s += WSEC();
    /* Print timing results */
    printf ("init(c= %.4lf w= %.4lf) up(c= %.4lf w= %.4lf) up/sec= %.0lf\n",
	    icputime, is, cputime, s, ((double)nupdate / s));

    /* Verification of results (in serial or "safe" mode; optional) */
    temp = 0x1;
    for (i=0; i<nupdate; i++)
    {
	temp = (temp << 1) ^ (((int64) temp < 0) ? POLY : 0);
	table[temp & (tabsize-1)] ^= temp;
    }
    
    nfailures = 0;
    for (i=0; i<tabsize; i++)
	if (table[i] != i)
	    nfailures++;
    
    printf ("Found %d errors in %d locations (%s).\n",
	    j, tabsize, (j <= 0.01*tabsize) ? "passed" : "failed");

    /* Added by mdlewis for Bogobits */
    printf("##%9.6lf\n\n", ((double)nupdate / s) / 1000000);
}


/*
  update_table_2 -- multi-threaded update_table for 2 or more threads
*/
void update_table_2(WorkerArg_t *arg)
{
    int64 i;
    int64 j;
    uint64 ran[VLEN];              /* Current random numbers */
    uint64 temp;
    int64 start, stop, size;
    int64 ltabsize;
    
    /* this is a necessary line if you want to pass arguments */
    MyArg_t *myArgs = (MyArg_t *) EZfnArg(arg);
    ltabsize = myArgs->ltabsize;
    tabsize = myArgs->tabsize;
    nupdate = myArgs->nupdate;
    
    EZwaitBarSB(arg);

    if(EZthreadID(arg) == 0)
    {
	/* Begin timing here */
	icputime = -CPUSEC();
	is = -WSEC();
    }

    /* Initialize main table */
    Block(EZthreadID(arg), EZnThreads(arg), tabsize, &start, &stop, &size);
    
    for (i = start; i < stop; i++)
	table[i] = i;

    EZwaitBarSB(arg);

    if(EZthreadID(arg) == 0)
    {
	/* Begin timing here */
	icputime += CPUSEC();
	is += WSEC();
    }

    if(EZthreadID(arg) == 0)
    {
	/* Begin timing here */
	cputime = -CPUSEC();
	s = -WSEC();
    }
    
    EZwaitBarSB(arg);

    Block(EZthreadID(arg), EZnThreads(arg), nupdate, &start, &stop, &size);

    for (j=0; j<128; j++)
	ran[j] = startr(start + (j * (size/128)));
    for (i=0; i<size/128; i++)
    {
#pragma ivdep
	for (j=0; j<128; j++)
	{
	    ran[j] = (ran[j] << 1) ^ ((int64) ran[j] < 0 ? POLY : 0);
	    table[ran[j] & (tabsize-1)] ^= ran[j];
	}
    }

    EZwaitBarSB(arg);

    if(EZthreadID(arg) == 0)
    {
	/* End timed section */
	cputime += CPUSEC();
	s += WSEC();
        
        /* Print timing results */
	/*printf ("init(c= %.4lf w= %.4lf) up(c= %.4lf w= %.4lf) up/sec= %.0lf\n",
		icputime, is, cputime, s, ((double)nupdate / s)); */
	/* Verification of results (in serial or "safe" mode; optional) */
	temp = 0x1;
	for (i=0; i<nupdate; i++)
	{
	    temp = (temp << 1) ^ (((int64) temp < 0) ? POLY : 0);
	    table[temp & (tabsize-1)] ^= temp;
	}
	
	nfailures = 0;
	for (i=0; i<tabsize; i++)
	    if (table[i] != i)
		nfailures++;
	/*
	printf ("Found %d errors in %d locations (%s).\n",
		nfailures, tabsize, (nfailures <= 0.01*tabsize) ? "passed" : "failed");
        */
    }

}

/*
  worker routine for child threads
*/
void *Worker(void *aptr)
{
    /* this is a necessary line */
    /* need this line all the time */
    /* remember these arguments are shared */
    WorkerArg_t *arg = (WorkerArg_t *) aptr;

    /* this is a necessary line if you want to pass arguments */
    MyArg_t *myArgs = (MyArg_t *) EZfnArg(arg);

    update_table_2(arg);

    /* this is also a necessary line */
    /* need this line all the time */
    return((void *) NULL);
}

/*
  main routine
*/
int main(int argc, char *argv[])
{
    char name[100]; /* node name from MPI */
    int length, rank, size, i, j;
    char *Month[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                       "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
                      };
    MPI_Init(&argc, &argv);  /* start MPI */
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);	/* get current process id */
    MPI_Comm_size (MPI_COMM_WORLD, &size);	/* get number of processes */
    MPI_Get_processor_name(name, &length);  /* get node name */
    
    MyArg_t inArgs;
    int nThreads = 1;

    inArgs.ltabsize = LTABSIZE;
    inArgs.tabsize = TABSIZE;	
    inArgs.nupdate = NUPDATE;	

    if(argc >= 2)
    {
	nThreads = atoi(argv[1]);
    }
    
    if(argc >= 3)
    {
	inArgs.ltabsize = (atoi(argv[2]));
	inArgs.tabsize = 1L << inArgs.ltabsize;
    }
    
    if(argc >= 4)
    {
	inArgs.nupdate = 1L << (atoi(argv[3]));	
    }
    
    table = malloc(inArgs.tabsize * sizeof(uint64));
    if (table == NULL)
    {
	perror("malloc");
	exit(1);
    }
    
    /* Print parameters for run */
    if(rank==0) {
      printf("nProcs = %d nThreads = %d\n", size, nThreads);
      printf("Main table size = 2^%ld = %ld words\n", inArgs.ltabsize, inArgs.tabsize);
      printf("Number of updates = %ld\n", inArgs.nupdate);
    }

    if(nThreads == 1)
    {
	update_table_1(&inArgs);
    }
    else if(nThreads > 1)
    {
	EZpcall(Worker, (void *) &inArgs, nThreads);
    }
    else
    {
	printf("Number of threads must at least one!!!\n");
    }
   //output each MPI process' results
   char filename2 [50];
   struct tm* tm;
   time_t now;
   now = time(0); // get current time
   tm = localtime(&now); // get structure
   MPI_Barrier( MPI_COMM_WORLD );
   if(rank == 0) sprintf(filename2, "node_%02d%s%04d:%02d:%02d", tm->tm_mday, Month[tm->tm_mon], tm->tm_year+1900, tm->tm_hour, tm->tm_min);
   MPI_Bcast(filename2, 50, MPI_CHAR, 0, MPI_COMM_WORLD);
   FILE *fp = fopen(filename2, "a");
   if(fp != NULL)
   {
      fprintf(fp, "%d, %s, %.2f, %ld\n", rank, name, (nupdate/s)/1000000, nfailures);
      fclose(fp); 
   } 
   else 
   {
      perror(filename2);
   }
    double sum, min, max;
    int64 failures;
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Reduce(&s, &sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&s, &min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&s, &max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&nfailures, &failures, 1, MPI_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
    if(rank==0) {
      printf("Results (in mups)\n");
      printf("min: %.2f max: %.2f avg: %.2f\n", (nupdate/max)/1000000, (nupdate/min)/1000000, (nupdate/(sum/size))/1000000);
      //Should modify to be more informative. How many nodes failed?
      printf ("Found max of %d errors in %d locations (%s).\n",
          failures, tabsize, (nfailures <= 0.01*tabsize) ? "passed" : "failed");
    }
    MPI_Finalize();
    return(0);
}





