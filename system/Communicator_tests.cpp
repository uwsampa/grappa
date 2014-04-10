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
bool sent = false;

/// make sure we can send and receive a message locally
void check_local_communication( ) {
  success = false;
  sent = false;

  auto receive_handler = [] ( int source, int tag, void * buf, size_t size ) {
    LOG(INFO) << "Message received.";
    success = true;
  };

  auto send_handler = [] ( int source, int tag, void * buf, size_t size ) {
    LOG(INFO) << "Message sent.";
    sent = true;
  };

  //void (*callback)(int source, int tag, void * buf, size_t size ),

  char recv_buf[1024] = {0};
  LOG(INFO) << "Posting receive";
  global_communicator.post_receive( &recv_buf[0], 1024, receive_handler );
  global_communicator.poll();
  BOOST_CHECK_EQUAL( success, false );
  BOOST_CHECK_EQUAL( sent, false );

  
  char send_buf[1024] = {0};
  LOG(INFO) << "Posting send";
  global_communicator.post_send( 0, &send_buf[0], 1024, send_handler );
  while( !success ) {
    global_communicator.poll();
  }
  BOOST_CHECK_EQUAL( success, true );
  BOOST_CHECK_EQUAL( sent, false );

  global_communicator.garbage_collect();
  BOOST_CHECK_EQUAL( success, true );
  BOOST_CHECK_EQUAL( sent, true );

  global_communicator.poll();
  BOOST_CHECK_EQUAL( success, true );
  BOOST_CHECK_EQUAL( sent, true );
}

BOOST_AUTO_TEST_CASE( test1 ) {
  google::ParseCommandLineFlags( &(boost::unit_test::framework::master_test_suite().argc),
                                 &(boost::unit_test::framework::master_test_suite().argv), true );
  google::InitGoogleLogging( boost::unit_test::framework::master_test_suite().argv[0] );

  global_communicator.init( &(boost::unit_test::framework::master_test_suite().argc),
             &(boost::unit_test::framework::master_test_suite().argv) );
  global_communicator.activate();

  // // make sure we've registered the handler properly and gasnet can call it
  // BOOST_CHECK_EQUAL( success, false );
  // gasnet_AMRequestShort0( s.mycore(), foo_h );
  // BOOST_CHECK_EQUAL( success, true );

  //   // make sure we've registered the handler properly and we can call it
  // success = false;
  // s.send( s.mycore(), foo_h, NULL, 0 );
  // BOOST_CHECK_EQUAL( success, true );


  // // make sure we can pass data to a handler
  // success = false;
  // const size_t bardata_size = gasnet_AMMaxMedium();
  // char bardata[ bardata_size ];
  // memset( &bardata[0], 1, bardata_size );
  // s.send( s.mycore(), bar_h, &bardata[0], bardata_size );
  // BOOST_CHECK_EQUAL( success, true );

  if( Grappa::mycore() == 0 ) {
    check_local_communication( );
  }
  
  
  BOOST_CHECK_EQUAL( true, true );

  global_communicator.finish();
}

BOOST_AUTO_TEST_SUITE_END();

