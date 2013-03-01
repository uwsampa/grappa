
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>
#include "Delegate.hpp"
#include "AsyncDelegate.hpp"
#include "GlobalCompletionEvent.hpp"
#include "MessagePool.hpp"
#include "Collective.hpp"

BOOST_AUTO_TEST_SUITE( New_delegate_tests );

using namespace Grappa;
using Grappa::wait;

void check_short_circuiting() {
  BOOST_MESSAGE("check_short_circuiting");
  // read
  int a = 0;
  auto ga = make_global(&a);
  BOOST_CHECK_EQUAL(delegate::read(ga), 0);
  
  // write
  BOOST_CHECK_EQUAL(a, 0); // value unchanged
  BOOST_CHECK_EQUAL(delegate::write(ga, 7), true);
  BOOST_CHECK_EQUAL(a, 7);
  
  // compare and swap
  double b = 3.14;
  auto gb = make_global(&b);
  BOOST_CHECK_EQUAL(delegate::compare_and_swap(gb, 3.14, 2.0), true);
  BOOST_CHECK_EQUAL(b, 2.0);
  BOOST_CHECK_EQUAL(delegate::compare_and_swap(gb, 3.14, 3.0), false);
  BOOST_CHECK_EQUAL(b, 2.0);
  
  // fetch and add
  uint64_t c = 1;
  auto gc = make_global(&c);
  BOOST_CHECK_EQUAL(delegate::fetch_and_add(gc, 1), 1);
  BOOST_CHECK_EQUAL(c, 2);
  BOOST_CHECK_EQUAL(delegate::fetch_and_add(gc, -2), 2);
  BOOST_CHECK_EQUAL(c, 0);
}

void check_remote() {
  BOOST_MESSAGE("check_remote");
  // read
  int a = 0;
  auto ga = make_global(&a);
  
  Grappa::ConditionVariable w;
  auto gw = make_global(&w);
  
  send_message(1, [ga, gw] {
    privateTask([=]{
      BOOST_CHECK_EQUAL(delegate::read(ga), 0);
      signal(gw);
    });
  });
  wait(&w);
  BOOST_CHECK_EQUAL(a, 0); // value unchanged
  
  // write
  send_message(1, [ga, gw] {
    privateTask([=]{
      BOOST_CHECK_EQUAL(delegate::write(ga, 7), true);
      signal(gw);
    });
  });
  wait(&w);
  BOOST_CHECK_EQUAL(a, 7);
  
  // compare and swap
  double b = 3.14;
  auto gb = make_global(&b);
  send_message(1, [gb, gw] {
    privateTask([=]{
      BOOST_CHECK_EQUAL(delegate::compare_and_swap(gb, 3.14, 2.0), true);
      signal(gw);
    });
  });
  wait(&w);
  BOOST_CHECK_EQUAL(b, 2.0);
  send_message(1, [gb, gw] {
    privateTask([=]{
      BOOST_CHECK_EQUAL(delegate::compare_and_swap(gb, 3.14, 3.0), false);
      signal(gw);
    });
  });
  wait(&w);
  BOOST_CHECK_EQUAL(b, 2.0);
  
  // fetch and add
  uint64_t c = 1;
  auto gc = make_global(&c);
  send_message(1, [gc, gw] {
    privateTask([=]{
      BOOST_CHECK_EQUAL(delegate::fetch_and_add(gc, 1), 1);
      signal(gw);
    });
  });
  wait(&w);
  BOOST_CHECK_EQUAL(c, 2);
  send_message(1, [gc, gw] {
    privateTask([=]{
      BOOST_CHECK_EQUAL(delegate::fetch_and_add(gc, -2), 2);
      signal(gw);
    });
  });
  wait(&w);
  BOOST_CHECK_EQUAL(c, 0);
}

GlobalCompletionEvent mygce;
int global_x;
int global_y;

void check_async_delegates() {
  BOOST_MESSAGE("check_async_delegates");
  BOOST_MESSAGE("  feed forward...");
  const int N = 1 << 8;
  
  MessagePool pool(N*(1<<8));

  delegate::write(make_global(&global_x,1), 0);
  
  mygce.reset_all();
  for (int i=0; i<N; i++) {
    delegate::call_async<&mygce>(pool, 1, []{ global_x++; });
  }
  mygce.wait();
  
  BOOST_CHECK_EQUAL(delegate::read(make_global(&global_x,1)), N);
  
  delegate::call(1, []{
    global_x = global_y = 0;
  });
  
  BOOST_MESSAGE("  promises...");
  delegate::Promise<int> a[N];
  for (int i=0; i<N; i++) {
    a[i].call_async(pool, 1, [i]()->int {
      global_y++;
      return global_x+i;
    });
  }
  for (int i=0; i<N; i++) {
    BOOST_CHECK_EQUAL(a[i].get(), N+i);
  }
  BOOST_CHECK_EQUAL(delegate::read(make_global(&global_y,1)), N);
}

void user_main(void * args) {
  CHECK(Grappa_nodes() >= 2); // at least 2 nodes for these tests...

  check_short_circuiting();
  
  check_remote();
  
  check_async_delegates();
 
  int64_t seed = 111;
  GlobalAddress<int64_t> seed_addr = make_global(&seed);

  Grappa::ConditionVariable waiter;
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
  
  Grappa::Statistics::merge_and_print();
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
