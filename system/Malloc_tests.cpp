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
