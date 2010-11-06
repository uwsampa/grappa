#include "defs.h"

double betweennessCentrality(graph* G, DOUBLE_T* BC) {

    VERT_T *S;         /* stack of vertices in the order of non-decreasing 
                          distance from s. Also used to implicitly 
                          represent the BFS queue */
    plist* P;          /* predecessors of a vertex v on shortest paths from s */
    DOUBLE_T* sig;     /* No. of shortest paths */
    LONG_T* d;         /* Length of the shortest path between every pair */
    DOUBLE_T* del;     /* dependency of vertices */
    LONG_T *in_degree, *numEdges, *pSums;
    LONG_T *pListMem;    
    LONG_T* Srcs; 
    LONG_T *start, *end;
    LONG_T MAX_NUM_PHASES;
    LONG_T *psCount;
#ifdef _OPENMP    
    omp_lock_t* vLock;
    LONG_T chunkSize;
#endif
    int seed = 2387;
    double elapsed_time;

#ifdef _OPENMP    
#pragma omp parallel
{
#endif

    VERT_T *myS, *myS_t;
    LONG_T myS_size;
    LONG_T i, j, k, p, count, myCount;
    LONG_T v, w, vert;
    LONG_T numV, num_traversals, n, m, phase_num;
    LONG_T tid, nthreads;
    int* stream;
#ifdef DIAGNOSTIC
    double elapsed_time_part;
#endif

#ifdef _OPENMP
    int myLock;
    tid = omp_get_thread_num();
    nthreads = omp_get_num_threads();
#else
    tid = 0;
    nthreads = 1;
#endif

#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds();
    }
#endif

    /* numV: no. of vertices to run BFS from = 2^K4approx */
    numV = 1<<K4approx;
    n = G->n;
    m = G->m;

    /* Permute vertices */
    if (tid == 0) {
        Srcs = (LONG_T *) malloc(n*sizeof(LONG_T));
#ifdef _OPENMP
        vLock = (omp_lock_t *) malloc(n*sizeof(omp_lock_t));
#endif
    }

#ifdef _OPENMP   
#pragma omp barrier
#pragma omp for
    for (i=0; i<n; i++) {
        omp_init_lock(&vLock[i]);
    }
#endif

    /* Initialize RNG stream */ 
	stream = init_sprng(0, tid, nthreads, seed, SPRNG_DEFAULT);

#ifdef _OPENMP
#pragma omp for
#endif
    for (i=0; i<n; i++) {
        Srcs[i] = i;
    }

#ifdef _OPENMP
#pragma omp for
#endif    
    for (i=0; i<n; i++) {
        j = n*sprng(stream);
        if (i != j) {
#ifdef _OPENMP
            int l1 = omp_test_lock(&vLock[i]);
            if (l1) {
                int l2 = omp_test_lock(&vLock[j]);
                if (l2) {
#endif
                    k = Srcs[i];
                    Srcs[i] = Srcs[j];
                    Srcs[j] = k;
#ifdef _OPENMP
                    omp_unset_lock(&vLock[j]);
                }
                omp_unset_lock(&vLock[i]);
            }
#endif
        }
    }

#ifdef _OPENMP    
#pragma omp barrier
#endif

#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds() -elapsed_time_part;
        fprintf(stderr, "Vertex ID permutation time: %lf seconds\n", elapsed_time_part);
        elapsed_time_part = get_seconds();
    }
#endif

    /* Start timing code from here */
    if (tid == 0) {
        elapsed_time = get_seconds();
#ifdef VERIFYK4
        MAX_NUM_PHASES = 2*sqrt(n);
#else
        MAX_NUM_PHASES = 50;
#endif
    }

#ifdef _OPENMP
#pragma omp barrier    
#endif

    /* Initialize predecessor lists */
    
    /* The size of the predecessor list of each vertex is bounded by 
       its in-degree. So we first compute the in-degree of every
       vertex */ 

    if (tid == 0) {
        P   = (plist  *) calloc(n, sizeof(plist));
        in_degree = (LONG_T *) calloc(n+1, sizeof(LONG_T));
        numEdges = (LONG_T *) malloc((n+1)*sizeof(LONG_T));
        pSums = (LONG_T *) malloc(nthreads*sizeof(LONG_T));
    }

#ifdef _OPENMP
#pragma omp barrier
#pragma omp for
#endif
    for (i=0; i<m; i++) {
        v = G->endV[i];
#ifdef _OPENMP
        omp_set_lock(&vLock[v]);
#endif
        in_degree[v]++;
#ifdef _OPENMP
        omp_unset_lock(&vLock[v]);
#endif
    }

    prefix_sums(in_degree, numEdges, pSums, n);
    
    if (tid == 0) {
        pListMem = (LONG_T *) malloc(m*sizeof(LONG_T));
    }

#ifdef _OPENMP
#pragma omp barrier
#pragma omp for
#endif
    for (i=0; i<n; i++) {
        P[i].list = pListMem + numEdges[i];
        P[i].degree = in_degree[i];
        P[i].count = 0;
    }

#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds() - elapsed_time_part;
        fprintf(stderr, "In-degree computation time: %lf seconds\n", elapsed_time_part);
        elapsed_time_part = get_seconds();
    }
#endif

    /* Allocate shared memory */ 
    if (tid == 0) {
        free(in_degree);
        free(numEdges);
        free(pSums);
        
        S   = (VERT_T *) malloc(n*sizeof(VERT_T));
        sig = (DOUBLE_T *) malloc(n*sizeof(DOUBLE_T));
        d   = (LONG_T *) malloc(n*sizeof(LONG_T));
        del = (DOUBLE_T *) calloc(n, sizeof(DOUBLE_T));
        
        start = (LONG_T *) malloc(MAX_NUM_PHASES*sizeof(LONG_T));
        end = (LONG_T *) malloc(MAX_NUM_PHASES*sizeof(LONG_T));
        psCount = (LONG_T *) malloc((nthreads+1)*sizeof(LONG_T));
    }

    /* local memory for each thread */  
    myS_size = (2*n)/nthreads;
    myS = (LONG_T *) malloc(myS_size*sizeof(LONG_T));
    num_traversals = 0;
    myCount = 0;

#ifdef _OPENMP    
#pragma omp barrier
#endif

#ifdef _OPENMP    
#pragma omp for
#endif
    for (i=0; i<n; i++) {
        d[i] = -1;
    }
 
#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds() -elapsed_time_part;
        fprintf(stderr, "BC initialization time: %lf seconds\n", elapsed_time_part);
        elapsed_time_part = get_seconds();
    }
#endif
   
    for (p=0; p<n; p++) {

        i = Srcs[p];
        if (G->numEdges[i+1] - G->numEdges[i] == 0) {
            continue;
        } else {
            num_traversals++;
        }

        if (num_traversals == numV + 1) {
            break;
        }
        
        if (tid == 0) {
            sig[i] = 1;
            d[i] = 0;
            S[0] = i;
            start[0] = 0;
            end[0] = 1;
        }
        
        count = 1;
        phase_num = 0;

#ifdef _OPENMP       
#pragma omp barrier
#endif
        
        while (end[phase_num] - start[phase_num] > 0) {
            
            myCount = 0;
#ifdef _OPENMP
#pragma omp barrier
#pragma omp for schedule(dynamic)
#endif
            for (vert = start[phase_num]; vert < end[phase_num]; vert++) {
                v = S[vert];
                for (j=G->numEdges[v]; j<G->numEdges[v+1]; j++) {

#ifndef VERIFYK4
                    /* Filter edges with weights divisible by 8 */
                    if ((G->weight[j] & 7) != 0) {
#endif
                        w = G->endV[j];
                        if (v != w) {

#ifdef _OPENMP                            
                            myLock = omp_test_lock(&vLock[w]);
                            if (myLock) { 
#endif             
                                /* w found for the first time? */ 
                                if (d[w] == -1) {
                                    if (myS_size == myCount) {
                                        /* Resize myS */
                                        myS_t = (LONG_T *)
                                            malloc(2*myS_size*sizeof(VERT_T));
                                        memcpy(myS_t, myS, myS_size*sizeof(VERT_T));
                                        free(myS);
                                        myS = myS_t;
                                        myS_size = 2*myS_size;
                                    }
                                    myS[myCount++] = w;
                                    d[w] = d[v] + 1;
                                    sig[w] = sig[v];
                                    P[w].list[P[w].count++] = v;
                                } else if (d[w] == d[v] + 1) {
                                    sig[w] += sig[v];
                                    P[w].list[P[w].count++] = v;
                                }
#ifdef _OPENMP  
                            
                            omp_unset_lock(&vLock[w]);
                            } else {
                                if ((d[w] == -1) || (d[w] == d[v]+ 1)) {
                                    omp_set_lock(&vLock[w]);
                                    sig[w] += sig[v];
                                    P[w].list[P[w].count++] = v;
                                    omp_unset_lock(&vLock[w]);
                                }
                            }
#endif
                            
                        }
#ifndef VERIFYK4
                    }
#endif
                }
             }
            /* Merge all local stacks for next iteration */
            phase_num++; 

            psCount[tid+1] = myCount;

#ifdef _OPENMP
#pragma omp barrier
#endif

            if (tid == 0) {
                start[phase_num] = end[phase_num-1];
                psCount[0] = start[phase_num];
                for(k=1; k<=nthreads; k++) {
                    psCount[k] = psCount[k-1] + psCount[k];
                }
                end[phase_num] = psCount[nthreads];
            }
            
#ifdef _OPENMP           
#pragma omp barrier
#endif

            for (k = psCount[tid]; k < psCount[tid+1]; k++) {
                S[k] = myS[k-psCount[tid]];
            } 
            
#ifdef _OPENMP            
#pragma omp barrier
#endif
            count = end[phase_num];
        }
     
        phase_num--;

#ifdef _OPENMP        
#pragma omp barrier
#endif

        while (phase_num > 0) {
#ifdef _OPENMP        
#pragma omp for
#endif
            for (j=start[phase_num]; j<end[phase_num]; j++) {
                w = S[j];
                for (k = 0; k<P[w].count; k++) {
                    v = P[w].list[k];
#ifdef _OPENMP
                    omp_set_lock(&vLock[v]);
#endif
                    del[v] = del[v] + sig[v]*(1+del[w])/sig[w];
#ifdef _OPENMP
                    omp_unset_lock(&vLock[v]);
#endif
                }
                BC[w] += del[w];
            }

            phase_num--;
            
#ifdef _OPENMP
#pragma omp barrier
#endif            
        }

        
#ifdef _OPENMP
        chunkSize = n/nthreads;
#pragma omp for schedule(static, chunkSize)
#endif
        for (j=0; j<count; j++) {
            w = S[j];
            d[w] = -1;
            del[w] = 0;
            P[w].count = 0;
        }


#ifdef _OPENMP
#pragma omp barrier
#endif

    }
 
#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds() -elapsed_time_part;
        fprintf(stderr, "BC computation time: %lf seconds\n", elapsed_time_part);
    }
#endif

#ifdef _OPENMP
#pragma omp for
    for (i=0; i<n; i++) {
        omp_destroy_lock(&vLock[i]);
    }
#endif

    free(myS);
    
    if (tid == 0) { 
        free(S);
        free(pListMem);
        free(P);
        free(sig);
        free(d);
        free(del);
#ifdef _OPENMP
        free(vLock);
#endif
        free(start);
        free(end);
        free(psCount);
        elapsed_time = get_seconds() - elapsed_time;
        free(Srcs);
    }

    free_sprng(stream);
#ifdef _OPENMP
}    
#endif
    /* Verification */
#ifdef VERIFYK4
    double BCval;
    if (SCALE % 2 == 0) {
        BCval = 0.5*pow(2, 3*SCALE/2)-pow(2, SCALE)+1.0;
    } else {
        BCval = 0.75*pow(2, (3*SCALE-1)/2)-pow(2, SCALE)+1.0;
    }
    int failed = 0;
    for (int i=0; i<G->n; i++) {
        if (round(BC[i] - BCval) != 0) {
            failed = 1;
            break;
        }
    }
    if (failed) {
        fprintf(stderr, "Kernel 4 failed validation!\n");
    } else {
        fprintf(stderr, "Kernel 4 validation successful!\n");
    }
#endif
    return elapsed_time;
}

