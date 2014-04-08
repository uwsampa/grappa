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

struct PagerankVertexData : public GraphlabVertexData<double> {
  double rank;
  PagerankVertexData(double initial_rank = 1.0): rank(initial_rank) {}
};

using G = Graph<PagerankVertexData,Empty>;

struct PagerankVertexProgram {
  double delta;
  
  bool gather_edges(const G::Vertex& v) const { return true; }
  
  double gather(G::Vertex& src, G::Edge& e) const {
    return src->rank / src.nadj;
  }
  void apply(G::Vertex& v, double total) {
    auto new_val = (1.0 - RESET_PROB) * total + RESET_PROB;
    delta = (new_val - v->rank) / v.nadj;
    v->rank = new_val;
  }
  bool scatter_edges(const G::Vertex& v) const {
    return std::fabs(delta * v.nadj) > TOLERANCE;
  }
  void scatter(const G::Edge& e, G::Vertex& target) const {
    target->post_delta(delta);
    target->activate();
  }
};

Reducer<double,ReducerType::Add> total_rank;
Reducer<int64_t,ReducerType::Add> nzero;
Reducer<int64_t,ReducerType::Add> count;

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{

    double t;
    
    DVLOG(0) << "debug mode";
    
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
    
    auto g = G::create(tg, true);
    
    GRAPPA_TIME_REGION(init_time) {
      // TODO: random init
      forall(g, [](G::Vertex& v){ new (&v.data) PagerankVertexData(1.0); });
    }
    
    forall(g, [](G::Vertex& v){ if (v.nadj == 0) nzero++; });
    VLOG(0) << "nzero: " << nzero;
    
    count = 0;
    forall(g, [](G::Vertex& v){ if (v.nadj > 0) v->activate(); });
    forall(g, [](G::Edge& e, G::Vertex& ve){ ve->activate(); });
    forall(g, [](G::Vertex& v){
      if (v->active) count++;
      v->deactivate();
    });
    VLOG(0) << "actual vertices: " << count;
    
    tg.destroy();
    
    construction_time = (walltime()-t);
    LOG(INFO) << construction_time;
    
    LOG(INFO) << "starting pagerank";

    GRAPPA_TIME_REGION(total_time) {
      activate_all(g);
      run_synchronous< PagerankVertexProgram >(g);
    }
    
    LOG(INFO) << "-- pagerank done";
    
    total_rank = 0;
    forall(g, [](G::Vertex& v){ total_rank += v->rank; });
    std::cerr << "total_rank: " << total_rank << "\n";
    
    if (FLAGS_metrics) Metrics::merge_and_print();
    else {
      std::cerr << total_time << "\n" << iteration_time << "\n";
    }
    Metrics::merge_and_dump_to_file();

    if (FLAGS_scale <= 8) {
      g->dump([](std::ostream& o, G::Vertex& v){
        o << "{ rank:" << v->rank << " }";
      });
    }
    
    g->destroy();
  });
  finalize();
}
