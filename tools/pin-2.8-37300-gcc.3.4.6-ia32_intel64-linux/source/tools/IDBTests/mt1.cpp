/*NO LEGAL*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

int numThreads = 1;
const int RunForever = -1;
int runCnt = 1;

const char* prog = "mt1";

typedef void* ((*ThdFun)(void*));

struct ThdArg {
    int  thdNum;
    int  runCnt;
    unsigned int sleepTime;
};
const unsigned int RandSleep = 0;
unsigned int nsecSleep = RandSleep;

enum ThdState {
    Init = 0,
    Run = 1,
    End = 2,
    Gone = 3
};

struct ThreadCtrl {
    ThdState thdState;
    pthread_t thd;
    ThdFun thdFun;
    ThdArg* thdArg;
};

ThreadCtrl* threadCtrl;

#define EC_LIST\
    EC_ID(Ok, 0) \
    EC_ID(TooFewArguments, 1) \
    EC_ID(BadNumberOfThreads, 2) \
    EC_ID(OutOfMemory, 3) \
    EC_ID(UnknownArgument, 4) \
    EC_ID(ThreadCreateFailed, 5) \
    EC_ID(BadRunCount, 6) \
    EC_ID(BadSleepTime, 7)
                                                                                
    enum    EC {
#define EC_ID(X,Y) X=Y,
        EC_LIST
#undef EC_ID
    };

const int max2 = 10;
struct data2 {
	int _d2_m1;
	char* _d2_m2[max2];
	};

const int max1_0 = 20;
const int max1_1 = 20;
struct data1 {
	char* _d1_m1;
	int _d1_m2[max1_0];
	struct data2 _d1_m3[max1_1];
};

const int max0 = 100;
data1 v1[max0];


const int maxd = 5;
#define s0 "_d1_m1"
const int smax0 = sizeof(s0)+maxd;
char buf0[max0*smax0];
#define s1 "_d2_m2"
const int smax1 = sizeof(s1)+2*maxd+1;
char buf1[max0*max1_1*max2*smax1];
    
void initData()
{
    char* p0 = buf0;
    char* p1 = buf1;

    for(int i = 0;i < max0; ++i) {
	sprintf(p0,"%s%05d",s0,i);
	v1[i]._d1_m1 = p0;
	p0 += smax0;
	
	for(int j = 0; j < max1_0;++j) {
		v1[i]._d1_m2[j] = (i+1)*1000+j;
	}
	for(int k = 0; k < max1_1;++k) {
		v1[i]._d1_m3[k]._d2_m1 = (i+1)*10000+k;
		for(int m = 0; m < max2; ++m) {
			sprintf(p1,"%s%05d_%05d",s1,k,m);
			v1[i]._d1_m3[k]._d2_m2[m] = p1;
			p1 += smax1;
		}
	}
    }
	printf("v1=%d bytes\n",sizeof(v1));
}

int error(int ec, const char* opt = 0) 
{
    fprintf(stderr,"\n");
    fprintf(stderr,"%s-Error", prog);
    if(opt)
	fprintf(stderr," (%s) ", opt);
    fprintf(stderr,":");
    switch(ec) {
#define EC_ID(X,Y) \
	case X : fprintf(stderr,"%s", #X); break;
	EC_LIST
#undef EC_ID
	default: fprintf(stderr,"%d", ec); break;
    }
    fprintf(stderr,"\n\n");
return ec;
}

int usage(int ec, const char* opt = 0) 
{
    error(ec, opt);

    fprintf(stderr,"usage: %s <options>\n", prog);
    fprintf(stderr,"        -c    ... run forever\n");
    fprintf(stderr,"        -rc # ... of thread local iterations (1..n)\n");
    fprintf(stderr,"        -s #  ... nsec to sleep in an iteration (1..n)\n");
    fprintf(stderr,"        -t #  ... of threads (1..n)\n");

return ec;
}

void inThreadLoop()
{
}

void* threadFun(void* arg) 
{
    ThdArg* thdArg = (ThdArg*)arg;
    threadCtrl[thdArg->thdNum-1].thdState = Run;
    printf("\nthread %d - started-\n", thdArg->thdNum);
    
    sleep(2);

    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = thdArg->sleepTime;

    srand(time(0));

    if(thdArg->runCnt == RunForever) {
	int runCnt = 1;
        while(1) {
	    printf("-%d.%d-",thdArg->thdNum, runCnt++);
	    inThreadLoop();
	    if(thdArg->sleepTime == RandSleep)
        	req.tv_nsec = ((unsigned int)rand()*10000) % 100000;
        //	printf("(%d)",req.tv_nsec);
	    nanosleep(&req, 0);
        }
    } else {
        while(thdArg->runCnt--) {
	    printf("-%d.%d-",thdArg->thdNum, thdArg->runCnt);
	    inThreadLoop();
	    if(thdArg->sleepTime == RandSleep)
        	req.tv_nsec = ((unsigned int)rand()*10000) % 100000;
        //	printf("(%d)",req.tv_nsec);
	    nanosleep(&req, 0);
        }
    }

    printf("\nthread %d - ended-\n", thdArg->thdNum);
    fflush(stdout);
    threadCtrl[thdArg->thdNum-1].thdState = End;

return 0;
}

void allRun1st()
{
	printf("\n*** all run ***\n");
}

void allRun()
{
static int cnt = 0;
	if(!cnt++) {
		allRun1st();
	}
}

int waitForCompletion()
{
int busy;
    do {
	int runThreads = 0;
	busy = 0;
	for(int i = 0;i < numThreads; ++i) {
	    switch(threadCtrl[i].thdState) {
	    case Run:
		runThreads++;
		busy = 1;
		break;
	    case Init:
		busy = 1;
		break;
            case End:
		pthread_join(threadCtrl[i].thd, 0);
		threadCtrl[i].thdState = Gone;
		printf("\nthread %d - Gone-\n",i+1);
		break;
	    case Gone:
		break;
	    }
	}
	if(runThreads == numThreads) 
		allRun();
    } while(busy);
    printf("\n");

return Ok;
}

int doArgs(int argc, char **argv) 
{

    prog = *argv++;

    if(argc == 1) {
	return usage(TooFewArguments);
    }
    while(--argc) {
	if(!strcmp(*argv,"-t")) {
	    if(--argc) {
		if((numThreads = atoi(*++argv)) < 1) {
		    return usage( BadNumberOfThreads, "-t");
                }
	    } else {
		return usage(TooFewArguments);
            }
        } else if(!strcmp(*argv,"-c")) {
	    runCnt = RunForever;
        } else if(!strcmp(*argv,"-rc")) {
	    if(--argc) {
		if((runCnt = atoi(*++argv)) < 1) {
		    return usage( BadRunCount, "-rc");
                }
	    } else {
		return usage(TooFewArguments);
            }
        } else if(!strcmp(*argv,"-s")) {
	    if(--argc) {
		if((nsecSleep = atoi(*++argv)) < 1) {
		    return usage( BadSleepTime, "-s");
                }
	    } else {
		return usage(TooFewArguments);
            }
        } else {
	    return usage(UnknownArgument, *argv);
	}
	++argv;
    }
return Ok;
}

int spawnThreads()
{

    if(!(threadCtrl = (ThreadCtrl*)calloc(sizeof(ThreadCtrl), numThreads)))
	return error(OutOfMemory, "1");

    setbuf(stdout, 0);

    for(int i = 0;i < numThreads; ++i) {
	int ret;

        if(!(threadCtrl[i].thdArg = (ThdArg*)calloc(sizeof(ThdArg), 1)))
	    return error(OutOfMemory, "2");

	threadCtrl[i].thdArg->thdNum = i+1; 
	threadCtrl[i].thdArg->runCnt = runCnt; 
	threadCtrl[i].thdArg->sleepTime = nsecSleep;

        threadCtrl[i].thdFun = threadFun;
	threadCtrl[i].thdState = Init;
	
        if(ret = pthread_create(&threadCtrl[i].thd,
                                NULL,
                                threadCtrl[i].thdFun,
                                threadCtrl[i].thdArg)) {
	    error(ThreadCreateFailed);
        }
    }
return Ok;
}

int main(int argc, char **argv) 
{
int ret = 0;
    
    initData();
    if(ret = doArgs(argc, argv))
	return ret;

    if(ret = spawnThreads())
	return ret;

    if(ret = waitForCompletion())
	return ret;

return ret;
}
