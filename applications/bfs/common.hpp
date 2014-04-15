#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <graph/Graph.hpp>

using namespace Grappa;

// additional data to attach to each vertex in the graph
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

using G = Graph<BFSData,Empty>;

extern int64_t nedge_traversed;

void bfs(GlobalAddress<G> g, int nbfs, TupleGraph tg);

inline int64_t verify(TupleGraph tg, GlobalAddress<G> g, int64_t root) {
  
  auto get_level = [g](int64_t j){
    return delegate::call(g->vs+j, [](G::Vertex& v){ return v->level; });
  };
  auto get_parent = [g](int64_t j){
    return delegate::call(g->vs+j, [](G::Vertex& v){ return v->parent; });
  };
  
  // check root
  delegate::call(g->vs+root, [=](G::Vertex& v){
    CHECK_EQ(v->parent, root);
  });
  
  // compute levels
  delegate::call(g->vs+root, [](G::Vertex& v){ v->level = 0; });
  
  forall(g->vs, g->nv, [=](int64_t i, G::Vertex& v){
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
        parent = delegate::call(g->vs+parent, [=](G::Vertex& v){
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
      delegate::call(g->vs+i, [](G::Vertex& v){ v->seen = true; });
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
  forall(g->vs, g->nv, [=](int64_t i, G::Vertex& v){
    if (i != root) {
      CHECK(!(v->parent >= 0 && !v->seen)) << "Error!";
      CHECK_NE(v->parent, i);
    }
  });
  
  // everything checked out!
  VLOG(1) << "verified!\n";
  return nedge_traversed;
}


template< typename V, typename E >
inline int64_t choose_root(GlobalAddress<Graph<V,E>> g) {
  int64_t root;
  do {
    root = random() % g->nv;
  } while (delegate::call(g->vs+root,[](typename G::Vertex& v){ return v.nadj; }) == 0);
  return root;
}

