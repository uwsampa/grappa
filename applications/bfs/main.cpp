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
#include "common.hpp"

DEFINE_bool( metrics, false, "Dump metrics");

DEFINE_int32(scale, 10, "Log2 number of vertices.");
DEFINE_int32(edgefactor, 16, "Average number of edges per vertex.");
DEFINE_int32(nbfs, 1, "Number of BFS traversals to do.");

DEFINE_string(path, "", "Path to graph source file.");
DEFINE_string(format, "bintsv4", "Format of graph source file.");

GRAPPA_DEFINE_METRIC(SimpleMetric<double>, init_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, tuple_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, construction_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, total_time, 0);

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, bfs_mteps, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, bfs_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, bfs_nedge, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, verify_time, 0);

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    
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
    
    double t = walltime();
    
    // construct the compact graph representation (roughly CSR)
    auto g = G::Undirected( tg );
    
    construction_time = (walltime()-t);
    LOG(INFO) << construction_time;
    
    GRAPPA_TIME_REGION(total_time) {
      bfs(g, FLAGS_nbfs, tg);
    }
    
    LOG(INFO) << "\n" << bfs_nedge << "\n" << bfs_time << "\n" << bfs_mteps;
    if (FLAGS_metrics) Metrics::merge_and_print();
    Metrics::merge_and_dump_to_file();
  });
  finalize();
}
