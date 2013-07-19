#pragma once

#include <Communicator.hpp>
#include <Addressing.hpp>
#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <GlobalAllocator.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>
#include <Array.hpp>

#include <algorithm>

#include "common.h"

// #define USE_MPI3_COLLECTIVES
#undef USE_MPI3_COLLECTIVES
#ifdef USE_MPI3_COLLECTIVES
#include <mpi.h>
#endif

using namespace Grappa;

struct Vertex {
  int64_t * local_adj; // adjacencies that are local
  int64_t nadj;        // number of adjacencies
  int64_t local_sz;    // size of local allocation (regardless of how full it is)
  // int64_t parent;
  
  Vertex(): local_adj(nullptr), nadj(0), local_sz(0) {}    
  ~Vertex() {}
  
  template< typename F >
  void forall_adj(F body) {
    for (int64_t i=0; i<nadj; i++) {
      body(local_adj[i]);
    }
  }
  
  auto adj_iter() -> decltype(util::iterator(local_adj)) { return util::iterator(local_adj, nadj); }
};

// vertex with parent
struct VertexP : public Vertex {
  int64_t parent;
  VertexP(): Vertex(), parent(-1) {}
};

template< class Vertex >
struct Graph {
  static_assert(block_size % sizeof(Vertex) == 0, "Vertex size not evenly divisible into blocks!");
  
  // // Helpers (for if we go with custom cyclic distribution)
  // inline Core    vertex_owner (int64_t v) { return v % cores(); }
  // inline int64_t vertex_offset(int64_t v) { return v / cores(); }
  
  // Fields
  GlobalAddress<Vertex> vs;
  int64_t nv, nadj, nadj_local;
  
  // Internal fields
  int64_t * adj_buf;
  int64_t * scratch;
  
  GlobalAddress<Graph> self;
  
  char _pad[block_size - sizeof(vs)-sizeof(nv)-sizeof(nadj)-sizeof(nadj_local)-sizeof(adj_buf)-sizeof(scratch)-sizeof(self)];
  
  Graph(GlobalAddress<Graph> self, GlobalAddress<Vertex> vs, int64_t nv)
    : self(self)
    , vs(vs)
    , nv(nv)
    , nadj(0)
    , nadj_local(0)
    , adj_buf(nullptr)
    , scratch(nullptr)
  { }
  
  ~Graph() {
    for (Vertex& v : iterate_local(vs, nv)) { v.~Vertex(); }
    if (adj_buf) locale_free(adj_buf);
  }
    
  void destroy() {
    auto self = this->self;
    global_free(this->vs);
    call_on_all_cores([self]{ self->~Graph(); });
    global_free(self);
  }
  
  template< int LEVEL = 0 >
  static void dump(GlobalAddress<Graph> g) {
    for (int64_t i=0; i<g->nv; i++) {
      delegate::call(g->vs+i, [i](Vertex * v){
        std::stringstream ss;
        ss << "<" << i << ">";
        for (int64_t i=0; i<v->nadj; i++) ss << " " << v->local_adj[i];
        VLOG(LEVEL) << ss.str();
      });
    }
  }
  
  // Constructor
  static GlobalAddress<Graph> create(tuple_graph& tg) {
    double t;
    auto g = mirrored_global_alloc<Graph<Vertex>>();
  
    // find nv
        t = walltime();
    forall_localized(tg.edges, tg.nedge, [g](packed_edge& e){
      if (e.v0 > g->nv) { g->nv = e.v0; }
      else if (e.v1 > g->nv) { g->nv = e.v1; }
    });
    on_all_cores([g]{
      g->nv = Grappa::allreduce<int64_t,collective_max>(g->nv) + 1;
    });
        VLOG(1) << "find_nv_time: " << walltime() - t;
  
    auto vs = global_alloc<Vertex>(g->nv);
    auto self = g;
    on_all_cores([g,vs]{
      new (g.localize()) Graph(g, vs, g->nv);
      for (Vertex& v : iterate_local(g->vs, g->nv)) {
        new (&v) Vertex();
      }
    
  #ifdef SMALL_GRAPH
      // g->scratch = locale_alloc<int64_t>(g->nv);
      if (locale_mycore() == 0) g->scratch = locale_alloc<int64_t>(g->nv);
      barrier();
      if (locale_mycore() == 0) {
        memset(g->scratch, 0, sizeof(int64_t)*g->nv);
      } else {
        g->scratch = delegate::call(mylocale()*locale_cores(), [g]{ return g->scratch; });
      }
      VLOG(0) << "locale = " << mylocale() << ", scratch = " << g->scratch;
  #endif
    });
                                                              t = walltime();
    forall_localized(tg.edges, tg.nedge, [g](packed_edge& e){
      CHECK_LT(e.v0, g->nv); CHECK_LT(e.v1, g->nv);
  #ifdef SMALL_GRAPH
      // g->scratch[e.v0]++;
      // g->scratch[e.v1]++;
      __sync_fetch_and_add(g->scratch+e.v0, 1);
      __sync_fetch_and_add(g->scratch+e.v1, 1);
  #else    
      auto count = [](GlobalAddress<Vertex> v){
        delegate::call_async(*shared_pool, v.core(), [v]{ v->local_sz++; });
      };
      count(g->vs+e.v0);
      count(g->vs+e.v1);
  #endif
    });
    VLOG(1) << "count_time: " << walltime() - t;
  
  #ifdef SMALL_GRAPH
    t = walltime();  
  #ifdef USE_MPI3_COLLECTIVES
    on_all_cores([g]{
      MPI_Request r; int done;
      MPI_Iallreduce(MPI_IN_PLACE, g->scratch, g->nv, MPI_INT64_T, MPI_SUM, MPI_COMM_WORLD, &r);
      do {
        MPI_Test( &r, &done, MPI_STATUS_IGNORE );
        if(!done) { Grappa_yield(); }
      } while(!done);
    });
  #else
    on_all_cores([g]{ allreduce_inplace<int64_t,collective_add>(g->scratch, g->nv); });
  #endif // USE_MPI3_COLLECTIVES
    VLOG(1) << "allreduce_inplace_time: " << walltime() - t;  
    // on_all_cores([g]{ VLOG(5) << util::array_str("scratch", g->scratch, g->nv, 25); });
  #endif // SMALL_GRAPH  
  
    // allocate space for each vertex's adjacencies (+ duplicates)
    forall_localized(g->vs, g->nv, [g](int64_t i, Vertex& v) {
  #ifdef SMALL_GRAPH
      // adjust b/c allreduce didn't account for having 1 instance per locale
      v.local_sz = g->scratch[i] / locale_cores();
  #endif
    
      v.nadj = 0;
      if (v.local_sz > 0) v.local_adj = new int64_t[v.local_sz];
    });
    VLOG(3) << "after adj allocs";
  
    // scatter
    forall_localized(tg.edges, tg.nedge, [g](packed_edge& e){
      auto scatter = [g](int64_t vi, int64_t adj) {
        auto vaddr = g->vs+vi;
        delegate::call_async(*shared_pool, vaddr.core(), [vaddr,adj]{
          auto& v = *vaddr.pointer();
          v.local_adj[v.nadj++] = adj;
        });
      };
      scatter(e.v0, e.v1);
      scatter(e.v1, e.v0);
    });
    VLOG(3) << "after scatter, nv = " << g->nv;
  
    // sort & de-dup
    forall_localized(g->vs, g->nv, [g](int64_t vi, Vertex& v){
      CHECK_EQ(v.nadj, v.local_sz);
      std::sort(v.local_adj, v.local_adj+v.nadj);
    
      int64_t tail = 0;
      for (int64_t i=0; i<v.nadj; i++, tail++) {
        v.local_adj[tail] = v.local_adj[i];
        while (v.local_adj[tail] == v.local_adj[i+1]) i++;
      }
      v.nadj = tail;
      // VLOG(0) << "<" << vi << ">" << util::array_str("", v.local_adj, v.nadj);
      g->nadj_local += v.nadj;
    });
    VLOG(3) << "after sort";
  
    // compact
    on_all_cores([g]{
  #ifdef SMALL_GRAPH
      if (locale_mycore() == 0) locale_free(g->scratch);
  #endif
    
      VLOG(2) << "nadj_local = " << g->nadj_local;
    
      // allocate storage for local vertices' adjacencies
      g->adj_buf = locale_alloc<int64_t>(g->nadj_local);
      // compute total nadj
      g->nadj = allreduce<int64_t,collective_add>(g->nadj_local);
    
      int64_t * adj = g->adj_buf;
      for (Vertex& v : iterate_local(g->vs, g->nv)) {
        Grappa::memcpy(adj, v.local_adj, v.nadj);
        if (v.local_sz > 0) delete[] v.local_adj;
        v.local_sz = v.nadj;
        v.local_adj = adj;
        adj += v.nadj;
      }
      CHECK_EQ(adj - g->adj_buf, g->nadj_local);
    });
  
    return g;
  }
  
};
