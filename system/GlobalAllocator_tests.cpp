
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <sys/mman.h>

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Delegate.hpp"
#include "GlobalMemoryChunk.hpp"
#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"

BOOST_AUTO_TEST_SUITE( GlobalAllocator_tests );

const size_t local_size_bytes = 1 << 14;

void user_main( void * ignore ) {
  Grappa::on_all_cores([] {
    GlobalAddress< int64_t > a = Grappa_malloc( 1 );
    LOG(INFO) << "got pointer " << a.pointer();

    GlobalAddress< int64_t > b = Grappa_typed_malloc< int64_t >( 1 );
    LOG(INFO) << "got pointer " << b.pointer();

    GlobalAddress< int64_t > c = Grappa_malloc( 8 );
    LOG(INFO) << "got pointer " << c.pointer();

    GlobalAddress< int64_t > d = Grappa_malloc( 1 );
    LOG(INFO) << "got pointer " << d.pointer();

//    LOG(INFO) << *global_allocator;

    if( Grappa_mynode() == 0 ) {
      BOOST_CHECK_EQUAL( global_allocator->total_bytes(), local_size_bytes );
      BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 1 + 8 + 8 + 1 );
    }

    LOG(INFO) << "freeing pointer " << c.pointer();
    Grappa_free( c );

    LOG(INFO) << "freeing pointer " << a.pointer();
    Grappa_free( a );

    LOG(INFO) << "freeing pointer " << d.pointer();
    Grappa_free( d );

    LOG(INFO) << "freeing pointer " << b.pointer();
    Grappa_free( b );

    if( Grappa::mycore() == 0 ) {
      BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 0 );
    }
  });
  
  BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 0 );
//  LOG(INFO) << *global_allocator;
  
  LOG(INFO) << "done!";
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv),
              local_size_bytes);

  CHECK_EQ(Grappa::cores(), 2);
  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

