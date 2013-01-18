
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Message.hpp"

BOOST_AUTO_TEST_SUITE( Message_tests );


struct Check {
  bool x;
  Check() : x( false ) {}
  void operator()() { BOOST_CHECK_EQUAL( x, true ); }
};

void user_main( void * args ) 
{

  LOG(INFO) << "Test -2";
  {
    bool x0 = false;
    {
      // must run on this node
      auto f0 = [&x0]{ BOOST_CHECK_EQUAL( x0, true ); };
      Grappa::Message< decltype(f0) > m0( 0, f0 );
      //auto m1 = Grappa::message( 0, [&]{ x1 = true; } );
      m0.send();
      x0 = true;
    }
    Grappa_flush( 0 );
    Grappa_poll( );

    LOG(INFO) << "Test -1";
    x0 = false;
    {
      // must run on this node
      auto m0 = Grappa::message( 0, [&x0]{ BOOST_CHECK_EQUAL( x0, true ); } );
      m0.send();
      x0 = true;
    }
    Grappa_flush( 0 );
    Grappa_poll( );

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
    Grappa_flush( 0 );
    Grappa_poll( );
  }

  LOG(INFO) << "Test 1";
  {
    bool x1 = false;
    {
      // must run on this node
      auto f1 = [&x1]{ x1 = true; };
      Grappa::Message< decltype(f1) > m1( 0, f1 );
      //auto m1 = Grappa::message( 0, [&]{ x1 = true; } );
      m1.send();
    }
    Grappa_flush( 0 );
    Grappa_poll();
    BOOST_CHECK_EQUAL( x1, true );
  }

  LOG(INFO) << "Test 2";
  {
    // must run on this node
    Grappa::Message< Check > m2( 0, Check() );
    //auto m2 = Grappa::message( 0, Check() );
    m2->x = true;
    m2.send();
    Grappa_flush( 0 );
    Grappa_poll();
  }

  LOG(INFO) << "Test 3";
  {
    Check x3;
    //auto m3 = Grappa::message( 0, &x3 );
    // must run on this node
    Grappa::ExternalMessage< Check > m3( 0, &x3 );
    m3->x = true;
    m3.send();
    Grappa_flush( 0 );
    Grappa_poll();
  }

  LOG(INFO) << "Test 4";
  {
    bool x4 = false;
    {
      //auto m4 = Grappa::send_message( 0, [&]{ x4 = true; } );
      // must run on this node
      auto f4 = [&]{ x4 = true; };
      Grappa::SendMessage< decltype(f4) > m4( 0, f4 );
    }
    Grappa_flush( 0 );
    Grappa_poll();
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
    Grappa_flush( 0 );
    Grappa_poll();

    {
      x5 = true;
      Grappa::SendPayloadMessage< decltype(f5) > m5a( 0, f5, &x5, sizeof(x5) );
    }
    Grappa_flush( 0 );
    Grappa_poll();
  }

  //Grappa_merge_and_dump_stats();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
	       &(boost::unit_test::framework::master_test_suite().argv),
	       (1L << 20) );

  //Grappa::impl::global_rdma_aggregator.init();
  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

