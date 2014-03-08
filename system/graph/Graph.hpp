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

// #define USE_MPI3_COLLECTIVES
#undef USE_MPI3_COLLECTIVES
#ifdef USE_MPI3_COLLECTIVES
#include <mpi.h>
#endif

namespace Grappa {
  
  class VertexID {
    int64_t idx;
  public:
    VertexID(int64_t idx): idx(idx) {}
    operator int64_t () { return idx; }
  };
  
  struct VertexBase {
    int64_t * local_adj; // adjacencies that are local
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
  template< typename T = int64_t, bool HeapData = (sizeof(T) > BLOCK_SIZE-sizeof(int64_t)*2-sizeof(int64_t*)) >
  struct Vertex : public VertexBase {
    T data;
    
    Vertex(): VertexBase(), data() {}
    Vertex(const VertexBase& v): VertexBase(v), data() {}
    ~Vertex() {}
    
    T* operator->() { return &data; }
    
  } GRAPPA_BLOCK_ALIGNED;
  
  template< typename T >
  struct Vertex<T, /*HeapData = */ true> : public VertexBase {
    T& data;
    
    Vertex(): VertexBase(), data(*locale_alloc<T>()) {}
    Vertex(const VertexBase& v): VertexBase(v), data(*locale_alloc<T>()) {}
    
    ~Vertex() { locale_free(&data); }
    
    T* operator->() { return &data; }
    
  } GRAPPA_BLOCK_ALIGNED;
  
  
  template< class V = Vertex<> >
  struct Graph {
    static_assert(block_size % sizeof(V) == 0, "V size not evenly divisible into blocks!");
    
    // using Vertex = V;
    
    // // Helpers (for if we go with custom cyclic distribution)
    // inline Core    vertex_owner (int64_t v) { return v % cores(); }
    // inline int64_t vertex_offset(int64_t v) { return v / cores(); }
  
    // Fields
    GlobalAddress<V> vs;
    int64_t nv, nadj, nadj_local;
  
    // Internal fields
    int64_t * adj_buf;
    int64_t * scratch;
  
    SymmetricAddress<Graph> self;
    
    Graph(SymmetricAddress<Graph> self, GlobalAddress<V> vs, int64_t nv)
      : self(self)
      , vs(vs)
      , nv(nv)
      , nadj(0)
      , nadj_local(0)
      , adj_buf(nullptr)
      , scratch(nullptr)
    { }
  
    ~Graph() {
      for (V& v : iterate_local(vs, nv)) { v.~V(); }
      if (adj_buf) locale_free(adj_buf);
    }
    
    GlobalAddress<V> vertices() { return vs; }
  
    void destroy() {
      auto self = this->self;
      global_free(this->vs);
      call_on_all_cores([self]{ self->~Graph(); });
      global_free(self);
    }
  
    void dump() {
      for (int64_t i=0; i<nv; i++) {
        delegate::call(vs+i, [i](V * v){
          std::stringstream ss;
          ss << "<" << i << ">";
          for (int64_t i=0; i<v->nadj; i++) ss << " " << v->local_adj[i];
          fprintf(stderr, "%s\n", ss.str().c_str());
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
    /// struct A { double weight; }
    /// GlobalAddress<Graph<Vertex<A>>> g = ...
    /// struct B { double value; }
    /// auto gnew = g->transform<B>([](Vertex<A>& v, B& b){
    ///   b.value = v->weight / v.nadj;
    /// });
    /// @endcode
    template< typename NewData, typename F = decltype(nullptr) >
    SymmetricAddress<Graph<Vertex<NewData>>> transform(F f) {
      static_assert(sizeof(V) == sizeof(Vertex<NewData>), "transformed vertex size must be the unchanged.");
      forall(vs, nv, [f](V& v){
        NewData d;
        f(v, d);
        v.~V();
        V b = v;
        auto nv = new (&v) Vertex<NewData>(b);
        nv->data = d;
      });
      return static_cast<SymmetricAddress<Graph<Vertex<NewData>>>>(self);
    }
  
    // Constructor
    static SymmetricAddress<Graph> create(const TupleGraph& tg, bool directed = false) {
      double t;
      auto g = symmetric_global_alloc<Graph<V>>();
  
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
  
      auto vs = global_alloc<V>(g->nv);
      auto self = g;
      on_all_cores([g,vs]{
        new (g.localize()) Graph(g, vs, g->nv);
        for (V& v : iterate_local(g->vs, g->nv)) {
          new (&v) V();
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
        auto count = [](GlobalAddress<V> v){
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
      forall(g->vs, g->nv, [g](int64_t i, V& v) {
    #ifdef SMALL_GRAPH
        // adjust b/c allreduce didn't account for having 1 instance per locale
        v.local_sz = g->scratch[i] / locale_cores();
    #endif
    
        v.nadj = 0;
        if (v.local_sz > 0) v.local_adj = new int64_t[v.local_sz];
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
      forall(g->vs, g->nv, [g](int64_t vi, V& v){
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
        for (V& v : iterate_local(g->vs, g->nv)) {
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
  
  } GRAPPA_BLOCK_ALIGNED;
  
  ////////////////////////////////////////////////////
  // Vertex iterators
  
  template< typename V >
  struct AdjIterator {
    SymmetricAddress<Graph<V>> g;
    GlobalAddress<int64_t> adj;
    int64_t nadj;
    
    AdjIterator(SymmetricAddress<Graph<V>> g, GlobalAddress<int64_t> adj, int64_t nadj):
      g(g), adj(adj), nadj(nadj) {}
  };
  
  template< typename V >
  AdjIterator<V> adj(SymmetricAddress<Graph<V>> g, V& v) {
    return AdjIterator<V>(g, make_global(v.local_adj), v.nadj);
  }

  template< typename V >
  AdjIterator<V> adj(SymmetricAddress<Graph<V>> g, GlobalAddress<V> v) {
    auto p = delegate::call(v, [](V& v){
      return std::make_pair(make_global(v.local_adj), v.nadj);
    });
    return AdjIterator<V>(g, p.first, p.second);
  }
  
  template< typename V >
  AdjIterator<V> adj(SymmetricAddress<Graph<V>> g, int64_t i) {
    auto p = delegate::call(g->vs+i, [](V& v){
      return std::make_pair(make_global(v.local_adj), v.nadj);
    });
    return AdjIterator<V>(g, p.first, p.second);
  }
  
#ifdef __GRAPPA_CLANG__
  template< typename V >
  AdjIterator<V> adj(Graph<V> grappa_symmetric* g, V grappa_global* v) {
    return adj(as_addr(g), as_addr(v));
  }
#endif
  
  namespace impl {
    template< SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename V, typename F >
    void forall(AdjIterator<V> a, F body,
                void (F::*mf)(int64_t,VertexID,GlobalAddress<V>) const)
    {
      if (C) C->enroll();
      auto origin = mycore();
      
      auto loop = [a,origin,body]{
        auto adj = a.adj.pointer();
        auto vs = a.g->vs;
        Grappa::forall_here<S,C,Threshold>(0, a.nadj, [body,origin,adj,vs](int64_t i){
          mark_async<C>([=]{
            body(i, adj[i], vs + adj[i]);
          })();
        });
        if (C) C->send_completion(origin);
      };
      
      if (a.adj.core() == mycore()) {
        loop();
      } else {
        if (S == SyncMode::Async) {
          spawnRemote<nullptr>(a.adj.core(), [loop]{ loop(); });
        } else {
          CompletionEvent ce(1);
          auto ce_a = make_global(&ce);
          spawnRemote<nullptr>(a.adj.core(), [loop,ce_a]{
            loop();
            complete(ce_a);
          });
          ce.wait();
        }
      }
    }
    template< SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename V, typename F >
    void forall(AdjIterator<V> a, F body, void (F::*mf)(GlobalAddress<V>) const) {
      auto f = [body](int64_t i, VertexID j, GlobalAddress<V> v){ body(v); };
      impl::forall<S,C,Threshold>(a, f, &decltype(f)::operator());
    }
    template< SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename V, typename F >
    void forall(AdjIterator<V> a, F body, void (F::*mf)(int64_t) const) {
      auto f = [body](int64_t i, VertexID j, GlobalAddress<V> v){ body(i); };
      impl::forall<S,C,Threshold>(a, f, &decltype(f)::operator());
    }
    template< SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename V, typename F >
    void forall(AdjIterator<V> a, F body, void (F::*mf)(VertexID) const) {
      auto f = [body](int64_t i, VertexID j, GlobalAddress<V> v){ body(j); };
      impl::forall<S,C,Threshold>(a, f, &decltype(f)::operator());
    }
    template< SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename V, typename F >
    void forall(AdjIterator<V> a, F body, void (F::*mf)(int64_t,VertexID) const) {
      auto f = [body](int64_t i, VertexID j, GlobalAddress<V> v){ body(i,j); };
      impl::forall<S,C,Threshold>(a, f, &decltype(f)::operator());
    }
    template< SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename V, typename F >
    void forall(AdjIterator<V> a, F body, void (F::*mf)(VertexID,GlobalAddress<V>) const) {
      auto f = [body](int64_t i, VertexID j, GlobalAddress<V> v){ body(j,v); };
      impl::forall<S,C,Threshold>(a, f, &decltype(f)::operator());
    }
    template< SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename V, typename F >
    void forall(AdjIterator<V> a, F body, void (F::*mf)(int64_t,GlobalAddress<V>) const) {
      auto f = [body](int64_t i, VertexID j, GlobalAddress<V> v){ body(i,v); };
      impl::forall<S,C,Threshold>(a, f, &decltype(f)::operator());
    }
  }
  
#define OVERLOAD(...) \
  template< __VA_ARGS__, typename V = nullptr_t, typename F = nullptr_t > \
  void forall(AdjIterator<V> a, F body) { \
    impl::forall<S,C,Threshold>(a, body, &F::operator()); \
  }
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
    template< GlobalCompletionEvent * C, int64_t Threshold, typename V, typename F >
    void forall(SymmetricAddress<Graph<V>> g, F loop_body, void (F::*mf)(V&) const) {
      forall<C,Threshold>(g->vs, g->nv, loop_body);
    }

    template< GlobalCompletionEvent * C, int64_t Threshold, typename V, typename F >
    void forall(SymmetricAddress<Graph<V>> g, F loop_body,
                void (F::*mf)(int64_t,V&) const) {
      forall<C,Threshold>(g->vs, g->nv, loop_body);
    }

    /// Demonstrating another "visitor" we could provide for graphs (this is kinda
    /// silly as it just gives you the indices of each edge).
    template< GlobalCompletionEvent * C, int64_t Threshold, typename V, typename F >
    void forall(SymmetricAddress<Graph<V>> g, F loop_body,
                void (F::*mf)(int64_t src, int64_t dst) const) {
      forall<C,Threshold>(g, [g,loop_body](int64_t i, V& v){
        Grappa::forall<SyncMode::Async,C,Threshold>(adj(g,v), [loop_body,i](int64_t j){
          loop_body(i, j);
        });
      });
    }
    
    template< GlobalCompletionEvent * C, int64_t Threshold, typename V, typename F >
    void forall(SymmetricAddress<Graph<V>> g, F loop_body,
                void (F::*mf)(GlobalAddress<V> src, GlobalAddress<V> dst) const) {
      forall<C,Threshold>(g, [g,loop_body](int64_t i, V& v){
        auto vi = make_linear(&v);
        Grappa::forall<SyncMode::Async,C,Threshold>(adj(g,v), [loop_body,vi](GlobalAddress<V> vj){
          loop_body(vi, vj);
        });
      });
    }
    
  }
  
  template< GlobalCompletionEvent * C = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename V = decltype(nullptr),
            typename F = decltype(nullptr) >
  void forall(SymmetricAddress<Graph<V>> g, F loop_body) {
    impl::forall<C,Threshold>(g, loop_body, &F::operator());
  }
  
#ifdef __GRAPPA_CLANG__
  template< GlobalCompletionEvent * C = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename V = decltype(nullptr),
            typename F = decltype(nullptr) >
  void forall(Graph<V> grappa_symmetric* g, F loop_body) {
    forall(as_addr(g), loop_body);
  }
#endif
  
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
  
} // namespace Grappa
