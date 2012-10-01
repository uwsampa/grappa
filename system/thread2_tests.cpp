
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"


BOOST_AUTO_TEST_SUITE( Thread_tests );

void child2( thread * me, void * args );

void child1( thread * me, void * args ) 
{
  char * str = (char *) args;
 BOOST_MESSAGE( "Spawning child1 thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );

 SoftXMT_yield();
 BOOST_MESSAGE( "child1 after yield thread " << (void *) current_thread <<
                 " " << me <<
                 " on node " << SoftXMT_mynode() );
 char str1[] = "Hello from child2-1";
 char str2[] = "Hello from child2-2";
 char str3[] = "Hello from child2-3";
 SoftXMT_spawn( &child2, str1 );
 SoftXMT_spawn( &child2, str2 );
 SoftXMT_spawn( &child2, str3 );
 BOOST_MESSAGE( "child1 after spawn child2 thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );

 thread_exit( me, (void *)1 );
 BOOST_MESSAGE( "child1 after exit thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );

}

void child2( thread * me, void * args ) 
{
  char * str = (char *) args;
 BOOST_MESSAGE( "Spawning child2 thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );

 SoftXMT_yield();
 BOOST_MESSAGE( "child2 after yield1 thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );

 SoftXMT_yield();
 BOOST_MESSAGE( "child2 after yield2 thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );
 SoftXMT_yield();
 BOOST_MESSAGE( "child2 after yield3 thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );
 SoftXMT_yield();
 BOOST_MESSAGE( "child2 after yield4 thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );
 SoftXMT_yield();
 BOOST_MESSAGE( "child2 after yield5 thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );
}

void user_main( thread * me, void * args ) 
{
  char * str = (char *) args;
  BOOST_MESSAGE( "Spawning user main thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );
  char str1[] = "hello from child1-1";
  SoftXMT_spawn( &child1, str1 );
  BOOST_MESSAGE( "user main after spawn child1 thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );

 SoftXMT_yield();
 BOOST_MESSAGE( "user main after yield1 thread " << (void *) current_thread <<
                 " " << me <<
                 " " << str <<
                 " on node " << SoftXMT_mynode() );

 SoftXMT_yield();
 BOOST_MESSAGE( "user main after yield2 thread " << (void *) current_thread <<
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

