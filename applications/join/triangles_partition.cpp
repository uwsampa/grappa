#include <stdint.h>
#include <iostream>
#include <cmath>
#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <Delegate.hpp>
#include <Grappa.hpp>
#include "local_graph.hpp"
#include "utility.hpp"

#ifdef PROGRESS
extern "C" {
  #include <ProgressOutput.h>
}
#endif

// graph includes
#include "generator/make_graph.h"
#include "generator/utils.h"
#include "prng.h"

#include "../graph500/grappa/graph.hpp"

using namespace Grappa;

#define DIFFERENT_RELATIONS 0
#define DEDUP_EDGES 1

// Currently assumes an undirected graph implemented as a large directed graph!!

DEFINE_uint64( scale, 7, "Graph will have ~ 2^scale vertices" );
DEFINE_uint64( edgefactor, 16, "Median degree; graph will have ~ 2*edgefactor*2^scale edges" );
DEFINE_uint64( progressInterval, 5, "interval between progress updates" );

GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, edges_transfered, 0);

//outputs
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, triangle_count, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, triangles_runtime, 0);

double generation_time;
double construction_time;

uint64_t count = 0;
uint64_t edgesSent = 0;

void emit(int64_t x, int64_t y, int64_t z) {
//  std::cout << x << " " << y << " " << z << std::endl;
  count++;
}

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

  static int64_t int_cbrt(int64_t x) {
    int64_t root = round(floor(cbrt(x)));
    int64_t rootCube = root*root*root;
    if (x != (rootCube) ) { LOG(WARNING) << x << " not a perfect cube; processors 0-"<<(rootCube-1)<<") will count"; }
    return root;
  }

  Loc3d(int64_t dim, int64_t x, int64_t y, int64_t z)
    : dim(dim), x(x), y(y), z(z) {}

  Iter begin() {
    return Iter(x,y,z,dim,false);
  }
  
  Iter end() {
    return Iter(x,y,z,dim,true);
  }
};

std::vector<Edge> localAssignedEdges_R1;
std::vector<Edge> localAssignedEdges_R2;
std::vector<Edge> localAssignedEdges_R3;


void triangles(GlobalAddress<Graph<Vertex>> g) {
  
  on_all_cores( [] { Grappa::Metrics::reset(); } );


  // need to arrange the processors in 3d cube
  auto sidelength = Loc3d::int_cbrt( cores() );

  double start, end;
  start = Grappa::walltime(); 
  
  // 1. Send edges to the partitions
  //
  // really just care about local edges; we get to them
  // indirectly through local vertices at the moment.
  // This is sequential access since edgeslists and vertices are sorted the same
  forall( g->vs, g->nv, [sidelength](int64_t i, Vertex& v) {
    // hash function
    auto hf = makeHash( sidelength );

    for (auto& dst : v.adj_iter()) {
      
      const int64_t from = i;
      const int64_t to = dst;

      // x-y
      auto locs_xy = Loc3d(sidelength, hf(from), hf(to), Loc3d::ALL);    
      for (auto l : locs_xy) {
        Edge e(from, to);
        delegate::call<async>( l, [e] { 
#if DIFFERENT_RELATIONS
          localAssignedEdges_R1.push_back(e); 
#else
          localAssignedEdges_R1.push_back(e);
#endif
        });
        edgesSent++;
      }

      // y-z
      auto locs_yz = Loc3d(sidelength, Loc3d::ALL, hf(from), hf(to));
      for (auto l : locs_yz) {
        Edge e(from, to);
        delegate::call<async>( l, [e] { 
#if DIFFERENT_RELATIONS
          localAssignedEdges_R2.push_back(e); 
#else
          localAssignedEdges_R1.push_back(e);
#endif
        });
        edgesSent++;
      }

      // z-x
      auto locs_zx = Loc3d(sidelength, hf(to), Loc3d::ALL, hf(from));
      for (auto l : locs_zx) {
        Edge e(from, to);
        delegate::call<async>( l, [e] { 
#if DIFFERENT_RELATIONS
          localAssignedEdges_R3.push_back(e); 
#else
          localAssignedEdges_R1.push_back(e);
#endif
        });
        edgesSent++;
      }
    }
  });
  on_all_cores([] { 
      LOG(INFO) << "edges sent: " << edgesSent; 
      edges_transfered += edgesSent;
  });
  

  // 2. compute triangles locally
  //
  on_all_cores([] {
#ifdef PROGRESS
    ProgressOutput countp;
    ProgressOutput_init( FLAGS_progressInterval, 1, PO_ARG(countp));
#endif

    LOG(INFO) << "received (" << localAssignedEdges_R1.size() << ", " << localAssignedEdges_R2.size() << ", " << localAssignedEdges_R3.size() << ") edges";

#ifdef DEDUP_EDGES
    // construct local graphs
    LOG(INFO) << "dedup...";
    std::unordered_set<Edge, Edge_hasher> R1_dedup;
    std::unordered_set<Edge, Edge_hasher> R2_dedup;
    std::unordered_set<Edge, Edge_hasher> R3_dedup;
    for (auto e : localAssignedEdges_R1) { R1_dedup.insert( e ); }
    for (auto e : localAssignedEdges_R2) { R2_dedup.insert( e ); }
    for (auto e : localAssignedEdges_R3) { R3_dedup.insert( e ); }
    
    LOG(INFO) << "after dedup (" << R1_dedup.size() << ", " << R2_dedup.size() << ", " << R3_dedup.size() << ") edges";

    localAssignedEdges_R1.resize(1);
    localAssignedEdges_R2.resize(1);
    localAssignedEdges_R3.resize(1);


    LOG(INFO) << "local graph construct...";
  #if DIFFERENT_RELATIONS
    auto& R1 = *new LocalAdjListGraph(R1_dedup);
    auto& R2 = *new LocalAdjListGraph(R2_dedup);
    auto& R3 = *new LocalMapGraph(R3_dedup);
  #else
    auto& R1 = *new LocalAdjListGraph(R1_dedup);
    auto R2 = R1;
    auto& R3 = *new LocalMapGraph(R1_dedup);
  #endif
#else
    LOG(INFO) << "local graph construct...";
    auto& R1 = *new LocalAdjListGraph(localAssignedEdges_R1);
    auto& R2 = *new LocalAdjListGraph(localAssignedEdges_R2);
    auto& R3 = *new LocalMapGraph(localAssignedEdges_R3);
#endif

    // use number of adjacencies (degree) as the ordering
               // EVAR: implementation of triangle count
    int64_t i=0;
    int64_t R1adjs = 0;
    for (auto xy : R1.vertices()) {
      int64_t x = xy.first;
      auto& xadj = xy.second;
      R1adjs += xadj.size();
      for (auto y : xadj) {
        auto& yadj = R2.neighbors(y);
        if (xadj.size() < yadj.size()) {            
          for (auto z : yadj) {
            if (yadj.size() < R3.nadj(z)) {
              if (R3.inNeighborhood(z, x)) {
                emit( x, y, z );
                triangle_count++;
#ifdef PROGRESS
                i++;
                if (i % (1<<20)) {
                  ProgressOutput_updateShared( &countp, count );
                }
#endif
              }
            }
          }
        }
      }
    }

    LOG(INFO) << "counted " << count << " triangles; R1adjs="<<R1adjs;
  });
  end = Grappa::walltime();
  triangles_runtime = end - start;
  
  
  Grappa::Metrics::merge_and_print();
 
}


int main(int argc, char** argv) {
  Grappa::init(&argc, &argv);
  Grappa::run([]{
    double t, start_time;
    start_time = walltime();
  
  	int64_t nvtx_scale = ((int64_t)1)<<FLAGS_scale;
  	int64_t desired_nedge = nvtx_scale * FLAGS_edgefactor;
  
    LOG(INFO) << "scale = " << FLAGS_scale << ", NV = " << nvtx_scale << ", NE = " << desired_nedge;
  
    // make raw graph edge tuples
    tuple_graph tg;
    tg.edges = global_alloc<packed_edge>(desired_nedge);
  
    LOG(INFO) << "graph generation...";
    t = walltime();
    make_graph( FLAGS_scale, desired_nedge, userseed, userseed, &tg.nedge, &tg.edges );
    generation_time = walltime() - t;
    LOG(INFO) << "graph_generation: " << generation_time;
  
    LOG(INFO) << "graph construction...";
    t = walltime();
    auto g = Graph<Vertex>::create(tg);
    construction_time = walltime() - t;
    LOG(INFO) << "construction_time: " << construction_time;

    LOG(INFO) << "num edges: " << g->nadj * 3; /* 3 b/c three copies*/

    LOG(INFO) << "query start...";
    triangles(g);
  });
  Grappa::finalize();
}
