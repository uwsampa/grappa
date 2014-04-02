#pragma once
#include <Grappa.hpp>
#include <graph/Graph.hpp>
#include <Reducer.hpp>

GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, iteration_time);
DECLARE_int32(max_iterations);

using namespace Grappa;
using delegate::call;

using Empty = struct {};

template< typename T >
struct GraphlabVertex {
  static Reducer<int64_t,ReducerType::Add> total_active;  
  
  bool active;
  T cache;
  GraphlabVertex(): active(false), cache() {}
  void activate() {
    if (!active) {
      total_active++;
      active = true;
    }
  }
  void post_delta(T d){ cache += d; }
};
template<typename T> Reducer<int64_t,ReducerType::Add> GraphlabVertex<T>::total_active;

template< typename V, typename E >
void activate_all(GlobalAddress<Graph<V,E>> g) {
  forall(g, [](typename Graph<V,E>::Vertex& v){ v->activate(); });
}

template< typename V >
void activate(GlobalAddress<V> v) {
  delegate::call(v, [](V& v){ v->activate(); });
}


////////////////////////////////////////////////////////
/// Synchronous GraphLab engine, assumes:
/// - Delta caching enabled
/// - (currently) gather_edges:IN_EDGES, scatter_edges:OUT_EDGES
template< typename VertexProg, typename V, typename E >
void run_synchronous(GlobalAddress<Graph<V,E>> g) {
  using G = Graph<V,E>;
  using GVertex = typename G::Vertex;
  using GEdge = typename G::Edge;
  
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
  
  forall(g, [=](GVertex& v){
    forall<async>(adj(g,v), [&v](GEdge& e){
      
      VertexProg prog;
            
      // gather
      auto delta = prog.gather(v, e);
      
      call<async>(e.ga, [=](GVertex& ve){
        ve->post_delta(delta);
      });
    });
  });
  
  int iteration = 0;
  
  while ( V::total_active > 0 ) GRAPPA_TIME_REGION(iteration_time) {
    
    VLOG(1) << "iteration " << std::setw(3) << iteration
            << " -- active:" << V::total_active;
    
    V::total_active = 0; // 'apply' deactivates all vertices 
    
    if (iteration > FLAGS_max_iterations) break;
    
    forall(g, [=](GVertex& v){
      if (!v->active) return;
      
      VertexProg prog;
      
      // apply
      prog.apply(v, v->cache);
      v->active = false;
      
      if (prog.scatter_edges(v)) {
        // scatter
        forall<async>(adj(g,v), [prog](GEdge& e){
          auto edge = e;
          call<async>(e.ga, [edge,prog](GVertex& ve){
            prog.scatter(edge, ve);
          });
        });
      }
    });
    
    iteration++;
  }
}
