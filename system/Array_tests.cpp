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

#include <complex>
using std::complex;

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "Array.hpp"
#include "ParallelLoop.hpp"
#include "GlobalAllocator.hpp"
#include "Delegate.hpp"
#include "PushBuffer.hpp"

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( Array_tests );

static const size_t N = (1L<<10);
static size_t NN = (1L<<20) - 21;

DEFINE_int64(nelems, NN, "number of elements in (large) test arrays");

GlobalCompletionEvent gce;

template<typename T, T Val>
void test_memset_memcpy(bool test_async = false) {
  GlobalAddress<T> xs = Grappa::global_alloc<T>(NN);
  GlobalAddress<T> ys = Grappa::global_alloc<T>(NN);

  Grappa::memset(xs, Val, NN);
  Grappa::forall(xs, NN, [](int64_t i, T& v) {
    BOOST_CHECK_EQUAL(v, Val);
  });

  if (test_async) {
    Grappa::memcpy_async<&gce>(ys, xs, NN);
    gce.wait();
  } else {
    Grappa::memcpy(ys, xs, NN);
  }

  Grappa::forall(ys, NN, [](int64_t i, T& v) {
    CHECK_EQ(v, Val);
  });

  // try the other way (ys -> xs)
  Grappa::memset(xs, 0, NN);
  if (test_async) {
    Grappa::memcpy_async<&gce>(xs, ys, NN);
    gce.wait();
  } else {
    Grappa::memcpy(xs, ys, NN);
  }
  Grappa::forall(xs, NN, [](int64_t i, T& v) {
    BOOST_CHECK_EQUAL(v, Val);
  });

  Grappa::global_free(xs);
  Grappa::global_free(ys);
}

void test_complex() {
  GlobalAddress< complex<double> > xs = Grappa::global_alloc< complex<double> >(NN);
  GlobalAddress< complex<double> > ys = Grappa::global_alloc< complex<double> >(NN);

  Grappa::memset(xs, complex<double>(7.0,1.0), NN);
  Grappa::forall(xs, NN, [](int64_t i, complex<double>& v) {
    BOOST_CHECK_EQUAL(v, complex<double>(7.0,1.0));
  });

  Grappa::memcpy(ys, xs, NN);

  Grappa::forall(ys, NN, [](int64_t i, complex<double>& v) {
    BOOST_CHECK_EQUAL(v, complex<double>(7.0,1.0));
  });

  Grappa::global_free(xs);
  Grappa::global_free(ys);
}

void test_prefix_sum() {
  BOOST_MESSAGE("prefix_sum");
  auto xs = Grappa::global_alloc<int64_t>(N);
  Grappa::memset(xs, 1, N);
  
  // prefix-sum
//  for (int64_t i=0; i<N; i++) {
//    Grappa::delegate::write(xs+i, i);
//  }
  Grappa::prefix_sum(xs, N);
  
  Grappa::forall(xs, N, [](int64_t i, int64_t& v){
    BOOST_CHECK_EQUAL(v, i);
  });
  Grappa::global_free(xs);
}

PushBuffer<int64_t> pusher;

void test_push_buffer() {
  BOOST_MESSAGE("Testing PushBuffer");
  // (not really in Array, but could be...)
  auto xs = Grappa::global_alloc<int64_t>(N*Grappa::cores());
  Grappa::memset(xs, 0, N*Grappa::cores());
  
  int64_t index = 0;
  auto ia = make_global(&index);
  
  Grappa::on_all_cores([xs,ia]{
    pusher.setup(xs, ia);
    
    for (int i=0; i<N; i++) {
      pusher.push(1);
    }
    
    pusher.flush();
  });
  Grappa::forall(xs, N*Grappa::cores(), [](int64_t i, int64_t& v) {
    BOOST_CHECK_EQUAL(v, 1);
  });
  
  Grappa::global_free(xs);
}


BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
  
    test_memset_memcpy<int64_t,7>();
    test_memset_memcpy<int64_t,7>(true); // test async
    // test_memset_memcpy<double,7.0>();
    test_complex();
    // test_prefix_sum(); // (not implemented yet)
    test_push_buffer();
    
    BOOST_MESSAGE("Testing memcpy on 2D addresses");
    
    int v[] = {1, 2, 3};
    auto sz = 3;
    auto gv = make_global(v);
    
    spawnRemote<&gce>(1, [=]{
      std::vector<int> v2(sz);
      memcpy(make_global(&v2[0]), gv, sz);
      BOOST_CHECK_EQUAL(v2[0], 1);
      BOOST_CHECK_EQUAL(v2[1], 2);
      BOOST_CHECK_EQUAL(v2[2], 3);
    });
    gce.wait();
    
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();

