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
    
    auto g = G::Directed(tg);
    
    construction_time = walltime()-t;
    LOG(INFO) << construction_time;
    
    GRAPPA_TIME_REGION(total_time) {
      ncomponents = connected_components(g);
    }
    LOG(INFO) << total_time;
    
    
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
