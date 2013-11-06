#include <stdint.h>
#include <iostream>
#include <cmath>
#include <graph.hpp>
#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <Delegate.hpp>
#include <Grappa.hpp>
#include "local_graph.hpp"

extern "C" {
  #include <ProgressOutput.h>
}

// graph includes
#include "../graph500/generator/make_graph.h"
#include "../graph500/generator/utils.h"
#include "../graph500/prng.h"

#include "../graph500/grappa/graph.hpp"

using namespace Grappa;

#define DIFFERENT_RELATIONS 0
#define DEDUP_EDGES 1

// Currently assumes an undirected graph implemented as a large directed graph!!

DEFINE_uint64( scale, 7, "Graph will have ~ 2^scale vertices" );
DEFINE_uint64( edgefactor, 16, "Median degree; graph will have ~ 2*edgefactor*2^scale edges" );
DEFINE_uint64( progressInterval, 5, "interval between progress updates" );

GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, edges_transfered, 0);

//outputs
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, triangle_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, triangles_runtime, 0);

double generation_time;
double construction_time;

uint64_t count = 0;
uint64_t edgesSent = 0;

void emit(int64_t x, int64_t y, int64_t z) {
//  std::cout << x << " " << y << " " << z << std::endl;
  count++;
}

class LocD {
  private:
    std::vector<int64_t> shares;

  class Iter {
    private:
      std::vector<int64> current_positions;
      std::vector<int64_t> wildcards;
    
//TODO good target for memoizing
      /* Turn the current n-dimensional vector into a sequential id */
      static int64_t serialId (std::vector<int64_t> pos, std::vector<int64_t> sizes) {
        int64_t sum = pos[0];
        for (int i=1; i<pos.size(); i++) {
          sum += pos[i] * sizes[i-1];
        }
        return sum;
      }

    public:
    Iter(std::vector<int64_t> iteration_box)    // TODO: would be elegant in Chapel distributions
      : current_positions(iteration_box.size()) 
      , wildcards() {

      for (int i = 0; i<iteration_box.size(); i++) {
        int64_t p = iteration_box[i];
        if (p==ALL) {
          // if whole dimension then start at 0 to prepare for iteration
          current_positions[i] = 0;
          wildcards.push_back(p);
        } else {
          // fixed dimension
          current_positions[i] = p;
        }
      }
    }
    
    bool operator!= (const Iter& other) const {
      return done!=other.done
        || other stuff
    }

    const Iter& operator++() {
      int i = 0;
      while (i < wildcards.size()) {
        int64_t dim = wildcards[i];
        if (current_positions[dim]+1 == shares[dim]) {
          current_positions[dim] = 0;
          i++;
        } else {
          current_positions[dim]++;
          break;
        }
      }

      // if ran out of dimensions then at end
      if (i == wildcards.size()) {
        done = true;
      }
      
      return this;
    }

    int64_t operator*() const {
      return serialId(current_position, shares);
    }
  };

  public:
  static const int64_t ALL = -1;

  //TODO replace with product find
  static int64_t int_cbrt(int64_t x) {
    int64_t root = round(floor(cbrt(x)));
    int64_t rootCube = root*root*root;
    if (x != (rootCube) ) { LOG(WARNING) << x << " not a perfect cube; processors 0-"<<(rootCube-1)<<") will count"; }
    return root;
  }

  LocD(std::vector<int64_t> shares) {
    : shares(shares) // copy 
    }

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

//TODO
std::function<int64_t (int64_t)> makeHash( int64_t dim ) {
  // identity
  return [dim](int64_t x) { return x % dim; };
}

void triangles(GlobalAddress<Graph<Vertex>> g) {
  
  on_all_cores( [] { Grappa::Statistics::reset(); } );


  // need to arrange the processors in 3d cube
  auto sidelength = Loc3d::int_cbrt( cores() );

  double start, end;
  start = Grappa_walltime(); 
  
  // 1. Send edges to the partitions
  //
  // really just care about local edges; we get to them
  // indirectly through local vertices at the moment.
  // This is sequential access since edgeslists and vertices are sorted the same
  forall_localized( g->vs, g->nv, [sidelength](int64_t i, Vertex& v) {
    // hash function
    auto hf = makeHash( sidelength );

    for (auto& dst : v.adj_iter()) {
      
      const int64_t from = i;
      const int64_t to = dst;

      // x-y
      auto locs_xy = Loc3d(sidelength, hf(from), hf(to), Loc3d::ALL);    
      for (auto l : locs_xy) {
        Edge e(from, to);
        delegate::call_async( *shared_pool, l, [e] { 
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
        delegate::call_async( *shared_pool, l, [e] { 
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
        delegate::call_async( *shared_pool, l, [e] { 
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
    ProgressOutput countp;
    ProgressOutput_init( FLAGS_progressInterval, 1, PO_ARG(countp));

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
                i++;
                if (i % (1<<20)) {
                  ProgressOutput_updateShared( &countp, count );
                }
              }
            }
          }
        }
      }
    }

    LOG(INFO) << "counted " << count << " triangles; R1adjs="<<R1adjs;
  });
  end = Grappa_walltime();
  triangles_runtime = end - start;
  
  
  Grappa::Statistics::merge_and_print();
 
}


void user_main( int * ignore ) {
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
}


int main(int argc, char** argv) {
  Grappa_init(&argc, &argv);
  Grappa_activate();

  Grappa_run_user_main(&user_main, (int*)NULL);

  Grappa_finish(0);
  return 0;
}
