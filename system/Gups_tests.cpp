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

/// One implementation of GUPS. This does no load-balancing, and may
/// suffer from some load imbalance.

#include <memory>
#include <algorithm>

#include <Grappa.hpp>
#include "GlobalAllocator.hpp"
#include "Array.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"
#include "Metrics.hpp"
#include "Collective.hpp"

#include "LocaleSharedMemory.hpp"

#include "ParallelLoop.hpp"
#include "AsyncDelegate.hpp"


#include <boost/test/unit_test.hpp>

DEFINE_int64( repeats, 1, "Repeats" );
DEFINE_int64( iterations, 1 << 25, "Iterations" );
DEFINE_int64( sizeA, 1024, "Size of array that gups increments" );
DEFINE_bool( validate, true, "Validate result" );

DECLARE_string( load_balance );

GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_throughput, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_throughput_per_locale, 0 );

const uint64_t LARGE_PRIME = 18446744073709551557UL;


BOOST_AUTO_TEST_SUITE( Gups_tests );

void validate(GlobalAddress<int64_t> A, size_t n) {
  int total = 0, max = 0, min = INT_MAX;
  double sum_sqr = 0.0;
  for (int i = 0; i < n; i++) {
    int tmp = Grappa::delegate::read(A+i);
    total += tmp;
    sum_sqr += tmp*tmp;
    max = tmp > max ? tmp : max;
    min = tmp < min ? tmp : min;
  }
  LOG(INFO) << "Validation:  total updates " << total 
            << "; min " << min
            << "; max " << max
            << "; avg value " << total/((double)n)
            << "; std dev " << sqrt(sum_sqr/n - ((total/n)*total/n));
}



BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{

    // allocate array
    GlobalAddress<int64_t> A = Grappa::global_alloc<int64_t>(FLAGS_sizeA);

    do {

      LOG(INFO) << "Starting";
      Grappa::memset(A, 0, FLAGS_sizeA);

      Grappa::Metrics::reset_all_cores();

      double start = Grappa::walltime();

      // best with loop threshold 1024
      // shared pool size 2^16
      Grappa::forall<unbound>( 0, FLAGS_iterations-1, [A] ( int64_t i ) {
          uint64_t b = (i * LARGE_PRIME) % FLAGS_sizeA;
          Grappa::delegate::increment<async>( A + b, 1 );
        } );

      double end = Grappa::walltime();

      Grappa::Metrics::start_tracing();
    
      gups_runtime = end - start;
      gups_throughput = FLAGS_iterations / (end - start);
      gups_throughput_per_locale = gups_throughput / Grappa::locales();
      
      Grappa::Metrics::stop_tracing();
      Grappa::Metrics::merge_and_print();

      if( FLAGS_validate ) {
        LOG(INFO) << "Validating....";
        validate(A, FLAGS_sizeA);
      }

     } while (FLAGS_repeats-- > 1);

    LOG(INFO) << "Done. ";
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
