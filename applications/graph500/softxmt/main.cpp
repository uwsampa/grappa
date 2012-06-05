/* Copyright (C) 2010 The Trustees of Indiana University.                  */
/*                                                                         */
/* Use, modification and distribution is subject to the Boost Software     */
/* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at */
/* http://www.boost.org/LICENSE_1_0.txt)                                   */
/*                                                                         */
/*  Authors: Jeremiah Willcock                                             */
/*           Andrew Lumsdaine                                              */

/* These need to be before any possible inclusions of stdint.h or inttypes.h.
 * */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "../generator/make_graph.h"
#include "../generator/utils.h"
#include "common.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>

#include "SoftXMT.hpp"
#include "GlobalAllocator.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"
#include "Delegate.hpp"
#include "timer.h"
#include "rmat.h"
#include "oned_csr.h"
#include "verify.hpp"
#include "options.h"

#include "PerformanceTools.hpp"

static int compare_doubles(const void* a, const void* b) {
  double aa = *(const double*)a;
  double bb = *(const double*)b;
  return (aa < bb) ? -1 : (aa == bb) ? 0 : 1;
}
enum {s_minimum, s_firstquartile, s_median, s_thirdquartile, s_maximum, s_mean, s_std, s_LAST};
static void get_statistics(const double x[], int n, double r[s_LAST]);
void output_results (const int64_t SCALE, int64_t nvtx_scale, int64_t edgefactor,
                const double A, const double B, const double C, const double D,
                const double generation_time,
                const double construction_time,
                const int NBFS, const double *bfs_time, const int64_t *bfs_nedge);

//### Globals ###
#define MEM_SCALE 36

int64_t nbfs;

//static int64_t bfs_root[NBFS_max];

static double generation_time;
static double construction_time;
static double bfs_time[NBFS_max];
static int64_t bfs_nedge[NBFS_max];

#define XOFF(k) (xoff+2*(k))
#define XENDOFF(k) (xoff+2*(k)+1)

#define read SoftXMT_delegate_read_word

inline bool has_adj(GlobalAddress<int64_t> xoff, int64_t i) {
  int64_t xoi = read(XOFF(i));
  int64_t xei = read(XENDOFF(i));
  return xei-xoi != 0;
}

static void choose_bfs_roots(GlobalAddress<int64_t> xoff, int64_t nvtx, int64_t * NBFS, int64_t bfs_roots[]) {

  // sample from 0..nvtx-1 without replacement
  int64_t m = 0, t = 0;
  while (m < *NBFS && t < nvtx) {
    double R = mrg_get_double_orig(prng_state);
    if (!has_adj(xoff, t) || (nvtx - t)*R > *NBFS - m) ++t;
    else bfs_roots[m++] = t++;
  }
  if (t >= nvtx && m < *NBFS) {
    if (m > 0) {
      LOG(INFO) << "Cannot find enough sample roots, using " << m;
      *NBFS = m;
    } else {
      LOG(ERROR) << "Cannot find any sample roots! Exiting...";
      exit(1);
    }
  }
}


LOOP_FUNCTION(func_enable_tau, nid) {
  //TAU_ENABLE_INSTRUMENTATION();
  //TAU_ENABLE_GROUP(TAU_USER);
  //TAU_ENABLE_GROUP(TAU_USER1);
  //TAU_ENABLE_GROUP(TAU_USER2);
  TAU_ENABLE_GROUP(TAU_USER3);

  FLAGS_record_grappa_events = true;
}
LOOP_FUNCTION(func_enable_google_profiler, nid) {
  SoftXMT_start_profiling();
}
static void enable_tau() {
#ifdef GRAPPA_TRACE
  VLOG(1) << "Enabling TAU recording.";
  func_enable_tau f;
  fork_join_custom(&f);
#endif
#ifdef GOOGLE_PROFILER
  func_enable_google_profiler g;
  fork_join_custom(&g);
#endif
}
LOOP_FUNCTION(func_disable_tau, nid) {
  //TAU_DISABLE_INSTRUMENTATION();
  //TAU_DISABLE_GROUP(TAU_USER);
  //TAU_DISABLE_GROUP(TAU_USER1);
  //TAU_DISABLE_GROUP(TAU_USER2);
  TAU_DISABLE_GROUP(TAU_USER3);

  FLAGS_record_grappa_events = false;
}
LOOP_FUNCTION(func_disable_google_profiler, nid) {
  SoftXMT_stop_profiling();
}
static void disable_tau() {
#ifdef GRAPPA_TRACE
  VLOG(1) << "Disabling TAU recording.";
  func_disable_tau f;
  fork_join_custom(&f);
#endif
#ifdef GOOGLE_PROFILER
  func_disable_google_profiler g;
  fork_join_custom(&g);
#endif
}

static void run_bfs(tuple_graph * tg, csr_graph * g, int64_t * bfs_roots) {
  double t;
  
  // build bfs tree for each root
  for (int64_t i=0; i < nbfs; i++) {
    GlobalAddress<int64_t> bfs_tree = SoftXMT_typed_malloc<int64_t>(g->nv);
    GlobalAddress<int64_t> max_bfsvtx;
    
    VLOG(1) << "Running bfs on root " << i << "(" << bfs_roots[i] << ")...";
    SoftXMT_reset_stats_all_nodes();
    
    enable_tau();

    t = timer();
    bfs_time[i] = make_bfs_tree(g, bfs_tree, bfs_roots[i]);
    t = timer() - t;

    disable_tau();
    
    VLOG(1) << "make_bfs_tree time: " << t;

    VLOG(1) << "Verifying bfs " << i << "...";
    t = timer();
    bfs_nedge[i] = verify_bfs_tree(bfs_tree, g->nv-1, bfs_roots[i], tg);
    t = timer() - t;
    VLOG(1) << "verify time: " << t;
    
    if (bfs_nedge[i] < 0) {
      LOG(ERROR) << "bfs " << i << " from root " << bfs_roots[i] << " failed verification: " << bfs_nedge[i];
      exit(1);
    } else {
      VLOG(1) << "bfs_time[" << i << "] = " << bfs_time[i];
    }
    
    TIME(t, SoftXMT_free(bfs_tree));
    VLOG(1) << "Free bfs_tree time: " << t;
  }
}

static void checkpoint_in(tuple_graph * tg, csr_graph * g, int64_t * bfs_roots) {
  TAU_PHASE("checkpoint_in","void (tuple_graph*,csr_graph*,int64_t*)", TAU_USER);
  
  VLOG(1) << "start reading checkpoint";
  double t = timer();
  
  char fname[256];
  sprintf(fname, "ckpts/graph500.%lld.%lld.ckpt", SCALE, edgefactor);
  FILE * fin = fopen(fname, "r");
  if (!fin) {
    LOG(ERROR) << "Unable to open file: " << fname << ", will generate graph and write checkpoint.";
    load_checkpoint = false;
    write_checkpoint = true;
    return;
  }
  
  fread(&tg->nedge, sizeof(tg->nedge), 1, fin);
  fread(&g->nv, sizeof(g->nv), 1, fin);
  fread(&g->nadj, sizeof(g->nadj), 1, fin);
  fread(&nbfs, sizeof(nbfs), 1, fin);
  
  tg->edges = SoftXMT_typed_malloc<packed_edge>(tg->nedge);
  g->xoff = SoftXMT_typed_malloc<int64_t>(2*g->nv+2);
  g->xadjstore = SoftXMT_typed_malloc<int64_t>(g->nadj);
  g->xadj = g->xadjstore+2;
  
  // write out edge tuples
  read_array(tg->edges, tg->nedge, fin);
  
  // xoff
  read_array(g->xoff, 2*g->nv+2, fin);
  
  // xadj
  read_array(g->xadjstore, g->nadj, fin);
  
  // bfs_roots
  fread(bfs_roots, sizeof(int64_t), nbfs, fin);
  
  fclose(fin);
  
  t = timer() - t;
  VLOG(1) << "done reading in checkpoint (time = " << t << ")";
}

static void checkpoint_out(tuple_graph * tg, csr_graph * g, int64_t * bfs_roots) {
  VLOG(1) << "starting checkpoint output";
  double t = timer();
  
  char fname[256];
  sprintf(fname, "ckpts/graph500.%lld.%lld.ckpt", SCALE, edgefactor);
  FILE * fout = fopen(fname, "w");
  if (!fout) {
    LOG(ERROR) << "Unable to open file for writing: " << fname;
    exit(1);
  }
  
  fwrite(&tg->nedge, sizeof(tg->nedge), 1, fout);
  fwrite(&g->nv, sizeof(g->nv), 1, fout);
  fwrite(&g->nadj, sizeof(g->nadj), 1, fout);
  fwrite(&nbfs, sizeof(nbfs), 1, fout);
  
  // write out edge tuples
  write_array(tg->edges, tg->nedge, fout);
  
  // xoff
  write_array(g->xoff, 2*g->nv+2, fout);
  
  // xadj
  write_array(g->xadjstore, g->nadj, fout);
  
  // bfs_roots
  fwrite(bfs_roots, sizeof(int64_t), nbfs, fout);
  
  fclose(fout);
  
  t = timer() - t;
  VLOG(1) << "done writing checkpoint (time = " << t << ")";
}

static void setup_bfs(tuple_graph * tg, csr_graph * g, int64_t * bfs_roots) {
  
  TIME(construction_time,
       create_graph_from_edgelist(tg, g)
  );
  VLOG(1) << "construction_time = " << construction_time;
  
  GlobalAddress<int64_t> xoff = g->xoff;
//  for (int64_t i=0; i < g.nv; i++) {
//    VLOG(1) << "xoff[" << i << "] = " << read(XOFF(i)) << " -> " << read(XENDOFF(i));
//  }
//#ifdef DEBUG
//  for (int64_t i=0; i<g.nv; i++) {
//    std::stringstream ss;
//    int64_t xoi = read(XOFF(i)), xei = read(XENDOFF(i));
//    CHECK(xoi <= g.nadj) << i << ": " << xoi << " > " << g.nadj;
//    CHECK(xei <= g.nadj) << i << ": " << xei << " > " << g.nadj;
//    ss << "xoff[" << i << "] = " << xoi << "->" << xei << ": (";
//    for (int64_t j=xoi; j<xei; j++) {
//      ss << read(g.xadj+j) << ",";
//    }
//    ss << ")";
//    VLOG(2) << ss.str();
//  }
//#endif
  double t;
  
  // no rootname input method, so randomly choose
  nbfs = NBFS_max;
  TIME(t, choose_bfs_roots(g->xoff, g->nv, &nbfs, bfs_roots));
  VLOG(1) << "choose_bfs_roots time: " << t;
  
//  for (int64_t i=0; i < nbfs; i++) {
//    VLOG(1) << "bfs_roots[" << i << "] = " << bfs_roots[i];
//  }
}

static void user_main(int * args) {
  double t = timer();
  
  tuple_graph tg;
  csr_graph g;
  int64_t bfs_roots[NBFS_max];



  if (load_checkpoint) {
    checkpoint_in(&tg, &g, bfs_roots);
  } // checkpoint_in may change 'load_checkpoint' to false if unable to read in file correctly
  
  if (!load_checkpoint) {
    
    tg.nedge = (int64_t)(edgefactor) << SCALE;
    tg.edges = SoftXMT_typed_malloc<packed_edge>(tg.nedge);
    
    /* Make the raw graph edges. */
    /* Get roots for BFS runs, plus maximum vertex with non-zero degree (used by
     * validator). */
    
    double start, stop;
    start = timer();
    {
      /* As the graph is being generated, also keep a bitmap of vertices with
       * incident edges.  We keep a grid of processes, each row of which has a
       * separate copy of the bitmap (distributed among the processes in the
       * row), and then do an allreduce at the end.  This scheme is used to avoid
       * non-local communication and reading the file separately just to find BFS
       * roots. */
      
      rmat_edgelist(&tg, SCALE);
      
      //debug: verify that rmat edgelist is valid
  //    for (int64_t i=0; i<tg.nedge; i++) {
  //      Incoherent<packed_edge>::RO e(tg.edges+i, 1);
  //      VLOG(1) << "edge[" << i << "] = " << (*e).v0 << " -> " << (*e).v1;
  //    }
  //    
  //    int64_t NV = 1<<SCALE;
  //    int64_t degree[NV];
  //    for (int64_t i=0; i<NV; i++) degree[i] = 0;
  //    
  //    for (int64_t i=0; i<tg.nedge; i++) {
  //      Incoherent<packed_edge>::RO e(tg.edges+i, 1);
  //      assert(e[0].v0 < NV);
  //      assert(e[0].v1 < NV);
  //      if (e[0].v0 != e[0].v1) {
  //        degree[e[0].v0]++;
  //        degree[e[0].v1]++;
  //      }
  //    }
  //    for (int64_t i=0; i<NV; i++) {
  //      VLOG(1) << "degree[" << i << "] = " << degree[i];
  //    }
    }
    stop = timer();
    generation_time = stop - start;
    VLOG(1) << "graph_generation: " << generation_time;
    
    setup_bfs(&tg, &g, bfs_roots);
    
    if (write_checkpoint) checkpoint_out(&tg, &g, bfs_roots);
  }
  
  SoftXMT_reset_stats();
  
  run_bfs(&tg, &g, bfs_roots);
  
//  free_graph_data_structure();
  
  SoftXMT_free(tg.edges);
  
  /* Print results. */
  output_results(SCALE, 1<<SCALE, edgefactor, A, B, C, D, generation_time, construction_time, (int)nbfs, bfs_time, bfs_nedge);

  t = timer() - t;
  std::cout << "total_runtime: " << t << std::endl;
  
}

int main(int argc, char** argv) {
  SoftXMT_init(&argc, &argv, (1L<<MEM_SCALE));
  SoftXMT_activate();

  //TAU_DISABLE_GROUP(TAU_DEFAULT);
  //TAU_DISABLE_GROUP(TAU_USER);
  TAU_DISABLE_GROUP(TAU_USER3);
  //TAU_DISABLE_INSTRUMENTATION();
  //TAU_DISABLE_ALL_GROUPS();
  //TAU_ENABLE_GROUP(TAU_USER1);

  /* Parse arguments. */
  get_options(argc, argv);

  SoftXMT_run_user_main(&user_main, (int*)NULL);

  LOG(INFO) << "waiting to finish";
  
  SoftXMT_finish(0);
  return 0;
}

#define NSTAT 9
#define PRINT_STATS(lbl, israte)					\
  do {									\
    printf ("min_%s: %20.17e\n", lbl, stats[0]);			\
    printf ("firstquartile_%s: %20.17e\n", lbl, stats[1]);		\
    printf ("median_%s: %20.17e\n", lbl, stats[2]);			\
    printf ("thirdquartile_%s: %20.17e\n", lbl, stats[3]);		\
    printf ("max_%s: %20.17e\n", lbl, stats[4]);			\
    if (!israte) {							\
      printf ("mean_%s: %20.17e\n", lbl, stats[5]);			\
      printf ("stddev_%s: %20.17e\n", lbl, stats[6]);			\
    } else {								\
      printf ("harmonic_mean_%s: %20.17e\n", lbl, stats[7]);		\
      printf ("harmonic_stddev_%s: %20.17e\n", lbl, stats[8]);	\
    }									\
  } while (0)

static int
dcmp (const void *a, const void *b)
{
	const double da = *(const double*)a;
	const double db = *(const double*)b;
	if (da > db) return 1;
	if (db > da) return -1;
	if (da == db) return 0;
	fprintf (stderr, "No NaNs permitted in output.\n");
	abort ();
	return 0;
}

static void get_statistics(const double x[], int n, double r[s_LAST]) {
  double temp;
  int i;
  /* Compute mean. */
  temp = 0;
  for (i = 0; i < n; ++i) temp += x[i];
  temp /= n;
  r[s_mean] = temp;
  /* Compute std. dev. */
  temp = 0;
  for (i = 0; i < n; ++i) temp += (x[i] - r[s_mean]) * (x[i] - r[s_mean]);
  temp /= n - 1;
  r[s_std] = sqrt(temp);
  /* Sort x. */
  double* xx = (double*)xmalloc(n * sizeof(double));
  memcpy(xx, x, n * sizeof(double));
  qsort(xx, n, sizeof(double), compare_doubles);
  /* Get order statistics. */
  r[s_minimum] = xx[0];
  r[s_firstquartile] = (xx[(n - 1) / 4] + xx[n / 4]) * .5;
  r[s_median] = (xx[(n - 1) / 2] + xx[n / 2]) * .5;
  r[s_thirdquartile] = (xx[n - 1 - (n - 1) / 4] + xx[n - 1 - n / 4]) * .5;
  r[s_maximum] = xx[n - 1];
  /* Clean up. */
  free(xx);
}

static void statistics(double *out, double *data, int64_t n) {
	long double s, mean;
	double t;
	int64_t k;
	
	/* Quartiles */
	qsort (data, n, sizeof (*data), dcmp);
	out[0] = data[0];
	t = (n+1) / 4.0;
	k = (int) t;
	if (t == k)
		out[1] = data[k];
	else
		out[1] = 3*(data[k]/4.0) + data[k+1]/4.0;
	t = (n+1) / 2.0;
	k = (int) t;
	if (t == k)
		out[2] = data[k];
	else
		out[2] = data[k]/2.0 + data[k+1]/2.0;
	t = 3*((n+1) / 4.0);
	k = (int) t;
	if (t == k)
		out[3] = data[k];
	else
		out[3] = data[k]/4.0 + 3*(data[k+1]/4.0);
	out[4] = data[n-1];
	
	s = data[n-1];
	for (k = n-1; k > 0; --k)
		s += data[k-1];
	mean = s/n;
	out[5] = mean;
	s = data[n-1] - mean;
	s *= s;
	for (k = n-1; k > 0; --k) {
		long double tmp = data[k-1] - mean;
		s += tmp * tmp;
	}
	out[6] = sqrt (s/(n-1));
	
	s = (data[0]? 1.0L/data[0] : 0);
	for (k = 1; k < n; ++k)
		s += (data[k]? 1.0L/data[k] : 0);
	out[7] = n/s;
	mean = s/n;
	
	/*
	 Nilan Norris, The Standard Errors of the Geometric and Harmonic
	 Means and Their Application to Index Numbers, 1940.
	 http://www.jstor.org/stable/2235723
	 */
	s = (data[0]? 1.0L/data[0] : 0) - mean;
	s *= s;
	for (k = 1; k < n; ++k) {
		long double tmp = (data[k]? 1.0L/data[k] : 0) - mean;
		s += tmp * tmp;
	}
	s = (sqrt (s)/(n-1)) * out[7] * out[7];
	out[8] = s;
}

void output_results(const int64_t SCALE, int64_t nvtx_scale, int64_t edgefactor,
                    const double A, const double B, const double C, const double D,
                    const double generation_time,
                    const double construction_time,
                    const int NBFS, const double *bfs_time, const int64_t *bfs_nedge) {
	int k;
	int64_t sz;
	double tm[NBFS];
	double stats[NSTAT];
	
	if (!tm || !stats) {
		perror ("Error allocating within final statistics calculation.");
		abort ();
	}
	
	sz = (1L << SCALE) * edgefactor * 2 * sizeof (int64_t);
	printf ("SCALE: %" PRId64 "\nnvtx: %" PRId64 "\nedgefactor: %" PRId64 "\n"
          "terasize: %20.17e\n",
          SCALE, nvtx_scale, edgefactor, sz/1.0e12);
	printf ("A: %20.17e\nB: %20.17e\nC: %20.17e\nD: %20.17e\n", A, B, C, D);
	printf ("generation_time: %20.17e\n", generation_time);
	printf ("construction_time: %20.17e\n", construction_time);
	printf ("nbfs: %d\n", NBFS);
	
	memcpy(tm, bfs_time, NBFS*sizeof(tm[0]));
	statistics(stats, tm, NBFS);
	PRINT_STATS("time", 0);
	
	for (k = 0; k < NBFS; ++k)
		tm[k] = bfs_nedge[k];
	statistics(stats, tm, NBFS);
	PRINT_STATS("nedge", 0);
	
	for (k = 0; k < NBFS; ++k)
		tm[k] = bfs_nedge[k] / bfs_time[k];
	statistics(stats, tm, NBFS);
	PRINT_STATS("TEPS", 1);
}
