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

#include <boost/test/unit_test.hpp>
#include "ThreadQueue.hpp"
#include <iostream>

BOOST_AUTO_TEST_SUITE( ThreadQueue_tests );

using namespace Grappa;

BOOST_AUTO_TEST_CASE( test1 ) {
  // #ifndef DEBUG
  //   BOOST_MESSAGE( "This test requires -DDEBUG for full checks" );
  //   BOOST_CHECK( false );
  // #endif

  Worker * ws[100];
  for(int i=0; i< 100; i++) {
    ws[i] = new Worker();
  }

  PrefetchingThreadQueue q;
  q.init(4);

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
