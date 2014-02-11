////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

/// Tests for communicator

#include "Communicator.hpp"

/*
 * tests
 */
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( Communicator_tests );

bool success = false;
void foo( gasnet_token_t token ){
  success = true;
}


void bar( gasnet_token_t token, char * buf, size_t len ){
  int64_t sum = 0;
  for( int i = 0; i < len; ++i ) {
    //printf("%d: %d\n", i, buf[ i ] );
    sum += buf[ i ];
  }
  BOOST_CHECK_EQUAL( len, gasnet_AMMaxMedium() );
  BOOST_CHECK_EQUAL( sum, len );
  success = (sum == len);
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Communicator s;
  s.init( &(boost::unit_test::framework::master_test_suite().argc),
          &(boost::unit_test::framework::master_test_suite().argv) );
  
  //  int foo_h = s.register_active_message( reinterpret_cast< HandlerPointer >( &foo ) );
  int foo_h = s.register_active_message_handler( &foo );
  int bar_h = s.register_active_message_handler( &bar );
  s.activate();

  // make sure we've registered the handler properly and gasnet can call it
  BOOST_CHECK_EQUAL( success, false );
  gasnet_AMRequestShort0( s.mycore(), foo_h );
  BOOST_CHECK_EQUAL( success, true );

    // make sure we've registered the handler properly and we can call it
  success = false;
  s.send( s.mycore(), foo_h, NULL, 0 );
  BOOST_CHECK_EQUAL( success, true );


  // make sure we can pass data to a handler
  success = false;
  const size_t bardata_size = gasnet_AMMaxMedium();
  char bardata[ bardata_size ];
  memset( &bardata[0], 1, bardata_size );
  s.send( s.mycore(), bar_h, &bardata[0], bardata_size );
  BOOST_CHECK_EQUAL( success, true );

  s.finish();

}

BOOST_AUTO_TEST_SUITE_END();

