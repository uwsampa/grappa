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

#include "Grappa.hpp"
#include "FullEmpty.hpp"
#include "CompletionEvent.hpp"
#include "Collective.hpp"
#include "Tasking.hpp"

BOOST_AUTO_TEST_SUITE( FullEmpty_tests );

using namespace Grappa;

void test_remote_fe(int trial) {
  bool perturb = trial % 2 == 0;
  
  FullEmpty<long> _fe;
  auto fe = make_global(&_fe);
  
  CompletionEvent _ce(3);
  auto ce = make_global(&_ce);
  
  CompletionEvent ce1(1);
  spawn([fe,ce,&ce1,trial]{
    ce1.complete();
    auto val = readFF(fe);
    BOOST_CHECK_EQUAL(val, 42);
    complete(ce);
    DVLOG(2) << "task<" << trial << ",1> done";
  });
   if (perturb) ce1.wait();
  
  send_message(1, [fe,ce,trial]{
    spawn([fe,ce,trial]{
      spawn([fe,ce,trial]{
        writeXF(fe, 42);
        complete(ce);
        DVLOG(2) << "task<" << trial << ",3> done";
      });
      
      long val = readFF(fe);
      BOOST_CHECK_EQUAL(val, 42);
      complete(ce);
      DVLOG(2) << "task<" << trial << ",2> done";
    });
  });
  // wait for other task to begin and start doing readFF
  // fe->writeEF(42);
  ce->wait();
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{

    CompletionEvent ce;
    for (int i=0; i<100; i++) {
      test_remote_fe(i);
    }
    ce.wait();
  
    Grappa::FullEmpty< int64_t > fe_int;

    BOOST_CHECK_EQUAL( fe_int.readXX(), 0 );

    fe_int.writeXF( 1 );

    BOOST_CHECK_EQUAL( fe_int.full(), true );
    BOOST_CHECK_EQUAL( fe_int.readXX(), 1 );

    BOOST_CHECK_EQUAL( fe_int.readFE(), 1 );
    fe_int.writeEF( 2 );
    BOOST_CHECK_EQUAL( fe_int.readFE(), 2 );
    fe_int.writeEF( 3 );
    BOOST_CHECK_EQUAL( fe_int.readFE(), 3 );
    fe_int.writeEF( 1 );

    return;

    Grappa::spawn( [&] { 
        int64_t temp = 0;
        while( (temp = fe_int.readFE()) != 1 ) { 
  	fe_int.writeEF( temp ); 
        }

        fe_int.writeEF( 2 );

        while( (temp = fe_int.readFE()) != 3 ) { 
  	fe_int.writeEF( temp ); 
        }

        fe_int.writeEF( 4 );

      } );

    int64_t temp = 0;
  
    while( (temp = fe_int.readFE()) != 2 ) { 
      fe_int.writeEF( temp ); 
    }
  
    fe_int.writeEF( 3 );
  
    while( (temp = fe_int.readFE()) != 4 ) { 
      fe_int.writeEF( temp ); 
    }
  
    fe_int.writeEF( 5 );

    BOOST_CHECK_EQUAL( fe_int.readFE(), 5 );

    Grappa::Metrics::merge_and_print();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();

