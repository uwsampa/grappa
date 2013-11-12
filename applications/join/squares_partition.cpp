#include <stdint.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <Delegate.hpp>
#include <Grappa.hpp>
#include "local_graph.hpp"
#include "Hypercube.hpp"


// graph includes
#include "../graph500/grappa/graph.hpp"
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
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, total_edges, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, participating_cores, 0);

// intermediate results counts
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir1_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir2_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir3_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir4_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir5_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir6_count, 0);


//outputs
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, results_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, query_runtime, 0);

double generation_time;
double construction_time;

uint64_t count = 0;
uint64_t edgesSent = 0;

void emit(int64_t x, int64_t y, int64_t z, int64_t t) {
//  std::cout << x << " " << y << " " << z << std::endl;
  count++;
}


std::vector<Edge> localAssignedEdges_R1;
std::vector<Edge> localAssignedEdges_R2;
std::vector<Edge> localAssignedEdges_R3;
std::vector<Edge> localAssignedEdges_R4;

//TODO
std::function<int64_t (int64_t)> makeHash( int64_t dim ) {
  // identity
  return [dim](int64_t x) { return x % dim; };
}

int64_t fourth_root(int64_t x) {
  // index pow 4
  std::vector<int64_t> powers = {0, 1, 16, 81, 256, 625, 1296, 2401};
  int64_t ind = powers.size() / 2;
  int64_t hi = powers.size()-1;
  int64_t lo = 0;
  while(true) {
    if (x == powers[ind]) {
      return ind;
    } else if (x > powers[ind]) {
      int64_t next = (ind+hi)/2;
      if (next - ind == 0) {
        return ind;
      }
      lo = ind;
      ind = next;
    } else {
      int64_t next = (ind+lo)/2;
      hi = ind;
      ind = next;
    }
  }
}
  

void squares(GlobalAddress<Graph<Vertex>> g) {
  
  on_all_cores( [] { Grappa::Statistics::reset(); } );

  // arrange the processors in 4d
  auto sidelength = fourth_root(cores());
  LOG(INFO) << "using " << sidelength*sidelength*sidelength*sidelength << " of " << cores() << " cores";
  participating_cores = sidelength*sidelength*sidelength*sidelength;
  

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
    Hypercube h( { sidelength, sidelength, sidelength, sidelength } );

    for (auto& dest : v.adj_iter()) {

      total_edges += 1;
      
      const int64_t src = i;
      const int64_t dst = dest;

      // a->b
      auto locs_ab = h.slice( {hf(src), hf(dst), HypercubeSlice::ALL, HypercubeSlice::ALL} );
      for (auto l : locs_ab) {
        Edge e(src, dst);
        delegate::call_async( l, [e] { 
          localAssignedEdges_R1.push_back(e); 
        });
        edgesSent++;
      }

      // b->c
      auto locs_bc = h.slice( {HypercubeSlice::ALL, hf(src), hf(dst), HypercubeSlice::ALL} );
      for (auto l : locs_bc) {
        Edge e(src, dst);
        delegate::call_async( l, [e] { 
          localAssignedEdges_R2.push_back(e); 
        });
        edgesSent++;
      }

      // c->d
      auto locs_cd = h.slice( {HypercubeSlice::ALL, HypercubeSlice::ALL, hf(src), hf(dst)} );
      for (auto l : locs_cd) {
        Edge e(src, dst);
        delegate::call_async( l, [e] { 
          localAssignedEdges_R3.push_back(e); 
        });
        edgesSent++;
      }

      // d->a
      auto locs_da = h.slice( {hf(dst), HypercubeSlice::ALL, HypercubeSlice::ALL, hf(src)} );
      for (auto l : locs_da) {
        Edge e(src, dst);
        delegate::call_async( l, [e] { 
          localAssignedEdges_R4.push_back(e); 
        });
        edgesSent++;
      }
    }
  });
  on_all_cores([] { 
      LOG(INFO) << "edges sent: " << edgesSent;
      edges_transfered += edgesSent;
  });
  

  // 2. compute squares locally
  //
  on_all_cores([] {

    LOG(INFO) << "received (" << localAssignedEdges_R1.size() << ", " << localAssignedEdges_R2.size() << ", " << localAssignedEdges_R3.size() << ", " <<  localAssignedEdges_R4.size() << ") edges";

#ifdef DEDUP_EDGES
    // construct local graphs
    LOG(INFO) << "dedup...";
    std::unordered_set<Edge, Edge_hasher> R1_dedup;
    std::unordered_set<Edge, Edge_hasher> R2_dedup;
    std::unordered_set<Edge, Edge_hasher> R3_dedup;
    std::unordered_set<Edge, Edge_hasher> R4_dedup;
    for (auto e : localAssignedEdges_R1) { R1_dedup.insert( e ); }
    for (auto e : localAssignedEdges_R2) { R2_dedup.insert( e ); }
    for (auto e : localAssignedEdges_R3) { R3_dedup.insert( e ); }
    for (auto e : localAssignedEdges_R3) { R4_dedup.insert( e ); }
    
    LOG(INFO) << "after dedup (" << R1_dedup.size() << ", " << R2_dedup.size() << ", " << R3_dedup.size() << ", " << R4_dedup.size() << ") edges";

    localAssignedEdges_R1.resize(1);
    localAssignedEdges_R2.resize(1);
    localAssignedEdges_R3.resize(1);
    localAssignedEdges_R4.resize(1);


    LOG(INFO) << "local graph construct...";
    auto& R1 = *new LocalAdjListGraph(R1_dedup);
    auto& R2 = *new LocalAdjListGraph(R2_dedup);
    auto& R3 = *new LocalAdjListGraph(R3_dedup);
    auto& R4 = *new LocalMapGraph(R4_dedup);
#else
    LOG(INFO) << "local graph construct...";
    auto& R1 = *new LocalAdjListGraph(localAssignedEdges_R1);
    auto& R2 = *new LocalAdjListGraph(localAssignedEdges_R2);
    auto& R3 = *new LocalAdjListGraph(localAssignedEdges_R3);
    auto& R4 = *new LocalMapGraph(localAssignedEdges_R4);
#endif

    // use number of adjacencies (degree) as the ordering
               // EVAR: implementation of triangle count
    int64_t i=0;
    for (auto xy : R1.vertices()) {
      ir1_count++;
      int64_t x = xy.first;
      auto& xadj = xy.second;
      for (auto y : xadj) {
        ir2_count++;
        auto& yadj = R2.neighbors(y);
        if (xadj.size() < yadj.size()) {            
          ir3_count++;
          for (auto z : yadj) {
            ir4_count++;
            auto& zadj = R3.neighbors(y);
            if (yadj.size() < zadj.size()) {
              ir5_count++;
              for (auto t : zadj) {
                ir6_count++;
                if (R4.inNeighborhood(t, x)) {
                  emit( x,y,z,t );
                  results_count++;
                } // end select t.dst=x
              } // end over t
            } // end select y < z
          } // end over z
        } // end select x < y
      } // end over y
    } // end over x

    LOG(INFO) << "counted " << count << " squares";
  });
  end = Grappa_walltime();
  query_runtime = end - start;
  
  
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
  squares(g);
}


int main(int argc, char** argv) {
  Grappa_init(&argc, &argv);
  Grappa_activate();

  Grappa_run_user_main(&user_main, (int*)NULL);

  Grappa_finish(0);
  return 0;
}
