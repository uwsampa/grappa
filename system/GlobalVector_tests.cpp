
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <complex>
using std::complex;

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "ParallelLoop.hpp"
#include "GlobalAllocator.hpp"
#include "Delegate.hpp"
#include "GlobalVector.hpp"

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( GlobalVector_tests );

static size_t N = (1L<<10) - 21;

DEFINE_int64(nelems, N, "number of elements in (large) test arrays");

void test_global_vector() {
  BOOST_MESSAGE("Testing GlobalVector"); VLOG(1) << "testing global queue";
  
  auto qa = GlobalVector<int64_t>::create(N);
  
  on_all_cores([qa] {
    switch (mycore()) {
    case 0:
      for (int i=0; i<N/2; i++) {
        qa->push(7);
      }
      break;
    case 1:
      for (int i=N/2; i<N; i++) {
        qa->push(7);
      }
      break;
    }
  });
  
  for (int i=0; i<N; i++) {
    BOOST_CHECK_EQUAL(delegate::read(qa->storage()+i), 7);
  }
  
  GlobalVector<int64_t>::destroy(qa);
}

void user_main( void * ignore ) {
  test_global_vector();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  Grappa_activate();
  N = FLAGS_nelems;

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

