
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "ParallelLoop.hpp"

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Message.hpp"
#include "MessagePool.hpp"
#include "CompletionEvent.hpp"
#include "ConditionVariable.hpp"
#include "Delegate.hpp"
#include "Tasking.hpp"
#include "GlobalAllocator.hpp"

BOOST_AUTO_TEST_SUITE( New_loop_tests );

using namespace Grappa;
using Grappa::wait;

static bool touched = false;

void test_on_all_cores() {
  BOOST_MESSAGE("Testing on_all_cores...");
  
  on_all_cores([] {
    BOOST_MESSAGE("hello world from " << mycore() << "!");
    touched = true;
  });
  
  BOOST_CHECK_EQUAL(delegate::read(make_global(&touched,1)), true);
  BOOST_CHECK_EQUAL(touched, true);
}

void test_loop_decomposition() {
  int N = 16;
  
  CompletionEvent ce(N);
  
  impl::loop_decomposition_private<2>(0, N, [&ce](int64_t start, int64_t iters) {
    VLOG(1) << "loop(" << start << ", " << iters << ")";
    ce.complete(iters);
  });
  ce.wait();
}

void test_loop_decomposition_global() {
  int N = 16;
  
  CompletionEvent ce(N);
  auto ce_addr = make_global(&ce);
  
  impl::loop_decomposition_public(0, N, [ce_addr](int64_t start, int64_t iters) {
    VLOG(1) << "loop(" << start << ", " << iters << ")";
    complete(ce_addr,iters);
  });
  ce.wait();
}

CompletionEvent my_ce;

void test_forall_here() {
  BOOST_MESSAGE("Testing forall_here...");
  const int N = 15;
  
  {
    int x = 0;
    forall_here(0, N, [&x](int64_t start, int64_t iters) {
      CHECK(mycore() == 0);
      for (int64_t i=0; i<iters; i++) {
        x++;
      }
    });
    BOOST_CHECK_EQUAL(x, N);
  }
  
  {
    int x = 0;
    forall_here<&my_ce,2>(0, N, [&x](int64_t start, int64_t iters) {
      CHECK(mycore() == 0);
      for (int64_t i=0; i<iters; i++) {
        x++;
      }
    });
    BOOST_CHECK_EQUAL(x, N);
  }
  
}

static int test_global = 0;
CompletionEvent test_global_ce;

void test_forall_global() {
  BOOST_MESSAGE("Testing forall_global...");
  const int64_t N = 1 << 8;
  
  forall_global_nosteal(0, N, [](int64_t start, int64_t iters) {
    for (int i=0; i<iters; i++) {
      test_global++;
    }
  });
  
  on_all_cores([]{
    BOOST_CHECK_EQUAL(test_global, N/cores());
    test_global = 0;
  });
  
  forall_global_nosteal<&test_global_ce>(0, N, [](int64_t start, int64_t iters) {
    for (int i=0; i<iters; i++) {
      test_global++;
    }
  });
  
}


void test_forall_localized() {
  BOOST_MESSAGE("Testing forall_localized...");
  const int64_t N = 1 << 8;
  
  auto array = Grappa_typed_malloc<int64_t>(N);
  
  forall_localized(array, N, [](int64_t i, int64_t& e){
    e = 1;
  });
  
  forall_here(0, N, [array](int64_t s, int64_t n) {
    for (int i=s; i<s+n; i++) { BOOST_CHECK_EQUAL(delegate::read(array+i), 1); }
  });
}

void user_main(void * args) {
  CHECK(Grappa::cores() >= 2); // at least 2 nodes for these tests...

  test_on_all_cores();
  
  test_loop_decomposition();
  test_loop_decomposition_global();
  
  test_forall_here();
  test_forall_global();
  
  test_forall_localized();
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
