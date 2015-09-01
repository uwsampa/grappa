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
/// Demonstrates using the GraphLab API to implement SSSP
////////////////////////////////////////////////////////////////////////
#include <Grappa.hpp>
#include "graphlab.hpp"

DEFINE_bool( metrics, false, "Dump metrics");

DEFINE_bool( max_degree_source, false, "Start from maximum degree vertex");
DEFINE_int32(scale, 10, "Log2 number of vertices.");
DEFINE_int32(edgefactor, 16, "Average number of edges per vertex.");

DEFINE_int32(trials, 3, "Number of timed trials to run and average over.");

DEFINE_string(path, "", "Path to graph source file.");
DEFINE_string(format, "bintsv4", "Format of graph source file.");

GRAPPA_DEFINE_METRIC(SimpleMetric<double>, tuple_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, construction_time, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, total_time, 0);

using distance_t = float;

const distance_t MAX_DIST = std::numeric_limits<distance_t>::max();

struct SSSPVertexData : public GraphlabVertexData< SSSPVertexData > {
  distance_t dist, new_dist;
  SSSPVertexData(): dist(MAX_DIST), new_dist(0.0) {}
};
struct EdgeDistance {
  distance_t dist;
};

using G = Graph<SSSPVertexData,EdgeDistance>;


template< typename G >
inline VertexID choose_root(GlobalAddress<G> g) {
  VertexID root;
  do {
    root = random() % g->nv;
  } while (call(g->vs+root,[](typename G::Vertex& v){ return v.nadj; }) == 0);
  return root;
}

struct SSSP : public GraphlabVertexProgram<G,bool> {
  bool changed;
  double min_dist;
  
  SSSP(const Vertex& v) {}
  
  bool gather_edges(Vertex& v) const { return false; }
  
  bool gather(Vertex& src, Edge& e) const { return 0; }
  
  void apply(Vertex& v, bool ignore) {
    changed = false;
    if (v->dist > v->new_dist) {
      changed = true;
      v->dist = v->new_dist;
    }
    min_dist = v->dist; // so we can use it in 'scatter'
  }
  
  bool scatter_edges(Vertex& v) const { return changed; }
  
  bool scatter(const Edge& e, Vertex& target) const {
    auto new_dist = min_dist + e->dist;
    if (new_dist < target->dist) {
      target->new_dist = new_dist;
      target->activate();
    }
    return false;
  }
};

using MaxDegree = CmpElement<VertexID,int64_t>;
Reducer<MaxDegree,ReducerType::Max> max_degree;

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    double t;
    
    TupleGraph tg;
    
    GRAPPA_TIME_REGION(tuple_time) {
      if (FLAGS_path.empty()) {
        int64_t NE = (1L << FLAGS_scale) * FLAGS_edgefactor;
        tg = TupleGraph::Kronecker(FLAGS_scale, NE, 111, 222);
      } else {
        LOG(INFO) << "loading " << FLAGS_path;
        tg = TupleGraph::Load(FLAGS_path, FLAGS_format);
        LOG(INFO) << "done! loaded " << tg.nedge << " edges";
      }
    }
    
    t = walltime();
    
    auto g = G::create(tg); // undirected
    
    // TODO: random init
    forall(g, [=](G::Vertex& v){
      new (&v.data) SSSPVertexData();
      forall<async>(adj(g,v), [](G::Edge& e){
        e->dist = 1.0;
      });
    });
    
    tg.destroy();
    
    construction_time = (walltime()-t);
    LOG(INFO) << construction_time;
    
    VertexID root;
    if (FLAGS_max_degree_source) {
      forall(g, [](VertexID i, G::Vertex& v){
        max_degree << MaxDegree(i, v.nadj);
      });
      root = static_cast<MaxDegree>(max_degree).idx();
    } else {
      root = choose_root(g);
    }
    
    LOG(INFO) << "SSSP on root:" << root;
    
    for (int i = 0; i < FLAGS_trials; i++) {
      if (FLAGS_trials > 1) LOG(INFO) << "trial " << i;
      
      // re-initialize
      forall(g, [](G::Vertex& v){ new (&v.data) SSSPVertexData(); });
      
      GRAPPA_TIME_REGION(total_time) {
        activate(g->vs+root);
        NaiveGraphlabEngine<G,SSSP>::run_sync(g);
      }
      
      if (i == 0) Metrics::reset_all_cores(); // don't count the first one
    }
    
    LOG(INFO) << "-- SSSP done";
    
    if (FLAGS_metrics) Metrics::merge_and_print();
    else {
      std::cerr << total_time << "\n"
                << iteration_time << "\n";
    }
    Metrics::merge_and_dump_to_file();

    if (FLAGS_scale <= 8) {
      g->dump([](std::ostream& o, G::Vertex& v){
        o << "{ dist:" << v->dist << " }";
      });
    }
    
    g->destroy();
  });
  finalize();
}
