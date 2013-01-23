
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>
#include "Delegate.hpp"

BOOST_AUTO_TEST_SUITE( New_delegate_tests );

using namespace Grappa;

void user_main(void * args) {
  bool done = false;
 
  int64_t seed = 111;
  GlobalAddress<int64_t> seed_addr = make_global(&seed);

  ConditionVariable waiter;
  auto waiter_addr = make_global(&waiter);
  
  send_message(1, [seed_addr, waiter_addr] {
    // on node 1
    privateTask([seed_addr, waiter_addr] {
      int64_t vseed = delegate::read(seed_addr);

      BOOST_CHECK_EQUAL(111, vseed);
      
      delegate::write(seed_addr, 222);
      signal(waiter_addr);
    });
  });
  Grappa::wait(&waiter);
  BOOST_CHECK_EQUAL(seed, 222);
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
