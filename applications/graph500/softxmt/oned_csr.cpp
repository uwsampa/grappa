/* Copyright (C) 2010-2011 The Trustees of Indiana University.             */
/*                                                                         */
/* Use, modification and distribution is subject to the Boost Software     */
/* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at */
/* http://www.boost.org/LICENSE_1_0.txt)                                   */
/*                                                                         */
/*  Authors: Jeremiah Willcock                                             */
/*           Andrew Lumsdaine                                              */

#include "common.h"
#include "oned_csr.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "SoftXMT.hpp"
#include "GlobalAllocator.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"
#include "Collective.hpp"
#include "Delegate.hpp"

#define MINVECT_SIZE 2

#define XOFF(k) (xoff+2*(k))
#define XENDOFF(k) (xoff+1+2*(k))

static int64_t maxvtx, nv, sz;

//struct temp_graph_data {
//  GlobalAddress<int64_t> xoff, xadjstore, xadj;
//  int64_t nv;
//};

struct func_set_const : public ForkJoinIteration {
  GlobalAddress<int64_t> base_addr;
  int64_t value;
  void operator()(thread * me, int64_t index) {
    DVLOG(1) << "called func_initialize with index = " << index;
    Incoherent<int64_t>::RW c(base_addr+index, 1);
    c[0] = value;
  }
};

struct max_func : public ForkJoinIteration {
  GlobalAddress<packed_edge> edges;
  int64_t * max;
  void operator()(thread * me, int64_t index) {
    Incoherent<packed_edge>::RO cedge(edges+index, 1);
    if (cedge[0].v0 > *max) {
      *max = cedge[0].v0;
    }
    if (cedge[0].v1 > *max) {
      *max = cedge[0].v1;
    }
  }
};

struct node_max_func : public ForkJoinIteration {
  GlobalAddress<packed_edge> edges;
  int64_t start, end;
  int64_t max;
  void operator()(thread * me, int64_t mynode) {
    max = -1;
    range_t myblock = blockDist(start, end, mynode, SoftXMT_nodes());
    max_func f;
    f.edges = edges;
    f.max = &max;
    fork_join_onenode(me, &f, myblock.start, myblock.end);
    
    maxvtx = SoftXMT_collective_reduce(&collective_max, 0, max, -1);
    nv = maxvtx+1;
  }
};

static void find_nv(const tuple_graph* const tg) {
  node_max_func f;
  f.edges = tg->edges;
  f.start = 0;
  f.end = tg->nedge;
  fork_join_custom(current_thread, &f);
}

// note: this corresponds to how Graph500 counts 'degree' (both in- and outgoing edges to each vertex)
struct degree_func : public ForkJoinIteration {
  GlobalAddress<packed_edge> edges;
  GlobalAddress<int64_t> xoff;
  void operator()(thread * me, int64_t index) {
    Incoherent<packed_edge>::RO cedge(edges+index, 1);
    int64_t i = cedge[0].v0;
    int64_t j = cedge[0].v1;
    if (i != j) { //skip self-edges
      SoftXMT_delegate_fetch_and_add_word(XOFF(i), 1);
      SoftXMT_delegate_fetch_and_add_word(XOFF(j), 1);
    }
  }
};

struct local_reduce_sum : public ForkJoinIteration {
  GlobalAddress<int64_t> xoff;
  int64_t * sum;
  void operator()(thread * me, int64_t index) {
    int64_t v = SoftXMT_delegate_read_word(XOFF(index));
    *sum += v;
  }
};

struct prefix_sum_node : public ForkJoinIteration {
  GlobalAddress<int64_t> xoff;
  GlobalAddress<int64_t> buf; // one per node, used to store local total
  int64_t nv;
  void operator()(thread * me, int64_t nid) {
    range_t myblock = blockDist(0, nv, nid, SoftXMT_nodes());
    
    int64_t mysum;
    local_reduce_sum fr;
    fr.xoff = xoff;
    fr.sum = &mysum;
    fork_join_onenode(me, &fr, myblock.start, myblock.end);
    
    // add my sum to every buf count following this node
    // TODO: do this in a tree rather than brute-force push-to-all
    for (int64_t i=nid; i<SoftXMT_nodes(); i++) {
      SoftXMT_delegate_fetch_and_add_word(buf+i, mysum);
    }
    
    SoftXMT_barrier_commsafe();
    
    int64_t prev_sum = (nid > 0) ? SoftXMT_delegate_read_word(buf+nid-1) : 0;
    
    for (int64_t i=myblock.start; i<myblock.end; i++) {
      int64_t tmp = SoftXMT_delegate_read_word(XOFF(i));
      SoftXMT_delegate_write_word(XOFF(i), prev_sum);
      prev_sum += tmp;
    }
  }
};

static int64_t prefix_sum(GlobalAddress<int64_t> xoff, int64_t nv) {
  prefix_sum_node fps;
  fps.xoff = xoff;
  fps.buf = SoftXMT_typed_malloc<int64_t>(SoftXMT_nodes());
  fps.nv = nv;
  fork_join_custom(current_thread, &fps);
  
  return SoftXMT_delegate_read_word(fps.buf+SoftXMT_nodes()-1);
}

struct minvect_func : public ForkJoinIteration {
  GlobalAddress<int64_t> xoff;
  void operator()(thread * me, int64_t index) {
    int64_t v = SoftXMT_delegate_read_word(XOFF(index));
    if (v < MINVECT_SIZE) {
      SoftXMT_delegate_write_word(XOFF(index), MINVECT_SIZE);
    }
  }
};

struct init_xendoff_func : public ForkJoinIteration {
  GlobalAddress<int64_t> xoff;
  void operator()(thread * me, int64_t index) {
    SoftXMT_delegate_write_word(XENDOFF(index), SoftXMT_delegate_read_word(XOFF(index)));
  }
};

static void setup_deg_off(const tuple_graph * const tg, csr_graph * g) {
  GlobalAddress<int64_t> xoff = g->xoff;
  // initialize xoff to 0
  func_set_const fc; fc.value = 0; fc.base_addr = g->xoff;
  fork_join(current_thread, &fc, 0, 2*g->nv+2);
  
  // count occurrences of each vertex in edges
  degree_func fd; fd.edges = tg->edges; fd.xoff = g->xoff;
  fork_join(current_thread, &fd, 0, tg->nedge);
  
  for (int64_t i=0; i<g->nv; i++) {
    VLOG(1) << "degree[" << i << "] = " << SoftXMT_delegate_read_word(XOFF(i));
  }
  
  // make sure every degree is at least MINVECT_SIZE (don't know why yet...)
  minvect_func fm; fm.xoff = g->xoff;
  fork_join(current_thread, &fm, 0, g->nv);
  
  // simple parallel prefix sum to compute offsets from degrees
  int64_t accum = prefix_sum(g->xoff, g->nv);
  
  VLOG(1) << "accum = " << accum;
  for (int64_t i=0; i<g->nv; i++) {
    VLOG(1) << "offset[" << i << "] = " << SoftXMT_delegate_read_word(XOFF(i));
  }
  
  //initialize XENDOFF to be the same as XOFF
  init_xendoff_func fe; fe.xoff = g->xoff;
  fork_join(current_thread, &fe, 0, g->nv);
  
  SoftXMT_delegate_write_word(XOFF(g->nv), accum);
  
  g->xadjstore = SoftXMT_typed_malloc<int64_t>(accum + MINVECT_SIZE);
  g->xadj = g->xadjstore+MINVECT_SIZE; // cheat and permit xadj[-1] to work
  // func set const = fc
  fc.value = -1; fc.base_addr = g->xadjstore;
  fork_join(current_thread, &fc, 0, accum+MINVECT_SIZE);
}

inline void scatter_edge(GlobalAddress<int64_t> xoff, GlobalAddress<int64_t> xadj, const int64_t i, const int64_t j) {
  int64_t where = SoftXMT_delegate_fetch_and_add_word(XENDOFF(i), 1);
  SoftXMT_delegate_write_word(xadj+where, j);
}

struct scatter_func : public ForkJoinIteration {
  GlobalAddress<packed_edge> ij;
  GlobalAddress<int64_t> xoff;
  GlobalAddress<int64_t> xadj;
  void operator()(thread * me, int64_t k) {
    Incoherent<packed_edge>::RO cedge(ij+k, 1);
    int64_t i = cedge[0].v0, j = cedge[0].v1;
    if (i >= 0 && j >= 0 && i != j) {
      scatter_edge(xoff, xadj, i, j);
      scatter_edge(xoff, xadj, j, i);
    }
  }
};

static int
i64cmp (const void *a, const void *b)
{
	const int64_t ia = *(const int64_t*)a;
	const int64_t ib = *(const int64_t*)b;
	if (ia < ib) return -1;
	if (ia > ib) return 1;
	return 0;
}

// TODO: with power-law graphs, this could be a problem (everyone waiting for one big one to finish). Consider breaking up this operation into smaller chunks of work (maybe once we have work-stealing working).
struct pack_vtx_edges_func : public ForkJoinIteration {
  GlobalAddress<int64_t> xoff;
  GlobalAddress<int64_t> xadj;
  void operator()(thread * me, int64_t i) {
    int64_t kcur;
//    Incoherent<int64_t>::RO xoi(XOFF(i), 1);
//    Incoherent<int64_t>::RO xei(XENDOFF(i), 1);
    int64_t xoi = SoftXMT_delegate_read_word(XOFF(i));
    int64_t xei = SoftXMT_delegate_read_word(XENDOFF(i));
    if (xoi+1 >= xei) return;
    
    int64_t * buf = (int64_t*)alloca(xei-xoi*sizeof(int64_t));
    Incoherent<int64_t>::RW cadj(xadj+xoi, xei-xoi, buf);
    qsort(buf, xei-xoi, sizeof(int64_t), i64cmp);
    kcur = 0;
    for (int64_t k = 1; k < xei-xoi; k++) {
      if (buf[k] != buf[kcur]) {
        buf[++kcur] = buf[k];
      }
    }
    ++kcur;
    for (int64_t k = kcur; k < xei-xoi; k++) {
      buf[k] = -1;
    }
    SoftXMT_delegate_write_word(XENDOFF(i), xoi+kcur);
  }
};

static void gather_edges(const tuple_graph * const tg, csr_graph * g) {
//  for (k = 0; k < nedge; ++k) {
//    int64_t i = get_v0_from_edge(&IJ[k]);
//    int64_t j = get_v1_from_edge(&IJ[k]);
//    if (i >= 0 && j >= 0 && i != j) {
//      scatter_edge (i, j);
//      scatter_edge (j, i);
//    }
//  }
  scatter_func sf;
  sf.ij = tg->edges;
  sf.xoff = g->xoff;
  sf.xadj = g->xadj;
  fork_join(current_thread, &sf, 0, tg->nedge);
  
//  pack_edges():
//    for (v = 0; v < nv; ++v)
//      pack_vtx_edges (v);
  pack_vtx_edges_func pf;
  pf.xoff = g->xoff;
  pf.xadj = g->xadj;
  fork_join(current_thread, &pf, 0, g->nv);
}

void convert_graph_to_oned_csr(const tuple_graph* const tg, csr_graph* const g) {    
  find_nv(tg);
  LOG(INFO) << "nv = " << nv;
  
  g->nv = nv;
  g->xoff = SoftXMT_typed_malloc<int64_t>(2*nv+2);
  
  setup_deg_off(tg, g);
  
  gather_edges(tg, g);
}

void free_oned_csr_graph(csr_graph* const g) {
  SoftXMT_free(g->xoff);
  SoftXMT_free(g->xadjstore);
}

