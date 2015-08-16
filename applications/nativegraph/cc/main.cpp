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
/// Simon Kahan's 3-phase Connected Components (for Grappa Graph)
////////////////////////////////////////////////////////////////////////

#include <Grappa.hpp>
#include "cc_kahan.hpp"

DEFINE_bool( metrics, false, "Dump metrics");

DEFINE_int32(scale, 10, "Log2 number of vertices.");
DEFINE_int32(edgefactor, 16, "Average number of edges per vertex.");

DEFINE_string(path, "", "Path to graph source file.");
DEFINE_string(format, "bintsv4", "Format of graph source file.");

GRAPPA_DEFINE_METRIC(SimpleMetric<double>, init_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, tuple_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, construction_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, total_time, 0);

////////////////////
// used in cc_kahan
GlobalCompletionEvent phaser;

DEFINE_int64(hash_size, 1<<14, "size of GlobalHashSet");
DEFINE_int64(concurrent_roots, 1, "number of concurrent `explores`");

GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, ncomponents, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, pram_passes, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, set_size, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, set_insert_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, pram_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, propagate_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, components_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, graph_create_time, 0);

size_t connected_components(GlobalAddress<G> g);

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
    
    GRAPPA_TIME_REGION(total_time) {
      ncomponents = connected_components(g);
    }
    LOG(INFO) << total_time;
    
    if (FLAGS_scale <= 8) {
      g->dump([](std::ostream& o, G::Vertex& v){
        o << "{ label:" << v->color << " }";
      });
    }
    
    if (FLAGS_metrics) Metrics::merge_and_print();
    else {
      LOG(INFO) << "\n" << set_insert_time
                << "\n" << pram_time
                << "\n" << propagate_time
                << "\n" << ncomponents
                << "\n" << set_size
                << "\n" << components_time;
    }
    Metrics::merge_and_dump_to_file();
    
  });
  finalize();
}
