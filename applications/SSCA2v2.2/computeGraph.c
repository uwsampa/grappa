#include "defs.h"

double computeGraph(graph* G, graphSDG* SDGdata) {

    VERT_T* endV;
    LONG_T *degree, *numEdges, *pos, *pSums;
    WEIGHT_T* w;
    double elapsed_time;

#ifdef _OPENMP
    omp_lock_t *vLock;
    LONG_T chunkSize;
#endif

    elapsed_time = get_seconds();

#ifdef _OPENMP
    omp_set_num_threads(NUM_THREADS);
#endif

#ifdef _OPENMP
#pragma omp parallel
{
#endif    
    LONG_T i, j, u, n, m, tid, nthreads;
#ifdef DIAGNOSTIC
    double elapsed_time_part;
#endif
    
#ifdef _OPENMP    
    nthreads = omp_get_num_threads();
    tid = omp_get_thread_num();
#else
    tid = 0;
    nthreads = 1;
#endif

    n = N;
    m = M;
    
    if (tid == 0) {
#ifdef _OPENMP
        vLock = (omp_lock_t *) malloc(n*sizeof(omp_lock_t));
        assert(vLock != NULL);
        chunkSize = n/nthreads;
#endif
        pos = (LONG_T *) malloc(m*sizeof(LONG_T));
        assert(pos != NULL);
        degree = (LONG_T *) calloc(n, sizeof(LONG_T));
        assert(degree != NULL);
    }
  
#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds();
    }
#endif
    
#ifdef _OPENMP    
#pragma omp barrier
    
    #pragma omp for schedule(static, chunkSize)
    for (i=0; i<n; i++) {
        omp_init_lock(&vLock[i]);
    }

    #pragma omp barrier
  
#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds() - elapsed_time_part;
        fprintf(stderr, "Lock initialization time: %lf seconds\n",
                elapsed_time_part);
        elapsed_time_part = get_seconds();

    }
#endif
 
    #pragma omp for
#endif
    for (i=0; i<m; i++) {
        u = SDGdata->startVertex[i];
#ifdef _OPENMP        
        omp_set_lock(&vLock[u]);
#endif
        pos[i] = degree[u]++;
#ifdef _OPENMP
        omp_unset_lock(&vLock[u]);
#endif
    } 
   
#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds() - elapsed_time_part;
        fprintf(stderr, "Degree computation time: %lf seconds\n",
                elapsed_time_part);
        elapsed_time_part = get_seconds();

    }
#endif
   
#ifdef _OPENMP
#pragma omp barrier

#pragma omp for schedule(static, chunkSize)
    for (i=0; i<n; i++) {
        omp_destroy_lock(&vLock[i]);
    }

    if (tid == 0) 
        free(vLock);
#endif
    
#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds() - elapsed_time_part;
        fprintf(stderr, "Lock destruction time: %lf seconds\n",
                elapsed_time_part);
        elapsed_time_part = get_seconds();

    }
#endif
   
    if (tid == 0) {
        numEdges = (LONG_T *) malloc((n+1)*sizeof(LONG_T));
        pSums = (LONG_T *) malloc(nthreads*sizeof(LONG_T));
   }

#ifdef _OPENMP
#pragma omp barrier
#endif

    prefix_sums(degree, numEdges, pSums, n); 
    
#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds() - elapsed_time_part;
        fprintf(stderr, "Prefix sums time: %lf seconds\n",
                elapsed_time_part);
        elapsed_time_part = get_seconds();

    }
#endif
 
#ifdef _OPENMP
#pragma omp barrier
#endif

    if (tid == 0) {
        free(degree);
        free(pSums);
        w = (WEIGHT_T *) malloc(m*sizeof(WEIGHT_T));
        endV = (VERT_T *) malloc(m* sizeof(VERT_T));
    }

#ifdef _OPENMP
    #pragma omp barrier

    #pragma omp for
#endif
    for (i=0; i<m; i++) {
        u = SDGdata->startVertex[i];
        j = numEdges[u] + pos[i];
        endV[j] = SDGdata->endVertex[i];
        w[j] = SDGdata->weight[i]; 
    }
    
#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds() - elapsed_time_part;
        fprintf(stderr, "Edge data structure construction time: %lf seconds\n",
                elapsed_time_part);
        elapsed_time_part = get_seconds();

    }
#endif
 
    if (tid == 0) {
        free(pos);
        G->n = n;
        G->m = m;
        G->numEdges = numEdges;
        G->endV = endV;
        G->weight = w;
    }
#ifdef _OPENMP    
}
#endif
    /* Verification */
#if 0 
    fprintf(stderr, "SDG data:\n");
    for (int i=0; i<SDGdata->m; i++) {
        fprintf(stderr, "[%ld %ld %ld] ", SDGdata->startVertex[i], 
                SDGdata->endVertex[i], SDGdata->weight[i]);
    }
 
    fprintf(stderr, "\n");

    for (int i=0; i<G->n + 1; i++) {
        fprintf(stderr, "[%ld] ", G->numEdges[i]);
    }
    
    fprintf(stderr, "\nGraph:\n");
    for (int i=0; i<G->n; i++) {
        for (int j=G->numEdges[i]; j<G->numEdges[i+1]; j++) {
            fprintf(stderr, "[%ld %ld %ld] ", i, G->endV[j], G->weight[j]);
        }
    }
#endif 
    
    free(SDGdata->startVertex);
    free(SDGdata->endVertex);
    free(SDGdata->weight);
    
    elapsed_time = get_seconds() - elapsed_time; 
    
    return elapsed_time;
}
