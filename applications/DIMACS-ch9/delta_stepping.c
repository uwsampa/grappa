#include "delta_stepping.h"
#define DEBUG 0
double delta;
double INFTY;

double delta_stepping(graph *G, int s, double* cs) {

    int  i, j, m, n;
    int  *numEdges, *endV;
    double *W, *d, *dR;
    int **B, numBuckets, *Bcount, *Bsize, *Braise, *Bdel, *Bt, *Btemp,
        Btemp_count, Bdel_t, Bcount_t;
    int currBucketNum, lastBucketNum, *vBucketMap, *vBucketLoc;
    int *S, *SLoc, Scount, *R, *RLoc, Rcount;
    int *memBlock;
    double delta_l, INFTY_l;

#if SORTW
    int *endVSorted, *delta_thresh;
    double *WSorted;
#endif

    double running_time, start_time, end_time, freq, mhz;
    int sum, not_connected;

#if DEBUG
    int numPhases, *numVertsInPhase, numRelaxations, numRelaxationsHeavy, maxBucketNum;
#endif

    double Wsum, checksum;
#ifdef __MTA__
    int ticks, spticks, sort_ticks;
#else
    struct timeval tp;
#endif

#ifdef __MTA__
    freq = (double) mta_clock_freq();
    mhz = freq/1000000;
#endif
    s = 1;
    m = G->m;
    n = G->n;

    numEdges = G->numEdges;
    endV  = G->endV;
    W = G->realWt;

    INFTY = ((double) m) + 1.0;
    delta = ((double) n)/((double) m);
    // delta = 0.3;
    delta_l = delta;
    INFTY_l = INFTY;
    numBuckets = (int) (1.0/delta) * (n/2);
    // numBuckets = (int) ((1.0/delta)) * 3 * ((int) ceil(log10(n)/log10(2.0)));
    
#if DEBUG
    fprintf(stderr, "source: %d\n", s);
    fprintf(stderr, "delta: %lf\n", delta);
    fprintf(stderr, "No. of buckets: %d\n", numBuckets);

    /* Assuming a max of n phases */
    /* Storing the no. of verts visited in each phase 
       for statistics */
    numVertsInPhase = (int *) calloc(n, sizeof(int));
#endif

#ifndef __MTA__
    gettimeofday(&tp, NULL);
    start_time = tp.tv_sec + (tp.tv_usec*1E-6);
#else
    mhz = freq/1000000;
    freq = (double) mta_clock_freq();
    start_time = 0;
    ticks = mta_get_clock(0);
#endif

#if SORTW
#ifdef __MTA__
    sort_ticks = mta_get_clock(0);
#endif

    endVSorted = (int *) malloc(m*sizeof(int));
    WSorted = (double *) malloc(m*sizeof(double));
    delta_thresh = (int *) malloc((n+1)*sizeof(int));

    #pragma mta trace "sort start"

    #pragma mta assert no dependence
    #pragma mta block dynamic schedule
    #pragma mta use 128 streams
    for (i=0; i<n; i++) {
        int start = numEdges[i];
        int end   = numEdges[i+1];
        int posS  = start - 1;
        int posE  = end;
        #pragma mta assert no dependence
        for (j = start; j < end; j++) {
            double weight = W[j];
            int k = (weight >= delta_l) ? --posE : ++posS;
            endVSorted[k] = endV[j];
            WSorted[k]    = weight;
        }
        delta_thresh[i] = posE-start;
    }
    delta_thresh[n] = 0;
    free(endV);
    free(W);
    #pragma mta trace "sort end"

#ifdef __MTA__
    sort_ticks = mta_get_clock(ticks);
    freq = (double) mta_clock_freq();
    end_time = ((double) sort_ticks)/freq;
    fprintf(stderr, "Sorting time: %lf\n", end_time);
#endif
    G->endV = endVSorted;
    G->realWt = WSorted;
    endV = endVSorted;
    W = WSorted;
#endif

    /* Memory allocation */

    #pragma mta trace "malloc start"
    B = (int **) malloc(numBuckets*sizeof(int *));
    Bsize = (int *) calloc(numBuckets, sizeof(int));
    Bcount = (int *) calloc(numBuckets, sizeof(int));
    Braise = (int *) calloc(4*numBuckets, sizeof(int));
    Bdel = (int *) calloc(numBuckets, sizeof(int));

    #pragma mta trace "memblock malloc start"    
#ifdef __MTA__
    memBlock = (int *) malloc((9*n+1)*sizeof(int));
#else
    if (sizeof(int) == 4) 
        memBlock = (int *) malloc((11*n+2)*sizeof(int));
    if (sizeof(int) == 8)
        memBlock = (int *) malloc((9*n+1)*sizeof(int));
#endif

    #pragma mta trace "memblock malloc end"

    S = memBlock;
    R = memBlock + n;
    SLoc = memBlock + 2*n;
    RLoc = memBlock + 3*n;
    Btemp = memBlock + 4*n;
    vBucketMap = memBlock + 5*n;
    vBucketLoc = memBlock + 6*n;
    dR = (double *) (memBlock + 7*n);

#ifdef __MTA__
    d = (double *) (memBlock + 8*n);
#else
    if (sizeof(int) == 4)
        d = (double *) (memBlock + 9*n);
    if (sizeof(int) == 8)
        d = (double *) (memBlock + 8*n);
#endif

    /* Initializing the bucket data structure */
    for (i=0; i<n; i++) {
        vBucketMap[i] = -1;
        d[i] = INFTY_l;   
        RLoc[i] = -1;
        SLoc[i] = -1;
    }

    d[n] = INFTY_l;

    #pragma mta trace "malloc end"

    R[0] = s;
    dR[0] = 0;
    Rcount = 0;
    Scount = 0; 

    #pragma mta trace "relax s start"
    lastBucketNum = relax(R, dR, RLoc, 1, G, d, B, 0, numBuckets, Bsize, Bcount, Braise, Bdel,
            vBucketMap, vBucketLoc);
    #pragma mta trace "relax s end"

#if DEBUG
    numRelaxations = 1;
    numRelaxationsHeavy = 0;    
    numPhases = 0;
    maxBucketNum = 0;
#endif

    currBucketNum = 0;
    while (currBucketNum <= lastBucketNum) {

        if (Bcount[currBucketNum] == 0) {
            currBucketNum++;
            continue;
        }

        /* Handle light edges */
        while (Bcount[currBucketNum] != 0) {

            Bcount_t = Bcount[currBucketNum];
            Bdel_t = Bdel[currBucketNum];
            Bt = B[currBucketNum];
            Btemp_count = 0;
            Rcount = 0;

            if (Bdel_t == Bcount_t) {
                Btemp_count = 0;

                /* The bucket doesn't have a lot of empty spots */
            } else if (Bdel_t < Bcount_t/3 + 2) {
    
                Btemp_count = Bcount_t;

                if (Bcount_t > 30) {
                    #pragma mta trace "light nodup start"
                    #pragma mta assert parallel
                    for (i=0; i<Bcount_t; i++) {
                        double du;
                        int u, v, dR_rlv, rlv, start, pos, end;
                        u = Bt[i];
                        du = d[u];
                    #if SORTW
                        start = numEdges[u];
                        end = numEdges[u]+delta_thresh[u];
                    #else
                        start = numEdges[u];
                        end = numEdges[u+1];
                    #endif
                        #pragma mta assert nodep
                        for (j=start; j<end; j++) {
                            v = endV[j];
                            if (du + W[j] < d[v]) {
                            #if SORTW == 0
                                if (delta_l > W[j]) {
                            #endif
                                    rlv = readfe(&RLoc[v]);
                                    if (rlv == -1) {
                                        pos = int_fetch_add(&Rcount, 1);
                                        R[pos] = v;
                                        dR[pos] = du + W[j];
                                        writeef(&RLoc[v], pos);
                                    } else {
                                        if (du + W[j] < dR[rlv])
                                            dR[rlv] = du + W[j];
                                        writeef(&RLoc[v], rlv);
                                    }
                            #if SORTW == 0
                                }
                            #endif
                            }
                        }
                    }
                    #pragma mta trace "light nodup end"
                
                } else {
                    for (i=0; i<Bcount_t; i++) {
                        double du;
                        int u, v, dR_rlv, rlv, start, pos, end;
                        u = Bt[i];
                        du = d[u];
                    #if SORTW
                        start = numEdges[u];
                        end = numEdges[u]+delta_thresh[u];
                    #else
                        start = numEdges[u];
                        end = numEdges[u+1];
                    #endif
                        #pragma mta assert nodep
                        for (j=start; j<end; j++) {
                            v = endV[j];
                            if (du + W[j] < d[v]) {
                            #if SORTW == 0
                                if (delta_l > W[j]) {
                            #endif
                                    rlv = readfe(&RLoc[v]);
                                    if (rlv == -1) {
                                        pos = int_fetch_add(&Rcount, 1);
                                        R[pos] = v;
                                        dR[pos] = du + W[j];
                                        writeef(&RLoc[v], pos);
                                    } else {
                                        if (du + W[j] < dR[rlv])
                                            dR[rlv] = du + W[j];
                                        writeef(&RLoc[v], rlv);
                                    }
                            #if SORTW == 0
                                }
                            #endif
                            }
                        }
                    }
                }
                
                /* Add to S */
                if (Bcount_t > 30) {
                    #pragma mta trace "light nodup S start"
                    #pragma mta assert parallel
                    for (i=0; i<Bcount_t; i++) {

                        int slv;
                        int Gn = n;
                        int u = Bt[i];

                        if (u == Gn) 
                            continue;

                        slv = readfe(&SLoc[u]);

                        /* Add the vertex to S */
                        if (slv == -1) {
                            int pos = int_fetch_add(&Scount, 1);
                            S[pos] = u;
                            writeef(&SLoc[u], pos);
                        } else {
                            writeef(&SLoc[u], slv);
                        }

                        vBucketMap[u] = -1;
                        vBucketLoc[u] = -1;
                    }
                    
                    #pragma mta trace "light nodup S end"
                } else {

                    for (i=0; i<Bcount_t; i++) {
                        int slv;
                        int Gn = n;
                        int u = Bt[i];

                        if (u == Gn) 
                            continue;

                        slv = readfe(&SLoc[u]);

                        /* Add the vertex to S */
                        if (slv == -1) {
                            int pos = int_fetch_add(&Scount, 1);
                            S[pos] = u;
                            writeef(&SLoc[u], pos);
                        } else {
                            writeef(&SLoc[u], slv);
                        }

                        vBucketMap[u] = -1;
                        vBucketLoc[u] = -1;
                    }
                }

            } else {        
            
                /* Bdel_t > Bcount_t/3  */
                /* There are a significant number of empty spots in the bucket.
                 * So we get the non-empty vertices and store them in a compact
                 * array */

                int Gn = n;

                if (Bcount_t > 30) {

                    #pragma mta trace "light dup filter start"
                    #pragma mta assert nodep
                    #pragma mta interleave schedule
                    // #pragma mta use 60 streams
                    for (i=0; i<Bcount_t; i++) {
                        int u = Bt[i];
                        if (u != Gn) {
                            int pos = int_fetch_add(&Btemp_count, 1);
                            Btemp[pos] = u;
                        }
                    }
                    
                    #pragma mta trace "light dup filter end"
                } else {
        
                    for (i=0; i<Bcount_t; i++) {
                        int u = Bt[i];
                        if (u != Gn) {
                            int pos = int_fetch_add(&Btemp_count, 1);
                            Btemp[pos] = u;
                        }
                    }
                } 

                /* The nested loop can be collapsed, but this results
                 * in a lot of hotspots */

                if (Btemp_count > 30) {

                    #pragma mta trace "light dup start"
                    #pragma mta assert parallel
                    for (i=0; i<Btemp_count; i++) {
                        int u = Btemp[i];
                        int pos;
                        double du = d[u];
                        int start, end;
                    #if SORTW
                        start = numEdges[u];
                        end = numEdges[u]+delta_thresh[u];
                    #else
                        start = numEdges[u];
                        end = numEdges[u+1];
                    #endif
                        for (j=start; j<end; j++) {
                            int v = endV[j];
                            if (du + W[j] < d[v]) {
                            #if SORTW == 0
                                if (delta_l > W[j]) {
                            #endif
                                    int rlv = readfe(&RLoc[v]);
                                    if (rlv == -1) {
                                        int pos = int_fetch_add(&Rcount, 1);
                                        R[pos] = v;
                                        dR[pos] = du + W[j];
                                        writeef(&RLoc[v], pos);
                                    } else {
                                        if (du + W[j] < dR[rlv])
                                            dR[rlv] = du + W[j];
                                        writeef(&RLoc[v], rlv);
                                    }
                            #if SORTW == 0
                                }
                            #endif
                            }
                        }   
                    }
                    #pragma mta trace "light dup end"
    
                } else {
        
                    for (i=0; i<Btemp_count; i++) {

                        int u = Btemp[i];
                        int pos;
                        double du = d[u];
                        int start, end;
                    
                    #if SORTW
                        start = numEdges[u];
                        end = numEdges[u]+delta_thresh[u];
                    #else
                        start = numEdges[u];
                        end = numEdges[u+1];
                    #endif
            
                        for (j=start; j<end; j++) {
                            int v = endV[j];
                            if (du + W[j] < d[v]) {
                            #if SORTW == 0
                                if (delta_l > W[j]) {
                            #endif
                                    int rlv = readfe(&RLoc[v]);
                                    if (rlv == -1) {
                                        int pos = int_fetch_add(&Rcount, 1);
                                        R[pos] = v;
                                        dR[pos] = du + W[j];
                                        writeef(&RLoc[v], pos);
                                    } else {
                                        if (du + W[j] < dR[rlv])
                                            dR[rlv] = du + W[j];
                                        writeef(&RLoc[v], rlv);
                                    }
                            #if SORTW == 0
                                }
                            #endif
                            }
                        }   
                    }
                }

                if (Btemp_count > 30) {     
                    #pragma mta trace "light dup S start"
                    #pragma mta assert parallel
                    for (i=0; i<Btemp_count; i++) {

                        int slv;
                        int u = Btemp[i];
                        slv = readfe(&SLoc[u]);

                        /* Add the vertex to S */   
                
                        if (slv == -1) {
                            int pos = int_fetch_add(&Scount, 1);
                            S[pos] = u;
                            writeef(&SLoc[u], pos); 
                        } else {
                            writeef(&SLoc[u], slv);
                        }
                
                        vBucketMap[u] = -1;
                        vBucketLoc[u] = -1;
                    }
                    #pragma mta trace "light dup S end"

                } else {
                
                    for (i=0; i<Btemp_count; i++) {

                        int slv;
                        int u = Btemp[i];
                        slv = readfe(&SLoc[u]);

                        /* Add the vertex to S */   
                
                        if (slv == -1) {
                            int pos = int_fetch_add(&Scount, 1);
                            S[pos] = u;
                            writeef(&SLoc[u], pos); 
                        } else {
                            writeef(&SLoc[u], slv);
                        }

                        vBucketMap[u] = -1;
                        vBucketLoc[u] = -1;
                    }
                }
            } /* end of if .. then .. else */
            /* We have collected all the light edges in R */

        #if DEBUG
            if (Btemp_count != 0)
                maxBucketNum = currBucketNum;
        #endif
    
            Bcount[currBucketNum] = 0;
            Bdel[currBucketNum] = 0;

        #if DEBUG
            numVertsInPhase[numPhases++] = Rcount;
            numRelaxations += Rcount;
        #endif
            /* Relax all light edges */
            if (Rcount != 0) 
                lastBucketNum = relax(R, dR, RLoc, Rcount, G, d, B, currBucketNum, numBuckets, Bsize, Bcount, Braise, Bdel, vBucketMap, vBucketLoc);
        }       
        
        Rcount = 0;
        /* Collect heavy edges into R */
        if (Scount > 10) {
            #pragma mta trace "heavy collect start"
            #pragma mta assert parallel
            for (i=0; i<Scount; i++) {
                int u = S[i];
                double du = d[u];   
            #if SORTW
                int start = numEdges[u] + delta_thresh[u];
                int end = numEdges[u+1];
            #else   
                int start = numEdges[u];
                int end = numEdges[u+1];
            #endif
                SLoc[u] = -1;
                 
                for (j=start; j<end; j++) {
                
                    int v = endV[j];
                    if (W[j] + du < d[v]) {
                    #if SORTW == 0
                        if (delta_l <= W[j]) {
                    #endif
                            int rlv = readfe(&RLoc[v]);
                            if (rlv == -1) {
                                int pos = int_fetch_add(&Rcount, 1);
                                R[pos] = endV[j];
                                dR[pos] = d[u] + W[j];
                                writeef(&RLoc[v], pos);
                            } else {
                                if (du + W[j] < dR[rlv])
                                    dR[rlv] = du + W[j];
                                writeef(&RLoc[v], rlv);
                            }
                    #if SORTW == 0
                        }
                    #endif
                    }
                }
            }
            #pragma mta trace "heavy collect end"
        } else {
            for (i=0; i<Scount; i++) {
                int u = S[i];
                double du = d[u];   
            #if SORTW
                int start = numEdges[u] + delta_thresh[u];
                int end = numEdges[u+1];
            #else   
                int start = numEdges[u];
                int end = numEdges[u+1];
            #endif
                SLoc[u] = -1;
                for (j=start; j<end; j++) {
                
                    int v = endV[j];
                    if (W[j] + du < d[v]) {
                    #if SORTW == 0
                        if (delta_l <= W[j]) {
                    #endif
                            int rlv = readfe(&RLoc[v]);
                            if (rlv == -1) {
                                int pos = int_fetch_add(&Rcount, 1);
                                R[pos] = endV[j];
                                dR[pos] = d[u] + W[j];
                                writeef(&RLoc[v], pos);
                            } else {
                                if (du + W[j] < dR[rlv])
                                    dR[rlv] = du + W[j];
                                writeef(&RLoc[v], rlv);
                            }
                    #if SORTW == 0
                        }
                    #endif
                    }
                }
            }
        }
        
        Scount = 0; 
        /* Relax heavy edges */
        #pragma mta trace "heavy relax start"
        if (Rcount != 0) {
            lastBucketNum = relax(R, dR, RLoc, Rcount, G, d, B, currBucketNum, numBuckets, Bsize, Bcount, Braise, Bdel, vBucketMap, vBucketLoc);
        } 
        #pragma mta trace "heavy relax end"

#if DEBUG
        numRelaxationsHeavy += Rcount;
#endif
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
        if (d[i] < INFTY_l)
            checksum = checksum + d[i];
        else
            int_fetch_add(&not_connected, 1);
    }
    *cs = checksum;

#if DEBUG
    /* Compute W checksum */
    Wsum = 0;
    for (i=0; i<m; i++) {
        Wsum = Wsum + W[i];
    }
    
    *cs = checksum;
    fprintf(stderr, "d checksum: %lf, W checksum: %lf, Avg. distance %lf\n",
            checksum, Wsum, checksum/(n-not_connected));

    fprintf(stderr, "Last non-empty bucket: %d\n", maxBucketNum);
    fprintf(stderr, "No. of phases: %d\n", numPhases);
    
    fprintf(stderr, "No. of light relaxations: %d\n", numRelaxations);
    fprintf(stderr, "No. of heavy relaxations: %d\n", numRelaxationsHeavy); 
    fprintf(stderr, "Avg. no. of light edges relaxed in a phase: %d\n", numRelaxations/numPhases);
    fprintf(stderr, "Avg. no. of heavy edges relaxed per bucket: %d\n", numRelaxationsHeavy/maxBucketNum);
    fprintf(stderr, "Total no. of relaxations: %d\n\n", numRelaxations+numRelaxationsHeavy);
#endif 

#if FREEMEM 
    /* Free memory */    
    #pragma mta assert parallel
    for (i=0; i<numBuckets; i++) {
        if (Bsize[i] != 0)
           free(B[i]);
    }

#if SORTW
    free(delta_thresh);
#endif
    free(B);
    free(Bsize);
    free(Bcount);
    free(Braise); 
    free(Bdel);
    free(memBlock);
#if DEBUG
    free(numVertsInPhase);
#endif
#endif
    return running_time;    
}


#pragma mta inline
int relax(int* R, double* dR, int* RLoc, int Rcount, graph* G, double* d, int**
        B, int currBucketNum, int numBuckets, int* Bsize, int* Bcount, int*
        Braise, int* Bdel, int* vBucketMap, int* vBucketLoc) {

    int i, offset;
    double delta_l = delta;
    double INFTY_l = INFTY;
    int Gn = G->n;
    int lastBucketNum = -1;

    if (Rcount > 30) {
        #pragma mta trace "loop 1 start"
        #pragma mta assert nodep
        #pragma mta interleave schedule
        for (i=0; i<Rcount; i++) {
            int v = R[i];
            int bn = (int) floor(dR[i]/delta_l);
            int bn_old = vBucketMap[v];
            int offset = numBuckets * (i & 3);
            
            if (bn > lastBucketNum)
                lastBucketNum = bn;
            /*
            if (bn >= numBuckets) {
                fprintf(stderr, "Error: relaxation failed, bn: %d, numBuckets: %d\n", bn, numBuckets); 
                exit(1); 
            }
            */
            RLoc[v] = (i & 3) * Gn + int_fetch_add(&Braise[bn+offset], 1);

            if ((d[v] < INFTY_l) && (bn_old != -1)) {
                    B[bn_old][vBucketLoc[v]] = Gn;
                    Bdel[bn_old]++;
            }
        }
        #pragma mta trace "loop 1 end"
    } else {
         for (i=0; i<Rcount; i++) {
            int v = R[i];
            int bn = (int) floor(dR[i]/delta_l);
            int bn_old = vBucketMap[v];
            int offset = numBuckets * (i & 3);
             
            if (bn > lastBucketNum)
                lastBucketNum = bn;
            /*
            if (bn >= numBuckets) {
                fprintf(stderr, "Error: relaxation failed, bn: %d, numBuckets: %d\n", bn, numBuckets); 
                exit(1); 
            }
            */
            RLoc[v] = (i & 3) * Gn + int_fetch_add(&Braise[bn+offset], 1);

            if ((d[v] < INFTY_l) && (bn_old != -1)) {
                    B[bn_old][vBucketLoc[v]] = Gn;
                    Bdel[bn_old]++;
            }
        }
    }
   
    lastBucketNum++;
    // fprintf(stderr, "[%d %d] ", currBucketNum, lastBucketNum);
    #pragma mta trace "loop 2 start"
    for (i=currBucketNum; i<lastBucketNum; i++) {
        int *Bi = B[i];
        int Bsize_i = Bsize[i];
        int size_incr = Braise[i] + Braise[i+numBuckets] + 
            Braise[i+2*numBuckets] + Braise[i+3*numBuckets];

        if ((size_incr > 0) && (Bcount[i] + size_incr >= Bsize[i])) {
        
            int Bsize_tmp = Bcount[i] + size_incr + INIT_SIZE;
            int* Bt = (int *) malloc(Bsize_tmp*sizeof(int));

            if (Bsize_i != 0) {
                int j;
                if (Bsize_i > 30) {
                    #pragma mta assert nodep
                    #pragma mta interleave schedule
                    // #pragma mta use 60 streams
                    for (j=0; j<Bsize_i; j++)
                        Bt[j] = Bi[j];
                } else {
                    for (j=0; j<Bsize_i; j++)
                        Bt[j] = Bi[j];
                }
                free(Bi);
            }
    
            B[i] = Bt;
            Bsize[i] = Bsize_tmp;
        }
    }
    #pragma mta trace "loop 2 end"

    if (Rcount > 30) { 

        #pragma mta trace "loop 3 start"
        #pragma mta assert nodep
        #pragma mta interleave schedule
        for (i=0; i<Rcount; i++) {
            int v = R[i];
            double x = dR[i];
            int loc = RLoc[v];
            int locDiv = loc/Gn;
            int locMod = loc % Gn;
            int bn = (int) floor(x/delta_l);
            int pos = Bcount[bn] + locMod;
            pos += (locDiv >= 1) * Braise[bn];
            pos += (locDiv >= 2) * Braise[bn+numBuckets];
            pos += (locDiv >= 3) * Braise[bn+2*numBuckets]; 
                                    
            B[bn][pos] = v;
            vBucketLoc[v] = pos;
            vBucketMap[v] = bn;
            d[v] = x;
            RLoc[v] = -1;
        }
        #pragma mta trace "loop 3 end" 
    } else {
         for (i=0; i<Rcount; i++) {
            int v = R[i];
            double x = dR[i];
            int loc = RLoc[v];
            int locDiv = loc/Gn;
            int locMod = loc % Gn;
            int bn = (int) floor(x/delta_l);
            int pos = Bcount[bn] + locMod;
            pos += (locDiv >= 1) * Braise[bn];
            pos += (locDiv >= 2) * Braise[bn+numBuckets];
            pos += (locDiv >= 3) * Braise[bn+2*numBuckets]; 
                                    
            B[bn][pos] = v;
            vBucketLoc[v] = pos;
            vBucketMap[v] = bn;
            d[v] = x;
            RLoc[v] = -1;
        }
    }

    if (lastBucketNum - currBucketNum > 30) {

        #pragma mta trace "loop 4 start"
        #pragma mta parallel single processor
        #pragma mta assert nodep
        #pragma mta interleave schedule
        for (i=currBucketNum; i<lastBucketNum; i++) {
            Bcount[i] += Braise[i] + Braise[i+numBuckets] + Braise[i+2*numBuckets] +
                        Braise[i+3*numBuckets];

            Braise[i] = 0;
            Braise[i+numBuckets] = 0;
            Braise[i+2*numBuckets] = 0;
            Braise[i+3*numBuckets] = 0; 
        }
    } else {
    
        for (i=currBucketNum; i<lastBucketNum; i++) {
            Bcount[i] += Braise[i] + Braise[i+numBuckets] + Braise[i+2*numBuckets] +
                        Braise[i+3*numBuckets];

            Braise[i] = 0;
            Braise[i+numBuckets] = 0;
            Braise[i+2*numBuckets] = 0;
            Braise[i+3*numBuckets] = 0; 
        }
    }
    #pragma mta trace "loop 4 end"
    
    return lastBucketNum;

}
