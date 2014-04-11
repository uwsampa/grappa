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

namespace Grappa {
  
  namespace impl { class GraphlabGraphBase {}; }
  
  template< typename V, typename E >
  class GraphlabGraph : public impl::GraphlabGraphBase {
  public:
    struct Vertex;
    
    struct Edge {
      VertexID src, dst;
      E data;
      Vertex *srcv, *dstv;
      
      Edge() = default;
      Edge(const TupleGraph::Edge& e): src(e.v0), dst(e.v1) {}
      
      friend std::ostream& operator<<(std::ostream& o, const Edge& e) {
        return o << "<" << e.src << "," << e.dst << ">";
      }
      
      Vertex& source() { return *srcv; }
      Vertex& dest() { return *dstv; }
    };
    
    struct MasterInfo {
      std::vector<Core> mirrors;
    };
    
    struct Vertex {
      VertexID id;
      V data;
      GlobalAddress<Vertex> master;
      size_t n_in, n_out;
      
      Edge * l_out;
      size_t l_nout;
      
      static Reducer<int64_t,ReducerType::Add> total_active;
  
      void* prog;
      bool active, active_minor_step;
      
      Vertex(VertexID id = -1): id(id), data(), n_in(), n_out(), l_out(nullptr), l_nout(0), prog(nullptr), active(false), active_minor_step(false) {}
      
      V* operator->(){ return &data; }
      const V* operator->() const { return &data; }
      
      size_t num_in_edges() const { return n_in; }
      size_t num_out_edges() const { return n_out; }
        
      void activate() {
        if (!active) {
          if (is_master()) total_active++;
          active = true;
        }
      }
      void deactivate() {
        if (active) {
          if (is_master()) total_active--;
          active = false;
        }
      }
      
      bool is_master() { return master.core() == mycore(); }
    };
    
    GlobalAddress<GraphlabGraph> self;
    size_t nv, nv_over;
    
    std::vector<Edge> l_edges;   ///< Local edges
    std::vector<Vertex> l_verts; ///< Local vertices (indexes into l_edges)
    size_t l_nsrc;             ///< Number of source vertices
    
    unordered_map<VertexID,Vertex*> l_vmap;
    unordered_map<VertexID,MasterInfo> l_masters;
    
    static GlobalCompletionEvent phaser;
    
  private:
    
    GraphlabGraph(GlobalAddress<GraphlabGraph> self)
      : self(self) , l_edges() , l_verts() , l_nsrc(0) , l_vmap()
    { }
    
  public:
    GraphlabGraph() = default;
    ~GraphlabGraph() = default;
    
    static GlobalAddress<GraphlabGraph> create(TupleGraph tg) {
      VLOG(1) << "GraphlabGraph::create( directed, greedy_oblivious )";
      auto g = symmetric_global_alloc<GraphlabGraph>();
      
#define LOG_ALL_CORES(NAME, TYPE, WHAT) \
      if (VLOG_IS_ON(2)) { \
        barrier(); \
        if (mycore() == 0) { \
          std::vector<TYPE> lst; lst.resize(cores()); \
          for (Core c = 0; c < cores(); c++) { \
            lst[c] = delegate::call(c, [=]{ \
              return (WHAT); \
            }); \
          } \
          std::cerr << util::array_str(NAME, lst) << "\n"; \
        } \
        barrier(); \
      }
      
      
      on_all_cores([=]{
        srand(12345);
        // intialize graph
        new (g.localize()) GraphlabGraph(g);
        
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
          if (e.v0 == e.v1) {
            assignments[idx(e)] = CoreSet::INVALID;
            continue;
          }
          
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
        
        if (mycore() == 0 && VLOG_IS_ON(2)) {
          std::cerr << util::array_str("edge_cts", edge_cts, cores()) << "\n";
        }
        
        //////////////////////////
        // actually scatter edges
        auto& edges = g->l_edges;
        
        edges.reserve(edge_cts[mycore()]);
        barrier();
        
        
        for (auto& e : local_edges) {
          auto target = assignments[idx(e)];
          if (target != CoreSet::INVALID) {
            auto e_copy = e;
            delegate::call<async,&phaser>(target, [e_copy,g]{
              g->l_edges.emplace_back(e_copy);
            });
          }
        }
        phaser.wait();
        
        // if (mycore() == 0) global_free(tg.edges);        
        std::sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b){
          if (a.src == b.src) return a.dst < b.dst;
          else return a.src < b.src;
        });
        
        auto it = std::unique(edges.begin(), edges.end(), [](const Edge& a, const Edge& b){
          return (a.src == b.src) && (a.dst == b.dst);
        });
        edges.resize(std::distance(edges.begin(), it));
        
        // VLOG(3) << "l_edges: " << edges;
        
        auto& l_vmap = g->l_vmap;
        auto& lvs = g->l_verts;
        
        // so we can pre-size l_verts and get all the hashtable allocs over with
        for (auto& e : edges) {
          for (auto v : {e.src, e.dst}) {
            l_vmap[v] = nullptr;
          }
        }
        lvs.reserve(l_vmap.size());
        
        LOG_ALL_CORES("edges", size_t, g->l_edges.size());
        LOG_ALL_CORES("vertices", size_t, g->l_vmap.size());
                
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
        g->nv_over = nv_overestimate; // (over-estimate, includes mirrors)
        g->nv = 0;
        if (mycore() == 0) VLOG(2) << "total_vert_ct: " << nv_overestimate;
      }); // on_all_cores
      
      ///////////////////////////////////////////////////////////////////
      // find all the existing mirrors of each vertex, choose a master, 
      // and propagate this info
      auto mirror_map = GlobalHashMap<VertexID,CoreSet>::create(g->nv_over);
      
      on_all_cores([=]{
        for (auto& v : g->l_verts) {
          auto c = mycore();
          insert<async,&phaser>(mirror_map, v.id, [c](CoreSet& cs){
            cs.insert(c);
          });
        }
      });
      phaser.wait();
      
      forall<&phaser>(mirror_map, [g](const VertexID& k, CoreSet& cs){
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
          delegate::call<async,&phaser>(master, [=]{
            g->l_masters[k].mirrors.push_back(c);
          });
        }
      });
      
      
      on_all_cores([=]{
        LOG_ALL_CORES("masters", size_t, g->l_masters.size());
        
        // propagate 'master' info to all mirrors
        for (auto& p : g->l_masters) {
          auto& vid = p.first;
          MasterInfo& master = p.second;
          
          VLOG(4) << "master<" << vid << ">: "
            << util::array_str(master.mirrors);
        
          auto ga = make_global(g->l_vmap[vid]);
          for (auto c : master.mirrors) {
            delegate::call<async,&phaser>(c, [=]{
              g->l_vmap[vid]->master = ga;
            });
          }
        }
      });
      phaser.wait();
      
      on_all_cores([=]{
        
        // have each mirror send its local n_in/n_out counts to master
        auto& vm = g->l_vmap;
        for (auto& e : g->l_edges) {
          e.srcv = vm[e.src];
          e.dstv = vm[e.dst];
          
          e.srcv->n_out++;
          e.dstv->n_in++;
        }
        
        for (auto& v : g->l_verts) {
          auto n_in = v.n_in, n_out = v.n_out;
          if (v.master.core() != mycore()) {
            call<async,&phaser>(v.master, [=](Vertex& m){
              m.n_in += n_in;
              m.n_out += n_out;
            });
          }
        }
      });
      phaser.wait();
      on_all_cores([=]{ 
        // send totals back out to each of the mirrors
        for (auto& v : g->l_verts) {
          if (v.master.core() == mycore()) {
            // from master:
            auto vid = v.id;
            auto n_in = v.n_in, n_out = v.n_out;
            auto& master = g->l_masters[v.id];
            // to all the other mirrors (excluding the master):
            for (auto c : master.mirrors) if (c != mycore()) {
              delegate::call<async,&phaser>(c, [=]{
                g->l_vmap[vid]->n_in = n_in;
                g->l_vmap[vid]->n_out = n_out;
              });
            }
          }
        }
      });
      phaser.wait();
      on_all_cores([=]{
        if (VLOG_IS_ON(4)) {
          for (auto& v : g->l_verts) {
            std::cerr << "<" << std::setw(2) << v.id << "> master:" << v.master << "\n";
          }
        }
        
        g->nv = allreduce<int64_t,collective_add>(g->l_masters.size());
      });
            
      VLOG(0) << "num_vertices: " << g->nv;
      VLOG(0) << "replication_factor: " << (double)g->nv_over / g->nv;
      return g;
    }
    
  } GRAPPA_BLOCK_ALIGNED;
  
  template< SyncMode S = SyncMode::Blocking,
            GlobalCompletionEvent * C = &impl::local_gce,
            typename V = nullptr_t, typename E = nullptr_t,
            typename F = nullptr_t >
  void on_mirrors(GlobalAddress<GraphlabGraph<V,E>> g, typename GraphlabGraph<V,E>::Vertex& v, F work) {
    auto id = v.id;
    for (auto c : g->l_masters[id].mirrors) if (c != mycore()) {
      delegate::call<S,C>(c, [=]{
        work(*g->l_vmap[id]);
      });
    }
  }
  
  template< typename V, typename E >
  Reducer<int64_t,ReducerType::Add> GraphlabGraph<V,E>::Vertex::total_active;

  template< typename V, typename E >
  GlobalCompletionEvent GraphlabGraph<V,E>::phaser;
  
  struct Iter {
    /// Iterator over master vertices in GraphlabGraph.
    template< typename G, class = typename std::enable_if<std::is_base_of<impl::GraphlabGraphBase,G>::value>::type >
    struct Masters { GlobalAddress<G> g; };
    
    /// Iterator over all vertices in GraphlabGraph.
    template< typename G, class = typename std::enable_if<std::is_base_of<impl::GraphlabGraphBase,G>::value>::type >
    struct Mirrors { GlobalAddress<G> g; };
  };
  
  /// Iterator over master vertices in GraphlabGraph.
  template< typename V, typename E >
  Iter::Masters<GraphlabGraph<V,E>> masters(GlobalAddress<GraphlabGraph<V,E>> g) {
    return Iter::Masters<GraphlabGraph<V,E>>{ g };
  }

  /// Iterator over all vertices in GraphlabGraph.
  template< typename V, typename E >
  Iter::Mirrors<GraphlabGraph<V,E>> mirrors(GlobalAddress<GraphlabGraph<V,E>> g) {
    return Iter::Mirrors<GraphlabGraph<V,E>>{ g };
  }
  
  namespace impl {
  
    /// Iterate over just master vertices in GraphlabGraph.
    template< GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(Iter::Masters<G> it, F body,
        void (F::*mf)(typename G::Vertex&) const) {
      finish<C>([=]{
        on_all_cores([=]{
          auto g = it.g;
          forall_here<TaskMode::Bound,SyncMode::Async,C,Threshold>
          (0, g->l_verts.size(), [g,body](int64_t i){
            auto& v = g->l_verts[i];
            if (v.is_master()) {
              body(v);
            }
          });
        });
      });
    }
    
    /// Iterate over just master vertices in GraphlabGraph.
    template< GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(Iter::Masters<G> it, F body,
        void (F::*mf)(typename G::Vertex&, typename G::MasterInfo&) const) {
      finish<C>([=]{
        on_all_cores([=]{
          auto g = it.g;
          forall_here<TaskMode::Bound,SyncMode::Async,C,Threshold>
          (0, g->l_verts.size(), [g,body](int64_t i){
            auto& v = g->l_verts[i];
            if (v.is_master()) {
              body(v, g->l_masters[v.id]);
            }
          });
        });
      });
    }    
    
    /// Iterate over all vertices including mirrors
    template< GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
    void forall(Iter::Mirrors<G> it, F body,
                void (F::*mf)(typename G::Vertex&) const) {
      finish<C>([=]{
        on_all_cores([=]{
          auto g = it.g;
          forall_here<TaskMode::Bound,SyncMode::Async,C,Threshold>
          (0, g->l_verts.size(), [g,body](int64_t i){
            body(g->l_verts[i]);
          });
        });
      });
    }
    
    /// Iterate over each vertex once; equivalent to forall(masters(g),...).
    template< GlobalCompletionEvent * C, int64_t Threshold, typename V, typename E, typename F >
    void forall(GlobalAddress<GraphlabGraph<V,E>> g, F body,
                void (F::*mf)(typename GraphlabGraph<V,E>::Vertex&) const) {
      forall(masters(g), body, &F::operator());
    }

    /// Iterate over all edges
    template< GlobalCompletionEvent * C, int64_t Threshold, typename V, typename E, typename F >
    void forall(GlobalAddress<GraphlabGraph<V,E>> g, F body,
                void (F::*mf)(typename GraphlabGraph<V,E>::Edge&) const) {
      finish<C>([=]{
        on_all_cores([=]{
          forall_here<TaskMode::Bound,SyncMode::Async,C,Threshold>
          (0, g->l_edges.size(), [g,body](int64_t i){
            body(g->l_edges[i]);
          });
        });
      });
    }
    
  }
  
  template< GlobalCompletionEvent * C = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename Iter,
            typename F = nullptr_t >
  void forall(Iter it, F body) {
    impl::forall<C,Threshold>(it, body, &F::operator());
  }
  
} // namespace Grappa

template< typename G, typename GatherType >
struct GraphlabVertexProgram {
  using Vertex = typename G::Vertex;
  using Edge = typename G::Edge;
  using Gather = GatherType;
  
  GatherType cache;
  
  GraphlabVertexProgram(): cache() {}
  
  void post_delta(GatherType d) { cache += d; }
  void reset() { cache = GatherType(); }
};

template< typename Subclass >
struct GraphlabVertexData {
};

template< typename V, typename E >
void activate_all(GlobalAddress<Graph<V,E>> g) {
  forall(g, [](typename Graph<V,E>::Vertex& v){ v->activate(); });
}

template< typename V, typename E >
void activate_all(GlobalAddress<GraphlabGraph<V,E>> g) {
  forall(mirrors(g), [](typename GraphlabGraph<V,E>::Vertex& v){
    v.activate();
  });
}

template< typename V >
void activate(GlobalAddress<V> v) {
  delegate::call(v, [](V& v){ v->activate(); });
}

enum class EdgeDirection { None, In, Out, All };

template< typename G, typename VertexProg,
  class = typename std::enable_if<
    std::is_base_of<impl::GraphlabGraphBase,G>
  // ::value>::type,
  // class = typename std::enable_if<
  //   std::is_base_of<GraphlabVertexProgram,VertexProg>
  ::value>::type >
struct GraphlabEngine {
  using Vertex = typename G::Vertex;
  using Edge = typename G::Edge;
  using MasterInfo = typename G::MasterInfo;
  
  static VertexProg& prog(Vertex& v) { return *static_cast<VertexProg*>(v.prog); };
  
  /// 
  /// Assuming: `gather_edges = EdgeDirection::In`
  /// 
  static void run_sync(GlobalAddress<G> g) {
    
    VLOG(1) << "GraphlabEngine::run_sync(active:" << Vertex::total_active << ")";
    
    ///////////////
    // initialize
    forall(mirrors(g), [=](Vertex& v){
      v.prog = new VertexProg(v);
    });
    
    int iteration = 0;
    while ( Vertex::total_active > 0 && iteration < FLAGS_max_iterations )
        GRAPPA_TIME_REGION(iteration_time) {
      VLOG(1) << "iteration " << iteration;
      VLOG(1) << "  active: " << Vertex::total_active;
      double t = walltime();
      
      ////////////////////////////////////////////////////////////
      // gather (TODO: do this in fewer 'forall's)
      
      // reset cache
      forall(mirrors(g), [=](Vertex& v){
        prog(v).reset();
        v.active_minor_step = false;
      });
      
      // gather in_edges
      forall(g, [=](Edge& e){
        auto& v = e.dest();
        if (v.active) {
          auto& p = prog(v);
          p.cache += p.gather(v, e);
        }
      });
    
      // send accumulated gather to master to compute total
      forall(mirrors(g), [=](Vertex& v){
        if (v.active) {
          auto& p = prog(v);
          if (v.master.core() != mycore()) {
            auto accum = p.cache;
            call<async>(v.master, [=](Vertex& m){
              prog(m).cache += accum;
            });
          }
        }
      });
    
      ////////////////////////////////////////////////////////////
      // apply
      forall(mirrors(g), [=](Vertex& v){
        if (v.active) {
          v.deactivate();
          v.active_minor_step = true;
        }
      });
      
      forall(masters(g), [=](Vertex& v, MasterInfo& master){
        if (!v.active_minor_step) return;
        
        auto& p = prog(v);
        p.apply(v, p.cache);
        
        v.active_minor_step = p.scatter_edges(v);
        
        // broadcast out to mirrors
        auto p_copy = p;
        auto data = v.data;
        auto id = v.id;
        auto active = v.active_minor_step;
        for (auto c : master.mirrors) if (c != mycore()) {
          delegate::call<async>(c, [=]{
            auto& v = *g->l_vmap[id];
            v.data = data;
            v.active_minor_step = active;
            prog(v) = p_copy;
          });
        }
      });
      
      ////////////////////////////////////////////////////////////
      // scatter
      forall(g, [=](Edge& e){
        // scatter out_edges
        auto& v = e.source();
        if (v.active_minor_step) {
          auto& p = prog(v);
          p.scatter(e, e.dest());
        }
      });
      
      // only thing that should've been changed about vertices is activation,
      // so make sure all mirrors know (send to master, then broadcast)
      forall(mirrors(g), [=](Vertex& v){
        if (v.active) {
          delegate::call<async>(v.master, [=](Vertex& m){
            m.activate();
          });
        }
      });
      forall(masters(g), [=](Vertex& v){
        if (v.active) {
          on_mirrors<async>(g, v, [](Vertex& m){
            m.activate();
          });
        }
      });
      
      iteration++;
      VLOG(1) << "  time:   " << walltime()-t;
    } // while
  }
};


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
