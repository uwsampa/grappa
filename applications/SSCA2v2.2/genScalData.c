#include "defs.h"

/* Set this variable to zero to run the data generator 
   on one thread (for debugging purposes) */
#define PARALLEL_SDG 0

double genScalData(graphSDG* SDGdata) {

    VERT_T *src, *dest;
    WEIGHT_T *wt;
    LONG_T n, m;
    VERT_T *permV;
#ifdef _OPENMP    
	omp_lock_t* vLock;
#endif
    
    double elapsed_time;
    int seed;
    
    n = N;
    m = M;
   
    /* allocate memory for edge tuples */
    src  = (VERT_T *) malloc(M*sizeof(VERT_T));
    dest = (VERT_T *) malloc(M*sizeof(VERT_T));
    assert(src  != NULL);
    assert(dest != NULL); 

    /* sprng seed */
    seed = 2387;

    elapsed_time = get_seconds();

#ifdef _OPENMP
#if PARALLEL_SDG
    omp_set_num_threads(omp_get_max_threads());
    // omp_set_num_threads(16);
#else
    omp_set_num_threads(1);
#endif
#endif
 

#ifdef _OPENMP
#pragma omp parallel
{
#endif
    int tid, nthreads;
#ifdef DIAGNOSTIC
    double elapsed_time_part;
#endif
    int *stream;

    LONG_T i, j, u, v, step;
    DOUBLE_T av, bv, cv, dv, p, S, var;
    LONG_T tmpVal;

#ifdef _OPENMP
    nthreads = omp_get_num_threads();
    tid = omp_get_thread_num();
#else
    nthreads = 1;
    tid  = 0;    
#endif
    
    /* Initialize RNG stream */ 
	stream = init_sprng(0, tid, nthreads, seed, SPRNG_DEFAULT);

#ifdef DIAGNOSTIC
    if (tid == 0) 
        elapsed_time_part = get_seconds();
#endif

    /* Start adding edges */
#ifdef _OPENMP
#pragma omp for
#endif    
    for (i=0; i<m; i++) {

        u = 1;
        v = 1;
        step = n/2;

        av = A;
        bv = B;
        cv = C;
        dv = D;

        p = sprng(stream);
        if (p < av) {
            /* Do nothing */
        } else if ((p >= av) && (p < av+bv)) {
            v += step;
        } else if ((p >= av+bv) && (p < av+bv+cv)) {
            u += step;
        } else {
            u += step;
            v += step;
        }
        
        for (j=1; j<SCALE; j++) {
            step = step/2;

            /* Vary a,b,c,d by up to 10% */
            var = 0.1;
            av *= 0.95 + var * sprng(stream);
            bv *= 0.95 + var * sprng(stream);
            cv *= 0.95 + var * sprng(stream);
            dv *= 0.95 + var * sprng(stream);

            S = av + bv + cv + dv;
            av = av/S;
            bv = bv/S;
            cv = cv/S;
            dv = dv/S;
            
            /* Choose partition */ 
            p = sprng(stream);
            if (p < av) {
                /* Do nothing */
            } else if ((p >= av) && (p < av+bv)) {
                v += step;
            } else if ((p >= av+bv) && (p < av+bv+cv)) {
                u += step;
            } else {
                u += step;
                v += step;
            }
        }           
        
        src[i] = u-1;
        dest[i] = v-1;
    }

#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds() -elapsed_time_part;
        fprintf(stderr, "Tuple generation time: %lf seconds\n", elapsed_time_part);
        elapsed_time_part = get_seconds();
    }
#endif

    /* Generate vertex ID permutations */

    if (tid == 0) {
        permV = (VERT_T *) malloc(N*sizeof(VERT_T));
        assert(permV != NULL);
    }

#ifdef _OPENMP
#pragma omp barrier

#pragma omp for
#endif    
    for (i=0; i<n; i++) {
        permV[i] = i;
    }

#ifdef _OPENMP
    if (tid == 0) {
        vLock = (omp_lock_t *) malloc(n*sizeof(omp_lock_t));
        assert(vLock != NULL);
    }

#pragma omp barrier

#pragma omp for
    for (i=0; i<n; i++) {
        omp_init_lock(&vLock[i]);
    }

#endif
    
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
                    tmpVal = permV[i];
                    permV[i] = permV[j];
                    permV[j] = tmpVal;
#ifdef _OPENMP
                    omp_unset_lock(&vLock[j]);
                }
                omp_unset_lock(&vLock[i]);
            }
#endif
        }
    }

#ifdef _OPENMP
#pragma omp for
    for (i=0; i<n; i++) {
        omp_destroy_lock(&vLock[i]);
    }
    
#pragma omp barrier

    if (tid == 0) {
        free(vLock);
    }
#endif

#ifdef _OPENMP
#pragma omp for
#endif    
    for (i=0; i<m; i++) {
        src[i] = permV[src[i]];
        dest[i] = permV[dest[i]];
    } 

#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds() - elapsed_time_part;
        fprintf(stderr, "Permuting vertex IDs: %lf seconds\n", elapsed_time_part);
        elapsed_time_part = get_seconds();
    }
#endif

    if (tid == 0) {
        free(permV);
    }
    
    /* Generate edge weights */
    if (tid == 0) {
        wt = (WEIGHT_T *) malloc(M*sizeof(WEIGHT_T));
        assert(wt != NULL);
    }

#ifdef _OPENMP
#pragma omp barrier

#pragma omp for
#endif    
    for (i=0; i<m; i++) {
        wt[i] = 1 + MaxIntWeight * sprng(stream); 
    }
 
#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds() - elapsed_time_part;
        fprintf(stderr, "Generating edge weights: %lf seconds\n", elapsed_time_part);
        elapsed_time_part = get_seconds();
    }
#endif

   
    SDGdata->n = n;
    SDGdata->m = m;
    SDGdata->startVertex = src;
    SDGdata->endVertex = dest;
    SDGdata->weight = wt;

    free_sprng(stream);
#ifdef _OPENMP
}
#endif
    
    elapsed_time = get_seconds() - elapsed_time;
    return elapsed_time;
}

