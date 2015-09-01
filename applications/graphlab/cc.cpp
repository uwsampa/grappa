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
/// Use the GraphLab API to implement Connected Components
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
