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
#include "Message.hpp"
#include "CompletionEvent.hpp"
#include "ConditionVariable.hpp"
#include "Delegate.hpp"
#include "Tasking.hpp"
#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"
#include "Array.hpp"
#include "Collective.hpp"

BOOST_AUTO_TEST_SUITE( New_loop_tests );

using namespace Grappa;
using Grappa::wait;


static int test_global = 0;
CompletionEvent test_global_ce;
GlobalCompletionEvent my_gce;
CompletionEvent my_ce;

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
  BOOST_MESSAGE("Testing loop_decomposition_private...");
  int N = 16;
  
  CompletionEvent ce(N);
  
  impl::loop_decomposition<TaskMode::Bound,2>(0, N, [&ce](int64_t start, int64_t iters) {
    VLOG(1) << "loop(" << start << ", " << iters << ")";
    ce.complete(iters);
  });
  ce.wait();
}

void test_loop_decomposition_global() {
  BOOST_MESSAGE("Testing loop_decomposition_public..."); VLOG(1) << "loop_decomposition_public";
  int N = 160000;
  
  my_gce.enroll();
  impl::loop_decomposition<unbound,&my_gce>(0, N, [](int64_t start, int64_t iters) {
    if ( start%10000==0 ) {
      VLOG(1) << "loop(" << start << ", " << iters << ")";
    }
  });
	my_gce.complete();
	my_gce.wait();
}

void test_forall_here() {
  BOOST_MESSAGE("Testing forall_here..."); VLOG(1) << "forall_here";
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
    forall_here<2>(0, N, [&x](int64_t start, int64_t iters) {
      CHECK(mycore() == 0);
      for (int64_t i=0; i<iters; i++) {
        x++;
      }
    });
    BOOST_CHECK_EQUAL(x, N);
  }

  { VLOG(1) << "Testing forall_here overload";
    int x = 0;
    forall_here<2>(0, N, [&x](int64_t i) {
      CHECK(mycore() == 0);
      x++;
    });
    BOOST_CHECK_EQUAL(x, N);
    call_on_all_cores([]{ VLOG(1) << ".. done"; });
  }
  
}

void test_forall_global_private() {
  BOOST_MESSAGE("Testing forall_global...");
  const int64_t N = 1 << 8;
  
  BOOST_MESSAGE("  private");
  
  VLOG(1) << "forall_global_private {";
  
  forall(0, N, [](int64_t start, int64_t iters) {
    for (int i=0; i<iters; i++) {
      test_global++;
    }
  });
  
  VLOG(1) << "forall_global_private }";
  
  on_all_cores([]{
    range_t r = blockDist(0,N,mycore(),cores());
    BOOST_CHECK_EQUAL(test_global, r.end-r.start);
    test_global = 0;
  });

  forall<&my_gce>(0, N, [](int64_t i) {
    test_global++;
  });
  auto total = reduce<decltype(test_global),collective_add>(&test_global);
  BOOST_CHECK_EQUAL(total, N);
  
  call_on_all_cores([]{ VLOG(1) << "  -- done 'forall_global_private'"; });
}

void test_forall_global_public() {
  BOOST_MESSAGE("Testing forall_global_public..."); VLOG(1) << "forall_global_public";
  const int64_t N = 1 << 8;
  
  on_all_cores([]{ test_global = 0; });
  
  forall<unbound>(0, N, [](int64_t s, int64_t n) {
    test_global += n;
  });
  
  for (int i=0; i<cores(); i++) {
    VLOG(1) << "test_global => " << delegate::call(i, []{ return test_global; });
  }
  
  {
    auto total = reduce<decltype(test_global),collective_add>(&test_global);
    BOOST_CHECK_EQUAL(total, N);
  }
  
  VLOG(1) << "-- done";
  
  BOOST_MESSAGE("  with nested spawns"); VLOG(1) << "nested spawns";
  on_all_cores([]{ test_global = 0; });
  
  forall<unbound,&my_gce>(0, N, [](int64_t s, int64_t n){
    for (int i=s; i<s+n; i++) {
      spawn<unbound,&my_gce>([]{
        test_global++;
      });
    }
  });

  {
    auto total = reduce<decltype(test_global),collective_add>(&test_global);
    BOOST_CHECK_EQUAL(total, N);
  }
}

void test_forall_localized() {
  BOOST_MESSAGE("Testing forall (localized)..."); VLOG(1) << "testing forall (localized)";
  const int64_t N = 100;
  
  auto array = Grappa::global_alloc<int64_t>(N);
  
  VLOG(1) << "checking 'on_cores_localized'";
  on_cores_localized_async(array, N, [](int64_t* local_base, size_t nelem){
    VLOG(1) << "local_base => " << local_base <<"\nnelem => " << nelem;
  });
  
  forall(array, N, [](int64_t i, int64_t& e) {
    e = 1;
  });
  for (int i=0; i<N; i++) {
    BOOST_CHECK_EQUAL(delegate::read(array+i), 1);
  }

  forall(array, N, [](int64_t& e) {
    e = 2;
  });
  for (int i=0; i<N; i++) {
    BOOST_CHECK_EQUAL(delegate::read(array+i), 2);
  }

  forall(array, N, [](int64_t s, int64_t n, int64_t* e) {
    for (auto i=0; i<n; i++) {
      e[i] = 3;
    }
  });
  for (int i=0; i<N; i++) {
    BOOST_CHECK_EQUAL(delegate::read(array+i), 3);
  }
  
  BOOST_MESSAGE("Testing forall_async..."); VLOG(1) << "testing forall_async";
  
  VLOG(1) << "start spawning";
  forall<async,&my_gce>(array+ 0, 25, [](int64_t i, int64_t& e) { e = 2; });
  VLOG(1) << "after async";
  forall<async,&my_gce>(array+25, 25, [](int64_t i, int64_t& e) { e = 2; });
  VLOG(1) << "after async";
  forall<async,&my_gce>(array+50, 25, [](int64_t i, int64_t& e) { e = 2; });
  VLOG(1) << "after async";
  forall<async,&my_gce>(array+75, 25, [](int64_t i, int64_t& e) { e = 2; });
  VLOG(1) << "done spawning";
  
  my_gce.wait();
  
  int npb = block_size / sizeof(int64_t);
  
  auto * base = array.localize();
  auto * end = (array+N).localize();
  for (auto* x = base; x < end; x++) {
    BOOST_CHECK_EQUAL(*x, 2);
  }
  
  VLOG(1) << "checking indexing...";
  
  VLOG(1) << ">> forall";
  Grappa::memset(array, 0, N);
  forall(array, N, [](int64_t i, int64_t& e){ e = i; });
  for (int i=0; i<N; i++) {
    BOOST_CHECK_EQUAL(delegate::read(array+i), i);
  }
  
  VLOG(1) << ">> forall_async";
  VLOG(1) << ">>   my_gce => " << &my_gce;
  Grappa::memset(array, 0, N);
  forall<async,&my_gce>(array, N, [](int64_t i, int64_t& e){ e = i; });
  my_gce.wait();
  
  for (int i=0; i<N; i++) {
    BOOST_CHECK_EQUAL(delegate::read(array+i), i);
  }

  Grappa::memset(array, 0, N);    
  struct Pair { int64_t x, y; };
  auto pairs = static_cast<GlobalAddress<Pair>>(array);
  forall<&my_gce>(pairs, N/2, [](int64_t i, Pair& e){ e.x = i; e.y = i; });
  
  for (int i=0; i<N; i++) {
    BOOST_CHECK_EQUAL(delegate::read(array+i), i/2);
  }  
    
}

void test_forall_here_async() {
  const int N = 1117376;
  const int64_t x = 4;
  char * y = new char[N];
// test with different loop_thresholds
  impl::local_gce.enroll(1);
  forall_here<async,&impl::local_gce>( 0, N, [x,y](int64_t s, int64_t i) {
    for (int ii=0; ii<i; ii++) {
      y[s+ii] = (char) 0xff & x;
    }
  });
  impl::local_gce.complete();
  impl::local_gce.wait();
  for (int i=0; i<N; i++) {
    BOOST_CHECK_EQUAL(y[i], x);
  }
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    CHECK(Grappa::cores() >= 2); // at least 2 nodes for these tests...
    
    VLOG(1) << "my_gce => " << &my_gce;
    
    test_on_all_cores();
  
    test_loop_decomposition();
    test_loop_decomposition_global();
  
    test_forall_here();
    test_forall_global_private();
    test_forall_global_public();
  
    test_forall_localized();

    test_forall_here_async();
    
    Metrics::merge_and_dump_to_file();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
