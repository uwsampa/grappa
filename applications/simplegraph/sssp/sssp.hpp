#pragma once
#include "../verifier.hpp"

extern int64_t nedge_traversed;

/* Vertex specific data */
struct SSSPData {
  double dist;
  double *weights;
  int64_t parent;
  int64_t level;
  bool seen;

  void init(int64_t nadj) {
    dist = std::numeric_limits<double>::max();
    weights = new double [nadj];
    for (int i=0; i<nadj; i++)
      weights[i] = drand48();

    parent = -1;
    level = 0;
    seen = false;
  }
};

using SSSPVertex = Vertex<SSSPData>;

template <typename T = SSSPVertex>
class Verificator : public VerificatorBase <T> {
public:

  static double get_dist(GlobalAddress<Graph<T>> g, int64_t j) {
    return delegate::call(g->vs+j, [](T& v){ return v->dist; });
  }

  static double get_edge_weight(GlobalAddress<Graph<T>> g, int64_t i, int64_t j) {
    return delegate::call(g->vs+i, [=](T& v){ 
        for (int k = 0; v.nadj; k++)
          if (v.local_adj[k] == j)
            return v->weights[k];
        // we can not reach this, possibly better to throw exception
        return 0.0;
    });
  }

  static inline int64_t verify(TupleGraph tg, GlobalAddress<Graph<T>> g, int64_t root) {
    VerificatorBase<T>::verify(tg,g,root);

    // SSSP distances verification
    forall(tg.edges, tg.nedge, [=](TupleGraph::Edge& e){
      auto i = e.v0, j = e.v1;

      /* Eliminate self loops from verification */
      if ( i == j)
        return;

      /* SSSP specific checks */
      auto ti = VerificatorBase<T>::get_parent(g,i), tj = VerificatorBase<T>::get_parent(g,j);
      auto di = get_dist(g,i), dj = get_dist(g,j);
      auto wij = get_edge_weight(g,i,j), wji = get_edge_weight(g,j,i);
      CHECK(!((di < dj) && ((di + wij) < dj))) << "Error, distance of the nearest neighbor is too great :" 
        << "(" << i << "," << di << ")" << "--" << wij << "-->" <<  "(" << j << "," << dj << ")" ;
      CHECK(!((dj < di) && ((dj + wji) < di))) << "Error, distance of the nearest neighbor is too great : "
        << "(" << j << "," << dj << ")" << "--" << wji << "-->" <<  "(" << i << "," << di << ")" ;
      CHECK(!((i == tj) && ((di + wij) != dj))) << "Error, distance of the child vertex is not equil to "
        << "sum of its parent distance and edge weight :" 
        << "(" << i << "," << di << ")" << "--" << wij << "-->" <<  "(" << j << "," << dj << ")" ;
      CHECK(!((j == ti) && ((dj + wji) != di))) << "Error, distance of the child vertex is not equil to "
        << "sum of its parent distance and edge weight :"
        << "(" << j << "," << dj << ")" << "--" << wji << "-->" <<  "(" << i << "," << di << ")" ;
    });

    // everything checked out!
    VLOG(1) << "SSSP verified!\n";

    return nedge_traversed;
  }
};

void dump_sssp_graph(GlobalAddress<Graph<SSSPVertex>> &g) {
  for(int i=0; i < g->nv; i++) {
    delegate::call(g->vs+i,[i](SSSPVertex &v){
      VLOG(1) << "Vertex[" << i << "] -> " << v->dist;
    });
  }
}
