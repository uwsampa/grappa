
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "Message.hpp"
#include "FullEmpty.hpp"
#include "Message.hpp"
#include "ConditionVariable.hpp"
#include "tasks/TaskingScheduler.hpp"

BOOST_AUTO_TEST_SUITE( New_delegate_tests );

using namespace Grappa;

int64_t delegate_read(GlobalAddress<int64_t> target) {
  FullEmpty<int64_t> result;
  auto result_addr = &result;
  Node origin = Grappa_mynode();
  
  VLOG(1) << "issuer Worker*: " << global_scheduler.get_current_thread();
  
  {
    send_message(target.node(), [=]() mutable {
      CHECK(target.node() == Grappa_mynode());
      int64_t val = *target.pointer();
      VLOG(1) << "val = " << val << "\n";
      
      send_message(origin, [=]{
        VLOG(1) << "val = " << val << " (back on origin)\n";
        result_addr->writeEF(val);
      });
    });
  }
  
  int64_t r = result.readFE();
  VLOG(1) << "read full: " << r;
  return r;
}

void user_main(void * args) {
  bool done = false;
 
  int64_t seed = 12345;
  GlobalAddress<int64_t> seed_addr = make_global(&seed);

  ConditionVariable waiter;
  auto waiter_addr = make_global(&waiter);
  
  send_message(1, [=]{
    // on node 1
    privateTask([=]{
      int64_t vseed = delegate_read(seed_addr);

      VLOG(1) << "delegate_read(seed) = " << vseed << "\n";
      signal(waiter_addr);
    });
  });
  Grappa::wait(&waiter);
  VLOG(1) << "done waiting\n";
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
