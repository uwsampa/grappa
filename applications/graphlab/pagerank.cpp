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
const double TOLERANCE = 1.0E-2;

Reducer<int64_t,ReducerType::Add> active_count;

struct PagerankVertexData : public GraphlabVertex<double> {

  double rank;
  
  PagerankVertexData(double initial_rank = 0.0)
    : GraphlabVertex()
    , rank(initial_rank)
  { }
};

using G = Graph<PagerankVertexData,Empty>;

struct PagerankVertexProgram {
  double last_change;
  
  bool gather_edges(const G::Vertex& v) const { return true; }
  bool scatter_edges(const G::Vertex& v) const { return true; }
  
  double gather(G::Vertex& src, G::Edge& e) const {
    return src->rank / src.nadj;
  }
  void apply(G::Vertex& v, double total) {
    auto new_val = (1.0 - RESET_PROB) * v->cache + RESET_PROB;
    last_change = (new_val - v->rank) / v.nadj;
    v->rank = new_val;
  }
  void scatter(const G::Edge& e, G::Vertex& target) const {
    target->post_delta(last_change);
    if (std::fabs(last_change) > TOLERANCE) {
      target->activate();
    }
  }
};

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
    
    tg.destroy();
    
    construction_time = (walltime()-t);
    LOG(INFO) << construction_time;
    
    LOG(INFO) << "starting pagerank";

    GRAPPA_TIME_REGION(total_time) {
      activate_all(g);
      run_synchronous< PagerankVertexProgram >(g);
    }
    
    LOG(INFO) << "-- pagerank done";
    
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
