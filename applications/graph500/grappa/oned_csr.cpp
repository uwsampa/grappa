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
#include "Cache.hpp"
#include "Collective.hpp"
#include "Delegate.hpp"
#include "AsyncDelegate.hpp"
#include <Array.hpp>
#include "GlobalCompletionEvent.hpp"
#include "LocaleSharedMemory.hpp"

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




template< typename T >
inline void print_array(const char * name, GlobalAddress<T> base, size_t nelem, int width = 10) {
  std::stringstream ss; ss << name << ": [";
  for (size_t i=0; i<nelem; i++) {
    if (i % width == 0) ss << "\n";
    ss << " " << delegate::read(base+i);
  }
  ss << " ]"; VLOG(1) << ss.str();
}
template<>
inline void print_array(const char * name, GlobalAddress<packed_edge> base, size_t nelem, int width) {
  std::stringstream ss; ss << name << ": [";
  for (size_t i=0; i<nelem; i++) {
    if (i % width == 0) ss << "\n";
    auto e = delegate::read(base+i);
    ss << e.v0 << ":" << e.v1 << " ";
  }
  ss << " ]"; VLOG(1) << ss.str();
}

inline std::string graph_str(csr_graph* g) {
  auto offsets = static_cast<GlobalAddress<range_t>>(g->xoff);
  std::stringstream ss;
  for (int64_t i=0; i<g->nv; i++) {
    range_t r = delegate::read(offsets+i);
    ss << "<" << i << ">[" << r.start << "-" << r.end << "](deg:" << r.end-r.start << "): ";
    for (int64_t j=r.start; j<r.end; j++) {
      int64_t v = delegate::read(g->xadj+j);
      ss << v << " ";
    }
    ss << "\n";
  }
  return ss.str();
}




static void find_nv(const tuple_graph* const tg) {
  forall(tg->edges, tg->nedge, [](int64_t i, packed_edge& e){
    if (e.v0 > maxvtx) { maxvtx = e.v0; }
    else if (e.v1 > maxvtx) { maxvtx = e.v1; }
  });
  on_all_cores([]{
    maxvtx = Grappa::allreduce<int64_t,collective_max>(maxvtx);
    nv = maxvtx+1;
  });
}

#define minint(A,B) ((A)<(B)) ? (A) : (B)

int64_t * prefix_temp_base;
int64_t prefix_count;

static int64_t prefix_sum(GlobalAddress<int64_t> xoff, int64_t nv) {
  call_on_all_cores([nv]{
    auto r = blockDist(0,nv,mycore(),cores());
    prefix_temp_base = locale_alloc<int64_t>(r.end-r.start);
  });
  
  auto offsets = static_cast<GlobalAddress<range_t>>(xoff);
  
  forall<&my_gce>(offsets, nv, [offsets,nv](int64_t s, int64_t n, range_t * x){
    for (int i=0; i<n; i++) {
      int64_t index = make_linear(x+i)-offsets;
      block_offset_t b = indexToBlock(index, nv, cores());
      DVLOG(5) << "[" << index << "] -> " << b.block << ":" << b.offset << " (@ " << prefix_temp_base << ")  (" << x << ")[" << i << "] == " << x[i].start << "\nmake_global: " << make_global(prefix_temp_base+b.offset, b.block);
      
      CHECK_LT(b.block, cores());
      
      auto val = x[i].start;
      auto offset = b.offset;
      delegate::call<async,&my_gce>(b.block, [offset,val] {
        prefix_temp_base[offset] = val;
      });
    }
  });
  
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
    // VLOG(1) << "global_scan = " << prefix_count;
    
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
    forall_here(0, nlocal, [xoff,r](int64_t i){
      delegate::write<async,&my_gce>(xoff+2*(r.start+i), prefix_temp_base[i]);
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
  forall<&my_gce>(tg->edges, tg->nedge, [](int64_t i, packed_edge& edge) {
    if (edge.v0 != edge.v1) { //skip self-edges
      delegate::increment<async,&my_gce>(XOFF(edge.v0), 1);
      delegate::increment<async,&my_gce>(XOFF(edge.v1), 1);
    }
  });
  
  VLOG(2) << "minvect func";
  // make sure every degree is at least MINVECT_SIZE (don't know why yet...)
  forall<&my_gce>(xoff, g->nv, [](int64_t& vo) {
    if (vo < MINVECT_SIZE) { vo = MINVECT_SIZE; }
  });
  
  // simple parallel prefix sum to compute offsets from degrees
  VLOG(2) << "prefix sum";
  int64_t accum = prefix_sum(g->xoff, g->nv);
    
  VLOG(2) << "accum = " << accum;
  
  //initialize XENDOFF to be the same as XOFF
  auto xoffr = static_cast<GlobalAddress<range_t>>(xoff);
  forall<&my_gce>(xoffr, g->nv, [](range_t& o){
    o.end = o.start;
  });
  
  delegate::write(xoff+2*g->nv, accum);
  
  g->nadj = accum+MINVECT_SIZE;
  
  g->xadjstore = Grappa::global_alloc<int64_t>(accum + MINVECT_SIZE);
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


inline void scatter_edge(GlobalAddress<int64_t> xoff, GlobalAddress<int64_t> xadj, const int64_t i, const int64_t j) {
  int64_t where = delegate::fetch_and_add(XENDOFF(i), 1);
  delegate::write<async,&my_gce>(xadj+where, j);
  
  DVLOG(5) << "scattering [" << i << "] xendoff[i] " << XENDOFF(i) << " = " << j << " (where: " << where << ")";
}

static void gather_edges(const tuple_graph * const tg, csr_graph * g_ptr) {
  auto& g = *g_ptr;
  VLOG(2) << "scatter edges";
  
  forall<&my_gce>(tg->edges, tg->nedge, [g](packed_edge& e) {
    int64_t i = e.v0, j = e.v1;
    
    if (i >= 0 && j >= 0 && i != j) {
      scatter_edge(g.xoff, g.xadj, i, j);
      scatter_edge(g.xoff, g.xadj, j, i);
    }
  });
  
  VLOG(2) << "pack_vtx_edges";
  auto xoffr = static_cast<GlobalAddress<range_t>>(g.xoff);
  forall<&my_gce>(xoffr, g.nv, [g](int64_t i, range_t& xi) {
    auto xoi = xi.start;
    auto xei = xi.end;
    
    if (xoi+1 >= xei) return;

    //int64_t * buf = new int64_t[xei-xoi];// (int64_t*)alloca((xei-xoi)*sizeof(int64_t));
    int64_t * buf = locale_alloc<int64_t>(xei-xoi);
    Incoherent<int64_t>::RW cadj(g.xadj+xoi, xei-xoi, buf);
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
    auto xoff = g.xoff;
    Incoherent<int64_t>::WO cend(XENDOFF(i), 1, &end);
    *cend = xoi+kcur;

    cadj.block_until_released();
    locale_free(buf);

    // on scope: cend.block_until_released();
  });

}

void create_graph_from_edgelist(const tuple_graph* const tg, csr_graph* const g) {    
  find_nv(tg);
  VLOG(2) << "nv = " << nv;
  
  g->nv = nv;
  g->xoff = global_alloc<int64_t>(2*nv+2);
  
  double time;
  VLOG(2) << "setup_deg_off...";
  TIME(time, setup_deg_off(tg, g));
  LOG(INFO) << "setup_deg_off time: " << time;
  
  VLOG(2) << "gather_edges...";
  TIME(time, gather_edges(tg, g));
  LOG(INFO) << "gather_edges time: " << time;
  
  DVLOG(3) << graph_str(g);
}

void free_oned_csr_graph(csr_graph* const g) {
  Grappa::global_free(g->xoff);
  Grappa::global_free(g->xadjstore);
}

