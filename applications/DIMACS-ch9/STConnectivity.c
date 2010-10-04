#include "STConnectivity.h"
#define DEBUG 0

double STConnectivity(graph *G, int s, int t)
{

    int  i, j, m, n;
    int *numEdges;
    int *startVertex, *endVertex;
    int *Q, *color;
    int startRed, countRed;
    int startGreen, countGreen;
    int startIndex, endIndex, doRed, doGreen;
    int distance, count, count_old;
    int found, no_path;
    int ticks, spticks;
    double freq, mhz, start_time, end_time, running_time;
#ifndef __MTA__
    struct timeval tp;
#endif
#if DEBUG
    int STticks;
    int *nVertsInPhase, *nEdgesInPhase, numPhases;
    double *time;
    FILE* outfp;
    int avgV, avgE;
    double avgT;

    nVertsInPhase = (int *) calloc(1000, sizeof(int));
    nEdgesInPhase = (int *) calloc(1000, sizeof(int));
    time = (double *) calloc(1000, sizeof(double));
    numPhases = 0;
    freq = (double) mta_clock_freq();
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
    color[s] = RED;
    color[t] = GREEN;
    found = (s == t);
    no_path = 0;
    distance = 0;
    /* Initialize Red Queue to hold source vertex s */
    Q[0] = s;
    startRed = 0;
    countRed = 1;

    /* Initialize Green Queue to hold distination vertex t */
    Q[1]        = t;
    startGreen  = 1;
    countGreen  = 1;

    count = 2;

    while (!found && !no_path) {
        distance++;
        doRed = (countRed <= countGreen);
        if (doRed) {
            startIndex = startRed;
            endIndex   = startRed+countRed;
        } else {
            startIndex = startGreen;
            endIndex   = startGreen+countGreen;
        }

        count_old = count;

#if DEBUG
        nVertsInPhase[numPhases] = endIndex - startIndex;
        for (i=startIndex; i<endIndex; i++) {
            int_fetch_add(&nEdgesInPhase[numPhases], numEdges[Q[i]+1] - numEdges[Q[i]]);
        }


        ticks = mta_get_clock(0);
#endif

        if (doRed) {

#pragma mta assert parallel
#pragma mta block dynamic schedule
            for (i=startIndex; i<endIndex; i++) {
                int u = Q[i];

                int startIter = numEdges[u];
                int endIter = numEdges[u+1];

#pragma mta assert parallel
                for (j=startIter ; j<endIter; j++) {
                    int v = endVertex[j];
                    int clr = int_fetch_add(&color[v], 1);
                    if (clr == WHITE) {
                        Q[int_fetch_add(&count,1)] = v;
                        color[v] = RED;
                    } else {
                        if (clr >= GREEN)
                            int_fetch_add(&found, 1);
                    }
                }
            }

        } else {

#pragma mta assert parallel
#pragma mta block dynamic schedule
            for (i=startIndex; i<endIndex; i++) {
                int u = Q[i];
                int startIter = numEdges[u];
                int endIter = numEdges[u+1];

                #pragma mta assert parallel
                for (j=startIter ; j<endIter ; j++) {
                    int v = endVertex[j];
                    int clr = int_fetch_add(&color[v], 1);
                    if (clr == WHITE) {
                        Q[int_fetch_add(&count,1)] = v;
                        color[v] = GREEN;
                    } else {
                        if (clr < GREEN)
                            int_fetch_add(&found, 1);
                        }
                }
            }
        }

#if DEBUG
        STticks = mta_get_clock(ticks);
        time[numPhases] = ((double) STticks)/freq;
        numPhases++;
#endif

        if (doRed) {
            startRed = count_old;
            countRed = count - count_old;
        } else {
            startGreen = count_old;
            countGreen = count - count_old;
        }

        if ((count - count_old) == 0)
            no_path = 1;

    }

    /*
    if (no_path)
        fprintf(stderr, "Disconnected, vertices visited: %d\n", count);
    else
        fprintf(stderr, "Path length: %d, vertices visited: %d\n", distance, count);
    */

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
        fprintf(outfp, "numPhases: %d, avg. verts per phase %d (%d), avg. edges per phase %d (%d)\n", numPhases, n/numPhases, avgV/numPhases, m/numPhases, avgE/numPhases);
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
