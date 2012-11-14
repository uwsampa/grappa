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

#include "Grappa.hpp"
#include "GlobalAllocator.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"
#include "Collective.hpp"
#include "Delegate.hpp"
#include "GlobalTaskJoiner.hpp"

#include "timer.h"

#include <sstream>

#define MINVECT_SIZE 2

#define XOFF(k) (xoff+2*(k))
#define XENDOFF(k) (xoff+1+2*(k))

static int64_t maxvtx, nv;

LOOP_FUNCTOR( max_func, index,
             (( GlobalAddress<packed_edge>,edges))
             ((int64_t *, max)) )
{
  Incoherent<packed_edge>::RO cedge(edges+index, 1);
  if (cedge[0].v0 > *max) {
    *max = cedge[0].v0;
  }
  if (cedge[0].v1 > *max) {
    *max = cedge[0].v1;
  }
}

LOOP_FUNCTOR( node_max_func, mynode, 
  ((GlobalAddress<packed_edge>, edges))
  ((int64_t,start)) (( int64_t,end )) ((int64_t,init_max)) )
{
  int64_t max = init_max;
  range_t myblock = blockDist(start, end, mynode, Grappa_nodes());
  max_func f;
  f.edges = edges;
  f.max = &max;
  fork_join_onenode(&f, myblock.start, myblock.end);
  
  maxvtx = Grappa_collective_reduce(&collective_max, 0, max, -1);
  nv = maxvtx+1;
}

static void find_nv(const tuple_graph* const tg) {
  node_max_func f(tg->edges, 0, tg->nedge, 0);
  fork_join_custom(&f);
}

// note: this corresponds to how Graph500 counts 'degree' (both in- and outgoing edges to each vertex)
LOOP_FUNCTOR( degree_func, index, (( GlobalAddress<packed_edge>, edges )) ((GlobalAddress<int64_t>,xoff)) )
{ 
  Incoherent<packed_edge>::RO cedge(edges+index, 1);
  int64_t i = cedge[0].v0;
  int64_t j = cedge[0].v1;
  if (i != j) { //skip self-edges
    // TODO: these should be overlapping
    Grappa_delegate_fetch_and_add_word(XOFF(i), 1);
    Grappa_delegate_fetch_and_add_word(XOFF(j), 1);
  }
}

LOOP_FUNCTOR( local_reduce_sum, index,
    (( GlobalAddress<int64_t>, xoff )) (( int64_t *, sum )) )
{
  int64_t v = Grappa_delegate_read_word(XOFF(index));
  *sum += v;
}

#define minint(A,B) ((A)<(B)) ? (A) : (B)
int64_t local_prefix;
// ideally this would be forall local with contiguous addresses
LOOP_FUNCTOR( prefix_sum_node, nid,
    ((GlobalAddress<int64_t>,xoff))
    ((int64_t,nv)) ) {
  range_t myblock = blockDist(0, nv, nid, Grappa_nodes());
  
  local_prefix = 0;
  local_reduce_sum fr;
  fr.xoff = xoff;
  fr.sum = &local_prefix;
  fork_join_onenode(&fr, myblock.start, myblock.end);
  
  Grappa_barrier_commsafe(); // TODO: replace with fjs

  // node 0 sets buf[i] to prefix sum
  // TODO: do this as parallel prefix
  if (nid == 0) {
    int64_t total = local_prefix;
    for (int64_t i=1; i<Grappa_nodes(); i++) {
      //LOG(INFO) << "will add total=" << total << " to node " << i;
      total += Grappa_delegate_fetch_and_add_word(make_global(&local_prefix,i), total);
    }
  }
  
  Grappa_barrier_commsafe();
  
  int64_t prev_sum = (nid > 0) ? Grappa_delegate_read_word(make_global(&local_prefix,nid-1)) : 0;
  //LOG(INFO) << "prev_sum= " << prev_sum;
 
  // do prefix sum within the slice 
  // Cache a part at a time, doing the sum locally and writing back
  size_t bufsize = 1024;
  int64_t * buf = new int64_t[2*bufsize];
  for (int64_t i=myblock.start; i<myblock.end; i+=bufsize) {
    size_t end = minint(myblock.end, i+bufsize);
    int64_t len = end - i;
    Incoherent<int64_t>::RW cv(XOFF(i), XOFF(end)-XOFF(i), buf);
    for (int64_t j = 0; j<len; j++) {
      int64_t tmp = cv[2*j]; // 2*j because XOFF entries are even
      cv[2*j] = prev_sum;
      prev_sum += tmp;
    } 
  }
  delete buf;
}

static int64_t prefix_sum(GlobalAddress<int64_t> xoff, int64_t nv) {
  prefix_sum_node fps;
  fps.xoff = xoff;
  fps.nv = nv;
  fork_join_custom(&fps);
 
  // return total sum 
  return Grappa_delegate_read_word(make_global(&local_prefix, Grappa_nodes()-1));
}

LOOP_FUNCTOR( minvect_func, index,
          (( GlobalAddress<int64_t>,xoff )) )
{
  int64_t v = Grappa_delegate_read_word(XOFF(index));
  if (v < MINVECT_SIZE) {
    Grappa_delegate_write_word(XOFF(index), MINVECT_SIZE);
  }
}

LOOP_FUNCTOR( init_xendoff_func, index,
             (( GlobalAddress<int64_t>, xoff )) )
{
  Grappa_delegate_write_word(XENDOFF(index), Grappa_delegate_read_word(XOFF(index)));
}

static void setup_deg_off(const tuple_graph * const tg, csr_graph * g) {
  GlobalAddress<int64_t> xoff = g->xoff;
  // initialize xoff to 0
  Grappa_memset(g->xoff, (int64_t)0, 2*g->nv+2);

  // count occurrences of each vertex in edges
  VLOG(2) << "degree func";
  degree_func fd; fd.edges = tg->edges; fd.xoff = g->xoff;
  fork_join(&fd, 0, tg->nedge);
  
//  for (int64_t i=0; i<g->nv; i++) {
//    std::stringstream ss;
//    int64_t xoi = Grappa_delegate_read_word(XOFF(i)), xei = Grappa_delegate_read_word(XENDOFF(i));
//    ss << "degree[" << i << "] = " << xoi << " : (";
////    for (int64_t j=xoi; j<xei; j++) {
////      ss << Grappa_delegate_read_word(g->xadj+j) << ", ";
////    }
//    ss << ")";
//    VLOG(1) << ss.str();
//  }
  
  // make sure every degree is at least MINVECT_SIZE (don't know why yet...)
  VLOG(2) << "minvect func";
  minvect_func fm; fm.xoff = g->xoff;
  fork_join(&fm, 0, g->nv);
  
  // simple parallel prefix sum to compute offsets from degrees
  VLOG(2) << "prefix sum";
  int64_t accum = prefix_sum(g->xoff, g->nv);
    
  VLOG(2) << "accum = " << accum;
//  for (int64_t i=0; i<g->nv; i++) {
//    VLOG(1) << "offset[" << i << "] = " << Grappa_delegate_read_word(XOFF(i));
//  }
  
  //initialize XENDOFF to be the same as XOFF
  init_xendoff_func fe; fe.xoff = g->xoff;
  fork_join(&fe, 0, g->nv);
  
  Grappa_delegate_write_word(XOFF(g->nv), accum);
  g->nadj = accum+MINVECT_SIZE;
  
  g->xadjstore = Grappa_typed_malloc<int64_t>(accum + MINVECT_SIZE);
  g->xadj = g->xadjstore+MINVECT_SIZE; // cheat and permit xadj[-1] to work
  
  Grappa_memset(g->xadjstore, (int64_t)0, accum+MINVECT_SIZE);
}

inline void scatter_edge(GlobalAddress<int64_t> xoff, GlobalAddress<int64_t> xadj, const int64_t i, const int64_t j) {
  int64_t where = Grappa_delegate_fetch_and_add_word(XENDOFF(i), 1);
  Grappa_delegate_write_word(xadj+where, j);
}

LOOP_FUNCTOR( scatter_func, k, 
  (( GlobalAddress<packed_edge>, ij ))
  (( GlobalAddress<int64_t>, xoff ))
  (( GlobalAddress<int64_t>, xadj )) )
{
  Incoherent<packed_edge>::RO cedge(ij+k, 1);
  int64_t i = cedge[0].v0, j = cedge[0].v1;
  if (i >= 0 && j >= 0 && i != j) {
    scatter_edge(xoff, xadj, i, j);
    scatter_edge(xoff, xadj, j, i);
  }
}

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
LOOP_FUNCTOR( pack_vtx_edges_func, i,
  (( GlobalAddress<int64_t>, xoff ))
  (( GlobalAddress<int64_t>, xadj )) )
{   
  int64_t kcur;
//    Incoherent<int64_t>::RO xoi(XOFF(i), 1);
//    Incoherent<int64_t>::RO xei(XENDOFF(i), 1);
  int64_t xoi = Grappa_delegate_read_word(XOFF(i));
  int64_t xei = Grappa_delegate_read_word(XENDOFF(i));
  if (xoi+1 >= xei) return;
  
  int64_t * buf = (int64_t*)alloca((xei-xoi)*sizeof(int64_t));
  //    Incoherent<int64_t>::RW cadj(xadj+xoi, xei-xoi, buf);
  //    cadj.block_until_acquired();
  // poor man's larger cache buffer, TODO: use cache as above once multi-node acquires are implemented...
  for (int64_t i=0; i<xei-xoi; i++) {
    buf[i] = Grappa_delegate_read_word(xadj+xoi+i);
  }
  
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
  Grappa_delegate_write_word(XENDOFF(i), xoi+kcur);
  
  // poor man's cache release, TODO: let RW cache just release it (once implemented)
  for (int64_t i=0; i<xei-xoi; i++) {
    Grappa_delegate_write_word(xadj+xoi+i, buf[i]);
  }
}

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
  fork_join(&sf, 0, tg->nedge);
  
//  GlobalAddress<int64_t> xoff = g->xoff;  
//  for (int64_t i=0; i<g->nv; i++) {
//    std::stringstream ss;
//    int64_t xoi = Grappa_delegate_read_word(XOFF(i)), xei = Grappa_delegate_read_word(XENDOFF(i));
//    ss << "scat_xoff[" << i << "] = " << xoi << " : (";
//    for (int64_t j=xoi; j<xei; j++) {
//      ss << Grappa_delegate_read_word(g->xadj+j) << ",";
//    }
//    ss << ")";
//    VLOG(1) << ss.str();
//  }
  
//  pack_edges():
//    for (v = 0; v < nv; ++v)
//      pack_vtx_edges (v);
  pack_vtx_edges_func pf;
  pf.xoff = g->xoff;
  pf.xadj = g->xadj;
  fork_join(&pf, 0, g->nv);
}

void create_graph_from_edgelist(const tuple_graph* const tg, csr_graph* const g) {    
  find_nv(tg);
  VLOG(2) << "nv = " << nv;
  
  g->nv = nv;
  g->xoff = Grappa_typed_malloc<int64_t>(2*nv+2);
  
  double time;
  VLOG(2) << "setup_deg_off...";
  TIME(time, setup_deg_off(tg, g));
  LOG(INFO) << "setup_deg_off time: " << time;
  
  VLOG(2) << "gather_edges...";
  TIME(time, gather_edges(tg, g));
  LOG(INFO) << "gather_edges time: " << time;
}

void free_oned_csr_graph(csr_graph* const g) {
  Grappa_free(g->xoff);
  Grappa_free(g->xadjstore);
}

