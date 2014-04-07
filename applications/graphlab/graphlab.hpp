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
struct GraphlabVertexData {
  static Reducer<int64_t,ReducerType::Add> total_active;
  
  bool active;
  T cache;
  GraphlabVertexData(): active(false), cache() {}
  void activate() { if (!active) { total_active++; active = true; } }
  void deactivate() { if (active) { total_active--; active = false; } }
  void post_delta(T d){ cache += d; }
};
template<typename T> Reducer<int64_t,ReducerType::Add> GraphlabVertexData<T>::total_active;

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
/// - (currently) gather_edges:IN_EDGES, scatter_edges:(OUT_EDGES || NONE)
template< typename VertexProg, typename V, typename E >
void run_synchronous(GlobalAddress<Graph<V,E>> g) {
#define GVertex typename Graph<V,E>::Vertex
#define GEdge typename Graph<V,E>::Edge
  // using G = Graph<V,E>;
  // using GVertex = typename G::Vertex;
  // using GEdge = typename G::Edge;
  
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
  
  while ( V::total_active > 0 && iteration < FLAGS_max_iterations )
      GRAPPA_TIME_REGION(iteration_time) {
    
    VLOG(1) << "iteration " << std::setw(3) << iteration
            << " -- active:" << V::total_active;
    double t = walltime();
    
    V::total_active = 0; // 'apply' deactivates all vertices 
    
    forall(g, [=](GVertex& v){
      if (!v->active) return;
      
      VertexProg prog;
      
      // apply
      prog.apply(v, v->cache);
      
      v->deactivate(); // FIXME: race with scatter
      
      if (prog.scatter_edges(v)) {
        // scatter
        forall<async>(adj(g,v), [g,prog](GEdge& e){
          auto e_id = e.id;
          auto e_data = e.data;
          call<async>(e.ga, [g,e_id,e_data,prog](GVertex& ve){
            auto local_e_data = e_data;
            GEdge e = { e_id, g->vs+e_id, local_e_data };
            prog.scatter(e, ve);
          });
        });
      }
    });
    
    iteration++;
    VLOG(1) << "> time: " << walltime()-t;
  }
}
