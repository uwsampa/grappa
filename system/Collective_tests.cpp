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
#include "Communicator.hpp"
#include "Collective.hpp"
#include "GlobalAllocator.hpp"
#include "Addressing.hpp"

// Tests the functions in Collective.hpp

BOOST_AUTO_TEST_SUITE( Collective_tests );

using namespace Grappa;

static int global_x;

struct TestObj {
  int64_t ignore;
  int64_t c;

} GRAPPA_BLOCK_ALIGNED;
TestObj operator+( const TestObj& o1, const TestObj& o2 ) {
  return {0, o1.c + o2.c};
}

int64_t accessObj(GlobalAddress<TestObj> g) {
  return g->c;
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    BOOST_MESSAGE("testing allreduce");
    Grappa::on_all_cores([]{
      int x = 7;
      int total_x = Grappa::allreduce<int,collective_add>(x);
      BOOST_CHECK_EQUAL(total_x, 7*Grappa::cores());
      
      global_x = Grappa::mycore() + 1;
    });

    BOOST_MESSAGE("testing reduce");
    int total_x = Grappa::reduce<int,collective_add>(&global_x);
    Core n = Grappa::cores();
    BOOST_CHECK_EQUAL(total_x, n*(n+1)/2);

    GlobalAddress<TestObj> replIntAddr = Grappa::symmetric_global_alloc<TestObj>();
    Grappa::on_all_cores([replIntAddr] {
      replIntAddr->c = Grappa::mycore() + 1;
    });
    BOOST_MESSAGE("testing dynamic reduce");
    {
      auto result = Grappa::reduce<TestObj,collective_add>(replIntAddr);
      BOOST_CHECK_EQUAL(result.c, n*(n+1)/2);
    }

    BOOST_MESSAGE("testing dynamic reduce with accessor");
    {
      int64_t result = Grappa::reduce<int64_t, TestObj,collective_add,accessObj>(replIntAddr);
      BOOST_CHECK_EQUAL(result, n*(n+1)/2);
    }


    // Test fails: sadly localization of a member of block-aligned does not work
    //BOOST_MESSAGE("testing dynamic reduce without accessor");
    //{
    //  int64_t result = Grappa::reduce<int64_t,collective_add>(global_pointer_to_member(replIntAddr, &TestObj::c) );
    //  BOOST_CHECK_EQUAL(result, n*(n+1)/2);
    //}
  
    BOOST_MESSAGE("testing allreduce_inplace");
    Grappa::on_all_cores([]{
      const int N = 1024;
      int xs[N];
      for (int i=0; i<N; i++) xs[i] = i;
    
      Grappa::allreduce_inplace<int,collective_add>(xs, N);
    
      for (int i=0; i<N; i++) BOOST_CHECK_EQUAL(xs[i], Grappa::cores() * i);
    });
    
    Grappa::call_on_all_cores([]{ global_x = 1; });
    
    auto total = Grappa::sum_all_cores([]{ return global_x; });
    CHECK_EQ(total, cores());
    
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();

