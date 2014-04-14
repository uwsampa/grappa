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
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

/// Tests for delegate operations

#include "Grappa.hpp"
#include <unordered_set>
using std::unordered_set;

using namespace Grappa;

DEFINE_int64(log_n, 10, "how many mallocs to do per core.");

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, malloc_time, 0.0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, insert_time, 0.0);

int main(int argc, char* argv[]) {
  Grappa::init(&argc, &argv);
  Grappa::run([]{
    
    LOG(INFO) << "checking malloc throughput";
        
    size_t n = 1 << FLAGS_log_n;
    
    on_all_cores([=]{
      auto a = new long*[n];
      // for (int i=0; i < n; i++) a[i] = nullptr;
      
      double t = walltime();
      
      for (int i=0; i < n; i++) {
        a[i] = new long[2];
        // if (i > 10 && i % 2) {
        //   size_t r = rand() % (i-1);
        //   delete[] a[ rand() % (i-1) ];
        // }
      }
      
      malloc_time += walltime() - t;
      
      for (int i=0; i < n; i++) delete[] a[i];
    });

    LOG(INFO) << "checking unordered_set insert";
    on_all_cores([=]{
      unordered_set<long> s;
      GRAPPA_TIME_REGION(insert_time) {
        for (size_t i=0; i<n; i++) {
          s.insert(rand());
        }
      }
    });
    // LOG(INFO) << "set_insert_time: " << walltime()-t;
    
    Metrics::merge_and_dump_to_file();
  });
  Grappa::finalize();
}
