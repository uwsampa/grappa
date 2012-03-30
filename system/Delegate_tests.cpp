
#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Delegate.hpp"

BOOST_AUTO_TEST_SUITE( Delegate_tests );


int64_t some_data = 1234;

int64_t other_data __attribute__ ((aligned (2048))) = 0;

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

  // check compare_and_swap
  bool swapped;
  swapped = SoftXMT_delegate_compare_and_swap_word( make_global(&some_data,1), 123, 3333); // shouldn't swap
  BOOST_CHECK_EQUAL( swapped, false );
  // verify value is unchanged
  remote_data = SoftXMT_delegate_read_word( make_global(&some_data,1) );
  BOOST_CHECK_EQUAL( 2346, remote_data );
  
  // now actually do swap
  swapped = SoftXMT_delegate_compare_and_swap_word( make_global(&some_data,1), 2346, 3333); // shouldn't swap
  BOOST_CHECK_EQUAL( swapped, true );
  // verify value is unchanged
  remote_data = SoftXMT_delegate_read_word( make_global(&some_data,1) );
  BOOST_CHECK_EQUAL( 3333, remote_data );
  
  // try linear global address

  // initialize
  other_data = 0;
  SoftXMT_delegate_write_word( make_global(&other_data,1), 1 );

  int * foop = new int;
  *foop = 1234;
  BOOST_MESSAGE( *foop );
    

  // make address
  BOOST_MESSAGE( "pointer is " << &other_data );
  GlobalAddress< int64_t > la = make_linear( &other_data );

  // check pointer computation
  BOOST_CHECK_EQUAL( la.node(), 0 );
  BOOST_CHECK_EQUAL( la.pointer(), &other_data );

  // check data
  BOOST_CHECK_EQUAL( 0, other_data );
  remote_data = SoftXMT_delegate_read_word( la );
  BOOST_CHECK_EQUAL( 0, remote_data );

  // change pointer and check computation
  ++la;
  BOOST_CHECK_EQUAL( la.node(), 0 );
  BOOST_CHECK_EQUAL( la.pointer(), &other_data + 1 );

  // change pointer and check computation
  la += 7;
  BOOST_CHECK_EQUAL( la.node(), 1 );
  BOOST_CHECK_EQUAL( la.pointer(), &other_data );

  // check remote data
  remote_data = SoftXMT_delegate_read_word( la );
  BOOST_CHECK_EQUAL( 1, remote_data );
  

  SoftXMT_signal_done();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();



  DVLOG(1) << "Spawning user main thread....";
  SoftXMT_run_user_main( &user_main, NULL );
  BOOST_CHECK( SoftXMT_done() == true );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

