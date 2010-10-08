#ifndef QTHREAD_QLOOP
#define QTHREAD_QLOOP

#include <qthread/qthread.h>
#include <qthread/futurelib.h>

Q_STARTCXX			       /* */
/* for convenient arguments to qt_loop_future */
typedef void (*qt_loop_f) (qthread_t * me, const size_t startat,
			   const size_t stopat, void *arg);
typedef void (*qt_loopr_f) (qthread_t * me, const size_t startat,
			    const size_t stopat, void *restrict arg,
			    void *restrict ret);
typedef void (*qt_accum_f) (void *restrict a, void *restrict b);

typedef struct qqloop_handle_s qqloop_handle_t;

void qt_loop(const size_t start, const size_t stop,
	     const qt_loop_f func, void *argptr);
void qt_loop_future(const size_t start, const size_t stop,
		    const qt_loop_f func, void *argptr);
void qt_loop_balance(const size_t start, const size_t stop,
		     const qt_loop_f func, void *argptr);
void qt_loop_balance_future(const size_t start, const size_t stop,
			    const qt_loop_f func, void *argptr);
void qt_loopaccum_balance(const size_t start, const size_t stop,
			  const size_t size, void *restrict out,
			  const qt_loopr_f func, void *restrict argptr,
			  const qt_accum_f acc);
void qt_loopaccum_balance_future(const size_t start, const size_t stop,
				 const size_t size, void *restrict out,
				 const qt_loopr_f func, void *restrict argptr,
				 const qt_accum_f acc);

typedef enum { CHUNK, GUIDED, FACTORED, TIMED } qt_loop_queue_type;
qqloop_handle_t *qt_loop_queue_create(const qt_loop_queue_type type,
	const size_t start, const size_t stop, const size_t incr,
	const qt_loop_f func, void *const argptr);
void qt_loop_queue_run(qqloop_handle_t * loop);
void qt_loop_queue_run_there(qqloop_handle_t * loop,
			     qthread_shepherd_id_t shep);
void qt_loop_queue_addworker(qqloop_handle_t * loop,
			     const qthread_shepherd_id_t shep);

#ifdef QTHREAD_USE_ROSE_EXTENSIONS
typedef void (*qt_loop_step_f) (qthread_t * me, const size_t startat,
			   const size_t stopat, const size_t step, void *arg);
typedef struct qtrose_loop_handle_s qtrose_loop_handle_t;

void qt_loop_queue_run_single(volatile qtrose_loop_handle_t * loop, void *t);
void qt_parallel(const qt_loop_step_f func, const unsigned int threads,
		 void *argptr);
void qt_parallel_for(const qt_loop_step_f func, const size_t startat,
		     const size_t stopat, const size_t step,
		     void *restrict argptr);
#endif

double qt_double_sum(double *array, size_t length, int checkfeb);
double qt_double_prod(double *array, size_t length, int checkfeb);
double qt_double_max(double *array, size_t length, int checkfeb);
double qt_double_min(double *array, size_t length, int checkfeb);

saligned_t qt_int_sum(saligned_t * array, size_t length, int checkfeb);
saligned_t qt_int_prod(saligned_t * array, size_t length, int checkfeb);
saligned_t qt_int_max(saligned_t * array, size_t length, int checkfeb);
saligned_t qt_int_min(saligned_t * array, size_t length, int checkfeb);

aligned_t qt_uint_sum(aligned_t * array, size_t length, int checkfeb);
aligned_t qt_uint_prod(aligned_t * array, size_t length, int checkfeb);
aligned_t qt_uint_max(aligned_t * array, size_t length, int checkfeb);
aligned_t qt_uint_min(aligned_t * array, size_t length, int checkfeb);

/* These are some utility accumulator functions */
static Q_UNUSED void qt_dbl_add_acc(void *restrict a, void *restrict b)
{
    *(double *)a += *(double *)b;
}

static Q_UNUSED void qt_int_add_acc(void *restrict a, void *restrict b)
{
    *(saligned_t *) a += *(saligned_t *) b;
}

static Q_UNUSED void qt_uint_add_acc(void *restrict a, void *restrict b)
{
    *(aligned_t *) a += *(aligned_t *) b;
}

static Q_UNUSED void qt_dbl_prod_acc(void *restrict a, void *restrict b)
{
    *(double *)a *= *(double *)b;
}

static Q_UNUSED void qt_int_prod_acc(void *restrict a, void *restrict b)
{
    *(saligned_t *) a *= *(saligned_t *) b;
}

static Q_UNUSED void qt_uint_prod_acc(void *restrict a, void *restrict b)
{
    *(aligned_t *) a *= *(aligned_t *) b;
}

static Q_UNUSED void qt_dbl_max_acc(void *restrict a, void *restrict b)
{
    if (*(double *)b > *(double *)a)
	*(double *)a = *(double *)b;
}

static Q_UNUSED void qt_int_max_acc(void *restrict a, void *restrict b)
{
    if (*(saligned_t *) b > *(saligned_t *) a)
	*(saligned_t *) a = *(saligned_t *) b;
}

static Q_UNUSED void qt_uint_max_acc(void *restrict a, void *restrict b)
{
    if (*(aligned_t *) b > *(aligned_t *) a)
	*(aligned_t *) a = *(aligned_t *) b;
}

static Q_UNUSED void qt_dbl_min_acc(void *restrict a, void *restrict b)
{
    if (*(double *)b < *(double *)a)
	*(double *)a = *(double *)b;
}

static Q_UNUSED void qt_int_min_acc(void *restrict a, void *restrict b)
{
    if (*(saligned_t *) b < *(saligned_t *) a)
	*(saligned_t *) a = *(saligned_t *) b;
}

static Q_UNUSED void qt_uint_min_acc(void *restrict a, void *restrict b)
{
    if (*(aligned_t *) b < *(aligned_t *) a)
	*(aligned_t *) a = *(aligned_t *) b;
}

void qt_qsort(qthread_t * me, double *array, const size_t length);

Q_ENDCXX			       /* */
#endif
