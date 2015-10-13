////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
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
