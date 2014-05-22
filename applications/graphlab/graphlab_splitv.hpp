////////////////////////////////////////////////////////////////////////
/// GraphLab is an API and runtime system for graph-parallel computation.
/// This is a rough prototype implementation of the programming model to
/// demonstrate using Grappa as a platform for other models.
/// More information on the actual GraphLab system can be found at:
/// graphlab.org.
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Implementation of GraphLab's split-vertex graph representation and
/// synchronous engine built for it.
///
/// (included from graphlab.hpp)

#include "graphlab_borrowed.hpp"

GRAPPA_DECLARE_METRIC(SummarizingMetric<int>, core_set_size);


#define MAX_CORES 256
using CoreBitset = graphlab::fixed_dense_bitset<MAX_CORES>;

using CoreSet = SmallLocalSet<Core>;

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
    
    struct MasterInfo;
    
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
      
      MasterInfo* master_info;

      Vertex(VertexID id = -1): id(id), data(), n_in(), n_out(), l_out(nullptr), l_nout(0), prog(nullptr), active(false), active_minor_step(false) {}

      V* operator->(){ return &data; }
      const V* operator->() const { return &data; }

      size_t num_in_edges() const { return n_in; }
      size_t num_out_edges() const { return n_out; }
      
      /// Activate vertex (or mirror)
      /// @return true if first activation
      bool activate() {
        if (!active) {
          if (is_master()) total_active++;
          active = true;
          return true;
        } else {
          return false;
        }
      }
      void deactivate() {
        if (active) {
          if (is_master()) total_active--;
          active = false;
        }
      }

      bool is_master() {
        return master.core() == mycore() && master.pointer() == this;
      }
    };
    
    struct MasterInfo {
      std::vector<Core> mirrors;
      std::vector<GlobalAddress<Vertex>> mirror_verts;
    };

    GlobalAddress<GraphlabGraph> self;
    size_t nv, nmirrors, ne;

    std::vector<Edge> l_edges;   ///< Local edges
    std::vector<Vertex> l_verts; ///< Local vertices (indexes into l_edges)
    size_t l_nsrc;             ///< Number of source vertices

    unordered_map<VertexID,Vertex*> l_vmap;
    unordered_map<VertexID,MasterInfo> l_masters;
    std::vector<Vertex> l_master_verts; ///< Master vertices
    
    static GlobalCompletionEvent phaser;
    
    size_t tmp;
    
  private:

    GraphlabGraph(GlobalAddress<GraphlabGraph> self)
      : self(self) , l_edges() , l_verts() , l_nsrc(0) , l_vmap()
    { }

  public:
    GraphlabGraph() = default;
    ~GraphlabGraph() = default;
    
    /// Greedy assign (source, target) to a machine.
    /// 
    /// @param source      source vertex id
    /// @param target      destination vertex id
    /// @param src_degree  the degree presence of source over machines
    /// @param dst_degree  the degree presence of target over machines
    /// @param edge_cts    the edge counts over machines
    /// @param usehash     consistently hash edges to nodes
    /// @param userecent   ???
    /// 
    /// This function was copied nearly verbatim from GraphLab's source code.
    /// 
    static Core edge_to_core_greedy (const VertexID source, 
       const VertexID target,
       CoreBitset& src_degree,
       CoreBitset& dst_degree,
       size_t* edge_cts,
       bool usehash = true,
       bool userecent = false)
    {
       
      // Compute the score of each proc.
      Core best_proc = -1; 
      double maxscore = 0.0;
      double epsilon = 1.0; 
      std::vector<double> proc_score(cores()); 
      size_t minedges = *std::min_element(edge_cts, edge_cts+cores());
      size_t maxedges = *std::max_element(edge_cts, edge_cts+cores());
      
      for (size_t i = 0; i < cores(); ++i) {
        size_t sd = src_degree.get(i) + (usehash && (source % cores() == i));
        size_t td = dst_degree.get(i) + (usehash && (target % cores() == i));
        double bal = (maxedges - edge_cts[i])/(epsilon + maxedges - minedges);
        proc_score[i] = bal + ((sd > 0) + (td > 0));
      }
      maxscore = *std::max_element(proc_score.begin(), proc_score.end());
   
      std::vector<Core> top_procs; 
      for (size_t i = 0; i < cores(); ++i)
        if (std::fabs(proc_score[i] - maxscore) < 1e-5)
          top_procs.push_back(i);

      // Hash the edge to one of the best procs.
      typedef std::pair<VertexID, VertexID> edge_pair_type;
      const edge_pair_type edge_pair(std::min(source, target), 
                                     std::max(source, target));
      best_proc = top_procs[hash_edge(edge_pair) % top_procs.size()];
      
      CHECK_LT(best_proc, cores());
      if (userecent) {
        src_degree.clear();
        dst_degree.clear();
      }
      src_degree.set_bit(best_proc);
      dst_degree.set_bit(best_proc);
      edge_cts[best_proc]++;
      return best_proc;
    }
        
    static GlobalAddress<GraphlabGraph> create(TupleGraph tg) {
      VLOG(1) << "GraphlabGraph::create( directed, greedy_oblivious )";
      auto g = symmetric_global_alloc<GraphlabGraph>();
      
      on_all_cores([=]{
        double t = walltime();
        
        /// MARK: assigning edges
        PHASE_BEGIN("  assigning edges");
        PHASE_BEGIN("  - assign");
        
        srand(12345);
        // intialize graph
        new (g.localize()) GraphlabGraph(g);

        // vertex placements that this core knows about (array of cores, each with a set of vertices mapped to it)
        auto local_edges = iterate_local(tg.edges, tg.nedge);
        auto nlocal = local_edges.size();
        auto idx = [&](TupleGraph::Edge& e){ return &e - local_edges.begin(); };
        
        std::vector<Core> assignments; assignments.resize(local_edges.size());
        size_t* edge_cts = locale_alloc<size_t>(cores());
        Grappa::memset(edge_cts, 0, cores());
        
        {
          unordered_map<VertexID,CoreBitset> vplace;
        
          /// cores this vertex has been placed on
          auto vcores = [&](VertexID i) -> CoreBitset& {
            if (vplace.count(i) == 0) {
              vplace[i];
            }
            return vplace[i];
          };
        
          auto cmp_load = [&](Core c0, Core c1) { return edge_cts[c0] < edge_cts[c1]; };

          for (auto& e : local_edges) {
            auto i = idx(e);
            if (e.v0 == e.v1) {
              assignments[i] = INVALID;
            } else {
              auto &vs0 = vcores(e.v0), &vs1 = vcores(e.v1);
              assignments[i] = edge_to_core_greedy(e.v0, e.v1, vs0, vs1, edge_cts);
            }
          }
        
          barrier();
          PHASE_END();
          
          g->tmp = vplace.size();
          LOG_ALL_CORES("bitset_fraction_verts", size_t, g->tmp);
        }
        
        /// MARK: scattering
        PHASE_BEGIN("  scattering");

        //////////////////////////
        // actually scatter edges
        barrier();
        
        PHASE_BEGIN("  - pre-sorting");
        // std::cerr << util::array_str("assignments", assignments) << std::endl;
        std::vector<int> counts(cores()), offsets(cores()), scan(cores());
        // std::cerr << util::array_str("counts", counts) << std::endl;
        for (auto c : assignments) if (c != INVALID) counts[c]++;
        // std::cerr << util::array_str("counts", counts) << std::endl;
        std::partial_sum(counts.begin(), counts.end(), scan.begin());
        offsets[0] = 0; for (size_t i=1; i<cores(); i++) offsets[i] = scan[i-1];
        
        // std::cerr << util::array_str("counts", counts) << std::endl;
        // std::cerr << util::array_str("offsets", offsets) << std::endl;
        
        std::vector<TupleGraph::Edge> sorted(scan.back());
        for (auto& e : local_edges) {
          auto a = assignments[idx(e)];
          if (a != INVALID) sorted[ offsets[a]++ ] = e;
        }
        offsets[0] = 0; for (size_t i=1; i<cores(); i++) offsets[i] = scan[i-1];
        
        // get receive count from other cores
        std::vector<int> rcounts(cores()), roffsets(cores());
        
        PHASE_END(); PHASE_BEGIN("  - MPI_Alltoall");
        auto counts_ptr = &counts[0];
        auto rcounts_ptr = &rcounts[0];
        global_communicator.with_request_do_blocking([=](MPI_Request* request) {
          MPI_CHECK(
            MPI_Ialltoall(counts_ptr, 1, MPI_INT,
                          rcounts_ptr, 1, MPI_INT,
                          MPI_COMM_WORLD, request)
          );
        });
        
        std::partial_sum(rcounts.begin(), rcounts.end(), scan.begin());
        roffsets[0] = 0; for (size_t i=1; i<cores(); i++) roffsets[i] = scan[i-1];
        
        // VLOG(0) << "edges.size => " << edges.capacity();
        // VLOG(0) << "scan counts => " << scan.back();
        
        // std::cerr << util::array_str("rcounts", rcounts) << std::endl;
        // std::cerr << util::array_str("roffsets", roffsets) << std::endl;
        
        MPI_Datatype mpi_edge_type;
        MPI_Type_contiguous(2, MPI_INT64_T, &mpi_edge_type);
        
        PHASE_END();
        
        auto& edges = g->l_edges;
        
        auto nrecv = scan.back();
        edges.reserve(nrecv);
        auto buf = new TupleGraph::Edge[edges.capacity()];
        
        PHASE_BEGIN("  - MPI_Alltoallv");
        global_communicator.with_request_do_blocking([&](MPI_Request* request) {
          MPI_CHECK(
            MPI_Ialltoallv(&sorted[0], &counts[0], &offsets[0], mpi_edge_type,
                           buf, &rcounts[0], &roffsets[0], mpi_edge_type,
                           MPI_COMM_WORLD, request)
          );
        });
        PHASE_END(); PHASE_BEGIN("  - copying");
        
        for (size_t i = 0; i < nrecv; i++) { edges.emplace_back(buf[i]); }
        delete[] buf;
        
        PHASE_END();
      });
      
      on_all_cores([=]{
        double t;
        auto& edges = g->l_edges;
        
        /// MARK: sorting
        PHASE_BEGIN("  sorting");
        
        size_t before = edges.size();
        
        // if (mycore() == 0) global_free(tg.edges);
        std::sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b){
          if (a.src == b.src) return a.dst < b.dst;
          else return a.src < b.src;
        });
        
        auto it = std::unique(edges.begin(), edges.end(), [](const Edge& a, const Edge& b){
          return (a.src == b.src) && (a.dst == b.dst);
        });
        edges.resize(std::distance(edges.begin(), it));
        
        size_t after = edges.size();
        // VLOG(1) << "fraction_edges_elim: " << (double)(before-after)/before;
        
        PHASE_END(); PHASE_BEGIN("  creating mirror vertices");
        
        auto& l_vmap = g->l_vmap;
        auto& lvs = g->l_verts;

        // so we can pre-size l_verts and get all the hashtable allocs over with

        PHASE_BEGIN("  - filling map");
        for (auto& e : edges) {
          for (auto v : {e.src, e.dst}) {
            l_vmap[v] = nullptr;
          }
        }
        PHASE_END(); PHASE_BEGIN("  - creating mirror vertices");
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

        size_t ct = allreduce<int64_t,collective_add>(g->l_vmap.size());
        g->nmirrors = ct; // (over-estimate, includes mirrors)
        g->nv = 0;
        PHASE_END();
        if (mycore() == 0) VLOG(2) << "nmirrors: " << ct;
      }); // on_all_cores

      ///////////////////////////////////////////////////////////////////
      // find all the existing mirrors of each vertex, choose a master,
      // and propagate this info
      double t = walltime();
      
      /// MARK: assigning masters
      PHASE_BEGIN("  assigning masters");

      auto mirror_map = GlobalHashMap<VertexID,CoreSet>::create(g->nmirrors);

      PHASE_BEGIN("  - inserting in global map");

      on_all_cores([=]{
        for (auto& v : g->l_verts) {
          auto c = mycore();
          insert<async,&phaser>(mirror_map, v.id, [c](CoreSet& cs){
            cs.insert(c);
          });
        }
      });
      phaser.wait();

      PHASE_END(); PHASE_BEGIN("  - choosing & collecting");

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

      /// MARK: propagating masters out to mirrors
      PHASE_END(); PHASE_BEGIN("  - propagating");

      on_all_cores([=]{
        LOG_ALL_CORES("masters", size_t, g->l_masters.size());

        // propagate 'master' info to all mirrors
        g->l_master_verts.reserve(g->l_masters.size());
        for (auto& p : g->l_masters) {
          auto& vid = p.first;
          MasterInfo& master = p.second;

          VLOG(4) << "master<" << vid << ">: "
            << util::array_str(master.mirrors);
          
          g->l_master_verts.emplace_back(vid);
          Vertex& master_v = g->l_master_verts.back();
          master_v.master_info = &master;
          auto ga = make_global(&master_v);
          // std::cerr << "[" << std::setw(2) << vid << "] master: " << ga << ", mirrors: " << util::array_str(master.mirrors) << "\n";
          ga->master = ga;
          master.mirror_verts.reserve(master.mirrors.size());
          
          for (auto c : master.mirrors) {
            delegate::call<async,&phaser>(c, [=]{
              // std::cerr << "[" << std::setw(2) << vid << "] set master -> " << ga << "\n";
              CHECK(g->l_vmap.count(vid)) << "—— vid: " << vid;
              g->l_vmap[vid]->master = ga;
            });
          }
        }
      });
      phaser.wait();

      PHASE_END();
      VLOG(1) << "  computing in/out";
      PHASE_BEGIN("  - collect");
      
      phaser.enroll(cores());
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
          // std::cerr << "[" << std::setw(2) << v.id << "] n_out:" << v.n_out << "\n";
          auto n_in = v.n_in, n_out = v.n_out;
          // std::cerr << "[" << std::setw(2) << v.id << "] master on " << v.master.core() << ", @ " << v.master.pointer() << "\n";
          auto ga = make_global(&v);
          delegate::call<async,&phaser>(v.master, [=](Vertex& m){
            m.n_in += n_in;
            m.n_out += n_out;
            m.master_info->mirror_verts.push_back(ga);
          });
        }
        phaser.send_completion(0);
      });
      phaser.wait();
      PHASE_END(); PHASE_BEGIN("  - propagate");
      phaser.enroll(cores());
      on_all_cores([=]{
        // send totals back out to each of the mirrors
        for (Vertex& v : g->l_master_verts) {
          // std::cerr << "m[" << std::setw(2) << v.id << "] n_out:" << v.n_out << " @ " << &v << "\n";
          
          // from master:
          auto vid = v.id;
          auto n_in = v.n_in, n_out = v.n_out;
          CHECK(g->l_masters.count(v.id)) << "—— v.id = " << v.id;
          auto& master = g->l_masters[v.id];
          // to all the other mirrors (excluding the master):
          for (auto c : master.mirrors) {
            delegate::call<async,&phaser>(c, [=]{
              CHECK(g->l_vmap.count(vid));
              g->l_vmap[vid]->n_in = n_in;
              g->l_vmap[vid]->n_out = n_out;
            });
          }
        }
        phaser.send_completion(0);
      });
      phaser.wait();
      PHASE_END();
      
      on_all_cores([=]{
        if (VLOG_IS_ON(4)) {
          for (auto& v : g->l_verts) {
            std::cerr << "<" << std::setw(2) << v.id << "> master:" << v.master << "\n";
          }
        }
        
        CHECK_EQ(g->l_master_verts.size(), g->l_masters.size());
        
        g->nv = allreduce<int64_t,collective_add>(g->l_master_verts.size());
        g->ne = allreduce<int64_t,collective_add>(g->l_edges.size());
      });
      PHASE_END();

      VLOG(0) << "num_vertices: " << g->nv;
      VLOG(0) << "replication_factor: " << (double)g->nmirrors / g->nv;
      return g;
    }

  } GRAPPA_BLOCK_ALIGNED;

  template< SyncMode S = SyncMode::Blocking,
            GlobalCompletionEvent * C = &impl::local_gce,
            typename V = nullptr_t, typename E = nullptr_t,
            typename F = nullptr_t >
  void on_mirrors(GlobalAddress<GraphlabGraph<V,E>> g, typename GraphlabGraph<V,E>::Vertex& v, F work) {
    auto id = v.id;
    CHECK( g->l_masters.count(id) );
    for (auto gv : v.master_info->mirror_verts) {
      delegate::call<S,C>(gv, [=](typename GraphlabGraph<V,E>::Vertex& v){
        work(v);
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
          (0, g->l_master_verts.size(), [g,body](int64_t i){
            auto& v = g->l_master_verts[i];
            body(v);
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
          (0, g->l_master_verts.size(), [g,body](int64_t i){
            auto& v = g->l_master_verts[i];
            body(v, g->l_masters[v.id]);
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
      finish<C>([=]{
        on_all_cores([=]{
          forall_here<TaskMode::Bound,SyncMode::Async,C,Threshold>
          (0, g->l_master_verts.size(), [g,body](int64_t i){
            body(g->l_master_verts[i]);
          });
          forall_here<TaskMode::Bound,SyncMode::Async,C,Threshold>
          (0, g->l_verts.size(), [g,body](int64_t i){
            body(g->l_verts[i]);
          });
        });
      });
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

template< typename V, typename E >
void activate_all(GlobalAddress<GraphlabGraph<V,E>> g) {
  forall(g, [](typename GraphlabGraph<V,E>::Vertex& v){
    v.activate();
  });
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
  using Gather = typename VertexProg::Gather;

  static VertexProg& prog(Vertex& v) { return *static_cast<VertexProg*>(v.prog); };
  
  static GlobalAddress<G> g;
  static VertexProg* prog_storage;
  
  ///
  /// Assuming: `gather_edges = EdgeDirection::In`
  ///
  static void run_sync(GlobalAddress<G> g_in, bool delta_caching = true) {
    
    VLOG(1) << "GraphlabEngine::run_sync(active:" << Vertex::total_active << ")";

    ///////////////
    // initialize
    on_all_cores([=]{
      g = g_in;
      
      auto n = g->l_verts.size() + g->l_master_verts.size();
      prog_storage = new VertexProg[n];
      
      size_t i=0;
      auto init_prog = [&i](Vertex& v){
        v.prog = new (prog_storage+i) VertexProg(v);
        v.active_minor_step = false;
        i++;
      };
      
      for (auto& v : g->l_master_verts) init_prog(v);
      for (auto& v : g->l_verts)        init_prog(v);
    });
    
    int iteration = 0;
    while ( Vertex::total_active > 0 && iteration < FLAGS_max_iterations )
        GRAPPA_TIME_REGION(iteration_time) {
      VLOG(1) << "iteration " << iteration;
      VLOG(1) << "  active: " << Vertex::total_active;
      double t = walltime();
      
      ////////////////////////////////////////////////////////////
      // gather (TODO: do this in fewer 'forall's)
      
      if (!delta_caching || iteration == 0) {
        // gather in_edges
        forall(g, [=](Edge& e){
          auto& v = e.dest();
          if (v.active) {
            auto& p = prog(v);
            p.post_delta( p.gather(v, e) );
          }
        });
        
        // send accumulated gather to master to compute total
        forall(mirrors(g), [=](Vertex& v){
          if (v.active) {
            v.deactivate();
            
            auto& p = prog(v);
            auto accum = p.cache;
            call<async>(v.master, [=](Vertex& m){
              prog(m).post_delta( accum );
            });
            p.reset();
          }
        });
      }
            
      ////////////////////////////////////////////////////////////
      // apply
      forall(masters(g), [=](Vertex& m){
        if (!m.active) return;
        m.deactivate();
        
        auto& p = prog(m);
        p.apply(m, p.cache);
        
        auto do_scatter = p.scatter_edges(m);
        
        // broadcast out to mirrors
        auto p_copy = p;
        auto data = m.data;
        auto id = m.id;
        on_mirrors<async>(g, m, [=](Vertex& v){
          v.data = data;
          v.active_minor_step = do_scatter;
          prog(v) = p_copy;
          prog(v).reset();
        });
      });
      
      ////////////////////////////////////////////////////////////
      // scatter
      // (only works for out_edges)
      forall(mirrors(g), [=](Vertex& v){
        if (v.active_minor_step) {
          v.active_minor_step = false;
          
          auto& p = prog(v);
          for (Edge& e : util::iterate(v.l_out, v.l_nout)) {
            auto delta = p.scatter(e, e.dest());
            if (delta_caching) {
              prog(e.dest()).post_delta( delta );
            }
          }
        }
      });
      
      forall(mirrors(g), [=](Vertex& v){
        v.active_minor_step = false;
        if (v.active) {
          if (delta_caching) {
            v.deactivate();
            auto delta = prog(v).cache;
            delegate::call<async>(v.master, [=](Vertex& m){
              m.activate();
              prog(m).post_delta(delta);
            });
          } else {
            delegate::call<async>(v.master, [=](Vertex& m){
              m.activate();
              prog(m).reset();
            });
          }
        }
      });
      
      if (!delta_caching) {
        forall(masters(g), [=](Vertex& m){
          if (m.active) {
            // activate all mirrors for next phase
            on_mirrors<async>(g, m, [](Vertex& v){
              v.activate();
            });
          }
        });          
      }
      
      iteration++;
      VLOG(1) << "  time:   " << walltime()-t;
    } // while
  }
};

template< typename G, typename VertexProg, class C >
GlobalAddress<G> GraphlabEngine<G,VertexProg,C>::g;

template< typename G, typename VertexProg, class C >
VertexProg* GraphlabEngine<G,VertexProg,C>::prog_storage;
