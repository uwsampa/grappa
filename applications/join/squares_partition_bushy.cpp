#include <stdint.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <Delegate.hpp>
#include <Grappa.hpp>

#include "squares_partition_bushy.hpp"
#include "local_graph.hpp"
#include "Hypercube.hpp"
#include "utility.hpp"


// graph includes
#include "grappa/graph.hpp"

using namespace Grappa;

#define DIFFERENT_RELATIONS 0
#define DEDUP_EDGES 1

// Currently assumes an undirected graph implemented as a large directed graph!!



#include <sstream>
#include <string>


namespace squares_partition_bushy {
  std::vector<Edge> localAssignedEdges_R1;
  std::vector<Edge> localAssignedEdges_R2;
  std::vector<Edge> localAssignedEdges_R3;
  std::vector<Edge> localAssignedEdges_R4;

  uint64_t edgesSent = 0;

  
  int64_t started = 1;
  int64_t finished = 1;
}
using namespace squares_partition_bushy;

//TODO




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
  
  
void SquarePartitionBushy4way::preprocessing(std::vector<tuple_graph> relations) {
  // for this query plan, the graph construction is just to clean up
  // the input
  LOG(INFO) << "graph construction...";
  double t = walltime();
  this->index = Graph<Vertex>::create(relations[0], /*directed=*/true); 
  double construction_time = walltime() - t;
  LOG(INFO) << "construction_time: " << construction_time;
  LOG(INFO) << "num edges: " << this->index->nadj * 4; /* 4 b/c three copies*/

}

  
  
void SquarePartitionBushy4way::execute(std::vector<tuple_graph> relations) {
  // arrange the processors in 4d
  auto sidelength = fourth_root(cores());
  LOG(INFO) << "using " << sidelength*sidelength*sidelength*sidelength << " of " << cores() << " cores";
  participating_cores = sidelength*sidelength*sidelength*sidelength;

  auto g = this->index;
    
  // 1. Send edges to the partitions
  //
  // really just care about local edges; we get to them
  // indirectly through local vertices at the moment.
  // This is sequential access since edgeslists and vertices are sorted the same
  forall( g->vs, g->nv, [sidelength](int64_t i, Vertex& v) {
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
      delegate::call<async>( l, [e] { 
        localAssignedEdges_R1.push_back(e); 
        VLOG(5) << "received " << e << " as a->b";
        });
      edgesSent++;
      }

      // b->c
      auto locs_bc = h.slice( {HypercubeSlice::ALL, hf(src), hf(dst), HypercubeSlice::ALL} );
      for (auto l : locs_bc) {
        Edge e(src, dst);
        delegate::call<async>( l, [e] { 
            localAssignedEdges_R2.push_back(e); 
            VLOG(5) << "received " << e << " as b->c";
            });
        edgesSent++;
      }

      // c->d
      auto locs_cd = h.slice( {HypercubeSlice::ALL, HypercubeSlice::ALL, hf(src), hf(dst)} );
      for (auto l : locs_cd) {
        Edge e(src, dst);
        delegate::call<async>( l, [e] { 
            localAssignedEdges_R3.push_back(e); 
            VLOG(5) << "received " << e << " as c->d";
            });
        edgesSent++;
      }

      // d->a
      auto locs_da = h.slice( {hf(dst), HypercubeSlice::ALL, HypercubeSlice::ALL, hf(src)} );
      for (auto l : locs_da) {
        Edge e(src, dst);
        delegate::call<async>( l, [e] { 
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
      auto& R4 = *new LocalAdjListGraph(R4_dedup);
#else
      LOG(INFO) << "local graph construct...";
      auto& R1 = *new LocalAdjListGraph(localAssignedEdges_R1);
      auto& R2 = *new LocalAdjListGraph(localAssignedEdges_R2);
      auto& R3 = *new LocalAdjListGraph(localAssignedEdges_R3);
      auto& R4 = *new LocalAdjListGraph(localAssignedEdges_R4);
#endif


      LOG(INFO) << "query execution "; 


      std::unordered_set<pair_t> ac_b;

      for (auto xy: R1.vertices()) {
        int64_t x = xy.first;
        auto& xadj = xy.second;
        for (auto y : xadj) {
          ir1_count++; // count(R1)
          auto& yadj = R2.neighbors(y);
          VLOG(5) << "ir1: " << resultStr({x, y}, yadj.size());
          ir2_count+=yadj.size(); // count(R1xR2)
          for (auto z : yadj) {
            ac_b.insert( pair_t(x, z) );
          }
        }
      }

      for (auto zt: R3.vertices()) {
        int64_t z = zt.first;
        auto& zadj = zt.second;
        for (auto t : zadj) {
          ir3_count++;
          auto& tadj = R4.neighbors(t);  
          for (auto x : tadj) {
            if ( ac_b.end() != ac_b.find( pair_t(x, z) ) ) {
              VLOG(5) << "result: " << resultStr({x, -1, z, t});
              results_count++;
            }
          }
        }
      }
  });

      LOG(INFO) << "query complete ";

}

