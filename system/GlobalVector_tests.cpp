
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "ParallelLoop.hpp"
#include "GlobalAllocator.hpp"
#include "Delegate.hpp"
#include "GlobalVector.hpp"
#include "Statistics.hpp"

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( GlobalVector_tests );

static size_t N = (1L<<10) - 21;

DEFINE_int64(nelems, N, "number of elements in (large) test arrays");
DEFINE_int64(buffer_size, 1<<10, "number of elements in (large) test arrays");

void test_global_vector() {
  BOOST_MESSAGE("Testing GlobalVector"); VLOG(1) << "testing global queue";
  
  auto qa = GlobalVector<int64_t>::create(N, FLAGS_buffer_size);
  VLOG(1) << "queue addr: " << qa;
  on_all_cores([qa] {
    switch (mycore()) {
    case 0:
      VLOG(1) << "starting forall_here";
      forall_here(0, N/2, [qa](int64_t s, int64_t n) {
        for (int64_t i=s; i<s+n; i++) {
          VLOG(1) << "pushing " << i << " : " << qa->inflight;
          qa->push(7);
        }
      });
      break;
    case 1:
      forall_here(N/2, N, [qa](int64_t s, int64_t n) {
        for (int64_t i=s; i<s+n; i++) {
          qa->push(7);
        }
      });
      break;
    }
  });
  
  for (int i=0; i<N; i++) {
    VLOG(1) << "q[" << i << "] = " << delegate::read(qa->storage()+i);
    BOOST_CHECK_EQUAL(delegate::read(qa->storage()+i), 7);
  }
  
  GlobalVector<int64_t>::destroy(qa);
}

void user_main( void * ignore ) {
  test_global_vector();
  
  Statistics::merge_and_print();
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

