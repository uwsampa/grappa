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
      LOG(INFO) << "Cannot find any sample roots! Exiting...";
      exit(1);
    }
  }
}

struct func_set_const : public ForkJoinIteration {
  GlobalAddress<int64_t> base_addr;
  int64_t value;
  void operator()(thread * me, int64_t index) {
    DVLOG(3) << "called func_initialize with index = " << index;
    Incoherent<int64_t>::RW c(base_addr+index, 1);
    c[0] = value;
  }
};

struct func_bfs_onelevel : public ForkJoinIteration {
  GlobalAddress<int64_t> vlist;
  GlobalAddress<int64_t> xoff;
  GlobalAddress<int64_t> xadj;
  GlobalAddress<int64_t> bfs_tree;
  GlobalAddress<int64_t> k2;
  void operator()(thread * me, int64_t k) {
    const int64_t v = SoftXMT_delegate_read_word(vlist+k);
    
    // TODO: do these two together (cache)
    const int64_t vstart = SoftXMT_delegate_read_word(XOFF(v));
    const int64_t vend = SoftXMT_delegate_read_word(XENDOFF(v));
    
    for (int64_t vo = vstart; vo < vend; vo++) {
      const int64_t j = SoftXMT_delegate_read_word(xadj+vo);
      if (SoftXMT_delegate_compare_and_swap_word(bfs_tree+j, -1, v)) {
        int64_t voff = SoftXMT_delegate_fetch_and_add_word(k2, 1);
        SoftXMT_delegate_write_word(vlist+voff, j);
      }
    }
  }
};

static void make_bfs_tree(csr_graph * g, GlobalAddress<int64_t> bfs_tree, int64_t root) {
  int64_t NV = g->nv;
  GlobalAddress<int64_t> vlist = SoftXMT_typed_malloc<int64_t>(NV);
  
  
  // start with root as only thing in vlist
  SoftXMT_delegate_write_word(vlist, root);
  
  int64_t k1 = 0, k2 = 1;
  GlobalAddress<int64_t> k1addr = make_global(&k1);
  GlobalAddress<int64_t> k2addr = make_global(&k2);
  
  // initialize bfs_tree to -1
  func_set_const fc;
  fc.base_addr = bfs_tree;
  fc.value = -1;
  fork_join(current_thread, &fc, 0, NV);
  
  SoftXMT_delegate_write_word(bfs_tree+root, root); // parent of root is self
    
  while (k1 != k2) {
    const int64_t oldk2 = k2;
    
    func_bfs_onelevel fb;
      fb.vlist = vlist;
      fb.xoff = g->xoff;
      fb.xadj = g->xadj;
      fb.bfs_tree = bfs_tree;
      fb.k2 = k2addr;
    fork_join(get_current_thread(), &fb, k1, oldk2);
    
    k1 = oldk2;
  }
  
  SoftXMT_free(vlist);
}

static void compute_levels(GlobalAddress<int64_t> level, int64_t nv, GlobalAddress<int64_t> bfs_tree, int64_t root) {
  
  // Incoherent::RW c_level(level, nv);
  for (int64_t k=0; k < nv; k++) {
    SoftXMT_delegate_write_word(level+k, (k == root) ? 0 : -1);
  }
  
  for (int64_t k=0; k < nv; k++) {
    int64_t level_k = SoftXMT_delegate_read_word(level+k);
    if (level_k >= 0) continue;
    
    int64_t tree_k = SoftXMT_delegate_read_word(bfs_tree+k);
    if (tree_k >= 0 && k != root) {
      int64_t parent = k;
      int64_t nhop = 0;
      int64_t next_parent;
      
      /* Run up the three until we encounter an already-leveled vertex. */
      while (parent >= 0 && SoftXMT_delegate_read_word(level+parent) < 0 && nhop < nv) {
        next_parent = SoftXMT_delegate_read_word(bfs_tree+parent);
        assert(parent != next_parent);
        parent = next_parent;
        ++nhop;
      }
      assert(nhop < nv); // no cycles
      if (nhop >= nv) { LOG(INFO) << "Error: root " << k << " had a cycle."; }
      assert(parent >= 0); // did not run off the end
      if (parent < 0) { LOG(INFO) << "Error: ran off the end for root " << k << "."; }
      
      // Now assign levels until we meet an already-leveled vertex
      // NOTE: This permits benign races if parallelized.
      nhop += SoftXMT_delegate_read_word(level+parent);
      parent = k;
      while (SoftXMT_delegate_read_word(level+parent) < 0) {
        assert(nhop > 0);
        SoftXMT_delegate_write_word(level+parent, nhop);
        nhop--;
        parent = SoftXMT_delegate_read_word(bfs_tree+parent);
      }
      assert(nhop == SoftXMT_delegate_read_word(level+parent));
      
      // Internal check to catch mistakes in races...
#if defined(DEBUG)
      nhop = 0;
      parent = k;
      int64_t lastlvl = SoftXMT_delegate_read_word(level+k) + 1;
      while ((next_parent = SoftXMT_delegate_read_word(level+parent)) > 0) {
        assert(lastlvl == (1+next_parent));
        lastlvl = next_parent;
        parent = SoftXMT_delegate_read_word(bfs_tree+parent);
        nhop++;
      }
#endif
    }
  }
}

static int64_t verify_bfs_tree(GlobalAddress<int64_t> bfs_tree, int64_t max_bfsvtx, int64_t root, tuple_graph * tg) {
  assert(SoftXMT_delegate_read_word(bfs_tree+root) == root);
  
  int64_t nedge_traversed = 0;
  int64_t nv = max_bfsvtx+1;
  int64_t terr = 0;
  
  GlobalAddress<int64_t> seen_edge = SoftXMT_typed_malloc<int64_t>(nv);
  GlobalAddress<int64_t> level = SoftXMT_typed_malloc<int64_t>(nv);
  
  compute_levels(level, nv, bfs_tree, root);
  
  for (int64_t k=0; k < nv; k++) {
    SoftXMT_delegate_write_word(seen_edge+k, 0);
  }
  
  for (int64_t k = 0; k < tg->nedge; k++) {
    Incoherent<packed_edge>::RO cedge(tg->edges+k, 1);
    const int64_t i = cedge[0].v0;
    const int64_t j = cedge[0].v1;
    int64_t lvldiff;
    
    if (i < 0 || j < 0) continue;
    if (i > max_bfsvtx && j <= max_bfsvtx) {
      terr = -10;
      LOG(INFO) << "Error!";
      continue;
    }
    if (j > max_bfsvtx && i <= max_bfsvtx) {
      terr = -11;
      LOG(INFO) << "Error!";
      continue;
    }
    if (i > max_bfsvtx) // both i & j are on the same side of max_bfsvtx
      continue;
    
    // All neighbors must be in the tree.
    int64_t ti = SoftXMT_delegate_read_word(bfs_tree+i);
    int64_t tj = SoftXMT_delegate_read_word(bfs_tree+j);
    
    if (ti >= 0 && tj < 0) { terr = -12; LOG(INFO) << "Error!"; continue; }
    if (tj >= 0 && ti < 0) { terr = -13; LOG(INFO) << "Error!"; continue; }
    if (ti < 0) // both i & j have the same sign
      continue;
    
    /* Both i and j are in the tree, count as a traversed edge.
     
     NOTE: This counts self-edges and repeated edges.  They're
     part of the input data.
     */
    ++nedge_traversed;
    // Mark seen tree edges.
    if (i != j) {
      if (ti == j)
        SoftXMT_delegate_write_word(seen_edge+i, 1);
      if (tj == i)
        SoftXMT_delegate_write_word(seen_edge+j, 1);
    }
    lvldiff = SoftXMT_delegate_read_word(level+i) - SoftXMT_delegate_read_word(level+j);
    /* Check that the levels differ by no more than one. */
    if (lvldiff > 1 || lvldiff < -1) {
      terr = -14;
      LOG(INFO) << "Error, levels differ by more than one!";
    }    
  }
  
  if (!terr) {
    /* Check that every BFS edge was seen and that there's only one root. */
    OMP("omp for") MTA("mta assert parallel") MTA("mta use 100 streams")
    for (int64_t k = 0; k < nv; ++k) {
      if (!terr && k != root) {
        int64_t tk = SoftXMT_delegate_read_word(bfs_tree+k);
        if (tk >= 0 && !SoftXMT_delegate_read_word(seen_edge+k)) {
          terr = -15;
          LOG(INFO) << "Error!";
        }
        if (tk == k) {
          terr = -16;
          LOG(INFO) << "Error!";
        }
      }
    }
  }
  
  SoftXMT_free(seen_edge);
  SoftXMT_free(level);
  
  if (terr) {
    return terr;
  } else {
    return nedge_traversed;
  }
}

static void run_bfs(tuple_graph * tg) {
  csr_graph g;
  
  TIME(construction_time,
       create_graph_from_edgelist(tg, &g)
  );
  LOG(INFO) << "construction_time = " << construction_time;
  
  GlobalAddress<int64_t> xoff = g.xoff;
//  for (int64_t i=0; i < g.nv; i++) {
//    VLOG(1) << "xoff[" << i << "] = " << SoftXMT_delegate_read_word(XOFF(i)) << " -> " << SoftXMT_delegate_read_word(XENDOFF(i));
//  }  
  for (int64_t i=0; i<g.nv; i++) {
    std::stringstream ss;
    int64_t xoi = SoftXMT_delegate_read_word(XOFF(i)), xei = SoftXMT_delegate_read_word(XENDOFF(i));
    ss << "xoff[" << i << "] = " << xoi << "->" << xei << ": (";
    for (int64_t j=xoi; j<xei; j++) {
      ss << SoftXMT_delegate_read_word(g.xadj+j) << ",";
    }
    ss << ")";
    VLOG(1) << ss.str();
  }
  
  // no rootname input method, so randomly choose
  int64_t bfs_roots[NBFS_max];
  int64_t nbfs = NBFS_max;
  choose_bfs_roots(g.xoff, g.nv, &nbfs, bfs_roots);
  
//  for (int64_t i=0; i < nbfs; i++) {
//    VLOG(1) << "bfs_roots[" << i << "] = " << bfs_roots[i];
//  }
  
  // build bfs tree for each root
  for (int64_t i=0; i < nbfs; i++) {
    GlobalAddress<int64_t> bfs_tree = SoftXMT_typed_malloc<int64_t>(g.nv);
    GlobalAddress<int64_t> max_bfsvtx;
    
    VLOG(1) << "Running bfs on root " << bfs_roots[i] << "...";
    TIME(bfs_time[i],
      make_bfs_tree(&g, bfs_tree, bfs_roots[i])
    );
//    VLOG(1) << "done";
//    for (int64_t i=0; i < g.nv; i++) {
//      VLOG(1) << "bfs_tree[" << i << "] = " << SoftXMT_delegate_read_word(bfs_tree+i);
//    }
    
    VLOG(1) << "Verifying bfs " << i << "...";
    bfs_nedge[i] = verify_bfs_tree(bfs_tree, g.nv-1, bfs_roots[i], tg);
//    VLOG(1) << "done";
    
    if (bfs_nedge[i] < 0) {
      LOG(INFO) << "bfs " << i << " from root " << bfs_roots[i] << " failed verification: " << bfs_nedge[i];
      exit(1);
    } else {
      VLOG(1) << "bfs_time[" << i << "] = " << bfs_time[i];
    }
    
    SoftXMT_free(bfs_tree);
  }
}

static void user_main(thread * me, void * args) {    
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
  double make_graph_time = stop - start;
  fprintf(stderr, "graph_generation:               %f s\n", make_graph_time);
  
  run_bfs(&tg);
  
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

