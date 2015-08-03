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

#include "NTBuffer.hpp"

BOOST_AUTO_TEST_SUITE( NTBuffer_tests );

static const int fill = 14;
static const int max = 16;


struct tuple {
  int64_t a;
  int64_t b;
  int64_t c;
};

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::impl::NTBuffer b;
  LOG(INFO) << "Size of tuple is " << sizeof(tuple);
  for( int i = 0; i < fill; ++i ) {
    tuple x = { i*1000 + 0, i*1000 + 1, i*1000 + 2 };
    nt_enqueue( &b, &x, sizeof(x) );
  }

  nt_flush( &b );
  
  auto t = b.take_buffer();
  auto p = reinterpret_cast<tuple*>( std::get<0>( t ) );
  for( int i = 0; i < fill; ++i ) {
    BOOST_CHECK_EQUAL( p[i].a, i*1000 + 0 );
    BOOST_CHECK_EQUAL( p[i].b, i*1000 + 1 );
    BOOST_CHECK_EQUAL( p[i].c, i*1000 + 2 );
  }
  for( int i = fill; i < max; ++i ) {
    BOOST_CHECK_EQUAL( p[i].a, 0 );
    BOOST_CHECK_EQUAL( p[i].b, 0 );
    BOOST_CHECK_EQUAL( p[i].c, 0 );
  }
}

BOOST_AUTO_TEST_SUITE_END();
