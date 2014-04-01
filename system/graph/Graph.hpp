#pragma once

#include <Communicator.hpp>
#include <Addressing.hpp>
#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <GlobalAllocator.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>
#include <Array.hpp>
#include "TupleGraph.hpp"

#include <algorithm>
#include <iomanip>

// #define USE_MPI3_COLLECTIVES
#undef USE_MPI3_COLLECTIVES
#ifdef USE_MPI3_COLLECTIVES
#include <mpi.h>
#endif

namespace Grappa {
  /// @addtogroup Graph
  /// @{
  
  /// Currently just an overload for int64, may someday be used for distinguishing parameters in forall().
  using VertexID = int64_t;
  

  namespace impl {
    
    struct VertexBase {
      VertexID * local_adj; // adjacencies that are local
      int64_t nadj;        // number of adjacencies
      int64_t local_sz;    // size of local allocation (regardless of how full it is)
      
      VertexBase(): local_adj(nullptr), nadj(0), local_sz(0) {}
      
      VertexBase(const VertexBase& v):
        local_adj(v.local_adj),
        nadj(v.nadj),
        local_sz(v.local_sz)
      { }
    };
    
    ///////////////////////////////////////////////////////////////
    /// Vertex with customizable inline 'data' field. Will attempt 
    /// to pack the provided type into the block-aligned Vertex 
    /// class, but if it is too large, will heap-allocate (from 
    /// locale-shared heap). Defines a '->' operator to access data 
    /// fields.
    ///
    /// Example subclasses:
    /// @code
    /// ////////////////////////
    /// // Vertex with parent
    ///
    /// // Preferred:
    /// struct Parent { int64_t parent; };
    /// 
    /// Vertex<Parent> p;
    /// p->parent = -1;
    ///
    /// struct VertexP : public Vertex<int64_t> {
    ///   VertexP(): Vertex() { parent(-1); }
    ///   int64_t parent() { return data; }
    ///   void parent(int64_t parent) { data = parent; }
    /// };
    /// @endcode
    template< typename T = int64_t, typename E = double, bool HeapData = (sizeof(T) > BLOCK_SIZE-sizeof(int64_t)*2-sizeof(int64_t*)) >
    struct Vertex : public VertexBase {
      T data;
      E * local_edge_state;
    
      Vertex(): VertexBase(), data() {}
      Vertex(const VertexBase& v): VertexBase(v), data() {}
      ~Vertex() {}
    
      T* operator->() { return &data; }
    
    } GRAPPA_BLOCK_ALIGNED;
  
    template< typename T, typename E >
    struct Vertex<T, E, /*HeapData = */ true> : public VertexBase {
      T& data;
      E * local_edge_state;
    
      Vertex(): VertexBase(), data(*locale_alloc<T>()) {}
      Vertex(const VertexBase& v): VertexBase(v), data(*locale_alloc<T>()) {}
    
      ~Vertex() { locale_free(&data); }
    
      T* operator->() { return &data; }
    
    } GRAPPA_BLOCK_ALIGNED;
  
  }
  
  
  template< typename V, typename E >
  struct Graph {
    
    using Vertex = impl::Vertex<V,E>;
    using EdgeState = E;
    
    struct Edge {
      VertexID id; ///< Global index of adjacent vertex
      GlobalAddress<Vertex> ga; ///< Global address to adjacent vertex
      EdgeState& data;
      
      /// Access elements of EdgeState with operator '->'
      EdgeState* operator->(){ return &data; }
    };
    
    static_assert(block_size % sizeof(Vertex) == 0, "V size not evenly divisible into blocks!");
    
    // using Vertex = V;
    
    // // Helpers (for if we go with custom cyclic distribution)
    // inline Core    vertex_owner (int64_t v) { return v % cores(); }
    // inline int64_t vertex_offset(int64_t v) { return v / cores(); }
  
    // Fields
    GlobalAddress<Vertex> vs;
    int64_t nv, nadj, nadj_local;
  
    // Internal fields
    VertexID * adj_buf;
    EdgeState * edge_storage;
    
    // Temporary internal state
    void* scratch;
    
    GlobalAddress<Graph> self;
    
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
      if (edge_storage) {
        for (int64_t i=0; i<nadj_local; i++) {
          edge_storage[i].~E();
        }
        locale_free(edge_storage);
      }
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
        delegate::call(g->vs+i, [i](Vertex& v){
          std::stringstream ss;
          ss << "<" << i << ">";
          for (int64_t i=0; i<v.nadj; i++) ss << " " << v.local_adj[i];
          VLOG(LEVEL) << ss.str();
        });
      }
    }
    
    template< int LEVEL = 0, typename F = nullptr_t >
    void dump(F print_vertex) {
      for (int64_t i=0; i<nv; i++) {
        delegate::call(vs+i, [i,print_vertex](Vertex& v){
          std::stringstream ss;
          ss << "<" << std::setw(2) << i << ">";
          print_vertex(ss, v);
          for (int64_t i=0; i<v.nadj; i++) ss << " " << v.local_adj[i];
          VLOG(LEVEL) << ss.str();
        });
      }
    }
  
    /// Change the data associated with each vertex, keeping the same connectivity 
    /// structure and allowing the user to intialize the new data type using the old vertex.
    ///
    /// @param F f: void (Vertex<OldData>& v, NewData& d) {}
    ///
    /// Example:
    /// @code
    /// struct A { int64_t parent; }
    /// GlobalAddress<Graph<A>> g = ...;
    /// struct B { double value; };
    /// auto gnew = g->transform<B>([](Graph<A>::Vertex& v, B& b){
    ///   b.value = v->weight / v.nadj;
    /// });
    /// @endcode
    template< typename VV, typename F = decltype(nullptr) >
    GlobalAddress<Graph<VV,E>> transform(F f) {
      forall(vs, nv, [f](Vertex& v){
        VV d;
        f(v, d);
        v.~Vertex();
        Vertex b = v;
        auto vv = new (&v) typename Graph<VV,E>::Vertex(b);
        vv->data = d;
        vv->local_edge_state = b.local_edge_state;
      });
      return static_cast<GlobalAddress<Graph<VV,E>>>(self);
    }
    
    // Constructor
    static GlobalAddress<Graph> create(const TupleGraph& tg, bool directed = false) {
      double t;
      auto g = symmetric_global_alloc<Graph>();
      
      // find nv
          t = walltime();
      forall(tg.edges, tg.nedge, [g](TupleGraph::Edge& e){
        if (e.v0 > g->nv) { g->nv = e.v0; }
        if (e.v1 > g->nv) { g->nv = e.v1; }
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
      // count the outgoing/undirected edges per vertex
      forall(tg.edges, tg.nedge, [g,directed](TupleGraph::Edge& e){
        CHECK_LT(e.v0, g->nv); CHECK_LT(e.v1, g->nv);
    #ifdef SMALL_GRAPH
        // g->scratch[e.v0]++;
        // if (!directed) g->scratch[e.v1]++;
        __sync_fetch_and_add(g->scratch+e.v0, 1);
        if (!directed) __sync_fetch_and_add(g->scratch+e.v1, 1);
    #else    
        auto count = [](GlobalAddress<Vertex> v){
          delegate::call<SyncMode::Async>(v.core(), [v]{ v->local_sz++; });
        };
        count(g->vs+e.v0);
        if (!directed) count(g->vs+e.v1);
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
          if(!done) { Grappa::yield(); }
        } while(!done);
      });
    #else
      on_all_cores([g]{ allreduce_inplace<int64_t,collective_add>(g->scratch, g->nv); });
    #endif // USE_MPI3_COLLECTIVES
      VLOG(1) << "allreduce_inplace_time: " << walltime() - t;  
      // on_all_cores([g]{ VLOG(5) << util::array_str("scratch", g->scratch, g->nv, 25); });
    #endif // SMALL_GRAPH  
  
      // allocate space for each vertex's adjacencies (+ duplicates)
      forall(g->vs, g->nv, [g](int64_t i, Vertex& v) {
    #ifdef SMALL_GRAPH
        // adjust b/c allreduce didn't account for having 1 instance per locale
        v.local_sz = g->scratch[i] / locale_cores();
    #endif
    
        v.nadj = 0;
        if (v.local_sz > 0) v.local_adj = new VertexID[v.local_sz];
      });
      VLOG(3) << "after adj allocs";
  
      // scatter
      forall(tg.edges, tg.nedge, [g,directed](TupleGraph::Edge& e){
        auto scatter = [g](int64_t vi, int64_t adj) {
          auto vaddr = g->vs+vi;
          delegate::call<SyncMode::Async>(vaddr.core(), [vaddr,adj]{
            auto& v = *vaddr.pointer();
            v.local_adj[v.nadj++] = adj;
          });
        };
        scatter(e.v0, e.v1);
        if (!directed) scatter(e.v1, e.v0);
      });
      VLOG(3) << "after scatter, nv = " << g->nv;
  
      // sort & de-dup
      forall(g->vs, g->nv, [g](int64_t vi, Vertex& v){
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
        g->adj_buf = locale_alloc<VertexID>(g->nadj_local);
        g->edge_storage = locale_alloc<EdgeState>(g->nadj_local);
        
        // default-initialize edges
        // TODO: import edge info from TupleGraph
        for (size_t i=0; i<g->nadj_local; i++) {
          new (g->edge_storage+i) EdgeState();
        }
        
        // compute total nadj
        g->nadj = allreduce<int64_t,collective_add>(g->nadj_local);
        
        size_t offset = 0;
        for (Vertex& v : iterate_local(g->vs, g->nv)) {
          auto adj = g->adj_buf + offset;
          Grappa::memcpy(adj, v.local_adj, v.nadj);
          if (v.local_sz > 0) delete[] v.local_adj;
          v.local_sz = v.nadj;
          v.local_adj = adj;
          v.local_edge_state = g->edge_storage+offset;
          offset += v.nadj;
        }
        CHECK_EQ(offset, g->nadj_local);
      });
  
      return g;
    }
  
  } GRAPPA_BLOCK_ALIGNED;
  
  ////////////////////////////////////////////////////
  // Vertex iterators
  
  template< typename G >
  struct AdjIterator {
    GlobalAddress<G> g;
    VertexID i;
    AdjIterator(GlobalAddress<G> g, VertexID i): g(g), i(i) {}
  };
  
  /// Iterator over adjacent vertices. Used with Grappa::forall().
  template< typename G >
  AdjIterator<G> adj(GlobalAddress<G> g, typename G::Vertex& v) {
    return AdjIterator<G>(g, make_linear(&v) - g->vs);
  }
  
  template< typename G >
  AdjIterator<G> adj(GlobalAddress<G> g, GlobalAddress<typename G::Vertex> v) {
    return AdjIterator<G>(g, v - g->vs);
  }
  
  template< typename G >
  AdjIterator<G> adj(GlobalAddress<G> g, VertexID i) { return AdjIterator<G>(g, i); }  
  
  namespace impl {
    template< SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(AdjIterator<G> a, F body,
                void (F::*mf)(int64_t,typename G::Edge&) const)
    {
      if (C) C->enroll();
      auto origin = mycore();
      
      auto loop = [a,origin,body]{
        auto vs = a.g->vs;
        auto v = (vs+a.i).pointer();
        Grappa::forall_here<S,C,Threshold>(0, v->nadj, [body,v,vs](int64_t i){
          auto j = v->local_adj[i];
          typename G::Edge e = { j, vs+j, v->local_edge_state[i] };
          body(i, e);
        });
        if (C) C->send_completion(origin);
      };
      
      auto v = a.g->vs+a.i;
      
      if (v.core() == mycore()) {
        loop();
      } else {
        if (S == SyncMode::Async) {
          spawnRemote<nullptr>(v.core(), [loop]{ loop(); });
        } else {
          CompletionEvent ce(1);
          auto ce_a = make_global(&ce);
          spawnRemote<nullptr>(v.core(), [loop,ce_a]{
            loop();
            complete(ce_a);
          });
          ce.wait();
        }
      }
    }
    template< SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(AdjIterator<G> a, F body, void (F::*mf)(int64_t) const) {
      auto f = [body](int64_t i, typename G::Edge& e){ body(i); };
      impl::forall<S,C,Threshold>(a, f, &decltype(f)::operator());
    }
    template< SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(AdjIterator<G> a, F body, void (F::*mf)(typename G::Edge&) const) {
      auto f = [body](int64_t i, typename G::Edge& e){ body(e); };
      impl::forall<S,C,Threshold>(a, f, &decltype(f)::operator());
    }
  }
  
#define OVERLOAD(...) \
  template< __VA_ARGS__, typename G = nullptr_t, typename F = nullptr_t > \
  void forall(AdjIterator<G> a, F body) { \
    impl::forall<S,C,Threshold>(a, body, &F::operator()); \
  }
  /// Parallel loop over adjacent vertices. Use adj() to construct iterator
  OVERLOAD( SyncMode S = SyncMode::Blocking,
            GlobalCompletionEvent * C = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG );
  OVERLOAD( GlobalCompletionEvent * C,
            SyncMode S = SyncMode::Blocking,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG );
#undef OVERLOAD
  
  ////////////////////////////////////////////////////
  // Graph iterators
  
  namespace impl {
        
    /// Iterate over all vertices, with vertex index
    template< GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(GlobalAddress<G> g, F body,
                void (F::*mf)(VertexID,typename G::Vertex&) const) {
      Grappa::forall<C,Threshold>(g->vs, g->nv, [body](int64_t i, typename G::Vertex& v){
        body(i, v);
      });
    }
    
    /// Iterate over all vertices
    template< GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(GlobalAddress<G> g, F body, void (F::*mf)(typename G::Vertex&) const) {
      auto f = [body](VertexID i, typename G::Vertex& v){ body(v); };
      impl::forall<C,Threshold>(g, f, &decltype(f)::operator());
    }
    
    /// Iterate over all adjacencies of all vertices in parallel
    template< GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(GlobalAddress<G> g, F loop_body,
                void (F::*mf)(typename G::Vertex& src, typename G::Edge& adj) const) {
      auto f = [g,loop_body](VertexID i, typename G::Vertex& v){
        Grappa::forall<SyncMode::Async,C,Threshold>(adj(g,v),
            [&v,loop_body](typename G::Edge& e){
          loop_body(v, e);
        });
      };
      forall<C,Threshold>(g, f, &decltype(f)::operator());
    }
    
    template< GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(GlobalAddress<G> g, F loop_body,
                void (F::*mf)(GlobalAddress<typename G::Vertex> src, GlobalAddress<typename G::Vertex> dst) const) {
      Grappa::forall<C,Threshold>(g, [g,loop_body](VertexID i, typename G::Vertex& v){
        auto vi = make_linear(&v);
        Grappa::forall<SyncMode::Async,C,Threshold>(adj(g,v), [loop_body,vi](typename G::Edge& e){
          loop_body(vi, e.ga);
        });
      });
    }
    
  }
  
  /// Parallel iteration over a Graph. Options are:
  ///
  /// @code
  /// using G = Graph<VertexData,EdgeData>;
  /// GlobalAddress<G> g;
  ///
  /// // simple iteration over vertices
  /// forall(g, [](G::Vertex& v){});
  ///
  /// // simple iteration over vertices with index
  /// forall(g, [](VertexID i, G::Vertex& v){});
  ///
  /// // iterate over all adjacencies of all vertices
  /// forall(g, [](G::Vertex& src, G::Edge& adj){ ... });
  /// // (equivalent to)
  /// forall(g, [](G::Vertex& src){
  ///   forall(adj(g,src), [](G::Edge& adj){
  ///     ...
  ///   });
  /// });
  ///
  template< GlobalCompletionEvent * C = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename G = nullptr_t,
            typename F = nullptr_t >
  void forall(GlobalAddress<G> g, F loop_body) {
    impl::forall<C,Threshold>(g, loop_body, &F::operator());
  }
    
  
  ///////////////////////////////////////////////////
  // proposed forall overloads for Graph iteration
  ///////////////////////////////////////////////////
  // forall(g, [](int64_t i, Vertex& v){
  //   
  // });
  // 
  // forall(adj(src_v), [](GlobalAddress<Vertex> vj){
  //   
  // });
  // forall(adj(src_v), [](int64_t j, GlobalAddress<Vertex> vj){
  //   
  // });
  //
  // // run *at* the end vertex
  // forall(src_v.adj(), [](Vertex& ev){
  //   // delegate
  // });
  //
  // forall(g, [](GlobalAddress<Vertex> s, GlobalAddress<Vertex> e){
  //   // runs wherever
  // });
  //
  // forall(g, [](Vertex& s, GlobalAddress<Vertex> e){
  //   // runs on first Vertex
  // });
  //
  // forall(g, [](GlobalAddress<Vertex> s, Vertex& e){
  //   // runs on other Vertex
  //   // (how to do this w/o making a spawn per vertex?)
  // });
  //
  // forall(g, [](const Vertex& s, Vertex& e){
  //   // runs where 'e' is, caches source vertices somehow?
  //   // how the hell to run this efficiently????
  // });
  //
  
  /// @}
} // namespace Grappa
