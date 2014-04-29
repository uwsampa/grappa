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
#include <Reducer.hpp>
#include <GlobalBag.hpp>

DEFINE_bool( max_degree_source, false, "Start from maximum degree vertex");

DEFINE_double(beamer_alpha, 20.0, "Beamer BFS parameter for switching to bottom-up.");
DEFINE_double(beamer_beta, 20.0, "Beamer BFS parameter for switching back to top-down.");

GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, bfs_mteps);
GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, total_time);
GRAPPA_DECLARE_METRIC(SimpleMetric<int64_t>, bfs_nedge);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, graph_create_time);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, verify_time);

using MaxDegree = CmpElement<VertexID,int64_t>;
Reducer<MaxDegree,ReducerType::Max> max_degree;

int current_depth;
GlobalAddress<GlobalBag<VertexID>> frontier, next;
GlobalAddress<G> g;

GlobalCompletionEvent phaser;

Reducer<int64_t,ReducerType::Add> edge_count;

void bfs(GlobalAddress<G> _g, int nbfs, TupleGraph tg) {
  bool verified = false;
  double t;
      
  auto _frontier = GlobalBag<VertexID>::create(_g->nv);
  auto _next     = GlobalBag<VertexID>::create(_g->nv);
  call_on_all_cores([=]{ frontier = _frontier; next = _next; g = _g; });
    
  // do BFS from multiple different roots and average their times
  for (int root_idx = 0; root_idx < nbfs; root_idx++) {
  
    // intialize parent to -1
    forall(g, [](G::Vertex& v){ v->init(); v->level = -1; });
    
    VertexID root;
    if (FLAGS_max_degree_source) {
      forall(g, [](VertexID i, G::Vertex& v){
        max_degree << MaxDegree(i, v.nadj);
      });
      root = static_cast<MaxDegree>(max_degree).idx();
    } else {
      root = choose_root(g);
    }
    
    // setup 'root' as the parent of itself
    delegate::call(g->vs+root, [=](G::Vertex& v){
      v->parent = root;
      v->level = 0;
    });
    
    // reset frontier queues
    next->clear();
    frontier->clear();
    
    // start with root as only thing in frontier
    delegate::call((g->vs+root).core(), [=]{ frontier->add(root); });
    
    t = walltime();
    
    bool top_down = true;
    int64_t prev_nf = -1;
    int64_t frontier_edges = 0;
    int64_t remaining_edges = g->nadj;
    
    while (!frontier->empty()) {
      
      auto nf = frontier->size();
      VLOG(1) << "remaining_edges = " << remaining_edges << ", nf = " << nf << ", prev_nf = " << prev_nf << ", frontier_edges: " ;
      if (top_down && frontier_edges > remaining_edges/FLAGS_beamer_alpha && nf > prev_nf) {
        VLOG(1) << "switching to bottom-up";
        top_down = false;
      } else if (!top_down && frontier_edges < g->nv/FLAGS_beamer_beta && nf < prev_nf) {
        VLOG(1) << "switching to top-down";
        top_down = true;
      }
      
      edge_count = 0;
      
      if (top_down) {
                
        // iterate over vertices in this level of the frontier
        forall(frontier, [](VertexID& i){
          // visit all the adjacencies of the vertex
          // note: this has to be 'async' to prevent deadlock from
          //       running out of available workers
          forall<async>(adj(g,i), [i](G::Edge& e) {
            auto j = e.id;
            // at the core where the vertex is...
            delegate::call<async>(e.ga, [i,j](G::Vertex& vj){
              // note: no synchronization needed because 'call' is 
              // guaranteed to be executed atomically because it 
              // does no blocking operations
              if (vj->parent == -1) {
                // claim parenthood
                vj->parent = i;
                vj->level = current_depth;
                next->add(j);
                edge_count += vj.nadj;
              }
            });
          });
        });
      } else { // bottom-up
        
        forall<&phaser>(g, [](G::Vertex& v){
          if (v->level != -1) return;
          auto va = make_linear(&v);
          forall<async,&phaser>(adj(g,v), [=,&v](G::Edge& e){
            if (v->level != -1) return;
            
            phaser.enroll();
            auto eva = e.ga;
            send_heap_message(eva.core(), [=]{
              auto& ev = *eva.pointer();
              if (ev->level != -1 && ev->level < current_depth) {
                auto eid = g->id(ev);
                send_heap_message(va.core(), [=]{
                  auto& v = *va.pointer();
                  if (v->level == -1) {
                    next->add(g->id(v));
                    v->level = current_depth;
                    v->parent = eid;
                    edge_count += v.nadj;
                  }
                  phaser.complete();
                });
              } else {
                phaser.send_completion(va.core());
              }
            });
          });
        });
      }
      
      call_on_all_cores([=]{
        current_depth++;
        // switch to next frontier level
        std::swap(frontier, next);
      });
      next->clear();
      frontier_edges = edge_count;
      remaining_edges -= frontier_edges;
      prev_nf = nf;
    } // while (frontier not empty)
    
    double this_bfs_time = walltime() - t;
    LOG(INFO) << "(root=" << root << ", time=" << this_bfs_time << ")";
    
    if (!verified) {
      // only verify the first one to save time
      t = walltime();
      bfs_nedge = verify(tg, g, root);
      verify_time = (walltime()-t);
      LOG(INFO) << verify_time;
      verified = true;
      Metrics::reset_all_cores(); // don't count the first one
    } else {
      total_time += this_bfs_time;
    }
    
    bfs_mteps += bfs_nedge / this_bfs_time / 1.0e6;
  }
}
