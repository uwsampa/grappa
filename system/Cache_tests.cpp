
#include "Cache.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( Cache_tests );

int64_t foo = 1234;
int64_t bar[4] = { 2345, 3456, 4567, 6789 };

void user_main( thread * me, void * args ) 
{
  {
    Incoherent<int64_t>::RO buf( GlobalAddress< int64_t >( 0, &foo ) );
    BOOST_CHECK_EQUAL( *buf, foo );
    // *buf = 1234;
    // BOOST_CHECK_EQUAL( *buf, foo );
  }

  // {
  //   Incoherent<int64_t>::RW buf( GlobalAddress< int64_t >( 0, &foo ) );
  //   foo = 1235;
  //   BOOST_CHECK( *buf != foo );
  //   *buf = 1235;
  //   BOOST_CHECK_EQUAL( *buf, foo );
  // }

  // {
  //   int64_t x;
  //   Incoherent<int64_t>::RO buf( GlobalAddress< int64_t >( 0, &foo ), 1, &x );
  //   BOOST_CHECK_EQUAL( *buf, foo );
  // }

  // {
  //   int64_t y[4];
  //   Incoherent<int64_t>::RO buf2( GlobalAddress< int64_t >( 0, &bar[0] ), 4, &y[0] );
  //   BOOST_CHECK_EQUAL( buf2[2], bar[2] );
  // }

  // {
  //   Incoherent<int64_t>::RW buf3( GlobalAddress< int64_t >( 0, bar ), 4);
  //   BOOST_CHECK_EQUAL( buf3[2], bar[2] );
  // }

  SoftXMT_signal_done();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();

  SoftXMT_run_user_main( &user_main, NULL );
  BOOST_CHECK( SoftXMT_done() == true );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

