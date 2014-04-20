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
// http://www.affero.org/oagl.html.
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
/// Demonstrates using the GraphLab API to implement Pagerank
////////////////////////////////////////////////////////////////////////
#include <Grappa.hpp>
#include "graphlab.hpp"

DEFINE_bool( metrics, false, "Dump metrics");

DEFINE_int32(scale, 10, "Log2 number of vertices.");
DEFINE_int32(edgefactor, 16, "Average number of edges per vertex.");

DEFINE_string(path, "", "Path to graph source file.");
DEFINE_string(format, "bintsv4", "Format of graph source file.");

GRAPPA_DEFINE_METRIC(SimpleMetric<double>, init_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, tuple_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, construction_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, total_time, 0);

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
    
    GRAPPA_TIME_REGION(total_time) {
      activate_all(g);
      GraphlabEngine<G,PagerankVertexProgram>::run_sync(g);
    }
    
    Metrics::stop_tracing();
    
    LOG(INFO) << total_time;
    
    total_rank = 0;
    forall(masters(g), [](G::Vertex& v){ total_rank += v->rank; });
    LOG(INFO) << "total_rank: " << total_rank << "\n";
    
  });
  finalize();
}
