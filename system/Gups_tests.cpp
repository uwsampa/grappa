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
      Grappa::forall<Grappa::unbound>( 0, FLAGS_iterations-1, [A] ( int64_t i ) {
          uint64_t b = (i * LARGE_PRIME) % FLAGS_sizeA;
          Grappa::delegate::increment<Grappa::async>( A + b, 1 );
        } );

      double end = Grappa::walltime();

      Grappa::Metrics::start_tracing();
    
      gups_runtime = end - start;
      gups_throughput = FLAGS_iterations / (end - start);
      gups_throughput_per_locale = gups_throughput / Grappa::locales();
      
      Grappa::Metrics::stop_tracing();
      //Grappa::Metrics::merge_and_print();
      LOG(INFO) << gups_runtime;
      LOG(INFO) << gups_throughput;
      LOG(INFO) << gups_throughput_per_locale;
      
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
