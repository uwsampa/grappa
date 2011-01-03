#include "stdint.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include <sys/time.h>
#include "thread.h"
long int random(void);

typedef struct {
  int64_t *rowstart;
  int64_t *col;
  double *val;
} CSR;

typedef struct {
  int64_t id;
  int64_t m;
  int64_t m0;
  double * y;
  double * x;
  CSR A;
} loop_arg;

/*
void foo() {
  static double v[12];
#pragma omp parallel 
  {
    double *vt =  &v[omp_get_thread_num()];
    int i;
#pragma omp for schedule(static)
    for (i = 0; i < 999999; i++) {
      int j;
      double tmp = *vt;
      for (j = 0; j < 99999; j++) tmp = tmp*tmp;
      *vt = tmp;
    }
  }
}
*/

void matmul(double * y, CSR A, int m, double * x) {
  // Basic SpMV implementation, 
  // y <- y + A*x, where A is in CSR. 
  int i;
#pragma omp parallel for
  for ( i = 0; i < m; ++i) { 
    int k;
    double y0 = y[i];
    for ( k = A.rowstart[i]; k < A.rowstart[i+1]; ++k) {
      y0 += A.val[k] * x[A.col[k]];
    }
    y[i] = y0;
  }
}

/* called from a green thread */
void matmul_green(thread *me, void *arg) {
  loop_arg *la = (loop_arg *) arg;
  int i;
  int m = la->m;
  int m0 = la->m0;
  double *y = la->y;
  double *x = la->x;
  CSR A = la->A;

  // Basic SpMV implementation, 
  // y <- y + A*x, where A is in CSR. 
  for ( i = m0; i < m0+m; ++i) { 
    int k;
    double y0 = y[i];
    for ( k = A.rowstart[i]; k < A.rowstart[i+1]; ++k) {
      prefetch_and_switch(me, &(x[A.col[k]]), 0);
      y0 += A.val[k] * x[A.col[k]];
    }
    y[i] = y0;
  }
}


int main(int argc, char ** argv) {
  int i;
  int N = atoi(argv[1]);
  int NZ = argc > 2 ? atoi(argv[2]) : 2;
  int num_cores = argc > 3 ? atoi(argv[3]) : 1;
  int iters = argc > 4 ? atoi(argv[4]) : 1;
  int threads_per_core = argc > 5 ? atoi(argv[5]) : 0;
  omp_set_num_threads(num_cores);

  fprintf(stderr, "%d iters of SpMv with %d x %d matrix, %d nonzeros per row, targeting one thread on each of %d cores out of %d cores available\n", iters, N, N, NZ, num_cores, omp_get_num_procs());
  if (threads_per_core) fprintf(stderr, "Also running green threads w/ %d threads per core on %d cores\n", threads_per_core, num_cores);
  CSR A;
  A.rowstart = (int64_t *) malloc(sizeof(A.rowstart[0])*(N+1));
  A.col = (int64_t *) malloc(sizeof(A.col[0])*N*NZ);
  A.val = (double *) malloc(sizeof(A.val[0])*N*NZ);

  for ( i = 0; i <= N; i++) A.rowstart[i] = i*NZ;
  for ( i = 0; i < N*NZ; i++) A.col[i] = i % N; /* random() % N; */
  for ( i = 0; i < N*NZ; i++) A.val[i] = 1.0;

  double * x = (double *) malloc(sizeof(double)*N);
  double * y = (double *) malloc(sizeof(double)*N);
  for ( i = 0; i < N; i++) x[i] = 1.0;
  memset(y, 0, sizeof(double)*N);

#pragma omp parallel
  if (omp_get_num_threads() < num_cores) fprintf(stderr, "num threads is really %d\n", omp_get_num_threads());

  struct timeval start,finish;
  gettimeofday(&start,NULL);
  for (i = 0; i < iters; i++) matmul(y, A, N, x);
  gettimeofday(&finish,NULL);
  double elapsed = ((finish.tv_sec - start.tv_sec)*1000000.0 + finish.tv_usec - start.tv_usec)/1000000.0;
  fprintf(stderr, "pthreads: %g flops:  %d iters each in %g secs\n", iters*NZ*2*N/elapsed,iters, elapsed/iters);

  /* check sum */
  double check = N*NZ*1.0*iters;
  for (i = 0; i < N; i++) check -= y[i];
  if (check != 0.0) fprintf(stderr, "pthreads: Checksum failed: %g\n", check);



  /* same computation with green threads */
  if (!threads_per_core) exit(0);
  memset(y, 0, sizeof(double)*N);

  gettimeofday(&start,NULL);
  for (i = 0; i < iters; i++) {
    int c;
#pragma omp parallel for
    for ( c = 0; c < num_cores; c++) { 
      loop_arg * la = (loop_arg *) malloc(sizeof(loop_arg)*threads_per_core);

      thread *master = thread_init();
      scheduler * sched = create_scheduler(master);
      thread *thread;

      /* M in this scope is # rows for which this core is responsible; M0 is index of the first of these rows */
#define ROWS_IN_BLOCK(num_rows, block_index, num_blocks) ((num_rows/num_blocks)+(block_index < num_rows%num_blocks?1:0))
#define FIRST_ROW_IN_BLOCK(num_rows, block_index, num_blocks) ((num_rows/num_blocks)*block_index + (block_index < num_rows%num_blocks?block_index:num_rows%num_blocks))
      int M = ROWS_IN_BLOCK(N,c,num_cores);
      int M0 = FIRST_ROW_IN_BLOCK(N,c,num_cores);
      fprintf(stderr, "gthreads: Core %d responsible for rows %d to %d\n", c, M0, M0+M-1);

      int t;
      for (t = 0; t < threads_per_core; ++t) {
	/* each green thread is responsible for a contiguous block of roughly M/threads_per_core rows */
	int m = ROWS_IN_BLOCK(M,t,threads_per_core);
	int m0 = FIRST_ROW_IN_BLOCK(M,t,threads_per_core);
	la[t].id = t;
        la[t].x = x;
	la[t].y = y;
	la[t].A.rowstart = &A.rowstart[M0+m0];
	la[t].A.col = &A.col[(M0+m0)*NZ];
	la[t].A.val = &A.val[(M0+m0)*NZ];
	la[t].m = m;
	la[t].m0 = m0;
        thread = thread_spawn(master, sched, matmul_green, (void*) &la[t]);
      }
      for (t = 0; t < threads_per_core; ++t) {
	fprintf(stderr, "t %d: id %ld, x %p, y %p, rs %ld, col %ld, val %ld, m %ld m0 %ld\n", t, la[t].id, (void*)la[t].x, (void*)la[t].y,
		la[t].A.rowstart - A.rowstart, la[t].A.col - A.col,
		la[t].A.val -A.val,
		la[t].m, la[t].m0);
      }
      fprintf(stderr, "running sched core %d\n", c);
      run_all(sched);
      fprintf(stderr, "done running sched core %d\n", c);
      destroy_scheduler(sched);
      destroy_thread(master);
      free(la);
    }
  }
  gettimeofday(&finish,NULL);

  elapsed = ((finish.tv_sec - start.tv_sec)*1000000.0 + finish.tv_usec - start.tv_usec)/1000000.0;
  fprintf(stderr, "gthreads: %g flops:  %d iters each in %g secs\n", iters*NZ*2*N/elapsed,iters, elapsed/iters);

  /* check sum */
  check = N*NZ*1.0*iters;
  for (i = 0; i < N; i++) check -= y[i];
  if (check != 0.0) fprintf(stderr, "gthreads: Checksum failed w/ green threads: %g\n", check);
}
