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
