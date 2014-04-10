#pragma once
#include <Grappa.hpp>
#include <graph/Graph.hpp>
#include <Reducer.hpp>
#include <SmallLocalSet.hpp>
#include <GlobalHashMap.hpp>

#include <unordered_set>
#include <unordered_map>
#include <vector>
using std::unordered_set;
using std::unordered_map;
using std::vector;

using CoreSet = SmallLocalSet<Core>;

// #include "cuckoo_set_pow2.hpp"
// using graphlab::cuckoo_set_pow2;
//using CoreSet = cuckoo_set_pow2<Core>;

GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, iteration_time);
DECLARE_int32(max_iterations);

using namespace Grappa;
using delegate::call;

using Empty = struct {};

namespace Graphlab {

  template< typename V, typename E >
  class Graph {
    
    struct Edge {
      VertexID src, dst;
      E data;
      Edge() = default;
      Edge(const TupleGraph::Edge& e): src(e.v0), dst(e.v1) {}
      
      friend std::ostream& operator<<(std::ostream& o, const Edge& e) {
        return o << "<" << e.src << "," << e.dst << ">";
      }
    };
    
    struct MasterInfo {
      std::vector<Core> mirrors;
    };
    
    struct Vertex {
      VertexID id;
      V data;
      Edge * l_out;
      size_t l_nout;
      GlobalAddress<Vertex> master;
      
      Vertex(VertexID id): id(id) {}
    };
    
    GlobalAddress<Graph> self;
    size_t nv, nv_over;
    
    std::vector<Edge> l_edges;   ///< Local edges
    std::vector<Vertex> l_verts; ///< Local vertices (indexes into l_edges)
    size_t l_nsrc;             ///< Number of source vertices
    
    unordered_map<VertexID,Vertex*> l_vmap;
    unordered_map<VertexID,MasterInfo> l_masters;
        
    Graph(GlobalAddress<Graph> self)
      : self(self) , l_edges() , l_verts() , l_nsrc(0) , l_vmap()
    { }
    
  public:
    Graph() = default;
    
    static GlobalAddress<Graph> create(TupleGraph tg) {
      VLOG(1) << "Graphlab::Graph::create( undirected, greedy_oblivious )";
      auto g = symmetric_global_alloc<Graph>();
      
      on_all_cores([=]{
        // intialize graph
        new (g.localize()) Graph(g);
        
        // vertex placements that this core knows about (array of cores, each with a set of vertices mapped to it)
        unordered_map<VertexID,CoreSet> vplace;
        auto local_edges = iterate_local(tg.edges, tg.nedge);
        auto nlocal = local_edges.size();
        auto idx = [&](TupleGraph::Edge& e){ return &e - local_edges.begin(); };
        
        std::vector<Core> assignments; assignments.resize(local_edges.size());
        size_t* edge_cts = locale_alloc<size_t>(cores());
        Grappa::memset(edge_cts, 0, cores());
        
        /// cores this vertex has been placed on
        auto vcores = [&](VertexID i) -> CoreSet& {
          if (vplace.count(i) == 0) {
            vplace[i];
          }
          return vplace[i];
        };
        
        auto assign = [&](TupleGraph::Edge& e, Core c) {
          CHECK_LT(c, cores());
          CHECK_LT(idx(e), nlocal);
          assignments[idx(e)] = c;
          edge_cts[c]++;
          for (auto v : {e.v0, e.v1}) {
            if (vplace[v].count(c) == 0) {
              CHECK_GE(c, 0);
              vplace[v].insert(c);
            }
          }
        };
        
        auto cmp_load = [&](Core c0, Core c1) { return edge_cts[c0] < edge_cts[c1]; };
        
        for (auto& e : local_edges) {
          auto &vs0 = vcores(e.v0), &vs1 = vcores(e.v1);
          
          auto common = intersect_choose_random(vs0, vs1);
          if (common != CoreSet::INVALID) {
            // place this edge on 'common' because both vertices are already there
            assign(e, common);
          } else if (!vs0.empty() && !vs1.empty()) {
            // both assigned but no intersect, choose the least-loaded machine
            assign(e, min_element(vs0, vs1, cmp_load));
          } else if (!vs0.empty()) {
            // only v0 assigned, choose one of its cores
            assign(e, min_element(vs0, cmp_load));
          } else if (!vs1.empty()) {
            // only v1 assigned, choose one of its cores
            assign(e, min_element(vs1, cmp_load));
          } else {
            // neither assigned, so pick overall least-loaded core
            auto c = min_element(Range<Core>{0,cores()}, cmp_load);
            // prefer spreading out over all cores
            if (edge_cts[mycore()] <= edge_cts[c]) c = mycore();
            assign(e, c);
          }
        }
        
        allreduce_inplace<size_t,collective_add>(&edge_cts[0], cores());
        
        if (mycore() == 0) {
          std::cerr << util::array_str("edge_cts", edge_cts, cores()) << "\n";
        }
        
        //////////////////////////
        // actually scatter edges
        auto& edges = g->l_edges;
        
        edges.reserve(edge_cts[mycore()]);
        barrier();
        
        finish([&]{
          for (auto& e : local_edges) {
            auto e_copy = e;
            call<async>(assignments[idx(e)], [e_copy,g]{
              g->l_edges.emplace_back(e_copy);
            });
          }
        });
        
        // if (mycore() == 0) global_free(tg.edges);        
        std::sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b){
          if (a.src == b.src) return a.dst < b.dst;
          else return a.src < b.src;
        });
        
        auto it = std::unique(edges.begin(), edges.end(), [](const Edge& a, const Edge& b){
          return (a.src == b.src) && (a.dst == b.dst);
        });
        edges.resize(std::distance(edges.begin(), it));
        
        VLOG(0) << "l_edges: " << edges;
        
        auto& l_vmap = g->l_vmap;
        auto& lvs = g->l_verts;
        
        // so we can pre-size l_verts and get all the hashtable allocs over with
        for (auto& e : edges) {
          for (auto v : {e.src, e.dst}) {
            l_vmap[v] = nullptr;
          }
        }
        lvs.reserve(l_vmap.size());
        
        VertexID src = -1;
        Vertex* cv;
        
        for (Edge& e : edges) {
          if (src != e.src) {
            if (src >= 0) {
              // finish previous one
              cv->l_nout = &e - cv->l_out;
            }
            // start of new src
            lvs.emplace_back(e.src);
            cv = &lvs.back();
            l_vmap[e.src] = cv;
            cv->l_out = &e;
            src = e.src;
          }
        }
        cv->l_nout = &*g->l_edges.end() - cv->l_out;
        g->l_nsrc = lvs.size(); // vertices that have at least one outgoing edge are first
        
        // now go add all the ones that are destination-only (on this core)
        for (auto& e : g->l_edges) {
          if (l_vmap[e.dst] == nullptr) {
            lvs.emplace_back(e.dst);
            cv = &lvs.back();
            l_vmap[e.dst] = cv;
            cv->l_out = nullptr;
            cv->l_nout = 0;
          }
        }
        
        size_t nv_overestimate = allreduce<int64_t,collective_add>(g->l_vmap.size());
        g->nv_over = nv_overestimate; // (over-estimate, includes ghosts)
        
        if (mycore() == 0) VLOG(0) << "total_vert_ct => " << nv_overestimate;
      }); // on_all_cores
      
      ///////////////////////////////////////////////////////////////////
      // find all the existing mirrors of each vertex, choose a master, 
      // and propagate this info
      auto mirror_map = GlobalHashMap<VertexID,CoreSet>::create(g->nv_over);
      
      on_all_cores([=]{
        finish([=]{
          for (auto& v : g->l_verts) {
            auto c = mycore();
            mirror_map->insert_async(v.id, [c](CoreSet& cs){ cs.insert(c); });
          }
        });
      });
      
      forall(mirror_map, [g](const VertexID& k, CoreSet& cs){
        int rnd = rand() % cs.size();
        int i=0;
        Core master = -1;
        for (auto c : cs) {
          if (i == rnd) {
            master = c;
            break;
          }
          i++;
        }
        CHECK(master >= 0);
        
        // TODO: do in bulk?
        for (auto c : cs) {
          delegate::call<async>(master, [=]{
            g->l_masters[k].mirrors.push_back(c);
          });
        }
      });
      
      on_all_cores([=]{
        finish([=]{
          for (auto& p : g->l_masters) {
            auto& vid = p.first;
            auto& master = p.second;
          
            VLOG(3) << "master<" << vid << ">: " << master.mirrors;
          
            auto ga = make_global(g->l_vmap[vid]);
            for (auto c : master.mirrors) {
              delegate::call<async>(c, [=]{
                g->l_vmap[vid]->master = ga;
              });
            }
          }
        });
        
        if (VLOG_IS_ON(3)) {
          for (auto& v : g->l_verts) {
            std::cerr << "<" << std::setw(2) << v.id << "> master:" << v.master << "\n";
          }
        }
        
      });
      
      return g;
    }
  } GRAPPA_BLOCK_ALIGNED;

}

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
  bool active, active_minor_step;
  
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
    
    VLOG(1) << "iteration " << std::setw(3) << iteration
            << " -- active:" << V::total_active;
    double t = walltime();
    
    forall(g, [=](GVertex& v){
      if (v->active) {
        v->active_minor_step = true;
        v->deactivate();
      }
    });
    
    forall(g, [=](GVertex& v){
      if (!v->active_minor_step) return;
      
      auto& p = prog(v);
      
      // apply
      p.apply(v, p.cache);
      
      v->active_minor_step = p.scatter_edges(v);
    });
    
    forall(g, [=](GVertex& v){
      if (v->active_minor_step) {
        auto prog_copy = prog(v);
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
