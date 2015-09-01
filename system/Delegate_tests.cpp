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

/// Tests for delegate operations

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Delegate.hpp"
#include "GlobalAllocator.hpp"

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( Delegate_tests );

int64_t some_data = 1234;

double some_double = 123.0;

int64_t other_data __attribute__ ((aligned (2048))) = 0;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    
    // BOOST_CHECK_EQUAL( 2, Grappa::cores() );
    // try read
    some_data = 1111;
    int64_t remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 1234, remote_data );
    BOOST_CHECK_EQUAL( 1111, some_data );
  
    // write
    Grappa::delegate::write( make_global(&some_data,1), 2345 );
    BOOST_CHECK_EQUAL( 1111, some_data );
  
    // verify write
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 2345, remote_data );

    // fetch and add
    remote_data = delegate::fetch_and_add( make_global(&some_data,1), 1 );
    BOOST_CHECK_EQUAL( 1111, some_data );
    BOOST_CHECK_EQUAL( 2345, remote_data );
  
    // verify write
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 2346, remote_data );

    // check compare_and_swap
    bool swapped;
    swapped = delegate::compare_and_swap( make_global(&some_data,1), 123, 3333); // shouldn't swap
    BOOST_CHECK_EQUAL( swapped, false );
    // verify value is unchanged
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 2346, remote_data );
  
    // now actually do swap
    swapped = delegate::compare_and_swap( make_global(&some_data,1), 2346, 3333);
    BOOST_CHECK_EQUAL( swapped, true );
    // verify value is changed
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 3333, remote_data );
  
    // try linear global address
    
    // initialize
    auto i64_per_block = block_size / sizeof(int64_t);
    
    call_on_all_cores([]{ other_data = mycore(); });
    
    int * foop = new int;
    *foop = 1234;
    BOOST_MESSAGE( *foop );
    

    // hack the test
    // void* prev_base = Grappa::impl::global_memory_chunk_base;
    call_on_all_cores([]{
      Grappa::impl::global_memory_chunk_base = 0;
    });

    // make address
    BOOST_MESSAGE( "pointer is " << &other_data );

    GlobalAddress< int64_t > la = make_linear( &other_data );
    
    // check pointer computation
    BOOST_CHECK_EQUAL( la.core(), 0 );
    BOOST_CHECK_EQUAL( la.pointer(), &other_data );

    // check data
    BOOST_CHECK_EQUAL( 0, other_data );
    remote_data = delegate::read( la );
    BOOST_CHECK_EQUAL( 0, remote_data );
    
    // change pointer and check computation
    ++la;
    BOOST_CHECK_EQUAL( la.core(), 0 );
    BOOST_CHECK_EQUAL( la.pointer(), &other_data + 1 );

    // change pointer and check computation
    la += (i64_per_block-1);
    BOOST_CHECK_EQUAL( la.core(), 1 );
    
    // check remote data
    remote_data = delegate::read( la );
    BOOST_CHECK_EQUAL( 1, remote_data );

    // check template read
    // try read
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 3333, remote_data );
    
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
