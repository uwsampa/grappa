#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <graph/Graph.hpp>

#ifdef __GRAPPA_CLANG__
#include <Primitive.hpp>
#endif

using namespace Grappa;

struct BFSData {
  int64_t parent;
  int64_t level;
  bool seen;
  
  void init() {
    parent = -1;
    level = 0;
    seen = false;
  }
};

using BFSVertex = Vertex<BFSData>;

DEFINE_bool( metrics, false, "Dump metrics");

DEFINE_int32(scale, 10, "Log2 number of vertices.");
DEFINE_int32(edgefactor, 16, "Average number of edges per vertex.");
DEFINE_int32(nbfs, 1, "Number of BFS traversals to do.");

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, bfs_mteps, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, bfs_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, bfs_nedge, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, graph_create_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, verify_time, 0);

//int64_t *frontier_base, *f_head, *f_tail, *f_level;

struct Frontier {
  int64_t *base, *head, *tail, *level;
  int64_t sz;
  
  void init(size_t sz) {
    this->sz = sz;
    base = head = tail = level = locale_alloc<int64_t>(sz);
  }
  
  void clear() {
    head = tail = level = (base);
  }
  
  void push(int64_t v) { *tail++ = v; assert(tail < base+sz); }
  int64_t pop() { return *head++;     CHECK_LE(head, level); }
  
  int64_t next_size() { return tail - level; }
  void next_level() { level = tail; }
  bool level_empty() { return head >= level; }
};

Frontier frontier;

GlobalCompletionEvent joiner;

int64_t nedge_traversed;


int64_t verify(TupleGraph tg, SymmetricAddress<Graph<BFSVertex>> g, int64_t root) {
  
  auto get_level = [g](int64_t j){
    return delegate::call(g->vs+j, [](BFSVertex& v){ return v->level; });
  };
  auto get_parent = [g](int64_t j){
    return delegate::call(g->vs+j, [](BFSVertex& v){ return v->parent; });
  };
  
  // check root
  delegate::call(g->vs+root, [=](BFSVertex& v){
    CHECK_EQ(v->parent, root);
  });
  
  // compute levels
  delegate::call(g->vs+root, [](BFSVertex& v){ v->level = 0; });
  
  forall(g->vs, g->nv, [=](int64_t i, BFSVertex& v){
    if (v->level >= 0) return;
    
    if (v->parent >= 0 && i != root) {
      int64_t parent = i;
      int64_t nhop = 0;
      int64_t next_parent;
      
      while (parent >= 0 && get_level(parent) < 0 && nhop < g->nv) {
        next_parent = get_parent(parent);
        CHECK_NE(parent, next_parent);
        parent = next_parent;
        ++nhop;
      }
      
      CHECK_LT(nhop, g->nv);
      CHECK_GE(parent, 0);
      
      // now assign levels until we meet an already-leveled vertex
      nhop += get_level(parent);
      parent = i;
      while (get_level(parent) < 0) {
        CHECK_GT(nhop, 0);
        parent = delegate::call(g->vs+parent, [=](BFSVertex& v){
          v->level = nhop;
          return v->parent;
        });
        nhop--;
      }
      CHECK_EQ(nhop, get_level(parent));
    }
  });
  
  call_on_all_cores([]{ nedge_traversed = 0; });
  
  // verify levels & parents match
  forall(tg.edges, tg.nedge, [=](TupleGraph::Edge& e){
    auto max_bfsvtx = g->nv - 1;
    auto i = e.v0, j = e.v1;
    
    int64_t lvldiff;
    
    if (i < 0 || j < 0) return;
    CHECK(!(i > max_bfsvtx && j <= max_bfsvtx)) << "Error!";
    CHECK(!(j > max_bfsvtx && i <= max_bfsvtx)) << "Error!";
    if (i > max_bfsvtx) // both i & j are on the same side of max_bfsvtx
      return;
    
    // All neighbors must be in the tree.
    auto ti = get_parent(i), tj = get_parent(j);
    CHECK(!(ti >= 0 && tj < 0)) << "Error! ti=" << ti << ", tj=" << tj;
    CHECK(!(tj >= 0 && ti < 0)) << "Error! ti=" << ti << ", tj=" << tj;
    if (ti < 0) // both i & j have the same sign
      return;
    
    /* Both i and j are in the tree, count as a traversed edge.
     
     NOTE: This counts self-edges and repeated edges.  They're
     part of the input data.
     */
    nedge_traversed++;
    
    auto mark_seen = [g](int64_t i){
      delegate::call(g->vs+i, [](BFSVertex& v){ v->seen = true; });
    };
    
    // Mark seen tree edges.
    if (i != j) {
      if (ti == j)
        mark_seen(i);
      if (tj == i)
        mark_seen(j);
    }
    lvldiff = get_level(i) - get_level(j);
    /* Check that the levels differ by no more than one. */
    CHECK(!(lvldiff > 1 || lvldiff < -1)) << "Error, levels differ by more than one! " << "(k = " << ", lvl[" << i << "]=" << get_level(i) << ", lvl[" << j << "]=" << get_level(j) << ")";
  });
  
  nedge_traversed = Grappa::reduce<int64_t,collective_add>(&nedge_traversed);
  
  // check that every BFS edge was seen & that there's only one root
  forall(g->vs, g->nv, [=](int64_t i, BFSVertex& v){
    if (i != root) {
      CHECK(!(v->parent >= 0 && !v->seen)) << "Error!";
      CHECK_NE(v->parent, i);
    }
  });
  
  // everything checked out!
  VLOG(1) << "verified!\n";
  return nedge_traversed;
}

#ifdef __GRAPPA_CLANG__
  template< typename T >
  int64_t choose_root(Graph<T> symmetric* g) {
    int64_t root;
    do {
      root = random() % g->nv;
    } while (g->vs[root].nadj == 0);
    return root;
  }
#else
  template< typename T >
  int64_t choose_root(SymmetricAddress<Graph<T>> g) {
    int64_t root;
    do {
      root = random() % g->nv;
    } while (delegate::call(g->vs+root,[](BFSVertex& v){ return v.nadj; }) == 0);
    return root;
  }
#endif

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    int64_t NE = (1L << FLAGS_scale) * FLAGS_edgefactor;
    bool verified = false;
    double t;
    
    t = walltime();
    
    auto tg = TupleGraph::Kronecker(FLAGS_scale, NE, 111, 222);
    
#ifdef __GRAPPA_CLANG__
    auto g = as_ptr( Graph<BFSVertex>::create( tg ) );
#else
    auto g = Graph<BFSVertex>::create( tg );
#endif
    
    graph_create_time = (walltime()-t);
    LOG(INFO) << graph_create_time;
    
    // initialize frontier on each core
    call_on_all_cores([g]{
      //      frontier.init(g->nv / cores() * 2);
      frontier.init(g->nv / cores() * 8);
    });
    
    for (int root_idx = 0; root_idx < FLAGS_nbfs; root_idx++) {
    
      // intialize parent to -1
      forall(g, [](BFSVertex& v){ v->init(); });
      call_on_all_cores([]{ frontier.clear(); });
      
      int64_t root = choose_root(g);
      VLOG(1) << "root => " << root;
  #ifdef __GRAPPA_CLANG__
      g->vs[root]->parent = root;
  #else      
      delegate::call(g->vs+root, [=](BFSVertex& v){
        v->parent = root;
      });
  #endif
      
      frontier.push(root);
      
      t = walltime();
      
      on_all_cores([=]{
        int64_t level = 0;
        
  #ifdef __GRAPPA_CLANG__
        auto vs = as_ptr(g->vertices());
        
        int64_t next_level_total = 1;
        
        while (next_level_total > 0) {
          if (mycore() == 0) VLOG(1) << "level " << level;
          frontier.next_level();
          joiner.enroll();
          barrier();
          
          while ( !frontier.level_empty() ) {
            auto vi = frontier.pop();
            
            forall<async,&joiner>(adj(g,vs+vi), [=](VertexID j){
              if (__sync_bool_compare_and_swap(&vs[j]->parent, -1, vi)) {
                frontier.push(j);
              }
            });
          }
          
          joiner.complete();
          joiner.wait();
          
          next_level_total = allreduce<int64_t, collective_add>(frontier.next_size());
          level++;
        }
  #else
        
        auto vs = g->vertices();
        
        int64_t next_level_total;
        
        do {
          if (mycore() == 0) VLOG(1) << "level " << level;
          frontier.next_level();
          joiner.enroll();
          barrier();
          
          while ( !frontier.level_empty() ) {
            auto i = frontier.pop();
            VLOG(2) << "  " << i;
            
            forall<async,&joiner>(adj(g,vs+i), [i,vs](int64_t j, GlobalAddress<BFSVertex> vj){
              delegate::call<async,&joiner>(vj, [i,j](BFSVertex& v){
                if (v->parent == -1) {
                  v->parent = i;
                  frontier.push(j);
                }
              });
            });
          }
          
          joiner.complete();
          joiner.wait();
          
          next_level_total = allreduce<int64_t, collective_add>(frontier.next_size());
          level++;
        } while (next_level_total > 0);
  #endif

      });
      
      double this_bfs_time = walltime() - t;
      LOG(INFO) << "(root=" << root << ", time=" << this_bfs_time << ")";
      bfs_time += this_bfs_time;
      
      if (!verified) {
        t = walltime();
    #ifdef __GRAPPA_CLANG__
        bfs_nedge = verify(tg, as_addr(g), root);
    #else
        bfs_nedge = verify(tg, g, root);
    #endif
        verify_time = (walltime()-t);
        LOG(INFO) << verify_time;
        verified = true;
      }
      
      bfs_mteps += bfs_nedge / this_bfs_time / 1.0e6;
    }
    
    LOG(INFO) << "\n" << bfs_nedge << "\n" << bfs_time << "\n" << bfs_mteps;
    
    if (FLAGS_metrics) Metrics::merge_and_print();
    Metrics::merge_and_dump_to_file();
    
  });
  finalize();
}
