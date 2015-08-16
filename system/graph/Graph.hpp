////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

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
  
  /// Empty struct, for specifying lack of either Vertex or Edge data in @ref Graph.
  struct Empty {};
  
  namespace impl {
    
    struct VertexBase {
      bool valid; // vertices with no connections (in/out) are marked invalid TODO: eliminate these from the representation entirely
      VertexID * local_adj; // adjacencies that are local
      int64_t nadj;        // number of adjacencies
      int64_t local_sz;    // size of local allocation (regardless of how full it is)
      
      VertexBase(): valid(true), local_adj(nullptr), nadj(0), local_sz(0) {}
      
      VertexBase(const VertexBase& v):
        local_adj(v.local_adj),
        nadj(v.nadj),
        local_sz(v.local_sz),
        valid(true)
      { }
    };
    
    /// Vertex with customizable inline 'data' field. Will attempt 
    /// to pack the provided type into the block-aligned Vertex 
    /// class, but if it is too large, will heap-allocate (from 
    /// locale-shared heap). Defines a '->' operator to access data 
    /// fields.
    ///
    /// Example subclasses:
    /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
    /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    template< typename T, typename E, bool HeapData = (sizeof(T) > BLOCK_SIZE-sizeof(VertexBase)-sizeof(E*)) >
    struct Vertex : public VertexBase {
      T data;
      E * local_edge_state;
    
      Vertex(): VertexBase(), data() {}
      Vertex(const VertexBase& v): VertexBase(v), data() {}
      ~Vertex() {}
    
      T* operator->() { return &data; }
      const T* operator->() const { return &data; }
      
      static constexpr size_t global_heap_size() { return sizeof(Vertex); }
      static constexpr size_t locale_heap_size() { return 0; }
      static constexpr size_t size() { return locale_heap_size() + global_heap_size(); }
      
    } GRAPPA_BLOCK_ALIGNED;
  
    template< typename T, typename E >
    struct Vertex<T, E, /*HeapData = */ true> : public VertexBase {
      T& data;
      E * local_edge_state;
    
      Vertex(): VertexBase(), data(*locale_alloc<T>()) {}
      Vertex(const VertexBase& v): VertexBase(v), data(*locale_alloc<T>()) {}
    
      ~Vertex() { locale_free(&data); }
    
      T* operator->() { return &data; }
      const T* operator->() const { return &data; }
      
      static constexpr size_t global_heap_size() { return sizeof(Vertex); }
      static constexpr size_t locale_heap_size() { return sizeof(T); }
      static constexpr size_t size() { return locale_heap_size() + global_heap_size(); }
      
    } GRAPPA_BLOCK_ALIGNED;
  
  }
  
  /// Distributed graph data structure, with customizable vertex and edge data.
  /// 
  /// This is Grappa's primary graph data structure. Graph is a 
  /// *symmetric data structure*, so a Graph proxy object will be allocated 
  /// on all cores, providing local access to common data and methods to
  /// access the entirety of the graph.
  /// 
  /// The Graph class defines two types based on the specified template
  /// parameters: Vertex and Edge. Vertex holds information about an
  /// individual vertex, such as the degree (`nadj`), and the associated
  /// data whose type is specified by the `V` template parameter. 
  /// Edge holds the two global addresses to its source and destination
  /// vertices, as well as data associated with this edge, of the type
  /// specified by the `E` parameter.
  /// 
  /// A typical use of Graph will define a custom graph type, construct it, 
  /// and then use the returned symmetric pointer to refer to the graph,
  /// possibly looking something like this:
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// // define the graph type:
  /// struct VertexData { double rank; };
  /// struct EdgeData { double weight; };
  /// using G = Graph<VertexData,EdgeData>;
  /// 
  /// // load tuples from a file:
  /// TupleGraph tuples = TupleGraph::Load("twitter.bintsv4", "bintsv4");
  /// 
  /// // after constructing a graph, we get a symmetric pointer back
  /// GlobalAddress<G> g = G::create(tuples);
  /// 
  /// // we can easily get info such as the number of vertices out by
  /// // using the local proxy directly through the global address:
  /// LOG(INFO) << "graph has " << g->nv << " vertices";
  /// 
  /// // or we can use the global address itself as an iterator over vertices:
  /// forall(g, [](G::Vertex& v){ });
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// 
  /// Graph Structure
  /// ----------------
  /// 
  /// This Graph structure is constructed from a TupleGraph (a simple list
  /// of edge tuples -- source & dest) using Graph::create().
  /// 
  /// Vertices are randomly distributed among cores (using a simple global
  /// heap allocation). Edges are placed on the core of their *source* vertex.
  /// Therefore, iterating over outgoing edges is very efficient, but a 
  /// vertex's incoming edges must be found the hard way (in practice, 
  /// we just avoid doing it entirely if using this graph structure).
  /// 
  /// Parallel Iterators
  /// -------------------
  /// 
  /// A number of parallel iterators are available for Graphs, to iterate 
  /// over vertices or edges using `forall`. Specialization is done based 
  /// on which parameters are requested, sometimes resulting in different 
  /// iteration schemes.
  /// 
  /// Full graph iteration:
  /// 
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// 
  /// using G = Graph<VertexData,EdgeData>;
  /// GlobalAddress<G> g = /* initialize graph */;
  ///
  /// // simple iteration over vertices
  /// forall(g, [](G::Vertex& v){});
  ///
  /// // simple iteration over vertices with index
  /// forall(g, [](VertexID i, G::Vertex& v){});
  ///
  /// // iterate over all adjacencies of all vertices
  /// // (at the source vertex and edge, so both may be modified)
  /// forall(g, [](G::Vertex& src, G::Edge& e){ ... });
  /// 
  /// // (equivalent to)
  /// forall(g, [](G::Vertex& src){
  ///   forall(adj(g,src), [](G::Edge& adj){
  ///     ...
  ///   });
  /// });
  ///
  /// // iterate over edges from the destination vertex, 
  /// // must be **non-blocking**
  /// // (edge no longer modifiable, but a copy is carried along)
  /// forall(g, [](const G::Edge& e, G::Vertex& dst){ ... });
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// 
  /// Iteration over adjacencies of single Vertex:
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// // usually done inside another iteration over vertices, like so:
  /// forall(g, [=](Vertex& v){
  /// 
  ///   // variations exist for constructing a parallel iterator over
  ///   // a vertex's outgoing edges (adjacency list, or `adj`):
  ///   // - from a Vertex&: adj(g,v)
  ///   // - or a VertexID: adj(g, v.id)
  ///   
  ///   // `forall` has various specializations for `adj`, including:
  ///   forall<async>(adj(g,v), [&v](Edge& e){
  ///     LOG(INFO) << v.id << " -> " << e.id;
  ///   });
  ///   // or this form, which provides the index within the adj list [0,v.nadj)
  ///   forall<async>(adj(g,v), [&v](int64_t ei, Edge& e){
  ///     LOG(INFO) << "v.adj[" << ei << "] = " << e.id;
  ///   });
  /// 
  /// });
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  template< typename V = Empty, typename E = Empty >
  struct Graph {
    
    using Vertex = impl::Vertex<V,E>;
    using EdgeState = E;
    
    struct Edge {
      VertexID id; ///< Global index of adjacent vertex
      GlobalAddress<Vertex> ga; ///< Global address to adjacent vertex
      EdgeState& data;
      
      /// Access elements of EdgeState with operator '->'
      EdgeState* operator->() { return &data; }
      const EdgeState* operator->() const { return &data; }
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
          if (VLOG_IS_ON(LEVEL)) std::cerr << ss.str() << "\n";
        });
      }
    }
  
    /// Change the data associated with each vertex, keeping the same connectivity 
    /// structure and allowing the user to intialize the new data type using the old vertex.
    ///
    /// @param f: void (Vertex<OldData>& v, NewData& d) {}
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
    static GlobalAddress<Graph> create(const TupleGraph& tg, bool directed = false, bool solo_invalid = true);
    
    static GlobalAddress<Graph> Undirected(const TupleGraph& tg) { return create(tg, false); }
    static GlobalAddress<Graph> Directed(const TupleGraph& tg) { return create(tg, true); }
      
    VertexID id(Vertex& v) {
      return make_linear(&v) - vs;
    }
    
    Edge edge(Vertex& v, size_t i) {
      auto j = v.local_adj[i];
      return Edge{ j, vs+j, v.local_edge_state[i] };
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
  
  template< typename G = nullptr_t, typename F = nullptr_t >
  void serial_for(AdjIterator<G> a, F body) {
    auto vs = a.g->vs;
    auto v = (vs+a.i).pointer();
    CHECK((vs+a.i).core() == mycore());
    for (int64_t i = 0; i < v->nadj; i++) {
      auto j = v->local_adj[i];
      typename G::Edge e = { j, vs+j, v->local_edge_state[i] };
      body(e);
    }
  }
  
  
  ////////////////////////////////////////////////////
  // Graph iterators
  
  namespace impl {
        
    /// Parallel iteration over vertices with their corresponding index
    template< GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(GlobalAddress<G> g, F body,
                void (F::*mf)(VertexID,typename G::Vertex&) const) {
      Grappa::forall<C,Threshold>(g->vs, g->nv,
      [body](int64_t i, typename G::Vertex& v){
        if (v.valid) {
          body(i, v);
        }
      });
    }
    
    /// Parallel iteration over each vertex
    template< GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(GlobalAddress<G> g, F body, void (F::*mf)(typename G::Vertex&) const) {
      auto f = [body](VertexID i, typename G::Vertex& v){ body(v); };
      impl::forall<C,Threshold>(g, f, &decltype(f)::operator());
    }
    
    /// Parallel iteration over all adjacencies of all vertices,
    /// executing at the *source* vertex.
    template< GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(GlobalAddress<G> g, F loop_body,
                void (F::*mf)(typename G::Vertex& src, typename G::Edge& adj) const) {
      auto f = [g,loop_body](typename G::Vertex& v){
        Grappa::forall<SyncMode::Async,C,Threshold>(adj(g,v),
            [&v,loop_body](typename G::Edge& e){
          loop_body(v, e);
        });
      };
      forall<C,Threshold>(g, f, &decltype(f)::operator());
    }

    /// Parallel iteration over all adjacencies of all vertices,
    /// executing at the *destination* vertex.
    template< GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(GlobalAddress<G> g, F loop_body,
                void (F::*mf)(typename G::Edge& adj, typename G::Vertex& src) const) {
      auto f = [g,loop_body](typename G::Vertex& v){
        Grappa::forall<SyncMode::Async,C,Threshold>(adj(g,v),
            [&v,loop_body,g](typename G::Edge& e){
          auto e_id = e.id;
          auto e_data = e.data;
          Grappa::delegate::call<SyncMode::Async>(e.ga, [=](typename G::Vertex& ve){
            auto local_e_data = e_data;
            typename G::Edge e = { e_id, g->vs+e_id, local_e_data };
            loop_body(e, ve);
          });
        });
      };
      forall<C,Threshold>(g, f, &decltype(f)::operator());
    }
    
    /// @deprecated
    /// Parallel iteration over edges, making no assumptions about where it is executed.
    /// Provides global addresses to both source and destination.
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
  
  /// Parallel iterator over Graph, specializes based on arguments.
  /// See Graph overview page for details.
  template< GlobalCompletionEvent * C = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename V, typename E,
            typename F = nullptr_t >
  void forall(GlobalAddress<Graph<V,E>> g, F loop_body) {
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
  
  /// @brief Construct a distributed adjacency-list Graph.
  /// 
  /// @return The symmetric address to the 'proxy' allocated on each core,
  ///         which has the size information and a portion of the graph.
  /// 
  /// @param tg            input edges
  /// @param directed      create additional edges to make it undirected
  /// @param solo_invalid  mark vertices with no in- or out-edges as 
  ///                      invalid (not to be visited when iterating 
  ///                      over vertices)
  template< typename V, typename E >
  GlobalAddress<Graph<V,E>> Graph<V,E>::create(const TupleGraph& tg,
      bool directed, bool solo_invalid) {
    VLOG(1) << "Graph: " << (directed ? "directed" : "undirected");
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
        VLOG(2) << "find_nv_time: " << walltime() - t;

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
    VLOG(2) << "count_time: " << walltime() - t;

  #ifdef SMALL_GRAPH
    t = walltime();  
  #ifdef USE_MPI3_COLLECTIVES
    on_all_cores([g]{
      MPI_Request r; int done;
      MPI_Iallreduce(MPI_IN_PLACE, g->scratch, g->nv, MPI_INT64_T, MPI_SUM, global_communicator.grappa_comm, &r);
      do {
        MPI_Test( &r, &done, MPI_STATUS_IGNORE );
        if(!done) { Grappa::yield(); }
      } while(!done);
    });
  #else
    on_all_cores([g]{ allreduce_inplace<int64_t,collective_add>(g->scratch, g->nv); });
  #endif // USE_MPI3_COLLECTIVES
    VLOG(2) << "allreduce_inplace_time: " << walltime() - t;
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
    
    if (solo_invalid) {
      // (note: this isn't necessary if we don't create vertices for those with no edges)
      // find which are actually active (first, those with outgoing edges)
      forall(g, [](Vertex& v){ v.valid = (v.nadj > 0); });
      // then those with only incoming edges (reachable from at least one active vertex)
      forall(g, [](Edge& e, Vertex& ve){ ve.valid = true; });
    }    
    VLOG(1) << "-- vertices: " << g->nv;
    
    auto gsz = Vertex::global_heap_size()*g->nv
                          + sizeof(Graph) * cores();
    auto lsz = Vertex::locale_heap_size()*g->nv
                          + (sizeof(VertexID)+sizeof(EdgeState))*g->nadj;
    auto GB = [](size_t v){ return static_cast<double>(v) / (1L<<30); };
    LOG(INFO) << "\nGraph memory breakdown:"
              << "\n  locale_heap_size: " << GB(lsz) << " GB"
              << "\n  global_heap_size: " << GB(gsz) << " GB"
              << "\n  graph_total_size: " << GB(lsz+gsz) << " GB";
    return g;
  }
  
  /// @}
} // namespace Grappa
