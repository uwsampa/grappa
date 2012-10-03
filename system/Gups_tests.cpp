
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

/// One implementation of GUPS. This does no load-balancing, and may
/// suffer from some load imbalance.

#include <Grappa.hpp>
#include "ForkJoin.hpp"
#include "GlobalAllocator.hpp"

#include <boost/test/unit_test.hpp>

DEFINE_int64( iterations, 1 << 30, "Iterations" );
DEFINE_int64( sizeA, 1000, "Size of array that gups increments" );


double wall_clock_time() {
  const double nano = 1.0e-9;
  timespec t;
  clock_gettime( CLOCK_MONOTONIC, &t );
  return nano * t.tv_nsec + t.tv_sec;
}

BOOST_AUTO_TEST_SUITE( Gups_tests );

LOOP_FUNCTION( func_start_profiling, index ) {
  Grappa_start_profiling();
}

LOOP_FUNCTION( func_stop_profiling, index ) {
  Grappa_stop_profiling();
}

/// Functor to execute one GUP.
LOOP_FUNCTOR( func_gups, index, ((GlobalAddress<int64_t>, Array)) ) {
  const uint64_t LARGE_PRIME = 18446744073709551557UL;
  uint64_t b = (index*LARGE_PRIME) % FLAGS_sizeA;
  Grappa_delegate_fetch_and_add_word( Array + b, 1 );
}

void user_main( int * args ) {

  func_start_profiling start_profiling;
  func_stop_profiling stop_profiling;

  GlobalAddress<int64_t> A = Grappa_typed_malloc<int64_t>(FLAGS_sizeA);

  func_gups gups( A );

  fork_join_custom( &start_profiling );

  double start = wall_clock_time();
  fork_join( &gups, 1, FLAGS_iterations );
  double end = wall_clock_time();

  fork_join_custom( &stop_profiling );

<<<<<<< HEAD
  Grappa_merge_and_dump_stats();

=======
>>>>>>> change stats to dump a single JSON blob
  double runtime = end - start;
  double throughput = FLAGS_iterations / runtime;

  int nnodes = atoi(getenv("SLURM_NNODES"));
  double throughput_per_node = throughput/nnodes;

  SoftXMT_add_profiling_value( &runtime, "runtime", "s", false, 0.0 );
  SoftXMT_add_profiling_integer( &FLAGS_iterations, "iterations", "it", false, 0 );
  SoftXMT_add_profiling_integer( &FLAGS_sizeA, "sizeA", "entries", false, 0 );
  SoftXMT_add_profiling_value( &throughput, "updates_per_s", "up/s", false, 0.0 );
  SoftXMT_add_profiling_value( &throughput_per_node, "updates_per_s_per_node", "up/s", false, 0.0 );

  SoftXMT_merge_and_dump_stats();

  LOG(INFO) << "GUPS: "
            << FLAGS_iterations << " updates at "
            << throughput << "updates/s ("
            << throughput/nnodes << " updates/s/node).";
}


BOOST_AUTO_TEST_CASE( test1 ) {
    Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
		  &(boost::unit_test::framework::master_test_suite().argv) );
    Grappa_activate();

    Grappa_run_user_main( &user_main, (int*)NULL );

    Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
