
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "RDMAAggregator.hpp"
#include "Message.hpp"

BOOST_AUTO_TEST_SUITE( RDMAAggregator_tests );


int count = 0;

void user_main( void * args ) {
  if (true) {
    auto m = Grappa::message( 0, []{ LOG(INFO) << "Test message 1a: count = " << count++; } );
    auto n = Grappa::message( 0, []{ LOG(INFO) << "Test message 2a: count = " << count++; } );
    auto o = Grappa::message( 1, []{ LOG(INFO) << "Test message 3a: count = " << count++; } );
    auto p = Grappa::message( 1, []{ LOG(INFO) << "Test message 4a: count = " << count++; } );

    LOG(INFO) << "Sending m";
    m.send_immediate();
    LOG(INFO) << "Sending n";
    n.send_immediate();
    o.send_immediate();
    p.send_immediate();
  }

  if (true) {
    auto o = Grappa::message( 1, []{ LOG(INFO) << "Test message 1b: count = " << count++; } );
    auto p = Grappa::message( 1, []{ LOG(INFO) << "Test message 2b: count = " << count++; } );
    count += 2;
  
    o.enqueue();
    p.enqueue();

    LOG(INFO) << "Flushing 1";
    Grappa::impl::global_rdma_aggregator.flush( 1 );
  }

  if (true) {
    auto m = Grappa::send_message( 0, []{ LOG(INFO) << "Test message 1c: count = " << count++; } );
    auto n = Grappa::send_message( 0, []{ LOG(INFO) << "Test message 2c: count = " << count++; } );
    auto o = Grappa::send_message( 1, []{ LOG(INFO) << "Test message 3c: count = " << count++; } );
    auto p = Grappa::send_message( 1, []{ LOG(INFO) << "Test message 4c: count = " << count++; } );

    LOG(INFO) << "Flushing 0";
    Grappa::impl::global_rdma_aggregator.flush( 0 );

    LOG(INFO) << "Flushing 1";
    Grappa::impl::global_rdma_aggregator.flush( 1 );
  }


  Grappa_merge_and_dump_stats();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
               &(boost::unit_test::framework::master_test_suite().argv),
               (1L << 20) );

  Grappa::impl::global_rdma_aggregator.init();
  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  BOOST_CHECK_EQUAL( count, 6 );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

