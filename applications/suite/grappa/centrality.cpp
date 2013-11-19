// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include "defs.hpp"
#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>
#include <Collective.hpp>
#include <common.hpp>
#include <Cache.hpp>
#include <PushBuffer.hpp>
#include <Array.hpp>
#include <Statistics.hpp>

#include "../compat/mersenne.h"

using namespace Grappa;

struct CentralityScratch {
  GlobalAddress<double> delta;
  GlobalAddress<graphint> dist, Q, sigma, marks, child, child_count, explored,
                          Qnext;
};

static graph g;
static CentralityScratch c;
static GlobalAddress<double> bc;
static graphint d_phase;
static graphint local_Qnext;

static double local_centrality_total;

static int64_t nedge_traversed;

static PushBuffer<int64_t> Qbuf;

void do_bfs_push(graphint d_phase_, int64_t start, int64_t end) {
  
  call_on_all_cores([d_phase_]{
    Qbuf.setup(c.Q, c.Qnext);
    d_phase = d_phase_;
  });

  forall_localized(c.Q+start, end-start, [](int64_t i, graphint& v){
    CHECK(v < g.numVertices) << v << " < " << g.numVertices;
    
    int64_t bufEdgeStart[2];
    Incoherent<int64_t>::RO cr(g.edgeStart+v, 2, bufEdgeStart);

    const graphint vstart = cr[0];
    const graphint vend = cr[1];
    CHECK(v < (1L<<32)) << "can't pack 'v' into 32-bit value! have to get more creative";
    CHECK(vend < (1L<<32)) << "can't pack 'vo' into 32-bit value! have to get more creative";
 
    nedge_traversed += vend - vstart;
    DVLOG(3) << "visit (" << i << "): " << vstart << " -> " << vend;
    forall_localized_async(g.endVertex+vstart, vend-vstart,
                    [v](int64_t kstart, int64_t kiters, int64_t * kfirst) {
      DVLOG(3) << "neighbors " << v << ": " << kstart << " -> " << kstart+kiters;

      // TODO: overlap these
      graphint sigmav = delegate::read(c.sigma+v);
      graphint vStart = delegate::read(g.edgeStart+v);
  
      graphint ccount = 0;
      graphint bufChild[kiters];
      
      for (int64_t k=0; k<kiters; k++) {
        graphint w = kfirst[k];
        graphint d = delegate::read(c.dist+w);
            
        // If node has not been visited, set distance and push on Q (but only once)
        if (d < 0) {
          if (delegate::compare_and_swap(c.marks+w, 0, 1)) {
            delegate::write_async(c.dist+w, d_phase);
            Qbuf.push(w);
          }
          d = d_phase;
        }
        if (d == d_phase) {
          delegate::increment_async(c.sigma+w, sigmav);
          bufChild[ccount++] = w;
        }
      }
      // TODO: find out if it makes sense to buffer these
      graphint l = vStart + delegate::fetch_and_add(c.child_count+v, ccount);
      Incoherent<int64_t>::WO cChild(c.child+l, ccount, bufChild);
    });
  });

  on_all_cores([] { Qbuf.flush(); });
}

void do_bfs_pop(graphint start, graphint end) {
  forall_localized(c.Q+start, end-start, [](int64_t i, graphint& v){
    // TODO: overlap reads
    graphint myStart = delegate::read(g.edgeStart+v);
    graphint myEnd   = myStart + delegate::read(c.child_count+v);
  
    nedge_traversed += myEnd-myStart;
    DVLOG(4) << "pop " << v << " (" << i << ")";
    
    // pop children
    forall_localized_async(c.child+myStart, myEnd-myStart,
        [v](int64_t kstart, int64_t kiters, graphint * kchildren){

      int64_t sigma_v = delegate::read(c.sigma+v);

      double sum = 0;

      for (graphint k=0; k<kiters; k++) {
        graphint w = kchildren[k];
        // TODO: overlap
        sum += ((double)sigma_v) * (1.0 + delegate::read(c.delta+w)) / (double)delegate::read(c.sigma+w);
      }
  
      DVLOG(4) << "v(" << v << ")[" << kstart << "," << kstart+kiters << "] sum: " << sum;
      // TODO: maybe shared pool so we don't have to block on sending message?
      delegate::increment_async(c.delta+v, sum);
    });
  });
  
  DVLOG(4) << "###################";
  forall_localized(c.Q+start, end-start, [](int64_t jstart, int64_t jiters, graphint * cQ){

    for (int64_t j=0; j<jiters; j++) {
      const graphint v = cQ[j];
      
      double d = delegate::read(c.delta+v);
      DVLOG(4) << "updating " << v << " (" << jstart+j << ") <= " << d;

      delegate::increment_async(bc+v, d);
    }
  });
}

//////////////////////
// Profiling stuff
/////////////////////
static void enable_tau() {
#ifdef GRAPPA_TRACE
  VLOG(1) << "Enabling TAU recording.";
  call_on_all_cores([]{ FLAGS_record_grappa_events = true; });
#endif
  Grappa::Statistics::start_tracing();
}
static void disable_tau() {
#ifdef GRAPPA_TRACE
  VLOG(1) << "Disabling TAU recording.";
  call_on_all_cores([]{ FLAGS_record_grappa_events = false; });
#endif
  Grappa::Statistics::stop_tracing();
#ifndef GOOGLE_PROFILER
  Statistics::merge_and_print();
  call_on_all_cores([]{ Statistics::reset(); });
#endif
}
///////////////////////

/// Computes the approximate vertex betweenness centrality on an unweighted
/// graph using 'Vs' source vertices. Returns the average centrality.
double centrality(graph *g_in, GlobalAddress<double> bc_in, graphint Vs,
    /* outputs: */ double * avg_centrality = NULL, int64_t * total_nedge = NULL) {
  graphint Qnext;

  bool computeAllVertices = (Vs == g_in->numVertices);
  
  graphint QHead[100 * SCALE];
  
  c.delta       = Grappa_typed_malloc<double>  (g_in->numVertices);
  c.dist        = Grappa_typed_malloc<graphint>(g_in->numVertices);
  c.Q           = Grappa_typed_malloc<graphint>(g_in->numVertices);
  c.sigma       = Grappa_typed_malloc<graphint>(g_in->numVertices);
  c.marks       = Grappa_typed_malloc<graphint>(g_in->numVertices+2);
  c.child       = Grappa_typed_malloc<graphint>(g_in->numEdges);
  c.child_count = Grappa_typed_malloc<graphint>(g_in->numVertices);
  if (!computeAllVertices) c.explored = Grappa_typed_malloc<graphint>(g_in->numVertices);
  c.Qnext       = make_global(&Qnext);
  
  double t; t = timer();
  double rngtime, tt;

  enable_tau();

  { // initialize globals on all cores
    auto _g = *g_in;
    auto _c = c;
    call_on_all_cores([_g,_c,bc_in]{
      nedge_traversed = 0;
      g = _g;
      c = _c;
      bc = bc_in;
    });
  }
  
  Grappa::memset(bc, 0.0, g.numVertices);
  if (!computeAllVertices) Grappa::memset(c.explored, (graphint)0L, g.numVertices);
  
  mersenne_seed(12345);

  graphint nQ, d_phase, Qstart, Qend;
  
  for (graphint x = 0; (x < g.numVertices) && (Vs > 0); x++) {
    /// Choose vertex at random
    graphint s;
    tt = timer();
    if (computeAllVertices) {
      s = x;
    } else {
      do {
        s = mersenne_rand() % g.numVertices;
        VLOG(1) << "s (" << s << ")";
      } while (!delegate::compare_and_swap(c.explored+s, 0, 1));
    }
    rngtime += timer() - tt;
    
    graphint pair_[2];
    Incoherent<graphint>::RO pair(g.edgeStart+s, 2, pair_);
    
    VLOG(1) << "degree (" << pair[1]-pair[0] << ")";

    if (pair[0] == pair[1]) {
      continue;
    } else {
      Vs--;
    }
    
    VLOG(3) << "s = " << s;
    
    Grappa::memset(c.dist,       (graphint)-1, g.numVertices);
    Grappa::memset(c.sigma,      (graphint) 0, g.numVertices);
    Grappa::memset(c.marks,      (graphint) 0, g.numVertices);
    Grappa::memset(c.child_count,(graphint) 0, g.numVertices);
    Grappa::memset(c.delta,        (double) 0, g.numVertices);
    
    // Push node i onto Q and set bounds for first Q sublist
    delegate::write(c.Q+0, s);
    Qnext = 1;
    nQ = 1;
    QHead[0] = 0;
    QHead[1] = 1;
    delegate::write(c.dist+s, 0);
    delegate::write(c.marks+s, 1);
    delegate::write(c.sigma+s, 1);

  PushOnStack: // push nodes onto Q
    
    // Execute the nested loop for each node v on the Q AND
    // for each neighbor w of v whose edge weight is not divisible by 8
    d_phase = nQ;
    Qstart = QHead[nQ-1];
    Qend = QHead[nQ];
    DVLOG(2) << "pushing d_phase(" << d_phase << ") " << Qstart << " -> " << Qend;
    do_bfs_push(d_phase, Qstart, Qend);
    
    // If new nodes pushed onto Q
    if (Qnext != QHead[nQ]) {
      nQ++;
      QHead[nQ] = Qnext;
      goto PushOnStack;
    }
    
    // Dependence accumulation phase
    nQ--;
    DVLOG(3) << "nQ = " << nQ;

    Grappa::memset(c.delta, 0.0, g.numVertices);
    
    // Pop nodes off of Q in the reverse order they were pushed on
    for ( ; nQ > 1; nQ--) {
      Qstart = QHead[nQ-1];
      Qend = QHead[nQ];
      d_phase--;
      DVLOG(2) << "popping d_phase(" << d_phase << ") " << Qstart << " -> " << Qend;
      
      do_bfs_pop(Qstart, Qend);      
      
    }
    
    // DVLOG(2) << util::array_str("delta", c.delta, g.numVertices);
  } // end for(x=0; x<NV && Vs>0)

  DVLOG(3) << util::array_str("bc", bc, g.numVertices, 20);
  
  t = timer() - t;
  disable_tau();
  
  VLOG(1) << "centrality rngtime = " << rngtime;
  
  Grappa_free(c.delta);
  Grappa_free(c.dist);
  Grappa_free(c.Q);
  Grappa_free(c.sigma);
  Grappa_free(c.marks);
  Grappa_free(c.child);
  Grappa_free(c.child_count);
  Grappa_free(c.explored);
  
  double bc_total = 0;
  Core origin = mycore();
  // TODO: use array reduction op, or mutable "forall_localized"-held state
  on_all_cores([&bc_total, origin]{
    auto b = bc.localize();
    auto local_end  = (bc+g.numVertices).localize();
    double sum = 0.0;
    for (; b < local_end; b++) { sum += *b; }
    sum = allreduce<double,collective_add>(sum);
    if (mycore() == origin) bc_total = sum;
  });
  nedge_traversed = reduce<int64_t,collective_add>(&nedge_traversed);

  VLOG(2) << "nedge_traversed: " << nedge_traversed;

  if (avg_centrality != NULL) *avg_centrality = bc_total / g.numVertices;
  if (total_nedge != NULL) *total_nedge = nedge_traversed;
  return t;
}
