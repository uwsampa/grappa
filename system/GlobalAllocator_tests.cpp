
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

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    Grappa::on_all_cores([] {
      GlobalAddress< int8_t > a = Grappa::global_alloc( 1 );
      LOG(INFO) << "got pointer " << a.pointer();

      GlobalAddress< int8_t > b = Grappa::global_alloc( 1 );
      LOG(INFO) << "got pointer " << b.pointer();

      GlobalAddress< int8_t > c = Grappa::global_alloc( 8 );
      LOG(INFO) << "got pointer " << c.pointer();

      GlobalAddress< int8_t > d = Grappa::global_alloc( 1 );
      LOG(INFO) << "got pointer " << d.pointer();

  //    LOG(INFO) << *global_allocator;

      if( Grappa_mynode() == 0 ) {
        BOOST_CHECK_EQUAL( global_allocator->total_bytes(), local_size_bytes );
        BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 1 + 8 + 8 + 1 );
      }

      LOG(INFO) << "freeing pointer " << c.pointer();
      Grappa::global_free( c );

      LOG(INFO) << "freeing pointer " << a.pointer();
      Grappa::global_free( a );

      LOG(INFO) << "freeing pointer " << d.pointer();
      Grappa::global_free( d );

      LOG(INFO) << "freeing pointer " << b.pointer();
      Grappa::global_free( b );

      if( Grappa::mycore() == 0 ) {
        BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 0 );
      }
    });
  
    BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 0 );
  //  LOG(INFO) << *global_allocator;
  
    LOG(INFO) << "done!";
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
