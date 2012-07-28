/* CVS info   */
/* $Date: 2005/06/07 18:57:43 $     */
/* $Revision: 1.1 $ */
/* $RCSfile: EZtest.c,v $  */
/* $Name:  $     */
/*
  -how to compile in general: cc -D_REENTRANT -o EZtest EZtest.c -lpthread libEZThreads.a
  -you might need more options to tell the compiler to load
  reentrant versions of the standard C libraries
  -or maybe you might need something like -D_REENTRANT on
  the compile-line  
  -on the DEC you need the -pthread option to do this
  
  -compile DEC: cc -pthread -o EZtest EZtest.c libEZThreads.a
  -compile HP:  cc +DA2.0W -D_POSIX_C_SOURCE=199506L -o EZtest EZtest.c -lpthread libEZThreads.a
  -compile Sun: cc -mt -xO3 -xarch=v9 -o EZtest EZtest.c -lpthread libEZThreads.a

*/

#include "EZThreads.h"

#define LEN 100

typedef struct myarg_s
{
    int Value;
    float Const;
    char Buf[LEN];
}
MyArg_t;


/*
  worker routine for child threads
*/
void *Worker(void *aptr)
{
    /* this is a necessary line */
    /* need this line all the time */
    WorkerArg_t *arg = (WorkerArg_t *) aptr;

    /* this is a necessary line if you want to pass arguments */
    MyArg_t *myArgs = (MyArg_t *) EZfnArg(arg);

    unsigned int seed = EZthreadID(arg);

    int cntValue;
    int oldValue;

    /* remember these arguments are shared */
    printf("%d: %d, %f, %s\n",
	   EZthreadID(arg), myArgs->Value, myArgs->Const, myArgs->Buf);
    
    EZwaitBarSB(arg);

    /* sleep from 0 to 4 seconds */
    /* notice call to rand_r */
    sleep(rand_r(&seed)%5);

    /* print out my id */
    printf("hello from %d of %d threads\n", EZthreadID(arg), EZnThreads(arg));
    fflush(stdout);

    EZwaitBarSB(arg);

    printf("%d before barrier\n", EZthreadID(arg));
    fflush(stdout);

    EZwaitBarSB(arg);

    printf("%d after barrier\n", EZthreadID(arg));
    fflush(stdout);
    
    EZwaitBarSB(arg);

    cntValue = EZfetchIncSC(arg);
    printf("count up: %d got %d\n", EZthreadID(arg), cntValue);
    fflush(stdout);
    
    EZwaitBarSB(arg);
    if(EZthreadID(arg) == 0)
	oldValue = EZsetAtomicCntSC(arg, 50);
    EZwaitBarSB(arg);

    cntValue = EZfetchAddSC(arg, -1);
    printf("count dn: %d got %d\n", EZthreadID(arg), cntValue);
    fflush(stdout);
    
    EZwaitBarSB(arg);

    /* this is also a necessary line */
    /* need this line all the time */
    return((void *) NULL);
}


/*
  main routine
*/
int main(int argc, char *argv[])
{
    MyArg_t inArgs;
    int nThreads = 4;
    
    static char cvs_info[] = "BMARKGRP $Date: 2005/06/07 18:57:43 $ $Revision: 1.1 $ $RCSfile: EZtest.c,v $ $Name:  $";


    if(argc == 2)
	nThreads = atoi(argv[1]);

    if((nThreads <= 0)||(nThreads > 64))
	nThreads = 4;

    printf("nThreads = %d\n", nThreads);

    inArgs.Value = 100000;
    inArgs.Const = 3.1415;
    strncpy(inArgs.Buf, "This is a test", LEN-1);

    EZpcall(Worker, (void *) &inArgs, nThreads);
    
    return(0);
}

