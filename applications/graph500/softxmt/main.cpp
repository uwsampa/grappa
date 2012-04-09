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
#define NBFS_max 2
#define MAX_SCALE 20

int SCALE;
int edgefactor;
int64_t nbfs;

//static int64_t bfs_root[NBFS_max];

static double generation_time;
static double construction_time;
static double bfs_time[NBFS_max];
static int64_t bfs_nedge[NBFS_max];

int lgsize;

#define XOFF(k) (xoff+2*(k))
#define XENDOFF(k) (xoff+2*(k)+1)

inline bool has_adj(GlobalAddress<int64_t> xoff, int64_t i) {
  int64_t xoi = SoftXMT_delegate_read_word(XOFF(i));
  int64_t xei = SoftXMT_delegate_read_word(XENDOFF(i));
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

#define BUF_LEN 16384

struct func_bfs_onelevel : public ForkJoinIteration {
  GlobalAddress<int64_t> vlist;
  GlobalAddress<int64_t> xoff;
  GlobalAddress<int64_t> xadj;
  GlobalAddress<int64_t> bfs_tree;
  GlobalAddress<int64_t> k2;
  int64_t * kbuf;
  int64_t * buf;
  int64_t nadj;
  void operator()(int64_t k) {
    const int64_t v = SoftXMT_delegate_read_word(vlist+k);
    
    // TODO: do these two together (cache)
    const int64_t vstart = SoftXMT_delegate_read_word(XOFF(v));
    const int64_t vend = SoftXMT_delegate_read_word(XENDOFF(v));
    CHECK(vstart < nadj) << vstart << " < " << nadj;
    CHECK(vend < nadj) << vend << " < " << nadj;

    for (int64_t vo = vstart; vo < vend; vo++) {
      const int64_t j = SoftXMT_delegate_read_word(xadj+vo); // cadj[vo];
      if (SoftXMT_delegate_compare_and_swap_word(bfs_tree+j, -1, v)) {
        while (*kbuf == -1) { SoftXMT_yield(); }
        if (*kbuf < BUF_LEN) {
          buf[*kbuf] = j;
          (*kbuf)++;
        } else {
          *kbuf = -1; // lock other threads out temporarily
          int64_t voff = SoftXMT_delegate_fetch_and_add_word(k2, BUF_LEN);
          Incoherent<int64_t>::RW cvlist(vlist+voff, BUF_LEN);
          for (int64_t vk=0; vk < BUF_LEN; vk++) {
            cvlist[vk] = buf[vk];
          }
          buf[0] = j;
          *kbuf = 1;
        }
      }
    }
  }
};

struct func_bfs_node : public ForkJoinIteration {
  GlobalAddress<int64_t> vlist;
  GlobalAddress<int64_t> xoff;
  GlobalAddress<int64_t> xadj;
  GlobalAddress<int64_t> bfs_tree;
  GlobalAddress<int64_t> k2;
  int64_t start, end;
  int64_t nadj; // TODO: DEBUG only...
  void operator()(int64_t mynode) {
    int64_t kbuf;
    int64_t buf[BUF_LEN];
    
    range_t r = blockDist(start, end, mynode, SoftXMT_nodes());
    
    func_bfs_onelevel f;
    f.vlist = vlist; f.xoff = xoff; f.xadj = xadj; f.bfs_tree = bfs_tree; f.k2 = k2;
    f.kbuf = &kbuf;
    f.buf = buf;
    f.nadj = nadj;
//    VLOG(1) << "on node";
    fork_join_onenode(&f, r.start, r.end);
    
    // make sure to commit what's left in the buffer at the end
    if (kbuf) {
      int64_t voff = SoftXMT_delegate_fetch_and_add_word(k2, kbuf);
      Incoherent<int64_t>::RW cvlist(vlist+voff, kbuf);
      for (int64_t vk=0; vk < kbuf; vk++) {
        cvlist[vk] = buf[vk];
      }
    }
  }
};

static double make_bfs_tree(csr_graph * g, GlobalAddress<int64_t> bfs_tree, int64_t root) {
  int64_t NV = g->nv;
  GlobalAddress<int64_t> vlist = SoftXMT_typed_malloc<int64_t>(NV);
  
//  VLOG(1) << "actually starting...";
  double start, stop;
  start = timer();
  
  // start with root as only thing in vlist
  SoftXMT_delegate_write_word(vlist, root);
  
  int64_t k1 = 0, k2 = 1;
  GlobalAddress<int64_t> k1addr = make_global(&k1);
  GlobalAddress<int64_t> k2addr = make_global(&k2);
  
  // initialize bfs_tree to -1
  func_set_const fc;
  fc.base_addr = bfs_tree;
  fc.value = -1;
  fork_join(&fc, 0, NV);
  
  SoftXMT_delegate_write_word(bfs_tree+root, root); // parent of root is self
  
  func_bfs_node fb;
  fb.vlist = vlist;
  fb.xoff = g->xoff;
  fb.xadj = g->xadj;
  fb.bfs_tree = bfs_tree;
  fb.k2 = k2addr;
  fb.nadj = g->nadj; //DEBUG only...
  
  while (k1 != k2) {
    VLOG(2) << "k1=" << k1 << ", k2=" << k2;
    const int64_t oldk2 = k2;
    
    fb.start = k1;
    fb.end = oldk2;

    fork_join_custom(&fb);
    
    k1 = oldk2;
  }
  
  stop = timer();
  
  SoftXMT_free(vlist);
  
  return stop-start;
}

static void run_bfs(tuple_graph * tg) {
  csr_graph g;
  
  TIME(construction_time,
       create_graph_from_edgelist(tg, &g)
  );
  VLOG(1) << "construction_time = " << construction_time;
  
  GlobalAddress<int64_t> xoff = g.xoff;
//  for (int64_t i=0; i < g.nv; i++) {
//    VLOG(1) << "xoff[" << i << "] = " << SoftXMT_delegate_read_word(XOFF(i)) << " -> " << SoftXMT_delegate_read_word(XENDOFF(i));
//  }
//#ifdef DEBUG
//  for (int64_t i=0; i<g.nv; i++) {
//    std::stringstream ss;
//    int64_t xoi = SoftXMT_delegate_read_word(XOFF(i)), xei = SoftXMT_delegate_read_word(XENDOFF(i));
//    CHECK(xoi <= g.nadj) << i << ": " << xoi << " > " << g.nadj;
//    CHECK(xei <= g.nadj) << i << ": " << xei << " > " << g.nadj;
//    ss << "xoff[" << i << "] = " << xoi << "->" << xei << ": (";
//    for (int64_t j=xoi; j<xei; j++) {
//      ss << SoftXMT_delegate_read_word(g.xadj+j) << ",";
//    }
//    ss << ")";
//    VLOG(2) << ss.str();
//  }
//#endif
  
  // no rootname input method, so randomly choose
  int64_t bfs_roots[NBFS_max];
  nbfs = NBFS_max;
  choose_bfs_roots(g.xoff, g.nv, &nbfs, bfs_roots);
  
//  for (int64_t i=0; i < nbfs; i++) {
//    VLOG(1) << "bfs_roots[" << i << "] = " << bfs_roots[i];
//  }
  
  // build bfs tree for each root
  for (int64_t i=0; i < nbfs; i++) {
    GlobalAddress<int64_t> bfs_tree = SoftXMT_typed_malloc<int64_t>(g.nv);
    GlobalAddress<int64_t> max_bfsvtx;
    
    VLOG(1) << "Running bfs on root " << i << "(" << bfs_roots[i] << ")...";
    bfs_time[i] = make_bfs_tree(&g, bfs_tree, bfs_roots[i]);
//    VLOG(1) << "done";
//    for (int64_t i=0; i < g.nv; i++) {
//      VLOG(1) << "bfs_tree[" << i << "] = " << SoftXMT_delegate_read_word(bfs_tree+i);
//    }
    
//    std::stringstream ss;
//    ss << "bfs_tree[" << i << "] = ";
//    for (int64_t k=0; k<g.nv; k++) {
//      ss << SoftXMT_delegate_read_word(bfs_tree+k) << ",";
//    }
//    VLOG(1) << ss.str();
    
    VLOG(1) << "Verifying bfs " << i << "...";
    bfs_nedge[i] = verify_bfs_tree(bfs_tree, g.nv-1, bfs_roots[i], tg);
//    VLOG(1) << "done";
    
    if (bfs_nedge[i] < 0) {
      LOG(ERROR) << "bfs " << i << " from root " << bfs_roots[i] << " failed verification: " << bfs_nedge[i];
      exit(1);
    } else {
      VLOG(1) << "bfs_time[" << i << "] = " << bfs_time[i];
    }
    
    SoftXMT_free(bfs_tree);
  }
}

static void user_main(Thread * me, void * args) {    
  tuple_graph tg;
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
  
  run_bfs(&tg);
  
//  free_graph_data_structure();
  
  SoftXMT_free(tg.edges);
  
  /* Print results. */
  output_results(SCALE, 1<<SCALE, edgefactor, A, B, C, D, generation_time, construction_time, (int)nbfs, bfs_time, bfs_nedge);

  SoftXMT_signal_done();
}

int main(int argc, char** argv) {
  SoftXMT_init(&argc, &argv, 1<<(MAX_SCALE+8));
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
