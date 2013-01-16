
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "ConditionVariable.hpp"

BOOST_AUTO_TEST_SUITE( ConditionVariable_tests );


void user_main( void * args ) 
{
  Grappa::Mutex m;
  Grappa::ConditionVariable cv, cv2;

  int data = 0;

  Grappa::privateTask( [&] { 
      Grappa::lock( &m ); 
      data++; 
      Grappa::signal( &cv ); 
      Grappa::unlock( &m ); 

      Grappa::wait( &cv2 );
      Grappa::signal( &cv );
    } );

  Grappa::lock( &m );
  Grappa::wait( &cv, &m );
  Grappa::unlock( &m );

  Grappa::signal( &cv2 );
  Grappa::wait( &cv );

  BOOST_CHECK_EQUAL( data, 1 );

  Grappa_merge_and_dump_stats();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
	       &(boost::unit_test::framework::master_test_suite().argv),
	       (1L << 20) );

  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

