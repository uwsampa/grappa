#ifndef __VERIFICATOR_HPP__
#define __VERIFICATOR_HPP__

#include <Grappa.hpp>
#include <graph/Graph.hpp>

using namespace Grappa;

extern int64_t nedge_traversed;

template <typename G>
class VerificatorBase {
  using Vertex = typename G::Vertex;
public:

  static int64_t get_level(GlobalAddress<G> g, int64_t j) {
    return delegate::call(g->vs+j, [](Vertex& v){ return v->level; });
  }
  static int64_t get_parent(GlobalAddress<G> g, int64_t j) {
    return delegate::call(g->vs+j, [](Vertex& v){ return v->parent; });
  }

  static int compute_levels(GlobalAddress<G> g, int64_t root) {
    // compute levels
    delegate::call(g->vs+root, [](Vertex& v){ v->level = 0; });
  
    forall(g, [=](VertexID i, Vertex& v){
      if (v->level >= 0) return;
      
      if (v->parent >= 0 && i != root) {
        int64_t parent = i;
        int64_t nhop = 0;
        int64_t next_parent;
        
        while (parent >= 0 && get_level(g, parent) < 0 && nhop < g->nv) {
          next_parent = get_parent(g, parent);
          CHECK_NE(parent, next_parent);
          parent = next_parent;
          ++nhop;
        }
        
        CHECK_LT(nhop, g->nv);
        CHECK_GE(parent, 0);
        
        // now assign levels until we meet an already-leveled vertex
        nhop += get_level(g, parent);
        parent = i;
        while (get_level(g, parent) < 0) {
          CHECK_GT(nhop, 0);
          parent = delegate::call(g->vs+parent, [=](Vertex& v){
            v->level = nhop;
            return v->parent;
          });
          nhop--;
        }
        CHECK_EQ(nhop, get_level(g, parent));
      }
    });
  }

  static inline int64_t verify(TupleGraph tg, GlobalAddress<G> g, int64_t root) {

    // check root
    delegate::call(g->vs+root, [=](Vertex& v){
        CHECK_EQ(v->parent, root);
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
      auto ti = get_parent(g,i), tj = get_parent(g,j);
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
        delegate::call(g->vs+i, [](Vertex& v){ v->seen = true; });
      };

      // Mark seen tree edges.
      if (i != j) {
        if (ti == j)
          mark_seen(i);
        if (tj == i)
          mark_seen(j);
      }
      lvldiff = get_level(g,i) - get_level(g,j);
      /* Check that the levels differ by no more than one. */
      CHECK(!(lvldiff > 1 || lvldiff < -1)) << "Error, levels differ by more than one! " << "(k = " << ", lvl[" << i << "]=" 
        << get_level(g,i) << ", lvl[" << j << "]=" << get_level(g,j) << ")";

    });

    nedge_traversed = Grappa::reduce<int64_t,collective_add>(&nedge_traversed);

    // check that every BFS edge was seen & that there's only one root
    forall(g, [=](VertexID i, Vertex& v){
      if (i != root) {
        CHECK(!(v->parent >= 0 && !v->seen)) << "Error!" << "VertexID: " << i <<" v->parent =" << v->parent << " v->seen = " << v->seen;
        CHECK_NE(v->parent, i);
      }
    });

    // everything checked out!
    VLOG(1) << "verified!\n";

    return nedge_traversed;
  }
};

#endif
