// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include "defs.hpp"
#include <Grappa.hpp>
#include <ForkJoin.hpp>
#include <GlobalAllocator.hpp>
#include <Delegate.hpp>
#include <Collective.hpp>
#include <common.hpp>
#include <Cache.hpp>
#include <GlobalTaskJoiner.hpp>
#include <PushBuffer.hpp>
#include <Array.hpp>

#include "../compat/mersenne.h"

#define read      Grappa_delegate_read_word
#define write     Grappa_delegate_write_word
#define cmp_swap  Grappa_delegate_compare_and_swap_word
#define fetch_add Grappa_delegate_fetch_and_add_word

static inline double read_double(GlobalAddress<double> addr) {
  int64_t temp = Grappa_delegate_read_word(addr);
  return *reinterpret_cast<double*>(&temp);
}

struct CentralityScratch {
  GlobalAddress<double> delta;
  GlobalAddress<graphint> dist, Q, sigma, marks, child, child_count, explored,
                          Qnext;
};

static LocalTaskJoiner joiner;

static graph g;
static CentralityScratch c;
static GlobalAddress<double> bc;
static graphint d_phase;
static graphint local_Qnext;

static int64_t nedge_traversed;

static PushBuffer<int64_t> Qbuf;

void bfs_push_visit_neighbor(int64_t kstart, int64_t kiters, GlobalAddress<void*> packed) {
  graphint v = (graphint)packed.pointer();
  DVLOG(3) << "neighbors " << v << ": " << kstart << " -> " << kstart+kiters;

  graphint sigmav = read(c.sigma+v);
  graphint vStart = read(g.edgeStart+v);

  graphint bufEndV[kiters];
  Incoherent<int64_t>::RO cendV(g.endVertex+kstart, kiters, bufEndV);
  
  graphint ccount = 0;
  graphint bufChild[kiters];
    
  for (int64_t k=0; k<kiters; k++) {
    graphint w = cendV[k];
    graphint d = read(c.dist+w);
    
    /* If node has not been visited, set distance and push on Q (but only once) */
    if (d < 0) {
      if (cmp_swap(c.marks+w, 0, 1)) {
        write(c.dist+w, d_phase);
        //write(c.Q + fetch_add(c.Qnext, 1), w);
        Qbuf.push(w);
      }
    }
    if (d < 0 || d == d_phase) {
      fetch_add(c.sigma+w, sigmav);
      bufChild[ccount++] = w;
    }
  }
  graphint l = vStart + fetch_add(c.child_count+v, ccount);
  Incoherent<int64_t>::WO cChild(c.child+l, ccount, bufChild);
}

void bfs_push_visit_vertex(int64_t jstart, int64_t jiters) {
  DVLOG(3) << "visit_vertex< " << jstart << " -> " << jstart+jiters << " >";
  int64_t bufQ[jiters];
  Incoherent<int64_t>::RO cQ(c.Q+jstart, jiters, bufQ);

  for (int64_t j=0; j<jiters; j++) {
    const int64_t v = cQ[j];
    CHECK(v < g.numVertices) << v << " < " << g.numVertices;
    
    int64_t bufEdgeStart[2];
    Incoherent<int64_t>::RO cr(g.edgeStart+v, 2, bufEdgeStart);

    const graphint vstart = cr[0];
    const graphint vend = cr[1];
    CHECK(v < (1L<<32)) << "can't pack 'v' into 32-bit value! have to get more creative";
    CHECK(vend < (1L<<32)) << "can't pack 'vo' into 32-bit value! have to get more creative";
 
    nedge_traversed += vend - vstart;
    DVLOG(3) << "visit (" << jstart+j << "): " << vstart << " -> " << vend;
    async_parallel_for_hack(bfs_push_visit_neighbor, vstart, vend-vstart, v);
  }
}

LOOP_FUNCTOR(bfs_push, nid, ((graphint,d_phase_)) ((int64_t,start)) ((int64_t,end))) {
  d_phase = d_phase_;

  Qbuf.setup(c.Q, c.Qnext);

  global_async_parallel_for_thresh(bfs_push_visit_vertex, start, end-start, 1);  

  Qbuf.flush();
}

void bfs_pop_children(int64_t kstart, int64_t kiters, GlobalAddress<void*> packed) {
  int64_t v = (int64_t)packed.pointer();
  int64_t sigma_v = read(c.sigma+v);
  
  graphint bufChildren[kiters];
  Incoherent<graphint>::RO cChildren(c.child+kstart, kiters, bufChildren);

  double sum = 0;

  for (graphint k=0; k<kiters; k++) {
    graphint w = cChildren[k];
    sum += ((double)sigma_v) * (1.0 + read_double(c.delta+w)) / (double)read(c.sigma+w);
  }
  
  DVLOG(4) << "v(" << v << ")[" << kstart << "," << kstart+kiters << "] sum: " << sum;
  ff_delegate_add(c.delta+v, sum);
}


void bfs_pop_vertex(int64_t jstart, int64_t jiters) {
  DVLOG(3) << "pop_vertex < " << jstart << " -> " << jstart+jiters << " >";
  int64_t bufQ[jiters];
  Incoherent<int64_t>::RO cQ(c.Q+jstart, jiters, bufQ);

  double sum = 0;
  
  for (int64_t j=0; j<jiters; j++) {
    graphint v = cQ[j];
    graphint myStart = read(g.edgeStart+v);
    graphint myEnd   = myStart + read(c.child_count+v);
  
    nedge_traversed += myEnd-myStart;
    DVLOG(4) << "pop " << v << " (" << jstart+j << ")";
    
    async_parallel_for_hack(bfs_pop_children, myStart, myEnd-myStart, v);
  }
}

void bc_add_delta(int64_t jstart, int64_t jiters) {
  int64_t bufQ[jiters];
  Incoherent<int64_t>::RO cQ(c.Q+jstart, jiters, bufQ);
  for (int64_t j=0; j<jiters; j++) {
    const graphint v = cQ[j];
    
    double d = read_double(c.delta+v);
    DVLOG(4) << "updating " << v << " (" << jstart+j << ") <= " << d;

    ff_delegate_add(bc+v, d);
  }
}

LOOP_FUNCTOR(bfs_pop, nid, ((graphint,start)) ((graphint,end)) ) {
  //TODO: consider adjusting threshold programmatically for different kinds of apf's...
  global_async_parallel_for_thresh(bfs_pop_vertex, start, end-start, 1);
  DVLOG(4) << "###################";
  global_async_parallel_for_thresh(bc_add_delta, start, end-start, 1);
}

LOOP_FUNCTOR( initCentrality, nid, ((graph,g_)) ((CentralityScratch,s_)) ((GlobalAddress<double>,bc_)) ) {
  nedge_traversed = 0;
  g = g_;
  c = s_;
  bc = bc_;
}

LOOP_FUNCTOR( totalCentrality, v, ((GlobalAddress<double>,total)) ) {
  ff_delegate_add(total, read_double(bc+v));
}

LOOP_FUNCTION( totalNedgeFunc, n ) {
  nedge_traversed = Grappa_allreduce<int64_t,collective_add<int64_t>,0>(nedge_traversed);
}

//////////////////////
// Profiling stuff
/////////////////////
LOOP_FUNCTION(func_enable_tau, nid) {
  FLAGS_record_grappa_events = true;
}
LOOP_FUNCTION(func_enable_google_profiler, nid) {
  Grappa_start_profiling();
}
static void enable_tau() {
#ifdef GRAPPA_TRACE
  VLOG(1) << "Enabling TAU recording.";
  func_enable_tau f;
  fork_join_custom(&f);
#endif
#ifdef GOOGLE_PROFILER
  func_enable_google_profiler g;
  fork_join_custom(&g);
#else
  Grappa_reset_stats_all_nodes();
#endif
}
LOOP_FUNCTION(func_disable_tau, nid) {
  FLAGS_record_grappa_events = false;
}
LOOP_FUNCTION(func_disable_google_profiler, nid) {
  Grappa_stop_profiling();
}
static void disable_tau() {
#ifdef GRAPPA_TRACE
  VLOG(1) << "Disabling TAU recording.";
  func_disable_tau f;
  fork_join_custom(&f);
#endif
#ifdef GOOGLE_PROFILER
  func_disable_google_profiler g;
  fork_join_custom(&g);
#else
  Grappa_merge_and_dump_stats();
  Grappa_reset_stats_all_nodes();
#endif
}
///////////////////////

/// Computes the approximate vertex betweenness centrality on an unweighted
/// graph using 'Vs' source vertices. Returns the average centrality.
double centrality(graph *g, GlobalAddress<double> bc, graphint Vs,
    /* outputs: */ double * avg_centrality = NULL, int64_t * total_nedge = NULL) {
  graphint Qnext;

  bool computeAllVertices = (Vs == g->numVertices);
  
  graphint QHead[100 * SCALE];
  
  c.delta       = Grappa_typed_malloc<double>  (g->numVertices);
  c.dist        = Grappa_typed_malloc<graphint>(g->numVertices);
  c.Q           = Grappa_typed_malloc<graphint>(g->numVertices);
  c.sigma       = Grappa_typed_malloc<graphint>(g->numVertices);
  c.marks       = Grappa_typed_malloc<graphint>(g->numVertices+2);
  c.child       = Grappa_typed_malloc<graphint>(g->numEdges);
  c.child_count = Grappa_typed_malloc<graphint>(g->numVertices);
  if (!computeAllVertices) c.explored = Grappa_typed_malloc<graphint>(g->numVertices);
  c.Qnext       = make_global(&Qnext);
  
  double t; t = timer();
  double rngtime, tt;

  enable_tau();

  {
    initCentrality f(*g, c, bc); fork_join_custom(&f);
  }
  
  Grappa_memset(bc, 0.0, g->numVertices);
  if (!computeAllVertices) Grappa_memset(c.explored, (graphint)0L, g->numVertices);
  
  //srand(12345);
  mersenne_seed(12345);

  graphint nQ, d_phase, Qstart, Qend;
  
  for (graphint x = 0; (x < g->numVertices) && (Vs > 0); x++) {
    /// Choose vertex at random
    graphint s;
    tt = timer();
    if (computeAllVertices) {
      s = x;
    } else {
      do {
        s = mersenne_rand() % g->numVertices;
        VLOG(1) << "s (" << s << ")";
      } while (!cmp_swap(c.explored+s, 0, 1));
    }
    rngtime += timer() - tt;
    
    graphint pair_[2];
    Incoherent<graphint>::RO pair(g->edgeStart+s, 2, pair_);
    
    VLOG(1) << "degree (" << pair[1]-pair[0] << ")";

    if (pair[0] == pair[1]) {
      continue;
    } else {
      Vs--;
    }
    
    VLOG(3) << "s = " << s;
    
    Grappa_memset(c.dist,       (graphint)-1, g->numVertices);
    Grappa_memset(c.sigma,      (graphint) 0, g->numVertices);
    Grappa_memset(c.marks,      (graphint) 0, g->numVertices);
    Grappa_memset(c.child_count,(graphint) 0, g->numVertices);
    Grappa_memset(c.delta,        (double) 0, g->numVertices);
    
    // Push node i onto Q and set bounds for first Q sublist
    write(c.Q+0, s);
    Qnext = 1;
    nQ = 1;
    QHead[0] = 0;
    QHead[1] = 1;
    write(c.dist+s, 0);
    write(c.marks+s, 1);
    write(c.sigma+s, 1);

  PushOnStack: // push nodes onto Q
    
    // Execute the nested loop for each node v on the Q AND
    // for each neighbor w of v whose edge weight is not divisible by 8
    d_phase = nQ;
    Qstart = QHead[nQ-1];
    Qend = QHead[nQ];
    DVLOG(1) << "pushing d_phase(" << d_phase << ") " << Qstart << " -> " << Qend;
    {
      bfs_push f(d_phase, Qstart, Qend); fork_join_custom(&f);
    }
    
    // If new nodes pushed onto Q
    if (Qnext != QHead[nQ]) {
      nQ++;
      QHead[nQ] = Qnext; 
      goto PushOnStack;
    }
    
    // Dependence accumulation phase
    nQ--;
    VLOG(3) << "nQ = " << nQ;
    
//    {
//      std::stringstream ss;
//      ss << "sigma = [";
//      for (graphint i=0; i<g->numVertices; i++) ss << " " << read(c.sigma+i);
//      ss << " ]";
//      VLOG(1) << ss.str();
//    } 
    Grappa_memset(c.delta, 0.0, g->numVertices);
    
    // Pop nodes off of Q in the reverse order they were pushed on
    for ( ; nQ > 1; nQ--) {
      Qstart = QHead[nQ-1];
      Qend = QHead[nQ];
      
      {
        bfs_pop f(Qstart, Qend); fork_join_custom(&f);
      }
      //VLOG(1) << "-----------------------";
      //for (int64_t i=0; i<g->numVertices; i++) {
      //  VLOG(1) << "bc[" << i << "] = " << read_double(bc+i);
      //}
    }
//    {
//      std::stringstream ss;
//      ss << "bc = [";
//      for (graphint i=0; i<g->numVertices; i++) ss << " " << read_double(bc+i);
//      ss << " ]";
//      VLOG(1) << ss.str();
//    }
//    VLOG(1) << "bc[" << 12 << "] = " << read_double(bc+12);
  } // end for(x=0; x<NV && Vs>0)
    
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
  {
    totalCentrality f(make_global(&bc_total)); fork_join(&f, 0, g->numVertices);
  }
  { totalNedgeFunc f; fork_join_custom(&f); }

  VLOG(2) << "nedge_traversed: " << nedge_traversed;

  if (avg_centrality != NULL) *avg_centrality = bc_total / g->numVertices;
  if (total_nedge != NULL) *total_nedge = nedge_traversed;
  return t;
}
