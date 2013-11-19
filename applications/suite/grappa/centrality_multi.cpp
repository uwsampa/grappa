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
    if (cmp_val == *target) {
      *target = new_val;
      return true;
    } else {
      return false;
    }
  }
  
  template< typename T, typename U >
  inline void memset(T* array, const U& value, size_t nelem) {
    for (size_t i=0; i<nelem; i++) array[i] = value;
  }
}

inline std::string graph_str(graph* g) {
  std::stringstream ss;
  for (graphint i=0; i<g->numVertices; i++) {
    graphint rstart = delegate::read(g->edgeStart+i);
    graphint rend = delegate::read(g->edgeStart+i+1);
    ss << "<" << i << ">(deg:" << rend-rstart << "): ";
    for (graphint j=rstart; j<rend; j++) {
      graphint v = delegate::read(g->endVertex+j);
      ss << v << " ";
    }
    ss << "\n";
  }
  return ss.str();
}


struct CentralityScratch {
  double * delta;
  double * bctemp;
  GlobalAddress<graphint> explored;
  graphint * dist;
  graphint * Q;
  graphint * sigma;
  graphint * marks;
  graphint * Qnext;
};

static graph g;
static CentralityScratch c;
// static GlobalAddress<double> bc;
static graphint d_phase;
static graphint local_Qnext;

static double local_centrality_total;

static int64_t nedge_traversed;

// static PushBuffer<int64_t> Qbuf;

void do_bfs_push_multi(graphint d_phase_, int64_t start, int64_t end) {
  d_phase = d_phase_;
  CompletionEvent ce;
  forall_here(start, end-start, [&ce](int64_t s, int64_t n){
    for (int64_t i=s; i<s+n; i++) {
      graphint& v = c.Q[i];
      CHECK(v < g.numVertices) << v << " < " << g.numVertices;
      
      graphint bufEdgeStart[2];
      Incoherent<graphint>::RO cr(g.edgeStart+v, 2, bufEdgeStart);
      const graphint vstart = cr[0];
      const graphint vend = cr[1];
      CHECK(v < (1L<<32)) << "can't pack 'v' into 32-bit value! have to get more creative";
      CHECK(vend < (1L<<32)) << "can't pack 'vo' into 32-bit value! have to get more creative";
 
      nedge_traversed += vend - vstart;
      DVLOG(3) << "visit (" << i << "): " << vstart << " -> " << vend;
      
      ce.enroll(vend-vstart);
      forall_here_async(vstart, vend-vstart, [v,&ce](int64_t vs, int64_t kiters) {
        // TODO: overlap these      
        graphint sigmav = local::read(c.sigma+v);
        graphint vStart = delegate::read(g.edgeStart+v);
        graphint buf_eV[kiters];
        Incoherent<graphint>::RO ceV(g.endVertex+vs, kiters, buf_eV);

        for (int64_t k=0; k<kiters; k++) {
          graphint w = ceV[k];
          graphint d = local::read(c.dist+w);
        
          // If node has not been visited, set distance and push on Q (but only once)
          if (d < 0) {
            if (local::compare_and_swap(c.marks+w, 0, 1)) {
              local::write(c.dist+w, d_phase);
              c.Q[(*c.Qnext)++] = w;
            }
          }
          if (d < 0 || d == d_phase) {
            c.sigma[w] += sigmav;
          }
        }
        ce.complete(kiters);
      });
    }
  });
  ce.wait();
}

void do_bfs_pop_multi(graphint start, graphint end) {
  CompletionEvent ce;
  forall_here(start, end-start, [&ce](int64_t s, int64_t n){
    for (int64_t j=s; j<s+n; j++) {    
      graphint& v = c.Q[j];
      
      int64_t bufEdgeStart[2];
      Incoherent<int64_t>::RO cr(g.edgeStart+v, 2, bufEdgeStart);
      const graphint myStart = cr[0];
      const graphint myEnd = cr[1];    
      
      nedge_traversed += myEnd-myStart;
      DVLOG(4) << "pop " << v << " (" << j << ")";
      
      // pop children, TODO: try with very coarse decomp, cache large blocks at a time
      ce.enroll(myEnd-myStart);
      // TODO: make sure lambda's not being heap-allocated?
      forall_here_async(myStart, myEnd-myStart, [v,&ce](int64_t kstart, int64_t kiters){
      
        int64_t sigma_v = local::read(c.sigma+v);
        
        double sum = 0;
        
        graphint buf_eV[kiters];
        Incoherent<graphint>::RO ceV(g.endVertex+kstart, kiters, buf_eV);
        for (graphint k=0; k<kiters; k++) {
          graphint w = ceV[k];
          if (c.dist[w] != c.dist[v]+1) continue;
          sum += ((double)sigma_v) * (1.0 + local::read(c.delta+w)) / (double)local::read(c.sigma+w);
        }
  
        DVLOG(4) << "v(" << v << ")[" << kstart << "," << kstart+kiters << "] sum: " << sum;
        c.delta[v] += sum;
        
        ce.complete(kiters);
      });
    }
  });
  ce.wait();
  
  DVLOG(4) << "###################";
  
  // // TODO: do GASNet reduction
  // auto pool_sz = (end-start)*sizeof(delegate::write_msg_proxy<double>);
  // auto * pool_buf = new char[pool_sz];
  // MessagePool pool(pool_buf, pool_sz);
  // for (graphint j=start; j<end; j++) {
  //   graphint v = c.Q[j];
  //   delegate::increment_async(pool, bc+v, d);    
  // }
  // 
}

//////////////////////
// Profiling stuff
/////////////////////
static void enable_tau() {
#ifdef GRAPPA_TRACE
  VLOG(1) << "Enabling TAU recording.";
  call_on_all_cores([]{ FLAGS_record_grappa_events = true; });
#endif
  Statistics::reset_all_cores();
  Statistics::start_tracing();
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
/// graph using 'total_num_roots' source vertices. Returns the average centrality.
double centrality_multi(graph *g_in, GlobalAddress<double> bc, graphint total_num_roots,
    /* outputs: */ double * avg_centrality = NULL, int64_t * total_nedge = NULL) {

  c.explored = Grappa_typed_malloc<graphint>(g_in->numVertices);  
  
  DVLOG(2) << graph_str(g_in);
  
  double t; t = timer();
  double rngtime, tt;

  enable_tau();

  { // initialize globals on all cores
    auto _g = *g_in;
    auto c_explored = c.explored;
    call_on_all_cores([_g,c_explored]{
      nedge_traversed = 0;
      g = _g;
      
      c.delta    = locale_alloc<double>(g.numVertices);
      c.bctemp    = locale_alloc<double>(g.numVertices);
      c.dist     = locale_alloc<graphint>(g.numVertices);
      c.Q        = locale_alloc<graphint>(g.numVertices);
      c.sigma    = locale_alloc<graphint>(g.numVertices);
      c.marks    = locale_alloc<graphint>(g.numVertices+2);
      c.explored = c_explored;
    });
  }
  
  Grappa::memset(bc, 0.0, g.numVertices);
  Grappa::memset(c.explored, (graphint)0L, g.numVertices);

  // pick roots
  
  mersenne_seed(12345);
  
  std::deque<graphint> root_vertices;
  // graphint root_vertices[total_num_roots];
  for (graphint i=0; i<total_num_roots; i++) {
    graphint root_vertex;
    do {
      root_vertex = mersenne_rand() % g.numVertices;
      VLOG(2) << "root_vertex (" << root_vertex << ")";
    } while (!delegate::compare_and_swap(c.explored+root_vertex, 0L, 1L));
    // rngtime += timer() - tt;

    graphint pair_[2];
    Incoherent<graphint>::RO pair(g.edgeStart+root_vertex, 2, pair_);
    graphint root_degree = pair[1]-pair[0];
    VLOG(2) << "degree (" << root_degree << ")";
    
    if (root_degree == 0) {
      i--; // try again...
    } else {
      root_vertices.push_back(root_vertex);
    }
  }
  // graphint num_roots_todo = total_num_roots;
  // auto roots_todo_addr = make_global(&num_roots_todo);
  // graphint root_index = 0;
  auto root_vertices_addr = make_global(&root_vertices);
  
  on_all_cores([root_vertices_addr]{
    
    local::memset(c.bctemp, (double)0, g.numVertices);
    
    graphint Qnext;
    c.Qnext = &Qnext;
    graphint QHead[100 * SCALE];
    graphint nQ, d_phase, Qstart, Qend;
    
    // start doing centrality traversals
    // for (graphint x = 0; (x < g.numVertices) && (total_num_roots > 0); x++) {
    while (true) { // (breaks out when roots_todo -> 0)
      
      /// Choose vertex at random
      graphint root_vertex = delegate::call(root_vertices_addr.core(), [root_vertices_addr]() -> graphint {
        auto& rv = *root_vertices_addr.pointer();
        if (rv.empty()) {
          return -1;
        } else {
          graphint v = rv.front();
          root_vertices_addr.pointer()->pop_front();
          return v;
        }
      });
      if (root_vertex == -1) break;
      
      VLOG(1) << "root_vertex = " << root_vertex;
    
      local::memset(c.dist,  (graphint)-1, g.numVertices);
      local::memset(c.sigma, (graphint) 0, g.numVertices);
      local::memset(c.marks, (graphint) 0, g.numVertices);
      local::memset(c.delta,   (double) 0, g.numVertices);
      
      // Push node i onto Q and set bounds for first Q sublist
      local::write(c.Q+0, root_vertex);
      Qnext = 1;
      nQ = 1;
      QHead[0] = 0;
      QHead[1] = 1;
      local::write(c.dist+root_vertex, 0);
      local::write(c.marks+root_vertex, 1);
      local::write(c.sigma+root_vertex, 1);
      
      PushOnStack: // push nodes onto Q
      
      // Execute the nested loop for each node v on the Q AND
      // for each neighbor w of v whose edge weight is not divisible by 8
      d_phase = nQ;
      Qstart = QHead[nQ-1];
      Qend = QHead[nQ];
      DVLOG(2) << "pushing d_phase(" << d_phase << ") " << Qstart << " -> " << Qend;
      
      DVLOG(3) << util::array_str("Q", c.Q+Qstart, Qend-Qstart);
      
      do_bfs_push_multi(d_phase, Qstart, Qend);
  
      // If new nodes pushed onto Q
      if (Qnext != QHead[nQ]) {
        nQ++;
        QHead[nQ] = Qnext;
        goto PushOnStack;
      }
    
      // Dependence accumulation phase
      nQ--;
      VLOG(3) << "nQ = " << nQ;

      // local::memset(c.delta, 0.0, g.numVertices);
  
      // Pop nodes off of Q in the reverse order they were pushed on
      for ( ; nQ > 1; nQ--) {
        Qstart = QHead[nQ-1];
        Qend = QHead[nQ];
        d_phase--;
        DVLOG(1) << "popping d_phase(" << d_phase << ") " << Qstart << " -> " << Qend;
    
        do_bfs_pop_multi(Qstart, Qend);      
    
      }
      
      for (int i=0; i<g.numVertices; i++) {
        c.bctemp[i] += c.delta[i];
      }
        
      // DVLOG(2) << util::array_str("delta", c.delta, g.numVertices);
    } // (for each starting vertex)
    
  });
  
  // on_all_cores([]{
  //   DVLOG(2) << util::array_str("delta", c.delta, g.numVertices);
  // });
  
  // all-reduce everyone's deltas
  on_all_cores([]{ allreduce_inplace<double,collective_add>(c.bctemp, g.numVertices); });
  // put them into the global "centrality" array
  forall_localized(bc, g.numVertices, [](int64_t i, double& e){
    e = c.bctemp[i];
  });
  
  DVLOG(3) << util::array_str("bc", bc, g.numVertices, 20);
  
  t = timer() - t;
  disable_tau();

  // VLOG(1) << "centrality rngtime = " << rngtime;

  on_all_cores([]{
    locale_free(c.delta);
    locale_free(c.bctemp);
    locale_free(c.dist);
    locale_free(c.Q);
    locale_free(c.sigma);
    locale_free(c.marks);
  });
  
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
