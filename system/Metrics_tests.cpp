
#include <iostream>
#include <glog/logging.h>
#include <boost/test/unit_test.hpp>
#include "Metrics.hpp"

BOOST_AUTO_TEST_SUITE( Metrics_tests );

BOOST_AUTO_TEST_CASE( test1 ) {
  
  google::ParseCommandLineFlags( &(boost::unit_test::framework::master_test_suite().argc),
                                 &(boost::unit_test::framework::master_test_suite().argv),
                                 true );
  google::InitGoogleLogging( boost::unit_test::framework::master_test_suite().argv[0] );
  google::InstallFailureSignalHandler( );
  
  int i = 0;
  char j[] = "foo";
  double k = 1.234;
  
#define METRICS( METRIC )                       \
  METRIC( "an integer", i )                     \
    METRIC( "a string", j )                     \
    METRIC( "a double", k )
    
  
  BOOST_MESSAGE( "std::cout output" );
  std::cout << STREAMIFY_HUMAN_METRICS( METRICS ) << std::endl;
  std::cout << STREAMIFY_CSV_METRICS( METRICS ) << std::endl;


  BOOST_MESSAGE( "Boost message output" );
  BOOST_MESSAGE( STREAMIFY_HUMAN_METRICS( METRICS ) );
  BOOST_MESSAGE( STREAMIFY_CSV_METRICS( METRICS ) );

  LOG(INFO) << "Google log output";
  LOG(INFO) << STREAMIFY_HUMAN_METRICS( METRICS );
  LOG(INFO) << STREAMIFY_CSV_METRICS( METRICS );

}

BOOST_AUTO_TEST_SUITE_END();

