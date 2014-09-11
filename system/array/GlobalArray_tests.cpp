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
#include <Grappa.hpp>
#include <array/GlobalArray.hpp>

BOOST_AUTO_TEST_SUITE( GlobalArray_tests );

using namespace Grappa;

//DEFINE_uint64(x, 1000, "size of array in X dimension");
//DEFINE_uint64(y, 1000, "size of array in Y dimension");
//DEFINE_uint64(iterations, 1, "number of iterations");

GlobalArray< double, Distribution::Block, Distribution::Local > ga;

BOOST_AUTO_TEST_CASE( test1 ) {
  init( GRAPPA_TEST_ARGS );
  run([]{
      size_t s = static_cast<int64_t>(Grappa::cores()) * 1000;
      ga.allocate( s, s );
      
      DVLOG(1) << typename_of(ga);
      DVLOG(1) << typename_of(ga[1]);
      DVLOG(1) << typename_of(ga[1][2]);
      DVLOG(1) << typename_of(&ga[1][2]);
      DVLOG(1) << typename_of(static_cast<double>(ga[1][2]));
      
      // initialize with localized iteration
      auto gaga = make_global( &ga );
      forall( gaga, [s] (size_t x, size_t y, double& d) {
          d = x * s + y;
        } );
      
      // check and increment with dereferences
      forall( 0, s, [s] (int64_t x) {
          forall<async>( 0, s, [x] (int64_t y) {
              auto addr = &ga[x][y];
              DVLOG(1) << "At (" << x << "," << y << ") &d == " << addr;
              delegate::call<async>( addr, [x,y] (double & d) {
                  DVLOG(1) << "At (" << x << "," << y << ") d == " << d;
                  d += 1;
                } );
            } );
        } );
      
      // verify increment with localized iteration
      forall( ga, [s] (size_t x, size_t y, double& d) {
          CHECK_EQ( d, x * s + y + 1);
          BOOST_CHECK_EQUAL( d, x * s + y + 1);
        } );
      
      ga.deallocate( );
    });
  finalize();
}

BOOST_AUTO_TEST_SUITE_END();
