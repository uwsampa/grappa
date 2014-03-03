#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <graph/Graph.hpp>

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

GRAPPA_DEFINE_METRIC(SimpleMetric<double>, bfs_mteps, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, bfs_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, bfs_nedge, 0);

int64_t *frontier_base, *f_head, *f_tail, *f_level;

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

template< typename T >
int64_t choose_root(SymmetricAddress<Graph<T>> g) {
  int64_t root;
  do {
    root = random() % g->nv;
  } while (delegate::call(g->vs+root,[](BFSVertex& v){ return v.nadj; }) == 0);
  return root;
}

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    int64_t NE = (1L << FLAGS_scale) * FLAGS_edgefactor;
    
    auto tg = TupleGraph::Kronecker(FLAGS_scale, NE, 111, 222);
    auto g = Graph<BFSVertex>::create( tg );
    
    // initialize frontier on each core
    call_on_all_cores([g]{
      frontier_base = f_head = f_tail = f_level = locale_alloc<int64_t>(g->nv / cores() * 2);
    });
    
    // intialize parent to -1
    forall(g, [](BFSVertex& v){ v->init(); });
    
    int64_t root = choose_root(g);
    VLOG(1) << "root => " << root;
    delegate::call(g->vs+root, [=](BFSVertex& v){
      v->parent = root;
      *f_tail++ = root;
    });
    
    double t = walltime();
    
    on_all_cores([=]{
      int64_t level = 0;
      
      auto vs = g->vertices();
      
      int64_t next_level_total;
      
      do {
        if (mycore() == 0) VLOG(1) << "level " << level;
        f_level = f_tail;
        joiner.enroll();
        barrier();
        
        while (f_head < f_level) {
          auto i = *f_head++;  // pop off frontier
          VLOG(2) << "  " << i;
          auto vi = vs+i;
          CHECK_EQ(vi.core(), mycore());
          auto& v = *vi.pointer();
          forall<async,&joiner>(adj(g,v), [i,vs](int64_t j, GlobalAddress<BFSVertex> vj){
            delegate::call<async,&joiner>(vj, [i,j](BFSVertex& v){
              if (v->parent == -1) {
                v->parent = i;
                *f_tail++ = j; // push 'j' onto frontier
              }
            });
          });
        }
        
        joiner.complete();
        joiner.wait();
        
        VLOG(1) << "(" << mycore() << ":" << f_tail-f_level << ")";
        next_level_total = allreduce<int64_t, collective_add>(f_tail - f_level);
        level++;
      } while (next_level_total > 0);
      
    });
    
    bfs_time = walltime() - t;
    
    bfs_nedge = verify(tg, g, root);
    
    bfs_mteps = bfs_nedge / bfs_time / 1.0e6;
    
    LOG(INFO) << "\n" << bfs_nedge << "\n" << bfs_time << "\n" << bfs_mteps;
    
    if (FLAGS_metrics) Metrics::merge_and_print();    
    Metrics::merge_and_dump_to_file();
    
  });
  finalize();
}
