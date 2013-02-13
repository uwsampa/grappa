
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
#include "ParallelLoop.hpp"

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

GlobalCompletionEvent gce;

void try_global_ce() {
  BOOST_MESSAGE("GlobalCompletionEvent:");
  
  const int64_t N = 128;
  int64_t x = 0;
  auto xa = make_global(&x);
  
  BOOST_MESSAGE("  block on user_main only");
  on_all_cores([xa]{
    Core origin = mycore();
    gce.enroll(N+1);
    for (int i=0; i<N; i++) {
      publicTask([xa,origin]{
        delegate::fetch_and_add(xa, 1);
        complete(make_global(&gce,origin));
      });
    }
    
    gce.complete();
  });
  
  gce.wait();
  BOOST_CHECK_EQUAL(x, N*cores());
  
  
  BOOST_MESSAGE("  block in SPMD tasks");
  on_all_cores([]{  gce.reset();  });
  
  x = 0;
  on_all_cores([xa]{
    gce.reset();
    int y = 0;
    auto ya = make_global(&y);
    
    Core origin = mycore();
    gce.enroll(N);
    for (int i=0; i<N; i++) {
      publicTask([xa,ya,origin]{
        delegate::fetch_and_add(xa, 1);
        delegate::fetch_and_add(ya, 1);
        complete(make_global(&gce,origin));
      });
    }
    gce.wait();
    BOOST_CHECK_EQUAL(y, N);
  });
  BOOST_CHECK_EQUAL(x, N*cores());
}

void rec_spawn(GlobalAddress<int64_t> xa, int N) {
  Core origin = mycore();
  for (int i=0; i<N/2+1; i++) {
    gce.enroll();
    publicTask([xa,origin]{
      delegate::fetch_and_add(xa, 1);
      complete(make_global(&gce,origin));
    });
  }
  if (N>1) rec_spawn(xa, N-N/2-1);
}

void try_global_ce_recursive() {
  BOOST_MESSAGE("GlobalCompletionEvent (recursive spawns):");
  
  const int64_t N = 128;
  int64_t x = 0;
  auto xa = make_global(&x);
  
  BOOST_MESSAGE("  block on user_main only");
  on_all_cores([]{  gce.reset();  });
  
  on_all_cores([xa]{
    gce.enroll();
    Core origin = mycore();
    for (int i=0; i<N; i++) {
      gce.enroll();
      publicTask([xa,origin]{
        delegate::fetch_and_add(xa, 1);
        complete(make_global(&gce,origin));
      });
    }
    
    gce.complete();
  });
  
  // overload Core0 with extra work
  rec_spawn(xa, N*2);
  
  gce.wait();
  BOOST_CHECK_EQUAL(x, N*cores()+N*2);
  
  
  BOOST_MESSAGE("  block in SPMD tasks");
  on_all_cores([]{  gce.reset();  });
  
  x = 0;
  on_all_cores([xa]{
    gce.reset();
    int y = 0;
    auto ya = make_global(&y);
    
    Core origin = mycore();
    gce.enroll(N);
    for (int i=0; i<N; i++) {
      publicTask([xa,ya,origin]{
        delegate::fetch_and_add(xa, 1);
        delegate::fetch_and_add(ya, 1);
        complete(make_global(&gce,origin));
      });
    }
    
    if (mycore() == 0) {
      // overload Core0 with extra work
      rec_spawn(xa, N*2);
    }
    
    gce.wait();
    BOOST_CHECK_EQUAL(y, N);
  });
  BOOST_CHECK_EQUAL(x, N*cores()+N*2);
}


void user_main(void * args) {
  CHECK(cores() >= 2); // at least 2 nodes for these tests...

  try_no_tasks();
  try_one_task();
  try_many_tasks();
  try_global_ce();
  try_global_ce_recursive();
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
