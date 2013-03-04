
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "Message.hpp"
#include "MessagePool.hpp"
#include "CompletionEvent.hpp"
#include "ConditionVariable.hpp"
#include "Delegate.hpp"
#include "Tasking.hpp"
#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"

BOOST_AUTO_TEST_SUITE( Scratch_tests );

using namespace Grappa;

struct Foo {
  int x;
  Foo(): x(57) {}
  void foo() {
    VLOG(1) << "x is " << x;    
  }
  void foo_all() {
    call_on_all_cores([this]{ foo(); });
  }
};

static Foo my_foo;

void user_main(void * args) {
  CHECK(Grappa::cores() >= 2); // at least 2 nodes for these tests...
  
  my_foo.foo_all();
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
	       &(boost::unit_test::framework::master_test_suite().argv));
  Grappa_activate();
  Grappa_run_user_main(&user_main, (void*)NULL);
  Grappa_finish(0);
}

BOOST_AUTO_TEST_SUITE_END();
