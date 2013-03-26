#include <boost/test/unit_test.hpp>
#include "ThreadQueue.hpp"
#include <iostream>

BOOST_AUTO_TEST_SUITE( ThreadQueue_tests );

BOOST_AUTO_TEST_CASE( test1 ) {
  #ifndef DEBUG
    BOOST_MESSAGE( "This test requires -DDEBUG for full checks" );
    BOOST_CHECK( false );
#endif

  Worker * ws[100];
  for(int i=0; i< 100; i++) {
    ws[i] = new Worker();
  }

  PrefetchingThreadQueue q(4);

  q.enqueue(ws[0]);
  BOOST_CHECK( q.length() == 1 );
  BOOST_CHECK( q.dequeue() == ws[0] );
  BOOST_CHECK( q.length() == 0 );
  BOOST_CHECK( q.dequeue() == NULL );
  BOOST_CHECK( q.length() == 0 );

  q.enqueue(ws[1]);
  BOOST_CHECK( q.length() == 1 );
  q.enqueue(ws[2]);
  BOOST_CHECK( q.length() == 2 );
  BOOST_CHECK( q.dequeue() == ws[1] );
  BOOST_CHECK( q.length() == 1 );
  q.enqueue(ws[3]);
  BOOST_CHECK( q.length() == 2 );

  int iters = 10;
  for ( int i=1; i<=iters; i++ ) {
    q.enqueue(ws[3+i]);
    BOOST_CHECK( q.length() == 2+i );
  }
  int len = q.length();

  BOOST_CHECK( q.dequeue() == ws[2] );
  BOOST_CHECK( q.length() == len-1 );
  BOOST_CHECK( q.dequeue() == ws[3] );
  BOOST_CHECK( q.length() == len-2 );
  BOOST_CHECK( q.dequeue() == ws[4] );
  BOOST_CHECK( q.length() == len-3 );
  q.enqueue(ws[14]);
  BOOST_CHECK( q.length() == len-2 );
  BOOST_CHECK( q.dequeue() == ws[5] );
  BOOST_CHECK( q.length() == len-3 );

  BOOST_MESSAGE( "done" );
}

BOOST_AUTO_TEST_SUITE_END();
