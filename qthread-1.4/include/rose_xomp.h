#ifndef ROSE_XOMP_H
#define ROSE_XOMP_H

#include <qthread/qloop.h>	       // for qqloop_handle_t

//Runtime library initialization routine
void XOMP_init(
    int argc,
    char **argv);
// Runtime library termination routine
void XOMP_terminate(
    int exitcode);
void XOMP_parallel_start(
    void (*func) (void *),
    void *data,
    unsigned numThread);
void XOMP_parallel_end(
    void);
qqloop_handle_t *qt_loop_rose_queue_create(
    int64_t start,
    int64_t stop,
    int64_t incr);
void XOMP_loop_guided_init(
    int lower,
    int upper,
    int stride,
    int chunk_size);
unsigned char XOMP_loop_guided_start(
    long startLower,
    long startUpper,
    long stride,
    long chunk_size,
    long *returnLower,
    long *returnUpper);
unsigned char XOMP_loop_guided_next(
    long *returnLower,
    long *returnUpper);
void XOMP_loop_end(
    void);
void XOMP_loop_end_nowait(
    void);
void XOMP_barrier(
    void);
void XOMP_atomic_start(
    void);
void XOMP_atomic_end(
    void);
// needed for full OpenMP 3.0 support
void XOMP_loop_ordered_guided_init(
    int lower,
    int upper,
    int stride,
    int chunk_size);
unsigned char XOMP_loop_ordered_guided_start(
    long a,
    long b,
    long c,
    long d,
    long *e,
    long *f);
unsigned char XOMP_loop_ordered_guided_next(
    long *a,
    long *b);
void XOMP_task(
    void (*a) (void *),
    void *b,
    void (*c) (void *,
	       void *),
    long d,
    long e,
    unsigned char f,
    unsigned g);
void XOMP_taskwait(
    void);
void XOMP_loop_static_init(
    int lower,
    int upper,
    int stride,
    int chunk_size);
void XOMP_loop_dynamic_init(
    int lower,
    int upper,
    int stride,
    int chunk_size);
void XOMP_loop_runtime_init(
    int lower,
    int upper,
    int stride);
//ordered case
void XOMP_loop_ordered_static_init(
    int lower,
    int upper,
    int stride,
    int chunk_size);
void XOMP_loop_ordered_dynamic_init(
    int lower,
    int upper,
    int stride,
    int chunk_size);
void XOMP_loop_ordered_runtime_init(
    int lower,
    int upper,
    int stride);
// rest of the functions
// omp ordered directive
void XOMP_ordered_start(
    void);
void XOMP_ordered_end(
    void);
unsigned char XOMP_loop_static_start(
    long a,
    long b,
    long c,
    long d,
    long *e,
    long *f);
unsigned char XOMP_loop_dynamic_start(
    long a,
    long b,
    long c,
    long d,
    long *e,
    long *f);
unsigned char XOMP_loop_runtime_start(
    long a,
    long b,
    long c,
    long *d,
    long *e);
unsigned char XOMP_loop_ordered_static_start(
    long a,
    long b,
    long c,
    long d,
    long *e,
    long *f);
unsigned char XOMP_loop_ordered_dynamic_start(
    long a,
    long b,
    long c,
    long d,
    long *e,
    long *f);
unsigned char XOMP_loop_ordered_runtime_start(
    long a,
    long b,
    long c,
    long *d,
    long *e);
// next
unsigned char XOMP_loop_static_next(
    long *a,
    long *b);
unsigned char XOMP_loop_dynamic_next(
    long *a,
    long *b);
unsigned char XOMP_loop_runtime_next(
    long *a,
    long *b);
unsigned char XOMP_loop_ordered_static_next(
    long *a,
    long *b);
unsigned char XOMP_loop_ordered_dynamic_next(
    long *a,
    long *b);
unsigned char XOMP_loop_ordered_runtime_next(
    long *a,
    long *b);
void XOMP_critical_start(
    void **data);
void XOMP_critical_end(
    void **data);
unsigned char XOMP_single(
    void);
unsigned char XOMP_master(
    void);
// flush without variable list
void XOMP_flush_all(
    void);
// omp flush with variable list, flush one by one, given each's start address and size
void XOMP_flush_one(
    char *startAddress,
    int nbyte);
#endif
