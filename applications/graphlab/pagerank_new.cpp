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
/// Demonstrates using the GraphLab API to implement Pagerank
/// 
/// This variant uses GraphlabGraph (which uses GraphLab's split vertex
/// representation).
/// 
/// @note: This is currently *slower* than the "naive" engine, due to
/// an inefficient implementation of the graph structure.
////////////////////////////////////////////////////////////////////////

#include <Grappa.hpp>
#include "graphlab.hpp"

DEFINE_bool( metrics, false, "Dump metrics");

DEFINE_int32(scale, 10, "Log2 number of vertices.");
DEFINE_int32(edgefactor, 16, "Average number of edges per vertex.");

DEFINE_int32(trials, 3, "Number of timed trials to run and average over.");

DEFINE_string(path, "", "Path to graph source file.");
DEFINE_string(format, "bintsv4", "Format of graph source file.");

GRAPPA_DEFINE_METRIC(SimpleMetric<double>, init_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, tuple_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, construction_time, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, total_time, 0);

const double RESET_PROB = 0.15;
DEFINE_double(tolerance, 1.0E-2, "tolerance");
#define TOLERANCE FLAGS_tolerance

struct PagerankVertexData {
  double rank;
  PagerankVertexData(double rank = 1.0): rank(rank) {}
};

using G = GraphlabGraph<PagerankVertexData,Empty>;

struct PagerankVertexProgram : public GraphlabVertexProgram<G,double> {
  double delta;
  
  PagerankVertexProgram() = default;
  PagerankVertexProgram(const Vertex& v) {}
  
  bool gather_edges(const Vertex& v) const { return true; }
  
  Gather gather(const Vertex& v, Edge& e) const {
    auto& src = e.source();
    return src->rank / src.num_out_edges();
  }
  void apply(Vertex& v, const Gather& total) {
    auto new_val = (1.0 - RESET_PROB) * total + RESET_PROB;
    delta = (new_val - v->rank) / v.num_out_edges();
    v->rank = new_val;
  }
  bool scatter_edges(const Vertex& v) const {
    return std::fabs(delta * v.num_out_edges()) > TOLERANCE;
  }
  Gather scatter(const Edge& e, Vertex& target) const {
    target.activate();
    return delta;
  }
};

Reducer<double,ReducerType::Add> total_rank;
Reducer<int64_t,ReducerType::Add> count;

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
      }
    }
    LOG(INFO) << tuple_time;
    LOG(INFO) << "constructing graph";
    t = walltime();
    
    auto g = G::create(tg);
    
    construction_time = walltime()-t;
    LOG(INFO) << construction_time;
    
    count = 0;
    forall(masters(g), [](G::Vertex& v){
      count += v.n_out;
    });
    CHECK_EQ(count, g->ne);
    
    Metrics::start_tracing();
    
    for (int i = 0; i < FLAGS_trials; i++) {
      if (FLAGS_trials > 1) LOG(INFO) << "trial " << i;
      
      forall(g, [](G::Vertex& v){ v->rank = 1.0; });
      
      GRAPPA_TIME_REGION(total_time) {
        activate_all(g);
        GraphlabEngine<G,PagerankVertexProgram>::run_sync(g);
      }
      
      if (i == 0) {
        total_time.reset(); // don't count the first one
        total_rank = 0;
        forall(g, [](G::Vertex& v){ total_rank += v->rank; });
        std::cerr << "total_rank: " << total_rank << "\n";
      }      
    }
    
    Metrics::stop_tracing();
    
    LOG(INFO) << total_time;
    
    total_rank = 0;
    forall(masters(g), [](G::Vertex& v){ total_rank += v->rank; });
    LOG(INFO) << "total_rank: " << total_rank << "\n";
    
  });
  finalize();
}
