
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "LocaleSharedMemory.hpp"
#include "ParallelLoop.hpp"

BOOST_AUTO_TEST_SUITE( LocaleSharedMemory_tests );

void user_main( void * args ) 
{

  int64_t * arr = NULL;

  LOG(INFO) << "Allocating";
  
  // allocate array
  if( Grappa::node_mycore() == 0 ) {
    // we need to do this on each node (not core)
    arr = static_cast< int64_t* >( Grappa::impl::locale_shared_memory.segment.allocate_aligned( sizeof(int64_t) * Grappa::node_cores(), 1<<3 ) );
    for( int i = 0; i < Grappa::node_cores(); ++i ) {
      arr[i] = 0;
    }
  }

  LOG(INFO) << "Setting address";
  Grappa::on_all_cores( [arr] {
      int other_index = Grappa::node_cores() - Grappa::node_mycore() - 1;
      arr[ other_index ] = Grappa::node_mycore();
    });

  LOG(INFO) << "Checking";
  Grappa::on_all_cores( [arr] {
      int other_index = Grappa::node_cores() - Grappa::node_mycore() - 1;
      BOOST_CHECK_EQUAL( arr[ Grappa::node_mycore() ], other_index );
    });

  LOG(INFO) << "Done";
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
               &(boost::unit_test::framework::master_test_suite().argv),
               1 << 23 );

  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*) NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

