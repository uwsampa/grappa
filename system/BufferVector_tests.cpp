
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

/// This file contains tests for the GlobalAddress<> class.

#include <boost/test/unit_test.hpp>

#include <Grappa.hpp>
#include "BufferVector.hpp"

#define DOTEST(str) VLOG(1) << "TEST: " << (str);

BOOST_AUTO_TEST_SUITE( BufferVector_tests );

void user_main( int * ignore ) {

  DOTEST("basic insert") {
    BufferVector<int64_t> b(10);
    BOOST_CHECK_EQUAL( b.getLength(), 0 );

    const int64_t v1 = 11;
    b.insert( v1 );
    BOOST_CHECK_EQUAL( b.getLength(), 1 );

    b.setReadMode();
    GlobalAddress<int64_t> vs = b.getReadBuffer();
    int64_t r;
    Grappa_delegate_read( vs, &r );
    BOOST_CHECK_EQUAL( r, v1 );
  }

  DOTEST("one capacity growth") {
    BufferVector<int64_t> b(4);
    BOOST_CHECK_EQUAL( b.getLength(), 0 );

    const int64_t v1 = 11;
    b.insert( v1 );
    BOOST_CHECK_EQUAL( b.getLength(), 1 );
    
    const int64_t v2 = 22;
    b.insert( v2 );
    BOOST_CHECK_EQUAL( b.getLength(), 2 );
    
    const int64_t v3 = 33;
    b.insert( v3 );
    BOOST_CHECK_EQUAL( b.getLength(), 3 );
    
    const int64_t v4 = 44;
    b.insert( v4 );
    BOOST_CHECK_EQUAL( b.getLength(), 4 );

    b.setReadMode();
    GlobalAddress<int64_t> vs = b.getReadBuffer();
    {
      int64_t r;
      Grappa_delegate_read( vs+0, &r );
      BOOST_CHECK_EQUAL( *(vs.pointer()), v1);
      BOOST_CHECK_EQUAL( r, v1 );
    }
    {
      int64_t r;
      Grappa_delegate_read( vs+1, &r );
      BOOST_CHECK_EQUAL( r, v2 );
    }
    {
      int64_t r;
      Grappa_delegate_read( vs+2, &r );
      BOOST_CHECK_EQUAL( r, v3 );
    }
    {
      int64_t r;
      Grappa_delegate_read( vs+3, &r );
      BOOST_CHECK_EQUAL( r, v4 );
    }
  }

  DOTEST("more capacity growth") {
    BufferVector<int64_t> b(2);
    BOOST_CHECK_EQUAL( b.getLength(), 0 );

    const uint64_t num = 222;
    int64_t vals[num];
    for (uint64_t i=0; i<num; i++) {
      vals[i] = i*2;
      b.insert(vals[i]);
      BOOST_CHECK_EQUAL( b.getLength(), i+1 );
   //   BOOST_MESSAGE( "insert and now " << b );
    }
    
    b.setReadMode();
    GlobalAddress<int64_t> vs = b.getReadBuffer();
    for (uint64_t i=0; i<num; i++) {
      int64_t r;
      Grappa_delegate_read( vs+i, &r );
      BOOST_CHECK_EQUAL( r, vals[i] );
      BOOST_CHECK_EQUAL( b.getLength(), num );
    }
  }
}


BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  Grappa_activate();

  DVLOG(1) << "Spawning user main Thread....";
  Grappa_run_user_main( &user_main, (int*)NULL );
  VLOG(5) << "run_user_main returned";
  CHECK( Grappa_done() );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

