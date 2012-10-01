
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include <SoftXMT.hpp>
#include <GlobalTaskJoiner.hpp>
#include <ForkJoin.hpp>
#include <AsyncParallelFor.hpp>

#include <Delegate.hpp>
#define read SoftXMT_delegate_read_word
#define write SoftXMT_delegate_write_word

#include <GlobalAllocator.hpp>
#include <PerformanceTools.hpp>


BOOST_AUTO_TEST_SUITE( LocalIterator_tests );

GlobalAddress<int64_t> array;
int64_t * local_base;

const size_t N = (1L<<24);
const size_t nbuf = 1L<<22;

void user_main( void * ignore ) {
  array = SoftXMT_typed_malloc<int64_t>(N);
  int64_t * buf = new int64_t[nbuf];
  
  double t, comm_time, local_time;

  SOFTXMT_TIME(comm_time, {
    SoftXMT_memset(array, 0L, N);
  });

  BOOST_MESSAGE("checking local memset works");
  SOFTXMT_TIME(local_time, {
    // local memset uses 'localize' to have each node do all only the elements local to it
    SoftXMT_memset_local(array, 1L, N);
  });
  //log_array("array", array);

  // verify that memset got everything
  int64_t * buf = new int64_t[nbuf];
  for (size_t i=0; i<N; i+=nbuf) {
    size_t n = MIN(N-i, nbuf);
    Incoherent<int64_t>::RO c(array+i, n, buf);
    for (size_t j=0; j<n; j++) {
      BOOST_CHECK_EQUAL(c[j], 1);
    }
  }
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();

  SoftXMT_run_user_main( &user_main, (void*)NULL );
  CHECK( SoftXMT_done() );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();


