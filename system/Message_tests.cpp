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

#define LEGACY_SEND

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Message.hpp"
#include "ConditionVariable.hpp"

BOOST_AUTO_TEST_SUITE( Message_tests );


struct Check {
  bool x;
  Check() : x( false ) {}
  void operator()() { BOOST_CHECK_EQUAL( x, true ); }
};

void user_main( void * args ) 
{

  LOG(INFO) << "Test -2";
  bool x0 = false;
  {
    {
      // must run on this node
      auto f0 = [&x0]{ BOOST_CHECK_EQUAL( x0, true ); };
      Grappa::Message< decltype(f0) > m0( 0, f0 );
      //auto m1 = Grappa::message( 0, [&]{ x1 = true; } );
      m0.enqueue();
      x0 = true;
    }
    Grappa::flush( 0 );
    Grappa::impl::poll( );
  }

  LOG(INFO) << "Test -1";
  x0 = false;
  {
    // must run on this node
    auto m0 = Grappa::message( 0, [&x0]{ BOOST_CHECK_EQUAL( x0, true ); } );
    m0.enqueue();
    x0 = true;
  }
  Grappa::flush( 0 );
  Grappa::impl::poll( );
  
  LOG(INFO) << "Test 0";
  x0 = false;
  {
    // must run on this node
    auto m0 = Grappa::send_message( 0, [&x0]{ BOOST_CHECK_EQUAL( x0, true ); } );
    
    // //auto l0 = [&x0]{ BOOST_CHECK_EQUAL( x0, true ); };
    // // Grappa::SendMessage< decltype(l0) > m0( 0, l0 );
    
    // auto l0a = [&x0](void * useless, size_t nothing){ BOOST_CHECK_EQUAL( x0, true ); };
    // int foo[10];
    // auto m0 = Grappa::send_message( 0, l0a, foo, sizeof(foo) );
    
    x0 = true;
  }
  Grappa::flush( 0 );
  Grappa::impl::poll( );
  

  LOG(INFO) << "Test 1";
  {
    bool x1 = false;
    {
      // must run on this node
      auto f1 = [&x1]{ x1 = true; };
      Grappa::Message< decltype(f1) > m1( 0, f1 );
      //auto m1 = Grappa::message( 0, [&]{ x1 = true; } );
      m1.enqueue();
    }
    Grappa::flush( 0 );
    Grappa::impl::poll();
    BOOST_CHECK_EQUAL( x1, true );
  }
  
  LOG(INFO) << "Test 2";
  {
    // must run on this node
    Grappa::Message< Check > m2( 0, Check() );
    //auto m2 = Grappa::message( 0, Check() );
    m2->x = true;
    m2.enqueue();
    Grappa::flush( 0 );
    Grappa::impl::poll();
  }

  LOG(INFO) << "Test 4";
  {
    bool x4 = false;
    {
      //auto m4 = Grappa::send_message( 0, [&]{ x4 = true; } );
      // must run on this node
      auto f4 = [&]{ x4 = true; };
      auto m4 = Grappa::send_message( 0, f4 );
    }
    Grappa::flush( 0 );
    Grappa::impl::poll();
    BOOST_CHECK_EQUAL( x4, true );
  }

  LOG(INFO) << "Test 5";
  {
    bool x5 = false;
    // must run on this node
    auto f5 = [&x5] (void * payload, size_t payload_size) { 
      bool * b = reinterpret_cast< bool * >( payload );
      BOOST_CHECK_EQUAL( *b, x5 );
    };

    {
      //Grappa::SendPayloadMessage< decltype(f5) > m5( 0, f5, &x5, sizeof(x5) );
      auto m5 = Grappa::send_message( 0, f5, &x5, sizeof(x5) );
    }
    Grappa::flush( 0 );
    Grappa::impl::poll();

    {
      x5 = true;
      auto m5a = Grappa::send_message( 0, f5, &x5, sizeof(x5) );
    }
    Grappa::flush( 0 );
    Grappa::impl::poll();
  }

  LOG(INFO) << "Test 6";
  {
    Grappa::ConditionVariable cv;
    Grappa::ConditionVariable * cvp = &cv;

    auto m6 = Grappa::send_heap_message( 1, [cvp] { 
        BOOST_CHECK_EQUAL( Grappa::mycore(), 1 );
        auto m6a = Grappa::send_heap_message( 0, [cvp] {
            BOOST_CHECK_EQUAL( Grappa::mycore(), 0 );
            Grappa::signal( cvp );
          } );
        Grappa::flush( 0 );
        Grappa::impl::poll();
      } );
    Grappa::flush( 1 );
    Grappa::impl::poll();
    // TODO: does this need to be protected with a while loop?
    Grappa::wait( cvp );
    LOG(INFO) << "Test 6 done";
  }

  //Grappa_merge_and_dump_stats();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
               &(boost::unit_test::framework::master_test_suite().argv),
               (1L << 20) );

  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

