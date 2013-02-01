
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <complex>
using std::complex;

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "Tasking.hpp"
#include "FileIO.hpp"
#include "Array.hpp"

BOOST_AUTO_TEST_SUITE( Array_tests );

static const size_t N = (1L<<10);
static const size_t NN = (1L<<10);

template<typename T, T Val>
void check_value(T * v) {
  BOOST_CHECK_EQUAL(*v, Val);
}

template<typename T, T Val>
void test_memset_memcpy() {
  GlobalAddress<T> xs = Grappa_typed_malloc<T>(NN);
  GlobalAddress<T> ys = Grappa_typed_malloc<T>(NN);

  Grappa_memset_local(xs, Val, NN);
  forall_local< T, check_value<T,Val> >(xs, NN);

  Grappa_memcpy(ys, xs, NN);

  forall_local< T, check_value<T,Val> >(ys, NN);

  Grappa_free(xs);
  Grappa_free(ys);
}

void check_complex(complex<double> * v) {
  BOOST_CHECK_EQUAL(*v, complex<double>(7.0,1.0));
}

void test_complex() {
  GlobalAddress< complex<double> > xs = Grappa_typed_malloc< complex<double> >(NN);
  GlobalAddress< complex<double> > ys = Grappa_typed_malloc< complex<double> >(NN);

  Grappa_memset_local(xs, complex<double>(7.0,1.0), NN);
  forall_local<complex<double>,check_complex>(xs, NN);

  Grappa_memcpy(ys, xs, NN);

  forall_local< complex<double>, check_complex >(ys, NN);

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

