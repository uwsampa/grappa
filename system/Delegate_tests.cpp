
#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"


BOOST_AUTO_TEST_SUITE( Delegate_tests );


int64_t some_data = 1234;

void user_main( thread * me, void * args ) 
{
  BOOST_MESSAGE( "Spawning user main thread " << (void *) current_thread <<
                 " " << me <<
                 " on node " << SoftXMT_mynode() );

  BOOST_CHECK_EQUAL( 2, SoftXMT_nodes() );
  // try read
  some_data = 1111;
  int64_t remote_data = SoftXMT_delegate_read_word( make_global(&some_data,1) );
  BOOST_CHECK_EQUAL( 1234, remote_data );
  BOOST_CHECK_EQUAL( 1111, some_data );
  
  // write
  SoftXMT_delegate_write_word( make_global(&some_data,1), 2345 );
  BOOST_CHECK_EQUAL( 1111, some_data );
  
  // verify write
  remote_data = SoftXMT_delegate_read_word( make_global(&some_data,1) );
  BOOST_CHECK_EQUAL( 2345, remote_data );

  // fetch and add
  remote_data = SoftXMT_delegate_fetch_and_add_word( make_global(&some_data,1), 1 );
  BOOST_CHECK_EQUAL( 1111, some_data );
  BOOST_CHECK_EQUAL( 2345, remote_data );
  
  // verify write
  remote_data = SoftXMT_delegate_read_word( make_global(&some_data,1) );
  BOOST_CHECK_EQUAL( 2346, remote_data );

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

