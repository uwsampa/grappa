
#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "DictOut.hpp"

#define BILLION 1000000000

BOOST_AUTO_TEST_SUITE( Scheduler_benchmarking_tests );

int64_t warmup_iters = 1<<18;
int64_t iters = 1<<24;

#include <sys/time.h>
double wctime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

void user_main( void* args ) 
{

  double start, end;
  
  for (int64_t i=0; i<warmup_iters; i++) {
    SoftXMT_yield();
  }

  start = wctime();
  for (int64_t i=0; i<iters; i++) {
    SoftXMT_yield();
  }
  end = wctime();

  double runtime = end - start;

  DictOut d;
  d.add( "iterations", iters );
  d.add( "runtime", runtime ); 
  BOOST_MESSAGE( d.toString() );

  BOOST_MESSAGE( (runtime / iters) * BILLION << " ns / switch" );

  BOOST_MESSAGE( "user main is exiting" );
}

BOOST_AUTO_TEST_CASE( benchmark_test1) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();

  DVLOG(1) << "Spawning user main Thread....";
  SoftXMT_run_user_main( &user_main, (void*)NULL );
  VLOG(5) << "run_user_main returned";
  CHECK( SoftXMT_done() );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

