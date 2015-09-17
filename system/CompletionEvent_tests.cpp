////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "Delegate.hpp"
#include "CompletionEvent.hpp"
#include "GlobalCompletionEvent.hpp"
#include "ParallelLoop.hpp"

DEFINE_int64( outer, 1 << 4, "iterations of outer loop in iterative GCE test" );
DEFINE_int64( inner, 1 << 14, "iterations of inner loop in iterative GCE test" );

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
  
  spawn(&ce, [&foo] {
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
    spawn(&ce, [&foo] {
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
  gce.enroll(1); // gce.reset_all() is deprecated; make sure at least one thing is enrolled until wait
  gce.enroll(cores());
  on_all_cores([xa]{
    Core origin = mycore();
    gce.enroll(N+1);
    complete(make_global(&gce,0));
    for (int i=0; i<N; i++) {
      spawn<unbound>([xa,origin]{
        delegate::fetch_and_add(xa, 1);
        complete(make_global(&gce,origin));
      });
    }
    
    gce.complete();
  });

  gce.complete();
  gce.wait();
  BOOST_CHECK_EQUAL(x, N*cores());
  
  
  BOOST_MESSAGE("  block in SPMD tasks");
  
  x = 0;
  gce.enroll(1); // gce.reset_all() is deprecated; make sure at least one thing is enrolled until wait

  gce.enroll(cores());

  on_all_cores([xa,N]{
    int y = 0;
    auto ya = make_global(&y);
    
    Core origin = mycore();
    gce.enroll(N);
    complete(make_global(&gce,0));

    for (int i=0; i<N; i++) {
      spawn<unbound>([xa,ya,origin]{
        delegate::fetch_and_add(xa, 1);
        delegate::fetch_and_add(ya, 1);
        complete(make_global(&gce,origin));
      });
    }

    if( mycore() == 0 ) gce.complete();
    gce.wait();
    BOOST_CHECK_EQUAL(y, N);
  });
  BOOST_CHECK_EQUAL(x, N*cores());
}

void rec_spawn(GlobalAddress<int64_t> xa, int N) {
  Core origin = mycore();
  for (int i=0; i<N/2+1; i++) {
    gce.enroll();
    spawn<unbound>([xa,origin]{
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
  gce.enroll(1); // gce.reset_all() is deprecated; make sure at least one thing is enrolled until wait
  gce.enroll(cores());

  on_all_cores([xa]{
    gce.enroll();
    complete(make_global(&gce,0));

    Core origin = mycore();
    for (int i=0; i<N; i++) {
      gce.enroll();
      spawn<unbound>([xa,origin]{
        delegate::fetch_and_add(xa, 1);
        complete(make_global(&gce,origin));
      });
    }
    
    gce.complete();
  });

  // overload Core0 with extra work
  rec_spawn(xa, N*2);

  gce.complete();
  gce.wait();
  BOOST_CHECK_EQUAL(x, N*cores()+N*2);
  
  BOOST_MESSAGE("  block in SPMD tasks");
  
  x = 0;
  gce.enroll(1); // gce.reset_all() is deprecated; make sure at least one thing is enrolled until wait
  gce.enroll(cores());

  on_all_cores([xa,N]{
    int y = 0;
    auto ya = make_global(&y);
    
    Core origin = mycore();
    gce.enroll(N);
    complete(make_global(&gce,0));

    for (int i=0; i<N; i++) {
      spawn<unbound>([xa,ya,origin]{
        delegate::fetch_and_add(xa, 1);
        delegate::fetch_and_add(ya, 1);
        complete(make_global(&gce,origin));
      });
    }
    
    if (mycore() == 0) {
      // overload Core0 with extra work
      rec_spawn(xa, N*2);
      gce.complete(); // complete last one.
    }

    gce.wait();
    BOOST_CHECK_EQUAL(y, N);
  });
  BOOST_CHECK_EQUAL(x, N*cores()+N*2);
  
  BOOST_MESSAGE("test finish block syntactic sugar");
    
  long xx = 0;
  auto a = make_global(&xx);
  
  finish([=]{
    forall<unbound,async>(0, N, [=](int64_t i){
      delegate::increment<async>(a, 1);
    });
  });

  BOOST_CHECK_EQUAL(xx, N);
}

static int global_x;

void try_synchronizing_spawns() {
  BOOST_MESSAGE("Testing synchronizing spawns");
  const int N = 1<<8;
  
  BOOST_MESSAGE("  private,local");
  CompletionEvent ce;
  int x = 0;
  for (int i=0; i<N; i++) {
    spawn(&ce, [&x]{
      x++;
    });
  }
  ce.wait();
  BOOST_CHECK_EQUAL(x, N);
  
  BOOST_MESSAGE("  private,global");
  gce.enroll(1); // gce.reset_all() is deprecated; make sure at least one thing is enrolled until wait
  on_all_cores([N]{
    int x = 0;
    
    for (int i=0; i<N; i++) {
      spawn<&gce>([&x]{
        x++;
      });
    }
    
    if( mycore() == 0 ) gce.complete();
    gce.wait();
    BOOST_CHECK_EQUAL(x, N);
  });
  
  BOOST_MESSAGE("  public,global"); VLOG(1) << "actually in public_global";
  on_all_cores([]{ global_x = 0; });
  
  gce.enroll(1); // gce.reset_all() is deprecated; make sure at least one thing is enrolled until wait
  for (int i=0; i<N; i++) {
    spawn<unbound,&gce>([]{
      global_x++;
    });
  }
  gce.complete();
  gce.wait();
  
  int total = 0;
  auto total_addr = make_global(&total);
  on_all_cores([total_addr]{
    BOOST_MESSAGE("global_x on " << mycore() << ": " << global_x);
    delegate::fetch_and_add(total_addr, global_x);
  });
  BOOST_CHECK_EQUAL(total, N);
}

void try_iterative_spmd_gce() {
  static int val, stolen;

  // set up re-enrolling across iterations
  auto completion_target = local_gce.enroll_recurring(cores());
  
  on_all_cores( [completion_target] {
    for( int i = 0; i < FLAGS_outer; ++i ) {

      if( mycore() == 0 ) {
        val = 0;
        stolen = 0;
      }

      for( int j = 0; j < FLAGS_inner; ++j ) {
        auto message_completion_target = local_gce.enroll();

        spawn<unbound>( [j,message_completion_target] {
          if( message_completion_target.core != mycore() ) stolen++;
          send_heap_message( j % cores(), [message_completion_target] {
            val++;
            local_gce.complete( message_completion_target );
          });
        });
      }

      // wait for everyone to finish and prepare for next iteration.
      local_gce.complete( completion_target );
      local_gce.wait();
    }
  });
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    CHECK(cores() >= 2); // at least 2 nodes for these tests...

    try_no_tasks();
    try_one_task();
    try_many_tasks();
    try_global_ce();
    for( int i = 0; i < 100; ++i ) {
      try_global_ce_recursive();
    }
    try_synchronizing_spawns();
    try_iterative_spmd_gce();
  
    Metrics::merge_and_dump_to_file();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
