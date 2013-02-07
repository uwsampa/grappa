
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Message.hpp"
#include "MessagePool.hpp"
#include "CountConditionVariable.hpp"
#include "ConditionVariable.hpp"
#include "Delegate.hpp"

#include "ParallelLoop.hpp"

BOOST_AUTO_TEST_SUITE( New_loop_tests );

using namespace Grappa;
using Grappa::wait;

static bool touched = false;

void user_main(void * args) {
  CHECK(Grappa::cores() >= 2); // at least 2 nodes for these tests...

  BOOST_MESSAGE("Testing SIMD Spawn...");
  
  forall_cores([] {
    BOOST_MESSAGE("hello world from " << mycore() << "!");
    touched = true;
  });
  
  BOOST_CHECK_EQUAL(delegate::read(make_global(&touched,1)), true);
  BOOST_CHECK_EQUAL(touched, true);
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
