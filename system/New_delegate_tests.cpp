
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

template <typename F>
inline auto delegate_call(Core dest, F func) -> decltype(func()) {
  using R = decltype(func());
  FullEmpty<R> result;
  Node origin = Grappa_mynode();
  
  send_message(dest, [&result, origin, func] {
    R val = func();
    VLOG(3) << "rpc result = " << val << " (at destination)";
    
    send_message(origin, [&result, val] {
      VLOG(3) << "rpc result = " << val << " (back on origin)";
      result.writeXF(val); // can't block in message, assumption is that result is already empty
    });
  }); // send message
  // ... and wait for the result
  R r = result.readFE();
  VLOG(3) << "" << r;
  return r;
}

template< typename T >
T delegate_read(GlobalAddress<T> target) {
  return delegate_call(target.node(), [target]() -> T {
    return *target.pointer();
  });
}

void user_main(void * args) {
  bool done = false;
 
  int64_t seed = 12345;
  GlobalAddress<int64_t> seed_addr = make_global(&seed);

  ConditionVariable waiter;
  auto waiter_addr = make_global(&waiter);
  
  send_message(1, [seed_addr, waiter_addr] {
    // on node 1
    privateTask([seed_addr, waiter_addr] {
      int64_t vseed = delegate_read(seed_addr);

      VLOG(1) << "delegate_read(seed) = " << vseed;
      signal(waiter_addr);
    });
  });
  Grappa::wait(&waiter);
  VLOG(1) << "done waiting";
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
