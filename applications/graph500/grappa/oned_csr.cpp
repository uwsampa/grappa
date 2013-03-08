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

static int64_t maxvtx = 0, nv = 0;

static GlobalCompletionEvent my_gce;

static void find_nv(const tuple_graph* const tg) {
  forall_localized(tg->edges, tg->nedge, [](int64_t i, packed_edge& e){
    if (e.v0 > maxvtx) { maxvtx = e.v0; }
    else if (e.v1 > maxvtx) { maxvtx = e.v1; }
  });
  on_all_cores([]{
    maxvtx = Grappa::allreduce<int64_t,collective_add>(maxvtx);
    nv = maxvtx+1;
  });
}

#define minint(A,B) ((A)<(B)) ? (A) : (B)

int64_t * prefix_temp_base;
int64_t prefix_count;

static int64_t prefix_sum(GlobalAddress<int64_t> xoff, int64_t nv) {
  auto prefix_temp = Grappa_typed_malloc<int64_t>(nv);
  call_on_all_cores([prefix_temp,nv]{ prefix_temp_base = prefix_temp.localize(); });
  
  auto offsets = static_cast<GlobalAddress<range_t>>(xoff);
  
  forall_localized<&my_gce>(offsets, nv, [offsets,nv](int64_t s, int64_t n, range_t * x){
    char buf[n * sizeof(delegate::write_msg_proxy<int64_t>)];
    MessagePool pool(buf, sizeof(buf));
    
    for (int i=0; i<n; i++) {
      int64_t index = make_linear(x+i)-offsets;
      block_offset_t b = indexToBlock(index, nv, cores());
      CHECK_LT(b.block, cores());
      delegate::write_async<&my_gce>(pool, make_global(prefix_temp_base+b.offset, b.block), x[i].start);
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

    my_gce.enroll(); // make sure it doesn't dip down to 0 early

    barrier();    
    
    if (mycore() == cores()-1) {
      auto accum = prefix_count + local_sum;
      VLOG(1) << "nadj = " << accum + MINVECT_SIZE;
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
      MessagePool pool(buf, sizeof(buf));
      for (int64_t i=s; i<s+n; i++) {
        delegate::write_async<&my_gce>(pool, xoff+2*(r.start+i), prefix_temp_base[i]);
      }
    });
    my_gce.complete();
    my_gce.wait();
  });

  // return total sum
  return total_sum;
}

static void setup_deg_off(const tuple_graph * const tg, csr_graph * g) {
  // note: this corresponds to how Graph500 counts 'degree' (both in- and outgoing edges to each vertex)
  auto _xoff = g->xoff;
  call_on_all_cores([_xoff]{ xoff = _xoff; });
  
  // initialize xoff to 0
  Grappa::memset(g->xoff, (int64_t)0, 2*g->nv+2);

  // count occurrences of each vertex in edges
  VLOG(2) << "degree func";
  // note: this corresponds to how Graph500 counts 'degree' (both in- and outgoing edges to each vertex)
  forall_localized<&my_gce>(tg->edges, tg->nedge, [](int64_t s, int64_t n, packed_edge * edge) {
    char _poolbuf[2*n*128];
    MessagePool pool(_poolbuf, sizeof(_poolbuf));
    
    for (int64_t i=0; i<n; i++) {
      packed_edge& cedge = edge[i];
      if (cedge.v0 != cedge.v1) { //skip self-edges
        delegate::increment_async<&my_gce>(pool, XOFF(cedge.v0), 1);
        delegate::increment_async<&my_gce>(pool, XOFF(cedge.v1), 1);
      }
    }
  });
  
  VLOG(2) << "minvect func";
  // make sure every degree is at least MINVECT_SIZE (don't know why yet...)
  forall_localized<&my_gce>(xoff, g->nv, [](int64_t i, int64_t& vo) {
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
  auto xoffr = static_cast<GlobalAddress<range_t>>(xoff);
  forall_localized<&my_gce>(xoffr, g->nv, [](int64_t i, range_t& o){
    o.end = o.start;
  });
  
  delegate::write(xoff+2*g->nv, accum);
  g->nadj = accum+MINVECT_SIZE;
  
  g->xadjstore = Grappa_typed_malloc<int64_t>(accum + MINVECT_SIZE);
  g->xadj = g->xadjstore+MINVECT_SIZE; // cheat and permit xadj[-1] to work
  
  Grappa::memset(g->xadjstore, (int64_t)0, accum+MINVECT_SIZE);
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



inline void scatter_edge(MessagePool& pool, GlobalAddress<int64_t> xoff, GlobalAddress<int64_t> xadj, const int64_t i, const int64_t j) {
  int64_t where = delegate::fetch_and_add(XENDOFF(i), 1);
  delegate::write_async<&my_gce>(pool, xadj+where, j);
  // delegate::write(xadj+where, j);
}

static void gather_edges(const tuple_graph * const tg, csr_graph * g) {
  VLOG(2) << "scatter edges";
  
  csr_graph& graph = *g;
  forall_localized<&my_gce>(tg->edges, tg->nedge, [graph](int64_t s, int64_t n, packed_edge * e) {
    char bp[2 * n * sizeof(delegate::write_msg_proxy<int64_t>)];
    MessagePool pool(bp, sizeof(bp));
    
    for (int k = 0; k < n; k++) {
      int64_t i = e[k].v0, j = e[k].v1;    
      if (i >= 0 && j >= 0 && i != j) {
        scatter_edge(pool, graph.xoff, graph.xadj, i, j);
        scatter_edge(pool, graph.xoff, graph.xadj, j, i);
      }
    }
  });
  
  VLOG(2) << "pack_vtx_edges";
  auto xoffr = static_cast<GlobalAddress<range_t>>(xoff);
  forall_localized<&my_gce>(xoffr, g->nv, [graph](int64_t i, range_t& xi) {
    auto xoi = xi.start;
    auto xei = xi.end;
    
    if (xoi+1 >= xei) return;

    int64_t * buf = new int64_t[xei-xoi];// (int64_t*)alloca((xei-xoi)*sizeof(int64_t));
    Incoherent<int64_t>::RW cadj(graph.xadj+xoi, xei-xoi, buf);
    cadj.block_until_acquired();

    // pass in the underlying buf, since qsort can't take a cache obj 
    qsort(buf, xei-xoi, sizeof(int64_t), i64cmp);
    int64_t kcur = 0;
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
    auto xoff = graph.xoff;
    Incoherent<int64_t>::WO cend(XENDOFF(i), 1, &end);
    *cend = xoi+kcur;

    cadj.block_until_released();
    delete buf;

    // on scope: cend.block_until_released();
  });
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

