#include "BFS.h"
#define DEBUG 0

double BFS(graph *G, int s)
{

    int  i, m, n;
    int *numEdges, *numEdgesBACK;
    int *startVertex, *endVertex, *startVertexBACK, *endVertexBACK;
    int *Q, *color;
    int startIndex, endIndex;
    int distance;
    int count;
    int** neighbor;
    double freq, mhz, start_time, end_time, running_time;
    int ticks, spticks;
#ifndef __MTA__
    struct timeval tp;
#endif

#if DEBUG
    int BFSticks;
    int *nVertsInPhase, *nEdgesInPhase, numPhases;
    double *time;
    FILE* outfp;
    int avgV, avgE;
    double avgT;
    nVertsInPhase = (int *) calloc(1000, sizeof(int));
    nEdgesInPhase = (int *) calloc(1000, sizeof(int));
    time = (double *) calloc(1000, sizeof(double));
    numPhases = 0;

#ifdef __MTA__
    freq = (double) mta_clock_freq();
#endif

#endif

    endVertex = G->endV;
    numEdges = G->numEdges;
    n = G->n;
    m = G->m;

#ifndef __MTA__
    gettimeofday(&tp, NULL);
    start_time = tp.tv_sec + (tp.tv_usec*1E-6);
#else
    freq = (double) mta_clock_freq();
	mhz = freq/1000000;
	start_time = 0;
	ticks = mta_get_clock(0);
#endif

    Q  = (int *) malloc(n*sizeof(int));
    color = (int *) malloc(n*sizeof(int));

    for (i=0; i<n; i++) color[i] = WHITE;
    color[s] = BLACK;

    Q[0] = s;
    startIndex = 0;
    endIndex = 1;

    while (endIndex - startIndex > 0) {

        count = endIndex;
#if DEBUG
        nVertsInPhase[numPhases] = endIndex - startIndex;
        for (i=startIndex; i<endIndex; i++) {
            int_fetch_add(&nEdgesInPhase[numPhases], numEdges[Q[i]+1] - numEdges[Q[i]]);
        }
#ifdef __MTA__
        ticks = mta_get_clock(0);
#else
        gettimeofday(&tp, NULL);
        start_time = tp.tv_sec + (tp.tv_usec*1E-6);

#endif
#endif

        #pragma mta assert parallel
        // #pragma mta loop future
        for (i=startIndex; i<endIndex; i++) {
            int j = 0;  
            int u = Q[i];

            int startIter = numEdges[u];
            int endIter = numEdges[u+1];

            // #pragma mta assert parallel
            for (j = startIter; j < endIter; j++) {
                int v    = endVertex[j];
                int clr = int_fetch_add(color + v, 1);
                if (clr == 0) {
                    int index = int_fetch_add(&count, 1);
                    Q[index] = v;
                }
            }
        }

#ifndef __MTA__
	gettimeofday(&tp, NULL);
	end_time = tp.tv_sec + (tp.tv_usec*1E-6);
    running_time = end_time - start_time;
#else
	spticks = mta_get_clock(ticks);
	end_time = ((double) spticks)/freq;
    running_time = end_time - start_time;
#endif


#if DEBUG
#ifdef __MTA__
        BFSticks = mta_get_clock(ticks);
        time[numPhases] = ((double) BFSticks)/freq;
#else
        gettimeofday(&tp, NULL);
        end_time = tp.tv_sec + (tp.tv_usec*1E-6);
        time[numPhases] = end_time - start_time;
#endif
        numPhases++;    
#endif

        startIndex = endIndex;
        endIndex = count;
    }   

#if DEBUG
    avgV = 0;
    avgE = 0;
    avgT = 0;

    for (i=0; i<numPhases; i++) {
        avgV += nVertsInPhase[i];
        avgE += nEdgesInPhase[i];
        avgT += time[i];    
    }
    
    outfp = fopen("stats.txt", "a");
    fprintf(outfp, "n:%d, m:%d\n", n, m);
    fprintf(outfp, "numPhases: %d, avg. verts per phase %d (%d), avg. edges per phase %d (%d)\n", 
            numPhases, n/numPhases, avgV/numPhases, m/numPhases, avgE/numPhases);
    fprintf(outfp, "Time taken: %lf, avg time %lf\n", avgT, avgT/numPhases);

    fprintf(outfp, "Phase Work and running times:\n");

    for (i=0; i<numPhases; i++) {
        fprintf(outfp, "%d  -- n:%d, m:%d, t:%lf\n", i, nVertsInPhase[i], nEdgesInPhase[i], time[i]);
    }
    fprintf(outfp, "\n");
    fclose(outfp);
    
    free(nVertsInPhase);
    free(nEdgesInPhase);
    free(time);
#endif

    free(Q);
    free(color);
    return running_time;
}
