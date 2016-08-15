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

////////////////////////////////////////////////////////////////////////
/// GraphLab is an API and runtime system for graph-parallel computation.
/// This is a rough prototype implementation of the programming model to
/// demonstrate using Grappa as a platform for other models.
/// More information on the actual GraphLab system can be found at:
/// graphlab.org.
////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Simplified GraphLab API, implemented for Grappa's builtin Graph structure.
// (included from graphlab.hpp)
#pragma once

/// Additional vertex state needed for GraphLab engine. Your Graph's VertexData
/// must be a subclass of this to use the NaiveGraphlabEngine.
template< typename Subclass >
struct GraphlabVertexData {
  static Reducer<int64_t,ReducerType::Add> total_active;

  void* prog;
  bool active, active_minor_step;

  GraphlabVertexData(): active(false) {}
  void activate() { if (!active) { total_active++; active = true; } }
  void deactivate() { if (active) { total_active--; active = false; } }
};

/// Additional state for tracking active vertices in NaiveGraphlabEngine
template< typename Subclass >
Reducer<int64_t,ReducerType::Add> GraphlabVertexData<Subclass>::total_active;

/// Activate all vertices (for NaiveGraphlabEngine)
template< typename V, typename E >
void activate_all(GlobalAddress<Graph<V,E>> g) {
  forall(g, [](typename Graph<V,E>::Vertex& v){ v->activate(); });
}

/// Activate a single vertex (for NaiveGraphlabEngine)
template< typename V >
void activate(GlobalAddress<V> v) {
  delegate::call(v, [](V& v){ v->activate(); });
}

/// Graphlab engine implemented over the naive Grappa graph structure.
/// 
/// Because Grappa's Graph only has out-edges per vertex, this engine can
/// only execute a subset of Graphlab vertex programs. Namely, it forces
/// the following:
/// 
/// - gather_edges: IN_EDGES | NONE, represented as a bool 
///   (note: ALL_EDGES can also be done if the graph is constructed as 
///   "undirected" as edges will then be duplicated)
/// - scatter_edges: OUT_EDGES | NONE, represented as a bool
/// - If the graph was constructed as "undirected", then both bools above
///   instead indicate ALL_EDGES | NONE.
/// - Delta caching is assumed to be *enabled* (if you have no Gather type,
///   this won't bother you)
/// 
/// A couple additional caveats:
/// - only one Engine can be executed at a time in the system
/// - Gather type must be POD
/// 
/// @tparam G           Graph type
/// @tparam VertexProg  Vertex program, subclass of GraphlabVertexProgram.
/// 
template< typename G, typename VertexProg >
struct NaiveGraphlabEngine {
  using Vertex = typename G::Vertex;
  using Edge = typename G::Edge;
  using Gather = typename VertexProg::Gather;
  
  static GlobalAddress<G> g;
  static Reducer<int64_t,ReducerType::Add> ct;
  
  static VertexProg& prog(Vertex& v) {
    return *static_cast<VertexProg*>(v->prog);
  }
  
  static void _do_scatter(const VertexProg& prog_copy, Edge& e,
                  Gather (VertexProg::*f)(Vertex&) const) {
    call<async>(e.ga, [=](Vertex& ve){
      auto gather_delta = prog_copy.scatter(ve);
      prog(ve).post_delta(gather_delta);
    });
  }
  
  static void _do_scatter(const VertexProg& prog_copy, Edge& e,
                  Gather (VertexProg::*f)(const Edge&, Vertex&) const) {
    auto e_id = e.id;
    auto e_data = e.data;
    call<async>(e.ga, [=](Vertex& ve){
      auto local_e_data = e_data;
      Edge e = { e_id, g->vs+e_id, local_e_data };
      auto gather_delta = prog_copy.scatter(e, ve);
      prog(ve).post_delta(gather_delta);
    });
  }
  
  /// Run synchronous engine, assumes:
  /// - Delta caching enabled
  /// - gather_edges:IN_EDGES, scatter_edges:(OUT_EDGES || NONE)
  template< typename V, typename E >
  static void run_sync(GlobalAddress<Graph<V,E>> _g) {
    
    call_on_all_cores([=]{ g = _g; });
    
    ct = 0;
    // initialize GraphlabVertexProgram
    forall(g, [=](Vertex& v){
      v->prog = new VertexProg(v);
      if (prog(v).gather_edges(v)) ct++;
    });
    
    if (ct > 0) {
      forall(g, [=](Vertex& v){
        forall<SyncMode::Async>(adj(g,v), [=,&v](Edge& e){
          // gather
          auto delta = prog(v).gather(v, e);

          call<async>(e.ga, [=](Vertex& ve){
            prog(ve).post_delta(delta);
          });
        });
      });
    }
    int iteration = 0;
    size_t active = V::total_active;
    while ( active > 0 && iteration < FLAGS_max_iterations )
        GRAPPA_TIME_REGION(iteration_time) {
      VLOG(1) << "iteration " << std::setw(3) << iteration;
      VLOG(1) << "  active: " << active;

      double t = walltime();
      
      forall(g, [=](Vertex& v){
        if (!v->active) return;
        v->deactivate();
        
        auto& p = prog(v);

        // apply
        p.apply(v, p.cache);

        v->active_minor_step = p.scatter_edges(v);
      });

      forall(g, [=](Vertex& v){
        if (v->active_minor_step) {
          v->active_minor_step = false;
          auto prog_copy = prog(v);
          // scatter
          forall<SyncMode::Async>(adj(g,v), [=](Edge& e){
            _do_scatter(prog_copy, e, &VertexProg::scatter);
          });
        }
      });
    
      iteration++;
      VLOG(1) << "  time:   " << walltime()-t;
      active = V::total_active;
    }

    forall(g, [](Vertex& v){ delete static_cast<VertexProg*>(v->prog); });
  }
};

template< typename G, typename VertexProg >
GlobalAddress<G> NaiveGraphlabEngine<G,VertexProg>::g;

template< typename G, typename VertexProg >
Reducer<int64_t,ReducerType::Add> NaiveGraphlabEngine<G,VertexProg>::ct;
