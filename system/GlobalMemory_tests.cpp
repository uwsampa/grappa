
#include <sys/mman.h>

#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Delegate.hpp"
#include "Cache.hpp"
#include "GlobalMemory.hpp"

BOOST_AUTO_TEST_SUITE( GlobalMemory_tests );


void user_main( Thread * me, void * args ) 
{
  // size_t N = 128;
  // GlobalAddress<int64_t> data = SoftXMT_typed_malloc<int64_t>(N);
  
  // for (size_t i=0; i<N; i++) {
  //   SoftXMT_delegate_write_word(data+i, i);
  // }
  
  // for (size_t i=0; i<N; i++) {
  //   Incoherent<int64_t>::RO c(data+i, 1);
  //   VLOG(2) << i << " == " << *c;
  //   BOOST_CHECK_EQUAL(i, *c);
  // }

  // SoftXMT_free(data);

  SoftXMT_waitForTasks( );
  BOOST_MESSAGE( "signaling done" );
  SoftXMT_signal_done( );
  BOOST_MESSAGE( "signaled done" );
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv),
		1 << 14 );

  SoftXMT_activate();

  DVLOG(1) << "Spawning user main Thread....";
  SoftXMT_run_user_main( &user_main, NULL );
  BOOST_CHECK( SoftXMT_done() == true );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

