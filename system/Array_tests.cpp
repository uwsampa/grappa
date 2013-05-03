
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

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
  GlobalAddress<T> xs = Grappa_typed_malloc<T>(NN);
  GlobalAddress<T> ys = Grappa_typed_malloc<T>(NN);

  Grappa::memset(xs, Val, NN);
  Grappa::forall_localized(xs, NN, [](int64_t i, T& v) {
    BOOST_CHECK_EQUAL(v, Val);
  });

  if (test_async) {
    Grappa::memcpy_async<&gce>(ys, xs, NN);
    gce.wait();
  } else {
    Grappa::memcpy(ys, xs, NN);
  }

  Grappa::forall_localized(ys, NN, [](int64_t i, T& v) {
    BOOST_CHECK_EQUAL(v, Val);
  });

  // try the other way (ys -> xs)
  Grappa::memset(xs, 0, NN);
  if (test_async) {
    Grappa::memcpy_async<&gce>(xs, ys, NN);
    gce.wait();
  } else {
    Grappa::memcpy(xs, ys, NN);
  }
  Grappa::forall_localized(xs, NN, [](int64_t i, T& v) {
    BOOST_CHECK_EQUAL(v, Val);
  });

  Grappa_free(xs);
  Grappa_free(ys);
}

void test_complex() {
  GlobalAddress< complex<double> > xs = Grappa_typed_malloc< complex<double> >(NN);
  GlobalAddress< complex<double> > ys = Grappa_typed_malloc< complex<double> >(NN);

  Grappa::memset(xs, complex<double>(7.0,1.0), NN);
  Grappa::forall_localized(xs, NN, [](int64_t i, complex<double>& v) {
    BOOST_CHECK_EQUAL(v, complex<double>(7.0,1.0));
  });

  Grappa::memcpy(ys, xs, NN);

  Grappa::forall_localized(ys, NN, [](int64_t i, complex<double>& v) {
    BOOST_CHECK_EQUAL(v, complex<double>(7.0,1.0));
  });

  Grappa_free(xs);
  Grappa_free(ys);
}

void test_prefix_sum() {
  BOOST_MESSAGE("prefix_sum");
  auto xs = Grappa_typed_malloc<int64_t>(N);
  Grappa::memset(xs, 1, N);
  
  // prefix-sum
//  for (int64_t i=0; i<N; i++) {
//    Grappa::delegate::write(xs+i, i);
//  }
  Grappa::prefix_sum(xs, N);
  
  Grappa::forall_localized(xs, N, [](int64_t i, int64_t& v){
    BOOST_CHECK_EQUAL(v, i);
  });
  Grappa_free(xs);
}

PushBuffer<int64_t> pusher;

void test_push_buffer() {
  BOOST_MESSAGE("Testing PushBuffer");
  // (not really in Array, but could be...)
  auto xs = Grappa_typed_malloc<int64_t>(N*Grappa::cores());
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
  Grappa::forall_localized(xs, N*Grappa::cores(), [](int64_t i, int64_t& v) {
    BOOST_CHECK_EQUAL(v, 1);
  });
  
  Grappa_free(xs);
}


void test_global_queue() {
  BOOST_MESSAGE("Testing GlobalQueue"); VLOG(1) << "testing global queue";
  
  auto qa = GlobalQueue<int64_t>::create(NN);
  
  on_all_cores([qa] {
    switch (mycore()) {
    case 0:
      for (int i=0; i<NN/2; i++) {
        qa->push(7);
      }
      break;
    case 1:
      for (int i=NN/2; i<NN; i++) {
        qa->push(7);
      }
      break;
    }
  });
  
  for (int i=0; i<NN; i++) {
    BOOST_CHECK_EQUAL(delegate::read(qa->storage()+i), 7);
  }
  
  // gq.free();
}

void user_main( void * ignore ) {
  
  // test_memset_memcpy<int64_t,7>();
  // test_memset_memcpy<int64_t,7>(true); // test async
  // test_memset_memcpy<double,7.0>();
  // test_complex();
  // test_prefix_sum(); // (not implemented yet)
  // test_push_buffer();
  
  test_global_queue();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  Grappa_activate();
  NN = FLAGS_nelems;

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

