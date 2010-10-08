#ifdef HAVE_CONFIG_H
# include "config.h"		       /* for _GNU_SOURCE */
#endif

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>		       /* for INT_MIN & friends (according to C89) */
#include <float.h>		       /* for DBL_EPSILON (according to C89) */
#include <math.h>		       /* for fabs() */
#include <assert.h>
#include <sys/mman.h>

#include <sys/time.h>		       /* for gettimeofday() */
#include <time.h>		       /* for gettimeofday() */

#include <qthread/qutil.h>
#include <qthread/qtimer.h>
#include "argparsing.h"

static int dcmp(const void *a, const void *b)
{
    return (int)(*(double *)a - *(double *)b);
}

static int acmp(const void *a, const void *b)
{
    return (*(aligned_t *) a - *(aligned_t *) b);
}

int main(int argc, char *argv[])
{
    aligned_t *ui_array, *ui_array2;
    double *d_array, *d_array2;
    size_t len = 1000000;
    qtimer_t timer = qtimer_create();
    double cumulative_time = 0.0;
    int using_doubles = 0;
    unsigned long iterations = 10;

    qthread_initialize();

    CHECK_VERBOSE();
    printf("%i threads\n", (int)qthread_num_shepherds());
    NUMARG(len, "TEST_LEN");
    NUMARG(iterations, "TEST_ITERATIONS");
    NUMARG(using_doubles, "TEST_USING_DOUBLES");
    printf("using %s\n", using_doubles ? "doubles" : "aligned_ts");


    if (using_doubles) {
	d_array = calloc(len, sizeof(double));
	assert(d_array);
	//madvise(d_array,len*sizeof(double), MADV_SEQUENTIAL);
	for (unsigned int i = 0; i < len; i++) {
	    d_array[i] = ((double)random()) / ((double)RAND_MAX) + random();
	}
	d_array2 = calloc(len, sizeof(double));
	assert(d_array2);
	//madvise(d_array2,len*sizeof(double), MADV_RANDOM);
	iprintf("double array generated...\n");
	for (unsigned int i = 0; i < iterations; i++) {
	    memcpy(d_array2, d_array, len * sizeof(double));
	    qtimer_start(timer);
	    qutil_qsort(qthread_self(), d_array2, len);
	    qtimer_stop(timer);
	    cumulative_time += qtimer_secs(timer);
	    iprintf("\t%u: sorting %lu doubles with qutil took: %f seconds\n",
		    i, (unsigned long)len, qtimer_secs(timer));
	}
	printf("sorting %lu doubles with qutil took: %f seconds (avg)\n",
	       (unsigned long)len, cumulative_time / (double)iterations);
	cumulative_time = 0;
	for (unsigned int i = 0; i < iterations; i++) {
	    memcpy(d_array2, d_array, len * sizeof(double));
	    qtimer_start(timer);
	    qsort(d_array2, len, sizeof(double), dcmp);
	    qtimer_stop(timer);
	    cumulative_time += qtimer_secs(timer);
	    iprintf("\t%u: sorting %lu doubles with libc took: %f seconds\n",
		    i, (unsigned long)len, qtimer_secs(timer));
	}
	printf("sorting %lu doubles with libc took: %f seconds\n",
	       (unsigned long)len, cumulative_time / (double)iterations);
	free(d_array);
	free(d_array2);
    } else {
	ui_array = calloc(len, sizeof(aligned_t));
	for (unsigned int i = 0; i < len; i++) {
	    ui_array[i] = random();
	}
	ui_array2 = calloc(len, sizeof(aligned_t));
	iprintf("ui_array generated...\n");
	for (int i = 0; i < 10; i++) {
	    memcpy(ui_array2, ui_array, len * sizeof(aligned_t));
	    qtimer_start(timer);
	    qutil_aligned_qsort(qthread_self(), ui_array2, len);
	    qtimer_stop(timer);
	    cumulative_time += qtimer_secs(timer);
	}
	printf("sorting %lu aligned_ts with qutil took: %f seconds\n",
	       (unsigned long)len, cumulative_time / (double)iterations);
	cumulative_time = 0;
	for (int i = 0; i < 10; i++) {
	    memcpy(ui_array2, ui_array, len * sizeof(aligned_t));
	    qtimer_start(timer);
	    qsort(ui_array2, len, sizeof(double), acmp);
	    qtimer_stop(timer);
	    cumulative_time += qtimer_secs(timer);
	}
	printf("sorting %lu aligned_ts with libc took: %f seconds (avg)\n",
	       (unsigned long)len, cumulative_time / (double)iterations);
	free(ui_array);
	free(ui_array2);
    }

    qtimer_destroy(timer);

    return 0;
}
