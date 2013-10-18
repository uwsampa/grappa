#include <stdint.h>
#include <graph.hpp>
#include <Collective.hpp>
#include "local_graph.hpp"


// Currently assumes an undirected graph implemented as a large directed graph!!



class Loc3d {
  private:
  int64_t dim;
  int64_t x;
  int64_t y;
  int64_t z;


  class Iter {
    private:
    int64_t pos;
    int64_t stride;
    int64_t fixed;
    public:
    Iter(int64_t x, int64_t y, int64_t z, int64_t dim, bool end) : pos(0) {
      if (x==ALL) {
        stride = 1;
        fixed = y*dim + z*dim*dim;
      } else if (y==ALL) {
        stride = dim;
        fixed = x + z*dim*dim;
      } else { // z==ALL
        stride = dim*dim;
        fixed = x + y*dim;
      }

      if (end) {
        pos += stride*dim;
      }
    }

    bool operator!= (const Iter& other) const {
      return pos!=other.pos
        || fixed != other.fixed
        || stride != other.stride;
    }

    const Iter& operator++() {
      pos += stride;
      return *this;
    }

    int64_t operator*() const {
      return pos + fixed;
    }
  };

  public:
  static const int64_t ALL = -1;

  Iter begin() {
    return Iter(x,y,z,dim,false);
  }
  
  Iter end() {
    return Iter(x,y,z,dim,true);
  }
};

struct Edge {
  int64_t src, dst;
  Edge(int64_t src, int64_t dst) : src(src), dst(dst) {}
};

std::vector<Edge> localAssignedEdges_R1;
std::vector<Edge> localAssignedEdges_R2;
std::vector<Edge> localAssignedEdges_R3;

void triangles() {

  //TODO construct
  Graph g;

  // 1. Send edges to the partitions
  //
  // really just care about local edges; we get to them
  // indirectly through local vertices at the moment.
  // This is sequential access since edgeslists and vertices are sorted the same
  forall_localized( g->vs, g->nv, [](int64_t i, Vertex& v) {
    for (auto& dst : v.adj_iter()) {
      
      int64_t from = getFrom(i) 

      // x-y
      auto locs_xy = Loc3d(sidelength, hf(from), hf(to), Loc3d.ALL);    
      for (auto l : locs) {
        Edge e(from, to);
        delegate::call_async( *shared_pool, l, [e] { 
          localAssignedEdges_R1.push_back(e); 
        });
      }

      // y-z
      auto locs_yz = Loc3d(sidelength, Loc3d.ALL, hf(from), hf(to));
      for (auto l : locs) {
        Edge e(from, to);
        delegate::call_async( *shared_pool, l, [e] { 
          localAssignedEdges_R2.push_back(e); 
        });
      }

      // z-x
      auto locs_zx = Loc3d(sidelength, hf(to), Loc3d.ALL, hf(from));
      for (auto l : locs) {
        Edge e(from, to);
        delegate::call_async( *shared_pool, l, [e] { 
          localAssignedEdges_R3.push_back(e); 
        });
      }
    }
  });

  // 2. compute triangles locally
  //
  on_all_cores([] {

    // construct local graphs
    auto& R1 = new LocalAdjListGraph(localAssignedEdges_R1);
    auto& R2 = new LocalAdjListGraph(localAssignedEdges_R2);
    auto& R3 = new LocalMapGraph(localAssignedEdges_R3);

    // use number of adjacencies (degree) as the ordering
               // EVAR: implementation of triangle count
    for (auto xy : R1.vertices()) {
      int64_t x = xy.first
      auto& xadj = xy.second
      for (auto y : xadj) {
        auto& yadj = R2.neighbors(y);
        if (xadj.size() < yadj.size()) {            
          for (auto z : yadj)
            if (y.najs < R3.nadj(z)) {
              if (R3.inNeighborhood(z, x)) {
                emit( x, y, z );
              }
            }
          }
        }
      }
    }
  });
 
}
