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
#include "GlobalAllocator.hpp"
#include "Reducer.hpp"
#include "Barrier.hpp"

BOOST_AUTO_TEST_SUITE( Reducer_tests );

using namespace Grappa;
//#define TEST_MSG BOOST_MESSAGE( "In test " << __func__ );
#define TEST(name) void test_##name()
#define RUNTEST(name) BOOST_MESSAGE( "== Test " << #name << " =="); test_##name()

TEST(int_add) {
  int64_t expected = Grappa::cores(); 
  on_all_cores( [expected] {
    AllReducer<int64_t, collective_add> r( 0 );
    r.reset();
    r.accumulate( 1 );

    barrier();

    int64_t result = r.finish();
    BOOST_CHECK( result == expected );
  });
}

TEST(int_add_more) {
  int64_t nc = Grappa::cores();
  int64_t expected = nc*(nc+1)*(2*nc+1)/6;
  on_all_cores( [expected] {
    AllReducer<int64_t, collective_add> r( 0 );
    r.reset();
    for (int i=0; i<Grappa::mycore()+1; i++) {
      r.accumulate( Grappa::mycore()+1 );
    }

    barrier();

    int64_t result = r.finish();
    BOOST_CHECK( result == expected );
  });
}

TEST(int_max) {
  int64_t expected = 1; 
  on_all_cores( [expected] {
    AllReducer<int64_t, collective_max> r( 0 );
    r.reset();
    r.accumulate( -1*Grappa::mycore() + 1  );

    barrier();

    int64_t result = r.finish();
    BOOST_CHECK( result == expected );
  });
}


TEST(reduction) {
  int64_t nc = Grappa::cores();
  int64_t expected = 6 * nc;

  auto dummy = global_alloc<int64_t>(expected);
  int64_t result = reduction<int64_t,AllReducer<int64_t,collective_add>>(0, [=](GlobalAddress<AllReducer<int64_t,collective_add>> r) {
    forall(dummy, expected, [=](int64_t i, int64_t& ignore) {
        r->accumulate( 1 );
        });
    });
    BOOST_CHECK( result == expected );
}

struct BigT {
  int64_t x;
  BigT() : x(0) {}
  BigT(int64_t y) : x(y) {}
  BigT operator+=(const BigT& rhs) {
    this->x += rhs.x;
  }
  BigT operator+(const BigT& rhs) const {
    BigT r(*this);
    r += rhs;
    return r;
  }
} GRAPPA_BLOCK_ALIGNED;

TEST(reduction_big) {
  int64_t nc = Grappa::cores();
  int64_t expected = 6 * nc;

  auto dummy = global_alloc<BigT>(expected);
  BigT result = reduction<BigT,AllReducer<BigT,collective_add>>(BigT(0), [=](GlobalAddress<AllReducer<BigT,collective_add>> r) {
    forall(dummy, expected, [=](int64_t i, BigT& ignore) {
        r->accumulate( BigT(1) );
        });
    });
    BOOST_CHECK( result.x == expected );
}


  
SimpleSymmetric<bool> s_active;
SimpleSymmetric<int> s_count;

Reducer<int,ReducerType::Add> count;
Reducer<bool,ReducerType::Or> active;

using C = CmpElement<int,double>;
Reducer<C,ReducerType::Max> best;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    BOOST_CHECK(Grappa::cores() >= 2); // at least 2 nodes for these tests...

    RUNTEST(int_add);
    RUNTEST(int_add_more);
    RUNTEST(int_max);
    RUNTEST(reduction);
    RUNTEST(reduction_big);
    
    BOOST_MESSAGE("== Test SimpleSymmetric<T> ==");
    set(s_active, false);
    BOOST_CHECK_EQUAL(any(s_active), false);
    
    s_active |= true;
    BOOST_CHECK_EQUAL(any(s_active), true);
    BOOST_CHECK_EQUAL(all(s_active), false);
    
    set(s_count, 0);
    BOOST_CHECK_EQUAL(sum(s_count), 0);
    
    s_count += 1;
    BOOST_CHECK_EQUAL(sum(s_count), 1);
    
    call_on_all_cores([]{ s_count += 1; });
    BOOST_CHECK_EQUAL(sum(s_count), cores()+1);
    
    BOOST_MESSAGE("# Test Reducer<T>");
    BOOST_MESSAGE("## Test Reducer<T,Add>");
    
    BOOST_CHECK_EQUAL(count, 0);
    
    count++;
    BOOST_CHECK_EQUAL(count, 1);
    
    on_all_cores([]{ count++; });
    BOOST_CHECK_EQUAL(count, cores()+1);
    
    BOOST_MESSAGE("## Test Reducer<T,Or>");
    BOOST_CHECK(!active);
    
    active |= true;
    BOOST_CHECK(active);
    
    active = false;
    BOOST_CHECK(!active);

    BOOST_MESSAGE("## Test Reducer<T,Max>");
    on_all_cores([]{ best << C(mycore(), 3.0*mycore()); });
    BOOST_CHECK_EQUAL(static_cast<C>(best).idx(), cores()-1);
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
