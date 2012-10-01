
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"


BOOST_AUTO_TEST_SUITE( Thread_tests );

struct th_args {
	char str[512];
};
void child1( thread * me, th_args * args ) 
{
 BOOST_MESSAGE( "Spawning child1 thread " << (void *) current_thread <<
                 " " << me <<
                 " " << args->str <<
                 " on node " << SoftXMT_mynode() );
}

void user_main( thread * me, void * args ) 
{
  char * str = (char *) args;
  BOOST_MESSAGE( "Spawning user main thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );
  //char str1[] = "hello from child1-1";
  th_args a = { "hello from child1-1" };
  SoftXMT_remote_spawn( &child1, &a, 1);
  BOOST_MESSAGE( "user main after spawn child1 thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );

  SoftXMT_signal_done();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();

  BOOST_MESSAGE( "main before user main thread " << (void *) current_thread <<
                 " on node " << SoftXMT_mynode() );
  char str[] = "user main";
  SoftXMT_run_user_main( &user_main, str );
  BOOST_CHECK( SoftXMT_done() == true );
  BOOST_MESSAGE( "main after user main thread " << (void *) current_thread <<
                 " on node " << SoftXMT_mynode() );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

