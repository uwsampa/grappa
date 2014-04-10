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

DECLARE_int64( log2_concurrent_receives );

BOOST_AUTO_TEST_SUITE( Communicator_tests );

bool success = false;
bool sent = false;

int send_count = 1 << 22;
int receive_count = 0;


void ping_test() {
  success = false;

  int target = (Grappa::mycore() + ( Grappa::cores() / 2 ) ) % Grappa::cores();

  double start = MPI_Wtime();
  MPI_CHECK( MPI_Barrier( MPI_COMM_WORLD ) );

  if( Grappa::mycore() < Grappa::cores() / 2 ) {
    for( int i = 0; i < send_count; ++i ) {
      global_communicator.send_immediate( target, [] {
          receive_count++;
          DVLOG(1) << "Receive count now " << receive_count;
        });
    }
  } else {
    while( receive_count != send_count ) {
      global_communicator.poll();
    }
  }

  DVLOG(1) << "Done.";
  
  MPI_CHECK( MPI_Barrier( MPI_COMM_WORLD ) );
  double end = MPI_Wtime();

  if( Grappa::mycore() >= Grappa::cores() / 2 ) {
    BOOST_CHECK_EQUAL( send_count, receive_count );
  }

  if( Grappa::mycore() == 0 ) {
    double time = end - start;
    double rate = send_count * ( Grappa::cores() / 2 ) / time;
    LOG(INFO) << send_count << " messages in "
              << time << " seconds: "
              << rate << " msgs/s";
  }
}


BOOST_AUTO_TEST_CASE( test1 ) {
  google::ParseCommandLineFlags( &(boost::unit_test::framework::master_test_suite().argc),
                                 &(boost::unit_test::framework::master_test_suite().argv), true );
  google::InitGoogleLogging( boost::unit_test::framework::master_test_suite().argv[0] );

  global_communicator.init( &(boost::unit_test::framework::master_test_suite().argc),
             &(boost::unit_test::framework::master_test_suite().argv) );
  global_communicator.activate();

  ping_test();

  MPI_CHECK( MPI_Barrier( MPI_COMM_WORLD ) );
  
  BOOST_CHECK_EQUAL( true, true );

  global_communicator.finish();
}

BOOST_AUTO_TEST_SUITE_END();

