////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
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

Grappa::GlobalCompletionEvent gce;

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

		std::function<void (G::Vertex& )> update = [&](G::Vertex& v){
			if (v->parent == -1) {
				VLOG(1) << &v << ": updated\n";
				v->parent = 0;//FIXME: here parent must be used 
				forall<async,&gce>(adj(g,v), [=](G::Edge& e) {
						VLOG(1) << "call update\n";
						delegate::call<async, &gce>(e.ga, update);
					});
			}
		};
    
    // setup 'root' as the parent of itself
    delegate::call<async, &gce>(g->vs+root, update); 

		gce.wait();

    double this_total_time = walltime() - t;
    LOG(INFO) << "(root=" << root << ", time=" << this_total_time << ")";
    total_time += this_total_time;
    
    /*if (!verified) {
      // only verify the first one to save time
      t = walltime();
      bfs_nedge = verify(tg, g, root);
      verify_time = (walltime()-t);
      LOG(INFO) << verify_time;
      verified = true;
    }*/
    
    bfs_mteps += bfs_nedge / this_total_time / 1.0e6;
  }
}
