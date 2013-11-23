
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

/// Tests for delegate operations

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Delegate.hpp"

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( Delegate_tests );

int64_t some_data = 1234;

double some_double = 123.0;

int64_t other_data __attribute__ ((aligned (2048))) = 0;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    
    BOOST_CHECK_EQUAL( 2, Grappa_nodes() );
    // try read
    some_data = 1111;
    int64_t remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 1234, remote_data );
    BOOST_CHECK_EQUAL( 1111, some_data );
  
    // write
    Grappa::delegate::write( make_global(&some_data,1), 2345 );
    BOOST_CHECK_EQUAL( 1111, some_data );
  
    // verify write
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 2345, remote_data );

    // fetch and add
    remote_data = delegate::fetch_and_add( make_global(&some_data,1), 1 );
    BOOST_CHECK_EQUAL( 1111, some_data );
    BOOST_CHECK_EQUAL( 2345, remote_data );
  
    // verify write
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 2346, remote_data );

    // check compare_and_swap
    bool swapped;
    swapped = delegate::compare_and_swap( make_global(&some_data,1), 123, 3333); // shouldn't swap
    BOOST_CHECK_EQUAL( swapped, false );
    // verify value is unchanged
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 2346, remote_data );
  
    // now actually do swap
    swapped = delegate::compare_and_swap( make_global(&some_data,1), 2346, 3333);
    BOOST_CHECK_EQUAL( swapped, true );
    // verify value is changed
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 3333, remote_data );
  
    // try linear global address

    // initialize
    other_data = 0;
    Grappa::delegate::write( make_global(&other_data,1), 1 );

    int * foop = new int;
    *foop = 1234;
    BOOST_MESSAGE( *foop );
    

    // hack the test
    void* prev_base = Grappa::impl::global_memory_chunk_base;
    Grappa::impl::global_memory_chunk_base = 0;

    // make address
    BOOST_MESSAGE( "pointer is " << &other_data );
    GlobalAddress< int64_t > la = make_linear( &other_data );

    // check pointer computation
    BOOST_CHECK_EQUAL( la.node(), 0 );
    BOOST_CHECK_EQUAL( la.pointer(), &other_data );

    // check data
    BOOST_CHECK_EQUAL( 0, other_data );
    remote_data = delegate::read( la );
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
    remote_data = delegate::read( la );
    BOOST_CHECK_EQUAL( 1, remote_data );
  
    // check template read
    // try read
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 3333, remote_data );

  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
