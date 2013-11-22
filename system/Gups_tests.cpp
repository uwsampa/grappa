
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

/// One implementation of GUPS. This does no load-balancing, and may
/// suffer from some load imbalance.

#include <memory>
#include <algorithm>

#include <Grappa.hpp>
#include "GlobalAllocator.hpp"
#include "Array.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"
#include "Statistics.hpp"
#include "Collective.hpp"

#include "LocaleSharedMemory.hpp"

#include "ParallelLoop.hpp"
#include "AsyncDelegate.hpp"


#include <boost/test/unit_test.hpp>

DEFINE_int64( repeats, 1, "Repeats" );
DEFINE_int64( iterations, 1 << 30, "Iterations" );
DEFINE_int64( sizeA, 1024, "Size of array that gups increments" );
DEFINE_bool( validate, true, "Validate result" );

DECLARE_string( load_balance );

GRAPPA_DEFINE_STAT( SimpleStatistic<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<double>, gups_throughput, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<double>, gups_throughput_per_locale, 0 );

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



void user_main( int * args ) {

  // allocate array
  GlobalAddress<int64_t> A = Grappa::global_alloc<int64_t>(FLAGS_sizeA);

  do {

    LOG(INFO) << "Starting";
    Grappa::memset(A, 0, FLAGS_sizeA);

    Grappa::Statistics::reset_all_cores();

    double start = Grappa::walltime();

    // best with loop threshold 1024
    // shared pool size 2^16
    Grappa::forall_global_public( 0, FLAGS_iterations-1, [A] ( int64_t i ) {
        uint64_t b = (i * LARGE_PRIME) % FLAGS_sizeA;
        Grappa::delegate::increment_async( A + b, 1 );
      } );

    double end = Grappa::walltime();

    Grappa::on_all_cores( [] {
        Grappa_stop_profiling();
      } );
    
    gups_runtime = end - start;
    gups_throughput = FLAGS_iterations / (end - start);
    gups_throughput_per_locale = gups_throughput / Grappa::locales();

    Grappa::Statistics::merge_and_print();

    if( FLAGS_validate ) {
      LOG(INFO) << "Validating....";
      validate(A, FLAGS_sizeA);
    }

   } while (FLAGS_repeats-- > 1);

  LOG(INFO) << "Done. ";
}


BOOST_AUTO_TEST_CASE( test1 ) {
    Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
		  &(boost::unit_test::framework::master_test_suite().argv) );
    Grappa_activate();

    Grappa_run_user_main( &user_main, (int*)NULL );

    Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
