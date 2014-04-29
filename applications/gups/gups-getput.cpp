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
DEFINE_int64( sizeA, 1 << 30, "Size of array that GUPS increments" );
DEFINE_int64( sizeB, 1 << 20, "Number of iterations" );

// define custom statistics which are logged by the runtime
// (here we're not using these features, just printing them ourselves)
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_throughput, 0.0 );

const uint64_t LARGE_PRIME = 18446744073709551557UL;

int main(int argc, char * argv[]) {
  init( &argc, &argv );
  run([]{

    // create target array that we'll be updating
    auto A = global_alloc<int64_t>(FLAGS_sizeA);
    Grappa::memset( A, 0, FLAGS_sizeA );
    
    // create array of random indexes into A
    auto B = global_alloc<int64_t>(FLAGS_sizeB);
    forall( B, FLAGS_sizeB, [](int64_t& b) {
      b = random() % FLAGS_sizeA;
    });

    double start = walltime();

    forall(B, FLAGS_sizeB, [=](int64_t& b){
//     forall<unbound>( 0, FLAGS_sizeB-1, [=] ( int64_t i ) {
//                        uint64_t b = (i * LARGE_PRIME) % FLAGS_sizeA;
                       auto a = delegate::read( A + b );
                       delegate::write<async>( A + b, a + 1 );
    });

    gups_runtime = walltime() - start;
    gups_throughput = FLAGS_sizeB / gups_runtime;

    LOG(INFO) << gups_throughput.value() << " UPS in " << gups_runtime.value() << " seconds";
    
    global_free(B);
    global_free(A);

    Grappa::Metrics::merge_and_print();
  });
  finalize();
}
