#include "utils.h"

void storeTimes(double* times, double* all_times, int j, int nTests) {

    double max, min, avg;
    int i;
    FILE* outfp;

    if (nTests == 1) {
        max = min = avg = times[0];
        outfp = fopen("log", "a");

        fprintf(outfp, "nTests %d, avg %3.4lf sec\n", nTests, avg);
        all_times[j] = avg;
        fclose(outfp);    
        return;
    }

    if (nTests == 2) {
        if (times[0] > times[1]) {
            max = times[0];
            min = times[1];
        } else {
            max = times[1];
            min = times[0];
        }

        avg = (max + min)/2.0;

        outfp = fopen("log", "a");

        fprintf(outfp, "nTests %d, max %3.4lf sec, min %3.4lf sec, avg %3.4lf sec\n", nTests, max, min, avg);
        all_times[j] = avg;
        fclose(outfp);
        return;
    }

    max = times[0];
    min = times[0];
    avg = times[0];

    for (i=1; i<nTests; i++) {

        if (times[i] > max)
            max = times[i];

        if (times[i] < min)
            min = times[i];

        avg = avg + times[i];

    }

    /* calculate the avg, exclusing the max and min values */
    avg = (avg - max - min)/(nTests-2);
    outfp = fopen("log", "a");

    fprintf(outfp, "nTests %d, max %3.4lf sec, min %3.4lf sec, avg %3.4lf sec\n", nTests, max, min, avg);
    all_times[j] = avg;
    fclose(outfp);

}

void printTimingStats(double* times, int N) {

    double max, min, avg;
    int i;

    max = times[0];
    min = times[0];
    avg = times[0];

    for (i=1; i<N; i++) {

        if (times[i] > max)
            max = times[i];

        if (times[i] < min)
            min = times[i];

        avg = avg + times[i];

    }

    /* calculate the avg, excluding the max and min values */

    if (N == 2) {
        avg = (max + min)/2.0;
    } else if (N > 2) { 
        avg = (avg - max - min)/(N-2);
    } 

    fprintf(stdout, "nSrcs %d, max %3.4lf, min %3.4lf, avg %3.4lf\n", N, max, min, avg);

}

int find(char** v, int len, char * key)
{
    int i=0;
    while (i < len) {
        if (strcmp(v[i++],key)==0)
            return 1;
    }
    return 0;
}


void randPerm(int n, int m, int *srcs, int *dests) {

	RandomValue* randVals;

	int i, j, tmp, x, y;
	int* perm;

	randVals = (RandomValue *) malloc(n*sizeof(RandomValue));
	
	giant_(&n, randVals);

	perm = (int *) malloc(n*sizeof(int));

	for (i=0; i<n; i++) {
		perm[i] = i;
	}

#ifdef __MTA__
 	#pragma mta assert parallel
#endif
	for (i=0; i<n; i++) {

#ifdef __MTA__
		j = randVals[i] % n;
#else
		j = randVals[i] % ((unsigned int) n);
#endif
		if (i==j)
			continue;
		
		/* Swap perm[i] and perm[j] */
#ifdef __MTA__
		x = readfe(perm + i);
		y = readfe(perm + j);
		writeef(perm + i, y);
		writeef(perm + j, x);
#else
		tmp = perm[i];
		perm[i] = perm[j];
		perm[j] = tmp;

#endif
	}

#ifdef __MTA__
	#pragma mta assert nodep
#endif
	for (i=0; i<m; i++) {
		srcs[i] = perm[srcs[i]];
		dests[i] = perm[dests[i]];
	} 

	free(perm);
	free(randVals);
}

#ifndef __MTA__
int int_fetch_add(int* val, int incr) {
        int result = *val;
        *val = *val + incr;
        return result;
}

int readfe(int *addr) {
        return *addr;
}

int writeef(int *addr, int val) {
        *addr = val;
        return *addr;
}
#endif

