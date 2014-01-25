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
