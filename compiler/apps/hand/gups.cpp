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

#include <Grappa.hpp>

using namespace Grappa;

// define command-line flags (third-party 'gflags' library)
DEFINE_int64( log_array_size, 28, "Size of array that GUPS increments (log2)" );
DEFINE_int64( log_iterations, 20, "Iterations (log2)" );
DEFINE_bool( metrics, false, "Dump metrics");

static int64_t sizeA, sizeB;

// define custom statistics which are logged by the runtime
// (here we're not using these features, just printing them ourselves)
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_throughput, 0.0 );

int main(int argc, char * argv[]) {
  init( &argc, &argv );
  
  sizeA = 1L << FLAGS_log_array_size,
  sizeB = 1L << FLAGS_log_iterations;
  
  run([]{

    // create target array that we'll be updating
    auto A = global_alloc<int64_t>(sizeA);
    Grappa::memset( A, 0, sizeA );
    
    // create array of random indexes into A
    auto B = global_alloc<int64_t>(sizeB);
    forall( B, sizeB, [](int64_t& b) {
      b = random() % sizeA;
    });

    double start = walltime();

    forall(B, sizeB, [=](int64_t& b){
      delegate::increment<async>( A + b, 1);
    });

    gups_runtime = walltime() - start;
    gups_throughput = sizeB / gups_runtime;

    LOG(INFO) << gups_throughput.value() << " UPS in " << gups_runtime.value() << " seconds";
    
    global_free(B);
    global_free(A);
        
    if (FLAGS_metrics) Metrics::merge_and_print();
    
  });
  finalize();
}
