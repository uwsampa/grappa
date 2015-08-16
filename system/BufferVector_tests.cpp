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

/// This file contains tests for the GlobalAddress<> class.

#include <boost/test/unit_test.hpp>

#include <Grappa.hpp>
#include "BufferVector.hpp"
#include "Delegate.hpp"

#define DOTEST(str) VLOG(1) << "TEST: " << (str);

BOOST_AUTO_TEST_SUITE( BufferVector_tests );

using namespace Grappa;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{

    DOTEST("basic insert") {
      BufferVector<int64_t> b(10);
      BOOST_CHECK_EQUAL( b.getLength(), 0 );

      const int64_t v1 = 11;
      b.insert( v1 );
      BOOST_CHECK_EQUAL( b.getLength(), 1 );

      b.setReadMode();
      GlobalAddress<const int64_t> vs = b.getReadBuffer();

      int64_t r = delegate::read( vs );
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
      GlobalAddress<const int64_t> vs = b.getReadBuffer();
      {
        int64_t r = delegate::read( vs+0 );
        BOOST_CHECK_EQUAL( *(vs.pointer()), v1);
        BOOST_CHECK_EQUAL( r, v1 );
      }
      {
        int64_t r = delegate::read( vs+1 );
        BOOST_CHECK_EQUAL( r, v2 );
      }
      {
        int64_t r = delegate::read( vs+2 );
        BOOST_CHECK_EQUAL( r, v3 );
      }
      {
        int64_t r = delegate::read( vs+3 );
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
      GlobalAddress<const int64_t> vs = b.getReadBuffer();
      for (uint64_t i=0; i<num; i++) {
        int64_t r = delegate::read( vs+i );
        BOOST_CHECK_EQUAL( r, vals[i] );
        BOOST_CHECK_EQUAL( b.getLength(), num );
      }
    }
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
