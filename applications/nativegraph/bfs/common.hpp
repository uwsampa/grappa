#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <graph/Graph.hpp>
#include "../verifier.hpp"

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

template< typename V, typename E >
inline int64_t choose_root(GlobalAddress<Graph<V,E>> g) {
  int64_t root;
  do {
    root = random() % g->nv;
  } while (delegate::call(g->vs+root,[](typename G::Vertex& v){ return v.nadj; }) == 0);
  return root;
}

inline int64_t verify(TupleGraph tg, GlobalAddress<G> g, int64_t root) {
  return VerificatorBase<G>::verify(tg, g, root);
}

