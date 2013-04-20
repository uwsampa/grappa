
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
#include "GlobalCompletionEvent.hpp"
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
//  gce.reset_all(); (don't need to call `reset` anymore)
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
  
  x = 0;
//  gce.reset_all(); (don't need this anymore)
  on_all_cores([xa]{
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
//  gce.reset_all();
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
  
  x = 0;
//  gce.reset_all();
  on_all_cores([xa]{
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

static int global_x;

void try_synchronizing_spawns() {
  BOOST_MESSAGE("Testing synchronizing spawns");
  const int N = 1<<8;
  
  BOOST_MESSAGE("  private,local");
  CompletionEvent ce;
  int x = 0;
  for (int i=0; i<N; i++) {
    privateTask(&ce, [&x]{
      x++;
    });
  }
  ce.wait();
  BOOST_CHECK_EQUAL(x, N);
  
  BOOST_MESSAGE("  private,global");
  on_all_cores([]{
//    gce.reset();
//    barrier();
    
    int x = 0;
    
    for (int i=0; i<N; i++) {
      privateTask(&gce, [&x] {
        x++;
      });
    }
    
    gce.wait();
    BOOST_CHECK_EQUAL(x, N);
  });
  
  BOOST_MESSAGE("  public,global"); VLOG(1) << "actually in public_global";
  on_all_cores([]{ global_x = 0; });
  
//  gce.reset_all();
  for (int i=0; i<N; i++) {
    publicTask<&gce>([]{
      global_x++;
    });
  }
  gce.wait();
  
  int total = 0;
  auto total_addr = make_global(&total);
  on_all_cores([total_addr]{
    BOOST_MESSAGE("global_x on " << mycore() << ": " << global_x);
    delegate::fetch_and_add(total_addr, global_x);
  });
  BOOST_CHECK_EQUAL(total, N);
}

void user_main(void * args) {
  CHECK(cores() >= 2); // at least 2 nodes for these tests...

  try_no_tasks();
  try_one_task();
  try_many_tasks();
  try_global_ce();
  try_global_ce_recursive();
  try_synchronizing_spawns();
  
  Statistics::merge_and_print();
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
              &(boost::unit_test::framework::master_test_suite().argv));
  Grappa_activate();
  Grappa_run_user_main( &user_main, (void*)NULL );
  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
