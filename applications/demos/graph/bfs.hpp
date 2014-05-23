#ifndef __BFS_HPP__
#define __BFS_HPP__
#include "verificator.hpp"

extern int64_t nedge_traversed;

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

using BFSVertex = Vertex<BFSData>;

template <typename T = BFSVertex>
class Verificator : public VerificatorBase <T> {
public:
  static inline int64_t verify(TupleGraph tg, GlobalAddress<Graph<T>> g, int64_t root) {
    VerificatorBase<T>::verify(tg,g,root);
    return nedge_traversed;
  }
};

inline int64_t verify(TupleGraph tg, GlobalAddress<Graph<T>> g, int64_t root) {
  return Verificator<BFSVertex>::verify(tg, g, root);
}
  

template< typename T >
inline int64_t choose_root(GlobalAddress<Graph<T>> g) {
  int64_t root;
  do {
    root = random() % g->nv;
  } while (delegate::call(g->vs+root,[](BFSVertex& v){ return v.nadj; }) == 0);
  return root;
}


#endif
