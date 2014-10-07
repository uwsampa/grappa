////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
/// This mini-app demonstrates a breadth-first-search of the built-in 
/// graph data structure. This implement's Graph500's BFS benchmark:
/// - Uses the Graph500 Specification Kronecker graph generator with
///   numVertices = 2^scale (--scale specified on command-line)
/// - Uses the builtin hybrid compressed-sparse-row graph format
/// - Computes the 'parent' tree given a root, and does this a number 
///   of times (specified by --nbfs).
////////////////////////////////////////////////////////////////////////

#include "common.hpp"

GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, bfs_mteps);
GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, total_time);
GRAPPA_DECLARE_METRIC(SimpleMetric<int64_t>, bfs_nedge);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, graph_create_time);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, verify_time);

void bfs(GlobalAddress<G> g, int nbfs, TupleGraph tg) {
  bool verified = false;
  double t;
      
  auto frontier = GlobalVector<int64_t>::create(g->nv);
  auto next     = GlobalVector<int64_t>::create(g->nv);
  
  // do BFS from multiple different roots and average their times
  for (int root_idx = 0; root_idx < nbfs; root_idx++) {
  
    // intialize parent to -1
    forall(g, [](G::Vertex& v){ v->init(); });
    
    int64_t root = choose_root(g);
    VLOG(1) << "root => " << root;
    
    // setup 'root' as the parent of itself
    delegate::call(g->vs+root, [=](G::Vertex& v){ v->parent = root; });
    
    // reset frontier queues
    next->clear();
    frontier->clear();
    
    // start with root as only thing in frontier
    frontier->push(root);
    
    t = walltime();
    
    while (!frontier->empty()) {
      // iterate over vertices in this level of the frontier
      forall(frontier, [g,next](int64_t& i){
        // visit all the adjacencies of the vertex
        // note: this has to be 'async' to prevent deadlock from
        //       running out of available workers
        forall<async>(adj(g,g->vs+i), [i,next](G::Edge& e) {
          auto j = e.id;
          // at the core where the vertex is...
          bool claimed = delegate::call(e.ga, [i](G::Vertex& v){
            // note: no synchronization needed because 'call' is 
            // guaranteed to be executed atomically because it 
            // does no blocking operations
            if (v->parent == -1) {
              // claim parenthood
              v->parent = i;
              return true;
            }
            return false;
          });
          if (claimed) {
            // add this vertex to the frontier for the next level
            // note: we (currently) can't do this 'push' inside the delegate because it may block
            next->push(j);
          }
        });
      });
      // switch to next frontier level
      std::swap(frontier, next);
      next->clear();
    }
    
    double this_total_time = walltime() - t;
    LOG(INFO) << "(root=" << root << ", time=" << this_total_time << ")";
    total_time += this_total_time;
    
    if (!verified) {
      // only verify the first one to save time
      t = walltime();
      bfs_nedge = verify(tg, g, root);
      verify_time = (walltime()-t);
      LOG(INFO) << verify_time;
      verified = true;
    }
    
    bfs_mteps += bfs_nedge / this_total_time / 1.0e6;
  }
}
