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
#include "timer.h"

static int compare_doubles(const void* a, const void* b) {
  double aa = *(const double*)a;
  double bb = *(const double*)b;
  return (aa < bb) ? -1 : (aa == bb) ? 0 : 1;
}
enum {s_minimum, s_firstquartile, s_median, s_thirdquartile, s_maximum, s_mean, s_std, s_LAST};
static void get_statistics(const double x[], int n, double r[s_LAST]);


//### Globals ###
#define MAX_SCALE 20

int lgsize;
double A = 0.57;
double B = 0.19;
double C = 0.19;
double D = 1.0-(A+B+C);

static mrg_state prng_state_store;

#define NRAND(ne) (5 * SCALE * (ne))

struct func_initialize : public ForkJoinIteration {
  GlobalAddress<int64_t> base_addr;
  int64_t value;
  void operator()(thread * me, int64_t index) {
    SLOG(2) << "called func_initialize with index = " << index;
    Incoherent<int64_t>::RW c(base_addr+index, 1);
    c[0] = value+index;
  }
};

struct randpermute_func : public ForkJoinIteration {
  GlobalAddress<int64_t> array;
  int64_t nelem;
  mrg_state st;
  void operator()(thread * me, int64_t index) {
    int64_t k = index;
    int64_t place;
//    elt_type Ak, Aplace;
//    Ak = take (&array[k]);
    Incoherent<int64_t>::RW Ak(array+k, 1);
    
    mrg_skip(&st, 1, k, 0);
    
    place = k + (int64_t)floor( mrg_get_double_orig(&st) * (nelem - k) );
    if (k != place) {
      assert (place > k);
      assert (place < nelem);
//      Aplace = take (&array[place]);
      Incoherent<int64_t>::RW Aplace(array+place, 1);
      {
        int64_t t;
        t = array[place];
        array[place] = array[k];
        array[k] = t;
      }
      {
        elt_type t;
        t = Aplace;
        Aplace = Ak;
        Ak = t;
      }
//      release (&array[place], Aplace);
    }
    release (&array[k], Ak);
  }
};

static void permute_vertex_labels (packed_edge * restrict IJ, int64_t nedge, int64_t max_nvtx, mrg_state * restrict st, GlobalAddress<int64_t> newlabel) {
	int64_t k;
	
//  OMP("omp for")
//	for (k = 0; k < max_nvtx; ++k)
//		newlabel[k] = k;
  {
    func_initialize f;
      f.base_addr = newlabel;
      f.value = 0.0;
    fork_join(current_thread, &f, 0, max_nvtx);
  }

  //	randpermute(newlabel, max_nvtx, st);
	{
    randpermute_func f;
      f.array = newlabel;
      f.nelem = max_nvtx;
      f.st = *st;
    fork_join(current_thread, &f, 0, max_nvtx);
  }
  
	
	OMP("omp for")
	for (k = 0; k < nedge; ++k)
		write_edge(&IJ[k],
               newlabel[get_v0_from_edge(&IJ[k])],
               newlabel[get_v1_from_edge(&IJ[k])]);
}

static void permute_edgelist(packed_edge * restrict IJ, int64_t nedge, mrg_state *st) {
	randpermute(IJ, nedge, st);
}

static void rmat_edge(packed_edge *out, int SCALE, double A, double B, double C, double D, const double *rn) {
	size_t rni = 0;
	int64_t i = 0, j = 0;
	int64_t bit = ((int64_t)1) << (SCALE-1);
	
	while (1) {
		const double r = rn[rni++];
		if (r > A) { /* outside quadrant 1 */
			if (r <= A + B) /* in quadrant 2 */
				j |= bit;
			else if (r <= A + B + C) /* in quadrant 3 */
				i |= bit;
			else { /* in quadrant 4 */
				j |= bit;
				i |= bit;
			}
		}
		if (1 == bit) break;
		
		/*
		 Assuming R is in (0, 1), 0.95 + 0.1 * R is in (0.95, 1.05).
		 So the new probabilities are *not* the old +/- 10% but
		 instead the old +/- 5%.
		 */
		A *= 0.95 + rn[rni++]/10;
		B *= 0.95 + rn[rni++]/10;
		C *= 0.95 + rn[rni++]/10;
		D *= 0.95 + rn[rni++]/10;
		/* Used 5 random numbers. */
		
		{
			const double norm = 1.0 / (A + B + C + D);
			A *= norm; B *= norm; C *= norm;
		}
		/* So long as +/- are monotonic, ensure a+b+c+d <= 1.0 */
		D = 1.0 - (A + B + C);
		
		bit >>= 1;
	}
	/* Iterates SCALE times. */
	write_edge(out, i, j);
}

struct random_edges_functor : public ForkJoinIteration {
  GlobalAddress<packed_edge> ij;
  GlobalAddress<int64_t> iwork;
  int64_t nedge;
  mrg_state prng_state;
  int SCALE;
  void operator()(thread * me, int64_t index) {
    double * restrict Rlocal = (double*)alloca(NRAND(1) * sizeof(double));
    mrg_state new_st = prng_state;
    mrg_skip(&new_st, 1, NRAND(1), 0);
    for (int64_t i; i < NRAND(1); i++) {
      Rlocal[i] = mrg_get_double_orig(&new_st);
    }
    packed_edge new_edge;
    rmat_edge(&new_edge, SCALE, A, B, C, D, Rlocal);
    
    Incoherent<packed_edge>::RW cedge(ij+index, 1);
    *cedge = new_edge;
  }
};

static void rmat_edgelist(tuple_graph* grin, int SCALE) {
  GlobalAddress<int64_t> iwork = SoftXMT_typed_malloc<int64_t>(1L<<SCALE);
  
  random_edges_functor f;
  f.iwork = iwork;
  f.nedge = grin->nedge;
  f.prng_state = prng_state_store;
  f.SCALE = SCALE;
  fork_join(current_thread, &f, 0, grin->nedge);
  
  mrg_skip(&prng_state_store, 1, NRAND(1), 0);
  
  
}

int main(int argc, char** argv) {
  SoftXMT_init(&argc, &argv, 1<<(MAX_SCALE+2));
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
  assert(SCALE <= MAX_SCALE);
  uint64_t seed1 = 2, seed2 = 3;

  tuple_graph tg;
  tg.nedge = (int64_t)(edgefactor) << SCALE;
  int64_t nglobalverts = (int64_t)(1) << SCALE;

  /* Make the raw graph edges. */
  /* Get roots for BFS runs, plus maximum vertex with non-zero degree (used by
   * validator). */
  int num_bfs_roots = 64;
//  int64_t* bfs_roots = (int64_t*)xmalloc(num_bfs_roots * sizeof(int64_t));
  GlobalAddress<int64_t> bfs_roots = SoftXMT_typed_malloc<int64_t>(num_bfs_roots);
  int64_t max_used_vertex = 0;

  double start, stop;
  start = timer();
  {
    /* Spread the two 64-bit numbers into five nonzero values in the correct
     * range. */
    uint_fast32_t seed[5];
    make_mrg_seed(seed1, seed2, seed);
    mrg_seed(&prng_state_store, seed);

    /* As the graph is being generated, also keep a bitmap of vertices with
     * incident edges.  We keep a grid of processes, each row of which has a
     * separate copy of the bitmap (distributed among the processes in the
     * row), and then do an allreduce at the end.  This scheme is used to avoid
     * non-local communication and reading the file separately just to find BFS
     * roots. */
    tg.edges = SoftXMT_typed_malloc<packed_edge>(tg.nedge);
    
    rmat_edgelist(&tg, SCALE);
    
    //TODO
  }
  stop = timer();
  double make_graph_time = stop - start;
  if (rank == 0) { /* Not an official part of the results */
    fprintf(stderr, "graph_generation:               %f s\n", make_graph_time);
  }

  SoftXMT_free(bfs_roots);
  free_graph_data_structure();

  SoftXMT_free(tg.edges);

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

