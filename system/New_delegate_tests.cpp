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

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "PerformanceTools.hpp"

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
  delegate::write(ga, 7);
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
  
  FullEmpty<bool> w;
  auto gw = make_global(&w);
  
  send_message(1, [ga, gw] {
    spawn([=]{
      BOOST_CHECK_EQUAL(delegate::read(ga), 0);
      send_heap_message(gw.core(), [gw]{
          gw->writeXF(true);
        });
    });
  });
  auto r1 = w.readFE();
  BOOST_CHECK_EQUAL(a, 0); // value unchanged
  
  // write
  send_message(1, [ga, gw] {
    spawn([=]{
      delegate::write(ga, 7);
      send_heap_message(gw.core(), [gw]{
          gw->writeXF(true);
        });
    });
  });
  auto r2 = w.readFE();
  BOOST_CHECK_EQUAL(a, 7);
  
  // compare and swap
  double b = 3.14;
  auto gb = make_global(&b);
  send_message(1, [gb, gw] {
    spawn([=]{
      BOOST_CHECK_EQUAL(delegate::compare_and_swap(gb, 3.14, 2.0), true);
      send_heap_message(gw.core(), [gw]{
          gw->writeXF(true);
        });
    });
  });
  auto r3 = w.readFE();
  BOOST_CHECK_EQUAL(b, 2.0);
  send_message(1, [gb, gw] {
    spawn([=]{
      BOOST_CHECK_EQUAL(delegate::compare_and_swap(gb, 3.14, 3.0), false);
      send_heap_message(gw.core(), [gw]{
          gw->writeXF(true);
        });
    });
  });
  auto r4 = w.readFE();
  BOOST_CHECK_EQUAL(b, 2.0);
  
  // fetch and add
  uint64_t c = 1;
  auto gc = make_global(&c);
  send_message(1, [gc, gw] {
    spawn([=]{
      BOOST_CHECK_EQUAL(delegate::fetch_and_add(gc, 1), 1);
      send_heap_message(gw.core(), [gw]{
          gw->writeXF(true);
        });
    });
  });
  auto r5 = w.readFE();
  BOOST_CHECK_EQUAL(c, 2);
  send_message(1, [gc, gw] {
    spawn([=]{
      BOOST_CHECK_EQUAL(delegate::fetch_and_add(gc, -2), 2);
      send_heap_message(gw.core(), [gw]{
          gw->writeXF(true);
        });
    });
  });
  auto r6 = w.readFE();
  BOOST_CHECK_EQUAL(c, 0);
}

GlobalCompletionEvent mygce;
int global_x;
int global_y;

void check_async_delegates() {
  BOOST_MESSAGE("check_async_delegates");
  BOOST_MESSAGE("  feed forward...");
  const int N = 1 << 8;
  
  delegate::write(make_global(&global_x,1), 0);
  
  for (int i=0; i<N; i++) {
    delegate::call<async,&mygce>(1, []{ global_x++; });
  }
  mygce.wait();
  
  BOOST_CHECK_EQUAL(delegate::read(make_global(&global_x,1)), N);
  
  auto xa = make_global(&global_x,1);
  delegate::write<async,&mygce>(xa, 0);
  mygce.wait();
  
  for (int i=0; i<N; i++) {
    delegate::increment<async,&mygce>(xa, 1);
  }
  mygce.wait();
  
  BOOST_CHECK_EQUAL(delegate::read(xa), N);
  
  delegate::call(1, []{
    global_y = 0;
  });
  
  BOOST_MESSAGE("  promises...");
  delegate::Promise<int> a[N];
  for (int i=0; i<N; i++) {
    a[i].call_async( 1, [i]()->int {
      global_y++;
      return global_x+i;
    });
  }
  for (int i=0; i<N; i++) {
    BOOST_CHECK_EQUAL(a[i].get(), N+i);
  }
  BOOST_CHECK_EQUAL(delegate::read(make_global(&global_y,1)), N);
}

uint64_t fc_targ = 0;
uint64_t non_fc_targ = 0;
void check_fetch_add_combining() {
  BOOST_MESSAGE("check_fetch_add_combining");
  delegate::FetchAddCombiner<uint64_t,uint64_t> fc( make_global(&fc_targ, 1), 4, 0 );

  int N = 9;
  CompletionEvent done( N );
  
  uint64_t actual_total = 0;
  for (int i=0; i<N; i++) {
    spawn([&fc,&actual_total,&done] {
      fc.promise();
      // just find a reason to suspend
      // to make fetch_and_add likely to aggregate
      delegate::fetch_and_add( make_global(&non_fc_targ, 1), 1 );

      actual_total += fc.fetch_and_add( 1 );
      done.complete();
    });
  }
  done.wait();

  BOOST_CHECK( actual_total == (N-1)*N/2 );
  delegate::call( 1, [N] {
    BOOST_CHECK( fc_targ == N );
    BOOST_CHECK( non_fc_targ == N );
  });
}

void check_call_suspending() {
  BOOST_MESSAGE("Check delegate::call_suspendable...");
  
  int x = 42;
  auto xa = make_global(&x);
  
  int y = delegate::call_suspendable(1, [xa]{
    return delegate::read(xa);
  });
  
  BOOST_CHECK_EQUAL(x, y);
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    CHECK(Grappa::cores() >= 2); // at least 2 nodes for these tests...

    Grappa::Metrics::start_tracing();

    check_short_circuiting();
  
    check_remote();
  
    check_async_delegates();

    check_fetch_add_combining();
 
    check_call_suspending();
 
    int64_t seed = 111;
    GlobalAddress<int64_t> seed_addr = make_global(&seed);

    Grappa::ConditionVariable waiter;
    auto waiter_addr = make_global(&waiter);
  
    send_message(1, [seed_addr, waiter_addr] {
      // on node 1
      spawn([seed_addr, waiter_addr] {
        int64_t vseed = delegate::read(seed_addr);
        BOOST_CHECK_EQUAL(111, vseed);
      
        delegate::write(seed_addr, 222);
        signal(waiter_addr);
      });
    });
    Grappa::wait(&waiter);
    BOOST_CHECK_EQUAL(seed, 222);
    
    // new delegate::call(GlobalAddress) overload
    call_on_all_cores([]{ global_x = 0; });
    
    auto xa = make_global(&global_x, 1);
    
    auto r = delegate::call(xa, [](int& x){ BOOST_CHECK_EQUAL(x, 0); x = 1; return true; });
    BOOST_CHECK(r);
    
    delegate::call(xa, [](int* x){ BOOST_CHECK_EQUAL(*x, 1); *x = 2; });
    
    finish([xa]{
      for (int i=0; i < 10; i++) {
        delegate::call<async>(xa, [](int& x){
          BOOST_CHECK(x >= 2 && x < 12);
          x++;
        });
      }
    });
    
    Grappa::Metrics::stop_tracing();
    Grappa::Metrics::merge_and_dump_to_file();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
