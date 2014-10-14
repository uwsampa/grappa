#ifndef __LUBY_HPP__
#define __LUBY_HPP__
//#include "verificator.hpp"

using namespace Grappa;

extern int64_t nedge_traversed;

// additional data to attach to each vertex in the graph
struct LubyData {
  int64_t parent;
  int64_t level;
  int64_t rank;
  bool seen;
  
  void init() {
    parent = -1;
    level = 0;
    rank = 0;
    seen = false;
  }
};

using LubyVertex = Vertex<LubyData>;

// template <typename T = LubyVertex>
// class Verificator : public VerificatorBase <T> {
// public:
//   static inline int64_t verify(TupleGraph tg, GlobalAddress<Graph<T>> g, int64_t root) {
//     VerificatorBase<T>::verify(tg,g,root);
//     return nedge_traversed;
//   }
// };

#endif
