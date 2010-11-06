#include "defs.h"
#define PARALLEL_SDG

/* Set this variable to zero to run the data generator 
   on one thread (for debugging purposes) */

double gen2DTorus(graphSDG* SDGdata) {

    VERT_T *src, *dest;
    WEIGHT_T *wt;

#ifdef _OPENMP    
	omp_lock_t* vLock;
#endif
    
    double elapsed_time;
    int seed;

    elapsed_time = get_seconds();

    /* allocate memory for edge tuples */
    src  = (VERT_T *)   malloc(M*sizeof(VERT_T));
    dest = (VERT_T *)   malloc(M*sizeof(VERT_T));
    wt   = (WEIGHT_T *) malloc(M*sizeof(WEIGHT_T));
    
    assert(src  != NULL);
    assert(dest != NULL); 
    assert(wt != NULL);

    /* sprng seed */
    seed = 2387;

#ifdef _OPENMP
#ifdef PARALLEL_SDG
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

    LONG_T n, m;
    LONG_T i, j, x, y;
    LONG_T x_start, x_end, offset;
    LONG_T count;

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

    n = N;
    m = M;

    if (SCALE % 2 == 0) {
        x = 1<<(SCALE/2);
        y = 1<<(SCALE/2);
    } else {
        x = 1<<((SCALE+1)/2);
        y = 1<<((SCALE-1)/2);
    }
    
    count = 0;

    x_start = (x/nthreads)*tid;
    x_end = (x/nthreads)*(tid+1);

    if (tid == 0)
        x_start = 0;

    if (tid == nthreads-1)
        x_end = x;
            
    offset = 4*x_start*y;

    fprintf(stderr, "tid: %d, x_start: %d, x_end: %d, offset: %d\n", 
            tid, x_start, x_end, offset);

    // if (tid == 0) {
    for (i = x_start; i < x_end; i++) {

        for (j = 0; j < y; j++) {
      
            /* go down */
            if (j > 0) {
                src[offset+count] = y*i + j;
                dest[offset+count] = y*i + j - 1;
            } else {
                src[offset+count] = y*i + j;
                dest[offset+count] = y*i + y - 1;
            }

            count++;
  
            /* go up */
            if (j < y-1) {
                src[offset+count] = y*i + j;
                dest[offset+count] = y*i + j + 1;
            } else {
                src[offset+count] = y*i + j;
                dest[offset+count] = y*i;
            }
            
            count++;
            
            /* go left */
            if (i > 0) {
                src[offset+count] = y*i + j;
                dest[offset+count] = y*(i-1) + j;
            } else {
                src[offset+count] = y*i + j;
                dest[offset+count] = y*(x-1) + j;
            } 

            count++;
            
            /* go right */
            if (i < x-1) {
                src[offset+count] = y*i + j;
                dest[offset+count] = y*(i+1) + j;
            } else {
                src[offset+count] = y*i + j;
                dest[offset+count] = j;
            }
            
            count++;

        }
    }

    // }
    
#ifdef _OPENMP
#pragma omp barrier
#endif

#ifdef DIAGNOSTIC
    if (tid == 0) {
        elapsed_time_part = get_seconds() -elapsed_time_part;
        fprintf(stderr, "Tuple generation time: %lf seconds\n", elapsed_time_part);
        elapsed_time_part = get_seconds();
    }
#endif
   
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

#ifdef _OPENMP
}
#endif
    
    elapsed_time = get_seconds() - elapsed_time;
    return elapsed_time;
}

