
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <iostream>
#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "Delegate.hpp"
#include "CompletionEvent.hpp"

BOOST_AUTO_TEST_SUITE( CompletionEvent_tests );

using namespace Grappa;

void try_no_tasks() {
  BOOST_MESSAGE("no tasks...");
  int foo = 0;
  CompletionEvent ce;
  ce.wait();
  BOOST_CHECK_EQUAL(foo, 0);
}

void try_one_task() {
  BOOST_MESSAGE("one task...");
  CompletionEvent ce;
  
  int foo = 0;
  
  privateTask(&ce, [&foo] {
    foo++;
  });
  
  ce.wait();
  BOOST_CHECK_EQUAL(foo, 1);
}

void try_many_tasks() {
  BOOST_MESSAGE("many tasks...");
  CompletionEvent ce;
  
  int N = 1024;
  int foo = 0;

  for (int i=0; i<N; i++) {
    privateTask(&ce, [&foo] {
      foo++;
    });
  }
  
  ce.wait();
  BOOST_CHECK_EQUAL(foo, N);
}

void user_main(void * args) {
  CHECK(cores() >= 2); // at least 2 nodes for these tests...

  try_no_tasks();
  try_one_task();
  try_many_tasks();
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
