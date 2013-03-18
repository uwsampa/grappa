// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include "defs.hpp"
#include <Grappa.hpp>
#include <ForkJoin.hpp>
#include <GlobalAllocator.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>
#include <Collective.hpp>
#include <common.hpp>
#include <Cache.hpp>
#include <GlobalTaskJoiner.hpp>
#include <PushBuffer.hpp>
#include <Array.hpp>
#include <Statistics.hpp>

#include "../compat/mersenne.h"

using namespace Grappa;

static inline double read_double(GlobalAddress<double> addr) {
  int64_t temp = Grappa_delegate_read_word(addr);
  return *reinterpret_cast<double*>(&temp);
}

namespace local {
  
  template< typename T >
  inline T read(T* target) {
    return *target;
  }
  
  template< typename T, typename U >
  inline void write(T* target, U value) {
    *target = value;
  }
  
  template< typename T, typename U, typename V >
  bool compare_and_swap(T* target, U cmp_val, V new_val) {
    if (cmp_val == *p) {
      *p = new_val;
      return true;
    } else {
      return false;
    }
  }
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

static double local_centrality_total;

static int64_t nedge_traversed;

static PushBuffer<int64_t> Qbuf;

void do_bfs_push(graphint d_phase_, int64_t start, int64_t end) {
  
  call_on_all_cores([d_phase_]{
    Qbuf.setup(c.Q, c.Qnext);
    d_phase = d_phase_;
  });

  forall_here(start, end-start, [](int64_t s, int64_t n){
  for (int64_t i=s; i<s+n; i++) {
    graphint& v = c.Q[i];
    CHECK(v < g.numVertices) << v << " < " << g.numVertices;
    
    int64_t bufEdgeStart[2];
    Incoherent<int64_t>::RO cr(g.edgeStart+v, 2, bufEdgeStart);

    const graphint vstart = cr[0];
    const graphint vend = cr[1];
    CHECK(v < (1L<<32)) << "can't pack 'v' into 32-bit value! have to get more creative";
    CHECK(vend < (1L<<32)) << "can't pack 'vo' into 32-bit value! have to get more creative";
 
    nedge_traversed += vend - vstart;
    DVLOG(3) << "visit (" << i << "): " << vstart << " -> " << vend;
    
    forall_here_async(vstart, vend-vstart, [v](int64_t vs, int64_t kiters) {
      // TODO: overlap these      
      graphint sigmav = local::read(c.sigma+v);
      graphint vStart = local::read(g.edgeStart+v);
      graphint buf_eV[kiters];
      Incoherent<graphint>::RO ceV(g.endVertex+vs, kiters, buf_eV);

      for (int64_t k=0; k<kiters; k++) {
        graphint w = ceV[k];
        graphint d = local::read(c.dist+w);
        
        // If node has not been visited, set distance and push on Q (but only once)
        if (d < 0) {
          if (local::compare_and_swap(c.marks+w, 0, 1)) {
            local::write(c.dist+w, d_phase);
            Qbuf.push(w);
          }
        }
        if (d < 0 || d == d_phase) {
          c.sigma[w] += sigmav;
        }
      }
    });
  }});

  on_all_cores([] { Qbuf.flush(); });
}

void do_bfs_pop(graphint start, graphint end) {
  forall_here(start, end-start, []{
    graphint& v = c.Q[i];
    
    int64_t bufEdgeStart[2];
    Incoherent<int64_t>::RO cr(g.edgeStart+v, 2, bufEdgeStart);
    const graphint myStart = cr[0];
    const graphint myEnd = cr[1];    
    
    nedge_traversed += myEnd-myStart;
    DVLOG(4) << "pop " << v << " (" << i << ")";
    
    // pop children, TODO: try with very coarse decomp, cache large blocks at a time
    forall_here(myStart, myEnd-myStart, [](int64_t kstart, int64_t kiters){
      
      int64_t sigma_v = local::read(c.sigma+v);

      double sum = 0;
      
      graphint buf_eV[kiters];
      Incoherent<graphint>::RO ceV(g.endVertex+vs, kiters, buf_eV);
      for (graphint k=0; k<kiters; k++) {
        graphint w = ceV[k];        
        sum += ((double)sigma_v) * (1.0 + local::read(c.delta+w)) / (double)local::read(c.sigma+w);
      }
  
      DVLOG(4) << "v(" << v << ")[" << kstart << "," << kstart+kiters << "] sum: " << sum;
      c.delta[v] += sum;
    });
  });
  
  DVLOG(4) << "###################";
  
  // TODO: do GASNet reduction
  auto pool_sz = (end-start)*sizeof(delegate::write_msg_proxy<double>)
  auto * pool_buf = new char[pool_sz];
  MessagePool pool(pool_buf, pool_sz);
  for (graphint j=start; j<end; j++) {
    graphint v = c.Q[j];
    delegate::increment_async(pool, bc+v, d);    
  }
  
}

//////////////////////
// Profiling stuff
/////////////////////
static void enable_tau() {
#ifdef GRAPPA_TRACE
  VLOG(1) << "Enabling TAU recording.";
  call_on_all_cores([]{ FLAGS_record_grappa_events = true; });
#endif
#ifdef GOOGLE_PROFILER
  call_on_all_cores([]{ Grappa_start_profiling(); });
#else
  call_on_all_cores([]{ Statistics::reset(); });
#endif
}
static void disable_tau() {
#ifdef GRAPPA_TRACE
  VLOG(1) << "Disabling TAU recording.";
  call_on_all_cores([]{ FLAGS_record_grappa_events = false; });
#endif
#ifdef GOOGLE_PROFILER
  call_on_all_cores([]{ Grappa_stop_profiling(); });
#else
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
  
  c.delta       = new double[g_in->numVertices];
  c.dist        = new graphint[g_in->numVertices];
  c.Q           = new graphint[g_in->numVertices];
  c.sigma       = new graphint[g_in->numVertices];
  c.marks       = new graphint[g_in->numVertices+2];
  // c.child       = new graphint[g_in->numEdges];
  // c.child_count = new graphint[g_in->numVertices];
  if (!computeAllVertices) c.explored = Grappa_typed_malloc<graphint>g_in->numVertices);
  c.Qnext       = make_global(&Qnext);
  
  double t; t = timer();
  double rngtime, tt;

  enable_tau();

  { // initialize globals on all cores
    auto _g = *g_in;
    auto c_explored = c.explored;
    call_on_all_cores([_g,bc_in,c_explored]{
      nedge_traversed = 0;
      g = _g;
      // c = _c;
      bc = bc_in;
      
      c.delta       = new double[g_in->numVertices];
      c.dist        = new graphint[g_in->numVertices];
      c.Q           = new graphint[g_in->numVertices];
      c.sigma       = new graphint[g_in->numVertices];
      c.marks       = new graphint[g_in->numVertices+2];
      // c.child       = new graphint[g_in->numEdges];
      // c.child_count = new graphint[g_in->numVertices];
      if (!computeAllVertices) c.explored = c_explored;
      c.Qnext       = make_global(&Qnext);
    });
  }
  
  Grappa::memset(bc, 0.0, g.numVertices);
  if (!computeAllVertices) Grappa::memset(c.explored, (graphint)0L, g.numVertices);
  
  mersenne_seed(12345);

  on_all_cores([]{
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
        } while (!local_compare_and_swap(c.explored+s, 0L, 1L));
      }
      rngtime += timer() - tt;
    
      
    
  });

    graphint pair_[2];
    Incoherent<graphint>::RO pair(g.edgeStart+s, 2, pair_);
    
    VLOG(1) << "degree (" << pair[1]-pair[0] << ")";

    if (pair[0] == pair[1]) {
      continue;
    } else {
      Vs--;
    }
    
    VLOG(3) << "s = " << s;
    
    on_all_cores([]{
      memset(c.dist,  (graphint)-1, g.numVertices);
      memset(c.sigma, (graphint) 0, g.numVertices);
      memset(c.marks, (graphint) 0, g.numVertices);
      memset(c.delta,   (double) 0, g.numVertices);      
    });
    
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
    DVLOG(1) << "pushing d_phase(" << d_phase << ") " << Qstart << " -> " << Qend;
    do_bfs_push(d_phase, Qstart, Qend);
    
    // If new nodes pushed onto Q
    if (Qnext != QHead[nQ]) {
      nQ++;
      QHead[nQ] = Qnext;
      goto PushOnStack;
    }
    
    // Dependence accumulation phase
    nQ--;
    VLOG(3) << "nQ = " << nQ;

    Grappa::memset(c.delta, 0.0, g.numVertices);
    
    // Pop nodes off of Q in the reverse order they were pushed on
    for ( ; nQ > 1; nQ--) {
      Qstart = QHead[nQ-1];
      Qend = QHead[nQ];
      d_phase--;
      DVLOG(1) << "popping d_phase(" << d_phase << ") " << Qstart << " -> " << Qend;
      
      do_bfs_pop(Qstart, Qend);      
      
    }
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
  Core origin = mycore();
  // TODO: use array reduction op, or mutable "forall_localized"-held state
  on_all_cores([bc, &bc_total, origin]{
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
