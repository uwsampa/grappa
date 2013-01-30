
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "Statistics.hpp"

BOOST_AUTO_TEST_SUITE( Statistics_tests );

using namespace Grappa;

//equivalent to: static Statistic<int> foo("foo", 0);
GRAPPA_DEFINE_STAT(foo, int, 0);

//equivalent to: static Statistic<double> bar("bar", 0);
GRAPPA_DEFINE_STAT(bar, double, 0);

void user_main(void * args) {
  CHECK(Grappa_nodes() >= 2); // at least 2 nodes for these tests...

  foo++;
  bar = 3.14;
  
  PrintStatistics();
}

BOOST_AUTO_TEST_CASE( test1 ) {
  
  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
              &(boost::unit_test::framework::master_test_suite().argv)
              );
  
  Grappa_activate();
  
  Grappa_run_user_main( &user_main, (void*)NULL );
  
  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
