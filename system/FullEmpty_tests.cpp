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

