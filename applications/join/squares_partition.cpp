#include <stdint.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <Delegate.hpp>
#include <Grappa.hpp>
#include "local_graph.hpp"
#include "relation_IO.hpp"
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

DEFINE_string( fin, "", "Tuple file input" );
DEFINE_uint64( finNumTuples, 0, "Number of tuples in file input" );

GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, edges_transfered, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, total_edges, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, participating_cores, 0);

// intermediate results counts
// only care about min, max, totals
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir1_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir2_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir3_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir4_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir5_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir6_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir7_count, 0);

GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, ir1_final_count, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, ir2_final_count, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, ir3_final_count, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, ir4_final_count, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, ir5_final_count, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, ir6_final_count, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, ir7_final_count, 0);


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

#include <sstream>
#include <string>
std::string resultStr(std::vector<int64_t> v, int64_t size=-1) {
  std::stringstream ss;
  ss << "{ ";
  for (auto i : v) {
    ss << i << " ";
  }
  ss << "}";
  if (size!=-1) ss << " size:" << size;
  return ss.str();
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

//int64_t random_assignment(int64_t x) {
//  std::random_device rd;
//
//  // Choose a random mean between 1 and 6
//  std::default_random_engine e1(rd());
//  const int64_t root1 = fourth_root(x);
//  std::normal_distribution<> normal_dist1(root1, root1/1.25);
//
//  const int64_t share1 = std::round(max(1, normal_dist1(e1)));
//  const int64_t left1 = x/share1;
//  const int64_t root2 = std::round(cbrt(left));
//  std::normal_distribution<> normal_dist2(root2, root2/1.25);
//  const int64_t share2 = std::round(max(1, normal_dist2(e1)));
//  const int64_t left2 =  left1/share2;
//
  
  

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

      total_edges += 4; // count 4 for 4 relations
      
      const int64_t src = i;
      const int64_t dst = dest;

      VLOG(5) << "replicating edge " << src << "," << dst;

      // a->b
      auto locs_ab = h.slice( {hf(src), hf(dst), HypercubeSlice::ALL, HypercubeSlice::ALL} );
      for (auto l : locs_ab) {
        Edge e(src, dst);
        delegate::call_async( l, [e] { 
          localAssignedEdges_R1.push_back(e); 
          VLOG(5) << "received " << e << " as a->b";
        });
        edgesSent++;
      }

      // b->c
      auto locs_bc = h.slice( {HypercubeSlice::ALL, hf(src), hf(dst), HypercubeSlice::ALL} );
      for (auto l : locs_bc) {
        Edge e(src, dst);
        delegate::call_async( l, [e] { 
          localAssignedEdges_R2.push_back(e); 
          VLOG(5) << "received " << e << " as b->c";
        });
        edgesSent++;
      }

      // c->d
      auto locs_cd = h.slice( {HypercubeSlice::ALL, HypercubeSlice::ALL, hf(src), hf(dst)} );
      for (auto l : locs_cd) {
        Edge e(src, dst);
        delegate::call_async( l, [e] { 
          localAssignedEdges_R3.push_back(e); 
          VLOG(5) << "received " << e << " as c->d";
        });
        edgesSent++;
      }

      // d->a
      auto locs_da = h.slice( {hf(dst), HypercubeSlice::ALL, HypercubeSlice::ALL, hf(src)} );
      for (auto l : locs_da) {
        Edge e(src, dst);
        delegate::call_async( l, [e] { 
          localAssignedEdges_R4.push_back(e); 
          VLOG(5) << "received " << e << " as d->a";
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
    for (auto e : localAssignedEdges_R4) { R4_dedup.insert( e ); }
    
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
      int64_t x = xy.first;
      auto& xadj = xy.second;
      for (auto y : xadj) {
        ir1_count++; // count(R1)
        auto& yadj = R2.neighbors(y);
        VLOG(5) << "ir1: " << resultStr({x, y}, yadj.size());
        ir2_count+=yadj.size(); // count(R1xR2)
//        if (xadj.size() < yadj.size()) {            
//          ir3_count+=yadj.size(); // count(sel(R1xR2))
          for (auto z : yadj) {
            auto& zadj = R3.neighbors(z);
            VLOG(5) << "ir3: " << resultStr({x, y, z}, zadj.size());
            ir4_count+=zadj.size(); // count(sel(R1xR2)xR3)
//            if (yadj.size() < zadj.size()) {
//              ir5_count+=zadj.size(); // count(sel(sel(R1xR2)xR3))
              for (auto t : zadj) {
                auto tadjsize = R4.nadj(t);
                VLOG(5) << "ir5: " << resultStr({x, y, z, t}, tadjsize);
                ir6_count+=tadjsize; // count(sel(sel(R1xR2)xR3)xR4)
//                if (zadj.size() < tadjsize) {
//                  ir7_count+=tadjsize; // count(sel(sel(sel(R1xR2)xR3)xR4))
                  VLOG(5) << "ir7: " << resultStr({x, y, z, t});
                  if (R4.inNeighborhood(t, x)) {
                    emit( x,y,z,t );
                    VLOG(5) << "result: " << resultStr({x, y, z, t});
                    results_count++;  // count(sel(sel(sel(R1dxR2)xR3)xR4)xR1s)
                  } // end select t.dst=x
//                } // end select z < t
              } // end over t
//            } // end select y < z
          } // end over z
//        } // end select x < y
      } // end over y
    } // end over x

    LOG(INFO) << "counted " << count << " squares";


    // used to calculate max total over all cores
    ir1_final_count=ir1_count;
    ir2_final_count=ir2_count;
    ir3_final_count=ir3_count;
    ir4_final_count=ir4_count;
    ir5_final_count=ir5_count;
    ir6_final_count=ir6_count;
    ir7_final_count=ir7_count;

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

  tuple_graph tg;

  if (FLAGS_fin.compare("") != 0) {
    tg = readTuples( FLAGS_fin, FLAGS_finNumTuples );
  } else {
    // make raw graph edge tuples
    tg.edges = global_alloc<packed_edge>(desired_nedge);

    LOG(INFO) << "graph generation...";
    t = walltime();
    make_graph( FLAGS_scale, desired_nedge, userseed, userseed, &tg.nedge, &tg.edges );
    generation_time = walltime() - t;
    LOG(INFO) << "graph_generation: " << generation_time;
  }

  LOG(INFO) << "graph construction...";
  t = walltime();
  auto g = Graph<Vertex>::create(tg, /*directed=*/true); 
  construction_time = walltime() - t;
  LOG(INFO) << "construction_time: " << construction_time;

  LOG(INFO) << "num edges: " << g->nadj * 4; /* 4 b/c three copies*/

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
