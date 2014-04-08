#pragma once
#include <Grappa.hpp>
#include <graph/Graph.hpp>
#include <Reducer.hpp>

GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, iteration_time);
DECLARE_int32(max_iterations);

using namespace Grappa;
using delegate::call;

using Empty = struct {};

template< typename G, typename GatherType >
struct GraphlabVertexProgram {
  using Vertex = typename G::Vertex;
  using Edge = typename G::Edge;
  using Gather = GatherType;
  
  GatherType cache;
  
  GraphlabVertexProgram(): cache() {}
  
  void post_delta(GatherType d){ cache += d; }
};

template< typename Subclass >
struct GraphlabVertexData {
  static Reducer<int64_t,ReducerType::Add> total_active;
  
  void* prog;
  bool active, temp_active;
  
  GraphlabVertexData(): active(false) {}
  void activate() { if (!active) { total_active++; active = true; } }
  void deactivate() { if (active) { total_active--; active = false; } }
};

template< typename Subclass >
Reducer<int64_t,ReducerType::Add> GraphlabVertexData<Subclass>::total_active;

template< typename V, typename E >
void activate_all(GlobalAddress<Graph<V,E>> g) {
  forall(g, [](typename Graph<V,E>::Vertex& v){ v->activate(); });
}

template< typename V >
void activate(GlobalAddress<V> v) {
  delegate::call(v, [](V& v){ v->activate(); });
}


extern Reducer<int64_t,ReducerType::Add> ct;

////////////////////////////////////////////////////////
/// Synchronous GraphLab engine, assumes:
/// - Delta caching enabled
/// - (currently) gather_edges:IN_EDGES, scatter_edges:(OUT_EDGES || NONE)
/// 
/// Also requires that the Graph already contains the GraphlabVertexProgram
template< typename VertexProg, typename V, typename E >
void run_synchronous(GlobalAddress<Graph<V,E>> g) {
#define GVertex typename Graph<V,E>::Vertex
#define GEdge typename Graph<V,E>::Edge
  auto prog = [](GVertex& v) -> VertexProg& {
    return *static_cast<VertexProg*>(v->prog);
  };
  
  // // tack the VertexProg data onto the existing vertex data
  // struct VPlus : public V {
  //   VertexProg prog;
  //   VPlus(typename Graph<V,E>::Vertex& v): V(v.data), prog(v) {}
  // };
  // auto g = g->template transform<VPlus>([](typename Graph<V,E>::Vertex& v, VPlus& d){
  //   new (&d) VPlus(v);
  // });
  // using GPVertex = typename Graph<VPlus,E>::Vertex;
  // using GPEdge = typename Graph<VPlus,E>::Edge;
  
  // "gather" once to initialize cache (doing with a scatter)
  
  // TODO: find efficient way to skip 'gather' if 'gather_edges' is always false
  
  // initialize GraphlabVertexProgram
  forall(g, [=](GVertex& v){ v->prog = new VertexProg(v); });
  
  forall(g, [=](GVertex& v){
    forall<async>(adj(g,v), [=,&v](GEdge& e){
      // gather
      auto delta = prog(v).gather(v, e);
      
      call<async>(e.ga, [=](GVertex& ve){        
        prog(ve).post_delta(delta);
      });
    });
  });
  
  int iteration = 0;
  
  while ( V::total_active > 0 && iteration < FLAGS_max_iterations )
      GRAPPA_TIME_REGION(iteration_time) {
    
    ct = 0;
    forall(g, [=](GVertex& v){ if (v->active) ct++; });
    CHECK_EQ(ct, V::total_active);
    
    VLOG(1) << "iteration " << std::setw(3) << iteration
            << " -- active:" << V::total_active;
    double t = walltime();
    
    forall(g, [=](GVertex& v){
      if (v->active) {
        v->temp_active = true;
        v->deactivate();
      }
    });
    
    forall(g, [=](GVertex& v){
      if (!v->temp_active) return;
      
      auto& p = prog(v);
      
      // apply
      p.apply(v, p.cache);
      
      if (p.scatter_edges(v)) {
        auto prog_copy = p;
        // scatter
        forall<async>(adj(g,v), [=](GEdge& e){
          auto e_id = e.id;
          auto e_data = e.data;
          call<async>(e.ga, [=](GVertex& ve){
            auto local_e_data = e_data;
            GEdge e = { e_id, g->vs+e_id, local_e_data };
            auto gather_delta = prog_copy.scatter(e, ve);
            prog(ve).post_delta(gather_delta);
          });
        });
      }
    });
    
    iteration++;
    VLOG(1) << "> time: " << walltime()-t;
  }
  
  forall(g, [](GVertex& v){ delete static_cast<VertexProg*>(v->prog); });
}
