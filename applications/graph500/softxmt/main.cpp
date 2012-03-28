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

static int compare_doubles(const void* a, const void* b) {
  double aa = *(const double*)a;
  double bb = *(const double*)b;
  return (aa < bb) ? -1 : (aa == bb) ? 0 : 1;
}
enum {s_minimum, s_firstquartile, s_median, s_thirdquartile, s_maximum, s_mean, s_std, s_LAST};
static void get_statistics(const double x[], int n, double r[s_LAST]);

//### Globals ###
#define NBFS_max 64
#define MAX_SCALE 20

int SCALE;
int edgefactor;

//static int64_t bfs_root[NBFS_max];

static double generation_time;
static double construction_time;
static double bfs_time[NBFS_max];
static int64_t bfs_nedge[NBFS_max];

int lgsize;

static void run_bfs(tuple_graph * tg) {
  csr_graph g;
  
  TIME(construction_time,
       convert_graph_to_oned_csr(tg, &g)
  );
  LOG(INFO) << "construction_time = " << construction_time;
  
  
}

static void user_main(thread * me, void * args) {    
  tuple_graph tg;
  tg.nedge = (int64_t)(edgefactor) << SCALE;
  tg.edges = SoftXMT_typed_malloc<packed_edge>(tg.nedge);

  int64_t NV = (int64_t)(1) << SCALE;
  
  /* Make the raw graph edges. */
  /* Get roots for BFS runs, plus maximum vertex with non-zero degree (used by
   * validator). */
  int num_bfs_roots = NBFS_max;
  //  int64_t* bfs_roots = (int64_t*)xmalloc(num_bfs_roots * sizeof(int64_t));
  GlobalAddress<int64_t> bfs_roots = SoftXMT_typed_malloc<int64_t>(num_bfs_roots);
  int64_t max_used_vertex = 0;
  
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
  double make_graph_time = stop - start;
  fprintf(stderr, "graph_generation:               %f s\n", make_graph_time);
  
  run_bfs(&tg);
  
  SoftXMT_free(bfs_roots);
//  free_graph_data_structure();
  
  SoftXMT_free(tg.edges);
  
  SoftXMT_signal_done();
}

int main(int argc, char** argv) {
  SoftXMT_init(&argc, &argv, 1<<(MAX_SCALE+2));
  SoftXMT_activate();

  Node rank = SoftXMT_mynode();
  
  /* Parse arguments. */
  SCALE = 16;
  edgefactor = 16; /* nedges / nvertices, i.e., 2*avg. degree */
  if (argc >= 2) SCALE = atoi(argv[1]);
  if (argc >= 3) edgefactor = atoi(argv[2]);
  if (argc <= 1 || argc >= 4 || SCALE == 0 || edgefactor == 0) {
    if (rank == 0) {
      fprintf(stderr, "Usage: %s SCALE edgefactor\n  SCALE = log_2(# vertices) [integer, required]\n  edgefactor = (# edges) / (# vertices) = .5 * (average vertex degree) [integer, defaults to 16]\n(Random number seed and Kronecker initiator are in main.c)\n", argv[0]);
    }
    exit(0);
  }
  assert(SCALE <= MAX_SCALE);

  SoftXMT_run_user_main(&user_main, NULL);
  
  /* Print results. */

  SoftXMT_finish(0);
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

