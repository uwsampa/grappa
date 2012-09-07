#include <SoftXMT.hpp>
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
  SoftXMT_start_profiling();
}

LOOP_FUNCTION( func_stop_profiling, index ) {
  SoftXMT_stop_profiling();
}

LOOP_FUNCTOR( func_gups, index, ((GlobalAddress<int64_t>, Array)) ) {
  const uint64_t LARGE_PRIME = 18446744073709551557UL;
  uint64_t b = (index*LARGE_PRIME) % FLAGS_sizeA;
  SoftXMT_delegate_fetch_and_add_word( Array + b, 1 );
}

void user_main( int * args ) {

  func_start_profiling start_profiling;
  func_stop_profiling stop_profiling;

  GlobalAddress<int64_t> A = SoftXMT_typed_malloc<int64_t>(FLAGS_sizeA);

  func_gups gups( A );

  fork_join_custom( &start_profiling );

  double start = wall_clock_time();
  fork_join( &gups, 1, FLAGS_iterations );
  double end = wall_clock_time();

  fork_join_custom( &stop_profiling );

  SoftXMT_merge_and_dump_stats();

  double runtime = end - start;
  double throughput = FLAGS_iterations / runtime;

  int nnodes = atoi(getenv("SLURM_NNODES"));

  LOG(INFO) << "GUPS: "
            << FLAGS_iterations << " updates at "
            << throughput << "updates/s ("
            << throughput/nnodes << " updates/s/node).";

  std::cout << "GUPS { "
	    << "runtime:" << runtime << ", "
            << "iterations:" << FLAGS_iterations << ", "
            << "sizeA:" << FLAGS_sizeA << ", "
            << "updates_per_s:" << throughput << ", "
            << "updates_per_s_per_node:" << throughput/nnodes << ", "
	    << "}" << std::endl;
}


BOOST_AUTO_TEST_CASE( test1 ) {
    SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
		  &(boost::unit_test::framework::master_test_suite().argv) );
    SoftXMT_activate();

    SoftXMT_run_user_main( &user_main, (int*)NULL );

    SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
