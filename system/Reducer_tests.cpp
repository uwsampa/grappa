
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "GlobalAllocator.hpp"
#include "Reducer.hpp"
#include "Barrier.hpp"

BOOST_AUTO_TEST_SUITE( Reducer_tests );

using namespace Grappa;
//#define TEST_MSG BOOST_MESSAGE( "In test " << __func__ );
#define TEST(name) void test_##name()
#define RUNTEST(name) BOOST_MESSAGE( "== Test " << #name << " =="); test_##name()

TEST(int_add) {
  int64_t expected = Grappa::cores(); 
  on_all_cores( [expected] {
    Reducer<int64_t, collective_add> r( 0 );
    r.reset();
    r.accumulate( 1 );

    barrier();

    int64_t result = r.finish();
    BOOST_CHECK( result == expected );
  });
}

TEST(int_add_more) {
  int64_t nc = Grappa::cores();
  int64_t expected = nc*(nc+1)*(2*nc+1)/6;
  on_all_cores( [expected] {
    Reducer<int64_t, collective_add> r( 0 );
    r.reset();
    for (int i=0; i<Grappa::mycore()+1; i++) {
      r.accumulate( Grappa::mycore()+1 );
    }

    barrier();

    int64_t result = r.finish();
    BOOST_CHECK( result == expected );
  });
}

TEST(int_max) {
  int64_t expected = 1; 
  on_all_cores( [expected] {
    Reducer<int64_t, collective_max> r( 0 );
    r.reset();
    r.accumulate( -1*Grappa::mycore() + 1  );

    barrier();

    int64_t result = r.finish();
    BOOST_CHECK( result == expected );
  });
}
  

void user_main(void * args) {
  BOOST_CHECK(Grappa::cores() >= 2); // at least 2 nodes for these tests...

  RUNTEST(int_add);
  RUNTEST(int_add_more);
  RUNTEST(int_max);
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
	       &(boost::unit_test::framework::master_test_suite().argv)
	       );

  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
