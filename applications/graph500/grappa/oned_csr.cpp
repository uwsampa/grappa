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
#include "AsyncDelegate.hpp"
#include "GlobalTaskJoiner.hpp"
#include <Array.hpp>
#include "GlobalCompletionEvent.hpp"

using namespace Grappa;

#include "timer.h"

#include <sstream>
#include <functional>

#define MINVECT_SIZE 2

#define XOFF(k) (xoff+2*(k))
#define XENDOFF(k) (xoff+1+2*(k))

GlobalAddress<int64_t> xoff;

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
    
  maxvtx = Grappa_allreduce<int64_t, collective_max, 0>( max );
  
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
 
  Grappa_barrier_suspending(); // "phaser style", alternative is replace with forjoins 

  // node 0 sets buf[i] to prefix sum
  // TODO: do this as parallel prefix
  if (nid == 0) {
    int64_t total = local_prefix;
    for (int64_t i=1; i<Grappa_nodes(); i++) {
      //LOG(INFO) << "will add total=" << total << " to node " << i;
      total += Grappa_delegate_fetch_and_add_word(make_global(&local_prefix,i), total);
    }
  }
  
  Grappa_barrier_suspending(); // alternative: replace with forkjoins
  
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

int64_t * prefix_temp_base;
int64_t prefix_count;

static int64_t prefix_sum(GlobalAddress<int64_t> xoff, int64_t nv) {
  auto prefix_temp = Grappa_typed_malloc<int64_t>(nv);
  call_on_all_cores([prefix_temp,nv]{ prefix_temp_base = prefix_temp.localize(); });
  
  struct XoffT { int64_t start, end; };
  auto offsets = static_cast<GlobalAddress<XoffT>>(xoff);
  
  forall_localized(offsets, nv, [offsets,nv](int64_t s, int64_t n, XoffT * x){
    char buf[n * sizeof(delegate::write_msg_proxy<int64_t>)];
    MessagePoolBase pool(buf, sizeof(buf));
    
    for (int i=0; i<n; i++) {
      int64_t index = make_linear(x+i)-offsets;
      block_offset_t b = indexToBlock(index, nv, cores());
      CHECK_LT(b.block, cores());
      delegate::write_async(pool, make_global(prefix_temp_base+b.offset, b.block), x[i].start);
    }
  });
  
  VLOG(1) << "after moving to block-distribution";
  
  int64_t total_sum = 0;
  auto total_addr = make_global(&total_sum);
  
  on_all_cores([xoff,nv,total_addr] {
    // compute reduction locally
    range_t r = blockDist(0, nv, mycore(), cores());
    auto nlocal = r.end - r.start;
    
    int64_t local_sum = 0;
    for (int64_t i=0; i<nlocal; i++) { local_sum += prefix_temp_base[i]; }
    
    // compute global scan across cores
    Core cafter = cores()-mycore();
    for (Core c = mycore()+1; c < cores(); c++) {
      // TODO: overlap delegates
      delegate::fetch_and_add(make_global(&prefix_count,c), local_sum);
    }

    impl::local_gce.enroll(); // make sure it doesn't dip down to 0 early

    barrier();    
    
    if (mycore() == cores()-1) {
      auto accum = prefix_count + local_sum;
      VLOG(1) << "nadj = " << accum + MINVECT_SIZE;
      delegate::write(xoff+2*nv, accum);
      delegate::write(total_addr, accum + MINVECT_SIZE);
    }
    
    DVLOG(4) << "prefix_count = " << prefix_count << ", local_sum = " << local_sum;
    // do prefix sum locally
    local_sum = prefix_count;
    for (int64_t i=0; i<nlocal; i++) {
      int64_t tmp = prefix_temp_base[i];
      prefix_temp_base[i] = local_sum;
      local_sum += tmp;
    }
    
    // put back into original array
    forall_here(0, nlocal, [xoff,r](int64_t s, int64_t n){
      char buf[n * sizeof(delegate::write_msg_proxy<int64_t>)];
      MessagePoolBase pool(buf, sizeof(buf));
      for (int64_t i=s; i<s+n; i++) {
        delegate::write_async(pool, xoff+2*(r.start+i), prefix_temp_base[i]);
      }
    });
    impl::local_gce.complete();
    impl::local_gce.wait();
  });

  // return total sum
  return total_sum;
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
  // note: this corresponds to how Graph500 counts 'degree' (both in- and outgoing edges to each vertex)
  auto _xoff = g->xoff;
  call_on_all_cores([_xoff]{ xoff = _xoff; });
  
  // initialize xoff to 0
  Grappa::memset(g->xoff, (int64_t)0, 2*g->nv+2);

  // count occurrences of each vertex in edges
  VLOG(2) << "degree func";
  forall_localized(tg->edges, tg->nedge, [](int64_t s, int64_t n, packed_edge * edge) {
    char _poolbuf[2*n*128];
    MessagePoolBase pool(_poolbuf, sizeof(_poolbuf));
    
    for (int64_t i=0; i<n; i++) {
      packed_edge& cedge = edge[i];
      if (cedge.v0 != cedge.v1) { //skip self-edges
        delegate::increment_async(pool, XOFF(cedge.v0), 1);
        delegate::increment_async(pool, XOFF(cedge.v1), 1);
      }
    }
  });
  
  VLOG(2) << "minvect func";
  // make sure every degree is at least MINVECT_SIZE (don't know why yet...)
  forall_localized(xoff, g->nv, [](int64_t i, int64_t& vo) {
    if (vo < MINVECT_SIZE) { vo = MINVECT_SIZE; }
  });
  
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


static int
i64cmp (const void *a, const void *b)
{
	const int64_t ia = *(const int64_t*)a;
	const int64_t ib = *(const int64_t*)b;
	if (ia < ib) return -1;
	if (ia > ib) return 1;
	return 0;
}

struct pack_vtx_edges_args {
  GlobalAddress<int64_t> xoff;
  GlobalAddress<int64_t> xadj;
};
pack_vtx_edges_args local_pack_vtx_edges_args;
const int PACK_EDGES_THESHOLD = 4;
void pack_edges_loop_body( int64_t start, int64_t iterations );
LOOP_FUNCTOR( pack_vtx_edges_func, nid,
    (( GlobalAddress<int64_t>, xoff ))
    (( GlobalAddress<int64_t>, xadj ))
    (( int64_t, numvert )) ) {

  local_pack_vtx_edges_args.xoff = xoff;
  local_pack_vtx_edges_args.xadj = xadj;
  
  global_joiner.reset();
  if (nid==0) {
    async_parallel_for< pack_edges_loop_body, joinerSpawn<pack_edges_loop_body,PACK_EDGES_THESHOLD>,PACK_EDGES_THESHOLD>(0, numvert);
  }  
  global_joiner.wait();
}

// TODO: with power-law graphs, this could still be a problem? (everyone waiting for one big one to finish).
//LOOP_FUNCTOR( pack_vtx_edges_func, i,
//  (( GlobalAddress<int64_t>, xoff ))
//  (( GlobalAddress<int64_t>, xadj )) )
void pack_edges_loop_body( int64_t start, int64_t iterations ) {
  GlobalAddress<int64_t> xoff = local_pack_vtx_edges_args.xoff;
  GlobalAddress<int64_t> xadj = local_pack_vtx_edges_args.xadj;

  // TODO: try harder to hoist cache out of for loop
  for ( int64_t i = start; i < start+iterations; i++ ) {
    int64_t kcur;
    int64_t xoi, xei;
    Incoherent<int64_t>::RO cxoi(XOFF(i), 1, &xoi);
    Incoherent<int64_t>::RO cxei(XENDOFF(i), 1, &xei);
    cxoi.start_acquire(); cxei.start_acquire();
    cxoi.block_until_acquired(); cxei.block_until_acquired();
    if (xoi+1 >= xei) continue;

    int64_t * buf = new int64_t[xei-xoi];// (int64_t*)alloca((xei-xoi)*sizeof(int64_t));
    Incoherent<int64_t>::RW cadj(xadj+xoi, xei-xoi, buf);
    cadj.block_until_acquired();

    // pass in the underlying buf, since qsort can't take a cache obj 
    qsort(buf, xei-xoi, sizeof(int64_t), i64cmp);
    kcur = 0;
    for (int64_t k = 1; k < xei-xoi; k++) {
      if (cadj[k] != cadj[kcur]) {
        cadj[++kcur] = cadj[k];
      }
    }
    ++kcur;
    for (int64_t k = kcur; k < xei-xoi; k++) {
      cadj[k] = -1;
    }
    cadj.start_release(); // done writing to local xadj

    int64_t end;
    Incoherent<int64_t>::WO cend(XENDOFF(i), 1, &end);
    *cend = xoi+kcur;

    cadj.block_until_released();
    delete buf;

    // on scope: cend.block_until_released();

  }
}

inline void scatter_edge(GlobalAddress<int64_t> xoff, GlobalAddress<int64_t> xadj, const int64_t i, const int64_t j) {
  int64_t where = Grappa_delegate_fetch_and_add_word(XENDOFF(i), 1);
  Grappa_delegate_write_word(xadj+where, j);
}

//scatter, private tasks version
//
//LOOP_FUNCTOR( scatter_func, k, 
//  (( GlobalAddress<packed_edge>, ij ))
//  (( GlobalAddress<int64_t>, xoff ))
//  (( GlobalAddress<int64_t>, xadj )) )
//{
//  Incoherent<packed_edge>::RO cedge(ij+k, 1);
//  int64_t i = cedge[0].v0, j = cedge[0].v1;
//  if (i >= 0 && j >= 0 && i != j) {
//    scatter_edge(xoff, xadj, i, j);
//    scatter_edge(xoff, xadj, j, i);
//  }
//}

struct scatter_args {
  GlobalAddress<packed_edge> ij;
  GlobalAddress<int64_t> xoff;
  GlobalAddress<int64_t> xadj;
};

// local arg cache for scatter
scatter_args local_scatter_args;

void scatter_loop_body( int64_t start, int64_t iterations ) {
  GlobalAddress<packed_edge> ij = local_scatter_args.ij;
  GlobalAddress<int64_t> xoff = local_scatter_args.xoff;
  GlobalAddress<int64_t> xadj = local_scatter_args.xadj;

  Incoherent<packed_edge>::RO cedge(ij+start, iterations);
  for ( int64_t k = 0; k < iterations; k++ ) {
    int64_t i = cedge[k].v0, j = cedge[k].v1;
    if (i >= 0 && j >= 0 && i != j) {
      scatter_edge(xoff, xadj, i, j);
      scatter_edge(xoff, xadj, j, i);
      // TODO optimize overlap of scatters
    }
  }
}



LOOP_FUNCTOR( scatter_func, nid, 
  (( GlobalAddress<packed_edge>, ij ))
  (( GlobalAddress<int64_t>, xoff ))
  (( GlobalAddress<int64_t>, xadj ))
  (( int64_t, nedge )) )
{
  local_scatter_args.ij = ij;
  local_scatter_args.xoff = xoff;
  local_scatter_args.xadj = xadj; 

  global_joiner.reset();
  if (nid==0) {
    async_parallel_for< scatter_loop_body, joinerSpawn<scatter_loop_body,ASYNC_PAR_FOR_DEFAULT>,ASYNC_PAR_FOR_DEFAULT>(0, nedge);
  }  
  global_joiner.wait();
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
  VLOG(2) << "scatter edges";
  scatter_func sf;
  sf.ij = tg->edges;
  sf.xoff = g->xoff;
  sf.xadj = g->xadj;
  sf.nedge = tg->nedge;
  fork_join_custom(&sf);

  
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
  VLOG(2) << "pack_vtx_edges";
  pack_vtx_edges_func pf;
  pf.xoff = g->xoff;
  pf.xadj = g->xadj;
  pf.numvert = g->nv;
  fork_join_custom(&pf);
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

