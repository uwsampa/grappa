#include "BellmanFord.h"

double BellmanFord(graph *G, int s, double* cs) {

    int  i, m, n;
    int  *numEdges, *startV, *endV;
    double *W, *d, INFTY;
    int ticks, spticks;
    double running_time, start_time, end_time, freq, mhz;
    int sum, not_connected;
    double Wsum, checksum;
#ifndef __MTA__
    struct timeval tp;
#endif
    running_time = 0;  
#ifdef __MTA__
    freq = (double) mta_clock_freq();
    mhz = freq/1000000;
#endif

    m = G->m;
    n = G->n;
    numEdges = G->numEdges;
    endV  = G->endV;
    W = G->realWt;

    startV = (int *) malloc(m*sizeof(int));
    #pragma mta assert nodep 
    for (i=0; i<n; i++) {
        int j;
        #pragma mta assert nodep 
        for (j=numEdges[i]; j<numEdges[i+1]; j++) {
            startV[j] = i;
        }
    }

#ifndef __MTA__
    gettimeofday(&tp, NULL);
    start_time = tp.tv_sec + (tp.tv_usec*1E-6);
#else
    freq = (double) mta_clock_freq();
    mhz = freq/1000000;
    start_time = 0;
    ticks = mta_get_clock(0);
#endif

    INFTY = (double) n+1;
    d = (double *) malloc(n*sizeof(double));
    for (i=0; i<n; i++) {
        d[i] = INFTY;
    } 
    s = 1;    
    d[s] = 0;

    for (i=0; i<n; i++) {
        int j; 
        #pragma mta assert nodep
        #pragma mta interleave schedule
        for (j=0; j<m; j++) {
            int u = startV[j];
            int v = endV[j];
            double du = d[u];
            double w = W[j];
#ifdef __MTA__
            double dv = readfe(&d[v]);
#else
            double dv = d[v];
#endif
            if (du + w < dv) {
#ifdef __MTA__
                writeef(&d[v], du + w);
#else
                d[v] = d[u] + w;
#endif
                
            } else {
#ifdef __MTA__
                writeef(&d[v], dv);
#else
                d[v] = dv;
#endif
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
    running_time = end_time;
#endif
	
    /* Compute d checksum */
    checksum = 0;
    not_connected = 0; 
    #pragma mta assert parallel
    for (i=0; i<n; i++) {
        if (d[i] < INFTY)
            checksum = checksum + d[i];
        else
            int_fetch_add(&not_connected, 1);
    }
    *cs = checksum;

    free(d);
    free(startV);
    return running_time;    
}
