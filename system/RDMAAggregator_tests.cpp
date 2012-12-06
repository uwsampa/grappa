
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "RDMAAggregator.hpp"

BOOST_AUTO_TEST_SUITE( RDMAAggregator_tests );


void user_main( void * args ) 
{
  auto m = Grappa::scopedMessage( 0, []{ std::cout << "Test message" << std::endl; } );

  Grappa::impl::global_rdma_aggregator.enqueue( &m );
  Grappa::impl::global_rdma_aggregator.flush( 0 );
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
	       &(boost::unit_test::framework::master_test_suite().argv),
	       (1L << 20) );

  Grappa::impl::global_rdma_aggregator.init();
  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

