
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
#include "GlobalTaskJoiner.hpp"
#include "Array.hpp"

#include <boost/test/unit_test.hpp>

DEFINE_int64( repeats, 1, "Repeats" );
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
static GlobalAddress<int64_t> Array;
static int64_t * base;
LOOP_FUNCTOR( func_gups, index, ((GlobalAddress<int64_t>, _Array)) ) {
  Array = _Array;
  base = Array.localize();
}

void func_gups_x(int64_t * p) {
  const uint64_t LARGE_PRIME = 18446744073709551557UL;
  /* across all nodes and all calls, each instance of index in [0.. iterations)
  ** must be encounted exactly once */
  //uint64_t index = random() + Grappa_mynode();//(p - base) * Grappa_nodes() + Grappa_mynode();
  //uint64_t b = (index*LARGE_PRIME) % FLAGS_sizeA;
  static uint64_t index = 1;
  uint64_t b = ((Grappa_mynode() + index++) *LARGE_PRIME) & 1023;
  //fprintf(stderr, "%d ", b);
  ff_delegate_add( Array + b, (const int64_t &) 1 );
}
void validate(GlobalAddress<uint64_t> A, size_t n) {
  int total = 0, max = 0, min = INT_MAX;
  double sum_sqr = 0.0;
  for (int i = 0; i < n; i++) {
    int tmp = Grappa_delegate_read_word(A+i);
    total += tmp;
    sum_sqr += tmp*tmp;
    max = tmp > max ? tmp : max;
    min = tmp < min ? tmp : min;
  }
  fprintf(stderr, "Validation:  total updates %d; min %d; max %d; avg value %g; std dev %g\n",
	  total, min, max, total/((double)n), sqrt(sum_sqr/n - ((total/n)*total/n)));
}
void user_main( int * args ) {

  fprintf(stderr, "Entering user_main\n");
  func_start_profiling start_profiling;
  func_stop_profiling stop_profiling;

  GlobalAddress<int64_t> A = Grappa_typed_malloc<int64_t>(FLAGS_sizeA);

  fprintf(stderr, "user_main: Allocated global array of %d integers\n", FLAGS_sizeA);
  fprintf(stderr, "user_main: Will run %d iterations\n", FLAGS_iterations);

  func_gups gups( A );

    double runtime = 0.0;
    double throughput = 0.0;
    int nnodes = atoi(getenv("SLURM_NNODES"));
    double throughput_per_node = 0.0;

    Grappa_add_profiling_value( &runtime, "runtime", "s", false, 0.0 );
    Grappa_add_profiling_integer( &FLAGS_iterations, "iterations", "it", false, 0 );
    Grappa_add_profiling_integer( &FLAGS_sizeA, "sizeA", "entries", false, 0 );
    Grappa_add_profiling_value( &throughput, "updates_per_s", "up/s", false, 0.0 );
    Grappa_add_profiling_value( &throughput_per_node, "updates_per_s_per_node", "up/s", false, 0.0 );

  do {

    Grappa_memset_local(A, 0, FLAGS_sizeA);
   
    fork_join_custom( &start_profiling );

  
    double start = wall_clock_time();
    fork_join_custom( &gups );
    printf ("Yeahoow!\n");
    forall_local <int64_t, func_gups_x> (A, FLAGS_iterations);
    double end = wall_clock_time();

    fork_join_custom( &stop_profiling );

    runtime = end - start;
    throughput = FLAGS_iterations / runtime;

    throughput_per_node = throughput/nnodes;

    validate(A, FLAGS_sizeA);

    Grappa_merge_and_dump_stats();

    LOG(INFO) << "GUPS: "
            << FLAGS_iterations << " updates at "
            << throughput << "updates/s ("
            << throughput/nnodes << " updates/s/node).";
   } while (FLAGS_repeats-- > 1);
}


BOOST_AUTO_TEST_CASE( test1 ) {
    Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
		  &(boost::unit_test::framework::master_test_suite().argv) );
    Grappa_activate();

    Grappa_run_user_main( &user_main, (int*)NULL );

    Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
