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

struct CCData : public GraphlabVertexData<CCData> {
  uint64_t label;
};

struct MinLabel {
  uint64_t v;
  explicit MinLabel(uint64_t v): v(v) {}
  MinLabel(): v(std::numeric_limits<uint64_t>::max()) {}
  MinLabel& operator+=(const MinLabel& o) {
    v = std::min(v, o.v);
    return *this;
  }
  operator uint64_t () const { return v; }
};

using G = Graph<CCData,Empty>;

struct LabelPropagation : public GraphlabVertexProgram<G,MinLabel> {
  bool do_scatter;
  uint64_t tmp_label;
  
  LabelPropagation(Vertex& v): tmp_label(-1) {}
  
  bool gather_edges(const Vertex& v) const { return true; }
  
  Gather gather(const Vertex& v, Edge& e) const { return MinLabel(v->label); }
  
  void apply(Vertex& v, const Gather& total) {
    uint64_t min_label = static_cast<uint64_t>(total);
    if (v->label > min_label) {
      v->label = tmp_label = min_label;
      do_scatter = true;
    } else {
      do_scatter = false;
    }
  }
  
  bool scatter_edges(const Vertex& v) const { return do_scatter; }
  
  Gather scatter(const Edge& e, Vertex& target) const {
    if (target->label > tmp_label) {
      target->activate();
      return MinLabel(tmp_label);
    }
    return MinLabel();
  }
};

Reducer<int64_t,ReducerType::Add> nc;

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
    
    auto g = G::Undirected(tg);
    
    construction_time = walltime()-t;
    LOG(INFO) << construction_time;
        
    for (int i = 0; i < FLAGS_trials; i++) {
      if (FLAGS_trials > 1) LOG(INFO) << "trial " << i;
      
      GRAPPA_TIME_REGION(total_time) {
        forall(g, [](VertexID i, G::Vertex& v){
          v->label = i;
          v->activate();
        });
        NaiveGraphlabEngine<G,LabelPropagation>::run_sync(g);
      }
      
      if (i == 0) Metrics::reset_all_cores(); // don't count the first one
    }
    
    LOG(INFO) << total_time;
    
    if (FLAGS_scale <= 8) {
      g->dump([](std::ostream& o, G::Vertex& v){
        o << "{ label:" << v->label << " }";
      });
    }
    
    nc = 0;
    forall(g, [](VertexID i, G::Vertex& v){ if (v->label == i) nc++; });
    LOG(INFO) << "ncomponents: " << nc;
    
    if (FLAGS_metrics) Metrics::merge_and_print();
    else { std::cerr << total_time << "\n" << iteration_time << "\n"; }
    Metrics::merge_and_dump_to_file();
  });
  finalize();
}
