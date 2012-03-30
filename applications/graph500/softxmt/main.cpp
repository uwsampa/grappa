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
#include "timer.h"

int lgsize;

static int compare_doubles(const void* a, const void* b) {
  double aa = *(const double*)a;
  double bb = *(const double*)b;
  return (aa < bb) ? -1 : (aa == bb) ? 0 : 1;
}

enum {s_minimum, s_firstquartile, s_median, s_thirdquartile, s_maximum, s_mean, s_std, s_LAST};
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

int main(int argc, char** argv) {
  SoftXMT_init(&argc, &argv);
  SoftXMT_activate();

  Node rank = SoftXMT_mynode();
  
  /* Parse arguments. */
  int SCALE = 16;
  int edgefactor = 16; /* nedges / nvertices, i.e., 2*avg. degree */
  if (argc >= 2) SCALE = atoi(argv[1]);
  if (argc >= 3) edgefactor = atoi(argv[2]);
  if (argc <= 1 || argc >= 4 || SCALE == 0 || edgefactor == 0) {
    if (rank == 0) {
      fprintf(stderr, "Usage: %s SCALE edgefactor\n  SCALE = log_2(# vertices) [integer, required]\n  edgefactor = (# edges) / (# vertices) = .5 * (average vertex degree) [integer, defaults to 16]\n(Random number seed and Kronecker initiator are in main.c)\n", argv[0]);
    }
  }
  uint64_t seed1 = 2, seed2 = 3;

  tuple_graph tg;
  tg.nglobaledges = (int64_t)(edgefactor) << SCALE;
  int64_t nglobalverts = (int64_t)(1) << SCALE;

  /* Make the raw graph edges. */
  /* Get roots for BFS runs, plus maximum vertex with non-zero degree (used by
   * validator). */
  int num_bfs_roots = 64;
  int64_t* bfs_roots = (int64_t*)xmalloc(num_bfs_roots * sizeof(int64_t));
  int64_t max_used_vertex = 0;

  double start, stop;
  start = timer();
  {
    /* Spread the two 64-bit numbers into five nonzero values in the correct
     * range. */
    uint_fast32_t seed[5];
    make_mrg_seed(seed1, seed2, seed);

    /* As the graph is being generated, also keep a bitmap of vertices with
     * incident edges.  We keep a grid of processes, each row of which has a
     * separate copy of the bitmap (distributed among the processes in the
     * row), and then do an allreduce at the end.  This scheme is used to avoid
     * non-local communication and reading the file separately just to find BFS
     * roots. */
    
    
    //TODO
  }
  stop = timer();
  double make_graph_time = stop - start;
  if (rank == 0) { /* Not an official part of the results */
    fprintf(stderr, "graph_generation:               %f s\n", make_graph_time);
  }

  free(bfs_roots);
  free_graph_data_structure();

  free(tg.edgememory); tg.edgememory = NULL;

  /* Print results. */
  if (rank == 0) {
    if (!validation_passed) {
      fprintf(stdout, "No results printed for invalid run.\n");
    } else {
      int i;
      fprintf(stdout, "SCALE:                          %d\n", SCALE);
      fprintf(stdout, "edgefactor:                     %d\n", edgefactor);
      fprintf(stdout, "NBFS:                           %d\n", num_bfs_roots);
      fprintf(stdout, "graph_generation:               %g\n", make_graph_time);
      fprintf(stdout, "num_processes:              %d\n", SoftXMT_nodes());
      fprintf(stdout, "construction_time:              %g\n", data_struct_time);
      double stats[s_LAST];
      get_statistics(bfs_times, num_bfs_roots, stats);
      fprintf(stdout, "min_time:                       %g\n", stats[s_minimum]);
      fprintf(stdout, "firstquartile_time:             %g\n", stats[s_firstquartile]);
      fprintf(stdout, "median_time:                    %g\n", stats[s_median]);
      fprintf(stdout, "thirdquartile_time:             %g\n", stats[s_thirdquartile]);
      fprintf(stdout, "max_time:                       %g\n", stats[s_maximum]);
      fprintf(stdout, "mean_time:                      %g\n", stats[s_mean]);
      fprintf(stdout, "stddev_time:                    %g\n", stats[s_std]);
      get_statistics(edge_counts, num_bfs_roots, stats);
      fprintf(stdout, "min_nedge:                      %.11g\n", stats[s_minimum]);
      fprintf(stdout, "firstquartile_nedge:            %.11g\n", stats[s_firstquartile]);
      fprintf(stdout, "median_nedge:                   %.11g\n", stats[s_median]);
      fprintf(stdout, "thirdquartile_nedge:            %.11g\n", stats[s_thirdquartile]);
      fprintf(stdout, "max_nedge:                      %.11g\n", stats[s_maximum]);
      fprintf(stdout, "mean_nedge:                     %.11g\n", stats[s_mean]);
      fprintf(stdout, "stddev_nedge:                   %.11g\n", stats[s_std]);
      double* secs_per_edge = (double*)xmalloc(num_bfs_roots * sizeof(double));
      for (i = 0; i < num_bfs_roots; ++i) secs_per_edge[i] = bfs_times[i] / edge_counts[i];
      get_statistics(secs_per_edge, num_bfs_roots, stats);
      fprintf(stdout, "min_TEPS:                       %g\n", 1. / stats[s_maximum]);
      fprintf(stdout, "firstquartile_TEPS:             %g\n", 1. / stats[s_thirdquartile]);
      fprintf(stdout, "median_TEPS:                    %g\n", 1. / stats[s_median]);
      fprintf(stdout, "thirdquartile_TEPS:             %g\n", 1. / stats[s_firstquartile]);
      fprintf(stdout, "max_TEPS:                       %g\n", 1. / stats[s_minimum]);
      fprintf(stdout, "harmonic_mean_TEPS:             %g\n", 1. / stats[s_mean]);
      /* Formula from:
       * Title: The Standard Errors of the Geometric and Harmonic Means and
       *        Their Application to Index Numbers
       * Author(s): Nilan Norris
       * Source: The Annals of Mathematical Statistics, Vol. 11, No. 4 (Dec., 1940), pp. 445-448
       * Publisher(s): Institute of Mathematical Statistics
       * Stable URL: http://www.jstor.org/stable/2235723
       * (same source as in specification). */
      fprintf(stdout, "harmonic_stddev_TEPS:           %g\n", stats[s_std] / (stats[s_mean] * stats[s_mean] * sqrt(num_bfs_roots - 1)));
      free(secs_per_edge); secs_per_edge = NULL;
      free(edge_counts); edge_counts = NULL;
      get_statistics(validate_times, num_bfs_roots, stats);
      fprintf(stdout, "min_validate:                   %g\n", stats[s_minimum]);
      fprintf(stdout, "firstquartile_validate:         %g\n", stats[s_firstquartile]);
      fprintf(stdout, "median_validate:                %g\n", stats[s_median]);
      fprintf(stdout, "thirdquartile_validate:         %g\n", stats[s_thirdquartile]);
      fprintf(stdout, "max_validate:                   %g\n", stats[s_maximum]);
      fprintf(stdout, "mean_validate:                  %g\n", stats[s_mean]);
      fprintf(stdout, "stddev_validate:                %g\n", stats[s_std]);
#if 0
      for (i = 0; i < num_bfs_roots; ++i) {
        fprintf(stdout, "Run %3d:                        %g s, validation %g s\n", i + 1, bfs_times[i], validate_times[i]);
      }
#endif
    }
  }
  free(bfs_times);
  free(validate_times);

  SoftXMT_finish(0);
  return 0;
}
