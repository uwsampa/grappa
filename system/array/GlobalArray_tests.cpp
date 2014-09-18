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

GlobalArray< double, Distribution::Local, Distribution::Block > ga;

BOOST_AUTO_TEST_CASE( test1 ) {
  init( GRAPPA_TEST_ARGS );
  run([]{
      size_t s = 10; //static_cast<int64_t>(Grappa::cores()) * 10 - 1;
      ga.allocate( s, s );
      
      DVLOG(1) << typename_of(ga);
      DVLOG(1) << typename_of(ga[1]);
      DVLOG(1) << typename_of(ga[1][2]);
      DVLOG(1) << typename_of(&ga[1][2]);
      DVLOG(1) << typename_of(static_cast<double>(ga[1][2]));
      
      // verify that indexing results in the same addresses as iteration
      forall( ga, [s] (size_t x, size_t y, double& d) {
          Core mycore = Grappa::mycore();
          LOG(INFO) << "x " << x << " y " << y << " core " << mycore << " addr " << &d;
          auto p = &ga[x][y];
          CHECK_EQ( mycore, p.core() ) << "at x=" << x << " y=" << y << " with pointer " << p;
          CHECK_EQ( &d, p.pointer() ) << "at x=" << x << " y=" << y << " with pointer " << p;
          //CHECK_EQ( make_linear(&d), &ga[x][y] ) << "at x=" << x << " y=" << y;
          //BOOST_CHECK_EQUAL( make_linear(&d), &ga[x][y] );
          BOOST_CHECK_EQUAL( &d, p.pointer() );
          BOOST_CHECK_EQUAL( mycore, p.core() );
        } );
      
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
          BOOST_CHECK_EQUAL( d, x * s + y + 1);
          CHECK_EQ( d, x * s + y + 1);
        } );
      
      ga.deallocate( );
    });
  finalize();
}

BOOST_AUTO_TEST_SUITE_END();
