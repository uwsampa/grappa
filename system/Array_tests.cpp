
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

BOOST_AUTO_TEST_SUITE( Array_tests );

static const size_t N = (1L<<10);
static const size_t NN = (1L<<10);

template<typename T, T Val>
void test_memset_memcpy() {
  GlobalAddress<T> xs = Grappa_typed_malloc<T>(NN);
  GlobalAddress<T> ys = Grappa_typed_malloc<T>(NN);

  Grappa::memset(xs, Val, NN);
  Grappa::forall_localized(xs, NN, [](int64_t i, T& v) {
    BOOST_CHECK_EQUAL(v, Val);
  });

  Grappa::memcpy(ys, xs, NN);

  Grappa::forall_localized(ys, NN, [](int64_t i, T& v) {
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

void user_main( void * ignore ) {
  test_memset_memcpy<int64_t,7>();
  //test_memset_memcpy<double,7.0>();
  test_complex();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

