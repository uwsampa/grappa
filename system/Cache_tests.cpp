
#include "Cache.hpp"



#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( Cache_tests );

int64_t foo = 1234;
int64_t bar[4] = { 2345, 3456, 4567, 6789 };

void user_main( thread * me, void * args ) 
{
  {
    Incoherent< int64_t >::RO buf( GlobalAddress< int64_t >( &foo ), 1 );
    BOOST_CHECK_EQUAL( *buf, foo );
  }

  {
    {
      Incoherent<int64_t>::RW buf( GlobalAddress< int64_t >( &foo ), 1 );
      BOOST_CHECK_EQUAL( *buf, foo );
      foo = 1235;
      BOOST_CHECK( *buf != foo );
      BOOST_CHECK_EQUAL( foo, 1235 );
      *buf = 1235;
      BOOST_CHECK_EQUAL( *buf, foo );
      *buf = 1236;
    }
    BOOST_CHECK_EQUAL( foo, 1236 );
  }

  {
    int64_t x;
    Incoherent<int64_t>::RO buf( GlobalAddress< int64_t >( &foo ), 1, &x );
    BOOST_CHECK_EQUAL( *buf, foo );
  }

  {
    int64_t y[4];
    Incoherent<int64_t>::RO buf2( GlobalAddress< int64_t >( bar ), 4, &y[0] );
    BOOST_CHECK_EQUAL( buf2[2], bar[2] );
  }

  {
    Incoherent<int64_t>::RW buf3( GlobalAddress< int64_t >( bar ), 4);
    BOOST_CHECK_EQUAL( buf3[2], bar[2] );
  }

  {
    // test for early wakeup handled properly
    BOOST_MESSAGE( "Wakeup test" );
    Incoherent<int64_t>::RW buf4( GlobalAddress< int64_t >( bar, 1 ), 1);
    buf4.start_acquire( );
    for (int i=0; i<2000; i++) {
        SoftXMT_yield();
    }
    buf4.block_until_acquired( );

    buf4.start_release( );
    for (int i=0; i<2000; i++) {
        SoftXMT_yield();
    }
    buf4.block_until_released( );
  }

  SoftXMT_signal_done();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();

  BOOST_CHECK_EQUAL( SoftXMT_nodes(), 2 );
  SoftXMT_run_user_main( &user_main, NULL );
  BOOST_CHECK( SoftXMT_done() == true );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

